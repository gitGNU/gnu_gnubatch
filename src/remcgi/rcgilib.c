/* rcgilib.c -- remote CGI library routines

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
#include "incl_ugid.h"
#include "incl_net.h"
#include "network.h"
#include "statenums.h"
#include "errnums.h"
#include "cfile.h"
#include "ecodes.h"
#include "helpalt.h"
#include "formats.h"
#include "optflags.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "listperms.h"
#include "files.h"
#include "rcgilib.h"

static  char    Filename[] = __FILE__;

int             xbapi_fd = -1;
apiBtuser       userpriv;

int  sort_j(apiBtjob *a, apiBtjob *b)
{
        apiBtjobh       *aj = &a->h, *bj = &b->h;
        Timecon         *at = &aj->bj_times,
                        *bt = &bj->bj_times;

        /* No we can't do subtraction (ever heard of overflow ?)  */

        if  (!at->tc_istime)  {
                if  (!bt->tc_istime)
                        return  aj->bj_job < bj->bj_job? -1: aj->bj_job == bj->bj_job? 0: 1;
                return  -1;
        }
        else  if  (!bt->tc_istime)
                return  1;

        return  at->tc_nexttime < bt->tc_nexttime? -1:
                at->tc_nexttime > bt->tc_nexttime? 1:
                aj->bj_job < bj->bj_job? -1:
                aj->bj_job == bj->bj_job? 0: 1;
}

int  sort_v(struct var_with_slot *a, struct var_with_slot *b)
{
        int     s;
        return  (s = strcmp(a->var.var_name, b->var.var_name)) != 0? s:
                a->var.var_id.hostid < b->var.var_id.hostid ? -1:
                a->var.var_id.hostid == b->var.var_id.hostid ? 0: 1;
}

int  numeric(const char *x)
{
        while  (*x)  {
                if  (!isdigit(*x))
                        return  0;
                x++;
        }
        return  1;
}

int  decode_jnum(char *jnum, struct jobswanted *jwp)
{
        char    *cp;

        if  ((cp = strchr(jnum, ':')))  {
                *cp = '\0';
                if  ((jwp->host = look_hostname(jnum)) == 0L)  {
                        *cp = ':';
                        disp_str = jnum;
                        return  $E{Unknown host name};
                }
                *cp++ = ':';
        }
        else  {
                jwp->host = dest_hostid;
                cp = jnum;
        }
        if  (!numeric(cp)  ||  (jwp->jno = (jobno_t) atol(cp)) == 0)  {
                disp_str = jnum;
                return  $E{Not numeric argument};
        }
        return  0;
}

apiBtvar *find_var(const slotno_t sl)
{
        struct  var_with_slot   *vs;

        for  (vs = var_sl_list;  vs < &var_sl_list[Nvars];  vs++)
                if  (vs->slot == sl)
                        return  &vs->var;
        return  (apiBtvar *) 0;
}

int  find_var_by_name(const char **namep, struct var_with_slot *rvar)
{
        const   char    *name = *namep;
        const   char    *colp = strchr(name, ':');
        int     vncnt = 0, ret;
        netid_t nid = dest_hostid;
        char    vn[BTV_NAME+1];

        if  (colp)  {
                char    hostn[HOSTNSIZE+1];
                if  (colp - name > HOSTNSIZE)
                        return  0;
                strncpy(hostn, name, colp - name);
                hostn[colp - name] = '\0';
                if  ((nid = my_look_hostname(hostn)) == 0)
                        return  0;
                name = colp + 1;
        }

        while  (isalnum(*name)  ||  *name == '_')
                if  (vncnt < BTV_NAME)
                        vn[vncnt++] = *name++;
        vn[vncnt] = '\0';

        if  ((ret = gbatch_varfind(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vn, nid, &rvar->slot, &rvar->var)) < 0)  {
                if  (ret != GBATCH_UNKNOWN_VAR)  {
                        html_disperror($E{Base for API errors} + ret);
                        exit(E_NETERR);
                }
                return  0;
        }
        return  1;
}

void  api_open(char *realuname)
{
        int     ret;
        char    gname[UIDSIZE+1];

        disp_str = dest_hostname;
        xbapi_fd = gbatch_open(dest_hostname, (char *) 0);
        if  (xbapi_fd < 0)  {
                html_disperror($E{Base for API errors} + xbapi_fd);
                exit(E_NETERR);
        }

        if  ((ret = gbatch_getbtu(xbapi_fd, realuname, gname, &userpriv)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_NETERR);
        }
}

