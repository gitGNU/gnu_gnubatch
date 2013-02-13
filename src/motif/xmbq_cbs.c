/* xmbq_cbs.c -- Generalised callback routines for gbch-xmq

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
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbq_ext.h"
#include "optflags.h"

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
static  char    Filename[] = __FILE__;
#endif

Widget  workw[30];

static  char    matchcase, wraparound, sbackward, searchvars;
static  char    searchtit = 1,
                searchuser = 1,
                searchgroup = 1,
                searchnam = 1,
                searchcomm = 1,
                searchval = 1;

static  char    *matchtext;

void  jdisplay();
void  vdisplay();

/* Set up a queue selection pane and dialog */

static Widget  CreateQselDialog(Widget mainform, Widget prevabove, char *existing, int nullok)
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
        workw[WORKW_INCNULL] = XtVaCreateManagedWidget("incnull",
                                                       xmToggleButtonGadgetClass,       mainform,
                                                       XmNtopAttachment,                XmATTACH_WIDGET,
                                                       XmNtopWidget,                    workw[WORKW_QTXTW],
                                                       XmNleftAttachment,               XmATTACH_FORM,
                                                       NULL);
        if  (!(Dispflags & DF_SUPPNULL))
                XmToggleButtonGadgetSetState(workw[WORKW_INCNULL], True, False);
        return  workw[WORKW_INCNULL];
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

static void  widgassign(int widnum, char **fld)
{
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
        XmString        item_txt;
#endif
        char            *restxt;

        if  (*fld)
                free(*fld);

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
        XtVaGetValues(workw[widnum], XmNselectedItem, &item_txt, NULL);
        XmStringGetLtoR(item_txt, XmSTRING_DEFAULT_CHARSET, &restxt);
        XmStringFree(item_txt);
        *fld = restxt[0] && restxt[0] != '-'? stracpy(restxt): (char *) 0;
#else
        XtVaGetValues(workw[widnum], XmNvalue, &restxt, NULL);
        *fld = restxt[0]? stracpy(restxt): (char *) 0;
#endif
        XtFree(restxt);
}

static void  endviewopt(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                widgassign(WORKW_QTXTW, &jobqueue);
                widgassign(WORKW_UTXTW, &Restru);
                widgassign(WORKW_GTXTW, &Restrg);
                if  (XmToggleButtonGadgetGetState(workw[WORKW_LOCO]))
                        Dispflags |= DF_LOCALONLY;
                else
                        Dispflags &= ~DF_LOCALONLY;
                if  (XmToggleButtonGadgetGetState(workw[WORKW_INCNULL]))
                        Dispflags &= ~DF_SUPPNULL;
                else
                        Dispflags |= DF_SUPPNULL;
                if  (XmToggleButtonGadgetGetState(workw[WORKW_CONFA]))
                        Dispflags |= DF_CONFABORT;
                else
                        Dispflags &= ~DF_CONFABORT;
        }
        XtDestroyWidget(GetTopShell(w));
        Last_j_ser = Last_v_ser = 0;
        jdisplay();
        vdisplay();
}

