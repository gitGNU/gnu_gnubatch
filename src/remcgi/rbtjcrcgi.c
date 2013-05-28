/* rbtjcrcgi.c -- remote CGI create job

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
#include <sys/stat.h>
#include <errno.h>
#include "gbatch.h"
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

#define C_MASK          0377    /* Umask value */

int             Nvars, Ci_num;
Cmdint          *ci_list;
struct  var_with_slot  *var_sl_list;

apiBtjob        JREQ;
char            *Job_data,
                *Job_filename,
                *Buff_filename;

char            setting_title;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  post_interp(char *arg)
{
        int     cnt;

        for  (cnt = 0;  cnt < Ci_num;  cnt++)  {
                if  (strcmp(ci_list[cnt].ci_name, arg) == 0)  {
                        strcpy(JREQ.h.bj_cmdinterp, arg);
                        JREQ.h.bj_ll = ci_list[cnt].ci_ll;
                        return;
                }
        }
        disp_str = arg;
        html_disperror($E{Unknown command interp});
        exit(E_USAGE);
}

void  post_queue(char *arg)
{
        int     ret = rjarg_queue(&JREQ, arg);
        if  (ret != 0)  {
                html_disperror(ret);
                exit(E_USAGE);
        }
}

void  post_title(char *arg)
{
        int     ret = rjarg_title(&JREQ, arg);
        if  (ret != 0)  {
                html_disperror(ret);
                exit(E_USAGE);
        }
        setting_title = 1;
}

void  post_directory(char *arg)
{
        int  ret = gbatch_putdirect(&JREQ, arg);
        if  (ret == 0)  {
                disp_arg[3] = JREQ.h.bj_job;
                disp_str = gbatch_gettitle(-1, &JREQ);
                html_disperror($E{Too many job strings});
                exit(E_USAGE);
        }
}

void  post_state(char *arg)
{
        switch  (tolower(arg[0]))  {
        case  'r':
                JREQ.h.bj_progress = BJP_NONE;
                return;
        default:
        case  'c':
                JREQ.h.bj_progress = BJP_CANCELLED;
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
        FILE            *ofl;
        int             cnt;

        if  ((!Job_data  ||  !Job_data[0])  &&  (!Job_filename || !Job_filename[0]))  {
                html_disperror($E{btjccrcgi no data});
                exit(E_USAGE);
        }

        if  (Job_filename  &&  Job_filename[0]  &&  !setting_title)  {
                int     retc = rjarg_title(&JREQ, Job_filename);
                if  (retc != 0)  {
                        html_disperror(retc);
                        unlink(Buff_filename);
                        exit(E_LIMIT);
                }
        }

        if  (!(ofl = gbatch_jobadd(xbapi_fd, &JREQ)))  {
                html_disperror($E{No room for job});
                exit(E_DFULL);
        }

        if  (Job_filename  &&  Job_filename[0])  {
                FILE    *tinp = fopen(Buff_filename, "r");
                char    inbuf[1024];

                if  (!tinp)  {
                        html_disperror($E{btjcrcrgi cant reopen temp file});
                        exit(E_SETUP);
                }
                while  ((cnt = fread(inbuf, sizeof(char), sizeof(inbuf), tinp)) > 0)  {
                        if  (fwrite(inbuf, sizeof(char), cnt, ofl) != cnt)  {
                                html_disperror($E{No room for job});
                                fclose(tinp);
                                unlink(Buff_filename);
                                exit(E_DFULL);
                        }
                }
                fclose(tinp);
                unlink(Buff_filename);
                if  (fclose(ofl) == EOF)  {
                        html_disperror($E{No room for job});
                        exit(E_DFULL);
                }
        }
        else  {
                cnt = strlen(Job_data);
                if  (fwrite(Job_data, sizeof(char), cnt, ofl) != cnt  ||  fclose(ofl) == EOF)  {
                        html_disperror($E{No room for job});
                        exit(E_DFULL);
                }
        }
        if  ((cnt = gbatch_jobres(xbapi_fd, &JREQ.h.bj_job)) < 0)  {
                html_disperror($E{Base for API errors} + cnt);
                exit(E_DFULL);
        }
        return  JREQ.h.bj_job;
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     ret, cnt;
        char    *realuname;
        unsigned        defavoid = 0;
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
        cgi_arginterp(argc, argv, CGI_AI_REMHOST|CGI_AI_SUBSID); /* Side effect of cgi_arginterp is to set Realuid */
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        realuname = prin_uname(Realuid);        /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;
        setgid(Realgid);
        setuid(Realuid);
        api_open(realuname);
        if  ((userpriv.btu_priv & BTM_CREATE) == 0)  {
                html_disperror($E{No create perm});
                return  E_NOPRIV;
        }
        if  ((ret = gbatch_ciread(xbapi_fd, 0, &Ci_num, &ci_list)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                return  E_SETUP;
        }

        /* Get standard names from file */

        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                if  (helpnstate((int) ($N{Base for days to avoid}+cnt)) > 0)
                        defavoid |= 1 << cnt;

        /* These are default values.  */

        strcpy(JREQ.h.bj_cmdinterp, ci_list[CI_STDSHELL].ci_name);
        JREQ.h.bj_ll = ci_list[CI_STDSHELL].ci_ll;
        JREQ.h.bj_umask = umask(C_MASK);
        JREQ.h.bj_ulimit = 0L;
        time(&JREQ.h.bj_time);
        JREQ.h.bj_pri = userpriv.btu_defp;
        JREQ.h.bj_mode.u_flags = userpriv.btu_jflags[0];
        JREQ.h.bj_mode.g_flags = userpriv.btu_jflags[1];
        JREQ.h.bj_mode.o_flags = userpriv.btu_jflags[2];
        /* bj_runtime = bj_autoksig = bj_runon = bj_deltime = 0 */
        /* JREQ.h.bj_times.tc_istime = 0;       No time spec */
        JREQ.h.bj_times.tc_repeat = TC_DELETE;
        JREQ.h.bj_times.tc_nposs = TC_WAIT1;
        JREQ.h.bj_times.tc_nvaldays = (USHORT) defavoid;
        JREQ.h.bj_exits.elower = 1;
        JREQ.h.bj_exits.eupper = 255;
        /* JREQ.h.bj_jflags = 0;                The default case */

        html_postvalues(postlist);
        html_out_param_file("submitok", 1, perform_submit(), 0L);
        return  0;
}
