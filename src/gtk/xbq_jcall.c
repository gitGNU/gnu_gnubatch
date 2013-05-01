/* xbq_jcall.c -- Callback routines for job handling

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

/* NB This is shared between gbch-xq and gbch-xr for the latter define
   IN_XBTR */

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
#include <gtk/gtk.h>
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
#include "stringvec.h"
#include "jvuprocs.h"
#include "gtk_lib.h"
#ifdef IN_XBTR
#include "xbr_ext.h"
#else
#include "xbq_ext.h"
#include "optflags.h"
#endif

static  char    Filename[] = __FILE__;

#define JOBPROP_TITPRILL_PAGE   0
#define JOBPROP_PROCPAR_PAGE    1
#define JOBPROP_TIMELIM_PAGE    2
#define JOBPROP_MAILWRT_PAGE    3

#define NOTE_PADDING    5

static  ULONG   ratelim[] =  { 60*24*366*5, 24*366*5, 366*5, 52*5, 60, 60, 5  };

#ifdef IN_XBTR
extern  HelpaltRef      daynames_full,
                        monnames,
                        ifnposs_names,
                        progresslist,
                        repunit_full;
extern  char            *delend_full, *retend_full;
extern  ULONG           default_rate;
extern  unsigned char   default_interval,
                        default_nposs;
extern  USHORT          defavoid;

void  gen_qlist(struct stringvec *);
#else /* !IN_XBTR */
static  HelpaltRef      daynames_full,
                        monnames,
                        repunit_full,
                        ifnposs_names;
static  char            *delend_full, *retend_full;
static  ULONG           default_rate;
static  unsigned char   default_interval,
                        default_nposs;
static  USHORT          defavoid;

extern  char            *execprog;
extern  char            *Curr_pwd;

struct  macromenitem    jobmacs[MAXMACS];
#endif /* IN_XBTR */

#define NULLENTRY(n)    (Ci_list[n].ci_name[0] == '\0')

#ifndef IN_XBTR

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
                doerror($EH{Unexpected sched message});
                return;
        case  JOB_REPLY:
                disp_str = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;
                doerror((int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler job errors}));
                return;
        case  NET_REPLY:
                disp_str = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;
                doerror((int) ((retc & ~REQ_TYPE) + $EH{Base for scheduler net errors}));
                return;
        }
}
#endif

/* Generate the label stuff at the start of a job dialog */

#ifdef  IN_XBTR
GtkWidget *start_jobdlg(struct pend_job *pj, const int dlgcode, const int labelcode)
#else /* !IN_XBTR */
GtkWidget *start_jobdlg(CBtjobRef jp, const int dlgcode, const int labelcode)
#endif /* IN_XBTR */
{
        GtkWidget  *dlg, *lab;
        char    *pr;
        GString  *labp;
#ifdef IN_XBTR
        CBtjobRef       jp = pj->job;
        static  char    *def_nam;
#else
        const   char    *tit;
#endif

        dlg = gprompt_dialog(toplevel, dlgcode);
        pr = gprompt(labelcode);
        labp = g_string_new(pr);
        free(pr);
        g_string_append_c(labp, ' ');

#ifdef IN_XBTR

        /* Adjust label if it's the default job */

        if  (pj == &default_pend)  {
                if  (!def_nam)
                        def_nam = gprompt($P{xmbtr default job});
                g_string_append(labp, def_nam);
        }
        else  {
                const   char    *tit;
                extern  char    *no_title;

                /* This is never going to be on another host */

                tit = title_of(jp);
                if  (strlen(tit) != 0)
                        g_string_append(labp, tit);
                else
                        g_string_append(labp, no_title);
        }
#else /* IN_XBTR */
        g_string_append(labp, JOB_NUMBER(jp));
        tit = qtitle_of(jp);
        if  (strlen(tit) != 0)
                g_string_append_printf(labp, " (%s)", tit);
#endif /* IN_XBTR */

        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, DEF_DLG_VPAD);
        return  dlg;
}

#ifdef IN_XBTR

/* Set job queue name */

void  cb_jqueue(GtkAction *action)
{
        GtkWidget  *dlg, *qwid, *uwid = 0, *gwid = 0, *verbw, *hbox;
        struct pend_job *cj;
        int     noupdug = (mypriv->btu_priv & BTM_WADMIN) == 0;
        int     cnt;
        struct  stringvec  possqs;

        if  (!(cj = job_or_deflt(action)))
                return;

        dlg = start_jobdlg(cj, $P{xbtr job queue dlgtit}, $P{xbtq job queue lab});

        /* Row about queue prefix */

        gen_qlist(&possqs);
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtr jqueue queue lab}), FALSE, FALSE, DEF_DLG_HPAD);
        qwid = gtk_combo_box_entry_new_text();
        if  (cj->jobqueue  &&  strlen(cj->jobqueue) != 0)
                gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(qwid))), cj->jobqueue);
        for  (cnt = 0;  cnt < stringvec_count(possqs);  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(qwid), stringvec_nth(possqs, cnt));
        stringvec_free(&possqs);
        gtk_box_pack_start(GTK_BOX(hbox), qwid, FALSE, FALSE, DEF_DLG_HPAD);

        /* Row about user / group possibly changeable */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtr jqueue user grp lab}), FALSE, FALSE, DEF_DLG_HPAD);

        if  (noupdug)  {
                GString  *ugl = g_string_new(prin_uname((uid_t) cj->userid));
                g_string_append_c(ugl, '/');
                g_string_append(ugl, prin_gname((gid_t) cj->grpid));
                gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(ugl->str), FALSE, FALSE, DEF_DLG_HPAD);
                g_string_free(ugl, TRUE);
        }
        else  {
                char    **uglist, **up, *cug;
                int     cnt = 0;
                uglist = gen_ulist((char *) 0);
                uwid = gtk_combo_box_new_text();
                gtk_box_pack_start(GTK_BOX(hbox), uwid, FALSE, FALSE, DEF_DLG_HPAD);
                cug = prin_uname((uid_t) cj->userid);
                for  (up = uglist;  *up;  up++)  {
                        gtk_combo_box_append_text(GTK_COMBO_BOX(uwid), *up);
                        if  (strcmp(*up, cug) == 0)
                                gtk_combo_box_set_active(GTK_COMBO_BOX(uwid), cnt);
                        cnt++;
                }
                freehelp(uglist);
                cnt = 0;
                uglist = gen_glist((char *) 0);
                gwid = gtk_combo_box_new_text();
                gtk_box_pack_start(GTK_BOX(hbox), gwid, FALSE, FALSE, DEF_DLG_HPAD);
                cug = prin_gname((gid_t) cj->grpid);
                for  (up = uglist;  *up;  up++)  {
                        gtk_combo_box_append_text(GTK_COMBO_BOX(gwid), *up);
                        if  (strcmp(*up, cug) == 0)
                                gtk_combo_box_set_active(GTK_COMBO_BOX(gwid), cnt);
                        cnt++;
                }
                freehelp(uglist);
        }

        verbw = gprompt_checkbutton($P{xbtr jqueue verbose});
        gtk_box_pack_start(GTK_BOX(hbox), verbw, FALSE, FALSE, DEF_DLG_HPAD);
        if  (cj->Verbose)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(verbw), TRUE);

        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newq = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(qwid))));
                char    *ntit;
                const   char    *cp, *tit;
                BtjobRef        bjp = cj->job;
                Btjob   njob;
                if  (!noupdug)  {
                        int_ugid_t      luid, lgid;
                        char  *newu = gtk_combo_box_get_active_text(GTK_COMBO_BOX(uwid));
                        char  *newg = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gwid));
                        luid = lookup_uname(newu);
                        lgid = lookup_gname(newg);
                        if  (luid == UNKNOWN_UID)  {
                                disp_str = newu;
                                doerror($EH{Unknown owner});
                                g_free(newu);
                                g_free(newg);
                                continue;
                        }
                        if  (lgid == UNKNOWN_GID)  {
                                disp_str = newg;
                                doerror($EH{Unknown group});
                                g_free(newu);
                                g_free(newg);
                                continue;
                        }
                        g_free(newu);
                        g_free(newg);
                        cj->userid = luid;
                        cj->grpid = lgid;
                }
                cj->Verbose = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verbw));

                /* Delete old one */

                if  (cj->jobqueue)
                        free(cj->jobqueue);

                cj->jobqueue = newq[0]? stracpy(newq): (char *) 0;

                /* Replace in title */

                tit = title_of(bjp);
                if  ((cp = strchr(tit, ':')))
                        cp++;
                else
                        cp = tit;
                if  (cj->jobqueue)  {
                        unsigned  tlng = strlen(cp) + strlen(cj->jobqueue) + 2;
                        if  (!(ntit = malloc(tlng)))
                                ABORT_NOMEM;
                        sprintf(ntit, "%s:%s", cj->jobqueue, cp);
                }
                else
                        ntit = stracpy(cp);
                njob = *bjp;

                if  (!repackjob(&njob, bjp, (char *) 0, ntit, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        doerror($EH{Too many job strings});
                        free(ntit);
                }
                *bjp = njob;
                free(ntit);
                break;
        }
        gtk_widget_destroy(dlg);
}

