/* rjfile.c -- User process access to jobs

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
#include <errno.h>
#include <sys/ipc.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "ecodes.h"
#include "jvuprocs.h"
#include "optflags.h"
#include "files.h"

static  char    Filename[] = __FILE__;

/* Define these here, but not Job_seg.dptr as we'd need to pull in
   this module for other things such as net_feed when using
   shared libraries.  */

struct  jshm_info       Job_seg;
BtjobRef                *jj_ptrs;
ULONG                   Last_j_ser;

#ifdef  USING_FLOCK

static void  setjhold(const int typ)
{
        struct  flock   lck;
        lck.l_type = typ;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Job_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Job_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        ABORT_NOMEM;
        }
}

#else

int     Sem_chan;                               /* Create this here */

struct  sembuf
jr[3] = {{      JQ_READING,     1,      SEM_UNDO        },
        {       JQ_FIDDLE,      -1,     0               },
        {       JQ_FIDDLE,      1,      0               }},
ju[1] = {{      JQ_READING,     -1,     SEM_UNDO        }};

#endif

void  jlock()
{
#ifdef  USING_FLOCK
        setjhold(F_RDLCK);
#else
        for  (;;)  {
                if  (semop(Sem_chan, &jr[0], 3) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_JOBQ);
        }
#endif
}

