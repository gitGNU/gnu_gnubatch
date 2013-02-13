/* optvars.c -- option handling for jobs related to variables

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
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "helpargs.h"
#include "btrvar.h"
#include "optflags.h"

/* Define these here */

int             Asscnt, Condcnt;
unsigned        sflags = BJA_START|BJA_REVERSE|BJA_OK|BJA_ERROR|BJA_ABORT;
struct  scond   Condlist[MAXCVARS];
struct  Sass    Asslist[MAXSEVARS];

/*
 * Extract variable name or hostname:variable name from argument
 * Return 0 if OK otherwise winge by returning error code.
 */

int  extractvar(const char **argp, struct vdescr *vdp)
{
        int     cnt;
        const   char    *arg = *argp;
        char    nbuf[BTV_NAME+1];

        vdp->crit = 0;
        vdp->hostid = 0;

        if  (strchr(arg, ':'))  {
                char    hbuf[HOSTNSIZE+1];

                cnt = 0;
                while  (*arg != ':')  {
                        if  (cnt < HOSTNSIZE)
                                hbuf[cnt++] = *arg;
                        arg++;
                }
                hbuf[cnt] = '\0';
                if  ((vdp->hostid = look_int_hostname(hbuf)) == -1)
                        return  $E{Unknown host name in var};
                arg++;          /* Past colon */
        }


        if  (!isalpha(*arg)  &&  *arg != '_')
                return  $E{Invalid variable name};

        cnt = 0;
        do  {
                if  (cnt >= BTV_NAME)
                        return  $E{Max var name size};
                nbuf[cnt++] = *arg++;
        }  while  (isalnum(*arg)  ||  *arg == '_');

        nbuf[cnt] = '\0';

        /* There is a storage leak here if we reset the count of
           conds/asses...  I don't think there is any more as we
           need to keep xmbtr clean.  */

        vdp->var = stracpy(nbuf);
        *argp = arg;
        return  0;
}

EOPTION(o_condcrit)
{
        Condasschanges |= OF_CRIT_COND;
        return  OPTRESULT_OK;
}

EOPTION(o_nocondcrit)
{
        Condasschanges &= ~OF_CRIT_COND;
        return  OPTRESULT_OK;
}

EOPTION(o_asscrit)
{
        Condasschanges |= OF_CRIT_ASS;
        return  OPTRESULT_OK;
}

EOPTION(o_noasscrit)
{
        Condasschanges &= ~OF_CRIT_ASS;
        return  OPTRESULT_OK;
}

EOPTION(o_canccond)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Condasschanges |= OF_COND_CHANGES;
        Condcnt = 0;
        return  OPTRESULT_OK;
}

EOPTION(o_condition)
{
        int     np;
        struct  scond   *sc;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Condasschanges |= OF_COND_CHANGES;

        while  (isspace(*arg))
                arg++;

        if  (Condcnt >= MAXCVARS)  {
                arg_errnum = $E{Condition max exceeded};
                return  OPTRESULT_ERROR;
        }

        sc = &Condlist[Condcnt];
        if  ((np = extractvar(&arg, &sc->vd)) != 0)  {
                arg_errnum = np;
                return  OPTRESULT_ERROR;
        }
        sc->vd.crit = sc->vd.hostid && Condasschanges & OF_CRIT_COND? CCRIT_NORUN: 0;

        /* Now look at condition */

        switch  (*arg++)  {
        default:
                goto  badcond;
        case  '!':
                if  (*arg++ != '=')
                        goto  badcond;
                sc->compar = C_NE;
                break;
        case  '=':
                if  (*arg == '=')
                        arg++;
                sc->compar = C_EQ;
                break;
        case  '<':
                if  (*arg == '=')  {
                        arg++;
                        sc->compar = C_LE;
                }
                else
                        sc->compar = C_LT;
                break;
        case  '>':
                if  (*arg == '=')  {
                        arg++;
                        sc->compar = C_GE;
                }
                else
                        sc->compar = C_GT;
                break;
        }

        if  (!isdigit(*arg) && *arg != '-')  {
                if  (*arg == ':')       /* Force to string */
                        arg++;
                np = 0;
                while  (*arg)  {
                        if  (np >= BTC_VALUE)  {
                                free(sc->vd.var);
                                arg_errnum = $E{Condition string too long};
                                return  OPTRESULT_ERROR;
                        }
                        sc->value.con_un.con_string[np++] = *arg++;
                }
                sc->value.con_un.con_string[np] = '\0';
                sc->value.const_type = CON_STRING;
        }
        else  {
                sc->value.con_un.con_long = atol(arg);
                sc->value.const_type = CON_LONG;
        }

        Condcnt++;
        return  OPTRESULT_ARG_OK;

 badcond:
        free(sc->vd.var);
        arg_errnum = $E{Bad condition};
        return  OPTRESULT_ERROR;
}

