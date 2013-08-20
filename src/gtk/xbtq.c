/* xbtq.c -- main module for gbch-xq

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
#include <gtk/gtk.h>
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
#include "statenums.h"
#include "ipcstuff.h"
#include "cfile.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xbq_ext.h"
#include "optflags.h"
#include "gtk_lib.h"

#define DEFAULT_WIDTH   400
#define DEFAULT_HEIGHT  400

#define IPC_MODE        0

void  load_optfile();
void  initcifile();
void  job_redisplay();
void  var_redisplay();

char    *spdir,
        *Curr_pwd;

int     poll_time;

Shipc           Oreq;
extern  long    mymtype;

/* X Stuff */

GtkWidget       *toplevel,      /* Main window */
                *jwid,          /* Job scroll list */
                *vwid;          /* Variable scroll list */

GtkUIManager    *ui;

GtkListStore    *jlist_store,
                *vlist_store;

extern  struct  macromenitem    jobmacs[], varmacs[];

char            xml_format;
char            Dirty;

static void  cb_about();
static void  cb_quit();
extern void  cb_viewopt();
extern void  cb_saveopts();
extern void  cb_syserr();
extern void  cb_setrunc(GtkAction *);
extern void  cb_force(GtkAction *);
extern void  cb_kill(GtkAction *);
extern void  cb_view();
extern void  cb_time();
extern void  cb_advtime();
extern void  cb_titprill();
extern void  cb_procpar();
extern void  cb_timelim();
extern void  cb_mailwrt();
extern void  cb_jperm();
extern void  cb_jowner();
extern void  cb_args();
extern void  cb_env();
extern void  cb_redir();
extern void  cb_createj();
extern void  cb_timedef();
extern void  cb_jmac();
extern void  cb_vmac();
extern void  cb_jmacedit();
extern void  cb_vmacedit();
extern void  cb_conddef();
extern void  cb_assdef();
extern void  cb_createv();
extern void  cb_renamev();
extern void  cb_interps();
extern void  cb_hols();
extern void  cb_jdel();
extern void  cb_vdel();
extern void  cb_unqueue();
extern void  cb_freeze(GtkAction *);
extern void  cb_conds();
extern void  cb_asses();
extern void  cb_assign();
extern void  cb_vcomment();
extern void  cb_vexport();
extern void  cb_vperm();
extern void  cb_vowner();
extern void  cb_arith(GtkAction *);
extern void  cb_cassign();
extern void  cb_search();
extern void  cb_rsearch(GtkAction *);

extern void  loadmacs(const char, struct macromenitem *);

