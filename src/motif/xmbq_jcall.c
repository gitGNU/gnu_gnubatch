/* xmbq_jcall.c -- job callbacks for gbch-xmq

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
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
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

#ifdef HAVE_XM_SPINB_H
static  char    Filename[] = __FILE__;
#endif

#define SECSPERDAY      (24 * 60 * 60L)

BtjobRef        cjob;
static  unsigned  char  nochanges;
static  USHORT          copyumask;
static  Timecon         ctimec;

static  int     longest_day = 0,
                longest_mon = 0;

static  HelpaltRef      daynames_full,
                        monnames;
#ifdef HAVE_XM_SPINB_H
static  XmStringTable   timezerof,
                        stdaynames,
                        stmonnames;
#endif

static  ULONG           default_rate;
static  unsigned char   default_interval,
                        default_nposs;
static  USHORT          defavoid;

static  struct  {
        char    *wname;
        int     signum;
}  siglist[] = {
#ifdef  SIGSTOP
        {       "Stopsig",      SIGSTOP },
#endif
#ifdef  SIGCONT
        {       "Contsig",      SIGCONT },
#endif
        {       "Termsig",      SIGTERM },
        {       "Killsig",      SIGKILL },
        {       "Hupsig",       SIGHUP  },
        {       "Intsig",       SIGINT  },
        {       "Quitsig",      SIGQUIT },
        {       "Alarmsig",     SIGALRM },
        {       "Bussig",       SIGBUS  },
        {       "Segvsig",      SIGSEGV }
};

/* Send job reference only to scheduler */

void  qwjimsg(const unsigned code, CBtjobRef jp)
{
        Oreq.sh_params.mcode = code;
        Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
        Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), 0) < 0)
                msg_error();
}

/* Send job-type message to scheduler */

void  wjmsg(const unsigned code, const ULONG indx)
{
        Oreq.sh_params.mcode = code;
        Oreq.sh_un.sh_jobindex = indx;
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), 0) < 0)
                msg_error();
}

/* Display job-type error message */

void  qdojerror(unsigned retc, BtjobRef jp)
{
        switch  (retc & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                doerror(jwid, $EH{Unexpected sched message});
                return;
        case  JOB_REPLY:
                disp_str = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;
                doerror(jwid, (int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler job errors}));
                return;
        case  NET_REPLY:
                disp_str = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;
                doerror(jwid, (int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler net errors}));
                return;
        }
}

Widget  CreateJtitle(Widget formw, BtjobRef jp)
{
        Widget          titw1, titw2;
        int     lng;
        char    nbuf[HOSTNSIZE+20];
        titw1 = XtVaCreateManagedWidget("jobnotitle",
                                        xmLabelWidgetClass,     formw,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_FORM,
                                        XmNborderWidth,         0,
                                        NULL);
        if  (jp->h.bj_hostid)
                sprintf(nbuf, "%s:%ld", look_host(jp->h.bj_hostid), (long) jp->h.bj_job);
        else
                sprintf(nbuf, "%ld", (long) jp->h.bj_job);
        lng = strlen(nbuf);

        titw2 = XtVaCreateManagedWidget("jobno",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     lng,
                                        XmNcursorPositionVisible,       False,
                                        XmNeditable,                    False,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  titw1,
                                        NULL);
        XmTextSetString(titw2, nbuf);
        return  titw2;
}


