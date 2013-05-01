/* xmbr_jobpar.c -- job parameters for gbch-xmr

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
#include <X11/cursorfont.h>
#include <Xm/ArrowB.h>
#include <Xm/DialogS.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
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
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "spitrouts.h"

#ifndef _NFILE
#define _NFILE  64
#endif

static  char    Filename[] = __FILE__;

#define JCCRIT_P        0
#define JCVAR_P         2
#define JCOP_P          (JCVAR_P + HOSTNSIZE + BTV_NAME + 1)
#define JCVAL_P         (JCOP_P + 4)
#define JACRIT_P        0
#define JAVAR_P         2
#define JAOP_P          (JCVAR_P + HOSTNSIZE + BTV_NAME + 1)
#define JAFLAGS_P       (JCOP_P + 7)
#define JAVAL_P         (JAFLAGS_P + 6)

HelpaltRef      assnames, actnames, stdinnames;
int             redir_actlen, redir_stdinlen;
extern  char    *ccritmark,
                *scritmark;

Widget  listw;
int     current_select = -1;
static  int     ismc = 0;
#define ISMC_NORMAL     0
#define ISMC_MOVE       1
#define ISMC_COPY       2
#define ISMC_NEW        3

extern void  extract_vval(BtconRef);
extern void  vval_cb(Widget, int, XmArrowButtonCallbackStruct *);
extern int  val_var(const char *, const unsigned);

/* Copy but avoid copying trailing null */

#define movein(to, from)        BLOCK_COPY(to, from, strlen(from))
#ifdef  HAVE_MEMCPY
#define BLOCK_SET(to, n, ch)    memset((void *) to, ch, (unsigned) n)
#else
static void  BLOCK_SET(char *to, unsigned n, const char ch)
{
        while  (n != 0)
                *to++ = ch;
}
#endif

static void  movelist(Widget w, int value)
{
         ismc = value;
}

static  void    list_select(Widget w, void (*proc)(), XmListCallbackStruct *cbs)
{
        int     itemnum = cbs->item_position - 1;
        switch  (ismc)  {
        case  ISMC_MOVE:
                (*proc)(ISMC_MOVE, current_select, itemnum);
                break;
        case  ISMC_COPY:
                (*proc)(ISMC_COPY, current_select, itemnum);
                break;
        }
        current_select = itemnum;
        ismc = ISMC_NORMAL;
}

/* Create the edit stuff for a fixed list dialog */

