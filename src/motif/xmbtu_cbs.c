/* xmbtu_cbs.c -- callback routines for gbch-xmuser

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
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
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
#include <Xm/Scale.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "defaults.h"
#include "btmode.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "files.h"
#include "xm_commlib.h"
#include "xmbtu_ext.h"

Widget  workw[9];
static  Widget  modew[66];

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

/* Positions (in the motif sense) of users we are thinking of mangling
   If null/zero then we are looking at the default list.  */

static  int     pendunum,
                *pendulist;

char **gen_qlist(char *unused)
{
        return  0;              /* Needed for linking with xm_commlib */
}

void  displaybusy(const int on)
{
        static  Cursor  cursor;
        XSetWindowAttributes    attrs;
        if  (!cursor)
                cursor = XCreateFontCursor(dpy, XC_watch);
        attrs.cursor = on? cursor: None;
        XChangeWindowAttributes(dpy, XtWindow(toplevel), CWCursor, &attrs);
        XFlush(dpy);
}

static int  getselectedusers(const int moan)
{
        int     nu, *nulist;

        if  (XmListGetSelectedPos(uwid, &nulist, &nu))  {
                if  (nu <= 0)  {
                        XtFree((XtPointer) nulist);
                        pendunum = 0;
                        pendulist = (int *) 0;
                        if  (moan)
                                doerror(uwid, $EH{xmbtuser no users selected});
                        return  0;
                }
                pendunum = nu;
                pendulist = nulist;
                return  1;
        }
        else  {
                if  (moan)
                        doerror(uwid, $EH{xmbtuser no users selected});
                pendulist = (int *) 0;
                pendunum = 0;
                return  0;
        }
}

/* Create the stuff at the beginning of a dialog */

void  CreateEditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres, const int nbutts)
{
        int     n = 0;
        Arg     arg[7];
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
                                    XmNfractionBase,            2 * nbutts,
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

/* Create the stuff at the beginning a dialog which can relate to a
   default value or a (group of) user(s).  */

static Widget  CreateUEditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres)
{
        char    resname[UIDSIZE*2 + 10];

        sprintf(resname, "%s%s", pendunum > 0? "u": "def", dlgname);
        CreateEditDlg(parent, resname, dlgres, paneres, formres, 3);
        if  (pendunum > 0)  {
                Widget  tit, result;
                tit = XtVaCreateManagedWidget("useredit",
                                              xmLabelWidgetClass,               *formres,
                                              XmNtopAttachment,                 XmATTACH_FORM,
                                              XmNleftAttachment,                XmATTACH_FORM,
                                              XmNborderWidth,                   0,
                                              NULL);
                result = XtVaCreateManagedWidget("userlist",
                                                 xmTextFieldWidgetClass,        *formres,
                                                 XmNcursorPositionVisible,      False,
                                                 XmNtopAttachment,              XmATTACH_FORM,
                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                 XmNleftWidget,                 tit,
                                                 XmNrightAttachment,            XmATTACH_FORM,
                                                 XmNeditable,                   False,
                                                 XmNborderWidth,                0,
                                                 NULL);
                if  (pendunum > 1)
                        sprintf(resname, "%s - %s",
                                       prin_uname((uid_t) ulist[pendulist[0] - 1].btu_user),
                                       prin_uname((uid_t) ulist[pendulist[pendunum-1] - 1].btu_user));
                else
                        sprintf(resname, "%s",
                                       prin_uname((uid_t) ulist[pendulist[0] - 1].btu_user));
                XmTextSetString(result, resname);
                return  result;
        }
        else
                return  XtVaCreateManagedWidget("defedit",
                                                xmLabelWidgetClass,             *formres,
                                                XmNtopAttachment,               XmATTACH_FORM,
                                                XmNleftAttachment,              XmATTACH_FORM,
                                                XmNborderWidth,                 0,
                                                NULL);
}

