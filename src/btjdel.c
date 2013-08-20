/* btjdel.c -- main program for gbch-jdel

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btuser.h"
#include "ecodes.h"
#include "statenums.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "shreq.h"
#include "files.h"
#include "helpargs.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "optflags.h"
#include "cgifndjb.h"
#include "shutilmsg.h"

#define DEFSLEEP        4
#define  MAXFNAME       200

char    *Curr_pwd;

#define IPC_MODE        0600

#ifndef USING_FLOCK
int     Sem_chan;
#endif

ULONG           indx;

int     killtype = SIGTERM;
unsigned        sleeptime = DEFSLEEP;

int     exit_code;

char    force,
        nodel,
        unqueue,
        XML_jobdump;

char    *jobprefix,
        *cmdprefix;

enum    cmdtype { CMD_DELETE, CMD_GO, CMD_GOADV, CMD_ADV };

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

/* Run job dump program if possible */

static void  dounqueue(CBtjobRef jp)
{
        PIDTYPE pid;
        int     ac;
        char    *argv[12];
        static  char    *udprog;
        char    cprefbuf[MAXFNAME + 8], jprefbuf[MAXFNAME + 8 + sizeof(XMLJOBSUFFIX)];

        if  (!udprog)
                udprog = envprocess(XML_jobdump? XMLDUMPJOB: DUMPJOB);

        if  ((pid = fork()))  {
                int     status;

                if  (pid < 0)  {
                        print_error($E{btjdel cannot fork});
                        return;
                }
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                if  (status == 0)       /* All ok */
                        return;
                if  (status & 0xff)  {
                        disp_arg[0] = status & 0xff;
                        print_error($E{Unqueue process aborted});
                        return;
                }
                status = (status >> 8) & 0xff;
                disp_arg[0] = jp->h.bj_job;
                disp_str = title_of(jp);
                switch  (status)  {
                default:
                        disp_arg[1] = status;
                        print_error($E{Undef err unq});
                        return;
                case  E_JDFNFND:
                        print_error($E{Unq file not found});
                        return;
                case  E_JDNOCHDIR:
                        print_error($E{Unq dir not found});
                        return;
                case  E_JDFNOCR:
                        print_error($E{Unq cannot create file});
                        return;
                case  E_JDJNFND:
                        print_error($E{Unq job not found});
                        return;
                case  E_CANTDEL:
                        print_error($E{Unq cannot del});
                        return;
                case  E_NOTIMPL:
                        print_error($E{No XML library});
                        return;
                }
        }

        /* Child process */

        setuid(Realuid);
        Ignored_error = chdir(Curr_pwd);        /* So that it picks up config file correctly */
        if  (XML_jobdump)
                sprintf(jprefbuf, "%s%.6ld" XMLJOBSUFFIX, jobprefix, (long) jp->h.bj_job);
        else  {
                sprintf(cprefbuf, "%s%.6ld", cmdprefix, (long) jp->h.bj_job);
                sprintf(jprefbuf, "%s%.6ld", jobprefix, (long) jp->h.bj_job);
        }
        if  (!(argv[0] = strrchr(udprog, '/')))
                argv[0] = udprog;
        else
                argv[0]++;
        ac = 0;
        if  (nodel)
                argv[++ac] = "-n";
        if  (XML_jobdump)  {
                argv[++ac] = "-j";
                argv[++ac] = (char *) JOB_NUMBER(jp);
                argv[++ac] = "-d";
                argv[++ac] = job_cwd;
                argv[++ac] = "-f";
        }
        else  {
                argv[++ac] = (char *) JOB_NUMBER(jp);
                argv[++ac] = job_cwd;
                argv[++ac] = cprefbuf;
        }
        argv[++ac] = jprefbuf;
        argv[++ac] = (char *) 0;
        execv(udprog, argv);
        exit(E_SETUP);
}

/* This is the main processing routine for delete */

