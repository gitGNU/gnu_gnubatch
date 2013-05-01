/* sh_vlist.c -- scheduler variable handling

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <sys/ipc.h>
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "statenums.h"
#include "errnums.h"
#include "files.h"
#include "ecodes.h"
#include "sh_ext.h"

/* Pointer right into shared memory segment for the value of Current
   load level system variable.  */

BtvarRef        Clsvp;

static  char    vfilename[] = VFILE;
static  int     vfilefd;

slotno_t        machname_slot;  /* Slot number of "machine name" variable */

#define visible(sr, md)         shmpermitted(sr, md, BTM_SHOW)

static vhash_t  addhash(BtvarRef);
extern char *look_hostid(const netid_t);

#ifndef USING_FLOCK
static  struct  sembuf
vw[2] = {{      VQ_READING,     0,      0       },      /* Write lock */
        {       VQ_FIDDLE,      -1,     0       }},
cvw[2] ={{      VQ_READING,     0,      IPC_NOWAIT      },      /* Cond write lock */
        {       VQ_FIDDLE,      -1,     IPC_NOWAIT      }},
vuw[1] ={{      VQ_FIDDLE,      1,      0       }};
#endif

void  lockvars()
{
#ifdef  USING_FLOCK
        struct  flock   lck;
        lck.l_type = F_WRLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Var_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Var_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        panic($E{Lock error vars});
        }
#else
        for  (;;)  {
                if  (semop(Sem_chan, vw, 2) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                panic($E{Semaphore error probably undo});
        }
#endif
}

void  unlockvars()
{
#ifdef  USING_FLOCK
        struct  flock   lck;
#endif
#ifdef  USING_MMAP
        msync(Var_seg.inf.seg, Var_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
#ifdef  USING_FLOCK
        lck.l_type = F_UNLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Var_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Var_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        panic($E{Unlock error});
        }
#else
        for  (;;)  {
                if  (semop(Sem_chan, vuw, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                panic($E{Semaphore error probably undo});
        }
#endif
}

int  clockvars()
{
        PIDTYPE cpid;
#ifdef  USING_FLOCK
        struct  flock   lck;
        lck.l_type = F_WRLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        do  {
#ifdef  USING_MMAP
                if  (fcntl(Var_seg.inf.mmfd, F_SETLK, &lck) >= 0)
                        return  CLOCK_OK;
#else
                if  (fcntl(Var_seg.inf.lockfd, F_SETLK, &lck) >= 0)
                        return  CLOCK_OK;
#endif
        }  while  (errno == EINTR);
        if  (errno != EAGAIN  &&  errno != EACCES)
                panic($E{Lock error vars});
#else
        if  (semop(Sem_chan, cvw, 2) >= 0)
                return  CLOCK_OK;
#endif
        cpid = forksafe();
        if  (cpid < 0)
                return  CLOCK_NOFORK;
        if  (cpid != 0)
                return  CLOCK_MADECHILD;

        /* Wait in the child process */

#ifdef  USING_FLOCK
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Var_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
                        return  CLOCK_AMCHILD;
#else
                if  (fcntl(Var_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
                        return  CLOCK_AMCHILD;
#endif
                if  (errno != EINTR)
                        panic($E{Lock error vars});
        }
#else
        lockvars();
        return  CLOCK_AMCHILD;
#endif
}

/* Initialise standard names */

static  slotno_t  initsvar(char *defname, char *defcomment, int code, unsigned type, unsigned flags, BtmodeRef mp, BtconRef cp)
{
        char    *p;
        struct  Ventry  *fp;
        int     nn;
        vhash_t hp;
        Btvar   sv;

        /* Change 18/04/94 - run this even if we have had it before.  */

        for  (nn = 0;  nn < VAR_HASHMOD;  nn++)  {
                for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = fp->Vnext)  {
                        fp = &Var_seg.vlist[hp];
                        if  (fp->Vent.var_type == type)
                                return  hp;
                }
        }

        /* If we can read default system variable names from the file
           do so, otherwise use that given in the argument.  */

        if  ((p = helpprmpt(code)))  {
                strncpy(sv.var_name, p, BTV_NAME);
                free(p);
        }
        else
                strncpy(sv.var_name, defname, BTV_NAME);

        /* Make sure we've got a null terminator */

        sv.var_name[BTV_NAME] = '\0';

        /* Check for clashes with other system variables */

        for  (hp = Var_seg.vhash[calchash(sv.var_name)];  hp >= 0;  hp = fp->Vnext)  {
                fp = &Var_seg.vlist[hp];
                if  (strcmp(fp->Vent.var_name, sv.var_name) == 0)  {
                        disp_str = sv.var_name;
                        print_error($E{Panic clash with system variables});
                        do_exit(E_VARL);
                }
        }

        sv.var_sequence = 1;
        sv.var_c_time = time(&sv.var_m_time);
        sv.var_type = (unsigned char) type;
        sv.var_flags = (unsigned char) flags;
        sv.var_id.hostid = 0;
        if  ((p = helpprmpt(code+1)))  {
                strncpy(sv.var_comment, p, BTV_COMMENT);
                free(p);
        }
        else
                strncpy(sv.var_comment, defcomment, BTV_COMMENT);

        sv.var_mode = *mp;
        sv.var_value = *cp;
        if  ((hp = addhash(&sv)) < 0)  {
                disp_str = sv.var_name;
                print_error($E{Panic trouble hashing system var});
                do_exit(E_VARL);
        }
        return  hp;
}

static void  initsvs(const LONG initll)
{
        vhash_t hp;
        Btcon   con;
        Btmode  mod;

        BLOCK_ZERO(&mod, sizeof(mod));
        initumode(Daemuid? Daemuid: ROOTID, &mod);
        if  (Daemuid != ROOTID)
                mod.c_uid = mod.o_uid = Daemuid;
        else
                mod.c_uid = mod.o_uid = ROOTID;
        mod.o_gid = mod.c_gid = Daemgid;
        strncpy(mod.o_user, prin_uname((uid_t) mod.o_uid), UIDSIZE);
        strncpy(mod.c_user, mod.o_user, UIDSIZE);
        strncpy(mod.o_group, prin_gname((gid_t) mod.o_gid), UIDSIZE);
        strncpy(mod.c_group, mod.o_group, UIDSIZE);

        con.const_type = CON_LONG;
        con.con_un.con_long = SYSDF_MAXLL;
        hp = initsvar("LOADLEVEL", "Max", $P{Sysvarname load level}, VT_LOADLEVEL, VF_LONGONLY, &mod, &con);
        if  (initll >= 0)
                Var_seg.vlist[hp].Vent.var_value.con_un.con_long = initll;
        Max_ll = Var_seg.vlist[hp].Vent.var_value.con_un.con_long;

        con.con_un.con_long = 0L;
        initsvar("CLOAD", "Curr", $P{Sysvarname cload}, VT_CURRLOAD, VF_LONGONLY|VF_READONLY, &mod, &con);

        con.con_un.con_long = INIT_STARTLIM;
        hp = initsvar("STARTLIM", "Limit", $P{Sysvarname startlim}, VT_STARTLIM, VF_LONGONLY, &mod, &con);
        Startlim = (unsigned) Var_seg.vlist[hp].Vent.var_value.con_un.con_long;

        con.con_un.con_long = INIT_STARTWAIT;
        hp = initsvar("STARTWAIT", "Wait time", $P{Sysvarname startwait}, VT_STARTWAIT, VF_LONGONLY, &mod, &con);
        Startwait = (unsigned) Var_seg.vlist[hp].Vent.var_value.con_un.con_long;

        con.const_type = CON_STRING;
        con.con_un.con_string[0] = '\0';

        initsvar("LOGJOBS", "Job Log", $P{Sysvarname log jobs}, VT_LOGJOBS, VF_STRINGONLY, &mod, &con);
        initsvar("LOGVARS", "Var Log", $P{Sysvarname log vars}, VT_LOGVARS, VF_STRINGONLY, &mod, &con);
        if  (Network_ok)
                initsvmachine();
}

void  initsvmachine()
{
        int     nn;
        Btcon   con;
        Btmode  mod;

        /* We search for it by type as we may have had it before and
           called it something else.  Do this every time as we
           want to be sure that we get the current machine name
           (in case it has changed) not some fossil one.  */

        for  (nn = 0;  nn < VAR_HASHMOD;  nn++)  {
                vhash_t hp;
                struct  Ventry  *fp;
                for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = fp->Vnext)  {
                        fp = &Var_seg.vlist[hp];
                        if  (fp->Vent.var_type == VT_MACHNAME)  {
                                fp->Vent.var_value.con_un.con_string[BTC_VALUE] = '\0';
                                strncpy(fp->Vent.var_value.con_un.con_string, get_myhostname(), BTC_VALUE);
                                machname_slot = hp;
                                return;
                        }
                }
        }

        /* Ok create it from scratch then */

        BLOCK_ZERO(&mod, sizeof(mod));
        initumode(Daemuid? Daemuid: ROOTID, &mod);
        if  (Daemuid != ROOTID)
                mod.c_uid = mod.o_uid = Daemuid;
        else
                mod.c_uid = mod.o_uid = ROOTID;
        mod.o_gid = mod.c_gid = Daemgid;
        strncpy(mod.o_user, prin_uname((uid_t) mod.o_uid), UIDSIZE);
        strncpy(mod.c_user, mod.o_user, UIDSIZE);
        strncpy(mod.o_group, prin_gname((gid_t) mod.o_gid), UIDSIZE);
        strncpy(mod.c_group, mod.o_group, UIDSIZE);
        mod.u_flags |= BTM_READ|BTM_SHOW;
        mod.g_flags |= BTM_READ|BTM_SHOW;
        mod.o_flags |= BTM_READ|BTM_SHOW;
        con.const_type = CON_STRING;
        con.con_un.con_string[BTC_VALUE] = '\0';
        strncpy(con.con_un.con_string, get_myhostname(), BTC_VALUE);
        machname_slot = initsvar("MACHINE", "Host name", $P{Sysvarname machine}, VT_MACHNAME, VF_STRINGONLY, &mod, &con);
}

/* Set up pointer to `Current_load' */

static void  setup_cvptr()
{
        int     nn;
        vhash_t hp;
        struct  Ventry  *fp;

        for  (nn = 0;  nn < VAR_HASHMOD;  nn++)  {
                for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = fp->Vnext)  {
                        fp = &Var_seg.vlist[hp];
                        if  (fp->Vent.var_type == VT_CURRLOAD  &&  fp->Vent.var_id.hostid == 0)  {
                                Clsvp = &fp->Vent;
                                Clsvp->var_value.con_un.con_long = Current_ll;
                                return;
                        }
                }
        }
        Clsvp = (BtvarRef) 0;
}