static void  CreateFixedListDlg(Widget formw, char *listwname, XtCallbackProc newrout, XtCallbackProc editrout, XtCallbackProc delrout, void *mcproc)
{
        Widget  neww, editw, delw, movew, copyw;

        neww = XtVaCreateManagedWidget("New",
                                       xmPushButtonGadgetClass, formw,
                                       XmNtopAttachment,        XmATTACH_FORM,
                                       XmNleftAttachment,       XmATTACH_POSITION,
                                       XmNleftPosition,         0,
                                       XmNshowAsDefault,        True,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,            0,
                                       XmNbottomOffset,         0,
                                       NULL);

        editw = XtVaCreateManagedWidget("Edit",
                                        xmPushButtonGadgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                2,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        delw = XtVaCreateManagedWidget("Delete",
                                       xmPushButtonGadgetClass, formw,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_POSITION,
                                       XmNleftPosition,                 4,
                                       XmNshowAsDefault,                False,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,                    0,
                                       XmNbottomOffset,                 0,
                                       NULL);

        movew = XtVaCreateManagedWidget("Move",
                                        xmPushButtonGadgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                6,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        copyw = XtVaCreateManagedWidget("Copy",
                                        xmPushButtonGadgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                8,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                        0,
                                        NULL);

        XtAddCallback(neww, XmNactivateCallback, newrout, 0);
        XtAddCallback(editw, XmNactivateCallback, editrout, 0);
        XtAddCallback(delw, XmNactivateCallback, delrout, 0);
        XtAddCallback(movew, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_MOVE);
        XtAddCallback(copyw, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_COPY);
        listw = XtVaCreateManagedWidget(listwname,
                                        xmListWidgetClass,      formw,
                                        XmNvisibleItemCount,    MAXCVARS,
                                        XmNselectionPolicy,     XmSINGLE_SELECT,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           neww,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNrightAttachment,     XmATTACH_FORM,
                                        NULL);
        current_select = -1;
        XtAddCallback(listw, XmNdefaultActionCallback, (XtCallbackProc) list_select, (XtPointer) mcproc);
        XtAddCallback(listw, XmNsingleSelectionCallback, (XtCallbackProc) list_select, (XtPointer) mcproc);
}

/* Create the edit stuff for a scrolled list dialog */

static void  CreateScrolledListDlg(Widget formw, char *listwname, XtCallbackProc newrout, XtCallbackProc editrout, XtCallbackProc delrout, void *mcproc)
{
        Widget  neww, editw, delw, movew, copyw;
        int             n;
        Arg             args[6];

        neww = XtVaCreateManagedWidget("New",
                                       xmPushButtonGadgetClass, formw,
                                       XmNtopAttachment,        XmATTACH_FORM,
                                       XmNleftAttachment,       XmATTACH_POSITION,
                                       XmNleftPosition,         0,
                                       XmNshowAsDefault,        True,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,            0,
                                       XmNbottomOffset,         0,
                                       NULL);

        editw = XtVaCreateManagedWidget("Edit",
                                        xmPushButtonGadgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                2,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        delw = XtVaCreateManagedWidget("Delete",
                                       xmPushButtonGadgetClass, formw,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_POSITION,
                                       XmNleftPosition,                 4,
                                       XmNshowAsDefault,                False,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,                    0,
                                       XmNbottomOffset,                 0,
                                       NULL);

        movew = XtVaCreateManagedWidget("Move",
                                        xmPushButtonGadgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                6,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        copyw = XtVaCreateManagedWidget("Copy",
                                        xmPushButtonGadgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                8,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                        0,
                                        NULL);

        XtAddCallback(neww, XmNactivateCallback, newrout, 0);
        XtAddCallback(editw, XmNactivateCallback, editrout, 0);
        XtAddCallback(delw, XmNactivateCallback, delrout, 0);
        XtAddCallback(movew, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_MOVE);
        XtAddCallback(copyw, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_COPY);

        n = 0;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNtopWidget, neww); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        listw = XmCreateScrolledList(formw, listwname, args, n);
        current_select = -1;
        XtAddCallback(listw, XmNdefaultActionCallback, (XtCallbackProc) list_select, (XtPointer) mcproc);
        XtAddCallback(listw, XmNsingleSelectionCallback, (XtCallbackProc) list_select, (XtPointer) mcproc);
}

static void  vlist_cb(Widget w, int notused, XmSelectionBoxCallbackStruct *cbs)
{
        char    *value;

        XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
        if  (!value[0])  {
                doerror(w, $EH{xmbtq null var name});
                XtFree(value);
                return;
        }
        XmTextSetString(workw[WORKW_JVARNAME], value);
        XtFree(value);
        XtDestroyWidget(w);
}

static  void    getvarsel(Widget w, char **(*proc)())
{
        getitemsel(w, WORKW_JVARNAME, 0, "vselect", proc, vlist_cb);
}

static Widget  CreateVselDlg(Widget formw, const vhash_t existind, int writevars)
{
        Widget  prevleft, selb;
        BtjobRef        cj = cjob->job;

        prevleft = XtVaCreateManagedWidget("Variable",
                                           xmLabelWidgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_FORM,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           XmNborderWidth,      0,
                                           NULL);

        workw[WORKW_JVARNAME] = XtVaCreateManagedWidget("var",
                                                        xmTextFieldWidgetClass, formw,
                                                        XmNcursorPositionVisible,       False,
                                                        XmNeditable,            False,
                                                        XmNtopAttachment,       XmATTACH_FORM,
                                                        XmNleftAttachment,      XmATTACH_WIDGET,
                                                        XmNleftWidget,          prevleft,
                                                        XmNcolumns,             BTV_NAME + HOSTNSIZE + 1,
                                                        NULL);
        selb = XtVaCreateManagedWidget("vselect",
                                       xmPushButtonWidgetClass,         formw,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_WIDGET,
                                       XmNleftWidget,                   workw[WORKW_JVARNAME],
                                       NULL);

        XtAddCallback(selb, XmNactivateCallback, (XtCallbackProc) getvarsel,
                      writevars?
                        (cj?
                         (cj->h.bj_jflags & BJ_EXPORT?
                          (XtPointer) gen_wvarse: (XtPointer) gen_wvars): (XtPointer) gen_wvarsa) :
                      cj? (cj->h.bj_jflags & BJ_EXPORT? (XtPointer) gen_rvarse: (XtPointer) gen_rvars):
                      (XtPointer) gen_rvarsa);

        if  (existind >= 0)  {
                BtvarRef        vp = &Var_seg.vlist[existind].Vent;
                XmTextSetString(workw[WORKW_JVARNAME], (char *) VAR_NAME(vp));
        }
        return  workw[WORKW_JVARNAME];
}

static  Jcond           *copyconds;
static  Jcond           work_cond;
static  Jass            *copyasses;
static  Jass            work_ass;
static  struct  {
        char    *wname;
        USHORT  flag;
}  flagnames[] = {
        {       "Start",        BJA_START       },
        {       "Reverse",      BJA_REVERSE     },
        {       "Normal",       BJA_OK          },
        {       "Error",        BJA_ERROR       },
        {       "Abort",        BJA_ABORT       },
        {       "Cancel",       BJA_CANCEL      }};

static void  filljclist()
{
        int             jcnt, outl;
        JcondRef        jcp;
        BtvarRef        vp;
        XmString        str;
        char            obuf[100];

        XmListDeleteAllItems(listw);

        for  (jcnt = 0;  jcnt < MAXCVARS;  jcnt++)  {
                jcp = &copyconds[jcnt];
                if  (jcp->bjc_compar == C_UNUSED)
                        break;
                BLOCK_SET(obuf, sizeof(obuf), ' ');
                vp = &Var_seg.vlist[jcp->bjc_varind].Vent;
                movein(&obuf[JCVAR_P], VAR_NAME(vp));
                if  (jcp->bjc_iscrit & CCRIT_NORUN)
                        movein(&obuf[JCCRIT_P], ccritmark);
                movein(&obuf[JCOP_P], condname[jcp->bjc_compar - C_EQ]);
                if  (jcp->bjc_value.const_type == CON_STRING)  {
                        char    nbuf[BTC_VALUE+3];
                        sprintf(nbuf, "\"%s\"", jcp->bjc_value.con_un.con_string);
                        movein(&obuf[JCVAL_P], nbuf);
                }
                else  {
                        char    nbuf[20];
                        sprintf(nbuf, "%ld", (long) jcp->bjc_value.con_un.con_long);
                        movein(&obuf[JCVAL_P], nbuf);
                }
                for  (outl = sizeof(obuf) - 1;  outl >= 0  &&  obuf[outl] == ' ';  outl--)
                        ;
                obuf[outl+1] = '\0';
                str = XmStringCreateSimple(obuf);
                XmListAddItem(listw, str, 0);
                XmStringFree(str);
        }
        if  (current_select >= 0)
                XmListSelectPos(listw, current_select+1, False);
}

static void  filljalist()
{
        int             jcnt, outl;
        JassRef         jap;
        BtvarRef        vp;
        XmString        str;
        char            obuf[100];

        XmListDeleteAllItems(listw);

        for  (jcnt = 0;  jcnt < MAXSEVARS;  jcnt++)  {
                jap = &copyasses[jcnt];
                if  (jap->bja_op == BJA_NONE)
                        break;
                BLOCK_SET(obuf, sizeof(obuf), ' ');
                vp = &Var_seg.vlist[jap->bja_varind].Vent;
                movein(&obuf[JAVAR_P], VAR_NAME(vp));
                if  (jap->bja_iscrit & ACRIT_NORUN)
                        movein(&obuf[JACRIT_P], scritmark);
                movein(&obuf[JAOP_P], disp_alt((int) jap->bja_op, assnames));
                if  (jap->bja_op < BJA_SEXIT)  {
                        outl = JAFLAGS_P;
                        if  (jap->bja_flags & BJA_START)
                                obuf[outl++] = 'S';
                        if  (jap->bja_flags & BJA_REVERSE)
                                obuf[outl++] = 'R';
                        if  (jap->bja_flags & BJA_OK)
                                obuf[outl++] = 'N';
                        if  (jap->bja_flags & BJA_ERROR)
                                obuf[outl++] = 'E';
                        if  (jap->bja_flags & BJA_ABORT)
                                obuf[outl++] = 'A';
                        if  (jap->bja_flags & BJA_CANCEL)
                                obuf[outl++] = 'C';
                        if  (jap->bja_con.const_type == CON_STRING)  {
                                char    nbuf[BTC_VALUE+3];
                                sprintf(nbuf, "\"%s\"", jap->bja_con.con_un.con_string);
                                movein(&obuf[JAVAL_P], nbuf);
                        }
                        else  {
                                char    nbuf[20];
                                sprintf(nbuf, "%ld", (long) jap->bja_con.con_un.con_long);
                                movein(&obuf[JAVAL_P], nbuf);
                        }
                }
                for  (outl = sizeof(obuf) - 1;  outl >= 0  &&  obuf[outl] == ' ';  outl--)
                        ;
                obuf[outl+1] = '\0';
                str = XmStringCreateSimple(obuf);
                XmListAddItem(listw, str, 0);
                XmStringFree(str);
        }
        if  (current_select >= 0)
                XmListSelectPos(listw, current_select+1, False);
}

static void  changecompar(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                work_cond.bjc_compar = (unsigned char) n;
}

static void  changeassop(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (n >= BJA_SEXIT  &&  work_ass.bja_op < BJA_SEXIT)  {
                        extract_vval(&work_ass.bja_con);
                        XmTextSetString(workw[WORKW_VARVAL], "");
                }
                else  if  (n < BJA_SEXIT  &&  work_ass.bja_op >= BJA_SEXIT)  {
                        if  (work_ass.bja_con.const_type == CON_STRING)  {
                                char    nbuf[BTC_VALUE+3];
                                sprintf(nbuf, "\"%s\"", work_ass.bja_con.con_un.con_string);
                                XmTextSetString(workw[WORKW_VARVAL], nbuf);
                        }
                        else  {
                                char    nbuf[20];
                                sprintf(nbuf, "%ld", (long) work_ass.bja_con.con_un.con_long);
                                XmTextSetString(workw[WORKW_VARVAL], nbuf);
                        }
                }
                work_ass.bja_op = (unsigned char) n;
        }
}

static void  changeccrit(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (n)
                        work_cond.bjc_iscrit |= CCRIT_NORUN;
                else
                        work_cond.bjc_iscrit &= ~CCRIT_NORUN;
        }
}

static void  changeacrit(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (n)
                        work_ass.bja_iscrit |= ACRIT_NORUN;
                else
                        work_ass.bja_iscrit &= ~ACRIT_NORUN;
        }
}

