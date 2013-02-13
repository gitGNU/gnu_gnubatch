/* xm_commlib.c -- common library routines for Motif programs

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
#include <X11/cursorfont.h>
#include <Xm/ArrowB.h>
#include <Xm/DialogS.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
#include <Xm/ComboBox.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "bjparam.h"
#include "cmdint.h"
#include "xm_commlib.h"

static  char    Filename[] = __FILE__;

XtIntervalId    arrow_timer;
int     arr_rtime, arr_rint;
#ifndef HAVE_XM_SPINB_H
unsigned        arrow_min,
                arrow_max,
                arrow_lng;
#endif

Widget  place_label_topleft(Widget form, char *widname)
{
        return  XtVaCreateManagedWidget(widname,
                                        xmLabelGadgetClass,     form,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        NULL);
}

Widget  place_label_left(Widget form, Widget above, char *widname)
{
        return  XtVaCreateManagedWidget(widname,
                                        xmLabelGadgetClass,     form,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           above,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        NULL);
}

Widget  place_label_right(Widget form, Widget above, char *widname)
{
        return  XtVaCreateManagedWidget(widname,
                                        xmLabelGadgetClass,     form,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           above,
                                        XmNrightAttachment,     XmATTACH_FORM,
                                        NULL);
}

Widget  place_label_top(Widget form, Widget left, char *widname)
{
        return  XtVaCreateManagedWidget(widname,
                                        xmLabelGadgetClass,     form,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_WIDGET,
                                        XmNleftWidget,          left,
                                        NULL);
}

Widget  place_label(Widget form, Widget above, Widget left, char *widname)
{
        return  XtVaCreateManagedWidget(widname,
                                        xmLabelGadgetClass,     form,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           above,
                                        XmNleftAttachment,      XmATTACH_WIDGET,
                                        XmNleftWidget,          left,
                                        NULL);
}

#ifdef HAVE_XM_SPINB_H
int  get_spinbox_int(Widget wid)
{
#ifdef  BROKEN_SPINBOX
        int     result, mini;
        XtVaGetValues(wid, XmNposition, &result, XmNminimumValue, &mini, NULL);
        return  result + mini;
#else
        int     result;
        XtVaGetValues(wid, XmNposition, &result, NULL);
        return  result;
#endif
}

void  put_spinbox_int(Widget wid, const int val)
{
#ifdef  BROKEN_SPINBOX
        int     mini;
        XtVaGetValues(wid, XmNminimumValue, &mini, NULL);
        XtVaSetValues(wid, XmNposition, val-mini, NULL);
#else
        XtVaSetValues(wid, XmNposition, val, NULL);
#endif
}
#endif

int  get_textbox_int(Widget wid)
{
        int     result;
        char    *txt;

        XtVaGetValues(wid, XmNvalue, &txt, NULL);
        result = atoi(txt);
        XtFree(txt);
        return  result;
}

void  put_textbox_int(Widget wid, const int val, int width)
{
        char    nbuf[20];
        if  (width >= sizeof(nbuf)-1)
                width = 0;
        sprintf(nbuf, "%*d", width, val);
        XmTextSetString(wid, nbuf);
}

Widget  GetTopShell(Widget w)
{
        while  (w && !XtIsWMShell(w))
                w = XtParent(w);
        return  w;
}

Widget  FindWidget(Widget w)
{
        while  (w && !XtIsWidget(w))
                w = XtParent(w);
        return  w;
}

static void  response(Widget w, int *answer, XmAnyCallbackStruct *cbs)
{
        switch  (cbs->reason)  {
        case  XmCR_OK:
                *answer = 1;
                break;
        case  XmCR_CANCEL:
                *answer = 0;
                break;
        }
        XtDestroyWidget(w);
}

int  Confirm(Widget parent, int code)
{
        Widget  dlg;
        static  int     answer;
        char    *msg;
        XmString        text;

        dlg = XmCreateQuestionDialog(FindWidget(parent), "Confirm", NULL, 0);
        XtVaSetValues(dlg,
                      XmNdialogStyle,           XmDIALOG_FULL_APPLICATION_MODAL,
                      NULL);
        XtAddCallback(dlg, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(code));
        XtAddCallback(dlg, XmNokCallback, (XtCallbackProc) response, &answer);
        XtAddCallback(dlg, XmNcancelCallback, (XtCallbackProc) response, &answer);
        answer = -1;
        msg = gprompt(code);
        text = XmStringCreateSimple(msg);
        free(msg);
        XtVaSetValues(dlg,
                      XmNmessageString,         text,
                      XmNdefaultButtonType,     XmDIALOG_CANCEL_BUTTON,
                      NULL);
        XmStringFree(text);
        XtManageChild(dlg);
        XtPopup(XtParent(dlg), XtGrabNone);
        while  (answer < 0)
                XtAppProcessEvent(app, XtIMAll);
        return  answer;
}

char *makebigvec(char **mat)
{
        unsigned  totlen = 0, len;
        char    **ep, *newstr, *pos;

        for  (ep = mat;  *ep;  ep++)
                totlen += strlen(*ep) + 1;

        newstr = malloc((unsigned) totlen);
        if  (!newstr)
                ABORT_NOMEM;
        pos = newstr;
        for  (ep = mat;  *ep;  ep++)  {
                len = strlen(*ep);
                strcpy(pos, *ep);
                free(*ep);
                pos += len;
                *pos++ = '\n';
        }
        pos[-1] = '\0';
        free((char *) mat);
        return  newstr;
}

void  dohelp(Widget wid, int helpcode)
{
        char    **evec = helpvec(helpcode, 'H'), *newstr;
        Widget          ew;
        if  (!evec[0])  {
                disp_arg[9] = helpcode;
                free((char *) evec);
                evec = helpvec($E{Missing help code}, 'E');
        }
        ew = XmCreateInformationDialog(FindWidget(wid), "help", NULL, 0);
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

void  doerror(Widget wid, int errnum)
{
        char    **evec = helpvec(errnum, 'E'), *newstr;
        Widget          ew;
        if  (!evec[0])  {
                disp_arg[9] = errnum;
                free((char *) evec);
                evec = helpvec($EH{Missing error code}, 'E');
        }
        ew = XmCreateErrorDialog(FindWidget(wid), "error", NULL, 0);
        XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
        XtAddCallback(ew, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(errnum));
        newstr = makebigvec(evec);
        XtVaSetValues(ew,
                      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
                      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
                      NULL);
        free(newstr);
        XtManageChild(ew);
        XtPopup(XtParent(ew), XtGrabNone);
}

/* Create pair of up/down arrows with appropriate callbacks.  */