/* Set job state */

void  cb_jstate(GtkAction *action)
{
        const  char     *act = gtk_action_get_name(action);
        int     lng = strlen(act);
        int     indx;

        if  (act[lng-1] == 'd')  {
                default_job.h.bj_progress = strcmp(act, "Setrund") == 0? BJP_NONE: BJP_CANCELLED;
                return;
        }

        if  ((indx = getselectedjob(1)) < 0)
                return;

        pend_list[indx].job->h.bj_progress = strcmp(act, "Setrun") == 0? BJP_NONE: BJP_CANCELLED;
        update_state(&pend_list[indx]);
}

#else /* !IN_XBTR */

static void  init_tdefaults()
{
        int     n;

        if  (repunit_full)
                return;
        repunit_full = helprdalt($Q{Repeat unit full});
        delend_full = gprompt($P{Delete at end full});
        retend_full = gprompt($P{Retain at end full});
        ifnposs_names = helprdalt($Q{Wtime if not poss});
        daynames_full = helprdalt($Q{Weekdays full});
        monnames = helprdalt($Q{Months full});
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
}

/* Advance time to next */

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
                doerror($EH{xmbtq no time to adv});
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

/* Set job run or cancelled */

void  cb_setrunc(GtkAction *action)
{
        BtjobRef        cj = getselectedjob(BTM_WRITE);
        BtjobRef        djp;
        unsigned        retc;
        ULONG           xindx;

        if  (!cj)
                return;
        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                doerror($EH{xmbtq job running});
                return;
        }
        djp = &Xbuffer->Ring[xindx = getxbuf()];
        *djp = *cj;
        djp->h.bj_progress = strcmp(gtk_action_get_name(action), "Setrun") == 0? BJP_NONE: BJP_CANCELLED;
        wjmsg(J_CHANGE, xindx);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, djp);
        freexbuf(xindx);
}

/* Force / force/adv */

void  cb_force(GtkAction *action)
{
        BtjobRef        cj = getselectedjob(BTM_WRITE|BTM_KILL);
        unsigned        retc;

        if  (!cj)
                return;
        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);
                doerror($EH{xmbtq job running});
                return;
        }
        if  (cj->h.bj_progress != BJP_NONE)  {
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
        qwjimsg(strcmp(gtk_action_get_name(action), "Force") == 0? J_FORCENA: J_FORCE, cj);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, cj);
}

/* Signal sending */

static  struct  {
        int     msgcode;
        int     signum;
        GtkWidget  *butt;
}  siglist[] = {
        {       $P{xbtq kill descr Stopsig},    SIGSTOP },
        {       $P{xbtq kill descr Contsig},    SIGCONT },
        {       $P{xbtq kill descr Termsig},    SIGTERM },
        {       $P{xbtq kill descr Killsig},    SIGKILL },
        {       $P{xbtq kill descr Hupsig},     SIGHUP  },
        {       $P{xbtq kill descr Intsig},     SIGINT  },
        {       $P{xbtq kill descr Quitsig},    SIGQUIT },
        {       $P{xbtq kill descr Alarmsig},   SIGALRM },
        {       $P{xbtq kill descr Bussig},     SIGBUS  },
        {       $P{xbtq kill descr Segvsig},    SIGSEGV }
};

/* Routine to send signal - possibly with dialog if action is "Othersig" */

void  cb_kill(GtkAction *action)
{
        BtjobRef        cj = getselectedjob(BTM_KILL);
        const   char    *act = gtk_action_get_name(action);
        int             signum = 0;
        unsigned        retc;

        if  (!cj)
                return;

        if  (act[0] == 'O')  {  /* Othersig */
                GtkWidget  *dlg, *gw;
                int     cnt;

                dlg = start_jobdlg(cj, $P{xbtq kill type dlg}, $P{xbtq kill type lab});

                siglist[0].butt = gw = gprompt_radiobutton(siglist[0].msgcode);
                gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), gw, FALSE, FALSE, DEF_DLG_VPAD);
                for  (cnt = 1;  cnt < sizeof(siglist)/sizeof(siglist[0]);  cnt++)  {
                        siglist[cnt].butt = gprompt_radiobutton_fromwidget(gw, siglist[cnt].msgcode);
                        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), siglist[cnt].butt, FALSE, FALSE, DEF_DLG_VPAD);
                }

                gtk_widget_show_all(dlg);
                if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                        for  (cnt = 0;  cnt < sizeof(siglist)/sizeof(siglist[0]);  cnt++)  {
                                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(siglist[cnt].butt)))  {
                                        signum = siglist[cnt].signum;
                                        break;
                                }
                        }
                }
                gtk_widget_destroy(dlg);
        }
        else  switch  (act[0])  {
        default:
                return;
        case  'I':              /* Intsig */
                signum = SIGINT;
                break;
        case  'Q':              /* Quitsig */
                signum = SIGQUIT;
                break;
        case  'S':              /* Stopsig */
                signum = SIGSTOP;
                break;
        case  'C':
                signum = SIGCONT;
                break;
        }

        if  (signum == 0)
                return;

        Oreq.sh_params.param = signum;
        qwjimsg(J_KILL, cj);
        if  ((retc = readreply()) != J_OK)
                qdojerror(retc, cj);
}
#endif /* !IN_XBTR */

struct  timedlgdata  {
        GtkWidget       *hastime;
        GtkWidget       *time_hr, *time_min;
        GtkWidget       *time_cal;
        GtkWidget       *rep_lab, *rep_units, *rep_style, *rep_mday;
        GtkWidget       *av_frame, *av_days[8];
        GtkWidget       *comesto;
        GtkWidget       *skip_lab, *skip_delay;
        int             last_hr, last_min;
        time_t          ctval;
        Btjob           cjob;
};

/* Do it this way to save unnecessary signals */

static void  set_spin_to(GtkWidget *spinb, const int val)
{
        if  (gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinb)) != val)
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinb), (gdouble) val);
}

/* Time set dialog stuff */

