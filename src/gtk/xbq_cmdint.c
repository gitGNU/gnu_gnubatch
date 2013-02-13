/* xbq_cmdint.c -- command interpreter handling for gbch-xq

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <gtk/gtk.h>
#include <errno.h>
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
#include "xbq_ext.h"
#include "gtk_lib.h"

static  char    Filename[] = __FILE__;

#define CILIST_NAME_COL         0
#define CILIST_PATH_COL         1
#define CILIST_PREDEF_COL       2
#define CILIST_NICE_COL         3
#define CILIST_LL_COL           4
#define CILIST_SETA0_COL        5
#define CILIST_INTERP_COL       6
#define CILIST_INDEX_COL        7

#define DEFAULT_CIDLG_WIDTH     100
#define DEFAULT_CIDLG_HEIGHT    300

struct  cmdint_data  {
        GtkWidget       *iwid;
        GtkListStore    *ilist_store;
};

void  initcifile()
{
        int     ret;

        if  ((ret = open_ci(O_RDWR)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
}

/* Check new name is valid and doesn't clash with an existing name */

static int  val_ci_name(const gchar *name, struct cmdint_data *cdata)
{
        const  gchar  *cp = name;
        gboolean valid;
        GtkTreeIter     iter;

        if  (strlen(name) > CI_MAXNAME  ||  (!isalpha(*cp)  &&  *cp != '_'))  {
                doerror($EH{xmbtq inval ci name});
                return  0;
        }

        for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(cdata->ilist_store), &iter);
              valid;
              valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(cdata->ilist_store), &iter))  {
                gchar  *nam;
                int     cmp;
                gtk_tree_model_get(GTK_TREE_MODEL(cdata->ilist_store), &iter, CILIST_NAME_COL, &nam, -1);
                cmp = strcmp(nam, name);
                g_free(nam);
                if  (cmp == 0)  {
                        doerror($EH{xmbtq ci name clash});
                        return  0;
                }
        }

        return  1;
}

static int  val_ci_path(const gchar *name, const gchar *path)
{
        struct  stat  sbuf;

        disp_str = (char *) name;
        disp_str2 = (char *) path;

        if  (path[0] != '/')  {
                doerror($EH{xmbtq ci path not abs});
                return  0;
        }
        if  (strlen(path) > CI_MAXFPATH)  {
                doerror($EH{xmbtq path name too long});
                return  0;
        }

        if  ((stat(path, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFREG  ||  (sbuf.st_mode & 0111) != 0111)  &&
             !Confirm($PH{xmbtq suspicious path name}))
                return  0;

        return  1;
}

static int  val_ci_args(const gchar *args)
{
        if  (strlen(args) > CI_MAXARGS)  {
                doerror($EH{xmbtq args too long});
                return  0;
        }
        return  1;
}


static void  add_interp(struct cmdint_data *cdata)
{
        GtkWidget       *dlg, *hbox, *namew, *pathw, *argsw, *nicew, *llw, *sa0w, *interpw;
        GtkAdjustment   *adj;
        char            *pr;

        pr = gprompt($P{xbtq add ci dlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        /* First row is name */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq add ci name}), FALSE, FALSE, DEF_DLG_HPAD);
        namew = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), namew, FALSE, FALSE, DEF_DLG_HPAD);

        /* Second row is path */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq add ci path}), FALSE, FALSE, DEF_DLG_HPAD);
        pathw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), pathw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Third row is args */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq add ci args}), FALSE, FALSE, DEF_DLG_HPAD);
        argsw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), argsw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(argsw), "-s");

        /* Fourth row is nice / ll */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq add ci nice}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) DEF_CI_NICE, 0, 39, 1.0, 1.0, 0.0);
        nicew = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), nicew, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq add ci ll}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) mypriv->btu_spec_ll, 1, 65535, 1.0, 1.0, 0.0);
        llw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), llw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Now toggles for set arg 0 and interp args */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        sa0w = gprompt_checkbutton($P{xbtq add ci set arg0});
        gtk_box_pack_start(GTK_BOX(hbox), sa0w, FALSE, FALSE, DEF_DLG_HPAD);
        interpw = gprompt_checkbutton($P{xbtq add ci interparg});
        gtk_box_pack_start(GTK_BOX(hbox), interpw, FALSE, FALSE, DEF_DLG_HPAD);

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const   gchar   *newname = gtk_entry_get_text(GTK_ENTRY(namew));
                const   gchar   *newpath = gtk_entry_get_text(GTK_ENTRY(pathw));
                const   gchar   *newargs = gtk_entry_get_text(GTK_ENTRY(argsw));
                gint    newnice = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(nicew));
                gint    newll = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(llw));
                gboolean  newsa0 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sa0w));
                gboolean  newinterp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(interpw));
                GtkTreeIter  iter;

                if  (!val_ci_name(newname, cdata))
                        continue;
                if  (!val_ci_path(newname, newpath))
                        continue;
                if  (!val_ci_args(newargs))
                        continue;

                gtk_list_store_append(cdata->ilist_store, &iter);

                gtk_list_store_set(cdata->ilist_store, &iter,
                                   CILIST_NAME_COL,     newname,
                                   CILIST_PATH_COL,     newpath,
                                   CILIST_PREDEF_COL,   newargs,
                                   CILIST_NICE_COL,     newnice,
                                   CILIST_LL_COL,       newll,
                                   CILIST_SETA0_COL,    newsa0,
                                   CILIST_INTERP_COL,   newinterp,
                                   CILIST_INDEX_COL,    -1, /* Indicating new */
                                   -1);
                break;
        }
        gtk_widget_destroy(dlg);
}

