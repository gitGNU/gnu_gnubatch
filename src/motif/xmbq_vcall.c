/* xmbq_vcall.c -- variable handling for gbch-xmq

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <X11/cursorfont.h>
#include <Xm/DialogS.h>
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
#ifdef HAVE_XM_SPINB_H
#include <Xm/SpinB.h>
#endif
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
#include "optflags.h"

static  char    Filename[] = __FILE__;

int             Const_val;              /* Value for constant in arith */
BtvarRef        cvar;
ULONG           Saveseq;

/* Send var-type message to scheduler */

void  qwvmsg(unsigned code, BtvarRef vp, ULONG Sseq)
{
        Oreq.sh_params.mcode = code;
        if  (vp)
                Oreq.sh_un.sh_var = *vp;
        Oreq.sh_un.sh_var.var_sequence = Sseq;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), 0) < 0)
                msg_error();
}

/* Display var-type error message */

void  qdoverror(unsigned retc, BtvarRef vp)
{
        switch  (retc  & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                doerror(vwid, $EH{Unexpected sched message});
                return;
        case  VAR_REPLY:
                disp_str = vp->var_name;
                doerror(vwid, (int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler var errors}));
                return;
        case  NET_REPLY:
                disp_str = vp->var_name;
                doerror(vwid, (int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler net errors}));
                return;
        }
}

static Widget  CreateVtitle(Widget formw, BtvarRef vp)
{
        Widget          titw1, titw2;
        int             lng;
        const   char    *nbuf = VAR_NAME(vp);

        titw1 = XtVaCreateManagedWidget("varnotitle",
                                        xmLabelWidgetClass,     formw,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNborderWidth,         0,
                                        NULL);
        lng = strlen(nbuf);

        titw2 = XtVaCreateManagedWidget("var",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     lng,
                                        XmNcursorPositionVisible,       False,
                                        XmNeditable,                    False,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  titw1,
                                        NULL);
        XmTextSetString(titw2, (char *) nbuf);
        return  titw2;
}

/* Initial part of var dialog.
   Return widget of tallest part of title.  */

static Widget CreateVeditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres)
{
        CreateEditDlg(parent, dlgname, dlgres, paneres, formres, 3);
        return  CreateVtitle(*formres, cvar);
}

#ifndef HAVE_XM_SPINB_H
/* Timeout routine for const val */
static void  cval_adj(int amount, XtIntervalId *id)
{
        int     newval;
        char    *txt, *cp;
        XtVaGetValues(workw[WORKW_CONVAL], XmNvalue, &txt, NULL);
        for  (cp = txt;  isspace(*cp);  cp++)
                ;
        if  (!isdigit(*cp)  &&  *cp != '-')  {
                XtFree(txt);
                return;
        }
        newval = atoi(cp) + amount;
        XtFree(txt);
        put_textbox_int(WW(WORKW_CONVAL), newval, 9);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) cval_adj, (XtPointer) amount);
}

/* Arrows on const val */

static void  cval_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                cval_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}
#endif

static void  endcval(Widget w, int data)
{
#ifdef  HAVE_XM_SPINB_H
        if  (data)
                Const_val = get_spinbox_int(WW(WORKW_CONVAL));
#else
        if  (data)  {
                char    *txt, *cp;
                XtVaGetValues(workw[WORKW_CONVAL], XmNvalue, &txt, NULL);
                for  (cp = txt;  isspace(*cp);  cp++)
                        ;
                if  (!isdigit(*cp)  &&  *cp != '-')  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid const var});
                        return;
                }
                Const_val = atoi(cp);
                XtFree(txt);
        }
#endif
        XtDestroyWidget(GetTopShell(w));
}

