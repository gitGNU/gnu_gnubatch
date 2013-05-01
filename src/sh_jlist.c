/* sh_jlist.c -- scheduler job handling

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
#include <sys/sem.h>
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
#ifdef  HAVE_SYS_UTIME_H
#include <sys/utime.h>
#elif   defined(HAVE_UTIME_H)
#include <utime.h>
#endif
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
#include "btuser.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "files.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "ecodes.h"
#include "sh_ext.h"
#include "jobsave.h"

#define MARGIN  10              /* Factor to consider it late */
#define MIGHTASWELL     3       /* Get on with it if this few seconds */

#if     INITNJOBS > 1000
#define DEFAULT_NJOBS   1000
#else
#define DEFAULT_NJOBS   INITNJOBS
#endif
#define MIN_NJOBS       10

static  int     jfilefd;
static  char    jfilename[] = JFILE;

#ifdef  USING_FLOCK

void  setjhold(const int typ)
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
                        panic($E{Lock error jobs});
        }
}

void  lockjobs()
{
        setjhold(F_WRLCK);
}

void  unlockjobs()
{
#ifdef  USING_MMAP
        msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        setjhold(F_UNLCK);
}
#else

/* Semaphore structures for jobs */

static  struct  sembuf
jw[2] = {{      JQ_READING,     0,      0 },
        {       JQ_FIDDLE,      -1,     0 }},
ju[1] = {{      JQ_FIDDLE,      1,      0 }};