void  cb_viewopt(Widget parent)
{
        Widget  view_shell, paneview, mainform, prevabove, butts, loconly, confbuts, allh, neva;
        Widget  jdisp_but, save_but;

        CreateEditDlg(parent, "Viewopts", &view_shell, &paneview, &mainform, 3);
        prevabove = XtVaCreateManagedWidget("viewtitle",
                                            xmLabelWidgetClass,         mainform,
                                            XmNtopAttachment,           XmATTACH_FORM,
                                            XmNleftAttachment,          XmATTACH_FORM,
                                            XmNborderWidth,             0,
                                            NULL);

        prevabove = CreateQselDialog(mainform, prevabove, jobqueue, 1);
        prevabove = CreateUselDialog(mainform, prevabove, Restru? Restru: "", 1, 0);
        prevabove = CreateGselDialog(mainform, prevabove, Restrg? Restrg: "", 1, 0);

        butts = XtVaCreateManagedWidget("butts",
                                        xmRowColumnWidgetClass, mainform,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           prevabove,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNrightAttachment,     XmATTACH_FORM,
                                        XmNpacking,             XmPACK_COLUMN,
                                        XmNnumColumns,          2,
                                        NULL);

        loconly = XtVaCreateManagedWidget("loconly",
                                          xmRowColumnWidgetClass, butts,
                                          XmNpacking,           XmPACK_COLUMN,
                                          XmNnumColumns,        1,
                                          XmNisHomogeneous,     True,
                                          XmNentryClass,        xmToggleButtonGadgetClass,
                                          XmNradioBehavior,     True,
                                          NULL);

        confbuts = XtVaCreateManagedWidget("confbuts",
                                           xmRowColumnWidgetClass, butts,
                                           XmNpacking,          XmPACK_COLUMN,
                                           XmNnumColumns,       1,
                                           XmNisHomogeneous,    True,
                                           XmNentryClass,       xmToggleButtonGadgetClass,
                                           XmNradioBehavior,    True,
                                           NULL);

        allh = XtVaCreateManagedWidget("allhosts",
                                       xmToggleButtonGadgetClass,
                                       loconly,
                                       NULL);

        workw[WORKW_LOCO] = XtVaCreateManagedWidget("localonly",
                                                    xmToggleButtonGadgetClass,
                                                    loconly,
                                                    NULL);

        XmToggleButtonGadgetSetState(Dispflags & DF_LOCALONLY? workw[WORKW_LOCO]: allh, True, False);


        neva = XtVaCreateManagedWidget("never", xmToggleButtonGadgetClass, confbuts, NULL);
        workw[WORKW_CONFA] = XtVaCreateManagedWidget("always", xmToggleButtonGadgetClass, confbuts, NULL);
        XmToggleButtonGadgetSetState(Dispflags & DF_CONFABORT? workw[WORKW_CONFA]: neva, True, False);

        jdisp_but = XtVaCreateManagedWidget("jdispfmt",
                                            xmPushButtonWidgetClass,    mainform,
                                            XmNtopAttachment,           XmATTACH_WIDGET,
                                            XmNtopWidget,               butts,
                                            XmNleftAttachment,          XmATTACH_FORM,
                                            NULL);

        XtAddCallback(jdisp_but, XmNactivateCallback, (XtCallbackProc) cb_setjdisplay, (XtPointer) 0);
        jdisp_but = XtVaCreateManagedWidget("vdispfmt1",
                                            xmPushButtonWidgetClass,    mainform,
                                            XmNtopAttachment,           XmATTACH_WIDGET,
                                            XmNtopWidget,               butts,
                                            XmNleftAttachment,          XmATTACH_WIDGET,
                                            XmNleftWidget,              jdisp_but,
                                            NULL);

        XtAddCallback(jdisp_but, XmNactivateCallback, (XtCallbackProc) cb_setvdisplay, (XtPointer) 0);

        jdisp_but = XtVaCreateManagedWidget("vdispfmt2",
                                            xmPushButtonWidgetClass,    mainform,
                                            XmNtopAttachment,           XmATTACH_WIDGET,
                                            XmNtopWidget,               butts,
                                            XmNleftAttachment,          XmATTACH_WIDGET,
                                            XmNleftWidget,              jdisp_but,
                                            NULL);

        XtAddCallback(jdisp_but, XmNactivateCallback, (XtCallbackProc) cb_setvdisplay, (XtPointer) 1);

        save_but = XtVaCreateManagedWidget("savehome",
                                            xmPushButtonWidgetClass,    mainform,
                                            XmNtopAttachment,           XmATTACH_WIDGET,
                                            XmNtopWidget,               jdisp_but,
                                            XmNleftAttachment,          XmATTACH_FORM,
                                            NULL);

        XtAddCallback(save_but, XmNactivateCallback, (XtCallbackProc) cb_saveformats, (XtPointer) 1);

        save_but = XtVaCreateManagedWidget("savecurr",
                                           xmPushButtonWidgetClass,     mainform,
                                           XmNtopAttachment,            XmATTACH_WIDGET,
                                           XmNtopWidget,                jdisp_but,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               save_but,
                                           NULL);

        XtAddCallback(save_but, XmNactivateCallback, (XtCallbackProc) cb_saveformats, (XtPointer) 0);
        XtManageChild(mainform);
        CreateActionEndDlg(view_shell, paneview, (XtCallbackProc) endviewopt, $H{xmbtq display opts dialog});
}

