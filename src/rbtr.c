/* rbtr.c -- main module for gbch-rbr

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
#include <sys/stat.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpalt.h"
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "jvuprocs.h"
#include "remsubops.h"
#include "btrvar.h"
#include "spitrouts.h"
#include "optflags.h"

#ifndef ROOTID
#define ROOTID  0
#endif

#define RBTR_INLINE

extern  char    **environ, **xenviron;

uid_t   Repluid;                /* Replacement if requested */
gid_t   Replgid;                /* Replacement if requested */

extern  netid_t         Out_host;
char    *realuname;
char    realgname[UIDSIZE+1];

char    *repluname, *replgname;

Btjob           Out_job;

char    Verbose,
        Outside_env;

#define MODE_NONE       0
#define MODE_SET        1
#define MODE_ON         2
#define MODE_OFF        3

char    *Out_interp;

void  remgetuml(const netid_t, USHORT *, ULONG *);
char **remread_envir(const netid_t);
BtuserRef  remgetbtuser(const netid_t, char *, char *);
int  remgoutfile(const netid_t, BtjobRef);
LONG  remprocreply(const int);
void  chkfuture(BtjobRef, const int);
int  sock_write(const int, char *, int);
void  checksetmode(const int, const ushort *, const ushort, USHORT *);

static  char    Filename[] = __FILE__;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