void  lockjobs()
{
        for  (;;)  {
                if  (semop(Sem_chan, jw, 2) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                panic($E{Semaphore error probably undo});
        }
}

void  unlockjobs()
{
#ifdef  USING_MMAP
        msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        for  (;;)  {
                if  (semop(Sem_chan, ju, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                panic($E{Semaphore error probably undo});
        }
}
#endif

unsigned        being_started;  /* Number of jobs being started */

extern int  stringsch(BtjobRef, BtjobRef);

static SHORT  round_short(float flt)
{
        if  (flt < 0.0)  {
                flt -= .5;
                if  (flt < -32768.0)
                        return  -32768;
        }
        else  {
                flt += .5;
                if  (flt > 32767.0)
                        return  32767;
        }
        return  (SHORT) flt;
}

static void  free_rest(unsigned jind, const unsigned njbs)
{
        HashBtjob       *jj = &Job_seg.jlist[jind];

        while  (jind < njbs)  {
                jj->j.h.bj_job = 0;
                jj->nxt_pid_hash = jj->prv_pid_hash =
                        jj->nxt_jno_hash = jj->prv_jno_hash =
                        jj->nxt_jid_hash = jj->prv_jid_hash =
                        jj->q_prv = JOBHASHEND;
                jj->q_nxt = Job_seg.dptr->js_freech;
                Job_seg.dptr->js_freech = jind;
                jind++;
                jj++;
        }
}

/* Create/open job file. Read it into memory if it exists.  (This
   routine used to be called openjfile but lint etc choked over
   having 2 similarly-named routines one in the library).  */

void  creatjfile(LONG jsize)
{
        HashBtjob       *jj;
        BtjobRef        jp;
        unsigned  char  flags_ok;
        int             Maxjobs, jcnt, jccnt;
        unsigned        hashval, jind, nxtj, prevind;
        vhash_t         vi;
        jident          jid;
        struct  stat    sbuf;
#if     defined(HAVE_UTIME_H) || defined(HAVE_SYS_UTIME_H)
        struct  utimbuf tj;
#else
        struct  {  time_t  actime, modtime; }   tj;
#endif
        struct  Jsave   inj;

#ifndef USING_MMAP
        Job_seg.inf.base = SHMID + JSHMOFF + envselect_value;
#endif

        if  ((jfilefd = open(jfilename, O_RDWR|O_CREAT, CMODE)) < 0)  {
                print_error($E{Panic couldnt create job file});
                do_exit(E_JOBQ);
        }

#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(jfilefd, Daemuid, Daemgid);
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(jfilename, Daemuid, Daemgid);
#endif
        fcntl(jfilefd, F_SETFD, 1);

        if  (jsize < MIN_NJOBS)
                jsize = jsize == 0? MIN_NJOBS: DEFAULT_NJOBS;

        /* Initialise shared memory segment to size of file, rounding up.  */

#ifndef USING_MMAP
#ifdef  USING_FLOCK
        if  ((Job_seg.inf.lockfd = open(JLOCK_FILE, O_CREAT|O_TRUNC|O_RDWR, 0600)) < 0)
                goto  fail;
#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(Job_seg.inf.lockfd, Daemuid, Daemgid);
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(JLOCK_FILE, Daemuid, Daemgid);
#endif
        fcntl(Job_seg.inf.lockfd, F_SETFD, 1);
#endif
        lockjobs();
#endif
        fstat(jfilefd, &sbuf);
        Maxjobs = sbuf.st_size / sizeof(struct Jsave);
        if  (Maxjobs < jsize)
                Maxjobs = jsize;

        Job_seg.inf.reqsize = Maxjobs * sizeof(HashBtjob) + 3*JOBHASHMOD*sizeof(USHORT) + sizeof(struct jshm_hdr) + sizeof(USHORT);
        if  (!gshmchan(&Job_seg.inf, JSHMOFF))  {
#if !defined(USING_MMAP) && defined(USING_FLOCK)
        fail:
#endif
                print_error($E{Panic trouble with job shm});
                do_exit(E_JOBQ);
        }

#ifdef  USING_MMAP
        lockjobs();
#endif
#ifdef  HAVE_MEMCPY
        /* Force contents of shm segment to -1 to aid finding bugs */
        memset(Job_seg.inf.seg, 255, Job_seg.inf.segsize);
#endif

        /* If kernel gave us different than we asked for, adjust Maxjobs accordingly.  */

        Maxjobs = (Job_seg.inf.segsize - sizeof(struct jshm_hdr) - 3*JOBHASHMOD*sizeof(USHORT) - sizeof(USHORT)) / sizeof(HashBtjob);

        Job_seg.dptr = (struct jshm_hdr *) Job_seg.inf.seg;
        Job_seg.hashp_pid = (USHORT *) (Job_seg.inf.seg + sizeof(struct jshm_hdr));
        Job_seg.hashp_jno = (USHORT *) ((char *) Job_seg.hashp_pid + JOBHASHMOD*sizeof(USHORT));
        Job_seg.hashp_jid = (USHORT *) ((char *) Job_seg.hashp_jno + JOBHASHMOD*sizeof(USHORT));
        Job_seg.jlist = (HashBtjob *) ((char *) Job_seg.hashp_jid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));

        /* Initialise hash tables, Initialise pointers to head and tail of queue and free chain.  */

        for  (jcnt = 0;  jcnt < JOBHASHMOD;  jcnt++)
                Job_seg.hashp_pid[jcnt] = Job_seg.hashp_jno[jcnt] = Job_seg.hashp_jid[jcnt] = JOBHASHEND;

        Job_seg.dptr->js_q_head = Job_seg.dptr->js_q_tail = Job_seg.dptr->js_freech = JOBHASHEND;

        Job_seg.Njobs = 0;
        jj = &Job_seg.jlist[0];
        jid.hostid = 0L;
        jid.slotno = 0;
        jind = 0;

        flags_ok = Network_ok? (BJ_WRT|BJ_MAIL|BJ_NOADVIFERR|BJ_EXPORT|BJ_REMRUNNABLE): (BJ_WRT|BJ_MAIL|BJ_NOADVIFERR);

        /* Read in the job queue file.  */

        tj.actime = time(&tj.modtime);

        while  (read(jfilefd, (char *) &inj, sizeof(struct Jsave)) == sizeof(struct Jsave))  {
                if  (inj.sj_job == 0)
                        continue;

                /* Skip over jobs we've lost the job file for.
                   At the same time touch the file so we can see any
                   dead wood files if we do ls -l.  */

                if  (utime(mkspid(SPNAM, inj.sj_job), &tj) < 0)
                        continue;

                /* Copy over stuff into shared memory job structure,
                   expanding out names NB - Someday we must fix
                   bj_orighostid and allow
                   BJ_CLIENTJOB/BJ_ROAMUSER in bj_jflags.  */

                jp = &jj->j;
                jp->h.bj_job = inj.sj_job;
                jp->h.bj_time = inj.sj_time;
                jp->h.bj_stime = inj.sj_stime;
                jp->h.bj_etime = inj.sj_etime;
                if  ((jp->h.bj_progress = inj.sj_progress) >= BJP_STARTUP1)
                        jp->h.bj_progress = BJP_ABORTED;
                jp->h.bj_pri = inj.sj_pri;
                jp->h.bj_wpri = inj.sj_wpri;
                jp->h.bj_pid = 0;
                jp->h.bj_orighostid = 0; /* To fix sometime */
                jp->h.bj_hostid = 0;
                jp->h.bj_runhostid = 0;
                jp->h.bj_slotno = jind;
                jp->h.bj_ll = inj.sj_ll;
                jp->h.bj_umask = inj.sj_umask;
                jp->h.bj_nredirs = inj.sj_nredirs;
                jp->h.bj_nargs = inj.sj_nargs;
                jp->h.bj_nenv = inj.sj_nenv;
                strcpy(jp->h.bj_cmdinterp, inj.sj_cmdinterp);

                /* Precautions in case cmd int file zapped */

                if  (validate_ci(jp->h.bj_cmdinterp) < 0)
                        strcpy(jp->h.bj_cmdinterp, Ci_list[CI_STDSHELL].ci_name);
                jp->h.bj_jflags = inj.sj_jflags & flags_ok;
                jp->h.bj_jrunflags = 0;
                jp->h.bj_title = inj.sj_title;
                jp->h.bj_direct = inj.sj_direct;
                jp->h.bj_runtime = inj.sj_runtime;
                jp->h.bj_autoksig = inj.sj_autoksig;
                jp->h.bj_runon = inj.sj_runon;
                jp->h.bj_deltime = inj.sj_deltime;
                jp->h.bj_padding = 0;
                jp->h.bj_mode = inj.sj_mode;
                for  (jcnt = jccnt = 0;  jcnt < MAXCVARS;  jcnt++)  {
                        struct  Sjcond  *js = &inj.sj_conds[jcnt];
                        if  ((jp->h.bj_conds[jccnt].bjc_compar = js->sjc_compar) == C_UNUSED)
                                break;
                        if  (!Network_ok  &&  js->sjc_varind.sv_hostid != 0)
                                continue;
                        if  ((vi = lookvar(&js->sjc_varind)) < 0L)  {
                                /* Just elide variables with errors
                                   due to invalid host ids.  */
                                if  (js->sjc_varind.sv_hostid != 0)
                                        continue;
                                else  {
                                        disp_str = js->sjc_varind.sv_name;
                                        print_error($E{Invalid cvariable in saved job});
                                        continue;
                                }
                        }
                        jp->h.bj_conds[jccnt].bjc_iscrit = js->sjc_crit;
                        jp->h.bj_conds[jccnt].bjc_varind = vi;
                        jp->h.bj_conds[jccnt].bjc_value = js->sjc_value;
                        jccnt++;
                }
                for  (jcnt = jccnt = 0;  jcnt < MAXSEVARS;  jcnt++)  {
                        struct  Sjass   *js = &inj.sj_asses[jcnt];
                        if  ((jp->h.bj_asses[jccnt].bja_op = js->sja_op) == BJA_NONE)
                                break;
                        if  (!Network_ok  &&  js->sja_varind.sv_hostid != 0)
                                continue;
                        if  ((vi = lookvar(&js->sja_varind)) < 0L)  {
                                if  (js->sja_varind.sv_hostid == 0)  {
                                        disp_str = js->sja_varind.sv_name;
                                        print_error($E{Invalid variable in saved job});
                                }
                                continue;
                        }
                        jp->h.bj_asses[jccnt].bja_iscrit = js->sja_crit;
                        jp->h.bj_asses[jccnt].bja_flags = js->sja_flags;
                        jp->h.bj_asses[jccnt].bja_varind = vi;
                        jp->h.bj_asses[jccnt].bja_con = js->sja_con;
                        jccnt++;
                }
                jp->h.bj_times = inj.sj_times;
                jp->h.bj_ulimit = inj.sj_ulimit;
                jp->h.bj_redirs = inj.sj_redirs;
                jp->h.bj_env = inj.sj_env;
                jp->h.bj_arg =  inj.sj_arg;
                jp->h.bj_lastexit = 0;
                jp->h.bj_exits= inj.sj_exits;
                BLOCK_COPY(jp->bj_space, inj.sj_space, JOBSPACE);

                /* Now insert into hash tables for job number and jid, and put on Q */

                jj->nxt_pid_hash = jj->prv_pid_hash = jj->prv_jno_hash = jj->prv_jid_hash = jj->q_nxt = JOBHASHEND;

                hashval = jno_jhash(jp->h.bj_job);
                prevind = jj->nxt_jno_hash = Job_seg.hashp_jno[hashval];
                Job_seg.hashp_jno[hashval] = jind;
                if  (prevind != JOBHASHEND)
                        Job_seg.jlist[prevind].prv_jno_hash = jind;

                hashval = jid_jhash(&jid);
                prevind = jj->nxt_jid_hash = Job_seg.hashp_jid[hashval];
                Job_seg.hashp_jid[hashval] = jind;
                if  (prevind != JOBHASHEND)
                        Job_seg.jlist[prevind].prv_jid_hash = jind;

                nxtj = jj->q_prv = Job_seg.dptr->js_q_tail;
                Job_seg.dptr->js_q_tail = jind;
                if  (nxtj == JOBHASHEND)
                        Job_seg.dptr->js_q_head = jind;
                else
                        Job_seg.jlist[nxtj].q_nxt = jind;

                Job_seg.Njobs++;
                jj++;
                jid.slotno++;
                jind++;
        }

        free_rest(jind, Maxjobs);

        /* Set up header structure.  */

        Job_seg.dptr->js_type = TY_ISJOB;
        Job_seg.dptr->js_nxtid = 0;
        Job_seg.dptr->js_njobs = Job_seg.Njobs;
        Job_seg.dptr->js_maxjobs = Maxjobs;
        Job_seg.dptr->js_lastwrite = sbuf.st_mtime;

        /* The following statement OR the one like it in
           attach_hosts() won't do anything useful depending upon
           which is called first see main().  Put in both places so
           that we can shuffle the call order to get the best effect.  */

        Job_seg.dptr->js_viewport = vportnum;
        Job_seg.dptr->js_serial = 1;
        unlockjobs();
}

/* Rewrite job file.
   If the number of jobs has shrunk since the last
   time, squash it up (by recreating the file, as vanilla UNIX
   doesn't have a "truncate file" syscall [why not?]).  */

void  rewrjq()
{
        int     jcnt;
#ifndef HAVE_FTRUNCATE
        int     fjobs, countjobs;
#endif
        unsigned        jind;
        HashBtjob       *hjp;
        BtjobRef        jp;
        struct  Jsave   Outj;
        struct  flock   wlock;

        wlock.l_type = F_WRLCK;
        wlock.l_whence = 0;
        wlock.l_start = 0L;
        wlock.l_len = 0L;

#ifdef  HAVE_FTRUNCATE
        while  (fcntl(jfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        panic($E{Panic couldnt lock job file});
        }
        Ignored_error = ftruncate(jfilefd, 0L);
        lseek(jfilefd, 0L, 0);
#else
        fjobs = lseek(jfilefd, 0L, 2) / sizeof(struct Jsave);

        countjobs = 0;
        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                hjp = &Job_seg.jlist[jind];
                if  (!hjp->j.h.bj_hostid)
                        countjobs++;
                jind = hjp->q_nxt;
        }

        if  (countjobs < fjobs)  {
                close(jfilefd);
                unlink(jfilename);
                if  ((jfilefd = open(jfilename, O_RDWR|O_CREAT, CMODE)) < 0)
                        panic($E{Panic couldnt create job file});
                fcntl(jfilefd, F_SETFD, 1);
#ifdef  HAVE_FCHOWN
                if  (Daemuid != ROOTID)
                        Ignored_error = fchown(jfilefd, Daemuid, Daemgid);
#else
                if  (Daemuid != ROOTID)
                        Ignored_error = chown(jfilename, Daemuid, Daemgid);
#endif
        }
        else
                lseek(jfilefd, 0L, 0);
        /* Note that there is a race if there's no ftruncate */
        while  (fcntl(jfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        panic($E{Panic couldnt lock job file});
        }
#endif /* !HAVE_FTRUNCATE */

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                hjp = &Job_seg.jlist[jind];
                jind = hjp->q_nxt;
                jp = &hjp->j;
                if  (jp->h.bj_hostid != 0)
                        continue;
                Outj.sj_job = jp->h.bj_job;
                Outj.sj_time = jp->h.bj_time;
                Outj.sj_stime = jp->h.bj_stime;
                Outj.sj_etime = jp->h.bj_etime;
                Outj.sj_progress = jp->h.bj_progress;
                Outj.sj_pri = jp->h.bj_pri;
                Outj.sj_wpri = jp->h.bj_wpri;
                Outj.sj_ll = jp->h.bj_ll;
                Outj.sj_umask = jp->h.bj_umask;
                Outj.sj_nredirs = jp->h.bj_nredirs,
                Outj.sj_nargs = jp->h.bj_nargs,
                Outj.sj_nenv = jp->h.bj_nenv;
                Outj.sj_jflags = jp->h.bj_jflags;
                Outj.sj_title = jp->h.bj_title;
                Outj.sj_direct = jp->h.bj_direct;
                Outj.sj_runtime = jp->h.bj_runtime;
                Outj.sj_autoksig = jp->h.bj_autoksig;
                Outj.sj_runon = jp->h.bj_runon;
                Outj.sj_deltime = jp->h.bj_deltime;
                strcpy(Outj.sj_cmdinterp, jp->h.bj_cmdinterp);
                Outj.sj_mode = jp->h.bj_mode;
                for  (jcnt = 0;  jcnt < MAXCVARS;  jcnt++)  {
                        struct  Sjcond  *js = &Outj.sj_conds[jcnt];
                        JcondRef        jcp = &jp->h.bj_conds[jcnt];
                        BtvarRef        vp;
                        if  ((js->sjc_compar = jcp->bjc_compar) == C_UNUSED)
                                break;
                        js->sjc_crit = jcp->bjc_iscrit;
                        vp = &Var_seg.vlist[jcp->bjc_varind].Vent;
                        strncpy(js->sjc_varind.sv_name, vp->var_name, BTV_NAME);
                        js->sjc_varind.sv_name[BTV_NAME] = '\0';
                        js->sjc_varind.sv_uid = vp->var_mode.o_uid;
                        js->sjc_varind.sv_gid = vp->var_mode.o_gid;
                        js->sjc_varind.sv_hostid = vp->var_id.hostid;
                        js->sjc_value = jcp->bjc_value;
                }
                for  (jcnt = 0;  jcnt < MAXSEVARS;  jcnt++)  {
                        struct  Sjass   *js = &Outj.sj_asses[jcnt];
                        JassRef         jap = &jp->h.bj_asses[jcnt];
                        BtvarRef        vp;
                        if  ((js->sja_op = jap->bja_op) == BJA_NONE)
                                break;
                        js->sja_crit = jap->bja_iscrit;
                        vp = &Var_seg.vlist[jap->bja_varind].Vent;
                        strncpy(js->sja_varind.sv_name, vp->var_name, BTV_NAME);
                        js->sja_varind.sv_name[BTV_NAME] = '\0';
                        js->sja_varind.sv_uid = vp->var_mode.o_uid;
                        js->sja_varind.sv_gid = vp->var_mode.o_gid;
                        js->sja_varind.sv_hostid = vp->var_id.hostid;
                        js->sja_flags = jap->bja_flags;
                        js->sja_con = jap->bja_con;
                }
                Outj.sj_times = jp->h.bj_times;
                Outj.sj_ulimit = jp->h.bj_ulimit;
                Outj.sj_redirs = jp->h.bj_redirs;
                Outj.sj_env = jp->h.bj_env;
                Outj.sj_arg = jp->h.bj_arg;
                Outj.sj_exits = jp->h.bj_exits;
                BLOCK_COPY(Outj.sj_space, jp->bj_space, JOBSPACE);
                Ignored_error = write(jfilefd, (char *)&Outj, sizeof(struct Jsave));
        }
        time(&Job_seg.dptr->js_lastwrite);

        wlock.l_type = F_UNLCK;
        while  (fcntl(jfilefd, F_SETLKW, &wlock) < 0)  {
                if  (errno != EINTR)
                        panic($E{Panic couldnt lock job file});
        }
}

/* Reallocate job shared segment if it grows too big.  We do this by
   allocating a new shared memory segment, and planting where we've
   gone in the old one. With memory mapped files we just make the file
   bigger */

int  growjseg()
{
        char            *newseg;
        unsigned        Oldmaxj = Job_seg.dptr->js_maxjobs;
        unsigned        Maxjobs = Oldmaxj + INCNJOBS;
        unsigned        Njobs = Job_seg.dptr->js_njobs;
        struct  jshm_hdr*newdptr;
        USHORT  *newhp, *newhjno, *newhjid;
        HashBtjob       *newlist;

#ifdef  USING_MMAP

        Job_seg.inf.reqsize = Maxjobs * sizeof(HashBtjob) + 3*JOBHASHMOD*sizeof(USHORT) + sizeof(struct jshm_hdr) + sizeof(USHORT);
        gshmchan(&Job_seg.inf, JSHMOFF); /* Panics if it can't grow */
        newseg = Job_seg.inf.seg;
        Maxjobs = (Job_seg.inf.segsize - sizeof(struct jshm_hdr) - 3*JOBHASHMOD*sizeof(USHORT) - sizeof(USHORT)) / sizeof(HashBtjob);

        /* We still have the same physical file so we don't need to copy.
           However it might have wound up at a different virt address */
#else
        struct btshm_info  new_info;
        new_info = Job_seg.inf;

        new_info.reqsize = Maxjobs * sizeof(HashBtjob) + 3*JOBHASHMOD*sizeof(USHORT) + sizeof(struct jshm_hdr) + sizeof(USHORT);
        if  (!gshmchan(&new_info, JSHMOFF))  {
                shmctl(new_info.chan, IPC_RMID, (struct shmid_ds *) 0);
                return  -1;
        }
        Maxjobs = (new_info.segsize - sizeof(struct jshm_hdr) - 3*JOBHASHMOD*sizeof(USHORT) - sizeof(USHORT)) / sizeof(HashBtjob);
        if  (Maxjobs <= Oldmaxj)  {
                shmctl(new_info.chan, IPC_RMID, (struct shmid_ds *) 0);
                return  -1;
        }
        newseg = new_info.seg;
        Job_seg.dptr->js_nxtid = Job_seg.inf.base;
#endif

        newdptr = (struct jshm_hdr *) newseg;
        newhp = (USHORT *) (newseg + sizeof(struct jshm_hdr));
        newhjno = (USHORT *) ((char *) newhp + JOBHASHMOD*sizeof(USHORT));
        newhjid = (USHORT *) ((char *) newhjno + JOBHASHMOD*sizeof(USHORT));
        newlist = (HashBtjob *) ((char *) newhjid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));

#ifdef  USING_MMAP
        newdptr->js_serial++;
#else
        newdptr->js_lastwrite = Job_seg.dptr->js_lastwrite;
        newdptr->js_serial = Job_seg.dptr->js_serial + 1;
        newdptr->js_type = TY_ISJOB;
        newdptr->js_nxtid = 0;

        /* Copy over stuff from old segment.  Use a block copy to
           preserve position.  The first one is a horrible cheat!!!!  */

        BLOCK_COPY(newhp, Job_seg.hashp_pid, 3 * sizeof(USHORT) * JOBHASHMOD); /* 3 at once!!!! */
        BLOCK_COPY(newlist, Job_seg.jlist, sizeof(HashBtjob) * Oldmaxj);
        newdptr->js_viewport = Job_seg.dptr->js_viewport;
        newdptr->js_q_head = Job_seg.dptr->js_q_head;
        newdptr->js_q_tail = Job_seg.dptr->js_q_tail;
        newdptr->js_freech = Job_seg.dptr->js_freech;

        /* Detach and remove old segment.  */

        shmdt(Job_seg.inf.seg);
        shmctl(Job_seg.inf.chan, IPC_RMID, (struct shmid_ds *) 0);
        Job_seg.inf = new_info;
#endif
        Job_seg.dptr = newdptr;
        Job_seg.hashp_pid = newhp;
        Job_seg.hashp_jno = newhjno;
        Job_seg.hashp_jid = newhjid;
        Job_seg.jlist = newlist;

        Job_seg.dptr->js_njobs = Njobs;
        Job_seg.dptr->js_maxjobs = Maxjobs;
        free_rest(Oldmaxj, Maxjobs);
#if     defined(USING_MMAP) && defined(MS_ASYNC)
        msync(newseg, Job_seg.inf.segsize, MS_ASYNC|MS_INVALIDATE);
#endif
        tellchild(O_JREMAP, 0L);
        if  (Netm_pid)
                kill(Netm_pid, NETSHUTSIG);
        jqpend++;
        qchanges++;
        return  1;
}

static void  takeoff_q(const unsigned jind)
{
        unsigned  nxtind = Job_seg.jlist[jind].q_nxt;
        unsigned  prvind = Job_seg.jlist[jind].q_prv;

        if  (nxtind == JOBHASHEND)
                Job_seg.dptr->js_q_tail = prvind;
        else
                Job_seg.jlist[nxtind].q_prv = prvind;
        if  (prvind == JOBHASHEND)
                Job_seg.dptr->js_q_head = nxtind;
        else
                Job_seg.jlist[prvind].q_nxt = nxtind;
}

static void  puton_q(const unsigned jind, const unsigned after)
{
        unsigned  nxt;

        Job_seg.jlist[jind].q_prv = after;

        if  (after == JOBHASHEND)  {
                /* At beginning of q. */
                nxt = Job_seg.dptr->js_q_head;
                Job_seg.dptr->js_q_head = jind;
        }
        else  {
                nxt = Job_seg.jlist[after].q_nxt;
                Job_seg.jlist[after].q_nxt = jind;
        }

        Job_seg.jlist[jind].q_nxt = nxt;
        if  (nxt == JOBHASHEND)
                Job_seg.dptr->js_q_tail = jind;
        else
                Job_seg.jlist[nxt].q_prv = jind;
}

/* Remove a job from the queue, assuming jobs already locked */

static void  dequeue_nolock(const unsigned jind)
{
        HashBtjob       *jhp = &Job_seg.jlist[jind];
        unsigned        prevind, nextind;

        /* Delete from job number chain.  */

        prevind = jhp->prv_jno_hash;
        nextind = jhp->nxt_jno_hash;
        if  (prevind == JOBHASHEND)
                Job_seg.hashp_jno[jno_jhash(jhp->j.h.bj_job)] = nextind;
        else
                Job_seg.jlist[prevind].nxt_jno_hash = nextind;

        if  (nextind != JOBHASHEND)
                Job_seg.jlist[nextind].prv_jno_hash = prevind;

        /* Delete from job ident chain.  */

        prevind = jhp->prv_jid_hash;
        nextind = jhp->nxt_jid_hash;

        if  (prevind == JOBHASHEND)  {
                jident  jid;
                jid.hostid = jhp->j.h.bj_hostid;
                jid.slotno = jhp->j.h.bj_slotno;
                Job_seg.hashp_jid[jid_jhash(&jid)] = nextind;
        }
        else
                Job_seg.jlist[prevind].nxt_jid_hash = nextind;

        if  (nextind != JOBHASHEND)
                Job_seg.jlist[nextind].prv_jid_hash = prevind;

        /* Delete from queue and add to free chain.  */

        jhp->j.h.bj_job = 0;
        takeoff_q(jind);

        jhp->q_prv = JOBHASHEND;
        jhp->q_nxt = Job_seg.dptr->js_freech;
        Job_seg.dptr->js_freech = jind;
        Job_seg.dptr->js_njobs--;
        Job_seg.dptr->js_serial++;
        jqpend++;
        qchanges++;
}

void  notify_stat_change(BtjobhRef jp)
{
        if  (jp->bj_jflags & BJ_EXPORT)  {
                if  (jp->bj_hostid)
                        job_rrchstat(jp);
                else
                        job_statbroadcast(jp);
        }
}

/* Return >=1 if job contains any variables referenced from outside.
   Return 0 if none
   Return -1 if job refers critically to non-available remote variables.  */

static int  listrems(BtjobhRef jp, const int caswell)
{
        unsigned  cnt;
        int     hadh = 0;

        /* If we are only doing assignments we don't have to worry
           about variables in conditions.  */

        if  (caswell)
                for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                        JcondRef  jcp = &jp->bj_conds[cnt];
                        BtvarRef  vp;
                        if  (jcp->bjc_compar == C_UNUSED)
                                break;
                        vp = &Var_seg.vlist[jcp->bjc_varind].Vent;
                        if  (vp->var_id.hostid)  {
                                if  (vp->var_flags & VF_SKELETON)  {
                                        if  (jcp->bjc_iscrit & CCRIT_NORUN)
                                                return  -1;
                                        continue;
                                }

                                /* If "cluster" var, see if local version is exported */

                                if  (vp->var_flags & VF_CLUSTER)  {
                                        vhash_t myvind = myvariable(vp, &jp->bj_mode, BTM_READ);
                                        if  (myvind < 0)  {
                                                jcp->bjc_iscrit |= myvind < -1? CCRIT_NOPERM: CCRIT_NONAVAIL;
                                                return  -1;
                                        }
                                        jcp->bjc_iscrit &= ~(CCRIT_NOPERM|CCRIT_NONAVAIL);
                                        if  (!(Var_seg.vlist[myvind].Vent.var_flags & VF_EXPORT))
                                                continue;
                                }
                                hadh++;
                        }
                        else  if  (vp->var_flags & VF_EXPORT  &&  vp->var_type != VT_MACHNAME)
                                hadh++;
                }

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                JassRef  jap = &jp->bj_asses[cnt];
                BtvarRef  vp;
                if  (jap->bja_op == BJA_NONE)
                        break;
                vp = &Var_seg.vlist[jap->bja_varind].Vent;
                if  (vp->var_id.hostid)  {
                        if  (vp->var_flags & VF_SKELETON)  {
                                if  (jap->bja_iscrit & ACRIT_NORUN)
                                        return  -1;
                                continue;
                        }
                        if  (vp->var_flags & VF_CLUSTER)  {
                                vhash_t myvind = myvariable(vp, &jp->bj_mode, jap->bja_op == BJA_ASSIGN? BTM_READ: BTM_READ|BTM_WRITE);
                                if  (myvind < 0)  {
                                        jap->bja_iscrit |= myvind < -1? ACRIT_NOPERM: ACRIT_NONAVAIL;
                                        return  -1;
                                }
                                jap->bja_iscrit &= ~(ACRIT_NOPERM|ACRIT_NONAVAIL);
                                if  (!(Var_seg.vlist[myvind].Vent.var_flags & VF_EXPORT))
                                        continue;
                        }
                        hadh++;
                }
                else  if  (vp->var_flags & VF_EXPORT)
                        hadh++;
        }

        return  hadh;
}