static int  smstr(const char *str, const int exact)
{
        int      l = strlen(matchtext), cl = strlen(str);

        if  (cl < l)
                return  0;

        if  (exact)  {
                int     cnt;
                if  (cl != l)
                        return  0;
                for  (cnt = 0;  cnt < l;  cnt++)  {
                        int     mch = matchtext[cnt];
                        int     sch = str[cnt];
                        if  (!matchcase)  {
                                mch = toupper(mch);
                                sch = toupper(sch);
                        }
                        if  (!(mch == sch  ||  mch == '.'))
                                return  0;
                }
                return  1;
        }
        else  {
                int      resid = cl - l, bcnt, cnt;
                for  (bcnt = 0;  bcnt <= resid;  bcnt++)  {
                        for  (cnt = 0;  cnt < l;  cnt++)  {
                                int     mch = matchtext[cnt];
                                int     sch = str[cnt+bcnt];
                                if  (!matchcase)  {
                                        mch = toupper(mch);
                                        sch = toupper(sch);
                                }
                                if  (!(mch == sch  ||  mch == '.'))
                                        goto  notmatch;
                        }
                        return  1;
                notmatch:
                        ;
                }
                return  0;
        }
}

static int  jobmatch(int n)
{
        BtjobRef        jp = jj_ptrs[n];

        if  (searchtit  &&  smstr(qtitle_of(jp), 0))
                return  1;
        if  (searchuser  &&  smstr(jp->h.bj_mode.o_user, 1))
                return  1;
        if  (searchgroup  &&  smstr(jp->h.bj_mode.o_group, 1))
                return  1;
        return  0;
}

static int  varmatch(int n)
{
        BtvarRef        vv = &vv_ptrs[n].vep->Vent;

        if  (searchuser  &&  smstr(vv->var_mode.o_user, 1))
                return  1;
        if  (searchgroup  &&  smstr(vv->var_mode.o_group, 1))
                return  1;
        if  (searchnam  &&  smstr(vv->var_name, 0))
                return  1;
        if  (searchcomm  &&  smstr(vv->var_comment, 0))
                return  1;
        if  (searchval)  {
                if  (vv->var_value.const_type == CON_LONG)  {
                        char    nbuf[20];
                        sprintf(nbuf, "%ld", (long) vv->var_value.con_un.con_long);
                        if  (smstr(nbuf, 0))
                                return  1;
                }
                else  if  (smstr(vv->var_value.con_un.con_string, 0))
                        return  1;
        }
        return  0;
}

