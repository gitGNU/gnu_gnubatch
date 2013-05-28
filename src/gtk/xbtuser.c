/* xbtuser.c -- main module for gbch-user

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
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "btmode.h"
#include "btuser.h"
#include "xbtu_ext.h"
#include "gtk_lib.h"

char    *Curr_pwd;

int     hchanges,       /* Had changes to default */
        uchanges;       /* Had changes to user(s) */

BtuserRef       ulist;
static  char    *defhdr;

#ifdef  NOTUSED
$P{Read adm full}
$P{Write adm full}
$P{Create entry full}
$P{Special create full}
$P{Stop sched full}
$P{Change default modes full}
$P{Combine user group full}
$P{Combine user other full}
$P{Combine group other full}
$P{Read mode name}
$P{Write mode name}
$P{Reveal mode name}
$P{Display mode name}
$P{Set mode name}
$P{Assume owner mode name}
$P{Assume group mode name}
$P{Give owner mode name}
$P{Give group mode name}
$P{Delete mode name}
$P{Kill mode name}
#endif

struct  privabbrev      privnames[] =
        {{      BTM_RADMIN,     $P{Read adm abbr}                       },
         {      BTM_WADMIN,     $P{Write adm abbr}                      },
         {      BTM_CREATE,     $P{Create entry abbr}                   },
         {      BTM_SPCREATE,   $P{Special create abbr}                 },
         {      BTM_SSTOP,      $P{Stop sched abbr}                     },
         {      BTM_UMASK,      $P{Change default modes abbr}           },
         {      BTM_ORP_UG,     $P{Combine user group abbr}             },
         {      BTM_ORP_UO,     $P{Combine user other abbr}             },
         {      BTM_ORP_GO,     $P{Combine group other abbr}            }};

#define NUM_PRIVS       sizeof(privnames) / sizeof(struct privabbrev)

GtkWidget       *toplevel,      /* Main window */
                *dwid,          /* Default list */
                *uwid;          /* User scroll list */

GtkListStore            *raw_ulist_store;
GtkTreeModelSort        *ulist_store;

static void  cb_about();

static GtkActionEntry entries[] = {
        { "FileMenu", NULL, "_File" },
        { "DefMenu", NULL, "_Defaults" },
        { "UserMenu", NULL, "_Users"  },
        { "HelpMenu", NULL, "_Help" },
        { "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit and save", G_CALLBACK(gtk_main_quit)},
        { "Dpri", NULL, "Default _pri", "<shift>P", "Set default priorities", G_CALLBACK(cb_pris)},
        { "Dloadl", NULL, "Default _loadlevel", "<shift>L", "Set default load levels", G_CALLBACK(cb_loadlev)},
        { "Djmode", NULL, "Default job perms", "<shift>J", "Set default job permissions", G_CALLBACK(cb_jmode)},
        { "Dvmode", NULL, "Default var perms", "<shift>A", "Set default var permissions", G_CALLBACK(cb_vmode)},
        { "Dpriv", NULL, "Default privileges", "<shift>V", "Set default privileges", G_CALLBACK(cb_priv)},
        { "Defcpy", NULL, "Copy all", NULL, "Copy to all users", G_CALLBACK(cb_copyall)},

        { "upri", NULL, "_Priorities", "P", "Set priorities", G_CALLBACK(cb_pris)},
        { "uloadl", NULL, "_Loadlevel", "L", "Set load levels for users", G_CALLBACK(cb_loadlev)},
        { "ujmode", NULL, "_Job permissions", "J", "Set default job permissions for users", G_CALLBACK(cb_jmode)},
        { "uvmode", NULL, "_Var pPermissions", "A", "Set default var permissions for users", G_CALLBACK(cb_vmode)},
        { "upriv", NULL, "Privileges", "V", "Set privileges for users", G_CALLBACK(cb_priv)},
        { "copydef", NULL, "Copy defaults", NULL, "Copy defaults to selected users", G_CALLBACK(cb_copydef)},

        { "About", NULL, "About xbtuser", NULL, "About xbtuser", G_CALLBACK(cb_about)}  };

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
        gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Xi Software Ltd 2008");
        gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), "http://www.xisl.com");
        gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), (const char **) authlist);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
}

/*  Make top level window and set title and icon */