static void  enddispopt(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                int     ret;
                ret = XmToggleButtonGadgetGetState(workw[WORKW_SORTU])? SORT_USER:
                        XmToggleButtonGadgetGetState(workw[WORKW_SORTG])? SORT_GROUP: SORT_NONE;

                XmScaleGetValue(workw[WORKW_LOADLS], (int *) &loadstep);
                if  (loadstep >= 10000)
                        loadstep = ((loadstep + 500) / 1000) * 1000;
                else  if  (loadstep >= 1000)
                        loadstep = ((loadstep + 50) / 100) * 100;
                else  if  (loadstep >= 100)
                        loadstep = ((loadstep + 5) / 10) * 10;

                if  (ret != alphsort)  {
                        alphsort = (char) ret;
                        switch  (alphsort)  {
                        default:
                        case  SORT_NONE:
                                qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_id);
                                break;
                        case  SORT_USER:
                                qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_u);
                                break;
                        case  SORT_GROUP:
                                qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_g);
                                break;
                        }
                        udisplay(0, (int *) 0);
                }
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_disporder(Widget parent)
{
        Widget  disp_shell, paneview, mainform, butts, sortu, prevabove;

        CreateEditDlg(parent, "Disporder", &disp_shell, &paneview, &mainform, 3);
        butts = XtVaCreateManagedWidget("butts",
                                        xmRowColumnWidgetClass, mainform,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNrightAttachment,     XmATTACH_FORM,
                                        XmNpacking,             XmPACK_COLUMN,
                                        XmNnumColumns,          1,
                                        XmNisHomogeneous,       True,
                                        XmNentryClass,          xmToggleButtonGadgetClass,
                                        XmNradioBehavior,       True,
                                        NULL);

        sortu = XtVaCreateManagedWidget("sortuid",
                                        xmToggleButtonGadgetClass,
                                        butts,
                                        NULL);
        workw[WORKW_SORTU] = XtVaCreateManagedWidget("sortuser",
                                                     xmToggleButtonGadgetClass,
                                                     butts,
                                                     NULL);
        workw[WORKW_SORTG] = XtVaCreateManagedWidget("sortgroup",
                                                     xmToggleButtonGadgetClass,
                                                     butts,
                                                     NULL);

        XmToggleButtonGadgetSetState(alphsort == SORT_USER? workw[WORKW_SORTU]:
                                     alphsort == SORT_GROUP? workw[WORKW_SORTG]: sortu, True, False);

        prevabove = place_label_left(mainform, butts, "llstep");
        workw[WORKW_LOADLS] = XtVaCreateManagedWidget("loadls",
                                                      xmScaleWidgetClass,       mainform,
                                                      XmNtopAttachment,         XmATTACH_WIDGET,
                                                      XmNtopWidget,             prevabove,
                                                      XmNleftAttachment,        XmATTACH_FORM,
                                                      XmNrightAttachment,       XmATTACH_FORM,
                                                      XmNorientation,           XmHORIZONTAL,
                                                      XmNminimum,               1,
                                                      XmNmaximum,               10000,
                                                      XmNvalue,                 loadstep,
                                                      XmNshowValue,             True,
                                                      NULL);

        XtManageChild(mainform);
        CreateActionEndDlg(disp_shell, paneview, (XtCallbackProc) enddispopt, $H{xmbtuser options dialog});
}

static void  endprio(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                int             cmin, cmax, cdef;
                XmScaleGetValue(workw[WORKW_MINPW], &cmin);
                XmScaleGetValue(workw[WORKW_MAXPW], &cmax);
                XmScaleGetValue(workw[WORKW_DEFPW], &cdef);
                if  (cmin > cmax)  {
                        doerror(w, $EH{xmbtuser minp gt maxp});
                        return;
                }
                if  (cdef < cmin)  {
                        doerror(w, $EH{xmbtuser defp lt minp});
                        return;
                }
                if  (cdef > cmax)  {
                        doerror(w, $EH{xmbtuser defp gt maxp});
                        return;
                }
                if  (pendunum > 0)  {
                        int     ucnt;
                        BtuserRef       uitem;
                        uchanges++;
                        for  (ucnt = 0;  ucnt < pendunum;  ucnt++)  {
                                uitem = &ulist[pendulist[ucnt]-1];
                                uitem->btu_minp = (unsigned char) cmin;
                                uitem->btu_maxp = (unsigned char) cmax;
                                uitem->btu_defp = (unsigned char) cdef;
                        }
                        udisplay(pendunum, pendulist);
                }
                else  {
                        hchanges++;
                        Btuhdr.btd_minp = (unsigned char) cmin;
                        Btuhdr.btd_maxp = (unsigned char) cmax;
                        Btuhdr.btd_defp = (unsigned char) cdef;
                        defdisplay();
                }
        }
        XtDestroyWidget(GetTopShell(w));
}

