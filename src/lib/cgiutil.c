/* cgiutil.c -- util funcs for CGI

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
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "errnums.h"
#include "jvuprocs.h"
#include "xihtmllib.h"
#include "cgiutil.h"

static  char    Filename[] = __FILE__;

int  jarg_queue(char *str)
{
        const   char    *etitle = title_of(JREQ);
        const   char    *colp = strchr(etitle, ':');
        const   char    *ntitle;
        Btjob   newj;
        newj = *JREQ;

        if  (*str)  {           /* Setting new prefix */
                if  (colp)  {   /* Existing */
                        ntitle = malloc((unsigned) (strlen(str) + strlen(colp) + 1));
                        if  (!ntitle)
                                ABORT_HTML_NOMEM;
                        sprintf((char *) ntitle, "%s%s", str, colp);
                }
                else  {
                        ntitle = malloc((unsigned) (strlen(str) + strlen(etitle) + 2));
                        if  (!ntitle)
                                ABORT_HTML_NOMEM;
                        sprintf((char *) ntitle, "%s:%s", str, etitle);
                }
        }
        else    /* Otherwise killing prefix */
                ntitle = colp? colp + 1: etitle;

        if  (!repackjob(&newj, JREQ, (char *) 0, (char *) ntitle, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                disp_arg[3] = JREQ->h.bj_job;
                disp_str = title_of(JREQ);
                return  $E{Too many job strings};
        }
        *JREQ = newj;
        if  (*str)              /* Allocated to ntitle */
                free((char *) ntitle);
        return  0;
}

int  jarg_title(char *str)
{
        const   char    *etitle = title_of(JREQ);
        const   char    *colp = strchr(etitle, ':');
        const   char    *ntitle = str;
        Btjob   newj;
        newj = *JREQ;

        if  (colp)  {
                ntitle = malloc((unsigned) ((colp-etitle) + strlen(str) + 2));
                if  (!ntitle)
                        ABORT_HTML_NOMEM;
                sprintf((char *) ntitle, "%.*s:%s", (int) (colp-etitle), etitle, str);
        }
        if  (!repackjob(&newj, JREQ, (char *) 0, (char *) ntitle, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                disp_arg[3] = JREQ->h.bj_job;
                disp_str = title_of(JREQ);
                return  $E{Too many job strings};
        }
        *JREQ = newj;
        if  (colp)
                free((char *) ntitle);
        return  0;
}

int  jarg_directory(char *str)
{
        Btjob   newj;
        newj = *JREQ;
        if  (!repackjob(&newj, JREQ, str, (char *) 0, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                disp_arg[3] = JREQ->h.bj_job;
                disp_str = title_of(JREQ);
                return  $E{Too many job strings};
        }
        *JREQ = newj;
        return  0;
}

int  jarg_interp(char *str)
{
        int     nn;
        if  ((nn = validate_ci(str)) >= 0)  {
                strcpy(JREQ->h.bj_cmdinterp, str);
                JREQ->h.bj_ll = Ci_list[nn].ci_ll;
                return  0;
        }
        return  $E{Unknown command interp};
}
