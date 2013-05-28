/* btjcrcgi.c -- job create CGI program

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
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include "errnums.h"
#include "statenums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "shreq.h"
#include "files.h"
#include "helpalt.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "btrvar.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "cgifndjb.h"
#include "shutilmsg.h"
#include "cgiutil.h"
#include "optflags.h"

static  char    Filename[] = __FILE__;

#define IPC_MODE        0600
#define C_MASK          0377    /* Umask value */

extern  int     Ctrl_chan;

ULONG   Saveseq;
ULONG   indx;

unsigned        defavoid;

char            *Job_data,
                *Job_filename,
                *Buff_filename;

char    setting_monthse,
        setting_mday,
        setting_ll,
        setting_title;

USHORT  ll_set;

extern int  extractvar(const char **, struct vdescr *);

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  post_interp(char *arg)
{
        int     retc = jarg_interp(arg);
        if  (retc != 0)  {
                html_disperror(retc);
                exit(E_USAGE);
        }
}

void  post_queue(char *arg)
{
        int     retc = jarg_queue(arg);
        if  (retc != 0)  {
                html_disperror(retc);
                exit(E_LIMIT);
        }
}

void  post_title(char *arg)
{
        int     retc = jarg_title(arg);
        if  (retc != 0)  {
                html_disperror(retc);
                exit(E_LIMIT);
        }
        setting_title = 1;
}

void  post_directory(char *arg)
{
        int     retc = jarg_directory(arg);
        if  (retc != 0)  {
                html_disperror(retc);
                exit(E_LIMIT);
        }
}

void  post_state(char *arg)
{
        switch  (tolower(arg[0]))  {
        case  'r':
                JREQ->h.bj_progress = BJP_NONE;
                return;
        default:
        case  'c':
                JREQ->h.bj_progress = BJP_CANCELLED;
                return;
        }
}

void  post_data(char *arg)
{
        Job_data = stracpy(arg);
}

void  post_file(char *arg)
{
        Job_filename = stracpy(arg);
}

struct  posttab  postlist[] =  {
        {       "jobdata",      post_data       },
        {       "jobfile",      post_file,      &Buff_filename  },
        {       "state",        post_state      },
        {       "title",        post_title      },
        {       "queue",        post_queue      },
        {       "interp",       post_interp     },
        {       "directory",    post_directory  },
        {       (char *) 0  }
};

