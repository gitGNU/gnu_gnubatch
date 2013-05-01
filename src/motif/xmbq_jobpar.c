/* xmbq_jobpar.c -- job parameters for gbch-xmq

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
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbq_ext.h"
#include "spitrouts.h"

#ifndef _NFILE
#define _NFILE  64
#endif

static  char    Filename[] = __FILE__;

#define JCCRIT_P        0
#define JCNONAV_P       1
#define JUNREAD_P       2
#define JCVAR_P         4
#define JCOP_P          (JCVAR_P + HOSTNSIZE + BTV_NAME + 1)
#define JCVAL_P         (JCOP_P + 4)
#define JACRIT_P        0
#define JANONAV_P       1
#define JAUNWRITE_P     2
#define JAVAR_P         4
#define JAOP_P          (JCVAR_P + HOSTNSIZE + BTV_NAME + 1)
#define JAFLAGS_P       (JCOP_P + 7)
#define JAVAL_P         (JAFLAGS_P + 6)

HelpaltRef      assnames, actnames, stdinnames;
int             redir_actlen, redir_stdinlen;
extern  char    *ccritmark, *cnonavail, *cunread,
                *scritmark, *snonavail, *sunwrite;

Widget  listw;
int     current_select = -1;
static  int     ismc = 0;
#define ISMC_NORMAL     0
#define ISMC_MOVE       1
#define ISMC_COPY       2
#define ISMC_NEW        3

static  char    *def_compvar = (char *) 0, *def_assvar = (char *) 0;
static  unsigned  char  def_compar = C_EQ, def_assop = BJA_ASSIGN,
                        def_compcrit = 0, def_asscrit = 0;
USHORT  def_assflags = BJA_START | BJA_REVERSE | BJA_ERROR | BJA_ABORT | BJA_OK;
static  Btcon   def_compvalue = { CON_LONG }, def_assvalue = { CON_LONG };

extern void  extract_vval(BtconRef);
extern int  val_var(const char *, const unsigned);
extern void  vval_cb(Widget, int, XmArrowButtonCallbackStruct *);

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

static void  CreateFixedListDlg(Widget formw, char *listwname, XtCallbackProc newrout, XtCallbackProc editrout, XtCallbackProc delrout, unsigned readonly, void *mcproc)
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

        if  (readonly)  {
                XtSetSensitive(neww, False);
                XtSetSensitive(editw, False);
                XtSetSensitive(delw, False);
                XtSetSensitive(movew, False);
                XtSetSensitive(copyw, False);
        }
        else  {
                XtAddCallback(neww, XmNactivateCallback, newrout, 0);
                XtAddCallback(editw, XmNactivateCallback, editrout, 0);
                XtAddCallback(delw, XmNactivateCallback, delrout, 0);
                XtAddCallback(movew, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_MOVE);
                XtAddCallback(copyw, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_COPY);
        }
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

static void  CreateScrolledListDlg(Widget formw, char *listwname, XtCallbackProc newrout, XtCallbackProc editrout, XtCallbackProc delrout, unsigned readonly, void *mcproc)
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

        if  (readonly)  {
                XtSetSensitive(neww, False);
                XtSetSensitive(editw, False);
                XtSetSensitive(delw, False);
                XtSetSensitive(movew, False);
                XtSetSensitive(copyw, False);
        }
        else  {
                XtAddCallback(neww, XmNactivateCallback, newrout, 0);
                XtAddCallback(editw, XmNactivateCallback, editrout, 0);
                XtAddCallback(delw, XmNactivateCallback, delrout, 0);
                XtAddCallback(movew, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_MOVE);
                XtAddCallback(copyw, XmNactivateCallback, (XtCallbackProc) movelist, (XtPointer) ISMC_COPY);
        }
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
                        (cjob?
                         (cjob->h.bj_jflags & BJ_EXPORT?
                          (XtPointer) gen_wvarse: (XtPointer) gen_wvars): (XtPointer) gen_wvarsa) :
                      cjob? (cjob->h.bj_jflags & BJ_EXPORT? (XtPointer) gen_rvarse: (XtPointer) gen_rvars):
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
                if  (vp->var_id.hostid)  {
                        char    nbuf[BTV_NAME+HOSTNSIZE+2];
                        sprintf(nbuf, "%s:%s", look_host(vp->var_id.hostid), vp->var_name);
                        movein(&obuf[JCVAR_P], nbuf);
                        if  (jcp->bjc_iscrit & CCRIT_NORUN)
                                movein(&obuf[JCCRIT_P], ccritmark);
                        if  (jcp->bjc_iscrit & CCRIT_NONAVAIL)
                                movein(&obuf[JCNONAV_P], cnonavail);
                        if  (jcp->bjc_iscrit & CCRIT_NOPERM)
                                movein(&obuf[JUNREAD_P], cunread);
                }
                else
                        movein(&obuf[JCVAR_P], vp->var_name);
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
                if  (vp->var_id.hostid)  {
                        char    nbuf[BTV_NAME+HOSTNSIZE+2];
                        sprintf(nbuf, "%s:%s", look_host(vp->var_id.hostid), vp->var_name);
                        movein(&obuf[JAVAR_P], nbuf);
                        if  (jap->bja_iscrit & ACRIT_NORUN)
                                movein(&obuf[JACRIT_P], scritmark);
                        if  (jap->bja_iscrit & ACRIT_NONAVAIL)
                                movein(&obuf[JANONAV_P], snonavail);
                        if  (jap->bja_iscrit & ACRIT_NONAVAIL)
                                movein(&obuf[JAUNWRITE_P], sunwrite);
                }
                else
                        movein(&obuf[JAVAR_P], vp->var_name);
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
                if  (!cjob)  {
                        if  (def_compvar)
                                free(def_compvar);
                        def_compvar = stracpy(txt);
                }
                XtFree(txt);
                work_cond.bjc_varind = vv_ptrs[varind].place;
                extract_vval(&work_cond.bjc_value);
                if  (cjob)  {
                        copyconds[current_select] = work_cond;
                        filljclist();
                }
                else  {
                        def_compar = work_cond.bjc_compar;
                        def_compvalue = work_cond.bjc_value;
                        def_compcrit = work_cond.bjc_iscrit;
                }
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
                if  (!cjob)  {
                        if  (def_assvar)
                                free(def_assvar);
                        def_assvar = stracpy(txt);
                }
                XtFree(txt);
                work_ass.bja_varind = vv_ptrs[varind].place;
                extract_vval(&work_ass.bja_con);
                if  (cjob)  {
                        copyasses[current_select] = work_ass;
                        filljalist();
                }
                else  {
                        def_assop = work_ass.bja_op;
                        def_assvalue = work_ass.bja_con;
                        def_asscrit = work_ass.bja_iscrit;
                        def_assflags = work_ass.bja_flags;
                }
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
        for  (cnt = 0;  cnt < 6;  cnt++)  {
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
                        condrc,                         prevleft,
                        (XtCallbackProc) vval_cb,       (XtCallbackProc) vval_cb,
                        1,                              -1);

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
                        assrc,                          prevleft,
                        (XtCallbackProc) vval_cb,       (XtCallbackProc) vval_cb,
                        1,                              -1);

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

static void  newcond(Widget w)
{
        int     jcnt;
        vhash_t varind;

        for  (jcnt = 0;  jcnt < MAXCVARS;  jcnt++)
                if  (copyconds[jcnt].bjc_compar == C_UNUSED)
                        goto  gotit;
        doerror(w, $EH{xmbtq too many conds});
        return;
 gotit:
        current_select = jcnt;
        varind = val_var(def_compvar, BTM_READ);
        work_cond.bjc_varind = varind < 0? varind: vv_ptrs[varind].place;
        work_cond.bjc_compar = def_compar;
        work_cond.bjc_value = def_compvalue;
        work_cond.bjc_iscrit = def_compcrit;
        condedit(w, 1);
}

static void  newass(Widget w)
{
        int     jcnt;
        vhash_t varind;

        for  (jcnt = 0;  jcnt < MAXSEVARS;  jcnt++)
                if  (copyasses[jcnt].bja_op == BJA_NONE)
                        goto  gotit;
        doerror(w, $EH{xmbtq too many asses});
        return;
 gotit:
        current_select = jcnt;
        varind = val_var(def_assvar, BTM_WRITE);
        work_ass.bja_varind = varind < 0? varind: vv_ptrs[varind].place;
        work_ass.bja_op = def_assop;
        work_ass.bja_con = def_assvalue;
        work_ass.bja_flags = def_assflags;
        work_ass.bja_iscrit = def_asscrit;
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
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                BLOCK_COPY(bjp->h.bj_conds, copyconds, MAXCVARS * sizeof(Jcond));
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
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
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                BLOCK_COPY(bjp->h.bj_asses, copyasses, MAXSEVARS * sizeof(Jass));
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
        }
        if  (copyasses)  {
                free((char *) copyasses);
                copyasses = (Jass *) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jconds(Widget parent)
{
        Widget  jc_shell, panew, editform;
        int     jcnt, hadj;
        BtjobRef        cj = getselectedjob(BTM_READ);
        unsigned        readonly;
        if  (!cj)
                return;
        cjob = cj;
        if  (!(copyconds = (Jcond *) malloc(sizeof(Jcond) * MAXCVARS)))
                ABORT_NOMEM;
        for  (jcnt = hadj = 0;  jcnt < MAXCVARS;  jcnt++)  {
                if  (cj->h.bj_conds[jcnt].bjc_compar == C_UNUSED)
                        break;
                copyconds[hadj++] = cj->h.bj_conds[jcnt];
        }
        for  (;  hadj < MAXCVARS;  hadj++)
                copyconds[hadj].bjc_compar = C_UNUSED;
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        CreateEditDlg(parent, "Jcond", &jc_shell, &panew, &editform, 5);
        CreateFixedListDlg(editform, "Jclist", (XtCallbackProc) newcond, (XtCallbackProc) editcond, (XtCallbackProc) delcond, readonly, (void *) mccond);
        filljclist();
        XtManageChild(editform);
        CreateActionEndDlg(jc_shell, panew, (XtCallbackProc) endjconds, $H{xmbtq condlist dialog});
}

void  cb_jass(Widget parent)
{
        Widget  ja_shell, panew, editform;
        int     jcnt, hadj;
        BtjobRef        cj = getselectedjob(BTM_READ);
        unsigned        readonly;
        if  (!cj)
                return;
        cjob = cj;
        if  (!(copyasses = (Jass *) malloc(sizeof(Jass) * MAXSEVARS)))
                ABORT_NOMEM;
        for  (jcnt = hadj = 0;  jcnt < MAXSEVARS;  jcnt++)  {
                if  (cj->h.bj_asses[jcnt].bja_op == BJA_NONE)
                        break;
                copyasses[hadj++] = cj->h.bj_asses[jcnt];
        }
        for  (;  hadj < MAXSEVARS;  hadj++)
                copyasses[hadj].bja_op = BJA_NONE;
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        CreateEditDlg(parent, "Jass", &ja_shell, &panew, &editform, 5);
        CreateFixedListDlg(editform, "Jalist", (XtCallbackProc) newass, (XtCallbackProc) editass, (XtCallbackProc) delass, readonly, (void *) mcass);
        filljalist();
        XtManageChild(editform);
        CreateActionEndDlg(ja_shell, panew, (XtCallbackProc) endjasses, $H{xmbtq asslist dialog});
}

void  cb_defcond(Widget parent)
{
        vhash_t varind;
        cjob = (BtjobRef) 0;
        varind = val_var(def_compvar, BTM_READ);
        work_cond.bjc_varind = varind < 0? varind: vv_ptrs[varind].place;
        work_cond.bjc_compar = def_compar;
        work_cond.bjc_value = def_compvalue;
        work_cond.bjc_iscrit = def_compcrit;
        condedit(parent, 0);
}

void  cb_defass(Widget parent)
{
        vhash_t varind;
        cjob = (BtjobRef) 0;
        varind = val_var(def_assvar, BTM_WRITE);
        work_ass.bja_varind = varind < 0? varind: vv_ptrs[varind].place;
        work_ass.bja_op = def_assop;
        work_ass.bja_con = def_assvalue;
        work_ass.bja_iscrit = def_asscrit;
        work_ass.bja_flags = def_assflags;
        assedit(parent, 0);
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
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                if  (!repackjob(bjp, cjob, (char *) 0, (char *) 0, 0, 0, Nitems, (MredirRef) 0, (MenvirRef) 0, copyargs))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(w, $EH{Too many job strings});
                        freexbuf(xindx);
                        return;
                }
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
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

void  cb_jargs(Widget parent)
{
        Widget  ja_shell, panew, editform;
        int     jcnt;
        BtjobRef        cj = getselectedjob(BTM_READ);
        unsigned        readonly;
        if  (!cj)
                return;
        cjob = cj;
        if  (!(copyargs = (char **) malloc(sizeof(char *) * MAXJARGS)))
                ABORT_NOMEM;
        Nitems = cj->h.bj_nargs;
        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)
                copyargs[jcnt] = stracpy(ARG_OF(cj, jcnt));
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        CreateEditDlg(parent, "Jargs", &ja_shell, &panew, &editform, 5);
        CreateScrolledListDlg(editform, "Jarglist", (XtCallbackProc) newarg, (XtCallbackProc) editarg, (XtCallbackProc) delarg, readonly, (void *) mcarg);
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
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                if  (!repackjob(bjp, cjob, (char *) 0, (char *) 0, 0, Nitems, 0, (MredirRef) 0, copyenvs, (char **) 0))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(w, $EH{Too many job strings});
                        freexbuf(xindx);
                        return;
                }
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
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

void  cb_jenv(Widget parent)
{
        Widget  ja_shell, panew, editform;
        int     jcnt;
        BtjobRef        cj = getselectedjob(BTM_READ);
        unsigned        readonly;
        if  (!cj)
                return;
        cjob = cj;
        if  (!(copyenvs = (Menvir*) malloc(sizeof(Menvir) * MAXJENVIR)))
                ABORT_NOMEM;
        Nitems = cj->h.bj_nenv;
        for  (jcnt = 0;  jcnt < Nitems;  jcnt++)  {
                char    *namep, *valp;
                ENV_OF(cj, jcnt, namep, valp);
                copyenvs[jcnt].e_name = stracpy(namep);
                copyenvs[jcnt].e_value = stracpy(valp);
        }
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        CreateEditDlg(parent, "Jenvs", &ja_shell, &panew, &editform, 5);
        CreateScrolledListDlg(editform, "Jenvlist", (XtCallbackProc) newenv, (XtCallbackProc) editenv, (XtCallbackProc) delenv, readonly, (void *) mcenv);
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
        put_textbox_int(WW(ww), newval, 2);
        if  (ww == WORKW_JRDFD1)
                XmTextSetString(workw[WORKW_JRDSTDIN], newval <= 2? disp_alt(newval, stdinnames): "");
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) fd_adj, INT_TO_XTPOINTER(amount));
}

/* Arrows on fd */

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
                               (Widget) 0,                      prevleft,
                               (XtCallbackProc) fd_cb,          (XtCallbackProc) fd_cb,
                               WORKW_JRDFD1,                    -WORKW_JRDFD1);

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
                               prevabove,                       prevleft,
                               (XtCallbackProc) fd_cb,          (XtCallbackProc) fd_cb,
                               WORKW_JRDFD2,                    -WORKW_JRDFD2);

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
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                if  (!repackjob(bjp, cjob, (char *) 0, (char *) 0, Nitems, 0, 0, copyredirs, (MenvirRef) 0, (char **) 0))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(w, $EH{Too many job strings});
                        freexbuf(xindx);
                        return;
                }
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
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

