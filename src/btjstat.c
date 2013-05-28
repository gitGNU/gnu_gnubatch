/* btjstat.c -- main program for gbch-jstat

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "cmdint.h"
#include "jvuprocs.h"
#include "ecodes.h"
#include "errnums.h"
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "helpalt.h"
#include "optflags.h"
#include "cgifndjb.h"
#include "shutilmsg.h"

#define IPC_MODE        0600

ULONG                   Saveseq;

HelpaltRef      progresslist;

static  char    *stat_codes;

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static int  getstat(char *jobnum)
{
        int             retc;
        struct  jobswanted      jw;
        CBtjobRef               jp;

        if  ((retc = decode_jnum(jobnum, &jw)) != 0)  {
                print_error(retc);
                return  E_USAGE;
        }
        if  (!find_job(&jw))  {
                print_error($E{btjstat job not found});
                return  E_NOJOB;
        }

        jp = jw.jp;
        if  (stat_codes)  {
                char    *sp = stat_codes, *cp;
                char    *statc = disp_alt(jp->h.bj_progress, progresslist);
                int     cnt;
                while  ((cp = strchr(sp, ',')))  {
                        *cp = '\0';
                        cnt = ncstrcmp(sp, statc);
                        *cp = ',';
                        if  (cnt == 0)
                                return  E_TRUE;
                        sp = cp + 1;
                }
                return  ncstrcmp(sp, statc) == 0? E_TRUE: E_FALSE;
        }
        return  jp->h.bj_progress >= BJP_STARTUP1? E_TRUE: E_FALSE;
}

OPTION(o_explain)
{
        print_error($E{btjstat explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_defstat)
{
        if  (stat_codes)  {
                free(stat_codes);
                stat_codes = (char *) 0;
        }
        return  OPTRESULT_OK;
}

OPTION(o_stats)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (stat_codes)
                free(stat_codes);
        stat_codes = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btjstat arg explain} },
  {  'd', $A{btjstat arg defaults} },
  {  's', $A{btjstat arg statcodes} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_defstat,      o_stats,
o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        fprintf(dest, "%s", name);
        if  (stat_codes)  {
                spitoption($A{btjstat arg statcodes}, $A{btjstat arg explain}, dest, '=', 0);
                fprintf(dest, " \"%s\"", stat_codes);
        }
        else
                spitoption($A{btjstat arg defaults}, $A{btjstat arg explain}, dest, '=', 0);
        putc('\n', dest);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *Curr_pwd = (char *) 0;
        int     ret;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        char    *whichj;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        tzset();
        SWAP_TO(Daemuid);
        mypriv = getbtuser(Realuid);
        SWAP_TO(Realuid);
        argv = optprocess(argv, Adefs, optprocs, $A{btjstat arg explain}, $A{btjstat arg freeze home}, 0);

#include "inline/freezecode.c"

        if  (Anychanges & OF_ANY_FREEZE_WANTED)
                exit(0);

        if  (argv[0])  {
                whichj = *argv++;
                if  (*argv)
                        goto  usage;
        }
        else  {
        usage:
                print_error($E{btjstat usage});
                exit(E_USAGE);
        }

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

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

        /* Open the other files. No read yet until the scheduler is
           aware of our existence, which it won't be until we
           send it a message.  */

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
        openjfile(0, 0);
        rjobfile(1);

        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
                print_error($E{Missing alternative code});
        }
        return  getstat(whichj);
}
