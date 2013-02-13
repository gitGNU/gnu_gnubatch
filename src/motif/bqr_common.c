/* bqr_common.c -- common routines for gbch-xm[qr]

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

static  ULONG   ratelim[] =  { 60*24*366*5, 24*366*5, 366*5, 52*5, 60, 60, 5  };

/* Initial part of job dialog.
   Return widget of tallest part of title.  */

Widget  CreateJeditDlg(Widget parent, char *dlgname, Widget *dlgres, Widget *paneres, Widget *formres)
{
        CreateEditDlg(parent, dlgname, dlgres, paneres, formres, 3);
        return  CreateJtitle(*formres, cjob);
}

#ifndef HAVE_XM_SPINB_H

/* Increment priorities */
static void  priup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_max = mypriv->btu_maxp;
                arrow_lng = 3;
                arrow_incr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}

/* Decrement priorities */

static void  pridn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_min = mypriv->btu_minp;
                arrow_lng = 3;
                arrow_decr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}

/* Increment normal/error exits */

static void  errup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_max = 255;
                arrow_lng = 3;
                arrow_incr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}

/* Decrement normal/error exits.  */

static void  errdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_min = 0;
                arrow_lng = 3;
                arrow_decr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}
#endif

#ifdef  XMBTQ_JCALL
static void  endmailwrt(Widget w, int data)
{
        if  (data  &&  !nochanges)  {
                unsigned        prevflags = cjob->h.bj_jflags;
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                bjp->h.bj_jflags &= ~(BJ_MAIL|BJ_WRT);
                if  (XmToggleButtonGadgetGetState(workw[WORKW_MAIL]))
                        bjp->h.bj_jflags |= BJ_MAIL;
                if  (XmToggleButtonGadgetGetState(workw[WORKW_WRITE]))
                        bjp->h.bj_jflags |= BJ_WRT;
                if  (bjp->h.bj_jflags != prevflags)  {
                        wjmsg(J_CHANGE, xindx);
                        retc = readreply();
                        if  (retc != J_OK)
                                qdojerror(retc, bjp);
                }
                freexbuf(xindx);
        }
        XtDestroyWidget(GetTopShell(w));
}
#endif

#ifdef  XMBTR_JCALL
static void  endmailwrt(Widget w, int data)
{
        if  (data)  {
                BtjobRef  cj = cjob->job;
                cj->h.bj_jflags &= ~(BJ_MAIL|BJ_WRT);
                if  (XmToggleButtonGadgetGetState(workw[WORKW_MAIL]))
                        cj->h.bj_jflags |= BJ_MAIL;
                if  (XmToggleButtonGadgetGetState(workw[WORKW_WRITE]))
                        cj->h.bj_jflags |= BJ_WRT;
                cjob->changes++;
                cjob->nosubmit++;
        }
        XtDestroyWidget(GetTopShell(w));
}
#endif

#ifdef  XMBTQ_JCALL
void  cb_jmail(Widget parent)
#endif
#ifdef  XMBTR_JCALL
void  cb_jmail(Widget parent, int isjob)
#endif
{
        Widget  mw_shell, panew, mainform, prevabove, mwrc;
#ifdef  XMBTQ_JCALL
        BtjobRef        cj = getselectedjob(BTM_READ);
        unsigned        readonly;

        if  (!cj)
                return;
        cjob = cj;
        nochanges = 0;
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
#endif
#ifdef  XMBTR_JCALL
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;
#endif

        prevabove = CreateJeditDlg(parent, "Mailw", &mw_shell, &panew, &mainform);
        mwrc = XtVaCreateManagedWidget("mwrc",
                                        xmRowColumnWidgetClass,         mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  1,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               False,
                                        NULL);

        workw[WORKW_MAIL] =
                XtVaCreateManagedWidget("mail",
                                        xmToggleButtonGadgetClass,      mwrc,
                                        XmNborderWidth,                 0,
                                        NULL);
        workw[WORKW_WRITE] =
                XtVaCreateManagedWidget("write",
                                        xmToggleButtonGadgetClass,      mwrc,
                                        XmNborderWidth,                 0,
                                        NULL);
        if  (cj->h.bj_jflags & BJ_MAIL)
                XmToggleButtonGadgetSetState(workw[WORKW_MAIL], True, False);
        if  (cj->h.bj_jflags & BJ_WRT)
                XmToggleButtonGadgetSetState(workw[WORKW_WRITE], True, False);
#ifdef XMBTQ_JCALL
        if  (readonly)  {
                XtSetSensitive(workw[WORKW_MAIL], False);
                XtSetSensitive(workw[WORKW_WRITE], False);
                nochanges = 1;
        }
#endif
        XtManageChild(mainform);
        CreateActionEndDlg(mw_shell, panew, (XtCallbackProc) endmailwrt, $H{xmbtq mw dialog});
}

Widget  timesubw, repsubw, mdaylab, mdayspin;

static  struct  {
        char    *wname;         /* Widget name */
        int     workw;          /* Workw entry */
}  wlist[] = {
        {       "Del",  WORKW_IDELETE   },
        {       "Ret",  WORKW_IRETAIN   },
        {       "Mins", WORKW_IMINUTES  },
        {       "Hours",WORKW_IHOURS    },
        {       "Days", WORKW_IDAYS     },
        {       "Weeks",WORKW_IWEEKS    },
        {       "Monb", WORKW_IMONTHSB  },
        {       "Mone", WORKW_IMONTHSE  },
        {       "Years",WORKW_IYEARS    }},
avlist[] = {
        {       "Sun",  WORKW_SUNDAY    },
        {       "Mon",  WORKW_MONDAY    },
        {       "Tue",  WORKW_TUESDAY   },
        {       "Wed",  WORKW_WEDNESDAY },
        {       "Thu",  WORKW_THURSDAY  },
        {       "Fri",  WORKW_FRIDAY    },
        {       "Sat",  WORKW_SATURDAY  },
        {       "Hday", WORKW_HOLIDAY   }};

static void  fillintimes()
{
        if  (ctimec.tc_istime)  {
                struct  tm      *tp = localtime(&ctimec.tc_nexttime);
#ifdef HAVE_XM_SPINB_H
                XtVaSetValues(workw[WORKW_HOURTW], XmNposition, tp->tm_hour, (XtPointer) 0);
                XtVaSetValues(workw[WORKW_MINTW], XmNposition, tp->tm_min, (XtPointer) 0);
                XtVaSetValues(workw[WORKW_DOWTW], XmNposition, tp->tm_wday, (XtPointer)0);
                XtVaSetValues(workw[WORKW_MONTW], XmNposition, tp->tm_mon, (XtPointer)0);
#ifdef  BROKEN_SPINBOX
                XtVaSetValues(workw[WORKW_DOMTW], XmNposition, tp->tm_mday-1, (XtPointer)0);
                XtVaSetValues(workw[WORKW_YEARTW], XmNposition, tp->tm_year, (XtPointer)0);
#else
                XtVaSetValues(workw[WORKW_DOMTW], XmNposition, tp->tm_mday, (XtPointer)0);
                XtVaSetValues(workw[WORKW_YEARTW], XmNposition, tp->tm_year+1900, (XtPointer)0);
#endif
#else
                char    nbuf[10];
                sprintf(nbuf, "%.2d", tp->tm_hour);
                XmTextSetString(workw[WORKW_HOURTW], nbuf);
                sprintf(nbuf, "%.2d", tp->tm_min);
                XmTextSetString(workw[WORKW_MINTW], nbuf);
                XmTextSetString(workw[WORKW_DOWTW], disp_alt(tp->tm_wday, daynames_full));
                sprintf(nbuf, "%.2d", tp->tm_mday);
                XmTextSetString(workw[WORKW_DOMTW], nbuf);
                XmTextSetString(workw[WORKW_MONTW], disp_alt(tp->tm_mon, monnames));
                sprintf(nbuf, "%d", tp->tm_year+1900);
                XmTextSetString(workw[WORKW_YEARTW], nbuf);
#endif
                if  (ctimec.tc_repeat >= TC_MINUTES)  {
                        time_t  foreg;
                        int     daysinmon = month_days[tp->tm_mon];
                        char    egbuf[50];
                        if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
                                daysinmon++;
#ifdef  HAVE_XM_SPINB_H
#ifdef  BROKEN_SPINBOX
                        XtVaSetValues(workw[WORKW_RINTERVAL], XmNposition, ctimec.tc_rate-1, (XtPointer) 0);
#else
                        XtVaSetValues(workw[WORKW_RINTERVAL], XmNposition, ctimec.tc_rate, (XtPointer) 0);
#endif
#else
                        sprintf(nbuf, "%lu", ctimec.tc_rate);
                        XmTextSetString(workw[WORKW_RINTERVAL], nbuf);
#endif
                        if  (ctimec.tc_repeat == TC_MONTHSB)  {
                                if  ((int) ctimec.tc_mday > daysinmon)
                                        ctimec.tc_mday = daysinmon;
#ifdef  HAVE_XM_SPINB_H
#ifdef  BROKEN_SPINBOX
                                XtVaSetValues(workw[WORKW_RMTHDAY], XmNposition, ctimec.tc_mday-1, (XtPointer) 0);
#else
                                XtVaSetValues(workw[WORKW_RMTHDAY], XmNposition, ctimec.tc_mday, (XtPointer) 0);
#endif
#else
                                sprintf(nbuf, "%d", ctimec.tc_mday);
                                XmTextSetString(workw[WORKW_RMTHDAY], nbuf);
#endif
                        }
                        else  if  (ctimec.tc_repeat == TC_MONTHSE)  {
                                if  ((int) ctimec.tc_mday >= daysinmon)
                                        ctimec.tc_mday = daysinmon - 1;
#ifdef  HAVE_XM_SPINB_H
#ifdef  BROKEN_SPINBOX
                                XtVaSetValues(workw[WORKW_RMTHDAY], XmNposition, daysinmon - (int)ctimec.tc_mday - 1, (XtPointer) 0);
#else
                                XtVaSetValues(workw[WORKW_RMTHDAY], XmNposition, daysinmon - (int)ctimec.tc_mday, (XtPointer)0);
#endif
#else
                                sprintf(nbuf, "%d", (int) month_days[tp->tm_mon] - (int) ctimec.tc_mday);
                                XmTextSetString(workw[WORKW_RMTHDAY], nbuf);
#endif
                        }
                        foreg = advtime(&ctimec);
                        tp = localtime(&foreg);
                        sprintf(egbuf, "%.2d:%.2d %s %.2d %s %d",
                                       tp->tm_hour, tp->tm_min,
                                       disp_alt(tp->tm_wday, daynames_full),
                                       tp->tm_mday, disp_alt(tp->tm_mon, monnames), tp->tm_year + 1900);
                        XmTextSetString(workw[WORKW_RESULTREP], egbuf);
                }
        }
}

#ifdef HAVE_XM_SPINB_H
static void  time_adj(Widget w, XtPointer wh, XmSpinBoxCallbackStruct *cbs)
{
        time_t  newtime = ctimec.tc_nexttime;
        Widget  whichw = cbs->widget;
        int     incr, pos;

        switch  (cbs->reason)  {
        default:
                return;
        case  XmCR_SPIN_NEXT:
                incr = 1;
                break;
        case  XmCR_SPIN_PRIOR:
                incr = -1;
                break;
        case  XmCR_SPIN_FIRST:
                incr = cbs->crossed_boundary? 1: -1;
                break;
        case  XmCR_SPIN_LAST:
                incr = cbs->crossed_boundary? -1: 1;
                break;
        }

        if  (whichw == workw[WORKW_MINTW])
                newtime += 60L * incr;
        else  if  (whichw == workw[WORKW_HOURTW])
                newtime += 3600L * incr;
        else  if  (whichw == workw[WORKW_DOWTW] || whichw == workw[WORKW_DOMTW])
                newtime += SECSPERDAY * incr;
        else  if  (whichw == workw[WORKW_MONTW])  {
                struct  tm  *tp = localtime(&newtime);
                if  (incr > 0)  {
                        newtime += month_days[tp->tm_mon] * SECSPERDAY;
                        if  (tp->tm_year % 4 == 0 && tp->tm_mon == 1)
                                newtime += SECSPERDAY;
                }
                else  {
                        newtime -= month_days[(tp->tm_mon + 11) % 12] * SECSPERDAY;
                        if  (tp->tm_year % 4 == 0 && tp->tm_mon == 2)
                                newtime -= SECSPERDAY;
                }
        }
        else  {                 /* Must be year */
                struct  tm  *tp = localtime(&newtime);
                if  (incr > 0)  {
                        newtime += 365 * SECSPERDAY;
                        if  ((tp->tm_year % 4 == 0  &&  tp->tm_mon <= 1)  ||  (tp->tm_year % 4 == 3  &&  tp->tm_mon > 1))
                                newtime += SECSPERDAY;
                }
                else  {
                        newtime -= 365 * SECSPERDAY;
                        if  ((tp->tm_year % 4 == 1  &&  tp->tm_mon <= 1)  ||  (tp->tm_year % 4 == 0  &&  tp->tm_mon > 1))
                                newtime -= SECSPERDAY;
                }
        }
        if  (newtime >= time((time_t *) 0))  {
                ctimec.tc_nexttime = newtime;
                fillintimes();
        }
        XtVaGetValues(whichw, XmNposition, &pos, (XtPointer) 0);
        cbs->position = pos;
}