static void  fillintimes(struct timedlgdata *tdata)
{
        TimeconRef  tc = &tdata->cjob.h.bj_times;

        if  (tc->tc_istime)  {
                struct  tm      *tp = localtime(&tc->tc_nexttime);
                guint   year, month, day;
                int     cnt;

                tdata->last_hr = tp->tm_hour;
                tdata->last_min = tp->tm_min;
                setifnotset(tdata->hastime);
                gtk_widget_show(tdata->time_hr);
                gtk_widget_show(tdata->time_min);
                gtk_widget_show(tdata->time_cal);
                set_spin_to(tdata->time_hr, tp->tm_hour);
                set_spin_to(tdata->time_min, tp->tm_min);
                gtk_calendar_get_date(GTK_CALENDAR(tdata->time_cal), &year, &month, &day);
                if  (year != tp->tm_year + 1900  ||  month != tp->tm_mon)
                        gtk_calendar_select_month(GTK_CALENDAR(tdata->time_cal), tp->tm_mon, tp->tm_year+1900);
                if  (day != tp->tm_mday)
                        gtk_calendar_select_day(GTK_CALENDAR(tdata->time_cal), tp->tm_mday);

                gtk_widget_show(tdata->rep_lab);
                gtk_widget_show(tdata->rep_style);

                if  (gtk_combo_box_get_active(GTK_COMBO_BOX(tdata->rep_style)) != (int) tc->tc_repeat)
                        gtk_combo_box_set_active(GTK_COMBO_BOX(tdata->rep_style), tc->tc_repeat);

                if  (tc->tc_repeat >= TC_MINUTES)  {
                        time_t  foreg;

                        gtk_widget_show(tdata->rep_units);
                        gtk_widget_show_all(tdata->av_frame);
                        gtk_widget_show(tdata->comesto);
                        gtk_widget_show(tdata->skip_lab);
                        gtk_widget_show(tdata->skip_delay);
                        set_spin_to(tdata->rep_units, tc->tc_rate);

                        if  (tc->tc_repeat == TC_MONTHSB)  {
                                int     daysinmon = month_days[tp->tm_mon];
                                if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
                                        daysinmon++;
                                if  ((int) tc->tc_mday > daysinmon)
                                        tc->tc_mday = daysinmon;

                                gtk_widget_show(tdata->rep_mday);
                                set_spin_to(tdata->rep_mday, tc->tc_mday);
                        }
                        else  if  (tc->tc_repeat == TC_MONTHSE)  {
                                int     daysinmon = month_days[tp->tm_mon];
                                if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
                                        daysinmon++;
                                if  ((int) tc->tc_mday >= daysinmon)
                                        tc->tc_mday = daysinmon - 1;

                                gtk_widget_show(tdata->rep_mday);
                                set_spin_to(tdata->rep_mday, daysinmon - (int) tc->tc_mday);
                        }
                        else
                                gtk_widget_hide(tdata->rep_mday);

                        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                                if  (tc->tc_nvaldays & (1 << cnt))
                                        setifnotset(tdata->av_days[cnt]);
                                else
                                        unsetifset(tdata->av_days[cnt]);

                        foreg = advtime(tc);
                        if  (tdata->ctval != foreg)  {
                                char    egbuf[50];
                                tdata->ctval = foreg;
                                tp = localtime(&foreg);
                                sprintf(egbuf, "%.2d:%.2d %s %.2d %s %d",
                                        tp->tm_hour, tp->tm_min, disp_alt(tp->tm_wday, daynames_full),
                                        tp->tm_mday, disp_alt(tp->tm_mon, monnames), tp->tm_year + 1900);
                                gtk_label_set_text(GTK_LABEL(tdata->comesto), egbuf);
                        }

                        if  (gtk_combo_box_get_active(GTK_COMBO_BOX(tdata->skip_delay)) != (int) tc->tc_nposs)
                                gtk_combo_box_set_active(GTK_COMBO_BOX(tdata->skip_delay), tc->tc_nposs);
                }
                else  {
                        gtk_widget_hide(tdata->rep_units);
                        gtk_widget_hide(tdata->rep_mday);
                        gtk_widget_hide_all(tdata->av_frame);
                        gtk_widget_hide(tdata->comesto);
                        gtk_widget_hide(tdata->skip_lab);
                        gtk_widget_hide(tdata->skip_delay);
                }
        }
        else  {
                unsetifset(tdata->hastime);
                gtk_widget_hide(tdata->time_hr);
                gtk_widget_hide(tdata->time_min);
                gtk_widget_hide(tdata->time_cal);
                gtk_widget_hide(tdata->rep_lab);
                gtk_widget_hide(tdata->rep_units);
                gtk_widget_hide(tdata->rep_style);
                gtk_widget_hide(tdata->rep_mday);
                gtk_widget_hide_all(tdata->av_frame);
                gtk_widget_hide(tdata->comesto);
                gtk_widget_hide(tdata->skip_lab);
                gtk_widget_hide(tdata->skip_delay);
        }
}

static void  tim_turn(struct timedlgdata *tdata)
{
        TimeconRef      tc = &tdata->cjob.h.bj_times;

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tdata->hastime)))  {
                if  (!tc->tc_istime)  {
                        tc->tc_istime = 1;
                        tc->tc_nexttime = ((time((time_t *) 0) + 59L) / 60L) * 60L;
                        tc->tc_repeat = default_interval;
                        tc->tc_rate = default_rate;
                        if  (tc->tc_repeat >= TC_MINUTES  &&  tc->tc_rate > ratelim[tc->tc_repeat-TC_MINUTES])
                                tc->tc_rate = ratelim[tc->tc_repeat-TC_MINUTES];
                        tc->tc_nposs = default_nposs;
                        tc->tc_nvaldays = defavoid;
                        tc->tc_mday = 1;
                }
        }
        else
                tc->tc_istime = 0;
        fillintimes(tdata);
}

static void  hrmin_turn(GtkWidget *hrmin_wid, struct timedlgdata *tdata)
{
        int     newval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hrmin_wid));
        int     adj;
        TimeconRef      tc = &tdata->cjob.h.bj_times;

        if  (hrmin_wid == tdata->time_hr)
                adj = (newval - tdata->last_hr) * 3600;
        else
                adj = (newval - tdata->last_min) * 60;

        tc->tc_nexttime += adj;
        fillintimes(tdata);
}

static void  calendar_turn(struct timedlgdata *tdata)
{
        int             hr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tdata->time_hr));
        int             min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tdata->time_min));
        guint           day, month, year;
        time_t          when;
        struct  tm      mt;

        gtk_calendar_get_date(GTK_CALENDAR(tdata->time_cal), &year, &month, &day);

        mt.tm_hour = hr;
        mt.tm_min = min;
        mt.tm_sec = 0;
        mt.tm_mday = day;
        mt.tm_mon = month;
        mt.tm_year = year - 1900;
        mt.tm_isdst = 0;
        when = mktime(&mt);
        if  (when == (time_t) -1)
                when = time((time_t *) 0);

        /* In case of crossing DST */

        if  (mt.tm_hour != hr)  {
                mt.tm_hour = hr;
                when = mktime(&mt);
        }

        tdata->cjob.h.bj_times.tc_nexttime = when;
        fillintimes(tdata);
}

/* Activate when style of repeat changes */

static void  style_turn(struct timedlgdata *tdata)
{
        int  newstyle = gtk_combo_box_get_active(GTK_COMBO_BOX(tdata->rep_style));
        TimeconRef      tc = &tdata->cjob.h.bj_times;
        int  oldstyle = tc->tc_repeat;

        /* If now undefined or equal to original, ignore it */

        if  (newstyle < 0  ||  newstyle == oldstyle)
                return;

        tc->tc_repeat = newstyle;

        if  (newstyle >= TC_MINUTES)  {

                /* If old style didn't have a repeat and new one does, insert default rate and not poss */

                if  (oldstyle < TC_MINUTES)  {
                        tc->tc_rate = default_rate;
                        tc->tc_nvaldays = defavoid;
                        tc->tc_nposs = default_nposs;
                }

                /* Check we've not got a ridiculous rate for the style */

                if  (tc->tc_rate > ratelim[newstyle-TC_MINUTES])
                        tc->tc_rate = ratelim[newstyle-TC_MINUTES];

                if  (newstyle == TC_MONTHSB  ||  newstyle == TC_MONTHSE)  {
                        struct  tm  *tp = localtime(&tc->tc_nexttime);
                        if  (newstyle == TC_MONTHSB)
                                tc->tc_mday = (unsigned char) tp->tm_mday;
                        else  {
                                /* Come back Brutus & Cassius all is forgiven */
                                int  daysinmon = month_days[tp->tm_mon];
                                if  (tp->tm_mon == 1)
                                        daysinmon = tp->tm_year % 4 == 0? 29: 28;
                                tc->tc_mday = daysinmon - tp->tm_mday;
                        }
                }
        }

        fillintimes(tdata);
}

static void  int_turn(struct timedlgdata *tdata)
{
        int     newval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tdata->rep_units));
        TimeconRef      tc = &tdata->cjob.h.bj_times;

        /* Ignore null change or if no actual repeat */

        if  (tc->tc_rate == newval  ||  tc->tc_repeat < TC_MINUTES)
                return;

        /* Reset to original value if over the top */

        if  (newval > (int) ratelim[tc->tc_repeat - TC_MINUTES])  {
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(tdata->rep_units), (gdouble) tc->tc_rate);
                return;
        }

        tc->tc_rate = newval;
        fillintimes(tdata);
}

static void  mday_turn(struct timedlgdata *tdata)
{
        int     newval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tdata->rep_units));
        TimeconRef      tc = &tdata->cjob.h.bj_times;
        struct  tm      *tp;
        int     daysinmon;

        /* Ignore null change or if no actual month day */

        if  (tc->tc_rate == newval  ||  (tc->tc_repeat != TC_MONTHSB  &&  tc->tc_repeat != TC_MONTHSE))
                return;

        tp = localtime(&tc->tc_nexttime);
        daysinmon = month_days[tp->tm_mon];
        if  (tp->tm_mon == 1)
                daysinmon = tp->tm_year % 4 == 0? 29: 28;

        if  (tc->tc_repeat == TC_MONTHSB)  {
                if  (newval > daysinmon)  {
                        gtk_spin_button_set_value(GTK_SPIN_BUTTON(tdata->rep_mday), (gdouble) tc->tc_mday);
                        return;
                }

                /* Avoid changing anything if day hasn't changed */

                if  (tc->tc_mday == newval)
                        return;

                tc->tc_mday = newval;
        }
        else  {
                if  (newval >= daysinmon)  {
                        tc->tc_mday = 0;
                        gtk_spin_button_set_value(GTK_SPIN_BUTTON(tdata->rep_mday), (gdouble) daysinmon);
                }
                tc->tc_mday = daysinmon - newval;
        }

        fillintimes(tdata);
}