Widget  CreateArrowPair(char *name, Widget formw, Widget topw, Widget leftw, XtCallbackProc upcall, XtCallbackProc dncall, int updata, int dndata)
{
        Widget  uparrow, dnarrow;
        char    fullname[10];

        sprintf(fullname, "%sup", name);
        if  (topw)  {
                uparrow = XtVaCreateManagedWidget(fullname,
                                                  xmArrowButtonWidgetClass,     formw,
                                                  XmNtopAttachment,             XmATTACH_WIDGET,
                                                  XmNtopWidget,                 topw,
                                                  XmNleftAttachment,            XmATTACH_WIDGET,
                                                  XmNleftWidget,                leftw,
                                                  XmNarrowDirection,            XmARROW_UP,
                                                  XmNborderWidth,               0,
                                                  XmNrightOffset,               0,
                                                  NULL);

                sprintf(fullname, "%sdn", name);

                dnarrow = XtVaCreateManagedWidget(fullname,
                                                  xmArrowButtonWidgetClass,     formw,
                                                  XmNtopAttachment,             XmATTACH_WIDGET,
                                                  XmNtopWidget,                 topw,
                                                  XmNleftAttachment,            XmATTACH_WIDGET,
                                                  XmNleftWidget,                uparrow,
                                                  XmNarrowDirection,            XmARROW_DOWN,
                                                  XmNborderWidth,               0,
                                                  XmNleftOffset,                0,
                                                  NULL);
        }
        else  {
                uparrow = XtVaCreateManagedWidget(fullname,
                                                  xmArrowButtonWidgetClass,     formw,
                                                  XmNtopAttachment,             XmATTACH_FORM,
                                                  XmNleftAttachment,            XmATTACH_WIDGET,
                                                  XmNleftWidget,                leftw,
                                                  XmNarrowDirection,            XmARROW_UP,
                                                  XmNborderWidth,               0,
                                                  XmNrightOffset,               0,
                                                  NULL);

                sprintf(fullname, "%sdn", name);

                dnarrow = XtVaCreateManagedWidget(fullname,
                                                  xmArrowButtonWidgetClass,     formw,
                                                  XmNtopAttachment,             XmATTACH_FORM,
                                                  XmNleftAttachment,            XmATTACH_WIDGET,
                                                  XmNleftWidget,                uparrow,
                                                  XmNarrowDirection,            XmARROW_DOWN,
                                                  XmNborderWidth,               0,
                                                  XmNleftOffset,                0,
                                                  NULL);
        }

        XtAddCallback(uparrow, XmNarmCallback, (XtCallbackProc) upcall, INT_TO_XTPOINTER(updata));
        XtAddCallback(dnarrow, XmNarmCallback, (XtCallbackProc) dncall, INT_TO_XTPOINTER(dndata));
        XtAddCallback(uparrow, XmNdisarmCallback, (XtCallbackProc) upcall, (XtPointer) 0);
        XtAddCallback(dnarrow, XmNdisarmCallback, (XtCallbackProc) dncall, (XtPointer) 0);
        return  dnarrow;
}

