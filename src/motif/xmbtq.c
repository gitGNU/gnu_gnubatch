/* xmbtq.c -- main module for gbch-xmq

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

static  char    rcsid1[] = "@(#) $Id: xmbtq.c,v 1.9 2009/02/18 06:51:32 toadwarble Exp $";              /* We use these in the about message */
static  char    rcsid2[] = "@(#) $Revision: 1.9 $";

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/CascadeB.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/SeparatoGP.h>
#include <Xm/TextF.h>
#include "incl_sig.h"
#include "incl_net.h"
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "network.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "ecodes.h"
#include "errnums.h"
#include "helpargs.h"
#include "statenums.h"
#include "ipcstuff.h"
#include "cfile.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbq_ext.h"
#include "xmmenu.h"
#include "optflags.h"

#define IPC_MODE        0

void  initcifile();
void  jdisplay();
void  vdisplay();
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
void  allocate_colours();
#endif

char    *spdir,
        *Curr_pwd;

char    Confvarname[] = "XMBTQCONF";

int     poll_time;

Shipc           Oreq;
extern  long    mymtype;

/* X Stuff */

XtAppContext    app;
Display         *dpy;

Widget  toplevel,       /* Main window */
        jtitwid,        /* Job title window */
        vtitwid,        /* Var title window */
        jwid,           /* Job scroll list */
        vwid;           /* Var scroll list */

static  Widget  panedw,         /* Paned window to stick rest in */
                jpopmenu,       /* Job popup menu */
                vpopmenu,       /* Ptr popup menu */
                menubar,        /* Menu */
                toolbar;        /* Optional toolbar */

XtIntervalId    Ptimeout;

typedef struct  {
        Boolean toolbar_pres;
        Boolean jobtit_pres;
        Boolean vartit_pres;
        Boolean footer_pres;
        int     pollfreq;
        int     rtime, rint;
}  vrec_t;

static void  cb_about();
static void  cb_quit(Widget, int);
static void  cb_saveopts(Widget);
extern void  initmoremsgs();
extern void  do_caltog(Widget, XEvent *, String *, Cardinal *);

static  XtResource      resources[] = {
        { "toolbarPresent", "ToolbarPresent", XtRBoolean, sizeof(Boolean),
                  XtOffsetOf(vrec_t, toolbar_pres), XtRImmediate, False },
        { "jtitlePresent", "JtitlePresent", XtRBoolean, sizeof(Boolean),
                  XtOffsetOf(vrec_t, jobtit_pres), XtRImmediate, False },
        { "vtitlePresent", "VtitlePresent", XtRBoolean, sizeof(Boolean),
                  XtOffsetOf(vrec_t, vartit_pres), XtRImmediate, False },
        { "footerPresent", "FooterPresent", XtRBoolean, sizeof(Boolean),
                  XtOffsetOf(vrec_t, footer_pres), XtRImmediate, False },
        { "pollFreq", "PollFreq", XtRInt, sizeof(int),
                  XtOffsetOf(vrec_t, pollfreq), XtRImmediate, (XtPointer) 10 },
        { "repeatTime", "RepeatTime", XtRInt, sizeof(int),
                  XtOffsetOf(vrec_t, rtime), XtRImmediate, (XtPointer) 500 },
        { "repeatInt", "RepeatInt", XtRInt, sizeof(int),
                  XtOffsetOf(vrec_t, rint), XtRImmediate, (XtPointer) 100 }};

static  casc_button
opt_casc[] = {
        {       ITEM,   "Viewopts",     cb_viewopt,     0       },
        {       SEP     },
        {       ITEM,   "Saveopts",     cb_saveopts,    0       },
        {       DSEP    },
        {       ITEM,   "Syserror",     cb_syserr,      0       },
        {       DSEP    },
        {       ITEM,   "Quit", cb_quit,        0       }},
act_casc[] = {
        {       ITEM,   "Setrun",       cb_jstate,      BJP_NONE        },
        {       ITEM,   "Setcanc",      cb_jstate,      BJP_CANCELLED   },
        {       ITEM,   "Force",        cb_jact,        J_FORCENA       },
        {       ITEM,   "Forceadv",     cb_jact,        J_FORCE         },
        {       ITEM,   "Intsig",       cb_jkill,       SIGINT          },
        {       ITEM,   "Quitsig",      cb_jkill,       SIGQUIT         },
#ifdef  SIGSTOP
        {       ITEM,   "Stopsig",      cb_jkill,       SIGSTOP         },
#endif
#ifdef  SIGCONT
        {       ITEM,   "Contsig",      cb_jkill,       SIGCONT         },
#endif
        {       ITEM,   "Othersig",     cb_jkill,       0               }},
