/* rbtjccgi.c -- remote CGI job operations

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
#include "optflags.h"

#ifndef _NFILE
#define _NFILE  64
#endif

apiBtjob        JREQ;
int             Nvars;
struct  var_with_slot  *var_sl_list;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

struct  argop  {
        const   char    *name;          /* Name of parameter case insens */
        int     (*arg_fn)(const struct argop *);
        unsigned  char  typ;                    /* Type of parameter */
#define AO_BOOL         0
#define AO_UCHAR        1
#define AO_USHORT       2
#define AO_ULONG        3
#define AO_TIME         4
#define AO_STRING       5
        unsigned  char  pass;           /* Pass number - 1 */
        unsigned  char  off;            /* Turn off */
        unsigned  char  had;
        union  {
                USHORT          ao_boolbit;
                unsigned  char  ao_uchar;
                USHORT          ao_ushort;
                ULONG           ao_ulong;
                char            *ao_string;
                time_t          ao_time;
        }  ao_un;
        struct  argop   *next;
};

int  arg_jflag(const struct argop *ao)
{
        USHORT  bit = ao->ao_un.ao_boolbit;
        if  (ao->off)
                JREQ.h.bj_jflags &= ~bit;
        else
                JREQ.h.bj_jflags |= bit;
        return  0;
}

int  arg_export(const struct argop *ao)
{
        JREQ.h.bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);
        switch  (tolower(ao->ao_un.ao_string[0]))  {
        case  'e':
                JREQ.h.bj_jflags |= BJ_EXPORT;
                break;
        case  'r':
                JREQ.h.bj_jflags |= BJ_EXPORT|BJ_REMRUNNABLE;
                break;
        }
        return  0;
}

int  arg_time(const struct argop *ao)
{
        JREQ.h.bj_times.tc_nexttime = ao->ao_un.ao_time;
        JREQ.h.bj_times.tc_istime = ao->ao_un.ao_time != 0;
        return  0;
}

int  arg_repeat(const struct argop *ao) /* Pass 1 */
{
        unsigned  resrep = ao->ao_un.ao_uchar;
        if  (resrep > TC_YEARS)
                return  $E{Bad repeat};

        JREQ.h.bj_times.tc_repeat = resrep;

        /* Force the issue to be sure they're sensible */

        if  (resrep == TC_MONTHSB)
                JREQ.h.bj_times.tc_mday = 1;
        else  if  (resrep == TC_MONTHSE)
                JREQ.h.bj_times.tc_mday = 0;

        return  0;
}

int  arg_repeatint(const struct argop *ao)
{
        JREQ.h.bj_times.tc_rate = ao->ao_un.ao_ulong;
        return  0;
}

int  arg_mday(const struct argop *ao) /* Pass 2 */
{
        TimeconRef  tc = &JREQ.h.bj_times;
        tc->tc_mday = ao->ao_un.ao_uchar;
#ifdef  GIVE_DAY_MONTHSE
        if  (tc->tc_istime  &&  tc->tc_repeat == TC_MONTHSE)  {
                struct  tm      *t = localtime(&tc->tc_nexttime);
                month_days[1] = t->tm_year % 4 == 0? 29: 28;
                tc->tc_mday = month_days[t->tm_mon] - tc->tc_mday;
        }
#else
        if  (tc->tc_istime)  {
                if  (tc->tc_repeat == TC_MONTHSE)
                        tc->tc_mday--;
                else  if  (tc->tc_repeat == TC_MONTHSB  &&  tc->tc_mday == 0)
                        tc->tc_mday = 1;
        }
#endif
        return  0;
}

int  arg_avdays(const struct argop *ao)
{
        JREQ.h.bj_times.tc_nvaldays = ao->ao_un.ao_ushort;
        return  0;
}

