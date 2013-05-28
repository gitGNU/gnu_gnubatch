/* ripc.c -- clean up / diagnose IPC facilities

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
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>
#ifdef  OS_LINUX                /* Later Linux doesn't define struct msgbuf otherwise */
#define __USE_GNU  1
#endif
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#ifdef  HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <pwd.h>
#include <grp.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "ipcstuff.h"
#ifndef USING_FLOCK
#define USING_FLOCK
#endif
#ifdef  USING_MMAP
#undef  USING_MMAP
#endif
#define mmfd    lockfd
#include "q_shm.h"
#include "btuser.h"
#include "files.h"
#include "cmdint.h"
#include "shreq.h"

#define DEFAULT_NWORDS  4
#define MAX_NWORDS      8

int                     Ignored_error;
int                     errors = 0, locktries = 0, locksem = -1, using_mmap = -1;
int                     hadalarm;
unsigned                lockwait;
unsigned                blksize = DEFAULT_NWORDS;               /* Bumped up to bytes later */
char                    npchar = '.', dumphex, dumpok, dumpall, psok, notifok,  dipc, showfree, nofgrep, lockfail;
char                    *psarg, *scriptname;
char                    *spooldir = "/usr/local/var/gnubatch";          /* Checkme */
int                     envselect_value;

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

struct  jshm_info       Job_seg;
struct  vshm_info       Var_seg;

#ifdef  OS_FREEBSD
#define msgbuf  mymsg
#endif

#if     (defined(OS_LINUX) || defined(OS_BSDI)) && !defined(_SEM_SEMUN_UNDEFINED)
#define my_semun        semun
#else

RETSIGTYPE  notealarm(const int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(sig, notealarm);
#endif

        hadalarm++;
}

/* Define a union as defined in semctl(2) to pass the 4th argument of
   semctl. On most machines it is good enough to pass the value
   but on the SPARC and PYRAMID unions are passed differently
   even if all the possible values would fit in an int.  */

union   my_semun        {
        int     val;
        struct  semid_ds        *buf;
        ushort                  *array;
};
#endif

char *mmfile_name(const char *name)
{
        static  char    fname[PATH_MAX];

        /* Paranoia about buffer overflow */
        if  (strlen(spooldir) + strlen(name) + 2 > PATH_MAX)
                return  "none";
        strcpy(fname, spooldir);
        strcat(fname, "/");
        strcat(fname, name);
        return  fname;
}

#ifdef  HAVE_SYS_MMAN_H

/* Locate a memory-mapped file in its hole and return an fd to it
   and stat-ify it into sb */

int  open_mmfile(const char *name, struct stat *sb)
{
        int     result;
        if  ((result = open(mmfile_name(name), O_RDONLY)) < 0)
                return  -1;
        fstat(result, sb);
        return  result;
}
#endif

void  segdump(unsigned char *shp, unsigned segsz)
{
        unsigned  addr = 0, had = 0, hadsame = 0, left = segsz;
        unsigned  char  lastbuf[MAX_NWORDS * sizeof(ULONG)];

        while  (left > 0)  {
                unsigned  pce = left > blksize? blksize: left;
                unsigned  cnt;

                if  (had  &&  pce == blksize  &&  memcmp(shp, lastbuf, blksize) == 0)  {
                        if  (!hadsame)  {
                                hadsame++;
                                printf("%.6x -- same ---\n", addr);
                        }
                }
                else  {
                        had++;
                        hadsame = 0;
                        memcpy(lastbuf, shp, blksize);
                        printf("%.6x ", addr);
                        for  (cnt = 0;  cnt < pce;  cnt++)  {
                                printf("%.2x", shp[cnt]);
                                if  ((cnt & (sizeof(ULONG)-1)) == sizeof(ULONG)-1)
                                        putchar(' ');
                        }
                        if  (pce < blksize)  {
                                unsigned  col;
                                for  (col = pce * 2 + pce/sizeof(ULONG);  col < blksize/sizeof(ULONG) * (2*sizeof(ULONG)+1);  col++)
                                        putchar(' ');
                        }
                        for  (cnt = 0;  cnt < pce;  cnt++)  {
                                putchar(isprint(shp[cnt])? shp[cnt]: npchar);
                                if  ((cnt & (sizeof(ULONG)-1)) == (sizeof(ULONG)-1)  &&  cnt != pce-1)
                                        putchar(' ');
                        }
                        putchar('\n');
                }

                addr += pce;
                left -= pce;
                shp += pce;
        }
        printf("%.6x --- end ---\n", addr);
}

void  dump_lock(const int fd, const char *msg)
{
        struct  flock   lck;
        if  (fd < 0)  {
                printf("Cannot open %s lock\n", msg);
                return;
        }
        lck.l_type = F_WRLCK;
        lck.l_whence = 0;
        lck.l_start = 0;
        lck.l_len = 0;
        if  (fcntl(fd, F_GETLK, &lck) < 0  ||  lck.l_type == F_UNLCK)
                printf("%s segment is not locked\n", msg);
        else
                printf("%s segment is locked by process id %d\n", msg, lck.l_pid);
}

void  dump_mode(unsigned mode)
{
        printf("%c%c-", mode & 4? 'r': '-', mode & 2? 'w': '-');
}

char *pr_uname(uid_t uid)
{
        static  char    buf[16];
        struct  passwd  *pw;

        if  ((pw = getpwuid(uid)))
                return  pw->pw_name;
        sprintf(buf, "U%ld", (long) uid);
        return  buf;
}

char *pr_gname(gid_t gid)
{
        static  char    buf[16];
        struct  group   *pw;

        if  ((pw = getgrgid(gid)))
                return  pw->gr_name;
        sprintf(buf, "G%ld", (long) gid);
        return  buf;
}

char *lookhost(netid_t hid)
{
        static  char    buf[100];
        struct  hostent *hp;

        if  (hid == 0L)
                return  "";
        if  (!(hp = gethostbyaddr((char *) &hid, sizeof(hid), AF_INET)))  {
                struct  in_addr addr;
                addr.s_addr = hid;
                sprintf(buf, "%s:", inet_ntoa(addr));
        }
        else
                sprintf(buf, "%s:", hp->h_name);
        return  buf;
}

void  dump_ipcperm(struct ipc_perm *perm)
{
        printf("Owner: %s/%s\t", pr_uname(perm->uid), pr_gname(perm->gid));
        printf("Creator: %s/%s\tMode: ", pr_uname(perm->cuid), pr_gname(perm->cgid));
        dump_mode(perm->mode >> 6);
        dump_mode(perm->mode >> 3);
        dump_mode(perm->mode);
        putchar('\n');
}

void  dotime(time_t arg, char *descr)
{
        struct  tm      *tp = localtime(&arg);
        static  char    *wdays[] =  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"  };

        printf("%s:\t%s %.2d/%.2d/%.4d at %.2d:%.2d:%.2d\n", descr,
               wdays[tp->tm_wday], tp->tm_mday, tp->tm_mon+1, tp->tm_year + 1900,
               tp->tm_hour, tp->tm_min, tp->tm_sec);
}

void  dops(const long lpid)
{
        if  (scriptname)  {
                if  (lpid != 0)  {
                        if  (lpid == getpid())
                                printf("Sorry pid field overwritten by self (%ld)\n", lpid);
                        else
                                printf("Last attaching process was %ld\n", lpid);
                }
                fflush(stdout);
                Ignored_error = system(scriptname);
                _exit(101);
        }
        else  {
                char    obuf[100];
                if  (lpid == 0L)
                        sprintf(obuf, "ps %s", psarg);
                else  if  (lpid == getpid())  {
                        printf("Sorry pid field overwritten by self (%ld)\n", lpid);
                        sprintf(obuf, "ps %s", psarg);
                }
                else  {
                        printf("Last attaching process was id %ld, output of ps follows\n\n", lpid);
                        if  (nofgrep)
                                sprintf(obuf, "ps %s", psarg);
                        else
                                sprintf(obuf, "ps %s | fgrep \' %ld \'", psarg, lpid);
                }
                fflush(stdout);
                Ignored_error = system(obuf);
        }
}

void  shmhdr(long key, int chan, struct shmid_ds *sp, char *descr)
{
        printf("Shared memory for %s, key %lx, id %d\n", descr, key, chan);
        dump_ipcperm(&sp->shm_perm);
        printf("Size is %d, last op pid %ld, creator pid %ld, attached %lu\n",
                      (int) sp->shm_segsz, (long) sp->shm_lpid, (long) sp->shm_cpid,
                      (unsigned long) sp->shm_nattch);
        dotime(sp->shm_atime, "Last attached");
        dotime(sp->shm_dtime, "Last detached");
        dotime(sp->shm_ctime, "Last changed");
}

