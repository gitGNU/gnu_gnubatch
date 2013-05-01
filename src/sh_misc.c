/* sh_misc.c -- scheduler misc routines

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
#include <sys/types.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
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
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "netmsg.h"
#include "files.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "sh_ext.h"
#include "notify.h"

extern  char    **environ;
extern  int     oldumask;

#ifdef  BUGGY_SIGCLD
int     nchild;
#endif

static  FILE    *rpfile;

FILE *net_feed(const int, const netid_t, const jobno_t, const int);

/* Open report file if possible and write message to it.  */

void  nfreport(int msgno)
{
        int     fid;
        time_t  tim;
        struct  tm      *tp;
        int     saverrno = errno;
        int     mon, mday;

        if  (rpfile == (FILE *) 0)  {
                fid = open(REPFILE, O_WRONLY|O_APPEND|O_CREAT, 0666);
                if  (fid < 0)
                        return;
                Ignored_error = chown(REPFILE, Daemuid, Daemgid);
                fcntl(fid, F_SETFD, 1);
                rpfile = fdopen(fid, "a");
                if  (rpfile == (FILE *) 0)  {
                        close(fid);
                        return;
                }
        }

        time(&tim);
        tp = localtime(&tim);

        /* Swap month and day somewhere in the Atlantic */

        mon = tp->tm_mon + 1;
        mday = tp->tm_mday;
#ifdef  HAVE_TM_ZONE
        if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
        if  (timezone >= 4 * 60 * 60)  {
#endif
                mday = mon;
                mon = tp->tm_mday;
        }

        fprintf(rpfile, "%.2d:%.2d:%.2d %.2d/%.2d - %s\n==============\n",
                tp->tm_hour, tp->tm_min, tp->tm_sec, mday, mon, progname);
        errno = saverrno;
        fprint_error(rpfile, msgno);
        fflush(rpfile);
}

/* Panic - generate fatal error message.  */

void  panic(int msgno)
{
        nfreport(msgno);
        exit(E_SHEDERR);
}

static  char    *msgdisp;

void  initmsgs()
{
        msgdisp = envprocess(MSGDISPATCH);
}

static void  spewdel(FILE *ofl, char *prefix, const jobno_t stuff)
{
        char    *fname;
        FILE    *fl;
        int     ch;

        if  ((fl = fopen(fname = mkspid(prefix, stuff), "r")) != (FILE *) 0)  {
                if  ((ch = getc(fl)) != EOF)  {
                        do      putc(ch, ofl);
                        while  ((ch = getc(fl)) != EOF);
                }
                fclose(fl);
                unlink(fname);
        }
        fclose(ofl);
}

static void  spewfile(FILE *ofl, const netid_t host, const int feedtype, const jobno_t stuff)
{
        FILE    *fl;
        if  ((fl = net_feed(feedtype, host, stuff, Job_seg.dptr->js_viewport)))  {
                int     ch;
                if  ((ch = getc(fl)) != EOF)  {
                        do      putc(ch, ofl);
                        while  ((ch = getc(fl)) != EOF);
                }
                fclose(fl);     /* The other end deletes when done */
        }
        fclose(ofl);
}

/* Invoke spmdisp command.  */