int  arg_ifnposs(const struct argop *ao)
{
        unsigned        resrep = ao->ao_un.ao_uchar;
        if  (resrep > TC_CATCHUP)
                resrep = TC_WAIT1;
        JREQ.h.bj_times.tc_nposs = resrep;
        return  0;
}

int  arg_interp(const struct argop *ao) /* Pass 1 */
{
        return  jarg_interp(ao->ao_un.ao_string);
}

int  arg_ll(const struct argop *ao)     /* Pass 2 */
{
        unsigned  num = ao->ao_un.ao_ushort;

        if  (num > 32767)
                return  $E{Load level out of range};

        if  (!(userpriv.btu_priv & BTM_SPCREATE)  &&  JREQ.h.bj_ll != num)
                return  $E{No special create};

        JREQ.h.bj_ll = num;
        return  0;
}

int  arg_pri(const struct argop *ao)
{
        unsigned  num = ao->ao_un.ao_uchar;
        if  (num < (unsigned) userpriv.btu_minp || num > (unsigned) userpriv.btu_maxp)  {
                disp_arg[0] = num;
                disp_arg[1] = userpriv.btu_minp;
                disp_arg[2] = userpriv.btu_maxp;
                return  $E{Invalid priority};
        }
        JREQ.h.bj_pri = num;
        return  0;
}

int  arg_deltime(const struct argop *ao)
{
        JREQ.h.bj_deltime = ao->ao_un.ao_ushort;
        return  0;
}

int  arg_runtime(const struct argop *ao)
{
        JREQ.h.bj_runtime = ao->ao_un.ao_ulong;
        return  0;
}

int  arg_gracetime(const struct argop *ao)
{
        JREQ.h.bj_runon = ao->ao_un.ao_ushort;
        return  0;
}

int  arg_ksig(const struct argop *ao)
{
        unsigned  num = ao->ao_un.ao_ushort;
        if  (num == 0  ||  num >= NSIG)
                return  $E{Bad signal number};
        JREQ.h.bj_autoksig = (USHORT) num;
        return  0;
}

int  arg_umask(const struct argop *ao)
{
        unsigned  num = ao->ao_un.ao_ushort;
        if  (num > 0777)
                return  $E{Bad umask};
        JREQ.h.bj_umask = (USHORT) num;
        return  0;
}

int  arg_ulimit(const struct argop *ao)
{
        JREQ.h.bj_ulimit = ao->ao_un.ao_ulong;
        return  0;
}

int  arg_queue(const struct argop *ao)
{
        return  rjarg_queue(&JREQ, ao->ao_un.ao_string);
}

int  arg_title(const struct argop *ao)
{
        return  rjarg_title(&JREQ, ao->ao_un.ao_string);
}

int  arg_directory(const struct argop *ao)
{
        apiBtjob        newj;
        newj = JREQ;
        if  (gbatch_putdirect(&newj, ao->ao_un.ao_string) == 0)  {
                disp_arg[3] = JREQ.h.bj_job;
                disp_str = gbatch_gettitle(-1, &JREQ);
                return  $E{Too many job strings};
        }
        JREQ = newj;
        return  0;
}

/* Now use fd,action,arg format */

int  add_redir(char *str, apiMredir *mr)
{
        long    whichfd = 0, action = RD_ACT_RD;
        int     dupfd = 0;
        char    *np;

        /* Grab file descriptor */

        whichfd = strtol(str, &np, 0);
        if  (whichfd < 0  ||  whichfd >= _NFILE)
                return  $E{File descriptor out of range};
        if  (*np != ',')
                return  $E{Bad redirection};
        str = ++np;
        action = strtol(str, &np, 0);
        if  (*np != ',')
                return  $E{Bad redirection};
        str = ++np;

        mr->fd = (unsigned char) whichfd;
        mr->action = (unsigned char) action;

        switch  ((int) action)  {
        default:
                return  $E{Bad redirection};
        case  RD_ACT_RD:
        case  RD_ACT_WRT:
        case  RD_ACT_APPEND:
        case  RD_ACT_RDWR:
        case  RD_ACT_RDWRAPP:
        case  RD_ACT_PIPEO:
        case  RD_ACT_PIPEI:
                while  (isspace(*str))
                        str++;
                if  (!*str)
                        return  $E{Bad redirection};
                mr->un.buffer = stracpy(str);
                break;
        case  RD_ACT_CLOSE:
                mr->un.arg = 0;
                break;
        case  RD_ACT_DUP:
                dupfd = atoi(str);
                if  (dupfd < 0  ||  dupfd >= _NFILE)
                        return  $E{File descriptor out of range};
                mr->un.arg = (USHORT) dupfd;
                break;
        }
        return  0;
}