static void  rint_adj(Widget w, XtPointer wh, XmSpinBoxCallbackStruct *cbs)
{
        int     newpos = cbs->position;
#ifdef  BROKEN_SPINBOX
        newpos++;
#endif
        if  (ctimec.tc_repeat < TC_MINUTES  ||  (ULONG) newpos > ratelim[ctimec.tc_repeat-TC_MINUTES])  {
                cbs->doit = False;
                return;
        }
        ctimec.tc_rate = newpos;
        fillintimes();
}

static void  mday_adj(Widget w, XtPointer wh, XmSpinBoxCallbackStruct *cbs)
{
        struct  tm  *tp = localtime(&ctimec.tc_nexttime);
        int     daysinmon = month_days[tp->tm_mon];
        int     newpos = cbs->position;
#ifdef  BROKEN_SPINBOX
        newpos++;
#endif
        if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
                daysinmon = 29;
        if  (newpos <= 0  || newpos > daysinmon)  {
                cbs->doit = False;
                return;
        }
        if  (ctimec.tc_repeat == TC_MONTHSB)
                ctimec.tc_mday = (unsigned char) newpos;
        else
                ctimec.tc_mday = (unsigned char) (daysinmon - newpos);
        fillintimes();
}

#else

/* Timeout routine for bits of dates.
   The "amount" is the widget number of the relevant bit
   if we are incrementing the time, otherwise - the widget number */

static void  time_adj(int amount, XtIntervalId *id)
{
        time_t  newtime = ctimec.tc_nexttime;
        struct  tm      *tp;

        switch  (amount)  {
        default:
        case  WORKW_MINTW:
                newtime += 60L;
                break;
        case  -WORKW_MINTW:
                newtime -= 60L;
                break;
        case  WORKW_HOURTW:
                newtime += 60*60L;
                break;
        case  -WORKW_HOURTW:
                newtime -= 60*60L;
                break;
        case  WORKW_DOWTW:
        case  WORKW_DOMTW:
                newtime += SECSPERDAY;
                break;
        case  -WORKW_DOWTW:
        case  -WORKW_DOMTW:
                newtime -= SECSPERDAY;
                break;
        case  WORKW_MONTW:
                tp = localtime(&newtime);
                newtime += month_days[tp->tm_mon] * SECSPERDAY;
                if  (tp->tm_year % 4 == 0 && tp->tm_mon == 1)
                        newtime += SECSPERDAY;
                break;
        case  -WORKW_MONTW:
                tp = localtime(&newtime);
                newtime -= month_days[(tp->tm_mon + 11) % 12] * SECSPERDAY;
                if  (tp->tm_year % 4 == 0 && tp->tm_mon == 2)
                        newtime -= SECSPERDAY;
                break;
        case  WORKW_YEARTW:
                tp = localtime(&newtime);
                newtime += 365 * SECSPERDAY;
                if  ((tp->tm_year % 4 == 0  &&  tp->tm_mon <= 1)  ||
                     (tp->tm_year % 4 == 3  &&  tp->tm_mon > 1))
                        newtime += SECSPERDAY;
                break;
        case  -WORKW_YEARTW:
                tp = localtime(&newtime);
                newtime -= 365 * SECSPERDAY;
                if  ((tp->tm_year % 4 == 1  &&  tp->tm_mon <= 1)  ||
                     (tp->tm_year % 4 == 0  &&  tp->tm_mon > 1))
                        newtime -= SECSPERDAY;
                break;
        }
        if  (ctimec.tc_nexttime >= newtime  &&  newtime < time((time_t *) 0))
                return;
        ctimec.tc_nexttime = newtime;
        fillintimes();
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) time_adj, (XtPointer) amount);
}

/* Callback for arrows on bits of dates.  */

static void  time_cb(Widget w, int amount, XmArrowButtonCallbackStruct *cbs)
{
        if  (!ctimec.tc_istime) /* Ignore if no time */
             return;

        if  (cbs->reason == XmCR_ARM)
                time_adj(amount, NULL);
        else
                CLEAR_ARROW_TIMER
}

/* Timeout routine for repeat interval.  */

static void  rint_adj(int amount, XtIntervalId *id)
{
        ULONG   newrate = ctimec.tc_rate + amount;
        if  (newrate == 0)
                return;
        ctimec.tc_rate = newrate;
        fillintimes();
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) rint_adj, (XtPointer) amount);
}

/* Arrows on repeat intervals */

static void  rint_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (!ctimec.tc_istime || ctimec.tc_repeat < TC_MINUTES)
             return;

        if  (cbs->reason == XmCR_ARM)
                rint_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}

/* Timeout routine for day number on month beginning/end */

static void  mday_adj(int amount, XtIntervalId *id)
{
        struct  tm      *tp = localtime(&ctimec.tc_nexttime);
        int     daysinmon;
        month_days[1] = tp->tm_year % 4 == 0? 29: 28;
        daysinmon = month_days[tp->tm_mon];

        if  (ctimec.tc_repeat == TC_MONTHSB)  {
                int  newday = ctimec.tc_mday + amount;
                if  (newday <= 0  ||  newday > daysinmon)
                        return;
                ctimec.tc_mday = (unsigned char) newday;
        }
        else  {
                int  newday = (int) ctimec.tc_mday - amount;
                if  (newday < 0  ||  newday >= daysinmon)
                        return;
                ctimec.tc_mday = (unsigned char) newday;
        }
        fillintimes();
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) mday_adj, (XtPointer) amount);
}

static void  mday_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (!ctimec.tc_istime || (ctimec.tc_repeat != TC_MONTHSB && ctimec.tc_repeat != TC_MONTHSE))
                return;
        if  (cbs->reason == XmCR_ARM)
                mday_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}
#endif

static void  tim_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                int     cnt;
                XtManageChild(timesubw);
                ctimec.tc_istime = 1;
                ctimec.tc_nexttime = ((time((time_t *) 0) + 59L) / 60L) * 60L;
                ctimec.tc_repeat = default_interval;
                ctimec.tc_rate = default_rate;
                if  (ctimec.tc_repeat >= TC_MINUTES)  {
                        if  (ctimec.tc_rate > ratelim[ctimec.tc_repeat-TC_MINUTES])
                                ctimec.tc_rate = ratelim[ctimec.tc_repeat-TC_MINUTES];
                        XtManageChild(repsubw);
                }
                else
                        XtUnmanageChild(repsubw);
                if  (ctimec.tc_repeat == TC_MONTHSB  ||  ctimec.tc_repeat == TC_MONTHSE)
                        XtManageChild(mdayspin);
                else
                        XtUnmanageChild(mdayspin);
                ctimec.tc_nposs = default_nposs;
                for  (cnt = 0;  cnt < XtNumber(wlist);  cnt++)
                        XmToggleButtonGadgetSetState(workw[wlist[cnt].workw], ctimec.tc_repeat == cnt + TC_DELETE? True: False, False);
                ctimec.tc_nvaldays = defavoid;
                for  (cnt = 0;  cnt < XtNumber(avlist);  cnt++)
                        XmToggleButtonGadgetSetState(workw[avlist[cnt].workw], ctimec.tc_nvaldays & (1 << cnt)?True: False, False);
                XmToggleButtonGadgetSetState(workw[WORKW_NPSKIP+default_nposs-TC_SKIP], True, False);
        }
        else  {
                XtUnmanageChild(timesubw);
                ctimec.tc_istime = 0;
        }
        fillintimes();
}

static void  int_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (n >= TC_MINUTES)
                        XtManageChild(repsubw);
                else
                        XtUnmanageChild(repsubw);
                if  (n == TC_MONTHSB  ||  n == TC_MONTHSE)  {
                        XtManageChild(mdayspin);
                        XtManageChild(mdaylab);
                }
                else  {
                        XtUnmanageChild(mdayspin);
                        XtUnmanageChild(mdaylab);
                }
                if  (ctimec.tc_repeat < TC_MINUTES  &&  n >= TC_MINUTES)  {
                        int     cnt;
                        ctimec.tc_rate = default_rate;
                        ctimec.tc_nposs = default_nposs;
                        if  (ctimec.tc_rate > ratelim[n-TC_MINUTES])
                                ctimec.tc_rate = ratelim[n-TC_MINUTES];
                        for  (cnt = TC_SKIP;  cnt <= TC_CATCHUP;  cnt++)
                                XmToggleButtonGadgetSetState(workw[WORKW_NPSKIP+cnt-TC_SKIP], ctimec.tc_nposs == cnt? True: False, False);
                }
                ctimec.tc_repeat = (unsigned char) n;
                if  (n == TC_MONTHSB  ||  n == TC_MONTHSE)  {
                        struct  tm  *tp = localtime(&ctimec.tc_nexttime);
                        if  (n == TC_MONTHSB)
                                ctimec.tc_mday = (unsigned char) tp->tm_mday;
                        else  { /* Come back Brutus & Cassius all is forgiven */
                                month_days[1] = tp->tm_year % 4 == 0? 29: 28;
                                ctimec.tc_mday = (unsigned char) (month_days[tp->tm_mon] - tp->tm_mday);
                        }
                }
                fillintimes();
        }
}

static void  av_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                ctimec.tc_nvaldays |= 1 << n;
        else
                ctimec.tc_nvaldays &= ~(1 << n);
        fillintimes();
}

static void  sk_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                ctimec.tc_nposs = (unsigned char) n;
}