static void  rmsg(cmd_type cmd, CBtjobRef jp, const netid_t host, const int msg, const jobno_t sostuff, const jobno_t sestuff, char **envlist)
{
        char    **ap, *cp;
        FILE    *po = (FILE *) 0, *pe = (FILE *) 0;
        int     so_pfds[2], se_pfds[2];
        PIDTYPE pid;
        char    *arglist[10];
        char    ebuf[20];

        /* Do the business.
           If we don't have a file to send, just do exec.  */

        ap = arglist;
        if  ((cp = strrchr(msgdisp, '/')))
                cp++;
        else
                cp = msgdisp;
        *ap++ = cp;
        cp = ebuf;

        /* Generate arg string -[mwd][o][e] */

        *cp++ = '-';
        switch  (cmd)  {
        default:
        case  NOTIFY_MAIL:
                *cp++ = 'm';
                break;
        case  NOTIFY_WRITE:
                *cp++ = 'w';
                break;
        case  NOTIFY_DOSWRITE:
                *cp++ = 'd';
                break;
        }
        if  (sostuff)
                *cp++ = 'o';
        if  (sestuff)
                *cp++ = 'e';
        *cp = '\0';
        *ap++ = stracpy(ebuf);

        /* Insert number, title, host, user name, status for job owner
           as next arguments */

        sprintf(ebuf, "%ld", (long) jp->h.bj_job);
        *ap++ = stracpy(ebuf);
        *ap++ = (char *) title_of(jp);
        *ap++ = host? look_host(host): "";
        *ap++ = (char *) jp->h.bj_mode.o_user;
        sprintf(ebuf, "%u", jp->h.bj_lastexit);
        *ap++ = stracpy(ebuf);

        /* Next arg (7) is message code.
           NB Assume we don't need "ebuf" again!!!!  */

        sprintf(ebuf, "%d", msg);
        *ap++ = ebuf;

        /* For DOS machines, append host name arg 8.  */

        if  (cmd == NOTIFY_DOSWRITE  &&  !(jp->h.bj_jflags & BJ_ROAMUSER))
                *ap++ = look_host(jp->h.bj_orighostid);

        /* Null on end (9).  */

        *ap = (char *) 0;

        /* If no file to send, just exec without worrying about file.  */

        if  (sostuff == 0  &&  sestuff == 0)  {
                if  (fork() != 0)
                        return;
                execve(msgdisp, arglist, envlist);
                exit(255);
        }

        if  (sostuff  &&  pipe(so_pfds) < 0)
                return;
        if  (sestuff  &&  pipe(se_pfds) < 0)  {
                if  (sostuff)  {
                        close(so_pfds[0]);
                        close(so_pfds[1]);
                }
                return;
        }

        if  ((pid = fork()) < 0)  {
                if  (sostuff)  {
                        close(so_pfds[0]);
                        close(so_pfds[1]);
                }
                if  (sestuff)  {
                        close(se_pfds[0]);
                        close(se_pfds[1]);
                }
                return;
        }

        /* Child process - gyrations to get standard out/error hooked
           up.  Do pipe stuff backwards to minimise chances of overlap
           with f.d.s for pipes.  */

        if  (pid == 0)  {               /*  Child process  */
                if  (sestuff)  {
                        close(se_pfds[1]);
                        if  (se_pfds[0] != SESTREAM)  {
                                close(SESTREAM);
                                fcntl(se_pfds[0], F_DUPFD, SESTREAM);
                                close(se_pfds[0]);
                        }
                }
                if  (sostuff)  {
                        close(so_pfds[1]);
                        if  (so_pfds[0] != SOSTREAM)  {
                                close(SOSTREAM);
                                fcntl(so_pfds[0], F_DUPFD, SOSTREAM);
                                close(so_pfds[0]);
                        }
                }
                execve(msgdisp, arglist, envlist);
                exit(255);
        }

        /* This is the parent process (the fork was in "notify" or "rem_notify").

           btsched main process
           +------>following code
                   +------>code just above */

        if  (sostuff)  {
                close(so_pfds[0]);
                if  ((po = fdopen(so_pfds[1], "w")) == (FILE *) 0)  {
                        kill(SIGKILL, pid);
                        return;
                }
        }
        if  (sestuff)  {
                close(se_pfds[0]);
                if  ((pe = fdopen(se_pfds[1], "w")) == (FILE *) 0)  {
                        kill(SIGKILL, pid);
                        return;
                }
        }

        if  (host)  {
                if  (sostuff)
                        spewfile(po, host, FEED_SO, sostuff);
                if  (sestuff)
                        spewfile(pe, host, FEED_SE, sestuff);
        }
        else  {
                if  (sostuff)
                        spewdel(po, SONAM, sostuff);
                if  (sestuff)
                        spewdel(pe, SENAM, sestuff);
        }
}

/* See if the said file of the said job has anything in it to bother
   with. If so return the job number, otherwise quietly delete it
   and return 0.  */

static jobno_t  stuffthere(const jobno_t jn, const char *name)
{
        char    *fname;
        struct  stat    sbuf;

        if  (stat(fname = mkspid(name, jn), &sbuf) >= 0)  {
                if  (sbuf.st_size != 0)
                        return  jn;
                unlink(fname);
        }
        return  0;
}

/* Fork off a process to do mail/write without running into problems
   with zombies (apart from me).  */