static void  execute_search()
{
        int     *plist, pcnt, cline, nline;
        int     topitem, visibleitem;

        if  (searchvars)  {
                int     newpos;
                if  (XmListGetSelectedPos(vwid, &plist, &pcnt))  {
                        cline = (plist[0] - 1) / VLINES;
                        XtFree((char *) plist);
                }
                else
                        cline = sbackward? (int) Var_seg.nvars: -1;

                if  (sbackward)  {
                        for  (nline = cline - 1;  nline >= 0;  nline--)
                                if  (varmatch(nline))
                                        goto  gotvar;
                        if  (wraparound)
                                for  (nline = Var_seg.nvars-1;  nline > cline;  nline--)
                                        if  (varmatch(nline))
                                                goto  gotvar;
                        doerror(vwid, $EH{xmbtq var not found backw});
                }
                else  {
                        for  (nline = cline + 1;  nline < Var_seg.nvars;  nline++)
                                if  (varmatch(nline))
                                        goto  gotvar;
                        if  (wraparound)
                                for  (nline = 0;  nline < cline;  nline++)
                                        if  (varmatch(nline))
                                                goto  gotvar;
                        doerror(vwid, $EH{xmbtq var not found forw});
                }
                return;
        gotvar:
                newpos = nline * VLINES + 1;
                XmListSelectPos(vwid, newpos,  False);
                XmListSelectPos(vwid, newpos+1,  False);
                if  (nline != cline)  {
                        XmListDeselectPos(vwid, cline*VLINES+1);
                        XmListDeselectPos(vwid, cline*VLINES+2);
                }
                XtVaGetValues(vwid, XmNtopItemPosition, &topitem, XmNvisibleItemCount, &visibleitem, NULL);
                if  (newpos < topitem)
                        XmListSetPos(vwid, newpos);
                else  if  (newpos >= topitem+visibleitem)
                        XmListSetBottomPos(vwid, newpos+1);
        }
        else  {
                if  (XmListGetSelectedPos(jwid, &plist, &pcnt))  {
                        cline = plist[0] - 1;
                        XtFree((char *) plist);
                }
                else
                        cline = sbackward? (int) Job_seg.njobs: -1;
                if  (sbackward)  {
                        for  (nline = cline - 1;  nline >= 0;  nline--)
                                if  (jobmatch(nline))
                                        goto  gotjob;
                        if  (wraparound)
                                for  (nline = Job_seg.njobs-1;  nline > cline;  nline--)
                                        if  (jobmatch(nline))
                                                goto  gotjob;
                        doerror(jwid, $EH{xmbtq job not found backw});
                }
                else  {
                        for  (nline = cline + 1;  nline < Job_seg.njobs;  nline++)
                                if  (jobmatch(nline))
                                        goto  gotjob;
                        if  (wraparound)
                                for  (nline = 0;  nline < cline;  nline++)
                                        if  (jobmatch(nline))
                                                goto  gotjob;
                        doerror(jwid, $EH{xmbtq job not found forw});
                }
                return;
        gotjob:
                nline++;
                XmListSelectPos(jwid, nline, False);
                XtVaGetValues(jwid, XmNtopItemPosition, &topitem, XmNvisibleItemCount, &visibleitem, NULL);
                if  (nline < topitem)
                        XmListSetPos(jwid, nline);
                else  if  (nline >= topitem+visibleitem)
                        XmListSetBottomPos(jwid, nline);
        }
}

void  InitsearchDlg(Widget parent, Widget *shellp, Widget *panep, Widget *formp, char *existing)
{
        Widget  prevleft;

        CreateEditDlg(parent, "Search", shellp, panep, formp, 3);

        prevleft = XtVaCreateManagedWidget("lookfor",
                                           xmLabelWidgetClass,  *formp,
                                           XmNtopAttachment,    XmATTACH_FORM,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           XmNborderWidth,      0,
                                           NULL);

        workw[WORKW_STXTW] = XtVaCreateManagedWidget("sstring",
                                                     xmTextFieldWidgetClass,    *formp,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNtopAttachment,          XmATTACH_FORM,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             prevleft,
                                                     XmNrightAttachment,        XmATTACH_FORM,
                                                     NULL);
        if  (existing)
                XmTextSetString(workw[WORKW_STXTW], existing);
}

void  InitsearchOpts(Widget formw, Widget after, int sback, int matchc, int wrap)
{
        Widget  dirrc, prevleft;

        dirrc = XtVaCreateManagedWidget("dirn",
                                        xmRowColumnWidgetClass,         formw,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   after,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  2,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               True,
                                        NULL);

        workw[WORKW_FORWW] = XtVaCreateManagedWidget("forward",
                                                     xmToggleButtonGadgetClass, dirrc,
                                                     XmNborderWidth,            0,
                                                     NULL);

        prevleft = XtVaCreateManagedWidget("backward",
                                           xmToggleButtonGadgetClass,   dirrc,
                                           XmNborderWidth,              0,
                                           NULL);

        XmToggleButtonGadgetSetState(sback? prevleft: workw[WORKW_FORWW], True, False);

        dirrc = XtVaCreateManagedWidget("opts",
                                        xmRowColumnWidgetClass,         formw,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   dirrc,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  2,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               False,
                                        NULL);

        workw[WORKW_MATCHW] =  XtVaCreateManagedWidget("match",
                                                       xmToggleButtonGadgetClass,       dirrc,
                                                       XmNborderWidth,                  0,
                                                       NULL);
        if  (matchc)
                XmToggleButtonGadgetSetState(workw[WORKW_MATCHW], True, False);

        workw[WORKW_WRAPW] =  XtVaCreateManagedWidget("wrap",
                                                      xmToggleButtonGadgetClass,        dirrc,
                                                      XmNborderWidth,                   0,
                                                      NULL);
        if  (wrap)
                XmToggleButtonGadgetSetState(workw[WORKW_WRAPW], True, False);

        XtManageChild(formw);
}