#ifdef  HAVE_SYS_MMAN_H
void  mmaphdr(struct stat *sp, const char *descr)
{
        printf("Memory mapped file for %s\n", descr);
        printf("Owner: %s/%s\tMode:", pr_uname(sp->st_uid), pr_gname(sp->st_gid));
        putchar(sp->st_mode & (1 << 8)? 'r': '-');
        putchar(sp->st_mode & (1 << 7)? 'w': '-');
        putchar(sp->st_mode & (1 << 6)? (sp->st_mode & (1 << 11)? 's': 'x'): (sp->st_mode & (1 << 11)? 'S': '-'));
        putchar(sp->st_mode & (1 << 5)? 'r': '-');
        putchar(sp->st_mode & (1 << 4)? 'w': '-');
        putchar(sp->st_mode & (1 << 3)? (sp->st_mode & (1 << 10)? 's': 'x'): (sp->st_mode & (1 << 10)? 'S': '-'));
        putchar(sp->st_mode & (1 << 2)? 'r': '-');
        putchar(sp->st_mode & (1 << 1)? 'w': '-');
        putchar(sp->st_mode & (1 << 0)? (sp->st_mode & (1 << 9)? 't': 'x'): (sp->st_mode & (1 << 9)? 'T': '-'));
        putchar('\n');
        dotime(sp->st_atime, "Last accessed");
        dotime(sp->st_mtime, "Last modified");
        dotime(sp->st_ctime, "Last changed");
}
#endif

void  dumpjob(BtjobhRef jp)
{
        printf("%s%.6ld\t", lookhost(jp->bj_hostid), (long) jp->bj_job);
        if  (jp->bj_jflags & BJ_WRT)
                printf(" WRT");
        if  (jp->bj_jflags & BJ_MAIL)
                printf(" ML");
        if  (jp->bj_jflags & BJ_NOADVIFERR)
                printf(" NOADV");
        if  (jp->bj_jflags & BJ_EXPORT)
                printf(" EXPORT");
        if  (jp->bj_jflags & BJ_REMRUNNABLE)
                printf(" RR");
        if  (jp->bj_jflags & BJ_CLIENTJOB)
                printf(" CLI");
        if  (jp->bj_jflags & BJ_ROAMUSER)
                printf(" ROAMU");
        if  (jp->bj_jrunflags & BJ_PROPOSED)
                printf(" PROP");
        if  (jp->bj_jrunflags & BJ_SKELHOLD)
                printf(" SKELH");
        if  (jp->bj_jrunflags & BJ_AUTOKILLED)
                printf(" AK");
        if  (jp->bj_jrunflags & BJ_AUTOMURDER)
                printf(" MURD");
        if  (jp->bj_jrunflags & BJ_HOSTDIED)
                printf(" HOSTDIED");
        if  (jp->bj_jrunflags & BJ_FORCE)
                printf(" FORCE");
        if  (jp->bj_jrunflags & BJ_FORCENA)
                printf(" FORCENA");
        if  (jp->bj_jrunflags & BJ_PENDKILL)
                printf(" PENDKILL");
}

void  dumpvar(BtvarRef vv)
{
        static  char    *snames[] =  { "ll", "curl", "logj", "logv", "mach", "stl", "stw"  };
        printf("%s%s\t%s\t", lookhost(vv->var_id.hostid), vv->var_name, vv->var_comment);
        if  (vv->var_flags & VF_READONLY)
                printf(" RO");
        if  (vv->var_flags & VF_STRINGONLY)
                printf(" STRINGO");
        if  (vv->var_flags & VF_LONGONLY)
                printf(" LONGO");
        if  (vv->var_flags & VF_EXPORT)
                printf(" EXPORT");
        if  (vv->var_flags & VF_SKELETON)
                printf(" SKEL");
        if  (vv->var_flags & VF_CLUSTER)
                printf(" CLUST");
        if  (vv->var_type != 0)
                printf(" %s", vv->var_type >= VT_STARTWAIT? "???" : snames[(int) vv->var_type]);
        if  (vv->var_value.const_type == CON_STRING)
                printf(": \"%s\"", vv->var_value.con_un.con_string);
        else
                printf(": %ld", (long) vv->var_value.con_un.con_long);
        putchar('\n');
}

struct  jbitmaps  {
        unsigned  msize;
        ULONG   *jbitmap,       /* Jobs */
                *fbitmap,       /* Free chain */
                *hbitmap;       /* Job numbers */
};

int  jbitmap_init(struct jbitmaps *bms)
{
        bms->msize = ((Job_seg.dptr->js_maxjobs + 31) >> 5) << 2;
        bms->jbitmap = (ULONG *) malloc(bms->msize);
        bms->fbitmap = (ULONG *) malloc(bms->msize);
        bms->hbitmap = (ULONG *) malloc(bms->msize);
        return  bms->jbitmap  &&  bms->fbitmap  &&  bms->hbitmap;
}

void  jbitmap_free(struct jbitmaps *bms)
{
        free((char *) bms->hbitmap);
        free((char *) bms->fbitmap);
        free((char *) bms->jbitmap);
}

int  jbitmap_check(struct jbitmaps *bms)
{
        if  (bms->msize == ((Job_seg.dptr->js_maxjobs + 31) >> 5) << 2)
                return  1;
        jbitmap_free(bms);
        return  jbitmap_init(bms);
}

void  jobseg_hinit()
{
        Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
        Job_seg.hashp_pid = (USHORT *) (Job_seg.inf.seg + sizeof(struct jshm_hdr));
        Job_seg.hashp_jno = (USHORT *) ((char *) Job_seg.hashp_pid + JOBHASHMOD*sizeof(USHORT));
        Job_seg.hashp_jid = (USHORT *) ((char *) Job_seg.hashp_jno + JOBHASHMOD*sizeof(USHORT));
        Job_seg.jlist = (HashBtjob *) ((char *) Job_seg.hashp_jid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));
}

