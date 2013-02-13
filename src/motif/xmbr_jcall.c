/* xmbr_jcall.c -- job callbacks for gbch-xmr

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
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <X11/cursorfont.h>
#include <Xm/ArrowB.h>
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
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"

static  char    Filename[] = __FILE__;

#define SECSPERDAY      (24 * 60 * 60L)

struct pend_job *cjob;
static  USHORT  copyumask;
static  Timecon         ctimec;

extern  HelpaltRef      daynames_full,
                        monnames;
#ifdef HAVE_XM_SPINB_H
extern  XmStringTable   timezerof, stdaynames, stmonnames;
#endif

extern  int             longest_day,
                        longest_mon;

extern  ULONG           default_rate;
extern  unsigned char   default_interval,
                        default_nposs;
extern  USHORT          defavoid;

static  struct  {
        char    *wname;
        int     signum;
}  siglist[] = {
        {       "Termsig",      SIGTERM },
        {       "Killsig",      SIGKILL },
        {       "Hupsig",       SIGHUP  },
        {       "Intsig",       SIGINT  },
        {       "Quitsig",      SIGQUIT },
        {       "Alarmsig",     SIGALRM },
        {       "Bussig",       SIGBUS  },
        {       "Segvsig",      SIGSEGV },
        {       "Cancel",       0       }
};

extern Widget  CreateJeditDlg(Widget, char *, Widget *, Widget *, Widget *);

Widget  CreateJtitle(Widget formw, struct pend_job *jp)
{
        Widget          titw1, titw2;
        static  char    *def_nam;

        titw1 = XtVaCreateManagedWidget("jobnotitle",
                                        xmLabelWidgetClass,     formw,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNborderWidth,         0,
                                        NULL);

        titw2 = XtVaCreateManagedWidget("jobno",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     30,
                                        XmNcursorPositionVisible,       False,
                                        XmNeditable,                    False,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  titw1,
                                        NULL);

        if  (jp == &default_pend)  {
                if  (!def_nam)
                        def_nam = gprompt($P{xmbtr default job});
                XmTextSetString(titw2, def_nam);
        }
        else
                XmTextSetString(titw2, (char *) title_of(jp->job));
        return  titw2;
}

static void  endqueue(Widget w, int data)
{
        if  (data)  {
                char    *txt, *ntit;
                const   char    *cp, *tit;
                int_ugid_t      luid, lgid;
                BtjobRef        bjp = cjob->job;
                Btjob   njob;
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XmString        item_txt;
                XtVaGetValues(workw[WORKW_UTXTW], XmNselectedItem, &item_txt, NULL);
                XmStringGetLtoR(item_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(item_txt);
#else
                XtVaGetValues(workw[WORKW_UTXTW], XmNvalue, &txt, NULL);
#endif
                luid = lookup_uname(txt);
                if  (luid == UNKNOWN_UID)  {
                        disp_str = txt;
                        doerror(jwid, $EH{Unknown owner});
                        XtFree(txt);
                        return;
                }
                XtFree(txt);
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XtVaGetValues(workw[WORKW_GTXTW], XmNselectedItem, &item_txt, NULL);
                XmStringGetLtoR(item_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(item_txt);
#else
                XtVaGetValues(workw[WORKW_GTXTW], XmNvalue, &txt, NULL);
#endif
                lgid = lookup_gname(txt);
                if  (lgid == UNKNOWN_GID)  {
                        disp_str = txt;
                        doerror(jwid, $EH{Unknown group});
                        XtFree(txt);
                        return;
                }
                XtFree(txt);
                if  (cjob->jobqueue)
                        free(cjob->jobqueue);
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XtVaGetValues(workw[WORKW_QTXTW], XmNselectedItem, &item_txt, NULL);
                XmStringGetLtoR(item_txt, XmSTRING_DEFAULT_CHARSET, &txt);
                XmStringFree(item_txt);
                cjob->jobqueue = txt[0] && txt[0] != '-'? stracpy(txt): (char *) 0;
#else
                XtVaGetValues(workw[WORKW_QTXTW], XmNvalue, &txt, NULL);
                cjob->jobqueue = txt[0]? stracpy(txt): (char *) 0;
#endif
                XtFree(txt);
                tit = title_of(bjp);
                if  ((cp = strchr(tit, ':')))
                        cp++;
                else
                        cp = tit;
                if  (cjob->jobqueue)  {
                        unsigned  tlng = strlen(cp) + strlen(cjob->jobqueue) + 2;
                        if  (!(ntit = malloc(tlng)))
                                ABORT_NOMEM;
                        sprintf(ntit, "%s:%s", cjob->jobqueue, cp);
                }
                else
                        ntit = stracpy(cp);
                njob = *bjp;
                if  (!repackjob(&njob, bjp, (char *) 0, ntit, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        doerror(w, $EH{Too many job strings});
                        free(ntit);
                }
                *bjp = njob;
                free(ntit);
                cjob->userid = luid;
                cjob->grpid = lgid;
                cjob->Verbose = XmToggleButtonGadgetGetState(workw[WORKW_VERBOSE])? 1: 0;
                cjob->changes++;
                cjob->nosubmit++;
                if  (cjob != &default_pend)  {
                        int     indx = getselectedjob(0);
                        if  (indx >= 0)  {
                                XmString        str = jdisplay(cjob);
                                XmListReplaceItemsPos(jwid, &str, 1, indx+1);
                                XmStringFree(str);
                                XmListSelectPos(jwid, indx+1, False);
                        }
                }
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jqueue(Widget parent, int isjob)
{
        Widget  q_shell, panew, mainform, prevabove, verbw;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        prevabove = CreateJeditDlg(parent, "Queuew", &q_shell, &panew, &mainform);
        prevabove = CreateQselDialog(mainform, prevabove, cjob->jobqueue, 1);
        prevabove = CreateUselDialog(mainform, prevabove, prin_uname((uid_t) cjob->userid), 0, (mypriv->btu_priv & BTM_WADMIN) == 0);
        prevabove = CreateGselDialog(mainform, prevabove, prin_gname((gid_t) cjob->grpid), 0, (mypriv->btu_priv & BTM_WADMIN) == 0);

        verbw = workw[WORKW_VERBOSE] =
                XtVaCreateManagedWidget("verbose",
                                        xmToggleButtonGadgetClass,      mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        NULL);
        if  (cjob->Verbose)
                XmToggleButtonGadgetSetState(verbw, True, False);

        XtManageChild(mainform);
        CreateActionEndDlg(q_shell, panew, (XtCallbackProc) endqueue, $H{xmbtr select queue dialog help});
}

void  cb_jstate(Widget parent, int data)
{
        int     indx;
        XmString        str;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pend_list[indx].job->h.bj_progress = (unsigned char) data;
        str = jdisplay(&pend_list[indx]);
        XmListReplaceItemsPos(jwid, &str, 1, indx+1);
        XmStringFree(str);
        XmListSelectPos(jwid, indx+1, False);
}

void  cb_djstate(Widget parent, int data)
{
        default_job.h.bj_progress = (unsigned char) data;
}

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

static void  CreateModeDialog(Widget formw, Widget prevabove, BtmodeRef current, int isvar)
{
        Widget  mrc;
        int     nrows = MODENUMBERS, row, col;
        int     ch = 'j';
        if  (isvar)  {
                nrows--;
                ch = 'v';
        }
        copymode[0] = current->u_flags;
        copymode[1] = current->g_flags;
        copymode[2] = current->o_flags;

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
                        XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) mdtoggle, INT_TO_XTPOINTER(indx));
                }
        }
}

static void  endjperm(Widget w, int data)
{
        if  (data)  {
                BtmodeRef       cj = &cjob->job->h.bj_mode;
                cj->u_flags = copymode[0];
                cj->g_flags = copymode[1];
                cj->o_flags = copymode[2];
                cjob->changes++;
                cjob->nosubmit++;
        }
        if  (modew)  {
                free((char *) modew);
                modew = (Widget *) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jperm(Widget parent, int isjob)
{
        Widget  jm_shell, panew, formw, prevabove;
        BtjobRef  cj;
        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;
        if  (!(modew = (Widget *) malloc((unsigned)(3 * MODENUMBERS * sizeof(Widget)))))
                ABORT_NOMEM;
        prevabove = CreateJeditDlg(parent, "jmode", &jm_shell, &panew, &formw);
        CreateModeDialog(formw, prevabove, &cj->h.bj_mode, 0);
        XtManageChild(formw);
        CreateActionEndDlg(jm_shell, panew, (XtCallbackProc) endjperm, $H{xmbtq jperm dialog});
}

#define XMBTR_JCALL
#include "bqr_common.c"