static void  editci(GtkTreeIter *iter, struct cmdint_data *cdata)
{
        GtkWidget       *dlg, *hbox, *namew, *pathw, *argsw, *nicew, *llw, *sa0w, *interpw;
        GtkAdjustment   *adj;
        char            *pr;
        gchar           *cname, *cpath, *cargs;
        gint            cnice, cll;
        gboolean        csa0, cinterp;

        pr = gprompt($P{xbtq edit ci dlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        gtk_tree_model_get(GTK_TREE_MODEL(cdata->ilist_store), iter,
                           CILIST_NAME_COL,     &cname,
                           CILIST_PATH_COL,     &cpath,
                           CILIST_PREDEF_COL,   &cargs,
                           CILIST_NICE_COL,     &cnice,
                           CILIST_LL_COL,       &cll,
                           CILIST_SETA0_COL,    &csa0,
                           CILIST_INTERP_COL,   &cinterp,
                           -1);

        /* First row is name */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq edit ci name}), FALSE, FALSE, DEF_DLG_HPAD);
        namew = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), namew, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(namew), cname);

        /* Second row is path */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq edit ci path}), FALSE, FALSE, DEF_DLG_HPAD);
        pathw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), pathw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(pathw), cpath);

        /* Third row is args */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq edit ci args}), FALSE, FALSE, DEF_DLG_HPAD);
        argsw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), argsw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(argsw), cargs);

        /* Fourth row is nice / ll */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq edit ci nice}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) cnice, 0, 39, 1.0, 1.0, 0.0);
        nicew = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), nicew, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq edit ci ll}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new((gdouble) cll, 1, 65535, 1.0, 1.0, 0.0);
        llw = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), llw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Now toggles for set arg 0 and interp args */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        sa0w = gprompt_checkbutton($P{xbtq edit ci set arg0});
        gtk_box_pack_start(GTK_BOX(hbox), sa0w, FALSE, FALSE, DEF_DLG_HPAD);
        if  (csa0)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sa0w), TRUE);
        interpw = gprompt_checkbutton($P{xbtq edit ci interparg});
        gtk_box_pack_start(GTK_BOX(hbox), interpw, FALSE, FALSE, DEF_DLG_HPAD);
        if  (cinterp)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(interpw), TRUE);

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const   gchar   *newname = gtk_entry_get_text(GTK_ENTRY(namew));
                const   gchar   *newpath = gtk_entry_get_text(GTK_ENTRY(pathw));
                const   gchar   *newargs = gtk_entry_get_text(GTK_ENTRY(argsw));
                gint    newnice = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(nicew));
                gint    newll = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(llw));
                gboolean  newsa0 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sa0w));
                gboolean  newinterp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(interpw));

                if  (strcmp(newname, cname) != 0  &&  !val_ci_name(newname, cdata))
                        continue;
                if  (!val_ci_path(newname, newpath))
                        continue;
                if  (!val_ci_args(newargs))
                        continue;

                gtk_list_store_set(cdata->ilist_store,  iter,
                                   CILIST_NAME_COL,     newname,
                                   CILIST_PATH_COL,     newpath,
                                   CILIST_PREDEF_COL,   newargs,
                                   CILIST_NICE_COL,     newnice,
                                   CILIST_LL_COL,       newll,
                                   CILIST_SETA0_COL,    newsa0,
                                   CILIST_INTERP_COL,   newinterp,
                                   -1);
                break;
        }
        g_free(cname);
        g_free(cpath);
        g_free(cargs);
        gtk_widget_destroy(dlg);
}

