/* xmbr_vlist.c -- variable list for gbch-xmr

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/List.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "helpargs.h"
#include "cfile.h"

static  char    Filename[] = __FILE__;

/* For job conditions and assignments.  Check that specified variable
   name is valid, and if so return the index or -1 if not.  */

int  val_var(const char *name, const unsigned modeflag)
{
        int     first, last, middle, s;
        const   char    *colp;
        BtvarRef        vp;
        netid_t hostid = 0;

        if  (!name)
                return  -1;

        if  ((colp = strchr(name, ':')) != (char *) 0)  {
                char    hname[HOSTNSIZE+1];
                s = colp - name;
                if  (s > HOSTNSIZE)
                        s = HOSTNSIZE;
                strncpy(hname, name, s);
                hname[s] = '\0';
                if  ((hostid = look_hostname(hname)) == 0)
                        return  -1;
                colp++;
        }
        else
                colp = name;

        rvarlist(1);
        first = 0;
        last = Var_seg.nvars;

        while  (first < last)  {
                middle = (first + last) / 2;
                vp = &vv_ptrs[middle].vep->Vent;
                if  ((s = strcmp(colp, vp->var_name)) == 0)  {
                        if  (vp->var_id.hostid == hostid)  {
                                if  (mpermitted(&vp->var_mode, modeflag, mypriv->btu_priv))
                                        return  middle;
                                return  -1;
                        }
                        if  (vp->var_id.hostid < hostid)
                                first = middle + 1;
                        else
                                last = middle;
                }
                else  if  (s > 0)
                        first = middle + 1;
                else
                        last = middle;
        }
        return  -1;
}

/* Generate matrix of variables accessible via a given mode for job vars.  */

#define INCVLIST        10

static char **gen_vars(int isexport, unsigned mode)
{
        unsigned        vcnt, maxr, countr;
        char    **result;

        if  ((result = (char **) malloc((Var_seg.nvars + 1)/2 * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        maxr = (Var_seg.nvars + 1) / 2;
        countr = 0;

        rvarlist(1);

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                BtvarRef        vp = &vv_ptrs[vcnt].vep->Vent;

                /* Skip ones which are not allowed.  */

                if  (!mpermitted(&vp->var_mode, mode, mypriv->btu_priv))
                        continue;
                if  (isexport >= 0)  {
                        if  (isexport)  {
                                if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                        continue;
                        }
                        else
                                if  (vp->var_flags & VF_EXPORT)
                                        continue;
                }
                if  (countr + 1 >= maxr)  {
                        maxr += INCVLIST;
                        if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
                                ABORT_NOMEM;
                }
                result[countr++] = stracpy(VAR_NAME(vp));
        }

        if  (countr == 0)  {
                free((char *) result);
                return  (char **) 0;
        }

        result[countr] = (char *) 0;
        return  result;
}

char **gen_rvars(char *prefix)
{
        return  gen_vars(0, BTM_READ);
}

char **gen_wvars(char *prefix)
{
        return  gen_vars(0, BTM_WRITE);
}

char **gen_rvarse(char *prefix)
{
        return  gen_vars(1, BTM_READ);
}

char **gen_wvarse(char *prefix)
{
        return  gen_vars(1, BTM_WRITE);
}

char **gen_rvarsa(char *prefix)
{
        return  gen_vars(-1, BTM_READ);
}

char **gen_wvarsa(char *prefix)
{
        return  gen_vars(-1, BTM_WRITE);
}

/* Timeout routine for var val */

static void  vval_adj(int amount, XtIntervalId *id)
{
        LONG    newval;
        char    *txt, *cp;
        char    nbuf[20];
        XtVaGetValues(workw[WORKW_VARVAL], XmNvalue, &txt, NULL);
        for  (cp = txt;  isspace(*cp);  cp++)
                ;
        if  (!isdigit(*cp)  &&  *cp != '-')  {
                XtFree(txt);
                return;
        }
        newval = atol(cp) + amount;
        XtFree(txt);
        sprintf(nbuf, "%ld", (long) newval);
        XmTextSetString(workw[WORKW_VARVAL], nbuf);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) vval_adj, INT_TO_XTPOINTER(amount));
}

/* Arrows on var val */

void  vval_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                vval_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}

void  extract_vval(BtconRef result)
{
        char    *txt, *cp;

        XtVaGetValues(workw[WORKW_VARVAL], XmNvalue, &txt, NULL);
        for  (cp = txt;  isspace(*cp);  cp++)
                ;
        if  (isdigit(*cp)  ||  *cp == '-')  {
                result->const_type = CON_LONG;
                result->con_un.con_long = atol(cp);
        }
        else  {
                int     lng = strlen(txt);
                cp = txt;
                if  (*cp == '\"')  {
                        cp++;
                        lng--;
                        if  (cp[lng-1] == '\"')
                                lng--;
                }
                if  (lng > BTC_VALUE)
                        lng = BTC_VALUE;
                result->const_type = CON_STRING;
                strncpy(result->con_un.con_string, cp, (unsigned) lng);
                result->con_un.con_string[lng] = '\0';
        }
        XtFree(txt);
}