static void  endstime(Widget w, int data)
{
#ifdef XMBTQ_JCALL
        if  (data  &&  !nochanges)  {
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
#endif
#ifdef  XMBTR_JCALL
        if  (data)  {
#endif
                if  (ctimec.tc_istime)  {
                        if  ((ctimec.tc_nvaldays & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)  {
                                doerror(w, $EH{xmbtq all days set});
                                return;
                        }
                        if  (ctimec.tc_nexttime < time((time_t *) 0))  {
                                doerror(w, $EH{xmbtq time passed});
                                return;
                        }
                }
#ifdef  XMBTQ_JCALL
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                bjp->h.bj_times = ctimec;
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
#endif
#ifdef  XMBTR_JCALL
                cjob->job->h.bj_times = ctimec;
                cjob->changes++;
                cjob->nosubmit++;
#endif
        }
        XtDestroyWidget(GetTopShell(w));
}

#ifdef  XMBTQ_JCALL
void  cb_stime(Widget parent)
#endif
#ifdef  XMBTR_JCALL
void  cb_stime(Widget parent, int isjob)
#endif
{
#ifdef  XMBTQ_JCALL
        BtjobRef        cj = getselectedjob(BTM_WRITE);
#endif
#ifdef  XMBTR_JCALL
        BtjobRef        cj;
#endif
        Widget          st_shell, panew, mainform, prevabove, prevleft, intrc, avrc, noprc, timespin;
        TimeconRef      tc;
        int             cnt;
#ifdef  XMBTQ_JCALL
        unsigned        readonly;
#endif

#ifdef  XMBTQ_JCALL
        if  (!cj)
                return;
        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                doerror(jwid, $EH{xmbtq job running});
                return;
        }
        cjob = cj;
        nochanges = 0;
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        init_tdefaults();
#endif
#ifdef  XMBTR_JCALL
        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;
#endif
        tc = &cj->h.bj_times;
        ctimec = *tc;

        prevabove = CreateJeditDlg(parent, "Stime", &st_shell, &panew, &mainform);
        prevleft = XtVaCreateManagedWidget("stime",
                                           xmToggleButtonGadgetClass,   mainform,
                                           XmNtopAttachment,            XmATTACH_FORM,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               prevabove,
                                           XmNborderWidth,              0,
                                           NULL);
        if  (tc->tc_istime)
                XmToggleButtonGadgetSetState(prevleft, True, False);
#ifdef  XMBTQ_JCALL
        if  (readonly)
                XtSetSensitive(prevleft, False);
        else
#endif
                XtAddCallback(prevleft, XmNvalueChangedCallback, (XtCallbackProc) tim_turn, (XtPointer) 0);

        timesubw = XtVaCreateWidget("timsub",
                                    xmFormWidgetClass,          mainform,
                                    XmNfractionBase,            6,
                                    XmNtopAttachment,           XmATTACH_WIDGET,
                                    XmNtopWidget,               prevabove,
                                    XmNleftAttachment,          XmATTACH_FORM,
                                    NULL);

        if  (tc->tc_istime)
                XtManageChild(timesubw);

#ifdef HAVE_XM_SPINB_H
        timespin = prevabove = XtVaCreateManagedWidget("timesp",
                                                       xmSpinBoxWidgetClass,    timesubw,
                                                       XmNtopAttachment,        XmATTACH_FORM,
                                                       XmNleftAttachment,       XmATTACH_FORM,
                                                       XmNarrowLayout,          XmARROWS_SPLIT,
#ifdef  XMBTQ_JCALL
                                                       XmNarrowSensitivity,     readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                                       NULL);


        workw[WORKW_HOURTW] = XtVaCreateManagedWidget("hour",
                                                      xmTextFieldWidgetClass,   timespin,
                                                      XmNcolumns,               2,
                                                      XmNmaxWidth,              2,
                                                      XmNeditable,              False,
                                                      XmNcursorPositionVisible, False,
                                                      XmNspinBoxChildType,      XmSTRING,
                                                      XmNnumValues,             24,
                                                      XmNvalues,                timezerof,
                                                      NULL);

        workw[WORKW_MINTW] = XtVaCreateManagedWidget("min",
                                                     xmTextFieldWidgetClass,    timespin,
                                                     XmNcolumns,                2,
                                                     XmNmaxWidth,               2,
                                                     XmNeditable,               False,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNspinBoxChildType,       XmSTRING,
                                                     XmNnumValues,              60,
                                                     XmNvalues,                 timezerof,
                                                     NULL);

        workw[WORKW_DOWTW] = XtVaCreateManagedWidget("dow",
                                                     xmTextFieldWidgetClass,    timespin,
                                                     XmNcolumns,                longest_day,
                                                     XmNmaxWidth,               longest_day,
                                                     XmNeditable,               False,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNspinBoxChildType,       XmSTRING,
                                                     XmNnumValues,              7,
                                                     XmNvalues,                 stdaynames,
                                                     NULL);

        workw[WORKW_DOMTW] = XtVaCreateManagedWidget("dm",
                                                     xmTextFieldWidgetClass,    timespin,
                                                     XmNcolumns,                2,
                                                     XmNmaxWidth,               2,
                                                     XmNeditable,               False,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNspinBoxChildType,       XmNUMERIC,
                                                     XmNmaximumValue,           31,
                                                     XmNminimumValue,           1,
#ifdef  BROKEN_SPINBOX
                                                     XmNpositionType,           XmPOSITION_INDEX,
                                                     XmNposition,               0,
#else
                                                     XmNposition,               1,
#endif
                                                     NULL);

        workw[WORKW_MONTW] = XtVaCreateManagedWidget("mon",
                                                     xmTextFieldWidgetClass,    timespin,
                                                     XmNcolumns,                longest_mon,
                                                     XmNmaxWidth,               longest_mon,
                                                     XmNeditable,               False,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNspinBoxChildType,       XmSTRING,
                                                     XmNnumValues,              12,
                                                     XmNvalues,                 stmonnames,
                                                     NULL);

        workw[WORKW_YEARTW] = XtVaCreateManagedWidget("yr",
                                                      xmTextFieldWidgetClass,   timespin,
                                                      XmNcolumns,               4,
                                                      XmNmaxWidth,              4,
                                                      XmNeditable,              False,
                                                      XmNcursorPositionVisible, False,
                                                      XmNspinBoxChildType,      XmNUMERIC,
                                                      XmNmaximumValue,          9999,
                                                      XmNminimumValue,          1900,
#ifdef  BROKEN_SPINBOX
                                                      XmNpositionType,          XmPOSITION_INDEX,
                                                      XmNposition,              0,
#else
                                                      XmNposition,              1900,
#endif
                                                      NULL);

        XtAddCallback(timespin, XmNmodifyVerifyCallback, (XtCallbackProc) time_adj, 0);
#else
        timespin =
        prevleft = workw[WORKW_HOURTW] = XtVaCreateManagedWidget("hour",
                                                                 xmTextFieldWidgetClass,        timesubw,
                                                                 XmNcolumns,                    2,
                                                                 XmNmaxWidth,                   2,
                                                                 XmNeditable,                   False,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_FORM,
                                                                 XmNleftAttachment,             XmATTACH_FORM,
                                                                 NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("h", timesubw, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_HOURTW, WORKW_HOURTW);

        prevleft = place_label_top(timesubw, prevleft, ":");

        prevleft = workw[WORKW_MINTW] = XtVaCreateManagedWidget("min",
                                                                xmTextFieldWidgetClass,         timesubw,
                                                                XmNcolumns,                     2,
                                                                XmNmaxWidth,                    2,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNeditable,                    False,
                                                                XmNtopAttachment,               XmATTACH_FORM,
                                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                                XmNleftWidget,                  prevleft,
                                                                NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("m", timesubw, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_MINTW, WORKW_MINTW);

        prevleft = workw[WORKW_DOWTW] = XtVaCreateManagedWidget("dow",
                                                                xmTextFieldWidgetClass,         timesubw,
                                                                XmNcolumns,                     longest_day,
                                                                XmNmaxWidth,                    longest_day,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNeditable,                    False,
                                                                XmNtopAttachment,               XmATTACH_FORM,
                                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                                XmNleftWidget,                  prevleft,
                                                                NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("dw", timesubw, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_DOWTW, WORKW_DOWTW);

        prevleft = workw[WORKW_DOMTW] = XtVaCreateManagedWidget("dm",
                                                                xmTextFieldWidgetClass,         timesubw,
                                                                XmNcolumns,                     2,
                                                                XmNmaxWidth,                    2,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNeditable,                    False,
                                                                XmNtopAttachment,               XmATTACH_FORM,
                                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                                XmNleftWidget,                  prevleft,
                                                                NULL);
#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("dm", timesubw, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_DOMTW, WORKW_DOMTW);

        prevleft = workw[WORKW_MONTW] = XtVaCreateManagedWidget("mon",
                                                                xmTextFieldWidgetClass,         timesubw,
                                                                XmNcolumns,                     longest_mon,
                                                                XmNmaxWidth,                    longest_mon,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNeditable,                    False,
                                                                XmNtopAttachment,               XmATTACH_FORM,
                                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                                XmNleftWidget,                  prevleft,
                                                                NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("mon", timesubw, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_MONTW, WORKW_MONTW);

        prevleft = workw[WORKW_YEARTW] = XtVaCreateManagedWidget("yr",
                                                                 xmTextFieldWidgetClass,        timesubw,
                                                                 XmNcolumns,                    4,
                                                                 XmNmaxWidth,                   4,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNeditable,                   False,
                                                                 XmNtopAttachment,              XmATTACH_FORM,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("yr", timesubw, 0, prevleft, (XtCallbackProc) time_cb, (XtCallbackProc) time_cb, -WORKW_YEARTW, WORKW_YEARTW);

        prevabove = workw[WORKW_YEARTW];

#endif /* ! HAVE_XM_SPINB_H */

        intrc = XtVaCreateManagedWidget("inttype",
                                        xmRowColumnWidgetClass,         timesubw,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   timespin,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  3,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               True,
                                        NULL);

        for  (cnt = 0;  cnt < XtNumber(wlist);  cnt++)  {
                Widget  w = workw[wlist[cnt].workw] = XtVaCreateManagedWidget(wlist[cnt].wname, xmToggleButtonGadgetClass, intrc, XmNborderWidth, 0, NULL);
                if  (tc->tc_istime  &&  tc->tc_repeat == cnt + TC_DELETE)
                        XmToggleButtonGadgetSetState(w, True, False);
#ifdef  XMBTQ_JCALL
                if  (readonly)
                        XtSetSensitive(w, False);
                else
#endif
                        XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) int_turn, INT_TO_XTPOINTER(cnt + TC_DELETE));
        }

        prevabove = intrc;

        repsubw = XtVaCreateWidget("repsub",
                                   xmFormWidgetClass,           timesubw,
                                   XmNfractionBase,             6,
                                   XmNtopAttachment,            XmATTACH_WIDGET,
                                   XmNtopWidget,                prevabove,
                                   XmNleftAttachment,           XmATTACH_FORM,
                                   NULL);

        if  (tc->tc_repeat >= TC_MINUTES)
                XtManageChild(repsubw);

#ifdef HAVE_XM_SPINB_H

        prevleft = place_label_topleft(repsubw, "Revery");
        timespin = prevleft = XtVaCreateManagedWidget("rintsp",
                                                      xmSpinBoxWidgetClass,     repsubw,
                                                      XmNtopAttachment,         XmATTACH_FORM,
                                                      XmNleftAttachment,        XmATTACH_WIDGET,
                                                      XmNleftWidget,            prevleft,
#ifdef  XMBTQ_JCALL
                                                      XmNarrowSensitivity,      readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                                      NULL);

        workw[WORKW_RINTERVAL] = XtVaCreateManagedWidget("rint",
                                                         xmTextFieldWidgetClass,        timespin,
                                                         XmNcolumns,                    4,
                                                         XmNmaxWidth,                   4,
                                                         XmNcursorPositionVisible,      False,
                                                         XmNeditable,                   False,
                                                         XmNspinBoxChildType,           XmNUMERIC,
                                                         XmNmaximumValue,               9999,
                                                         XmNminimumValue,               1,
#ifdef  BROKEN_SPINBOX
                                                         XmNpositionType,               XmPOSITION_INDEX,
                                                         XmNposition,                   0,
#else
                                                         XmNposition,                   1,
#endif
                                                         NULL);

        XtAddCallback(timespin, XmNmodifyVerifyCallback, (XtCallbackProc) rint_adj, 0);

        mdaylab = prevleft = place_label_top(repsubw, prevleft, "Mthday");

        mdayspin = XtVaCreateWidget("mdaysp",
                                    xmSpinBoxWidgetClass,       repsubw,
                                    XmNtopAttachment,           XmATTACH_FORM,
                                    XmNleftAttachment,          XmATTACH_WIDGET,
                                    XmNleftWidget,              prevleft,
#ifdef  XMBTQ_JCALL
                                    XmNarrowSensitivity,        readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                    NULL);

        workw[WORKW_RMTHDAY] = XtVaCreateManagedWidget("mday",
                                                       xmTextFieldWidgetClass,          mdayspin,
                                                       XmNcolumns,                      2,
                                                       XmNmaxWidth,                     2,
                                                       XmNcursorPositionVisible,        False,
                                                       XmNeditable,                     False,
                                                       XmNspinBoxChildType,             XmNUMERIC,
                                                       XmNmaximumValue,                 31,
                                                       XmNminimumValue,                 1,
#ifdef  BROKEN_SPINBOX
                                                       XmNpositionType,                 XmPOSITION_INDEX,
                                                       XmNposition,                     0,
#else
                                                       XmNposition,                     1,
#endif
                                                       NULL);

        XtAddCallback(mdayspin, XmNmodifyVerifyCallback, (XtCallbackProc) mday_adj, 0);

        prevabove = timespin;

#else
        prevleft = place_label_topleft(repsubw, "Revery");
        prevleft = workw[WORKW_RINTERVAL] = XtVaCreateManagedWidget("rint",
                                                                    xmTextFieldWidgetClass,             repsubw,
                                                                    XmNcolumns,                         10,
                                                                    XmNmaxWidth,                        10,
                                                                    XmNcursorPositionVisible,           False,
                                                                    XmNeditable,                        False,
                                                                    XmNtopAttachment,                   XmATTACH_FORM,
                                                                    XmNleftAttachment,                  XmATTACH_WIDGET,
                                                                    XmNleftWidget,                      prevleft,
                                                                    NULL);
#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("rint", repsubw, 0, prevleft, (XtCallbackProc) rint_cb, (XtCallbackProc) rint_cb, 1, -1);

        mdaylab = prevleft = place_label_top(repsubw, prevleft, "Mthday");

        mdayspin = XtVaCreateWidget("mdaysp",
                                    xmFormWidgetClass,          repsubw,
                                    XmNtopAttachment,           XmATTACH_FORM,
                                    XmNleftAttachment,          XmATTACH_WIDGET,
                                    XmNleftWidget,              prevleft,
                                    NULL);


        prevleft = workw[WORKW_RMTHDAY] = XtVaCreateManagedWidget("mday",
                                                                  xmTextFieldWidgetClass,       mdayspin,
                                                                  XmNcolumns,                   2,
                                                                  XmNmaxWidth,                  2,
                                                                  XmNcursorPositionVisible,     False,
                                                                  XmNeditable,                  False,
                                                                  XmNtopAttachment,             XmATTACH_FORM,
                                                                  XmNleftAttachment,            XmATTACH_FORM,
                                                                  NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("mday", mdayspin, 0, prevleft, (XtCallbackProc) mday_cb, (XtCallbackProc) mday_cb, 1, -1);

        prevabove = workw[WORKW_RINTERVAL];

#endif /* ! HAVE_XM_SPINB_H */

        if  (tc->tc_repeat == TC_MONTHSB  || tc->tc_repeat == TC_MONTHSE)
                XtManageChild(mdayspin);
        else
                XtUnmanageChild(mdaylab);

        avrc = XtVaCreateManagedWidget("avdays",
                                       xmRowColumnWidgetClass,          repsubw,
                                       XmNtopAttachment,                XmATTACH_WIDGET,
                                       XmNtopWidget,                    prevabove,
                                       XmNleftAttachment,               XmATTACH_FORM,
                                       XmNpacking,                      XmPACK_COLUMN,
                                       XmNnumColumns,                   4,
                                       XmNisHomogeneous,                True,
                                       XmNentryClass,                   xmToggleButtonGadgetClass,
                                       XmNradioBehavior,                False,
                                       NULL);

        for  (cnt = 0;  cnt < XtNumber(avlist);  cnt++)  {
                Widget  w = workw[avlist[cnt].workw] = XtVaCreateManagedWidget(avlist[cnt].wname, xmToggleButtonGadgetClass, avrc, XmNborderWidth, 0, NULL);
                if  (tc->tc_istime  &&  tc->tc_nvaldays & (1 << cnt))
                        XmToggleButtonGadgetSetState(w, True, False);
#ifdef  XMBTQ_JCALL
                if  (readonly)
                        XtSetSensitive(w, False);
                else
#endif
                        XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) av_turn, INT_TO_XTPOINTER(cnt));
        }

        prevabove = avrc;

        prevabove = place_label_left(repsubw, prevabove, "comesto");
        prevabove = workw[WORKW_RESULTREP] = XtVaCreateManagedWidget("result",
                                                                     xmTextFieldWidgetClass,    repsubw,
                                                                     XmNcursorPositionVisible,  False,
                                                                     XmNeditable,               False,
                                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                                     XmNtopWidget,              prevabove,
                                                                     XmNleftAttachment,         XmATTACH_FORM,
                                                                     XmNrightAttachment,        XmATTACH_FORM,
                                                                     NULL);

        noprc = XtVaCreateManagedWidget("nop",
                                        xmRowColumnWidgetClass,         repsubw,
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

        workw[WORKW_NPSKIP] = XtVaCreateManagedWidget("Skip", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        workw[WORKW_NPWAIT1] = XtVaCreateManagedWidget("Delay", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        workw[WORKW_NPWAITALL] = XtVaCreateManagedWidget("Delall", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        workw[WORKW_NPCATCHUP] = XtVaCreateManagedWidget("Catchup", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        if  (tc->tc_istime && tc->tc_repeat >= TC_MINUTES)
                XmToggleButtonGadgetSetState(workw[WORKW_NPSKIP+tc->tc_nposs-TC_SKIP], True, False);
#ifdef  XMBTQ_JCALL
        if  (readonly)  {
                nochanges = 1;
                XtSetSensitive(workw[WORKW_NPSKIP], False);
                XtSetSensitive(workw[WORKW_NPWAIT1], False);
                XtSetSensitive(workw[WORKW_NPWAITALL], False);
                XtSetSensitive(workw[WORKW_NPCATCHUP], False);
        }
        else  {
#endif
                XtAddCallback(workw[WORKW_NPSKIP], XmNvalueChangedCallback, (XtCallbackProc) sk_turn, (XtPointer) TC_SKIP);
                XtAddCallback(workw[WORKW_NPWAIT1], XmNvalueChangedCallback, (XtCallbackProc) sk_turn, (XtPointer) TC_WAIT1);
                XtAddCallback(workw[WORKW_NPWAITALL], XmNvalueChangedCallback, (XtCallbackProc) sk_turn, (XtPointer) TC_WAITALL);
                XtAddCallback(workw[WORKW_NPCATCHUP], XmNvalueChangedCallback, (XtCallbackProc) sk_turn, (XtPointer) TC_CATCHUP);
#ifdef  XMBTQ_JCALL
        }
#endif
        fillintimes();
        XtManageChild(mainform);
        CreateActionEndDlg(st_shell, panew, (XtCallbackProc) endstime, $H{xmbtq set time dialog});
}

#ifdef  XMBTQ_JCALL
#ifdef HAVE_XM_SPINB_H
static void  defrint_adj(Widget w, XtPointer wh, XmSpinBoxCallbackStruct *cbs)
{
        int     newpos = cbs->position;
#ifdef  BROKEN_SPINBOX
        newpos++;
#endif
        if  (default_interval < TC_MINUTES  ||  (ULONG) newpos > ratelim[default_interval-TC_MINUTES])  {
                cbs->doit = False;
                return;
        }
        default_rate = newpos;
}

static void  defint_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (n >= TC_MINUTES)  {
                        XtManageChild(repsubw);
                        if  (default_rate > ratelim[n-TC_MINUTES])  {
                                default_rate = ratelim[n-TC_MINUTES];
#ifdef  BROKEN_SPINBOX
                                XtVaSetValues(workw[WORKW_RINTERVAL], XmNposition, default_rate-1, (XtPointer) 0);
#else
                                XtVaSetValues(workw[WORKW_RINTERVAL], XmNposition, default_rate, (XtPointer) 0);
#endif
                        }
                }
                else
                        XtUnmanageChild(repsubw);
                default_interval = (unsigned char) n;
        }
}

#else
static void  defint_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                if  (default_interval < TC_MINUTES  &&  n >= TC_MINUTES)  {
                        char    nbuf[20];
                        sprintf(nbuf, "%lu", default_rate);
                        XmTextSetString(workw[WORKW_RINTERVAL], nbuf);
                        XtManageChild(repsubw);
                }
                else  if  (n < TC_MINUTES  &&  default_interval >= TC_MINUTES)
                        XtUnmanageChild(repsubw);
                default_interval = (unsigned char) n;
        }
}

/* Timeout routine for repeat interval.  */

static void  defrint_adj(int amount, XtIntervalId *id)
{
        char    nbuf[20];
        ULONG   newrate = default_rate + amount;
        if  (newrate == 0)
                return;
        default_rate = newrate;
        sprintf(nbuf, "%lu", default_rate);
        XmTextSetString(workw[WORKW_RINTERVAL], nbuf);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) defrint_adj, (XtPointer) amount);
}

/* Arrows on repeat intervals */

static void  defrint_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (default_interval < TC_MINUTES)
             return;

        if  (cbs->reason == XmCR_ARM)
                defrint_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}
#endif

static void  defav_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                defavoid |= 1 << n;
        else
                defavoid &= ~(1 << n);
}

