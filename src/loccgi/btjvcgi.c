/* btjvcgi.c -- CGI variable operations

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <time.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "btconst.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "netmsg.h"
#include "cmdint.h"
#include "ecodes.h"
#include "errnums.h"
#include "cfile.h"
#include "files.h"
#include "ipcstuff.h"
#include "helpalt.h"
#include "jvuprocs.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "cgifndjb.h"
#include "q_shm.h"

#define IPC_MODE        0600

extern  int     Ctrl_chan;

FILE *net_feed(const int, const netid_t, const jobno_t, const int);

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  perform_view(char *jnum)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        int                     ch;
        FILE                    *ifl;

        if  (!jnum  ||  decode_jnum(jnum, &jw))  {
                html_out_or_err("sbadargs", 1);
                exit(E_USAGE);
        }

        if  (!find_job(&jw))  {
        jgone:
                html_out_cparam_file("jobgone", 1, jnum);
                exit(E_NOJOB);
        }

        jp = jw.jp;
        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                html_out_cparam_file("nopriv", 1, jnum);
                exit(E_NOPRIV);
        }

        if  (jp->h.bj_hostid)
                ifl = net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job, Job_seg.dptr->js_viewport);
        else
                ifl = fopen(mkspid(SPNAM, jp->h.bj_job), "r");

        if  (!ifl)
                goto  jgone;

        html_out_or_err("viewstart", 1);

        fputs("<SCRIPT LANGUAGE=\"JavaScript\">\n", stdout);
        printf("viewheader(\"%s\", \"%s\", %d);\n",
               jnum, title_of(jp), mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv));
        fputs("</SCRIPT>\n<PRE>", stdout);
        while  ((ch = getc(ifl)) != EOF)
                html_pre_putchar(ch);
        fclose(ifl);
        fputs("</PRE>\n", stdout);
        html_out_or_err("viewend", 0);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    **newargs, *spdir;
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
        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                html_disperror($E{Cannot change directory});
                return  E_SETUP;
        }
        free(spdir);
        perform_view(newargs[0]);
        return  0;
}