static void  init_tdefaults()
{
        int     n;

        if  (longest_day != 0)
                return;
        if  ((daynames_full = helprdalt($Q{Weekdays full})))
                longest_day = altlen(daynames_full);
        if  ((monnames = helprdalt($Q{Months full})))
                longest_mon = altlen(monnames);
        if  ((n = helpnstate($N{Default repeat alternative})) < 0  ||  n > TC_YEARS)
                n = TC_RETAIN;
        default_interval = (unsigned char) n;
        if  ((n = helpnstate($N{Default number of units})) <= 0)
                n = 10;
        default_rate = (ULONG) n;
        if  ((n = helpnstate($N{Default skip delay option})) < 0  ||  n > TC_CATCHUP)
                n = TC_WAIT1;
        default_nposs = (unsigned char) n;
        defavoid = 0;
        for  (n = 0;  n < TC_NDAYS;  n++)
                if  (helpnstate($N{Base for days to avoid}+n) > 0)
                        defavoid |= 1 << n;
#ifdef HAVE_XM_SPINB_H

        /* Set up the string tables for time setting now
           Timezerof is done as strings to give us nice zero filled hh:mm etc */

        if  (!(timezerof = (XmStringTable) XtMalloc((unsigned) (60 * sizeof(XmString)))))
                ABORT_NOMEM;
        if  (!(stdaynames = (XmStringTable) XtMalloc((unsigned) (7 * sizeof(XmString)))))
                ABORT_NOMEM;
        if  (!(stmonnames = (XmStringTable) XtMalloc((unsigned) (12 * sizeof(XmString)))))
                ABORT_NOMEM;

        for  (n = 0;  n < 60;  n++)  {
                char    buf[3];
                sprintf(buf, "%.2d", n);
                timezerof[n] = XmStringCreateLocalized(buf);
        }
        for  (n = 0;  n < 7;  n++)
                stdaynames[n] = XmStringCreateLocalized(disp_alt(n, daynames_full));
        for  (n = 0;  n < 12;  n++)
                stmonnames[n] = XmStringCreateLocalized(disp_alt(n, monnames));
#endif
}

void  cb_advtime()
{
        BtjobRef        cj = getselectedjob(BTM_WRITE);
        BtjobRef        djp;
        unsigned        retc;
        ULONG           xindx;

        if  (!cj)
                return;
        if  (!cj->h.bj_times.tc_istime  ||  cj->h.bj_times.tc_repeat < TC_MINUTES)  {
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);
                doerror(jwid, $EH{xmbtq no time to adv});
                return;
        }
        djp = &Xbuffer->Ring[xindx = getxbuf()];
        *djp = *cj;
        djp->h.bj_times.tc_nexttime = advtime(&djp->h.bj_times);
        wjmsg(J_CHANGE, xindx);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, djp);
        freexbuf(xindx);
}

/* Force run / delete */

void  cb_jact(Widget parent, int data)
{
        BtjobRef        cj = getselectedjob(data == J_DELETE? BTM_DELETE: BTM_WRITE|BTM_KILL);
        unsigned        retc;

        if  (!cj)
                return;
        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);
                doerror(jwid, $EH{xmbtq job running});
                return;
        }
        if  (data == J_DELETE)  {
                if  (Dispflags & DF_CONFABORT  &&  !Confirm(jwid, $PH{xmbtq confirm delete job}))
                        return;
        }
        else  if  (cj->h.bj_progress != BJP_NONE)  {
                ULONG   xindx;
                BtjobRef  djp = &Xbuffer->Ring[xindx = getxbuf()];
                *djp = *cj;
                djp->h.bj_progress = BJP_NONE;
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)  {
                        qdojerror(retc, djp);
                        freexbuf(xindx);
                        return;
                }
                freexbuf(xindx);
        }
        qwjimsg(data, cj);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, cj);
}

static void  sendsig(Widget w, int data)
{
        if  (data)  {           /* signal pressed */
                unsigned        retc;
                Oreq.sh_params.param = data;
                qwjimsg(J_KILL, cjob);
                if  ((retc = readreply()) != J_OK)
                        qdojerror(retc, cjob);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_jkill(Widget parent, int data)
{
        BtjobRef        cj = getselectedjob(BTM_KILL);
        Widget          o_shell, panew, formw, prevabove;
        int             cnt;

        if  (!cj)
                return;
        if  (cj->h.bj_progress != BJP_RUNNING)  {
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);
                doerror(jwid, $EH{xmbtq job not running});
                return;
        }
        cjob = cj;

        /* "Other signal" has data as zero */

        if  (data != 0)  {
                unsigned        retc;
                Oreq.sh_params.param = data;
                qwjimsg(J_KILL, cj);
                if  ((retc = readreply()) != J_OK)
                        qdojerror(retc, cj);
                return;
        }

        prevabove = CreateJeditDlg(parent, "othersig", &o_shell, &panew, &formw);

        for  (cnt = 0;  cnt < XtNumber(siglist);  cnt++)  {
                prevabove = XtVaCreateManagedWidget(siglist[cnt].wname,
                                                    xmPushButtonWidgetClass,    formw,
                                                    XmNtopAttachment,           XmATTACH_WIDGET,
                                                    XmNtopWidget,               prevabove,
                                                    XmNleftAttachment,          XmATTACH_FORM,
                                                    NULL);
                XtAddCallback(prevabove, XmNactivateCallback, (XtCallbackProc) sendsig, INT_TO_XTPOINTER(siglist[cnt].signum));
        }
        XtManageChild(formw);
        XtUnmanageChild(XmMessageBoxGetChild(o_shell, XmDIALOG_OK_BUTTON));
        XtAddCallback(o_shell, XmNcancelCallback, (XtCallbackProc) sendsig, 0);
        XtAddCallback(o_shell, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtq signal select dialog});
        XtManageChild(o_shell);
}

