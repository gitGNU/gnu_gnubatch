/* btr.c -- main program for gbch-r

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
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
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpalt.h"
#include "helpargs.h"
#include "cfile.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "jvuprocs.h"
#include "btrvar.h"
#include "spitrouts.h"
#include "optflags.h"
#include "shutilmsg.h"

#define BTR_INLINE

#define IPC_MODE        0

#ifndef ROOTID
#define ROOTID  0
#endif

#define C_MASK          0377    /* Umask value */
#define NAMESIZE        14      /* Padding for temp file */

static  unsigned  defavoid;

extern  char    **environ, **xenviron;

char    *Curr_pwd;

jobno_t         job_num;

Shipc           Oreq;
extern  long    mymtype;
ULONG           Saveseq;

char    Verbose,
        Outside_env;

extern  netid_t Out_host;

char    *spdir,
        *tmpfl;

char *spath(const char *, const char *);
void  chkfuture(BtjobRef, const int);
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
DEOPTION(o_interpreter);
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

        if  (Oreq.sh_params.uuid != nuid && (mypriv->btu_priv & BTM_WADMIN) == 0)  {
                arg_errnum = $E{Cannot set owner};
                return  OPTRESULT_ERROR;
        }
        Oreq.sh_params.uuid = nuid;
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
        if  (Oreq.sh_params.ugid != ngid  &&  (mypriv->btu_priv & BTM_WADMIN) == 0)  {
                arg_errnum = $E{Cannot set group};
                return  OPTRESULT_ERROR;
        }
        Oreq.sh_params.ugid = ngid;
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

#include "inline/btradefs.c"
#include "inline/btroptp.c"

/* On a signal, remove file.  */

RETSIGTYPE  catchit(int n)
{
        unlink(tmpfl);
        exit(E_SIGNAL);
}

/* The following stuff is to try to keep consistency with the
   environment from which we are called.  We keep ignoring the
   signals we were ignoring, and sometimes we catch, sometimes we
   ingore the rest.  */

static  char    sigstocatch[] = { SIGINT, SIGQUIT, SIGTERM, SIGHUP };
static  char    sigchkd;        /*  Worked out which ones once  */
static  char    sig_ignd[sizeof(sigstocatch)];  /*  Ignore these indices */

/* If we have got better signal handling, use it.
   What a hassle!!!!!  */

void  catchsigs()
{
        int     i;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;

        z.sighandler_el = catchit;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;

        if  (sigchkd)  {
                for  (i = 0;  i < sizeof(sigstocatch);  i++)
                        if  (!sig_ignd[i])
                                sigact_routine(sigstocatch[i], &z, (struct sigstruct_name *) 0);
        }
        else  {
                sigchkd++;
                for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
                        struct  sigstruct_name  oldsig;
                        sigact_routine(sigstocatch[i], &z, &oldsig);
                        if  (oldsig.sighandler_el == SIG_IGN)  {
                                sigact_routine(sigstocatch[i], &oldsig, (struct sigstruct_name *) 0);
                                sig_ignd[i] = 1;
                        }
                }
        }
#else
        if  (sigchkd)  {
                for  (i = 0;  i < sizeof(sigstocatch);  i++)
                        if  (!sig_ignd[i])
                                signal(sigstocatch[i], catchit);
        }
        else  {
                sigchkd++;
                for  (i = 0;  i < sizeof(sigstocatch);  i++)
                        if  (signal(sigstocatch[i], catchit) == SIG_IGN)  {
                                signal(sigstocatch[i], SIG_IGN);
                                sig_ignd[i] = 1;
                        }
        }
#endif
}

static void  holdsigs()
{
        int     i;
#ifdef  HAVE_SIGACTION
        sigset_t        sset;
        sigemptyset(&sset);
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                sigaddset(&sset, sigstocatch[i]);
        sigprocmask(SIG_BLOCK, &sset, (sigset_t *) 0);

#elif   defined(STRUCT_SIG)

        int     msk = 0;
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                msk |= sigmask(sigstocatch[i]);
        sigsetmask(msk);

#elif   defined(HAVE_SIGSET)

        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (!sig_ignd[i])
                        sighold(sigstocatch[i]);

#else
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (!sig_ignd[i])
                        signal(sigstocatch[i], SIG_IGN);
#endif
}