/* Priorities which = 0 to set default, 1 to selected users */

void  cb_pris(Widget parent, int which)
{
        Widget  pri_shell, paneview, mainform, prevabove;
        int     cmin, cmax, cdef;

        if  (pendunum > 0)  {
                pendunum = 0;
                XtFree((char *) pendulist);
                pendulist = (int *) 0;
        }

        if  (which)  {
                BtuserRef       fu;
                if  (!getselectedusers(1))
                        return;
                fu = &ulist[pendulist[0]-1];
                cmin = fu->btu_minp;
                cmax = fu->btu_maxp;
                cdef = fu->btu_defp;
        }
        else  {
                cmin = Btuhdr.btd_minp;
                cmax = Btuhdr.btd_maxp;
                cdef = Btuhdr.btd_defp;
        }

        prevabove = CreateUEditDlg(parent, "pri", &pri_shell, &paneview, &mainform);
        prevabove = place_label_left(mainform, prevabove, "min");
        prevabove =
                workw[WORKW_MINPW] =
                        XtVaCreateManagedWidget("Minp",
                                                xmScaleWidgetClass,     mainform,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_FORM,
                                                XmNrightAttachment,     XmATTACH_FORM,
                                                XmNorientation,         XmHORIZONTAL,
                                                XmNminimum,             1,
                                                XmNmaximum,             255,
                                                XmNvalue,               cmin,
                                                XmNshowValue,           True,
                                                NULL);

        prevabove = place_label_left(mainform, prevabove, "def");
        prevabove =
                workw[WORKW_DEFPW] =
                        XtVaCreateManagedWidget("Defp",
                                                xmScaleWidgetClass,     mainform,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_FORM,
                                                XmNrightAttachment,     XmATTACH_FORM,
                                                XmNorientation,         XmHORIZONTAL,
                                                XmNminimum,             1,
                                                XmNmaximum,             255,
                                                XmNvalue,               cdef,
                                                XmNshowValue,           True,
                                                NULL);

        prevabove = place_label_left(mainform, prevabove, "max");
        prevabove =
                workw[WORKW_MAXPW] =
                        XtVaCreateManagedWidget("Maxp",
                                                xmScaleWidgetClass,     mainform,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_FORM,
                                                XmNrightAttachment,     XmATTACH_FORM,
                                                XmNorientation,         XmHORIZONTAL,
                                                XmNminimum,             1,
                                                XmNmaximum,             255,
                                                XmNvalue,               cmax,
                                                XmNshowValue,           True,
                                                NULL);

        XtManageChild(mainform);
        CreateActionEndDlg(pri_shell, paneview, (XtCallbackProc) endprio, which ? $H{xmbtuser user prio dialog}: $H{xmbtuser default prio dialog});
}

static void  endload(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                unsigned        cmax, ctot, cspec;
                XmScaleGetValue(workw[WORKW_MAXLLPW], (int *) &cmax);
                XmScaleGetValue(workw[WORKW_TOTLLPW], (int *) &ctot);
                XmScaleGetValue(workw[WORKW_SPECLLPW], (int *) &cspec);
                cmax = (cmax / loadstep) * loadstep;
                ctot = (ctot / loadstep) * loadstep;
                cspec = (cspec / loadstep) * loadstep;
                if  (cmax > ctot)  {
                        doerror(w, $EH{xmbtuser maxll gt totll});
                        return;
                }
                if  (cspec > cmax)  {
                        doerror(w, $EH{xmbtuser specll gt maxll});
                        return;
                }
                if  (cspec > ctot)  {
                        doerror(w, $EH{xmbtuser specll gt totll});
                        return;
                }
                if  (pendunum > 0)  {
                        int     ucnt;
                        BtuserRef       uitem;
                        uchanges++;
                        for  (ucnt = 0;  ucnt < pendunum;  ucnt++)  {
                                uitem = &ulist[pendulist[ucnt]-1];
                                uitem->btu_maxll = (USHORT) cmax;
                                uitem->btu_totll = (USHORT) ctot;
                                uitem->btu_spec_ll = (USHORT) cspec;
                        }
                        udisplay(pendunum, pendulist);
                }
                else  {
                        hchanges++;
                        Btuhdr.btd_maxll = (USHORT) cmax;
                        Btuhdr.btd_totll = (USHORT) ctot;
                        Btuhdr.btd_spec_ll = (USHORT) cspec;
                        defdisplay();
                }
        }
        XtDestroyWidget(GetTopShell(w));
}