void  junlock()
{
#ifdef  USING_FLOCK
        setjhold(F_UNLCK);
#else
        for  (;;)  {
                if  (semop(Sem_chan, &ju[0], 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_JOBQ);
        }
#endif
}

void  openjfile(const int inbdir, const int isserv)
{
        char    *buffer;
#ifdef  USING_MMAP
        int     Mfd;

        if  (inbdir)
                Mfd = open(JMMAP_FILE, O_RDONLY);
        else  {
                char    *fname = mkspdirfile(JMMAP_FILE);
                Mfd = open(fname, O_RDONLY);
                free(fname);
        }
        if  (Mfd < 0)  {
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }

        fcntl(Mfd, F_SETFD, 1);
        Job_seg.inf.reqsize = Job_seg.inf.segsize = lseek(Mfd, 0L, 2);
        if  ((buffer = mmap(0, Job_seg.inf.segsize, PROT_READ, MAP_SHARED, Mfd, 0)) == MAP_FAILED)  {
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }
        Job_seg.inf.mmfd = Mfd;
        Job_seg.dptr = (struct jshm_hdr *) buffer;
#else
        int     i;

#ifdef  USING_FLOCK
        if  (inbdir)
                i = open(JLOCK_FILE, O_RDWR);
        else  {
                char    *fname = mkspdirfile(JLOCK_FILE);
                i = open(fname, O_RDWR);
                free(fname);
        }
        if  (i < 0)
                goto  fail;
        fcntl(i, F_SETFD, 1);
        Job_seg.inf.lockfd = i;
#endif

        for  (i = 0;  i < MAXSHMS;  i += SHMINC)  {
                Job_seg.inf.base = SHMID + i + JSHMOFF + envselect_value;
        here:
                if  ((Job_seg.inf.chan = shmget((key_t) Job_seg.inf.base, 0, 0)) < 0)
                        continue;
                if  ((buffer = shmat(Job_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                        break;
                Job_seg.dptr = (struct jshm_hdr *) buffer;
                if  (Job_seg.dptr->js_type != TY_ISJOB)  {
                        shmdt(buffer);
                        continue;
                }
                if  (Job_seg.dptr->js_nxtid)  {
                        Job_seg.inf.base = Job_seg.dptr->js_nxtid;
                        shmdt(buffer);
                        goto  here;
                }
                goto  found;
        }
#ifdef  USING_FLOCK
 fail:
#endif
        Job_seg.inf.chan = 0;
        print_error($E{Cannot open jshm});
        exit(E_JOBQ);

 found:
#endif
        Last_j_ser = 0;
        Job_seg.inf.seg = buffer;
        Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
        Job_seg.hashp_pid = (USHORT *) (buffer + sizeof(struct jshm_hdr));
        Job_seg.hashp_jno = (USHORT *) ((char *) Job_seg.hashp_pid + JOBHASHMOD*sizeof(USHORT));
        Job_seg.hashp_jid = (USHORT *) ((char *) Job_seg.hashp_jno + JOBHASHMOD*sizeof(USHORT));
        Job_seg.jlist = (HashBtjob *) ((char *) Job_seg.hashp_jid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));
        if  ((jj_ptrs = (BtjobRef *) malloc((Job_seg.Njobs + 1) * sizeof(BtjobRef))) == (BtjobRef *) 0)
                ABORT_NOMEM;

#ifndef USING_FLOCK
        /* Fix semaphore flag for servers */

        if  (isserv)
                jr[0].sem_flg = ju[0].sem_flg = 0;
        else
                jr[0].sem_flg = ju[0].sem_flg = SEM_UNDO;
#endif
}

void  rjobfile(const int andunlock)
{
        unsigned        jind;

        jlock();

#ifdef  USING_MMAP

        /* With memory maps we can rely on the current pointer pointing to the
           job header even if the thing has grown, so js_maxjobs will give the
           current maximum. */

        if  (Job_seg.Njobs != Job_seg.dptr->js_maxjobs)  {
                char    *buffer = Job_seg.inf.seg;
                munmap(buffer, Job_seg.inf.segsize);
                Job_seg.inf.reqsize = Job_seg.inf.segsize = lseek(Job_seg.inf.mmfd, 0L, 2);
                if  ((buffer = mmap(0, Job_seg.inf.segsize, PROT_READ, MAP_SHARED, Job_seg.inf.mmfd, 0)) == MAP_FAILED)
                        ABORT_NOMEM;
                Job_seg.inf.seg = buffer;
                Job_seg.dptr = (struct jshm_hdr *) buffer;
                Job_seg.hashp_pid = (USHORT *) (buffer + sizeof(struct jshm_hdr));
                Job_seg.hashp_jno = (USHORT *) ((char *) Job_seg.hashp_pid + JOBHASHMOD*sizeof(USHORT));
                Job_seg.hashp_jid = (USHORT *) ((char *) Job_seg.hashp_jno + JOBHASHMOD*sizeof(USHORT));
                Job_seg.jlist = (HashBtjob *) ((char *) Job_seg.hashp_jid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));
                Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
                free((char *) jj_ptrs);
                if  ((jj_ptrs = (BtjobRef *) malloc((Job_seg.Njobs + 1) * sizeof(BtjobRef))) == (BtjobRef *) 0)
                        ABORT_NOMEM;
                Last_j_ser = 0; /* Make sure it gets reread */
        }

#else
        if  (Job_seg.dptr->js_nxtid)  {

                /* This copes with case where the job segment has moved.  */

                Last_j_ser = 0;

                do  {   /* Well it might move again (unlikely - but...) */
                        Job_seg.inf.base = Job_seg.dptr->js_nxtid;
                        shmdt(Job_seg.inf.seg); /*  Lose old one  */

                        if  ((Job_seg.inf.chan = shmget((key_t) Job_seg.inf.base, 0, 0)) <= 0  ||
                             (Job_seg.inf.seg = shmat(Job_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                                ABORT_NOMEM;
                        Job_seg.dptr = (struct jshm_hdr *) Job_seg.inf.seg;
                        if  (Job_seg.dptr->js_type != TY_ISJOB)
                                ABORT_NOMEM;
                }  while  (Job_seg.dptr->js_nxtid);

                /* Reinitialise pointers */

                Job_seg.hashp_pid = (USHORT *) (Job_seg.inf.seg + sizeof(struct jshm_hdr));
                Job_seg.hashp_jno = (USHORT *) ((char *) Job_seg.hashp_pid + JOBHASHMOD*sizeof(USHORT));
                Job_seg.hashp_jid = (USHORT *) ((char *) Job_seg.hashp_jno + JOBHASHMOD*sizeof(USHORT));
                Job_seg.jlist = (HashBtjob *) ((char *) Job_seg.hashp_jid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));

                if  (Job_seg.Njobs != Job_seg.dptr->js_maxjobs)  {
                        Job_seg.Njobs = Job_seg.dptr->js_maxjobs;
                        free((char *) jj_ptrs);
                        if  ((jj_ptrs = (BtjobRef *) malloc((Job_seg.Njobs + 1) * sizeof(BtjobRef))) == (BtjobRef *) 0)
                                ABORT_NOMEM;
                }
        }
#endif

        /* Do nothing if no changes have taken place */

        if  (Job_seg.dptr->js_serial == Last_j_ser)  {
                if  (andunlock)
                        junlock();
                return;
        }
        Last_j_ser = Job_seg.dptr->js_serial;

        Job_seg.njobs = 0;              /* Ones we're interested in */

        jind = Job_seg.dptr->js_q_head;

        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                BtjobRef        jp = &jhp->j;

                jind = jhp->q_nxt;

                if  (Dispflags & DF_LOCALONLY  &&  jp->h.bj_hostid  &&  (jp->h.bj_progress != BJP_RUNNING || jp->h.bj_runhostid))
                        continue;

                if  (jobqueue)  {
                        if  (jp->h.bj_title >= 0)  {
                                char  *title = &jp->bj_space[jp->h.bj_title];
                                if  (strchr(title, ':'))  {
                                        if  (!qmatch(jobqueue, title))
                                                continue;
                                }
                                else  if  (Dispflags & DF_SUPPNULL)
                                        continue;
                        }
                        else  if  (Dispflags & DF_SUPPNULL)
                                continue;
                }
                if  (!visible(&jp->h.bj_mode))
                        continue;
                if  (Restru  &&  !qmatch(Restru, jp->h.bj_mode.o_user))
                        continue;
                if  (Restrg  &&  !qmatch(Restrg, jp->h.bj_mode.o_group))
                        continue;
                jj_ptrs[Job_seg.njobs] = jp;
                Job_seg.njobs++;
        }
        if  (andunlock)
                junlock();
}