void  cb_jredirs(Widget parent)
{
        Widget  ja_shell, panew, editform;
        int     jcnt;
        BtjobRef        cj = getselectedjob(BTM_READ);
        unsigned        readonly;
        if  (!cj)
                return;
        cjob = cj;
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
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        CreateEditDlg(parent, "Jredirs", &ja_shell, &panew, &editform, 5);
        CreateScrolledListDlg(editform, "Jredirlist", (XtCallbackProc) newredir, (XtCallbackProc) editredir, (XtCallbackProc) delredir, readonly, (void *) mcredir);
        filljredirlist();
        XtManageChild(listw);
        XtManageChild(editform);
        CreateActionEndDlg(ja_shell, panew, (XtCallbackProc) endjredirs, $H{xmbtq redirlist dialog});
}

static  char    *Last_unqueue_dir;
static  char    *udprog;

static void  endjunqueue(Widget w, int data)
{
        if  (data)  {
                int     copyonly;
                char    *dtxt, *jftxt, *cftxt;
                PIDTYPE pid;
                struct  stat    sbuf;
                const char      *arg0, **ap, *argbuf[8];

                if  (!udprog)
                        udprog = envprocess(DUMPJOB);
                copyonly = XmToggleButtonGadgetGetState(workw[WORKW_JCONLY]);
                XtVaGetValues(workw[WORKW_DIRTXTW], XmNvalue, &dtxt, NULL);
                if  (dtxt[0] == '\0')  {
                        XtFree(dtxt);
                        doerror(w, $EH{xmbtq unqueue no dir});
                        return;
                }
                if  (dtxt[0] != '/')  {
                        disp_str = dtxt;
                        doerror(w, $EH{xmbtq unqueue dir not abs});
                        XtFree(dtxt);
                        return;
                }
                if  (stat(dtxt, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                        disp_str = dtxt;
                        doerror(w, $EH{xmbtq unq not dir});
                        XtFree(dtxt);
                        return;
                }
                free(Last_unqueue_dir);
                Last_unqueue_dir = stracpy(dtxt);
                XtFree(dtxt);   /* According to spec cannot mix XtFree and free */
                XtVaGetValues(workw[WORKW_JCFW], XmNvalue, &cftxt, NULL);
                if  (cftxt[0] == '\0')  {
                        XtFree(cftxt);
                        doerror(w, $EH{xmbtq no cmd file name});
                        return;
                }
                XtVaGetValues(workw[WORKW_JJFW], XmNvalue, &jftxt, NULL);
                if  (jftxt[0] == '\0')  {
                        XtFree(jftxt);
                        XtFree(cftxt);
                        doerror(w, $EH{xmbtq no job file name});
                        return;
                }
                XtDestroyWidget(GetTopShell(w));
                if  ((pid = fork()))  {
                        int     status;

                        XtFree(jftxt);
                        XtFree(cftxt);

                        if  (pid < 0)  {
                                doerror(jwid, $EH{xmbtq unqueue fork failed});
                                return;
                        }

#ifdef  HAVE_WAITPID
                        while  (waitpid(pid, &status, 0) < 0)
                                ;
#else
                        while  (wait(&status) != pid)
                                ;
#endif
                        if  (status == 0)       /* All ok */
                                return;
                        if  (status & 0xff)  {
                                disp_arg[0] = status & 0x7f;
                                doerror(jwid, status & 0x80? $EH{xmbtq unqueue crashed}: $EH{xmbtq unqueue terminated});
                                return;
                        }
                        status = (status >> 8) & 0xff;
                        disp_arg[0] = cjob->h.bj_job;
                        disp_str = qtitle_of(cjob);
                        switch  (status)  {
                        default:
                                disp_arg[1] = status;
                                doerror(jwid, $EH{xmbtq unqueue misc error});
                                return;
                        case  E_SETUP:
                                doerror(jwid, $EH{xmbtq no unq process});
                                return;
                        case  E_JDFNFND:
                                doerror(jwid, $EH{xmbtq unq dir not found});
                                return;
                        case  E_JDNOCHDIR:
                                doerror(jwid, $EH{xmbtq cannot chdir});
                                return;
                        case  E_JDFNOCR:
                                doerror(jwid, $EH{xmbtq cannot create cmdfile});
                                return;
                        case  E_JDJNFND:
                                doerror(jwid, $EH{xmbtq unq job not found});
                                return;
                        case  E_CANTDEL:
                                doerror(jwid, $EH{xmbtq unq cannot del});
                                return;
                        }
                }

                /* Child process */

                setuid(Realuid);
                chdir(Curr_pwd);        /* So it picks up the right config file */
                ap = argbuf;
                *ap++ = (arg0 = strrchr(udprog, '/'))? arg0 + 1: udprog;
                if  (copyonly)
                        *ap++ = "-n";
                *ap++ = JOB_NUMBER(cjob);
                *ap++ = Last_unqueue_dir;
                *ap++ = cftxt;
                *ap++ = jftxt;
                *ap++ = (char *) 0;
                execv(udprog, (char **) argbuf);
                exit(E_SETUP);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_unqueue(Widget parent)
{
        Widget  ju_shell, panew, mainform, prevabove, prevleft;
        BtjobRef        jp = getselectedjob(BTM_READ|BTM_DELETE);
        char    nbuf[40];

        if  (!jp)
                return;

        cjob = jp;

        if  (!Last_unqueue_dir)
                Last_unqueue_dir = stracpy(Curr_pwd);

        prevabove = CreateJeditDlg(parent, "Junqueue", &ju_shell, &panew, &mainform);
        prevabove = CreateDselPane(mainform, prevabove, Last_unqueue_dir);

        prevabove = workw[WORKW_JCONLY] = XtVaCreateManagedWidget("copyonly",
                                                                  xmToggleButtonGadgetClass,    mainform,
                                                                  XmNtopAttachment,             XmATTACH_WIDGET,
                                                                  XmNtopWidget,                 prevabove,
                                                                  XmNleftAttachment,            XmATTACH_FORM,
                                                                  NULL);

        prevleft = place_label_left(mainform, prevabove, "Cmdfile");
        workw[WORKW_JCFW] =
                XtVaCreateManagedWidget("cfile",
                                        xmTextFieldWidgetClass,         mainform,
                                        XmNcursorPositionVisible,       False,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        NULL);
        prevabove = workw[WORKW_JCFW];
        sprintf(nbuf, "C%ld", (long) jp->h.bj_job);
        XmTextSetString(prevabove, nbuf);

        prevleft = place_label_left(mainform, prevabove, "Jobfile");

        workw[WORKW_JJFW] =
                XtVaCreateManagedWidget("jfile",
                                        xmTextFieldWidgetClass,         mainform,
                                        XmNcursorPositionVisible,       False,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        NULL);

        sprintf(nbuf, "J%ld", (long) jp->h.bj_job);
        XmTextSetString(workw[WORKW_JJFW], nbuf);
        XtManageChild(mainform);
        CreateActionEndDlg(ju_shell, panew, (XtCallbackProc) endjunqueue, $H{xmbtq unqueue dialog});
}

static void  endjfreeze(Widget w, int data)
{
        if  (data)  {
                char    *arg0;
                PIDTYPE pid;
                char    abuf[3];

                if  (!udprog)
                        udprog = envprocess(DUMPJOB);

                abuf[0] = '-';
                abuf[2] = '\0';
                abuf[1] = XmToggleButtonGadgetGetState(workw[WORKW_CANC])? 'C': 'N';
                if  (XmToggleButtonGadgetGetState(workw[WORKW_VERBOSE]))
                        abuf[1] = tolower(abuf[1]);

                XtDestroyWidget(GetTopShell(w));
                if  ((pid = fork()))  {
                        int     status;

                        if  (pid < 0)  {
                                doerror(jwid, $EH{xmbtq unqueue fork failed});
                                return;
                        }

#ifdef  HAVE_WAITPID
                        while  (waitpid(pid, &status, 0) < 0)
                                ;
#else
                        while  (wait(&status) != pid)
                                ;
#endif
                        if  (status == 0)       /* All ok */
                                return;
                        if  (status & 0xff)  {
                                disp_arg[0] = status & 0x7f;
                                doerror(jwid, status & 0x80? $EH{xmbtq unqueue crashed}: $EH{xmbtq unqueue terminated});
                                return;
                        }
                        status = (status >> 8) & 0xff;
                        disp_arg[0] = cjob->h.bj_job;
                        disp_str = qtitle_of(cjob);
                        doerror(jwid, $EH{xmbtq unq cannot create config file});
                        return;
                }

                /* Child process */

                setuid(Realuid);
                chdir(Curr_pwd);        /* So it picks up the right config file */
                if  (!(arg0 = strrchr(udprog, '/')))
                        arg0 = udprog;
                else
                        arg0++;
                execl(udprog, arg0, abuf, ismc? "~": Curr_pwd, JOB_NUMBER(cjob), (char *) 0);
                exit(E_SETUP);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_freeze(Widget parent, const int ishome)
{
        Widget  jf_shell, panew, mainform, prevabove, verbrc, cancrc, other;
        BtjobRef        jp = getselectedjob(BTM_READ|BTM_DELETE);

        if  (!jp)
                return;

        cjob = jp;
        ismc = ishome;

        prevabove = CreateJeditDlg(parent, "Jfreeze", &jf_shell, &panew, &mainform);
        verbrc = XtVaCreateManagedWidget("verbrc",
                                         xmRowColumnWidgetClass,        mainform,
                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                         XmNtopWidget,                  prevabove,
                                         XmNleftAttachment,             XmATTACH_FORM,
                                         XmNpacking,                    XmPACK_COLUMN,
                                         XmNnumColumns,                 1,
                                         XmNisHomogeneous,              True,
                                         XmNentryClass,                 xmToggleButtonGadgetClass,
                                         XmNradioBehavior,              True,
                                         NULL);

        other = XtVaCreateManagedWidget("noverbose", xmToggleButtonGadgetClass, verbrc, NULL);
        workw[WORKW_VERBOSE] = XtVaCreateManagedWidget("verbose", xmToggleButtonGadgetClass, verbrc, NULL);
        XmToggleButtonGadgetSetState(other, True, False);

        cancrc = XtVaCreateManagedWidget("canc",
                                         xmRowColumnWidgetClass,        mainform,
                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                         XmNtopWidget,                  verbrc,
                                         XmNleftAttachment,             XmATTACH_FORM,
                                         XmNpacking,                    XmPACK_COLUMN,
                                         XmNnumColumns,                 1,
                                         XmNisHomogeneous,              True,
                                         XmNentryClass,                 xmToggleButtonGadgetClass,
                                         XmNradioBehavior,              True,
                                         NULL);

        other = XtVaCreateManagedWidget("normal", xmToggleButtonGadgetClass, cancrc, NULL);
        workw[WORKW_CANC] = XtVaCreateManagedWidget("cancelled", xmToggleButtonGadgetClass, cancrc, NULL);
        XmToggleButtonGadgetSetState(jp->h.bj_progress == BJP_NONE? other: workw[WORKW_CANC], True, False);
        XtManageChild(mainform);
        CreateActionEndDlg(jf_shell, panew, (XtCallbackProc) endjfreeze, $H{xmbtq copy to curr dir dialog}+ishome);
}

static int  tsystem(const char *cmd)
{
        PIDTYPE pid;
        if  ((pid = fork()) != 0)  {
                int     status;
                if  (pid < 0)
                        return  E_NOFORK;
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                return  status & 255? E_SIGNAL: status >> 8;
        }
        setuid(Realuid);
        exit(system(cmd));
        return  0;              /* Shut up fussy compilers */
}

static void  endfseld(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        if  (data)  {
                int     ec;
                char    *fname;
                struct  stat    sbuf;
                if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &fname))
                        return;
                disp_str = fname;
                if  (stat(fname, &sbuf) < 0)  {
                        doerror(w, $EH{xmbtq file not found});
                        XtFree(fname);
                        return;
                }
                if  ((sbuf.st_mode & S_IFMT) != S_IFREG)  {
                        doerror(w, $EH{xmbtq file not regular});
                        XtFree(fname);
                        return;
                }
                if  ((sbuf.st_mode & 0111) == 0)  {
                        char    *cmdbuf, *sp;
                        if  (!(cmdbuf = malloc(strlen(fname) + 30)))
                                ABORT_NOMEM;
                        if  ((sp = strrchr(fname, '/'))  &&  sp != fname)  {
                                *sp = '\0';
                                sprintf(cmdbuf, "cd %s;BTR=-C " BTR_PROGRAM " ./%s", fname, sp+1); /* FIXME */
                                *sp = '/';
                        }
                        else
                                sprintf(cmdbuf, "BTR=-C " BTR_PROGRAM " %s", fname); /* FIXME */
                        if  ((ec = tsystem(cmdbuf)) != 0)  {
                                disp_arg[0] = ec;
                                doerror(w, $EH{xmbtq could not submit job});
                        }
                        free(cmdbuf);
                }
                else  {
                        FILE    *fp = fopen(fname, "r");
                        char    *sp;
                        char    inbuf[200], cbuf[230];
                        if  (!fp)  {
                                doerror(w, $EH{xmbtq cannot open file});
                                XtFree(fname);
                                return;
                        }
                        if  (!fgets(inbuf, sizeof(inbuf), fp)  ||  strlen(inbuf) == sizeof(inbuf) - 1)  {
                                doerror(w, $EH{xmbtq bad file type});
                                XtFree(fname);
                                fclose(fp);
                                return;
                        }
                        fclose(fp);
                        if  (!(isalpha(inbuf[0]) || inbuf[0] == '_'))  {
                                doerror(w, $EH{xmbtq bad file type});
                                XtFree(fname);
                                return;
                        }
                        if  (!strchr(inbuf, '='))  {
                                doerror(w, $EH{xmbtq bad file type});
                                XtFree(fname);
                                return;
                        }
                        if  ((sp = strrchr(fname, '/'))  &&  sp != fname)  {
                                *sp = '\0';
                                sprintf(cbuf, "cd %s;./%s", fname, sp+1);
                                *sp = '/';
                        }
                        else
                                strcpy(cbuf, fname);
                        if  ((ec = tsystem(cbuf)) != 0)  {
                                disp_arg[0] = ec;
                                doerror(w, $EH{xmbtq submit job error});
                        }
                }
                XtFree(fname);
        }
        XtDestroyWidget(w);
}

void  cb_jcreate()
{
        Widget  fseld;

        if  (!(mypriv->btu_priv & BTM_CREATE))  {
                doerror(jwid, $EH{xmbtq no create permission});
                return;
        }

        if  (!Last_unqueue_dir)
                Last_unqueue_dir = stracpy(Curr_pwd);
        if  (chdir(Last_unqueue_dir) < 0)
                chdir(Curr_pwd);
        fseld = XmCreateFileSelectionDialog(jwid, "fselb", NULL, 0);
        XtAddCallback(fseld, XmNcancelCallback, (XtCallbackProc) endfseld, (XtPointer) 0);
        XtAddCallback(fseld, XmNokCallback, (XtCallbackProc) endfseld, (XtPointer) 1);
        XtAddCallback(fseld, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtq sel file dialog});
        chdir(spdir);
        XtManageChild(fseld);
}