static void  changesearch(Widget w, int unused, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                searchvars = 1;
                XtSetSensitive(workw[WORKW_HTXTW], False);
                XtSetSensitive(workw[WORKW_NTXTW], True);
                XtSetSensitive(workw[WORKW_CTXTW], True);
                XtSetSensitive(workw[WORKW_VTXTW], True);
        }
        else  {
                searchvars = 0;
                XtSetSensitive(workw[WORKW_HTXTW], True);
                XtSetSensitive(workw[WORKW_NTXTW], False);
                XtSetSensitive(workw[WORKW_CTXTW], False);
                XtSetSensitive(workw[WORKW_VTXTW], False);
        }
}

static void  endsdlg(Widget w, int data)
{
        if  (data)  {
                sbackward = XmToggleButtonGadgetGetState(workw[WORKW_FORWW])? 0: 1;
                matchcase = XmToggleButtonGadgetGetState(workw[WORKW_MATCHW])? 1: 0;
                wraparound = XmToggleButtonGadgetGetState(workw[WORKW_WRAPW])? 1: 0;
                searchtit = XmToggleButtonGadgetGetState(workw[WORKW_HTXTW])? 1: 0;
                searchuser = XmToggleButtonGadgetGetState(workw[WORKW_UTXTW])? 1: 0;
                searchgroup = XmToggleButtonGadgetGetState(workw[WORKW_GTXTW])? 1: 0;
                searchnam = XmToggleButtonGadgetGetState(workw[WORKW_NTXTW])? 1: 0;
                searchcomm = XmToggleButtonGadgetGetState(workw[WORKW_CTXTW])? 1: 0;
                searchval = XmToggleButtonGadgetGetState(workw[WORKW_VTXTW])? 1: 0;
                if  (searchvars)  {
                        if  (!(searchuser || searchgroup || searchnam || searchcomm || searchval))  {
                                doerror(w, $EH{xmbtq no var search atts});
                                return;
                        }
                }
                else  if  (!(searchtit || searchuser || searchgroup))  {
                        doerror(w, $EH{xmbtq no job search atts});
                        return;
                }
                if  (matchtext) /* Last time round */
                        XtFree(matchtext);
                XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &matchtext, NULL);
                if  (matchtext[0] == '\0')  {
                        doerror(w, $EH{xmbtq null search string});
                        return;
                }
                XtDestroyWidget(GetTopShell(w));
                execute_search();
        }
        else
                XtDestroyWidget(GetTopShell(w));
}

void  cb_rsrch(Widget w, int data)
{
        if  (!matchtext  ||  matchtext[0] == '\0')  {
                doerror(w, $EH{xmbtq no search string yet});
                return;
        }
        sbackward = (char) data;
        execute_search();
}