static void  ilist_dblclk(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn  *col, struct cmdint_data *cdata)
{
        GtkTreeIter     iter;

        if  (gtk_tree_model_get_iter(GTK_TREE_MODEL(cdata->ilist_store), &iter, path))
                editci(&iter, cdata);
}

static void  upd_interp(struct cmdint_data *cdata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cdata->iwid));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))
                editci(&iter, cdata);
}

static void  del_interp(struct cmdint_data *cdata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(cdata->iwid));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                gint    ind;
                gtk_tree_model_get(GTK_TREE_MODEL(cdata->ilist_store), &iter, CILIST_INDEX_COL, &ind, -1);
                if  (ind == CI_STDSHELL)  {
                        doerror($EH{xmbtq deleting std ci});
                        return;
                }
                if  (!chk_okcidel(ind))  {
                        doerror($EH{xmbtq ci in use});
                        return;
                }
                if  (!Confirm($PH{xmbtq confirm delete ci}))
                        return;
                gtk_list_store_remove(cdata->ilist_store, &iter);
        }
}

void  cb_interps()
{
        GtkWidget  *dlg, *scroll;
        GtkCellRenderer  *rend;
        GtkTreeViewColumn *col;
        unsigned  readonly = !(mypriv->btu_priv & BTM_SPCREATE);
        char    *pr;
        int     cnt;
        struct  cmdint_data     cdata;

        rereadcif();

        pr = gprompt($P{xbtq cmd interps dlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          readonly? GTK_RESPONSE_CANCEL: GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);
        gtk_window_set_default_size(GTK_WINDOW(dlg), DEFAULT_CIDLG_WIDTH, DEFAULT_CIDLG_HEIGHT);

        if  (!readonly)  {
                GtkWidget  *hbox, *butt;

                hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
                gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
                gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD, GTK_PACK_START);
                butt = gprompt_button($P{xbtq add interp});
                gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
                g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(add_interp), (gpointer) &cdata);
                butt = gprompt_button($P{xbtq del interp});
                gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
                g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(del_interp), (gpointer) &cdata);
                butt = gprompt_button($P{xbtq upd interp});
                gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
                g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(upd_interp), (gpointer) &cdata);
        }

        cdata.ilist_store = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT);

        for  (cnt = 0;  cnt < Ci_num;  cnt++)  {
                GtkTreeIter  iter;
                CmdintRef    ci = &Ci_list[cnt];
                if  (!ci->ci_name[0])
                        continue;

                gtk_list_store_append(cdata.ilist_store, &iter);
                gtk_list_store_set(cdata.ilist_store, &iter,
                                   CILIST_NAME_COL,     ci->ci_name,
                                   CILIST_PATH_COL,     ci->ci_path,
                                   CILIST_PREDEF_COL,   ci->ci_args,
                                   CILIST_NICE_COL,     ci->ci_nice,
                                   CILIST_LL_COL,       ci->ci_ll,
                                   CILIST_SETA0_COL,    ci->ci_flags & CIF_SETARG0,
                                   CILIST_INTERP_COL,   ci->ci_flags & CIF_INTERPARGS,
                                   CILIST_INDEX_COL,    cnt,
                                   -1);
        }

        cdata.iwid = gtk_tree_view_new();
        gtk_tree_view_set_model(GTK_TREE_VIEW(cdata.iwid), GTK_TREE_MODEL(cdata.ilist_store));

        /* First column - name */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col, "Name");
        gtk_tree_view_column_set_resizable(col, TRUE);
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", CILIST_NAME_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(cdata.iwid), col);

        /* Second column - path */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col, "Path");
        gtk_tree_view_column_set_resizable(col, TRUE);
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", CILIST_PATH_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(cdata.iwid), col);

        /* Third column - predef args */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col, "Predef");
        gtk_tree_view_column_set_resizable(col, TRUE);
        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", CILIST_PREDEF_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(cdata.iwid), col);

        /* Fourth column - nice value */

        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cdata.iwid), -1, "Nice", rend, "text", CILIST_NICE_COL, NULL);
        gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(cdata.iwid), CILIST_NICE_COL), TRUE);

        /* Fifth column - ll value */

        rend = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cdata.iwid), -1, "LL", rend, "text", CILIST_LL_COL, NULL);
        gtk_tree_view_column_set_resizable(gtk_tree_view_get_column(GTK_TREE_VIEW(cdata.iwid), CILIST_LL_COL), TRUE);

        /* Sixth column - set arg 0 toggle */

        rend = gtk_cell_renderer_toggle_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cdata.iwid), -1, "Set A0", rend, "active", CILIST_SETA0_COL, NULL);

        /* Seventh column - expand args */

        rend = gtk_cell_renderer_toggle_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(cdata.iwid), -1, "Interp", rend, "active", CILIST_INTERP_COL, NULL);

        /* Double-click to invoke edit */

        g_signal_connect(G_OBJECT(cdata.iwid), "row-activated", (GCallback) ilist_dblclk, (gpointer) &cdata);

        /* Now scroll the stuff */

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), cdata.iwid);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, DEF_DLG_VPAD, GTK_PACK_START);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                int     ninterps = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(cdata.ilist_store), NULL);
                gboolean valid;
                GtkTreeIter  iter;
                CmdintRef    cip;

                /* If it's shorter, then if we have ftruncate call then we shrink the file to fit the new list if it's shorter.
                   Otherwise write blank ones on the end later.
                   Reallocate the list if it's a different size. */