static void  defsk_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                default_nposs = (unsigned char) n;
}

static void  enddeftime(Widget w)
{
        XtDestroyWidget(GetTopShell(w));
}

void  cb_deftime(Widget parent)
{
        Widget          dt_shell, panew, mainform, prevabove, prevleft, intrc, avrc, noprc;
#ifdef HAVE_XM_SPINB_H
        Widget          timespin;
#endif
        int             cnt;

        init_tdefaults();
        CreateEditDlg(parent, "timedef", &dt_shell, &panew, &mainform, 3);
        intrc = XtVaCreateManagedWidget("inttype",
                                        xmRowColumnWidgetClass,         mainform,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  3,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               True,
                                        NULL);

        for  (cnt = 0;  cnt < XtNumber(wlist);  cnt++)  {
                workw[wlist[cnt].workw] = XtVaCreateManagedWidget(wlist[cnt].wname,
                                                                  xmToggleButtonGadgetClass,    intrc,
                                                                  XmNborderWidth,               0,
                                                                  NULL);
                XtAddCallback(workw[wlist[cnt].workw],
                              XmNvalueChangedCallback,
                              (XtCallbackProc) defint_turn, INT_TO_XTPOINTER(cnt + TC_DELETE));
                if  (default_interval == cnt + TC_DELETE)
                        XmToggleButtonGadgetSetState(workw[wlist[cnt].workw], True, False);
#ifdef HAVE_XM_SPINB_H
                XtAddCallback(workw[wlist[cnt].workw], XmNvalueChangedCallback, (XtCallbackProc) defint_turn, INT_TO_XTPOINTER(cnt + TC_DELETE));
#endif
        }

        prevabove = intrc;
        repsubw = XtVaCreateWidget("repsub",
                                   xmFormWidgetClass,           mainform,
                                   XmNfractionBase,             6,
                                   XmNtopAttachment,            XmATTACH_WIDGET,
                                   XmNtopWidget,                intrc,
                                   XmNleftAttachment,           XmATTACH_FORM,
                                   NULL);

        if  (default_interval >= TC_MINUTES)  {
                if  (default_rate > ratelim[default_interval-TC_MINUTES])
                        default_rate = ratelim[default_interval-TC_MINUTES];
                XtManageChild(repsubw);
        }

#ifdef HAVE_XM_SPINB_H
        prevleft = place_label_topleft(repsubw, "Revery");
        timespin = prevleft = XtVaCreateManagedWidget("rintsp",
                                                      xmSpinBoxWidgetClass,     repsubw,
                                                      XmNtopAttachment,         XmATTACH_FORM,
                                                      XmNleftAttachment,        XmATTACH_WIDGET,
                                                      XmNleftWidget,            prevleft,
                                                      NULL);

        workw[WORKW_RINTERVAL] = XtVaCreateManagedWidget("rint",
                                                         xmTextFieldWidgetClass,        timespin,
                                                         XmNcolumns,                    4,
                                                         XmNmaxWidth,                   4,
                                                         XmNcursorPositionVisible,      False,
                                                         XmNeditable,                   False,
                                                         XmNspinBoxChildType,           XmNUMERIC,
                                                         XmNmaximumValue,               9999,
                                                         XmNminimumValue,               1,
#ifdef  BROKEN_SPINBOX
                                                         XmNpositionType,               XmPOSITION_INDEX,
                                                         XmNposition,                   default_rate-1,
#else
                                                         XmNposition,                   default_rate,
#endif

                                                         NULL);

        XtAddCallback(timespin, XmNmodifyVerifyCallback, (XtCallbackProc) defrint_adj, 0);

        prevabove = timespin;
#else
        prevleft = place_label_topleft(repsubw, "Revery");
        prevleft = workw[WORKW_RINTERVAL] = XtVaCreateManagedWidget("rint",
                                                                    xmTextFieldWidgetClass,     repsubw,
                                                                    XmNcolumns,                 10,
                                                                    XmNmaxWidth,                10,
                                                                    XmNcursorPositionVisible,   False,
                                                                    XmNeditable,                False,
                                                                    XmNtopAttachment,           XmATTACH_FORM,
                                                                    XmNleftAttachment,          XmATTACH_WIDGET,
                                                                    XmNleftWidget,              prevleft,
                                                                    NULL);

        if  (default_interval >= TC_MINUTES)  {
                char    nbuf[20];
                sprintf(nbuf, "%lu", default_rate);
                XmTextSetString(prevleft, nbuf);
        }

        CreateArrowPair("rint", repsubw, 0, prevleft, (XtCallbackProc) defrint_cb, (XtCallbackProc) defrint_cb, 1, -1);
        prevabove = workw[WORKW_RINTERVAL];
#endif

        avrc = XtVaCreateManagedWidget("avdays",
                                       xmRowColumnWidgetClass,          repsubw,
                                       XmNtopAttachment,                XmATTACH_WIDGET,
                                       XmNtopWidget,                    prevabove,
                                       XmNleftAttachment,               XmATTACH_FORM,
                                       XmNpacking,                      XmPACK_COLUMN,
                                       XmNnumColumns,                   4,
                                       XmNisHomogeneous,                True,
                                       XmNentryClass,                   xmToggleButtonGadgetClass,
                                       XmNradioBehavior,                False,
                                       NULL);

        for  (cnt = 0;  cnt < XtNumber(avlist);  cnt++)  {
                workw[avlist[cnt].workw] = XtVaCreateManagedWidget(avlist[cnt].wname,
                                                                   xmToggleButtonGadgetClass,   avrc,
                                                                   XmNborderWidth,              0,
                                                                   NULL);
                XtAddCallback(workw[avlist[cnt].workw], XmNvalueChangedCallback, (XtCallbackProc) defav_turn, INT_TO_XTPOINTER(cnt));
                if  (defavoid & (1 << cnt))
                        XmToggleButtonGadgetSetState(workw[avlist[cnt].workw], True, False);
        }

        prevabove = avrc;

        noprc = XtVaCreateManagedWidget("nop",
                                        xmRowColumnWidgetClass,         repsubw,
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

        workw[WORKW_NPSKIP] = XtVaCreateManagedWidget("Skip", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        workw[WORKW_NPWAIT1] = XtVaCreateManagedWidget("Delay", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        workw[WORKW_NPWAITALL] = XtVaCreateManagedWidget("Delall", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        workw[WORKW_NPCATCHUP] = XtVaCreateManagedWidget("Catchup", xmToggleButtonGadgetClass, noprc, XmNborderWidth, 0, NULL);
        XtAddCallback(workw[WORKW_NPSKIP], XmNvalueChangedCallback, (XtCallbackProc) defsk_turn, (XtPointer) TC_SKIP);
        XtAddCallback(workw[WORKW_NPWAIT1], XmNvalueChangedCallback, (XtCallbackProc) defsk_turn, (XtPointer) TC_WAIT1);
        XtAddCallback(workw[WORKW_NPWAITALL], XmNvalueChangedCallback, (XtCallbackProc) defsk_turn, (XtPointer) TC_WAITALL);
        XtAddCallback(workw[WORKW_NPCATCHUP], XmNvalueChangedCallback, (XtCallbackProc) defsk_turn, (XtPointer) TC_CATCHUP);
        XmToggleButtonGadgetSetState(workw[WORKW_NPSKIP+default_nposs-TC_SKIP], True, False);
        XtManageChild(mainform);
        CreateActionEndDlg(dt_shell, panew, (XtCallbackProc) enddeftime, $H{xmbtq default time dialog});
}
#endif /* XMBTQ_JCALL */

static void  endtitprill(Widget w, int data)
{
        if  (data)  {
#ifdef  XMBTQ_JCALL
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
#endif
#ifdef  XMBTR_JCALL
                BtjobRef        bjp = cjob->job;
                Btjob           njob;
#endif
                char            *txt;
                int             cinumber;
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
                XtVaGetValues(workw[WORKW_ITXTW], XmNselectedPosition, &cinumber, NULL);
#else
                XtVaGetValues(workw[WORKW_ITXTW], XmNvalue, &txt, NULL);
                cinumber = validate_ci(txt);
                XtFree(txt);
                if  (cinumber < 0)  {
                        doerror(w, $EH{xmbtq unknown ci});
                        return;
                }
#endif
                XtVaGetValues(workw[WORKW_TITTW], XmNvalue, &txt, NULL);
#ifdef  XMBTQ_JCALL
                if  (jobqueue)  {
#endif
#ifdef  XMBTR_JCALL
                if  (cjob->jobqueue)  {
#endif
                        char    *ntit;
#ifdef  XMBTQ_JCALL
                        if  (!(ntit = malloc((unsigned) (strlen(jobqueue) + strlen(txt) + 2))))
                                ABORT_NOMEMINL;
                        sprintf(ntit, "%s:%s", jobqueue, txt);
#endif
#ifdef  XMBTR_JCALL
                        if  (!(ntit = malloc((unsigned) (strlen(cjob->jobqueue) + strlen(txt) + 2))))
                                ABORT_NOMEMINL;
                        sprintf(ntit, "%s:%s", cjob->jobqueue, txt);
#endif
                        XtFree(txt);
                        txt = ntit;
                }
#ifdef  XMBTQ_JCALL
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                if  (!repackjob(bjp, cjob, (char *) 0, txt, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        freexbuf(xindx);
#endif
#ifdef  XMBTR_JCALL
                njob = *bjp;
                if  (!repackjob(&njob, bjp, (char *) 0, txt, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
#endif
                        doerror(w, $EH{Too many job strings});
#ifdef  XMBTQ_JCALL
                        if  (jobqueue)
#endif
#ifdef  XMBTR_JCALL
                        if  (cjob->jobqueue)
#endif
                                free(txt);
                        else
                                XtFree(txt);
                        return;
                }
#ifdef  XMBTQ_JCALL
                if  (jobqueue)
#endif
#ifdef  XMBTR_JCALL
                *bjp = njob;
                if  (cjob->jobqueue)
#endif
                        free(txt);
                else
                        XtFree(txt);
                bjp->h.bj_pri = (unsigned char) GET_TEXTORSPINBOX_INT(WW(WORKW_PRITW));
                bjp->h.bj_ll = (USHORT) get_textbox_int(WW(WORKW_LLTXTW));
                strcpy(bjp->h.bj_cmdinterp, Ci_list[cinumber].ci_name);
#ifdef  XMBTQ_JCALL
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
#endif
#ifdef  XMBTR_JCALL
                if  (cjob != &default_pend)  {
                        int     indx = getselectedjob(0);
                        if  (indx >= 0)  {
                                XmString        str = jdisplay(cjob);
                                XmListReplaceItemsPos(jwid, &str, 1, indx+1);
                                XmStringFree(str);
                                XmListSelectPos(jwid, indx+1, False);
                        }
                }
                cjob->changes++;
                cjob->nosubmit++;
#endif
        }
        XtDestroyWidget(GetTopShell(w));
}

/* Timeout routine for load levels */

static void  ll_adj(int amount, XtIntervalId *id)
{
        char    *txt, nbuf[6];
        LONG    newll;
        XtVaGetValues(workw[WORKW_LLTXTW], XmNvalue, &txt, NULL);
        newll = atol(txt);
        XtFree(txt);
        if  (id)  {
                unsigned   pw10;
                LONG    incr = 1;
                for  (pw10 = 100;  pw10 < 10000  &&  newll > pw10;  pw10 *= 10)
                        incr *= 10;
                if  (amount >= 0)
                        newll += incr;
                else
                        newll -= incr;
                newll = (newll / incr) * incr;
        }
        else
                newll += amount;
        if  (newll <= 0  ||  newll > 0x0000FFFFL)
                return;
        sprintf(nbuf, "%5u", (USHORT) newll);
        XmTextSetString(workw[WORKW_LLTXTW], nbuf);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) ll_adj, INT_TO_XTPOINTER(amount));
}

void  ll_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                ll_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}

#ifdef  XMBTQ_JCALL
void  cb_titprill(Widget parent)
#endif
#ifdef  XMBTR_JCALL
void  cb_titprill(Widget parent, int isjob)
#endif
{
        Widget  tit_shell, panew, mainform, prevabove, prevleft;
#ifdef  HAVE_XM_SPINB_H
        int     curp;
#endif
#ifdef  XMBTQ_JCALL
        BtjobRef        cj = getselectedjob(BTM_WRITE);
        if  (!cj)
                return;
        cjob = cj;
#endif
#ifdef  XMBTR_JCALL
        BtjobRef        cj;
        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;
#endif

#ifdef HAVE_XM_SPINB_H
        /* This is a bit of a cheat in cases where the job has an
           existing priority which is outside the range of priorities
           allowed to the user, but spin boxes go bananas if you
           try to set values outside the range */
        curp = cj->h.bj_pri;
        if  (curp < (int) mypriv->btu_minp  ||  curp > (int) mypriv->btu_maxp)
                curp = mypriv->btu_defp;
#endif

        prevabove = CreateJeditDlg(parent, "Titpri", &tit_shell, &panew, &mainform);
        prevleft = place_label_left(mainform, prevabove, "title");
        prevabove = workw[WORKW_TITTW] = XtVaCreateManagedWidget("title",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

#ifdef  XMBTQ_JCALL
        XmTextSetString(prevabove, (char *) qtitle_of(cj));
#endif
#ifdef  XMBTR_JCALL
        XmTextSetString(prevabove, (char *) title_of(cjob->job));
#endif

        prevleft = place_label_left(mainform, prevabove, "priority");

#ifdef HAVE_XM_SPINB_H
        prevleft = XtVaCreateManagedWidget("prisp",
                                           xmSpinBoxWidgetClass,        mainform,
                                           XmNtopAttachment,            XmATTACH_WIDGET,
                                           XmNtopWidget,                prevabove,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               prevleft,
                                           NULL);

        workw[WORKW_PRITW] = XtVaCreateManagedWidget("pri",
                                                     xmTextFieldWidgetClass,    prevleft,
                                                     XmNcolumns,                3,
                                                     XmNeditable,               False,
                                                     XmNcursorPositionVisible,  False,
                                                     XmNmaximumValue,           (int) mypriv->btu_maxp,
                                                     XmNminimumValue,           (int) mypriv->btu_minp,
                                                     XmNspinBoxChildType,       XmNUMERIC,
#ifdef  BROKEN_SPINBOX
                                                     XmNpositionType,           XmPOSITION_INDEX,
                                                     XmNposition,               curp - (int) mypriv->btu_minp,
#else
                                                     XmNposition,               curp,
#endif
                                                     NULL);

        prevabove = CreateIselDialog(mainform, prevleft, cj->h.bj_cmdinterp);

#else
        prevleft = workw[WORKW_PRITW] = XtVaCreateManagedWidget("pri",
                                                                xmTextFieldWidgetClass,         mainform,
                                                                XmNcolumns,                     3,
                                                                XmNmaxWidth,                    3,
                                                                XmNeditable,                    False,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNtopAttachment,               XmATTACH_WIDGET,
                                                                XmNtopWidget,                   prevabove,
                                                                XmNleftAttachment,              XmATTACH_WIDGET,
                                                                XmNleftWidget,                  prevleft,
                                                                NULL);

        put_textbox_int(WW(WORKW_PRITW), cj->h.bj_pri, 30);

        CreateArrowPair("pri", mainform, prevabove, workw[WORKW_PRITW], (XtCallbackProc) priup_cb, (XtCallbackProc) pridn_cb, WORKW_PRITW, WORKW_PRITW);

        prevabove = CreateIselDialog(mainform, workw[WORKW_PRITW], cj->h.bj_cmdinterp);
#endif

        prevleft = place_label_left(mainform, prevabove, "loadlevel");
        prevleft = workw[WORKW_LLTXTW] = XtVaCreateManagedWidget("ll",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    5,
                                                                 XmNmaxWidth,                   5,
                                                                 XmNeditable,                   False,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

        put_textbox_int(WW(WORKW_LLTXTW), cj->h.bj_ll, 5);

        if  (mypriv->btu_priv & BTM_SPCREATE)
                CreateArrowPair("ll", mainform, prevabove, workw[WORKW_LLTXTW], (XtCallbackProc) ll_cb, (XtCallbackProc) ll_cb, 1, -1);
        XtManageChild(mainform);
        CreateActionEndDlg(tit_shell, panew, (XtCallbackProc) endtitprill, $H{xmbtq titprill dialog});
}

static void  umask_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                copyumask |= n;
        else
                copyumask &= ~n;
}

#ifdef  AMDAHL
ULONG  strtoul(char *buf, char **rubbish, int morerubbish)
{
        while (isspace(*buf))
                buf++;

        if  (buf[0] == '0'  &&  tolower(buf[1]) == 'x')  {
                ULONG  res = 0;
                buf += 2;
                while  (isxdigit(*buf))  {
                        res <<= 4;
                        if  (isdigit(*buf))
                                res += *buf - '0';
                        else
                                res += tolower(*buf) - 'a' + 10;
                        buf++;
                }
                return  res;
        }
        else
                return  atol(buf);
}
#endif

/* Timeout routine for load levels */

static void  ulim_adj(int amount, XtIntervalId *id)
{
        char    *txt, nbuf[10];
        ULONG   newul;
        XtVaGetValues(workw[WORKW_ULIMIT], XmNvalue, &txt, NULL);
        newul = strtoul(txt, (char **) 0, 0);
        XtFree(txt);
        if  (newul == 0)  {
                if  (amount < 0)
                        return;
                newul = 1;
        }
        else  {
                unsigned  bit;
                newul++;
                for  (bit = 0;  bit < 24;  bit++)
                        if  (newul & (1 << bit))
                                break;
                if  (amount < 0)  {
                        if  (bit != 0)
                                bit--;
                        newul -= 1 << bit;
                }
                else
                        newul += 1 << bit;
                newul--;
        }
        sprintf(nbuf, "0x%lx", (unsigned long) newul);
        XmTextSetString(workw[WORKW_ULIMIT], nbuf);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) ulim_adj, INT_TO_XTPOINTER(amount));
}

static void  ulim_cb(Widget w, int amt, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                ulim_adj(amt, NULL);
        else
                CLEAR_ARROW_TIMER
}

static void  endprocpar(Widget w, int data)
{
#ifdef  XMBTQ_JCALL
        if  (data  &&  !nochanges)  {
#endif
#ifdef  XMBTR_JCALL
        if  (data)  {
#endif
                int             n1, n2;
                char            *txt;
#ifdef  XMBTQ_JCALL
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
#endif
#ifdef  XMBTR_JCALL
                BtjobRef        bjp = cjob->job;
                Btjob           njob;

                njob = *bjp;
#endif
                XtVaGetValues(workw[WORKW_DIRTXTW], XmNvalue, &txt, NULL);
#ifdef  XMBTQ_JCALL
                if  (!repackjob(bjp, cjob, txt, (char *) 0, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        freexbuf(xindx);
                        doerror(w, $E{Too many job strings});
                        XtFree(txt);
                        return;
                }
#endif
#ifdef  XMBTR_JCALL
                if  (!repackjob(&njob, bjp, txt, (char *) 0, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        doerror(w, $EH{Too many job strings});
                        XtFree(txt);
                        return;
                }
                *bjp = njob;
#endif
                XtFree(txt);
                bjp->h.bj_umask = copyumask;
                XtVaGetValues(workw[WORKW_ULIMIT], XmNvalue, &txt, NULL);
                bjp->h.bj_ulimit = strtoul(txt, (char **) 0, 0);
                XtFree(txt);
                n1 = GET_TEXTORSPINBOX_INT(WW(WORKW_NORML));
                n2 = GET_TEXTORSPINBOX_INT(WW(WORKW_NORMU));
                if  (n2 < n1)  {
                        int     tmp = n1;
                        n1 = n2;
                        n2 = tmp;
                }
                bjp->h.bj_exits.nlower = (unsigned char) n1;
                bjp->h.bj_exits.nupper = (unsigned char) n2;
                n1 = GET_TEXTORSPINBOX_INT(WW(WORKW_ERRL));
                n2 = GET_TEXTORSPINBOX_INT(WW(WORKW_ERRU));
                if  (n2 < n1)  {
                        int     tmp = n1;
                        n1 = n2;
                        n2 = tmp;
                }
                bjp->h.bj_exits.elower = (unsigned char) n1;
                bjp->h.bj_exits.eupper = (unsigned char) n2;
                if  (XmToggleButtonGadgetGetState(workw[WORKW_NOADVTERR]))
                        bjp->h.bj_jflags |= BJ_NOADVIFERR;
                else
                        bjp->h.bj_jflags &= ~BJ_NOADVIFERR;

                if  (XmToggleButtonGadgetGetState(workw[WORKW_REMRUN]))
                        bjp->h.bj_jflags |= BJ_REMRUNNABLE|BJ_EXPORT;
                else  {
                        bjp->h.bj_jflags &= ~BJ_REMRUNNABLE;
                        if  (XmToggleButtonGadgetGetState(workw[WORKW_EXPORT]))
                                bjp->h.bj_jflags |= BJ_EXPORT;
                        else

                                bjp->h.bj_jflags &= ~BJ_EXPORT;
                }
#ifdef  XMBTQ_JCALL
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
#endif
#ifdef  XMBTR_JCALL
                cjob->changes++;
                cjob->nosubmit++;
#endif
        }
        XtDestroyWidget(GetTopShell(w));
}

#ifdef  XMBTQ_JCALL
static Widget  error_box(Widget mainform, Widget prevabove, Widget prevleft, char *widname, const int widnum, const int current, const unsigned readonly)
#endif
#ifdef  XMBTR_JCALL
static Widget  error_box(Widget mainform, Widget prevabove, Widget prevleft, char *widname, const int widnum, const int current)
#endif
{
        Widget  result;

#ifdef  HAVE_XM_SPINB_H
        result = XtVaCreateManagedWidget("sp",
                                         xmSpinBoxWidgetClass,  mainform,
#ifdef  XMBTQ_JCALL
                                         XmNarrowSensitivity,   readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                         XmNtopAttachment,      XmATTACH_WIDGET,
                                         XmNtopWidget,          prevabove,
                                         XmNleftAttachment,     XmATTACH_WIDGET,
                                         XmNleftWidget,         prevleft,
                                         NULL);

        workw[widnum] = XtVaCreateManagedWidget(widname,
                                                xmTextFieldWidgetClass, result,
                                                XmNspinBoxChildType,    XmNUMERIC,
                                                XmNcolumns,             3,
                                                XmNminimumValue,        0,
                                                XmNmaximumValue,        255,
                                                XmNposition,            current,
                                                XmNcursorPositionVisible,False,
                                                XmNeditable,            False,
                                                NULL);
#else
        result = workw[widnum] = XtVaCreateManagedWidget(widname,
                                                         xmTextFieldWidgetClass,        mainform,
                                                         XmNcolumns,                    3,
                                                         XmNmaxWidth,                   3,
                                                         XmNeditable,                   False,
                                                         XmNcursorPositionVisible,      False,
                                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                                         XmNtopWidget,                  prevabove,
                                                         XmNleftAttachment,             XmATTACH_WIDGET,
                                                         XmNleftWidget,                 prevleft,
                                                         NULL);

        put_textbox_int(result, current, 3);
#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                result = CreateArrowPair(widname, mainform, prevabove, result, (XtCallbackProc) errup_cb, (XtCallbackProc) errdn_cb, widnum, widnum);
#endif
        return  result;
}

#ifdef  XMBTQ_JCALL
static Widget  error_boxes(Widget mainform, Widget prevabove, char *labname, char *widnamel, char *widnameu, const int wnl, const int wnu, const int currl, const int curru, unsigned readonly)
#endif
#ifdef  XMBTR_JCALL
static Widget  error_boxes(Widget mainform, Widget prevabove, char *labname, char *widnamel, char *widnameu, const int wnl, const int wnu, const int currl, const int curru)
#endif
{
        Widget  prevleft;

#ifdef  XMBTQ_JCALL
        prevleft = place_label_left(mainform, prevabove, labname);
        prevleft = error_box(mainform, prevabove, prevleft, widnamel, wnl, currl, readonly);
        prevleft = place_label(mainform, prevabove, prevleft, "to");
#ifdef HAVE_XM_SPINB_H
        return  error_box(mainform, prevabove, prevleft, widnameu, wnu, curru, readonly);
#else
        error_box(mainform, prevabove, prevleft, widnameu, wnu, curru, readonly);
        return  workw[wnu];
#endif
#endif
#ifdef  XMBTR_JCALL
        prevleft = place_label_left(mainform, prevabove, labname);
        prevleft = error_box(mainform, prevabove, prevleft, widnamel, wnl, currl);
        prevleft = place_label(mainform, prevabove, prevleft, "to");
#ifdef HAVE_XM_SPINB_H
        return  error_box(mainform, prevabove, prevleft, widnameu, wnu, curru);
#else
        error_box(mainform, prevabove, prevleft, widnameu, wnu, curru);
        return  workw[wnu];
#endif
#endif
}

#ifdef  XMBTQ_JCALL
void  cb_procpar(Widget parent)
#endif
#ifdef  XMBTR_JCALL
void  cb_procpar(Widget parent, int isjob)
#endif
{
        int     ugo, rwx;
        Widget  pp_shell, panew, mainform, prevabove, prevleft, umaskrc, advrc, exprc;
        char    nbuf[20];
#ifdef  XMBTQ_JCALL
        unsigned        readonly;
        BtjobRef        cj = getselectedjob(BTM_READ);
        if  (!cj)
                return;
        cjob = cj;
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
        nochanges = 0;
#endif
#ifdef  XMBTR_JCALL
        BtjobRef        cj;
        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;
#endif

        prevabove = CreateJeditDlg(parent, "Procp", &pp_shell, &panew, &mainform);
        prevabove = CreateDselPane(mainform,
                                   prevabove,
                                   cj->h.bj_direct < 0? "/": &cj->bj_space[cj->h.bj_direct]);

        prevabove = place_label_left(mainform, prevabove, "umask");
        umaskrc = XtVaCreateManagedWidget("umaskbutt",
                                          xmRowColumnWidgetClass,       mainform,
                                          XmNtopAttachment,             XmATTACH_WIDGET,
                                          XmNtopWidget,                 prevabove,
                                          XmNleftAttachment,            XmATTACH_FORM,
                                          XmNpacking,                   XmPACK_COLUMN,
                                          XmNnumColumns,                3,
                                          XmNisHomogeneous,             True,
                                          XmNentryClass,                xmToggleButtonGadgetClass,
                                          XmNradioBehavior,             False,
                                          NULL);
        copyumask = cj->h.bj_umask;
        for  (ugo = 0;  ugo < 3;  ugo++)
                for  (rwx = 0;  rwx < 3;  rwx++)  {
                        Widget  w;
                        char    name[3];
                        unsigned   mask = 1 << (8 - ugo*3 - rwx);
                        name[0] = "ugo"[ugo];
                        name[1] = "rwx"[rwx];
                        name[2] = '\0';
                        w = XtVaCreateManagedWidget(name,
                                                    xmToggleButtonGadgetClass,  umaskrc,
                                                    XmNborderWidth,             0,
                                                    NULL);
                        if  (copyumask &  mask)
                                XmToggleButtonGadgetSetState(w, True, False);
#ifdef  XMBTQ_JCALL
                        if  (readonly)
                                XtSetSensitive(w, False);
                        else
#endif
                                XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) umask_turn, INT_TO_XTPOINTER(mask));
                }

        prevleft = place_label_left(mainform, umaskrc, "Ulimit");
        prevleft = workw[WORKW_ULIMIT] = XtVaCreateManagedWidget("ulim",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    10,
                                                                 XmNmaxWidth,                   10,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNeditable,                   False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  umaskrc,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

        sprintf(nbuf, "0x%lx", (unsigned long) cj->h.bj_ulimit);
        XmTextSetString(workw[WORKW_ULIMIT], nbuf);

#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                CreateArrowPair("ulim", mainform, umaskrc, prevleft, (XtCallbackProc) ulim_cb, (XtCallbackProc) ulim_cb, 1, -1);

#ifdef  XMBTQ_JCALL
        prevabove = error_boxes(mainform, workw[WORKW_ULIMIT], "Normal", "norml", "normu", WORKW_NORML, WORKW_NORMU, cj->h.bj_exits.nlower, cj->h.bj_exits.nupper, readonly);
        prevabove = error_boxes(mainform, prevabove, "Error", "errl", "erru", WORKW_ERRL, WORKW_ERRU, cj->h.bj_exits.elower, cj->h.bj_exits.eupper, readonly);
#endif
#ifdef  XMBTR_JCALL
        prevabove = error_boxes(mainform, workw[WORKW_ULIMIT], "Normal", "norml", "normu", WORKW_NORML, WORKW_NORMU, cj->h.bj_exits.nlower, cj->h.bj_exits.nupper);
        prevabove = error_boxes(mainform, prevabove, "Error", "errl", "erru", WORKW_ERRL, WORKW_ERRU, cj->h.bj_exits.elower, cj->h.bj_exits.eupper);
#endif

        advrc = XtVaCreateManagedWidget("adve",
                                        xmRowColumnWidgetClass,         mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  1,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               True,
                                        NULL);

        workw[WORKW_ADVTERR] = XtVaCreateManagedWidget("Advt", xmToggleButtonGadgetClass, advrc, XmNborderWidth, 0, NULL);
        workw[WORKW_NOADVTERR] = XtVaCreateManagedWidget("Noadvt", xmToggleButtonGadgetClass, advrc, XmNborderWidth, 0, NULL);
        XmToggleButtonGadgetSetState(workw[cj->h.bj_jflags & BJ_NOADVIFERR? WORKW_NOADVTERR: WORKW_ADVTERR], True, False);

        exprc = XtVaCreateManagedWidget("exp",
                                        xmRowColumnWidgetClass,         mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   advrc,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  1,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               True,
                                        NULL);

        workw[WORKW_NOEXPORT] = XtVaCreateManagedWidget("local", xmToggleButtonGadgetClass, exprc, XmNborderWidth, 0, NULL);
        workw[WORKW_EXPORT] = XtVaCreateManagedWidget("export", xmToggleButtonGadgetClass, exprc, XmNborderWidth, 0, NULL);
        workw[WORKW_REMRUN] = XtVaCreateManagedWidget("remrun", xmToggleButtonGadgetClass, exprc, XmNborderWidth, 0, NULL);

        XmToggleButtonGadgetSetState(workw[cj->h.bj_jflags & BJ_REMRUNNABLE? WORKW_REMRUN: cj->h.bj_jflags & BJ_EXPORT? WORKW_EXPORT: WORKW_NOEXPORT], True, False);
#ifdef  XMBTQ_JCALL
        if  (readonly)  {
                XtSetSensitive(workw[WORKW_ADVTERR], False);
                XtSetSensitive(workw[WORKW_NOADVTERR], False);
                XtSetSensitive(workw[WORKW_NOEXPORT], False);
                XtSetSensitive(workw[WORKW_EXPORT], False);
                XtSetSensitive(workw[WORKW_REMRUN], False);
                nochanges = 1;
        }
#endif
        XtManageChild(mainform);
        CreateActionEndDlg(pp_shell, panew, (XtCallbackProc) endprocpar, $H{xmbtq procpar dialog});
}

#ifndef HAVE_XM_SPINB_H
static void  deltup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_max = 0x0000ffff;
                arrow_lng = 5;
                arrow_incr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}

static void  deltdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_min = 0;
                arrow_lng = 5;
                arrow_decr(workw[subj], NULL);
        }
        else
                CLEAR_ARROW_TIMER
}
#endif

static  USHORT  copywsig;

#ifdef  HAVE_XM_SPINB_H
static void  zerod()
{
        put_spinbox_int(WW(WORKW_DELTW), 0);
}

static void  zeror()
{
        put_spinbox_int(WW(WORKW_RTHOURW), 0);
        put_spinbox_int(WW(WORKW_RTMINW), 0);
        put_spinbox_int(WW(WORKW_RTSECW), 0);
}

static void  zerog()
{
        put_spinbox_int(WW(WORKW_ROMINW), 0);
        put_spinbox_int(WW(WORKW_ROSECW), 0);
}

static int  incr_boxes(int widnum, int lim)
{
        Widget  wid = workw[widnum];
        int     v = get_spinbox_int(wid) + 1;
        if  (v >= lim)  {
                put_spinbox_int(wid, 0);
                return  1;
        }
        put_spinbox_int(wid, v);
        return  0;
}

static int  decr_boxes(int widnum, int lim)
{
        Widget  wid = workw[widnum];
        int     v = get_spinbox_int(wid) - 1;
        if  (v < 0)  {
                put_spinbox_int(wid, lim-1);
                return  1;
        }
        put_spinbox_int(wid, v);
        return  0;
}

static void  runt_ch_cb(Widget wid, XtPointer ismin, XmSpinBoxCallbackStruct *cb)
{
#ifdef  BROKEN_SPINBOX
        /* Can't rely on crossed_boundary going backwards.
           Going from 1 to 0 it sets cross_boundary and position to 60 (in this case)
           We'll have to change this if it's ever used for anything other than seconds
           or minutes!!
           Note this still doesn't display 0, the display goes from 1 to 59 and apparently nothing
           happens */
        if  (cb->widget == workw[WORKW_RTHOURW])
                return;
        if  (cb->reason == XmCR_SPIN_NEXT  &&  cb->position == 0)  {
                if  (cb->widget == workw[WORKW_RTMINW]  ||  incr_boxes(WORKW_RTMINW, 60))
                        incr_boxes(WORKW_RTHOURW, 100000);
        }
        else  if  (cb->reason == XmCR_SPIN_PRIOR  &&  cb->position == 59)  {
                if  (cb->widget == workw[WORKW_RTMINW]  ||  decr_boxes(WORKW_RTMINW, 60))
                        decr_boxes(WORKW_RTHOURW, 100000);
        }
#else
        if  (!cb->crossed_boundary  ||  cb->widget == workw[WORKW_RTHOURW])
                return;
        if  (cb->reason == XmCR_SPIN_NEXT  ||  cb->reason == XmCR_SPIN_FIRST)  {
                if  (cb->widget == workw[WORKW_RTMINW]  ||  incr_boxes(WORKW_RTMINW, 60))
                        incr_boxes(WORKW_RTHOURW, 100000);
        }
        else  if  (cb->reason == XmCR_SPIN_PRIOR  ||  cb->reason == XmCR_SPIN_LAST)  {
                if  (cb->widget == workw[WORKW_RTMINW]  ||  decr_boxes(WORKW_RTMINW, 60))
                        decr_boxes(WORKW_RTHOURW, 100000);
        }
#endif
}

static void  runon_ch_cb(Widget wid, XtPointer notused, XmSpinBoxCallbackStruct *cb)
{
#ifdef  BROKEN_SPINBOX
        /* See above comments about broken spinboxes. */
        if  (cb->widget == workw[WORKW_ROMINW])
                return;
        if  (cb->reason == XmCR_SPIN_NEXT  &&  cb->position == 0)
                incr_boxes(WORKW_ROMINW, 1000);
        else  if  (cb->reason == XmCR_SPIN_PRIOR  &&  cb->position == 59)
                decr_boxes(WORKW_ROMINW, 1000);
#else
        if  (!cb->crossed_boundary  ||  cb->widget == workw[WORKW_ROMINW])
                return;
        if  (cb->reason == XmCR_SPIN_NEXT  ||  cb->reason == XmCR_SPIN_FIRST)
                incr_boxes(WORKW_ROMINW, 1000);
        else  if  (cb->reason == XmCR_SPIN_PRIOR  ||  cb->reason == XmCR_SPIN_LAST)
                decr_boxes(WORKW_ROMINW, 1000);
#endif
}

#else
static  ULONG   copyrunt;
static  USHORT  copyrunon;

static void  fillinrunt()
{
        char    nbuf[20];
        sprintf(nbuf, "%lu", copyrunt / 3600L);
        XmTextSetString(workw[WORKW_RTHOURW], nbuf);
        sprintf(nbuf, "%lu", (copyrunt % 3600L) / 60L);
        XmTextSetString(workw[WORKW_RTMINW], nbuf);
        sprintf(nbuf, "%lu", copyrunt % 60L);
        XmTextSetString(workw[WORKW_RTSECW], nbuf);
}

static void  fillinrunon()
{
        char    nbuf[20];
        sprintf(nbuf, "%u", copyrunon / 60);
        XmTextSetString(workw[WORKW_ROMINW], nbuf);
        sprintf(nbuf, "%u", copyrunon % 60);
        XmTextSetString(workw[WORKW_ROSECW], nbuf);
}

static void  zerod()
{
        char    nbuf[10];
        sprintf(nbuf, "%5u", 0);
        XmTextSetString(workw[WORKW_DELTW], nbuf);
}

static void  zeror()
{
        copyrunt = 0;
        fillinrunt();
}

static void  zerog()
{
        copyrunon = 0;
        fillinrunon();
}

static void  runt_adj(int amount, XtIntervalId *id)
{
        if  (amount < 0)  {
                if  (copyrunt < (ULONG) -amount)
                        return;
        }
        else  {
                ULONG   nt = copyrunt + amount;
                if  (nt < copyrunt) /* Overflow */
                        return;
        }
        copyrunt += amount;
        fillinrunt();
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) runt_adj, (XtPointer) amount);
}

static void  runt_cb(Widget w, int amount, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                runt_adj(amount, NULL);
        else
                CLEAR_ARROW_TIMER
}

static void  runon_adj(int amount, XtIntervalId *id)
{
        if  (amount < 0)  {
                if  (copyrunon < (USHORT) -amount)
                        return;
        }
        else  {
                USHORT  nt = copyrunon + amount;
                if  (nt < copyrunon) /* Overflow */
                        return;
        }
        copyrunon += amount;
        fillinrunon();
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, (XtTimerCallbackProc) runon_adj, (XtPointer) amount);
}