static void  changaflags(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                work_ass.bja_flags |= n;
        else
                work_ass.bja_flags &= ~n;
}

static void  endcondedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt;
                vhash_t varind;
                XtVaGetValues(workw[WORKW_JVARNAME], XmNvalue, &txt, NULL);
                if  ((varind = val_var(txt, BTM_READ)) < 0)  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid variable});
                        return;
                }
                XtFree(txt);
                work_cond.bjc_varind = vv_ptrs[varind].place;
                extract_vval(&work_cond.bjc_value);
                copyconds[current_select] = work_cond;
                filljclist();
                ismc = ISMC_NORMAL;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  endassedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt;
                vhash_t varind;
                XtVaGetValues(workw[WORKW_JVARNAME], XmNvalue, &txt, NULL);
                if  ((varind = val_var(txt, BTM_WRITE)) < 0)  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq var not writable});
                        return;
                }
                XtFree(txt);
                work_ass.bja_varind = vv_ptrs[varind].place;
                extract_vval(&work_ass.bja_con);
                copyasses[current_select] = work_ass;
                filljalist();
                ismc = ISMC_NORMAL;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  condedit(Widget w, int isnew)
{
        Widget  ce_shell, panew, formw, condrc, critrc, ncrit, crit, prevabove, prevleft;
        int     cnt;

        ismc = isnew? ISMC_NEW: ISMC_NORMAL;

        CreateEditDlg(w, "condedit", &ce_shell, &panew, &formw, 3);
        prevabove = CreateVselDlg(formw, work_cond.bjc_varind, 0);
        condrc = XtVaCreateManagedWidget("conds",
                                         xmRowColumnWidgetClass,formw,
                                         XmNtopAttachment,      XmATTACH_WIDGET,
                                         XmNtopWidget,          prevabove,
                                         XmNleftAttachment,     XmATTACH_FORM,
                                         XmNpacking,            XmPACK_COLUMN,
                                         XmNnumColumns,         2,
                                         XmNisHomogeneous,      True,
                                         XmNentryClass,         xmToggleButtonGadgetClass,
                                         XmNradioBehavior,      True,
                                         NULL);
        critrc = XtVaCreateManagedWidget("crit",
                                         xmRowColumnWidgetClass,formw,
                                         XmNtopAttachment,      XmATTACH_WIDGET,
                                         XmNtopWidget,          prevabove,
                                         XmNleftAttachment,     XmATTACH_WIDGET,
                                         XmNleftWidget,         condrc,
                                         XmNpacking,            XmPACK_COLUMN,
                                         XmNnumColumns,         1,
                                         XmNisHomogeneous,      True,
                                         XmNentryClass,         xmToggleButtonGadgetClass,
                                         XmNradioBehavior,      True,
                                         NULL);
        for  (cnt = 0;  cnt < NUM_CONDTYPES;  cnt++)  {
                Widget  w = XtVaCreateManagedWidget(condname[cnt], xmToggleButtonGadgetClass, condrc, NULL);
                if  (work_cond.bjc_compar == cnt + C_EQ)
                        XmToggleButtonGadgetSetState(w, True, False);
                XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) changecompar, INT_TO_XTPOINTER(cnt + C_EQ));
        }

        ncrit = XtVaCreateManagedWidget("ncrit", xmToggleButtonGadgetClass, critrc, NULL);
        crit = XtVaCreateManagedWidget("crit", xmToggleButtonGadgetClass, critrc, NULL);
        XmToggleButtonGadgetSetState(work_cond.bjc_iscrit & CCRIT_NORUN? crit: ncrit, True, False);
        XtAddCallback(ncrit, XmNvalueChangedCallback, (XtCallbackProc) changeccrit, (XtPointer) 0);
        XtAddCallback(crit, XmNvalueChangedCallback, (XtCallbackProc) changeccrit, (XtPointer) 1);
        prevleft = place_label_left(formw, condrc, "value");
        prevleft =
                workw[WORKW_VARVAL] =
                        XtVaCreateManagedWidget("val",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   condrc,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNcolumns,                     BTC_VALUE+2,
                                                NULL);

        CreateArrowPair("vval",                         formw,
                               condrc,                          prevleft,
                               (XtCallbackProc) vval_cb,        (XtCallbackProc) vval_cb,
                               1,                               -1);

        if  (work_cond.bjc_value.const_type == CON_STRING)  {
                char    nbuf[BTC_VALUE+3];
                sprintf(nbuf, "\"%s\"", work_cond.bjc_value.con_un.con_string);
                XmTextSetString(workw[WORKW_VARVAL], nbuf);
        }
        else  {
                char    nbuf[20];
                sprintf(nbuf, "%ld", (long) work_cond.bjc_value.con_un.con_long);
                XmTextSetString(workw[WORKW_VARVAL], nbuf);
        }

        XtManageChild(formw);
        CreateActionEndDlg(ce_shell, panew, (XtCallbackProc) endcondedit, $H{xmbtq condedit dialog});
}