void  process(char **joblist)
{
        char    *jobc, **jobp;
        int     sleepfor = 0;

        for  (jobp = joblist;  (jobc = *jobp);  jobp++)  {
                int                     retc;
                struct  jobswanted      jw;
                CBtjobRef               jp;

                if  ((retc = decode_jnum(jobc, &jw)) != 0)  {
                        print_error(retc);
                        exit_code = E_NOJOB;
                        continue;
                }
                if  (!find_job(&jw))  {
                        disp_str = jobc;
                        print_error($E{Cannot find job});
                        exit_code = E_NOJOB;
                        continue;
                }

                jp = jw.jp;
                if  (jp->h.bj_progress >= BJP_STARTUP1)  {
                        if  (!force)  {
                                disp_str = jobc;
                                print_error($E{btjdel job running});
                                continue;
                        }
                        wjimsg_param(J_KILL, killtype, jp);
                        if  ((retc = readreply()) != J_OK)
                                print_error(dojerror(retc, jp));
                        else
                                sleepfor++;
                        continue;
                }
                if  (unqueue)
                        dounqueue(jp);
                else  if  (!nodel)  {
                        wjimsg(J_DELETE, jp);
                        if  ((retc = readreply()) != J_OK)
                                print_error(dojerror(retc, jp));
                }
        }

        if  (sleepfor)  {
                sleep(sleeptime);
                rjobfile(1);

                for  (jobp = joblist;  (jobc = *jobp);  jobp++)  {
                        int                     retc;
                        struct  jobswanted      jw;
                        CBtjobRef               jp;

                        if  (decode_jnum(jobc, &jw) != 0)
                                continue;

                        if  (!find_job(&jw))
                                continue;

                        jp = jw.jp;

                        if  (jp->h.bj_progress >= BJP_STARTUP1)  {
                                disp_str = jobc;
                                disp_str2 = title_of(jp);
                                disp_arg[0] = sleeptime;
                                print_error($E{btjdel job still running});
                                exit_code = E_FALSE;
                                continue;
                        }

                        if  (unqueue)
                                dounqueue(jp);
                        else  if  (!nodel)  {
                                wjimsg(J_DELETE, jp);
                                if  ((retc = readreply()) != J_OK)
                                        dojerror(retc, jp);
                        }
                }
        }
}

/* This is the main processing routine for other operations */

void  cprocess(enum cmdtype which, char **joblist)
{
        unsigned  cmd = which == CMD_GOADV? J_FORCE: J_FORCENA;
        char    *jobc, **jobp;

        for  (jobp = joblist;  (jobc = *jobp);  jobp++)  {
                int                     retc;
                struct  jobswanted      jw;
                CBtjobRef               jp;

                if  ((retc = decode_jnum(jobc, &jw)) != 0)  {
                        print_error(retc);
                        exit_code = E_NOJOB;
                        continue;
                }

                if  (!find_job(&jw))  {
                        disp_str = jobc;
                        print_error($E{Cannot find job});
                        exit_code = E_NOJOB;
                        continue;
                }

                jp = jw.jp;

                if  (jp->h.bj_progress >= BJP_STARTUP1)  {
                        if  (!force)  {
                                disp_str = jobc;
                                print_error($E{btjdel job running});
                                continue;
                        }
                        continue;
                }
                else  {
                        wjimsg(cmd, jp);
                        if  ((retc = readreply()) != J_OK)
                                dojerror(retc, jp);
                }
        }
}

void  advprocess(char **joblist)
{
        char    *jobc, **jobp;

        for  (jobp = joblist;  (jobc = *jobp);  jobp++)  {
                int                     retc;
                struct  jobswanted      jw;
                CBtjobRef               jp;

                if  ((retc = decode_jnum(jobc, &jw)) != 0)  {
                        print_error(retc);
                        exit_code = E_NOJOB;
                        continue;
                }

                if  (!find_job(&jw))  {
                        disp_str = jobc;
                        print_error($E{Cannot find job});
                        exit_code = E_NOJOB;
                        continue;
                }

                jp = jw.jp;

                if  (jp->h.bj_progress >= BJP_STARTUP1)  {
                        disp_str = jobc;
                        print_error($E{btjdel job running});
                        continue;
                }
                if  (!jp->h.bj_times.tc_istime)
                        continue;
                *JREQ = *jp;
                JREQ->h.bj_times.tc_nexttime = advtime(&JREQ->h.bj_times);
                wjxfermsg(J_CHANGE, indx);
                if  ((retc = readreply()) != J_OK)
                        dojerror(retc, jp);
        }
}


#define BTJDEL_INLINE

OPTION(o_explain)
{
        print_error($E{btjdel explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_noforce)
{
        force = 0;
        return  OPTRESULT_OK;
}

OPTION(o_force)
{
        force = 1;
        return  OPTRESULT_OK;
}

OPTION(o_nodel)
{
        nodel = 1;
        return  OPTRESULT_OK;
}

OPTION(o_del)
{
        nodel = 0;
        return  OPTRESULT_OK;
}

OPTION(o_signo)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        killtype = atoi(arg);
        if  (killtype <= 0 || killtype >= NSIG)  {
                disp_arg[0] = 0;
                disp_arg[1] = NSIG;
                arg_errnum = $E{Bad kill type};
                return  OPTRESULT_ERROR;
        }
        return  OPTRESULT_ARG_OK;
}

OPTION(o_sleeptime)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        sleeptime = atoi(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_nounqueue)
{
        unqueue = XML_jobdump = 0;
        return  OPTRESULT_OK;
}

OPTION(o_unqueue)
{
        unqueue = 1;
        XML_jobdump = 0;
        return  OPTRESULT_OK;
}

OPTION(o_xmlunqueue)
{
        unqueue = XML_jobdump = 1;
        return  OPTRESULT_OK;
}

OPTION(o_jobprefix)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (strlen(arg) > MAXFNAME)  {
                arg_errnum = $E{btjdel file name too long};
                return  OPTRESULT_ERROR;
        }
        free(jobprefix);
        jobprefix = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_cmdprefix)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (strlen(arg) > MAXFNAME)  {
                arg_errnum = $E{btjdel file name too long};
                return  OPTRESULT_ERROR;
        }
        free(cmdprefix);
        cmdprefix = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