jobno_t  perform_submit()
{
        TimeconRef      tc = &JREQ->h.bj_times;
        char            *tmpnam;
        FILE            *tmpfl;
        int             fid, cnt;
        unsigned        retc;
        jobno_t         jn = getpid();

        if  (setting_ll)  {
                if  (!(mypriv->btu_priv & BTM_SPCREATE)  &&  JREQ->h.bj_ll != ll_set)  {
                        html_disperror($E{No special create});
                        exit(E_NOPRIV);
                }
                JREQ->h.bj_ll = ll_set;
        }

        if  (tc->tc_istime  &&  tc->tc_repeat == TC_MONTHSE  &&  (setting_monthse || setting_mday))  {
                struct  tm      *t = localtime(&tc->tc_nexttime);
                month_days[1] = t->tm_year % 4 == 0? 29: 28;
                if  (setting_mday)
                        tc->tc_mday = month_days[t->tm_mon] - tc->tc_mday;
                else
                        tc->tc_mday = month_days[t->tm_mon] - t->tm_mday;
        }

        if  ((!Job_data  ||  !Job_data[0])  &&  (!Job_filename || !Job_filename[0]))  {
                html_disperror($E{btjccrcgi no data});
                exit(E_USAGE);
        }

        for  (;;)  {
                tmpnam = mkspid(SPNAM, jn);
                if  ((fid = open(tmpnam, O_WRONLY|O_CREAT|O_EXCL, 0666)) >= 0)
                        break;
                jn += JN_INC;
        }

        if  ((tmpfl = fdopen(fid, "w")) == (FILE *) 0)  {
                unlink(tmpnam);
                ABORT_HTML_NOMEM;
        }

        if  (Job_filename  &&  Job_filename[0])  {
                FILE    *tinp = fopen(Buff_filename, "r");
                char    inbuf[1024];

                /* Copy title from file name if we had no title given */

                if  (!setting_title)  {
                        int     retc = jarg_title(Job_filename);
                        if  (retc != 0)  {
                                html_disperror(retc);
                                fclose(tmpfl);
                                unlink(tmpnam);
                                unlink(Buff_filename);
                                exit(E_LIMIT);
                        }
                }

                if  (!tinp)  {
                        fclose(tmpfl);
                        unlink(tmpnam);
                        html_disperror($E{btjcrcrgi cant reopen temp file});
                        exit(E_SETUP);
                }
                while  ((cnt = fread(inbuf, sizeof(char), sizeof(inbuf), tinp)) > 0)  {
                        if  (fwrite(inbuf, sizeof(char), cnt, tmpfl) != cnt)  {
                                html_disperror($E{No room for job});
                                fclose(tmpfl);
                                fclose(tinp);
                                unlink(Buff_filename);
                                unlink(tmpnam);
                                exit(E_DFULL);
                        }
                }
                fclose(tinp);
                unlink(Buff_filename);
                if  (fclose(tmpfl) == EOF)  {
                        html_disperror($E{No room for job});
                        unlink(tmpnam);
                        exit(E_DFULL);
                }
        }
        else  {
                cnt = strlen(Job_data);
                if  (fwrite(Job_data, sizeof(char), cnt, tmpfl) != cnt  ||  fclose(tmpfl) == EOF)  {
                        html_disperror($E{No room for job});
                        unlink(tmpnam);
                        exit(E_DFULL);
                }
        }

        JREQ->h.bj_job = jn;
        if  ((cnt = wjxfermsg(J_CREATE, indx)) != 0)  {
                html_disperror(cnt);
                exit(E_SETUP);
        }
        if  ((retc = readreply()) != J_OK)  {
                unlink(tmpnam);
                html_disperror(dojerror(retc, JREQ));
                exit(E_NOPRIV);
        }

        return  jn;
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     ret, cnt;
        char    *spdir;
        jobno_t jobnum;
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
        cgi_arginterp(argc, argv, CGI_AI_SUBSID); /* Side effect of cgi_arginterp is to set Realuid */
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);

        prin_uname(Realuid);    /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;      /* lastgid got set by prin_uname */
        mypriv = getbtuser(Realuid);

        if  ((mypriv->btu_priv & BTM_CREATE) == 0)  {
                html_disperror($E{No create perm});
                return  E_NOPRIV;
        }

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                html_disperror($E{Scheduler not running});
                return  E_NOTRUN;
        }

#ifndef USING_FLOCK
        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                html_disperror($E{Cannot open semaphore});
                return  E_SETUP;
        }
#endif

        openvfile(0, 0);
        rvarfile(1);
        initxbuffer(0);

        JREQ = &Xbuffer->Ring[indx = getxbuf()];
        BLOCK_ZERO(JREQ, sizeof(Btjob));
        JREQ->h.bj_slotno = -1;
        JREQ->h.bj_umask = umask(C_MASK);
        JREQ->h.bj_ulimit = 0L;
        time(&JREQ->h.bj_time);
        JREQ->h.bj_pri = mypriv->btu_defp;
        JREQ->h.bj_mode.u_flags = mypriv->btu_jflags[0];
        JREQ->h.bj_mode.g_flags = mypriv->btu_jflags[1];
        JREQ->h.bj_mode.o_flags = mypriv->btu_jflags[2];

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                html_disperror(ret);
                return  E_SETUP;
        }

        /* Get standard names from file */

        SWAP_TO(Realuid);
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                if  (helpnstate((int) ($N{Base for days to avoid}+cnt)) > 0)
                        defavoid |= 1 << cnt;

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

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                html_disperror($E{Cannot change directory});
                return  E_SETUP;
        }
        html_postvalues(postlist);
        jobnum = perform_submit();
        html_out_param_file("submitok", 1, jobnum, 0L);
        return  0;
}