static GtkActionEntry entries[] = {
        { "FileMenu", NULL, "_File"  },
        { "ActMenu", NULL, "_Action"  },
        { "JobMenu", NULL, "_Jobs"  },
        { "CreateMenu", NULL, "_Create" },
        { "DeleteMenu", NULL, "_Delete" },
        { "CondMenu", NULL, "Co_nditions" },
        { "VarMenu", NULL, "_Variables"  },
        { "SrchMenu", NULL, "_Search"  },
        { "jmacMenu", NULL, "Job _macros" },
        { "vmacMenu", NULL, "Va_r macros" },
        { "HelpMenu", NULL, "_Help"  },
        { "Viewopts", NULL, "_View Options", "equal", "Select view", G_CALLBACK(cb_viewopt) },
        { "Saveopts", GTK_STOCK_SAVE, "_Save Options", NULL, "Remember view options", G_CALLBACK(cb_saveopts) },
        { "Syserror", NULL, "Display _Error Log", NULL, "Display system error log file", G_CALLBACK(cb_syserr) },
        { "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit program", G_CALLBACK(cb_quit)},
        { "Setrun", NULL, "Set job _runnable", "r", "Set job so it can run", G_CALLBACK(cb_setrunc) },
        { "Setcanc", NULL, "Set job _cancelled",  "z", "Set job to cancelled state", G_CALLBACK(cb_setrunc)  },
        { "Force", NULL, "_Force", "F", "Force job to run", G_CALLBACK(cb_force)  },
        { "Forceadv", NULL, "Force and _advance", "G", "Force job to run and advance", G_CALLBACK(cb_force) },
        { "Intsig", NULL, "Kill - _interrupt", "Delete", "Send interrupt signal to job", G_CALLBACK(cb_kill) },
        { "Quitsig", NULL, "Kill - _quit", "<control>backslash", "Send quit signal to job", G_CALLBACK(cb_kill) },
        { "Stopsig", GTK_STOCK_STOP, "_Stop", NULL, "Send stop signal to job", G_CALLBACK(cb_kill) },
        { "Contsig", NULL, "_Continue", NULL, "Send continue signal to job", G_CALLBACK(cb_kill) },
        { "Othersig", NULL, "_Other signal", NULL, "Send other signal to job", G_CALLBACK(cb_kill)  },
        { "View", NULL, "_View job script", "<shift>I", "View job script", G_CALLBACK(cb_view)  },
        { "Time", NULL, "Job _time parameters", "T", "Set job time parameters", G_CALLBACK(cb_time)  },
        { "Advtime", NULL, "_Advance to next time", "<shift>A", "Move time on to next", G_CALLBACK(cb_advtime)  },
        { "Title", NULL, "_Title priority load level", "P", "Set job title, priority, loadlevel", G_CALLBACK(cb_titprill)  },
        { "Process", NULL, "_Process parameters", "U", "Set job process parameters", G_CALLBACK(cb_procpar)  },
        { "Runtime", NULL, "Time _limits", "L", "Set job time limits", G_CALLBACK(cb_timelim)  },
        { "Mail", NULL, "_Mail", "<shift>F", "Set mail/write flags", G_CALLBACK(cb_mailwrt)  },
        { "Permj", NULL, "Set job _permissions", NULL, "Set permissions of job", G_CALLBACK(cb_jperm)  },
        { "Ownj", NULL, "Set job _owner", NULL, "Set owner of job", G_CALLBACK(cb_jowner)  },
        { "Args", NULL, "Set job ar_guments", "<shift>G", "Set job arguments", G_CALLBACK(cb_args)  },
        { "Env", NULL, "Set job _environment", "<shift>E", "Set environment variables", G_CALLBACK(cb_env)  },
        { "Redirs", NULL, "Set I/O _redirections", "<shift>R", "Select I/O redirections for job", G_CALLBACK(cb_redir)  },
        { "Createj", NULL, "Create _job", NULL, "Create a job", G_CALLBACK(cb_createj)  },
        { "Timedefs", NULL, "_Time defaults", NULL, "Set default time parameters", G_CALLBACK(cb_timedef)  },
        { "Conddefs", NULL, "_Condition defaults", NULL, "Set default conditions", G_CALLBACK(cb_conddef)  },
        { "Assdefs", NULL, "_Assignment defaults", NULL, "Set default assignments", G_CALLBACK(cb_assdef)  },
        { "Createv", NULL, "Create _variable", "<shift>C", "Create a variable", G_CALLBACK(cb_createv)  },
        { "Rename", NULL, "_Rename variable", NULL, "Rename a variable", G_CALLBACK(cb_renamev)  },
        { "Interps", NULL, "Command _interpreters", NULL, "Edit command interpreter list", G_CALLBACK(cb_interps)  },
        { "Holidays", NULL, "_Holiday list", NULL, "Edit holiday list", G_CALLBACK(cb_hols)  },
        { "Deletej", NULL, "Delete _job", NULL, "Delete selected job", G_CALLBACK(cb_jdel)  },
        { "Deletev", NULL, "Delete _variable", NULL, "Delete selected variable", G_CALLBACK(cb_vdel)  },
        { "Unqueue", NULL, "_Unqueue job", NULL, "Unqueue or copy job", G_CALLBACK(cb_unqueue)  },
        { "Freeeh", NULL, "Freeze _home", NULL, "Save default options to home dir", G_CALLBACK(cb_freeze)  },
        { "Freeec", NULL, "Freeze _current", NULL, "Save default options to current dir", G_CALLBACK(cb_freeze)  },
        { "Conds", NULL, "_Conditions list", NULL, "Edit conditions list for job", G_CALLBACK(cb_conds)  },
        { "Asses", NULL, "_Assignments list", NULL, "Edit assignments list for job", G_CALLBACK(cb_asses)  },
        { "Assign", NULL, "_Assign", NULL, "Assign new value to variable", G_CALLBACK(cb_assign)  },
        { "Comment", NULL, "_Comment", NULL, "Give variable a comment", G_CALLBACK(cb_vcomment)  },
        { "Export", NULL, "_Export", NULL, "Set variable local/exported/cluster", G_CALLBACK(cb_vexport)  },
        { "Permv", NULL, "Variable _perms", NULL, "Set variable permissions", G_CALLBACK(cb_vperm)  },
        { "Ownv", NULL, "Variable _owner", NULL, "Set variable owner", G_CALLBACK(cb_vowner)  },
        { "Plus", NULL, "_Add to variable", "plus", "Add numeric constant to variable", G_CALLBACK(cb_arith)  },
        { "Minus", NULL, "_Subtract from var", NULL, "Subtract numeric constant from variable", G_CALLBACK(cb_arith)  },
        { "Times", NULL, "_Multiply variable", NULL, "Multiply variable by numeric constant", G_CALLBACK(cb_arith)  },
        { "Div", NULL, "_Divide variable", NULL, "Divide variable by numeric constant", G_CALLBACK(cb_arith)  },
        { "Mod", NULL, "_Mod_ulus variable", NULL, "Replace variable by remainder divided by numeric constant", G_CALLBACK(cb_arith)  },
        { "Arithc", NULL, "_Set numeric constant", NULL, "Assign to numeric constant", G_CALLBACK(cb_cassign)  },
        { "Search", NULL, "Search for...", NULL, "Search jobs or variables", G_CALLBACK(cb_search)  },
        { "Fsearch", NULL, "Search _forward", "F3", "Repeat last search going forward", G_CALLBACK(cb_rsearch)  },
        { "Rsearch", NULL, "Search _backward", "F4", "Repeat last search going backward", G_CALLBACK(cb_rsearch)  },
        { "jrunmac", NULL, "_Run macro command", NULL, "Run macro for job", G_CALLBACK(cb_jmac)  },
        { "Jmacedit", NULL, "_Edit macro list", NULL, "Edit list of job macros", G_CALLBACK(cb_jmacedit)  },
        { "vrunmac", NULL, "_Run macro command", NULL, "Run macro for variable", G_CALLBACK(cb_vmac)  },
        { "Vmacedit", NULL, "_Edit macro list", NULL, "Edit list of variable maros", G_CALLBACK(cb_vmacedit)  },
        { "About", NULL, "About xbtq", NULL, "About xbtq", G_CALLBACK(cb_about)}  };