void  del_jshm(const int typ)
{
#ifdef  HAVE_SYS_MMAN_H
        if  (typ > 0)
#endif
                shmctl(Job_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef  HAVE_SYS_MMAN_H
        else  {
                munmap(Job_seg.inf.seg, Job_seg.inf.segsize);
                close(Job_seg.inf.mmfd);
                unlink(mmfile_name(JMMAP_FILE));
        }
#endif
}

int  jobshminit(struct shmid_ds *sp, struct stat *fsb)
{
        int     i;

#ifdef  HAVE_SYS_MMAN_H

        /* First try to find a memory-mapped file*/

        i = open_mmfile(JMMAP_FILE, fsb);

        if  (i >= 0)  {
                char    *buffer;
                Job_seg.inf.mmfd = i;
                Job_seg.inf.reqsize = Job_seg.inf.segsize = fsb->st_size;
                if  ((buffer = mmap(0, fsb->st_size, PROT_READ, MAP_SHARED, i, 0)) == MAP_FAILED)  {
                        fprintf(stderr, "Found job file %s but could not attach it\n", JMMAP_FILE);
                        close(i);
                        return  0;
                }
                Job_seg.inf.seg = buffer;
                Job_seg.dptr = (struct jshm_hdr *) buffer;
                jobseg_hinit();
                using_mmap = 1;
                return  -1;
        }
#endif
        for  (i = 0;  i < MAXSHMS;  i += SHMINC)  {
                Job_seg.inf.base = SHMID + i + JSHMOFF + envselect_value;
        here:
                if  ((Job_seg.inf.chan = shmget((key_t) Job_seg.inf.base, 0, 0)) < 0  ||  shmctl(Job_seg.inf.chan, IPC_STAT, sp) < 0)
                        continue;
                if  ((Job_seg.inf.seg = shmat(Job_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                        break;
                Job_seg.inf.segsize = sp->shm_segsz;
                Job_seg.dptr = (struct jshm_hdr *) Job_seg.inf.seg;
                if  (Job_seg.dptr->js_type != TY_ISJOB)  {
                        shmdt(Job_seg.inf.seg);
                        continue;
                }
                if  (Job_seg.dptr->js_nxtid)  {
                        Job_seg.inf.base = Job_seg.dptr->js_nxtid;
                        shmdt(Job_seg.inf.seg);
                        goto  here;
                }
                jobseg_hinit();
                using_mmap = 0;

                /* If we didn't have a semaphore, see if we have a lock file */

                if  (locksem < 0)
                        Job_seg.inf.lockfd = open(mmfile_name(JLOCK_FILE), O_RDWR);
                return  1;
        }

        Job_seg.inf.chan = 0;
        return  0;
}

inline int  bitmap_isset(ULONG *bm, const unsigned n)
{
        return  bm[n >> 5] & (1 << (n & 31));
}

inline void  bitmap_set(ULONG *bm, const unsigned n)
{
        bm[n >> 5] |= 1 << (n & 31);
}

/* Decide if the job segment is mangled.
   Return 1 if it is but don't print anything
   Note that there is a possibility of a race condition if the
   job shm is being updated but we daren't use the semaphore in case
   we miss whatever is doing the dirty. */

int  check_jmangled(struct jbitmaps *bms)
{
        unsigned  cnt;
        LONG    jind, prevind;

        if  (Job_seg.dptr->js_njobs > Job_seg.dptr->js_maxjobs)  {
                printf("***Job number error number=%u max=%u\n", Job_seg.dptr->js_njobs, Job_seg.dptr->js_maxjobs);
                return  1;
        }

        BLOCK_ZERO(bms->jbitmap, bms->msize);
        BLOCK_ZERO(bms->fbitmap, bms->msize);
        BLOCK_ZERO(bms->hbitmap, bms->msize);

        /* Check the job queue is OK with forward and back links */

        jind = Job_seg.dptr->js_q_head;
        prevind = JOBHASHEND;

        while  (jind != JOBHASHEND)  {
                HashBtjob       *hjp = &Job_seg.jlist[jind];

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("***Job queue slot %ld outside range %u\n", (long) jind, Job_seg.dptr->js_maxjobs);
                        return  1;
                }

                if  (bitmap_isset(bms->jbitmap, jind))  {
                        printf("***Duplicated slot %ld in queue\n", (long) jind);
                        return  1;
                }

                bitmap_set(bms->jbitmap, jind);

                if  (hjp->j.h.bj_hostid == 0  &&  jind != hjp->j.h.bj_slotno)  {
                        printf("***Slot number %ld in queue does not match location %ld\n", (long) hjp->j.h.bj_slotno, (long) jind);
                        return  1;
                }

                if  (prevind != hjp->q_prv)  {
                        printf("***Previous slot number %u for queue %ld not as expected of %ld\n", hjp->q_prv, (long) jind, (long) prevind);
                        return  1;
                }

                prevind = jind;
                jind = hjp->q_nxt;
        }

        /* Check the free chain is OK */

        jind = Job_seg.dptr->js_freech;
        while  (jind != JOBHASHEND)  {
                HashBtjob       *hjp = &Job_seg.jlist[jind];

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("***Free chain slot %ld outside range %u\n", (long) jind, Job_seg.dptr->js_maxjobs);
                        return  1;
                }

                if  (bitmap_isset(bms->fbitmap, jind))  {
                        printf("***Duplicated slot %ld on free chain\n", (long) jind);
                        return  1;
                }

                bitmap_set(bms->fbitmap, jind);
                if  (bitmap_isset(bms->jbitmap, jind))  {
                        printf("***Free chain slot %ld on job queue\n", (long) jind);
                        return  1;
                }

                if  (hjp->q_prv != JOBHASHEND)  {
                        printf("***Free chain previous for slot %ld unexpected value %u\n", (long) jind, hjp->q_prv);
                        return  1;
                }

                jind = hjp->q_nxt;
        }

        /* Look for "orphaned" jobs */

        for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
                if  (!bitmap_isset(bms->jbitmap, cnt)  &&  !bitmap_isset(bms->fbitmap, cnt))  {
                        printf("***Job slot %u orphaned\n", cnt);
                        return  1;
                }

        /* Check the job number hash */

        for  (cnt = 0;  cnt < JOBHASHMOD;  cnt++)  {
                int     ccnt = 0;
                jind = Job_seg.hashp_jno[cnt];

                if  (jind == JOBHASHEND)
                        continue;

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("***Job number entry %ld outside range %u\n", (long) jind, Job_seg.dptr->js_maxjobs);
                        return  1;
                }

                prevind = JOBHASHEND;

                do  {
                        HashBtjob       *hjp = &Job_seg.jlist[jind];

                        if  (jno_jhash(hjp->j.h.bj_job) != cnt)  {
                                printf("***Job number hash slot %ld was %u expected %u\n", (long) jind, jno_jhash(hjp->j.h.bj_job), cnt);
                                return  1;
                        }

                        if  (bitmap_isset(bms->hbitmap, jind))  {
                                printf("***Duplicated slot %ld on job number hash\n", (long) jind);
                                return  1;
                        }

                        if  (!bitmap_isset(bms->jbitmap, jind))  {
                                printf("***Slot %ld on job number hash but not queue\n", (long) jind);
                                return  1;
                        }

                        if  (bitmap_isset(bms->fbitmap, jind))  {
                                printf("***Slot %ld on job number hash and free chain\n", (long) jind);
                                return  1;
                        }

                        bitmap_set(bms->hbitmap, jind);

                        if  (prevind != hjp->prv_jno_hash)  {
                                printf("***Slot %ld previous %u expected %ld\n", (long) jind, hjp->prv_jno_hash, (long) prevind);
                                return  1;
                        }
                        ccnt++;
                        prevind = jind;
                        jind = hjp->nxt_jno_hash;
                }  while  (jind != JOBHASHEND);
        }

        /* Check for inconsistencies */

        for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
                if  (bitmap_isset(bms->jbitmap, cnt)  &&  !bitmap_isset(bms->hbitmap, cnt))  {
                        printf("***Slot %u on queue but not jnumber hash\n", cnt);
                        return  1;
                }

        /* Check Ident hash chain */

        BLOCK_ZERO(bms->hbitmap, bms->msize);

        for  (cnt = 0;  cnt < JOBHASHMOD;  cnt++)  {
                int     ccnt = 0;
                jind = Job_seg.hashp_jid[cnt];

                if  (jind == JOBHASHEND)
                        continue;

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("***Job ident hash entry %ld outside range %u\n", (long) jind, Job_seg.dptr->js_maxjobs);
                        return  1;
                }

                prevind = JOBHASHEND;

                do  {
                        HashBtjob       *hjp = &Job_seg.jlist[jind];
                        jident          jid;

                        jid.hostid = hjp->j.h.bj_hostid;
                        jid.slotno = hjp->j.h.bj_slotno;

                        if  (jid_jhash(&jid) != cnt)  {
                                printf("***Job ident hash slot %ld was %u expected %u\n", (long) jind, jid_jhash(&jid), cnt);
                                return  1;
                        }

                        if  (bitmap_isset(bms->hbitmap, jind))  {
                                printf("***Duplicate ident hash slot %ld\n", (long) jind);
                                return  1;
                        }

                        if  (!bitmap_isset(bms->jbitmap, jind))  {
                                printf("***Ident hash slot %ld not on queue\n", (long) jind);
                                return  1;
                        }

                        if  (bitmap_isset(bms->fbitmap, jind))  {
                                printf("***Ident hash slot %ld on free chain\n", (long) jind);
                                return  1;
                        }

                        bitmap_set(bms->hbitmap, jind);

                        if  (prevind != hjp->prv_jid_hash)  {
                                printf("***Ident hash slot %ld previous %u expected %ld\n", (long) jind, hjp->prv_jid_hash, (long) prevind);
                                return  1;
                        }

                        ccnt++;
                        prevind = jind;
                        jind = hjp->nxt_jid_hash;
                }  while  (jind != JOBHASHEND);
        }

        /* Check for jobs missing from ident hash */

        for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
                if  (bitmap_isset(bms->jbitmap, cnt)  &&  !bitmap_isset(bms->hbitmap, cnt))  {
                        printf("***Slot %u missing from ident hash\n", cnt);
                        return  1;
                }

        return  0;
}

void  flockit(const int mmfd)
{
        struct  flock   lck;
        lck.l_type = F_RDLCK;
        lck.l_whence = 0;
        lck.l_start = 0;
        lck.l_len = 0;
        if  (locktries == 0)
                while  (fcntl(mmfd, F_SETLKW, &lck) < 0  &&  errno == EINTR)
                        ;
        else  {
                int     cnt;
                for  (cnt = 0;  cnt < locktries;  cnt++)  {
                        for  (;;)  {
                                if  (fcntl(mmfd, F_SETLK, &lck) >= 0)
                                        return;
                                if  (errno == EINTR)
                                        continue;
                                if  (errno == EAGAIN  ||  errno == EACCES)
                                        break;
                                goto  giveup;
                        }
                        sleep(lockwait);
                }
        giveup:
                lockfail = 1;
                printf("Given up waiting for lock\n");
        }
}

void  funlockit(const int mmfd)
{
        struct  flock   lck;
        lck.l_type = F_UNLCK;
        lck.l_whence = 0;
        lck.l_start = 0;
        lck.l_len = 0;
        while  (fcntl(mmfd, F_SETLKW, &lck) < 0  &&  errno == EINTR)
                ;
}

void  semlockit(const int ind)
{
        struct  sembuf  lk[3];
        lockfail = 0;
        lk[0].sem_num = ind + 1;
        lk[1].sem_num = lk[2].sem_num = ind;
        lk[0].sem_op = 1;
        lk[1].sem_op = -1;
        lk[2].sem_op = 1;
        lk[0].sem_flg = SEM_UNDO;
        lk[1].sem_flg = locktries > 0? IPC_NOWAIT: 0;
        lk[2].sem_flg = 0;
        if  (locktries == 0)
                while  (semop(locksem, &lk[0], 3) < 0  &&  errno == EINTR)
                        ;
        else  {
                int     cnt;
                for  (cnt = 0;  cnt < locktries;  cnt++)  {
                        for  (;;)  {
                                if  (semop(locksem, &lk[0], 3) >= 0)
                                        return;
                                if  (errno == EINTR)
                                        continue;
                                if  (errno == EAGAIN)
                                        break;
                                goto  giveup;
                        }
                        sleep(lockwait);
                }
        giveup:
                lockfail = 1;
                printf("Given up waiting for lock\n");
        }
}