/* Load levels which = 0 to set default, 1 to selected users */

void  cb_loadlev(Widget parent, int which)
{
        Widget  load_shell, paneview, mainform, prevabove;
        unsigned        cmax, ctot, cspec;

        if  (pendunum > 0)  {
                pendunum = 0;
                XtFree((char *) pendulist);
                pendulist = (int *) 0;
        }

        if  (which)  {
                BtuserRef       fu;
                if  (!getselectedusers(1))
                        return;
                fu = &ulist[pendulist[0]-1];
                cmax = fu->btu_maxll;
                ctot = fu->btu_totll;
                cspec = fu->btu_spec_ll;
        }
        else  {
                cmax = Btuhdr.btd_maxll;
                ctot = Btuhdr.btd_totll;
                cspec = Btuhdr.btd_spec_ll;
        }

        prevabove = CreateUEditDlg(parent, "loadlev", &load_shell, &paneview, &mainform);

        prevabove = place_label_left(mainform, prevabove, "max");
        prevabove =
                workw[WORKW_MAXLLPW] =
                        XtVaCreateManagedWidget("Maxll",
                                                xmScaleWidgetClass,     mainform,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_FORM,
                                                XmNrightAttachment,     XmATTACH_FORM,
                                                XmNorientation,         XmHORIZONTAL,
                                                XmNminimum,             100,
                                                XmNmaximum,             60000,
                                                XmNvalue,               cmax,
                                                XmNshowValue,           True,
                                                NULL);

        prevabove = place_label_left(mainform, prevabove, "tot");
        prevabove =
                workw[WORKW_TOTLLPW] =
                        XtVaCreateManagedWidget("Totll",
                                                xmScaleWidgetClass,     mainform,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_FORM,
                                                XmNrightAttachment,     XmATTACH_FORM,
                                                XmNorientation,         XmHORIZONTAL,
                                                XmNminimum,             100,
                                                XmNmaximum,             60000,
                                                XmNvalue,               ctot,
                                                XmNshowValue,           True,
                                                NULL);

        prevabove = place_label_left(mainform, prevabove, "spec");
        prevabove =
                workw[WORKW_SPECLLPW] =
                        XtVaCreateManagedWidget("Specll",
                                                xmScaleWidgetClass,     mainform,
                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                XmNtopWidget,           prevabove,
                                                XmNleftAttachment,      XmATTACH_FORM,
                                                XmNrightAttachment,     XmATTACH_FORM,
                                                XmNorientation,         XmHORIZONTAL,
                                                XmNminimum,             100,
                                                XmNmaximum,             60000,
                                                XmNvalue,               cspec,
                                                XmNshowValue,           True,
                                                NULL);

        XtManageChild(mainform);
        CreateActionEndDlg(load_shell, paneview, (XtCallbackProc) endload, which ? $H{xmbtuser user llev dialog}: $H{xmbtuser default llev dialog});
}