void  cb_srchfor(Widget parent)
{
        Widget  s_shell, panew, formw, orc, jorv, whp;

        InitsearchDlg(parent, &s_shell, &panew, &formw, matchtext);

        orc = XtVaCreateManagedWidget("sopts",
                                      xmRowColumnWidgetClass,           formw,
                                      XmNtopAttachment,                 XmATTACH_WIDGET,
                                      XmNtopWidget,                     workw[WORKW_STXTW],
                                      XmNleftAttachment,                XmATTACH_FORM,
                                      XmNrightAttachment,               XmATTACH_FORM,
                                      XmNpacking,                       XmPACK_COLUMN,
                                      XmNnumColumns,                    2,
                                      NULL);

        jorv = XtVaCreateManagedWidget("jorv",
                                       xmRowColumnWidgetClass,          orc,
                                       XmNpacking,                      XmPACK_COLUMN,
                                       XmNnumColumns,                   1,
                                       XmNisHomogeneous,                True,
                                       XmNentryClass,                   xmToggleButtonGadgetClass,
                                       XmNradioBehavior,                True,
                                       NULL);

        workw[WORKW_JBUTW] = XtVaCreateManagedWidget("jobs",
                                                     xmToggleButtonGadgetClass, jorv,
                                                     XmNborderWidth,            0,
                                                     NULL);

        workw[WORKW_VBUTW] = XtVaCreateManagedWidget("vars",
                                                     xmToggleButtonGadgetClass, jorv,
                                                     XmNborderWidth,            0,
                                                     NULL);

        XmToggleButtonGadgetSetState(searchvars? workw[WORKW_VBUTW]: workw[WORKW_JBUTW], True, False);
        XtAddCallback(workw[WORKW_VBUTW], XmNvalueChangedCallback, (XtCallbackProc) changesearch, 0);

        whp = XtVaCreateManagedWidget("which",
                                      xmRowColumnWidgetClass,           orc,
                                      XmNpacking,                       XmPACK_COLUMN,
                                      XmNnumColumns,                    1,
                                      XmNisHomogeneous,                 True,
                                      XmNentryClass,                    xmToggleButtonGadgetClass,
                                      XmNradioBehavior,                 False,
                                      NULL);

        workw[WORKW_HTXTW] = XtVaCreateManagedWidget("title",
                                                     xmToggleButtonGadgetClass, whp,
                                                     XmNborderWidth,            0,
                                                     NULL);

        if  (searchtit)
                XmToggleButtonGadgetSetState(workw[WORKW_HTXTW], True, False);

        workw[WORKW_UTXTW] = XtVaCreateManagedWidget("user",
                                                     xmToggleButtonGadgetClass, whp,
                                                     XmNborderWidth,            0,
                                                     NULL);

        if  (searchuser)
                XmToggleButtonGadgetSetState(workw[WORKW_UTXTW], True, False);

        workw[WORKW_GTXTW] = XtVaCreateManagedWidget("group",
                                                     xmToggleButtonGadgetClass, whp,
                                                     XmNborderWidth,            0,
                                                     NULL);

        if  (searchgroup)
                XmToggleButtonGadgetSetState(workw[WORKW_GTXTW], True, False);

        workw[WORKW_NTXTW] = XtVaCreateManagedWidget("name",
                                                     xmToggleButtonGadgetClass, whp,
                                                     XmNborderWidth,            0,
                                                     NULL);

        if  (searchnam)
                XmToggleButtonGadgetSetState(workw[WORKW_NTXTW], True, False);

        workw[WORKW_CTXTW] = XtVaCreateManagedWidget("comment",
                                                     xmToggleButtonGadgetClass, whp,
                                                     XmNborderWidth,            0,
                                                     NULL);
        if  (searchcomm)
                XmToggleButtonGadgetSetState(workw[WORKW_CTXTW], True, False);

        workw[WORKW_VTXTW] = XtVaCreateManagedWidget("value",
                                                     xmToggleButtonGadgetClass, whp,
                                                     XmNborderWidth,            0,
                                                     NULL);
        if  (searchval)
                XmToggleButtonGadgetSetState(workw[WORKW_VTXTW], True, False);

        if  (searchvars)
                XtSetSensitive(workw[WORKW_HTXTW], False);
        else  {
                XtSetSensitive(workw[WORKW_NTXTW], False);
                XtSetSensitive(workw[WORKW_CTXTW], False);
                XtSetSensitive(workw[WORKW_VTXTW], False);
        }

        InitsearchOpts(formw, orc, sbackward, matchcase, wraparound);
        CreateActionEndDlg(s_shell, panew, (XtCallbackProc) endsdlg, $H{xmbtq search dialog});
}