static int  msg_fork(CBtjobRef jp, BtjobRef copyj)
{
#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
#endif
#else  /* BUGGY_SIGCLD */

        /* Do everything within a child process to avoid holding up
           the scheduler.  First wait for other processes though.  */

        if  (nchild > 0)  {
                alarm(2);
                while  (wait(0) >= 0)
                        ;
                if  (errno != EINTR)
                        nchild = 0;
        }
#endif

        /* Copy the job in case it gets deleted whilst we are forking.  */

        *copyj = *jp;

        if  (fork() != 0)  {
#ifdef  BUGGY_SIGCLD
                nchild++;
#endif
                return 1;
        }

#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        z.sighandler_el = SIG_DFL;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGCLD, &z, (struct sigstruct_name *) 0);
#else
        signal(SIGCLD, SIG_DFL);
#endif
#endif

        if  (Daemuid)  {
                setgid(Daemgid);
                setuid(Daemuid);
        }
        umask(oldumask);
        return  0;
}

void  notify(BtjobRef jp, const int msg, const int sendout)
{
        char    *up, **envlist;
        int     wm = jp->h.bj_jflags & BJ_WRT, mm = jp->h.bj_jflags & BJ_MAIL;
        jobno_t outstuff = 0, errstuff = 0;
        Btjob   copyj;

        if  (sendout)  {
                outstuff = stuffthere(jp->h.bj_sonum, SONAM);
                errstuff = stuffthere(jp->h.bj_senum, SENAM);
        }

        /* Route stuff for outside jobs to relevant host */

        if  (jp->h.bj_hostid)  {
                if  (wm || mm || outstuff || errstuff)
                        job_sendnote(jp, msg, outstuff, errstuff);
                return;
        }

        /* If user is using btq, turn off write type messages.  */

        if  (wm && islogged((uid_t) jp->h.bj_mode.o_uid))
                wm = 0;

        /* Force mail if there is something to send and nothing else doing.  */

        if  (!(mm || wm)  &&  (outstuff || errstuff))
                mm = 1;

        if  (!(mm || wm))
                return;

        /* In this version of prin_uname, unknown users are given as u
           and a string of digits. If so, we ignore users we don't
           understand.  */

        up = jp->h.bj_mode.o_user;
        if  (up[0] == 'u' && isdigit(up[1]))
                return;

        if  (msg_fork(jp, &copyj) != 0)
                return;

        /* Generate environment table (once!); Only pass files to
           writer program if no mail.  */

        envlist = envhandle(&copyj);
        if  (mm)  {
                rmsg(NOTIFY_MAIL, &copyj, 0L, msg, outstuff, errstuff, envlist);
                if  (wm)
                        rmsg(copyj.h.bj_jflags & BJ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE, &copyj, 0L, msg, (jobno_t) 0, (jobno_t) 0, envlist);
        }
        else  if  (wm)
                rmsg(copyj.h.bj_orighostid & BJ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE, &copyj, 0L, msg, outstuff, errstuff, envlist);
        exit(0);
}

/* Thing wot gets called from above routine on the host from which the
   job originated.  */

void  rem_notify(BtjobRef jp, const netid_t host, const struct jremnotemsg *msg)
{
        char    *up, **envlist;
        int     wm = jp->h.bj_jflags & BJ_WRT, mm = jp->h.bj_jflags & BJ_MAIL;
        Btjob   copyj;

        /* If user is using btq, turn off write type messages.  */

        if  (wm && islogged((uid_t) jp->h.bj_mode.o_uid))
                wm = 0;

        /* Force mail if there is something to send and nothing to send with.  */

        if  (!(mm || wm)  &&  (msg->sout || msg->serr))
                mm = 1;

        if  (!(mm || wm))
                return;

        /* In this version of prin_uname, unknown users are given as u
           and a string of digits.  */

        up = jp->h.bj_mode.o_user;
        if  (up[0] == 'u' && isdigit(up[1]))
                return;

        if  (msg_fork(jp, &copyj) != 0)
                return;

        /* Generate environment table (once!); */

        envlist = envhandle(&copyj);

        if  (mm)  {
                rmsg(NOTIFY_MAIL, &copyj, host, msg->msg, msg->sout, msg->serr, envlist);
                if  (wm)
                        rmsg(copyj.h.bj_orighostid & BJ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE, &copyj, host, msg->msg, (jobno_t) 0, (jobno_t) 0, envlist);
        }
        else  if  (wm)
                rmsg(copyj.h.bj_orighostid & BJ_CLIENTJOB? NOTIFY_DOSWRITE: NOTIFY_WRITE, &copyj, host, msg->msg, msg->sout, msg->serr, envlist);
        exit(0);
}