job_casc[] = {
        {       ITEM,   "View",         cb_view,        0       },
        {       ITEM,   "Time",         cb_stime,       0       },
        {       ITEM,   "Advtime",      cb_advtime,     0       },
        {       ITEM,   "Title",        cb_titprill,    0       },
        {       ITEM,   "Process",      cb_procpar,     0       },
        {       ITEM,   "Runtime",      cb_runtime,     0       },
        {       ITEM,   "Mail",         cb_jmail,       0       },
        {       ITEM,   "Permj",        cb_jperm,       0       },
        {       SEP     },
        {       ITEM,   "Args",         cb_jargs,       0       },
        {       ITEM,   "Env",          cb_jenv,        0       },
        {       ITEM,   "Redirs",       cb_jredirs,     0       }},
creat_casc[] = {
        {       ITEM,   "Createj",      cb_jcreate,     0       },
        {       SEP     },
        {       ITEM,   "Timedefs",     cb_deftime,     0       },
        {       ITEM,   "Conddefs",     cb_defcond,     0       },
        {       ITEM,   "Assdefs",      cb_defass,      0       },
        {       SEP     },
        {       ITEM,   "Createv",      cb_vcreate,     0       },
        {       ITEM,   "Rename",       cb_vrename,     0       },
        {       DSEP    },
        {       ITEM,   "Interps",      cb_interps,     0       },
        {       SEP     },
        {       ITEM,   "Holidays",     cb_hols,        0       }},
del_casc[] = {
        {       ITEM,   "Deletej",      cb_jact,        J_DELETE        },
        {       ITEM,   "Deletev",      cb_vact,        V_DELETE        },
        {       SEP     },
        {       ITEM,   "Unqueue",      cb_unqueue,     0       },
        {       SEP     },
        {       ITEM,   "Freezeh",      cb_freeze,      1       },
        {       ITEM,   "Freezec",      cb_freeze,      0       }},
cond_casc[] = {
        {       ITEM,   "Conds",        cb_jconds,      0       },
        {       ITEM,   "Asses",        cb_jass,        0       }},
var_casc[] = {
        {       ITEM,   "Assign",       cb_vass,        0       },
        {       ITEM,   "Comment",      cb_vcomm,       0       },
        {       SEP     },
        {       ITEM,   "Export",       cb_vexport,     VF_EXPORT       },
        {       ITEM,   "Clustered",    cb_vexport,     VF_CLUSTER      },
        {       ITEM,   "Local",        cb_vexport,     0       },
        {       ITEM,   "Permv",        cb_vperm,       0       },
        {       SEP     },
        {       ITEM,   "Plus",         cb_arith,       (int) '+'       },
        {       ITEM,   "Minus",        cb_arith,       (int) '-'       },
        {       ITEM,   "Times",        cb_arith,       (int) '*'       },
        {       ITEM,   "Div",          cb_arith,       (int) '/'       },
        {       ITEM,   "Mod",          cb_arith,       (int) '%'       },
        {       SEP     },
        {       ITEM,   "Arithc",       cb_setconst,    0       }},
search_casc[] = {
        {       ITEM,   "Search",       cb_srchfor,     0       },
        {       ITEM,   "Searchforw",   cb_rsrch,       0       },
        {       ITEM,   "Searchback",   cb_rsrch,       1       }},
help_casc[] = {
        {       ITEM,   "Help",         dohelp,         $H{xmbtq main help}     },
        {       ITEM,   "Helpon",       cb_chelp,       0       },
        {       SEP     },
        {       ITEM,   "About",        cb_about,       0       }},
jobpop_casc[] =  {
        {       ITEM,   "View",         cb_view,        0       },
        {       SEP     },
        {       ITEM,   "Time",         cb_stime,       0       },
        {       ITEM,   "Conds",        cb_jconds,      0       },
        {       ITEM,   "Asses",        cb_jass,        0       },
        {       SEP     },
        {       ITEM,   "Title",        cb_titprill,    0       },
        {       SEP     },
        {       ITEM,   "Setrun",       cb_jstate,      BJP_NONE        },
        {       ITEM,   "Setcanc",      cb_jstate,      BJP_CANCELLED   },
        {       ITEM,   "Force",        cb_jact,        J_FORCENA       },
        {       SEP     },
        {       ITEM,   "Deletej",      cb_jact,        J_DELETE        },
        {       SEP     },
        {       ITEM,   "Intsig",       cb_jkill,       SIGINT          },
        {       ITEM,   "Quitsig",      cb_jkill,       SIGQUIT         }},