static void  winit()
{
        GError *err;
        char    *fn;

        toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size(GTK_WINDOW(toplevel), 400, 400);
        fn = gprompt($P{xbtuser app title});
        gtk_window_set_title(GTK_WINDOW(toplevel), fn);
        free(fn);
        gtk_container_set_border_width(GTK_CONTAINER(toplevel), 5);
        fn = envprocess(XBTUSER_ICON);
        gtk_window_set_default_icon_from_file(fn, &err);
        free(fn);
        gtk_window_set_resizable(GTK_WINDOW(toplevel), TRUE);
        g_signal_connect(G_OBJECT(toplevel), "delete_event", G_CALLBACK(gtk_false), NULL);
        g_signal_connect(G_OBJECT(toplevel), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

gint  sort_userid(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        guint   id1, id2;
        gtk_tree_model_get(model, a, UID_COL, &id1, -1);
        gtk_tree_model_get(model, b, UID_COL, &id2, -1);
        return  id1 < id2? -1:  id1 == id2? 0: 1;
}

gint  sort_username(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        gchar   *name1, *name2;
        gint    ret = 0;

        gtk_tree_model_get(model, a, USNAM_COL, &name1, -1);
        gtk_tree_model_get(model, b, USNAM_COL, &name2, -1);

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

gint  sort_groupname(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
        gchar   *name1, *name2;
        gint    ret = 0;

        gtk_tree_model_get(model, a, GRPNAM_COL, &name1, -1);
        gtk_tree_model_get(model, b, GRPNAM_COL, &name2, -1);

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

void  set_sort_col(int colnum)
{
        GtkTreeSortable *sortable = GTK_TREE_SORTABLE(ulist_store);
        GtkSortType      order;
        gint             sortid;

        if  (gtk_tree_sortable_get_sort_column_id(sortable, &sortid, &order) == TRUE  &&  sortid == colnum)  {
                GtkSortType  neworder;
                neworder = (order == GTK_SORT_ASCENDING)? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
                gtk_tree_sortable_set_sort_column_id(sortable, sortid, neworder);
        }
        else
                gtk_tree_sortable_set_sort_column_id(sortable, colnum, GTK_SORT_ASCENDING);
}

GtkWidget *wstart()
{
        char    *mf;
        GError *err;
        GtkActionGroup *actions;
        GtkUIManager *ui;
        GtkTreeSelection    *sel;
        GtkWidget  *vbox;
        int     cnt;

        actions = gtk_action_group_new("Actions");
        gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
        ui = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(ui, actions, 0);
        gtk_window_add_accel_group(GTK_WINDOW(toplevel), gtk_ui_manager_get_accel_group(ui));
        mf = envprocess(XBTUSER_MENU);
        if  (!gtk_ui_manager_add_ui_from_file(ui, mf, &err))  {
                g_message("Menu build failed");
                exit(E_SETUP);
        }

        free(mf);
        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(toplevel), vbox);
        gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui, "/MenuBar"), FALSE, FALSE, 0);

        /* Create user display treeview */

        uwid = gtk_tree_view_new();
        for  (cnt = 0;  cnt <= P_COL-USNAM_COL;  cnt++)  {
                GtkCellRenderer     *renderer = gtk_cell_renderer_text_new();
                char    *msg = gprompt($P{xbtuser user hdr}+cnt);
                gtk_tree_view_column_set_resizable(
                        gtk_tree_view_get_column(GTK_TREE_VIEW(uwid),
                                                 gtk_tree_view_insert_column_with_attributes(
                                                        GTK_TREE_VIEW(uwid), -1, msg, renderer, "text", cnt+USNAM_COL, NULL) - 1),
                        TRUE);
                free(msg);
        }

        raw_ulist_store = gtk_list_store_new(12,
                                             G_TYPE_UINT, /* Index number we don't display */
                                             G_TYPE_UINT, /* User id which we don't display */
                                             G_TYPE_UINT, /* Group id which we don't display */
                                             G_TYPE_STRING, /* User name */
                                             G_TYPE_STRING, /* Group name */
                                             G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, /* Def/min/max pri */
                                             G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, /* Load levs */
                                             G_TYPE_STRING);

        ulist_store = (GtkTreeModelSort *) gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(raw_ulist_store));

        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ulist_store), USNAM_COL, sort_username, NULL, NULL);
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ulist_store), GRPNAM_COL, sort_groupname, NULL, NULL);
        for  (cnt = DEFP_COL;  cnt <= P_COL;  cnt++)
                gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(ulist_store), cnt, sort_userid, NULL, NULL);

        /* set initial sort order */

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ulist_store), DEFP_COL, GTK_SORT_ASCENDING);
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(uwid), TRUE);

        for  (cnt = 0;  cnt < 9;  cnt++)  {
                GtkTreeViewColumn   *col = gtk_tree_view_get_column(GTK_TREE_VIEW(uwid), cnt);
                g_signal_connect_swapped(G_OBJECT(col), "clicked", G_CALLBACK(set_sort_col), GINT_TO_POINTER(cnt+USNAM_COL));
        }

        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(uwid));
        gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

        /* Bit for display of default */

        dwid = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(dwid), FALSE);

        defhdr = gprompt($P{xbtuser default string});

        /* Fetch privilege names from file */

        for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)  {
                privnames[cnt].priv_abbrev = gprompt(privnames[cnt].priv_mcode);
                privnames[cnt].priv_name = gprompt(privnames[cnt].priv_mcode + $P{Read adm full} - $P{Read adm abbr});
        }
        return  vbox;
}

