/* rbtjdcgi.c -- remote CGI delete job

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

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "gbatch.h"
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "network.h"
#include "incl_ugid.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "files.h"
#include "helpalt.h"
#include "cfile.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "cgiutil.h"
#include "rcgilib.h"

int             Nvars;
struct  var_with_slot  *var_sl_list;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

int  act_delete(struct jobswanted *jw, const unsigned notused1, const int notused2)
{
        return  gbatch_jobdel(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw->slot);
}

int  act_jop(struct jobswanted *jw, const unsigned op, const int signum)
{
        return  gbatch_jobop(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw->slot, op, signum);
}

struct  actop  {
        char    *name;
        USHORT          ao_flags;       /* Various attribs */
#define NEEDS_XBUF      1
#define PROC_RUN        2
#define PERMS_SET       4
        USHORT          op;
        int     (*act_op)(struct jobswanted *, const unsigned, const int);
} actlist[] =  {
        {       "delete",       0,              0,                      act_delete  },
        {       "kill",         PROC_RUN,       GBATCH_JOP_KILL,        act_jop  },
        {       "ready",        NEEDS_XBUF,     GBATCH_JOP_SETRUN,      act_jop },
        {       "canc",         NEEDS_XBUF,     GBATCH_JOP_SETCANC,     act_jop  },
        {       "done",         NEEDS_XBUF,     GBATCH_JOP_SETDONE,     act_jop },
        {       "adv",          NEEDS_XBUF,     GBATCH_JOP_ADVTIME,     act_jop },
        {       "go",           0,              GBATCH_JOP_FORCE,       act_jop },
        {       "goadv",        0,              GBATCH_JOP_FORCEADV,    act_jop },
        {       "perms",        PERMS_SET|NEEDS_XBUF,   BTM_RDMODE|BTM_WRMODE,  0 }
};

void  act_perms(struct jobswanted *jw, apiBtjob *jp, char *setf, char *unsetf)
{
        int     ret;
        USHORT  sarr[3], usarr[3];
        Btmode  newmode;

        newmode = jp->h.bj_mode;
        decode_permflags(sarr, setf, 0, 1);
        decode_permflags(usarr, unsetf, 1, 1);
        newmode.u_flags |= sarr[0];
        newmode.u_flags &= ~usarr[0];
        newmode.g_flags |= sarr[1];
        newmode.g_flags &= ~usarr[1];
        newmode.o_flags |= sarr[2];
        newmode.o_flags &= ~usarr[2];
        if  ((ret = gbatch_jobchmod(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw->slot, &newmode)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_PERM);
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

        for  (;  (arg = *ap);  ap++)  {
                int                     retc;
                struct  jobswanted      jw;
                apiBtjob                jb;

                if  (decode_jnum(arg, &jw))  {
                        html_out_or_err("sbadargs", 1);
                        exit(E_USAGE);
                }
                if  ((retc = gbatch_jobfind(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &jb)) < 0)  {
                        if  (retc == GBATCH_UNKNOWN_JOB)
                                html_out_cparam_file("jobgone", 1, arg);
                        else
                                html_disperror($E{Base for API errors} + retc);
                        exit(E_NOJOB);
                }

                if  (aop->ao_flags & PROC_RUN)  {
                        if  (jb.h.bj_progress < BJP_STARTUP1)  {
                                html_out_cparam_file("jobnrun", 1, arg);
                                exit(E_NOTRUN);
                        }
                }
                else  {
                        if  (jb.h.bj_progress >= BJP_STARTUP1)  {
                                html_out_cparam_file("jobrun", 1, arg);
                                exit(E_RUNNING);
                        }
                }
                if  (aop->ao_flags & PERMS_SET)  {
                        if  (!ap[1]  ||  !ap[2]  ||  ap[3])  {
                                html_out_or_err("sbadargs", 1);
                                exit(E_USAGE);
                        }
                        act_perms(&jw, &jb, ap[1], ap[2]);
                        return;
                }
                else  {
                        if  ((retc = (*aop->act_op)(&jw, aop->op, param)) != 0)  {
                                html_disperror($E{Base for API errors} + retc);
                                exit(E_SETUP);
                        }
                }
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *realuname;
        char    **newargs;
        int_ugid_t      chku;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        tzset();
        html_openini();
        hash_hostfile();
        Effuid = geteuid();
        Effgid = getegid();
        if  ((chku = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)
                Daemuid = ROOTID;
        else
                Daemuid = chku;
        newargs = cgi_arginterp(argc, argv, CGI_AI_REMHOST|CGI_AI_SUBSID); /* Side effect of cgi_arginterp is to set Realuid */
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        realuname = prin_uname(Realuid);        /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;
        setgid(Realgid);
        setuid(Realuid);
        api_open(realuname);
        process(newargs);
        html_out_or_err("chngok", 1);
        return  0;
}