void  unsemlockit(const int ind)
{
        struct  sembuf  lk;
        if  (lockfail)
                return;
        lk.sem_num = ind;
        lk.sem_op = -1;
        lk.sem_flg = SEM_UNDO;
        while  (semop(locksem, &lk, 1) < 0  &&  errno == EINTR)
                ;
}

void  lock_jseg()
{
        if  (locksem >= 0)
                semlockit(JQ_FIDDLE);
        else  if  (Job_seg.inf.lockfd >= 0)
                flockit(Job_seg.inf.lockfd);
}

void  unlock_jseg()
{
        if  (locksem >= 0)
                unsemlockit(JQ_READING);
        else  if  (Job_seg.inf.lockfd >= 0)
                funlockit(Job_seg.inf.lockfd);
}

void  lock_vseg()
{
        if  (locksem >= 0)
                semlockit(VQ_FIDDLE);
        else  if  (Var_seg.inf.lockfd >= 0)
                flockit(Var_seg.inf.lockfd);
}

void  unlock_vseg()
{
        if  (locksem >= 0)
                unsemlockit(VQ_READING);
        else  if  (Var_seg.inf.lockfd >= 0)
                funlockit(Var_seg.inf.lockfd);
}

void  dump_jobshm()
{
        unsigned                cnt, jind, prevind, errs = 0;
        int                     typ;
        struct  shmid_ds        sbuf;
        struct  stat            fsbuf;
        struct  jbitmaps        jbm;

        if  ((typ = jobshminit(&sbuf, &fsbuf)) == 0)  {
                printf("Cannot open job shm\n");
                return;
        }

        if  (locksem < 0)
                dump_lock(Job_seg.inf.mmfd, "Job");

        if  (!jbitmap_init(&jbm))  {
                fprintf(stderr, "Sorry - cannot allocate bitmaps for jobs\n");
                if  (dipc)
                        del_jshm(typ);
                return;
        }

        if  (psarg  ||  scriptname)  {
                for  (;;)  {

                        if  (locktries >= 0)
                                lock_jseg();

                        if  (check_jmangled(&jbm))  {
                                dotime(time((time_t *) 0), "JOB SEGMENT CORRUPTED!!!");
                                dops(typ < 0? 0L: (long) sbuf.shm_lpid);
                                break;
                        }

                        if  (locktries >= 0)
                                unlock_jseg();

                        if  (!notifok)  {
                                dotime(time((time_t *) 0), "Jseg OK");
                                if  (psok)
                                        dops(typ < 0? 0L: (long) sbuf.shm_lpid);
                                if  (dumpok)
                                        segdump((unsigned char *) Job_seg.inf.seg, Job_seg.inf.segsize);
                        }

                        while  (hadalarm == 0)
                                pause();
                        hadalarm = 0;

#ifdef  HAVE_SYS_MMAN_H
                        if  (typ < 0) /* I.e. mmap file */
                                continue;
#endif

                        shmctl(Job_seg.inf.chan, IPC_STAT, &sbuf); /* Grab it quickly for last op pid */

                        while  (Job_seg.dptr->js_nxtid)  { /* While as it might have moved twice, unlikely, but.... */
                                printf("Job segment now moved to id %lx\n", (long) Job_seg.dptr->js_nxtid);
                                Job_seg.inf.base = Job_seg.dptr->js_nxtid;
                                shmdt(Job_seg.inf.seg);

                                if  ((Job_seg.inf.chan = shmget((key_t) Job_seg.inf.base, 0, 0)) < 0  ||
                                     shmctl(Job_seg.inf.chan, IPC_STAT, &sbuf) < 0  ||
                                     (Job_seg.inf.seg = shmat(Job_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)  {
                                        fprintf(stderr, "Sorry could not reattach new job segment %x\n", Job_seg.inf.base);
                                        return;
                                }

                                jobseg_hinit();

                                if  (!jbitmap_check(&jbm))  {
                                        fprintf(stderr, "Sorry - cannot re-allocate bitmaps for jobs\n");
                                        if  (dipc)
                                                shmctl(Job_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
                                        return;
                                }
                        }
                }
        }

        if  (locktries >= 0)
                lock_jseg();

#ifdef  HAVE_SYS_MMAN_H
        if  (typ < 0)  {
                if  (psarg || scriptname)
                        fstat(Job_seg.inf.mmfd, &fsbuf); /* Needs refresh */
                mmaphdr(&fsbuf, "Job mmap file");
        }
        else
#endif
                shmhdr(Job_seg.inf.base, Job_seg.inf.chan, &sbuf, "Job shared memory");

        printf("Header info:\n\nnjobs:\t%u\tmaxjobs:\t%u\n", Job_seg.dptr->js_njobs, Job_seg.dptr->js_maxjobs);
        dotime(Job_seg.dptr->js_lastwrite, "Last write");
        printf("Free chain %u\n", Job_seg.dptr->js_freech);
        printf("Serial:\t%lu\n\nQueue: Head %u Tail %u\n", (unsigned long) Job_seg.dptr->js_serial, Job_seg.dptr->js_q_head, Job_seg.dptr->js_q_tail);

        if  (dumpall)  {
                for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)  {
                        HashBtjob       *hjp = &Job_seg.jlist[cnt];
                        printf("%u (%x):\tNxt %u Prv %u\n", cnt, (unsigned int)((char *) hjp - Job_seg.inf.seg), hjp->q_nxt, hjp->q_prv);
                        if  (hjp->j.h.bj_job != 0)  {
                                dumpjob(&hjp->j.h);
                                if  (hjp->j.h.bj_title >= 0)
                                        printf("\t\"%s\"\n", &hjp->j.bj_space[hjp->j.h.bj_title]);
                        }
                }
                putchar('\n');
        }

        printf("Following queue\n");
        BLOCK_ZERO(jbm.jbitmap, jbm.msize);
        BLOCK_ZERO(jbm.fbitmap, jbm.msize);
        BLOCK_ZERO(jbm.hbitmap, jbm.msize);

        jind = Job_seg.dptr->js_q_head;
        prevind = JOBHASHEND;

        while  (jind != JOBHASHEND)  {
                HashBtjob       *hjp = &Job_seg.jlist[jind];

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("****Off end of job list, job slot %u\n", jind);
                        goto  baderr;
                }

                if  (bitmap_isset(jbm.jbitmap, jind))  {
                        printf("****Loop in job list, job slot %u\n", jind);
                        goto  baderr;
                }
                bitmap_set(jbm.jbitmap, jind);

                printf("%u:\t(%x)\t%s%.6ld\n", jind, (int) ((char *) hjp - Job_seg.inf.seg), lookhost(hjp->j.h.bj_hostid), (long) hjp->j.h.bj_job);

                if  (hjp->j.h.bj_hostid == 0  &&  jind != hjp->j.h.bj_slotno)  {
                        printf("****rslot is %ld not %u\n", (long) hjp->j.h.bj_slotno, jind);
                        errs++;
                }

                if  (prevind != hjp->q_prv)  {
                        printf("****Previous pointer points to %u and not %u as expected\n", hjp->q_prv, prevind);
                        errs++;
                }

                prevind = jind;
                jind = hjp->q_nxt;
        }

        printf("Following free chain\n");

        jind = Job_seg.dptr->js_freech;
        cnt = 0;
        while  (jind != JOBHASHEND)  {
                HashBtjob       *hjp = &Job_seg.jlist[jind];

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("****Off end of job list in free chain, job slot %u\n", jind);
                        goto  baderr;
                }

                if  (bitmap_isset(jbm.fbitmap, jind))  {
                        printf("****Loop in free chain, job slot %u\n", jind);
                        goto  baderr;
                }
                bitmap_set(jbm.fbitmap, jind);
                if  (bitmap_isset(jbm.jbitmap, jind))  {
                        printf("****Free chain contains job %u on job list\n", jind);
                        errs++;
                }

                if  (hjp->q_prv != JOBHASHEND)  {
                        printf("****Unexpected prev entry %u in free chain job %u\n", hjp->q_prv, jind);
                        errs++;
                }

                if  (showfree)  {
                        printf(" %6u", jind);
                        cnt += 7;
                        if  (cnt > 72)  {
                                putchar('\n');
                                cnt = 0;
                        }
                }
                jind = hjp->q_nxt;
        }
        if  (cnt > 0)
                putchar('\n');

        for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
                if  (!bitmap_isset(jbm.jbitmap, cnt)  &&  !bitmap_isset(jbm.fbitmap, cnt))  {
                        printf("****Job %u is orphaned\n", cnt);
                        errs++;
                }

        printf("\nFollowing job number hash\n");

        for  (cnt = 0;  cnt < JOBHASHMOD;  cnt++)  {
                int     ccnt = 0;
                jind = Job_seg.hashp_jno[cnt];

                if  (jind == JOBHASHEND)
                        continue;

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("****Off end of job list in job number hash, job slot %u\n", jind);
                        goto  baderr;
                }

                printf("Jno_hash %u:\n", cnt);
                prevind = JOBHASHEND;

                do  {
                        HashBtjob       *hjp = &Job_seg.jlist[jind];

                        if  (jno_jhash(hjp->j.h.bj_job) != cnt)  {
                                printf("****Incorrect hash value %u on chain for %u\n", jno_jhash(hjp->j.h.bj_job), cnt);
                                errs++;
                        }

                        if  (bitmap_isset(jbm.hbitmap, jind))  {
                                printf("****duplicated job %u job number %ld on job number hash\n", jind, (long) hjp->j.h.bj_job);
                                goto  baderr;
                        }

                        if  (!bitmap_isset(jbm.jbitmap, jind))  {
                                printf("****job %u number %ld on job number hash but not job list\n", jind, (long) hjp->j.h.bj_job);
                                errs++;
                        }
                        if  (bitmap_isset(jbm.fbitmap, jind))  {
                                printf("****job %u number %ld on job number hash and free list\n", jind, (long) hjp->j.h.bj_job);
                                errs++;
                        }

                        bitmap_set(jbm.hbitmap, jind);

                        printf("\t(%u)\t%s%ld\n", ccnt, lookhost(hjp->j.h.bj_hostid), (long) hjp->j.h.bj_job);

                        if  (prevind != hjp->prv_jno_hash)  {
                                printf("****Previous pointer points to %u and not %u as expected\n", hjp->prv_jno_hash, prevind);
                                errs++;
                        }

                        ccnt++;
                        prevind = jind;
                        jind = hjp->nxt_jno_hash;
                }  while  (jind != JOBHASHEND);
        }

        for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
                if  (bitmap_isset(jbm.jbitmap, cnt)  &&  !bitmap_isset(jbm.hbitmap, cnt))  {
                        printf("****Job slot %u missing from job number hash\n", cnt);
                        errs++;
                }

        printf("\nFollowing jident hash\n");
        BLOCK_ZERO(jbm.hbitmap, jbm.msize);

        for  (cnt = 0;  cnt < JOBHASHMOD;  cnt++)  {
                int     ccnt = 0;
                jind = Job_seg.hashp_jid[cnt];

                if  (jind == JOBHASHEND)
                        continue;

                if  (jind >= Job_seg.dptr->js_maxjobs)  {
                        printf("****Off end of job list in jident hash, job slot %u\n", jind);
                        goto  baderr;
                }

                printf("Jid_hash %u:\n", cnt);
                prevind = JOBHASHEND;

                do  {
                        HashBtjob       *hjp = &Job_seg.jlist[jind];
                        jident          jid;

                        jid.hostid = hjp->j.h.bj_hostid;
                        jid.slotno = hjp->j.h.bj_slotno;

                        if  (jid_jhash(&jid) != cnt)  {
                                printf("****Incorrect hash value %u on chain for %u\n", jid_jhash(&jid), cnt);
                                errs++;
                        }

                        if  (bitmap_isset(jbm.hbitmap, jind))  {
                                printf("****duplicated job %u job number %ld on jident hash\n", jind, (long) hjp->j.h.bj_job);
                                goto  baderr;
                        }

                        if  (!bitmap_isset(jbm.jbitmap, jind))  {
                                printf("****job %u number %ld on jident hash but not job list\n", jind, (long) hjp->j.h.bj_job);
                                errs++;
                        }
                        if  (bitmap_isset(jbm.fbitmap, jind))  {
                                printf("****job %u number %ld on jident hash and free list\n", jind, (long) hjp->j.h.bj_job);
                                errs++;
                        }

                        bitmap_set(jbm.hbitmap, jind);

                        printf("\t(%u)\t%u\t%s%ld\n", ccnt, jind, lookhost(hjp->j.h.bj_hostid), (long) hjp->j.h.bj_job);

                        if  (prevind != hjp->prv_jid_hash)  {
                                printf("****Previous pointer points to %u and not %u as expected\n", hjp->prv_jid_hash, prevind);
                                errs++;
                        }

                        ccnt++;
                        prevind = jind;
                        jind = hjp->nxt_jid_hash;
                }  while  (jind != JOBHASHEND);
        }

        for  (cnt = 0;  cnt < Job_seg.dptr->js_maxjobs;  cnt++)
                if  (bitmap_isset(jbm.jbitmap, cnt)  &&  !bitmap_isset(jbm.hbitmap, cnt))  {
                        printf("****Job slot %u missing from jident hash\n", cnt);
                        errs++;
                }

        if  (errs > 0)  {
                printf("********Found %u errors in job shm********\n", errs);
                errors += errs;
        }

 baderr:
        jbitmap_free(&jbm);
        if  (dumphex)
                segdump((unsigned char *) Job_seg.inf.seg, Job_seg.inf.segsize);
        if  (locktries >= 0)
                unlock_jseg();
        if  (dipc)
                del_jshm(typ);
}