/* Call this when we are setting a system variable */

static void  setsysval(BtvarRef vp)
{
        switch  (vp->var_type)  {
        case  VT_LOADLEVEL:
                Max_ll = vp->var_value.con_un.con_long;
                break;
        case  VT_LOGJOBS:
                openjlog(vp->var_value.con_un.con_string, &vp->var_mode);
                break;
        case  VT_LOGVARS:
                openvlog(vp->var_value.con_un.con_string, &vp->var_mode);
                break;
        case  VT_STARTLIM:
                Startlim = (unsigned) vp->var_value.con_un.con_long;
                break;
        case  VT_STARTWAIT:
                Startwait = (unsigned) vp->var_value.con_un.con_long;
                break;
        }
}

/* Create/open var file. Read it into memory if it exists.  This used
   to be called openvfile but lint choked over the fact that
   there is a library routine of that name.  */

void  creatvfile(LONG vsize, const LONG initll)
{
        int     Maxvars, i;
        vhash_t prevp;
        struct  stat    sbuf;
        Btvar   inv;

#ifndef USING_MMAP
        Var_seg.inf.base = SHMID + VSHMOFF + envselect_value;   /* Key */
#endif

        if  ((vfilefd = open(vfilename, O_RDWR|O_CREAT, CMODE)) < 0)  {
                print_error($E{Panic cannot create var file});
                do_exit(E_VARL);
        }

#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(vfilefd, Daemuid, Daemgid);
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(vfilename, Daemuid, Daemgid);
#endif
        fcntl(vfilefd, F_SETFD, 1);

        if  (vsize < INITNVARS)
                vsize = INITNVARS;

        /* Initialise shared memory segment to size of file, rounding up. */

#ifndef USING_MMAP
#ifdef  USING_FLOCK
        if  ((Var_seg.inf.lockfd = open(VLOCK_FILE, O_CREAT|O_TRUNC|O_RDWR, 0600)) < 0)
                goto  fail;
#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(Var_seg.inf.lockfd, Daemuid, Daemgid);
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(VLOCK_FILE, Daemuid, Daemgid);
#endif

        fcntl(Var_seg.inf.lockfd, F_SETFD, 1);
#endif
        lockvars();
#endif
        fstat(vfilefd, &sbuf);
        Maxvars = sbuf.st_size / sizeof(Btvar);
        if  (Maxvars < vsize)
                Maxvars = vsize;

        Var_seg.inf.reqsize = Maxvars * sizeof(struct Ventry) + 2 * sizeof(vhash_t) * VAR_HASHMOD + sizeof(struct vshm_hdr);
        if  (!gshmchan(&Var_seg.inf, VSHMOFF))  {
#if !defined(USING_MMAP) && defined(USING_FLOCK)
        fail:
#endif
                print_error($E{Panic trouble with var shm});
                do_exit(E_VARL);
        }

#ifdef  USING_MMAP
        lockvars();
#endif

        /* If kernel gave us more than we asked for, adjust Maxvars accordingly.  */

        Maxvars = (Var_seg.inf.segsize - sizeof(struct vshm_hdr) - 2 * sizeof(vhash_t) * VAR_HASHMOD) / sizeof(struct Ventry);

        Var_seg.dptr = (struct vshm_hdr *) Var_seg.inf.seg;
        Var_seg.vhash = (vhash_t *) (Var_seg.inf.seg + sizeof(struct vshm_hdr));
        Var_seg.vidhash = (vhash_t *) ((char *) Var_seg.vhash + VAR_HASHMOD * sizeof(vhash_t));
        Var_seg.vlist = (struct Ventry *) ((char *) Var_seg.vidhash + VAR_HASHMOD * sizeof(vhash_t));

        /* Initialise hash tables.  */

        for  (i = 0;  i < VAR_HASHMOD;  i++)
                Var_seg.vhash[i] = Var_seg.vidhash[i] = -1L;

        /* Initialise free chain */

        prevp = -1L;
        for  (i = Maxvars - 1;  i >= 0;  i--)  {
                Var_seg.vlist[i].Vnext = prevp;
                Var_seg.vlist[i].Vused = 0;
                prevp = i;
        }
        Var_seg.dptr->vs_freech = prevp;

        /* Read in var file and add to hash */

        while  (read(vfilefd, (char *) &inv, sizeof(inv)) == sizeof(inv))  {
                if  (inv.var_type)
                        setsysval(&inv);
                if  (!Network_ok)
                        inv.var_flags &= ~(VF_EXPORT|VF_CLUSTER);
                addhash(&inv);
        }

        /* Set up header structure.  */

        Var_seg.dptr->vs_type = TY_ISVAR;
        Var_seg.dptr->vs_info = 0;
        Var_seg.dptr->vs_nxtid = 0;
        Var_seg.dptr->vs_maxvars = Maxvars;
        Var_seg.dptr->vs_serial = 1;
        Var_seg.dptr->vs_lockid = 0;

        /* Initialise system variables.
           Make sure it gets written out soon. */

        initsvs(initll);
        if  (sbuf.st_size == 0)
                rewrvf();
        else
                Var_seg.dptr->vs_lastwrite = sbuf.st_mtime;

        /* Set up pointer to current load value */

        setup_cvptr();
        unlockvars();
}

/* Rewrite var file.
   If the number of vars has shrunk since the last
   time, squash it up (by recreating the file, as vanilla UNIX
   doesn't have a "truncate file" syscall [why not?]).  */

