/* btstart.c -- main program for gbch-start

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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
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
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "cmdint.h"
#include "shreq.h"
#include "btuser.h"
#include "files.h"
#include "ipcstuff.h"
#include "ecodes.h"
#include "errnums.h"
#include "helpargs.h"
#include "cfile.h"
#include "optflags.h"
#include "shutilmsg.h"

int     nosetpgrp = 0;
LONG    initll = -1, jsize = 0L, vsize = 0L;
float   decpri = 0.0;

extern  char    hostf_errors;

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

/* Find host name in host file or look it up and construct structure */

static  int  get_conn_host(const char *hostn, struct remote *drp)
{
        struct  remote  *rp;
        netid_t hostid;

        disp_str = hostn;       /* Prime this ready for our complaint */

        while  ((rp = get_hostfile()))
                if  (!(rp->ht_flags & HT_ROAMUSER)  &&  (strcmp(hostn, rp->hostname) == 0  ||  strcmp(hostn, rp->alias) == 0))  {

                        /* Don't allow connections to client machines */

                        if  (rp->ht_flags & HT_DOS)  {
                                print_error($E{Dos client as host});
                                return  E_NOHOST;
                        }
                        rp->ht_flags &= ~HT_MANUAL;
                        *drp = *rp;
                        end_hostfile();
                        if  (hostf_errors)
                                print_error($E{Warn errors in host file});
                        return  0;
                }
        end_hostfile();

        /* OK see if we can look it up */

        if  ((hostid = look_hostname(hostn)) == 0L)  {
                print_error($E{Btconn unknown host});
                return  E_NOHOST;
        }

        /* Manufacture struct */

        BLOCK_ZERO(drp, sizeof(struct remote));
        strncpy(drp->hostname, hostn, HOSTNSIZE-1);
        drp->hostid = hostid;
        return  0;
}

static int  doconnop(const unsigned cmd, const char *hostn)
{
        long            mymtype;
        Shipc           oreq;
        Repmess         rr;
        int     blkcount = MSGQ_BLOCKS, ret;

        if  ((ret = get_conn_host(hostn, &oreq.sh_un.sh_n)) != 0)
                return  ret;

        /* Check that the user is allowed to do this.  We check this
           here rather than in the scheduler to avoid hassles
           from automatically-done connections */

        mypriv = getbtuser(Realuid);
        if  ((mypriv->btu_priv & BTM_SSTOP) == 0)  {
                print_error($E{Btconn no conn priv});
                return  E_NOPRIV;
        }

        BLOCK_ZERO(&oreq, sizeof(oreq));
        oreq.sh_mtype = TO_SCHED;
        oreq.sh_params.mcode = cmd;
        oreq.sh_params.uuid = Realuid;
        oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (oreq.sh_params.upid = getpid());
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(Shreq) + sizeof(struct remote), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)  {
                        print_error($E{IPC msg q error});
                        exit(E_SETUP);
                }
                blkcount--;
                if  (blkcount <= 0)  {
                        print_error($E{IPC msg q full});
                        exit(E_SETUP);
                }
                sleep(MSGQ_BLOCKWAIT);
        }

        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
                if  (errno == EINTR)
                        continue;
                exit(E_SHUTD);
        }
        if  (rr.outmsg.mcode == N_CONNOK)
                return  0;
        print_error($E{Btconn connection fail});
        return  E_FALSE;
}

/* See what time the guy wants */