varpop_casc[] =  {
        {       ITEM,   "Plus",         cb_arith,       (int) '+'       },
        {       ITEM,   "Minus",        cb_arith,       (int) '-'       },
        {       ITEM,   "Assign",       cb_vass,        0       },
        {       SEP     },
        {       ITEM,   "Export",       cb_vexport,     VF_EXPORT       },
        {       ITEM,   "Clustered",    cb_vexport,     VF_CLUSTER      },
        {       ITEM,   "Local",        cb_vexport,     0       },
        {       SEP     },
        {       ITEM,   "Deletev",      cb_vact,        V_DELETE        }};

static  pull_button
        opt_button = {
                "Options",      XtNumber(opt_casc),     $H{xmbtq options menu help},    opt_casc        },
        act_button = {
                "Action",       XtNumber(act_casc),     $H{xmbtq action menu help},     act_casc        },
        job_button = {
                "Jobs",         XtNumber(job_casc),     $H{xmbtq jobs menu help},       job_casc        },
        create_button = {
                "Create",       XtNumber(creat_casc),   $H{xmbtq create menu help},     creat_casc      },
        delete_button = {
                "Delete",       XtNumber(del_casc),     $H{xmbtq delete menu help},     del_casc        },
        cond_button = {
                "Condition",    XtNumber(cond_casc),    $H{xmbtq conds menu help},      cond_casc       },
        var_button = {
                "Variable",     XtNumber(var_casc),     $H{xmbtq vars menu help},       var_casc        },
        srch_button = {
                "Search",       XtNumber(search_casc),  $H{xmbtq search menu help},     search_casc     },
        help_button = {
                "Help",         XtNumber(help_casc),    $H{xmbtq help menu help},       help_casc,      1       };

static  pull_button     *menlist[] = {
        &opt_button,    &act_button,    &job_button,    &create_button,
        &delete_button, &cond_button,   &var_button,    &srch_button,
        &help_button
};

typedef struct  {
        char    *name;
        void    (*callback)();
        int     callback_data;
        int     helpnum;
}  tool_button;

static  tool_button
        toollist1[] = {
        {       "View",         cb_view,        0,              $H{xmbtq view button help}      },
        {       "Delj",         cb_jact,        J_DELETE,       $H{xmbtq delj button help}      },
        {       "Srun",         cb_jstate,      BJP_NONE,       $H{xmbtq setrun button help}    },
        {       "Scanc",        cb_jstate,      BJP_CANCELLED,  $H{xmbtq setcanc button help}   },
        {       "Go",           cb_jact,        J_FORCENA,      $H{xmbtq force button help}     },
        {       "Goadv",        cb_jact,        J_FORCE,        $H{xmbtq forceadv button help}  },
        {       "Int",          cb_jkill,       SIGINT,         $H{xmbtq intr button help}      },
        {       "Quit",         cb_jkill,       SIGQUIT,        $H{xmbtq quit button help}      }},
        toollist2[] = {
        {       "Time",         cb_stime,       0,              $H{xmbtq settime button help}   },
        {       "Cond",         cb_jconds,      0,              $H{xmbtq jcond button help}     },
        {       "Ass",          cb_jass,        0,              $H{xmbtq jass button help}      },
        {       "Vass",         cb_vass,        0,              $H{xmbtq ass button help}       },
        {       "Vplus",        cb_arith,       (int) '+',      $H{xmbtq vadd button help}      },
        {       "Vminus",       cb_arith,       (int) '-',      $H{xmbtq vsub button help}      }};

#if     defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)

/* Define our own bcopy and bzero because X uses these in places and
   we don't want to include some -libucb which pulls in funny sprintfs etc */

void  bcopy(void *from, void *to, unsigned count)
{
        memcpy(to, from, count);
}
void  bzero(void *to, unsigned count)
{
        memset(to, '\0', count);
}
#endif

/* If we get a message error die appropriately */

void  msg_error()
{
        doerror(jwid, errno == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
        exit(E_SETUP);
}

/* Send op-type message to scheduler */

void  womsg(unsigned code)
{
        Oreq.sh_params.mcode = code;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq), 0) < 0)
                msg_error();
}