/* Find given job in queue or return -1.  */

int  findj(BtjobRef jp)
{
        unsigned        jind = Job_seg.hashp_jno[jno_jhash(jp->h.bj_job)];

        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                if  (jhp->j.h.bj_job == jp->h.bj_job  &&  jhp->j.h.bj_hostid == jp->h.bj_hostid)
                        return  (int) jind;
                jind = jhp->nxt_jno_hash;
        }
        return  -1;
}

/* Find job by job ident */

int  findj_by_jid(jident *jid)
{
        unsigned        jind = Job_seg.hashp_jid[jid_jhash(jid)];

        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                if  (jhp->j.h.bj_hostid == jid->hostid  &&  jhp->j.h.bj_slotno == jid->slotno)
                        return  (int) jind;
                jind = jhp->nxt_jid_hash;
        }
        return  -1;
}

/* Check that a job doesn't implicitly reference variables the user
   actually can't read/write.  At the same time we verify that a
   non-exported job doesn't access non-local variables. (Turn
   this off) Return 0 if ok otherwise error code.  */

int  checkvarsok(ShreqRef sr, BtjobhRef jp)
{
        int     i;
        BtvarRef        vr;

        for  (i = 0;  i < MAXCVARS;  i++)  {
                if  (jp->bj_conds[i].bjc_compar == C_UNUSED)
                        break;
                if  (!(vr = locvarind(sr, jp->bj_conds[i].bjc_varind)))
                        return  J_VNEXIST;
                if  (jp->bj_jflags & BJ_EXPORT)  {
                        if  (!(vr->var_flags & VF_EXPORT)  &&  vr->var_type != VT_MACHNAME)
                                return  J_LOCVINEXPJ;
                }
#ifdef  NOREMVINLOCJ
                else  if  (vr->var_id.hostid != 0)
                        return  J_REMVINLOCJ;
#endif
                if  (!(vr->var_flags & VF_SKELETON || shmpermitted(sr, &vr->var_mode, BTM_READ)))
                        return  J_VNOPERM;
        }
        for  (i = 0;  i < MAXSEVARS;  i++)  {
                if  (jp->bj_asses[i].bja_op == BJA_NONE)
                        break;
                if  (!(vr = locvarind(sr, jp->bj_asses[i].bja_varind)))
                        return  J_VNEXIST;

                if  (vr->var_type)  {
                        if  (vr->var_flags & VF_READONLY)
                                return  J_SYSVAR;
                        if  (vr->var_flags & VF_STRINGONLY)  {
                                if  (jp->bj_asses[i].bja_con.const_type != CON_STRING)
                                        return  J_SYSVTYPE;
                        }
                        else  if  (vr->var_flags & VF_LONGONLY)  {
                                if  (jp->bj_asses[i].bja_con.const_type != CON_LONG)
                                        return  J_SYSVTYPE;
                        }
                }

                if  (jp->bj_jflags & BJ_EXPORT)  {
                        if  (!(vr->var_flags & VF_EXPORT))
                                return  J_LOCVINEXPJ;
                }
#ifdef  NOREMVINLOCJ
                else  if  (vr->var_id.hostid != 0)
                        return  J_REMVINLOCJ;
#endif

                /* For ones which just set it we only insist on write
                   permission. However += ops imply reading the
                   original value */

                if  (!(vr->var_flags & VF_SKELETON || shmpermitted(sr, &vr->var_mode, (unsigned) (jp->bj_asses[i].bja_op <= BJA_ASSIGN? BTM_WRITE: BTM_READ|BTM_WRITE))))
                        return  J_VNOPERM;
        }
        return  0;
}

/* When a remote host comes up, check any other jobs on other
   machines which may contain conditions and assignments for
   that host and fix. */

void  deskel_jobs(const netid_t newhostid)
{
        unsigned  jind;

        lockjobs();

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                BtjobhRef  jp = &Job_seg.jlist[jind].j.h;
                jind = Job_seg.jlist[jind].q_nxt;
                if  (!(jp->bj_jrunflags & BJ_SKELHOLD))
                        continue;
                if  (!jp->bj_hostid) /* Don't worry about local jobs */
                        continue;
                if  (jp->bj_hostid == newhostid) /* Or ones we've just loaded */
                        continue;
                sync_single(jp->bj_hostid, jp->bj_slotno);
        }
        unlockjobs();
}

/* When a variable ceases to be a skeleton, because we have discovered
   the real thing when the other end comes up, run over the
   various jobs, and re-check the permissions on conditions and
   assignments.  If something is no longer permitted, remove it
   from the jobs as I don't know what else we can sensibly do.  */

void  deskeletonise(const vhash_t vind)
{
        unsigned  jind;

        lockjobs();

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                BtjobhRef  jp = &Job_seg.jlist[jind].j.h;
                int     in;
                Shreq   dummy;

                jind = Job_seg.jlist[jind].q_nxt;
                if  (jp->bj_hostid) /* Don't worry about remote jobs */
                        continue;

                dummy.uuid = jp->bj_mode.o_uid;
                dummy.ugid = jp->bj_mode.o_gid;
                for  (in = 0;  in < MAXCVARS;  in++)  {
                        if  (jp->bj_conds[in].bjc_compar == C_UNUSED)
                                break;
                        if  (jp->bj_conds[in].bjc_varind == vind)  {
                                if  (!shmpermitted(&dummy, &Var_seg.vlist[vind].Vent.var_mode, BTM_READ))  {
                                        int  kn;
                                        Job_seg.dptr->js_serial++;
                                        jqpend++;
                                        qchanges++;
                                        for  (kn = in + 1;  kn < MAXCVARS;  kn++)  {
                                                jp->bj_conds[kn-1] = jp->bj_conds[kn];
                                                if  (jp->bj_conds[kn].bjc_compar == C_UNUSED)
                                                        break;
                                        }
                                }
                        }
                }
                for  (in = 0;  in < MAXSEVARS;  in++)  {
                        if  (jp->bj_asses[in].bja_op == BJA_NONE)
                                break;
                        if  (jp->bj_asses[in].bja_varind == vind)  {
                                BtvarRef  vp = &Var_seg.vlist[vind].Vent;
                                int     kn;
                                if  (vp->var_type)  {
                                        if  (vp->var_flags & VF_READONLY)
                                                goto  ngood;
                                        if  (vp->var_flags & VF_STRINGONLY)  {
                                                if  (jp->bj_asses[in].bja_con.const_type != CON_STRING)
                                                        goto  ngood;
                                        }
                                        else  if  (vp->var_flags & VF_LONGONLY)  {
                                                if  (jp->bj_asses[in].bja_con.const_type != CON_LONG)
                                                        goto  ngood;
                                        }
                                }
                                if  (shmpermitted(&dummy, &vp->var_mode,
                                                 (unsigned) (jp->bj_asses[in].bja_op <= BJA_ASSIGN? BTM_WRITE: BTM_READ|BTM_WRITE)))
                                        continue;
                        ngood:
                                Job_seg.dptr->js_serial++;
                                jqpend++;
                                qchanges++;
                                for  (kn = in + 1;  kn < MAXSEVARS;  kn++)  {
                                        jp->bj_asses[kn-1] = jp->bj_asses[kn];
                                        if  (jp->bj_asses[kn].bja_op == BJA_NONE)
                                                break;
                                }
                        }
                }
                jp->bj_jrunflags &= ~BJ_SKELHOLD;
        }
        unlockjobs();
}

