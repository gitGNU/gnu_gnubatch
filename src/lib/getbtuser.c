/* getbtuser.c -- get user permission structure

   Copyright 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "incl_sig.h"
#include "files.h"
#include "btmode.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"

static  char    Filename[] = __FILE__;

#define INC_USERDETS    10

void    uloop_over(void (*)(char *, int_ugid_t), char *);

Btdef   Btuhdr;
static  BtuserRef       userdets_buf;
static  int     Num_userdets, Max_userdets;
static  int     btuf_fid = -1;
int             btu_new_format;
static  time_t  last_mod_time;

static  unsigned  char  igsigs[]= { SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGALRM, SIGUSR1, SIGUSR2 };

#ifdef  UNSAFE_SIGNALS
static  RETSIGTYPE      (*oldsigs[sizeof(igsigs)])(int);
#endif

/* Lock the whole caboodle */

static void  lockit(const int type)
{
        struct  flock   lk;

        lk.l_type = (SHORT) type;
        lk.l_whence = 0;
        lk.l_start = 0;
        lk.l_len = 0;
        lk.l_pid = 0;
        if  (fcntl(btuf_fid, F_SETLKW, &lk) < 0)  {
                print_error($E{Cannot lock user ctrl file});
                exit(E_SETUP);
        }
}

/* Unlock the whole caboodle */

static void  unlockit()
{
        struct  flock   lk;

        lk.l_type = F_UNLCK;
        lk.l_whence = 0;
        lk.l_start = 0;
        lk.l_len = 0;
        lk.l_pid = 0;
        if  (fcntl(btuf_fid, F_SETLKW, &lk) < 0)  {
                print_error($E{Cannot unlock user ctrl file});
                exit(E_SETUP);
        }
}

static void  savesigs(const int saving)
{
        int     cnt;
#ifdef  HAVE_SIGACTION
        sigset_t        nset;
        sigemptyset(&nset);
        for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                sigaddset(&nset, igsigs[cnt]);
        sigprocmask(saving? SIG_BLOCK: SIG_UNBLOCK, &nset, (sigset_t *) 0);
#elif   defined(STRUCT_SIG)
        int     msk = 0;
        for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                msk |= sigmask(igsigs[cnt]);
        if  (saving)
                sigsetmask(sigsetmask(~0) | msk);
        else
                sigsetmask(sigsetmask(~0) & ~msk);
#elif   defined(HAVE_SIGSET)
        if  (saving)
                for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                        sighold(igsigs[cnt]);
        else
                for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                        sigrelse(igsigs[cnt]);
#else
        if  (saving)
                for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                        oldsigs[cnt] = signal((int) igsigs[cnt], SIG_IGN);
        else
                for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
                        signal((int) igsigs[cnt], oldsigs[cnt]);
#endif
}

/* Binary search for user id in sorted list
   Return the index of where it's found (or just past that) which might be off the end. */

static  int  bsearch_btuser(const uid_t uid)
{
        int  first = 0, last = Num_userdets, mid;

        while  (first < last)  {
                BtuserRef  sp;
                mid = (first + last) / 2;
                sp = &userdets_buf[mid];
                if  (sp->btu_user == uid)
                        return  mid;
                if  (sp->btu_user > uid)
                        last = mid;
                else
                        first = mid + 1;
        }
        return  first;
}

void    copy_defs(BtuserRef res, uid_t uid)
{
        res->btu_isvalid = 1;
        res->btu_user = uid;
        res->btu_minp = Btuhdr.btd_minp;
        res->btu_maxp = Btuhdr.btd_maxp;
        res->btu_defp = Btuhdr.btd_defp;
        res->btu_priv = Btuhdr.btd_priv;
        res->btu_maxll = Btuhdr.btd_maxll;
        res->btu_totll = Btuhdr.btd_totll;
        res->btu_spec_ll = Btuhdr.btd_spec_ll;
        res->btu_jflags[0] = Btuhdr.btd_jflags[0];
        res->btu_jflags[1] = Btuhdr.btd_jflags[1];
        res->btu_jflags[2] = Btuhdr.btd_jflags[2];
        res->btu_vflags[0] = Btuhdr.btd_vflags[0];
        res->btu_vflags[1] = Btuhdr.btd_vflags[1];
        res->btu_vflags[2] = Btuhdr.btd_vflags[2];
}