void  cb_setconst(Widget parent)
{
        Widget  cv_shell, panew, mainform, prevleft;

        CreateEditDlg(parent, "setconst", &cv_shell, &panew, &mainform, 3);

        prevleft = place_label_topleft(mainform, "constval");
#ifdef HAVE_XM_SPINB_H
        prevleft = XtVaCreateManagedWidget("cval",
                                           xmSpinBoxWidgetClass,        mainform,
                                           XmNtopAttachment,            XmATTACH_FORM,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               prevleft,
                                           NULL);

        workw[WORKW_CONVAL] = XtVaCreateManagedWidget("cval",
                                                      xmTextFieldWidgetClass,           prevleft,
                                                      XmNcolumns,                       9,
                                                      XmNcursorPositionVisible,         False,
                                                      XmNspinBoxChildType,              XmNUMERIC,
                                                      XmNmaximumValue,                  99999999,
                                                      XmNminimumValue,                  -99999999,
#ifdef  BROKEN_SPINBOX
                                                      XmNpositionType,                  XmPOSITION_INDEX,
                                                      XmNposition,                      Const_val+99999999,
#else
                                                      XmNposition,                      Const_val,
#endif
                                                      NULL);
#else
        prevleft = workw[WORKW_CONVAL] = XtVaCreateManagedWidget("cval",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    9,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_FORM,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

        CreateArrowPair("cval", mainform, (Widget) 0, prevleft, (XtCallbackProc) cval_cb, (XtCallbackProc) cval_cb, 1, -1);
        put_textbox_int(WW(WORKW_CONVAL), Const_val, 9);

#endif /* ! HAVE_XM_SPINB_H */

        XtManageChild(mainform);
        CreateActionEndDlg(cv_shell, panew, (XtCallbackProc) endcval, $H{xmbtq cval dialog});
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

static void  endvass(Widget w, int data)
{
        if  (data)  {
                unsigned        retc;
                Btcon           newval;
                extract_vval(&newval);
                Oreq.sh_un.sh_var = *cvar;
                Oreq.sh_un.sh_var.var_value = newval;
                qwvmsg(V_ASSIGN, (BtvarRef) 0, Saveseq);
                if  ((retc = readreply()) != V_OK)
                        qdoverror(retc, &Oreq.sh_un.sh_var);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_vass(Widget parent)
{
        Widget  va_shell, panew, formw, prevabove, prevleft;
        BtvarRef  cv = getselectedvar(BTM_READ|BTM_WRITE);
        char    nbuf[BTC_VALUE+3];

        if  (!cv)
                return;
        cvar = cv;
        Saveseq = cv->var_sequence;

        prevabove = CreateVeditDlg(parent, "vass", &va_shell, &panew, &formw);
        prevleft = place_label_left(formw, prevabove, "value");
        prevleft =
                workw[WORKW_VARVAL] =
                        XtVaCreateManagedWidget("vval",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNcolumns,                     BTC_VALUE+2,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   prevabove,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                NULL);

        CreateArrowPair("vval",                 formw,
                        prevabove,              prevleft,
                        (XtCallbackProc) vval_cb,(XtCallbackProc) vval_cb,
                        1,                      -1);

        if  (cv->var_value.const_type == CON_STRING)
                sprintf(nbuf, "\"%s\"", cv->var_value.con_un.con_string);
        else
                sprintf(nbuf, "%ld", (long) cv->var_value.con_un.con_long);
        XmTextSetString(workw[WORKW_VARVAL], nbuf);
        XtManageChild(formw);
        CreateActionEndDlg(va_shell, panew, (XtCallbackProc) endvass, $H{xmbtq vass dialog});
}

static void  endvcomm(Widget w, int data)
{
        if  (data)  {
                unsigned        retc;
                char            *txt, *cp;
                int             lng;
                XtVaGetValues(workw[WORKW_VARCOMM], XmNvalue, &txt, NULL);
                for  (cp = txt;  isspace(*cp);  cp++)
                        ;
                lng = strlen(cp);
                if  (lng > BTV_COMMENT)
                        lng = BTV_COMMENT;
                Oreq.sh_un.sh_var = *cvar;
                strncpy(Oreq.sh_un.sh_var.var_comment, cp, (unsigned) lng);
                Oreq.sh_un.sh_var.var_comment[lng] = '\0';
                XtFree(txt);
                qwvmsg(V_CHCOMM, (BtvarRef) 0, Saveseq);
                if  ((retc = readreply()) != V_OK)
                        qdoverror(retc, &Oreq.sh_un.sh_var);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_vcomm(Widget parent)
{
        Widget  vc_shell, panew, formw, prevabove, prevleft;
        BtvarRef  cv = getselectedvar(BTM_READ|BTM_WRITE);

        if  (!cv)
                return;
        cvar = cv;
        Saveseq = cv->var_sequence;

        prevabove = CreateVeditDlg(parent, "vcomm", &vc_shell, &panew, &formw);
        prevleft = place_label_left(formw, prevabove, "comment");
        workw[WORKW_VARCOMM] =
                XtVaCreateManagedWidget("vcomm",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     BTV_COMMENT,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);

        XmTextSetString(workw[WORKW_VARCOMM], cv->var_comment);
        XtManageChild(formw);
        CreateActionEndDlg(vc_shell, panew, (XtCallbackProc) endvcomm, $H{xmbtq vcomment dialog});
}

void  cb_arith(Widget parent, int op)
{
        BtvarRef        cv = getselectedvar(BTM_READ|BTM_WRITE);
        BtconRef        cvalue;
        LONG            newvalue;
        unsigned        retc;

        if  (!cv)
                return;
        Saveseq = cv->var_sequence;
        cvalue = &cv->var_value;
        if  (cvalue->const_type != CON_LONG)  {
                doerror(vwid, $EH{xmbtq value not arith});
                return;
        }
        newvalue = cvalue->con_un.con_long;
        switch  (op)  {
        default:
        case  '+':
                newvalue += Const_val;
                break;
        case  '-':
                newvalue -= Const_val;
                break;
        case  '*':
                newvalue *= Const_val;
                break;
        case  '/':
                if  (Const_val == 0L)  {
                        doerror(vwid, $EH{xmbtq divide by zero});
                        return;
                }
                newvalue /= Const_val;
                break;
        case  '%':
                if  (Const_val == 0L)  {
                        doerror(vwid, $EH{xmbtq divide by zero});
                        return;
                }
                newvalue %= Const_val;
                break;
        }
        Oreq.sh_un.sh_var = *cv;
        Oreq.sh_un.sh_var.var_value.con_un.con_long = newvalue;
        qwvmsg(V_ASSIGN, (BtvarRef) 0, Saveseq);
        if  ((retc = readreply()) != V_OK)
                qdoverror(retc, &Oreq.sh_un.sh_var);
}

void  cb_vexport(Widget parent, int data)
{
        BtvarRef        cv = getselectedvar(BTM_DELETE);
        unsigned        retc;

        if  (!cv)
                return;

        Saveseq = cv->var_sequence;
        Oreq.sh_un.sh_var = *cv;
        if  (data)  {
                if  (Oreq.sh_un.sh_var.var_flags & data)
                        return;
                Oreq.sh_un.sh_var.var_flags |= data;
        }
        else  {                 /* Turn off cluster first, then export */
                if  (!(Oreq.sh_un.sh_var.var_flags & (VF_EXPORT|VF_CLUSTER)))
                        return;
                if  (Oreq.sh_un.sh_var.var_flags & VF_CLUSTER)
                        Oreq.sh_un.sh_var.var_flags &= ~VF_CLUSTER;
                else
                        Oreq.sh_un.sh_var.var_flags &= ~VF_EXPORT;
        }
        qwvmsg(V_CHFLAGS, (BtvarRef) 0, Saveseq);
        if  ((retc = readreply()) != V_OK)
                qdoverror(retc, &Oreq.sh_un.sh_var);
}

/* Action, currently only delete */

void  cb_vact(Widget parent, int data)
{
        BtvarRef        cv = getselectedvar(BTM_DELETE);
        unsigned        retc;
        if  (!cv)
                return;
        Saveseq = cv->var_sequence;
        if  (Dispflags & DF_CONFABORT  &&  !Confirm(vwid, $PH{xmbtq confirm delete var}))
                return;
        qwvmsg(data, cv, Saveseq);
        if  ((retc = readreply()) != V_OK)
                qdoverror(retc, &Oreq.sh_un.sh_var);
}

static void  endvcreate(Widget w, int data)
{
        if  (data)  {
                int             lng;
                unsigned        retc;
                char            *txt, *cp, *ckp;
                XtVaGetValues(workw[WORKW_VARNAME], XmNvalue, &txt, NULL);
                for  (cp = txt;  isspace(*cp);  cp++)
                        ;
                lng = strlen(cp);
                if  (lng <= 0)  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid var name});
                        return;
                }
                if  (lng > BTV_NAME)  {
                        XtFree(txt);
                        disp_arg[0] = BTV_NAME;
                        doerror(w, $EH{xmbtq varname too long});
                        return;
                }
                for  (ckp = cp;  *ckp;  ckp++)
                        if  (!isalnum(*ckp) && *ckp != '_')  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq invalid var name});
                                return;
                        }
                if  (!isalpha(*cp))  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid var name});
                        return;
                }
                strcpy(Oreq.sh_un.sh_var.var_name, cp);
                XtFree(txt);
                extract_vval(&Oreq.sh_un.sh_var.var_value);
                XtVaGetValues(workw[WORKW_VARCOMM], XmNvalue, &txt, NULL);
                for  (cp = txt;  isspace(*cp);  cp++)
                        ;
                lng = strlen(cp);
                if  (lng > BTV_COMMENT)
                        lng = BTV_COMMENT;
                strncpy(Oreq.sh_un.sh_var.var_comment, cp, (unsigned) lng);
                Oreq.sh_un.sh_var.var_comment[lng] = '\0';
                XtFree(txt);
                Oreq.sh_un.sh_var.var_mode.u_flags = mypriv->btu_vflags[0];
                Oreq.sh_un.sh_var.var_mode.g_flags = mypriv->btu_vflags[1];
                Oreq.sh_un.sh_var.var_mode.o_flags = mypriv->btu_vflags[2];
                Oreq.sh_un.sh_var.var_type = 0;
                Oreq.sh_un.sh_var.var_flags = 0;
                qwvmsg(V_CREATE, (BtvarRef) 0, 0L);
                if  ((retc = readreply()) != V_OK)
                        qdoverror(retc, &Oreq.sh_un.sh_var);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_vcreate(Widget parent)
{
        Widget  vc_shell, panew, formw, prevabove, prevleft;
        char    nbuf[20];

        if  (!(mypriv->btu_priv & BTM_CREATE))  {
                doerror(vwid, $EH{xmbtq no var create perm});
                return;
        }

        CreateEditDlg(parent, "vcreate", &vc_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "varname");
        prevabove =
                workw[WORKW_VARNAME] =
                        XtVaCreateManagedWidget("vname",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNcolumns,                     BTV_NAME,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                NULL);

        prevleft = place_label_left(formw, prevabove, "value");
        prevleft =
                workw[WORKW_VARVAL] =
                        XtVaCreateManagedWidget("vval",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNcolumns,                     BTC_VALUE+2,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   prevabove,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                NULL);

        sprintf(nbuf, "%ld", (long) Const_val);
        XmTextSetString(workw[WORKW_VARVAL], nbuf);

        CreateArrowPair("vval",                         formw,
                               prevabove,                       prevleft,
                               (XtCallbackProc) vval_cb,        (XtCallbackProc) vval_cb,
                               1,                               -1);

        prevabove = workw[WORKW_VARVAL];

        prevleft = place_label_left(formw, prevabove, "comment");
        prevleft =
                workw[WORKW_VARCOMM] =
                        XtVaCreateManagedWidget("vomm",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNcolumns,                     BTV_COMMENT,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   prevabove,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                NULL);
        XtManageChild(formw);
        CreateActionEndDlg(vc_shell, panew, (XtCallbackProc) endvcreate, $H{xmbtq vcreate dialog});
}