static void  default_sigs()
{
        int     i;

#ifdef  HAVE_SIGACTION
        struct  sigaction       ss;
        sigset_t        sset;
        ss.sa_handler = SIG_DFL;
        sigemptyset(&ss.sa_mask);
        ss.sa_flags = 0;
        sigemptyset(&sset);
        for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
                sigaddset(&sset, sigstocatch[i]);
                sigaction(sigstocatch[i], &ss, (struct sigaction *) 0);
        }
        sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);

#elif   defined(STRUCT_SIG)
        struct  sigstruct_name  ss;
        ss.sv_handler = SIG_DFL;
        ss.sv_mask = 0;
        ss.sv_flags = 0;
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                sigact_routine(sigstocatch[i], &ss, (struct sigstruct_name *) 0);
        sigsetmask(0);

#else

        for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
                if  (!sig_ignd[i])
                        signal(sigstocatch[i], SIG_DFL);
#ifdef  HAVE_SIGSET
                sigrelse(sigstocatch[i]);
#endif
        }
#endif
}

/* Generate output file name */

FILE *goutfile()
{
        FILE    *res;
        int     fid;

        for  (;;)  {
                sprintf(tmpfl, "%s/%s", spdir, mkspid(SPNAM, job_num));
                if  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0666)) >= 0)
                        break;
                job_num += JN_INC;
        }
        catchsigs();

        if  ((res = fdopen(fid, "w")) == (FILE *) 0)  {
                unlink(tmpfl);
                ABORT_NOMEM;
        }
        return  res;
}

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

#else  /* ! HAVE_SETEUID */
#ifdef  ID_SWAP

        /* If we can shuffle between uids, revert to real uid to get
           at file.  Use "access" call if we are sticking to root. */

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

                if  (lastpid >= 0)  {   /* Clean up zombie from last time around */
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

/* Enqueue request to batch scheduler.  */

void  enqueue(char *name)
{
        unsigned        retc;
        int     blkcount = MSGQ_BLOCKS;

        JREQ->h.bj_job = job_num;

        holdsigs();
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN  ||  --blkcount <= 0)  {
                        print_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
                        unlink(tmpfl);
                        exit(E_SETUP);
                }
                sleep(MSGQ_BLOCKWAIT);
        }
        if  ((retc = readreply()) != J_OK)  {
                unlink(tmpfl);
                if  ((retc & REQ_TYPE) != JOB_REPLY)  { /* Not expecting net errors */
                        disp_arg[0] = retc;
                        print_error($E{Unexpected sched message});
                }
                else  {
                        disp_str = job_title;
                        print_error((int) ((retc & ~REQ_TYPE) + $E{Base for scheduler job errors}));
                }
                exit(E_NOPRIV);
        }

        default_sigs();

        if  (Verbose)  {
                static  char    *stdin_pr;

                disp_str = job_title;
                if  (!stdin_pr)
                        stdin_pr = gprompt($P{Expecting terminal input});
                disp_str2 = name? name: stdin_pr;
                disp_arg[0] = job_num;
                print_error(job_title && job_title[0]? $E{Job created with name}: $E{Job created without name});
        }
}

