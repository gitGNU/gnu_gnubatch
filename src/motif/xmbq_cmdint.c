/* xmbq_cmdint.c -- command interpreter handling for gbch-xmq

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <Xm/ArrowB.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
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

static  char    Filename[] = __FILE__;

extern void  ll_cb(Widget, int, XmArrowButtonCallbackStruct *);

static  char            newflag = 0;
static  unsigned        copy_Ci_num;
static  CmdintRef       copy_Ci_list;
static  Cmdint          work_ci;
extern  Widget  listw;
extern  int     current_select;

#define CNAME_P 0
#define CNICE_P (CI_MAXNAME + 1)
#define CLL_P   (CNICE_P + 3)
#define CPATH_P (CLL_P + 6)
#define CARGS_P (CPATH_P + CI_MAXFPATH + 1)

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

static void  fillcilist()
{
        int             jcnt, outl;
        CmdintRef       cci;
        XmString        str;
        char            obuf[CI_MAXNAME+CI_MAXFPATH+CI_MAXARGS+3+6+2+10+10+6];
        char            nbuf[10];
        static  char    *set0flag, *expflag;
        if  (!set0flag)
                set0flag = gprompt($P{Ci set arg0});
        if  (!expflag)
                expflag = gprompt($P{Ci expand args});

        XmListDeleteAllItems(listw);

        for  (jcnt = 0;  jcnt < copy_Ci_num;  jcnt++)  {
                int     wh;
                cci = &copy_Ci_list[jcnt];
                if  (cci->ci_name[0] == '\0')
                        continue;
                BLOCK_SET(obuf, sizeof(obuf), ' ');
                movein(&obuf[CNAME_P], cci->ci_name);
                sprintf(nbuf, "%2u", (unsigned) cci->ci_nice);
                movein(&obuf[CNICE_P], nbuf);
                sprintf(nbuf, "%5u", cci->ci_ll);
                movein(&obuf[CLL_P], nbuf);
                movein(&obuf[CPATH_P], cci->ci_path);
                movein(&obuf[wh = CPATH_P+strlen(cci->ci_path)+1], cci->ci_args);
                wh += strlen(cci->ci_args) + 1;
                if  (cci->ci_flags & CIF_INTERPARGS)  {
                        movein(&obuf[wh], expflag);
                        wh += strlen(expflag) + 1;
                }
                if  (cci->ci_flags & CIF_SETARG0)
                        movein(&obuf[wh], set0flag);
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

static void  endciedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt;
                int     jcnt;
                struct  stat    sbuf;

                /* Get std flags */
                work_ci.ci_flags &= ~(CIF_SETARG0|CIF_INTERPARGS);
                if  (XmToggleButtonGadgetGetState(workw[WORKW_CISETARG0]))
                        work_ci.ci_flags |= CIF_SETARG0;
                if  (XmToggleButtonGadgetGetState(workw[WORKW_CIEXPAND]))
                        work_ci.ci_flags |= CIF_INTERPARGS;

                /* Get nice */
#ifdef HAVE_XM_SPINB_H
                work_ci.ci_nice = (unsigned char) get_spinbox_int(WW(WORKW_CINICE));
#else
                jcnt = get_textbox_int(WW(WORKW_CINICE));
                if  (jcnt < 0  ||  jcnt > 40)
                        jcnt = DEF_CI_NICE;
                work_ci.ci_nice = (unsigned char) jcnt;