/* Before a variable is deleted ensure that a job doesn't refer to it.
   Remotes only applies when we are only interested in remote jobs */

int  reqdforjobs(const vhash_t vind, const int remotesonly)
{
        unsigned  jind = Job_seg.dptr->js_q_head;

        while  (jind != JOBHASHEND)  {
                BtjobhRef  jp = &Job_seg.jlist[jind].j.h;
                int     in;

                jind = Job_seg.jlist[jind].q_nxt;

                if  (remotesonly && jp->bj_hostid == 0)
                        continue;

                for  (in = 0;  in < MAXCVARS;  in++)  {
                        if  (jp->bj_conds[in].bjc_compar == C_UNUSED)
                                break;
                        if  (jp->bj_conds[in].bjc_varind == vind)
                                return  1;
                }

                for  (in = 0;  in < MAXSEVARS;  in++)  {
                        if  (jp->bj_asses[in].bja_op == BJA_NONE)
                                break;
                        if  (jp->bj_asses[in].bja_varind == vind)
                                return  1;
                }
        }

        return  0;
}

/* Tell existing machines about all our luvly jobs after initial startup.  */

void  net_initjsync()
{
        unsigned  jind = Job_seg.dptr->js_q_head;
        unsigned  lumpcount = 0;

        while  (jind != JOBHASHEND)  {
                BtjobRef  jp = &Job_seg.jlist[jind].j;
                if  (jp->h.bj_hostid == 0  &&  jp->h.bj_jflags & BJ_EXPORT)
                        job_broadcast(jp, J_CREATE);
                if  ((++lumpcount % lumpsize) == 0)
                        sleep(lumpwait);
                jind = Job_seg.jlist[jind].q_nxt;
        }
}

/* Clear traces of jobs connected with dying machine.  The machine
   must have been first deleted from the host lists.  */

void  net_jclear(const netid_t hostid)
{
        unsigned  jind = Job_seg.dptr->js_q_head;

        lockjobs();

        while  (jind != JOBHASHEND)  {
                HashBtjob  *hjp = &Job_seg.jlist[jind];
                BtjobhRef  jp = &hjp->j.h;
                unsigned        nxtjind = hjp->q_nxt; /* Save it as it might get scrambled */

                if  (jp->bj_hostid == hostid)  {

                        /* Job belongs to dying machine.  We have to
                           worry about it if we are running it.  */

                        if  (jp->bj_progress >= BJP_STARTUP1  &&  jp->bj_runhostid == 0)  {
                                murder(jp);
                                jp->bj_jrunflags |= BJ_HOSTDIED;
                        }
                        else
                                dequeue_nolock(jind);
                }
                else  if  (jp->bj_hostid == 0  &&  jp->bj_runhostid == hostid  &&  (jp->bj_progress >= BJP_STARTUP1 || jp->bj_jrunflags & BJ_PROPOSED))  {

                        /* Job belongs to this machine, but it was running on the machine
                           which has died. Just mark as aborted and broadcast.
                           I don't know what to do about variables
                           I'll mark the job as specially aborted.  */

                        jp->bj_progress = BJP_ABORTED;
                        jp->bj_jrunflags = 0;
                        job_statbroadcast(jp);
                        logjob(&Job_seg.jlist[jind].j, $S{log code netabort}, 0L, jp->bj_mode.o_uid, jp->bj_mode.o_gid);
                        Job_seg.dptr->js_serial++;
                        jqpend++;
                        qchanges++;
                }
                jind = nxtjind;
        }
        unlockjobs();
}

/* Enqueue request on spool queue.  */

int  enqueue(ShreqRef sr, BtjobRef jp)
{
        unsigned  jind, hashval, nxtind, lastind;
        int             ret;
        HashBtjob       *jhp;
        BtjobRef        dest;
        jident          jid;
        float           fwpri;

        /* Skip checks if job comes from another machine - the checks
           are its problem not mine.  */

        if  (!sr->hostid)  {
                if  (!ppermitted(sr->uuid, BTM_CREATE))
                        return  J_NOPRIV;

                if  (!checkminmode(&jp->h.bj_mode))
                        return  J_MINPRIV;

                if  ((ret = checkvarsok(sr, &jp->h)))
                        return  ret;
        }

        /* Ok ready to go.....  */

        lockjobs();

        if  (Job_seg.dptr->js_njobs >= Job_seg.dptr->js_maxjobs  &&  growjseg() < 0)  {
                unlockjobs();
                return  J_FULLUP;
        }

        /* Take it off the free chain, stuff it in and insert default stuff.  */

        jind = Job_seg.dptr->js_freech;
        jhp = &Job_seg.jlist[jind];
        dest = &jhp->j;
        Job_seg.dptr->js_freech = jhp->q_nxt;
        *dest = *jp;

        if  (!Network_ok)
                dest->h.bj_jflags &= (BJ_WRT|BJ_MAIL|BJ_NOADVIFERR);

        /* For remote jobs ownership gets transmogrified in sh_pack */

        if  (!(dest->h.bj_hostid = sr->hostid))  {
                dest->h.bj_mode.o_uid = dest->h.bj_mode.c_uid = sr->uuid;
                dest->h.bj_mode.o_gid = dest->h.bj_mode.c_gid = sr->ugid;
                strncpy(dest->h.bj_mode.o_user, prin_uname((uid_t) sr->uuid), UIDSIZE);
                strncpy(dest->h.bj_mode.c_user, dest->h.bj_mode.o_user, UIDSIZE);
                strncpy(dest->h.bj_mode.o_group, prin_gname((gid_t) sr->ugid), UIDSIZE);
                strncpy(dest->h.bj_mode.c_group, dest->h.bj_mode.o_group, UIDSIZE);
                dest->h.bj_mode.o_user[UIDSIZE] = '\0';
                dest->h.bj_mode.c_user[UIDSIZE] = '\0';
                dest->h.bj_mode.o_group[UIDSIZE] = '\0';
                dest->h.bj_mode.c_group[UIDSIZE] = '\0';
                dest->h.bj_slotno = jind;
                dest->h.bj_runhostid = 0;
        }

        /* Now insert into job number hash chain, and job ident hash chain.  */

        hashval = jno_jhash(dest->h.bj_job);
        nxtind = Job_seg.hashp_jno[hashval];
        jhp->nxt_jno_hash = nxtind;
        jhp->prv_jno_hash = JOBHASHEND;
        if  (nxtind != JOBHASHEND)
                Job_seg.jlist[nxtind].prv_jno_hash = jind;
        Job_seg.hashp_jno[hashval] = jind;

        jid.hostid = dest->h.bj_hostid;
        jid.slotno = dest->h.bj_slotno;
        hashval = jid_jhash(&jid);
        nxtind = Job_seg.hashp_jid[hashval];
        jhp->nxt_jid_hash = nxtind;
        jhp->prv_jid_hash = JOBHASHEND;
        if  (nxtind != JOBHASHEND)
                Job_seg.jlist[nxtind].prv_jid_hash = jind;
        Job_seg.hashp_jid[hashval] = jind;

        /* In case of funnies */

        if  (!dest->h.bj_times.tc_istime  &&  dest->h.bj_times.tc_repeat > TC_RETAIN)
                dest->h.bj_times.tc_repeat = TC_DELETE;

        /* Initialise working priority to current priority.  Find
           higher priority job by moving up the queue,
           decrementing priority until we stop.  */

        fwpri = (float) dest->h.bj_pri;
        lastind = Job_seg.dptr->js_q_tail;

        if  (dest->h.bj_hostid)  {

                /* For non-local jobs take job submission time into
                   account...  This is not terribly accurate...  */

                while  (lastind != JOBHASHEND)  {
                        HashBtjob       *lhp = &Job_seg.jlist[lastind];
                        if  ((float) lhp->j.h.bj_pri >= fwpri)
                                break;
                        if  (lhp->j.h.bj_time < dest->h.bj_time)
                                fwpri -= pri_decrement;
                        lastind = lhp->q_prv;
                }
        }
        else  {
                while  (lastind != JOBHASHEND)  {
                        HashBtjob       *lhp = &Job_seg.jlist[lastind];
                        if  ((float) lhp->j.h.bj_pri >= fwpri)
                                break;
                        fwpri -= pri_decrement;
                        lastind = lhp->q_prv;
                }
        }

        dest->h.bj_wpri = round_short(fwpri);
        puton_q(jind, lastind);
        Job_seg.dptr->js_njobs++;
        Job_seg.dptr->js_serial++;
        jqpend++;
        qchanges++;

        /* Tell the glad tidings to the rest of the world */

        if  (dest->h.bj_hostid == 0  &&  dest->h.bj_jflags & BJ_EXPORT)
                job_broadcast(dest, J_CREATE);
        unlockjobs();
        if  (dest->h.bj_hostid == 0)
                logjob(dest, $S{log code create}, dest->h.bj_orighostid, sr->uuid, sr->ugid);
        return  J_OK;
}

/* Worry about priority changes.  */

static void  pri_diff(const unsigned jind, BtjobRef jb)
{
        int     pdiff;
        float   fwpri;
        BtjobhRef       ojb = &Job_seg.jlist[jind].j.h;

        if  ((pdiff = jb->h.bj_pri - ojb->bj_pri) == 0)
                return;

        ojb->bj_pri = jb->h.bj_pri;
        ojb->bj_wpri += pdiff;
        fwpri = ojb->bj_wpri;

        if  (pdiff > 0)  {      /*  Going up ....  */
                unsigned  pind = Job_seg.jlist[jind].q_prv;
                while  (pind != JOBHASHEND)  {
                        if  ((float) Job_seg.jlist[pind].j.h.bj_pri >= fwpri)
                                break;
                        fwpri -= pri_decrement;
                        pind = Job_seg.jlist[pind].q_prv;
                }
                takeoff_q(jind);
                puton_q(jind, pind);
        }
        else  {                 /* Going down.... */
                unsigned  cind = jind, nind;

                while  ((nind = Job_seg.jlist[cind].q_nxt) != JOBHASHEND)  {
                        if  ((float) Job_seg.jlist[nind].j.h.bj_pri <= fwpri)
                                break;
                        fwpri += pri_decrement;
                        cind = nind;
                }
                if  (cind != jind)  {
                        takeoff_q(jind);
                        puton_q(jind, cind);
                }
        }
        ojb->bj_wpri = round_short(fwpri);
}

/* Change details of job in queue.  The only allowed changes are to
   the priority, load level, mail/write flags, title, conditions
   and assignments and time control.  Mode is done in a separate
   routine.  If the job is starting up or shutting down, attempts
   to change the priority, load level, title, condition and
   assignment are ignored.  */