static void  runon_cb(Widget w, int amount, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)
                runon_adj(amount, NULL);
        else
                CLEAR_ARROW_TIMER
}
#endif

static void  wsig_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                copywsig = (USHORT)  n;
}

static void  endruntime(Widget w, int data)
{
#ifdef  XMBTQ_JCALL
        if  (data)  {
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *cjob;
                bjp->h.bj_deltime = (USHORT) GET_TEXTORSPINBOX_INT(WW(WORKW_DELTW));
#ifdef HAVE_XM_SPINB_H
                bjp->h.bj_runtime = (get_spinbox_int(workw[WORKW_RTHOURW]) * 60L +
                                     get_spinbox_int(workw[WORKW_RTMINW])) * 60L +
                                     get_spinbox_int(workw[WORKW_RTSECW]);
                bjp->h.bj_runon = (USHORT) (get_spinbox_int(workw[WORKW_ROMINW]) * 60 + get_spinbox_int(workw[WORKW_ROSECW]));
#else
                bjp->h.bj_runtime = copyrunt;
                bjp->h.bj_runon = copyrunon;
#endif
                bjp->h.bj_autoksig = copywsig;
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
        }
#endif
#ifdef  XMBTR_JCALL
        if  (data)  {
                BtjobhRef       cj = &cjob->job->h;
                cj->bj_deltime = (USHORT) GET_TEXTORSPINBOX_INT(WW(WORKW_DELTW));
#ifdef HAVE_XM_SPINB_H
                cj->bj_runtime = (get_spinbox_int(workw[WORKW_RTHOURW]) * 60L +
                                  get_spinbox_int(workw[WORKW_RTMINW])) * 60L +
                                  get_spinbox_int(workw[WORKW_RTSECW]);
                cj->bj_runon = (USHORT) (get_spinbox_int(workw[WORKW_ROMINW]) * 60 + get_spinbox_int(workw[WORKW_ROSECW]));
#else
                cj->bj_runtime = copyrunt;
                cj->bj_runon = copyrunon;
#endif
                cj->bj_autoksig = copywsig;
                cjob->changes++;
                cjob->nosubmit++;
        }
#endif
        XtDestroyWidget(GetTopShell(w));
}

