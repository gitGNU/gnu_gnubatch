/* btjdcgi.c -- job delete CGI program

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
#include <time.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "incl_sig.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "shreq.h"
#include "files.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "cgifndjb.h"
#include "shutilmsg.h"

#define DEFSLEEP        10

#define IPC_MODE        0600

extern  int     Ctrl_chan;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

int  act_delete(CBtjobRef jp, const int notused)
{
        return  wjimsg(J_DELETE, jp);
}

int  act_kill(CBtjobRef jp, const int signum)
{
        return  wjimsg_param(J_KILL, signum, jp);
}

int  job_progfix(CBtjobRef jp, const unsigned progcode)
{
        int     retc;
        ULONG   indx;
        BtjobRef  JREQ = &Xbuffer->Ring[indx = getxbuf()];
        *JREQ = *jp;
        JREQ->h.bj_progress = progcode;
        retc = wjxfermsg(J_CHANGE, indx);
        freexbuf(indx);
        return  retc;
}

int  act_ready(CBtjobRef jp, const int notused)
{
        return  job_progfix(jp, BJP_NONE);
}

int  act_canc(CBtjobRef jp, const int notused)
{
        return  job_progfix(jp, BJP_CANCELLED);
}

int  act_done(CBtjobRef jp, const int notused)
{
        return  job_progfix(jp, BJP_DONE);
}

int  act_adv(CBtjobRef jp, const int notused)
{
        int     retc;
        ULONG   indx;
        BtjobRef  JREQ;

        /* Forget it if no time set */

        if  (!jp->h.bj_times.tc_istime)
                return  0;

        JREQ = &Xbuffer->Ring[indx = getxbuf()];
        *JREQ = *jp;
        JREQ->h.bj_times.tc_nexttime = advtime(&JREQ->h.bj_times);
        retc = wjxfermsg(J_CHANGE, indx);
        freexbuf(indx);
        return  retc;
}

int  act_go(CBtjobRef jp, const int notused)
{
        return  wjimsg(J_FORCENA, jp);
}

int  act_goadv(CBtjobRef jp, const int notused)
{
        return  wjimsg(J_FORCE, jp);
}

struct  actop  {
        char    *name;
        USHORT          ao_flags;       /* Various attribs */
#define NEEDS_XBUF      1
#define PROC_RUN        2
#define PERMS_SET       4
        USHORT          perms;          /* Permissions we need */
        int     (*act_op)(CBtjobRef, const int);
} actlist[] =  {
        {       "delete",       0,              BTM_DELETE,     act_delete  },
        {       "kill",         PROC_RUN,       BTM_KILL,       act_kill  },
        {       "ready",        NEEDS_XBUF,     BTM_WRITE,      act_ready },
        {       "canc",         NEEDS_XBUF,     BTM_WRITE,      act_canc  },
        {       "done",         NEEDS_XBUF,     BTM_WRITE,      act_done },
        {       "adv",          NEEDS_XBUF,     BTM_READ|BTM_WRITE,     act_adv },
        {       "go",           0,              BTM_KILL,       act_go  },
        {       "goadv",        0,              BTM_KILL,       act_goadv },
        {       "perms",        PERMS_SET|NEEDS_XBUF,   BTM_RDMODE|BTM_WRMODE,  0 }
};

void  act_perms(CBtjobRef jp, char *setf, char *unsetf)
{
        int     retc;
        ULONG   indx;
        USHORT  sarr[3], usarr[3];
        BtjobRef  JREQ = &Xbuffer->Ring[indx = getxbuf()];

        *JREQ = *jp;
        decode_permflags(sarr, setf, 0, 1);
        decode_permflags(usarr, unsetf, 1, 1);
        JREQ->h.bj_mode.u_flags |= sarr[0];
        JREQ->h.bj_mode.u_flags &= ~usarr[0];
        JREQ->h.bj_mode.g_flags |= sarr[1];
        JREQ->h.bj_mode.g_flags &= ~usarr[1];
        JREQ->h.bj_mode.o_flags |= sarr[2];
        JREQ->h.bj_mode.o_flags &= ~usarr[2];
        retc = wjxfermsg(J_CHMOD, indx);
        freexbuf(indx);
        if  (retc != 0)  {
                html_disperror(retc);
                exit(E_SETUP);
        }

        if  ((retc = readreply()) != J_OK)  {
                html_disperror(dojerror(retc, jp));
                exit(E_NOPRIV);         /* Most probable */
        }
}

/* This is the main processing routine for delete */

void  process(char **joblist)
{
        char    **ap = joblist, *arg = *ap, *cp;
        struct  actop   *aop;
        int     param = SIGINT;

        if  (!arg)
                return;

        if  ((cp = strchr(arg, '=')))  {
                *cp = '\0';
                param = atoi(cp + 1);
        }

        for  (aop = &actlist[0];  aop < &actlist[sizeof(actlist)/sizeof(struct actop)];  aop++)  {
                if  (ncstrcmp(arg, aop->name) == 0)  {
                        ap++;
                        goto  found;
                }
        }
        aop = &actlist[0];      /* Delete case */

 found:
        if  (aop->ao_flags & NEEDS_XBUF)
                initxbuffer(0);

        for  (;  (arg = *ap);  ap++)  {
                int                     retc;
                struct  jobswanted      jw;
                CBtjobRef               jp;

                if  (decode_jnum(arg, &jw))  {
                        html_out_or_err("sbadargs", 1);
                        exit(E_USAGE);
                }
                if  (!find_job(&jw))  {
                        html_out_cparam_file("jobgone", 1, arg);
                        exit(E_NOJOB);
                }

                jp = jw.jp;
                if  (aop->ao_flags & PROC_RUN)  {
                        if  (jp->h.bj_progress < BJP_STARTUP1)  {
                                html_out_cparam_file("jobnrun", 1, arg);
                                exit(E_NOTRUN);
                        }
                }
                else  {
                        if  (jp->h.bj_progress >= BJP_STARTUP1)  {
                                html_out_cparam_file("jobrun", 1, arg);
                                exit(E_RUNNING);
                        }
                }
                if  (!mpermitted(&jp->h.bj_mode, aop->perms, mypriv->btu_priv))  {
                        html_out_cparam_file("noperm", 1, arg);
                        exit(E_NOPRIV);
                }

                if  (aop->ao_flags & PERMS_SET)  {
                        if  (!ap[1]  ||  !ap[2]  ||  ap[3])  {
                                html_out_or_err("sbadargs", 1);
                                exit(E_USAGE);
                        }
                        act_perms(jp, ap[1], ap[2]);
                        return;
                }
                else  {
                        if  ((retc = (*aop->act_op)(jp, param)) != 0)  {
                                html_disperror(retc);
                                exit(E_SETUP);
                        }

                        if  ((retc = readreply()) != J_OK)  {
                                html_disperror(dojerror(retc, jp));
                                exit(E_NOPRIV);         /* Most probable */
                        }
                }
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    **newargs;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        tzset();
        html_openini();
        newargs = cgi_arginterp(argc, argv, CGI_AI_SUBSID); /* Side effect of cgi_arginterp is to set Realuid */
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);
        prin_uname(Realuid);    /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;      /* lastgid got set by prin_uname */
        mypriv = getbtuser(Realuid);

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                html_disperror($E{Scheduler not running});
                return  E_NOTRUN;
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                html_disperror($E{Cannot open semaphore});
                return  E_SETUP;
        }
#endif

        /* Open the other files.  */

        openjfile(0, 0);
        rjobfile(1);
        process(newargs);
        html_out_or_err("chngok", 1);
        return  0;
}