int  chjob(ShreqRef sr, BtjobRef jb)
{
        BtjobRef        ojb;
        int     jind, ret, sch;
        ULONG   remreq;

        if  ((jind = findj(jb)) < 0)
                return  J_NEXIST;

        ojb = &Job_seg.jlist[jind].j;
        if  (ojb->h.bj_ll != jb->h.bj_ll  &&  !ppermitted(sr->uuid, BTM_SPCREATE))
                return  J_NOPRIV;
        if  (!shmpermitted(sr, &ojb->h.bj_mode, BTM_WRITE))
                return  J_NOPERM;
        if  ((ret = checkvarsok(sr, &jb->h)))
                return  ret;

        /* Remote job - that's its problem Compare strings and send
           shorter message if none changed */

        if  (ojb->h.bj_hostid)  {
                if  (ojb->h.bj_jrunflags & BJ_SKELHOLD)
                        return  J_ISSKEL;
                return  job_sendupdate(ojb, jb, sr, stringsch(ojb, jb)? J_CHANGED: J_HCHANGED);
        }

        /* If the job is being executed, we only allow changes to the
           mail/write bits and the time control Don't lock yet as
           we only changing "trivial" things.  */

        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        ojb->h.bj_jflags &= ~(BJ_WRT|BJ_MAIL|BJ_NOADVIFERR);
        ojb->h.bj_jflags |= jb->h.bj_jflags & (BJ_WRT|BJ_MAIL|BJ_NOADVIFERR);
        ojb->h.bj_times = jb->h.bj_times;

        /* In case of funnies */

        if  (!ojb->h.bj_times.tc_istime  &&  ojb->h.bj_times.tc_repeat > TC_RETAIN)
                ojb->h.bj_times.tc_repeat = TC_DELETE;

        /* If starting up or shutting down, ignore all other changes */

        if  (ojb->h.bj_progress >= BJP_STARTUP1  &&  ojb->h.bj_progress != BJP_RUNNING)  {
                if  (ojb->h.bj_jflags & BJ_EXPORT)
                        job_hbroadcast(&ojb->h, J_BHCHANGED);
                logjob(ojb, $S{log code chdetails}, sr->hostid, sr->uuid, sr->ugid);
                return  J_OK;
        }

        lockjobs();

        /* Worst case is where we send the whole shooting match */

        remreq = (sch = stringsch(ojb, jb))? J_BCHANGED: J_BHCHANGED;

        if  (!Network_ok)
                jb->h.bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);

        /* We only allow changes on jobs actually running which can't
           affect the status of the network connections.  */

        if  (ojb->h.bj_progress < BJP_STARTUP1)  {

                /* If job wasn't previously exported and now is or
                   vice versa change the request into a "new job"
                   or "delete" one as appropriate */

                if  ((ojb->h.bj_jflags & BJ_EXPORT) != (jb->h.bj_jflags & BJ_EXPORT))
                        remreq = jb->h.bj_jflags & BJ_EXPORT? J_CREATE: J_DELETED;
                else  if  (!(ojb->h.bj_jflags & BJ_EXPORT))
                        remreq = 0;

                ojb->h.bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);
                ojb->h.bj_jflags |= jb->h.bj_jflags & (BJ_EXPORT|BJ_REMRUNNABLE);

                /* Turn off SKELHOLD flag because we are about to
                   reassign the conditions.  */

                ojb->h.bj_jrunflags &= ~BJ_SKELHOLD;
                if  (jb->h.bj_progress < BJP_STARTUP1)
                        ojb->h.bj_progress = jb->h.bj_progress;

                ojb->h.bj_ll = jb->h.bj_ll;
                ojb->h.bj_umask = jb->h.bj_umask;
                ojb->h.bj_ulimit = jb->h.bj_ulimit;
                ojb->h.bj_exits = jb->h.bj_exits;
                ojb->h.bj_runtime = jb->h.bj_runtime;
                ojb->h.bj_autoksig = jb->h.bj_autoksig;
                ojb->h.bj_runon = jb->h.bj_runon;
                ojb->h.bj_deltime = jb->h.bj_deltime;
                strcpy(ojb->h.bj_cmdinterp, jb->h.bj_cmdinterp);
                BLOCK_COPY(ojb->h.bj_conds, jb->h.bj_conds, MAXCVARS * sizeof(Jcond));
                BLOCK_COPY(ojb->h.bj_asses, jb->h.bj_asses, MAXSEVARS * sizeof(Jass));

                pri_diff(jind, jb);
        }

        /* New feature - we allow string changes whilst the job is
           running, but not whilst it is starting up.  */

        if  (sch)  {
                ojb->h.bj_title = jb->h.bj_title;
                ojb->h.bj_direct = jb->h.bj_direct;
                ojb->h.bj_nredirs = jb->h.bj_nredirs;
                ojb->h.bj_nargs = jb->h.bj_nargs;
                ojb->h.bj_nenv = jb->h.bj_nenv;
                ojb->h.bj_redirs = jb->h.bj_redirs;
                ojb->h.bj_env = jb->h.bj_env;
                ojb->h.bj_arg = jb->h.bj_arg;
                BLOCK_COPY(ojb->bj_space, jb->bj_space, JOBSPACE);
        }

       /* Avoid sending the whole caboodle if we can.  */

        switch  (remreq)  {
        case  J_BCHANGED:
        case  J_CREATE:
                job_broadcast(ojb, remreq);
        case  0:                /* 0 if job started and finished local */
                break;
        case  J_BHCHANGED:
                job_hbroadcast(&ojb->h, remreq);
                break;
        case  J_DELETED:
                job_imessbcast(&ojb->h, J_DELETED, 0L);
                break;
        }

        unlockjobs();
        logjob(ojb, $S{log code chdetails}, sr->hostid, sr->uuid, sr->ugid);
        return  J_OK;
}

/* Note changes of details of remote job.  In cases where no job
   strings have changed we only get sent a valid jb->h and the
   bj_space will be garbage. This is denoted by the code being
   J_BHCHANGED rather than J_BCHANGED.  */

int  remchjob(ShreqRef sr, BtjobRef jb)
{
        BtjobRef        ojb;
        int     jind;

        if  ((jind = findj(jb)) < 0)
                return  J_NEXIST;

        ojb = &Job_seg.jlist[jind].j;

        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        /* Don't really need the masking below, but we might have mixed versions for a while */
        ojb->h.bj_jflags &= ~(BJ_WRT|BJ_MAIL|BJ_NOADVIFERR|BJ_EXPORT|BJ_REMRUNNABLE|BJ_CLIENTJOB|BJ_ROAMUSER);
        ojb->h.bj_jflags |= jb->h.bj_jflags & (BJ_WRT|BJ_MAIL|BJ_NOADVIFERR|BJ_EXPORT|BJ_REMRUNNABLE|BJ_CLIENTJOB|BJ_ROAMUSER);
        ojb->h.bj_jrunflags = jb->h.bj_jrunflags & BJ_SKELHOLD; /* In case unref vars */
        ojb->h.bj_times = jb->h.bj_times;
        ojb->h.bj_progress = jb->h.bj_progress;

        /* If running, ignore all other changes */

        if  (ojb->h.bj_progress >= BJP_STARTUP1)  {
                if  (ojb->h.bj_hostid == 0)
                        job_hbroadcast(&ojb->h, J_BHCHANGED);
                return  J_OK;
        }

        lockjobs();

        /* If job was proposed the load level will already have been
           added to the current load level, so we'd better adjust it.  */

        if  (ojb->h.bj_ll != jb->h.bj_ll)  {
                if  (ojb->h.bj_jrunflags & BJ_PROPOSED)
                        adjust_ll((LONG) jb->h.bj_ll - (LONG) ojb->h.bj_ll);
                ojb->h.bj_ll = jb->h.bj_ll;
        }
        ojb->h.bj_runtime = jb->h.bj_runtime;
        ojb->h.bj_autoksig = jb->h.bj_autoksig;
        ojb->h.bj_runon = jb->h.bj_runon;
        ojb->h.bj_deltime = jb->h.bj_deltime;
        ojb->h.bj_umask = jb->h.bj_umask;
        ojb->h.bj_ulimit = jb->h.bj_ulimit;
        ojb->h.bj_exits = jb->h.bj_exits;
        strcpy(ojb->h.bj_cmdinterp, jb->h.bj_cmdinterp);
        BLOCK_COPY(ojb->h.bj_conds, jb->h.bj_conds, MAXCVARS * sizeof(Jcond));
        BLOCK_COPY(ojb->h.bj_asses, jb->h.bj_asses, MAXSEVARS * sizeof(Jass));

        if  (sr->mcode == J_BCHANGED  ||  sr->mcode == J_CHANGED)  {
                ojb->h.bj_title = jb->h.bj_title;
                ojb->h.bj_direct = jb->h.bj_direct;
                ojb->h.bj_nredirs = jb->h.bj_nredirs;
                ojb->h.bj_nargs = jb->h.bj_nargs;
                ojb->h.bj_nenv = jb->h.bj_nenv;
                ojb->h.bj_redirs = jb->h.bj_redirs;
                ojb->h.bj_env = jb->h.bj_env;
                ojb->h.bj_arg = jb->h.bj_arg;
                BLOCK_COPY(ojb->bj_space, jb->bj_space, JOBSPACE);
        }

        pri_diff(jind, jb);
        unlockjobs();
        if  (ojb->h.bj_hostid == 0)  {
                logjob(ojb, $S{log code chdetails}, sr->hostid, sr->uuid, sr->ugid);
                if  (sr->mcode == J_CHANGED)
                        job_broadcast(ojb, J_BCHANGED);
                else
                        job_hbroadcast(&ojb->h, J_BHCHANGED);
        }
        return  J_OK;
}

int  dstadj(ShreqRef sr, struct adjstr *adj)
{
        unsigned  jind = Job_seg.dptr->js_q_head;

        if  (!ppermitted(sr->uuid, BTM_WADMIN))
                return  J_NOPERM;

        while  (jind != JOBHASHEND)  {
                BtjobRef        jobp = &Job_seg.jlist[jind].j;
                BtjobhRef       jp = &jobp->h;
                jind = Job_seg.jlist[jind].q_nxt;
                if  (jp->bj_hostid  &&  !sr->param)
                        continue;
                if  (!jp->bj_times.tc_istime)
                        continue;
                if  (jp->bj_times.tc_nexttime < adj->sh_starttime || jp->bj_times.tc_nexttime > adj->sh_endtime)
                        continue;
                /* Don't mess with repeat times less than days */
                if  (jp->bj_times.tc_repeat == TC_MINUTES || jp->bj_times.tc_repeat == TC_HOURS)
                        continue;
                jp->bj_times.tc_nexttime += adj->sh_adjust;
                notify_stat_change(jp);
                jqpend++;
                qchanges++;
                Job_seg.dptr->js_serial++;
                logjob(jobp, $S{log code dstadj}, sr->hostid, sr->uuid, sr->ugid);
        }
        return  J_OK;
}

int  remjchmog(BtjobRef jb)
{
        int     jind;

        if  ((jind = findj(jb)) < 0)
                return  J_NEXIST;

        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        Job_seg.jlist[jind].j.h.bj_mode = jb->h.bj_mode;
        return  J_OK;
}

/* Change modes of job in queue.  */

int  job_chmod(ShreqRef sr, BtjobRef jb)
{
        BtjobRef        ojb;
        int     jind;

        if  ((jind = findj(jb)) < 0)
                return  J_NEXIST;
        ojb = &Job_seg.jlist[jind].j;

        /* Remote jobs - send on request and return whatever it returns.  */

        if  (ojb->h.bj_hostid)
                return  job_sendmdupdate(ojb, jb, sr);

        if  (!shmpermitted(sr, &ojb->h.bj_mode, BTM_WRMODE))
                return  J_NOPERM;
        if  (!checkminmode(&jb->h.bj_mode))
                return  J_MINPRIV;
        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        ojb->h.bj_mode.u_flags = jb->h.bj_mode.u_flags;
        ojb->h.bj_mode.g_flags = jb->h.bj_mode.g_flags;
        ojb->h.bj_mode.o_flags = jb->h.bj_mode.o_flags;
        logjob(ojb, $S{log code chmod}, sr->hostid, sr->uuid, sr->ugid);
        if  (ojb->h.bj_jflags & BJ_EXPORT)
                job_hbroadcast(&ojb->h, J_CHMOGED);
        return  J_OK;
}

int  job_chown(ShreqRef sr, jident *jid)
{
        int     jind;
        BtjobRef        targ;

        if  ((jind = findj_by_jid(jid)) < 0)
                return  J_NEXIST;

        targ = &Job_seg.jlist[jind].j;

        /* Remote jobs - send on request and return whatever it returns.  */

        if  (targ->h.bj_hostid)
                return  job_sendugupdate(targ, sr);

        if  (ppermitted(sr->uuid, BTM_WADMIN))  {
                if  (!(shmpermitted(sr, &targ->h.bj_mode, BTM_UGIVE) || shmpermitted(sr, &targ->h.bj_mode, BTM_UTAKE)))
                        return  J_NOPERM;
                targ->h.bj_mode.o_uid = targ->h.bj_mode.c_uid = sr->param;
                strncpy(targ->h.bj_mode.o_user, prin_uname((uid_t) sr->param), UIDSIZE);
                strncpy(targ->h.bj_mode.c_user, targ->h.bj_mode.o_user, UIDSIZE);
        }
        else  if  (targ->h.bj_mode.c_uid == targ->h.bj_mode.o_uid)  {
                if  (!shmpermitted(sr, &targ->h.bj_mode, BTM_UGIVE))
                        return  J_NOPERM;

                /* Remember the proposed recipient */

                targ->h.bj_mode.c_uid = sr->param;
                strncpy(targ->h.bj_mode.c_user, prin_uname((uid_t) sr->param), UIDSIZE);
        }
        else  {
                /* Refuse to accept it unless I'm the proposed recipient.  */

                if  (targ->h.bj_mode.c_uid != sr->uuid  ||  !shmpermitted(sr, &targ->h.bj_mode, BTM_UTAKE))
                        return  J_NOPERM;

                targ->h.bj_mode.o_uid = sr->param;
                strncpy(targ->h.bj_mode.o_user, targ->h.bj_mode.c_user, UIDSIZE);
        }

        Job_seg.dptr->js_serial++;
        qchanges++;
        jqpend++;
        logjob(targ, $S{log code chown}, sr->hostid, sr->uuid, sr->ugid);
        if  (targ->h.bj_jflags & BJ_EXPORT)
                job_hbroadcast(&targ->h, J_CHMOGED);
        return  J_OK;
}