#ifdef  XMBTQ_JCALL
void  cb_runtime(Widget parent)
#endif
#ifdef  XMBTR_JCALL
void  cb_runtime(Widget parent, int isjob)
#endif
{
        int     dones, ws;
        Widget  rt_shell, panew, mainform, prevabove, prevleft, killrc, killw = (Widget) 0;
#ifdef  XMBTQ_JCALL
        unsigned        readonly;
        BtjobRef        cj = getselectedjob(BTM_READ);
        if  (!cj)
                return;
#ifdef  HAVE_XM_SPINB_H
        init_tdefaults();
#endif
        cjob = cj;
        readonly = !mpermitted(&cj->h.bj_mode, BTM_WRITE, mypriv->btu_priv);
#endif
#ifdef  XMBTR_JCALL
        Widget  pushb;
        BtjobRef        cj;

        if  (!(cjob = job_or_deflt(isjob)))
                return;
        cj = cjob->job;
#endif
        prevabove = CreateJeditDlg(parent, "Runtime", &rt_shell, &panew, &mainform);

        /* Set up delete time */

        prevleft = place_label_left(mainform, prevabove, "deletetime");
#ifdef  HAVE_XM_SPINB_H
        prevleft = XtVaCreateManagedWidget("deltsp",
                                           xmSpinBoxWidgetClass,        mainform,
#ifdef  XMBTQ_JCALL
                                           XmNarrowSensitivity,         readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                           XmNtopAttachment,            XmATTACH_WIDGET,
                                           XmNtopWidget,                prevabove,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               prevleft,
                                           NULL);

        workw[WORKW_DELTW] = XtVaCreateManagedWidget("delt",
                                                     xmTextFieldWidgetClass,    prevleft,
                                                     XmNminimumValue,           0,
                                                     XmNmaximumValue,           37267,
                                                     XmNspinBoxChildType,       XmNUMERIC,
                                                     XmNposition,               cj->h.bj_deltime,
                                                     XmNcolumns,                5,
                                                     NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)  {
                Widget  pb = XtVaCreateManagedWidget("zerod",
                                                     xmPushButtonWidgetClass,   mainform,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             prevleft,
                                                     NULL);
                XtAddCallback(pb, XmNactivateCallback, (XtCallbackProc) zerod, (XtPointer) 0);
        }
#endif
#ifdef  XMBTR_JCALL
        pushb = XtVaCreateManagedWidget("zerod",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);
        XtAddCallback(pushb, XmNactivateCallback, (XtCallbackProc) zerod, (XtPointer) 0);
#endif

        prevabove = prevleft;

        /* Now set up run time hours:mins:secs */

        prevleft = place_label_left(mainform, prevabove, "runtime");

        prevleft = XtVaCreateManagedWidget("rtspin",
                                           xmSpinBoxWidgetClass,mainform,
#ifdef  XMBTQ_JCALL
                                           XmNarrowSensitivity, readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        prevabove,
                                           XmNleftAttachment,   XmATTACH_WIDGET,
                                           XmNleftWidget,       prevleft,
                                           NULL);

        workw[WORKW_RTHOURW] = XtVaCreateManagedWidget("rh",
                                                       xmTextFieldWidgetClass,  prevleft,
                                                       XmNminimumValue,         0,
                                                       XmNmaximumValue,         99999,
                                                       XmNspinBoxChildType,     XmNUMERIC,
                                                       XmNposition,             (int) (cj->h.bj_runtime / 3600),
                                                       XmNcolumns,              5,
                                                       XmNeditable,             False,
                                                       XmNcursorPositionVisible,False,
                                                       NULL);

        workw[WORKW_RTMINW] = XtVaCreateManagedWidget("rm",
                                                      xmTextFieldWidgetClass,   prevleft,
                                                      XmNspinBoxChildType,      XmSTRING,
                                                      XmNcolumns,               2,
                                                      XmNeditable,              False,
                                                      XmNcursorPositionVisible, False,
                                                      XmNnumValues,             60,
                                                      XmNvalues,                timezerof,
                                                      XmNposition,              (int) ((cj->h.bj_runtime/60) % 60),
                                                      NULL);

        workw[WORKW_RTSECW] = XtVaCreateManagedWidget("rs",
                                                      xmTextFieldWidgetClass,   prevleft,
                                                      XmNspinBoxChildType,      XmSTRING,
                                                      XmNcolumns,               2,
                                                      XmNeditable,              False,
                                                      XmNcursorPositionVisible, False,
                                                      XmNnumValues,             60,
                                                      XmNvalues,                timezerof,
                                                      XmNposition,              (int) (cj->h.bj_runtime % 60),
                                                      NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)  {
                Widget  pb = XtVaCreateManagedWidget("zeror",
                                                     xmPushButtonWidgetClass,   mainform,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             prevleft,
                                                     NULL);

                XtAddCallback(pb, XmNactivateCallback, (XtCallbackProc) zeror, (XtPointer) 0);
                XtAddCallback(prevleft, XmNvalueChangedCallback, (XtCallbackProc) runt_ch_cb, 0);
        }
#endif
#ifdef  XMBTR_JCALL
        pushb = XtVaCreateManagedWidget("zeror",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);

        XtAddCallback(pushb, XmNactivateCallback, (XtCallbackProc) zeror, (XtPointer) 0);
        XtAddCallback(prevleft, XmNvalueChangedCallback, (XtCallbackProc) runt_ch_cb, 0);
#endif
#else  /* ! HAVE_XM_SPINB */
        prevleft = workw[WORKW_DELTW] = XtVaCreateManagedWidget("delt",
                                                                xmTextFieldWidgetClass, mainform,
                                                                XmNcolumns,             5,
                                                                XmNmaxWidth,            5,
                                                                XmNeditable,            False,
                                                                XmNcursorPositionVisible,       False,
                                                                XmNtopAttachment,       XmATTACH_WIDGET,
                                                                XmNtopWidget,           prevabove,
                                                                XmNleftAttachment,      XmATTACH_WIDGET,
                                                                XmNleftWidget,          prevleft,
                                                                NULL);
        put_textbox_int(prevleft, cj->h.bj_deltime, 5);

#ifdef  XMBTQ_JCALL
        if  (!readonly)  {
                Widget  pb, pl = CreateArrowPair("del", mainform, prevabove, prevleft, (XtCallbackProc) deltup_cb, (XtCallbackProc) deltdn_cb, WORKW_DELTW, WORKW_DELTW);
                pb = XtVaCreateManagedWidget("zerod",
                                             xmPushButtonWidgetClass,   mainform,
                                             XmNtopAttachment,          XmATTACH_WIDGET,
                                             XmNtopWidget,              prevabove,
                                             XmNleftAttachment,         XmATTACH_WIDGET,
                                             XmNleftWidget,             pl,
                                             NULL);

                XtAddCallback(pb, XmNactivateCallback, (XtCallbackProc) zerod, (XtPointer) 0);
        }
        prevabove = prevleft;
#endif
#ifdef  XMBTR_JCALL
        prevleft = CreateArrowPair("del", mainform, prevabove, prevleft, (XtCallbackProc) deltup_cb, (XtCallbackProc) deltdn_cb, WORKW_DELTW, WORKW_DELTW);
        pushb = XtVaCreateManagedWidget("zerod",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);
        XtAddCallback(pushb, XmNactivateCallback, (XtCallbackProc) zerod, (XtPointer) 0);
        prevabove = workw[WORKW_DELTW];
#endif

        prevleft = place_label_left(mainform, prevabove, "runtime");

        prevleft = workw[WORKW_RTHOURW] = XtVaCreateManagedWidget("rh",
                                                                  xmTextFieldWidgetClass,       mainform,
                                                                  XmNcolumns,                   5,
                                                                  XmNmaxWidth,                  5,
                                                                  XmNeditable,                  False,
                                                                  XmNcursorPositionVisible,     False,
                                                                  XmNtopAttachment,             XmATTACH_WIDGET,
                                                                  XmNtopWidget,                 prevabove,
                                                                  XmNleftAttachment,            XmATTACH_WIDGET,
                                                                  XmNleftWidget,                prevleft,
                                                                  NULL);
#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("rh", mainform, prevabove, prevleft, (XtCallbackProc) runt_cb, (XtCallbackProc) runt_cb, 3600, -3600);
        prevleft = place_label(mainform, prevabove, prevleft, ":");
        prevleft = workw[WORKW_RTMINW] = XtVaCreateManagedWidget("rm",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    2,
                                                                 XmNmaxWidth,                   2,
                                                                 XmNeditable,                   False,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);
#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("rm", mainform, prevabove, prevleft, (XtCallbackProc) runt_cb, (XtCallbackProc) runt_cb, 60, -60);
        prevleft = place_label(mainform, prevabove, prevleft, ":");
        prevleft = workw[WORKW_RTSECW] = XtVaCreateManagedWidget("rs",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    2,
                                                                 XmNmaxWidth,                   2,
                                                                 XmNeditable,                   False,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);
#ifdef  XMBTQ_JCALL
        if  (!readonly)  {
                Widget  pb, pl = CreateArrowPair("rs", mainform, prevabove, prevleft, (XtCallbackProc) runt_cb, (XtCallbackProc) runt_cb, 1, -1);
                pb = XtVaCreateManagedWidget("zeror",
                                             xmPushButtonWidgetClass,   mainform,
                                             XmNtopAttachment,          XmATTACH_WIDGET,
                                             XmNtopWidget,              prevabove,
                                             XmNleftAttachment,         XmATTACH_WIDGET,
                                             XmNleftWidget,             pl,
                                             NULL);
                XtAddCallback(pb, XmNactivateCallback, (XtCallbackProc) zeror, (XtPointer) 0);
        }
#endif
#ifdef  XMBTR_JCALL
        prevleft = CreateArrowPair("rs", mainform, prevabove, prevleft, (XtCallbackProc) runt_cb, (XtCallbackProc) runt_cb, 1, -1);

        pushb = XtVaCreateManagedWidget("zeror",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);

        XtAddCallback(pushb, XmNactivateCallback, (XtCallbackProc) zeror, (XtPointer) 0);
#endif
        copyrunt = cj->h.bj_runtime;
        fillinrunt();
#endif /* ! HAVE_XM_SPINB */

        prevabove = prevleft;
        prevabove = place_label_left(mainform, prevabove, "killwith");
        killrc = XtVaCreateManagedWidget("killbutt",
                                         xmRowColumnWidgetClass,        mainform,
                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                         XmNtopWidget,                  prevabove,
                                         XmNleftAttachment,             XmATTACH_FORM,
                                         XmNpacking,                    XmPACK_COLUMN,
                                         XmNnumColumns,                 2,
                                         XmNisHomogeneous,              True,
                                         XmNentryClass,                 xmToggleButtonGadgetClass,
                                         XmNradioBehavior,              True,
                                         NULL);

        copywsig = cj->h.bj_autoksig;

        dones = 0;
        for  (ws = 0;  ws < XtNumber(siglist);  ws++)  {
                Widget  w;
                if  (siglist[ws].signum == 0)
                        break;
                w = XtVaCreateManagedWidget(siglist[ws].wname,
                                            xmToggleButtonGadgetClass,  killrc,
                                            XmNborderWidth,             0,
                                            NULL);
                if  (siglist[ws].signum == SIGKILL)
                        killw = w;
                if  (siglist[ws].signum == cj->h.bj_autoksig)  {
                        XmToggleButtonGadgetSetState(w, True, False);
                        dones++;
                }
#ifdef  XMBTQ_JCALL
                if  (readonly)
                        XtSetSensitive(w, False);
                else
#endif
                        XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) wsig_turn, INT_TO_XTPOINTER(siglist[ws].signum));
        }

        if  (!dones  &&  killw)
                XmToggleButtonGadgetSetState(killw, True, False);

        prevabove = killrc;

        /* And now for grace time */

        prevleft = place_label_left(mainform, prevabove, "grace");