static void  endmode(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                int     col, row;
                USHORT   jmode[3], vmode[3];

                for  (col = 0;  col < 3;  col++)  {
                        jmode[col] = 0;
                        for  (row = 0;  row < 11;  row++)
                                if  (XmToggleButtonGadgetGetState(modew[col*11 + row]))
                                        jmode[col] |= 1 << row;
                }
                for  (col = 0;  col < 3;  col++)  {
                        vmode[col] = 0;
                        for  (row = 0;  row < 10;  row++)
                                if  (XmToggleButtonGadgetGetState(modew[col*11 + row + 33]))
                                        vmode[col] |= 1 << row;
                }

                if  (pendunum > 0)  {
                        int             ucnt;
                        BtuserRef       uitem;
                        uchanges++;
                        for  (ucnt = 0;  ucnt < pendunum;  ucnt++)  {
                                uitem = &ulist[pendulist[ucnt]-1];
                                for  (col = 0;  col < 3;  col++)  {
                                        uitem->btu_jflags[col] = jmode[col];
                                        uitem->btu_vflags[col] = vmode[col];
                                }
                        }
                        /* no udisplay(pendunum, pendulist); as not affected */
                }
                else  {
                        hchanges++;
                        for  (col = 0;  col < 3;  col++)  {
                                Btuhdr.btd_jflags[col] = jmode[col];
                                Btuhdr.btd_vflags[col] = vmode[col];
                        }
                        /* no defdisplay(); as not affected */
                }
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  mdtoggle(Widget parent, int which, XmToggleButtonCallbackStruct *cbs)
{
        int     isvar = which / 33, ugo = (which / 11) % 3, wflg = which % 11, cnt;

        if  (cbs->set)  {
                if  (modenames[wflg].sflg)  {
                        for  (cnt = 0;  cnt < 11;  cnt++)
                                if  (modenames[wflg].sflg & (1 << cnt))
                                        XmToggleButtonGadgetSetState(modew[isvar*33+ugo*11+cnt], True, False);
                }
        }
        else  if  (modenames[wflg].rflg)  {
                for  (cnt = 0;  cnt < 11;  cnt++)
                        if  (modenames[wflg].rflg & (1 << cnt))
                                XmToggleButtonGadgetSetState(modew[isvar*33+ugo*11+cnt], False, False);
        }
}

static void  setdef(Widget parent)
{
        int     row, col;

        for  (col = 0;  col < 3;  col++)
                for  (row = 0;  row < 11;  row++)
                        XmToggleButtonGadgetSetState(modew[col*11 + row],
                                                     Btuhdr.btd_jflags[col] & (1 << row)? True: False,
                                                     False);
        for  (col = 0;  col < 3;  col++)
                for  (row = 0;  row < 10;  row++)
                        XmToggleButtonGadgetSetState(modew[col*11 + row + 33],
                                                     Btuhdr.btd_vflags[col] & (1 << row)? True: False,
                                                     False);
}

void  cb_mode(Widget parent, int which)
{
        Widget  mode_shell, paneview, mainform, prevabove, mainrc, jobrc, varrc;
        int     cnt, row, col;
        USHORT   jmode[3], vmode[3];

        if  (pendunum > 0)  {
                pendunum = 0;
                XtFree((char *) pendulist);
                pendulist = (int *) 0;
        }

        if  (which)  {
                if  (!getselectedusers(1))
                        return;
                for  (cnt = 0;  cnt < 3;  cnt++)  {
                        jmode[cnt] = ulist[pendulist[0]-1].btu_jflags[cnt];
                        vmode[cnt] = ulist[pendulist[0]-1].btu_vflags[cnt];
                }
        }
        else    for  (cnt = 0;  cnt < 3;  cnt++)  {
                jmode[cnt] = Btuhdr.btd_jflags[cnt];
                vmode[cnt] = Btuhdr.btd_vflags[cnt];
        }

        prevabove = CreateUEditDlg(parent, "mode", &mode_shell, &paneview, &mainform);

        place_label_left(mainform, prevabove, "jobsm");
        place_label_right(mainform, prevabove, "varsm");
        mainrc = XtVaCreateManagedWidget("mdmain",
                                         xmRowColumnWidgetClass,        mainform,
                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                         XmNtopWidget,                  prevabove,
                                         XmNleftAttachment,             XmATTACH_FORM,
                                         XmNpacking,                    XmPACK_COLUMN,
                                         XmNnumColumns,                 2,
                                         NULL);

        jobrc  = XtVaCreateManagedWidget("mdjob",
                                         xmRowColumnWidgetClass,        mainrc,
                                         XmNnumColumns,                 3,
                                         XmNpacking,                    XmPACK_COLUMN,
                                         NULL);

        varrc  = XtVaCreateManagedWidget("mdvar",
                                         xmRowColumnWidgetClass,        mainrc,
                                         XmNnumColumns,                 3,
                                         XmNpacking,                    XmPACK_COLUMN,
                                         NULL);

        for  (col = 0;  col < 3;  col++)  {
                for  (row = 0;  row < 11;  row++)  {
                        Widget  w;
                        int     indx = col*11 + row;
                        char    name[12];
                        sprintf(name, "j%c%s", "ugo" [col], modenames[row].name);
                        modew[indx] = w =
                                XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, jobrc, NULL);
                        if  (jmode[col] & (1 << row))
                                XmToggleButtonGadgetSetState(w, True, False);
                        XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) mdtoggle, INT_TO_XTPOINTER(indx));
                }
        }

        for  (col = 0;  col < 3;  col++)  {
                for  (row = 0;  row < 10;  row++)  {
                        Widget  w;
                        int     indx = col*11 + row + 33;
                        char    name[12];
                        sprintf(name, "v%c%s", "ugo" [col], modenames[row].name);
                        modew[indx] = w =
                                XtVaCreateManagedWidget(name, xmToggleButtonGadgetClass, varrc, NULL);
                        if  (vmode[col] & (1 << row))
                                XmToggleButtonGadgetSetState(w, True, False);
                        XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) mdtoggle, INT_TO_XTPOINTER(indx));
                }
        }

        if  (which)  {
                Widget  sdef = XtVaCreateManagedWidget("Setdef",
                                                       xmPushButtonGadgetClass, mainform,
                                                       XmNtopAttachment,                XmATTACH_WIDGET,
                                                       XmNtopWidget,                    mainrc,
                                                       XmNleftAttachment,               XmATTACH_FORM,
                                                       NULL);

                XtAddCallback(sdef, XmNactivateCallback, (XtCallbackProc) setdef, (XtPointer) 0);
        }
        XtManageChild(mainform);
        CreateActionEndDlg(mode_shell, paneview, (XtCallbackProc) endmode, which? $H{xmbtuser user perm dialog}: $H{xmbtuser default perm dialog});
}

