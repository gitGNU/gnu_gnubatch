/* xbtr.c -- main module for gbch-xr

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
#include "files.h"
#include "incl_ugid.h"
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
#include "statenums.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "cfile.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xbr_ext.h"
#include "gtk_lib.h"
#include "xmlldsv.h"

#define IPC_MODE        0

#define DEFAULT_WIDTH   400
#define DEFAULT_HEIGHT  400

gint    Mwwidth = DEFAULT_WIDTH, Mwheight = DEFAULT_HEIGHT;

void  initcifile();

char    *spdir,
        *Curr_pwd;

extern  long    mymtype;

struct  pend_job        default_pend;
Btjob                   default_job;

char    internal_edit = 1;          /* Use internal editor */
char    xterm_edit = 1;             /* Invoke "xterm" to run editor */
#ifdef	HAVE_LIBXML2
char	xml_format = 1;              /* Use XML format on by default */
#else
char    xml_format = 0;             /* Use XML format */
#endif
char    *editor_name;               /* Name of favourite editor */
char    *realuname;

/* X Stuff */

GtkWidget       *toplevel,      /* Main window */
                *jwid;          /* Job scroll list */
GtkListStore            *raw_jlist_store;
GtkTreeModelSort        *sorted_jlist_store;

GtkUIManager    *ui;

int             Dirty;          /* Unsaved changes */

static void  cb_about();
static void  cb_quit();
extern void  cb_viewopt();
extern void  loadopts();
extern void  cb_saveopts();
extern void  cb_direct();
extern void  cb_loaddefs(GtkAction *);
extern void  cb_savedefs(GtkAction *);
extern void  cb_jqueue(GtkAction *);
extern void  cb_jstate(GtkAction *);
extern void  cb_time(GtkAction *);
extern void  cb_titprill(GtkAction *);
extern void  cb_procpar(GtkAction *);
extern void  cb_timelim(GtkAction *);
extern void  cb_mailwrt(GtkAction *);
extern void  cb_jperm(GtkAction *);
extern void  cb_args(GtkAction *);
extern void  cb_env(GtkAction *);
extern void  cb_redir(GtkAction *);
extern void  cb_conds(GtkAction *);
extern void  cb_asses(GtkAction *);
extern void  cb_jnew();
extern void  cb_jopen();
extern void  cb_jlegopen();
extern void  cb_jclosedel(GtkAction *);
extern void  cb_jsave();
extern  void  cb_jsaveas();
extern void  cb_edit();
extern void  cb_submit();
extern void  cb_remsubmit();
extern  void  cb_options();

extern void  initmoremsgs();