EOPTION(o_cancset)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Condasschanges |= OF_ASS_CHANGES;
        Asscnt = 0;
        return  OPTRESULT_OK;
}

EOPTION(o_flags)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        sflags = 0;

        while  (*arg)
                switch  (*arg++)  {
                case  'S':
                case  's':
                        sflags |= BJA_START;
                        break;
                case  'N':
                case  'n':
                        sflags |= BJA_OK;
                        break;
                case  'E':
                case  'e':
                        sflags |= BJA_ERROR;
                        break;
                case  'A':
                case  'a':
                        sflags |= BJA_ABORT;
                        break;
                case  'C':
                case  'c':
                        sflags |= BJA_CANCEL;
                        break;
                case  'R':
                case  'r':
                        sflags |= BJA_REVERSE;
                        break;
                }

        if  (sflags == 0)  {
                arg_errnum = $E{No set flags given};
                return  OPTRESULT_ERROR;
        }

        return  OPTRESULT_ARG_OK;
}

EOPTION(o_set)
{
        struct  Sass    *sa;
        int     np;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Condasschanges |= OF_ASS_CHANGES;

        while  (isspace(*arg))
                arg++;

        if  (Asscnt >= MAXSEVARS)  {
                arg_errnum = $E{Assignment max exceeded};
                return  OPTRESULT_ERROR;
        }

        sa = &Asslist[Asscnt];
        if  ((np = extractvar(&arg, &sa->vd)) != 0)  {
                arg_errnum = np;
                return  OPTRESULT_ERROR;
        }
        sa->vd.crit = sa->vd.hostid && Condasschanges & OF_CRIT_ASS? ACRIT_NORUN: 0;

        /* Now look at condition */

        switch  (*arg++)  {
        default:
                goto  badass;
        case  '=':
                sa->op = BJA_ASSIGN;
                break;
        case  '+':
                sa->op = BJA_INCR;
                goto  skipeq;
        case  '-':
                sa->op = BJA_DECR;
                goto  skipeq;
        case  '*':
                sa->op = BJA_MULT;
                goto  skipeq;
        case  '/':
                sa->op = BJA_DIV;
                goto  skipeq;
        case  '%':
                sa->op = BJA_MOD;
        skipeq:
                if  (*arg == '=')
                        arg++;
                break;
        }

        if  (!isdigit(*arg) && *arg != '-')  {
                if  (strcmp(arg, exitcodename) == 0)  {
                        if  (sa->op != BJA_ASSIGN)
                                goto  badass;
                        sa->op = BJA_SEXIT;
                        sa->con.con_un.con_long = 0;
                        sa->con.const_type = CON_LONG;
                        goto  fin;
                }
                else  if  (strcmp(arg, signalname) == 0)  {
                        if  (sa->op != BJA_ASSIGN)
                                goto  badass;
                        sa->op = BJA_SSIG;
                        sa->con.con_un.con_long = 0;
                        sa->con.const_type = CON_LONG;
                        goto  fin;
                }
                if  (*arg == ':')       /* Force to string */
                        arg++;
                np = 0;
                while  (*arg)  {
                        if  (np >= BTC_VALUE)  {
                                free(sa->vd.var);
                                arg_errnum = $E{String too long in set};
                                return  OPTRESULT_ERROR;
                        }
                        sa->con.con_un.con_string[np++] = *arg++;
                }
                sa->con.con_un.con_string[np] = '\0';
                sa->con.const_type = CON_STRING;
        }
        else  {
                sa->con.con_un.con_long = atol(arg);
                sa->con.const_type = CON_LONG;
        }
 fin:
        sa->flags = (USHORT) sflags;

        Asscnt++;
        return  OPTRESULT_ARG_OK;

 badass:
        free(sa->vd.var);
        arg_errnum = $E{Invalid assignment};
        return  OPTRESULT_ERROR;
}