int  job_chgrp(ShreqRef sr, jident *jid)
{
        int     jind;
        BtjobRef        targ;

        if  ((jind = findj_by_jid(jid)) < 0)
                return  J_NEXIST;

        targ = &Job_seg.jlist[jind].j;

        /* Remote jobs - send on request and return whatever it returns.  */

        if  (targ->h.bj_hostid)
                return  job_sendugupdate(targ, sr);

        if  (ppermitted(sr->uuid, BTM_WADMIN))  {
                if  (!(shmpermitted(sr, &targ->h.bj_mode, BTM_GGIVE) || shmpermitted(sr, &targ->h.bj_mode, BTM_GTAKE)))
                        return  J_NOPERM;
                targ->h.bj_mode.c_gid = targ->h.bj_mode.o_gid = sr->param;
                strncpy(targ->h.bj_mode.o_group, prin_gname((gid_t) sr->param), UIDSIZE);
                strncpy(targ->h.bj_mode.c_group, targ->h.bj_mode.o_group, UIDSIZE);
        }
        else  if  (targ->h.bj_mode.c_gid == targ->h.bj_mode.o_gid)  {
                if  (!shmpermitted(sr, &targ->h.bj_mode, BTM_GGIVE))
                        return  J_NOPERM;

                /* Remember the proposed recipient */

                targ->h.bj_mode.c_gid = sr->param;
                strncpy(targ->h.bj_mode.c_group, prin_gname((gid_t) sr->param), UIDSIZE);
        }
        else  {
                /* Refuse to accept it unless I'm the proposed recipient.  */

                if  (targ->h.bj_mode.c_gid != sr->ugid || !shmpermitted(sr, &targ->h.bj_mode, BTM_GTAKE))
                        return  J_NOPERM;

                targ->h.bj_mode.o_gid = sr->param;
                strncpy(targ->h.bj_mode.o_group, targ->h.bj_mode.c_group, UIDSIZE);
        }

        Job_seg.dptr->js_serial++;
        qchanges++;
        jqpend++;
        logjob(targ, $S{log code chgrp}, sr->hostid, sr->uuid, sr->ugid);
        if  (targ->h.bj_jflags & BJ_EXPORT)
                job_hbroadcast(&targ->h, J_CHMOGED);
        return  J_OK;
}

/* Remove job from queue.  Remove file and tell the world.  */

void  dequeue(unsigned jn)
{
        BtjobhRef       jp = &Job_seg.jlist[jn].j.h;

        lockjobs();
        if  (jp->bj_hostid == 0)  {
                if  (jp->bj_jflags & BJ_EXPORT)
                        job_imessbcast(jp, J_DELETED, 0L);
                unlink(mkspid(SPNAM, jp->bj_job));
        }
        dequeue_nolock(jn);
        unlockjobs();
}

/* Move the job to the back of the queue and float up again according
   to priority rules This applies whoever the job belongs to.  */

void  back_of_queue(const unsigned jind)
{
        unsigned  lastind;
        float           fwpri;
        BtjobhRef       jp = &Job_seg.jlist[jind].j.h;

        lockjobs();
        takeoff_q(jind);
        fwpri = jp->bj_pri;
        lastind = Job_seg.dptr->js_q_tail;
        while  (lastind != JOBHASHEND)  {
                if  ((float) Job_seg.jlist[lastind].j.h.bj_pri >= fwpri)
                        break;
                fwpri -= pri_decrement;
                lastind = Job_seg.jlist[lastind].q_prv;
        }
        jp->bj_wpri = round_short(fwpri);
        puton_q(jind, lastind);
        Job_seg.dptr->js_serial++;
        unlockjobs();
        if  (jp->bj_hostid == 0  &&  jp->bj_jflags & BJ_EXPORT)
                job_imessbcast(jp, J_BOQ, 0L);
        jqpend++;
        qchanges++;
}

int  deljob(ShreqRef sr, jident *jid)
{
        BtjobRef        targ;
        int             jn;

        if  ((jn = findj_by_jid(jid)) < 0)
                return  J_NEXIST;

        targ = &Job_seg.jlist[jn].j;
        if  (targ->h.bj_hostid)
                return  job_message(targ->h.bj_hostid, &targ->h, sr);
        if  (!shmpermitted(sr, &targ->h.bj_mode, BTM_DELETE))
                return  J_NOPERM;
        if  (targ->h.bj_progress >= BJP_STARTUP1)
                return  J_ISRUNNING;
        logjob(targ, $S{log code delete}, sr->hostid, sr->uuid, sr->ugid);
        dequeue(jn);            /* Contains broadcast */
        return  J_OK;
}

/* Process remote job deleted notification */

void  job_deleted(jident *jid)
{
        BtjobhRef       targ;
        int             jn;

        if  ((jn = findj_by_jid(jid)) < 0)
                return;         /* Huh? */

        targ = &Job_seg.jlist[jn].j.h;

        /* A proposed job will have had its load level already added
           to the current load level.*/

        if  (targ->bj_jrunflags & BJ_PROPOSED)  {
                adjust_ll(-(LONG) targ->bj_ll);
                being_started--;
        }
        dequeue(jn);
}

/* Check job conditions and return 1 if held off 0 if ready to run Use
   quick lookup for value of varcomp.  Fourth column for undefined values.  */

const   unsigned  char  clook[6][4] = {
        { 1, 0, 1, 1 }, /*  C_EQ        */
        { 0, 1, 0, 1 }, /*  C_NE        */
        { 0, 1, 1, 1 }, /*  C_LT        */
        { 0, 0, 1, 1 }, /*  C_LE        */
        { 1, 1, 0, 1 }, /*  C_GT        */
        { 1, 0, 0, 1 }  /*  C_GE        */
};

/* Check conditions on job regardless of whose variables they are */

static int  condcheck(BtjobhRef jp)
{
        JcondRef  cr = jp->bj_conds;
        int     k;

        for  (k = MAXCVARS-1;  k >= 0;  cr++, k--)  {
                if  (cr->bjc_compar == C_UNUSED)
                        return  0;
                if  (is_skeleton(cr->bjc_varind))  {
                        if  (cr->bjc_iscrit & CCRIT_NORUN)
                                return  1;
                        continue;
                }
                if  (clook[cr->bjc_compar-1][varcomp(cr, jp)])
                        return  1;
        }
        return  0;
}

/* Check that given job wouldn't send user's run limit over the top */

int  ulcheck(BtjobhRef jp)
{
        unsigned        jind = Job_seg.dptr->js_q_head, sofar = 0;
        BtuserRef       g;

        while  (jind != JOBHASHEND)  {
                BtjobhRef       njp = &Job_seg.jlist[jind].j.h;
                if  (njp->bj_mode.o_uid == jp->bj_mode.o_uid  &&  njp->bj_progress >= BJP_STARTUP1  &&  njp->bj_runhostid == 0)
                        sofar += njp->bj_ll;
                jind = Job_seg.jlist[jind].q_nxt;
        }

        g = getbtuentry((uid_t) jp->bj_mode.o_uid);
        return  jp->bj_ll + sofar > g->btu_totll;
}

/* Deal with assignments at the beginning or the end of a job under
   the control of flag.  Each host does this believing that it
   will deliver the same answer so we don't have to set off a
   firestorm of messages to each host from each other one.  */

void  setasses(BtjobRef jp, unsigned flag, unsigned source, unsigned status, const netid_t setting_host)
{
        int             k, op;
        BtconRef        conp;
        BtvarRef        vp;
        JassRef         ja = jp->h.bj_asses;
        vhash_t         varind;
        Btcon           zer;

        for  (k = MAXSEVARS-1;  k >= 0;  ja++, k--)  {
                unsigned  jflags;

                if  ((op = ja->bja_op) == BJA_NONE)
                        return;

                jflags = op >= BJA_SEXIT? BJA_OK|BJA_ERROR|BJA_ABORT: ja->bja_flags;

                /* Skip if it doesn't apply to this case.  */

                if  ((jflags & flag) == 0)
                        continue;

                conp = &ja->bja_con;

                if  (jflags & BJA_REVERSE  &&  (flag & BJA_START) == 0)  {
                        static const unsigned char revop[] =
                        { BJA_NONE, BJA_ASSIGN, BJA_DECR, BJA_INCR,
                          BJA_DIV, BJA_MULT, BJA_MOD, BJA_SEXIT, BJA_SSIG };
                        if  (op == BJA_ASSIGN || op > BJA_SSIG)  {
                                if  ((zer.const_type = conp->const_type) == CON_LONG)
                                        zer.con_un.con_long = 0L;
                                else
                                        zer.con_un.con_string[0] = '\0';
                                conp = &zer;
                        }
                        else
                                op = revop[op];
                }
                varind = ja->bja_varind;
                vp = &Var_seg.vlist[varind].Vent;
                if  (vp->var_flags & VF_CLUSTER)  {

                        /* For cluster vars we don't try to do anything other than on
                           the machine which actually ran the job.
                           This goes for cluster vars belonging to this
                           machine as well even if they don't get transmogrified.  */

                        if  (setting_host != 0L)
                                continue;
                        if  (vp->var_id.hostid == 0L)
                                goto  contupd;
                        if  ((varind = myvariable(vp, &jp->h.bj_mode, op == BJA_ASSIGN? BTM_WRITE: BTM_READ|BTM_WRITE)) < 0)
                                continue;
                        vp = &Var_seg.vlist[varind].Vent; /* Remember which for later */
                        if  (vp->var_flags & VF_EXPORT)
                                goto  contupd;
                }
                vp = (BtvarRef) 0;      /* Don't need broadcast */
        contupd:
                switch  (op)  {
                case  BJA_SSIG:
                        zer.con_un.con_long = status & 0xff;
                        goto  erest;
                case  BJA_SEXIT:
                        zer.con_un.con_long = (status >> 8) & 0xff;
                erest:
                        zer.const_type = CON_LONG;
                        conp = &zer;
                case  BJA_ASSIGN:
                        jassvar(varind, conp, source, jp);
                        break;
                case  BJA_INCR:
                        jopvar(varind, conp, '+', source, jp);
                        break;
                case  BJA_DECR:
                        jopvar(varind, conp, '-', source, jp);
                        break;
                case  BJA_MULT:
                        jopvar(varind, conp, '*', source, jp);
                        break;
                case  BJA_DIV:
                        jopvar(varind, conp, '/', source, jp);
                        break;
                case  BJA_MOD:
                        jopvar(varind, conp, '%', source, jp);
                        break;
                }
                if  (vp)        /* Cluster var remembered */
                        var_broadcast(vp, V_ASSIGNED);
        }
}

/* Abort job.  */

int  doabort(ShreqRef sr, jident *jid)
{
        int     jn = findj_by_jid(jid);
        int     nrems = 0;
        BtjobRef        targ;
        BtjobhRef       targh;

        if  (jn < 0)
                return  J_NEXIST;

        targ = &Job_seg.jlist[jn].j;
        targh = &targ->h;

        if  (targh->bj_progress >= BJP_STARTUP1)  {
                if  (targh->bj_runhostid)
                        return  job_message(targh->bj_runhostid, targh, sr);
        }
        else  if  (targh->bj_hostid)
                return  job_message(targh->bj_hostid, targh, sr);

        if  (!shmpermitted(sr, &targh->bj_mode, BTM_KILL))
                return  J_NOPERM;

        if  (targh->bj_progress >= BJP_STARTUP1)  {
                if  (targh->bj_progress != BJP_FINISHED)  {
                        if  (targh->bj_progress == BJP_RUNNING)
                                jbabort(targh, (int) sr->param);
                        else  {
                                targh->bj_jrunflags |= BJ_PENDKILL;
                                targh->bj_lastexit = (USHORT) sr->param;
                                targh->bj_runhostid = targh->bj_hostid;
                        }
                        notify(targ, $PE{Job manually aborted}, 0);
                        logjob(targ, $S{log code kill}, sr->hostid, sr->uuid, sr->ugid);
                }
        }
        else  if  (targh->bj_progress != BJP_CANCELLED)  {
                int     oldp = targh->bj_progress;
                targh->bj_progress = BJP_CANCELLED;
                Job_seg.dptr->js_serial++;
                jqpend++;
                qchanges++;
                if  (oldp != BJP_ERROR  &&  oldp != BJP_ABORTED)  {
                        if  (targh->bj_jrunflags & BJ_PROPOSED)  {
                                targh->bj_jrunflags &= ~BJ_PROPOSED;
                                adjust_ll(-(LONG) targh->bj_ll);
                                being_started--;
                        }
                        nrems = listrems(targh, 0);
                        if  (nrems >= 0)  {
                                if  (nrems > 0)  {
                                        PIDTYPE ret;
                                        if  ((ret = forksafe()) < 0)  {
                                                nrems = 0;
                                                goto  skipit;
                                        }
                                        if  (ret != 0)
                                                return  J_OK;
                                        get_remotelock();
                                        lockvars();
                                        setasses(targ, BJA_CANCEL, $S{log source jcancel}, 0, 0L);
                                        remasses(targh, BJA_CANCEL, $S{log source jcancel}, 0);
                                        tellsched(J_RESCHED, 0L);
                                }
                                else
                                        setasses(targ, BJA_CANCEL, $S{log source jcancel}, 0, 0L);
                        }
                skipit:
                        logjob(targ, $S{log code cancel}, sr->hostid, sr->uuid, sr->ugid);
                        notify(targ, $PE{Job cancelled msg}, 0);
                }
                notify_stat_change(targh);
        }

        if  (nrems > 0)
                exit(0);
        return  J_OK;
}

/* Force job to go ahead */