static void  assedit(Widget w, int isnew)
{
        Widget  ae_shell, panew, formw, assrc, critrc, flagrc, ncrit, crit, prevabove, prevleft;
        int     cnt;

        ismc = isnew? ISMC_NEW: ISMC_NORMAL;

        CreateEditDlg(w, "assedit", &ae_shell, &panew, &formw, 3);
        prevabove = CreateVselDlg(formw, work_ass.bja_varind, 1);
        assrc = XtVaCreateManagedWidget("asses",
                                        xmRowColumnWidgetClass, formw,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           prevabove,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNpacking,             XmPACK_COLUMN,
                                        XmNnumColumns,          2,
                                        XmNisHomogeneous,       True,
                                        XmNentryClass,          xmToggleButtonGadgetClass,
                                        XmNradioBehavior,       True,
                                        NULL);
        critrc = XtVaCreateManagedWidget("crit",
                                         xmRowColumnWidgetClass,formw,
                                         XmNtopAttachment,      XmATTACH_WIDGET,
                                         XmNtopWidget,          prevabove,
                                         XmNleftAttachment,     XmATTACH_WIDGET,
                                         XmNleftWidget,         assrc,
                                         XmNpacking,            XmPACK_COLUMN,
                                         XmNnumColumns,         1,
                                         XmNisHomogeneous,      True,
                                         XmNentryClass,         xmToggleButtonGadgetClass,
                                         XmNradioBehavior,      True,
                                         NULL);
        flagrc = XtVaCreateManagedWidget("flags",
                                         xmRowColumnWidgetClass,formw,
                                         XmNtopAttachment,      XmATTACH_WIDGET,
                                         XmNtopWidget,          prevabove,
                                         XmNleftAttachment,     XmATTACH_WIDGET,
                                         XmNleftWidget,         critrc,
                                         XmNpacking,            XmPACK_COLUMN,
                                         XmNnumColumns,         2,
                                         XmNisHomogeneous,      True,
                                         XmNentryClass,         xmToggleButtonGadgetClass,
                                         XmNradioBehavior,      False,
                                         NULL);

        for  (cnt = 0;  cnt < assnames->numalt;  cnt++)  {
                Widget  w = XtVaCreateManagedWidget(assnames->list[cnt], xmToggleButtonGadgetClass, assrc, NULL);
                if  (work_ass.bja_op == assnames->alt_nums[cnt])
                        XmToggleButtonGadgetSetState(w, True, False);
                XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) changeassop, INT_TO_XTPOINTER((int) assnames->alt_nums[cnt]));
        }

        for  (cnt = 0;  cnt < XtNumber(flagnames);  cnt++)  {
                Widget  w = XtVaCreateManagedWidget(flagnames[cnt].wname, xmToggleButtonGadgetClass, flagrc, NULL);
                if  (work_ass.bja_flags & flagnames[cnt].flag)
                        XmToggleButtonGadgetSetState(w, True, False);
                XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) changaflags, INT_TO_XTPOINTER((int) flagnames[cnt].flag));
        }
        ncrit = XtVaCreateManagedWidget("ncrit", xmToggleButtonGadgetClass, critrc, NULL);
        crit = XtVaCreateManagedWidget("crit", xmToggleButtonGadgetClass, critrc, NULL);
        XmToggleButtonGadgetSetState(work_ass.bja_iscrit & ACRIT_NORUN? crit: ncrit, True, False);
        XtAddCallback(ncrit, XmNvalueChangedCallback, (XtCallbackProc) changeacrit, (XtPointer) 0);
        XtAddCallback(crit, XmNvalueChangedCallback, (XtCallbackProc) changeacrit, (XtPointer) 1);
        prevleft = place_label_left(formw, assrc, "value");
        prevleft =
                workw[WORKW_VARVAL] =
                        XtVaCreateManagedWidget("val",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   assrc,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNcolumns,                     BTC_VALUE+2,
                                                NULL);

        CreateArrowPair("vval",                         formw,
                               assrc,                           prevleft,
                               (XtCallbackProc) vval_cb,        (XtCallbackProc) vval_cb,
                               1,                               -1);

        if  (work_ass.bja_op < BJA_SEXIT)  {
                if  (work_ass.bja_con.const_type == CON_STRING)  {
                        char    nbuf[BTC_VALUE+3];
                        sprintf(nbuf, "\"%s\"", work_ass.bja_con.con_un.con_string);
                        XmTextSetString(workw[WORKW_VARVAL], nbuf);
                }
                else  {
                        char    nbuf[20];
                        sprintf(nbuf, "%ld", (long) work_ass.bja_con.con_un.con_long);
                        XmTextSetString(workw[WORKW_VARVAL], nbuf);
                }
        }

        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endassedit, $H{xmbtq assedit dialog});
}

/* Maybe put default-setting back in sometime....  */

static void  newcond(Widget w)
{
        int     jcnt;

        for  (jcnt = 0;  jcnt < MAXCVARS;  jcnt++)
                if  (copyconds[jcnt].bjc_compar == C_UNUSED)
                        goto  gotit;
        doerror(w, $EH{xmbtq too many conds});
        return;
 gotit:
        current_select = jcnt;
        work_cond.bjc_varind = -1;
        work_cond.bjc_compar = C_EQ;
        work_cond.bjc_value.const_type = CON_LONG;
        work_cond.bjc_value.con_un.con_long = 0;
        work_cond.bjc_iscrit = 0;
        condedit(w, 1);
}

static void  newass(Widget w)
{
        int     jcnt;

        for  (jcnt = 0;  jcnt < MAXSEVARS;  jcnt++)
                if  (copyasses[jcnt].bja_op == BJA_NONE)
                        goto  gotit;
        doerror(w, $EH{xmbtq too many asses});
        return;
 gotit:
        current_select = jcnt;
        work_ass.bja_varind = -1;
        work_ass.bja_op = BJA_ASSIGN;
        work_ass.bja_con.const_type = CON_LONG;
        work_ass.bja_con.con_un.con_long = CON_LONG;
        work_ass.bja_flags = BJA_START|BJA_REVERSE|BJA_OK|BJA_ERROR|BJA_ABORT;
        work_ass.bja_iscrit = 0;
        assedit(w, 1);
}

static void  editcond(Widget w)
{
        JcondRef        cc;
        if  (current_select < 0  ||  (cc = &copyconds[current_select])->bjc_compar == C_UNUSED)  {
                doerror(w, $EH{xmbtq no cond selected});
                return;
        }
        work_cond = *cc;
        condedit(w, 0);
}

static void  editass(Widget w)
{
        JassRef ca;
        if  (current_select < 0  ||  (ca = &copyasses[current_select])->bja_op == BJA_NONE)  {
                doerror(w, $EH{xmbtq no ass selected});
                return;
        }
        work_ass = *ca;
        assedit(w, 0);
}

static void  delcond(Widget w)
{
        JcondRef        cc;
        if  (current_select < 0  ||  (cc = &copyconds[current_select])->bjc_compar == C_UNUSED)  {
                doerror(w, $EH{xmbtq no cond selected});
                return;
        }
        while  (cc < &copyconds[MAXCVARS])  {
                cc[0] = cc[1];
                cc++;
        }
        cc--;
        cc->bjc_compar = C_UNUSED;
        filljclist();
}

static void  delass(Widget w)
{
        JassRef ca;
        if  (current_select < 0  ||  (ca = &copyasses[current_select])->bja_op == BJA_NONE)  {
                doerror(w, $EH{xmbtq no ass selected});
                return;
        }
        while  (ca < &copyasses[MAXSEVARS])  {
                ca[0] = ca[1];
                ca++;
        }
        ca--;
        ca->bja_op = BJA_NONE;
        filljalist();
}

static void  mccond(int mtype, int from, int to)
{
        int     jcnt;
        Jcond   save;

        save = copyconds[from];

        if  (mtype == ISMC_MOVE)  {
                if  (from == to)
                        return;
                if  (from < to)
                        for  (jcnt = from;  jcnt < to;  jcnt++)
                                copyconds[jcnt] = copyconds[jcnt+1];
                else
                        for  (jcnt = from;  jcnt > to;  jcnt--)
                                copyconds[jcnt] = copyconds[jcnt-1];
                copyconds[to] = save;
        }
        else  {
                for  (jcnt = 0;  jcnt < MAXCVARS;  jcnt++)
                        if  (copyconds[jcnt].bjc_compar == C_UNUSED)
                                goto  gotit;
                doerror(listw, $EH{xmbtq too many conds});
                return;
        gotit:
                for  (jcnt = MAXCVARS-1;  jcnt > to;  jcnt--)
                        copyconds[jcnt] = copyconds[jcnt-1];
                copyconds[to] = save;
        }
        filljclist();
}