void  exit_cleanup()
{
        if  (Ctrl_chan >= 0)
                womsg(O_LOGOFF);
}

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
#ifndef HAVE_ATEXIT
        exit_cleanup();
#endif
        exit(E_NOMEM);
}

/* Don't put exit as a callback or we'll get some weird exit code
   based on a Widget pointer.  */

static void  cb_quit(Widget w, int n)
{
#ifndef HAVE_ATEXIT
        exit_cleanup();
#endif
        exit(n);
}

static void  dumpbool(FILE *tf, char *name, const int value)
{
        fprintf(tf, "%s.%s:\t%s\n", progname, name, value? "True": "False");
}

static  char  *confline_arg;

static  void  save_confline_opt(FILE *fp, const char *vname)
{
        fprintf(fp, "%s=%s\n", vname, confline_arg);
}

static void  cb_saveopts(Widget w)
{
        int     ret, items;
        Dimension  wid;
        char    digbuf[20];

        if  (!Confirm(w, $PH{Confirm write options}))
                return;

        disp_str = "(Home)";

        digbuf[0] = Dispflags & DF_SUPPNULL? '0': '1';
        digbuf[1] = Dispflags & DF_LOCALONLY? '0': '1';
        digbuf[2] = Dispflags & DF_CONFABORT? '1': '0';
        digbuf[3] = Dispflags & DF_SCRKEEP? '1': '0';
        digbuf[4] = '\0';

        confline_arg = digbuf;
        if  ((ret = proc_save_opts((const char *) 0, "XMBTQDISPOPT", save_confline_opt)) != 0)  {
                doerror(w, ret);
                return;
        }

        confline_arg = Restru && Restru[0]? Restru: "-";
        proc_save_opts((const char *) 0, "XMBTQDISPUSER", save_confline_opt);
        confline_arg = Restrg && Restrg[0]? Restrg: "-";
        proc_save_opts((const char *) 0, "XMBTQDISPGROUP", save_confline_opt);
        confline_arg = jobqueue && jobqueue[0]? jobqueue: "-";
        proc_save_opts((const char *) 0, "XMBTQDISPQUEUE", save_confline_opt);

        XtVaGetValues(jwid, XmNwidth, &wid, XmNvisibleItemCount, &items, NULL);
        sprintf(digbuf, "%d", wid);
        confline_arg = digbuf;
        proc_save_opts((const char *) 0, "XMBTQJWIDTH", save_confline_opt);
        sprintf(digbuf, "%d", items);
        proc_save_opts((const char *) 0, "XMBTQJITEMS", save_confline_opt);
        XtVaGetValues(vwid, XmNvisibleItemCount, &items, NULL);
        sprintf(digbuf, "%d", items);
        proc_save_opts((const char *) 0, "XMBTQVITEMS", save_confline_opt);
}

static void  cb_about()
{
        Widget          dlg;
        char    buf[sizeof(rcsid1) + sizeof(rcsid2) + 2];
        sprintf(buf, "%s\n%s", rcsid1, rcsid2);
        dlg = XmCreateInformationDialog(jwid, "about", NULL, 0);
        XtVaSetValues(dlg,
                      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
                      XtVaTypedArg, XmNmessageString, XmRString, buf, strlen(buf),
                      NULL);
        XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
        XtManageChild(dlg);
        XtPopup(XtParent(dlg), XtGrabNone);
}

/* This deals with alarm calls whilst polling.  */

void  pollit(int n, XtIntervalId id)
{
        if  (Last_j_ser != Job_seg.dptr->js_serial)
                jdisplay();
        if  (Last_v_ser != Var_seg.dptr->vs_serial)
                vdisplay();
        Ptimeout = XtAppAddTimeOut(app, (ULONG) (poll_time * 1000), (XtTimerCallbackProc) pollit, (XtPointer) 0);
}

/* This notes signals from (presumably) the scheduler.  */

static RETSIGTYPE  markit(int sig)
{
#ifdef  UNSAFE_SIGNALS
        signal(sig, markit);
#endif
        if  (sig != QRFRESH)  {
#ifndef HAVE_ATEXIT
                exit_cleanup();
#endif
                exit(E_SIGNAL);
        }
        if  (Ptimeout)  {
                XtRemoveTimeOut(Ptimeout);
                Ptimeout = (XtIntervalId) 0;
        }
        Ptimeout = XtAppAddTimeOut(app, 10L, (XtTimerCallbackProc) pollit, (XtPointer) 0);
}