#ifdef HAVE_XM_SPINB_H
        prevleft = XtVaCreateManagedWidget("rosp",
                                           xmSpinBoxWidgetClass,mainform,
#ifdef  XMBTQ_JCALL
                                           XmNarrowSensitivity, readonly? XmARROWS_INSENSITIVE: XmARROWS_SENSITIVE,
#endif
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        prevabove,
                                           XmNleftAttachment,   XmATTACH_WIDGET,
                                           XmNleftWidget,       prevleft,
                                           NULL);

        workw[WORKW_ROMINW] = XtVaCreateManagedWidget("rom",
                                                      xmTextFieldWidgetClass,   prevleft,
                                                      XmNminimumValue,          0,
                                                      XmNmaximumValue,          999,
                                                      XmNspinBoxChildType,      XmNUMERIC,
                                                      XmNposition,              (int) cj->h.bj_runon / 60,
                                                      XmNcolumns,               3,
                                                      XmNeditable,              False,
                                                      XmNcursorPositionVisible, False,
                                                      NULL);

        workw[WORKW_ROSECW] = XtVaCreateManagedWidget("ros",
                                                      xmTextFieldWidgetClass,   prevleft,
                                                      XmNspinBoxChildType,      XmSTRING,
                                                      XmNposition,              (int) cj->h.bj_runon % 60,
                                                      XmNcolumns,               2,
                                                      XmNeditable,              False,
                                                      XmNcursorPositionVisible, False,
                                                      XmNnumValues,             60,
                                                      XmNvalues,                timezerof,
                                                      NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)  {
                Widget  pb = XtVaCreateManagedWidget("zerog",
                                                     xmPushButtonWidgetClass,   mainform,
                                                     XmNtopAttachment,          XmATTACH_WIDGET,
                                                     XmNtopWidget,              prevabove,
                                                     XmNleftAttachment,         XmATTACH_WIDGET,
                                                     XmNleftWidget,             prevleft,
                                                     NULL);

                XtAddCallback(pb, XmNactivateCallback, (XtCallbackProc) zerog, (XtPointer) 0);
                XtAddCallback(prevleft, XmNvalueChangedCallback, (XtCallbackProc) runon_ch_cb, (XtPointer) 0);
        }