int     issame_defs(CBtuserRef item)
{
        return  item->btu_minp == Btuhdr.btd_minp  &&
                item->btu_maxp == Btuhdr.btd_maxp  &&
                item->btu_defp == Btuhdr.btd_defp  &&
                item->btu_priv == Btuhdr.btd_priv  &&
                item->btu_maxll == Btuhdr.btd_maxll  &&
                item->btu_totll == Btuhdr.btd_totll  &&
                item->btu_spec_ll == Btuhdr.btd_spec_ll  &&
                item->btu_jflags[0] == Btuhdr.btd_jflags[0]  &&
                item->btu_jflags[1] == Btuhdr.btd_jflags[1]  &&
                item->btu_jflags[2] == Btuhdr.btd_jflags[2]  &&
                item->btu_vflags[0] == Btuhdr.btd_vflags[0]  &&
                item->btu_vflags[1] == Btuhdr.btd_vflags[1]  &&
                item->btu_vflags[2] == Btuhdr.btd_vflags[2];
}

/* Read user from file or memory.
   File is assumed to be locked. */

void    readu(const int_ugid_t uid, BtuserRef item)
{
        int     whu;
        BtuserRef       sp;

        if  (!btu_new_format)  {

                /* If it's below the magic number at which we store them as a
                   vector, jump to the right place and go home.  */

                if  ((ULONG) uid < SMAXUID)  {
                        lseek(btuf_fid, (long)(sizeof(Btdef) + uid * sizeof(Btuser)), 0);
                        if  (read(btuf_fid, (char *) item, sizeof(Btuser)) == sizeof(Btuser)  &&  item->btu_isvalid)
                                copy_defs(item, uid);
                        return;
                }
        }

        /* For new format files and for excess over SMAXUID of old format, we save them in the vector */


        whu = bsearch_btuser(uid);
        sp = &userdets_buf[whu];
        if  (whu < Num_userdets  &&  sp->btu_user == uid)  {
                *item = *sp;
                return;
        }

        /* Otherwise copy defaults */

        copy_defs(item, uid);
}
static  void    insert_item_vec(CBtuserRef item, const int whu)
{
        BtuserRef  sp = &userdets_buf[whu];
        int     cnt;

        if  (Num_userdets >= Max_userdets)  {
                Max_userdets += INC_USERDETS;
                if  (userdets_buf)
                        userdets_buf = (BtuserRef) realloc((char *) userdets_buf, Max_userdets * sizeof(Btuser));
                else
                        userdets_buf = (BtuserRef) malloc(Max_userdets * sizeof(Btuser));
                if  (!userdets_buf)
                        ABORT_NOMEM;
                sp = &userdets_buf[whu];
        }
        for  (cnt = Num_userdets;  cnt > whu;  cnt--)
                userdets_buf[cnt] = userdets_buf[cnt-1];
        Num_userdets++;
        *sp = *item;
}