/* Other signals are errors Suppress final message....  */

static RETSIGTYPE  catchit(int n)
{
        Ctrl_chan = -1;
        exit(E_SIGNAL);
}

Widget  BuildPulldown(Widget menub, pull_button *item)
{
        int     cnt;
        Widget  pulldown, cascade, button;

        pulldown = XmCreatePulldownMenu(menub, "pulldown", NULL, 0);
        cascade = XtVaCreateManagedWidget(item->pull_name, xmCascadeButtonWidgetClass, menub,
                                          XmNsubMenuId, pulldown, NULL);

        if  (item->helpnum != 0)
                XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(item->helpnum));

        for  (cnt = 0;  cnt < item->nitems;  cnt++)  {
                char    sname[20];
                casc_button     *cb = &item->items[cnt];
                switch  (cb->type)  {
                case  SEP:
                        sprintf(sname, "separator%d", cnt);
                        button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown, NULL);
                        continue;
                case  DSEP:
                        sprintf(sname, "separator%d", cnt);
                        button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown,
                                                         XmNseparatorType, XmDOUBLE_LINE, NULL);
                        continue;
                case  ITEM:
                        button = XtVaCreateManagedWidget(cb->name, xmPushButtonGadgetClass, pulldown, NULL);
                        if  (cb->callback)
                                XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) cb->callback, INT_TO_XTPOINTER(cb->callback_data));
                        continue;
                }
        }
        if  (item->ishelp)
                return  cascade;
        return  NULL;
}

static void  setup_macros(Widget menub, const int helpcode, const int helpbase, char *pullname, XtCallbackProc macroproc)
{
        int     cnt, had = 0;
        Widget  pulldown, cascade, button;
        char    *macroprmpt[10];

        for  (cnt = 0;  cnt < 10;  cnt++)
                if  ((macroprmpt[cnt] = helpprmpt(helpbase+cnt)))
                        had++;

        if  (had <= 0)
                return;

        pulldown = XmCreatePulldownMenu(menub, pullname, NULL, 0);
        cascade = XtVaCreateManagedWidget(pullname, xmCascadeButtonWidgetClass, menub, XmNsubMenuId, pulldown, NULL);
        XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        for  (cnt = 0;  cnt < 10;  cnt++)  {
                char    sname[20];
                if  (!macroprmpt[cnt])
                        continue;
                free(macroprmpt[cnt]);
                sprintf(sname, "macro%d", cnt);
                button = XtVaCreateManagedWidget(sname, xmPushButtonGadgetClass, pulldown, NULL);
                XtAddCallback(button, XmNactivateCallback, macroproc, INT_TO_XTPOINTER(cnt));
        }
}

static void  setup_menus()
{
        int                     cnt;
        XtWidgetGeometry        size;
        Widget                  helpw;

        menubar = XmCreateMenuBar(panedw, "menubar", NULL, 0);

        /* Get rid of resize button for menubar */

        size.request_mode = CWHeight;
        XtQueryGeometry(menubar, NULL, &size);
        XtVaSetValues(menubar, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

        for  (cnt = 0;  cnt < XtNumber(menlist);  cnt++)
                if  ((helpw = BuildPulldown(menubar, menlist[cnt])))
                        XtVaSetValues(menubar, XmNmenuHelpWidget, helpw, NULL);

        setup_macros(menubar, $H{xmbtq job macro help}, $PH{Job or User macro}, "jobmacro", (XtCallbackProc) cb_macroj);
        setup_macros(menubar, $H{xmbtq var macro help}, $PH{Var macro}, "varmacro", (XtCallbackProc) cb_macrov);
        XtManageChild(menubar);
}

static void  Buildpopup(Widget wid, casc_button *list, unsigned nlist)
{
        unsigned  cnt;
        Widget  button;
        char    sname[20];

        for  (cnt = 0;  cnt < nlist;  cnt++)  {
                casc_button     *cb = &list[cnt];
                switch  (cb->type)  {
                case  SEP:
                        sprintf(sname, "separator%d", cnt);
                        XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, wid, NULL);
                        continue;
                case  DSEP:
                        sprintf(sname, "separator%d", cnt);
                        XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, wid, XmNseparatorType, XmDOUBLE_LINE, NULL);
                        continue;
                case  ITEM:
                        button = XtVaCreateManagedWidget(cb->name, xmPushButtonGadgetClass, wid, NULL);
                        if  (cb->callback)
                                XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) cb->callback, INT_TO_XTPOINTER(cb->callback_data));
                        continue;
                }
        }
}