static void  mcass(int mtype, int from, int to)
{
        int     jcnt;
        Jass    save;

        save = copyasses[from];

        if  (mtype == ISMC_MOVE)  {
                if  (from == to)
                        return;
                if  (from < to)
                        for  (jcnt = from;  jcnt < to;  jcnt++)
                                copyasses[jcnt] = copyasses[jcnt+1];
                else
                        for  (jcnt = from;  jcnt > to;  jcnt--)
                                copyasses[jcnt] = copyasses[jcnt-1];
                copyasses[to] = save;
        }
        else  {
                for  (jcnt = 0;  jcnt < MAXSEVARS;  jcnt++)
                        if  (copyasses[jcnt].bja_op == BJA_NONE)
                                goto  gotit;
                doerror(listw, $EH{xmbtq too many asses});
                return;
        gotit:
                for  (jcnt = MAXSEVARS-1;  jcnt > to;  jcnt--)
                        copyasses[jcnt] = copyasses[jcnt-1];
                copyasses[to] = save;
        }
        filljalist();
}

static void  endjconds(Widget w, int data)
{
        if  (data)  {
                BtjobRef        bjp = cjob->job;
                BLOCK_COPY(bjp->h.bj_conds, copyconds, MAXCVARS * sizeof(Jcond));
                cjob->changes++;
                cjob->nosubmit++;
        }
        if  (copyconds)  {
                free((char *) copyconds);
                copyconds = (Jcond *) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  endjasses(Widget w, int data)
{
        if  (data)  {
                BtjobRef        bjp = cjob->job;
                BLOCK_COPY(bjp->h.bj_asses, copyasses, MAXSEVARS * sizeof(Jass));
                cjob->changes++;
                cjob->nosubmit++;
        }
        if  (copyasses)  {
                free((char *) copyasses);
                copyasses = (Jass *) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jconds(Widget parent, int isjob)
{
        Widget  jc_shell, panew, editform;
        int     jcnt, hadj;
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;

        if  (!(copyconds = (Jcond *) malloc(sizeof(Jcond) * MAXCVARS)))
                ABORT_NOMEM;
        for  (jcnt = hadj = 0;  jcnt < MAXCVARS;  jcnt++)
                if  (cj->h.bj_conds[jcnt].bjc_compar != C_UNUSED)
                        copyconds[hadj++] = cj->h.bj_conds[jcnt];
        for  (;  hadj < MAXCVARS;  hadj++)
                copyconds[hadj].bjc_compar = C_UNUSED;

        CreateEditDlg(parent, "Jcond", &jc_shell, &panew, &editform, 5);
        CreateFixedListDlg(editform, "Jclist", (XtCallbackProc) newcond, (XtCallbackProc) editcond, (XtCallbackProc) delcond, (void *) mccond);
        filljclist();
        XtManageChild(editform);
        CreateActionEndDlg(jc_shell, panew, (XtCallbackProc) endjconds, $H{xmbtq condlist dialog});
}

void  cb_jass(Widget parent, int isjob)
{
        Widget  ja_shell, panew, editform;
        int     jcnt, hadj;
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;

        if  (!(copyasses = (Jass *) malloc(sizeof(Jass) * MAXSEVARS)))
                ABORT_NOMEM;
        for  (jcnt = hadj = 0;  jcnt < MAXSEVARS;  jcnt++)
                if  (cj->h.bj_asses[jcnt].bja_op != BJA_NONE)
                        copyasses[hadj++] = cj->h.bj_asses[jcnt];
        for  (;  hadj < MAXSEVARS;  hadj++)
                copyasses[hadj].bja_op = BJA_NONE;

        CreateEditDlg(parent, "Jass", &ja_shell, &panew, &editform, 5);
        CreateFixedListDlg(editform, "Jalist", (XtCallbackProc) newass, (XtCallbackProc) editass, (XtCallbackProc) delass, (void *) mcass);
        filljalist();
        XtManageChild(editform);
        CreateActionEndDlg(ja_shell, panew, (XtCallbackProc) endjasses, $H{xmbtq asslist dialog});
}

static  char    **copyargs;
static  Menvir  *copyenvs;
static  Mredir  *copyredirs;
static  Mredir  work_redir;
static  int     Nitems;

static void  filljarglist()
{
        int             jcnt;
        XmString        str;

        XmListDeleteAllItems(listw);

        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)  {
                str = XmStringCreateSimple(copyargs[jcnt][0] == '\0'? "\"\"": copyargs[jcnt]);
                XmListAddItem(listw, str, 0);
                XmStringFree(str);
        }
        if  (current_select >= 0)
                XmListSelectPos(listw, current_select+1, False);
}

static void  endargedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt;
                int     cnt;
                XtVaGetValues(workw[WORKW_JARGVAL], XmNvalue, &txt, NULL);
                if  (ismc == ISMC_NEW)  {
                        current_select++;
                        for  (cnt = Nitems - 1;  cnt >= current_select;  cnt--)
                                copyargs[cnt+1] = copyargs[cnt];
                        Nitems++;
                }
                else  if  (current_select < Nitems)
                        free(copyargs[current_select]);
                copyargs[current_select] = stracpy(txt);
                XtFree(txt);
                filljarglist();
                ismc = ISMC_NORMAL;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  argedit(Widget w, int isnew)
{
        Widget  ae_shell, panew, formw, prevleft;

        ismc = isnew? ISMC_NEW: ISMC_NORMAL;

        CreateEditDlg(w, "argedit", &ae_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "value");
        prevleft =
                workw[WORKW_JARGVAL] =
                        XtVaCreateManagedWidget("val",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNrightAttachment,             XmATTACH_FORM,
                                                NULL);

        if  (!isnew)
                XmTextSetString(workw[WORKW_JARGVAL], copyargs[current_select]);
        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endargedit, $H{xmbtq argedit dialog});
}

static void  newarg(Widget w)
{
        if  (Nitems >= MAXJARGS)  {
                doerror(w, $EH{Too many arguments});
                return;
        }
        argedit(w, 1);
}

static void  editarg(Widget w)
{
        if  (current_select < 0  ||  current_select >= Nitems)  {
                doerror(w, $EH{xmbtq no arg selected});
                return;
        }
        argedit(w, 0);
}

static void  delarg(Widget w)
{
        int     cnt;
        if  (current_select < 0  ||  current_select >= Nitems)  {
                doerror(w, $EH{xmbtq no arg selected});
                return;
        }
        Nitems--;
        free(copyargs[current_select]);
        for  (cnt = current_select;  cnt < Nitems;  cnt++)
                copyargs[cnt] = copyargs[cnt+1];
        filljarglist();
}

static void  mcarg(int mtype, int from, int to)
{
        int     jcnt;
        char    *save = copyargs[from];

        if  (mtype == ISMC_MOVE)  {
                if  (from == to)
                        return;
                if  (from < to)
                        for  (jcnt = from;  jcnt < to;  jcnt++)
                                copyargs[jcnt] = copyargs[jcnt+1];
                else
                        for  (jcnt = from;  jcnt > to;  jcnt--)
                                copyargs[jcnt] = copyargs[jcnt-1];
                copyargs[to] = save;
        }
        else  {
                if  (Nitems >= MAXJARGS)  {
                        doerror(listw, $EH{Too many arguments});
                        return;
                }
                for  (jcnt = Nitems;  jcnt > to;  jcnt--)
                        copyargs[jcnt] = copyargs[jcnt-1];
                copyargs[to] = stracpy(save);
                Nitems++;
        }
        filljarglist();
}

static void  endjargs(Widget w, int data)
{
        if  (data)  {
                BtjobRef        bjp = cjob->job;
                Btjob           njob;

                njob = *bjp;
                if  (!repackjob(&njob, bjp, (char *) 0, (char *) 0, 0, 0, Nitems, (MredirRef) 0, (MenvirRef) 0, copyargs))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = title_of(bjp);
                        doerror(w, $EH{Too many job strings});
                        return;
                }
                *bjp = njob;
                cjob->changes++;
                cjob->nosubmit++;
        }
        if  (copyargs)  {
                int     cnt;
                for  (cnt = 0;  cnt < Nitems;  cnt++)
                        free(copyargs[cnt]);
                free((char *) copyargs);
                copyargs = (char **) 0;
        }
        Nitems = 0;
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jargs(Widget parent, int isjob)
{
        Widget  ja_shell, panew, editform;
        int     jcnt;
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;

        if  (!(copyargs = (char **) malloc(sizeof(char *) * MAXJARGS)))
                ABORT_NOMEM;
        Nitems = cj->h.bj_nargs;
        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)
                copyargs[jcnt] = stracpy(ARG_OF(cj, jcnt));

        CreateEditDlg(parent, "Jargs", &ja_shell, &panew, &editform, 5);
        CreateScrolledListDlg(editform, "Jarglist", (XtCallbackProc) newarg, (XtCallbackProc) editarg, (XtCallbackProc) delarg, (void *) mcarg);
        filljarglist();
        XtManageChild(listw);
        XtManageChild(editform);
        CreateActionEndDlg(ja_shell, panew, (XtCallbackProc) endjargs, $H{xmbtq arglist dialog});
}

static void  filljenvlist()
{
        int             jcnt;
        XmString        str1, str2, str3, res1, res2;

        str2 = XmStringCreateSimple("=");
        XmListDeleteAllItems(listw);

        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)  {
                str1 = XmStringCreateSimple(copyenvs[jcnt].e_name);
                str3 = XmStringCreateSimple(copyenvs[jcnt].e_value);
                res1 = XmStringConcat(str1, str2);
                XmStringFree(str1);
                res2 = XmStringConcat(res1, str3);
                XmStringFree(str3);
                XmStringFree(res1);
                XmListAddItem(listw, res2, 0);
                XmStringFree(res2);
        }
        if  (current_select >= 0)
                XmListSelectPos(listw, current_select+1, False);
        XmStringFree(str2);
}