static void  av_turn(GtkWidget *avwid, struct timedlgdata *tdata)
{
        TimeconRef      tc = &tdata->cjob.h.bj_times;
        int     daycount, isset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(avwid));

        for  (daycount = 0;  daycount < TC_NDAYS;  daycount++)
                if  (tdata->av_days[daycount] == avwid)  {
                        if  (isset)  {
                                if  (!(tc->tc_nvaldays & (1 << daycount)))  {
                                        tc->tc_nvaldays |= (1 << daycount);
                                        fillintimes(tdata);
                                }
                        }
                        else  {
                                if  ((tc->tc_nvaldays & (1 << daycount)))  {
                                        tc->tc_nvaldays &= ~(1 << daycount);
                                        fillintimes(tdata);
                                }
                        }
                        return;
                }
}

/* Check everything makes sense */

int  extract_time(struct timedlgdata *tdata)
{
        TimeconRef      tc = &tdata->cjob.h.bj_times;
        time_t          when = tc->tc_nexttime;

        /* If no time set, go home */

        if  (!tc->tc_istime)
                return  1;

        if  (when < time((time_t *) 0)  &&  !Confirm($PH{xbtq time in past cont}))
                return  0;

        if  (tc->tc_repeat >= TC_MINUTES)  {
                int     sk;

                if  ((tc->tc_nvaldays & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)  {
                        doerror($EH{xmbtq all days set});
                        return  0;
                }

                sk = gtk_combo_box_get_active(GTK_COMBO_BOX(tdata->skip_delay));
                if  (sk < 0)  {
                        doerror($EH{xbtq no ifnposs selected});
                        return  0;
                }
                tc->tc_nposs = sk;
        }

        return  1;
}

#ifdef IN_XBTR
void  cb_time(GtkAction *action)
#else
void  cb_time()
#endif
{
#ifdef IN_XBTR
        struct  pend_job  *pj = job_or_deflt(action);
#else
        BtjobRef        cj = getselectedjob(BTM_WRITE);
#endif
        int             cnt;
        GtkWidget       *dlg, *hbox, *avtab;
        GtkAdjustment   *adj;
        char            *pr;
        struct  timedlgdata     tdata;

#ifdef IN_XBTR
        if  (!pj)
                return;

        tdata.cjob = *pj->job;
        tdata.ctval = 0;

        dlg = start_jobdlg(pj, $P{xbtq set time dlg}, $P{xbtq set time lab});
#else
        if  (!cj)
                return;

        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                doerror($EH{xmbtq job running});
                return;
        }

        /* Make copy of job in case it changes while we're fiddling.
           The scheduler will return sequence error if someone else gets in */

        tdata.cjob = *cj;
        tdata.ctval = 0;
        init_tdefaults();

        dlg = start_jobdlg(&tdata.cjob, $P{xbtq set time dlg}, $P{xbtq set time lab});
#endif

        /* Time and date row */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        tdata.hastime = gprompt_checkbutton($P{xbtq job has time set});
        gtk_box_pack_start(GTK_BOX(hbox), tdata.hastime, FALSE, FALSE, 0);

        adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 23.0, 1.0, 1.0, 0.0);
        tdata.time_hr = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), tdata.time_hr, FALSE, FALSE, 0);

        adj = (GtkAdjustment *) gtk_adjustment_new(0.0, 0.0, 59.0, 1.0, 1.0, 0.0);
        tdata.time_min = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), tdata.time_min, FALSE, FALSE, 0);

        tdata.time_cal = gtk_calendar_new();
        gtk_calendar_set_display_options(GTK_CALENDAR(tdata.time_cal), GTK_CALENDAR_SHOW_HEADING|GTK_CALENDAR_SHOW_DAY_NAMES);
        gtk_box_pack_start(GTK_BOX(hbox), tdata.time_cal, FALSE, FALSE, 0);

        /* Repeat style and units */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        tdata.rep_lab = gprompt_label($P{xbtq repeat opt lab});
        gtk_box_pack_start(GTK_BOX(hbox), tdata.rep_lab, FALSE, FALSE, DEF_DLG_HPAD);

        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) default_rate, 1.0, 9999.0, 1.0, 10.0, 0.0);
        tdata.rep_units = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), tdata.rep_units, FALSE, FALSE, DEF_DLG_HPAD);

        tdata.rep_style = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), tdata.rep_style, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_combo_box_append_text(GTK_COMBO_BOX(tdata.rep_style), delend_full);
        gtk_combo_box_append_text(GTK_COMBO_BOX(tdata.rep_style), retend_full);
        for  (cnt = 0;  cnt <= TC_YEARS-TC_MINUTES;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(tdata.rep_style), disp_alt(cnt, repunit_full));


        adj = (GtkAdjustment *) gtk_adjustment_new(1.0, 1.0, 31.0, 1.0, 1.0, 0.0);
        tdata.rep_mday = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), tdata.rep_mday, FALSE, FALSE, DEF_DLG_HPAD);

        /* Avoiding days */

        tdata.av_frame = gtk_frame_new(NULL);
        pr = gprompt($P{xbtq avoiding days frame lab});
        gtk_frame_set_label(GTK_FRAME(tdata.av_frame), pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(tdata.av_frame), 0.0, 1.0);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), tdata.av_frame, FALSE, FALSE, DEF_DLG_VPAD);

        avtab = gtk_table_new(2, 4, TRUE);
        gtk_container_add(GTK_CONTAINER(tdata.av_frame), avtab);

        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)  {
                GtkWidget  *butt = gtk_check_button_new_with_label(disp_alt(cnt, daynames_full));
                int  row = cnt & 1;
                int  col = cnt >> 1;
                gtk_table_attach_defaults(GTK_TABLE(avtab), butt, col, col+1, row, row+1);
                tdata.av_days[cnt] = butt;
        }

        /* Comes to time */

        tdata.comesto = gtk_label_new(NULL);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), tdata.comesto, FALSE, FALSE, DEF_DLG_VPAD);

        /* If not possible */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        tdata.skip_lab = gprompt_label($P{xbtq if not poss lab});
        gtk_box_pack_start(GTK_BOX(hbox), tdata.skip_lab, FALSE, FALSE, DEF_DLG_VPAD);
        tdata.skip_delay = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), tdata.skip_delay, FALSE, FALSE, DEF_DLG_HPAD);
        for  (cnt = 0;  cnt < 4;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(tdata.skip_delay), disp_alt(cnt, ifnposs_names));

        /* Show all now fillintimes may change */

        gtk_widget_show_all(dlg);

        fillintimes(&tdata);

        /* Set up signals now so we don't get lots of recursive calls to fillintimes first time round */

        g_signal_connect_swapped(G_OBJECT(tdata.hastime), "toggled", G_CALLBACK(tim_turn), (gpointer) &tdata);
        g_signal_connect(G_OBJECT(tdata.time_hr), "value_changed", G_CALLBACK(hrmin_turn), (gpointer) &tdata);
        g_signal_connect(G_OBJECT(tdata.time_min), "value_changed", G_CALLBACK(hrmin_turn), (gpointer) &tdata);
        g_signal_connect_swapped(G_OBJECT(tdata.time_cal), "day-selected", G_CALLBACK(calendar_turn), (gpointer) &tdata);
        g_signal_connect_swapped(G_OBJECT(tdata.rep_units), "value_changed", G_CALLBACK(int_turn), (gpointer) &tdata);
        g_signal_connect_swapped(G_OBJECT(tdata.rep_style), "changed", G_CALLBACK(style_turn), (gpointer) &tdata);
        g_signal_connect_swapped(G_OBJECT(tdata.rep_mday), "value_changed", G_CALLBACK(mday_turn), (gpointer) &tdata);
        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                g_signal_connect(G_OBJECT(tdata.av_days[cnt]), "toggled", G_CALLBACK(av_turn), (gpointer) &tdata);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
#ifdef IN_XBTR
                if  (extract_time(&tdata))  {
                        pj->job->h.bj_times  = tdata.cjob.h.bj_times;
                        note_changes(pj);
                        break;
                }
#else
                BtjobRef        bjp;
                ULONG           xindx;
                unsigned        retc;

                if  (!extract_time(&tdata))
                        continue;

                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = tdata.cjob;
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
                break;
#endif
        }
        gtk_widget_destroy(dlg);
}

