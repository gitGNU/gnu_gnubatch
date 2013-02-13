/* xmbr_cbs.c -- generalised callbacks for gbch-xmr

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
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
#include <Xm/ComboBox.h>
#endif
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
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
static  char    Filename[] = __FILE__;
#endif

Widget  workw[30];

void  doinfo(Widget wid, int code)
{
        char    **evec = helpvec(code, 'E'), *newstr;
        Widget          ew;
        if  (!evec[0])  {
                disp_arg[9] = code;
                free((char *) evec);
                evec = helpvec($E{Missing error code}, 'E');
        }
        ew = XmCreateInformationDialog(FindWidget(wid), "info", NULL, 0);
        XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_HELP_BUTTON));
        newstr = makebigvec(evec);
        XtVaSetValues(ew,
                      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
                      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
                      NULL);
        free(newstr);
        XtManageChild(ew);
        XtPopup(XtParent(ew), XtGrabNone);
}

/* Set up a queue selection pane and dialog */

Widget  CreateQselDialog(Widget mainform, Widget prevabove, char *existing, int nullok)
{
        Widget  qtitw;
#if  !defined(HAVE_XM_COMBOBOX_H) || defined(BROKEN_COMBOBOX)
        Widget  qselb;
#else
        char    **qlist = gen_qlist((char *) 0);
        int     nrows, ncols, cnt;
        XmStringTable   st;

        count_hv(qlist, &nrows, &ncols);
        qsort(QSORTP1 qlist, nrows, sizeof(char *), QSORTP4 sort_qug);
        if  (!(st = (XmStringTable) XtMalloc((unsigned) ((nrows+1) * sizeof(XmString *)))))
                ABORT_NOMEM;
        for  (cnt = 0;  cnt < nrows;  cnt++)
                st[cnt] = XmStringCreateLocalized(qlist[cnt][0]? qlist[cnt]: "-");
        freehelp(qlist);
#endif

        qtitw = place_label_left(mainform, prevabove, "queuename");

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
        workw[WORKW_QTXTW] = XtVaCreateManagedWidget("queue",
                                                     xmComboBoxWidgetClass,     mainform,
                                                     XmNcomboBoxType,           XmDROP_DOWN_COMBO_BOX,
                                                     XmNitemCount,              nrows,
                                                     XmNitems,                  st,
                                                     XmNvisibleItemCount,       nrows <= 0? 1: nrows > 15? 15: nrows,
                                                     XmNcolumns,                50,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             qtitw,
                                                     NULL);

        if  (existing  &&  existing[0])  {
                XmString        existstr = XmStringCreateLocalized(existing);
                XtVaSetValues(workw[WORKW_QTXTW], XmNselectedItem, existstr, NULL);
                XmStringFree(existstr);
        }
        for  (cnt = 0;  cnt < nrows;  cnt++)
                XmStringFree(st[cnt]);
        XtFree((XtPointer) st);
#else
        workw[WORKW_QTXTW] = XtVaCreateManagedWidget("queue",
                                                     xmTextFieldWidgetClass,    mainform,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNcolumns,                50,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             qtitw,
                                                     NULL);

        if  (existing  &&  existing[0])
                XmTextSetString(workw[WORKW_QTXTW], existing);

        qselb = XtVaCreateManagedWidget("qselect",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  workw[WORKW_QTXTW],
                                        NULL);

        XtAddCallback(qselb, XmNactivateCallback, (XtCallbackProc) getqueuesel, (XtPointer) nullok);
#endif

        return  workw[WORKW_QTXTW];
}

/* Set up a command interp selection pane and dialog */