#ifndef HAVE_XM_SPINB_H

/* Generic callback for increment arrows.
   id is NULL the first time to get a different timeout
   The passed parameter is the Widget we are mangling */

void  arrow_incr(Widget w, XtIntervalId *id)
{
        int   newval = get_textbox_int(w) + 1;
        if  (newval > (int) arrow_max)
                return;
        put_textbox_int(w, newval, arrow_lng);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) arrow_incr, (XtPointer) w);
}

/* Ditto for decrement.  */

void  arrow_decr(Widget w, XtIntervalId *id)
{
        int     newval = get_textbox_int(w) - 1;
        if  (newval < (int) arrow_min)
                return;
        put_textbox_int(w, newval, arrow_lng);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) arrow_decr, (XtPointer) w);
}
#endif

static void  enddseld(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        char    *dirname;

        if  (data)  {
                int     n;
                if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &dirname))
                        return;
                if  (!dirname  ||  dirname[0] == '\0')
                        return;
                if  (dirname[n = strlen(dirname) - 1] == '/')
                        dirname[n] = '\0';
                XmTextSetString(workw[WORKW_DIRTXTW], dirname);
                XtFree(dirname);
        }
        XtDestroyWidget(w);
}

static void  selectdir(Widget w)
{
        Widget          dseld;
        char            *txt;
        XmString        str;
        Arg             args[2];
        struct  stat    sbuf;

        XtVaGetValues(workw[WORKW_DIRTXTW], XmNvalue, &txt, NULL);
        if  (txt[0] != '/'  ||  stat(txt, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)
                str = XmStringCreateSimple(Curr_pwd);
        else
                str = XmStringCreateSimple(txt);
        XtFree(txt);
        XtSetArg(args[0], XmNdirectory, str);
        dseld = XmCreateFileSelectionDialog(FindWidget(w), "dselb", args, 1);
        XmStringFree(str);
        XtUnmanageChild(XtParent(XmFileSelectionBoxGetChild(dseld, XmDIALOG_LIST)));
        XtUnmanageChild(XmFileSelectionBoxGetChild(dseld, XmDIALOG_LIST_LABEL));
        XtAddCallback(dseld, XmNcancelCallback, (XtCallbackProc) enddseld, (XtPointer) 0);
        XtAddCallback(dseld, XmNokCallback, (XtCallbackProc) enddseld, (XtPointer) 1);
        XtAddCallback(dseld, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtq dir sel dialog});
        XtManageChild(dseld);
}