void  insertu(CBtuserRef item)
{
        Btuser  force;

        /* Force on all permissions if root or batch user */

        if  (item->btu_user == ROOTID || item->btu_user == Daemuid)  {
                force = *item;
                force.btu_priv = ALLPRIVS;
                item = &force;
        }

        if  (btu_new_format)  {
#ifndef HAVE_FTRUNCATE
                char    *fname;
#endif
                int  whu = bsearch_btuser(item->btu_user);
                BtuserRef  sp = &userdets_buf[whu];

                if  (whu >= Num_userdets || sp->btu_user != item->btu_user)  {
                        /* We haven't met this user before.
                           If it's the same as the default, forget it. */
                        if  (issame_defs(item))
                                return;
                        insert_item_vec(item, whu);
                }
                else  {
                        *sp = *item;

                        /* If it's the same as the default, then we want to delete the user */

                        if  (issame_defs(sp))  {
                                int  cnt;
                                for  (cnt = whu+1;  cnt < Num_userdets;  cnt++)
                                        userdets_buf[cnt-1] = userdets_buf[cnt];
                                Num_userdets--;
                        }
                }

                /* And now write the thing out */

#ifdef  HAVE_FTRUNCATE
                Ignored_error = ftruncate(btuf_fid, 0L);
                lseek(btuf_fid, 0L, 0);
#else
                close(btuf_fid);
                fname = envprocess(BTUFILE);

                btuf_fid = open(fname, O_RDWR|O_TRUNC);
                free(fname);
                fcntl(btuf_fid, F_SETFD, 1);
                lockit(F_WRLCK);
#endif

                Ignored_error = write(btuf_fid, (char *) &Btuhdr, sizeof(Btuhdr));
                if  (Num_userdets != 0)
                        Ignored_error = write(btuf_fid, (char *) userdets_buf, Num_userdets * sizeof(Btuser));
        }
        else  {                                 /* Old-style format */

                /* If it's below maximum for vector, stuff it in.  */

                if  ((ULONG) item->btu_user < SMAXUID)  {
                        lseek(btuf_fid, (long) (sizeof(Btdef) + item->btu_user * sizeof(Btuser)), 0);
                        Ignored_error = write(btuf_fid, (char *) item, sizeof(Btuser));
                }
                else  {
                        /* We now hold details for users >SMAXUID in the in-memory vector used for new fmt
                           We won't worry for now about items duplicating the default as that will happen
                           when the whole file is written out in the new format. */

                        int  whu = bsearch_btuser(item->btu_user);
                        BtuserRef  sp = &userdets_buf[whu];

                        if  (whu >= Num_userdets || sp->btu_user != item->btu_user)
                                insert_item_vec(item, whu); /* Not found insert into vector */
                        else
                                *sp = *item;

                        /* Write the new vector to the file */

                        lseek(btuf_fid, (long) (sizeof(Btdef) + sizeof(Btuser) * SMAXUID), 0);
                        Ignored_error = write(btuf_fid, (char *) userdets_buf, Num_userdets * sizeof(Btuser));
                }
        }

        last_mod_time = time(0);
}

/* Create user control file from scratch.  Return 0 - failure, 1 - ok */