static void  fixcsvars()
{
        int     nn;

        /* Don't worry if none are specified */

        if  (Condcnt <= 0  &&  Asscnt <= 0)
                return;

        openvfile(0, 0);
        rvarfile(0);

        for  (nn = 0;  nn < Condcnt;  nn++)  {
                struct  scond   *sp = &Condlist[nn];
                JcondRef        jc = &JREQ->h.bj_conds[nn];
                vhash_t         vp = lookupvar(sp->vd.var, sp->vd.hostid, BTM_READ, &Saveseq);

                if  (vp < 0)  {
                        disp_str = sp->vd.var;
                        print_error($E{Unreadable variable});
                        exit(E_NOPRIV);
                }

                jc->bjc_compar = sp->compar;
                jc->bjc_varind = vp;
                jc->bjc_value = sp->value;
                jc->bjc_iscrit = sp->vd.hostid? sp->vd.crit: 0;
        }

        for  (nn = 0;  nn < Asscnt;  nn++)  {
                struct  Sass    *sp = &Asslist[nn];
                JassRef ja = &JREQ->h.bj_asses[nn];
                vhash_t vp = lookupvar(sp->vd.var, sp->vd.hostid, (unsigned)(sp->op == BJA_ASSIGN? BTM_WRITE: BTM_READ|BTM_WRITE), &Saveseq);

                if  (vp < 0)  {
                        disp_str = sp->vd.var;
                        print_error($E{Unwritable variable});
                        exit(E_NOPRIV);
                }

                ja->bja_flags = sp->flags;
                ja->bja_op = sp->op;
                ja->bja_varind = vp;
                ja->bja_con = sp->con;
                ja->bja_iscrit = sp->vd.hostid? sp->vd.crit: 0;
        }
        vunlock();
}

/* Read environment.
   In fact we "squash out" everything the same as
   in the static environment table.  */