int  arg_redirs(const struct argop *ao)
{
        apiBtjob        newj;
        char            *str = ao->ao_un.ao_string;
        unsigned        nr = 0;
        int             retc;
        apiMredir       Nredirs[MAXJREDIRS];

        newj = JREQ;

        if  (*str)  {
                char  *lp;
                do  {
                        if  ((lp = strchr(str, '\n')))
                                *lp = '\0';
                        if  ((retc = add_redir(str, &Nredirs[nr])) != 0)
                                return  retc;
                        if  (++nr >= MAXJREDIRS)  {
                                disp_arg[0] = nr;
                                disp_arg[1] = JREQ.h.bj_nredirs;
                                disp_arg[2] = MAXJREDIRS;
                                disp_arg[3] = JREQ.h.bj_job;
                                disp_str = gbatch_gettitle(-1, &JREQ);
                                return  $E{Too many redirections};
                        }
                        str = lp + 1;
                }  while  (lp);
        }

        if  (gbatch_putredirlist(&newj, Nredirs, nr) == 0)  {
                disp_arg[3] = JREQ.h.bj_job;
                disp_str = gbatch_gettitle(-1, &JREQ);
                return  $E{Too many job strings};
        }

        JREQ = newj;
        return  0;
}

#define Jarg    char *

int  arg_args(const struct argop *ao)
{
        apiBtjob        newj;
        char    *str = ao->ao_un.ao_string;
        unsigned        na = 0;
        char    *Nargs[MAXJARGS];

        newj = JREQ;

        if  (*str)  {
                char  *lp;
                do  {
                        if  ((lp  = strchr(str, '\n')))
                                *lp = '\0';
                        Nargs[na] = str;
                        if  (++na >= MAXJARGS-1)  {
                                disp_arg[0] = na;
                                disp_arg[1] = JREQ.h.bj_nargs;
                                disp_arg[2] = MAXJARGS;
                                disp_arg[3] = JREQ.h.bj_job;
                                disp_str = gbatch_gettitle(-1, &JREQ);
                                return  $E{Too many arguments};
                        }
                        str = lp + 1;
                }  while  (lp);
        }

        Nargs[na] = (char *) 0;

        if  (gbatch_putarglist(&newj, (const char **) Nargs) == 0)  {
                disp_arg[3] = JREQ.h.bj_job;
                disp_str = gbatch_gettitle(-1, &JREQ);
                return  $E{Too many job strings};
        }
        JREQ = newj;
        return  0;
}

int  exdecode(const struct argop *ao, unsigned char *rp)
{
        char    *str = ao->ao_un.ao_string;
        unsigned  res = 0;
        if  (!isdigit(*str))
                return  $E{Bad exit code spec};
        do  res = res * 10 + *str++ - '0';
        while  (isdigit(*str));
        if  (res > 255)
                return  $E{Bad exit code spec};
        *rp = res;
        return  0;
}

/* Exit codes normal lower, upper, error lower, upper */

int  arg_nel(const struct argop *ao)
{
        return  exdecode(ao, &JREQ.h.bj_exits.nlower);
}

int  arg_neu(const struct argop *ao)
{
        return  exdecode(ao, &JREQ.h.bj_exits.nupper);
}