static void  pcopydef()
{
        int     cnt;
        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)
                XmToggleButtonGadgetSetState(workw[privnames[cnt].priv_workw],
                                             Btuhdr.btd_priv & privnames[cnt].priv_flag? True: False, False);
}

static void  endpriv(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                ULONG  newflags = 0;
                int     cnt;
                for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)
                        if  (XmToggleButtonGadgetGetState(workw[privnames[cnt].priv_workw]))
                                newflags |= privnames[cnt].priv_flag;
                if  (pendunum > 0)  {
                        uchanges++;
                        for  (cnt = 0;  cnt < pendunum;  cnt++)
                                ulist[pendulist[cnt]-1].btu_priv = newflags;
                        udisplay(pendunum, pendulist);
                }
                else  {
                        if  (Confirm(w, $PH{xmbtuser copy privs to everyone}))  {
                                Btuhdr.btd_priv = newflags;
                                hchanges++;
                                uchanges++;
                                for  (cnt = 0;  cnt < (int) Npwusers;  cnt++)
                                        if  (ulist[cnt].btu_user != Realuid)
                                                ulist[cnt].btu_priv = newflags;
                                udisplay(0, (int *) 0);
                        }
                        else  if  (Btuhdr.btd_priv != newflags)  {
                                Btuhdr.btd_priv = newflags;
                                hchanges++;
                        }
                        defdisplay();
                }
        }
        XtDestroyWidget(GetTopShell(w));
}

/* Handle "linked" privileges */

static void  changepriv(Widget w, int wpriv, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                switch  (wpriv)  {
                case  BTM_SPCREATE:
                        XmToggleButtonGadgetSetState(workw[WORKW_CREATE], True, False);
                        break;
                case  BTM_WADMIN:
                        XmToggleButtonGadgetSetState(workw[WORKW_RADMIN], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_SSTOP], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_UMASK], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_CREATE], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_SPCREATE], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_ORUG], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_ORUO], True, False);
                        XmToggleButtonGadgetSetState(workw[WORKW_ORGO], True, False);
                        break;
                }
        }
        else  {
                switch  (wpriv)  {
                case  BTM_CREATE:
                        XmToggleButtonGadgetSetState(workw[WORKW_SPCREATE], False, False);
                        break;
                case  BTM_UMASK:
                case  BTM_RADMIN:
                        XmToggleButtonGadgetSetState(workw[WORKW_WADMIN], False, False);
                        break;
                }
        }
}

