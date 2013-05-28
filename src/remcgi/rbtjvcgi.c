/* rbtjvcgi.c -- remote CGI variable operations

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

int     Nvars;
struct  var_with_slot  *var_sl_list;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  perform_view(char *jnum)
{
        struct  jobswanted      jw;
        int                     ch;
        FILE                    *ifl;
        apiBtjob                jb;

        if  (!jnum  ||  decode_jnum(jnum, &jw))  {
                html_out_or_err("sbadargs", 1);
                exit(E_USAGE);
        }

        if  (gbatch_jobfind(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &jb) < 0  ||
             !(ifl = gbatch_jobdata(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw.slot)))  {
                html_out_cparam_file("jobgone", 1, jnum);
                exit(E_NOJOB);
        }

        html_out_or_err("viewstart", 1);

        fputs("<SCRIPT LANGUAGE=\"JavaScript\">\n", stdout);
        printf("viewheader(\"%s\", \"%s\", %d);\n", jnum, gbatch_gettitle(-1, &jb), 1);
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
        char    *realuname, **newargs;
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
        perform_view(newargs[0]);
        return  0;
}