#endif

                /* Get load level */
                jcnt = get_textbox_int(WW(WORKW_LLTXTW));
                if  (jcnt <= 0  ||  jcnt > 0x7fff)
                        jcnt = mypriv->btu_spec_ll;
                work_ci.ci_ll = (USHORT) jcnt;

                /* Get name and path */

                XtVaGetValues(workw[WORKW_CINAME], XmNvalue, &txt, NULL);
                if  (txt[0] == '\0'  ||  !isalpha(txt[0]))  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq inval ci name});
                        return;
                }
                jcnt = strlen(txt);
                if  (jcnt > CI_MAXNAME)
                        jcnt = CI_MAXNAME;
                strncpy(work_ci.ci_name, txt, jcnt);
                XtFree(txt);
                work_ci.ci_name[jcnt] = '\0';
                if  (newflag)
                        for  (jcnt = 0;  jcnt < copy_Ci_num;  jcnt++)
                                if  (strcmp(work_ci.ci_name, copy_Ci_list[jcnt].ci_name) == 0)  {
                                        doerror(w, $EH{xmbtq ci name clash});
                                        return;
                                }
                XtVaGetValues(workw[WORKW_CIPATH], XmNvalue, &txt, NULL);
                if  (txt[0] != '/')  {
                        XtFree(txt);
                        doerror(w, $EH{xmbtq ci path not abs});
                        return;
                }
                jcnt = strlen(txt);
                if  (jcnt > CI_MAXFPATH)
                        jcnt = CI_MAXFPATH;
                strncpy(work_ci.ci_path, txt, jcnt);
                XtFree(txt);
                work_ci.ci_path[jcnt] = '\0';
                disp_str = work_ci.ci_name;
                disp_str2 = work_ci.ci_path;
                if  ((stat(work_ci.ci_path, &sbuf) < 0  ||
                     (sbuf.st_mode & S_IFMT) != S_IFREG  ||
                     (sbuf.st_mode & 0111) != 0111)  &&  !Confirm(w, $PH{xmbtq suspicious path name}))
                        return;
                XtVaGetValues(workw[WORKW_CIARGS], XmNvalue, &txt, NULL);
                jcnt = strlen(txt);
                if  (jcnt > CI_MAXARGS)
                        jcnt = CI_MAXARGS;
                strncpy(work_ci.ci_args, txt, jcnt);
                XtFree(txt);
                work_ci.ci_args[jcnt] = '\0';
                if  (newflag)  {
                        for  (jcnt = 0;  jcnt < copy_Ci_num;  jcnt++)
                                if  (copy_Ci_list[jcnt].ci_name[0] == '\0')
                                        goto  gotit;
                        copy_Ci_num++;
                        if  (!(copy_Ci_list = (CmdintRef) realloc((char *) copy_Ci_list, (unsigned)(copy_Ci_num * sizeof(Cmdint)))))
                                ABORT_NOMEM;
                }
                else  {
                        int     had = 0;
                        for  (jcnt = 0;  jcnt < copy_Ci_num;  jcnt++)
                                if  (copy_Ci_list[jcnt].ci_name[0]  &&  ++had > current_select)
                                        goto  gotit;
                        doerror(w, $EH{xmbtq selection not found});
                }
        gotit:
                copy_Ci_list[jcnt] = work_ci;
                fillcilist();
        }
        XtDestroyWidget(GetTopShell(w));
}

#ifndef HAVE_XM_SPINB_H

/* Increment nice */

static void  niceup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_max = 40;
                arrow_lng = 2;
                arrow_incr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}

/* Decrement nice */

static void  nicedn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_min = 0;
                arrow_lng = 2;
                arrow_decr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}
#endif