int  arg_eel(const struct argop *ao)
{
        return  exdecode(ao, &JREQ.h.bj_exits.elower);
}

int  arg_eeu(const struct argop *ao)
{
        return  exdecode(ao, &JREQ.h.bj_exits.eupper);
}

int  add_cond(char *arg, apiJcond *jc)
{
        long            crit, act;
        char            *oarg = arg, *np;
        struct  var_with_slot   vs;

        disp_str = arg;         /* In case of error */

        crit = strtol(arg, &np, 0);
        jc->bjc_iscrit &= ~CCRIT_NORUN;
        if  (crit)
                jc->bjc_iscrit |= CCRIT_NORUN;

        if  (*np != ',')
                return  $E{Bad condition};
        arg = ++np;

        if  (!find_var_by_name((const char **) &arg, &vs))  {
                disp_str = oarg;
                html_disperror($E{Unreadable variable});
                exit(E_NOPRIV);
        }

        arg++;
        jc->bjc_var.slotno = vs.slot;

        act = strtol(arg, &np, 0);
        if  (act < C_EQ  ||  act > C_GE)
                return  $E{Bad condition};

        jc->bjc_compar = (unsigned char) act;

        if  (*np != ',')
                return  $E{Bad condition};

        arg = ++np;
        if  (isdigit(*arg))  {
                jc->bjc_value.con_un.con_long = atol(arg);
                jc->bjc_value.const_type = CON_LONG;
        }
        else  {
                int  np = 0;
                while  (*arg)  {
                        if  (np >= BTC_VALUE)  {
                                html_disperror($E{Condition string too long});
                                exit(E_USAGE);
                        }
                        jc->bjc_value.con_un.con_string[np++] = *arg++;
                }
                jc->bjc_value.con_un.con_string[np] = '\0';
                jc->bjc_value.const_type = CON_STRING;
        }

        return  0;
}

int  arg_conds(const struct argop *ao)
{
        char    *str = ao->ao_un.ao_string;
        apiJcond        *jc = JREQ.h.bj_conds;
        int     nc = 0, retc;

        if  (*str)  {
                char    *lp;
                do  {
                        lp = strchr(str, '\n');
                        if  (nc >= MAXCVARS)
                                return  $E{Condition max exceeded};
                        if  (lp)
                                *lp = '\0';
                        if  ((retc = add_cond(str, jc)))
                                return  retc;
                        jc++;
                        nc++;
                        str = lp + 1;
                }  while  (lp);
        }

        while  (jc < &JREQ.h.bj_conds[MAXCVARS])  {
                jc->bjc_compar = C_UNUSED;
                jc++;
        }

        return  0;
}

int  add_ass(char *arg, apiJass *ja)
{
        long            crit, act;
        unsigned  long  flags;
        char            *oarg = arg, *np;
        struct  var_with_slot   vs;

        disp_str = arg;

        crit = strtol(arg, &np, 0);
        if  (*np != ',')
                return  $E{Invalid assignment};
        arg = ++np;
        ja->bja_iscrit &= ~ACRIT_NORUN;
        if  (crit)
                ja->bja_iscrit |= ACRIT_NORUN;

        flags = strtoul(arg, &np, 0);
        if  (*np != ',')
                return  $E{Invalid assignment};
        arg = ++np;
        ja->bja_flags = (USHORT) flags;

        if  (!find_var_by_name((const char **) &arg, &vs))  {
                disp_str = oarg;
                return  $E{Unwritable variable};
        }
        arg++;

        ja->bja_var.slotno = vs.slot;
        act = strtol(arg, &np, 0);
        if  (act < BJA_ASSIGN ||  act > BJA_SSIG  ||  *np != ',')
                return  $E{Invalid assignment};
        arg = ++np;
        ja->bja_op = (unsigned char) act;
        if  (act >= BJA_SEXIT)  {
                ja->bja_con.con_un.con_long = 0;
                ja->bja_con.const_type = CON_LONG;
        }
        else  if  (isdigit(*arg))  {
                ja->bja_con.con_un.con_long = atol(arg);
                ja->bja_con.const_type = CON_LONG;
        }
        else  {
                int  np = 0;
                while  (*arg)  {
                        if  (np >= BTC_VALUE)
                                return  $E{String too long in set};
                        ja->bja_con.con_un.con_string[np++] = *arg++;
                }
                ja->bja_con.con_un.con_string[np] = '\0';
                ja->bja_con.const_type = CON_STRING;
        }
        return  0;
}

