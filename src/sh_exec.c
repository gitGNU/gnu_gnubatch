/* sh_exec.c -- scheduler job launching

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
#ifdef  HAVE_ULIMIT_H
#include <ulimit.h>
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
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "shreq.h"
#include "netmsg.h"
#include "files.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "errnums.h"
#include "ecodes.h"
#include "statenums.h"
#include "sh_ext.h"
#if     defined(OS_LINUX) && !defined(HAVE_ULIMIT_H)
extern  int     ulimit(int, long);      /* Linux doesn't implement this stupid thing */
#endif

extern char *strread(FILE *, const char *);

#define INITARGS        10
#define ARGINC          7

extern  char    **environ, **xenviron;
static  unsigned        xenv_cnt;

/* This constant gives the amount of time to wait before exiting a
   child process when something has gone wrong with redirections.
   By doing it this way we (try to) ensure that a message gets
   written to the standard error file before the process actually
   is noted as halted and the standard error file is passed on to
   the user.  */

#define WTIME   10

PIDTYPE child_pid;
int     nosetpgrp;

#ifdef  USING_FLOCK

extern void  setjhold(const int);

#define holdjobs()      setjhold(F_RDLCK)
#define unholdjobs()    setjhold(F_UNLCK)

#else

static  struct  sembuf
jr[3] = {{      JQ_READING,     1,      0       },
        {       JQ_FIDDLE,      -1,     0       },
        {       JQ_FIDDLE,      1,      0       }},
jru[1] ={{      JQ_READING,     -1,     0       }};

#define holdjobs()      while  (semop(Sem_chan, &jr[0], 3) < 0 && errno == EINTR)
#define unholdjobs()    while  (semop(Sem_chan, &jru[0], 1) < 0 && errno == EINTR)
#endif

FILE *net_feed(const int, const netid_t, const jobno_t, const int);

static  char    Filename[] = __FILE__;

/* Find job by pid only on current machine, only by child proc. */

static int  findj_by_pid(const int_pid_t pid)
{
        unsigned        jind = Job_seg.hashp_pid[pid_jhash(pid)];

        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                if  (jhp->j.h.bj_pid == pid  &&  jhp->j.h.bj_runhostid == 0)
                        return  (int) jind;
                jind = jhp->nxt_pid_hash;
        }
        return  -1;
}

/* This routine is also used in the network code to pass messages back
   to the scheduler.  */

void  tellsched(const unsigned code, const ULONG param)
{
        Repmess  mess;

        mess.mm = TO_SCHED;
        mess.outmsg.mcode = code;
        mess.outmsg.upid = getpid();
        mess.outmsg.uuid = getuid();
        mess.outmsg.ugid = getgid();
        mess.outmsg.hostid = 0;
        mess.outmsg.param = param;
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &mess, sizeof(mess) - sizeof(long), 0) < 0  &&  errno == EINTR)
                ;
}

/* Replace % args in arguments and environment. */

static char *argmangle(char *arg, BtjobRef jp)
{
        char            *fl[2];

        fl[0] = arg;
        fl[1] = (char *) 0;
        disp_str = jp->h.bj_cmdinterp;
        disp_str2 = title_of(jp);
        disp_arg[0] = int2ext_netid_t(jp->h.bj_hostid);
        disp_arg[1] = jp->h.bj_job;
        disp_arg[2] = jp->h.bj_pri;
        disp_arg[3] = jp->h.bj_ll;
        disp_arg[4] = int2ext_netid_t(jp->h.bj_orighostid);
        disp_arg[5] = jp->h.bj_time;
        disp_arg[6] = jp->h.bj_stime;
        mmangle(fl);
        return  fl[0];
}

/* Slurp up environment variables.
   We build up the environment from scratch */