static void  endrename(Widget w, int data)
{
        if  (data)  {
                unsigned        retc;
                char            *txt, *cp, *ckp;
                int             lng;
                XtVaGetValues(workw[WORKW_VARNAME], XmNvalue, &txt, NULL);
                for  (cp = txt;  isspace(*cp);  cp++)
                        ;
                lng = strlen(cp);
                if  (lng <= 0)  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid var name});
                        return;
                }
                if  (lng > BTV_NAME)  {
                        XtFree(txt);
                        disp_arg[0] = BTV_NAME;
                        doerror(w, $EH{xmbtq varname too long});
                        return;
                }
                for  (ckp = cp;  *ckp;  ckp++)
                        if  (!isalnum(*ckp) && *ckp != '_')  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq invalid var name});
                                return;
                        }
                if  (!isalpha(*cp))  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq invalid var name});
                        return;
                }
                Oreq.sh_params.mcode = V_NEWNAME;
                Oreq.sh_un.sh_rn.sh_ovar = *cvar;
                Oreq.sh_un.sh_rn.sh_ovar.var_sequence = Saveseq;
                strcpy(Oreq.sh_un.sh_rn.sh_rnewname, cp);
                XtFree(txt);
                if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar) + lng + 1, 0) < 0)
                        msg_error();
                if  ((retc = readreply()) != V_OK)
                        qdoverror(retc, &Oreq.sh_un.sh_rn.sh_ovar);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_vrename(Widget parent)
{
        Widget  vr_shell, panew, formw, prevabove, prevleft;
        BtvarRef  cv = getselectedvar(BTM_DELETE);
        if  (!cv)
                return;
        if  (cv->var_id.hostid != 0)  {
                doerror(vwid, $EH{xmbtq renaming remote var});
                return;
        }
        cvar = cv;
        Saveseq = cv->var_sequence;
        prevabove = CreateVeditDlg(parent, "vrename", &vr_shell, &panew, &formw);
        prevleft = XtVaCreateManagedWidget("newname",
                                           xmLabelWidgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        prevabove,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           XmNborderWidth,      0,
                                           NULL);
        workw[WORKW_VARNAME] =
                XtVaCreateManagedWidget("vnewname",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     BTV_NAME,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);
        XtManageChild(formw);
        CreateActionEndDlg(vr_shell, panew, (XtCallbackProc) endrename, $H{xmbtq renamevar dialog});
}