static void  setup_popupmenus()
{
        jpopmenu = XmCreatePopupMenu(jwid, "jobpopup", NULL, 0);
        Buildpopup(jpopmenu, jobpop_casc, XtNumber(jobpop_casc));
        vpopmenu = XmCreatePopupMenu(vwid, "varpopup", NULL, 0);
        Buildpopup(vpopmenu, varpop_casc, XtNumber(varpop_casc));
}

static void  setup_toolbar()
{
        int                     cnt;

        toolbar = XtVaCreateManagedWidget("toolbar", xmRowColumnWidgetClass, panedw,
                                          XmNorientation,       XmHORIZONTAL,
                                          XmNpacking,           XmPACK_COLUMN,
                                          XmNnumColumns,        2,      /* Meaning rows */
                                          NULL);

        /* Set up buttons */

        for  (cnt = 0;  cnt < XtNumber(toollist1);  cnt++)  {
                Widget  w = XtVaCreateManagedWidget(toollist1[cnt].name, xmPushButtonWidgetClass, toolbar, NULL);
                if  (toollist1[cnt].callback)
                        XtAddCallback(w,
                                      XmNactivateCallback,
                                      (XtCallbackProc) toollist1[cnt].callback,
                                      INT_TO_XTPOINTER(toollist1[cnt].callback_data));
                if  (toollist1[cnt].helpnum != 0)
                        XtAddCallback(w, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(toollist1[cnt].helpnum));
        }
        for  (cnt = 0;  cnt < XtNumber(toollist2);  cnt++)  {
                Widget  w = XtVaCreateManagedWidget(toollist2[cnt].name, xmPushButtonWidgetClass, toolbar, NULL);
                if  (toollist2[cnt].callback)
                        XtAddCallback(w, XmNactivateCallback,
                                      (XtCallbackProc) toollist2[cnt].callback,
                                      INT_TO_XTPOINTER(toollist2[cnt].callback_data));
                if  (toollist2[cnt].helpnum != 0)
                        XtAddCallback(w, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(toollist2[cnt].helpnum));
        }
}