void  cb_jstate(Widget parent, int data)
{
        BtjobRef        cj = getselectedjob(BTM_WRITE);
        BtjobRef        djp;
        unsigned        retc;
        ULONG           xindx;

        if  (!cj)
                return;
        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                doerror(jwid, $EH{xmbtq job running});
                return;
        }
        djp = &Xbuffer->Ring[xindx = getxbuf()];
        *djp = *cj;
        djp->h.bj_progress = (unsigned char) data;
        wjmsg(J_CHANGE, xindx);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, djp);
        freexbuf(xindx);
}

#define XMBTQ_JCALL
#include "bqr_common.c"

static void  jmacroexec(char *str, BtjobRef jp)
{
        static  char    *execprog;
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                char    nbuf[20+HOSTNSIZE];
                char    *argbuf[3];
                argbuf[0] = str;
                if  (jp)  {
                        if  (jp->h.bj_hostid)
                                sprintf(nbuf, "%s:%ld", look_host(jp->h.bj_hostid), (long) jp->h.bj_job);
                        else
                                sprintf(nbuf, "%ld", (long) jp->h.bj_job);
                        argbuf[1] = nbuf;
                        argbuf[2] = (char *) 0;
                }
                else
                        argbuf[1] = (char *) 0;
                chdir(Curr_pwd);
                execv(execprog, argbuf);
                exit(255);
        }
        if  (pid < 0)  {
                doerror(jwid, $EH{xmbtq macro cannot fork});
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
                        doerror(jwid, $EH{xmbtq job macro signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror(jwid, $EH{xmbtq job macro exit code});
                }
        }
}

static void  endjmacro(Widget w, int data)
{
        if  (data)  {
                char    *txt;
                XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &txt, NULL);
                if  (txt[0])  {
                        BtjobRef        jp = (BtjobRef) 0;
                        int     *plist, pcnt;
                        if  (XmListGetSelectedPos(jwid, &plist, &pcnt) && pcnt > 0)  {
                                jp = jj_ptrs[plist[0] - 1];
                                XtFree((XtPointer) plist);
                        }
                        jmacroexec(txt, jp);
                }
                XtFree(txt);
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_macroj(Widget parent, int data)
{
        char    *prompt = helpprmpt(data + $P{Job or User macro});
        int     *plist, pcnt;
        BtjobRef  jp = (BtjobRef) 0;
        Widget  jc_shell, panew, mainform, labw;

        if  (!prompt)  {
                disp_arg[0] = data + $P{Job or User macro};
                doerror(jwid, $EH{xmbtq macro prompt missing});
                return;
        }

        if  (XmListGetSelectedPos(jwid, &plist, &pcnt) && pcnt > 0)  {
                jp = jj_ptrs[plist[0] - 1];
                XtFree((XtPointer) plist);
        }

        if  (data != 0)  {
                jmacroexec(prompt, jp);
                return;
        }

        CreateEditDlg(parent, "jobcmd", &jc_shell, &panew, &mainform, 3);
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
        CreateActionEndDlg(jc_shell, panew, (XtCallbackProc) endjmacro, $H{Job or User macro});
}