static time_t  g_time(const char *arg)
{
        time_t  now = time((time_t *) 0);
        struct  tm      *tn = localtime(&now);
        int     year, month, day, hour = 0, min = 0, num, num2, i;
        time_t  result, testit;

        year = tn->tm_year;

        while  (isspace(*arg))
                arg++;

        disp_str = arg;         /* In case of error */
        if  (!isdigit(*arg))
                goto  badtime;

        num = 0;
        do      num = num * 10 + *arg++ - '0';
        while  (isdigit(*arg));

        if  (*arg != '/')
                goto  badtime;

        arg++;
        if  (!isdigit(*arg))
                goto  badtime;

        num2 = 0;
        do      num2 = num2 * 10 + *arg++ - '0';
        while  (isdigit(*arg));

        if  (*arg == '/')  { /* First digits were year */
                if  (num > 1900)
                        year = num - 1900;
                else  if  (num > 110)
                        goto  badtime;
                else  if  (num < 90)
                        year = num + 100;
                else
                        year = num;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                month = num2;
                day = 0;
                do      day = day * 10 + *arg++ - '0';
                while  (isdigit(*arg));
        }
        else  {         /* Day/month or Month/day
                           Decide by which side of the Atlantic */
#ifdef  HAVE_TM_ZONE
                if  (tn->tm_gmtoff <= -4 * 60 * 60)
#else
                if  (timezone >= 4 * 60 * 60)
#endif
                {
                        month = num;
                        day = num2;
                }
                else  {
                        month = num2;
                        day = num;
                }
                if  (month < tn->tm_mon + 1  ||  (month == tn->tm_mon + 1 && day < tn->tm_mday))
                        year++;
        }
        if  (*arg != '\0')  {
                if  (*arg != ',')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                do      hour = hour * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (*arg != ':')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                do      min = min * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (*arg != '\0')
                        goto  badtime;
        }

        if  (month > 12  || hour > 23  || min > 59)
                goto  badtime;

        month_days[1] = year % 4 == 0? 29: 28;
        month--;
        year -= 70;
        if  (day > month_days[month])
                goto  badtime;

        result = year * 365;
        if  (year > 2)
                result += (year + 1) / 4;

        for  (i = 0;  i < month;  i++)
                result += month_days[i];
        result = (result + day - 1) * 24;

        /* Build it up once as at 12 noon and work out timezone shift from that */

        testit = (result + 12) * 60 * 60;
        tn = localtime(&testit);
        result = ((result + hour + 12 - tn->tm_hour) * 60 + min) * 60;
        return  result;

 badtime:
        print_error($E{Bad time spec});
        exit(E_USAGE);
        return  0;              /* Silence compilers */
}

static LONG  g_adj(const char *arg)
{
        while  (isspace(*arg))
                arg++;
        if  (!isdigit(*arg)  &&  *arg != '-')  {
                print_error($E{Bad dst adjustment});
                exit(E_USAGE);
        }
        return  (LONG) atol(arg);
}

static int  dodst(const int remaswell, const time_t starttime, const time_t endtime, const LONG adj)
{
        long            mymtype;
        Shipc           oreq;
        Repmess         rr;
        int     blkcount = MSGQ_BLOCKS;

        if  (starttime >= endtime  ||  adj == 0)  {
                print_error($E{Bad dst adjustment});
                exit(E_USAGE);
        }
        BLOCK_ZERO(&oreq, sizeof(oreq));
        oreq.sh_mtype = TO_SCHED;
        oreq.sh_params.mcode = J_DSTADJ;
        oreq.sh_params.uuid = Realuid;
        oreq.sh_params.ugid = Realgid;
        oreq.sh_params.param = remaswell;
        mymtype = MTOFFSET + (oreq.sh_params.upid = getpid());
        oreq.sh_un.sh_adj.sh_starttime = starttime;
        oreq.sh_un.sh_adj.sh_endtime = endtime;
        oreq.sh_un.sh_adj.sh_adjust = adj;
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(Shreq) + sizeof(struct adjstr), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)  {
                        print_error($E{IPC msg q error});
                        exit(E_SETUP);
                }
                blkcount--;
                if  (blkcount <= 0)  {
                        print_error($E{IPC msg q full});
                        exit(E_SETUP);
                }
                sleep(MSGQ_BLOCKWAIT);
        }
        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
                if  (errno == EINTR)
                        continue;
                exit(E_SHUTD);
        }
        if  (rr.outmsg.mcode == J_OK)
                return  0;
        print_error($E{No perm for DST});
        return  E_PERM;
}

OPTION(o_explain)
{
        print_error($E{Btstart explain});
        exit(0);
}

OPTION(o_initll)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        initll = atol(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_jsize)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        jsize = atol(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_vsize)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        vsize = atol(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_decpri)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        decpri = (float) atof(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_nosetpgrp)
{
        nosetpgrp = 1;
        return  OPTRESULT_OK;
}

OPTION(o_setpgrp)
{
        nosetpgrp = 0;
        return  OPTRESULT_OK;
}


DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btstart arg explain} },
  {  'l', $A{btstart arg initll} },
  {  'j', $A{btstart arg jsize} },
  {  'v', $A{btstart arg vsize} },
  {  'd', $A{btstart arg decpri} },
  {  't', $A{btstart arg nosetpgrp} },
  {  'T', $A{btstart arg setpgrp} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_initll,       o_jsize,        o_vsize,
o_decpri,       o_nosetpgrp,    o_setpgrp,
o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        fprintf(dest, "%s", name);
        spitoption(nosetpgrp? $A{btstart arg nosetpgrp}: $A{btstart arg setpgrp}, $A{btstart arg explain}, dest, '=', 0);
        spitoption($A{btstart arg initll}, $A{btstart arg explain}, dest, ' ', 0);
        fprintf(dest, " %ld", (long) initll);
        spitoption($A{btstart arg decpri}, $A{btstart arg explain}, dest, ' ', 0);
        fprintf(dest, " %g", decpri);
        spitoption($A{btstart arg jsize}, $A{btstart arg explain}, dest, ' ', 0);
        fprintf(dest, " %ld", (long) jsize);
        spitoption($A{btstart arg vsize}, $A{btstart arg explain}, dest, ' ', 0);
        fprintf(dest, " %ld\n", (long) vsize);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *Curr_pwd = (char *) 0;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

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
        SWAP_TO(Daemuid);

        if  (strcmp(progname, "gbch-conn") == 0 || strcmp(progname, "gbch-disconn") == 0)  {
                if  (!argv[1])  {
                        print_error($E{Btconn no machine arg});
                        return  E_USAGE;
                }
                if  (argv[2])  {
                        print_error($E{Btconn multi machine});
                        return  E_USAGE;
                }
                if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                        print_error($E{Scheduler not running});
                        exit(E_NOTRUN);
                }
                return  doconnop(strcmp(progname, "btconn") == 0? N_CONNECT: N_DISCONNECT, argv[1]);
        }

        if  (strcmp(progname, "gbch-dst") == 0)  {
                if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                        print_error($E{Scheduler not running});
                        exit(E_NOTRUN);
                }

                if  (argc == 4)
                        return  dodst(0, g_time(argv[1]), g_time(argv[2]), g_adj(argv[3]));
                else  {
                        if  (argc != 5  ||  strcmp(argv[1], "-R") != 0)  {
                                print_error($E{Btdst usage});
                                return  E_USAGE;
                        }
                        return  dodst(1, g_time(argv[2]), g_time(argv[3]), g_adj(argv[4]));
                }
        }

        SWAP_TO(Realuid);
        argv = optprocess(argv, Adefs, optprocs, $A{btstart arg explain}, $A{btstart arg freeze home}, 0);
#include "inline/freezecode.c"
        if  (Anychanges & OF_ANY_FREEZE_WANTED)
                exit(0);
        SWAP_TO(Daemuid);

        /* Open control MSGID to see if scheduler process is there.  */

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                PIDTYPE pid;
                int     code;

                /* Not really an error, but easiest done this way.  */

                print_error($E{Restarting scheduler});

                while  ((pid = fork())  < 0)  {
                        print_error($E{Btstart fork looping});
                        sleep(10);
                }

                if  (pid == 0)  {       /*  Child process  */
                        char    *spshed = envprocess(BTSCHED), **ap, *cp;
                        char    *argv[10];
                        char    jsbuf[20], vsbuf[20], llbuf[20], dpbuf[30];
                        ap = argv;
                        *ap++ = (cp = strrchr(spshed, '/'))? cp+1: spshed;
                        if  (nosetpgrp)
                                *ap++ = "-t";
                        if  (initll >= 0)  {
                                sprintf(llbuf, "%ld", (long) initll);
                                *ap++ = "-l";
                                *ap++ = llbuf;
                        }
                        if  (jsize > 0)  {
                                sprintf(jsbuf, "%ld", (long) jsize);
                                *ap++ = "-j";
                                *ap++ = jsbuf;
                        }
                        if  (vsize > 0)  {
                                sprintf(vsbuf, "%ld", (long) vsize);
                                *ap++ = "-v";
                                *ap++ = vsbuf;
                        }
                        if  (decpri != 0.0)  {
                                sprintf(dpbuf, "%g", decpri);
                                *ap++ = "-d";
                                *ap++ = dpbuf;
                        }
                        *ap = (char *) 0;
                        execv(spshed, argv);
                        exit(255);
                }

                /* Main path of scheduler exits at once with ipc created.  */

#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &code, 0) < 0)
                        ;
#else
                while  (wait(&code) != pid)
                        ;
#endif
                if  ((code & 255) != 0)  {
                        print_error($E{Btstart giving up});
                        exit(code >> 8);
                }

                if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                        print_error($E{Btstart still couldnt open});
                        exit(E_SETUP);
                }
        }
        if  (fork() == 0)  {
                char    *xbn = envprocess(XBNETSERV);
                execl(xbn, xbn, (const char *) 0);
                exit(255);
        }
        return  0;
}