static void  wcomplete(GtkWidget *vbox)
{
        GtkWidget  *paned, *scroll;

        gtk_tree_view_set_model(GTK_TREE_VIEW(uwid), GTK_TREE_MODEL(ulist_store));
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), uwid);
        paned = gtk_vpaned_new();
        gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
        gtk_paned_pack1(GTK_PANED(paned), scroll, TRUE, TRUE);
        gtk_paned_pack2(GTK_PANED(paned), dwid, FALSE, TRUE);
        gtk_widget_show_all(toplevel);
}

void  upd_udisp(BtuserRef uitem, GtkTreeIter *iter)
{
        GString  *privstr = g_string_new(NULL);

        if  (uitem->btu_priv)  {
                int  cnt;
                for  (cnt = 0;  cnt < NUM_PRIVS;  cnt++)
                        if  (uitem->btu_priv & privnames[cnt].priv_flag)  {
                                if  (privstr->len != 0)
                                        g_string_append_c(privstr, '|');
                                g_string_append(privstr, privnames[cnt].priv_abbrev);
                        }
        }

        gtk_list_store_set(raw_ulist_store, iter,
                           DEFP_COL,    (guint) uitem->btu_defp,
                           MINP_COL,  (guint) uitem->btu_minp,
                           MAXP_COL,  (guint) uitem->btu_maxp,
                           MAXLL_COL,  (guint) uitem->btu_maxll,
                           TOTLL_COL,  (guint) uitem->btu_totll,
                           SPECLL_COL,  (guint) uitem->btu_spec_ll,
                           P_COL,       privstr->str,
                           -1);
        g_string_free(privstr, TRUE);
}

void  update_all_users()
{
        GtkTreeIter  iter;
        gboolean  valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(raw_ulist_store), &iter);

        while  (valid)  {
                unsigned  indx;
                gtk_tree_model_get(GTK_TREE_MODEL(raw_ulist_store), &iter, INDEX_COL, &indx, -1);
                upd_udisp(&ulist[indx], &iter);
                valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(raw_ulist_store), &iter);
        }
}

void  update_selected_users()
{
        GtkTreeSelection *selection;
        GList  *pu, *nxt;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(uwid));
        pu = gtk_tree_selection_get_selected_rows(selection, NULL);
        for  (nxt = g_list_first(pu);  nxt;  nxt = g_list_next(nxt))  {
                GtkTreeIter  iter, citer;
                unsigned  indx;
                if  (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ulist_store), &iter, (GtkTreePath *)(nxt->data)))
                        continue;
                gtk_tree_model_get(GTK_TREE_MODEL(ulist_store), &iter, INDEX_COL, &indx, -1);
                gtk_tree_model_sort_convert_iter_to_child_iter(ulist_store, &citer, &iter);
                upd_udisp(&ulist[indx], &citer);
        }
        g_list_foreach(pu, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(pu);
}

void  udisplay()
{
        unsigned  ucnt;
        GtkTreeIter   iter;

        for  (ucnt = 0;  ucnt < Npwusers;  ucnt++)  {

                /* Add reow to store.
                   Put in index and uid which udisp doesn't do */

                gtk_list_store_append(raw_ulist_store, &iter);
                gtk_list_store_set(raw_ulist_store, &iter,
                                   INDEX_COL, (guint) ucnt,
                                   UID_COL, (guint) ulist[ucnt].btu_user,
                                   USNAM_COL, prin_uname((uid_t) ulist[ucnt].btu_user),
                                   -1);
                gtk_list_store_set(raw_ulist_store, &iter,
                                   GID_COL, (guint) lastgid,
                                   GRPNAM_COL, prin_gname((gid_t) lastgid),
                                   -1);
                upd_udisp(&ulist[ucnt], &iter);
        }
}

void  defdisplay()
{
        GString  *defstr = g_string_new(NULL);
        g_string_printf(defstr, "%s Def pri %d min %d max %d\nMax ll %u Tot ll %u Specll %u",
                 defhdr, Btuhdr.btd_defp, Btuhdr.btd_minp, Btuhdr.btd_maxp, Btuhdr.btd_maxll, Btuhdr.btd_totll, Btuhdr.btd_spec_ll);
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(dwid)), defstr->str, -1);
        g_string_free(defstr, TRUE);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        BtuserRef       mypriv;
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
        Cfile = open_cfile("XBTUSERCONF", "xmbtuser.help");

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

#ifdef  HAVE_SETREUID
        setreuid(Daemuid, Daemuid);
#else
        setuid(Daemuid);
#endif
        gtk_init(&argc, &argv);

        winit();

        mypriv = getbtuentry(Realuid);

        if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                doerror($EH{No write admin file priv});
                exit(E_NOPRIV);
        }
        ulist = getbtulist();
        vbox = wstart();
        defdisplay();
        udisplay();
        wcomplete(vbox);
        gtk_main();
        if  (uchanges || hchanges)
                putbtulist(ulist);
        return  0;
}