int  arg_asses(const struct argop *ao)
{
        char    *str = ao->ao_un.ao_string;
        apiJass *ja = JREQ.h.bj_asses;
        int     na = 0, retc;

        if  (*str)  {
                char    *lp;
                do  {
                        lp = strchr(str, '\n');
                        if  (na >= MAXSEVARS)
                                return  $E{Assignment max exceeded};
                        if  (lp)
                                *lp = '\0';
                        if  ((retc = add_ass(str, ja)) != 0)
                                return  retc;
                        ja++;
                        na++;
                        str = lp + 1;
                }  while  (lp);
        }

        while  (ja < &JREQ.h.bj_asses[MAXSEVARS])  {
                ja->bja_op = BJA_NONE;
                ja++;
        }

        return  0;
}

struct  argop  aolist[] =  {
        {       "asses",        arg_asses,      AO_STRING },
        {       "conds",        arg_conds,      AO_STRING },
        {       "time",         arg_time,       AO_TIME },
        {       "repeat",       arg_repeat,     AO_UCHAR },
        {       "rate",         arg_repeatint,  AO_ULONG },
        {       "mday",         arg_mday,       AO_UCHAR, 1 },
        {       "avdays",       arg_avdays,     AO_USHORT },
        {       "ifnp",         arg_ifnposs,    AO_UCHAR },
        {       "title",        arg_title,      AO_STRING },
        {       "queue",        arg_queue,      AO_STRING },
        {       "export",       arg_export,     AO_STRING  },
        {       "mail",         arg_jflag,      AO_BOOL,        0,0,0,  {  (USHORT) BJ_MAIL  }  },
        {       "write",        arg_jflag,      AO_BOOL,        0,0,0,  {  (USHORT) BJ_WRT  }  },
        {       "noadv",        arg_jflag,      AO_BOOL,        0,0,0,  {  (USHORT) BJ_NOADVIFERR  }  },
        {       "interp",       arg_interp,     AO_STRING },
        {       "ll",           arg_ll,         AO_USHORT, 1 },
        {       "pri",          arg_pri,        AO_UCHAR },
        {       "deltime",      arg_deltime,    AO_USHORT },
        {       "runtime",      arg_runtime,    AO_ULONG },
        {       "runon",        arg_gracetime,  AO_USHORT },
        {       "ksig",         arg_ksig,       AO_USHORT },
        {       "umask",        arg_umask,      AO_USHORT },
        {       "ulimit",       arg_ulimit,     AO_ULONG },
        {       "directory",    arg_directory,  AO_STRING },
        {       "io",           arg_redirs,     AO_STRING },
        {       "args",         arg_args,       AO_STRING },
        {       "nel",          arg_nel,        AO_STRING },
        {       "neu",          arg_neu,        AO_STRING },
        {       "eel",          arg_eel,        AO_STRING },
        {       "eeu",          arg_eeu,        AO_STRING }
};

struct  argop   *aochain;