#ifdef  HAVE_FTRUNCATE
                if  (ninterps < Ci_num)
                        Ignored_error = ftruncate(Ci_fd, ninterps * sizeof(Cmdint));

                if  (ninterps != Ci_num  &&  !(Ci_list = (CmdintRef) realloc((char *) Ci_list, (unsigned)(ninterps * sizeof(Cmdint)))))
                        ABORT_NOMEM;
#else
                /* No ftrunctate - only reallocate if it's shrunk */

                if  (ninterps > Ci_num  &&  !(Ci_list = (CmdintRef) realloc((char *) Ci_list, (unsigned)(ninterps * sizeof(Cmdint)))))
                        ABORT_NOMEM;
#endif

                /* Current item */

                cip = Ci_list;

                for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(cdata.ilist_store), &iter);
                      valid;
                      valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(cdata.ilist_store), &iter))  {

                        gchar           *cname, *cpath, *cargs;
                        gint            cnice, cll;
                        gboolean        csa0, cinterp;

                        gtk_tree_model_get(GTK_TREE_MODEL(cdata.ilist_store), &iter,
                                           CILIST_NAME_COL,     &cname,
                                           CILIST_PATH_COL,     &cpath,
                                           CILIST_PREDEF_COL,   &cargs,
                                           CILIST_NICE_COL,     &cnice,
                                           CILIST_LL_COL,       &cll,
                                           CILIST_SETA0_COL,    &csa0,
                                           CILIST_INTERP_COL,   &cinterp,
                                           -1);

                        /* Copy in stuff */

                        strncpy(cip->ci_name, cname, CI_MAXNAME);
                        strncpy(cip->ci_path, cpath, CI_MAXFPATH);
                        strncpy(cip->ci_args, cargs, CI_MAXARGS);
                        cip->ci_nice = cnice;
                        cip->ci_ll = cll;
                        cip->ci_flags = 0;
                        if  (csa0)
                                cip->ci_flags |= CIF_SETARG0;
                        if  (cinterp)
                                cip->ci_flags |= CIF_INTERPARGS;
                        g_free(cname);
                        g_free(cpath);
                        g_free(cargs);
                        cip++;
                }

#ifdef  HAVE_FTRUNCATE
                Ci_num = ninterps;
#else
                /* No ftruncate, if it's smaller, keep the same length just blank balance out */

                if  (ninterps < Ci_num)  {
                        int     cnt;
                        for  (cnt = ninterps;  cnt < Ci_num;  cnt++)  {
                                cip->ci_name[0] = '\0';
                                cip++;
                        }
                }
                else
                        Ci_num = ninterps;
#endif
                lseek(Ci_fd, 0L, 0);
                Ignored_error = write(Ci_fd, (char *) Ci_list, Ci_num * sizeof(Cmdint));
        }

        gtk_widget_destroy(dlg);
}