static int  init_file(char *fname)
{
        int             fid;
        Btuser          Spec;

        if  ((fid = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
                return  0;

#if     defined(HAVE_FCHOWN) && !defined(M88000)
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(fid, (uid_t) Daemuid, getegid());
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(fname, (uid_t) Daemuid, getegid());
#endif
        savesigs(1);

        Btuhdr.btd_version = GNU_BATCH_MAJOR_VERSION;
        Btuhdr.btd_lastp = 0;                   /* Set time zero to signify new format */
        Btuhdr.btd_minp = U_DF_MINP;
        Btuhdr.btd_maxp = U_DF_MAXP;
        Btuhdr.btd_defp = U_DF_DEFP;
        Btuhdr.btd_maxll = U_DF_MAXLL;
        Btuhdr.btd_totll = U_DF_TOTLL;
        Btuhdr.btd_spec_ll = U_DF_SPECLL;
        Btuhdr.btd_priv = U_DF_PRIV;
        Btuhdr.btd_jflags[0] = U_DF_UJ;
        Btuhdr.btd_jflags[1] = U_DF_GJ;
        Btuhdr.btd_jflags[2] = U_DF_OJ;
        Btuhdr.btd_vflags[0] = U_DF_UV;
        Btuhdr.btd_vflags[1] = U_DF_GV;
        Btuhdr.btd_vflags[2] = U_DF_OV;
        Ignored_error = write(fid, (char *) &Btuhdr, sizeof(Btuhdr));

        /* Initialise root and Daemuid to have all privs */

        Spec.btu_isvalid = 1;
        Spec.btu_minp = U_DF_MINP;
        Spec.btu_maxp = U_DF_MAXP;
        Spec.btu_defp = U_DF_DEFP;
        Spec.btu_maxll = U_DF_MAXLL;
        Spec.btu_totll = U_DF_TOTLL;
        Spec.btu_spec_ll = U_DF_SPECLL;
        Spec.btu_priv = ALLPRIVS;
        Spec.btu_jflags[0] = U_DF_UJ;
        Spec.btu_jflags[1] = U_DF_GJ;
        Spec.btu_jflags[2] = U_DF_OJ;
        Spec.btu_vflags[0] = U_DF_UV;
        Spec.btu_vflags[1] = U_DF_GV;
        Spec.btu_vflags[2] = U_DF_OV;
        Spec.btu_user = ROOTID;
        Ignored_error = write(fid, (char *) &Spec, sizeof(Spec));
        if  (Daemuid != ROOTID)  {
                Spec.btu_user = Daemuid;
                Ignored_error = write(fid, (char *) &Spec, sizeof(Spec));
        }
        close(fid);
        savesigs(0);
        return  1;
}

/* Open user file and return file descriptor in btuf_fid
   Cope with new and old formats. New format has date = 0 */

static void  open_file(int mode)
{
        char    *fname = envprocess(BTUFILE);
        struct  stat    fbuf;

        if  ((btuf_fid = open(fname, mode)) < 0)  {
                if  (errno == EACCES)  {
                        print_error($E{Check file setup});
                        exit(E_SETUP);
                }
                if  (errno == ENOENT)
                        init_file(fname);
                btuf_fid = open(fname, mode);
        }
        free(fname);

        if  (btuf_fid < 0)  {
                print_error($E{Cannot create user control file});
                exit(E_SETUP);
        }

        lockit(F_RDLCK);
        fstat(btuf_fid, &fbuf);
        last_mod_time = fbuf.st_mtime;
        fcntl(btuf_fid, F_SETFD, 1);

        if  (read(btuf_fid, (char *)&Btuhdr, sizeof(Btuhdr)) != sizeof(Btuhdr))  {
                close(btuf_fid);
                btuf_fid = -1;
                print_error($E{Cannot create user control file});
                exit(E_SETUP);
        }

        /* Check version number and print warning message if funny.  */

        if  (Btuhdr.btd_version != GNU_BATCH_MAJOR_VERSION  ||  (fbuf.st_size - sizeof(Btdef)) % sizeof(Btuser) != 0)  {
                disp_arg[0] = Btuhdr.btd_version;
                disp_arg[1] = GNU_BATCH_MAJOR_VERSION;
                print_error($E{Wrong version of product});
        }

        Num_userdets = (fbuf.st_size - sizeof(Btdef)) / sizeof(Btuser);

        /* We signify the new format (default + exceptions) by having password time = 0 */

        if  (Btuhdr.btd_lastp == 0)
                btu_new_format = 1;
        else  {
                struct  stat    pwbuf;
                if  (stat("/etc/passwd", &pwbuf) < 0)  {
                        close(btuf_fid);
                        btuf_fid = -1;
                        return;
                }
                if  (Btuhdr.btd_lastp >= pwbuf.st_mtime)  {
                        if  (Btuhdr.btd_lastp > pwbuf.st_mtime)
                                print_error($E{Funny times passwd file});
                }

                /* Number of users is reduced by the ones saved as a vector */

                Num_userdets -= SMAXUID;
                if  (Num_userdets > 0)
                        lseek(btuf_fid, (long) (sizeof(Btdef) + SMAXUID * sizeof(Btuser)), 0);
                btu_new_format = 0;
        }

        if  (Num_userdets > 0)  {
                unsigned  sizeb = Num_userdets * sizeof(Btuser);
                if  (!(userdets_buf = (BtuserRef) malloc(sizeb)))
                        ABORT_NOMEM;
                if  (read(btuf_fid, (char *) userdets_buf, sizeb) != (int) sizeb)  {
                        print_error($E{Cannot open user control file});
                        exit(E_SETUP);
                }
        }

        Max_userdets = Num_userdets;
        unlockit();
}

static  void    close_file()
{
        if  (userdets_buf)  {
                free((char *) userdets_buf);
                userdets_buf = 0;
        }
        close(btuf_fid);
        btuf_fid = -1;
}

/* Get info about specific user.  If we haven't met the guy before return null.  */

static BtuserRef  gpriv(const int_ugid_t uid)
{
        static  Btuser  result;
        struct  stat  ufst;

        lockit(F_RDLCK);
        fstat(btuf_fid, &ufst);

        /* If file has changed since we read stuff in, close and reopen.
           This is assumed not to happen very often.
           Probably the only thing it will happen with is the API or if
           2 or more people are editing the user file at the same time */

        if  (ufst.st_mtime != last_mod_time)  {
                close_file();                   /* Kills the lock */
                open_file(O_RDWR);              /* Probably only done for edit-type cases */
                lockit(F_RDLCK);
        }
        readu(uid, &result);
        unlockit();
        return  &result;
}

/* Routine to access privilege/mode file.  This is now the basic
   routine for user programs and does not return if there's a problem.  */

BtuserRef  getbtuser(const uid_t uid)
{
        BtuserRef       result;

        open_file(O_RDONLY);
        result = gpriv(uid);
        close_file();
        return  result;
}

/* Get entry in user file, possibly for update Only done for utility routines.  */

BtuserRef  getbtuentry(const uid_t uid)
{
        if  (btuf_fid < 0)
                open_file(O_RDWR);
        return  gpriv(uid);
}

/* Update details for given user only.  */

void  putbtuentry(BtuserRef item)
{
        lockit(F_WRLCK);
        insertu(item);
        unlockit();
}

/* This routine is used by getbtulist via uloop_over */

static void gu(char *arg, int_ugid_t uid)
{
        BtuserRef  *rp = (BtuserRef *) arg;
        readu(uid, *rp);
        ++*rp;                                  /* pointer to rbuf - advance to next item */
}

static  int  sort_id(BtuserRef a, BtuserRef b)
{
        return  (ULONG) a->btu_user < (ULONG) b->btu_user ? -1: (ULONG) a->btu_user == (ULONG) b->btu_user? 0: 1;
}

BtuserRef  getbtulist()
{
        BtuserRef       result, rbuf;

        /* If we haven't got list of users yet, better get it */

        if  (Npwusers == 0)
                rpwfile();

        if  (btuf_fid < 0)  {
                open_file(O_RDWR);
                lockit(F_RDLCK);
        }
        else  {
                /* Check it hasn't changed */
                struct  stat  ufst;
                lockit(F_RDLCK);
                fstat(btuf_fid, &ufst);
                if  (ufst.st_mtime != last_mod_time)  {
                        close_file();
                        open_file(O_RDWR);
                        lockit(F_RDLCK);
                }
        }

        result = (BtuserRef) malloc(Npwusers * sizeof(Btuser));
        if  (!result)
                ABORT_NOMEM;
        rbuf = result;
        uloop_over(gu, (char *) &rbuf);
        unlockit();
        qsort(QSORTP1 result, Npwusers, sizeof(Btuser), QSORTP4 sort_id);
        return  result;
}

void  putbtuhdr()
{
        lockit(F_WRLCK);
        lseek(btuf_fid, 0L, 0);
        Ignored_error = write(btuf_fid, (char *) &Btuhdr, sizeof(Btuhdr));
        unlockit();
}

/* Save list. This always rewrites the header and
   probably is the last thing to be called before we quit */

void  putbtulist(BtuserRef list)
{
        BtuserRef  sp, ep;
#ifndef HAVE_FTRUNCATE
        char    *fname;
#endif

        lockit(F_WRLCK);
#ifdef  HAVE_FTRUNCATE
        Ignored_error = ftruncate(btuf_fid, 0L);
        lseek(btuf_fid, 0L, 0);
#else
        close(btuf_fid);
        fname = envprocess(BTUFILE);
        btuf_fid = open(fname, O_RDWR|O_TRUNC);
        free(fname);
        fcntl(btuf_fid, F_SETFD, 1);
        lockit(F_WRLCK);
#endif
        Btuhdr.btd_lastp = 0;                   /* Force new format */
        Ignored_error = write(btuf_fid, (char *) &Btuhdr, sizeof(Btuhdr));

        ep = &list[Npwusers];
        for  (sp = list;  sp < ep;  sp++)  {
                if  (sp->btu_user == ROOTID || sp->btu_user == Daemuid)
                        sp->btu_priv = ALLPRIVS;
                if  (issame_defs(sp))
                        continue;
                Ignored_error = write(btuf_fid, (char *) sp, sizeof(Btuser));
        }
        unlockit();
}