/* Other job properties done as notebook */

struct  propdialog_data  {
        GtkWidget       *titw, *priw, *llw, *ciw;       /* Title, pri, loadlev, cmd int */
        GtkWidget       *dirw, *umw[9], *ulw, *bkmw;    /* Directory, umask, ulim, bytes/k/m */
        GtkWidget       *new[2], *eew[2];               /* Normal error exit range */
        GtkWidget       *noadvw;                        /* No advance on error */
        GtkWidget       *exportw;                       /* Export option */
        GtkWidget       *delhw;                         /* Delete time (hours) */
        GtkWidget       *rthw, *rtmw, *rtsw;            /* Run time h/m/s */
        GtkWidget       *autoksigw;                     /* Signal option */
        GtkWidget       *romw, *rosw;                   /* Grace time min sec */
        GtkWidget       *mailw, *wrtw;                  /* Mail/write optiions */
#ifdef IN_XBTR
        struct  pend_job  *pending;                     /* Pending job pointer */
#endif
        Btjob           cjob;                           /* Job copy */
        USHORT          *lllist;
        GtkWidget       *dirsel;                        /* For selecting directories */
};

static void  cmdint_sel(struct propdialog_data *ddata)
{
        int     selw = gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->ciw));
        if  (selw >= 0)
                gtk_range_set_value(GTK_RANGE(ddata->llw), (gdouble) ddata->lllist[selw]);
}

static GtkWidget *create_titp_dlg(struct propdialog_data *ddata)
{
        GtkWidget  *frame, *vbox, *hbox;
        GtkAdjustment *adj;
        char    *pr;
        unsigned        cicnt, cin;
        int             ciw;

        pr = gprompt($P{xbtq framelab titprill});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* Title entry */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq titprill title}), FALSE, FALSE, DEF_DLG_HPAD);
        ddata->titw = gtk_entry_new();
#ifdef IN_XBTR
        gtk_entry_set_text(GTK_ENTRY(ddata->titw), title_of(&ddata->cjob));
#else
        gtk_entry_set_text(GTK_ENTRY(ddata->titw), qtitle_of(&ddata->cjob));
#endif
        gtk_box_pack_start(GTK_BOX(hbox), ddata->titw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Priority - spin box */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq titprill pri}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ddata->cjob.h.bj_pri, (gdouble) mypriv->btu_minp, (gdouble) mypriv->btu_maxp, 1.0, 1.0, 0.0);
        ddata->priw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->priw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Load level slide */

        ddata->llw = slider_with_buttons(vbox, $P{xbtq titprill ll}, ddata->cjob.h.bj_ll, !(mypriv->btu_priv & BTM_SPCREATE));

        /* Command int */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq titprill cmdint}), FALSE, FALSE, DEF_DLG_HPAD);

        ddata->ciw = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), ddata->ciw, FALSE, FALSE, DEF_DLG_VPAD*2);

        /* Vector of load levels to plug in when the corresponding ci is selected */

        ddata->lllist = (USHORT *) malloc((unsigned)(Ci_num * sizeof(USHORT)));
        if  (!ddata->lllist)
                ABORT_NOMEM;

        /* The ci list can have "holes" in where previous ones have been deleted
           cin counts the "real" entries which will be the ones put in the combo box
           We record the corresponding lls in lllist so we can set it when we select
           a ci. ciw records which the current one is */

        ciw = -1;
        for  (cicnt = cin = 0;  cicnt < Ci_num;  cicnt++)  {
                if  (NULLENTRY(cicnt))
                        continue;
                if  (strcmp(Ci_list[cicnt].ci_name, ddata->cjob.h.bj_cmdinterp) == 0)
                        ciw = cin;
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->ciw), Ci_list[cicnt].ci_name);
                ddata->lllist[cin] = Ci_list[cicnt].ci_ll;
                cin++;
        }
        if  (ciw >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(ddata->ciw), ciw);

        /* Set up signal so when we set the ci we reset the ll */

        g_signal_connect_swapped(G_OBJECT(ddata->ciw), "changed", G_CALLBACK(cmdint_sel), (gpointer) ddata);

        return  frame;
}

static void  dir_sel(struct propdialog_data *ddata)
{
        char    *dirname = envprocess(gtk_entry_get_text(GTK_ENTRY(ddata->dirw)));
        unsigned        lng;
        char    *pr;

        if  (strchr(dirname, '~'))  {
                char    *newdir = unameproc(dirname, dirname, Realuid);
                free(dirname);
                dirname = newdir;
        }

        lng = strlen(dirname);
        if  (dirname[lng-1] != '/')  {
                char    *newdir = malloc(lng+2);
                if  (!newdir)
                        ABORT_NOMEM;
                sprintf(newdir, "%s/", dirname);
                free(dirname);
                dirname = newdir;
        }

        pr = gprompt($P{xbtq select directory});
        ddata->dirsel = gtk_file_chooser_dialog_new(pr, NULL,
                                                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                    GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                    NULL);
        free(pr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(ddata->dirsel), dirname);
        free(dirname);
        gtk_widget_show(ddata->dirsel);

        if  (gtk_dialog_run(GTK_DIALOG(ddata->dirsel)) == GTK_RESPONSE_OK)  {
                gchar  *selected = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ddata->dirsel));
                gtk_entry_set_text(GTK_ENTRY(ddata->dirw), selected);
                g_free(selected);
        }
        gtk_widget_destroy(ddata->dirsel);
}

static GtkWidget *create_procp_dlg(struct propdialog_data *ddata)
{
        GtkWidget  *frame, *vbox, *hbox, *butt, *umframe, *umtab;
        GtkAdjustment *adj;
        char    *pr;
        long    ulim;
        int     ulimmult = 0, cnt;

        pr = gprompt($P{xbtq framelab procpar});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* Directory entry */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        butt = gprompt_button($P{xbtq procpar direct});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(dir_sel), (gpointer) ddata);
        ddata->dirw = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(ddata->dirw), ddata->cjob.h.bj_direct < 0? "/": &ddata->cjob.bj_space[ddata->cjob.h.bj_direct]);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->dirw, FALSE, FALSE, DEF_DLG_HPAD);

        pr = gprompt($P{xbtq framelab umask});
        umframe = gtk_frame_new(pr);
        free(pr);
        gtk_box_pack_start(GTK_BOX(vbox), umframe, FALSE, FALSE, DEF_DLG_VPAD);
        umtab = gtk_table_new(3, 3, TRUE);
        gtk_container_add(GTK_CONTAINER(umframe), umtab);

        for  (cnt = 0;  cnt < 9;  cnt++)  {
                int  row = cnt % 3;
                int  col = cnt / 3;
                butt = gprompt_checkbutton($P{xbtq umask lab UR} + cnt);
                if  (ddata->cjob.h.bj_umask & (1 << (8 - cnt)))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(umtab), butt, col, col+1, row, row+1);
                ddata->umw[cnt] = butt;
        }

        /* Ulimit */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq procpar ulimit}), FALSE, FALSE, DEF_DLG_HPAD);

        /* Quick and dirty way to get multiple */

        ulim = ddata->cjob.h.bj_ulimit;
        if  (ulim > 10000)  {
                ulimmult++;
                ulim /= 1024;
                if  (ulim > 10000)  {
                        ulimmult++;
                        ulim /= 1024;
                }
        }
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ulim , 0.0, 10000.0, 1.0, 1.0, 0.0);
        ddata->ulw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->ulw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Combo box for mult - byte / K / M */

        ddata->bkmw = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), ddata->bkmw, FALSE, FALSE, DEF_DLG_HPAD);

        for  (cnt = 0;  cnt < 3;  cnt++)  {
                pr = gprompt($P{xbtq procpar bytemult}+cnt);
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->bkmw), pr);
                free(pr);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(ddata->bkmw), ulimmult);

        /* Normal exit */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq procpar normal exit}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ddata->cjob.h.bj_exits.nlower, 0.0, 255.0, 1.0, 1.0, 0.0);
        ddata->new[0] = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->new[0], FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ddata->cjob.h.bj_exits.nupper, 0.0, 255.0, 1.0, 1.0, 0.0);
        ddata->new[1] = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->new[1], FALSE, FALSE, DEF_DLG_HPAD);

        /* Error exit */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq procpar error exit}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ddata->cjob.h.bj_exits.elower, 0.0, 255.0, 1.0, 1.0, 0.0);
        ddata->eew[0] = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->eew[0], FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ddata->cjob.h.bj_exits.eupper, 0.0, 255.0, 1.0, 1.0, 0.0);
        ddata->eew[1] = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->eew[1], FALSE, FALSE, DEF_DLG_HPAD);

        /* Advance time on error or not */

        ddata->noadvw = gprompt_checkbutton($P{xbtq procpar noadve});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->noadvw, FALSE, FALSE, DEF_DLG_VPAD);
        if  (ddata->cjob.h.bj_jflags & BJ_NOADVIFERR)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->noadvw), TRUE);

        /* Export option */

        ddata->exportw = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(vbox), ddata->exportw, FALSE, FALSE, DEF_DLG_VPAD);
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                pr = gprompt($P{xbtq procpar exporttype}+cnt);
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->exportw), pr);
                free(pr);
        }
        cnt = 0;
        if  (ddata->cjob.h.bj_jflags & BJ_EXPORT)  {
                cnt++;
                if  (ddata->cjob.h.bj_jflags & BJ_REMRUNNABLE)
                        cnt++;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(ddata->exportw), cnt);
        return  frame;
}