Widget  CreateDselPane(Widget mainform, Widget prevabove, char *existing)
{
        Widget          dselb;

        place_label_left(mainform, prevabove, "Directory");
        dselb = XtVaCreateManagedWidget("dselect",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        NULL);

        prevabove =
                workw[WORKW_DIRTXTW] =
                        XtVaCreateManagedWidget("dir",
                                                xmTextFieldWidgetClass,         mainform,
                                                XmNcursorPositionVisible,       False,
                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                XmNtopWidget,                   dselb,
                                                XmNleftAttachment,              XmATTACH_FORM,
                                                XmNrightAttachment,             XmATTACH_FORM,
                                                NULL);

        XmTextSetString(prevabove, existing);
        XtAddCallback(dselb, XmNactivateCallback, (XtCallbackProc) selectdir, 0);
        return  prevabove;
}

int  sort_qug(char **a, char **b)
{
        return  strcmp(*a, *b);
}

void    getitemsel(Widget parent, int work, int nullok, char *name, char **(*genfn)(), void (*cbfn)())
{
        Widget          dw, ww = workw[work];
        int             nitems, cnt;
        char            *existing;
        char            **list = (*genfn)((char *) 0);
        XmString        existstr;

        XtVaGetValues(ww, XmNvalue, &existing, NULL);
        existstr = XmStringCreateSimple(existing);
        XtFree(existing);
        count_hv(list, &nitems, &cnt);
        dw = XmCreateSelectionDialog(FindWidget(parent), name, NULL, 0);

        if  (nitems <= 0)  {
                XtVaSetValues(dw,
                              XmNlistItemCount, 0,
                              XmNtextString,    existstr,
                              NULL);
        }
        else  {
                XmString  *strlist = (XmString *) XtMalloc(nitems * sizeof(XmString));
                if  (work != WORKW_ITXTW)
                        qsort(QSORTP1 list, nitems, sizeof(char *), QSORTP4 sort_qug);
                for  (cnt = 0;  cnt < nitems;  cnt++)  {
                        strlist[cnt] = XmStringCreateSimple(list[cnt]);
                        free(list[cnt]);
                }
                free((char *) list);
                XtVaSetValues(dw,
                              XmNlistItems,     strlist,
                              XmNlistItemCount, nitems,
                              XmNtextString,    existstr,
                              NULL);
                for  (cnt = 0;  cnt < nitems;  cnt++)
                        XmStringFree(strlist[cnt]);
                XtFree((XtPointer) strlist);
        }
        XmStringFree(existstr);
        XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_APPLY_BUTTON));
        XtUnmanageChild(XmSelectionBoxGetChild(dw, XmDIALOG_HELP_BUTTON));
        XtAddCallback(dw, XmNokCallback, cbfn, INT_TO_XTPOINTER(nullok));
        XtManageChild(dw);
}

#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
void  ilist_cb(Widget w, int data, XmComboBoxCallbackStruct *cbs)
{
        int     cinumber = cbs->item_position;
        char    nbuf[6];
        sprintf(nbuf, "%5u", Ci_list[cinumber].ci_ll);
        XmTextSetString(workw[WORKW_LLTXTW], nbuf);
}
#else
void  ilist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
        char    *value;
        int     cnt;

        XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
        if  (!value[0])  {      /* Never right to be null */
                doerror(w, $EH{xmbtq null ci name});
                XtFree(value);
                return;
        }
        XmTextSetString(workw[WORKW_ITXTW], value);
        if  ((cnt = validate_ci(value)) >= 0)  {
                char    nbuf[6];
                sprintf(nbuf, "%5u", Ci_list[cnt].ci_ll);
                XmTextSetString(workw[WORKW_LLTXTW], nbuf);
        }
        XtFree(value);
        XtDestroyWidget(w);
}