static GtkActionEntry entries[] = {
        { "OptMenu", NULL, "_Options"  },
        { "DefsMenu", NULL, "_Defaults"  },
        { "FileMenu", NULL, "_File"  },
        { "JobMenu", NULL, "_Job"  },
        { "HelpMenu", NULL, "_Help"  },
        { "Viewopts", GTK_STOCK_PREFERENCES, "_View Options", "equal", "Specify program options", G_CALLBACK(cb_viewopt) },
        { "Saveopts", GTK_STOCK_SAVE, "_Save Options", "dollar", "Save program options", G_CALLBACK(cb_saveopts) },
        { "Selectdir", GTK_STOCK_DIRECTORY, "Select new _directory", NULL, "Select new working directory", G_CALLBACK(cb_direct) },
        { "Loaddefsc", NULL, "_Load defaults current", NULL, "Load defaults from current directory", G_CALLBACK(cb_loaddefs) },
        { "Savedefsc", NULL, "Save defaults _current", NULL, "Save defaults to current directory", G_CALLBACK(cb_savedefs) },
        { "Loaddefsh", NULL, "Load defaults _home", NULL, "Load defaults from home directory", G_CALLBACK(cb_loaddefs) },
        { "Savedefsh", NULL, "Sa_ve defaults home", NULL, "Save defaults to home directory", G_CALLBACK(cb_savedefs) },
        { "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit program", G_CALLBACK(cb_quit)},
        { "Queued", NULL, "Set default _queue", NULL, "Set default queue name prefix", G_CALLBACK(cb_jqueue) },
        { "Setrund", NULL, "Set default _runnable", NULL, "Set runnable by default", G_CALLBACK(cb_jstate) },
        { "Setcancd", NULL, "Set default _cancelled", NULL, "Set cancelled by default", G_CALLBACK(cb_jstate) },
        { "Timed", NULL, "Set default _time", NULL, "Set default time parameters", G_CALLBACK(cb_time) },
        { "Titled", NULL, "Set default title,pri,_ll", NULL, "Set default job title, priority, load level", G_CALLBACK(cb_titprill) },
        { "Processd", NULL, "Set default _process params", NULL, "Set default process parameters", G_CALLBACK(cb_procpar) },
        { "Runtimed", NULL, "Set default _run times", NULL, "Set default run time", G_CALLBACK(cb_timelim) },
        { "Maild", NULL, "Set default _mail/write", NULL, "Set default mail/write flags", G_CALLBACK(cb_mailwrt) },
        { "Permjd", NULL, "Set default _permissions", NULL, "Set default permissions", G_CALLBACK(cb_jperm) },
        { "Argsd", NULL, "Set default _arguments", NULL, "Set default job arguments", G_CALLBACK(cb_args) },
        { "Envd", NULL, "Set default _environment", NULL, "Set default environment", G_CALLBACK(cb_env) },
        { "Redirsd", NULL, "Set default _redirections", NULL, "Set default redirections", G_CALLBACK(cb_redir) },
        { "Condsd", NULL, "Set default _conditions", NULL, "Set default conditions", G_CALLBACK(cb_conds) },
        { "Assesd", NULL, "Set default _assignments", NULL, "Set default assignments", G_CALLBACK(cb_asses) },
        { "New", GTK_STOCK_NEW, "New job file", "<control>N", "Create new job file", G_CALLBACK(cb_jnew) },
        { "Open", GTK_STOCK_OPEN, "Open job file", "<control>O", "Open job file", G_CALLBACK(cb_jopen) },
        { "Legopen", NULL, "_Legacy open job file", NULL, "Open legacy job file", G_CALLBACK(cb_jlegopen), },
        { "Close", GTK_STOCK_CLOSE, "Close job file", NULL, "Close job file", G_CALLBACK(cb_jclosedel) },
        { "Save", GTK_STOCK_SAVE, "Save job file", "<control>S", "Save job file", G_CALLBACK(cb_jsave) },
        { "Saveas", GTK_STOCK_SAVE_AS, "Save job file as", "<ctrl><shift>S", "Save job to named file", G_CALLBACK(cb_jsaveas), },
        { "Edit", GTK_STOCK_EDIT, "Edit script", "E", "Edit job script", G_CALLBACK(cb_edit) },
        { "Delete", GTK_STOCK_DELETE, "Delete job file", "Delete", "Close and delete job file", G_CALLBACK(cb_jclosedel) },
        { "Submit", GTK_STOCK_EXECUTE, "_Submit", "exclam", "Submit job", G_CALLBACK(cb_submit) },
        { "Rsubmit", GTK_STOCK_EXECUTE, "_Remote Submit", "at", "Submit job remotely", G_CALLBACK(cb_remsubmit) },
        { "Options", NULL, "_Options", "<control>O", "Set job options", G_CALLBACK(cb_options) },
        { "Queue", NULL, "Set job _queue", NULL, "Set queue name prefix", G_CALLBACK(cb_jqueue) },
        { "Setrun", NULL, "Set _runnable", NULL, "Set runnable", G_CALLBACK(cb_jstate) },
        { "Setcanc", NULL, "Set _cancelled", NULL, "Set cancelled", G_CALLBACK(cb_jstate) },
        { "Time", NULL, "Set _time", NULL, "Set time parameters", G_CALLBACK(cb_time) },
        { "Title", NULL, "Set title,pri,_ll", NULL, "Set job title, priority, load level", G_CALLBACK(cb_titprill) },
        { "Process", NULL, "Set _process params", NULL, "Set process parameters", G_CALLBACK(cb_procpar) },
        { "Runtime", NULL, "Set _run times", NULL, "Set run time", G_CALLBACK(cb_timelim) },
        { "Mail", NULL, "Set _mail/write", NULL, "Set mail/write flags", G_CALLBACK(cb_mailwrt) },
        { "Permj", NULL, "Set _permissions", NULL, "Set permissions", G_CALLBACK(cb_jperm) },
        { "Args", NULL, "Set _arguments", NULL, "Set job arguments", G_CALLBACK(cb_args) },
        { "Env", NULL, "Set _environment", NULL, "Set environment", G_CALLBACK(cb_env) },
        { "Redirs", NULL, "Set _redirections", NULL, "Set redirections", G_CALLBACK(cb_redir) },
        { "Conds", NULL, "Set _conditions", NULL, "Set conditions", G_CALLBACK(cb_conds) },
        { "Asses", NULL, "Set _assignments", NULL, "Set assignments", G_CALLBACK(cb_asses) },
        { "About", NULL, "About xbtr", NULL, "About xbtr", G_CALLBACK(cb_about)}  };

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