extern void  initmoremsgs();
extern void  init_jlist_win();
extern void  init_vlist_win();
extern void  init_jdisplay();
extern void  init_vdisplay();

/* If we get a message error die appropriately */

void  msg_error()
{
        doerror(errno == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
        exit(E_SETUP);
}

/* Send op-type message to scheduler */

void  womsg(unsigned code)
{
        Oreq.sh_params.mcode = code;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq), 0) < 0)
                msg_error();
}

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

char    *authlist[] =  { "John M Collins", NULL  };

static void  cb_about()
{
        GtkWidget  *dlg = gtk_about_dialog_new();
        char    *cp = strchr(rcsid2, ':');
        char    vbuf[20];

        if  (!cp)
                strcpy(vbuf, "Initial version");
        else  {
                char  *ep;
                cp++;
                ep = strchr(cp, '$');
                int  n = ep - cp;
                strncpy(vbuf, cp, n);
                vbuf[n] = '\0';
        }
        gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), vbuf);
        gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Xi Software Ltd 2009");
        gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "http://www.xisl.com");
        gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), (const char **) authlist);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
}

/* Don't put exit as a callback or we'll get some weird exit code
   based on a Widget pointer.  */

static void  cb_quit()
{
        if  (Dirty  &&  !Confirm($PH{xbtq changes not saved ok}))
                return;
        gtk_main_quit();
}

gboolean  check_dirty()
{
        if  (Dirty  &&  !Confirm($PH{xbtq changes not saved ok}))
                return  TRUE;
        return  FALSE;
}