int  doforce(ShreqRef sr, jident *jid, const int noadv)
{
        BtjobRef        targ;
        BtjobhRef       targh;
        int             jn;

        if  ((jn = findj_by_jid(jid)) < 0)
                return  J_NEXIST;

        targ = &Job_seg.jlist[jn].j;
        targh = &targ->h;
        if  (targh->bj_hostid)
                return  job_message(targh->bj_hostid, targh, sr);

        if  (!shmpermitted(sr, &targh->bj_mode, BTM_KILL))
                return  J_NOPERM;

        /* If the job is being executed or in some other state, go away */

        if  (targh->bj_progress != BJP_NONE)
                return  J_ISRUNNING;

        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        if  (noadv)  {
                logjob(targ, $S{log code forcena}, sr->hostid, sr->uuid, sr->ugid);
                targh->bj_jrunflags |= BJ_FORCE | BJ_FORCENA;
                if  (targh->bj_hostid == 0  &&  targh->bj_jflags & BJ_EXPORT)
                        job_imessbcast(targh, J_BFORCEDNA, 0L);
        }
        else  {
                logjob(targ, $S{log code force}, sr->hostid, sr->uuid, sr->ugid);
                targh->bj_jrunflags |= BJ_FORCE;
                targh->bj_jrunflags &= ~BJ_FORCENA;
                if  (targh->bj_hostid == 0  &&  targh->bj_jflags & BJ_EXPORT)
                        job_imessbcast(targh, J_BFORCED, 0L);
        }
        return  J_OK;
}

/* Note above applied to remote job.  */

void  forced(jident *jid, const int noadv)
{
        int     jn;
        BtjobhRef       targ;
        if  ((jn = findj_by_jid(jid)) < 0)
                return;         /* Huh?? */
        targ = &Job_seg.jlist[jn].j.h;
        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        targ->bj_jrunflags |= BJ_FORCE;
        if  (noadv)
                targ->bj_jrunflags |= BJ_FORCENA;
        else
                targ->bj_jrunflags &= ~BJ_FORCENA;
        return;
}

/* Note job has changed state (this should NOT be one of "my" jobs run on a
   remote machine which we now worry about in the following routine)  */

void  statchange(struct jstatusmsg *jm)
{
        int             jn;
        BtjobhRef       targh;

        if  ((jn = findj_by_jid(&jm->jid)) < 0)
                return;         /* Huh?? */

        targh = &Job_seg.jlist[jn].j.h;
        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;

        /* If we had a proposed flag, set by us because we proposed it,
           turn it off and restore the current load level which we used up whilst
           the proposal was going on.  */

        if  (targh->bj_jrunflags & BJ_PROPOSED)  {
                targh->bj_jrunflags &= ~BJ_PROPOSED;
                adjust_ll(-(LONG) targh->bj_ll);
                being_started--;
        }
        targh->bj_progress = jm->prog;
        targh->bj_runhostid = jm->prog >= BJP_STARTUP1? jm->runhost: targh->bj_hostid;
        targh->bj_times.tc_nexttime = jm->nexttime;
        targh->bj_lastexit = jm->lastexit;
        targh->bj_pid = jm->lastpid;
        targh->bj_jrunflags &= ~(BJ_FORCE | BJ_FORCENA);
        /* if  (targh->bj_hostid == 0) job_statbroadcast(targh);  "Cannot happen" */
}

/* This routine is invoked on the owning machine of a job which is being
   run remotely to reflect state changes. */

void  rrstatchange(const netid_t run_hostid, struct jstatusmsg *jm)
{
        int             jn;
        BtjobhRef       targh;

        if  ((jn = findj_by_jid(&jm->jid)) < 0)
                return;         /* Huh?? */

        targh = &Job_seg.jlist[jn].j.h;
        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        targh->bj_jrunflags &= ~(BJ_FORCE | BJ_FORCENA | BJ_PROPOSED);
        /* Copy all this stuff across without worrying whether it means anything */
        targh->bj_times.tc_nexttime = jm->nexttime;
        targh->bj_lastexit = jm->lastexit;
        targh->bj_pid = jm->lastpid;

        /* Now set progress code as per passed parameter.
           Correct problem encountered 3/12/03 with "one off" jobs
           in that they got deleted on the wrong machine. In such cases
           we do the deletion here, on the machine where the job is and
           let the "dequeue" code tell everyone it's gone. */

        if  ((targh->bj_progress = jm->prog) >= BJP_STARTUP1)
                targh->bj_runhostid = run_hostid;
        else  {
                if  (targh->bj_progress >= BJP_DONE  &&  targh->bj_progress <= BJP_ABORTED &&
                     (!targh->bj_times.tc_istime  ||  targh->bj_times.tc_repeat == TC_DELETE))  {
                        dequeue(jn);
                        return;
                }
                targh->bj_runhostid = 0; /* I.e. set back to me - my job */
        }
        job_statbroadcast(targh);
}

/* We think that we can start a job.  */

void  startup_job(const unsigned jind)
{
        BtjobRef        jp = &Job_seg.jlist[jind].j;
        BtjobhRef       jh = &jp->h;
        PIDTYPE         ret;
        int             nrems = 0;

        /* If job doesn't depend in any way on remote variables, just
           carry on and don't worry about remote locks.  */

        if  (!(jh->bj_jflags & BJ_EXPORT)  ||  (nrems = listrems(jh, 1)) <= 0)  {

                /* nrems is -1 if job depends upon inaccessible remote
                   vars, if so we mark it for when the variables appear.  */

                if  (nrems < 0)  {
                        jh->bj_jrunflags |= BJ_SKELHOLD;
                        if  (jh->bj_jrunflags & BJ_PROPOSED)  {
                                jh->bj_jrunflags &= ~BJ_PROPOSED;
                                adjust_ll(- (LONG) jh->bj_ll);
                        }
                        return;
                }

                /* Enter initial startup state Adjust load level to
                   reflect job being started.  Assign variables.  */

                jh->bj_progress = BJP_STARTUP1;
                jh->bj_runhostid = 0;
                notify_stat_change(jh);
                if  (jh->bj_jrunflags & BJ_PROPOSED)
                        jh->bj_jrunflags &= ~BJ_PROPOSED;
                else
                        adjust_ll((LONG) jh->bj_ll);

                switch  (clockvars())  {
                default:        /* Error case */
                        adjust_ll(-(LONG) jh->bj_ll);
                        jh->bj_progress = BJP_ERROR;
                        notify_stat_change(jh);
                        jh->bj_jrunflags = 0;
                        Job_seg.dptr->js_serial++;
                        qchanges++;
                        return;

                case  CLOCK_OK:

                        /* We locked variables successfully at the first attempt.
                           We cannot have had any variables changed since the last time
                           we looked so we don't worry about synchronising changes.  */

                        setasses(jp, BJA_START, $S{log source jstart}, 0, 0L);
                        unlockvars();
                        Job_seg.dptr->js_serial++;
                        jh->bj_progress = BJP_STARTUP2;
                        notify_stat_change(jh);
                        tellchild(J_START, jind);
                        qchanges++;
                        return;

                case  CLOCK_MADECHILD:
                        /* Parent half of process forked off to do locking.  */
                        Job_seg.dptr->js_serial++;
                        qchanges++;
                        return;

                case  CLOCK_AMCHILD:

                        /* Child half of process forked off to do locking.
                           We return when we have the lock.
                           However the variables might have changed since
                           we started, so we re-check.
                           Theory: jp still points to the right place as
                           the slot number stays fixed.  */

                        if  (condcheck(jh))  {
                                /* Condition no longer holds */
                                unlockvars();
                                jh->bj_progress = BJP_NONE;
                                notify_stat_change(jh);
                                jh->bj_jrunflags = 0;
                                Job_seg.dptr->js_serial++;
                                tellsched(J_RESCHED_NS, jind);
                        }
                        else  {
                                setasses(jp, BJA_START, $S{log source jstart}, 0, 0L);
                                unlockvars();
                                jh->bj_progress = BJP_STARTUP2;
                                notify_stat_change(jh);
                                Job_seg.dptr->js_serial++;
                                tellchild(J_START, jind);
                        }
                        exit(0);
                }
        }

        /* Our job does include references to jobs on other machines.
           We must negotiate with those machines to get our stuff
           through.  We fire off a task to do it with.  */

        jh->bj_progress = BJP_STARTUP1;
        jh->bj_jrunflags = 0;
        notify_stat_change(jh);

        /* Adjust load level on local jobs only.  This is because we
           adjusted the load level when we proposed to execute a remote job.  */

        if  (!jh->bj_hostid)
                adjust_ll((LONG) jh->bj_ll);
        Job_seg.dptr->js_serial++;
        qchanges++;
        if  ((ret = forksafe()) != 0)  {
                if  (ret < 0)  {
                        jh->bj_progress = BJP_ERROR;
                        jh->bj_jrunflags = 0;
                        notify_stat_change(jh);
                        adjust_ll(- (LONG) jh->bj_ll);
                }
                return;
        }

        /* Lock the network Lock the variables shm Now re-check the
           conditions in case something moved */

        get_remotelock();
        lockvars();
        if  (condcheck(jh))  {
                /* Condition no longer holds */
                unlockvars();
                lose_remotelock();
                jh->bj_progress = BJP_NONE;
                jh->bj_jrunflags = 0;
                jh->bj_runhostid = jh->bj_hostid;
                Job_seg.dptr->js_serial++;
                notify_stat_change(jh);
                tellsched(J_RESCHED_NS, jind);
        }
        else  {
                setasses(jp, BJA_START, $S{log source jstart}, 0, 0L);
                remasses(jh, BJA_START, $S{log source jstart}, 0);
                unlockvars();
                lose_remotelock();
                jh->bj_progress = BJP_STARTUP2;
                jh->bj_runhostid = 0;
                Job_seg.dptr->js_serial++;
                tellchild(J_START, jind);
                notify_stat_change(jh);
        }
        exit(0);
}

/* Propose to a remote machine that we'll run one of its jobs Thinks:
   maybe sometime we'll do some sort of fancy load-balancing.  */

static void  propose(const unsigned jind)
{
        BtjobhRef       jp = &Job_seg.jlist[jind].j.h;
        jp->bj_jrunflags |= BJ_PROPOSED;
        job_imessage(jp->bj_hostid, jp, J_PROPOSE, 0L);
        /* Add this to load level. We must remember to adjust the load
           level if the load level gets changed, or if the job is
           deleted or executed by someone else */
        adjust_ll((LONG) jp->bj_ll);
}

/* Other end of above - decide what to do about a proposal.  We don't
   reject it (at present) because our idea of the time differs
   from the other end's.  */

void  reply_propose(ShreqRef sr, jident *jid)
{
        int             jn;
        BtjobhRef       targh;

        if  ((jn = findj_by_jid(jid)) < 0)
                return;                 /* Don't reply at all we must have deleted it */
        targh = &Job_seg.jlist[jn].j.h;
        if  (targh->bj_progress >= BJP_DONE)
                return;                 /* Don't reply at all we must have changed status */

        /* If already proposed don't bother to reply as the other end
           will find out the bad news soon enough.  */

        if  (targh->bj_jrunflags & BJ_PROPOSED)
                return;

        targh->bj_jrunflags |= BJ_PROPOSED;
        targh->bj_runhostid = sr->hostid;
        job_imessage(sr->hostid, targh, J_PROPOK, 0L);
}

/* Final phase of propose process - note acceptance and start job.  */

void  propok(jident *jid)
{
        int     jn;
        if  ((jn = findj_by_jid(jid)) >= 0)
                startup_job(jn);
}

/* Reschedule all jobs and return the time to the next one due.  */