gboolean  check_dirty()
{
        save_state();
        if  (Dirty  &&  !Confirm($PH{xbtq changes not saved ok}))
                return  TRUE;
        if  (jlist_dirty() && !Confirm($PH{xbtr job list changes}))
                return  TRUE;
        return  FALSE;
}

static void  cb_quit()
{
        if  (check_dirty())
                return;
        gtk_main_quit();
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

#define SORTBY_JSEQ     1
#define SORTBY_TITLE    2
#define SORTBY_JOBFILE  3
#define SORTBY_DIRECT   4

static gint  sort_uint(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        guint   seq1, seq2;
        gint    colnum = GPOINTER_TO_INT(userdata);
        gtk_tree_model_get(model, a, colnum, &seq1, -1);
        gtk_tree_model_get(model, b, colnum, &seq2, -1);
        return  seq1 < seq2? -1:  seq1 == seq2? 0: 1;
}

static gint  sort_string(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        gchar   *name1, *name2;
        gint    ret = 0;
        gint    colnum = GPOINTER_TO_INT(userdata);

        gtk_tree_model_get(model, a, colnum, &name1, -1);
        gtk_tree_model_get(model, b, colnum, &name2, -1);

        if  (!name1  ||  !name2)  {
                if  (!name1  &&  !name2)
                        return  0;
                if  (!name1)  {
                        g_free(name2);
                        return  -1;
                }
                else  {
                        g_free(name1);
                        return  1;
                }
        }

        ret = g_utf8_collate(name1, name2);
        g_free(name1);
        g_free(name2);
        return  ret;
}

static void  winit()
{
        GError *err;
        char    *fn;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  za;
#endif

#ifdef  STRUCT_SIG
        za.sighandler_el = SIG_IGN;
        sigmask_clear(za);
        za.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGPIPE, &za, (struct sigstruct_name *) 0);
#else
        signal(SIGPIPE, SIG_IGN);
#endif

        toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size(GTK_WINDOW(toplevel), Mwwidth, Mwheight);
        fn = gprompt($P{xbtr app title});
        gtk_window_set_title(GTK_WINDOW(toplevel), fn);
        free(fn);
        gtk_container_set_border_width(GTK_CONTAINER(toplevel), 5);
        fn = envprocess(XBTR_ICON);
        gtk_window_set_default_icon_from_file(fn, &err);
        free(fn);
        gtk_window_set_resizable(GTK_WINDOW(toplevel), TRUE);
        g_signal_connect(G_OBJECT(toplevel), "delete_event", G_CALLBACK(check_dirty), NULL);
        g_signal_connect(G_OBJECT(toplevel), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

static  char    *titles[] = { "State", "Title", "Jobfile", "Directory"  };
static  int     sortbys[] = { SORTBY_JSEQ, SORTBY_TITLE, SORTBY_JOBFILE, SORTBY_DIRECT  };

static void  wstart()
{
        char    *mf;
        GError  *err;
        GtkActionGroup  *actions;
        GtkWidget       *vbox, *scroll;
        GtkCellRenderer     *renderer;
        int     cnt;

        actions = gtk_action_group_new("Actions");
        gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
        ui = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(ui, actions, 0);
        gtk_window_add_accel_group(GTK_WINDOW(toplevel), gtk_ui_manager_get_accel_group(ui));
        mf = envprocess(XBTR_MENU);
        if  (!gtk_ui_manager_add_ui_from_file(ui, mf, &err))  {
                g_message("Menu build failed");
                exit(E_SETUP);
        }
        free(mf);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(toplevel), vbox);
        gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui, "/MenuBar"), FALSE, FALSE, 0);

        raw_jlist_store = gtk_list_store_new(6,
                                             G_TYPE_UINT,               /* Index number we don't display */
                                             G_TYPE_STRING,             /* Progress */
                                             G_TYPE_STRING,             /* Job title */
                                             G_TYPE_STRING,             /* Job File */
                                             G_TYPE_STRING,             /* Directory */
                                             G_TYPE_BOOLEAN);           /* Unsaved marker */
        sorted_jlist_store = (GtkTreeModelSort *) gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(raw_jlist_store));
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_jlist_store), SORTBY_JSEQ, sort_uint, GINT_TO_POINTER(JLIST_SEQ_COL), NULL);
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_jlist_store), SORTBY_JSEQ, sort_uint, GINT_TO_POINTER(JLIST_PROGRESS_COL), NULL);
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_jlist_store), SORTBY_TITLE, sort_string, GINT_TO_POINTER(JLIST_TITLE_COL), NULL);
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_jlist_store), SORTBY_JOBFILE, sort_string, GINT_TO_POINTER(JLIST_JOBFILE_COL), NULL);
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sorted_jlist_store), SORTBY_DIRECT, sort_string, GINT_TO_POINTER(JLIST_DIRECT_COL), NULL);

        /* Set initial sort - TODO read from config */

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sorted_jlist_store), SORTBY_JSEQ, GTK_SORT_ASCENDING);

        /* Create job file display treeview */

        jwid = gtk_tree_view_new();
        gtk_tree_view_set_model(GTK_TREE_VIEW(jwid), GTK_TREE_MODEL(sorted_jlist_store));

        for  (cnt = 0;  cnt < G_N_ELEMENTS(titles);  cnt++)  {
                renderer = gtk_cell_renderer_text_new();
                GtkTreeViewColumn  *col;
                gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(jwid), -1, titles[cnt], renderer, "text", cnt+1, NULL);
                col = gtk_tree_view_get_column(GTK_TREE_VIEW(jwid), cnt);
                gtk_tree_view_column_set_resizable(col, TRUE);
                gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
                gtk_tree_view_column_set_sort_column_id(col, sortbys[cnt]);
        }
        renderer = gtk_cell_renderer_toggle_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(jwid), 0, "N/S", renderer, "active", JLIST_UNSAVED_COL, NULL);
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(jwid), TRUE);

        g_signal_connect(jwid, "button-press-event", (GCallback) view_clicked, "/jpop");
        g_signal_connect(jwid, "popup-menu", (GCallback) view_popup_menu, "/jpop");

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), jwid);
        gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
        gtk_widget_show_all(toplevel);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        init_xenv();
        init_xml();
        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        if  ((LONG) (Daemuid = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)
                Daemuid = ROOTID;
        realuname = prin_uname(Realuid);

        Cfile = open_cfile("XBTRCONF", "xmbtr.help");
        gtk_chk_uid();
        tzset();

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd)  {
                if  ((Curr_pwd = getenv("PWD")))
                        Curr_pwd = stracpy(Curr_pwd);
                else
                        Curr_pwd = runpwd();
        }

        spdir = envprocess(SPDIR);
        initmoremsgs();

#ifdef  HAVE_SETREUID
        setreuid(Daemuid, Daemuid);
#else
        setuid(Daemuid);
#endif

        gtk_init(&argc, &argv);

#ifdef  DO_CHDIR
        if  (chdir(spdir) < 0)  {
                disp_str = spdir;
                print_error($E{Cannot change directory});
                exit(E_NOCHDIR);
        }
#endif

        initcifile();
        mypriv = getbtuser(Realuid);
        init_defaults();

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }
        mymtype = MTOFFSET + getpid();

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

        openjfile(0, 0);
        openvfile(0, 0);
        initxbuffer(0);
        loadopts();             /* Program options */
        winit();
        wstart();
        load_options();         /* Defaults for jobs */
        gtk_main();
        return  0;              /* Shut up compilers */
}