static Widget  maketitle(char *tname, char *tstring)
{
        Widget                  labv;
        XtWidgetGeometry        size;

        if  (tstring)  {
                XmString  str = XmStringCreateSimple(tstring);
                labv = XtVaCreateManagedWidget(tname,
                                               xmLabelWidgetClass, panedw,
                                               XmNlabelString,  str,
                                               NULL);
                XmStringFree(str);
        }
        else
                labv = XtVaCreateManagedWidget(tname, xmLabelWidgetClass, panedw, NULL);
        size.request_mode = CWHeight;
        XtQueryGeometry(labv, NULL, &size);
        XtVaSetValues(labv, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
        return  labv;
}

void  do_jpopup(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        XButtonPressedEvent  *bpe = (XButtonPressedEvent *) xev;
        int     pos = XmListYToPos(jwid, bpe->y);
        if  (pos <= 0)
                return;
        XmListSelectPos(jwid, pos, False);
        XmMenuPosition(jpopmenu, bpe);
        XtManageChild(jpopmenu);
}

void  do_vpopup(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        XButtonPressedEvent  *bpe = (XButtonPressedEvent *) xev;
        int     pos = XmListYToPos(vwid, bpe->y), *plist, nplist, cnt;
        /* Force to first of double line */
        if  (!(pos & 1))
                pos--;
        if  (pos <= 0)          /* <- covers 0 to start with */
                return;
        if  (XmListGetSelectedPos(vwid, &plist, &nplist))  {
                for  (cnt = 0;  cnt < nplist;  cnt++)
                        XmListDeselectPos(vwid, plist[cnt]);
                XtFree((char *) plist);
        }
        XmListSelectPos(vwid, pos, False);
        XmListSelectPos(vwid, pos+1, False);
        XmMenuPosition(vpopmenu, bpe);
        XtManageChild(vpopmenu);
}

#ifdef  ACCEL_TRANSLATIONS
void  do_quit(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_quit(wid, 0);
}

void  do_viewopts(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_viewopt(wid);
}

void  do_view(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_view();
}

void  do_setstat(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        if  (*nargs == 1)
                cb_jstate(wid, atoi(args[0]));
}

void  do_jact(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        if  (*nargs == 1)
                cb_jact(wid, atoi(args[0]));
}

void  do_kill(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        if  (*nargs != 1)
                return;
        cb_jkill(wid, atoi(args[0]));
}

void  do_stime(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_stime(wid);
}

void  do_advance(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_advtime();
}

void  do_titprill(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_titprill(wid);
}

void  do_procpar(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_procpar(wid);
}

void  do_timelim(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_runtime(wid);
}

void  do_mailwrt(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_jmail(wid);
}

void  do_args(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_jargs(wid);
}

void  do_envs(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_jenv(wid);
}

void  do_redirs(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_jredirs(wid);
}

void  do_jdelete(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_jact(wid, J_DELETE);
}

void  do_vdelete(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_vact(wid, V_DELETE);
}

void  do_vcreate(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        cb_vcreate(wid);
}

void  do_vflag(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        int     op;
        if  (*nargs != 1)
                return;
        op = atoi(args[0]);
        if  (op == 0  ||  op == VF_EXPORT  ||  op == VF_CLUSTER)
                cb_vexport(wid, op);
}

void  do_vadd(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        int     op;
        if  (*nargs != 1)
                return;
        op = atoi(args[0]);
        switch  (op)  {
        case  '+':case  '-':case  '*':case  '/':case  '%':
                cb_arith(wid, op);
        }
}

void  do_search(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        if  (*nargs != 1)
                return;
        cb_rsrch(wid, atoi(args[0]));
}

void  do_help(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        dohelp(wid, wid == jwid? $H{xmbtq jlist help}: $H{xmbtq vlist help});
}
#endif

static  XtActionsRec    arecs[] = {
#ifdef  ACCEL_TRANSLATIONS
        {       "do-viewopts",          do_viewopts     },
        {       "do-quit",              do_quit },
        {       "do-view",              do_view },
        {       "do-setstat",           do_setstat      },
        {       "do-jact",              do_jact },
        {       "do-kill",              do_kill },
        {       "do-stime",             do_stime        },
        {       "do-advance",           do_advance      },
        {       "do-titprill",          do_titprill     },
        {       "do-procpar",           do_procpar      },
        {       "do-timelim",           do_timelim      },
        {       "do-mailwrt",           do_mailwrt      },
        {       "do-args",              do_args },
        {       "do-envs",              do_envs },
        {       "do-redirs",            do_redirs       },
        {       "do-jdelete",           do_jdelete      },
        {       "do-vdelete",           do_vdelete      },
        {       "do-vcreate",           do_vcreate      },
        {       "do-vflag",             do_vflag        },
        {       "do-vadd",              do_vadd },
        {       "do-search",            do_search       },
        {       "do-help",              do_help },
#endif
        {       "do-jobpop",            do_jpopup       },
        {       "do-varpop",            do_vpopup       },
        {       "do-caltog",            do_caltog       }
};

#include "xmbtq.bm"

static void  wstart(int argc, char **argv)
{
        int     jwidth = -1, jitems = -1, vitems = -1;
        vrec_t  vrec;
        Pixmap  bitmap;
        char    *jtit, *arg;

        toplevel = XtVaAppInitialize(&app, "GBATCH", NULL, 0, &argc, argv, NULL, NULL);
        XtAppAddActions(app, arecs, XtNumber(arecs));
        XtGetApplicationResources(toplevel, &vrec, resources, XtNumber(resources), NULL, 0);
        bitmap = XCreatePixmapFromBitmapData(dpy = XtDisplay(toplevel),
                                             RootWindowOfScreen(XtScreen(toplevel)),
                                             xmbtq_bits, xmbtq_width, xmbtq_height, 1, 0, 1);
        XtVaSetValues(toplevel, XmNiconPixmap, bitmap, NULL);

        /* Set up parameters from resources */

        poll_time = vrec.pollfreq;
        arr_rtime = vrec.rtime;
        arr_rint = vrec.rint;

        /* Now do stuff from the saved options. */

        if  ((arg = optkeyword("XMSPQDISPOPT")))  {
                if  (arg[0])  {
                        if  (arg[0] == '0')
                                Dispflags |= DF_SUPPNULL;
                        else
                                Dispflags &= ~DF_SUPPNULL;
                        if  (arg[1])  {
                                if  (arg[1] == '0')
                                        Dispflags |= DF_LOCALONLY;
                                else
                                        Dispflags &= ~DF_LOCALONLY;
                                if  (arg[2])  {
                                        if  (arg[2] != '0')
                                                Dispflags |= DF_CONFABORT;
                                        else
                                                Dispflags &= ~DF_CONFABORT;
                                        if  (arg[3])  {
                                                if  (arg[3] != '0')
                                                        Dispflags |= DF_SCRKEEP;
                                                else
                                                        Dispflags &= ~DF_SCRKEEP;
                                        }
                                }
                        }
                }
                free(arg);
        }

        if  ((arg = optkeyword("XMBTQDISPUSER")))  {
                if  (strcmp(arg, "-") == 0)  {
                        Restru = (char *) 0;
                        free(arg);
                }
                else
                        Restru = arg;
        }
        if  ((arg = optkeyword("XMBTQDISPGROUP")))  {
                if  (strcmp(arg, "-") == 0)  {
                        Restrg = (char *) 0;
                        free(arg);
                }
                else
                        Restrg = arg;
        }
        if  ((arg = optkeyword("XMBTQDISPQUEUE")))  {
                if  (strcmp(arg, "-") == 0)  {
                        jobqueue = (char *) 0;
                        free(arg);
                }
                else
                        jobqueue = arg;
        }

        if  ((arg = optkeyword("XMBTQJWIDTH")))  {
                jwidth = atoi(arg);
                free(arg);
        }
        if  ((arg = optkeyword("XMBTQJITEMS")))  {
                jitems = atoi(arg);
                free(arg);
        }
        if  ((arg = optkeyword("XMBTQVITEMS")))  {
                vitems = atoi(arg);
                free(arg);
        }

        /* Now to create all the bits of the application */

        panedw = XtVaCreateWidget("layout", xmPanedWindowWidgetClass, toplevel, NULL);

        setup_menus();

        if  (vrec.toolbar_pres)
                setup_toolbar();

        jtit = get_jobtitle();

        if  (vrec.jobtit_pres)
                jtitwid = maketitle("jtitle", jtit);

        free(jtit);

        jwid = XmCreateScrolledList(panedw, "jlist", NULL, 0);
        XtAddCallback(jwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtq jlist help});
        if  (jwidth > 0)
                XtVaSetValues(jwid, XmNwidth, jwidth, NULL);
        if  (jitems > 0)
                XtVaSetValues(jwid, XmNvisibleItemCount, jitems, NULL);
        XtManageChild(jwid);
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
        allocate_colours();
#endif
        jtit = get_vartitle();

        if  (vrec.vartit_pres)
                vtitwid = maketitle("vtitle", jtit);

        free(jtit);

        vwid = XmCreateScrolledList(panedw, "vlist", NULL, 0);
        XtVaSetValues(vwid, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
        XtAddCallback(vwid, XmNmultipleSelectionCallback, (XtCallbackProc) vselect, NULL);
        XtAddCallback(vwid, XmNdefaultActionCallback, (XtCallbackProc) vselect, NULL);
        XtAddCallback(vwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtq vlist help});
        if  (vitems > 0)
                XtVaSetValues(vwid, XmNvisibleItemCount, vitems, NULL);
        XtManageChild(vwid);

        if  (vrec.footer_pres)
                maketitle("footer", (char *) 0);

        setup_popupmenus();
        XtManageChild(panedw);
        XtRealizeWidget(toplevel);
}