/* Possibly redisplay if something has changed.  */

gboolean  poll_changes()
{
        if  (Last_j_ser != Job_seg.dptr->js_serial)
                job_redisplay();
        if  (Last_v_ser != Var_seg.dptr->vs_serial)
                var_redisplay();
        return  TRUE;
}

static void  winit()
{
        GError *err;
        char    *fn;

        toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size(GTK_WINDOW(toplevel), DEFAULT_WIDTH, DEFAULT_HEIGHT);
        fn = gprompt($P{xbtq app title});
        gtk_window_set_title(GTK_WINDOW(toplevel), fn);
        free(fn);
        gtk_container_set_border_width(GTK_CONTAINER(toplevel), 5);
        fn = envprocess(XBTQ_ICON);
        gtk_window_set_default_icon_from_file(fn, &err);
        free(fn);
        gtk_window_set_resizable(GTK_WINDOW(toplevel), TRUE);
        g_signal_connect(G_OBJECT(toplevel), "delete_event", G_CALLBACK(check_dirty), NULL);
        g_signal_connect(G_OBJECT(toplevel), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

GtkWidget *wstart()
{
        char    *mf;
        GError *err;
        GtkActionGroup *actions;
        GtkWidget  *vbox;

        actions = gtk_action_group_new("Actions");
        gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
        ui = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(ui, actions, 0);
        gtk_window_add_accel_group(GTK_WINDOW(toplevel), gtk_ui_manager_get_accel_group(ui));
        mf = envprocess(XBTQ_MENU);
        if  (!gtk_ui_manager_add_ui_from_file(ui, mf, &err))  {
                g_message("Menu build failed");
                exit(99);
        }
        free(mf);
        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(toplevel), vbox);
        gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui, "/MenuBar"), FALSE, FALSE, 0);
        init_jlist_win();
        init_vlist_win();
        return  vbox;
}

void  view_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
        gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(ui, (const char *) userdata)), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
}

gboolean  view_clicked(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
        if  (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)  {
                GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
                GtkTreePath *path;
                if  (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL))  {
                        gtk_tree_selection_unselect_all(selection);
                        gtk_tree_selection_select_path(selection, path);
                        gtk_tree_path_free(path);
                        view_popup_menu(treeview, event, userdata);
                        return  TRUE;
                }
        }
        return  FALSE;
}

static void  wcomplete(GtkWidget *vbox)
{
        GtkWidget  *paned, *scroll1, *scroll2;

        scroll1 = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll1), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll1), jwid);
        scroll2 = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll2), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll2), vwid);
        paned = gtk_vpaned_new();
        gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
        gtk_paned_pack1(GTK_PANED(paned), scroll1, TRUE, TRUE);
        gtk_paned_pack2(GTK_PANED(paned), scroll2, TRUE, TRUE);
        g_signal_connect(jwid, "button-press-event", (GCallback) view_clicked, "/jpop");
        g_signal_connect(jwid, "popup-menu", (GCallback) view_popup_menu, "/jpop");
        g_signal_connect(vwid, "button-press-event", (GCallback) view_clicked, "/vpop");
        g_signal_connect(vwid, "popup-menu", (GCallback) view_popup_menu, "/vpop");
        gtk_widget_show_all(toplevel);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        GtkWidget  *vbox;

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
        if  ((LONG) (Daemuid = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)
                Daemuid = ROOTID;

        Cfile = open_cfile("XBTQCONF", "xmbtq.help");

        /* Check we have the right HOME before we run macros or anything */

        gtk_chk_uid();

        tzset();

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

#ifdef  HAVE_SETREUID
        setreuid(Daemuid, Daemuid);
#else
        setuid(Daemuid);
#endif

        gtk_init(&argc, &argv);

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
        load_optfile();
        winit();
        vbox = wstart();
        init_jdisplay();
        init_vdisplay();
        wcomplete(vbox);
        loadmacs('j', jobmacs);
        loadmacs('v', varmacs);
        g_timeout_add(1000, (GSourceFunc) poll_changes, NULL);
        gtk_main();
        return  0;
}