OPTION(o_explain)
{
        print_error($E{btr explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_noverbose)
{
        Verbose = 0;
        return  OPTRESULT_OK;
}

OPTION(o_verbose)
{
        Verbose = 1;
        return  OPTRESULT_OK;
}

DEOPTION(o_condcrit);
DEOPTION(o_nocondcrit);
DEOPTION(o_asscrit);
DEOPTION(o_noasscrit);
DEOPTION(o_canccond);
DEOPTION(o_condition);
DEOPTION(o_cancset);
DEOPTION(o_flags);
DEOPTION(o_set);
DEOPTION(o_advterr);
DEOPTION(o_noadvterr);
DEOPTION(o_directory);
DEOPTION(o_exits);
DEOPTION(o_noexport);
DEOPTION(o_export);
DEOPTION(o_fullexport);
DEOPTION(o_loadlev);
DEOPTION(o_mail);
DEOPTION(o_write);
DEOPTION(o_cancmailwrite);
DEOPTION(o_normal);
DEOPTION(o_ascanc);
DEOPTION(o_asdone);
DEOPTION(o_priority);
DEOPTION(o_deltime);
DEOPTION(o_runtime);
DEOPTION(o_whichsig);
DEOPTION(o_gracetime);
DEOPTION(o_umask);
DEOPTION(o_ulimit);
DEOPTION(o_title);
DEOPTION(o_cancargs);
DEOPTION(o_argument);
DEOPTION(o_cancio);
DEOPTION(o_io);
DEOPTION(o_queuehost);
DEOPTION(o_mode_job);
DEOPTION(o_notime);
DEOPTION(o_time);
DEOPTION(o_norepeat);
DEOPTION(o_deleteatend);
DEOPTION(o_repeat);
DEOPTION(o_avoiding);
DEOPTION(o_skip);
DEOPTION(o_hold);
DEOPTION(o_resched);
DEOPTION(o_catchup);
DEOPTION(o_jobqueue);
DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

OPTION(o_owner)
{
        int_ugid_t      nuid;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_OWNER_CHANGE;

        if  ((nuid = lookup_uname(arg)) == UNKNOWN_UID)  {
                arg_errnum = $E{Unknown owner};
                return  OPTRESULT_ERROR;
        }

        Repluid = (uid_t) nuid;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_group)
{
        int_ugid_t      ngid;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_GROUP_CHANGE;

        if  ((ngid = lookup_gname(arg)) == UNKNOWN_GID)  {
                arg_errnum = $E{Unknown group};
                return  OPTRESULT_ERROR;
        }
        Replgid = (gid_t) ngid;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_localenv)
{
        Outside_env = 0;
        return  OPTRESULT_OK;
}

OPTION(o_outsideenv)
{
        Outside_env = 1;
        return  OPTRESULT_OK;
}

OPTION(o_interpreter)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (Out_interp)
                free(Out_interp);
        Out_interp = stracpy(arg); /* Check this later */
        return  OPTRESULT_ARG_OK;
}

#include "inline/btradefs.c"
#include "inline/btroptp.c"

/* Get input file.  */

FILE *ginfile(char *arg)
{
        FILE  *inf;

        /* If we are reading from the standard input, then all is ok.
           Put out a message if it looks like the terminal, as
           the silent wait for action might confuse someone who
           has made a mistake.  */

        if  (arg == (char *) 0)  {
                struct  stat  sbuf;

                fstat(0, &sbuf);
                if  ((sbuf.st_mode & S_IFMT) == S_IFCHR)
                        print_error($E{Expecting terminal input});
                return  stdin;
        }

#ifdef  HAVE_SETEUID

        seteuid(Realuid);
        inf = fopen(arg, "r");
        seteuid(Daemuid);
        if  (inf)
                return  inf;

#else  /* !HAVE_SETEUID */
#ifdef  ID_SWAP

        /* If we can shuffle between uids, revert to real uid to get
           at file.  Use "access" call if we are sticking to root.  */

        if  (Realuid != ROOTID)  {
#if     defined(NHONSUID) || defined(DEBUG)
                if  (Daemuid != ROOTID  &&  Effuid != ROOTID)  {
                        setuid(Realuid);
                        inf = fopen(arg, "r");
                        setuid(Daemuid);
                }
                else  {
                        if  (access(arg, 04) < 0)
                                goto  noopen;
                        inf = fopen(arg, "r");
                }
#else
                if  (Daemuid != ROOTID)  {
                        setuid(Realuid);
                        inf = fopen(arg, "r");
                        setuid(Daemuid);
                }
                else  {
                        if  (access(arg, 04) < 0)
                                goto  noopen;
                        inf = fopen(arg, "r");
                }
#endif
        }
        else
                inf = fopen(arg, "r");

        if  (inf)
                return  inf;
#else
        if  (Daemuid == ROOTID)  {
                if  ((Realuid != ROOTID  &&  access(arg, 04) < 0) || (inf = fopen(arg, "r")) == (FILE *) 0)
                        goto  noopen;
                return   inf;
        }
        else  {
                FILE    *res;
                int     ch, pfile[2];
                static  PIDTYPE lastpid = -1;

                /* Otherwise we fork off a process to read the file as
                   the files might not be readable by the batch
                   effective userid, and it might be done as a
                   backdoor method of reading files only readable
                   by the batch effective uid.  */

                if  (lastpid >= 0)  { /* Clean up zombie from last time around */
#ifdef  HAVE_WAITPID
                        while  (waitpid(lastpid, (int *) 0, 0) < 0  &&  errno == EINTR)
                                ;
#else
                        PIDTYPE pid;

                        while  ((pid = wait((int *) 0)) != lastpid  &&  (pid >= 0 || errno == EINTR))
                                ;
#endif
                }

                if  (pipe(pfile) < 0)  {
                        print_error($E{Cannot pipe});
                        exit(E_NOPIPE);
                }

                if  ((lastpid = fork()) < 0)  {
                        print_error($E{Cannot fork});
                        exit(E_NOFORK);
                }

                if  (lastpid != 0)  {   /*  Parent process  */
                        close(pfile[1]);        /*  Write side of pipe  */
                        res = fdopen(pfile[0], "r");
                        if  (res == (FILE *) 0)  {
                                print_error($E{Cannot pipe});
                                exit(E_NOPIPE);
                        }
                        return  res;
                }

                /* The remaining code is executed by the child process.  */

                close(pfile[0]);
                if  ((res = fdopen(pfile[1], "w")) == (FILE *) 0)
                        exit(E_NOPIPE);

                /* Reset uid to real uid.  */

                setuid(Realuid);

                if  ((inf = fopen(arg, "r")) == (FILE *) 0)  {
                        disp_str = arg;
                        print_error($E{Cannot open file argument});
                }
                else  {
                        while  ((ch = getc(inf)) != EOF)
                                putc(ch, res);
                        fclose(inf);
                }
                fclose(res);
                exit(0);
        }
#endif

 noopen:
#endif /* !HAVE_SETEUID */
        disp_str = arg;
        print_error($E{Cannot open file argument});
        return  (FILE *) 0;
}

#include "inline/spopts_btr.c"

/* Convert environment.  */

static void  convert_envir(char **remenviron)
{
        char    **ep;
        int     envcount = 0;

        for  (ep = remenviron;  *ep;  ep++)  {
                char    *eqp, *ncopy;
                if  (!(eqp = strchr(*ep, '='))) /* Don't understand no = */
                        continue;
                if  (envcount < MAXJENVIR)  {
                        unsigned  lng = eqp - *ep;
                        if  ((ncopy = malloc(lng + 1)) == (char *) 0)
                                ABORT_NOMEM;
                        BLOCK_COPY(ncopy, *ep, lng);
                        ncopy[lng] = '\0';
                        Envs[envcount].e_name = ncopy;
                        Envs[envcount].e_value = eqp + 1;
                }
                envcount++;
        }
        if  (envcount > MAXJENVIR)  {
                disp_arg[0] = envcount;
                disp_arg[1] = MAXJENVIR;
                print_error($E{Too large environment});
                JREQ->h.bj_nenv = MAXJENVIR;
        }
        else
                JREQ->h.bj_nenv = (USHORT) envcount;
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        FILE            *inf;
        int             outsock, exitcode = 0, ret, ch;
        char            *Curr_pwd = (char *) 0;
        jobno_t         jn;
        unsigned        defavoid = 0;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        init_xenv();

        Realuid = Repluid = getuid();
        Effuid = geteuid();
        Effgid = getegid();
        Realgid = Replgid = getgid();
        INIT_DAEMUID

        JREQ = &Out_job;
        Mode_arg = &Out_job.h.bj_mode;

        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK

        /* Before reading arguments, read in days to avoid */

        for  (ch = 0;  ch < TC_NDAYS;  ch++)
                if  (helpnstate($N{Base for days to avoid}+ch) > 0)
                        defavoid |= 1 << ch;

        if  ((defavoid & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)  {
                print_error($E{Avoiding all in defaults});
                exit(E_SETUP);
        }

        if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
                disp_arg[9] = $Q{Repeat unit abbrev};
                print_error($E{Missing alternative code});
        }
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                print_error($E{Missing alternative code});
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});

        SWAP_TO(Daemuid);
        JREQ->h.bj_slotno = -1;
        time(&JREQ->h.bj_time);

        /* These are default values.  */

        JREQ->h.bj_ll = 0;              /* Reset later at other end perhaps */
        JREQ->h.bj_times.tc_istime = 0; /* No time spec */
        JREQ->h.bj_times.tc_nexttime = 0;
        JREQ->h.bj_times.tc_repeat = TC_DELETE;
        JREQ->h.bj_times.tc_nposs = TC_WAIT1;
        JREQ->h.bj_times.tc_nvaldays = (USHORT) defavoid;
        JREQ->h.bj_exits.elower = 1;
        JREQ->h.bj_exits.eupper = 255;
        JREQ->h.bj_jflags = 0;          /* The default case */

        /* Shuffle to real uid again so that we can read in
           environment and .xibatch files.  */

        SWAP_TO(Realuid);

        /* Slurp up arguments in the usual sort of way.  */

        argv = optprocess(argv, Adefs, optprocs, $A{btr arg explain}, $A{btr arg freeze home}, 0);
        SWAP_TO(Daemuid);

        /* We must specify a host name */

        if  (Out_host == 0)  {
                if    (!(Anychanges & OF_ANY_FREEZE_WANTED) || argv[0])  {
                        print_error(Procparchanges & OF_HOST_CHANGE? $E{Is my host}: $E{No remote host specified});
                        exit(E_USAGE);
                }
                if  (!(mypriv = (Btuser *) malloc(sizeof(Btuser))))
                        ABORT_NOMEM;
                /* Forgery so we bypass the checks and can do the +freeze options */
                mypriv->btu_isvalid = 1;
                mypriv->btu_minp = 1;
                mypriv->btu_maxp = 255;
                mypriv->btu_defp = U_DF_DEFP;
                mypriv->btu_user = Realuid;
                mypriv->btu_priv = ~0L;
                mypriv->btu_maxll = mypriv->btu_totll = 0x7fff;
                mypriv->btu_spec_ll = U_DF_SPECLL;
                mypriv->btu_jflags[0] = U_DF_UJ;
                mypriv->btu_jflags[1] = U_DF_GJ;
                mypriv->btu_jflags[2] = U_DF_OJ;
                mypriv->btu_vflags[0] = U_DF_UV;
                mypriv->btu_vflags[1] = U_DF_GV;
                mypriv->btu_vflags[2] = U_DF_OV;
        }

        /* Get user's privileges. Routine aborts if trouble arises.  */

        realuname = prin_uname(Realuid);
        if  (Out_host)
                mypriv = remgetbtuser(Out_host, realuname, realgname);
        repluname = realuname;
        replgname = realgname;

        if  ((mypriv->btu_priv & BTM_CREATE) == 0)  {
                print_error($E{No create perm});
                exit(E_NOPRIV);
        }

        if  (!(Procparchanges & OF_PRIO_CHANGES))
                JREQ->h.bj_pri = mypriv->btu_defp;

        checksetmode(0, mypriv->btu_jflags, mypriv->btu_jflags[0], &JREQ->h.bj_mode.u_flags);
        checksetmode(1, mypriv->btu_jflags, mypriv->btu_jflags[1], &JREQ->h.bj_mode.g_flags);
        checksetmode(2, mypriv->btu_jflags, mypriv->btu_jflags[2], &JREQ->h.bj_mode.o_flags);

        if  (Realuid != Repluid)  {
                if  ((mypriv->btu_priv & BTM_WADMIN) == 0)  {
                        print_error($E{Cannot set owner});
                        exit(E_NOPRIV);
                }
                repluname = prin_uname(Repluid);
        }
        if  (Realgid != Replgid)  {
                if  ((mypriv->btu_priv & BTM_WADMIN) == 0)  {
                        print_error($E{Cannot set group});
                        exit(E_NOPRIV);
                }
                replgname = prin_gname(Replgid);
        }
        if  ((Procparchanges & (OF_ULIMIT_CHANGES|OF_UMASK_CHANGES)) != (OF_ULIMIT_CHANGES|OF_UMASK_CHANGES))  {
                USHORT          remumask = 0;
                ULONG           remulimit = 0;
                if  (Out_host)
                        remgetuml(Out_host, &remumask, &remulimit);
                if  (!(Procparchanges & OF_UMASK_CHANGES))
                        JREQ->h.bj_umask = remumask;
                if  (!(Procparchanges & OF_ULIMIT_CHANGES))
                        JREQ->h.bj_ulimit = remulimit;
        }

        /* Validate priority.  */

        if  (JREQ->h.bj_pri < mypriv->btu_minp || JREQ->h.bj_pri > mypriv->btu_maxp)  {
                disp_arg[0] = JREQ->h.bj_pri;
                disp_arg[1] = mypriv->btu_minp;
                disp_arg[2] = mypriv->btu_maxp;
                print_error(mypriv->btu_minp > mypriv->btu_maxp? $E{Cannot use GNUbatch}:
                            JREQ->h.bj_pri == mypriv->btu_defp? $E{Must specify priority}: $E{Invalid priority});
                exit(E_USAGE);
        }

        /* Validate load level - worry about permissions at other end.  */

        if  (Procparchanges & OF_LOADLEV_CHANGES  &&  JREQ->h.bj_ll > mypriv->btu_maxll)  {
                disp_arg[0] = JREQ->h.bj_ll;
                disp_arg[1] = mypriv->btu_maxll;
                print_error($E{Invalid load level});
                exit(E_USAGE);
        }

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  ((ret = repmnthfix(&JREQ->h.bj_times)))
                print_error(ret);

        chkfuture(JREQ, Verbose);

        JREQ->h.bj_nargs = Argcnt;
        JREQ->h.bj_nredirs = Redircnt;

        /* Either use remote host's environment, or mine, depending on option.  */

        if  (Outside_env)  {
                char    **remenv = remread_envir(Out_host), **sq_env;
                sq_env = squash_envir(remenv, environ);
                convert_envir(sq_env);
                if  (sq_env != environ)
                        free((char *) sq_env);
                /* Dont free remenv as "convert_envir" may use bits of it */
        }
        else  {
                char    **sq_env = squash_envir(xenviron, environ);
                convert_envir(sq_env);
                if  (sq_env != environ)
                        free((char *) sq_env);
        }

        if  (argv[0] == (char *) 0)  {
                char    *fulltitle = (char *) 0;

                if  (!job_title)  {
                        if  (jobqueue)  {
                                fulltitle = malloc((unsigned) strlen(jobqueue) + 2);
                                if  (!fulltitle)
                                        ABORT_NOMEM;
                                sprintf(fulltitle, "%s:", jobqueue);
                        }
                }
                else  if  (strchr(job_title, ':')  ||  !jobqueue)
                        fulltitle = job_title;
                else  {
                        fulltitle = malloc((unsigned)(strlen(jobqueue) + strlen(job_title) + 2));
                        if  (!fulltitle)
                                ABORT_NOMEM;
                        sprintf(fulltitle, "%s:%s", jobqueue, job_title);
                }

                if  (!packjstring(JREQ, job_cwd, fulltitle, Redirs, Envs, Args))  {
                        print_error($E{Too many job strings});
                        exit(E_LIMIT);
                }

                if  ((outsock = remgoutfile(Out_host, JREQ)) < 0)
                        exit(E_SETUP);

                inf = ginfile((char *) 0);
                remsub_copyout(inf, outsock, repluname, replgname);

                jn = remprocreply(outsock);
                if  (jn != 0  &&  Verbose)  {
                        static  char    *stdin_pr;
                        disp_str = job_title;
                        if  (!stdin_pr)
                                stdin_pr = gprompt($P{Expecting terminal input});
                        disp_str2 = stdin_pr;
                        disp_arg[0] = jn;
                        print_error(job_title && job_title[0]? $E{Job created with name}: $E{Job created without name});
                }

                /* Close input and output files.  */

                fclose(inf);
                close(outsock);
                exit(0);
        }

        do  {
                char    *fulltitle, *alloctitle = (char *) 0;

                /* Extract last part of name */

                if  (!(Dispflags & DF_HAS_HDR))  {
                        char    *name;
                        if  ((name = strrchr(*argv, '/')) != (char *) 0)
                                name++;
                        else
                                name = *argv;
                        /* Free existing title from last iteration perhaps */
                        if  (job_title)
                                free(job_title);
                        job_title = stracpy(name);
                }

                /* If title has got a ":" on it already, use that
                   prefix, otherwise stick our one on the front.  */

                if  (strchr(job_title, ':')  ||  !jobqueue)
                        fulltitle = job_title;
                else  {
                        fulltitle = alloctitle = malloc((unsigned)(strlen(jobqueue) + strlen(job_title) + 2));
                        if  (!alloctitle)
                                ABORT_NOMEM;
                        sprintf(alloctitle, "%s:%s", jobqueue, job_title);
                }

                if  (!packjstring(JREQ, job_cwd, fulltitle, Redirs, Envs, Args))  {
                        print_error($E{Too many job strings});
                        exit(E_LIMIT);
                }
                if  (alloctitle)
                        free(alloctitle);

                /* Open the spool output file and input files.  */

                if  ((inf = ginfile(*argv)) == (FILE *) 0)  {
                        exitcode = E_FALSE;
                        continue;
                }

                if  (!(outsock = remgoutfile(Out_host, JREQ)))
                        continue;
                remsub_copyout(inf, outsock, realuname, realgname);
                jn = remprocreply(outsock);

                if  (jn != 0  &&  Verbose)  {
                        disp_str = job_title;
                        disp_str2 = *argv;
                        disp_arg[0] = jn;
                        print_error(job_title && job_title[0]? $E{Job created with name}: $E{Job created without name});
                }

                /* Close input and output files.  */

                fclose(inf);
                close(outsock);
        }  while  (*++argv != (char *) 0);

        return  exitcode;
}