static void  endenvedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt, *cp;
                unsigned lng;
                int      cnt;

                XtVaGetValues(workw[WORKW_JARGVAL], XmNvalue, &txt, NULL);
                if  (!(cp = strchr(txt, '=')))  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid environ});
                        return;
                }
                if  (ismc == ISMC_NEW)  {
                        current_select++;
                        for  (cnt = Nitems - 1;  cnt >= current_select;  cnt--)
                                copyenvs[cnt+1] = copyenvs[cnt];
                        Nitems++;
                }
                else  if  (current_select < Nitems)  {
                        free(copyenvs[current_select].e_name);
                        free(copyenvs[current_select].e_value);
                }
                lng = cp - txt;
                if  (!(copyenvs[current_select].e_name = malloc(lng+1)))
                        ABORT_NOMEM;
                strncpy(copyenvs[current_select].e_name, txt, lng);
                copyenvs[current_select].e_name[lng] = '\0';
                copyenvs[current_select].e_value = stracpy(cp+1);
                XtFree(txt);
                filljenvlist();
                ismc = ISMC_NORMAL;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  envedit(Widget w, int isnew)
{
        Widget  ae_shell, panew, formw, prevleft;

        ismc = isnew? ISMC_NEW: ISMC_NORMAL;

        CreateEditDlg(w, "envedit", &ae_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "value");
        prevleft =
                workw[WORKW_JARGVAL] =
                        XtVaCreateManagedWidget("val",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNrightAttachment,             XmATTACH_FORM,
                                                NULL);

        if  (!isnew)  {
                char    *nbuf = malloc((unsigned)(strlen(copyenvs[current_select].e_name) +
                                       strlen(copyenvs[current_select].e_value) + 2));
                if  (!nbuf)
                        ABORT_NOMEM;
                sprintf(nbuf, "%s=%s", copyenvs[current_select].e_name, copyenvs[current_select].e_value);
                XmTextSetString(workw[WORKW_JARGVAL], nbuf);
        }
        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endenvedit, $H{xmbtq envedit dialog});
}

static void  newenv(Widget w)
{
        if  (Nitems >= MAXJENVIR)  {
                doerror(w, $EH{Too large environment});
                return;
        }
        envedit(w, 1);
}

static void  editenv(Widget w)
{
        if  (current_select < 0  ||  current_select >= Nitems)  {
                doerror(w, $EH{xmbtq no environ selected});
                return;
        }
        envedit(w, 0);
}

static void  delenv(Widget w)
{
        int     cnt;
        if  (current_select < 0  ||  current_select >= Nitems)  {
                doerror(w, $EH{xmbtq no environ selected});
                return;
        }
        Nitems--;
        free(copyenvs[current_select].e_name);
        free(copyenvs[current_select].e_value);
        for  (cnt = current_select;  cnt < Nitems;  cnt++)
                copyenvs[cnt] = copyenvs[cnt+1];
        filljenvlist();
}

static void  mcenv(int mtype, int from, int to)
{
        int     jcnt;
        Menvir  save;

        save = copyenvs[from];

        if  (mtype == ISMC_MOVE)  {
                if  (from == to)
                        return;
                if  (from < to)
                        for  (jcnt = from;  jcnt < to;  jcnt++)
                                copyenvs[jcnt] = copyenvs[jcnt+1];
                else
                        for  (jcnt = from;  jcnt > to;  jcnt--)
                                copyenvs[jcnt] = copyenvs[jcnt-1];
                copyenvs[to] = save;
        }
        else  {
                if  (Nitems >= MAXJENVIR)  {
                        doerror(listw, $EH{Too large environment});
                        return;
                }
                for  (jcnt = Nitems;  jcnt > to;  jcnt--)
                        copyenvs[jcnt] = copyenvs[jcnt-1];
                copyenvs[to].e_name = stracpy(save.e_name);
                copyenvs[to].e_value = stracpy(save.e_value);
                Nitems++;
        }
        filljenvlist();
}