Widget  CreateIselDialog(Widget mainform, Widget prevabove, char *existing)
{
        Widget  ititw;
#if  !defined(HAVE_XM_COMBOBOX_H) || defined(BROKEN_COMBOBOX)
        Widget  iselb;
#else
        char    **ilist = listcis((char *) 0);
        int     nrows, ncols, cnt, spos = 0;
        XmStringTable   st;

        count_hv(ilist, &nrows, &ncols);
        if  (!(st = (XmStringTable) XtMalloc((unsigned) ((nrows+1) * sizeof(XmString *)))))
                ABORT_NOMEM;
        for  (cnt = 0;  cnt < nrows;  cnt++)  {
                if  (existing  &&  strcmp(ilist[cnt], existing) == 0)
                        spos = cnt;
                st[cnt] = XmStringCreateLocalized(ilist[cnt]);
        }
        freehelp(ilist);
#endif

        ititw = place_label_left(mainform, prevabove, "interp");

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
        workw[WORKW_ITXTW] = XtVaCreateManagedWidget("int",
                                                     xmComboBoxWidgetClass,     mainform,
                                                     XmNcomboBoxType,           XmDROP_DOWN_LIST,
                                                     XmNitemCount,              nrows,
                                                     XmNitems,                  st,
                                                     XmNcolumns,                CI_MAXNAME,
                                                     XmNmaxWidth,               CI_MAXNAME,
                                                     XmNselectedPosition,       spos,
                                                     XmNvisibleItemCount,       nrows <= 0? 1: nrows > 15? 15: nrows,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             ititw,
                                                     NULL);

        for  (cnt = 0;  cnt < nrows;  cnt++)
                XmStringFree(st[cnt]);
        XtFree((XtPointer) st);
        XtAddCallback(workw[WORKW_ITXTW], XmNselectionCallback, (XtCallbackProc) ilist_cb, (XtPointer) 0);

#else
        workw[WORKW_ITXTW] = XtVaCreateManagedWidget("int",
                                                     xmTextFieldWidgetClass,    mainform,
                                                     XmNcolumns,                CI_MAXNAME,
                                                     XmNmaxWidth,               CI_MAXNAME,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             ititw,
                                                     NULL);

        if  (existing)
                XmTextSetString(workw[WORKW_ITXTW], existing);

        iselb = XtVaCreateManagedWidget("iselect",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  workw[WORKW_ITXTW],
                                        NULL);

        XtAddCallback(iselb, XmNactivateCallback, (XtCallbackProc) getintsel, (XtPointer) 0);
#endif
        return  workw[WORKW_ITXTW];
}

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
static Widget  Create_ug_sel_dlg(Widget mainform, Widget prevabove, char *labname, char *widname, int widnum, char **uglist, char *existing, int nullok, unsigned readonly)
{
        Widget  ugtitw;
        int     nrows, ncols, cnt, cnt2, spos = -1;
        XmStringTable   st;

        count_hv(uglist, &nrows, &ncols);
        qsort(QSORTP1 uglist, nrows, sizeof(char *), QSORTP4 sort_qug);
        if  (!(st = (XmStringTable) XtMalloc((unsigned) ((nrows+1) * sizeof(XmString *)))))
                ABORT_NOMEM;
        cnt2 = 0;
        if  (nullok)  {
                if  (!existing  ||  !existing[0])
                        spos = 0;
                st[cnt2++] = XmStringCreateLocalized("-");
        }
        for  (cnt = 0;  cnt < nrows;  cnt++)  {
                if  (existing  &&  strcmp(uglist[cnt], existing) == 0)
                        spos = cnt2;
                st[cnt2++] = XmStringCreateLocalized(uglist[cnt]);
        }
        freehelp(uglist);

        ugtitw = place_label_left(mainform, prevabove, labname);
        workw[widnum] = XtVaCreateManagedWidget(widname,
                                                xmComboBoxWidgetClass,  mainform,
                                                XmNcomboBoxType,        nullok? XmDROP_DOWN_COMBO_BOX: XmDROP_DOWN_LIST,
                                                XmNitemCount,           nrows,
                                                XmNitems,               st,
                                                XmNvisibleItemCount,    nrows <= 0? 1: nrows > 15? 15: nrows,
                                                XmNcolumns,             UIDSIZE,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_WIDGET,
                                                XmNleftWidget,          ugtitw,
                                                NULL);

        if  (spos >= 0  ||  !nullok)
                XtVaSetValues(workw[widnum], XmNselectedPosition, spos >= 0? spos: 0, NULL);
        else  {
                XmString        existstr = XmStringCreateLocalized(existing? existing: "");
                XtVaSetValues(workw[widnum], XmNselectedItem, existstr, NULL);
                XmStringFree(existstr);
        }
        for  (cnt = 0;  cnt < cnt2;  cnt++)
                XmStringFree(st[cnt]);
        XtFree((XtPointer) st);
        if  (readonly)
                XtSetSensitive(workw[widnum], False);
        return  workw[widnum];
}

Widget  CreateUselDialog(Widget mainform, Widget prevabove, char *existing, int nullok, unsigned readonly)
{
        return  Create_ug_sel_dlg(mainform, prevabove, "username", "user", WORKW_UTXTW, gen_ulist((char *) 0), existing, nullok, readonly);
}

/* Set up a group selection pane and dialog */

Widget  CreateGselDialog(Widget mainform, Widget prevabove, char *existing, int nullok, unsigned readonly)
{
        return  Create_ug_sel_dlg(mainform, prevabove, "groupname", "group", WORKW_GTXTW, gen_glist((char *) 0), existing, nullok, readonly);
}

#else