static void  read_envir()
{
        char    **ep, **sq_env;
        int     envcount = 0;

        sq_env = squash_envir(xenviron, environ);

        for  (ep = sq_env;  *ep;  ep++)  {
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

        if  (sq_env != environ)
                free((char *) sq_env);
}

#include "inline/spopts_btr.c"

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        FILE  *inf, *outf;
        int  ch, exitcode = 0, ret;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        ULONG           indx;
        char            **origargv = argv;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        init_xenv();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
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

        /* Set up file names.  */

        spdir = envprocess(SPDIR);
        if  ((tmpfl = malloc((unsigned)(strlen(spdir) + 2 + NAMESIZE))) == (char *) 0)
                ABORT_NOMEM;

        /* See if scheduler is running.
           Unlike Xi-Text (pre 23) we don't start
           it because we need to look at semaphores and all sorts */

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif
        initxbuffer(0);
        JREQ = &Xbuffer->Ring[indx = getxbuf()];
        Mode_arg = &JREQ->h.bj_mode;
        BLOCK_ZERO(JREQ, sizeof(Btjob));
        JREQ->h.bj_slotno = -1;
        JREQ->h.bj_umask = Save_umask = umask(C_MASK);
        JREQ->h.bj_ulimit = 0L;
        time(&JREQ->h.bj_time);

        /* Set up scheduler request main parameters.  */

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = J_CREATE;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        Oreq.sh_un.sh_jobindex = indx;
        mymtype = MTOFFSET + (job_num = Oreq.sh_params.upid = getpid());

        /* Get user's privileges.
           Check that he can create a job and copy stuff into job */

        mypriv = getbtuser(Realuid);

        if  ((mypriv->btu_priv & BTM_CREATE) == 0)  {
                print_error($E{No create perm});
                exit(E_NOPRIV);
        }

        JREQ->h.bj_pri = mypriv->btu_defp;

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }

        /* These are default values.  */

        strcpy(JREQ->h.bj_cmdinterp, Ci_list[CI_STDSHELL].ci_name);
        JREQ->h.bj_ll = Ci_list[CI_STDSHELL].ci_ll;
        /* bj_runtime = bj_autoksig = bj_runon = bj_deltime = 0 */
        /* JREQ->h.bj_times.tc_istime = 0;      No time spec */
        JREQ->h.bj_times.tc_repeat = TC_DELETE;
        JREQ->h.bj_times.tc_nposs = TC_WAIT1;
        JREQ->h.bj_times.tc_nvaldays = (USHORT) defavoid;
        JREQ->h.bj_exits.elower = 1;
        JREQ->h.bj_exits.eupper = 255;
        /* JREQ->h.bj_jflags = 0;               The default case */

        /* Shuffle to real uid again so that we can read in
           environment and .xibatch files.  */

        SWAP_TO(Realuid);
        read_envir();
        argv = optprocess(argv, Adefs, optprocs, $A{btr arg explain}, $A{btr arg freeze home}, 0);
        SWAP_TO(Daemuid);

        /* If we haven't got a directory, use the current */

        if  (!(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();
        if  (!job_cwd)
                job_cwd = Curr_pwd;

        /* If sending to remote, invoke gbch-rr with the same arguments
           we started with not the ones constructed.
           By the way we don't let gbch-rr re-invoke
           this or we could loop forever.  */

        if  (Out_host)  {
                char    *npath;
                freexbuf(indx);
                npath = spath("gbch-rr", Curr_pwd);
                if  (!npath)  {
                        print_error($E{Cannot find rbtr});
                        exit(E_USAGE);
                }
                umask(Save_umask);
                execv(npath, origargv);
                print_error($E{Cannot run rbtr});
                exit(E_SETUP);
        }

        /* Validate priority.  */

        if  (JREQ->h.bj_pri < mypriv->btu_minp || JREQ->h.bj_pri > mypriv->btu_maxp)  {
                disp_arg[0] = JREQ->h.bj_pri;
                disp_arg[1] = mypriv->btu_minp;
                disp_arg[2] = mypriv->btu_maxp;
                print_error(mypriv->btu_minp > mypriv->btu_maxp?
                            $E{Cannot use GNUbatch}:
                            JREQ->h.bj_pri == mypriv->btu_defp?
                            $E{Must specify priority}: $E{Invalid priority});
                exit(E_USAGE);
        }

        /* Validate load level */

        if  (JREQ->h.bj_ll > mypriv->btu_maxll)  {
                disp_arg[0] = JREQ->h.bj_ll;
                disp_arg[1] = mypriv->btu_maxll;
                print_error($E{Invalid load level});
                exit(E_USAGE);
        }

        checksetmode(0, mypriv->btu_jflags, mypriv->btu_jflags[0], &JREQ->h.bj_mode.u_flags);
        checksetmode(1, mypriv->btu_jflags, mypriv->btu_jflags[1], &JREQ->h.bj_mode.g_flags);
        checksetmode(2, mypriv->btu_jflags, mypriv->btu_jflags[2], &JREQ->h.bj_mode.o_flags);

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  ((ret = repmnthfix(&JREQ->h.bj_times)))
                print_error(ret);

        chkfuture(JREQ, Verbose);

        /* Check condition and set vars */

        fixcsvars();
        JREQ->h.bj_nargs = Argcnt;
        JREQ->h.bj_nredirs = Redircnt;

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

                outf = goutfile();
                inf = ginfile((char *) 0);

                /* Copy to output file.  */

                while  ((ch = getc(inf)) != EOF)  {
                        if  (putc(ch, outf) == EOF)  {
                                print_error($E{No room for job});
                                unlink(tmpfl);
                                exit(E_DFULL);
                        }
                }

                /* Close input and output files.  */

                fclose(inf);
                fclose(outf);

#ifdef  NHONSUID
                if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))
                        chown(tmpfl, Daemuid, Realgid);
#elif   !defined(HAVE_SETEUID)  &&  defined(ID_SWAP)
                if  (Daemuid != ROOTID  &&  Realuid == ROOTID)
                        chown(tmpfl, Daemuid, Realgid);
#endif

                /* Now enqueue request to batch scheduler.  */

                enqueue((char *) 0);
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
                outf = goutfile();

                /* Copy to output file.  */

                while  ((ch = getc(inf)) != EOF)  {
                        if  (putc(ch, outf) == EOF)  {
                                print_error($E{No room for job});
                                unlink(tmpfl);
                                exit(E_DFULL);
                        }
                }

                /* Close input and output files.  */

                fclose(inf);
                fclose(outf);

#ifdef  NHONSUID
                if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))
                        chown(tmpfl, Daemuid, Realgid);
#elif   !defined(HAVE_SETEUID)  &&  defined(ID_SWAP)
                if  (Daemuid != ROOTID  &&  Realuid == ROOTID)
                        chown(tmpfl, Daemuid, Realgid);
#endif

                /* Now enqueue request to batch scheduler.  */

                enqueue(*argv);
                job_num = (job_num + 1) % JN_INC;
        }  while  (*++argv != (char *) 0);

        return  exitcode;
}