static void  endjenvs(Widget w, int data)
{
        if  (data)  {
                BtjobRef        bjp = cjob->job;
                Btjob           njob;

                njob = *bjp;
                if  (!repackjob(&njob, bjp, (char *) 0, (char *) 0, 0, Nitems, 0, (MredirRef) 0, copyenvs, (char **) 0))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = title_of(bjp);
                        doerror(w, $EH{Too many job strings});
                        return;
                }
                *bjp = njob;
                cjob->changes++;
                cjob->nosubmit++;
        }
        if  (copyenvs)  {
                int     cnt;
                for  (cnt = 0;  cnt < Nitems;  cnt++)  {
                        free(copyenvs[cnt].e_name);
                        free(copyenvs[cnt].e_value);
                }
                free((char *) copyenvs);
                copyenvs = (Menvir *) 0;
        }
        Nitems = 0;
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jenv(Widget parent, int isjob)
{
        Widget  ja_shell, panew, editform;
        int     jcnt;
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;

        if  (!(copyenvs = (Menvir*) malloc(sizeof(Menvir) * MAXJENVIR)))
                ABORT_NOMEM;
        Nitems = cj->h.bj_nenv;
        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)  {
                char    *namep, *valp;
                ENV_OF(cj, jcnt, namep, valp);
                copyenvs[jcnt].e_name = stracpy(namep);
                copyenvs[jcnt].e_value = stracpy(valp);
        }
        CreateEditDlg(parent, "Jenvs", &ja_shell, &panew, &editform, 5);
        CreateScrolledListDlg(editform, "Jenvlist", (XtCallbackProc) newenv, (XtCallbackProc) editenv, (XtCallbackProc) delenv, (void *) mcenv);
        filljenvlist();
        XtManageChild(listw);
        XtManageChild(editform);
        CreateActionEndDlg(ja_shell, panew, (XtCallbackProc) endjenvs, $H{xmbtq envlist dialog});
}

static void  filljredirlist()
{
        int             jcnt;
        char            *obuf;
        unsigned        buflen = 0;
        XmString        str;

        /* See how big a buffer we need */

        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)
                if  (copyredirs[jcnt].action < RD_ACT_CLOSE)  {
                        unsigned  lng = strlen(copyredirs[jcnt].un.buffer);
                        if  (lng > buflen)
                                buflen = lng;
                }

        buflen += redir_actlen + redir_stdinlen + 2 + 1 + 1 + 1 + 3;
        if  (!(obuf = malloc(buflen)))
                ABORT_NOMEM;

        XmListDeleteAllItems(listw);

        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)  {
                MredirRef       jrp = &copyredirs[jcnt];
                char    *sdname = jrp->fd > 2? "": disp_alt(jrp->fd, stdinnames);
                if  (jrp->action >= RD_ACT_CLOSE)  {
                        if  (jrp->action == RD_ACT_DUP)
                                sprintf(obuf, "%2d %-*.*s %-*.*s %d",
                                               jrp->fd, redir_stdinlen, redir_stdinlen, sdname,
                                               redir_actlen, redir_actlen, disp_alt(jrp->action, actnames),
                                               jrp->un.arg);
                        else
                                sprintf(obuf, "%2d %-*.*s %s",
                                               jrp->fd, redir_stdinlen, redir_stdinlen, sdname,
                                               disp_alt(jrp->action, actnames));
                }
                else
                        sprintf(obuf, "%2d %-*.*s %-*.*s %s",
                                       jrp->fd, redir_stdinlen, redir_stdinlen, sdname,
                                       redir_actlen, redir_actlen, disp_alt(jrp->action, actnames),
                                       jrp->un.buffer);
                str = XmStringCreateSimple(obuf);
                XmListAddItem(listw, str, 0);
                XmStringFree(str);
        }
        if  (current_select >= 0)
                XmListSelectPos(listw, current_select+1, False);
        free(obuf);
}

static void  endrediredit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt, *cp;
                int     cnt;
                XtVaGetValues(workw[WORKW_JRDFD1], XmNvalue, &txt, NULL);
                for  (cp = txt;  isspace(*cp);  cp++)
                        ;
                if  (!isdigit(*cp))  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid file descr});
                        return;
                }
                cnt = atoi(cp);
                if  (cnt >= _NFILE)  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq fd too large});
                        return;
                }
                XtFree(txt);
                work_redir.fd = (unsigned char) cnt;
                if  (work_redir.action >= RD_ACT_CLOSE)  {
                        if  (work_redir.action == RD_ACT_DUP)  {
                                XtVaGetValues(workw[WORKW_JRDFD2], XmNvalue, &txt, NULL);
                                for  (cp = txt;  isspace(*cp);  cp++)
                                        ;
                                if  (!isdigit(*cp))  {
                                        XtFree(txt);
                                        doerror(w, $EH{xmbtq invalid second fd});
                                        return;
                                }
                                cnt = atoi(cp);
                                if  (cnt >= _NFILE)  {
                                        XtFree(txt);
                                        doerror(w, $EH{xmbtq second fd too large});
                                        return;
                                }
                                XtFree(txt);
                                work_redir.un.arg = (USHORT) cnt;
                        }
                }
                else  {
                        XtVaGetValues(workw[WORKW_JARGVAL], XmNvalue, &txt, NULL);
                        if  (txt[0] == '\0')  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq null fill name});
                                return;
                        }
                        work_redir.un.buffer = stracpy(txt);
                        XtFree(txt);
                }
                if  (ismc == ISMC_NEW)  {
                        current_select++;
                        for  (cnt = Nitems - 1;  cnt >= current_select;  cnt--)
                                copyredirs[cnt+1] = copyredirs[cnt];
                        Nitems++;
                }
                else  if  (current_select < Nitems)  {
                        if  (copyredirs[current_select].action < RD_ACT_CLOSE)
                                free(copyredirs[current_select].un.buffer);
                }
                copyredirs[current_select] = work_redir;
                filljredirlist();
                ismc = ISMC_NORMAL;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  changeaction(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (n >= RD_ACT_CLOSE  &&  work_redir.action < RD_ACT_CLOSE)
                        XmTextSetString(workw[WORKW_JARGVAL], "");
                if  (n == RD_ACT_DUP)
                        XmTextSetString(workw[WORKW_JRDFD2], " 2");
                else  if  (n < RD_ACT_CLOSE  &&  work_redir.action >= RD_ACT_CLOSE)
                        XmTextSetString(workw[WORKW_JRDFD2], "");
                work_redir.action = (unsigned char) n;
        }
}

/* Timeout routine for file descrs */

static void  fd_adj(int amount, XtIntervalId *id)
{
        int     newval, ww = amount, incr = 1;
        char    *txt, *cp;
        char    nbuf[20];
        if  (amount < 0)  {
                ww = - amount;
                incr = -1;
        }
        XtVaGetValues(workw[ww], XmNvalue, &txt, NULL);
        for  (cp = txt;  isspace(*cp);  cp++)
                ;
        if  (!isdigit(*cp))  {
                XtFree(txt);
                return;
        }
        newval = atoi(cp) + incr;
        XtFree(txt);
        if  (newval < 0  || newval >= _NFILE)
                return;
        sprintf(nbuf, "%2d", newval);
        XmTextSetString(workw[ww], nbuf);
        if  (ww == WORKW_JRDFD1)
                XmTextSetString(workw[WORKW_JRDSTDIN], newval <= 2? disp_alt(newval, stdinnames): "");
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) fd_adj, INT_TO_XTPOINTER(amount));
}

/* Arrows on var val */

static void  fd_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                fd_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}