/* This is used by jobs too */

static  Widget  *modew;
static  USHORT   copymode[3];

struct  modeabbrev      {
        USHORT  sflg, rflg;
        char    *name;
};

static  struct  modeabbrev      modenames[] =
        {{      BTM_SHOW,               BTM_WRITE,                      "read"  },
        {       BTM_READ|BTM_SHOW,      0,                              "write" },
        {       0,                      BTM_READ|BTM_WRITE,             "show"  },
        {       0,                      BTM_WRMODE,                     "rmode" },
        {       BTM_RDMODE,             0,                              "wmode" },
        {       0,                      0,                              "utake" },
        {       0,                      0,                              "gtake" },
        {       0,                      0,                              "ugive" },
        {       0,                      0,                              "ggive" },
        {       0,                      0,                              "del"   },
        {       0,                      0,                              "kill"  }};

#define MODENUMBERS     (XtNumber(modenames))

static void  mdtoggle(Widget parent, int which, XmToggleButtonCallbackStruct *cbs)
{
        int     ugo = (which / MODENUMBERS) % 3, wflg = which % 11, cnt;

        if  (cbs->set)  {
                copymode[ugo] |= 1 << wflg;
                if  (modenames[wflg].sflg)  {
                        copymode[ugo] |= modenames[wflg].sflg;
                        for  (cnt = 0;  cnt < MODENUMBERS;  cnt++)
                                if  (modenames[wflg].sflg & (1 << cnt))
                                        XmToggleButtonGadgetSetState(modew[ugo*MODENUMBERS+cnt], True, False);
                }
        }
        else  {
                copymode[ugo] &= ~(1 << wflg);
                if  (modenames[wflg].rflg)  {
                        copymode[ugo] &= ~modenames[wflg].rflg;
                        for  (cnt = 0;  cnt < MODENUMBERS;  cnt++)
                                if  (modenames[wflg].rflg & (1 << cnt))
                                        XmToggleButtonGadgetSetState(modew[ugo*MODENUMBERS+cnt], False, False);
                }
        }
}