void  cb_priv(Widget parent, int which)
{
        Widget  priv_shell, paneview, mainform, prevabove;
        ULONG   existing;
        int     cnt;

        if  (pendunum > 0)  {
                pendunum = 0;
                XtFree((char *) pendulist);
                pendulist = (int *) 0;
        }

        if  (which)  {
                if  (!getselectedusers(1))
                        return;
                existing = ulist[pendulist[0]-1].btu_priv;
        }
        else
                existing = Btuhdr.btd_priv;

        prevabove = CreateUEditDlg(parent, "privs", &priv_shell, &paneview, &mainform);
        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)  {
                workw[privnames[cnt].priv_workw] = prevabove =
                        XtVaCreateManagedWidget(privnames[cnt].buttname,
                                                xmToggleButtonGadgetClass,              mainform,
                                                XmNtopAttachment,                       XmATTACH_WIDGET,
                                                XmNtopWidget,                           prevabove,
                                                XmNleftAttachment,                      XmATTACH_FORM,
                                                XmNborderWidth,                         0,
                                                NULL);
                if  (existing & privnames[cnt].priv_flag)
                        XmToggleButtonGadgetSetState(prevabove, True, False);
        }

        XtAddCallback(workw[WORKW_CREATE], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) BTM_CREATE);
        XtAddCallback(workw[WORKW_SPCREATE], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) BTM_SPCREATE);
        XtAddCallback(workw[WORKW_WADMIN], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) BTM_WADMIN);
        XtAddCallback(workw[WORKW_RADMIN], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) BTM_RADMIN);
        XtAddCallback(workw[WORKW_UMASK], XmNvalueChangedCallback, (XtCallbackProc) changepriv, (XtPointer) BTM_UMASK);
        if  (which)  {
                Widget  cdefb = XtVaCreateManagedWidget("cdef",
                                                        xmPushButtonWidgetClass,        mainform,
                                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                                        XmNtopWidget,                   prevabove,
                                                        XmNleftAttachment,              XmATTACH_FORM,
                                                        NULL);
                XtAddCallback(cdefb, XmNactivateCallback, (XtCallbackProc) pcopydef, 0);
        }
        XtManageChild(mainform);
        CreateActionEndDlg(priv_shell, paneview, (XtCallbackProc) endpriv, which? $H{xmbtuser user priv dialog}: $H{xmbtuser default priv dialog});
}

static void  copyu(BtuserRef n)
{
        n->btu_defp = Btuhdr.btd_defp;
        n->btu_minp = Btuhdr.btd_minp;
        n->btu_maxp = Btuhdr.btd_maxp;
        n->btu_maxll = Btuhdr.btd_maxll;
        n->btu_totll = Btuhdr.btd_totll;
        n->btu_spec_ll = Btuhdr.btd_spec_ll;
}

void  cb_copydef(Widget parent, int which)
{
        int     cnt;

        if  (pendunum > 0)  {
                pendunum = 0;
                XtFree((char *) pendulist);
                pendulist = (int *) 0;
        }

        if  (which)  {
                if  (!getselectedusers(1))
                        return;
                for  (cnt = 0;  cnt < pendunum;  cnt++)
                        copyu(&ulist[pendulist[cnt]-1]);
                udisplay(pendunum, pendulist);
        }
        else  {
                for  (cnt = 0;  cnt < (int) Npwusers;  cnt++)
                        copyu(&ulist[cnt]);
                udisplay(0, (int *) 0);
        }
        uchanges++;
}

static void  umacroexec(char *str)
{
        static  char    *execprog;
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                char    **argbuf, **ap;
                int     cnt;
                getselectedusers(0);
                if  (!(argbuf = (char **) malloc((unsigned) pendunum + 2)))
                        exit(0);
                ap = argbuf;
                *ap++ = str;
                for  (cnt = 0;  cnt < pendunum;  cnt++)
                        *ap++ = prin_uname((uid_t) ulist[pendulist[cnt] - 1].btu_user);
                *ap = (char *) 0;
                execv(execprog, argbuf);
                exit(255);
        }
        if  (pid < 0)  {
                doerror(uwid, $EH{xmbtuser macro fork failed});
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
                        doerror(uwid, $EH{xmbtuser macro gave signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror(uwid, $EH{xmbtuser macro error exit});
                }
        }
}