static Widget  Create_ug_sel_dlg(Widget mainform, Widget prevabove, char *labname, char *widname, char *butname, int widnum, XtCallbackProc callb, char *existing, int nullok, unsigned readonly)
{
        Widget  ugtitw, ugselb;

        ugtitw = place_label_left(mainform, prevabove, labname);
        workw[widnum] = XtVaCreateManagedWidget(widname,
                                                xmTextFieldWidgetClass, mainform,
                                                XmNcolumns,             UIDSIZE,
                                                XmNmaxWidth,            UIDSIZE,
                                                XmNeditable,            readonly? False: True,
                                                XmNcursorPositionVisible,       False,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_WIDGET,
                                                XmNleftWidget,          ugtitw,
                                                NULL);

        if  (existing)
                XmTextSetString(workw[widnum], existing);

        if  (!readonly)  {
                ugselb = XtVaCreateManagedWidget(butname,
                                                xmPushButtonWidgetClass,        mainform,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   prevabove,
                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                XmNleftWidget,                  workw[WORKW_UTXTW],
                                                NULL);

                XtAddCallback(ugselb, XmNactivateCallback, callb, (XtPointer) nullok);
        }
        return  workw[widnum];
}

/* Set up a user selection pane and dialog */

Widget  CreateUselDialog(Widget mainform, Widget prevabove, char *existing, int nullok, unsigned readonly)
{
        return  Create_ug_sel_dlg(mainform, prevabove, "username", "user", "uselect", WORKW_UTXTW, (XtCallbackProc) getusersel, existing, nullok, readonly);
}

/* Set up a group selection pane and dialog */

Widget  CreateGselDialog(Widget mainform, Widget prevabove, char *existing, int nullok, unsigned readonly)
{
        return  Create_ug_sel_dlg(mainform, prevabove, "groupname", "group", "gselect", WORKW_GTXTW, (XtCallbackProc) getgroupsel, existing, nullok, readonly);
}
#endif

/* Create the stuff at the beginning of a dialog */

void  CreateEditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres, const int nbutts)
{
        int     n = 0;
        Arg     arg[8];
        static  XmString oks, cancs, helps;

        if  (!oks)  {
                oks = XmStringCreateLocalized("Ok");
                cancs = XmStringCreateLocalized("Cancel");
                helps = XmStringCreateLocalized("Help");
        }
        XtSetArg(arg[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
        XtSetArg(arg[n], XmNdeleteResponse, XmDESTROY); n++;
        XtSetArg(arg[n], XmNokLabelString, oks);        n++;
        XtSetArg(arg[n], XmNcancelLabelString, cancs);  n++;
        XtSetArg(arg[n], XmNhelpLabelString, helps);    n++;
        XtSetArg(arg[n], XmNautoUnmanage, False);       n++;
        *dlgres = *paneres = XmCreateTemplateDialog(GetTopShell(parent), dlgname, arg, n);
        *formres = XtVaCreateWidget("form",
                                    xmFormWidgetClass,          *dlgres,
                                    XmNfractionBase,            2*nbutts,
                                    NULL);
}

/* Create the stuff at the end of a dialog.  */

void  CreateActionEndDlg(Widget shelldlg, Widget panew, XtCallbackProc endrout, int helpcode)
{
        XtAddCallback(shelldlg, XmNokCallback, endrout, (XtPointer) 1);
        XtAddCallback(shelldlg, XmNcancelCallback, endrout, (XtPointer) 0);
        XtAddCallback(shelldlg, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        XtManageChild(shelldlg);
}

static void  endviewopt(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *txt;
                XtVaGetValues(workw[WORKW_EDITORW], XmNvalue, &txt, NULL);
                if  (txt[0])  {
                        free(editor_name);
                        editor_name = stracpy(txt);
                        xterm_edit = XmToggleButtonGadgetGetState(workw[WORKW_XTERMW])? 1: 0;
                }
                XtFree(txt);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_viewopt(Widget parent, int notused)
{
        Widget  view_shell, paneview, mainform, prevabove, etit, ewid, xtermw;

        CreateEditDlg(parent, "Viewopts", &view_shell, &paneview, &mainform, 3);
        prevabove = XtVaCreateManagedWidget("viewtitle",
                                            xmLabelWidgetClass,         mainform,
                                            XmNtopAttachment,           XmATTACH_FORM,
                                            XmNleftAttachment,          XmATTACH_FORM,
                                            XmNborderWidth,             0,
                                            NULL);

        etit = place_label_left(mainform, prevabove, "editor");
        ewid = workw[WORKW_EDITORW] =
                XtVaCreateManagedWidget("editw",
                                        xmTextFieldWidgetClass,         mainform,
                                        XmNcursorPositionVisible,       False,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  etit,
                                        NULL);

        XmTextSetString(ewid, editor_name);

        xtermw = workw[WORKW_XTERMW] =
                XtVaCreateManagedWidget("xtermedit",
                                        xmToggleButtonGadgetClass,      mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   ewid,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        NULL);
        if  (xterm_edit)
                XmToggleButtonGadgetSetState(xtermw, True, False);

        XtManageChild(mainform);
        CreateActionEndDlg(view_shell, paneview, (XtCallbackProc) endviewopt, $H{xmbtr options dialog help});
}
