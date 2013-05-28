/* btsched.c -- main program for btsched

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
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef  OS_LINUX
#include <grp.h>
#endif
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "network.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "cfile.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "sh_ext.h"
#if   defined (OS_LINUX) || defined(OS_OSF1) || (defined(OS_SOLARIS) && !defined(i386))
#include <ucontext.h>
#endif
#ifdef  HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef  OS_FREEBSD
#define _SC_PAGE_SIZE _SC_PAGESIZE
#endif

#ifndef USING_FLOCK
#if     (defined(OS_LINUX) || defined(OS_BSDI)) && !defined(_SEM_SEMUN_UNDEFINED)
#define my_semun        semun
#else

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
#endif

#define C_MASK          0027            /*  Umask value  */
#define IPC_MODE        0600
#define IPC_SEMMODE     0666            /*  Need to leave semaphore wide open alas */

int     Ctrl_chan,
#ifndef USING_FLOCK
        Sem_chan,
#endif
        Xshmchan;
#if  defined(USING_FLOCK) && !defined(USING_MMAP)
int     Xlockfd;
#endif

int     jqpend,
        vqpend;

float           pri_decrement = 0.0;

struct  Transportbuf    *Xbuffer;
struct  jshm_info       Job_seg;
struct  vshm_info       Var_seg;

int     qchanges;

unsigned        Startlim,
                Startwait;

LONG    Max_ll,
        Current_ll;

FILE    *Cfile;                 /* Need this here because std one in client library */

char    *spdir;

int     oldumask;

time_t  hadalarm;

PIDTYPE Xbns_pid;               /* Process id of xbnetserv process */

int     Network_ok = 1;         /* Network connections in use turn off if -n */

extern char *look_hostid(const netid_t);

void  do_exit(const int n)
{
#ifndef USING_FLOCK
        union   my_semun        uun;
#endif

#ifndef DEBUG
        kill(-(PIDTYPE) getpid(), SIGTERM);     /* Any pending child processes in group */
#endif
        if  (Network_ok)
                end_remotelock();
#ifdef  USING_MMAP
        munmap(Job_seg.inf.seg, Job_seg.inf.segsize);
        munmap(Var_seg.inf.seg, Var_seg.inf.segsize);
        munmap((void *) Xbuffer, sizeof(struct Transportbuf));
#else
        shmdt(Job_seg.inf.seg);
        shmdt(Var_seg.inf.seg);
        shmdt((char *) Xbuffer);
#endif
        msgctl(Ctrl_chan, IPC_RMID, (struct msqid_ds *) 0);
#ifndef USING_FLOCK
        uun.val = 0;
        semctl(Sem_chan, 0, IPC_RMID, uun);
#endif
#ifdef  USING_MMAP
        close(Job_seg.inf.mmfd);
        close(Var_seg.inf.mmfd);
        close(Xshmchan);
        unlink(JMMAP_FILE);
        unlink(VMMAP_FILE);
        unlink(XFMMAP_FILE);
#else
        shmctl(Job_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
        shmctl(Var_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
        shmctl(Xshmchan, IPC_RMID, (struct shmid_ds *) 0);
#ifdef  USING_FLOCK
        close(Job_seg.inf.lockfd);
        close(Var_seg.inf.lockfd);
        close(Xlockfd);
        unlink(JLOCK_FILE);
        unlink(VLOCK_FILE);
        unlink(XLOCK_FILE);
#endif
#endif
        exit(n);
}

void  nomem(const char *fl, const int ln)
{
        disp_str = fl;
        disp_arg[8] = ln;
        panic($E{NO MEMORY});
        do_exit(E_NOMEM);
}

/* Synchronise files.  */

void  syncfls()
{
        if  (vqpend)  {
                rewrvf();
                vqpend = 0;
        }
        if  (jqpend)  {
                rewrjq();
                jqpend = 0;
        }
}

/* Come to a halt */

#ifdef  NOT_TO_BE_DEFINED
$E{Scheduler normal exit}       /* Force definition into help file */
#endif

void  niceend(int signum)
{
        if  (signum < NSIG)  {
#ifdef  UNSAFE_SIGNALS
                signal(signum, SIG_IGN);
#endif
                signum = signum == SIGTERM? $E{Scheduler killed}: $E{Scheduler core dump};
        }
        nfreport(signum);
        haltall();
        if  (Network_ok)
                netshut();
        if  (Xbns_pid)
                kill(-Xbns_pid, SIGTERM);
        syncfls();
        flushlogs(1);
        killops();
        kill(child_pid, SIGKILL);
        do_exit(0);
}

#if     defined(HAVE_SIGACTION) && defined(SA_SIGINFO)
void  trap_loop(int signum, siginfo_t *sit, void *p)
{
        unsigned  long  pc = 0;
        FILE    *fp = fopen("loop.trap", "a");
#ifdef  OS_LINUX
        struct  ucontext  *uc = (struct ucontext *) p;
        pc = uc->uc_mcontext.gregs[14];
#endif
#ifdef  OS_OSF1
        struct  sigcontext  *sc = (struct sigcontext *) p;
        pc = sc->sc_pc;
#endif
#ifdef  OS_AIX_4_3
        unsigned *addr = (unsigned *) p;
        pc = addr[10];          /* I'm sure it's declared somewhere but I can't find it */
#endif
#if     defined(OS_SOLARIS) && !defined(i386)
        struct ucontext *uc = (struct ucontext *) p;
        pc = uc->uc_mcontext.gregs[REG_PC];
#endif
        fprintf(fp, "Pid = %d:\tSignal %d pc = %lx addr = %lx\n", getpid(), sit->si_signo, pc, (unsigned long) sit->si_addr);
        fclose(fp);
        _exit(0);
}
#endif

/* Try to exit gracefully and quickly....
   Return error code if you can't do it.  */

int  o_niceend(ShreqRef sr)
{
        if  (sr->param == $E{Scheduler normal exit}  &&  !ppermitted(sr->uuid, BTM_SSTOP))
                return  O_NOPERM;
        niceend((int) sr->param);
        return  0;              /* Silence compilers */
}

/* Open IPC channel.  */

static void  creatrfile()
{
        struct  msqid_ds  mb;
#ifndef USING_FLOCK
        int     i;
        union   my_semun        uun;
        struct  semid_ds  sb;
        ushort  array[SEMNUMS + XBUFJOBS];
#endif

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, IPC_MODE|IPC_CREAT|IPC_EXCL)) < 0)  {
                if  (errno == EEXIST)
                        exit(0);
                print_error($E{Panic couldnt create msg id});
                exit(E_SETUP);
        }

        if  (Daemuid != ROOTID)  {
                if  (msgctl(Ctrl_chan, IPC_STAT, (struct msqid_ds *) &mb) < 0)  {
                        print_error($E{Panic couldnt reset msg id});
                        do_exit(E_SETUP);
                }
                mb.msg_perm.uid = Daemuid;
                mb.msg_perm.gid = Daemgid;
                if  (msgctl(Ctrl_chan, IPC_SET, (struct msqid_ds *) &mb) < 0)  {
                        print_error($E{Panic couldnt reset msg id});
                        do_exit(E_SETUP);
                }
        }