static void  endvperm(Widget w, int data)
{
        if  (data)  {
                unsigned        retc;
                char            *txt;
                int_ugid_t      nug;
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XmString        ug_txt;
                XtVaGetValues(workw[WORKW_GTXTW], XmNselectedItem, &ug_txt, NULL);
                XmStringGetLtoR(ug_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(ug_txt);
#else
                XtVaGetValues(workw[WORKW_GTXTW], XmNvalue, &txt, NULL);
#endif
                if  (strcmp(txt, cvar->var_mode.o_group) != 0)  {
                        if  ((nug = lookup_gname(txt)) == UNKNOWN_GID)  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq invalid group});
                                return;
                        }
                        Oreq.sh_params.param = nug;
                        qwvmsg(V_CHGRP, cvar, Saveseq);
                        if  ((retc = readreply()) != V_OK)  {
                                XtFree(txt);
                                qdoverror(retc, &Oreq.sh_un.sh_var);
                                return;
                        }
                        Saveseq++;
                }
                XtFree(txt);
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XtVaGetValues(workw[WORKW_UTXTW], XmNselectedItem, &ug_txt, NULL);
                XmStringGetLtoR(ug_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(ug_txt);
#else
                XtVaGetValues(workw[WORKW_UTXTW], XmNvalue, &txt, NULL);
#endif
                if  (strcmp(txt, cvar->var_mode.o_user) != 0)  {
                        if  ((nug = lookup_uname(txt)) == UNKNOWN_UID)  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq invalid user});
                                return;
                        }
                        Oreq.sh_params.param = nug;
                        qwvmsg(V_CHOWN, cvar, Saveseq);
                        if  ((retc = readreply()) != V_OK)  {
                                XtFree(txt);
                                qdoverror(retc, &Oreq.sh_un.sh_var);
                                return;
                        }
                        Saveseq++;
                }
                XtFree(txt);
                Oreq.sh_un.sh_var = *cvar;
                if  (Oreq.sh_un.sh_var.var_mode.u_flags != copymode[0]  ||
                     Oreq.sh_un.sh_var.var_mode.g_flags != copymode[1]  ||
                     Oreq.sh_un.sh_var.var_mode.o_flags != copymode[2])  {
                        Oreq.sh_un.sh_var.var_mode.u_flags = copymode[0];
                        Oreq.sh_un.sh_var.var_mode.g_flags = copymode[1];
                        Oreq.sh_un.sh_var.var_mode.o_flags = copymode[2];
                        qwvmsg(V_CHMOD, (BtvarRef) 0, Saveseq);
                        if  ((retc = readreply()) != V_OK)
                                qdoverror(retc, &Oreq.sh_un.sh_var);
                }
        }
        if  (modew)  {
                free((char *) modew);
                modew = (Widget *) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  CreateModeDialog(Widget formw, Widget prevabove, BtmodeRef current, unsigned readonly, int isvar)
{
        Widget  mrc;
        int     nrows = MODENUMBERS, row, col;
        unsigned     chgu, chgg;
        int     ch = 'j';
        if  (isvar)  {
                nrows--;
                ch = 'v';
        }
        copymode[0] = current->u_flags;
        copymode[1] = current->g_flags;
        copymode[2] = current->o_flags;

        if  (mypriv->btu_priv & BTM_WADMIN)
                chgu = chgg = 0;
        else  {
                chgu = !mpermitted(current, current->o_uid == Realuid? BTM_UGIVE: BTM_UTAKE, mypriv->btu_priv);
                chgg = !mpermitted(current, current->o_gid == Realgid? BTM_GGIVE: BTM_GTAKE, mypriv->btu_priv);
        }
        prevabove = CreateUselDialog(formw, prevabove, current->o_user, 0, chgu);
        prevabove = CreateGselDialog(formw, prevabove, current->o_group, 0, chgg);
        mrc  = XtVaCreateManagedWidget("md",
                                       xmRowColumnWidgetClass,  formw,
                                       XmNnumColumns,           3,
                                       XmNpacking,              XmPACK_COLUMN,
                                       XmNtopAttachment,        XmATTACH_WIDGET,
                                       XmNtopWidget,            prevabove,
                                       XmNleftAttachment,       XmATTACH_FORM,
                                       NULL);

        for  (col = 0;  col < 3;  col++)  {
                for  (row = 0;  row < nrows;  row++)  {
                        Widget  w;
                        int     indx = col*MODENUMBERS + row;
                        char    name[12];
                        sprintf(name, "%c%c%s", ch, "ugo" [col], modenames[row].name);
                        modew[indx] = w = XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, mrc, NULL);
                        if  (copymode[col] & (1 << row))
                                XmToggleButtonGadgetSetState(w, True, False);
                        if  (!readonly)
                                XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) mdtoggle, INT_TO_XTPOINTER(indx));
                }
        }
}