static void  endumacro(Widget w, int data)
{
        if  (data)  {
                char    *txt;
                XtVaGetValues(workw[WORKW_SORTU], XmNvalue, &txt, NULL);
                if  (txt[0])
                        umacroexec(txt);
                XtFree(txt);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_macrou(Widget parent, int data)
{
        char    *prompt = helpprmpt(data + $P{Job or User macro});
        Widget  uc_shell, panew, mainform, labw;

        if  (!prompt)  {
                disp_arg[0] = data + $P{Job or User macro};
                doerror(uwid, $EH{xmbtuser macro not defined});
                return;
        }

        if  (data != 0)  {
                umacroexec(prompt);
                return;
        }

        CreateEditDlg(parent, "usercmd", &uc_shell, &panew, &mainform, 3);
        labw = place_label_topleft(mainform, "cmdtit");
        workw[WORKW_SORTU] = XtVaCreateManagedWidget("cmd",
                                                     xmTextFieldWidgetClass,    mainform,
                                                     XmNcolumns,                20,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNtopAttachment,          XmATTACH_FORM,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             labw,
                                                     NULL);
        XtManageChild(mainform);
        CreateActionEndDlg(uc_shell, panew, (XtCallbackProc) endumacro, $H{Job or User macro});
}

static  char    matchcase, wraparound, sbackward, *matchtext;

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

/* Written like this for easy e-x-t-e-n-s-i-o-n.  */

static int  usermatch(int n)
{
        return  smstr(prin_uname((uid_t) ulist[n].btu_user), 0);
}

static void  execute_search()
{
        int     *plist, pcnt, cline, nline;
        int     topitem, visibleitem;

        if  (XmListGetSelectedPos(uwid, &plist, &pcnt))  {
                cline = plist[0] - 1;
                XtFree((char *) plist);
        }
        else
                cline = sbackward? (int) Npwusers: -1;
        if  (sbackward)  {
                for  (nline = cline - 1;  nline >= 0;  nline--)
                        if  (usermatch(nline))
                                goto  gotuser;
                if  (wraparound)
                        for  (nline = Npwusers-1;  nline > cline;  nline--)
                                if  (usermatch(nline))
                                        goto  gotuser;
                doerror(uwid, $EH{Rsearch user not found});
        }
        else  {
                for  (nline = cline + 1;  nline < Npwusers;  nline++)
                        if  (usermatch(nline))
                                goto  gotuser;
                if  (wraparound)
                        for  (nline = 0;  nline < cline;  nline++)
                                if  (usermatch(nline))
                                        goto  gotuser;
                doerror(uwid, $EH{Fsearch user not found});
        }
        return;
 gotuser:
        nline++;
        XmListSelectPos(uwid, nline, False);
        XtVaGetValues(uwid, XmNtopItemPosition, &topitem, XmNvisibleItemCount, &visibleitem, NULL);
        if  (nline < topitem)
                XmListSetPos(uwid, nline);
        else  if  (nline >= topitem + visibleitem)
                XmListSetBottomPos(uwid, nline);
}

static Widget  InitsearchDlg(Widget parent, Widget *shellp, Widget *panep, Widget *formp, char *existing)
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

        return  workw[WORKW_STXTW];
}

static void  endsdlg(Widget w, int data)
{
        if  (data)  {
                sbackward = XmToggleButtonGadgetGetState(workw[WORKW_FORWW])? 0: 1;
                matchcase = XmToggleButtonGadgetGetState(workw[WORKW_MATCHW])? 1: 0;
                wraparound = XmToggleButtonGadgetGetState(workw[WORKW_WRAPW])? 1: 0;
                if  (matchtext) /* Last time round */
                        XtFree(matchtext);
                XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &matchtext, NULL);
                if  (matchtext[0] == '\0')  {
                        doerror(w, $EH{xmbtuser no search string});
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
                doerror(w, $EH{xmbtuser no previous search});
                return;
        }
        sbackward = (char) data;
        execute_search();
}

void  cb_srchfor(Widget parent)
{
        Widget  s_shell, panew, formw, dirrc, prevleft, prevabove;

        prevabove = InitsearchDlg(parent, &s_shell, &panew, &formw, matchtext);

        dirrc = XtVaCreateManagedWidget("dirn",
                                        xmRowColumnWidgetClass,         formw,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
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

        XmToggleButtonGadgetSetState(sbackward? prevleft: workw[WORKW_FORWW], True, False);

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
        if  (matchcase)
                XmToggleButtonGadgetSetState(workw[WORKW_MATCHW], True, False);

        workw[WORKW_WRAPW] =  XtVaCreateManagedWidget("wrap",
                                                      xmToggleButtonGadgetClass,        dirrc,
                                                      XmNborderWidth,                   0,
                                                      NULL);
        if  (wraparound)
                XmToggleButtonGadgetSetState(workw[WORKW_WRAPW], True, False);

        XtManageChild(formw);

        CreateActionEndDlg(s_shell, panew, (XtCallbackProc) endsdlg, $H{xmbtuser search menu help});
}