void  list_op(char *arg, char *cp)
{
        int     cnt;

        *cp = '\0';

        for  (cnt = 0;  cnt < sizeof(aolist) / sizeof(struct argop);  cnt++)  {
                struct  argop  *aop = &aolist[cnt];
                unsigned  long  res;
                if  (ncstrcmp(aop->name, arg) == 0)  {
                        *cp++ = '=';
                        switch  (aop->typ)  {
                        case  AO_BOOL:
                                switch  (*cp)  {
                                case  'y':case  'Y':
                                case  't':case  'T':
                                        aop->off = 0;
                                        break;
                                case  'n':case  'N':
                                case  'f':case  'F':
                                        aop->off = 255;
                                        break;
                                default:
                                        goto  badarg;
                                }
                                break;
                        case  AO_UCHAR:
                                if  (!isdigit(*cp))
                                        goto  badarg;
                                res = strtoul(cp, (char **) 0, 0);
                                if  (res > 255)
                                        goto  badarg;
                                aop->ao_un.ao_uchar = res;
                                break;
                        case  AO_USHORT:
                                if  (!isdigit(*cp))
                                        goto  badarg;
                                res = strtoul(cp, (char **) 0, 0);
                                if  (res > 0xffff)
                                        goto  badarg;
                                aop->ao_un.ao_ushort = res;
                                break;
                        case  AO_ULONG:
                                if  (!isdigit(*cp))
                                        goto  badarg;
                                aop->ao_un.ao_ulong = strtoul(cp, (char **) 0, 0);
                                break;
                        case  AO_TIME:
                                aop->ao_un.ao_time = strtol(cp, (char **) 0, 0);
                                break;
                        case  AO_STRING:
                                aop->ao_un.ao_string = cp;
                                break;
                        }
                        if  (!aop->had)  {
                                aop->had = 1;
                                aop->next = aochain;
                                aochain = aop;
                        }
                        return;
                }
        }

        *cp++ = '=';
 badarg:
        if  (html_out_cparam_file("badcarg", 1, arg))
                exit(E_USAGE);
        html_error(arg);
        exit(E_SETUP);
}

/* This is the main processing routine.  */

void  apply_ops(char *arg)
{
        struct  argop           *aop;
        int                     pass, retc;
        struct  jobswanted      jw;

        if  (decode_jnum(arg, &jw))  {
                if  (html_out_cparam_file("badcarg", 1, arg))
                        exit(E_USAGE);
                html_error(arg);
                exit(E_SETUP);
        }

        if  ((retc = gbatch_jobfind(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw.jno, jw.host, &jw.slot, &JREQ)) < 0)  {
                if  (retc == GBATCH_UNKNOWN_JOB)
                        html_out_cparam_file("jobgone", 1, arg);
                else
                        html_disperror($E{Base for API errors} + retc);
                exit(E_NOJOB);
        }

        /* And now do the business, passes 0 and 1
           We have 2 passes for the benefit of month days,
           which we want to calculate after we've fixed the
           repeat unit, and for load level, which should be
           set after the interpreter */

        for  (pass = 0;  pass < 2;  pass++)
                for  (aop = aochain;  aop;  aop = aop->next)
                        if  (aop->pass == pass)  {
                                /* If the thing is a string, set it up in the
                                   error stuff as a default. The code may override
                                   this, hence we must do it first. */
                                if  (aop->typ == AO_STRING)
                                        disp_str = aop->ao_un.ao_string;
                                if  ((retc = (*aop->arg_fn)(aop)) != 0)  {
                                        html_disperror(retc);
                                        exit(E_USAGE);
                                }
                        }

        if  ((retc = gbatch_jobupd(xbapi_fd, GBATCH_FLAG_IGNORESEQ, jw.slot, &JREQ)) < 0)  {
                html_disperror($E{Base for API errors} + retc);
                exit(E_NOPRIV);
        }
}

void  perform_update(char **args)
{
        char    **ap, *arg;

        for  (ap = args;  (arg = *ap);  ap++)  {
                char    *cp = strchr(arg, '=');
                if  (cp)
                        list_op(arg, cp);
                else
                        apply_ops(arg);
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
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        api_open(realuname);
        api_readvars(0);
        perform_update(newargs);
        html_out_or_err("chngok", 1);
        return  0;
}