unsigned  resched()
{
        unsigned        jind, cind;
        BtjobhRef       jp;
        BtjobRef        jj;
        time_t          now = time((time_t *) 0);
        unsigned        minsecs = 0;

        jind = Job_seg.dptr->js_q_head;

        while  (jind != JOBHASHEND)  {
                cind = jind;
                jind = Job_seg.jlist[cind].q_nxt;
                jj = &Job_seg.jlist[cind].j;
                jp = &jj->h;

                if  (jp->bj_progress >= BJP_DONE)  {

                        /* See if we ought to delete this job.  */

                        if  (jp->bj_progress <= BJP_CANCELLED)  {
                                time_t  wh;

                                if  (jp->bj_hostid  ||  jp->bj_etime == 0  ||  jp->bj_deltime == 0)
                                        continue;

                                wh = jp->bj_etime + jp->bj_deltime * 3600L;
                                if  (wh <= now)  {
                                        logjob(jj, $S{log code autodel}, 0L, jp->bj_mode.o_uid, jp->bj_mode.o_gid);
                                        notify(jj, $PE{Job autodel msg}, 0);
                                        dequeue(cind);
                                        continue;
                                }
                                wh -= now;
                                if  (minsecs == 0  ||  minsecs > (unsigned) wh)
                                        minsecs = (unsigned) wh;
                                continue;
                        }

                        if  (jp->bj_progress >= BJP_ERROR)  {
                                time_t  wh;

                                /* Check for time limits. Only on jobs running on this
                                   machine with a time limit set.  */

                                if  (jp->bj_progress != BJP_RUNNING  ||  jp->bj_runhostid  ||  jp->bj_runtime == 0)
                                        continue;

                                wh = jp->bj_stime + jp->bj_runtime;
                                if  (wh > now)  { /* Not yet */
                                        wh -= now;
                                        if  (minsecs == 0  ||  minsecs > (unsigned) wh)
                                                minsecs = (unsigned) wh;
                                        continue;
                                }
                                wh += jp->bj_runon;

                                /* Kill with size 9s if no grace period or signal defined,
                                   or if run on past grace period.  */

                                if  (jp->bj_runon == 0  ||  now >= wh  ||
                                     jp->bj_autoksig == 0  ||  jp->bj_autoksig >= NSIG)  {
                                        if  (!(jp->bj_jrunflags & BJ_AUTOMURDER))  {
                                                jp->bj_jrunflags |= BJ_AUTOMURDER;
                                                logjob(jj, $S{log code runonkill}, 0L, jp->bj_mode.o_uid, jp->bj_mode.o_gid);
                                                notify(jj, $PE{Job final limit}, 0);
                                                murder(jp);
                                        }
                                        continue;
                                }

                                /* Set up grace period.  Apply softer boot.  */

                                wh -= now;
                                if  (minsecs == 0  ||  minsecs > (unsigned) wh)
                                        minsecs = (unsigned) wh;
                                if  (!(jp->bj_jrunflags & BJ_AUTOKILLED))  {
                                        jp->bj_jrunflags |= BJ_AUTOKILLED;
                                        logjob(jj, $S{log code runkill}, 0L, jp->bj_mode.o_uid, jp->bj_mode.o_gid);
                                        notify(jj, $PE{Job exceeded limit}, 0);
                                        jbabort(jp, (int) jp->bj_autoksig);
                                }
                                continue;
                        }

                        /* If we've done it once and we are not
                           repeating it, forget it.  */

                        if  (jp->bj_times.tc_repeat < TC_MINUTES)
                                continue;
                }

                /* If it would take the current load level over the top, skip over it */

                if  ((LONG) jp->bj_ll + Current_ll > Max_ll)
                        continue;

                /* If remote and not remote runnable, or proposed by
                   this or another machine, or waiting on
                   inaccessible vars, forget it */

                if  (jp->bj_jrunflags & (BJ_SKELHOLD|BJ_PROPOSED))
                        continue;

                if  (jp->bj_hostid)  {
                        if  (!(jp->bj_jflags & BJ_REMRUNNABLE))
                                continue;
                        if  (validate_ci(jp->bj_cmdinterp) < 0)
                                continue;
                }

                if  (jp->bj_times.tc_istime  &&  !(jp->bj_jrunflags & BJ_FORCE))  {
                        if  (now > jp->bj_times.tc_nexttime)  {
                                LONG    slack = jp->bj_times.tc_rate;

                                if  (jp->bj_times.tc_nposs == TC_CATCHUP)  {
                                        time_t  nextt;
                                        for  (;;)  {
                                                nextt = advtime(&jp->bj_times);
                                                if  (nextt > now  ||  nextt <= jp->bj_times.tc_nexttime)
                                                        goto  doit;
                                                jp->bj_times.tc_nexttime = nextt;
                                        }
                                }
                                else  if  (jp->bj_times.tc_nposs != TC_SKIP)
                                        goto  doit;

                                /* Missed it */

                                switch  (jp->bj_times.tc_repeat)  {
                                case  TC_DELETE:
                                case  TC_RETAIN:
                                        goto  doit;
                                case  TC_YEARS:
                                        slack *= 12;
                                case  TC_MONTHSE:
                                case  TC_MONTHSB:
                                        slack *= 4;
                                case  TC_WEEKS:
                                        slack *= 7;
                                case  TC_DAYS:
                                        slack *= 12;
                                case  TC_HOURS:
                                        slack *= 60;
                                case  TC_MINUTES:
                                        slack *= 60;
                                }
                                if  (now <= jp->bj_times.tc_nexttime + slack/MARGIN)
                                        goto  doit;

                                /* Missed the boat If local job, move to back of queue.  */

                                if  (jp->bj_hostid)
                                        continue;

                                back_of_queue(cind);
                                do      jp->bj_times.tc_nexttime = advtime(&jp->bj_times);
                                while  (jp->bj_times.tc_nexttime < now);
                                notify_stat_change(jp);
                                continue;
                        }
                        else  if  (now + MIGHTASWELL >= jp->bj_times.tc_nexttime)
                                goto  doit;
                        else  {
                                LONG    td = jp->bj_times.tc_nexttime - now;
                                if  (minsecs == 0  ||  minsecs > td)
                                        minsecs = td;
                                continue;
                        }
                }

        doit:
                /* See if that would send the given user over the top.  */

                if  (ulcheck(jp))
                        continue;

                /* Ready to go now, last chance - check
                   conditions. This is an initial scan on remote
                   variables. We check them properly in the next pass.  */

                if  (condcheck(jp))
                        continue;

                /* Check job won't cause the limit of jobs being
                   started at once to be exceeded.  */

                if  (being_started >= Startlim)  {
                        if  (minsecs == 0  ||  minsecs > Startwait)
                                minsecs = Startwait;
                        return  minsecs;
                }

                being_started++;

                /* If it's a remote job, propose to remote machine that we run it.  */

                if  (jp->bj_hostid)  {
                        /* (we won't have got this far if it isn't runnable) */
                        propose(cind);
                        continue;
                }

                /* Well I think that we are ready to roll */

                startup_job(cind);

                /* If we've got up to the limit, don't look any more */

                if  (Current_ll >= Max_ll)
                        break;
        }

        return  minsecs;
}

/* Note that a job has started.  The main point is to note the start
   time and implement the `delay other jobs' criterion.  */

void  started_job(const unsigned slotno)
{
        HashBtjob       *jhp = &Job_seg.jlist[slotno];
        BtjobRef        jp = &jhp->j;
        BtjobhRef       jh = &jp->h;

        jqpend++;
        qchanges++;
        Job_seg.dptr->js_serial++;
        being_started--;
        logjob(jp, $S{log code started}, 0L, jh->bj_mode.o_uid, jh->bj_mode.o_gid);
        notify_stat_change(jh);

        /* If it got forced through, unset flag and exit Maybe the
           BJ_FORCENA flag will still be set */

        if  (jh->bj_jrunflags & BJ_FORCE)  {
                jh->bj_jrunflags &= ~BJ_FORCE;
                return;
        }

        if  (jh->bj_times.tc_repeat < TC_MINUTES  || jh->bj_times.tc_nposs != TC_WAITALL)
                return;

        /* Reset standard time rounding off seconds */

        jh->bj_times.tc_nexttime = (jh->bj_stime / 60) * 60;
        notify_stat_change(jh);
}

#define SHM_TRY_MAX     10

void  completed_job(const unsigned slotno)
{
        int             msg, shmtries = 0;
        int             nrems;
        unsigned        flag, lcode, lvcode, savflags;
        volatile  HashBtjob     *jhp = &Job_seg.jlist[slotno];
        volatile  Btjob         *jp = &jhp->j;
        volatile  Btjobh        *jh = &jp->h;

        adjust_ll(- (LONG) jh->bj_ll);
        savflags = jh->bj_jrunflags;
        jh->bj_jrunflags = 0;

 retryshm:
        switch  (jh->bj_progress)  {
        default:
                disp_arg[0] = jh->bj_progress;
                panic($E{Panic job progress code});

        case  BJP_RUNNING:
                disp_arg[1] = ++shmtries;
                if  (shmtries <= SHM_TRY_MAX)  {
                        nfreport($E{Invalid progress code try});
                        sleep(shmtries);
                        goto  retryshm;
                }
                panic($E{Invalid progress code give up});

        case  BJP_FINISHED:
                flag = BJA_OK;
                lcode = $S{log code completed};
                lvcode = $S{log source jcompleted};
                msg = $PE{Job completed msg};
                break;
        case  BJP_ABORTED:
                flag = BJA_ABORT;
                lcode = $S{log code abort};
                lvcode = $S{log source jabort};
                msg = $PE{Job aborted msg};
                break;
        case  BJP_ERROR:
                flag = BJA_ERROR;
                lcode = $S{log code error};
                lvcode = $S{log source jerror};
                msg = $PE{Job error halted msg};
                break;
        }

        logjob((Btjob *) jp, lcode, 0L, jh->bj_mode.o_uid, jh->bj_mode.o_gid);

        nrems = listrems((Btjobh *) jh, 0);
        if  (nrems >= 0)  {
                if  (nrems > 0)  {
                        PIDTYPE ret;
                        if  ((ret = forksafe()) < 0)  {
                                nrems = 0;
                                goto  skipit;
                        }
                        if  (ret != 0)
                                return;
                        get_remotelock();
                        lockvars();
                        setasses((Btjob *) jp, flag, lvcode, jh->bj_lastexit, 0L);
                        remasses((Btjobh *) jh, flag, lvcode, jh->bj_lastexit);
                        unlockvars();
                        lose_remotelock();
                        tellsched(J_RESCHED, 0L);
                }
                else
                        setasses((Btjob *) jp, flag, lvcode, jh->bj_lastexit, 0L);
        }
skipit:
        notify((Btjob *) jp, msg, 1);

        /* Case where host who we are running remote job for kicks bucket */

        if  (savflags & BJ_HOSTDIED)  {
                dequeue(slotno);
                if  (nrems > 0)  {
                        tellsched(J_RESCHED, 0L);
                        exit(0);
                }
                return;
        }

        jh->bj_runhostid = jh->bj_hostid;

        if  (!jh->bj_times.tc_istime  ||  jh->bj_times.tc_repeat ==  TC_DELETE)  {
                if  (jh->bj_progress == BJP_FINISHED)
                        jh->bj_progress = BJP_DONE;

                if  (jh->bj_hostid)
                        job_rrchstat((BtjobhRef) jh);
                else
                        dequeue(slotno);
                if  (nrems > 0)  {
                        tellsched(J_RESCHED, 0L);
                        exit(0);
                }
                return;
        }

        if  (jh->bj_times.tc_repeat  == TC_RETAIN)  {
                back_of_queue(slotno);
                if  (jh->bj_progress == BJP_FINISHED)
                        jh->bj_progress = BJP_DONE;
                notify_stat_change((BtjobhRef) jh);
                if  (nrems > 0)  {
                        tellsched(J_RESCHED, 0L);
                        exit(0);
                }
                return;
        }

        back_of_queue(slotno);

        if  (jh->bj_progress == BJP_FINISHED)  {
                /* Advance time before we reset progress or it may get run again */
                if  (!(savflags & BJ_FORCENA))
                        jh->bj_times.tc_nexttime = advtime((Timecon *) &jh->bj_times);
                jh->bj_progress = BJP_NONE;
        }
        else  if  (!(jh->bj_jflags & BJ_NOADVIFERR)) /* Flags for ITSA */
                jh->bj_times.tc_nexttime = advtime((Timecon *) &jh->bj_times);

        notify_stat_change((BtjobhRef) jh);
        if  (nrems > 0)  {
                tellsched(J_RESCHED, 0L);
                exit(0);
        }
}

void  cannot_start(const unsigned slotno)
{
        int             nrems;
        BtjobRef        jp = &Job_seg.jlist[slotno].j;
        BtjobhRef       jh = &jp->h;

        adjust_ll(- (LONG)jh->bj_ll);
        being_started--;
        logjob(jp, $S{log code error}, 0L, jh->bj_mode.o_uid, jh->bj_mode.o_gid);
        nrems = listrems(jh, 0);
        if  (nrems >= 0)  {
                if  (nrems > 0)  {
                        if  (forksafe() != 0)
                                return;
                        get_remotelock();
                        lockvars();
                        setasses(jp, BJA_ERROR, $S{log source jerror}, 0, 0L);
                        remasses(jh, BJA_ERROR, $S{log source jerror}, 0);
                        unlockvars();
                        lose_remotelock();
                        tellsched(J_RESCHED, 0L);
                }
                else
                        setasses(jp, BJA_ERROR, $S{log source jerror}, 0, 0L);
        }
        jh->bj_runhostid = jh->bj_hostid;
        notify_stat_change(jh);
        if  (nrems > 0)
                exit(0);
}

void  haltall()
{
        int             hsig;
        unsigned        jind;
        char            *hsigc = envprocess(HALTSIG);

        hsig = atoi(hsigc);

        /* Tell the child process to stop */

        tellchild(O_CSTOP, 0L);

        if  (hsig <= 0  ||  hsig >= NSIG)
                return;

        /* Kill any outstanding jobs */

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                if  (jhp->j.h.bj_progress == BJP_RUNNING  &&  jhp->j.h.bj_runhostid == 0)
                        jbabort(&jhp->j.h, hsig);
                jind = jhp->q_nxt;
        }
}