void  cb_vperm(Widget parent)
{
        Widget  vm_shell, panew, formw, prevabove;
        unsigned        readonly;
        BtvarRef  cv = getselectedvar(BTM_RDMODE);
        if  (!cv)
                return;
        cvar = cv;
        Saveseq = cv->var_sequence;
        if  (!(modew = (Widget *) malloc((unsigned)(3 * MODENUMBERS * sizeof(Widget)))))
                ABORT_NOMEM;
        prevabove = CreateVeditDlg(parent, "vmode", &vm_shell, &panew, &formw);
        readonly = !mpermitted(&cv->var_mode, BTM_WRMODE, mypriv->btu_priv);
        CreateModeDialog(formw, prevabove, &cv->var_mode, readonly, 1);
        XtManageChild(formw);
        CreateActionEndDlg(vm_shell, panew, (XtCallbackProc) endvperm, $H{xmbtq vperm dialog});
}

static void  endjperm(Widget w, int data)
{
        if  (data)  {
                unsigned        retc;
                char            *txt;
                int_ugid_t      nug;
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XmString        ug_txt;
                XtVaGetValues(workw[WORKW_GTXTW], XmNselectedItem, &ug_txt, NULL);
                XmStringGetLtoR(ug_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(ug_txt);
#else
                XtVaGetValues(workw[WORKW_GTXTW], XmNvalue, &txt, NULL);
#endif
                if  (strcmp(txt, cjob->h.bj_mode.o_group) != 0)  {
                        if  ((nug = lookup_gname(txt)) == UNKNOWN_GID)  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq invalid group});
                                return;
                        }
                        Oreq.sh_params.param = nug;
                        qwjimsg(J_CHGRP, cjob);
                        if  ((retc = readreply()) != J_OK)  {
                                XtFree(txt);
                                qdojerror(retc, cjob);
                                return;
                        }
                }
                XtFree(txt);
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XtVaGetValues(workw[WORKW_UTXTW], XmNselectedItem, &ug_txt, NULL);
                XmStringGetLtoR(ug_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(ug_txt);
#else
                XtVaGetValues(workw[WORKW_UTXTW], XmNvalue, &txt, NULL);
#endif
                if  (strcmp(txt, cjob->h.bj_mode.o_user) != 0)  {
                        if  ((nug = lookup_uname(txt)) == UNKNOWN_UID)  {
                                XtFree(txt);
                                doerror(w, $EH{xmbtq invalid user});
                                return;
                        }
                        Oreq.sh_params.param = nug;
                        qwjimsg(J_CHOWN, cjob);
                        if  ((retc = readreply()) != J_OK)  {
                                XtFree(txt);
                                qdojerror(retc, cjob);
                                return;
                        }
                }
                XtFree(txt);
                if  (cjob->h.bj_mode.u_flags != copymode[0]  ||
                     cjob->h.bj_mode.g_flags != copymode[1]  ||
                     cjob->h.bj_mode.o_flags != copymode[2])  {
                        ULONG   xindx;
                        BtjobRef  bjp = &Xbuffer->Ring[xindx = getxbuf()];
                        *bjp = *cjob;
                        bjp->h.bj_mode.u_flags = copymode[0];
                        bjp->h.bj_mode.g_flags = copymode[1];
                        bjp->h.bj_mode.o_flags = copymode[2];
                        wjmsg(J_CHMOD, xindx);
                        if  ((retc = readreply()) != J_OK)
                                qdojerror(retc, bjp);
                        freexbuf(xindx);
                }
        }
        if  (modew)  {
                free((char *) modew);
                modew = (Widget *) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jperm(Widget parent)
{
        Widget  jm_shell, panew, formw, prevabove;
        unsigned        readonly;
        BtjobRef  cj = getselectedjob(BTM_RDMODE);
        if  (!cj)
                return;
        cjob = cj;
        if  (!(modew = (Widget *) malloc((unsigned)(3 * MODENUMBERS * sizeof(Widget)))))
                ABORT_NOMEM;
        prevabove = CreateJeditDlg(parent, "jmode", &jm_shell, &panew, &formw);
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRMODE, mypriv->btu_priv);
        CreateModeDialog(formw, prevabove, &cj->h.bj_mode, readonly, 0);
        XtManageChild(formw);
        CreateActionEndDlg(jm_shell, panew, (XtCallbackProc) endjperm, $H{xmbtq jperm dialog});
}