DEOPTION(o_directory);
DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  { '?', $A{btjdel arg explain} },
  { 'n', $A{btjdel arg noforce} },
  { 'N', $A{btjdel arg noforce} },
  { 'y', $A{btjdel arg force} },
  { 'Y', $A{btjdel arg force} },
  { 'k', $A{btjdel arg nodel} },
  { 'd', $A{btjdel arg del} },
  { 'K', $A{btjdel arg wsig} },
  { 'S', $A{btjdel arg sleep} },
  { 'u', $A{btjdel arg unqueue} },
  { 'e', $A{btjdel arg nounqueue} },
  { 'J', $A{btjdel arg jobpref} },
  { 'C', $A{btjdel arg cmdpref} },
  { 'D', $A{btjdel arg dir} },
  { 'X', $A{btjdel arg xml unq} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_noforce,      o_force,        o_nodel,
o_del,          o_signo,        o_sleeptime,    o_unqueue,
o_nounqueue,    o_jobprefix,    o_cmdprefix,    o_directory,
o_xmlunqueue,   o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0;
        fprintf(dest, "%s", name);
        cancont = spitoption(force? $A{btjdel arg force}: $A{btjdel arg noforce}, $A{btjdel arg explain}, dest, '=', 0);
        cancont = spitoption(nodel? $A{btjdel arg nodel}: $A{btjdel arg del}, $A{btjdel arg explain}, dest, ' ', cancont);
        cancont = spitoption(unqueue? (XML_jobdump? $A{btjdel arg xml unq}: $A{btjdel arg unqueue}): $A{btjdel arg nounqueue}, $A{btjdel arg explain}, dest, ' ', cancont);
        spitoption($A{btjdel arg wsig}, $A{btjdel arg explain}, dest, ' ', 0);
        fprintf(dest, " %d", killtype);
        spitoption($A{btjdel arg sleep}, $A{btjdel arg explain}, dest, ' ', 0);
        fprintf(dest, " %d", sleeptime);
        spitoption($A{btjdel arg jobpref}, $A{btjdel arg explain}, dest, ' ', 0);
        fprintf(dest, " %s", jobprefix);
        spitoption($A{btjdel arg cmdpref}, $A{btjdel arg explain}, dest, ' ', 0);
        fprintf(dest, " %s", cmdprefix);
        if  (Procparchanges & OF_DIR_CHANGES)  {
                spitoption($A{btjdel arg dir}, $A{btjdel arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", job_cwd);
        }
        putc('\n', dest);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
	int	clng;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        enum    cmdtype which = CMD_DELETE;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        /* Select action according to program name */

        clng = strlen(progname);
        if  (ncstrcmp(progname+clng-2, "go") == 0)
        	which = CMD_GO;
        else  if  (ncstrcmp(progname+clng-5, "goadv") == 0)
        	which = CMD_GOADV;
        else  if  (ncstrcmp(progname+clng-3, "adv") == 0)
        	which = CMD_ADV;

        init_mcfile();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        jobprefix = gprompt($P{Default job file prefix});
        cmdprefix = gprompt($P{Default cmd file prefix});
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);
        mypriv = getbtuser(Realuid);
        SWAP_TO(Realuid);
        argv = optprocess(argv, Adefs, optprocs, $A{btjdel arg explain}, $A{btjdel arg freeze home}, 0);

        if  (unqueue)  {
                if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                        Curr_pwd = runpwd();
                if  (!job_cwd)
                        job_cwd = Curr_pwd;
        }

        SWAP_TO(Daemuid);

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  (argv[0] == (char *) 0)  {
                print_error($E{No jobs specified});
                exit(E_USAGE);
        }

        /* Grab message id */

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

        /* Open the other files.  */

        openjfile(0, 0);
        rjobfile(1);
        switch  (which)  {
        default:
                process(&argv[0]);
                break;
        case  CMD_GO:
        case  CMD_GOADV:
                cprocess(which, &argv[0]);
                break;
        case  CMD_ADV:
                initxbuffer(0);
                JREQ = &Xbuffer->Ring[indx = getxbuf()];
                advprocess(&argv[0]);
                break;
        }

        return  exit_code;
}