static void  clr_delt(struct propdialog_data *ddata)
{
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->delhw), 0.0);
}

static void  clr_runt(struct propdialog_data *ddata)
{
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->rthw), 0.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->rtmw), 0.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->rtsw), 0.0);
}

static void  clr_ront(struct propdialog_data *ddata)
{
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->romw), 0.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(ddata->rosw), 0.0);
}

struct  aksig  {
        int     signum;
        int     code;
        char    *msg;
};

struct  aksig   autoksiglist[] =  {
        {       SIGTERM,        $P{Autoksig termsig}  },
        {       SIGKILL,        $P{Autoksig killsig}  },
        {       SIGHUP,         $P{Autoksig hupsig}  },
        {       SIGINT,         $P{Autoksig intsig}  },
        {       SIGQUIT,        $P{Autoksig quitsig}  },
        {       SIGALRM,        $P{Autoksig alarmsig}  },
        {       SIGBUS,         $P{Autoksig bussig}  },
        {       SIGSEGV,        $P{Autoksig segvsig}  }  };

static GtkWidget *create_timelim_dlg(struct propdialog_data *ddata)
{
        GtkWidget  *frame, *vbox, *hbox, *butt;
        GtkAdjustment *adj;
        char    *pr;
        int     cnt, was = 0;

        pr = gprompt($P{xbtq framelab timelim});
        frame = gtk_frame_new(pr);
        free(pr);

        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* Delete time and reset to zero */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq timelim deltime}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ddata->cjob.h.bj_deltime, 0.0, 65535.0, 1.0, 1.0, 0.0);
        ddata->delhw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->delhw, FALSE, FALSE, DEF_DLG_HPAD);
        butt = gprompt_button($P{xbtq procpar clear delt});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(clr_delt), (gpointer) ddata);

        /* Run time and reset to zero */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq timelim runtime}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) (ddata->cjob.h.bj_runtime/3600), 0.0, 99999.0, 1.0, 1.0, 0.0);
        ddata->rthw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->rthw, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) ((ddata->cjob.h.bj_runtime/60) % 60), 0.0, 59.0, 1.0, 1.0, 0.0);
        ddata->rtmw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->rtmw, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) (ddata->cjob.h.bj_runtime % 60), 0.0, 59.0, 1.0, 1.0, 0.0);
        ddata->rtsw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->rtsw, FALSE, FALSE, DEF_DLG_HPAD);
        butt = gprompt_button($P{xbtq timelim clear runt});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(clr_runt), (gpointer) ddata);

        /* Signal to kill with */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq timelim runtime sig}), FALSE, FALSE, DEF_DLG_HPAD);

        /* Fill in the first time */

        if  (!autoksiglist[0].msg)
                for  (cnt = 0;  cnt < sizeof(autoksiglist) / sizeof(struct aksig);  cnt++)
                        autoksiglist[cnt].msg = gprompt(autoksiglist[cnt].code);

        ddata->autoksigw = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), ddata->autoksigw, FALSE, FALSE, DEF_DLG_HPAD);
        for  (cnt = 0;  cnt < sizeof(autoksiglist) / sizeof(struct aksig);  cnt++)  {
                if  (autoksiglist[cnt].signum == ddata->cjob.h.bj_autoksig)
                        was = cnt;
                gtk_combo_box_append_text(GTK_COMBO_BOX(ddata->autoksigw), autoksiglist[cnt].msg);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(ddata->autoksigw), was);

        /* Grace time */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq timelim gracetime}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) (ddata->cjob.h.bj_runon/60), 0.0, 999.0, 1.0, 1.0, 0.0);
        ddata->romw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->romw, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) (ddata->cjob.h.bj_runon % 60), 0.0, 59.0, 1.0, 1.0, 0.0);
        ddata->rosw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->rosw, FALSE, FALSE, DEF_DLG_HPAD);
        butt = gprompt_button($P{xbtq procpar clear gracetime});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(clr_ront), (gpointer) ddata);

        return  frame;
}

static GtkWidget *create_mailw_dlg(struct propdialog_data *ddata)
{
        GtkWidget  *frame, *vbox;
        char    *pr;

        pr = gprompt($P{xbtq framelab mailwrt});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD*2);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        ddata->mailw = gprompt_checkbutton($P{xbtq mailwrt mail});
        ddata->wrtw = gprompt_checkbutton($P{xbtq mailwrt write});

        gtk_box_pack_start(GTK_BOX(vbox), ddata->mailw, FALSE, FALSE, DEF_DLG_VPAD*4);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->wrtw, FALSE, FALSE, DEF_DLG_VPAD*4);

        if  (ddata->cjob.h.bj_jflags & BJ_MAIL)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->mailw), TRUE);
        if  (ddata->cjob.h.bj_jflags & BJ_WRT)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->wrtw), TRUE);

        return  frame;
}

static GtkWidget *create_propdialog(struct propdialog_data *ddata, const int wpage)
{
        GtkWidget  *dlg, *notebook, *titpdlg, *procpdlg, *timeldlg, *mailwdlg, *lab;

#ifdef IN_XBTR
        dlg = start_jobdlg(ddata->pending, $P{xbtq job props dlgtit}, $P{xbtq job props lab});
#else
        dlg = start_jobdlg(&ddata->cjob, $P{xbtq job props dlgtit}, $P{xbtq job props lab});
#endif
        notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
        titpdlg = create_titp_dlg(ddata);
        procpdlg = create_procp_dlg(ddata);
        timeldlg = create_timelim_dlg(ddata);
        mailwdlg = create_mailw_dlg(ddata);

        lab = gprompt_label($P{xbtq frametab titp});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), titpdlg, lab);
        lab = gprompt_label($P{xbtq frametab procp});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), procpdlg, lab);
        lab = gprompt_label($P{xbtq frametab timelim});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), timeldlg, lab);
        lab = gprompt_label($P{xbtq frametab mailwrt});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mailwdlg, lab);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), notebook, TRUE, TRUE, NOTE_PADDING);
        gtk_widget_show_all(dlg);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), wpage);
        return  dlg;
}

static void  extract_exits(unsigned char *l, unsigned char *u, GtkWidget **evec)
{
        int     low = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(evec[0]));
        int     high = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(evec[1]));
        if  (low > high)  {
                int  tmp = low;
                low = high;
                high = tmp;
        }
        *l = (unsigned char) low;
        *u = (unsigned char) high;
}