#ifndef USING_FLOCK
        /* Set up semaphores.  */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_SEMMODE|IPC_CREAT)) < 0)  {
                print_error($E{Panic couldn't create sem id});
                do_exit(E_SETUP);
        }

        if  (Daemuid != ROOTID)  {
                uun.buf = &sb;
                if  (semctl(Sem_chan, 0, IPC_STAT, uun) < 0)  {
                        print_error($E{Panic couldn't reset sem id});
                        do_exit(E_SETUP);
                }

                sb.sem_perm.uid = Daemuid;
                sb.sem_perm.gid = Daemgid;

                if  (semctl(Sem_chan, 0, IPC_SET, uun) < 0)  {
                        print_error($E{Panic couldn't reset sem id});
                        do_exit(E_SETUP);
                }
        }

        for  (i = 0;  i < SEMNUMS;  i++)
                array[i] = 0;
        array[JQ_FIDDLE] = 1;
        array[VQ_FIDDLE] = 1;
        array[TQ_INDEX] = 1;
        for  (i = 0;  i < XBUFJOBS;  i++)
                array[i + SEMNUMS] = 1;

        uun.array = array;
        if  (semctl(Sem_chan, 0, SETALL, uun) < 0)  {
                print_error($E{Panic couldn't reset sem id});
                do_exit(E_SETUP);
        }
#endif
}

/* Get a shared memory id, and increment key.  */

int  gshmchan(struct btshm_info *inf, const int off)
{
#ifdef  USING_MMAP
        LONG    pagesize = sysconf(_SC_PAGE_SIZE);
        ULONG   rqsize = (inf->reqsize + pagesize - 1) & ~(pagesize-1);
        char    byte = 0;

        if  (inf->segsize > 0)  {       /* Existing segment we are growing */
                munmap(inf->seg, inf->segsize);
        }
        else  {
                char    *fname = off == JSHMOFF? JMMAP_FILE: VMMAP_FILE;
                if  ((inf->mmfd = open(fname, O_CREAT|O_RDWR|O_EXCL, IPC_MODE)) < 0)
                        goto  fail;
#ifdef  HAVE_FCHOWN
                if  (Daemuid != ROOTID)
                        Ignored_error = fchown(inf->mmfd, Daemuid, Daemgid);
#else
                if  (Daemuid != ROOTID)
                        Ignored_error = chown(fname, Daemuid, Daemgid);
#endif
                fcntl(inf->mmfd, F_SETFD, 1);
        }

        /* Write a byte to the last byte of the file */

        lseek(inf->mmfd, (long) (rqsize - 1), 0);
        if  (write(inf->mmfd, &byte, 1) == 1)  {

                /* Map what we can give up if we are stuck */

                for  (;  rqsize > inf->segsize;  rqsize >>= 1)  {
                        void  *segp = mmap(0, rqsize, PROT_READ|PROT_WRITE, MAP_SHARED, inf->mmfd, 0);
                        if  (segp != MAP_FAILED)  {
                                inf->seg = segp;
                                inf->segsize = rqsize;
                                return  1;
                        }
                }
        }

 fail:
        disp_arg[9] = off;
        panic($E{Panic trouble creating shared mem});
        return  0;              /* To keep C compilers happy */
#else
        ULONG   rqsize = inf->reqsize;
        int     result, bomb = MAXSHMS / SHMINC;
        void    *segp;
        struct  shmid_ds        shb;

        while  ((result = shmget(inf->base, rqsize, IPC_MODE|IPC_CREAT|IPC_EXCL)) < 0)  {
                if  (errno == EINVAL)  { /* Halve until it fits */
                        rqsize >>= 1;
                        if  (rqsize > inf->segsize)
                                continue;
                }
                if  (errno != EEXIST  ||  --bomb <= 0)  {
                        disp_arg[9] = inf->base;
                        panic($E{Panic trouble creating shared mem});
                }
                if  ((inf->base += SHMINC) >= SHMID + MAXSHMS + envselect_value)
                        inf->base = SHMID + off + envselect_value;
        }
        inf->chan = result;
        inf->segsize = rqsize;

        /* Find out what size we really got....  */

        if  (shmctl(result, IPC_STAT, &shb) < 0)
                panic($E{Panic trouble resetting shared mem});

        inf->segsize = shb.shm_segsz;

        if  (Daemuid != ROOTID)  {
                shb.shm_perm.uid = Daemuid;
                shb.shm_perm.gid = Daemgid;

                if  (shmctl(result, IPC_SET, &shb) < 0)
                        panic($E{Panic trouble resetting shared mem});
        }

        /* Now attach segment */

        if  ((segp = shmat(result, 0, 0)) == (void *) -1)
                return  0;

        inf->seg = segp;
        return  1;
#endif
}

void  gxbuffshm()
{
        char    *seg;
#ifdef  USING_MMAP
        LONG    pagesize = sysconf(_SC_PAGE_SIZE);
        ULONG   rqsize = (sizeof(struct Transportbuf) + pagesize - 1) & ~(pagesize-1);
        char    byte = 0;

        if  ((Xshmchan = open(XFMMAP_FILE, O_RDWR|O_TRUNC|O_CREAT, IPC_MODE)) < 0)  {
        fail:
                disp_arg[9] = TRANSHMID+envselect_value;
                panic($E{Panic trouble creating shared mem});
        }
#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(Xshmchan, Daemuid, Daemgid);
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(XFMMAP_FILE, Daemuid, Daemgid);
#endif
        fcntl(Xshmchan, F_SETFD, 1);
        lseek(Xshmchan, (long) (rqsize - 1), 0);
        if  (write(Xshmchan, &byte, 1) != 1)
                goto  fail;

        if  ((seg = mmap(0, rqsize, PROT_READ|PROT_WRITE, MAP_SHARED, Xshmchan, 0)) == MAP_FAILED)  {
                print_error($E{Panic trouble with job shm});
                do_exit(E_JOBQ);
        }

#else  /* SHM rather than MMAP */

        struct  shmid_ds        shb;

#ifdef  USING_FLOCK
        if  ((Xlockfd = open(XLOCK_FILE, O_RDWR|O_CREAT|O_TRUNC, IPC_MODE)) < 0)
                goto  fail;
#ifdef  HAVE_FCHOWN
        if  (Daemuid)
                Ignored_error = fchown(Xlockfd, Daemuid, Daemgid);
#else
        if  (Daemuid)
                Ignored_error = chown(XLOCK_FILE, Daemuid, Daemgid);
#endif
        fcntl(Xlockfd, F_SETFD, 1);
#endif

        if  ((Xshmchan = shmget(TRANSHMID+envselect_value, sizeof(struct Transportbuf), IPC_MODE|IPC_CREAT|IPC_EXCL)) < 0)  {
#ifdef  USING_FLOCK
        fail:
#endif
                disp_arg[9] = TRANSHMID+envselect_value;
                panic($E{Panic trouble creating shared mem});
        }
        if  (shmctl(Xshmchan, IPC_STAT, &shb) < 0)
                panic($E{Panic trouble resetting shared mem});
        if  (Daemuid != ROOTID)  {
                shb.shm_perm.uid = Daemuid;
                shb.shm_perm.gid = Daemgid;

                if  (shmctl(Xshmchan, IPC_SET, &shb) < 0)
                        panic($E{Panic trouble resetting shared mem});
        }
        if  ((seg = shmat(Xshmchan, (char *) 0, 0)) == (char *) -1)  {
                print_error($E{Panic trouble with job shm});
                do_exit(E_JOBQ);
        }
#endif
        Xbuffer = (struct Transportbuf *) seg;
        Xbuffer->Next = 0;
}

/* Catch alarm signals.  */

RETSIGTYPE  acatch(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(n, acatch);
#endif
        time(&hadalarm);
}

static void  o_reqs(ShipcRef rq, int bytes)
{
        unsigned ret;

        switch  (rq->sh_params.mcode)  {
        default:
                disp_arg[9] = rq->sh_params.mcode;
                nfreport($E{Invalid message type});
                return;
        badlen:
                disp_arg[9] = bytes;
                nfreport($E{Panic invalid op message length});
                return;

        case  O_LOGON:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                ret = addoper(&rq->sh_params);
                break;

        case  O_LOGOFF:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                deloper(&rq->sh_params);
                return;

        case  O_STOP:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                ret = o_niceend(&rq->sh_params);
                /* Only get here if error */
                break;

        case  O_PWCHANGED:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                un_rpwfile();
                rpwfile();
#ifdef  HAVE_GETGROUPS
                rgrpfile();
#endif
                tellchild(O_PWCHANGED, 0L);
                if  (Netm_pid)
                        kill(Netm_pid, NETSHUTSIG);
                if  (Xbns_pid)
                        kill(-Xbns_pid, SIGHUP);
                return;
        }

        sendreply(rq->sh_params.upid, ret);
}

static void  j_reqs(ShipcRef rq, int bytes)
{
        unsigned ret;

        switch  (rq->sh_params.mcode)  {
        default:
                disp_arg[9] = rq->sh_params.mcode;
                nfreport($E{Invalid message type});
                return;
        badlen:
                disp_arg[9] = bytes;
                nfreport($E{Panic invalid job message length});
                return;

        case  J_CREATE:
                if  (bytes != sizeof(Shreq) + sizeof(ULONG))
                        goto  badlen;
                ret = enqueue(&rq->sh_params, &Xbuffer->Ring[rq->sh_un.sh_jobindex]);
                break;

        case  J_DELETE:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                ret = deljob(&rq->sh_params, &rq->sh_un.jobref);
                break;

        case  J_DELETED:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                job_deleted(&rq->sh_un.jobref);
                return;

        case  J_CHANGE:
                if  (bytes != sizeof(Shreq) + sizeof(ULONG))
                        goto  badlen;
                ret = chjob(&rq->sh_params, &Xbuffer->Ring[rq->sh_un.sh_jobindex]);
                break;

        case  J_CHOWN:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                ret = job_chown(&rq->sh_params, &rq->sh_un.jobref);
                break;

        case  J_CHGRP:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                ret = job_chgrp(&rq->sh_params, &rq->sh_un.jobref);
                break;

        case  J_CHMOD:
                if  (bytes != sizeof(Shreq) + sizeof(ULONG))
                        goto  badlen;
                ret = job_chmod(&rq->sh_params, &Xbuffer->Ring[rq->sh_un.sh_jobindex]);
                break;

        case  J_KILL:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                ret = doabort(&rq->sh_params, &rq->sh_un.jobref);
                break;

        case  J_FORCE:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                ret = doforce(&rq->sh_params, &rq->sh_un.jobref, 0);
                break;

        case  J_FORCENA:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                ret = doforce(&rq->sh_params, &rq->sh_un.jobref, 1);
                break;

        case  J_STOK:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                started_job((unsigned) rq->sh_params.param);
                return;

        case  J_NOFORK:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                cannot_start((unsigned) rq->sh_params.param);
                return;

        case  J_COMPLETED:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                completed_job((unsigned) rq->sh_params.param);
                return;

        case  J_DSTADJ:
                if  (bytes != sizeof(Shreq) + sizeof(struct adjstr))
                        goto  badlen;
                ret = dstadj(&rq->sh_params, &rq->sh_un.sh_adj);
                break;

        case  J_CHANGED:        /* User request case */
        case  J_HCHANGED:
        case  J_BCHANGED:       /* Internal broadcast case*/
        case  J_BHCHANGED:
                if  (bytes != sizeof(Shreq) + sizeof(ULONG))
                        goto  badlen;
                ret = remchjob(&rq->sh_params, &Xbuffer->Ring[rq->sh_un.sh_jobindex]);
                break;

        case  J_CHMOGED:
                if  (bytes != sizeof(Shreq) + sizeof(ULONG))
                        goto  badlen;
                ret = remjchmog(&Xbuffer->Ring[rq->sh_un.sh_jobindex]);
                break;

        case  J_BOQ:            /* Broadcast case - back of queue */
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                {
                        int     jn = findj_by_jid(&rq->sh_un.jobref);
                        if  (jn >= 0)
                                back_of_queue(jn);
                }
                return;

        case  J_BFORCED:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                forced(&rq->sh_un.jobref, 0);
                return;

        case  J_BFORCEDNA:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                forced(&rq->sh_un.jobref, 1);
                return;

        case  J_CHSTATE:
                if  (bytes != sizeof(Shreq) + sizeof(struct jstatusmsg))
                        goto  badlen;
                statchange(&rq->sh_un.remstat);
                return;

        case  J_PROPOSE:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                reply_propose(&rq->sh_params, &rq->sh_un.jobref);
                return;

        case  J_PROPOK:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                propok(&rq->sh_un.jobref);
                return;

        case  J_RRCHANGE:
                if  (bytes != sizeof(Shreq) + sizeof(struct jstatusmsg))
                        goto  badlen;
                rrstatchange(rq->sh_params.hostid, &rq->sh_un.remstat);
                return;

        case  J_RNOTIFY:
                if  (bytes != sizeof(Shreq) + sizeof(struct jremnotemsg))
                        goto  badlen;
                {
                        int     jn = findj_by_jid(&rq->sh_un.remnote.jid);
                        if  (jn >= 0)
                                rem_notify(&Job_seg.jlist[jn].j, rq->sh_params.hostid, &rq->sh_un.remnote);
                        return;
                }

        case  J_RESCHED_NS:
                being_started--;
                adjust_ll(-(LONG) Job_seg.jlist[rq->sh_params.param].j.h.bj_ll);

        case  J_RESCHED:
                qchanges++;
                return;
        }

        sendreply(rq->sh_params.upid, ret);
}

static void  v_reqs(ShipcRef rq, int bytes)
{
        unsigned ret;

        switch  (rq->sh_params.mcode)  {
        default:
                disp_arg[9] = rq->sh_params.mcode;
                nfreport($E{Invalid message type});
                return;
        badlen:
                disp_arg[9] = bytes;
                nfreport($E{Panic invalid var message length});
                return;

        case  V_CREATE:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_create(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_DELETE:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_delete(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_ASSIGN:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_assign(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_CHOWN:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_chown(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_CHGRP:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_chgrp(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_CHMOD:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_chmod(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_CHCOMM:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_chcomm(&rq->sh_params, &rq->sh_un.sh_var);
                break;

        case  V_CHFLAGS:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_chflags(&rq->sh_params, &rq->sh_un.sh_var);
                if  (rq->sh_params.hostid) /* Remote broadcast from toggling cluster flag */
                        return;
                break;

        case  V_NEWNAME:
                if  (bytes < sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                ret = var_rename(&rq->sh_params, &rq->sh_un.sh_rn.sh_ovar, rq->sh_un.sh_rn.sh_rnewname);
                break;

                /* Exclusively network...  */

        case  V_DELETED:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                var_remdelete(&rq->sh_un.sh_var);
                return;

        case  V_ASSIGNED:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                var_remassign(&rq->sh_un.sh_var);
                return;

        case  V_CHMOGED:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                var_remchmog(&rq->sh_un.sh_var);
                return;

        case  V_RENAMED:
                if  (bytes != sizeof(Shreq) + sizeof(Btvar))
                        goto  badlen;
                var_remchname(&rq->sh_un.sh_var);
                return;
        }

        sendreply(rq->sh_params.upid, ret);
}

/* Network-type messages */

static void  n_reqs(ShipcRef rq, int bytes)
{
        unsigned  ret = 0;
        struct  remote  *rp;

        switch  (rq->sh_params.mcode)  {
        default:
                disp_arg[9] = rq->sh_params.mcode;
                nfreport($E{Invalid message type});
                return;
        badlen:
                disp_arg[9] = bytes;
                nfreport($E{Panic invalid var message length});
                return;

        case  N_SHUTHOST:
                clearhost(rq->sh_params.hostid, rq->sh_params.param);

        case  N_ABORTHOST:
                netmonitor();
                break;

        case  N_NEWHOST:
                newhost();
                netmonitor();
                break;

        case  N_CONNECT:
                if  (bytes != sizeof(Shreq) + sizeof(struct remote))
                        goto  badlen;
                if  (find_connected(rq->sh_un.sh_n.hostid))  {
                        ret = N_CONNOK;
                        break;
                }
                if  (Netm_pid)
                        kill(Netm_pid, NETSHUTSIG);
                if  ((rp = conn_attach(&rq->sh_un.sh_n))  &&  sendsync(rp))  {
                        ret = N_CONNOK;
                        break;
                }
                ret = N_CONNFAIL;
                break;

        case  N_DISCONNECT:
                if  (bytes != sizeof(Shreq) + sizeof(struct remote))
                        goto  badlen;
                shut_host(rq->sh_un.sh_n.hostid);
                clearhost(rq->sh_un.sh_n.hostid, -1);
                if  (Netm_pid)
                        kill(Netm_pid, NETSHUTSIG);
                ret = N_CONNOK;
                break;

        case  N_PCONNOK:        /* Internal response to probe ok */
                if  (bytes != sizeof(Shreq) + sizeof(struct remote))
                        goto  badlen;
                if  ((rp = find_probe(rq->sh_un.sh_n.hostid)))  {
                        rp = conn_attach(rp); /* Now includes free_probe */
                        if  (rp)
                                sendsync(rp);
                }
                netmonitor();
                return;

        case  N_REQSYNC:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                if  ((rp = find_sync(rq->sh_params.hostid, (int) rq->sh_params.param))  &&  sendsync(rp))
                        send_endsync(rp);
                return;

        case  N_ENDSYNC:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                endsync(rq->sh_params.hostid);
                /* Fix "forward refs" on previously-connected machines
                   to the new one */
                deskel_jobs(rq->sh_params.hostid);
                return;

        case  N_SYNCSINGLE:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                send_single_jobhdr(rq->sh_params.hostid, rq->sh_params.param);
                return;

        case  N_SETNOTSERVER:
        case  N_SETISSERVER:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                set_not_server(rq, rq->sh_params.mcode == N_SETNOTSERVER);
                return;

        case  N_XBNATT:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                /* We kill off the process if we think a previous one is running */
                if  (Xbns_pid  &&  kill(Xbns_pid, 0) >= 0)
                        kill(-(PIDTYPE) rq->sh_params.upid, SIGKILL);
                else
                        Xbns_pid = rq->sh_params.upid;
                break;

        case  N_ROAMUSER:       /* Message from xbnetserv to accept roamer */
                if  (bytes != sizeof(Shreq) + sizeof(struct remote))
                        goto  badlen;
                alloc_roam(&rq->sh_un.sh_n);
                return;

        case  N_REQREPLY:
                if  (bytes != sizeof(Shreq))
                        goto  badlen;
                {
                        Repmess rep;
                        BLOCK_ZERO(&rep, sizeof(rep));
                        rep.mm = rq->sh_params.upid + NTOFFSET;
                        rep.outmsg.param = rq->sh_params.param;
                        forcemsg((char *) &rep, sizeof(rep) - sizeof(long));
                        return;
                }

        case  N_DOCHARGE:
                if  (bytes != sizeof(Shreq) + sizeof(jident))
                        goto  badlen;
                /* Don't do anything but continue to support people who do */
                return;

        case  N_RJASSIGN:
                if  (bytes != sizeof(Shreq) + sizeof(struct jremassmsg))
                        goto  badlen;
                {
                        int     jn = findj_by_jid(&rq->sh_un.remas.jid);
                        if  (jn >= 0)
                                setasses(&Job_seg.jlist[jn].j,
                                         (unsigned) rq->sh_un.remas.flags,
                                         (unsigned) rq->sh_un.remas.source,
                                         (unsigned) rq->sh_un.remas.status,
                                         rq->sh_params.hostid);
                        return;
                }
        }
        sendreply(rq->sh_params.upid, ret);
}

/* This routine is the main process.  */

void  process()
{
        int     bytes;
        unsigned        nextalarm, which, nextt;
        union   {
                Shipc   inreq;
                char    inbuffer[MSGBUFFSIZE];
        } mun;

        /* In case there is anything to do */

        if  (Netsync_req > 0)
                netsync();

        which = nextalarm = resched();
        nextt = nettickle();
        if  (nextt != 0  &&  which > nextt)
                which = nextt;
        alarm(which);

        for  (;;)  {
                if  ((bytes = msgrcv(Ctrl_chan, (struct msgbuf *) mun.inbuffer, sizeof(mun) - sizeof(long), TO_SCHED, 0)) >= 0)  {
                        switch  (mun.inreq.sh_params.mcode & REQ_TYPE)  {
                        default:
                                disp_arg[9] = mun.inreq.sh_params.mcode;
                                nfreport($E{Invalid message type});
                                continue;

                        case  SYS_REQ:
                                o_reqs(&mun.inreq, bytes);
                                break;

                        case  JOB_REQ:
                                j_reqs(&mun.inreq, bytes);
                                break;

                        case  VAR_REQ:
                                v_reqs(&mun.inreq, bytes);
                                break;

                        case  NET_REQ:
                                n_reqs(&mun.inreq, bytes);
                                break;
                        }

                        /* If that hasn't affected anything which might control the job status, read the next one.  */

                        if  (!qchanges && Netsync_req == 0)
                             continue;
                }
                else    if  (errno != EINTR)
                        panic($E{Error on IPC});

                nextalarm = resched();

                /* Advise operators.  */

                if  (qchanges)
                        tellopers();

                /* Update files now or when due.  */

                if  (jqpend || vqpend)  {
                        time_t  now = time((time_t *) 0);
                        LONG    jtim = now - Job_seg.dptr->js_lastwrite;
                        LONG    vtim = now - Var_seg.dptr->vs_lastwrite;
                        unsigned        wa = QREWRITE;
                        if  (jtim >= QREWRITE  ||  vtim >= QREWRITE)  {
                                syncfls();
                                flushlogs(0);
                        }
                        else  {
                                if  (jqpend)
                                        wa -= jtim;
                                if  (vqpend  &&  QREWRITE - vtim < wa)
                                        wa = QREWRITE - vtim;
                                if  (nextalarm == 0 || wa < nextalarm)
                                        nextalarm = wa;
                        }
                }
                if  (Netsync_req > 0)
                        netsync();
                which = nextalarm;
                nextt = nettickle();
                if  (nextt != 0  &&  which > nextt)
                        which = nextt;
                alarm(which);
        }
}

/* Initialise holidays file to stop advtime continually reopening it */

void    initholfile()
{
        char    *fname = envprocess(HOLFILE);
        if  (access(fname, 4) >= 0)  {
                free(fname);
                return;
        }
        close(open(fname, O_CREAT|O_WRONLY, 0644));
        Ignored_error = chown(fname, Daemuid, Daemgid);
        free(fname);
}

/* Fork, restore necessary signals and (if sigcld doesn't work) dodge zombies.  */

PIDTYPE  forksafe()
{
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zign;
#endif
        PIDTYPE ret;
#ifndef BUGGY_SIGCLD
        if  ((ret = fork()) != 0)       /* Error or parent process case */
                return  ret;
#else
        if  ((ret = fork()) < 0)        /* Error case */
                return  -1;
        if  (ret > 0)  {                /* Parent process wait for child to die */
                int     status;
#ifdef  HAVE_WAITPID
                while  (waitpid(ret, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != ret)
                        ;
#endif
                if  (status != 0)       /* Child exits with error if grandchild fails */
                        return  -1;
                return  1;
        }

        /* Child process - immediately create grandchild process */

        if  ((ret = fork()) > 0)        /* Ok - done */
                exit(0);
        if  (ret < 0)                   /* Failed tell parent */
                exit(255);
#endif

        /* Restore status of hangup signal so that master scheduler
           process can get rid of me if it wants.  */

#ifdef  STRUCT_SIG
        zign.sighandler_el = SIG_DFL;
        sigact_routine(SIGHUP, &zign, (struct sigstruct_name *) 0);
#else
        signal(SIGHUP, SIG_DFL);
#endif
        return  0;
}

/* Ye olde main routine. */

MAINFN_TYPE  main(int argc, char **argv)
{
        LONG    initll = -1L;   /* Set to indicate leave alone */
        LONG    jsize = 0L, vsize = 0L;
        int     chku;
#ifndef DEBUG
        PIDTYPE pid;
#endif
#ifdef  HAVE_GETGROUPS
        gid_t   dumm = getgid(); /* Dummy var in case it checks address */
#endif
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zign;
#endif
#ifdef  HAVE_SETRLIMIT
        struct rlimit  rlt;
#endif

        versionprint(argv, "$Revision: 1.9 $", 1);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        /*      Parse arguments fairly crudely.
                -l nnn initial load level.
                -j nnn initial number of jobs
                -v nnn initial number of vars
                -d nnnn.nnn set pri_decrement
                -t turn off use of setpgrp
                -n no network */

        argv++;
        while  (*argv  &&  argv[0][0] == '-')  {
                LONG    *wh;
                switch  (argv[0][1])  {
                default:
                        if  (!argv[0][2])
                                argv++;
                        argv++;
                        continue;
                case  'n':
                        Network_ok = 0;
                        continue;
                case  't':
                        nosetpgrp++;
                        argv++;
                        continue;
                case  'l':
                        wh = &initll;
                        break;
                case  'j':
                        wh = &jsize;
                        break;
                case  'v':
                        wh = &vsize;
                        break;
                case  'd':
                        if  (argv[0][2])  {
                                pri_decrement = (float) atof(&argv[0][2]);
                                argv++;
                        }
                        else  {
                                argv++;
                                if  (argv[0])  {
                                        pri_decrement = (float) atof(argv[0]);
                                        argv++;
                                }
                        }
                        continue;
                }
                if  (argv[0][2])  {
                        *wh = atol(&argv[0][2]);
                        argv++;
                }
                else  {
                        argv++;
                        if  (argv[0])  {
                                *wh = atol(argv[0]);
                                argv++;
                        }
                }
        }

        /* Fix hassles with ulimit once and for all */
#ifdef  HAVE_SETRLIMIT
        rlt.rlim_cur = rlt.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_FSIZE, &rlt);
#endif

        init_mcfile();
        init_xenv();

        if  ((Cfile = open_icfile()) == (FILE *) 0)
                exit(E_NOCONFIG);

        fcntl(fileno(Cfile), F_SETFD, 1);

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                disp_str = spdir;
                print_error($E{Cannot change directory});
                exit(E_NOCHDIR);
        }

        Realuid = getuid();
        Realgid = getgid();

#ifdef  HAVE_GETGROUPS
        Requires_suppgrps = 1;
        setgroups(0, &dumm); /* Zap any existing supp groups */
#endif
        rpwfile();
        rgrpfile();

        /* For batch we save all the hassles by running as root (we
           have to anyhow if we are to masquerade as other users
           from time to time).  However if there is a user called
           `batch' we'll make him/her/it own the files and
           suchwhat */

        if  ((chku = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)  {
                Daemuid = ROOTID;
                lookup_uname("root");   /* Side effect sets lastgid */
        }
        else
                Daemuid = chku;
        Daemgid = lastgid;
#ifdef  SCO_SECURITY
        setluid(ROOTID);
#endif
        setuid(ROOTID);
        oldumask = umask(C_MASK);
        Ignored_error = nice(DEF_BASE_NICE);

        initlog();

        /* As parent process, generate IPC message.  Open job and var
           files.  All the stdin etc streams are sent to
           /dev/null.  This is because if the scheduler is
           invoked by 'btr', the latter does not want to restart
           until the IPC has been created.  */

#ifdef  STRUCT_SIG
        zign.sighandler_el = SIG_IGN;
        sigmask_clear(zign);
        zign.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGHUP, &zign, (struct sigstruct_name *) 0);
        sigact_routine(SIGINT, &zign, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &zign, (struct sigstruct_name *) 0);
        zign.sighandler_el = niceend;
        sigact_routine(SIGTERM, &zign, (struct sigstruct_name *) 0);
#ifndef DEBUG
#if     defined(HAVE_SIGACTION) && defined(SA_SIGINFO)
        zign.sigflags_el = SA_SIGINFO;
#ifdef  NO_SA_SIGACTION
        zign.sighandler_el = trap_loop;
#else
        zign.sa_sigaction = trap_loop;
#endif
#endif
        sigact_routine(SIGFPE, &zign, (struct sigstruct_name *) 0);
        sigact_routine(SIGBUS, &zign, (struct sigstruct_name *) 0);
        sigact_routine(SIGSEGV, &zign, (struct sigstruct_name *) 0);
        sigact_routine(SIGILL, &zign, (struct sigstruct_name *) 0);
#ifdef  SIGSYS
        sigact_routine(SIGSYS, &zign, (struct sigstruct_name *) 0);
#endif /* SIGSYS */
#ifdef  HAVE_SIGACTION
        zign.sigflags_el = SIGVEC_INTFLAG;
        zign.sighandler_el = niceend;
#endif
#endif /* !DEBUG */
#else  /* !STRUCT_SIG */
        signal(SIGHUP, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, niceend);
#ifndef DEBUG
        signal(SIGFPE, niceend);
        signal(SIGBUS, niceend);
        signal(SIGSEGV, niceend);
        signal(SIGILL, niceend);
#ifdef  SIGSYS
        signal(SIGSYS, niceend);
#endif /* SIGSYS */
#endif /* !DEBUG */
#endif /* !STRUCT_SIG */

        /* Must open var file first as it's needed by job file */

        creatrfile();
        creatvfile(vsize, initll);
        initcifile();           /* Read first to check jobs */
        creatjfile(jsize);
        gxbuffshm();
        initholfile();

#ifndef DEBUG
        close(0);  close(1);  close(2);
        Ignored_error = dup(dup(open("/dev/null", O_RDWR)));

        if  (!nosetpgrp)  {
#ifdef  SETPGRP_VOID
                setpgrp();
#else
                setpgrp(0, getpid());
#endif
        }
#endif

        initmsgs();             /* Initialise completion messages */

        /* Fork off to leave an orphaned child process.  */

#ifndef DEBUG
        pid = fork();
        if  (pid > 0)           /*  Main path  */
                exit(0);

        if  (pid < 0)
                panic($E{Cannot fork});
#endif  /* DEBUG */

#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        zign.sighandler_el = SIG_IGN;
#ifdef  SA_NOCLDWAIT
        zign.sigflags_el |= SA_NOCLDWAIT;
#endif
        sigact_routine(SIGCLD, &zign, (struct sigstruct_name *) 0);
#else
        signal(SIGCLD, SIG_IGN);
#endif
#endif

        /* Ignore PIPE errors in case daemon processes quit.  (Signal
           #defined as sigset on appropriate systems).  */

#ifdef  STRUCT_SIG
        zign.sighandler_el = SIG_IGN;
        sigact_routine(SIGPIPE, &zign, (struct sigstruct_name *) 0);
        zign.sighandler_el = acatch;
        sigact_routine(SIGALRM, &zign, (struct sigstruct_name *) 0);
#else
        signal(SIGPIPE, SIG_IGN);
        signal(SIGALRM, acatch);
#endif

        /* Initiate child process.  */

        childproc();

        if  (Network_ok)  {
                /* Set up remote lock semaphore.
                   Attach to other people
                   Send details of my jobs to other people
                   Start up net monitor process */

                init_remotelock();
                attach_hosts();
                net_initvsync();
                net_initjsync();        /* Also splats ci file across */
                netmonitor();
        }
        nfreport($E{Scheduler started});
        process();
        return  0;              /* process never returns, but the compiler doesn't know it */
}