static void  rediredit(Widget w, int isnew)
{
        Widget  ae_shell, panew, formw, prevabove, prevleft, actrc;
        int     cnt;
        char    nbuf[4];

        ismc = isnew? ISMC_NEW: ISMC_NORMAL;

        CreateEditDlg(w, "rediredit", &ae_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "fileno");
        prevleft =
                workw[WORKW_JRDFD1] =
                        XtVaCreateManagedWidget("fd1",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNcolumns,                     2,
                                                XmNcursorPositionVisible,       False,
                                                XmNeditable,                    False,
                                                NULL);
        prevleft =
                workw[WORKW_JRDSTDIN] =
                        XtVaCreateManagedWidget("stdname",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNcolumns,                     redir_stdinlen,
                                                XmNcursorPositionVisible,       False,
                                                XmNeditable,                    False,
                                                NULL);

        CreateArrowPair("fd1",                          formw,
                        (Widget) 0,                     prevleft,
                        (XtCallbackProc) fd_cb,         (XtCallbackProc) fd_cb,
                        WORKW_JRDFD1,                   -WORKW_JRDFD1);

        prevabove = workw[WORKW_JRDFD1];

        actrc = XtVaCreateManagedWidget("acts",
                                        xmRowColumnWidgetClass,formw,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           prevabove,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNpacking,             XmPACK_COLUMN,
                                        XmNnumColumns,          2,
                                        XmNisHomogeneous,       True,
                                        XmNentryClass,          xmToggleButtonGadgetClass,
                                        XmNradioBehavior,       True,
                                        NULL);

        for  (cnt = 0;  cnt < actnames->numalt;  cnt++)  {
                Widget  w = XtVaCreateManagedWidget(actnames->list[cnt], xmToggleButtonGadgetClass, actrc, NULL);
                if  (work_redir.action == actnames->alt_nums[cnt])
                        XmToggleButtonGadgetSetState(w, True, False);
                XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) changeaction, INT_TO_XTPOINTER((int) actnames->alt_nums[cnt]));
        }

        prevleft = place_label_left(formw, actrc, "file");
        prevabove = prevleft =
                workw[WORKW_JARGVAL] =
                        XtVaCreateManagedWidget("fl",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   actrc,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNrightAttachment,             XmATTACH_FORM,
                                                NULL);

        prevleft = place_label_left(formw, prevabove, "dupfd");
        prevleft =
                workw[WORKW_JRDFD2] =
                        XtVaCreateManagedWidget("fd2",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   prevabove,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNcolumns,                     2,
                                                XmNcursorPositionVisible,       False,
                                                XmNeditable,                    False,
                                                NULL);

        CreateArrowPair("fd2",                          formw,
                        prevabove,                      prevleft,
                        (XtCallbackProc) fd_cb,         (XtCallbackProc) fd_cb,
                        WORKW_JRDFD2,                   -WORKW_JRDFD2);

        sprintf(nbuf, "%2d", work_redir.fd);
        XmTextSetString(workw[WORKW_JRDFD1], nbuf);
        if  (work_redir.fd <= 2)
                XmTextSetString(workw[WORKW_JRDSTDIN], disp_alt(work_redir.fd, stdinnames));
        if  (work_redir.action >= RD_ACT_CLOSE)  {
                if  (work_redir.action == RD_ACT_DUP)  {
                        sprintf(nbuf, "%2d", work_redir.un.arg);
                        XmTextSetString(workw[WORKW_JRDFD2], nbuf);
                }
        }
        else  if  (!isnew)
                XmTextSetString(workw[WORKW_JARGVAL], work_redir.un.buffer);
        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endrediredit, $H{xmbtq rediredit dialog});
}

static void  newredir(Widget w)
{
        if  (Nitems >= MAXJREDIRS)  {
                doerror(w, $EH{Too many redirections});
                return;
        }
        work_redir.fd = 0;
        work_redir.action = RD_ACT_RDWR;
        work_redir.un.buffer = "";
        rediredit(w, 1);
}

static void  editredir(Widget w)
{
        if  (current_select < 0  ||  current_select >= Nitems)  {
                doerror(w, $EH{xmbtq no redir selected});
                return;
        }
        work_redir = copyredirs[current_select];
        rediredit(w, 0);
}

static void  delredir(Widget w)
{
        int     cnt;
        if  (current_select < 0  ||  current_select >= Nitems)  {
                doerror(w, $EH{xmbtq no redir selected});
                return;
        }
        Nitems--;
        if  (copyredirs[current_select].action < RD_ACT_CLOSE)
                free(copyredirs[current_select].un.buffer);
        for  (cnt = current_select;  cnt < Nitems;  cnt++)
                copyredirs[cnt] = copyredirs[cnt+1];
        filljredirlist();
}

static void  mcredir(int mtype, int from, int to)
{
        int     jcnt;
        Mredir  save;

        save = copyredirs[from];

        if  (mtype == ISMC_MOVE)  {
                if  (from == to)
                        return;
                if  (from < to)
                        for  (jcnt = from;  jcnt < to;  jcnt++)
                                copyredirs[jcnt] = copyredirs[jcnt+1];
                else
                        for  (jcnt = from;  jcnt > to;  jcnt--)
                                copyredirs[jcnt] = copyredirs[jcnt-1];
                copyredirs[to] = save;
        }
        else  {
                if  (Nitems >= MAXJREDIRS)  {
                        doerror(listw, $EH{Too many redirections});
                        return;
                }
                for  (jcnt = Nitems;  jcnt > to;  jcnt--)
                        copyredirs[jcnt] = copyredirs[jcnt-1];
                copyredirs[to] = save;
                if  (save.action < RD_ACT_CLOSE)
                        copyredirs[to].un.buffer = stracpy(save.un.buffer);
                Nitems++;
        }
        filljredirlist();
}

static void  endjredirs(Widget w, int data)
{
        if  (data)  {
                BtjobRef        bjp = cjob->job;
                Btjob           njob;
                njob = *bjp;
                if  (!repackjob(&njob, bjp, (char *) 0, (char *) 0, Nitems, 0, 0, copyredirs, (MenvirRef) 0, (char **) 0))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = title_of(bjp);
                        doerror(w, $EH{Too many job strings});
                        return;
                }
                *bjp = njob;
                cjob->changes++;
                cjob->nosubmit++;
        }
        if  (copyredirs)  {
                int     cnt;
                for  (cnt = 0;  cnt < Nitems;  cnt++)
                        if  (copyredirs[cnt].action < RD_ACT_CLOSE)
                                free(copyredirs[cnt].un.buffer);
                free((char *) copyredirs);
                copyredirs = (Mredir *) 0;
        }
        Nitems = 0;
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jredirs(Widget parent, int isjob)
{
        Widget  ja_shell, panew, editform;
        int     jcnt;
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;

        if  (!(copyredirs = (Mredir *) malloc(sizeof(Mredir) * MAXJREDIRS)))
                ABORT_NOMEM;
        Nitems = cj->h.bj_nredirs;
        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)  {
                RedirRef        rp = REDIR_OF(cj, jcnt);
                copyredirs[jcnt].fd = rp->fd;
                if  ((copyredirs[jcnt].action = rp->action) >= RD_ACT_CLOSE)
                        copyredirs[jcnt].un.arg = rp->arg;
                else
                        copyredirs[jcnt].un.buffer = stracpy(&cj->bj_space[rp->arg]);
        }
        CreateEditDlg(parent, "Jredirs", &ja_shell, &panew, &editform, 5);
        CreateScrolledListDlg(editform, "Jredirlist", (XtCallbackProc) newredir, (XtCallbackProc) editredir, (XtCallbackProc) delredir, (void *) mcredir);
        filljredirlist();
        XtManageChild(listw);
        XtManageChild(editform);
        CreateActionEndDlg(ja_shell, panew, (XtCallbackProc) endjredirs, $H{xmbtq redirlist dialog});
}