static void  vmacroexec(char *str, BtvarRef vp)
{
        static  char    *execprog;
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                const  char     *argbuf[3];
                argbuf[0] = str;
                if  (vp)  {
                        argbuf[1] = VAR_NAME(vp);
                        argbuf[2] = (const char *) 0;
                }
                else
                        argbuf[1] = (const char *) 0;
                chdir(Curr_pwd);
                execv(execprog, (char **) argbuf);
                exit(255);
        }
        if  (pid < 0)  {
                doerror(vwid, $EH{xmbtq var macro cannot fork});
                return;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif
        if  (status != 0)  {
                if  (status & 255)  {
                        disp_arg[0] = status & 255;
                        doerror(vwid, $EH{xmbtq var macro signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror(vwid, $EH{xmbtq var macro exit code});
                }
        }
}

static void  endvmacro(Widget w, int data)
{
        if  (data)  {
                char    *txt;
                XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &txt, NULL);
                if  (txt[0])  {
                        int     *plist, pcnt;
                        BtvarRef vp = (BtvarRef) 0;
                        if  (XmListGetSelectedPos(vwid, &plist, &pcnt)  &&  pcnt > 0)  {
                                vp = &vv_ptrs[(plist[0] - 1) / VLINES].vep->Vent;
                                XtFree((XtPointer) plist);
                        }
                        vmacroexec(txt, vp);
                }
                XtFree(txt);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_macrov(Widget parent, int data)
{
        char    *prompt = helpprmpt(data + $P{Var macro});
        int     *plist, pcnt;
        BtvarRef  vp = (BtvarRef) 0;
        Widget  pc_shell, panew, mainform, labw;

        if  (!prompt)  {
                disp_arg[0] = data + $P{Var macro};
                doerror(vwid, $EH{xmbtq macro prompt missing});
                return;
        }

        if  (XmListGetSelectedPos(vwid, &plist, &pcnt)  &&  pcnt > 0)  {
                vp = &vv_ptrs[(plist[0] - 1) / VLINES].vep->Vent;
                XtFree((XtPointer) plist);
        }

        if  (data != 0)  {
                vmacroexec(prompt, vp);
                return;
        }

        CreateEditDlg(parent, "varcmd", &pc_shell, &panew, &mainform, 3);
        labw = place_label_topleft(mainform, "cmdtit");
        workw[WORKW_STXTW] = XtVaCreateManagedWidget("cmd",
                                                     xmTextFieldWidgetClass,    mainform,
                                                     XmNcolumns,                20,
                                                     XmNcursorPositionVisible,False,
                                                     XmNtopAttachment,  XmATTACH_FORM,
                                                     XmNleftAttachment, XmATTACH_WIDGET,
                                                     XmNleftWidget,             labw,
                                                     NULL);
        XtManageChild(mainform);
        CreateActionEndDlg(pc_shell, panew, (XtCallbackProc) endvmacro, $H{Var macro});
}