int  varshminit(struct shmid_ds *sp, struct stat *fsb)
{
        int     i;

#ifdef  HAVE_SYS_MMAN_H

        /* First try to find a memory-mapped file*/

        if  (using_mmap == 0)
                goto  nommap;

        i = open_mmfile(VMMAP_FILE, fsb);

        if  (i >= 0)  {
                char    *buffer;
                Var_seg.inf.mmfd = i;
                Var_seg.inf.reqsize = Var_seg.inf.segsize = fsb->st_size;
                if  ((buffer = mmap(0, fsb->st_size, PROT_READ, MAP_SHARED, i, 0)) == MAP_FAILED)  {
                        fprintf(stderr, "Found var file %s but could not attach it\n", VMMAP_FILE);
                        close(i);
                        return  0;
                }
                Var_seg.inf.seg = buffer;
                Var_seg.dptr = (struct vshm_hdr *) buffer;
                Var_seg.vhash = (vhash_t *) (buffer + sizeof(struct vshm_hdr));
                Var_seg.vidhash = (vhash_t *) ((char *) Var_seg.vhash + VAR_HASHMOD * sizeof(vhash_t));
                Var_seg.vlist = (struct Ventry *) ((char *) Var_seg.vidhash + VAR_HASHMOD * sizeof(vhash_t));
                Var_seg.Nvars = Var_seg.dptr->vs_maxvars;
                return  -1;
        }

 nommap:

#endif

        for  (i = 0;  i < MAXSHMS;  i += SHMINC)  {
                Var_seg.inf.base = SHMID + i + VSHMOFF + envselect_value;
        here:
                if  ((Var_seg.inf.chan = shmget((key_t) Var_seg.inf.base, 0, 0)) < 0  ||  shmctl(Var_seg.inf.chan, IPC_STAT, sp) < 0)
                        continue;
                if  ((Var_seg.inf.seg = shmat(Var_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                        break;
                Var_seg.inf.segsize = sp->shm_segsz;
                Var_seg.dptr = (struct vshm_hdr *) Var_seg.inf.seg;
                if  (Var_seg.dptr->vs_type != TY_ISVAR)  {
                        shmdt(Var_seg.inf.seg);
                        continue;
                }
                if  (Var_seg.dptr->vs_nxtid)  {
                        Var_seg.inf.base = Var_seg.dptr->vs_nxtid;
                        shmdt(Var_seg.inf.seg);
                        goto  here;
                }
                Var_seg.vhash = (vhash_t *) (Var_seg.inf.seg + sizeof(struct vshm_hdr));
                Var_seg.vidhash = (vhash_t *) ((char *) Var_seg.vhash + VAR_HASHMOD * sizeof(vhash_t));
                Var_seg.vlist = (struct Ventry *) ((char *) Var_seg.vidhash + VAR_HASHMOD * sizeof(vhash_t));
                Var_seg.Nvars = Var_seg.dptr->vs_maxvars;

                /* If we didn't have a semaphore, see if we have a lock file */

                if  (locksem < 0)
                        Var_seg.inf.lockfd = open(mmfile_name(VLOCK_FILE), O_RDWR);
                return  1;
        }
        Var_seg.inf.chan = 0;
        return  0;
}

void  dump_varshm()
{
        int             typ;
        unsigned        cnt;
        struct  shmid_ds        sbuf;
        struct  stat            fsbuf;

        if  ((typ = varshminit(&sbuf, &fsbuf)) == 0)  {
                printf("Cannot open var shm\n");
                return;
        }

        if  (locksem < 0)
                dump_lock(Var_seg.inf.mmfd, "Var");

#ifdef  HAVE_SYS_MMAN_H
        if  (typ < 0)
                mmaphdr(&fsbuf, "Var mmap file");
        else
#endif
                shmhdr(Var_seg.inf.base, Var_seg.inf.chan, &sbuf, "Var shared memory");

        if  (locktries >= 0)
                lock_vseg();

        printf("Header info:\n\nnvars:\t%u\tmaxvars:\t%u\n", Var_seg.dptr->vs_nvars, Var_seg.dptr->vs_maxvars);
        dotime(Var_seg.dptr->vs_lastwrite, "Last write");
        printf("Free chain %ld\n", (long) Var_seg.dptr->vs_freech);
        printf("Serial:\t%lu\n\n", (unsigned long) Var_seg.dptr->vs_serial);
        if  (Var_seg.dptr->vs_lockid)
                printf("Remote lock %s\n", lookhost(Var_seg.dptr->vs_lockid));

        if  (dumpall)  {
                for  (cnt = 0;  cnt < Var_seg.dptr->vs_maxvars;  cnt++)  {
                        struct Ventry *hvv = &Var_seg.vlist[cnt];
                        printf("\n%u:\n", cnt);
                        if  (!hvv->Vused)
                                printf("NULL\n");
                        else
                                dumpvar(&hvv->Vent);
                }
                putchar('\n');
        }

        if  (dumphex)
                segdump((unsigned char *) Var_seg.inf.seg, Var_seg.inf.segsize);
        if  (locktries >= 0)
                unlock_vseg();
        if  (dipc)  {
#ifdef  HAVE_SYS_MMAN_H
                if  (typ > 0)
#endif
                        shmctl(Var_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef  HAVE_SYS_MMAN_H
                else  {
                        munmap(Var_seg.inf.seg, Var_seg.inf.segsize);
                        close(Var_seg.inf.mmfd);
                        unlink(mmfile_name(VMMAP_FILE));
                }
#endif
        }
}

void  dump_xfershm()
{
        int                     xfer_chan, xfer_lockfd = -1;
        unsigned                segsize = 0;
        char                    *xret;
        struct  Transportbuf    *Xfer_shmp;
        struct  shmid_ds        sbuf;
#ifdef  HAVE_SYS_MMAN_H
        int                     typ = 0;
        struct  stat            fsbuf;

        if  ((xfer_chan = open_mmfile(XFMMAP_FILE, &fsbuf)) >= 0)  {
                if  (locksem < 0)
                        xfer_lockfd = xfer_chan;
                segsize = fsbuf.st_size;
                if  ((xret = mmap(0, segsize, PROT_READ, MAP_SHARED, xfer_chan, 0)) == MAP_FAILED)  {
                        fprintf(stderr, "Found xfer buf file %s but could not attach it\n", XFMMAP_FILE);
                        close(xfer_chan);
                        return;
                }
                mmaphdr(&fsbuf, "Xfer buffer mmap");
                typ = -1;
        }
        else   {
#endif
                if  ((xfer_chan = shmget((key_t) TRANSHMID + envselect_value, 0, 0)) < 0  ||  shmctl(xfer_chan, IPC_STAT, &sbuf) < 0 ||
                     (xret = shmat(xfer_chan, (char *) 0, SHM_RDONLY)) == (char *) -1)  {
                        printf("Could not open xfer shm\n\n");
                        return;
                }
                segsize = sbuf.shm_segsz;
                shmhdr(TRANSHMID+envselect_value, xfer_chan, &sbuf, "Xfer buffer shm");
                if  (locksem < 0)
                        xfer_lockfd = open(mmfile_name(XLOCK_FILE), O_RDWR);
                typ = 1;
#ifdef  HAVE_SYS_MMAN_H
        }
#endif

        Xfer_shmp = (struct Transportbuf *) xret;
        printf("Next entry on xbuf %lu\n", (unsigned long) Xfer_shmp->Next);

        if  (xfer_lockfd >= 0)  {
                int     cnt;
                struct  flock   lck;
                lck.l_type = F_WRLCK;
                lck.l_whence = 0;
                lck.l_start = 0;
                lck.l_len = 0;
                if  (fcntl(xfer_lockfd, F_GETLK, &lck) > 0  &&  lck.l_type != F_UNLCK)  {
                        lck.l_type = F_WRLCK;
                        lck.l_len = sizeof(USHORT);
                        fcntl(xfer_lockfd, F_GETLK, &lck);
                        if  (lck.l_type != F_UNLCK)
                                printf("Xfer seg locked by pid %d\n", lck.l_pid);
                        lck.l_len = sizeof(Btjob);
                        for  (cnt = 0;  cnt < XBUFJOBS;  cnt++)  {
                                unsigned  offset = (char *) &Xfer_shmp->Ring[cnt] - xret;
                                lck.l_type = F_WRLCK;
                                lck.l_start = offset;
                                fcntl(xfer_lockfd, F_GETLK, &lck);
                                if  (lck.l_type != F_UNLCK)
                                        printf("Ring index %d offset %.4x seg locked by pid %d\n", cnt, offset, lck.l_pid);
                        }
                }
                else
                        printf("Xfer segment not locked\n");
        }

        if  (dumphex)
                segdump((unsigned char *) xret, segsize);

        if  (dipc)  {
#ifdef  HAVE_SYS_MMAN_H
                if  (typ >= 0)
#endif
                        shmctl(xfer_chan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef  HAVE_SYS_MMAN_H
                else  {
                        munmap(xret, segsize);
                        close(xfer_chan);
                        unlink(mmfile_name(XFMMAP_FILE));
                }
#endif
        }
}

void  dump_sem(int semid, int semnum, char *descr)
{
        union   my_semun        zz;
        zz.val = 0;
        printf("%s:\t%d\t%d\t%d\t%d\n",
               descr,
               semctl(semid, semnum, GETVAL, zz),
               semctl(semid, semnum, GETNCNT, zz),
               semctl(semid, semnum, GETZCNT, zz),
               semctl(semid, semnum, GETPID, zz));
}

void  dump_sema4()
{
        int                     semid, cnt;
        struct  semid_ds        sbuf;
        union   my_semun        zz;

        zz.buf = &sbuf;

        if  (locksem >= 0)  {

                if  (semctl(locksem, 0, IPC_STAT, zz) < 0)  {
                        printf("Cannot stat semaphore (%d)\n", errno);
                        return;
                }

                if  (sbuf.sem_nsems != SEMNUMS+XBUFJOBS)  {
                        printf("Confused about number of semaphores expected %d found %ld\n", SEMNUMS+XBUFJOBS, sbuf.sem_nsems);
                        errors++;
                        return;
                }

                printf("Semaphore 0x%.8x id %d\n", SEMID+envselect_value, locksem);
                dump_ipcperm(&sbuf.sem_perm);
                dotime(sbuf.sem_otime, "Op time");
                dotime(sbuf.sem_ctime, "Change time");

                printf("Sem:\t\t\tNcnt\tZcnt\tPid\n");
                dump_sem(locksem, JQ_FIDDLE, "Job Fiddle");
                dump_sem(locksem, JQ_READING, "Job Read");
                dump_sem(locksem, VQ_FIDDLE, "Var Fiddle");
                dump_sem(locksem, VQ_READING, "Var Read");
                dump_sem(locksem, TQ_INDEX, "Xfer LI");
                for  (cnt = 0;  cnt < XBUFJOBS;  cnt++)  {
                        char    mbuf[30];
                        sprintf(mbuf, "Xbuffer %d", cnt);
                        dump_sem(locksem, SEMNUMS+cnt, mbuf);
                }
                putchar('\n');

                if  (dipc)
                        semctl(locksem, 0, IPC_RMID, zz);
        }

        if  ((semid = semget(NETSEMID+envselect_value, 3, 0600)) < 0)  {
                printf("Cannot open net semaphore (%d)\n", errno);
                return;
        }

        if  (semctl(semid, 0, IPC_STAT, zz) < 0)  {
                printf("Cannot stat net semaphore (%d)\n", errno);
                return;
        }
        if  (sbuf.sem_nsems != 3)  {
                printf("Confused about number of net semaphores expected %d found %ld\n", 3, sbuf.sem_nsems);
                errors++;
                return;
        }

        printf("Net Semaphore 0x%.8x id %d\n", NETSEMID+envselect_value, semid);
        dump_ipcperm(&sbuf.sem_perm);
        dotime(sbuf.sem_otime, "Op time");
        dotime(sbuf.sem_ctime, "Change time");

        printf("Sem:\t\t\tNcnt\tZcnt\tPid\n");
        dump_sem(semid, 0, "Lock all");
        dump_sem(semid, 1, "Lock type");
        dump_sem(semid, 2, "Xmit count");
        putchar('\n');

        if  (dipc)
                semctl(semid, 0, IPC_RMID, zz);
}

void  dump_q(int rq)
{
        int                     msgid;
        struct  msqid_ds        sbuf;

        if  ((msgid = msgget(MSGID+envselect_value, 0)) < 0)  {
                printf("No message queue\n");
                return;
        }

        if  (msgctl(msgid, IPC_STAT, &sbuf) < 0)  {
                printf("Cannot find message state\n");
                return;
        }

        printf("Message queue key 0x%.8x id %d\n", MSGID+envselect_value, msgid);
        dump_ipcperm(&sbuf.msg_perm);
        printf("Number of bytes %lu messages %lu max bytes %lu\n",
               (unsigned long) sbuf.msg_cbytes, (unsigned long) sbuf.msg_qnum, (unsigned long) sbuf.msg_qbytes);
        printf("Last sender %ld receiver %ld\n", (long) sbuf.msg_lspid, (long) sbuf.msg_lrpid);
        dotime(sbuf.msg_stime, "Send time");
        dotime(sbuf.msg_rtime, "Receive time");
        dotime(sbuf.msg_ctime, "Change time");

        if  (rq)  {
                int     bytes;
                union  {
                        ShipcRef        shmsg;
                        Repmess         shrep;
                        char            buf[2048+sizeof(long)];
                        struct  msgbuf  mbuf;
                } un;

                if  ((bytes = msgrcv(msgid, &un.mbuf, sizeof(un)-sizeof(long), 0, IPC_NOWAIT)) < 0)
                        printf("Nothing readable\n");
                else  {
                        unsigned  count = 1;
                        do  {
                                printf("Message %d, %d bytes:", count, bytes);
                                if  (un.shrep.mm == TO_SCHED)
                                        printf(" TO SCHEDULER\t");
                                else  if  (un.shrep.mm == CHILD)
                                        printf(" TO CHILD\t");
                                else  if  (un.shrep.mm & MTOFFSET)
                                        printf(" To process pid=%ld\t", un.shrep.mm &~MTOFFSET);
                                else  if  (un.shrep.mm & NTOFFSET)
                                        printf(" To net process pid=%ld\t", un.shrep.mm &~NTOFFSET);
                                else  {
                                        printf(" Unknown message type %ld 0x%lx\n", un.shrep.mm, un.shrep.mm);
                                        count++;
                                        continue;
                                }
                                switch  (un.shrep.outmsg.mcode)  {
                                default:
                                        printf(" Unknown message type %ld", (long) un.shrep.outmsg.mcode);
                                        break;
                                case  O_LOGON:
                                        printf(" O_LOGON");
                                        break;
                                case  O_LOGOFF:
                                        printf(" O_LOGOFF");
                                        break;
                                case  O_STOP:
                                        printf(" O_STOP");
                                        break;
                                case  O_PWCHANGED:
                                        printf(" O_PWCHANGED");
                                        break;
                                case  O_OK:
                                        printf(" O_OK");
                                        break;
                                case  O_CSTOP:
                                        printf(" O_CSTOP");
                                        break;
                                case  O_JREMAP:
                                        printf(" O_JREMAP");
                                        break;
                                case  O_VREMAP:
                                        printf(" O_VREMAP");
                                        break;
                                case  O_NOPERM:
                                        printf(" O_NOPERM");
                                        break;
                                case  J_CREATE:
                                        printf(" J_CREATE");
                                        break;
                                case  J_DELETE:
                                        printf(" J_DELETE");
                                        break;
                                case  J_CHANGE:
                                        printf(" J_CHANGE");
                                        break;
                                case  J_CHOWN:
                                        printf(" J_CHOWN");
                                        break;
                                case  J_CHGRP:
                                        printf(" J_CHGRP");
                                        break;
                                case  J_CHMOD:
                                        printf(" J_CHMOD");
                                        break;
                                case  J_KILL:
                                        printf(" J_KILL");
                                        break;
                                case  J_FORCE:
                                        printf(" J_FORCE");
                                        break;
                                case  J_FORCENA:
                                        printf(" J_FORCENA");
                                        break;
                                case  J_CHANGED:
                                        printf(" J_CHANGED");
                                        break;
                                case  J_HCHANGED:
                                        printf(" J_HCHANGED");
                                        break;
                                case  J_BCHANGED:
                                        printf(" J_BCHANGED");
                                        break;
                                case  J_BHCHANGED:
                                        printf(" J_BHCHANGED");
                                        break;
                                case  J_BOQ:
                                        printf(" J_BOQ");
                                        break;
                                case  J_BFORCED:
                                        printf(" J_BFORCED");
                                        break;
                                case  J_BFORCEDNA:
                                        printf(" J_BFORCEDNA");
                                        break;
                                case  J_DELETED:
                                        printf(" J_DELETED");
                                        break;
                                case  J_CHMOGED:
                                        printf(" J_CHMOGED");
                                        break;
                                case  J_STOK:
                                        printf(" J_STOK");
                                        break;
                                case  J_NOFORK:
                                        printf(" J_NOFORK");
                                        break;
                                case  J_COMPLETED:
                                        printf(" J_COMPLETED");
                                        break;
                                case  J_PROPOSE:
                                        printf(" J_PROPOSE");
                                        break;
                                case  J_PROPOK:
                                        printf(" J_PROPOK");
                                        break;
                                case  J_CHSTATE:
                                        printf(" J_CHSTATE");
                                        break;
                                case  J_RRCHANGE:
                                        printf(" J_RRCHANGE");
                                        break;
                                case  J_RNOTIFY:
                                        printf(" J_RNOTIFY");
                                        break;
                                case  J_RESCHED:
                                        printf(" J_RESCHED");
                                        break;
                                case  J_RESCHED_NS:
                                        printf(" J_RESCHED_NS");
                                        break;
                                case  J_DSTADJ:
                                        printf(" J_DSTADJ");
                                        break;
                                case  J_OK:
                                        printf(" J_OK");
                                        break;
                                case  J_NEXIST:
                                        printf(" J_NEXIST");
                                        break;
                                case  J_VNEXIST:
                                        printf(" J_VNEXIST");
                                        break;
                                case  J_NOPERM:
                                        printf(" J_NOPERM");
                                        break;
                                case  J_VNOPERM:
                                        printf(" J_VNOPERM");
                                        break;
                                case  J_NOPRIV:
                                        printf(" J_NOPRIV");
                                        break;
                                case  J_SYSVAR:
                                        printf(" J_SYSVAR");
                                        break;
                                case  J_SYSVTYPE:
                                        printf(" J_SYSVTYPE");
                                        break;
                                case  J_FULLUP:
                                        printf(" J_FULLUP");
                                        break;
                                case  J_ISRUNNING:
                                        printf(" J_ISRUNNING");
                                        break;
                                case  J_REMVINLOCJ:
                                        printf(" J_REMVINLOCJ");
                                        break;
                                case  J_LOCVINEXPJ:
                                        printf(" J_LOCVINEXPJ");
                                        break;
                                case  J_MINPRIV:
                                        printf(" J_MINPRIV");
                                        break;
                                case  J_START:
                                        printf(" J_START");
                                        break;
                                case  V_CREATE:
                                        printf(" V_CREATE");
                                        break;
                                case  V_DELETE:
                                        printf(" V_DELETE");
                                        break;
                                case  V_ASSIGN:
                                        printf(" V_ASSIGN");
                                        break;
                                case  V_CHOWN:
                                        printf(" V_CHOWN");
                                        break;
                                case  V_CHGRP:
                                        printf(" V_CHGRP");
                                        break;
                                case  V_CHMOD:
                                        printf(" V_CHMOD");
                                        break;
                                case  V_CHCOMM:
                                        printf(" V_CHCOMM");
                                        break;
                                case  V_NEWNAME:
                                        printf(" V_NEWNAME");
                                        break;
                                case  V_CHFLAGS:
                                        printf(" V_CHFLAGS");
                                        break;
                                case  V_DELETED:
                                        printf(" V_DELETED");
                                        break;
                                case  V_ASSIGNED:
                                        printf(" V_ASSIGNED");
                                        break;
                                case  V_CHMOGED:
                                        printf(" V_CHMOGED");
                                        break;
                                case  V_RENAMED:
                                        printf(" V_RENAMED");
                                        break;
                                case  V_OK:
                                        printf(" V_OK");
                                        break;
                                case  V_EXISTS:
                                        printf(" V_EXISTS");
                                        break;
                                case  V_NEXISTS:
                                        printf(" V_NEXISTS");
                                        break;
                                case  V_CLASHES:
                                        printf(" V_CLASHES");
                                        break;
                                case  V_NOPERM:
                                        printf(" V_NOPERM");
                                        break;
                                case  V_NOPRIV:
                                        printf(" V_NOPRIV");
                                        break;
                                case  V_SYNC:
                                        printf(" V_SYNC");
                                        break;
                                case  V_SYSVAR:
                                        printf(" V_SYSVAR");
                                        break;
                                case  V_SYSVTYPE:
                                        printf(" V_SYSVTYPE");
                                        break;
                                case  V_FULLUP:
                                        printf(" V_FULLUP");
                                        break;
                                case  V_DSYSVAR:
                                        printf(" V_DSYSVAR");
                                        break;
                                case  V_INUSE:
                                        printf(" V_INUSE");
                                        break;
                                case  V_MINPRIV:
                                        printf(" V_MINPRIV");
                                        break;
                                case  V_DELREMOTE:
                                        printf(" V_DELREMOTE");
                                        break;
                                case  V_UNKREMUSER:
                                        printf(" V_UNKREMUSER");
                                        break;
                                case  V_UNKREMGRP:
                                        printf(" V_UNKREMGRP");
                                        break;
                                case  V_RENEXISTS:
                                        printf(" V_RENEXISTS");
                                        break;
                                case  V_NOTEXPORT:
                                        printf(" V_NOTEXPORT");
                                        break;
                                case  V_RENAMECLUST:
                                        printf(" V_RENAMECLUST");
                                        break;
                                case  N_SHUTHOST:
                                        printf(" N_SHUTHOST");
                                        break;
                                case  N_ABORTHOST:
                                        printf(" N_ABORTHOST");
                                        break;
                                case  N_NEWHOST:
                                        printf(" N_NEWHOST");
                                        break;
                                case  N_TICKLE:
                                        printf(" N_TICKLE");
                                        break;
                                case  N_CONNECT:
                                        printf(" N_CONNECT");
                                        break;
                                case  N_DISCONNECT:
                                        printf(" N_DISCONNECT");
                                        break;
                                case  N_PCONNOK:
                                        printf(" N_PCONNOK");
                                        break;
                                case  N_REQSYNC:
                                        printf(" N_REQSYNC");
                                        break;
                                case  N_ENDSYNC:
                                        printf(" N_ENDSYNC");
                                        break;
                                case  N_REQREPLY:
                                        printf(" N_REQREPLY");
                                        break;
                                case  N_DOCHARGE:
                                        printf(" N_DOCHARGE");
                                        break;
                                case  N_WANTLOCK:
                                        printf(" N_WANTLOCK");
                                        break;
                                case  N_UNLOCK:
                                        printf(" N_UNLOCK");
                                        break;
                                case  N_RJASSIGN:
                                        printf(" N_RJASSIGN");
                                        break;
                                case  N_XBNATT:
                                        printf(" N_XBNATT");
                                        break;
                                case  N_ROAMUSER:
                                        printf(" N_ROAMUSER");
                                        break;
                                case  N_SETNOTSERVER:
                                        printf(" N_SETNOTSERVER");
                                        break;
                                case  N_SETISSERVER:
                                        printf (" N_SETISSERVER");
                                        break;
                                case  N_CONNOK:
                                        printf(" N_CONNOK");
                                        break;
                                case  N_REMLOCK_NONE:
                                        printf(" N_REMLOCK_NONE");
                                        break;
                                case  N_REMLOCK_OK:
                                        printf(" N_REMLOCK_OK");
                                        break;
                                case  N_REMLOCK_PRIO:
                                        printf(" N_REMLOCK_PRIO");
                                        break;
                                case  N_NOFORK:
                                        printf(" N_NOFORK");
                                        break;
                                case  N_NBADMSGQ:
                                        printf(" N_NBADMSGQ");
                                        break;
                                case  N_NTIMEOUT:
                                        printf(" N_NTIMEOUT");
                                        break;
                                case  N_HOSTOFFLINE:
                                        printf(" N_HOSTOFFLINE");
                                        break;
                                case  N_HOSTDIED:
                                        printf(" N_HOSTDIED");
                                        break;
                                case  N_CONNFAIL:
                                        printf(" N_CONNFAIL");
                                        break;
                                case  N_WRONGIP:
                                        printf(" N_WRONGIP");
                                        break;
                                }
                                printf("\npid = %s%ld uid=%ld (%s) gid=%ld (%s) param=%ld\n",
                                       lookhost(un.shrep.outmsg.hostid),
                                       (long) un.shrep.outmsg.upid,
                                       (long) un.shrep.outmsg.uuid,
                                       pr_uname(un.shrep.outmsg.uuid),
                                       (long) un.shrep.outmsg.ugid,
                                       pr_gname(un.shrep.outmsg.ugid),
                                       (long) un.shrep.outmsg.param);
                                count++;
                        }  while  ((bytes = msgrcv(msgid, &un.mbuf, sizeof(un)-sizeof(long), 0, IPC_NOWAIT)) >= 0);
                }
        }
        if  (dipc)
                msgctl(msgid, IPC_RMID, (struct msqid_ds *) 0);
}

MAINFN_TYPE  main(int argc, char **argv)
{
        int     c, hadsd = 0;
        int     rq = 0;
        double  daemtime = 0.0;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);

        while  ((c = getopt(argc, argv, "rdFAD:P:o:nxabB:N:S:Gl:L:X:")) != EOF)
                switch  (c)  {
                default:
                usage:
                        fprintf(stderr, "Usage: %s [-r] [-d] [-F] [-A] [-x] [-a] [-b] [-B n] [-N ch] [-S dir ] [-D secs] [-P psarg] [-G ] [-n] [-l n ] [-L n] [-o outfile]\n", argv[0]);
                        return  1;
                case  'r':
                        rq++;
                        continue;
                case  'd':
                        dipc = 1;
                        continue;
                case  'F':
                        showfree = 1;
                        continue;
                case  'A':
                        dumpall = 1;
                        continue;
                case  'S':
                        hadsd++;
                        spooldir = optarg;
                        continue;
                case  'D':
                        daemtime = atof(optarg);
                        continue;
                case  'P':
                        psarg = optarg;
                        continue;
                case  'X':
                        scriptname = optarg;
                        if  (access(scriptname, 1) < 0)  {
                                fprintf(stderr, "Cannot execute: %s\n", scriptname);
                                return  3;
                        }
                        continue;
                case  'G':
                        nofgrep = 1;
                        continue;
                case  'o':
                        if  (!freopen(optarg, "a", stdout))  {
                                fprintf(stderr, "Cannot output to %s\n", optarg);
                                return  2;
                        }
                        continue;
                case  'n':
                        notifok = 1;
                        continue;
                case  'x':
                        dumphex = 1;
                        continue;
                case  'a':
                        dumpok = 1;
                        continue;
                case  'b':
                        psok = 1;
                        continue;
                case  'B':
                        blksize = atoi(optarg);
                        if  (blksize == 0  ||  blksize > MAX_NWORDS)
                                goto  usage;
                        continue;
                case  'N':
                        npchar = optarg[0];
                        continue;
                case  'l':
                        locktries = atoi(optarg);
                        continue;
                case  'L':
                        lockwait = atoi(optarg);
                        continue;
                }

        if  (daemtime > 0.0)  {
                double ipart, fpart;
                struct  itimerval  itv;
#ifdef  STRUCT_SIG
                struct  sigstruct_name  z;
                z.sighandler_el = notealarm;
                sigmask_clear(z);
                z.sigflags_el = SIGVEC_INTFLAG;
                sigact_routine(SIGALRM, &z, (struct sigstruct_name *) 0);
#else
                signal(QRFRESH, notealarm);
#endif
                if  (psarg)  {
                        if  (scriptname)  {
                                fprintf(stderr, "Cannot have both script and ps arg\n");
                                return  4;
                        }
                }
                else  if  (!scriptname)  {
                        fprintf(stderr, "Must have a script (-X) or ps arg (-P)\n");
                        return  5;
                }
                fpart = modf(daemtime, &ipart);
                itv.it_interval.tv_sec = itv.it_value.tv_sec = (long) ipart;
                itv.it_interval.tv_usec = itv.it_value.tv_usec = (long)(fpart * 1000000.0);
                setitimer(ITIMER_REAL, &itv, 0);
        }
        else  if  (psarg || scriptname)  {
                fprintf(stderr, "Please specify a loop time (-D) with the script\n");
                return  6;
        }

        blksize *= sizeof(ULONG);

#ifdef  HAVE_SYS_MMAN_H

        /* Quick and dirty interpretation of /etc/Xibatch-config
           We need this for memory-mapped files */

        if  (!hadsd)  {
                FILE  *cf = fopen(MASTER_CONFIG, "r");
                if  (cf)  {
                        char    inbuf[200];
                        while  (fgets(inbuf, 200, cf))  {
                                if  (strncmp(inbuf, "SPOOLDIR", 8) == 0  &&  (inbuf[8] == '=' || inbuf[8] == ':'))  {
                                        int     ln = strlen(inbuf) - 1;
                                        char    *sv = getenv(ENV_SELECT_VAR);
                                        char    *nd;
                                        if  (sv)  {
                                                ln += strlen(sv);
                                                envselect_value = (atoi(sv) + 1) * ENV_INC_MULT;
                                        }
                                        if  (inbuf[ln] == '\n')
                                                inbuf[ln--] = '\0';
                                        if  (inbuf[9] != '/')
                                                break;
                                        if  (!(nd = malloc((unsigned) ln - 8)))  {
                                                fprintf(stderr, "Out of memory for directory\n");
                                                return  255;
                                        }
                                        strcpy(nd, &inbuf[9]);
                                        if  (sv)
                                                strcat(nd, sv);
                                        spooldir = nd;
                                        hadsd++;
                                        break;
                                }
                        }
                        fclose(cf);
                }
        }
#endif
        /* We might still not have had it */
        if  (!hadsd)  {
                char  *esv = getenv(ENV_SELECT_VAR);
                if  (esv)  {
                        char  *spdir = SPDIR_RAW;
                        char  *nspdir = malloc((unsigned) (strlen(spdir) + strlen(esv) + 1));
                        if  (!nspdir)  {
                                fprintf(stderr, "Out of memory for directory\n");
                                return  255;
                        }
                        strcpy(nspdir, spdir);
                        strcat(nspdir, esv);
                        spooldir = nspdir;
                        envselect_value = (atoi(esv) + 1) * ENV_INC_MULT;
                        hadsd++;
                }
        }

        /* Find this now */
        locksem = semget(SEMID+envselect_value, SEMNUMS, 0600);

        dump_jobshm();
        dump_varshm();
        dump_xfershm();

        dump_sema4();

        dump_q(rq);

        return  errors > 0? 100: 0;
}