static int  extract_job_propresp(struct propdialog_data *ddata)
{
#ifdef IN_XBTR
        BtjobRef        bjp = ddata->pending->job;
#else
        ULONG           xindx;
        BtjobRef        bjp = &Xbuffer->Ring[xindx = getxbuf()];
        unsigned        retc;
#endif
        GString         *title_text = g_string_new(NULL);
        gchar           *txt;
        const   gchar   *direc;
        unsigned        numask = 0;
        int             cnt;

        *bjp = ddata->cjob;

        /* Copy in text stuff first so we don't waste time if no space */

#ifdef IN_XBTR
        g_string_assign(title_text, gtk_entry_get_text(GTK_ENTRY(ddata->titw)));
#else
        if  (jobqueue)
                g_string_printf(title_text, "%s:%s", jobqueue, gtk_entry_get_text(GTK_ENTRY(ddata->titw)));
        else
                g_string_assign(title_text, gtk_entry_get_text(GTK_ENTRY(ddata->titw)));
#endif

        /* Directory */

        direc = gtk_entry_get_text(GTK_ENTRY(ddata->dirw));

        if  (!repackjob(bjp, &ddata->cjob, direc, title_text->str, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
#ifndef IN_XBTR
                freexbuf(xindx);
#endif
                g_string_free(title_text, TRUE);
                doerror($EH{Too many job strings});
                return  0;
        }
        g_string_free(title_text, TRUE);

        /* Now get priority, load level, cmd interp */

        bjp->h.bj_pri = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->priw));
        bjp->h.bj_ll = (USHORT) gtk_range_get_value(GTK_RANGE(ddata->llw));
        txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ddata->ciw));
        strcpy(bjp->h.bj_cmdinterp, txt);
        g_free(txt);

        /* Umask */

        for  (cnt = 0;  cnt < 9;  cnt++)
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->umw[cnt])))
                        numask |= 1 << (8 - cnt);

        bjp->h.bj_umask = numask;

        /* Ulimit */

        bjp->h.bj_ulimit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->ulw)) <<
                           (gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->bkmw)) * 10);

        /* Exit codes */

        extract_exits(&bjp->h.bj_exits.nlower, &bjp->h.bj_exits.nupper, ddata->new);
        extract_exits(&bjp->h.bj_exits.elower, &bjp->h.bj_exits.eupper, ddata->eew);

        /* No advance on error */

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->noadvw)))
                bjp->h.bj_jflags |= BJ_NOADVIFERR;
        else
                bjp->h.bj_jflags &= ~BJ_NOADVIFERR;

        /* Export settings */

        bjp->h.bj_jflags &= ~(BJ_REMRUNNABLE|BJ_EXPORT);
        cnt = gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->exportw));
        if  (cnt > 0)  {
                bjp->h.bj_jflags |= BJ_EXPORT;
                if  (cnt > 1)
                        bjp->h.bj_jflags |= BJ_REMRUNNABLE;
        }

        /* Delete time */

        bjp->h.bj_deltime = (USHORT) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->delhw));

        /* Run time */

        bjp->h.bj_runtime = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->rthw)) * 3600L +
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->rtmw)) * 60L +
                            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->rtsw));

        /* Auto-Kill signal */

        cnt = gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->autoksigw));
        if  (cnt >= 0)
                bjp->h.bj_autoksig = autoksiglist[cnt].signum;

        /* Grace time */

        bjp->h.bj_runon = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->romw)) * 60 +
                          gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->rosw));

        /* Mail and write flags */

        bjp->h.bj_jflags &= ~(BJ_MAIL|BJ_WRT);
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->mailw)))
                bjp->h.bj_jflags |= BJ_MAIL;
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->wrtw)))
                bjp->h.bj_jflags |= BJ_WRT;

#ifdef IN_XBTR
        update_title(ddata->pending);
        note_changes(ddata->pending);
#else
        wjmsg(J_CHANGE, xindx);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, bjp);
        freexbuf(xindx);
#endif
        return  1;
}

#ifdef IN_XBTR
static void  cb_jobprops(GtkAction *action, const int wpage)
#else
static void  cb_jobprops(const int wpage)
#endif
{
        struct  propdialog_data  ddata;
        GtkWidget  *dlg;
#ifdef IN_XBTR
        struct  pend_job  *pj = job_or_deflt(action);

        if  (!pj)
                return;

        /* Make copy of job */
        ddata.cjob = *pj->job;
        ddata.pending = pj;
#else
        BtjobRef        cj = getselectedjob(BTM_WRITE);

        if  (!cj)
                return;

        /* Make copy of job */
        ddata.cjob = *cj;
#endif

        dlg = create_propdialog(&ddata, wpage);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK  &&  !extract_job_propresp(&ddata))
                ;
        gtk_widget_destroy(dlg);
        free((char *) ddata.lllist);
}

#ifdef IN_XBTR
void  cb_titprill(GtkAction *action)
{
        cb_jobprops(action, JOBPROP_TITPRILL_PAGE);
}
void  cb_procpar(GtkAction *action)
{
        cb_jobprops(action, JOBPROP_PROCPAR_PAGE);
}
void  cb_timelim(GtkAction *action)
{
        cb_jobprops(action, JOBPROP_TIMELIM_PAGE);
}
void  cb_mailwrt(GtkAction *action)
{
        cb_jobprops(action, JOBPROP_MAILWRT_PAGE);
}
#else
void  cb_titprill()
{
        cb_jobprops(JOBPROP_TITPRILL_PAGE);
}
void  cb_procpar()
{
        cb_jobprops(JOBPROP_PROCPAR_PAGE);
}
void  cb_timelim()
{
        cb_jobprops(JOBPROP_TIMELIM_PAGE);
}
void  cb_mailwrt()
{
        cb_jobprops(JOBPROP_MAILWRT_PAGE);
}
#endif

extern  void    setup_jmodebits(GtkWidget *, GtkWidget *[3][NUM_JMODEBITS], USHORT, USHORT, USHORT);
extern  void    read_jmodes(GtkWidget *[3][NUM_JMODEBITS], USHORT *, USHORT *, USHORT *);

#ifdef IN_XBTR
void  cb_jperm(GtkAction *action)
{
        struct  pend_job  *pj = job_or_deflt(action);
        BtjobRef  cj;
        GtkWidget       *dlg, *frame, *jmodes[3][NUM_JMODEBITS];
        char    *pr;
        Btmode  copy_mode;

        if  (!pj)
                return;
        cj = pj->job;
        copy_mode = cj->h.bj_mode;

        dlg = start_jobdlg(pj, $P{xbtq job mode dlgtit}, $P{xbtq job mode lab});
#else
void  cb_jperm()
{
        BtjobRef  cj = getselectedjob(BTM_WRMODE);
        GtkWidget       *dlg, *frame, *jmodes[3][NUM_JMODEBITS];
        char    *pr;
        Btjob   cjob;

        if  (!cj)
                return;
        cjob = *cj;

        dlg = start_jobdlg(&cjob, $P{xbtq job mode dlgtit}, $P{xbtq job mode lab});
#endif
        pr = gprompt($P{xbtq job mode frame lab});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
#ifdef IN_XBTR
        setup_jmodebits(frame, jmodes, copy_mode.u_flags, copy_mode.g_flags, copy_mode.o_flags);
#else
        setup_jmodebits(frame, jmodes, cjob.h.bj_mode.u_flags, cjob.h.bj_mode.g_flags, cjob.h.bj_mode.o_flags);
#endif
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
#ifdef IN_XBTR
                read_jmodes(jmodes, &copy_mode.u_flags, &copy_mode.g_flags, &copy_mode.o_flags);
                if  (copy_mode.u_flags != cj->h.bj_mode.u_flags ||
                     copy_mode.g_flags != cj->h.bj_mode.g_flags ||
                     copy_mode.o_flags != cj->h.bj_mode.o_flags)  {
                        cj->h.bj_mode = copy_mode;
                        note_changes(pj);
                }
#else
                read_jmodes(jmodes, &cjob.h.bj_mode.u_flags, &cjob.h.bj_mode.g_flags, &cjob.h.bj_mode.o_flags);
                if  (cjob.h.bj_mode.u_flags != cj->h.bj_mode.u_flags ||
                     cjob.h.bj_mode.g_flags != cj->h.bj_mode.g_flags ||
                     cjob.h.bj_mode.o_flags != cj->h.bj_mode.o_flags)  {
                        ULONG   xindx;
                        unsigned        retc;
                        BtjobRef  bjp = &Xbuffer->Ring[xindx = getxbuf()];
                        *bjp = cjob;
                        wjmsg(J_CHMOD, xindx);
                        if  ((retc = readreply()) != J_OK)
                                qdojerror(retc, bjp);
                        freexbuf(xindx);
                }
#endif
        }
        gtk_widget_destroy(dlg);
}