static void  ciedit(Widget w, int isnew)
{
        Widget  ce_shell, panew, formw, prevabove, prevleft;

        newflag = (char) isnew;

        CreateEditDlg(w, "ciedit", &ce_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "name");
        prevabove =
                workw[WORKW_CINAME] =
                        XtVaCreateManagedWidget("cinam",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                XmNcolumns,                     CI_MAXNAME,
                                                NULL);

        XmTextSetString(workw[WORKW_CINAME], work_ci.ci_name);

        prevleft = place_label_left(formw, prevabove, "nice");

#ifdef HAVE_XM_SPINB_H
        prevleft = XtVaCreateManagedWidget("ncesp",
                                           xmSpinBoxWidgetClass,        formw,
                                           XmNtopAttachment,            XmATTACH_WIDGET,
                                           XmNtopWidget,                prevabove,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,                       prevleft,
                                           NULL);

        workw[WORKW_CINICE] = XtVaCreateManagedWidget("nce",
                                                      xmTextFieldWidgetClass,   prevleft,
                                                      XmNminimumValue,          0,
                                                      XmNmaximumValue,          40,
                                                      XmNspinBoxChildType,      XmNUMERIC,
                                                      XmNposition,              (int) work_ci.ci_nice,
                                                      XmNcursorPositionVisible, False,
                                                      XmNeditable,              False,
                                                      XmNcolumns,               2,
                                                      NULL);
#else

        prevleft = workw[WORKW_CINICE] = XtVaCreateManagedWidget("nce",
                                                                 xmTextFieldWidgetClass,        formw,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNeditable,                   False,
                                                                 XmNcolumns,                    2,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

        put_textbox_int(WW(WORKW_CINICE), work_ci.ci_nice, 2);

        prevleft = CreateArrowPair("nce", formw, prevabove, prevleft, (XtCallbackProc) niceup_cb, (XtCallbackProc) nicedn_cb, WORKW_CINICE, WORKW_CINICE);
#endif

        prevleft = place_label(formw, prevabove, prevleft, "loadlev");
        prevleft = workw[WORKW_LLTXTW] = XtVaCreateManagedWidget("ll",
                                                                 xmTextFieldWidgetClass,        formw,
                                                                 XmNcolumns,                    5,
                                                                 XmNmaxWidth,                   5,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNeditable,                   False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

        put_textbox_int(WW(WORKW_LLTXTW), work_ci.ci_ll, 5);
        CreateArrowPair("ll", formw, prevabove, prevleft, (XtCallbackProc) ll_cb, (XtCallbackProc) ll_cb, 1, -1);

#ifdef HAVE_XM_SPINB_H
        prevabove = workw[WORKW_LLTXTW];
#else
        prevabove = workw[WORKW_CINICE];
#endif

        prevleft = place_label_left(formw, prevabove, "path");
        prevabove =
                workw[WORKW_CIPATH] =
                        XtVaCreateManagedWidget("pth",
                                                xmTextFieldWidgetClass,         formw,
                                                XmNcolumns,                     CI_MAXFPATH,
                                                XmNmaxWidth,                    CI_MAXFPATH,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   prevabove,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  prevleft,
                                                NULL);

        XmTextSetString(workw[WORKW_CIPATH], work_ci.ci_path);

        prevleft = place_label_left(formw, prevabove, "args");

        prevleft = workw[WORKW_CIARGS] =
                XtVaCreateManagedWidget("ags",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     CI_MAXARGS,
                                        XmNmaxWidth,                    CI_MAXARGS,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);

        XmTextSetString(workw[WORKW_CIARGS], work_ci.ci_args);
        workw[WORKW_CISETARG0] = XtVaCreateManagedWidget("setarg0",
                                                         xmToggleButtonGadgetClass,     formw,
                                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                                         XmNtopWidget,                  prevabove,
                                                         XmNleftAttachment,             XmATTACH_WIDGET,
                                                         XmNleftWidget,                 prevleft,
                                                         NULL);
        if  (work_ci.ci_flags & CIF_SETARG0)
                XmToggleButtonGadgetSetState(workw[WORKW_CISETARG0], True, False);

        workw[WORKW_CIEXPAND] = XtVaCreateManagedWidget("expandargs",
                                                         xmToggleButtonGadgetClass,     formw,
                                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                                         XmNtopWidget,                  workw[WORKW_CISETARG0],
                                                         XmNleftAttachment,             XmATTACH_FORM,
                                                         NULL);

        if  (work_ci.ci_flags & CIF_INTERPARGS)
                XmToggleButtonGadgetSetState(workw[WORKW_CIEXPAND], True, False);

        XtManageChild(formw);
        CreateActionEndDlg(ce_shell, panew, (XtCallbackProc) endciedit, $H{xmbtq edit ci dialog});
}

static void  newci(Widget w)
{
        work_ci.ci_name[0] = '\0';
        work_ci.ci_nice = DEF_CI_NICE;
        work_ci.ci_flags = 0;
        work_ci.ci_ll = mypriv->btu_spec_ll;
        work_ci.ci_path[0] = '\0';
        work_ci.ci_args[0] = '\0';
        ciedit(w, 1);
}

static void  editci(Widget w)
{
        int     had = 0, jcnt;
        if  (current_select < 0)  {
                doerror(w, $EH{xmbtq no ci selected});
                return;
        }
        for  (jcnt = 0;  jcnt < copy_Ci_num;  jcnt++)
                if  (copy_Ci_list[jcnt].ci_name[0]  &&  ++had > current_select)
                        goto  gotit;
        doerror(w, $EH{xmbtq selection not found});
 gotit:
        work_ci = copy_Ci_list[jcnt];
        ciedit(w, 0);
}

static void  delci(Widget w)
{
        int     jcnt, had = 0;
        if  (current_select < 0)  {
                doerror(w, $EH{xmbtq no ci selected});
                return;
        }
        for  (jcnt = 0;  jcnt < copy_Ci_num;  jcnt++)
                if  (copy_Ci_list[jcnt].ci_name[0]  &&  ++had > current_select)
                        goto  gotit;
        doerror(w, $EH{xmbtq selection not found});
 gotit:
        disp_str = copy_Ci_list[jcnt].ci_name;
        disp_str2 = copy_Ci_list[jcnt].ci_path;
        if  (jcnt == CI_STDSHELL)  {
                doerror(w, $EH{xmbtq deleting std ci});
                return;
        }
        if  (!chk_okcidel(jcnt))  {
                doerror(w, $EH{xmbtq ci in use});
                return;
        }
        if  (!Confirm(w, $PH{xmbtq confirm delete ci}))
                return;
        copy_Ci_list[jcnt].ci_name[0] = '\0';
        fillcilist();
}

static void  endinterps(Widget w, int data)
{
        if  (data)  {
                free((char *) Ci_list);
                Ci_list = copy_Ci_list;
                copy_Ci_list = (CmdintRef) 0;
                Ci_num = copy_Ci_num;
                lseek(Ci_fd, 0L, 0);
                write(Ci_fd, (char *) Ci_list, Ci_num * sizeof(Cmdint));
        }
        if  (copy_Ci_list)  {
                free((char *) copy_Ci_list);
                copy_Ci_list = (CmdintRef) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  ilist_select(Widget w, int unused, XmListCallbackStruct *cbs)
{
        current_select = cbs->item_position - 1;
}

void  cb_interps(Widget parent)
{
        Widget  ci_shell, panew, editform, neww, editw, delw;
        unsigned  readonly = !(mypriv->btu_priv & BTM_SPCREATE);
        int     n;
        Arg             args[6];

        rereadcif();
        copy_Ci_num = Ci_num;
        if  (!(copy_Ci_list = (CmdintRef) malloc((unsigned) (copy_Ci_num * sizeof(Cmdint)))))
                ABORT_NOMEM;
        BLOCK_COPY(copy_Ci_list, Ci_list, (unsigned) (Ci_num * sizeof(Cmdint)));
        CreateEditDlg(parent, "Interps", &ci_shell, &panew, &editform, 3);

        neww = XtVaCreateManagedWidget("New",
                                       xmPushButtonGadgetClass,         editform,
                                       XmNshowAsDefault,                True,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,                    0,
                                       XmNbottomOffset,                 0,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_POSITION,
                                       XmNleftPosition,                 0,
                                       NULL);

        editw = XtVaCreateManagedWidget("Edit",
                                        xmPushButtonGadgetClass,        editform,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                2,
                                        NULL);

        delw = XtVaCreateManagedWidget("Delete",
                                       xmPushButtonGadgetClass, editform,
                                       XmNshowAsDefault,                False,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,                    0,
                                       XmNbottomOffset,                 0,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_POSITION,
                                       XmNleftPosition,                 4,
                                       NULL);
        if  (readonly)  {
                XtSetSensitive(neww, False);
                XtSetSensitive(editw, False);
                XtSetSensitive(delw, False);
        }
        else  {
                XtAddCallback(neww, XmNactivateCallback, (XtCallbackProc) newci, (XtPointer) 0);
                XtAddCallback(editw, XmNactivateCallback, (XtCallbackProc) editci, (XtPointer) 0);
                XtAddCallback(delw, XmNactivateCallback, (XtCallbackProc) delci, (XtPointer) 0);
        }
        n = 0;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNtopWidget, neww); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        listw = XmCreateScrolledList(editform, "Interplist", args, n);
        current_select = -1;
        XtAddCallback(listw, XmNdefaultActionCallback, (XtCallbackProc) ilist_select, (XtPointer) 0);
        XtAddCallback(listw, XmNsingleSelectionCallback, (XtCallbackProc) ilist_select, (XtPointer) 0);
        fillcilist();
        XtManageChild(listw);
        XtManageChild(editform);
        CreateActionEndDlg(ci_shell, panew, (XtCallbackProc) endinterps, $H{xmbtq ci display dialog});
}