char **envhandle(BtjobRef jp)
{
        char    **envlist, **ep, **rp;
        unsigned  cnt;

        if  ((envlist = (char **) malloc((xenv_cnt + jp->h.bj_nenv) * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        /* Copy in static environment. */

        rp = envlist;
        ep = xenviron;
        while  (*ep)
                *rp++ = *ep++;
        /*ASSERT(rp == &envlist[xenv_cnt]);*/

        /* Put in environment from job. */

        for  (cnt = 0;  cnt < jp->h.bj_nenv;  cnt++)  {
                char    *name, *value, *result;
                unsigned        lname;
                int     isallocv = 0;           /* Denotes we have "malloced" the value */
                ENV_OF(jp, cnt, name, value);
                if  (strchr(value, '%'))  {
                        value = argmangle(stracpy(value), jp);
                        isallocv = 1;
                }

                /* Allocate new environment variable string */

                lname = strlen(name);
                if  (!(result = malloc((unsigned) (lname + strlen(value) + 2))))
                        ABORT_NOMEM;
                sprintf(result, "%s=%s", name, value);
                if  (isallocv)
                        free(value);

                /* See if new environment variable overrides existing
                   one.  There is a storage leak here only if the
                   new environment variable is
                   duplicated. However we don't worry about that
                   as we are about to "exec".  */

                for  (ep = envlist;  ep < rp;  ep++)
                        if  (strncmp(*ep, name, lname) == 0  &&  (*ep)[lname] == '=')  {
                                *ep = result; /* LEAK if ep >= &xenviron[xenv_cnt] */
                                goto  dun;
                        }

                /* No match, insert at end. */

                *rp++ = result;
        dun:
                ;
        }

        /* Insert trailing null. */

        *rp = (char *) 0;
        return  envlist;
}

/* Return the result of running the supplied argument as a shell command */

#define PSTDIN  0
#define PSTDOUT 1
#define PSTDERR 2

static char *runprog(char *arg)
{
        char    *newarg;
#ifdef  HAVE_WAITPID
        PIDTYPE pid;
#else
        PIDTYPE pid, wpid;
#endif
        FILE    *rfd;
        int     pfds[2];

        if  (pipe(pfds) < 0)  {
                unholdjobs();
                exit(E_EXENOPIPE);
        }
        if  ((pid = fork()) < 0)  {
                unholdjobs();
                exit(E_EXENOFORK);
        }
        if  (pid == 0)  {
                close(pfds[0]);

                /* The following gyrations are because
                   f.d.s 1 and/or 2 might be closed.
                   popen falls over on most machines if it is. */

                if  (pfds[1] != PSTDOUT)  {
                        close(PSTDOUT);
                        Ignored_error = dup(pfds[1]);
                }
                if  (pfds[1] != PSTDERR)  {
                        close(PSTDERR);
                        Ignored_error = dup(pfds[1]);
                }
                if  (pfds[1] != PSTDOUT && pfds[1] != PSTDERR)
                        close(pfds[1]);

                execl(Ci_list[CI_STDSHELL].ci_path, Ci_list[CI_STDSHELL].ci_name, "-c", arg, (char *) 0);
                exit(E_NOCONFIG);
        }
        close(pfds[1]);
        unholdjobs();
        if  (!(rfd = fdopen(pfds[0], "r")))
                exit(E_NOMEM);
        newarg = strread(rfd, "\n");
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
                ;
#else
        while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
                ;
#endif
        holdjobs();
        if  (newarg)
                return  newarg;
        return  stracpy("");
}

static char  *catsegs(char *s1, char *s2)
{
        char    *result = malloc((unsigned) (strlen(s1)+strlen(s2)+1));
        if  (!result)
                ABORT_NOMEM;
        strcpy(result, s1);
        strcat(result, s2);
        return  result;
}

/* Preen a string to get `` constructs out of it */

static char *progpreen(char *arg)
{
        char    *ap = arg, *sp, *ep, *seg;
        char    *result = (char *) 0, *rp;

        /* Find the first ` in the string */

        while  ((sp = strchr(ap, '`')))  {
                *sp = '\0';
                rp = stracpy(ap);
                if  (result)  {         /* Already some stuff */
                        seg = catsegs(result, rp);
                        free(result);
                        free(rp);
                        result = seg;
                }
                else    /* First bit up to first ` */
                        result = rp;
                sp++;   /* Past the ` */
                ep = strchr(sp, '`');           /* Up to next ` */
                if  (ep)  {                     /* Found it */
                        *ep = '\0';             /* Set end of string */
                        ap = ep + 1;            /* Next segment just after it */
                }
                else
                        ap = sp + strlen(sp);   /* Set to end of string */
                rp = runprog(sp);               /* Now get the actual output */
                seg = catsegs(result, rp);      /* Append to result */
                free(result);
                free(rp);
                result = seg;
        }
        if  (result)  {                         /* Stuff there, just stuff trailing chars in */
                if  (*ap)  {    /* Something to come */
                        seg = catsegs(result, ap);
                        free(result);
                        free(arg);
                        return  seg;
                }
                free(arg);
                return  result;
        }
        return  arg;
}

/* Extract file name from redirection */

static char *getfname(BtjobRef jp, RedirRef rp, char *dirname)
{
        char    *fname = envprocess(&jp->bj_space[rp->arg]);
        if  (strchr(fname, '~'))  {
                char    *newres = unameproc(fname, dirname, (uid_t) jp->h.bj_mode.o_uid);
                free(fname);
                fname = newres;
        }
        if  (strchr(fname, '%'))
                fname = argmangle(fname, jp);
        if  (strchr(fname, '`'))
                fname = progpreen(fname);
        return  fname;
}

/* Do redirections.
   These now live in the job not the job file so we don't have to juggle
   with the file where the job lives. */

static void  doredirs(BtjobRef jp, unsigned nicev, char *dirname)
{
        int             pfds[2], wstream = -1;
        PIDTYPE         pid;
        unsigned        cnt;

        for  (cnt = 0;  cnt < jp->h.bj_nredirs;  cnt++)  {
                RedirRef  rp = REDIR_OF(jp, cnt);
                char    *fname = (char *) 0;

                switch  (rp->action)  {
                case  RD_ACT_RD:
                        wstream = open(fname = getfname(jp, rp, dirname), O_RDONLY);
                        break;
                case  RD_ACT_WRT:
                        wstream = open(fname = getfname(jp, rp, dirname), O_WRONLY | O_TRUNC | O_CREAT, 0666);
                        break;
                case  RD_ACT_APPEND:
                        wstream = open(fname = getfname(jp, rp, dirname), O_WRONLY | O_APPEND | O_CREAT, 0666);
                        break;
                case  RD_ACT_RDWR:
                        wstream = open(fname = getfname(jp, rp, dirname), O_RDWR | O_CREAT, 0666);
                        break;
                case  RD_ACT_RDWRAPP:
                        wstream = open(fname = getfname(jp, rp, dirname), O_RDWR | O_APPEND | O_CREAT, 0666);
                        break;
                case  RD_ACT_PIPEO:
                        fname = getfname(jp, rp, dirname);
                        if  (pipe(pfds) < 0)  {
                                if  (Cfile)  {
                                        disp_str = fname;
                                        print_error($E{Exec cannot pipe});
                                }
                                exit(E_EXENOPIPE);
                        }
                        if  ((pid = fork()) == 0)  {
                                close(pfds[PSTDOUT]);
                                if  (pfds[PSTDIN] != PSTDIN)  {
                                        close(PSTDIN);
                                        fcntl(pfds[PSTDIN], F_DUPFD, PSTDIN);
                                        close(pfds[PSTDIN]);
                                }

                                /*  Do nice at last minute */

                                Ignored_error = nice((int) nicev);
                                execl(DEF_CI_PATH, DEF_CI_NAME, "-c", fname, (char *) 0);
                                exit(E_NOCONFIG);
                        }
                        if  (pid < 0)  {
                                if  (Cfile)  {
                                        disp_str = fname;
                                        print_error($E{Exec cannot fork});
                                }
                                exit(E_EXENOFORK);
                        }
                        close(pfds[PSTDIN]);
                        wstream = pfds[PSTDOUT];
                        break;

                case  RD_ACT_PIPEI:
                        fname = getfname(jp, rp, dirname);
                        if  (pipe(pfds) < 0)  {
                                if  (Cfile)  {
                                        disp_str = fname;
                                        print_error($E{Exec cannot pipe});
                                }
                                exit(E_EXENOPIPE);
                        }
                        if  ((pid = fork()) == 0)  {
                                close(pfds[PSTDIN]);
                                if  (pfds[PSTDOUT] != PSTDOUT)  {
                                        close(PSTDOUT);
                                        fcntl(pfds[PSTDOUT], F_DUPFD, PSTDOUT);
                                        close(pfds[PSTDOUT]);
                                }

                                /* Do nice at last minute */

                                Ignored_error = nice((int) nicev);
                                execl(DEF_CI_PATH, DEF_CI_NAME, "-c", fname, (char *) 0);
                                exit(E_NOCONFIG);
                        }
                        if  (pid < 0)  {
                                if  (Cfile)  {
                                        disp_str = fname;
                                        print_error($E{Exec cannot fork});
                                }
                                exit(E_EXENOFORK);
                        }
                        close(pfds[PSTDOUT]);
                        wstream = pfds[PSTDIN];
                        break;

                case  RD_ACT_CLOSE:
                        if  (Cfile  &&  rp->fd == fileno(Cfile))  {
                                fclose(Cfile);
                                Cfile = (FILE *) 0;
                        }
                        else
                                close(rp->fd);
                        continue;
                case  RD_ACT_DUP:
                        wstream = rp->arg;
                        fname = (char *) 0;
                        break;
                }

                if  (wstream < 0)  {
                        if  (Cfile)  {
                                disp_str = fname;
                                print_error($E{Exec cannot open redirection file});
                        }
                        exit(E_EXENOOPEN);
                }

                if  (fname)
                        free(fname);

                if  (wstream == rp->fd)
                        continue;

                if  (Cfile  &&  rp->fd == fileno(Cfile))  {
                        fclose(Cfile);
                        Cfile = (FILE *) 0;
                }
                else
                        close(rp->fd);

                if  (fcntl(wstream, F_DUPFD, (int) rp->fd) != rp->fd)  {
                        if  (Cfile)  {
                                disp_arg[0] = rp->fd;
                                print_error($E{Exec cannot dup});
                        }
                        exit(E_EXENOOPEN);
                }
                if  (rp->action != RD_ACT_DUP)
                        close(wstream);
        }
}

/* Get argument and preen it for %s and ``s. */

static char *arg_preen(BtjobRef jp, const unsigned num)
{
        char    *arg = stracpy(ARG_OF(jp, num));
        /* Substitute environment variables. */
        if  (strchr(arg, '$'))  {
                char  *newarg = envprocess(arg);
                /* If this doesn't work, it'll return a null value so we just ignore the $ construct */
                if  (newarg)  {
                        free(arg);
                        arg = newarg;
                }
        }
        if  (strchr(arg, '%'))
                arg = argmangle(arg, jp);
        if  (strchr(arg, '`'))
                arg = progpreen(arg);
        return  arg;
}

/* Process argument vector */

static char **arghandle(BtjobRef jp, CmdintRef ci)
{
        char    *args;
        const   char    *tit;
        char    *startarg, **argv;
        int     argmax, argc;
        unsigned        cnt;

        if  ((argv = (char **) malloc((INITARGS+jp->h.bj_nargs+1) * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        argmax = INITARGS;

        /* If flag set, and title non-null, set 0th argument to title
           Otherwise set name of command interpreter. */

        tit = title_of(jp);
        argv[0] = ci->ci_flags & CIF_SETARG0 && tit[0]? (char *) tit: ci->ci_name;

        argc = 1;
        args = ci->ci_args;

        /* Deal with predefined arguments.  */

        while  (*args)  {

                /* Forget any leading white space */

                while  (isspace(*args))
                        args++;

                /* Mark start of argument */

                startarg = args;

                for  (;  *args && !isspace(*args);  args++)  {

                        /* A backslash escapes the next char so that
                           we can incorporate a space or a quote.  */

                        if  (*args == '\\')  {
                                if  (*++args == '\0')
                                        goto  endc;
                                continue;
                        }
                }

                if  (args != startarg)  {

                        /* Found an actual argument.  */

                        if  (argc >= argmax)  {
                                argmax += ARGINC + jp->h.bj_nargs;
                                if  ((argv = (char **) realloc((char *) argv, (unsigned) ((argmax + 1) * sizeof(char *)))) == (char **) 0)
                                        ABORT_NOMEM;
                        }

                        argv[argc] = startarg;
                        argc++;

                        /* If it ended with a space, null-terminate it */

                        if  (*args)
                                *args++ = '\0';
                }
        }

 endc:
        if  (!(ci->ci_flags & CIF_INTERPARGS))  {
                if  (argc + (int) jp->h.bj_nargs >= argmax)  {
                        argmax = argc + jp->h.bj_nargs;
                        if  ((argv = (char **) realloc((char *) argv, (unsigned) ((argmax + 1) * sizeof(char *)))) == (char **) 0)
                                ABORT_NOMEM;
                }

                for  (cnt = 0;  cnt < jp->h.bj_nargs;  cnt++)
                        argv[argc++] = arg_preen(jp, cnt);
        }
        argv[argc] = (char *) 0;
        return  argv;
}

static void  connect_jobfile(BtjobRef jp)
{
        char  *dummstr;

        /* If the job belongs to some other machine, copy it into a
           temporary file on this machine. */

        if  (jp->h.bj_hostid)  {
                FILE    *netf, *outf;
                char    *tnam;
                jobno_t jn = jp->h.bj_job;
                int     fid, ch;

                if  (!(netf = net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job, Job_seg.dptr->js_viewport)))
                        exit(E_EXENOJOB);

                /* Loop until the file name is unique.  */

                for  (;;)  {
                        tnam = mkspid(NTNAM, jn);
                        if  ((fid = open(tnam, O_WRONLY|O_CREAT|O_EXCL, CMODE)) >= 0)
                                break;
                        jn += JN_INC;
                }
#ifdef  HAVE_FCHOWN
                if  (Daemuid != ROOTID)
                        Ignored_error = fchown(fid, Daemuid, Daemgid);
#else
                if  (Daemuid != ROOTID)
                        Ignored_error = chown(tnam, Daemuid, Daemgid);
#endif
                if  (!(outf = fdopen(fid, "w")))  {
                        unlink(tnam);
                        exit(E_NOMEM);
                }
                while  ((ch = getc(netf)) != EOF)
                        putc(ch, outf);
                fclose(netf);
                fclose(outf);
                if  (!freopen(tnam, "r", stdin))  {
                        unlink(tnam);
                        exit(E_NOMEM);
                }

                /* Success - but arrange for the job file to be
                   deleted as soon as this end loses interest */

                unlink(tnam);
        }
        else  if  (freopen(dummstr = mkspid(SPNAM, jp->h.bj_job), "r", stdin) == (FILE *) 0)  {
                perror(dummstr);
                exit(E_EXENOJOB);
        }
}

static void  proc_args(BtjobRef jp)
{
        int     ch, num;

        while  ((ch = getchar()) != EOF)  {

                /* We just look for $s and \s, the latter as escapes */

                while  (ch == '$' || ch == '\\')  {
                        if  (ch == '\\')  {
                                if  ((ch = getchar()) != EOF)
                                        break;          /* And drop into the putchar */
                                exit(0);
                        }

                        /* Must have read a $ */

                        if  ((ch = getchar()) == EOF)
                                exit(0);

                        /* If we read a $, look at the next char.
                           If it's not a digit, then spit out the $ and loop
                           round with whatever is in ch - it might really be a \ or $ */

                        if  (ch != '*' && ch != '@' && !isdigit(ch))  {
                                putchar('$');
                                continue;
                        }

                        if  (ch == '*' || ch == '@')  {
                                for  (num = 0;  num < (unsigned) jp->h.bj_nargs;  num++)  {
                                        char    *arg = arg_preen(jp, (unsigned) num);
                                        if  (num != 0)
                                                putchar(' ');
                                        fputs(arg, stdout);
                                        free(arg);
                                }
                                ch = getchar();
                        }
                        else  {

                                /* Otherwise read in number, leaving next char in ch. */

                                num = 0;
                                do  {
                                        num = num * 10 + ch - '0';
                                        ch = getchar();
                                }  while  (isdigit(ch));

                                /* Arg number should be 1 to number of args, which we offset
                                   by -1. */

                                if  (num > 0  &&  (unsigned) num <= jp->h.bj_nargs)  {
                                        char    *arg = arg_preen(jp, (unsigned) (num-1));
                                        fputs(arg, stdout);
                                        free(arg);
                                }
                        }

                        /* and loop again 'cous ch might be \ or $
                           careful to catch EOF immediately after $n!*/

                        if  (ch == EOF)
                                exit(0);
                }
                putchar(ch);
        }

        exit(0);
}

static void  sigfix()
{
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zs;
#ifdef  HAVE_SIGACTION
        sigset_t        nset;
        sigemptyset(&nset);
#endif
        sigmask_clear(zs);
        zs.sigflags_el = SIGVEC_INTFLAG;
        zs.sighandler_el = SIG_DFL;
#ifndef DEBUG
        sigact_routine(SIGFPE, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGBUS, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGSEGV, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGILL, &zs, (struct sigstruct_name *) 0);
#ifdef  SIGSYS
        sigact_routine(SIGSYS, &zs, (struct sigstruct_name *) 0);
#endif /* SIGSYS */
#endif /* !DEBUG */
        sigact_routine(SIGINT, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGHUP, &zs, (struct sigstruct_name *) 0);
        sigact_routine(SIGPIPE, &zs, (struct sigstruct_name *) 0);
        sigact_routine(RESCHED, &zs, (struct sigstruct_name *) 0);
#ifdef  HAVE_SIGACTION
        sigprocmask(SIG_SETMASK, &nset, (sigset_t *) 0);
#else  /* !HAVE_SIGACTION, sigvec or sigvector */
        sigsetmask(0);
#endif /* !HAVE_SIGACTION */
#else  /* !STRUCT_SIG */
#ifndef DEBUG
        signal(SIGFPE, SIG_DFL);
        signal(SIGBUS, SIG_DFL);
        signal(SIGSEGV, SIG_DFL);
        signal(SIGILL, SIG_DFL);
#ifdef  SIGSYS
        signal(SIGSYS, SIG_DFL);
#endif
#endif /* !DEBUG */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);
        signal(RESCHED, SIG_DFL);
#endif /* !STRUCT_SIG */
}

/* Start off job */

static void  startj(const unsigned indx)
{
        PIDTYPE         rpid;
        char            *dirname, **argv;
        CmdintRef       ci;
        int             cinum;
        HashBtjob       *jhp;
        BtjobRef        jp;
#ifdef  HAVE_GETGROUPS
        int             ngroups;
        gid_t           *glist;
#endif

        jhp = &Job_seg.jlist[indx];
        jp = &jhp->j;

        if  (jp->h.bj_jrunflags & BJ_PENDKILL)  {

                /* If we got a kill halfway through startup,
                   Pretend it started, then pretend it finished.
                   The signal number was put in lastexit. */

                jp->h.bj_jrunflags &= ~BJ_PENDKILL;
                jp->h.bj_progress = BJP_RUNNING;
                jp->h.bj_pid = 0;
                jp->h.bj_stime = time((time_t *) 0);
#ifdef  USING_MMAP
                msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
                tellsched(J_STOK, (ULONG) indx);

                /* OK done that, now pretend it's finished. */

                jp->h.bj_etime = time((time_t *) 0);
                jp->h.bj_progress = BJP_ABORTED;
                Job_seg.dptr->js_serial++;
#ifdef  USING_MMAP
                msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
                tellsched(J_COMPLETED, (ULONG) indx);
                return;
        }

        if  ((rpid = fork()) != 0)  {
                unsigned  hashval, nxtind;
                if  (rpid < 0)  {
                        jp->h.bj_progress = BJP_ERROR;
#ifdef  USING_MMAP
                        msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
                        tellsched(J_NOFORK, (ULONG) indx);
                }

                /* Put onto pid hash chain.  I don't think that we
                   need to lock it as we are the only people who
                   use the stuff.  */

                hashval = pid_jhash(rpid);
                nxtind = Job_seg.hashp_pid[hashval];
                jhp->nxt_pid_hash = nxtind;
                jhp->prv_pid_hash = JOBHASHEND;
                if  (nxtind != JOBHASHEND)
                        Job_seg.jlist[nxtind].prv_pid_hash = indx;
                Job_seg.hashp_pid[hashval] = indx;
                return;
        }

        /* Some signal implementations based on sigset don't clear
           pending sigs after fork */

#if     !defined(STRUCT_SIG) && defined(BUGGY_SIGSET_HOLDS)
        signal(RESCHED, SIG_IGN);
#endif

        /* In case job completes before we return to main process we
           reset the state here.  */

        jp->h.bj_progress = BJP_RUNNING;
        jp->h.bj_pid = getpid();
        jp->h.bj_stime = time((time_t *) 0);
#ifdef  USING_MMAP
        msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        tellsched(J_STOK, (ULONG) indx);

        if  (!nosetpgrp)  {
#ifdef  SETPGRP_VOID
                setpgrp();
#else
                setpgrp(0, getpid());
#endif
        }

        /* Ok we are now a new child process Find command interpreter.
           Open the job file, create the std in and std error files */

        if  ((cinum = validate_ci(jp->h.bj_cmdinterp)) < 0)
                exit(E_EXENOCI);
        ci = &Ci_list[cinum];

        /* If we are interpreting $n ourselves, do it as another child
           process and pipe it in.  */

        if  (ci->ci_flags & CIF_INTERPARGS)  {
                PIDTYPE epid;
                int     ps[2];
                if  (pipe(ps) < 0  || (epid = fork()) < 0)
                        exit(E_EXENOOPEN);

                if  (epid == 0)  {
                        sigfix();

                        /* Try to become a grandchild process in case the job
                           waits for child processes and waits for me.
                           (If the fork fails, don't bother */

                        if  (fork() > 0)
                                exit(0);
                        close(ps[0]);
                        close(1);
                        Ignored_error = dup(ps[1]);
                        close(ps[1]);
                        connect_jobfile(jp);
                        proc_args(jp);
                }
                else  {
                        close(ps[1]);
                        close(0);
                        Ignored_error = dup(ps[0]);
                        close(ps[0]);
                }
        }
        else
                connect_jobfile(jp);

        /* We can't just use the job number any more for standard
           output and standard error because there might be
           another job executing with the same job number. */

        jp->h.bj_sonum = jp->h.bj_senum = jp->h.bj_job;

        for  (;;)  {
                char    *f;
                int     fid;
                f = mkspid(SONAM, jp->h.bj_sonum);
                if  ((fid = open(f, O_WRONLY|O_CREAT|O_EXCL, CMODE)) < 0)  {
                        jp->h.bj_sonum += JN_INC;
                        continue;
                }
#ifdef  HAVE_FCHOWN
                if  (Daemuid != ROOTID)
                        Ignored_error = fchown(fid, Daemuid, Daemgid);
#else
                if  (Daemuid != ROOTID)
                        Ignored_error = chown(f, Daemuid, Daemgid);
#endif
                close(fid);
                if  (freopen(f, "w", stdout))
                        break;
                exit(E_EXENOJOB);
        }
        for  (;;)  {
                char    *f;
                int     fid;
                f = mkspid(SENAM, jp->h.bj_senum);
                if  ((fid = open(f, O_WRONLY|O_CREAT|O_EXCL, CMODE)) < 0)  {
                        jp->h.bj_senum += JN_INC;
                        continue;
                }
#ifdef  HAVE_FCHOWN
                if  (Daemuid != ROOTID)
                        Ignored_error = fchown(fid, Daemuid, Daemgid);
#else
                if  (Daemuid != ROOTID)
                        Ignored_error = chown(f, Daemuid, Daemgid);
#endif
                close(fid);
                if  (freopen(f, "w", stderr))
                        break;
                exit(E_EXENOOPEN);
        }

        /* Set up arguments for % mangling in unameproc & what have
           you. The title is set up in disp_str after we've locked the jobs.  */

        disp_str2 = ci->ci_name;
        disp_arg[0] = jp->h.bj_pri;
        disp_arg[1] = jp->h.bj_ll;

        /* Set umask and (how I hate this stupid concept) ulimit values.
           Forget all about ulimit if it is zero */

        umask((int) jp->h.bj_umask);
#ifndef OS_FREEBSD
        if  (jp->h.bj_ulimit != 0L)
                ulimit(2, jp->h.bj_ulimit);
#endif

        /* We can now change user and group to the job, and select
          current directory */

#ifdef  HAVE_GETGROUPS
#ifdef  GETGROUPS_SAME_SIZE
        if  ((ngroups = get_suppgrps(jp->h.bj_mode.o_uid, &glist)) > 0)
                setgroups(ngroups, glist);
#else

        /* The good people who invented Ultrix and a few other such goodies hit on the
           clever idea of making the 2nd argument to setgroups an 'int*' rather than a
           'gid_t*'. (gid_t is a short).  */

        if  ((ngroups = get_suppgrps(jp->h.bj_mode.o_uid, &glist)) > 0)  {
                int     cnt;
                GETGROUPS_T     iglist[NGROUPS];
                for  (cnt = 0;  cnt < ngroups && cnt < NGROUPS;  cnt++)
                        iglist[cnt] = glist[cnt];
                setgroups(ngroups, iglist);
        }
#endif
#endif
        setgid((gid_t) jp->h.bj_mode.o_gid);
        setuid((uid_t) jp->h.bj_mode.o_uid);

        /* Hang on to string pointers while we muck around with them */

        holdjobs();
        disp_str = title_of(jp);
        environ = envhandle(jp); /* We've forked and are just about to exec don't forget */
        dirname = envprocess(jp->h.bj_direct < 0? homedof(jp->h.bj_mode.o_uid):  &jp->bj_space[jp->h.bj_direct]);
        if  (strchr(dirname, '~'))  {
                char    *newdir = unameproc(dirname, dirname, (uid_t) jp->h.bj_mode.o_uid);
                free(dirname);
                dirname = newdir;
        }
        if  (chdir(dirname) < 0)  {
                disp_str = dirname;
                print_error($E{Exec cannot change directory});
                unholdjobs();
                exit(E_EXENODIR);
        }
        argv = arghandle(jp, ci);

        doredirs(jp, (unsigned) ci->ci_nice, dirname);
        unholdjobs();           /* Finished with pointers */

        /* Do the business */

        sigfix();
        Ignored_error = nice((int) ci->ci_nice);
        execv(ci->ci_path, argv);
        exit(E_NOCONFIG);
}

/* This gets called when we discover that a job has finished. */

static void  procdone(const PIDTYPE pid, int status)
{
        volatile  HashBtjob     *jhp;
        volatile  Btjob         *jp;
        volatile  Btjobh        *jh;
        int             indx;
        unsigned        nextind, prevind;

        if  ((indx = findj_by_pid(pid)) < 0)
                return;         /* Huh? */

        jhp = &Job_seg.jlist[indx];
        jp = &jhp->j;
        jh = &jp->h;

        prevind = jhp->prv_pid_hash;
        nextind = jhp->nxt_pid_hash;
        if  (prevind == JOBHASHEND)
                Job_seg.hashp_pid[pid_jhash(jh->bj_pid)] = nextind;
        else
                Job_seg.jlist[prevind].nxt_pid_hash = nextind;

        if  (nextind != JOBHASHEND)
                Job_seg.jlist[nextind].prv_pid_hash = prevind;

        jh->bj_lastexit = (USHORT) status;
        jh->bj_etime = time((time_t *) 0);
        jh->bj_pid = 0;

        if  ((status & 0xff) != 0)
                jh->bj_progress = BJP_ABORTED;
        else  {
                unsigned  exitc = (status >> 8) & 0xff;

                if  (exitc >= jh->bj_exits.nlower  &&  exitc <= jh->bj_exits.nupper)  {

                        /* If it fits both chose the one with the smaller range */

                        if  (exitc >= jh->bj_exits.elower  &&  exitc <= jh->bj_exits.eupper)  {
                                if  ((int) (jh->bj_exits.eupper - jh->bj_exits.elower) >=
                                     (int) (jh->bj_exits.nupper - jh->bj_exits.nlower))
                                        jh->bj_progress = BJP_FINISHED;
                                else
                                        jh->bj_progress = BJP_ERROR;
                        }
                        else
                                jh->bj_progress = BJP_FINISHED;
                }
                else  if  (exitc >= jh->bj_exits.elower  &&  exitc <= jh->bj_exits.eupper)
                        jh->bj_progress = BJP_ERROR;
                else
                        jh->bj_progress = BJP_ABORTED;
        }
        Job_seg.dptr->js_serial++;
#ifdef  USING_MMAP
        msync(Job_seg.inf.seg, Job_seg.inf.segsize, MS_SYNC|MS_INVALIDATE);
#endif
        tellsched(J_COMPLETED, (ULONG) indx);
}

/* Deal with messages to remap the job list cous we have
   changed/expanded the shm segment */

static void  remapj()
{
        void    *newseg;
        holdjobs();
#ifdef  USING_MMAP
        munmap(Job_seg.inf.seg, Job_seg.inf.segsize);
        Job_seg.inf.reqsize = Job_seg.inf.segsize = lseek(Job_seg.inf.mmfd, 0L, 2);
        if  ((newseg = mmap(0, Job_seg.inf.segsize, PROT_READ|PROT_WRITE, MAP_SHARED, Job_seg.inf.mmfd, 0)) == MAP_FAILED)
                ABORT_NOMEM;
        Job_seg.dptr = (struct jshm_hdr *) newseg;
#else
        if  (!Job_seg.dptr->js_nxtid)  {
                unholdjobs();
                return;
        }

        newseg = Job_seg.inf.seg;
        do  {   /* Well it might move again (unlikely - but...) */
                Job_seg.inf.base = Job_seg.dptr->js_nxtid;
                shmdt(newseg);  /*  Lose old one  */

                if  ((Job_seg.inf.chan = shmget((key_t) Job_seg.inf.base, 0, 0)) <= 0  ||
                     (newseg = shmat(Job_seg.inf.chan, (char *) 0, 0)) == (char *) -1)
                        ABORT_NOMEM;
                Job_seg.dptr = (struct jshm_hdr *) newseg;
                if  (Job_seg.dptr->js_type != TY_ISJOB)
                        ABORT_NOMEM;
        }  while  (Job_seg.dptr->js_nxtid);

        /* NB: We have lost track of what the job segment size is in this case
           but I don't think we need it */
#endif

        /* Reinitialise pointers */

        Job_seg.inf.seg = newseg;
        Job_seg.hashp_pid = (USHORT *) ((char *) newseg + sizeof(struct jshm_hdr));
        Job_seg.hashp_jno = (USHORT *) ((char *) Job_seg.hashp_pid + JOBHASHMOD*sizeof(USHORT));
        Job_seg.hashp_jid = (USHORT *) ((char *) Job_seg.hashp_jno + JOBHASHMOD*sizeof(USHORT));
        Job_seg.jlist = (HashBtjob *) ((char *) Job_seg.hashp_jid + JOBHASHMOD*sizeof(USHORT) + sizeof(USHORT));
        unholdjobs();
}

/* Deal with messages to start up processes etc */

void  do_resched_msgs()
{
        Repmess inmess;
#ifdef  UNSAFE_SIGNALS
        static  RETSIGTYPE catchit(int);
        signal(RESCHED, SIG_IGN);
#endif

        /* Read in all the messages we've got. */

        for  (;;)  {
                if  (msgrcv(Ctrl_chan, (struct msgbuf *) &inmess, sizeof(inmess) - sizeof(long), CHILD, IPC_NOWAIT) < 0)  {
                        if  (errno != EINTR)  {
                                if  (errno == EIDRM)
                                        exit(0);
                                break;
                        }
                        continue;
                }

                switch  (inmess.outmsg.mcode)  {
                default:
                        continue;
                case  J_START:
                        startj((unsigned) inmess.outmsg.param);
                        continue;
                case  O_CSTOP:
                        exit(0);
                case  O_JREMAP:
                        remapj();
                        continue;
                case  O_VREMAP:
                        /* Ignore this */
                        continue;
                case  O_PWCHANGED:
                        un_rpwfile();
                        rpwfile();
#ifdef  HAVE_GETGROUPS
                        rgrpfile();
#endif
                        continue;
                }
        }

#ifdef  UNSAFE_SIGNALS
        signal(RESCHED, catchit);
#endif
}

static RETSIGTYPE  stop_child(int signum)
{
        Shipc   oreq;

#ifdef  UNSAFE_SIGNALS
        signal(signum, SIG_IGN);
#endif

        oreq.sh_mtype = TO_SCHED;
        oreq.sh_params.mcode = O_STOP;
        oreq.sh_params.uuid = getuid();
        oreq.sh_params.ugid = getgid();
        oreq.sh_params.upid = getpid();
        oreq.sh_params.hostid = 0L;
        oreq.sh_params.param = signum == SIGTERM? $E{Scheduler killed}: $E{Scheduler exec core dump};
        msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(Shreq), 0);
        exit(0);
}

/* Signal to catch pokes from the scheduler proper */

static RETSIGTYPE  catchit(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(RESCHED, catchit);
#endif
        do_resched_msgs();
}

/* This routine does all the dirty work for the slave process which runs the jobs */

void  childproc()
{
        PIDTYPE wpid;
        int     status;
        char    **ep;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
#endif

        /* Count the number of fixed environment variables now.  We do
           this as the main process 'cous code in sh_misc.c uses
           envhandle which requires it.  */

        xenv_cnt = 1;
        for  (ep = xenviron;  *ep;  ep++)
                xenv_cnt++;

        if  ((wpid = fork()) != 0)  {
#ifdef  BUGGY_SIGCLD
                PIDTYPE   pid2;
#endif
                if  (wpid < 0)
                        panic($E{Cannot fork});
#ifdef  BUGGY_SIGCLD

                /* Contortions to avoid being a parent of the child process */

                if  ((pid2 = fork()) != 0)  {
                        if  (pid2 < 0)  {
                                kill(wpid, SIGKILL);
                                panic($E{Cannot fork});
                        }
                        exit(0);
                }

                /* The main btsched process is now a sibling of the child process... */

#endif
                child_pid = wpid;
                return;
        }

#ifdef  STRUCT_SIG
        z.sighandler_el = stop_child;
        sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#ifndef DEBUG
        sigact_routine(SIGFPE, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGBUS, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGSEGV, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGILL, &z, (struct sigstruct_name *) 0);
#ifdef  SIGSYS
        sigact_routine(SIGSYS, &z, (struct sigstruct_name *) 0);
#endif /* SIGSYS */
#endif /* !DEBUG */
#ifndef BUGGY_SIGCLD
        z.sighandler_el = SIG_DFL;
        sigact_routine(SIGCLD, &z, (struct sigstruct_name *) 0);
#endif
        z.sighandler_el = catchit;
        sigact_routine(RESCHED, &z, (struct sigstruct_name *) 0);
#else  /* !STRUCT_SIG */
        signal(SIGTERM, stop_child);
#ifndef DEBUG
        signal(SIGFPE, stop_child);
        signal(SIGBUS, stop_child);
        signal(SIGSEGV, stop_child);
        signal(SIGILL, stop_child);
#ifdef  SIGSYS
        signal(SIGSYS, stop_child);
#endif /* SIGSYS */
#endif /* !DEBUG */
#ifndef BUGGY_SIGCLD
        signal(SIGCLD, SIG_DFL);
#endif
        signal(RESCHED, catchit);
#endif /* !STRUCT_SIG */

        for  (;;)  {
                wpid = wait(&status); /* We really want wait here! */
                if  (wpid < 0)  {
                        if  (errno == ECHILD)
                                pause();
                        continue;
                }

                procdone(wpid, status);
        }
}

/* Tell child process to do something */

void  tellchild(const unsigned code, const ULONG param)
{
        Repmess mess;

        mess.mm = CHILD;
        mess.outmsg.mcode = code;
        mess.outmsg.hostid = 0;
        mess.outmsg.param = param;
        forcemsg((char *) &mess, sizeof(Shreq));
        kill(child_pid, RESCHED);
}

/* Send signal to process group */

void  jbabort(BtjobhRef jp, int n)
{
        if  (jp->bj_pid)
                kill(-(PIDTYPE) jp->bj_pid, n);
}

/* Exterminate all traces of job */

void  murder(BtjobhRef jp)
{
        if  (jp->bj_pid)
                kill(-(PIDTYPE) jp->bj_pid, SIGKILL);
}