#ifndef IN_XBTR
void  cb_jowner()
{
        BtjobRef  cj = getselectedjob(BTM_RDMODE);
        GtkWidget  *dlg, *hbox, *usel, *gsel;
        int     uw, cnt;
        char    **uglist, **up;

        if  (!cj)
                return;

        dlg = start_jobdlg(cj, $P{xbtq job owner dlgtit}, $P{xbtq job owner lab});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq job owner user}), FALSE, FALSE, DEF_DLG_HPAD);
        usel = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), usel, FALSE, FALSE, DEF_DLG_VPAD);
        uw = -1;
        cnt = 0;
        uglist = gen_ulist((char *) 0);

        for  (up = uglist;  *up;  up++)  {
                gtk_combo_box_append_text(GTK_COMBO_BOX(usel), *up);
                if  (strcmp(*up, cj->h.bj_mode.o_user) == 0)
                        uw = cnt;
                cnt++;
        }
        freehelp(uglist);
        if  (uw >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(usel), uw);

        /* Same for groups */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq job owner group}), FALSE, FALSE, DEF_DLG_HPAD);
        gsel = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), gsel, FALSE, FALSE, DEF_DLG_VPAD);
        uw = -1;
        cnt = 0;
        uglist = gen_glist((char *) 0);

        for  (up = uglist;  *up;  up++)  {
                gtk_combo_box_append_text(GTK_COMBO_BOX(gsel), *up);
                if  (strcmp(*up, cj->h.bj_mode.o_group) == 0)
                        uw = cnt;
                cnt++;
        }
        freehelp(uglist);
        if  (uw >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(gsel), uw);

        if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                if  (!mpermitted(&cj->h.bj_mode, cj->h.bj_mode.o_uid == Realuid? BTM_UGIVE: BTM_UTAKE, mypriv->btu_priv))
                        gtk_widget_set_sensitive(usel, FALSE);
                if  (!mpermitted(&cj->h.bj_mode, cj->h.bj_mode.o_gid == Realgid? BTM_GGIVE: BTM_GTAKE, mypriv->btu_priv))
                        gtk_widget_set_sensitive(gsel, FALSE);
        }

        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                gchar  *newug;

                newug = gtk_combo_box_get_active_text(GTK_COMBO_BOX(gsel));
                if  (strcmp(newug, cj->h.bj_mode.o_group) != 0)  {
                        int_ugid_t  nug = lookup_gname(newug);
                        unsigned        retc;

                        if  (nug == UNKNOWN_GID)
                                doerror($EH{xmbtq invalid group});
                        else  {
                                Oreq.sh_params.param = nug;
                                qwjimsg(J_CHGRP, cj);
                                if  ((retc = readreply()) != J_OK)
                                        qdojerror(retc, cj);
                        }
                }
                g_free(newug);
                newug = gtk_combo_box_get_active_text(GTK_COMBO_BOX(usel));
                if  (strcmp(newug, cj->h.bj_mode.o_user) != 0)  {
                        int_ugid_t      nug = lookup_uname(newug);
                        unsigned        retc;

                        if  (nug == UNKNOWN_UID)
                                doerror($EH{xmbtq invalid user});
                        else  {
                                Oreq.sh_params.param = nug;
                                qwjimsg(J_CHOWN, cj);
                                if  ((retc = readreply()) != J_OK)
                                        qdojerror(retc, cj);
                        }
                }
                g_free(newug);
        }

        gtk_widget_destroy(dlg);
}

void  cb_jdel()
{
        BtjobRef  cj = getselectedjob(BTM_DELETE);
        unsigned        retc;

        if  (!cj)
                return;

        if  (cj->h.bj_progress >= BJP_STARTUP1)  {
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);
                doerror($EH{xmbtq job running});
                return;
        }
        if  (Dispflags & DF_CONFABORT  &&  !Confirm($PH{xmbtq confirm delete job}))
                return;

        qwjimsg(J_DELETE, cj);
        retc = readreply();
        if  (retc != J_OK)
                qdojerror(retc, cj);
}

void  cb_timedef()
{
        GtkWidget       *dlg, *hbox, *stylew, *unitsw, *skipw, *avframe, *avtab, *av_days[TC_NDAYS];
        GtkAdjustment   *adj;
        int             cnt;
        char            *pr;

        init_tdefaults();
        dlg = gprompt_dialog(toplevel, $P{xbtq time dflt dlgtit});

        /* First row - style */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq deflt style lab}), FALSE, FALSE, DEF_DLG_HPAD);
        stylew = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), stylew, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_combo_box_append_text(GTK_COMBO_BOX(stylew), delend_full);
        gtk_combo_box_append_text(GTK_COMBO_BOX(stylew), retend_full);
        for  (cnt = 0;  cnt <= TC_YEARS-TC_MINUTES;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(stylew), disp_alt(cnt, repunit_full));
        gtk_combo_box_set_active(GTK_COMBO_BOX(stylew), default_interval);

        /* Second row - number of units */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq deflt units lab}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) default_rate, 1.0, 9999.0, 1.0, 10.0, 0.0);
        unitsw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), unitsw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Frame of avoiding days */

        pr = gprompt($P{xbtq avoiding days frame lab});
        avframe = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(avframe), 0.0, 1.0);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), avframe, FALSE, FALSE, DEF_DLG_VPAD);
        avtab = gtk_table_new(2, 4, TRUE);
        gtk_container_add(GTK_CONTAINER(avframe), avtab);

        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)  {
                GtkWidget  *butt = gtk_check_button_new_with_label(disp_alt(cnt, daynames_full));
                int  row = cnt & 1;
                int  col = cnt >> 1;
                gtk_table_attach_defaults(GTK_TABLE(avtab), butt, col, col+1, row, row+1);
                if  (defavoid & (1 << cnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                av_days[cnt] = butt;
        }

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq if not poss lab}), FALSE, FALSE, DEF_DLG_VPAD);

        skipw = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), skipw, FALSE, FALSE, DEF_DLG_HPAD);
        for  (cnt = 0;  cnt < 4;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(skipw), disp_alt(cnt, ifnposs_names));

        gtk_combo_box_set_active(GTK_COMBO_BOX(skipw), default_nposs);

        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) ==  GTK_RESPONSE_OK)  {
                default_interval = gtk_combo_box_get_active(GTK_COMBO_BOX(stylew));
                default_rate = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(unitsw));
                defavoid = 0;
                for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(av_days[cnt])))
                                defavoid |= 1 << cnt;
                default_nposs = gtk_combo_box_get_active(GTK_COMBO_BOX(skipw));
        }
        gtk_widget_destroy(dlg);
}

static int  jmacroexec(const char *str, BtjobRef jp)
{
        PIDTYPE pid;
        int     status;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) == 0)  {
                const  char     *argbuf[3];
                argbuf[0] = (const char *) str;
                if  (jp)  {
                        argbuf[1] = JOB_NUMBER(jp);
                        argbuf[2] = (const char *) 0;
                }
                else
                        argbuf[1] = (const char *) 0;
                Ignored_error = chdir(Curr_pwd);
                execv(execprog, (char **) argbuf);
                exit(E_BTEXEC1);
        }
        if  (pid < 0)  {
                doerror($EH{xmbtq macro cannot fork});
                return  0;
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
                        doerror($EH{xmbtq job macro signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror($EH{xmbtq job macro exit code});
                }
                return  0;
        }

        return  1;
}

static  struct  stringvec  previous_commands;

/* Version of job macro for where we prompt */

void  cb_jmac()
{
        BtjobRef  jp = getselectedjob(0);
        GtkWidget  *dlg, *lab, *hbox, *cmdentry;
        int        oldmac = is_init(previous_commands);
        char       *pr;

        dlg = gprompt_dialog(toplevel, $P{xbtq jmac dlg});

        if  (jp)  {
                GString *labp = g_string_new(NULL);
                pr = gprompt($P{xbtq jmac named});
                g_string_printf(labp, "%s %s", pr, JOB_NUMBER(jp));
                free(pr);
                lab = gtk_label_new(labp->str);
                g_string_free(labp, TRUE);
        }
        else
                lab = gprompt_label($P{xbtq jmac noname});

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, 0);
        lab = gprompt_label($P{xbtq jmac cmd});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        if  (oldmac)  {
                unsigned  cnt;
                cmdentry = gtk_combo_box_entry_new_text();
                for  (cnt = 0;  cnt < stringvec_count(previous_commands);  cnt++)
                        gtk_combo_box_append_text(GTK_COMBO_BOX(cmdentry), stringvec_nth(previous_commands, cnt));
        }
        else
                cmdentry = gtk_entry_new();

        gtk_box_pack_start(GTK_BOX(hbox), cmdentry, FALSE, FALSE, 0);
        gtk_widget_show_all(dlg);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *cmdtext;
                if  (oldmac)
                        cmdtext = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cmdentry))));
                else
                        cmdtext = gtk_entry_get_text(GTK_ENTRY(cmdentry));
                if  (strlen(cmdtext) == 0)  {
                        doerror($EH{xbtq empty macro command});
                        continue;
                }

                if  (jmacroexec(cmdtext, jp))  {
                        if  (add_macro_to_list(cmdtext, 'j', jobmacs))
                                break;
                        if  (!oldmac)
                                stringvec_init(&previous_commands);
                        stringvec_insert_unique(&previous_commands, cmdtext);
                        break;
                }
        }
        gtk_widget_destroy(dlg);
}

void  jmacruncb(GtkAction *act, struct macromenitem *mitem)
{
        jmacroexec(mitem->cmd, getselectedjob(0));
}
#endif /* ! IN_XBTR */