static void  qlist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
        char    *value;

        XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
        if  (!value[0]  &&  !nullok)  {
                doerror(w, $EH{xmbtq null queue name});
                XtFree(value);
                return;
        }
        XmTextSetString(workw[WORKW_QTXTW], value);
        XtFree(value);
        XtDestroyWidget(w);
}

static void  ulist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
        char    *value;

        XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
        if  (!value[0]  &&  !nullok)  {
                doerror(w, $EH{xmbtq null user name});
                XtFree(value);
                return;
        }
        XmTextSetString(workw[WORKW_UTXTW], value);
        XtFree(value);
        XtDestroyWidget(w);
}

static void  glist_cb(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
        char    *value;

        XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &value);
        if  (!value[0]  &&  !nullok)  {
                doerror(w, $EH{xmbtq null group name});
                XtFree(value);
                return;
        }
        XmTextSetString(workw[WORKW_GTXTW], value);
        XtFree(value);
        XtDestroyWidget(w);
}

void  getqueuesel(Widget w, int nullok)
{
        getitemsel(w, WORKW_QTXTW, nullok, "qselect", gen_qlist, qlist_cb);
}

void  getintsel(Widget w, int nullok)
{
        getitemsel(w, WORKW_ITXTW, nullok, "iselect", listcis, ilist_cb);
}

void  getusersel(Widget w, int nullok)
{
        getitemsel(w, WORKW_UTXTW, nullok, "uselect", gen_ulist, ulist_cb);
}

void  getgroupsel(Widget w, int nullok)
{
        getitemsel(w, WORKW_GTXTW, nullok, "gselect", gen_glist, glist_cb);
}
#endif

void  cb_chelp(Widget w, int data, XmAnyCallbackStruct *cbs)
{
        Widget  help_w;
        Cursor  cursor;

        cursor = XCreateFontCursor(dpy, XC_hand2);
        if  ((help_w = XmTrackingLocate(toplevel, cursor, False))  &&
             XtHasCallbacks(help_w, XmNhelpCallback) == XtCallbackHasSome)  {
                cbs->reason = XmCR_HELP;
                XtCallCallbacks(help_w, XmNhelpCallback, cbs);
        }
        XFreeCursor(dpy, cursor);
}

#define INCSILIST       3
#define NULLENTRY(n)    (Ci_list[n].ci_name[0] == '\0')

void  initcifile()
{
        int     ret;

        if  ((ret = open_ci(O_RDWR)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
}

char **listcis(char *prefix)
{
        char    **result;
        unsigned        cicnt, maxr, countr;
        int     sfl = 0;

        if  (prefix)  {
                sfl = strlen(prefix);
                maxr = (Ci_num + 1) / 2;
        }
        else
                maxr = Ci_num;

        if  ((result = (char **) malloc(maxr * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        countr = 0;

        for  (cicnt = 0;  cicnt < Ci_num;  cicnt++)  {

                if  (NULLENTRY(cicnt))
                        continue;

                /* Skip ones which don't match the prefix.  */

                if  (strncmp(Ci_list[cicnt].ci_name, prefix, (unsigned) sfl) != 0)
                        continue;

                if  (countr + 1 >= maxr)  {
                        maxr += INCSILIST;
                        if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
                                ABORT_NOMEM;
                }
                result[countr++] = stracpy(Ci_list[cicnt].ci_name);
        }

        if  (countr == 0)  {
                free((char *) result);
                return  (char **) 0;
        }

        result[countr] = (char *) 0;
        return  result;
}