/* Tell the scheduler we are here and do the business The initial
   refresh will fill up the job and printer screens for us.  */

static void  process()
{
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
        z.sighandler_el = markit;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
        z.sighandler_el = catchit;
        sigact_routine(SIGINT, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGHUP, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#else
        signal(QRFRESH, markit);
        signal(SIGINT, catchit);
        signal(SIGQUIT, catchit);
        signal(SIGHUP, catchit);
        signal(SIGTERM, catchit);
#endif
        womsg(O_LOGON);
        readreply();
        markit(QRFRESH);        /* Cheat to kick off timeout */
        XtAppMainLoop(app);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(Confvarname, "xmbtq.help");
        SCRAMBLID_CHECK
        tzset();

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        SWAP_TO(Daemuid);
        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                disp_str = spdir;
                print_error($E{Cannot change directory});
                exit(E_NOCHDIR);
        }

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

        /* Set up scheduler request main parameters.  */

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        mypriv = getbtuser(Realuid);

        initmoremsgs();
        openjfile(1, 0);
        openvfile(1, 0);
        initxbuffer(1);
        initcifile();
        wstart(argc, argv);
        jdisplay();
        vdisplay();
#ifdef  HAVE_ATEXIT
        atexit(exit_cleanup);
#endif
        process();
        return  0;
}