void  rewrvf()
{
        int     hadvars, countvars;
#ifndef HAVE_FTRUNCATE
        int     fvars;
#endif
        int     hn;
        vhash_t hp;
        struct  flock   wlock;

        countvars = 0;
        for  (hn = 0;  hn < VAR_HASHMOD;  hn++)
                for  (hp = Var_seg.vhash[hn];  hp >= 0;  hp = Var_seg.vlist[hp].Vnext)
                        if  (Var_seg.vlist[hp].Vent.var_id.hostid == 0)
                                countvars++;

        wlock.l_type = F_WRLCK;
        wlock.l_whence = 0;
        wlock.l_start = 0L;
        wlock.l_len = 0L;

#ifdef  HAVE_FTRUNCATE
        Ignored_error = ftruncate(vfilefd, 0L);
        lseek(vfilefd, 0L, 0);
        while  (fcntl(vfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        panic($E{Panic couldnt lock var file});
        }
#else
        fvars = lseek(vfilefd, 0L, 2) / sizeof(Btvar);
        if  (countvars < fvars)  {
                close(vfilefd);
                unlink(vfilename);
                if  ((vfilefd = open(vfilename, O_RDWR|O_CREAT, CMODE)) < 0)
                        panic($E{Panic cannot create var file});
                fcntl(vfilefd, F_SETFD, 1);
#ifdef  HAVE_FCHOWN
                if  (Daemuid != ROOTID)
                        Ignored_error = fchown(vfilefd, Daemuid, Daemgid);
#else
                if  (Daemuid != ROOTID)
                        Ignored_error = chown(vfilename, Daemuid, Daemgid);
#endif
        }
        else
                lseek(vfilefd, 0L, 0);
        /* Note that there is a race if there's no ftruncate */
        while  (fcntl(vfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        panic($E{Panic couldnt lock var file});
        }
#endif /* !HAVE_FTRUNCATE */

        hadvars = 0;

        for  (hn = 0;  hn < VAR_HASHMOD;  hn++)  {
                for  (hp = Var_seg.vhash[hn];  hp >= 0;  hp = Var_seg.vlist[hp].Vnext)
                        if  (Var_seg.vlist[hp].Vent.var_id.hostid == 0)  {
                                hadvars++;
                                Ignored_error = write(vfilefd, (char *) &Var_seg.vlist[hp].Vent, sizeof(Btvar));
                        }
        }

        time(&Var_seg.dptr->vs_lastwrite);

        /* Check everything ties up */

        if  (hadvars != countvars)  {
                disp_arg[9] = hadvars;
                disp_arg[8] = countvars;
                panic($E{Panic lost count of vars});
        }

        wlock.l_type = F_UNLCK;
        while  (fcntl(vfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        panic($E{Panic couldnt lock var file});
        }
}

/* Attempt to grow (by allocating a new) variable segment.
   Return -1 if the world is too small.

   Reallocate var shared segment if it grows too big.
   We do this by allocating a new shared memory segment,
   and planting where we've gone in the old one.  */

int  growvseg()
{
        char            *newseg;
        unsigned        Oldmaxv = Var_seg.dptr->vs_maxvars;
        unsigned        Maxvars = Oldmaxv + INCNVARS;
        unsigned        Nvars = Var_seg.dptr->vs_nvars;
        struct  vshm_hdr*newdptr;
        vhash_t         *newhash, *newidhash;
        struct  Ventry  *newlist;
        int             vind;
        vhash_t         prevp;

#ifdef  USING_MMAP
        Var_seg.inf.reqsize = Maxvars * sizeof(struct Ventry) + 2 * sizeof(vhash_t) * VAR_HASHMOD + sizeof(struct vshm_hdr);
        gshmchan(&Var_seg.inf, VSHMOFF); /* Panics if it can't grow */
        Maxvars = (Var_seg.inf.segsize - sizeof(struct vshm_hdr) - 2 * sizeof(vhash_t) * VAR_HASHMOD) / sizeof(struct Ventry);

        /* We still have the same physical file so we don't need to copy.
           However it might have wound up at a different virt address */
#else
        struct  btshm_info      new_info;
        new_info = Var_seg.inf;

        new_info.reqsize = Maxvars * sizeof(struct Ventry) + 2 * sizeof(vhash_t) * VAR_HASHMOD + sizeof(struct vshm_hdr);
        if  (!gshmchan(&new_info, VSHMOFF))  {
                shmctl(new_info.chan, IPC_RMID, (struct shmid_ds *) 0);
                return  -1;
        }
        Maxvars = (new_info.segsize - sizeof(struct vshm_hdr) - 2 * sizeof(vhash_t) * VAR_HASHMOD) / sizeof(struct Ventry);
        if  (Maxvars <= Oldmaxv)  {
                shmctl(new_info.chan, IPC_RMID, (struct shmid_ds *) 0);
                return  -1;
        }

        /* Save pointer to the new segment */

        Var_seg.dptr->vs_nxtid = Var_seg.inf.base;
#endif

        newseg = Var_seg.inf.seg;
        newdptr = (struct vshm_hdr *) newseg;
        newhash = (vhash_t *) (newseg + sizeof(struct vshm_hdr));
        newidhash = (vhash_t *) ((char *) newhash + VAR_HASHMOD * sizeof(vhash_t));
        newlist = (struct Ventry *) ((char *) newidhash + VAR_HASHMOD * sizeof(vhash_t));

#ifdef  USING_MMAP
        newdptr->vs_serial++;
#else
        newdptr->vs_info = Var_seg.dptr->vs_info;
        newdptr->vs_lastwrite = Var_seg.dptr->vs_lastwrite;
        newdptr->vs_serial = Var_seg.dptr->vs_serial + 1;
        newdptr->vs_lockid = Var_seg.dptr->vs_lockid; /* Hmmm child process might be looking at it... */
        newdptr->vs_type = TY_ISVAR;
        newdptr->vs_nxtid = 0;

        /* Copy over stuff from old segment.
           Use a block copy to preserve position.  */

        BLOCK_COPY(newhash, Var_seg.vhash, sizeof(vhash_t) * VAR_HASHMOD);
        BLOCK_COPY(newidhash, Var_seg.vidhash, sizeof(vhash_t) * VAR_HASHMOD);
        BLOCK_COPY(newlist, Var_seg.vlist, sizeof(struct Ventry) * Oldmaxv);

        /* Detach and remove old segment.  */

        shmdt(Var_seg.inf.seg);
        shmctl(Var_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
        Var_seg.inf = new_info;
#endif
        prevp = Var_seg.dptr->vs_freech;        /* Should be -1 but do it properly */

        Var_seg.dptr = newdptr;
        Var_seg.vhash = newhash;
        Var_seg.vidhash = newidhash;
        Var_seg.vlist = newlist;
        Var_seg.dptr->vs_nvars = Nvars;
        Var_seg.dptr->vs_maxvars = Maxvars;

        /* Link all the new ones together on the free chain.
           We remembered the old free chain pointer before we detached the old segment.
           This should be -1 but this code is `carefully written so it won't break if it's something else' (HTB).  */

        for  (vind = Maxvars - 1;  vind >= Oldmaxv;  vind--)  {
                Var_seg.vlist[vind].Vnext = prevp;
                Var_seg.vlist[vind].Vused = 0;
                prevp = vind;
        }
        Var_seg.dptr->vs_freech = prevp;
        setup_cvptr();
#if     defined(USING_MMAP) && defined(MS_ASYNC)
        msync(newseg, Var_seg.inf.segsize, MS_ASYNC|MS_INVALIDATE);
#endif
        tellchild(O_VREMAP, 0L);
        if  (Netm_pid)
                kill(Netm_pid, NETSHUTSIG);
        qchanges++;
        vqpend++;
        return  1;              /* Victory */
}

static  void    addidhash(vhash_t ind)
{
        struct  Ventry  *fp = &Var_seg.vlist[ind];
        BtvarRef  vp = &fp->Vent;
        unsigned  hashval = vid_hash(&vp->var_id);
        fp->Vidnext = Var_seg.vidhash[hashval];
        Var_seg.vidhash[hashval] = ind;
}

static vhash_t  addnamehash(BtvarRef vp)
{
        struct  Ventry  *fp;
        vhash_t         *hpp, hp, newindex;
        unsigned        hashval;

        /* If we just ran off the end of the segment grow as necessary
           and rebuild free chain.  */

        if  (Var_seg.dptr->vs_freech < 0  &&  growvseg() < 0)
                return  -1;

        /* Take new structure off the free chain, and copy over variable */

        Var_seg.dptr->vs_serial++;
        qchanges++;
        vqpend++;
        Var_seg.dptr->vs_nvars++;
        fp = &Var_seg.vlist[newindex = Var_seg.dptr->vs_freech];
        Var_seg.dptr->vs_freech = fp->Vnext;
        fp->Vent = *vp;
        if  (!vp->var_id.hostid)
                fp->Vent.var_id.slotno = newindex;
        fp->Vused = 1;

        /* Calculate slot in hash table.  hpp points to the `thing
           which pointed to what we are looking at'.  */

        hpp = &Var_seg.vhash[hashval = calchash(vp->var_name)];

        /* If it matches another similar variable name, put it next to
           it, so we search collision chain.  */

        while  ((hp = *hpp) >= 0)  {
                if  (strcmp(Var_seg.vlist[hp].Vent.var_name, vp->var_name) == 0)  {
                        /* Matched name, put it in front */
                        *hpp = newindex;
                        fp->Vnext = hp;         /* Index of old one */
                        return  newindex;
                }
                hpp = &Var_seg.vlist[hp].Vnext; /* Next one */
        }

        fp->Vnext = Var_seg.vhash[hashval];             /* Puts at front of chain */
        Var_seg.vhash[hashval] = newindex;

        return  newindex;                       /* All ok */
}

static vhash_t  addhash(BtvarRef vp)
{
        vhash_t  newindex = addnamehash(vp);
        addidhash(newindex);
        return  newindex;
}

static void del_vidhash(BtvarRef vp)
{
        unsigned  hashval = vid_hash(&vp->var_id);
        vhash_t  *hpp = &Var_seg.vidhash[hashval];
        vhash_t  hp;

        while  ((hp = *hpp) >= 0)  {
                struct  Ventry  *fp = &Var_seg.vlist[hp];
                if  (&fp->Vent == vp)  {
                        *hpp = fp->Vidnext;
                        return;
                }
                hpp = &fp->Vidnext;
        }

        panic($E{Panic hash deletion bug});
}

/* Delete a structure from the hash tables.
   We are assumed to be pointing to the Vent itself.  */

void  delhash(BtvarRef vp)
{
        unsigned        hashval;
        vhash_t         *hpp, hp;

        /* First delete from ID chain */

        del_vidhash(vp);

        /* Now delete from name chain */

        hashval = calchash(vp->var_name);
        hpp = &Var_seg.vhash[hashval];

        while  ((hp = *hpp) >= 0)  {
                struct Ventry *fp = &Var_seg.vlist[hp];
                if  (&fp->Vent == vp)  {
                        *hpp = fp->Vnext;
                        fp->Vnext = Var_seg.dptr->vs_freech;
                        fp->Vused = 0;
                        Var_seg.dptr->vs_freech = hp;
                        Var_seg.dptr->vs_serial++;
                        qchanges++;
                        vqpend++;
                        Var_seg.dptr->vs_nvars--;
                        return;
                }
                hpp = &fp->Vnext;       /*  Whoops... */
        }
        panic($E{Panic hash deletion bug});
}

/* Find index of variable, additionally look for constraints so as
   to select the right one (if any) for the given user. */

vhash_t  findvarbyname(ShreqRef sr, BtvarRef vp, int *rp)
{
        vhash_t hp;
        struct  Ventry  *fp;

        for  (hp = Var_seg.vhash[calchash(vp->var_name)];  hp >= 0;  hp = fp->Vnext)  {
                fp = &Var_seg.vlist[hp];
                if  (strcmp(fp->Vent.var_name, vp->var_name) == 0  &&
                     vp->var_id.hostid == fp->Vent.var_id.hostid  &&
                     (sr->hostid != 0  ||  visible(sr, &fp->Vent.var_mode)))
                                return  hp;
        }

        *rp = V_NEXISTS;
        return  -1;
}

/* Look up variable by host it belongs to and slot For references over network.
   Souped up to look in hash table  */

vhash_t  findvar(const vident *id)
{
        vhash_t hp = Var_seg.vidhash[vid_hash(id)];

        while  (hp >= 0)  {
                struct  Ventry  *fp = &Var_seg.vlist[hp];
                vident  *vidp = &fp->Vent.var_id;
                if  (vidp->hostid == id->hostid  &&  vidp->slotno == id->slotno)
                        return  hp;
                hp = fp->Vidnext;
        }
        return  -1;
}

/* Similar routine used internally when job file read in We deal with
   references to variables of machines which aren't running yet
   by creating skeleton entries.  */

vhash_t  lookvar(Vref *vrr)
{
        vhash_t hp;
        struct  Ventry  *fp;

        /* My host id is saved in Vrefs as 0 */

        if  (vrr->sv_hostid != 0)  {
                Btvar   skelvar;

                /* Not current, but do we know it at all?  I don't
                   want to be caught out if some turkey shuffles
                   round the /etc/hosts file.  */

                if  (!find_connected(vrr->sv_hostid)  &&  !look_hostid(vrr->sv_hostid))
                        return  -1;

                for  (hp = Var_seg.vhash[calchash(vrr->sv_name)];  hp >= 0;  hp = fp->Vnext)  {
                        fp = &Var_seg.vlist[hp];
                        if  (fp->Vent.var_id.hostid != vrr->sv_hostid)
                                continue;
                        if  (strcmp(fp->Vent.var_name, vrr->sv_name) == 0  &&
                             fp->Vent.var_mode.o_uid == vrr->sv_uid  &&
                             fp->Vent.var_mode.o_gid == vrr->sv_gid)
                                return  hp;
                }
                BLOCK_ZERO(&skelvar, sizeof(skelvar));
                skelvar.var_id.hostid = vrr->sv_hostid;
                skelvar.var_flags = VF_SKELETON;
                strncpy(skelvar.var_name, vrr->sv_name, BTV_NAME);
                skelvar.var_mode.o_uid = vrr->sv_uid;
                skelvar.var_mode.o_gid = vrr->sv_gid;
                strncpy(skelvar.var_mode.o_user, prin_uname((uid_t) vrr->sv_uid), UIDSIZE);
                strncpy(skelvar.var_mode.o_group, prin_gname((gid_t) vrr->sv_gid), UIDSIZE);
                return  addnamehash(&skelvar);
        }
        else
                for  (hp = Var_seg.vhash[calchash(vrr->sv_name)];  hp >= 0;  hp = fp->Vnext)  {
                        fp = &Var_seg.vlist[hp];
                        if  (strcmp(fp->Vent.var_name, vrr->sv_name) == 0)  {
                                if  (fp->Vent.var_mode.o_uid == vrr->sv_uid  &&  fp->Vent.var_mode.o_gid == vrr->sv_gid)
                                        return  hp;
                        }
                }
        return  -1L;
}

int  is_skeleton(const vhash_t ind)
{
        BtvarRef        vp = &Var_seg.vlist[ind].Vent;
        return  vp->var_id.hostid != 0  &&  vp->var_flags & VF_SKELETON;
}

/* Insist that cluster variables are completely unique.  */

static int  clustwouldclash(BtvarRef vp)
{
        vhash_t hp;
        struct  Ventry  *fp;
        BtvarRef        vfp;
        for  (hp = Var_seg.vhash[calchash(vp->var_name)];  hp >= 0;  hp = fp->Vnext)  {
                fp = &Var_seg.vlist[hp];
                vfp = &fp->Vent;
                if  (vfp == vp  ||  vfp->var_id.hostid != 0)
                        continue;
                if  (strcmp(vfp->var_name, vp->var_name) == 0)
                        return  1;
        }
        return  0;
}

/* Return pointer to variable specified by job in similar way.  */

BtvarRef  locvarind(ShreqRef sr, const vhash_t ind)
{
        struct  Ventry  *fp = &Var_seg.vlist[ind];

        if  (!fp->Vused  ||  !visible(sr, &fp->Vent.var_mode))
                return  (BtvarRef)  0;

        return  &fp->Vent;
}

/* See if putative variable would clash with someone else's.  */

int  wouldclash(BtvarRef vp, uid_t newowner, gid_t newgroup)
{
        vhash_t hp;
        struct  Ventry  *fp;
        BtvarRef        vfp;
        unsigned        bits;

        for  (hp = Var_seg.vhash[calchash(vp->var_name)];  hp >= 0;  hp = fp->Vnext)  {
                fp = &Var_seg.vlist[hp];
                vfp = &fp->Vent;
                if  (strcmp(vfp->var_name, vp->var_name) == 0  &&  vfp->var_id.hostid == vp->var_id.hostid)  {

                        /* If it's the one we're talking about, forget it. */

                        if  (vfp->var_mode.o_uid == vp->var_mode.o_uid  &&
                             vfp->var_mode.o_gid == vp->var_mode.o_gid)
                                continue;

                        bits = vfp->var_mode.o_flags & vp->var_mode.o_flags;
                        if  (vfp->var_mode.o_gid == newgroup)
                                bits |= vfp->var_mode.g_flags & vp->var_mode.g_flags;
                        else
                                bits |= (vfp->var_mode.o_flags & vp->var_mode.g_flags) |
                                        (vfp->var_mode.g_flags & vp->var_mode.o_flags);
                        if  (vfp->var_mode.o_uid == newowner)
                                bits |= vfp->var_mode.u_flags & vp->var_mode.u_flags;
                        else
                                bits |= (vfp->var_mode.u_flags & vp->var_mode.g_flags) |
                                        (vfp->var_mode.g_flags & vp->var_mode.u_flags) |
                                        (vfp->var_mode.u_flags & vp->var_mode.o_flags) |
                                        (vfp->var_mode.o_flags & vp->var_mode.u_flags);
                        if  (bits & BTM_SHOW)
                                return  1;
                }
        }
        return  0;
}

/* Create a variable.
   This is only done locally, and in response to a
   broadcast message from other machines.  */

int  var_create(ShreqRef sr, BtvarRef vp)
{
        int     ret;
        vhash_t ind;

        vp->var_sequence = 1;
        vp->var_c_time = time(&vp->var_m_time);
        vp->var_type = 0;
        if  (Network_ok)
                vp->var_flags &= VF_EXPORT|VF_CLUSTER;  /* Everything should be 0 except...
                                                           System vars are the owning machine's problem */
        else
                vp->var_flags = 0;

        /* Skip checks if variable comes from another machine - that
           is its problem not mine.  */

        if  (!(vp->var_id.hostid = sr->hostid))  {
                if  (!ppermitted(sr->uuid, BTM_CREATE))
                        return  V_NOPERM;
                if  (!checkminmode(&vp->var_mode))
                        return  V_MINPRIV;
                vp->var_mode.o_uid = vp->var_mode.c_uid = sr->uuid;
                vp->var_mode.o_gid = vp->var_mode.c_gid = sr->ugid;
                strncpy(vp->var_mode.o_user, prin_uname((uid_t) sr->uuid), UIDSIZE);
                strncpy(vp->var_mode.c_user, vp->var_mode.o_user, UIDSIZE);
                strncpy(vp->var_mode.o_group, prin_gname((gid_t) sr->ugid), UIDSIZE);
                strncpy(vp->var_mode.c_group, vp->var_mode.o_group, UIDSIZE);
                vp->var_mode.o_user[UIDSIZE] = '\0';
                vp->var_mode.c_user[UIDSIZE] = '\0';
                vp->var_mode.o_group[UIDSIZE] = '\0';
                vp->var_mode.c_group[UIDSIZE] = '\0';
        }

        /* Protest if variable already exists.
           Now the variable may exist already in skeleton form.
           It may also be duplicated because two or more user/group names
           on the remote machine map onto just one uid/gid on this one.
           We don't worry about such cases at present. Just the
           first such variable will be recognised.  */

        if  ((ind = findvarbyname(sr, vp, &ret)) >= 0)  {
                BtvarRef  orig = &Var_seg.vlist[ind].Vent;
                if  (vp->var_id.hostid == 0 || !(orig->var_flags & VF_SKELETON))
                        return  V_EXISTS;
                lockvars();
                *orig = *vp;
                unlockvars();
                if (findvar(&vp->var_id) < 0)
                        addidhash(ind);
                deskeletonise(ind);
                Var_seg.dptr->vs_serial++;
                qchanges++;
                vqpend++;
                return  V_OK;
        }
        else  if  ((vp->var_flags & (VF_EXPORT|VF_CLUSTER)) == VF_CLUSTER)
                ret = V_NOTEXPORT;
        else  if  (wouldclash(vp, sr->uuid, sr->ugid) ||
                   (vp->var_id.hostid == 0L  &&  vp->var_flags & VF_CLUSTER && clustwouldclash(vp)))
                ret = V_CLASHES;
        else  {
                lockvars();
                if  ((vp->var_id.slotno = addhash(vp)) < 0)
                        ret = V_FULLUP;
                else
                        ret = V_OK;
                unlockvars();
                if  (vp->var_id.hostid == 0)  {
                        if  (vp->var_flags & VF_EXPORT)
                                var_broadcast(vp, V_CREATE);
                        logvar(vp, $S{log code create}, $S{log source manual}, 0L, sr->uuid, sr->ugid, (BtjobRef) 0);
                }
        }

        return  ret;
}

/* Delete such a creature.  */

int  var_delete(ShreqRef sr, BtvarRef vp)
{
        int     ret;
        vhash_t vind;
        BtvarRef        targ;

        if  ((vind = findvarbyname(sr, vp, &ret)) >= 0)  {
                targ = &Var_seg.vlist[vind].Vent;
                if  (targ->var_type)
                        ret = V_DSYSVAR;
                else  if  (!shmpermitted(sr, &targ->var_mode, BTM_DELETE))
                        ret = V_NOPERM;
                else  if  (reqdforjobs(vind, 0))
                        ret = V_INUSE;
                else  if  (targ->var_id.hostid)
                        ret = V_DELREMOTE;
                else  {
                        if  (targ->var_flags & VF_EXPORT)
                                var_broadcast(targ, V_DELETED);
                        logvar(targ, $S{log code delete}, $S{log source manual}, 0L, sr->uuid, sr->ugid, (BtjobRef) 0);
                        lockvars();
                        delhash(targ);
                        unlockvars();
                        ret = V_OK;
                }
        }
        return  ret;
}

/* Reflect above at remote end */

void  var_remdelete(BtvarRef vp)
{
        vhash_t vind;

        if  ((vind = findvar(&vp->var_id)) >= 0)  {
                BtvarRef  targ = &Var_seg.vlist[vind].Vent;
                /* If it doesn't match it's because we've got more
                   than one variable on remote machine mapping
                   onto one on this machine.  */
                if  (strcmp(targ->var_name, vp->var_name) == 0)  {
                        lockvars();
                        delhash(targ);
                        unlockvars();
                }
        }
}

/* Tell existing machines about all our luvly variables after initial startup.  */

void  net_initvsync()
{
        int     nn;
        vhash_t hp;
        unsigned  lumpcount = 0;

        for  (nn = 0;  nn < VAR_HASHMOD;  nn++)
                for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = Var_seg.vlist[hp].Vnext)  {
                        BtvarRef  vp = &Var_seg.vlist[hp].Vent;
                        if  (vp->var_id.hostid == 0  &&  vp->var_flags & VF_EXPORT)
                                var_broadcast(vp, V_CREATE);
                        if  ((++lumpcount % lumpsize) == 0)
                                sleep(lumpwait);
                }
}

/* Delete variables associated with dying machine.  We may have to
   leave skeletons associated with jobs which refer to the variables.  */

void  net_vclear(const netid_t hostid)
{
        int     nn;
        vhash_t hp;
        struct  Ventry  *fp;
        BtvarRef        vp;

        lockvars();
        for  (nn = 0;  nn < VAR_HASHMOD;  nn++)  {
        retry:
                for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = fp->Vnext)  {
                        fp = &Var_seg.vlist[hp];
                        vp = &fp->Vent;
                        if  (vp->var_id.hostid == hostid)  {
                                if  (reqdforjobs(hp, 0))  {
                                        vp->var_flags |= VF_SKELETON;
                                        vp->var_mode.u_flags = vp->var_mode.g_flags = vp->var_mode.o_flags = 0;
                                        Var_seg.dptr->vs_serial++;
                                        qchanges++;
                                        vqpend++;
                                }
                                else  {
                                        delhash(vp);
                                        goto  retry; /* Cous hash is scrambled */
                                }
                        }
                }
        }
        unlockvars();
}
/* Assign a new value.  */

int  var_assign(ShreqRef sr, BtvarRef vp)
{
        int     ret;
        vhash_t vind;
        BtvarRef        targ;

        if  ((vind = findvarbyname(sr, vp, &ret)) >= 0)  {
                targ = &Var_seg.vlist[vind].Vent;
                /* Remote assigns are the remote machine's problem */
                if  (targ->var_id.hostid)
                        return  var_sendupdate(targ, vp, sr);

                if  (!shmpermitted(sr, &targ->var_mode, BTM_WRITE))  {
                        ret = V_NOPERM;
                        goto  dun;
                }

                /* Only validate sequence on local machine */

                if  (sr->hostid == 0  &&  targ->var_sequence > vp->var_sequence)  {
                        ret = V_SYNC;
                        goto  dun;
                }

                /* If it's a system variable which we can't write or
                   we're trying to set it to a silly type, fault it.  */

                if  (targ->var_type)  {
                        if  (targ->var_flags & VF_READONLY)  {
                                ret = V_SYSVAR;
                                goto  dun;
                        }
                        if  (targ->var_flags & VF_STRINGONLY)  {
                                if  (vp->var_value.const_type != CON_STRING)  {
                                        ret = V_SYSVTYPE;
                                        goto  dun;
                                }
                        }
                        else  if  (targ->var_flags & VF_LONGONLY)  {
                                if  (vp->var_value.const_type != CON_LONG)  {
                                        ret = V_SYSVTYPE;
                                        goto  dun;
                                }
                        }

                        lockvars();
                        targ->var_value = vp->var_value;
                        setsysval(targ);
                }
                else  {         /* Common or garden variable */
                        lockvars();
                        targ->var_value = vp->var_value;
                }

                targ->var_m_time = time((time_t *) 0);
                targ->var_sequence++;
                unlockvars();
                if  (targ->var_flags & VF_EXPORT)
                        var_broadcast(targ, V_ASSIGNED);
                Var_seg.dptr->vs_serial++;
                qchanges++;
                vqpend++;
                logvar(vp, $S{log code assign}, $S{log source manual}, sr->hostid, sr->uuid, sr->ugid, (BtjobRef) 0);
                ret = V_OK;
        }
 dun:
        return  ret;
}

/* Receive broadcast of var assignment.  */

void  var_remassign(BtvarRef vp)
{
        vhash_t         vind;
        BtvarRef        targ;

        if  ((vind = findvar(&vp->var_id)) >= 0)  {
                targ = &Var_seg.vlist[vind].Vent;

                /* If it doesn't match it's because we've got more
                   than one variable on remote machine mapping
                   onto one on this machine.  */

                if  (strcmp(targ->var_name, vp->var_name) == 0)  {
                        targ->var_m_time = time((time_t *) 0);
                        targ->var_sequence++;
                        lockvars();
                        targ->var_value = vp->var_value; /* Combine value and comment we don't distinguish */
                        strncpy(targ->var_comment, vp->var_comment, BTV_COMMENT);
                        targ->var_comment[BTV_COMMENT] = '\0';
                        unlockvars();
                        Var_seg.dptr->vs_serial++;
                        qchanges++;
                        vqpend++;
                }
        }
}

int  var_chown(ShreqRef sr, BtvarRef vp)
{
        int             ret;
        vhash_t         vind;
        BtvarRef        targ;

        if  ((vind = findvarbyname(sr, vp, &ret)) < 0)
                return  ret;

        targ = &Var_seg.vlist[vind].Vent;

        /* Only check sequence on local machine.  */

        if  (sr->hostid == 0  &&  targ->var_sequence > vp->var_sequence)
                return  V_SYNC;

        if  (targ->var_id.hostid)
                return  var_sendugupdate(targ, sr);

        /* Are we giving it away?  */

        if  (ppermitted(sr->uuid, BTM_WADMIN))  {
                if  (!(shmpermitted(sr, &targ->var_mode, BTM_UGIVE) || shmpermitted(sr, &targ->var_mode, BTM_UTAKE)))
                        return  V_NOPERM;
                targ->var_mode.o_uid = targ->var_mode.c_uid = sr->param;
                strncpy(targ->var_mode.o_user, prin_uname((uid_t) sr->param), UIDSIZE);
                strncpy(targ->var_mode.c_user, targ->var_mode.o_user, UIDSIZE);
        }
        else  if  (targ->var_mode.c_uid == targ->var_mode.o_uid)  {
                if  (!shmpermitted(sr, &targ->var_mode, BTM_UGIVE))
                        return  V_NOPERM;

                /* Remember the proposed recipient */

                targ->var_mode.c_uid = sr->param;
                strncpy(targ->var_mode.c_user, prin_uname((uid_t) sr->param), UIDSIZE);
        }
        else  {
                /* Refuse to accept it unless I'm the proposed recipient.  */

                if  (targ->var_mode.c_uid != sr->uuid  ||
                     !shmpermitted(sr, &targ->var_mode, BTM_UTAKE))
                        return  V_NOPERM;

                vp->var_mode = targ->var_mode;
                if  (wouldclash(vp, sr->uuid, vp->var_mode.o_gid))
                        return  V_CLASHES;

                targ->var_mode.o_uid = sr->param;
                strncpy(targ->var_mode.o_user, targ->var_mode.c_user, UIDSIZE);
        }

        targ->var_m_time = time((time_t *) 0);
        targ->var_sequence++;
        Var_seg.dptr->vs_serial++;
        qchanges++;
        vqpend++;
        logvar(targ, $S{log code chown}, $S{log source manual}, sr->hostid, sr->uuid, sr->ugid, (BtjobRef) 0);
        if  (targ->var_flags & VF_EXPORT)
                var_broadcast(targ, V_CHMOGED);
        return  V_OK;
}

int  var_chgrp(ShreqRef sr, BtvarRef vp)
{
        int             ret;
        vhash_t         vind;
        BtvarRef        targ;

        if  ((vind = findvarbyname(sr, vp, &ret)) < 0)
                return  ret;
        targ = &Var_seg.vlist[vind].Vent;
        if  (sr->hostid == 0  &&  targ->var_sequence > vp->var_sequence)
                return V_SYNC;

        if  (targ->var_id.hostid)
                return  var_sendugupdate(targ, sr);

        /* Are we giving it away?  */

        if  (ppermitted(sr->uuid, BTM_WADMIN))  {
                if  (!(shmpermitted(sr, &targ->var_mode, BTM_GGIVE) || shmpermitted(sr, &targ->var_mode, BTM_GTAKE)))
                        return  V_NOPERM;
                targ->var_mode.o_gid = targ->var_mode.c_gid = sr->param;
                strncpy(targ->var_mode.o_group, prin_gname((gid_t) sr->param), UIDSIZE);
                strncpy(targ->var_mode.c_group, targ->var_mode.o_group, UIDSIZE);
        }
        else  if  (targ->var_mode.c_gid == targ->var_mode.o_gid)  {
                if  (!shmpermitted(sr, &targ->var_mode, BTM_GGIVE))
                        return  V_NOPERM;

                /* Remember the proposed recipient */

                targ->var_mode.c_gid = sr->param;
                strncpy(targ->var_mode.c_group, prin_gname((gid_t) sr->param), UIDSIZE);
        }
        else  {
                /* Refuse to accept it unless I'm the proposed recipient.  */

                if  (targ->var_mode.c_gid != sr->ugid  || !shmpermitted(sr, &targ->var_mode, BTM_GTAKE))
                        return  V_NOPERM;

                vp->var_mode = targ->var_mode;
                if  (wouldclash(vp, vp->var_mode.o_uid, sr->ugid))
                        return  V_CLASHES;

                targ->var_mode.o_gid = sr->param;
                strncpy(targ->var_mode.o_group, targ->var_mode.c_group, UIDSIZE);
        }

        targ->var_m_time = time((time_t *) 0);
        targ->var_sequence++;
        Var_seg.dptr->vs_serial++;
        qchanges++;
        vqpend++;
        if  (targ->var_flags & VF_EXPORT)
                var_broadcast(targ, V_CHMOGED);
        logvar(targ, $S{log code chgrp}, $S{log source manual}, sr->hostid, sr->uuid, sr->ugid, (BtjobRef) 0);
        return  V_OK;
}

/* Acknowledge change of owner/group/mode on remote variable.
   We may have to delete it if it now clashes */

void  var_remchmog(BtvarRef vp)
{
        vhash_t         vind;
        BtvarRef        targ;

        if  ((vind = findvar(&vp->var_id)) < 0)
                return;         /* Huh?? */
        targ = &Var_seg.vlist[vind].Vent;

        /* If it doesn't match it's because we've got more than one
           variable on remote machine mapping onto one on this
           machine - i.e. we deleted it before.  */

        if  (strcmp(targ->var_name, vp->var_name) != 0)
                return;

        if  (wouldclash(targ, vp->var_mode.o_uid, vp->var_mode.o_gid))  {
                lockvars();
                delhash(targ);
                unlockvars();
                return;
        }

        targ->var_mode = vp->var_mode;
        targ->var_m_time = time((time_t *) 0);
        targ->var_sequence++;
        Var_seg.dptr->vs_serial++;
        qchanges++;
        vqpend++;
}

int  var_chmod(ShreqRef sr, BtvarRef vp)
{
        int             ret;
        vhash_t         vind;
        BtvarRef        targ;

        if  ((vind = findvarbyname(sr, vp, &ret)) >= 0)  {
                targ = &Var_seg.vlist[vind].Vent;
                if  (sr->hostid == 0  &&  targ->var_sequence > vp->var_sequence)
                        ret = V_SYNC;
                else  if  (targ->var_id.hostid)
                        return  var_sendupdate(targ, vp, sr);
                else  if  (!shmpermitted(sr, &targ->var_mode, BTM_WRMODE))
                        ret = V_NOPERM;
                else  if  (!checkminmode(&vp->var_mode))
                        ret = V_MINPRIV;
                else  if  (wouldclash(vp, vp->var_mode.o_uid, vp->var_mode.o_gid))
                        ret = V_CLASHES;
                else  {
                        targ->var_mode.u_flags = vp->var_mode.u_flags;
                        targ->var_mode.g_flags = vp->var_mode.g_flags;
                        targ->var_mode.o_flags = vp->var_mode.o_flags;
                        targ->var_m_time = time((time_t *) 0);
                        targ->var_sequence++;
                        Var_seg.dptr->vs_serial++;
                        qchanges++;
                        vqpend++;
                        if  (targ->var_flags & VF_EXPORT)
                                var_broadcast(targ, V_CHMOGED);
                        logvar(targ, $S{log code chmod}, $S{log source manual}, sr->hostid, sr->uuid, sr->ugid, (BtjobRef) 0);
                        ret = V_OK;
                }
        }
        return  ret;
}

int  var_chcomm(ShreqRef sr, BtvarRef vp)
{
        int             ret;
        vhash_t         vind;
        BtvarRef        targ;

        if  ((vind = findvarbyname(sr, vp, &ret)) >= 0)  {
                targ = &Var_seg.vlist[vind].Vent;
                if  (sr->hostid == 0  &&  targ->var_sequence > vp->var_sequence)
                        ret = V_SYNC;
                else  if  (targ->var_id.hostid)
                        return  var_sendupdate(targ, vp, sr);
                else  if  (!shmpermitted(sr, &vp->var_mode, BTM_WRITE))
                        ret = V_NOPERM;
                else  {
                        targ->var_m_time = time((time_t *) 0);
                        targ->var_sequence++;
                        Var_seg.dptr->vs_serial++;
                        qchanges++;
                        vqpend++;
                        strncpy(targ->var_comment, vp->var_comment, BTV_COMMENT);
                        targ->var_comment[BTV_COMMENT] = '\0';
                        if  (targ->var_flags & VF_EXPORT)
                                var_broadcast(targ, V_ASSIGNED);
                        logvar(targ, $S{log code chcomment}, $S{log source manual}, sr->hostid, sr->uuid, sr->ugid, (BtjobRef) 0);
                        ret = V_OK;
                }
        }

        return  ret;
}

/* Change flags. Currently only export/non-export and cluster status.  */

int  var_chflags(ShreqRef sr, BtvarRef vp)
{
        int             ret = V_OK;
        unsigned        diffs;
        vhash_t         vind;
        BtvarRef        targ;

        if  (sr->hostid != 0)  { /* Broadcast receipt */
                if  ((vind = findvar(&vp->var_id)) < 0)
                        return  V_OK; /* Return something... */
                targ = &Var_seg.vlist[vind].Vent;
                targ->var_flags &= ~VF_CLUSTER;
                targ->var_flags |= vp->var_flags & VF_CLUSTER;
                goto  vupd;
        }

        if  ((vind = findvarbyname(sr, vp, &ret)) < 0)
                return  ret;
        targ = &Var_seg.vlist[vind].Vent;
        if  (targ->var_sequence > vp->var_sequence)
                return  V_SYNC;
        if  (targ->var_id.hostid)
                return  V_DELREMOTE;
        if  (!shmpermitted(sr, &vp->var_mode, BTM_DELETE))
                return  V_NOPERM;
        if  (((diffs = targ->var_flags ^ vp->var_flags) & (VF_EXPORT|VF_CLUSTER)) == 0) /* No change */
                return  V_OK;
        if  (!Network_ok)  {
                if  (!(targ->var_flags & (VF_EXPORT|VF_CLUSTER)))
                        return  V_OK;
                targ->var_flags &= ~(VF_EXPORT|VF_CLUSTER);
                goto  vupd;
        }
        if  (diffs & VF_CLUSTER)  {
                if  (targ->var_type == VT_MACHNAME)
                        return  V_DSYSVAR;
                if  (reqdforjobs(vind, 0))
                        return  V_INUSE;
                if  (vp->var_flags & VF_CLUSTER)  { /* Setting it */
                        if  (!(vp->var_flags & VF_EXPORT))
                                return  V_NOTEXPORT;
                        if  (clustwouldclash(targ))
                                return  V_CLASHES;
                        if  (targ->var_flags & VF_EXPORT)  {
                                targ->var_flags |= VF_CLUSTER;
                                var_broadcast(targ, V_CHFLAGS);
                        }
                        else  {
                                targ->var_flags |= VF_EXPORT|VF_CLUSTER;
                                var_broadcast(targ, V_CREATE);
                        }
                        goto  vupd;
                }
                /* Unsetting cluster */
                if  (vp->var_flags & VF_EXPORT)  {
                        targ->var_flags &= ~VF_CLUSTER;
                        var_broadcast(targ, V_CHFLAGS);
                        goto  vupd;
                }
                /* Unsetting both */
                targ->var_flags &= ~(VF_EXPORT|VF_CLUSTER);
                var_broadcast(targ, V_DELETED);
                goto  vupd;
        }

        /* Just changing export status diffs must be just VF_EXPORT */

        if  (targ->var_flags & VF_EXPORT)  {    /* Was set - we are unsetting it */
                if  (targ->var_flags & VF_CLUSTER)
                        return  V_NOTEXPORT;
                if  (targ->var_type != VT_MACHNAME &&  reqdforjobs(vind, 1))
                        return  V_INUSE;
                targ->var_flags &= ~VF_EXPORT;
                var_broadcast(targ, V_DELETED);
        }
        else  {                 /* Wasn't set - setting it */
                if  (targ->var_type != VT_MACHNAME &&  reqdforjobs(vind, 0))
                        return  V_INUSE;
                targ->var_flags |= VF_EXPORT;
                var_broadcast(targ, V_CREATE);
        }

vupd:
        targ->var_m_time = time((time_t *) 0);
        targ->var_sequence++;
        Var_seg.dptr->vs_serial++;
        qchanges++;
        vqpend++;
        logvar(targ, $S{log code chflags}, $S{log source manual}, sr->hostid, sr->uuid, sr->ugid, (BtjobRef) 0);
        return  V_OK;
}

int  var_rename(ShreqRef sr, BtvarRef vp, char *newname)
{
        int             ret;
        vhash_t         vind;
        BtvarRef        targ;

        /* Look for old name */

        if  ((vind = findvarbyname(sr, vp, &ret)) >= 0)  {
                targ = &Var_seg.vlist[vind].Vent;
                if  (targ->var_sequence > vp->var_sequence)
                        ret = V_SYNC;
                else  if  (targ->var_id.hostid)
                        ret = V_DELREMOTE;
                else  if  (!shmpermitted(sr, &targ->var_mode, BTM_DELETE))
                        ret = V_NOPERM;
                else  if  (targ->var_flags & VF_CLUSTER)
                        ret = V_RENAMECLUST;
                else  {
                        *vp = *targ;    /* Copy all attributes */
                        strncpy(vp->var_name, newname, BTV_NAME);
                        vp->var_name[BTV_NAME] = '\0';
                        if  (findvarbyname(sr, vp, &ret) >= 0)
                                ret = V_RENEXISTS;              /* Clash */
                        else  {
                                lockvars();
                                delhash(targ);
                                vp->var_m_time = time((time_t *) 0);
                                vp->var_sequence++;
                                /* Theory: we can't be full up if we've just
                                   deleted something so we won't check...
                                   ALSO note assumption that we will re-use
                                   the same structure */
                                addhash(vp);
                                unlockvars();
                                logvar(vp, $S{log code rename}, $S{log source manual}, 0, sr->uuid, sr->ugid, (BtjobRef) 0);
                                if  (targ->var_flags & VF_EXPORT)
                                        var_broadcast(targ, V_RENAMED);
                                jqpend++;       /* Force save of job file which has vars by name in */
                                ret = V_OK;
                        }
                }
        }
        return  ret;
}

vhash_t  myvariable(BtvarRef vp, BtmodeRef jmode, const unsigned perms)
{
        vhash_t retvh;
        struct  Ventry  *fp;
        Shreq   sr;

        sr.hostid = 0;          /* Must set this! */
        sr.uuid = jmode->o_uid; /* Set these for "visible" call */
        sr.ugid = jmode->o_gid; /* and following "shmpermitted" */
        for  (retvh = Var_seg.vhash[calchash(vp->var_name)];  retvh >= 0;  retvh = fp->Vnext)  {
                fp = &Var_seg.vlist[retvh];
                if  (strcmp(fp->Vent.var_name, vp->var_name) == 0  &&  fp->Vent.var_id.hostid == 0L)  {
                        if  (visible(&sr, &fp->Vent.var_mode))  {
                                vp = &Var_seg.vlist[retvh].Vent;
                                if  (vp->var_type != 0) /* No fiddling with sys vars */
                                        return  -2;
                                return  shmpermitted(&sr, &vp->var_mode, perms)? retvh: -2;
                        }
                }
        }
        return  -1;     /* Not found */
}

/* Acknowledge change of name of remote variable.
   We may have to delete it if it now clashes */

void  var_remchname(BtvarRef vp)
{
        vhash_t vind, hp;
        struct  Ventry  *fp;
        BtvarRef        targ;

        /* The name passed is the new name - if that clashes then we
           have another variable mapped to this name.  */

        if  ((vind = findvar(&vp->var_id)) < 0)
                return;
        for  (hp = Var_seg.vhash[calchash(vp->var_name)];  hp >= 0;  hp = fp->Vnext)  {
                fp = &Var_seg.vlist[hp];
                targ = &fp->Vent;
                if  (strcmp(targ->var_name, vp->var_name) == 0  &&
                     targ->var_mode.o_uid == vp->var_mode.o_uid  &&
                     targ->var_mode.o_gid == vp->var_mode.o_gid)  {
                        lockvars();
                        delhash(&Var_seg.vlist[vind].Vent);
                        unlockvars();
                        return;
                }
        }

        fp = &Var_seg.vlist[vind];
        targ = &fp->Vent;
        lockvars();
        delhash(targ);  /* Also deletes the id hash which shouldn't have changed */
        vp->var_m_time = time((time_t *) 0);
        vp->var_sequence++;
        addhash(vp);    /* Restores the ID hash */
        unlockvars();
}

/* Return value of variable if a long.  */

static LONG  varlvalue(BtvarRef vp)
{
        if  (vp->var_value.const_type == CON_STRING)
                return  0L;
        return  vp->var_value.con_un.con_long;
}

/* Return value of variable if a string.  */

static char *varsvalue(BtvarRef vp)
{
        static  char    rnbuf[20];

        if  (vp->var_value.const_type == CON_LONG)  {
                sprintf(rnbuf, "%ld", (long) vp->var_value.con_un.con_long);
                return  rnbuf;
        }
        return  vp->var_value.con_un.con_string;
}

/* Compare variable value with constant */

int  varcomp(JcondRef cr, BtjobhRef jp)
{
        vhash_t         varind = cr->bjc_varind;
        BtconRef        cp = &cr->bjc_value;
        BtvarRef        vp = &Var_seg.vlist[varind].Vent;
        int             strc;
        LONG            lval;

        if  (!Var_seg.vlist[varind].Vused)
                return  COMPAR_UNDEF;
        if  (vp->var_id.hostid  &&  vp->var_flags & VF_CLUSTER)  {
                varind = myvariable(vp, &jp->bj_mode, BTM_READ);
                if  (varind < 0  ||  !Var_seg.vlist[varind].Vused)
                        return  COMPAR_UNDEF;
                vp = &Var_seg.vlist[varind].Vent;
        }

        switch  (cp->const_type)  {
        case  CON_LONG:
                lval = varlvalue(vp);
                return  lval == cp->con_un.con_long? COMPAR_EQ:
                        lval < cp->con_un.con_long? COMPAR_LT: COMPAR_GT;
        case  CON_STRING:
                strc = strcmp(varsvalue(vp), cp->con_un.con_string);
                return  strc == 0? COMPAR_EQ: strc < 0? COMPAR_LT: COMPAR_GT;
        }
        return  0;
}

void  assrest(BtvarRef targ, unsigned source, BtjobRef jp)
{
        if  (targ->var_id.hostid == 0)  {
                if  (targ->var_type)
                        setsysval(targ);
                logvar(targ, $S{log code assign}, source, jp->h.bj_runhostid, jp->h.bj_mode.o_uid, jp->h.bj_mode.o_gid, jp);
        }
        targ->var_m_time = time((time_t *) 0);
        targ->var_sequence++;
        Var_seg.dptr->vs_serial++;
        qchanges++;
        vqpend++;
}

/* Complete arithmetic and assignment operations on job */

void  jassvar(const vhash_t varind, BtconRef cp, unsigned source, BtjobRef jp)
{
        Var_seg.vlist[varind].Vent.var_value = *cp;
        assrest(&Var_seg.vlist[varind].Vent, source, jp);
}

void  jopvar(const vhash_t varind, BtconRef cp, int op, unsigned source, BtjobRef jp)
{
        BtconRef  vcp =  &Var_seg.vlist[varind].Vent.var_value;

        if  (vcp->const_type == CON_STRING || cp->const_type == CON_STRING)
                return;

        switch  (op)  {
        case  '+':
                vcp->con_un.con_long += cp->con_un.con_long;
                break;
        case  '-':
                vcp->con_un.con_long -= cp->con_un.con_long;
                break;
        case  '*':
                vcp->con_un.con_long *= cp->con_un.con_long;
                break;
        case  '/':
                if  (cp->con_un.con_long == 0L)
                        return;
                vcp->con_un.con_long /= cp->con_un.con_long;
                break;
        case  '%':
                if  (cp->con_un.con_long == 0L)
                        return;
                vcp->con_un.con_long %= cp->con_un.con_long;
                break;
        }
        assrest(&Var_seg.vlist[varind].Vent, source, jp);
}

/* Adjust current load level.
   Tell the world if we have to.  */

void  adjust_ll(const LONG amt)
{
        Current_ll += amt;
        if  (Clsvp)  {
                Clsvp->var_value.con_un.con_long += amt;
                Var_seg.dptr->vs_serial++;
                if  (Clsvp->var_flags & VF_EXPORT)
                        var_broadcast(Clsvp, V_ASSIGNED);
        }
}