#endif
#ifdef  XMBTR_JCALL
        XtAddCallback(prevleft, XmNvalueChangedCallback, (XtCallbackProc) runon_ch_cb, (XtPointer) 0);
#endif
#else  /* ! HAVE_XM_SPINB_H */
        prevleft = workw[WORKW_ROMINW] = XtVaCreateManagedWidget("rom",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    5,
                                                                 XmNmaxWidth,                   5,
                                                                 XmNeditable,                   False,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);
#ifdef  XMBTQ_JCALL
        if  (!readonly)
#endif
                prevleft = CreateArrowPair("rom", mainform, prevabove, prevleft, (XtCallbackProc) runon_cb, (XtCallbackProc) runon_cb, 60, -60);
        prevleft = place_label(mainform, prevabove, prevleft, ":");
        prevleft = workw[WORKW_ROSECW] = XtVaCreateManagedWidget("ros",
                                                                 xmTextFieldWidgetClass,        mainform,
                                                                 XmNcolumns,                    2,
                                                                 XmNmaxWidth,                   2,
                                                                 XmNeditable,                   False,
                                                                 XmNcursorPositionVisible,      False,
                                                                 XmNtopAttachment,              XmATTACH_WIDGET,
                                                                 XmNtopWidget,                  prevabove,
                                                                 XmNleftAttachment,             XmATTACH_WIDGET,
                                                                 XmNleftWidget,                 prevleft,
                                                                 NULL);

#ifdef  XMBTQ_JCALL
        if  (!readonly)  {
                Widget  pb, pl = CreateArrowPair("ros", mainform, prevabove, prevleft, (XtCallbackProc) runon_cb, (XtCallbackProc) runon_cb, 1, -1);
                pb = XtVaCreateManagedWidget("zerog",
                                             xmPushButtonWidgetClass,   mainform,
                                             XmNtopAttachment,          XmATTACH_WIDGET,
                                             XmNtopWidget,              prevabove,
                                             XmNleftAttachment,         XmATTACH_WIDGET,
                                             XmNleftWidget,             pl,
                                             NULL);
                XtAddCallback(pb, XmNactivateCallback, (XtCallbackProc) zerog, (XtPointer) 0);
        }
#endif
#ifdef  XMBTR_JCALL
        prevleft = CreateArrowPair("ros", mainform, prevabove, prevleft, (XtCallbackProc) runon_cb, (XtCallbackProc) runon_cb, 1, -1);
        pushb = XtVaCreateManagedWidget("zerog",
                                        xmPushButtonWidgetClass,        mainform,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   prevabove,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);
        XtAddCallback(pushb, XmNactivateCallback, (XtCallbackProc) zerog, (XtPointer) 0);
#endif
        copyrunon = cj->h.bj_runon;
        fillinrunon();
#endif /* ! HAVE_XM_SPINB_H */
        XtManageChild(mainform);
        CreateActionEndDlg(rt_shell, panew, (XtCallbackProc) endruntime, $H{xmbtq set run time dialog});
}