void  api_readvars(const unsigned afl)
{
        slotno_t        *slots;
        int             ret;

        disp_str = dest_hostname;

        if  ((ret = gbatch_varlist(xbapi_fd, afl, &Nvars, &slots)) < 0)  {
                html_disperror($E{Base for API errors} + ret);
                exit(E_NETERR);
        }

        if  (Nvars > 0)  {
                int     cnt;
                if  (!(var_sl_list = (struct var_with_slot *) malloc((unsigned) (sizeof(struct var_with_slot) * Nvars))))
                        ABORT_HTML_NOMEM;
                for  (cnt = 0;  cnt < Nvars;  cnt++)  {
                        var_sl_list[cnt].slot = slots[cnt];
                        if  ((ret = gbatch_varread(xbapi_fd, GBATCH_FLAG_IGNORESEQ, slots[cnt], &var_sl_list[cnt].var)) < 0)  {
                                html_disperror($E{Base for API errors} + ret);
                                exit(E_NETERR);
                        }
                }
                qsort(QSORTP1 var_sl_list, Nvars, sizeof(struct var_with_slot), QSORTP4 sort_v);
        }
}

int  rjarg_queue(apiBtjob *jp, char *str)
{
        const   char    *etitle = gbatch_gettitle(-1, jp);
        const   char    *colp = strchr(etitle, ':');
        const   char    *ntitle;
        apiBtjob        newj;

        newj = *jp;             /* Nasty C compilers don't like aggregate inits */

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

        if  (gbatch_puttitle(-1, &newj, ntitle) == 0)  {
                if  (str)
                        free((char *) ntitle);
                disp_arg[3] = jp->h.bj_job;
                disp_str = gbatch_gettitle(-1, jp);
                return  $E{Too many job strings};
        }

        *jp = newj;

        if  (*str)              /* Allocated to ntitle */
                free((char *) ntitle);
        return  0;
}

int  rjarg_title(apiBtjob *jp, char *str)
{
        const   char    *etitle = gbatch_gettitle(-1, jp);
        const   char    *colp = strchr(etitle, ':');
        const   char    *ntitle = str;
        apiBtjob        newj;
        newj = *jp;

        if  (colp)  {
                ntitle = malloc((unsigned) ((colp-etitle) + strlen(str) + 2));
                if  (!ntitle)
                        ABORT_HTML_NOMEM;
                sprintf((char *) ntitle, "%.*s:%s", (int) (colp-etitle), etitle, str);
        }
        if  (gbatch_puttitle(-1, &newj, ntitle) == 0)  {
                disp_arg[3] = jp->h.bj_job;
                disp_str = gbatch_gettitle(-1, jp);
                return  $E{Too many job strings};
        }
        *jp = newj;
        if  (colp)
                free((char *) ntitle);
        return  0;
}

/* Decode permission flags in CGI routines */

void  decode_permflags(USHORT *arr, char *str, const int off, const int isjob)
{
        USHORT  *ap;

        arr[0] = arr[1] = arr[2] = 0;

        while  (*str)  {
                switch  (*str++)  {
                default:        return;
                case  'U':case  'u':    ap = &arr[0];  break;
                case  'G':case  'g':    ap = &arr[1];  break;
                case  'O':case  'o':    ap = &arr[2];  break;
                }
                switch  (*str++)  {
                default:        return;
                case  'R':case  'r':
                        if  (off)
                                *ap |= BTM_READ|BTM_WRITE;
                        else
                                *ap |= BTM_SHOW|BTM_READ;
                        break;

                case  'W':case  'w':
                        if  (off)
                                *ap |= BTM_WRITE;
                        else
                                *ap |= BTM_SHOW|BTM_READ|BTM_WRITE;
                        break;

                case  'S':case  's':
                        if  (off)
                                *ap |= BTM_SHOW|BTM_READ|BTM_WRITE;
                        else
                                *ap |= BTM_SHOW;
                        break;

                case  'M':case  'm':
                        if  (off)
                                *ap |= BTM_RDMODE|BTM_WRMODE;
                        else
                                *ap |= BTM_RDMODE;
                        break;

                case  'P':case  'p':
                        if  (off)
                                *ap |= BTM_WRMODE;
                        else
                                *ap |= BTM_RDMODE|BTM_WRMODE;
                        break;

                case  'U':case  'u':    *ap |= BTM_UGIVE;       break;
                case  'V':case  'v':    *ap |= BTM_UTAKE;       break;
                case  'G':case  'g':    *ap |= BTM_GGIVE;       break;
                case  'H':case  'h':    *ap |= BTM_GTAKE;       break;
                case  'D':case  'd':    *ap |= BTM_DELETE;      break;

                case  'K':case  'k':
                        if  (isjob)  {
                                *ap |= BTM_KILL;
                                break;
                        }
                        return;
                }
        }
}
