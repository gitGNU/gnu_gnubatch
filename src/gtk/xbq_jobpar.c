/* xbq_jobpar.c -- job parameters for GTK

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
#include <sys/stat.h>
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
#ifndef IN_XBTR
#include "shreq.h"
#include "ipcstuff.h"
#endif
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "jvuprocs.h"
#ifdef IN_XBTR
#include "xbr_ext.h"
#else
#include "xbq_ext.h"
#endif
#include "spitrouts.h"
#include "gtk_lib.h"

static  char    Filename[] = __FILE__;

#define DEFAULT_ARGSDLG_WIDTH   150
#define DEFAULT_ARGSDLG_HEIGHT  300
#define DEFAULT_ENVDLG_WIDTH    400
#define DEFAULT_ENVDLG_HEIGHT   400
#define DEFAULT_REDIRDLG_WIDTH  250
#define DEFAULT_REDIRDLG_HEIGHT 300

#ifdef  HAVE_SYS_RESOURCE_H
int     Max_files;
#else
#ifndef _NFILE
#define _NFILE  64
#endif
int     Max_files = _NFILE;
#endif

extern  char    *Curr_pwd;      /* Directory on entry */

#ifndef IN_XBTR
static  char    *Last_unqueue_dir;
extern  char    *execprog;
char            *udprog, *xmludprog;
HelpaltRef      cancalts;

/* Default set of conditions/assignments */

Jcond   default_conds[MAXCVARS];
Jass    default_asses[MAXSEVARS];
#endif

HelpaltRef      assnames, actnames;

struct  conddata  {
        GtkWidget               *awid;
        GtkListStore            *alist_store;
};

#define LAB_PADDING     5

static vhash_t  lookup_varind(const char *vname, const unsigned perm)
{
        ULONG  sp;
        netid_t hostid = 0;

        if  (strchr(vname, ':'))  {
                char    hostn[HOSTNSIZE+1];
                int     cnt = 0;
                while  (*vname != ':')  {
                        if  (cnt < HOSTNSIZE)
                                hostn[cnt++] = *vname;
                        vname++;
                }
                hostn[cnt] = '\0';
                if  (!(hostid = look_int_hostname(hostn)) == -1)
                        return  -1;
                vname++;
        }
        return  lookupvar(vname, hostid, perm, &sp);
}

#ifndef IN_XBTR
char *encode_defcond(const int condnum)
{
        JcondRef  jc = &default_conds[condnum];
        GString *rb;
        char    *res;

        if  (jc->bjc_compar == C_UNUSED)
                return  (char *) 0;

        rb = g_string_new(NULL);
        g_string_printf(rb, "%s/%d/%d", VAR_NAME(&Var_seg.vlist[jc->bjc_varind].Vent), jc->bjc_compar, jc->bjc_iscrit & CCRIT_NORUN? 1: 0);
        if  (jc->bjc_value.const_type == CON_STRING)
                g_string_append_printf(rb, "/T:%s", jc->bjc_value.con_un.con_string);
        else
                g_string_append_printf(rb, "/I:%ld", (long) jc->bjc_value.con_un.con_long);
        res = stracpy(rb->str);
        g_string_free(rb, TRUE);
        return  res;
}

void  decode_defcond(char *coded)
{
        char  *np = strchr(coded, '/'), *nxt;
        JcondRef        jc;

        for  (jc = default_conds;  jc < &default_conds[MAXCVARS];  jc++)
                if  (jc->bjc_compar == C_UNUSED)
                        break;

        if  (jc >= &default_conds[MAXCVARS]  ||  !np)
                return;

        *np = '\0';
        if  ((jc->bjc_varind = lookup_varind(coded, BTM_READ)) < 0)
                return;

        np++;
        nxt = strchr(np, '/');
        if  (!nxt)
                return;
        jc->bjc_compar = atoi(np);
        np = nxt+1;
        if  (*np == '1')
                jc->bjc_iscrit |= CCRIT_NORUN;
        np += 2;                /* Over digit and slash */
        if  (*np == 'T')  {
                jc->bjc_value.const_type = CON_STRING;
                np += 2;        /* Over T and : */
                strncpy(jc->bjc_value.con_un.con_string, np, BTC_VALUE);
        }
        else  {
                jc->bjc_value.const_type = CON_LONG;
                np += 2;
                jc->bjc_value.con_un.con_long = atoi(np);
        }
}

char *encode_defass(const int assnum)
{
        JassRef  ja = &default_asses[assnum];
        GString *rb;
        char    *res;

        if  (ja->bja_op == BJA_NONE)
                return  (char *) 0;

        rb = g_string_new(NULL);
        g_string_printf(rb, "%s/%d/%d/%d", VAR_NAME(&Var_seg.vlist[ja->bja_varind].Vent),
                        ja->bja_op, ja->bja_iscrit & ACRIT_NORUN? 1: 0, ja->bja_flags);
        if  (ja->bja_con.const_type == CON_STRING)
                g_string_append_printf(rb, "/T:%s", ja->bja_con.con_un.con_string);
        else
                g_string_append_printf(rb, "/I:%ld", (long) ja->bja_con.con_un.con_long);
        res = stracpy(rb->str);
        g_string_free(rb, TRUE);
        return  res;
}

void  decode_defass(char *coded)
{
        char  *np = strchr(coded, '/'), *nxt;
        JassRef ja;

        for  (ja = default_asses;  ja < &default_asses[MAXSEVARS];  ja++)
                if  (ja->bja_op == BJA_NONE)
                        break;

        if  (ja >= &default_asses[MAXSEVARS]  ||  !np)
                return;

        *np = '\0';
        if  ((ja->bja_varind = lookup_varind(coded, BTM_READ)) < 0)
                return;

        np++;
        nxt = strchr(np, '/');
        if  (!nxt)
                return;
        ja->bja_op = atoi(np);
        np = nxt+1;
        if  (*np == '1')
                ja->bja_iscrit |= ACRIT_NORUN;
        np += 2;                /* Over digit and slash */
        ja->bja_flags = atoi(np);
        nxt = strchr(np, '/');
        if  (!nxt)
                return;
        np = nxt+1;
        if  (*np == 'T')  {
                ja->bja_con.const_type = CON_STRING;
                np += 2;        /* Over T and : */
                strncpy(ja->bja_con.con_un.con_string, np, BTC_VALUE);
        }
        else  {
                ja->bja_con.const_type = CON_LONG;
                np += 2;
                ja->bja_con.con_un.con_long = atoi(np);
        }
}
#endif /* ! IN_XBTR */

/* Set up a cell renderer combo for a list of variable names
   Assume in 0th column (only used for conds and asses)
   isexport is -1 to list everything 0 for unexported 1 for exported */

static  void  setup_varlist(struct conddata *adata, GCallback vlchanged, unsigned perm, int isexport)
{
        GtkListStore    *varlist_store;
        GtkTreeViewColumn *col;
        GtkCellRenderer  *rend;
        GtkTreeIter   iter;
        unsigned        vcnt;

#ifdef IN_XBTR
        rvarlist(1);
#endif

        varlist_store = gtk_list_store_new(1, G_TYPE_STRING);

        gtk_list_store_append(varlist_store, &iter);
        gtk_list_store_set(varlist_store, &iter, 0, "--", -1);

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                BtvarRef        vp = &vv_ptrs[vcnt].vep->Vent;

                /* Skip ones which are not allowed.  */

                if  (!mpermitted(&vp->var_mode, perm, mypriv->btu_priv))
                        continue;

                if  (isexport >= 0)  {
                        if  (isexport)  {
                                if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                        continue;
                        }
                        else
                                if  (vp->var_flags & VF_EXPORT)
                                        continue;
                }
                gtk_list_store_append(varlist_store, &iter);
                gtk_list_store_set(varlist_store, &iter, 0, VAR_NAME(vp), -1);
        }

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Variable");
        rend = gtk_cell_renderer_combo_new();
        g_signal_connect(G_OBJECT(rend), "edited", vlchanged, (gpointer) adata);
        g_object_set(G_OBJECT(rend), "model", GTK_TREE_MODEL(varlist_store), "text-column", 0, "editable", TRUE, "has_entry", FALSE, NULL);
        g_object_unref(G_OBJECT(varlist_store));
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", 0, NULL); /* NB Assumes column 0 here and above */
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata->awid), col);
}

#define COND_VAR_COL    0
#define COND_TYPE_COL   1
#define COND_CRIT_COL   2
#define COND_VALUE_ISTEXT_COL   3
#define COND_VALUE_TEXT_COL     4
#define COND_VALUE_INT_COL      5
#define COND_CRIT_SENS_COL      6
#define COND_VALUE_TEXT_SENS_COL        7
#define COND_VALUE_INT_SENS_COL         8

static void  cb_cond_var_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gchar        *old_text, *curr_type;
        gboolean      ist, csens, tsens, isens;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(adata->alist_store), &iter, COND_VAR_COL, &old_text, COND_TYPE_COL, &curr_type, COND_VALUE_ISTEXT_COL, &ist, -1);
        if  (strcmp(old_text, new_text) == 0)  {
                g_free(old_text);
                g_free(curr_type);
                return;
        }

        g_free(old_text);

        if  (strcmp(new_text, "--") == 0  ||  strcmp(curr_type, "--") == 0)
                csens = tsens = isens = FALSE;
        else  {
                csens = TRUE;
                tsens = ist;
                isens = !ist;
        }

        g_free(curr_type);

        gtk_list_store_set(adata->alist_store, &iter, COND_VAR_COL, new_text, COND_CRIT_SENS_COL, csens, COND_VALUE_TEXT_SENS_COL, tsens, COND_VALUE_INT_SENS_COL, isens, -1);
}

static void  cb_cond_type_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gchar        *old_text, *curr_var;
        gboolean      ist, csens, tsens, isens;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(adata->alist_store), &iter, COND_TYPE_COL, &old_text, COND_VAR_COL, &curr_var, COND_VALUE_ISTEXT_COL, &ist, -1);
        if  (strcmp(old_text, new_text) == 0)  {
                g_free(old_text);
                g_free(curr_var);
                return;
        }

        g_free(old_text);

        if  (strcmp(new_text, "--") == 0  ||  strcmp(curr_var, "--") == 0)
                csens = tsens = isens = FALSE;
        else  {
                csens = TRUE;
                tsens = ist;
                isens = !ist;
        }

        g_free(curr_var);

        gtk_list_store_set(adata->alist_store, &iter, COND_TYPE_COL, new_text, COND_CRIT_SENS_COL, csens, COND_VALUE_TEXT_SENS_COL, tsens, COND_VALUE_INT_SENS_COL, isens, -1);
}

static void  cb_cond_crit_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        GtkTreeIter   iter;
        gboolean      value;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        value = !gtk_cell_renderer_toggle_get_active(rend);
        gtk_cell_renderer_toggle_set_active(rend, value);
        gtk_list_store_set(adata->alist_store, &iter, COND_CRIT_COL, value, -1);
}

static void  cb_cond_textint_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        GtkTreeIter   iter;
        gboolean      value;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        value = !gtk_cell_renderer_toggle_get_active(rend);
        gtk_cell_renderer_toggle_set_active(rend, value);
        gtk_list_store_set(adata->alist_store, &iter, COND_VALUE_ISTEXT_COL, value, COND_VALUE_TEXT_SENS_COL, value, COND_VALUE_INT_SENS_COL, !value, -1);
}

static void  cb_cond_textval_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, COND_VALUE_TEXT_COL, new_text, -1);
}

static void  cb_cond_intval_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, COND_VALUE_INT_COL, atoi(new_text), -1);
}

static int  condedit(GtkWidget *dlg, JcondRef condlist, const unsigned perm, const int isexport)
{
        GtkCellRenderer  *rend;
        GtkTreeViewColumn *col;
        GtkListStore    *condt_store;
        GtkTreeIter  iter;
        GtkAdjustment   *adj;
        struct  conddata         adata;
        unsigned        cnt;
        int     ret = 0;

        adata.alist_store = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                JcondRef  jc = &condlist[cnt];
                const  char *vname, *cname, *tval;
                int     ival;
                gboolean  crit, istext, crit_sens, valtext_sens, valint_sens;

                if  (jc->bjc_compar == C_UNUSED  ||  jc->bjc_compar > NUM_CONDTYPES)  {
                        cname = vname = "--";
                        tval = "";
                        ival = 0;
                        crit = istext = crit_sens = valtext_sens = valint_sens = FALSE;
                }
                else  {
                        BtvarRef  vp = &Var_seg.vlist[jc->bjc_varind].Vent;

                        cname = condname[jc->bjc_compar-C_EQ];
                        vname = VAR_NAME(vp);
                        crit = jc->bjc_iscrit & CCRIT_NORUN;
                        crit_sens = TRUE;

                        if  (jc->bjc_value.const_type == CON_STRING)  {
                                istext = valtext_sens = TRUE;
                                valint_sens = FALSE;
                                ival = 0;
                                tval = jc->bjc_value.con_un.con_string;
                        }
                        else  {
                                istext = valtext_sens = FALSE;
                                valint_sens = TRUE;
                                ival = jc->bjc_value.con_un.con_long;
                                tval = "";
                        }
                }

                gtk_list_store_append(adata.alist_store, &iter);
                gtk_list_store_set(adata.alist_store, &iter,
                                   COND_VAR_COL,        vname,
                                   COND_TYPE_COL,       cname,
                                   COND_CRIT_COL,       crit,
                                   COND_VALUE_ISTEXT_COL,istext,
                                   COND_VALUE_TEXT_COL, tval,
                                   COND_VALUE_INT_COL,  ival,
                                   COND_CRIT_SENS_COL,  crit_sens,
                                   COND_VALUE_TEXT_SENS_COL,    valtext_sens,
                                   COND_VALUE_INT_SENS_COL,     valint_sens,
                                   -1);

        }

        adata.awid = gtk_tree_view_new();
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(adata.awid), TRUE);
        gtk_tree_view_set_model(GTK_TREE_VIEW(adata.awid), GTK_TREE_MODEL(adata.alist_store));

        /* First column is variable names */

        setup_varlist(&adata, G_CALLBACK(cb_cond_var_changed), perm, isexport);

        /* Next column is combo with comparison type */

        condt_store = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_list_store_append(condt_store, &iter);
        gtk_list_store_set(condt_store, &iter, 0, "--", -1);
        for  (cnt = 0;  cnt < NUM_CONDTYPES;  cnt++)  {
                gtk_list_store_append(condt_store, &iter);
                gtk_list_store_set(condt_store, &iter, 0, condname[cnt], -1);
        }

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Comp");
        rend = gtk_cell_renderer_combo_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_cond_type_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "model", GTK_TREE_MODEL(condt_store), "text-column", 0, "editable", TRUE, "has_entry", FALSE, NULL);
        g_object_unref(G_OBJECT(condt_store));
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", COND_TYPE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is "condition critical" toggle */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Crit");
        rend = gtk_cell_renderer_toggle_new();
        g_signal_connect(G_OBJECT(rend), "toggled", G_CALLBACK(cb_cond_crit_changed), (gpointer) &adata);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "active", COND_CRIT_COL, "sensitive", COND_CRIT_SENS_COL, "activatable", COND_CRIT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is "comparison is text" toggle which is also sensitive or not as is CRIT_SENS_COL */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Text");
        rend = gtk_cell_renderer_toggle_new();
        g_signal_connect(G_OBJECT(rend), "toggled", G_CALLBACK(cb_cond_textint_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "radio", TRUE, NULL);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "active", COND_VALUE_ISTEXT_COL, "sensitive", COND_CRIT_SENS_COL, "activatable", COND_CRIT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is text entry */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Text compare");
        rend = gtk_cell_renderer_text_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_cond_textval_changed), (gpointer) &adata);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend,
                                            "text",     COND_VALUE_TEXT_COL,
                                            "sensitive",COND_VALUE_TEXT_SENS_COL,
                                            "editable", COND_VALUE_TEXT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is number entry */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Int compare");
        adj = (GtkAdjustment *) gtk_adjustment_new(0, INT_MIN, INT_MAX, 1, 10, 0);
        rend = gtk_cell_renderer_spin_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_cond_intval_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "adjustment", adj, "editable", TRUE, NULL);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend,
                                            "text",     COND_VALUE_INT_COL,
                                            "sensitive",COND_VALUE_INT_SENS_COL,
                                            "editable", COND_VALUE_INT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Show dialog and do the business */

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), adata.awid, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                gboolean        valid;
                GtkTreeIter     iter;
                int             ccount = 0, cnt, cnum = C_UNUSED;

                for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(adata.alist_store), &iter);
                      valid;
                      valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(adata.alist_store), &iter))  {
                        gchar   *vname, *typname, *tval;
                        gint    ival;
                        gboolean        istxt, critval;
                        vhash_t         varind;
                        JcondRef        jc;

                        gtk_tree_model_get(GTK_TREE_MODEL(adata.alist_store), &iter,
                                           COND_VAR_COL,        &vname,
                                           COND_TYPE_COL,       &typname,
                                           COND_CRIT_COL,       &critval,
                                           COND_VALUE_ISTEXT_COL,       &istxt,
                                           COND_VALUE_TEXT_COL, &tval,
                                           COND_VALUE_INT_COL,  &ival,
                                           -1);

                        if  (strcmp(vname, "--") == 0  ||  strcmp(typname, "--") == 0)
                                goto  nullcond;

                        for  (cnt = 0;  cnt < NUM_CONDTYPES;  cnt++)
                                if  (strcmp(condname[cnt], typname) == 0)  {
                                        cnum = cnt+C_EQ;
                                        break;
                                }

                        if  (cnum == C_UNUSED)
                                goto  nullcond;

                        if  ((varind = lookup_varind(vname, BTM_READ)) < 0)
                                goto  nullcond;

                        jc = &condlist[ccount];
                        ccount++;

                        jc->bjc_compar = cnum;
                        jc->bjc_iscrit = critval? CCRIT_NORUN: 0;
                        jc->bjc_varind = varind;
                        if  (istxt)  {
                                jc->bjc_value.const_type = CON_STRING;
                                jc->bjc_value.con_un.con_string[BTC_VALUE] = '\0';
                                strncpy(jc->bjc_value.con_un.con_string, tval, BTC_VALUE);
                        }
                        else  {
                                jc->bjc_value.const_type = CON_LONG;
                                jc->bjc_value.con_un.con_long = ival;
                        }
                nullcond:
                        g_free(vname);
                        g_free(typname);
                        g_free(tval);
                }

                /*  Fill in balance */

                while  (ccount < MAXCVARS)  {
                        JcondRef  jc = &condlist[ccount];
                        jc->bjc_compar = C_UNUSED;
                        jc->bjc_iscrit = 0;
                        jc->bjc_varind = -1;
                        jc->bjc_value.const_type = CON_NONE;
                        ccount++;
                }

                ret = 1;
        }

        gtk_widget_destroy(dlg);

        return  ret;
}

#ifdef IN_XBTR
void  cb_conds(GtkAction *action)
{
        GtkWidget  *dlg;
        struct  pend_job  *pj = job_or_deflt(action);
        int     isexport;
        Jcond   copyconds[MAXCVARS];

        if  (!pj)
                return;

        BLOCK_COPY(copyconds, pj->job->h.bj_conds, sizeof(copyconds));

        dlg = start_jobdlg(pj, $P{xbtq job conds dlgtit}, $P{xbtq job conds lab});
        isexport = pj->job->h.bj_jflags & BJ_EXPORT? 1: 0;

        if  (condedit(dlg, copyconds, BTM_READ, isexport))  {
                BLOCK_COPY(pj->job->h.bj_conds, copyconds, sizeof(copyconds));
                note_changes(pj);
        }
}

#else
void  cb_conds()
{
        GtkWidget  *dlg;
        BtjobRef  cj = getselectedjob(BTM_WRITE);
        int     cnt, ccount = 0, isexport;
        Btjob   copyjob;

        if  (!cj)
                return;
        copyjob = *cj;

        dlg = start_jobdlg(&copyjob, $P{xbtq job conds dlgtit}, $P{xbtq job conds lab});

        /* If the job currently has no conditions, then fill in the defaults (only applicable ones)  */

        isexport = copyjob.h.bj_jflags & BJ_EXPORT? 1: 0;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                if  (copyjob.h.bj_conds[cnt].bjc_compar == C_UNUSED)
                        break;
                ccount++;
        }

        if  (ccount == 0)  {
                for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                        JcondRef  jc = &default_conds[cnt];
                        BtvarRef  vp;

                        if  (jc->bjc_compar == C_UNUSED)
                                continue;

                        vp = &Var_seg.vlist[jc->bjc_varind].Vent;
                        if  (!mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv))
                                continue;
                        if  (isexport)  {
                                if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                        continue;
                        }
                        else  if  (vp->var_flags & VF_EXPORT)
                                continue;
                        copyjob.h.bj_conds[ccount] = *jc;
                        ccount++;
                }
        }

        if  (condedit(dlg, copyjob.h.bj_conds, BTM_READ, isexport))  {
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = copyjob;
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
        }
}

void  cb_conddef()
{
        GtkWidget       *dlg;
        Jcond           copycond[MAXCVARS];

        dlg = gprompt_dialog(toplevel, $P{xbtq cond deflt dlgtit});
        BLOCK_COPY(copycond, default_conds, sizeof(copycond));
        if  (condedit(dlg, copycond, BTM_READ, -1))  {
                BLOCK_COPY(default_conds, copycond, sizeof(default_conds));
                Dirty++;
        }
}
#endif /* !IN_XBTR */

#define ASS_VAR_COL             0
#define ASS_TYPE_COL            1
#define ASS_CRIT_COL            2
#define ASS_START_FLAG_COL      3
#define ASS_NORM_FLAG_COL       4
#define ASS_ERR_FLAG_COL        5
#define ASS_ABORT_FLAG_COL      6
#define ASS_CANC_FLAG_COL       7
#define ASS_REVERSE_FLAG_COL    8
#define ASS_VALUE_ISTEXT_COL    9
#define ASS_VALUE_TEXT_COL      10
#define ASS_VALUE_INT_COL       11
#define ASS_CRIT_SENS_COL       12
#define ASS_FLAGS_SENS_COL      13
#define ASS_VALUE_TEXT_SENS_COL 14
#define ASS_VALUE_INT_SENS_COL  15
#define ASS_VALUE_ISTEXT_SENS_COL       16

static void  cb_ass_var_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gchar        *old_text, *curr_type;
        gboolean      ist, csens, tsens, isens, fsens, ist_sens;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(adata->alist_store), &iter, ASS_VAR_COL, &old_text, ASS_TYPE_COL, &curr_type, ASS_VALUE_ISTEXT_COL, &ist, -1);
        if  (strcmp(old_text, new_text) == 0)  {
                g_free(old_text);
                g_free(curr_type);
                return;
        }

        g_free(old_text);

        if  (strcmp(new_text, "--") == 0  ||  strcmp(curr_type, "--") == 0)
                csens = tsens = isens = fsens = ist_sens = FALSE;
        else  {
                int     typ = lookup_alt(curr_type, assnames);
                csens = TRUE;
                if  (typ >= BJA_SEXIT)
                        tsens = isens = fsens = ist_sens = FALSE;
                else  {
                        if  (typ == BJA_ASSIGN)
                                ist_sens = TRUE;
                        else
                                ist_sens = ist = FALSE;
                        tsens = ist;
                        isens = !ist;
                        fsens = TRUE;
                }
        }

        g_free(curr_type);

        gtk_list_store_set(adata->alist_store, &iter,
                           ASS_VAR_COL, new_text,
                           ASS_CRIT_SENS_COL,   csens,
                           ASS_VALUE_TEXT_SENS_COL,     tsens,
                           ASS_VALUE_INT_SENS_COL,      isens,
                           ASS_FLAGS_SENS_COL,  fsens,
                           ASS_VALUE_ISTEXT_SENS_COL,   ist_sens,
                           -1);
}

static void  cb_ass_type_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gchar        *old_text, *curr_var;
        gboolean      ist, fsens, csens, tsens, isens, ist_sens;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(adata->alist_store), &iter, ASS_TYPE_COL, &old_text, ASS_VAR_COL, &curr_var, COND_VALUE_ISTEXT_COL, &ist, -1);
        if  (strcmp(old_text, new_text) == 0)  {
                g_free(old_text);
                g_free(curr_var);
                return;
        }

        g_free(old_text);

        if  (strcmp(new_text, "--") == 0  ||  strcmp(curr_var, "--") == 0)
                csens = tsens = isens = fsens = ist_sens = FALSE;
        else  {
                int     typ = lookup_alt(new_text, assnames);
                csens = TRUE;
                if  (typ >= BJA_SEXIT)
                        tsens = isens = fsens = ist_sens = FALSE;
                else  {
                        if  (typ == BJA_ASSIGN)
                                ist_sens = TRUE;
                        else
                                ist_sens = ist = FALSE;
                        tsens = ist;
                        isens = !ist;
                        fsens = TRUE;
                }
        }

        g_free(curr_var);

        gtk_list_store_set(adata->alist_store, &iter,
                           ASS_TYPE_COL,        new_text,
                           ASS_CRIT_SENS_COL,   csens,
                           ASS_VALUE_TEXT_SENS_COL,     tsens,
                           ASS_VALUE_INT_SENS_COL,      isens,
                           ASS_FLAGS_SENS_COL,  fsens,
                           ASS_VALUE_ISTEXT_SENS_COL,   ist_sens,
                           -1);
}

static void  cb_ass_toggle(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata, const int col)
{
        GtkTreeIter     iter;
        gboolean        value;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        value = !gtk_cell_renderer_toggle_get_active(rend);
        gtk_cell_renderer_toggle_set_active(rend, value);
        gtk_list_store_set(adata->alist_store, &iter, col, value, -1);
}
static void  cb_ass_crit_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_CRIT_COL);
}
static void  cb_startf_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_START_FLAG_COL);
}
static void  cb_okf_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_NORM_FLAG_COL);
}
static void  cb_errf_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_ERR_FLAG_COL);
}
static void  cb_abortf_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_ABORT_FLAG_COL);
}
static void  cb_cancf_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_CANC_FLAG_COL);
}
static void  cb_revf_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        cb_ass_toggle(rend, path, adata, ASS_REVERSE_FLAG_COL);
}

static void  cb_ass_textint_changed(GtkCellRendererToggle *rend, gchar *path, struct conddata *adata)
{
        GtkTreeIter   iter;
        gboolean      value;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        value = !gtk_cell_renderer_toggle_get_active(rend);
        gtk_cell_renderer_toggle_set_active(rend, value);
        gtk_list_store_set(adata->alist_store, &iter, ASS_VALUE_ISTEXT_COL, value, ASS_VALUE_TEXT_SENS_COL, value, ASS_VALUE_INT_SENS_COL, !value, -1);
}

static void  cb_ass_textval_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, ASS_VALUE_TEXT_COL, new_text, -1);
}

static void  cb_ass_intval_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct conddata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, ASS_VALUE_INT_COL, atoi(new_text), -1);
}

struct  assf_list  {
        const   char    *title;
        int             col;
        void            (*cb_func)(GtkCellRendererToggle *, gchar *, struct conddata *);
}  assflist[] =
        {  { "S",       ASS_START_FLAG_COL,     cb_startf_changed  },
           { "N",       ASS_NORM_FLAG_COL,      cb_okf_changed  },
           { "E",       ASS_ERR_FLAG_COL,       cb_errf_changed  },
           { "A",       ASS_ABORT_FLAG_COL,     cb_abortf_changed  },
           { "C",       ASS_CANC_FLAG_COL,      cb_cancf_changed },
           { "R",       ASS_REVERSE_FLAG_COL,   cb_revf_changed  }  };

static int  assedit(GtkWidget *dlg, JassRef asslist, const unsigned perm, const int isexport)
{
        GtkCellRenderer  *rend;
        GtkTreeViewColumn *col;
        GtkListStore    *asst_store;
        GtkAdjustment   *adj;
        GtkTreeIter  iter;
        struct  conddata         adata;
        unsigned        cnt;
        int     ret = 0;

        adata.alist_store = gtk_list_store_new(17,
                                               G_TYPE_STRING, /* Var name */
                                               G_TYPE_STRING, /* Ass type */
                                               G_TYPE_BOOLEAN,/* Crit */
                                               G_TYPE_BOOLEAN,/* Start flag */
                                               G_TYPE_BOOLEAN,/* Normal exit flag */
                                               G_TYPE_BOOLEAN,/* Error exit flag */
                                               G_TYPE_BOOLEAN,/* Abort exit flag */
                                               G_TYPE_BOOLEAN,/* Cancel flag */
                                               G_TYPE_BOOLEAN,/* Reverse flag */
                                               G_TYPE_BOOLEAN,/* Constant is text */
                                               G_TYPE_STRING, /* Text string */
                                               G_TYPE_INT,    /* Integer value */
                                               G_TYPE_BOOLEAN,/* Crit sensitive (plus all other) */
                                               G_TYPE_BOOLEAN,/* Flags and values sensitive (not if set exit/signal) */
                                               G_TYPE_BOOLEAN,/* Text value sensitive */
                                               G_TYPE_BOOLEAN,/* Int value sensitive */
                                               G_TYPE_BOOLEAN); /* Text/int flag sensitive */

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                JassRef  ja = &asslist[cnt];
                const  char *vname, *aname, *tval;
                int     ival;
                gboolean  crit, istext = FALSE, istext_sens, startf, okf, errf, abf, cancf, revf, crit_sens, flags_sens, valtext_sens, valint_sens;

                if  (ja->bja_op == BJA_NONE  ||  ja->bja_op > NUM_ASSTYPES)  {
                        aname = vname = "--";
                        tval = "";
                        ival = 0;
                        crit = istext = istext_sens = startf = okf = errf = abf = cancf = revf = crit_sens = flags_sens = valtext_sens = valint_sens = FALSE;
                }
                else  {
                        BtvarRef  vp = &Var_seg.vlist[ja->bja_varind].Vent;

                        aname = disp_alt(ja->bja_op, assnames);
                        vname = VAR_NAME(vp);
                        crit = ja->bja_iscrit & ACRIT_NORUN;
                        crit_sens = TRUE;

                        if  (ja->bja_op >= BJA_SEXIT)  {
                                tval = "";
                                ival = 0;
                                startf = okf = errf = abf = cancf = revf = flags_sens = valtext_sens = valint_sens = istext_sens = FALSE;
                        }
                        else  {
                                if  (ja->bja_op == BJA_ASSIGN)  {
                                        istext_sens = TRUE;
                                        if  (ja->bja_con.const_type == CON_STRING)  {
                                                istext = valtext_sens = TRUE;
                                                valint_sens = FALSE;
                                                ival = 0;
                                                tval = ja->bja_con.con_un.con_string;
                                        }
                                        else  {
                                                istext = valtext_sens = FALSE;
                                                valint_sens = TRUE;
                                                ival = ja->bja_con.con_un.con_long;
                                                tval = "";
                                        }
                                }
                                else  {
                                        istext_sens = FALSE;
                                        istext = valtext_sens = FALSE;
                                        valint_sens = TRUE;
                                        ival = ja->bja_con.const_type == CON_STRING? 0: ja->bja_con.con_un.con_long;
                                        tval = "";
                                }

                                flags_sens = TRUE;
                                startf = ja->bja_flags & BJA_START;
                                okf = ja->bja_flags & BJA_OK;
                                errf = ja->bja_flags & BJA_ERROR;
                                abf = ja->bja_flags & BJA_ABORT;
                                cancf = ja->bja_flags & BJA_CANCEL;
                                revf = ja->bja_flags & BJA_REVERSE;
                        }
                }

                gtk_list_store_append(adata.alist_store, &iter);
                gtk_list_store_set(adata.alist_store, &iter,
                                   ASS_VAR_COL,         vname,
                                   ASS_TYPE_COL,        aname,
                                   ASS_CRIT_COL,        crit,
                                   ASS_START_FLAG_COL,  startf,
                                   ASS_NORM_FLAG_COL,   okf,
                                   ASS_ERR_FLAG_COL,    errf,
                                   ASS_ABORT_FLAG_COL,  abf,
                                   ASS_CANC_FLAG_COL,   cancf,
                                   ASS_REVERSE_FLAG_COL,revf,
                                   ASS_VALUE_ISTEXT_COL,istext,
                                   ASS_VALUE_TEXT_COL,  tval,
                                   ASS_VALUE_INT_COL,   ival,
                                   ASS_CRIT_SENS_COL,   crit_sens,
                                   ASS_FLAGS_SENS_COL,  flags_sens,
                                   ASS_VALUE_TEXT_SENS_COL,     valtext_sens,
                                   ASS_VALUE_INT_SENS_COL,      valint_sens,
                                   ASS_VALUE_ISTEXT_SENS_COL,   istext_sens,
                                   -1);

        }

        adata.awid = gtk_tree_view_new();
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(adata.awid), TRUE);
        gtk_tree_view_set_model(GTK_TREE_VIEW(adata.awid), GTK_TREE_MODEL(adata.alist_store));

        /* First column is variable names */

        setup_varlist(&adata, G_CALLBACK(cb_ass_var_changed), perm, isexport);

        /* Next column is combo with assignment type */

        asst_store = gtk_list_store_new(1, G_TYPE_STRING);
        gtk_list_store_append(asst_store, &iter);
        gtk_list_store_set(asst_store, &iter, 0, "--", -1);
        for  (cnt = 0;  cnt < NUM_ASSTYPES;  cnt++)  {
                gtk_list_store_append(asst_store, &iter);
                gtk_list_store_set(asst_store, &iter, 0, assnames->list[cnt], -1);
        }

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Op");
        rend = gtk_cell_renderer_combo_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_ass_type_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "model", GTK_TREE_MODEL(asst_store), "text-column", 0, "editable", TRUE, "has_entry", FALSE, NULL);
        g_object_unref(G_OBJECT(asst_store));
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", ASS_TYPE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is "assignment critical" toggle */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Crit");
        rend = gtk_cell_renderer_toggle_new();
        g_signal_connect(G_OBJECT(rend), "toggled", G_CALLBACK(cb_ass_crit_changed), (gpointer) &adata);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "active", ASS_CRIT_COL, "sensitive", ASS_CRIT_SENS_COL, "activatable", ASS_CRIT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next 6 columns are assignment flags */

        for  (cnt = 0;  cnt < 6;  cnt++)  {
                col = gtk_tree_view_column_new();
                gtk_tree_view_column_set_resizable(col, TRUE);
                gtk_tree_view_column_set_title(col, assflist[cnt].title);
                rend = gtk_cell_renderer_toggle_new();
                g_signal_connect(G_OBJECT(rend), "toggled", G_CALLBACK(assflist[cnt].cb_func), (gpointer) &adata);
                gtk_tree_view_column_pack_start(col, rend, TRUE);
                gtk_tree_view_column_set_attributes(col, rend, "active", assflist[cnt].col, "sensitive", ASS_FLAGS_SENS_COL, "activatable", ASS_FLAGS_SENS_COL, NULL);
                gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);
        }

        /* Next column is "value is text" toggle */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Text");
        rend = gtk_cell_renderer_toggle_new();
        g_signal_connect(G_OBJECT(rend), "toggled", G_CALLBACK(cb_ass_textint_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "radio", TRUE, NULL);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "active", ASS_VALUE_ISTEXT_COL, "sensitive", ASS_VALUE_ISTEXT_SENS_COL, "activatable", ASS_VALUE_ISTEXT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is text entry */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Text val");
        rend = gtk_cell_renderer_text_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_ass_textval_changed), (gpointer) &adata);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend,
                                            "text",     ASS_VALUE_TEXT_COL,
                                            "sensitive",ASS_VALUE_TEXT_SENS_COL,
                                            "editable", ASS_VALUE_TEXT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Next column is number entry */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Int val");
        adj = (GtkAdjustment *) gtk_adjustment_new(0, INT_MIN, INT_MAX, 1, 10, 0);
        rend = gtk_cell_renderer_spin_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_ass_intval_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "adjustment", adj, "editable", TRUE, NULL);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend,
                                            "text",     ASS_VALUE_INT_COL,
                                            "sensitive",ASS_VALUE_INT_SENS_COL,
                                            "editable", ASS_VALUE_INT_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Show dialog and do the business */

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), adata.awid, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                gboolean        valid;
                GtkTreeIter     iter;
                int             acount = 0, anum = BJA_NONE;

                for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(adata.alist_store), &iter);
                      valid;
                      valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(adata.alist_store), &iter))  {
                        gchar   *vname, *typname, *tval;
                        gint            ival;
                        gboolean        istxt, critval, startf, okf, errf, abf, cancf, revf;
                        vhash_t         varind;
                        JassRef         ja;
                        USHORT          flgs = 0;

                        gtk_tree_model_get(GTK_TREE_MODEL(adata.alist_store), &iter,
                                           ASS_VAR_COL,         &vname,
                                           ASS_TYPE_COL,        &typname,
                                           ASS_CRIT_COL,        &critval,
                                           ASS_START_FLAG_COL,  &startf,
                                           ASS_NORM_FLAG_COL,   &okf,
                                           ASS_ERR_FLAG_COL,    &errf,
                                           ASS_ABORT_FLAG_COL,  &abf,
                                           ASS_CANC_FLAG_COL,   &cancf,
                                           ASS_REVERSE_FLAG_COL,&revf,
                                           ASS_VALUE_ISTEXT_COL,&istxt,
                                           ASS_VALUE_TEXT_COL,  &tval,
                                           ASS_VALUE_INT_COL,   &ival,
                                           -1);

                        if  (strcmp(vname, "--") == 0  ||  strcmp(typname, "--") == 0)
                                goto  nullass;

                        anum = lookup_alt(typname, assnames);
                        if  (anum < 0)
                                goto  nullass;

                        if  ((varind = lookup_varind(vname, BTM_WRITE)) < 0)
                                goto  nullass;

                        if  (startf)
                                flgs |= BJA_START;
                        if  (okf)
                                flgs |= BJA_OK;
                        if  (errf)
                                flgs |= BJA_ERROR;
                        if  (abf)
                                flgs |= BJA_ABORT;
                        if  (cancf)
                                flgs |= BJA_CANCEL;
                        if  (revf)
                                flgs |= BJA_REVERSE;

                        if  (flgs == 0)
                                goto  nullass;

                        ja = &asslist[acount];
                        acount++;

                        ja->bja_op = anum;
                        ja->bja_iscrit = critval? ACRIT_NORUN: 0;
                        ja->bja_varind = varind;
                        ja->bja_flags = flgs;
                        if  (istxt)  {
                                ja->bja_con.const_type = CON_STRING;
                                ja->bja_con.con_un.con_string[BTC_VALUE] = '\0';
                                strncpy(ja->bja_con.con_un.con_string, tval, BTC_VALUE);
                        }
                        else  {
                                ja->bja_con.const_type = CON_LONG;
                                ja->bja_con.con_un.con_long = ival;
                        }
                nullass:
                        g_free(vname);
                        g_free(typname);
                        g_free(tval);
                }

                /*  Fill in balance */

                while  (acount < MAXSEVARS)  {
                        JassRef  ja = &asslist[acount];
                        ja->bja_op = BJA_NONE;
                        ja->bja_flags = 0;
                        ja->bja_iscrit = 0;
                        ja->bja_varind = -1;
                        ja->bja_con.const_type = CON_NONE;
                        acount++;
                }

                ret = 1;
        }

        gtk_widget_destroy(dlg);

        return  ret;
}

#ifdef IN_XBTR
void  cb_asses(GtkAction *action)
{
        GtkWidget  *dlg;
        struct  pend_job  *pj = job_or_deflt(action);
        int     isexport;
        Jass    copyasses[MAXSEVARS];

        if  (!pj)
                return;

        BLOCK_COPY(copyasses, pj->job->h.bj_asses, sizeof(copyasses));
        dlg = start_jobdlg(pj, $P{xbtq job asses dlgtit}, $P{xbtq job asses lab});
        isexport = pj->job->h.bj_jflags & BJ_EXPORT? 1: 0;

        if  (assedit(dlg, copyasses, BTM_WRITE, isexport))  {
                BLOCK_COPY(pj->job->h.bj_asses, copyasses, sizeof(copyasses));
                note_changes(pj);
        }
}

#else /* !IN_XBTR */
void  cb_asses()
{
        GtkWidget  *dlg;
        BtjobRef  cj = getselectedjob(BTM_WRITE);
        int     cnt, acount = 0, isexport;
        Btjob   copyjob;

        if  (!cj)
                return;
        copyjob = *cj;

        dlg = start_jobdlg(&copyjob, $P{xbtq job asses dlgtit}, $P{xbtq job asses lab});

        /* If the job currently has no conditions, then fill in the defaults (only applicable ones)  */

        isexport = copyjob.h.bj_jflags & BJ_EXPORT? 1: 0;

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                if  (copyjob.h.bj_asses[cnt].bja_op == BJA_NONE)
                        break;
                acount++;
        }

        if  (acount == 0)  {
                for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                        JassRef  ja = &default_asses[cnt];
                        BtvarRef  vp;

                        if  (ja->bja_op == BJA_NONE)
                                continue;

                        vp = &Var_seg.vlist[ja->bja_varind].Vent;
                        if  (!mpermitted(&vp->var_mode, BTM_WRITE, mypriv->btu_priv))
                                continue;
                        if  (isexport)  {
                                if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                        continue;
                        }
                        else  if  (vp->var_flags & VF_EXPORT)
                                continue;
                        copyjob.h.bj_asses[acount] = *ja;
                        acount++;
                }
        }

        if  (assedit(dlg, copyjob.h.bj_asses, BTM_WRITE, isexport))  {
                unsigned        retc;
                ULONG           xindx;
                BtjobRef        bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = copyjob;
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
        }
}

void  cb_assdef()
{
        GtkWidget       *dlg;
        Jass            copyass[MAXSEVARS];
        dlg = gprompt_dialog(toplevel, $P{xbtq ass deflt dlgtit});
        BLOCK_COPY(copyass, default_asses, sizeof(copyass));
        if  (assedit(dlg, copyass, BTM_WRITE, -1))  {
                BLOCK_COPY(default_asses, copyass, sizeof(default_asses));
                Dirty++;
        }
}
#endif

struct  aerdata  {
        Btjob                   cjob;
        GtkWidget               *awid;
        GtkListStore            *alist_store;
};

static void  add_arg(struct aerdata *adata)
{
        GtkWidget       *dlg, *argent;

        dlg = gprompt_dialog(toplevel, $P{xbtq arg add dlgtit});
        argent = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), argent, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newarg = gtk_entry_get_text(GTK_ENTRY(argent));
                GtkTreeIter  iter;
                gtk_list_store_append(adata->alist_store, &iter);
                gtk_list_store_set(adata->alist_store, &iter, 0, newarg, -1);
        }
        gtk_widget_destroy(dlg);
}

static void  del_arg(struct aerdata *adata)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(adata->awid));
        GtkTreeIter  iter;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))
                gtk_list_store_remove(adata->alist_store, &iter);
}

static void  cb_arg_val_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, 0, new_text, -1);
}

#ifdef IN_XBTR
void  cb_args(GtkAction *action)
{
        struct  pend_job  *pj = job_or_deflt(action);
        GtkWidget  *dlg, *scroll, *hbox, *butt;
        GtkCellRenderer  *rend;
        struct  aerdata  adata;
        unsigned        cnt;

        if  (!pj)
                return;

        adata.cjob = *pj->job;
        dlg = start_jobdlg(pj, $P{xbtq job args dlgtit}, $P{xbtq job args lab});
#else
void  cb_args()
{
        BtjobRef  cj = getselectedjob(BTM_WRITE);
        GtkWidget  *dlg, *scroll, *hbox, *butt;
        GtkCellRenderer  *rend;
        struct  aerdata  adata;
        unsigned        cnt;

        if  (!cj)
                return;

        adata.cjob = *cj;
        dlg = start_jobdlg(&adata.cjob, $P{xbtq job args dlgtit}, $P{xbtq job args lab});
#endif

        gtk_window_set_default_size(GTK_WINDOW(dlg), DEFAULT_ARGSDLG_WIDTH, DEFAULT_ARGSDLG_HEIGHT);
        adata.alist_store = gtk_list_store_new(1, G_TYPE_STRING);

        for  (cnt = 0;  cnt < adata.cjob.h.bj_nargs;  cnt++)  {
                GtkTreeIter   iter;
                gtk_list_store_append(adata.alist_store, &iter);
                gtk_list_store_set(adata.alist_store, &iter, 0, ARG_OF((&adata.cjob), cnt), -1);
        }

        adata.awid = gtk_tree_view_new();
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(adata.awid), TRUE);
        rend = gtk_cell_renderer_text_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_arg_val_changed), (gpointer) &adata);
        g_object_set(rend, "editable", TRUE, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(adata.awid), -1, "Arg", rend, "text", 0, NULL);
        gtk_tree_view_set_model(GTK_TREE_VIEW(adata.awid), GTK_TREE_MODEL(adata.alist_store));
        g_object_unref(adata.alist_store);              /* So that it gets deallocated */
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), adata.awid);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, DEF_DLG_VPAD, GTK_PACK_START);

        hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD, GTK_PACK_START);

        butt = gprompt_button($P{xbtq add arg});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(add_arg), (gpointer) &adata);
        butt = gprompt_button($P{xbtq del arg});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(del_arg), (gpointer) &adata);
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                char    **copyargs = 0;
                int     nargs = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(adata.alist_store),  NULL);
#ifdef IN_XBTR
                Btjob   newcopy;
#else
                ULONG           xindx;
                unsigned        retc;
                BtjobRef        bjp;
#endif

                if  (nargs > 0)  {
                        char  **ca;
                        gboolean valid;
                        GtkTreeIter  iter;
                        copyargs = (char **) malloc((nargs + 1) * sizeof(char *));
                        if  (!copyargs)
                                ABORT_NOMEM;
                        ca = copyargs;

                        for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(adata.alist_store), &iter);
                              valid;
                              valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(adata.alist_store), &iter))  {
                                gchar  *aval;
                                gtk_tree_model_get(GTK_TREE_MODEL(adata.alist_store), &iter, 0, &aval, -1);
                                *ca++ = stracpy(aval);
                                g_free(aval);
                        }
                        *ca = 0;
                }

#ifdef IN_XBTR
                newcopy = adata.cjob;
                if  (!repackjob(&newcopy, &adata.cjob, (char *) 0, (char *) 0, 0, 0, nargs, (MredirRef) 0, (MenvirRef) 0, copyargs))  {
                        disp_arg[3] = adata.cjob.h.bj_job;
                        disp_str = title_of(&adata.cjob);
                        doerror($EH{Too many job strings});
                        if  (copyargs)
                                freehelp(copyargs);
                        continue;
                }
                *pj->job = newcopy;
                if  (copyargs)
                        freehelp(copyargs);
                note_changes(pj);
#else
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = adata.cjob;
                if  (!repackjob(bjp, &adata.cjob, (char *) 0, (char *) 0, 0, 0, nargs, (MredirRef) 0, (MenvirRef) 0, copyargs))  {
                        disp_arg[3] = adata.cjob.h.bj_job;
                        disp_str = qtitle_of(&adata.cjob);
                        doerror($EH{Too many job strings});
                        freexbuf(xindx);
                        if  (copyargs)
                                freehelp(copyargs);
                        continue;
                }
                if  (copyargs)
                        freehelp(copyargs);
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
#endif
                break;
        }
        gtk_widget_destroy(dlg);
}

static void  add_env(struct aerdata *adata)
{
        GtkWidget       *dlg, *ename, *evalue;

        dlg = gprompt_dialog(toplevel, $P{xbtq env add dlgtit});
        ename = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ename, FALSE, FALSE, DEF_DLG_VPAD);
        evalue = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), evalue, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newename = gtk_entry_get_text(GTK_ENTRY(ename));
                const  char  *newevalue = gtk_entry_get_text(GTK_ENTRY(evalue));
                GtkTreeIter  iter;
                gtk_list_store_append(adata->alist_store, &iter);
                gtk_list_store_set(adata->alist_store, &iter, 0, newename, 1, newevalue, -1);
        }
        gtk_widget_destroy(dlg);
}

static void  cb_env_name_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, 0, new_text, -1);
}

static void  cb_env_val_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, 1, new_text, -1);
}

#ifdef IN_XBTR
void  cb_env(GtkAction *action)
{
        struct  pend_job  *pj = job_or_deflt(action);
        GtkWidget  *dlg, *scroll, *hbox, *butt;
        GtkCellRenderer  *rend;
        struct  aerdata  adata;
        unsigned        cnt;

        if  (!pj)
                return;

        adata.cjob = *pj->job;
        dlg = start_jobdlg(pj, $P{xbtq job env dlgtit}, $P{xbtq job env lab});
#else
void  cb_env()
{
        BtjobRef  cj = getselectedjob(BTM_WRITE);
        GtkWidget  *dlg, *scroll, *hbox, *butt;
        GtkCellRenderer  *rend;
        struct  aerdata  adata;
        unsigned        cnt;

        if  (!cj)
                return;

        adata.cjob = *cj;
        dlg = start_jobdlg(cj, $P{xbtq job env dlgtit}, $P{xbtq job env lab});
#endif

        gtk_window_set_default_size(GTK_WINDOW(dlg), DEFAULT_ENVDLG_WIDTH, DEFAULT_ENVDLG_HEIGHT);
        adata.alist_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
        for  (cnt = 0;  cnt < adata.cjob.h.bj_nenv;  cnt++)  {
                GtkTreeIter   iter;
                char    *ename, *evalue;
                ENV_OF(&adata.cjob, cnt, ename, evalue);
                gtk_list_store_append(adata.alist_store, &iter);
                gtk_list_store_set(adata.alist_store, &iter, 0, ename, 1, evalue, -1);
        }

        adata.awid = gtk_tree_view_new();
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(adata.awid), TRUE);
        rend = gtk_cell_renderer_text_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_env_name_changed), (gpointer) &adata);
        g_object_set(rend, "editable", TRUE, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(adata.awid), -1, "Name", rend, "text", 0, NULL);
        rend = gtk_cell_renderer_text_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_env_val_changed), (gpointer) &adata);
        g_object_set(rend, "editable", TRUE, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(adata.awid), -1, "Value", rend, "text", 1, NULL);
        gtk_tree_view_set_model(GTK_TREE_VIEW(adata.awid), GTK_TREE_MODEL(adata.alist_store));
        g_object_unref(adata.alist_store);              /* So that it gets deallocated */

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), adata.awid);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, DEF_DLG_VPAD, GTK_PACK_START);

        hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD, GTK_PACK_START);

        butt = gprompt_button($P{xbtq add env});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(add_env), (gpointer) &adata);
        butt = gprompt_button($P{xbtq del env});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(del_arg), (gpointer) &adata); /* Did mean del_arg */
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                Menvir  *copyenv = 0;
                int     nenv = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(adata.alist_store),  NULL);
#ifdef IN_XBTR
                Btjob           newcopy;
#else
                ULONG           xindx;
                unsigned        retc;
                BtjobRef        bjp;
#endif
                if  (nenv > 0)  {
                        Menvir  *ce;
                        gboolean valid;
                        GtkTreeIter  iter;
                        copyenv = (Menvir *) malloc(nenv * sizeof(Menvir));
                        if  (!copyenv)
                                ABORT_NOMEM;
                        ce = copyenv;

                        for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(adata.alist_store), &iter);
                              valid;
                              valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(adata.alist_store), &iter))  {
                                gchar  *nval, *vval;
                                gtk_tree_model_get(GTK_TREE_MODEL(adata.alist_store), &iter, 0, &nval, 1, &vval, -1);
                                ce->e_name = stracpy(nval);
                                ce->e_value = stracpy(vval);
                                ce++;
                                g_free(nval);
                                g_free(vval);
                        }
                }
#ifdef IN_XBTR
                newcopy = adata.cjob;
                if  (!repackjob(&newcopy, &adata.cjob, (char *) 0, (char *) 0, 0, nenv, 0, (MredirRef) 0, copyenv, (char **) 0))  {
                        disp_arg[3] = adata.cjob.h.bj_job;
                        disp_str = title_of(&adata.cjob);
                        doerror($EH{Too many job strings});
#else
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = adata.cjob;
                if  (!repackjob(bjp, &adata.cjob, (char *) 0, (char *) 0, 0, nenv, 0, (MredirRef) 0, copyenv, (char **) 0))  {
                        disp_arg[3] = adata.cjob.h.bj_job;
                        disp_str = qtitle_of(&adata.cjob);
                        doerror($EH{Too many job strings});
                        freexbuf(xindx);
#endif
                        if  (copyenv)  {
                                Menvir  *ce;
                                for  (ce = copyenv;  ce < &copyenv[nenv];  ce++)  {
                                        free(ce->e_name);
                                        free(ce->e_value);
                                }
                                free((char *) copyenv);
                        }
                        continue;
                }
                if  (copyenv)  {
                        Menvir  *ce;
                        for  (ce = copyenv;  ce < &copyenv[nenv];  ce++)  {
                                free(ce->e_name);
                                free(ce->e_value);
                        }
                        free((char *) copyenv);
                }
#ifdef IN_XBTR
                *pj->job = newcopy;
                note_changes(pj);
#else
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
#endif
                break;
        }
        gtk_widget_destroy(dlg);
}

#define REDIR_FD_COL            0
#define REDIR_TYPE_COL          1
#define REDIR_FILENAME_COL      2
#define REDIR_FILENAME_SENS_COL 3
#define REDIR_FD2_COL           4
#define REDIR_FD2_SENS_COL      5

static void  cb_redir_fd_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, REDIR_FD_COL, atoi(new_text), -1);
}

static void  cb_redir_type_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter     iter;
        gchar           *old_text;
        gboolean        filen_sens = TRUE;
        gboolean        fd2_sens = FALSE;
        int             rtype;

        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_tree_model_get(GTK_TREE_MODEL(adata->alist_store), &iter, REDIR_TYPE_COL, &old_text, -1);
        if  (strcmp(old_text, new_text) == 0)  {
                g_free(old_text);
                return;
        }

        g_free(old_text);

        rtype = lookup_alt(new_text, actnames);
        if  (rtype >= RD_ACT_CLOSE)  {
                filen_sens = FALSE;
                if  (rtype > RD_ACT_CLOSE)
                        fd2_sens = TRUE;
        }

        gtk_list_store_set(adata->alist_store, &iter, REDIR_TYPE_COL, new_text, REDIR_FILENAME_SENS_COL, filen_sens, REDIR_FD2_SENS_COL, fd2_sens, -1);
}

static void  cb_redir_file_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, REDIR_FILENAME_COL, new_text, -1);
}

static void  cb_redir_dup_fd_changed(GtkCellRendererText *rend, gchar *path, gchar *new_text, struct aerdata *adata)
{
        GtkTreeIter   iter;
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(adata->alist_store), &iter, path);
        gtk_list_store_set(adata->alist_store, &iter, REDIR_FD2_COL, atoi(new_text), -1);
}

struct  addredirstr  {
        GtkWidget       *typew, *filenw, *fd2w;
};

static void  rtype_turn(struct addredirstr *rdata)
{
        int     act = gtk_combo_box_get_active(GTK_COMBO_BOX(rdata->typew));
        if  (act < 0)
                return;
        act += RD_ACT_RD;
        if  (act >= RD_ACT_CLOSE)  {
                gtk_widget_set_sensitive(rdata->filenw, FALSE);
                if  (act == RD_ACT_DUP)
                        gtk_widget_set_sensitive(rdata->fd2w, TRUE);
                else
                        gtk_widget_set_sensitive(rdata->fd2w, FALSE);
        }
        else  {
                gtk_widget_set_sensitive(rdata->filenw, TRUE);
                gtk_widget_set_sensitive(rdata->fd2w, FALSE);
        }
}

static void  add_redir(struct aerdata *adata)
{
        GtkWidget       *dlg, *hbox, *fd1w;
        GtkAdjustment   *adj;
        GtkTreeIter     iter;
        gboolean        valid;
        int             possfd = 0;
        unsigned        cnt;
        struct  addredirstr     rdata;

        for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(adata->alist_store), &iter);
              valid;
              valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(adata->alist_store), &iter))  {
                gint    fd1;
                gtk_tree_model_get(GTK_TREE_MODEL(adata->alist_store), &iter, REDIR_FD_COL, &fd1, -1);
                if  (fd1 >= possfd)
                        possfd = fd1+1;
        }

        if  (possfd >= Max_files)
                possfd = 0;

        dlg = gprompt_dialog(toplevel, $P{xbtq redir add dlgtit});

        /* File descriptor and action */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);

        adj = (GtkAdjustment *) gtk_adjustment_new(possfd, 0, Max_files-1, 1, 10, 0);
        fd1w = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), fd1w, FALSE, FALSE, DEF_DLG_HPAD);
        rdata.typew = gtk_combo_box_new_text();
        gtk_box_pack_start(GTK_BOX(hbox), rdata.typew, FALSE, FALSE, DEF_DLG_HPAD);
        for  (cnt = 0;  cnt < actnames->numalt;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(rdata.typew), actnames->list[cnt]);
        gtk_combo_box_set_active(GTK_COMBO_BOX(rdata.typew), 0);
        g_signal_connect_swapped(G_OBJECT(rdata.typew), "changed", G_CALLBACK(rtype_turn), (gpointer) &rdata);

        /* Filename and 2nd fd */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        rdata.filenw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), rdata.filenw, FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new(possfd, 0, Max_files-1, 1, 10, 0);
        rdata.fd2w = gtk_spin_button_new(adj, 0.1, 0);
        gtk_box_pack_start(GTK_BOX(hbox), rdata.fd2w, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_widget_set_sensitive(rdata.fd2w, FALSE);
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *newfn = "";
                gchar   *newact;
                int     fd1, fd2 = 0, rtype;
                gboolean  fd2_sens = FALSE, fn_sens = TRUE;

                GtkTreeIter  iter;
                newact = gtk_combo_box_get_active_text(GTK_COMBO_BOX(rdata.typew));
                if  (!newact)
                        continue;
                rtype = lookup_alt(newact, actnames);
                if  (rtype < 0)  {
                        g_free(newact);
                        continue;
                }
                fd1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fd1w));
                if  (rtype >= RD_ACT_CLOSE)  {
                        fn_sens = FALSE;
                        if  (rtype == RD_ACT_DUP)  {
                                fd2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(rdata.fd2w));
                                fd2_sens = TRUE;
                        }
                }
                else  {
                        newfn = gtk_entry_get_text(GTK_ENTRY(rdata.filenw));

                        if  (strlen(newfn) == 0)  {
                                doerror($EH{xmbtq null fill name});
                                g_free(newact);
                                continue;
                        }
                }

                gtk_list_store_append(adata->alist_store, &iter);
                gtk_list_store_set(adata->alist_store, &iter,
                                   REDIR_FD_COL,        fd1,
                                   REDIR_TYPE_COL,      newact,
                                   REDIR_FILENAME_COL,  newfn,
                                   REDIR_FILENAME_SENS_COL,     fn_sens,
                                   REDIR_FD2_COL,       fd2,
                                   REDIR_FD2_SENS_COL,  fd2_sens,
                                   -1);
                g_free(newact);
                break;
        }
        gtk_widget_destroy(dlg);
}

static void  free_copy_redirs(Mredir *copyredirs, const int nredirs)
{
        if  (copyredirs)  {
                Mredir  *cr;
                for  (cr = copyredirs;  cr < &copyredirs[nredirs];  cr++)  {
                        if  (cr->action < RD_ACT_CLOSE)
                                free(cr->un.buffer);
                }
                free((char *) copyredirs);
        }
}

#ifdef IN_XBTR
void  cb_redir(GtkAction *action)
{
        struct  pend_job  *pj = job_or_deflt(action);
        GtkWidget  *dlg, *scroll, *hbox, *butt;
        GtkCellRenderer *rend;
        GtkAdjustment   *adj;
        GtkTreeViewColumn *col;
        GtkListStore    *rdtype_store;
        struct  aerdata  adata;
        unsigned        cnt;

        if  (!pj)
                return;

        adata.cjob = *pj->job;
        dlg = start_jobdlg(pj, $P{xbtq job redirs dlgtit}, $P{xbtq job redirs lab});
#else
void  cb_redir()
{
        BtjobRef  cj = getselectedjob(BTM_WRITE);
        GtkWidget  *dlg, *scroll, *hbox, *butt;
        GtkCellRenderer *rend;
        GtkAdjustment   *adj;
        GtkTreeViewColumn *col;
        GtkListStore    *rdtype_store;
        struct  aerdata  adata;
        unsigned        cnt;

        if  (!cj)
                return;

        adata.cjob = *cj;
        dlg = start_jobdlg(cj, $P{xbtq job redirs dlgtit}, $P{xbtq job redirs lab});
#endif

        gtk_window_set_default_size(GTK_WINDOW(dlg), DEFAULT_REDIRDLG_WIDTH, DEFAULT_REDIRDLG_HEIGHT);

        /* Create "main" list store */

        adata.alist_store = gtk_list_store_new(6, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_BOOLEAN);
        for  (cnt = 0;  cnt < adata.cjob.h.bj_nredirs;  cnt++)  {
                RedirRef  rp = REDIR_OF((&adata.cjob), cnt);
                gboolean  sens_str = TRUE, sens_int = FALSE;
                const   char    *filen = "";
                int     intarg = 0;
                GtkTreeIter   iter;

                if  (rp->action >= RD_ACT_CLOSE)  {
                        sens_str = FALSE;
                        if  (rp->action == RD_ACT_DUP)  {
                                sens_int = TRUE;
                                intarg = rp->arg;
                        }
                }
                else
                        filen = &adata.cjob.bj_space[rp->arg];

                gtk_list_store_append(adata.alist_store, &iter);
                gtk_list_store_set(adata.alist_store, &iter,
                                   REDIR_FD_COL,         (int) rp->fd,
                                   REDIR_TYPE_COL,      disp_alt(rp->action, actnames),
                                   REDIR_FILENAME_COL,  filen,
                                   REDIR_FILENAME_SENS_COL,     sens_str,
                                   REDIR_FD2_COL,       intarg,
                                   REDIR_FD2_SENS_COL,  sens_int,
                                   -1);
        }

        /* Create tree view widget */

        adata.awid = gtk_tree_view_new();
        gtk_tree_view_set_reorderable(GTK_TREE_VIEW(adata.awid), TRUE);
        gtk_tree_view_set_model(GTK_TREE_VIEW(adata.awid), GTK_TREE_MODEL(adata.alist_store));
        g_object_unref(G_OBJECT(adata.alist_store));

        /* Create spin box for first column, the file descriptor */

        adj = (GtkAdjustment *) gtk_adjustment_new(0, 0, Max_files-1, 1, 10, 0);
        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "FD");
        rend = gtk_cell_renderer_spin_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_redir_fd_changed), &adata);
        g_object_set(G_OBJECT(rend), "adjustment", adj, "editable", TRUE, NULL);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", REDIR_FD_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Now set up combo box for second column, the type of redirection */

        rdtype_store = gtk_list_store_new(1, G_TYPE_STRING);
        for  (cnt = 0;  cnt < actnames->numalt;  cnt++)  {
                GtkTreeIter   iter;
                gtk_list_store_append(rdtype_store, &iter);
                gtk_list_store_set(rdtype_store, &iter, 0, actnames->list[cnt], -1);
        }

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Type");
        rend = gtk_cell_renderer_combo_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_redir_type_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "model", GTK_TREE_MODEL(rdtype_store), "text-column", 0, "editable", TRUE, "has_entry", FALSE, NULL);
        g_object_unref(G_OBJECT(rdtype_store));
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", REDIR_TYPE_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Text box for third column */

        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "File/prog");
        rend = gtk_cell_renderer_text_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_redir_file_changed), (gpointer) &adata);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", REDIR_FILENAME_COL, "sensitive", REDIR_FILENAME_SENS_COL, "editable", REDIR_FILENAME_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Fourth column for 2nd fd */

        adj = (GtkAdjustment *) gtk_adjustment_new(0, 0, Max_files-1, 1, 10, 0);
        col = gtk_tree_view_column_new();
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_title(col, "Dup fd");
        rend = gtk_cell_renderer_spin_new();
        g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(cb_redir_dup_fd_changed), (gpointer) &adata);
        g_object_set(G_OBJECT(rend), "adjustment", adj, "editable", TRUE, NULL);
        gtk_tree_view_column_pack_start(col, rend, TRUE);
        gtk_tree_view_column_set_attributes(col, rend, "text", REDIR_FD2_COL, "sensitive", REDIR_FD2_SENS_COL, "editable", REDIR_FD2_SENS_COL, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(adata.awid), col);

        /* Now scroll the stuff */

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), adata.awid);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, FALSE, FALSE, DEF_DLG_VPAD);

        hbox = gtk_hbox_new(TRUE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD, GTK_PACK_START);
        gtk_box_set_child_packing(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, DEF_DLG_VPAD, GTK_PACK_START);

        butt = gprompt_button($P{xbtq add redir});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(add_redir), (gpointer) &adata);
        butt = gprompt_button($P{xbtq del redir});
        gtk_box_pack_start(GTK_BOX(hbox), butt, FALSE, FALSE, 0);
        g_signal_connect_swapped(G_OBJECT(butt), "clicked", G_CALLBACK(del_arg), (gpointer) &adata); /* Did mean del_arg */
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                Mredir  *copyredirs = 0;
                int             nredirs = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(adata.alist_store),  NULL);
#ifdef IN_XBTR
                Btjob           newcopy;
#else
                ULONG           xindx;
                unsigned        retc;
                BtjobRef        bjp;
#endif

                if  (nredirs > 0)  {
                        Mredir  *cr;
                        gboolean valid;
                        GtkTreeIter  iter;
                        copyredirs = (Mredir *) malloc(nredirs * sizeof(Mredir));
                        if  (!copyredirs)
                                ABORT_NOMEM;
                        cr = copyredirs;

                        for  (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(adata.alist_store), &iter);
                              valid;
                              valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(adata.alist_store), &iter))  {
                                gchar   *typestr, *filestr;
                                gint    fd1, fd2;
                                int     rtype;

                                gtk_tree_model_get(GTK_TREE_MODEL(adata.alist_store), &iter,
                                                   REDIR_FD_COL,        &fd1,
                                                   REDIR_FD2_COL,       &fd2,
                                                   REDIR_TYPE_COL,      &typestr,
                                                   REDIR_FILENAME_COL,  &filestr,
                                                   -1);
                                cr->fd = fd1;

                                rtype = lookup_alt(typestr, actnames);
                                if  (rtype <= 0)  {
                                        /* This cannot happen */
                                        g_free(typestr);
                                        g_free(filestr);
                                        nredirs = cr - copyredirs;
                                        break;
                                }
                                cr->action = rtype;
                                g_free(typestr);

                                if  (rtype < RD_ACT_CLOSE)
                                        cr->un.buffer = stracpy(filestr);
                                else
                                        cr->un.arg = fd2;

                                g_free(filestr);
                                cr++;
                        }
                }

#ifdef IN_XBTR
                newcopy = adata.cjob;
                if  (!repackjob(&newcopy, &adata.cjob, (char *) 0, (char *) 0, nredirs, 0, 0, copyredirs, (Menvir *) 0, (char **) 0))  {
                        disp_arg[3] = adata.cjob.h.bj_job;
                        disp_str = title_of(&adata.cjob);
                        doerror($EH{Too many job strings});
                        free_copy_redirs(copyredirs, nredirs);
                        continue;
                }
                free_copy_redirs(copyredirs, nredirs);
                *pj->job = newcopy;
                note_changes(pj);
#else
                /* Allocate xfer buffer and do the bizniz */

                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = adata.cjob;
                if  (!repackjob(bjp, &adata.cjob, (char *) 0, (char *) 0, nredirs, 0, 0, copyredirs, (Menvir *) 0, (char **) 0))  {
                        disp_arg[3] = adata.cjob.h.bj_job;
                        disp_str = qtitle_of(&adata.cjob);
                        doerror($EH{Too many job strings});
                        freexbuf(xindx);
                        free_copy_redirs(copyredirs, nredirs);
                        continue;
                }
                free_copy_redirs(copyredirs, nredirs);
                wjmsg(J_CHANGE, xindx);
                retc = readreply();
                if  (retc != J_OK)
                        qdojerror(retc, bjp);
                freexbuf(xindx);
#endif
                break;
        }
        gtk_widget_destroy(dlg);
}

#ifndef IN_XBTR

/* Set up and run a dialog to get a new filename.
   We don't want to create directories as we'd be doing this as thr wrong user */

static  char    *unqueue_fileselect(char *existing, const int dlgpr, const int errmsg)
{
         GtkWidget *fsw;
         char    *pr = gprompt(dlgpr);

         fsw = gtk_file_selection_new(pr);
         free(pr);
         gtk_file_selection_set_filename(GTK_FILE_SELECTION(fsw), existing);
         gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsw));
         while  (gtk_dialog_run(GTK_DIALOG(fsw)) == GTK_RESPONSE_OK)  {
                 const char *newfile = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fsw));
                 const char *sp = strrchr(newfile, '/');
                 char   *result;
                 if  (!sp)  {
                         doerror($EH{xmbtq unqueue no dir});
                         continue;
                 }
                 if  (strlen(sp) <= 1)  {
                         doerror(errmsg);
                         continue;
                 }
                 result = stracpy(newfile);
                 gtk_widget_destroy(fsw);
                 return  result;
         }
         gtk_widget_destroy(fsw);
         return  (char *) 0;
}

/* Make ourselves a full filename out of a directory name widget and a file one */

static  char    *get_existing(GtkWidget *dirw, GtkWidget *fw)
{
        const  char  *dirn;
        const  char  *filen = gtk_label_get_text(GTK_LABEL(fw));
        GString  *fullp;
        char    *result;

        if  (filen[0] == '/')                   /* Cope with it being abs path */
                return  stracpy(filen);
        dirn = gtk_label_get_text(GTK_LABEL(dirw));

        fullp = g_string_new(NULL);
        g_string_assign(fullp, dirn);
        if  (strlen(dirn) > 1)
                g_string_append_c(fullp, '/');
        g_string_append(fullp, filen);
        /* Make our own copy of the string as if we return with fullp->str the blurb says
           this must be freed with g_free and we want to be consistent */
        result = stracpy(fullp->str);
        g_string_free(fullp, TRUE);
        return  result;
}

struct  unqueue_data  {
        GtkWidget  *dirsw;
        GtkWidget  *cfw;
        GtkWidget  *jfw;
};

/* Make a label button we can click on */

static  GtkWidget  *clickable_label(GtkWidget *dlg, const int labprompt, GString *initlabel, void (*cbfunc)(struct unqueue_data *), struct unqueue_data *ud)
{
        GtkWidget  *hbox, *lab, *button, *result;

        /* Set up hbox to accommodate description and button, add to dlg get label, add that */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label(labprompt);
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        /* Result is a label which we put in a button, deallocate the initial label */

        result = gtk_label_new(initlabel->str);
        g_string_free(initlabel, TRUE);

        /* Generate button, add it to the hbox (after the description) */

        button = gtk_button_new();
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

        /* Now we create another hbox to put inside the button and put our label in that */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        gtk_box_pack_start(GTK_BOX(hbox), result, FALSE, FALSE, DEF_BUTTON_PAD);

        g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(cbfunc), (gpointer) ud);
        return  result;
}

/* Select XML job file name */

static  void  xml_job_sel(struct unqueue_data *unqd)
{
        char    *current_jname = get_existing(unqd->dirsw, unqd->jfw);
        char    *newname = unqueue_fileselect(current_jname, $P{xbtq unqueue job dlg title}, $EH{xmbtq no job file name});
        char    *sp, *newd, *newf;
        int     lng;

        if  (!newname)  {
                free(current_jname);
                return;
        }

        if  (strcmp(current_jname, newname) == 0)  {
                free(current_jname);
                free(newname);
                return;
        }

        free(current_jname);
        if  (!(sp = strrchr(newname, '/')))  {
                newd = Curr_pwd;
                newf = newname;
        }
        else  {
                newf = sp+1;
                newd = newname;
                *sp = '\0';
                if  (sp == newname)
                        newd = "/";
        }

        gtk_label_set_text(GTK_LABEL(unqd->dirsw), newd);

        /* Insist on suffix for XML files */

        lng = strlen(newf);
        if  (lng < sizeof(XMLJOBSUFFIX)  ||  ncstrcmp(newf + lng - sizeof(XMLJOBSUFFIX) + 1, XMLJOBSUFFIX) == 0)  {
                GString *nf = g_string_new(newf);
                g_string_append(nf, XMLJOBSUFFIX);
                gtk_label_set_text(GTK_LABEL(unqd->jfw), nf->str);
                g_string_free(nf, TRUE);
        }
        else
                gtk_label_set_text(GTK_LABEL(unqd->jfw), newf);
        free(newname);
}

static  void  reset_jcmd(struct unqueue_data *unqd, char *cname, char *jname)
{
        int     preflen = 1;
        char    *dir;

        for  (;;)  {
                char    *cseg = cname + preflen, *jseg = jname + preflen;
                char    *csp, *jsp;
                int     csl, jsl;
                if  (!(csp = strchr(cseg, '/')))
                        break;
                if  (!(jsp = strchr(jseg, '/')))
                        break;
                csl = csp - cseg;
                jsl = jsp - jseg;
                if  (csl != jsl)
                        break;
                if  (strncmp(cseg, jseg, csl) != 0)
                        break;
                preflen += csl + 1;
        }

        if  (preflen <= 1)
                dir = "/";
        else  {
                cname[preflen-1] = '\0';
                dir = cname;
        }
        cname += preflen;
        jname += preflen;
        gtk_label_set_text(GTK_LABEL(unqd->dirsw), dir);
        gtk_label_set_text(GTK_LABEL(unqd->cfw), cname);
        gtk_label_set_text(GTK_LABEL(unqd->jfw), jname);
}

static  void  cmd_sel(struct unqueue_data *unqd)
{
        char    *current_cname = get_existing(unqd->dirsw, unqd->cfw);
        char    *newname = unqueue_fileselect(current_cname, $P{xbtq unqueue cmd dlg title}, $EH{xmbtq no cmd file name});
        char    *current_jname;

        if  (!newname)  {
                free(current_cname);
                return;
        }

        if  (strcmp(current_cname, newname) == 0)  {
                free(current_cname);
                free(newname);
                return;
        }

        free(current_cname);
        current_jname = get_existing(unqd->dirsw, unqd->jfw);

        reset_jcmd(unqd, newname, current_jname);
        free(current_jname);
        free(newname);
}

static  void  job_sel(struct unqueue_data *unqd)
{
        char    *current_jname = get_existing(unqd->dirsw, unqd->jfw);
        char    *newname = unqueue_fileselect(current_jname, $P{xbtq unqueue job dlg title}, $EH{xmbtq no job file name});
        char    *current_cname;

        if  (!newname)  {
                free(current_jname);
                return;
        }

        if  (strcmp(current_jname, newname) == 0)  {
                free(current_jname);
                free(newname);
                return;
        }

        free(current_jname);
        current_cname = get_existing(unqd->dirsw, unqd->cfw);

        reset_jcmd(unqd, current_cname, newname);
        free(current_cname);
        free(newname);
}

void  cb_unqueue()
{
        GtkWidget  *dlg, *hbox, *lab, *copyonly, *verb = 0, *cantype = 0;
        char    *pr;
        GString *labp;
        BtjobRef  cj = getselectedjob(BTM_READ);                /* Allow selection of jobs for copying only */
        struct  unqueue_data    unqd;

        if  (!cj)
                return;

        if  (!Last_unqueue_dir)
                Last_unqueue_dir = stracpy(Curr_pwd);

        dlg = gprompt_dialog(toplevel, $P{xbtq unqueue dialogtit});

        /* Say what job we are unqueuing */

        pr = gprompt($P{xbtq unqueuing job});
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %s", pr, JOB_NUMBER(cj));
        free(pr);
        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), lab, FALSE, FALSE, LAB_PADDING);

        /* Checkbox about copy only no delete on separate line.
           If user can't delete job force on the setting and don't
           let it be changed. */

        copyonly = gprompt_checkbutton($P{xbtq copy no delete});
        if  (!mpermitted(&cj->h.bj_mode, BTM_DELETE, mypriv->btu_priv))  {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(copyonly), TRUE);
                gtk_widget_set_sensitive(copyonly, FALSE);
        }
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), copyonly, FALSE, FALSE, DEF_DLG_VPAD);

        /* Display line with directory */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xbtq unqueue directory});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);
        unqd.dirsw = gtk_label_new(Last_unqueue_dir);
        gtk_box_pack_start(GTK_BOX(hbox), unqd.dirsw, FALSE, FALSE, DEF_DLG_HPAD);

        /* Now it depends if we're just doing one XML file or legacy version */

        if  (xml_format)  {
                int     cnt, sel;

                /* Initialise button text with job number, prefix and standard suffix for XML files */

                pr = gprompt($P{Default job file prefix});
                labp = g_string_new(NULL);
                g_string_printf(labp, "%s%ld" XMLJOBSUFFIX, pr, (long) cj->h.bj_job);
                free(pr);

                unqd.jfw = clickable_label(dlg, $P{xbtq job file name}, labp, xml_job_sel, &unqd);

                /* Checkbox for verbose and combo box for force cancelled/runnable */

                verb = gprompt_checkbutton($P{xbtq unq force verbose});
                gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), verb, FALSE, FALSE, DEF_DLG_VPAD);

                hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
                gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
                lab = gprompt_label($P{xbtq unq setcanc lab});
                gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

                if  (!cancalts)  {
                        cancalts = helprdalt($Q{xbtq unq setcanc});
                        if  (!cancalts)  {
                                disp_arg[9] = $Q{xbtq unq setcanc};
                                doerror($EH{Missing alternative code});
                                return;
                        }
                }

                cantype = gtk_combo_box_new_text();
                for  (cnt = 0;  cnt < cancalts->numalt;  cnt++)
                        gtk_combo_box_append_text(GTK_COMBO_BOX(cantype), cancalts->list[cnt]);
                sel = 0;
                if  (cancalts->def_alt > 0)                             /* Let user set a default if he wants */
                        sel = cancalts->def_alt;
                gtk_combo_box_set_active(GTK_COMBO_BOX(cantype), sel);
                gtk_box_pack_start(GTK_BOX(hbox), cantype, FALSE, FALSE, DEF_DLG_HPAD);
        }
        else  {

                /* Legacy style with shell script file and job file
                   Kick off by setting up line with a default shell script file. */

                labp = g_string_new(NULL);
                pr = gprompt($P{Default cmd file prefix});
                g_string_printf(labp, "%s%ld", pr, (long) cj->h.bj_job);
                free(pr);

                /* Use helpprmpt because that returns null if no suffix */

                pr = helpprmpt($P{Default cmd file suffix});
                if  (pr)  {
                        g_string_append(labp, pr);
                        free(pr);
                }
                unqd.cfw = clickable_label(dlg, $P{xbtq shell script}, labp, cmd_sel, &unqd);

                /* Repeat that for job file name first label */

                labp = g_string_new(NULL);
                pr = gprompt($P{Default job file prefix});
                g_string_printf(labp, "%s%ld", pr, (long) cj->h.bj_job);
                free(pr);
                pr = helpprmpt($P{Default job file suffix});
                if  (pr)  {
                        g_string_append(labp, pr);
                        free(pr);
                }
                unqd.jfw = clickable_label(dlg, $P{xbtq job file name}, labp, job_sel, &unqd);
        }

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                struct  stat  sbuf;
                PIDTYPE pid;
                int     status;
                char    *wprog;
                int     conly = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(copyonly));

                if  (xml_format)  {
                        const  char  *newdir = gtk_label_get_text(GTK_LABEL(unqd.dirsw));
                        const  char  *newjf = gtk_label_get_text(GTK_LABEL(unqd.jfw));
                        int     vv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verb));
                        int     ccan = gtk_combo_box_get_active(GTK_COMBO_BOX(cantype));

                        if  (newdir[0] == '\0')  {
                                doerror($EH{xmbtq unqueue no dir});
                                continue;
                        }
                        if  (newdir[0] != '/')  {
                                disp_str = newdir;
                                doerror($EH{xmbtq unqueue dir not abs});
                                continue;
                        }
                        if  (stat(newdir, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                                disp_str = newdir;
                                doerror($EH{xmbtq unq not dir});
                                continue;
                        }
                        if  (newjf[0] == '\0')  {
                                doerror($EH{xmbtq no job file name});
                                continue;
                        }

                        if  (!execprog)
                                execprog = envprocess(EXECPROG);

                        if  (!xmludprog)
                                xmludprog = envprocess(XMLDUMPJOB);

                        wprog = xmludprog;              /* What to moan about if we have an error */

                        /* Remember last unqueue directory for next time */

                        if  (strcmp(newdir, Last_unqueue_dir) != 0)  {
                                free(Last_unqueue_dir);
                                Last_unqueue_dir = stracpy(newdir);
                        }

                        if  ((pid = fork()) == 0)  {
                                char    *argbuf[12];
                                char    **ap = argbuf;

                                *ap++ = xmludprog;
                                 if  (conly)
                                         *ap++ = "-n";
                                 if  (vv)
                                         *ap++ = "-v";
                                 if  (ccan > 0)
                                         *ap++ = ccan > 1? "-C": "-N";
                                 *ap++ = (char *) "-j";
                                 *ap++ = (char *) JOB_NUMBER(cj);
                                 *ap++ = (char *) "-d";
                                 *ap++ = Last_unqueue_dir;
                                 *ap++ = (char *) "-f";
                                 *ap++ = (char *) newjf;
                                 *ap = (char *) 0;
                                 execv(execprog, (char **) argbuf);
                                 exit(E_SETUP);
                        }
                }
                else  {
                        const  char  *newdir = gtk_label_get_text(GTK_LABEL(unqd.dirsw));
                        const  char  *newcf = gtk_label_get_text(GTK_LABEL(unqd.cfw));
                        const  char  *newjf = gtk_label_get_text(GTK_LABEL(unqd.jfw));
                        struct  stat    sbuf;

                        if  (!execprog)
                                 execprog = envprocess(EXECPROG);

                        if  (!udprog)
                                 udprog = envprocess(DUMPJOB);
                        wprog = udprog;                                 /* So we know what to moan about */

                        if  (newdir[0] == '\0')  {
                                 doerror($EH{xmbtq unqueue no dir});
                                 continue;
                        }
                        if  (newdir[0] != '/')  {
                                disp_str = newdir;
                                doerror($EH{xmbtq unqueue dir not abs});
                                continue;
                        }
                        if  (stat(newdir, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                                disp_str = newdir;
                                doerror($EH{xmbtq unq not dir});
                                continue;
                        }
                        if  (newcf[0] == '\0')  {
                                doerror($EH{xmbtq no cmd file name});
                                return;
                        }

                        /* Remember last unqueue directory for next time */

                        if  (strcmp(newdir, Last_unqueue_dir) != 0)  {
                                free(Last_unqueue_dir);
                                Last_unqueue_dir = stracpy(newdir);
                        }

                        if  ((pid = fork()) == 0)  {
                                char    *argbuf[8];
                                char    **ap = argbuf;

                                /* Child process */

                                Ignored_error = chdir(Curr_pwd);        /* So it picks up the right config file */

                                *ap++ = udprog;
                                if  (conly)
                                        *ap++ = "-n";
                                *ap++ = (char *) JOB_NUMBER(cj);
                                *ap++ = Last_unqueue_dir;
                                *ap++ = (char *) newcf;
                                *ap++ = (char *) newjf;
                                *ap = (char *) 0;
                                execv(execprog, (char **) argbuf);
                                exit(E_SETUP);
                        }
                }

                /* Main process ends up here */

                if  (pid < 0)  {
                        doerror($EH{xmbtq unqueue fork failed});
                        continue;
                }

#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                if  (status == 0)       /* All ok */
                        break;

                if  (status & 0xff)  {
                        disp_arg[9] = status & 0xff;
                        doerror(status & 0x80? $EH{xmbtq unqueue crashed}: $EH{xmbtq unqueue terminated});
                        continue;
                }

                status = (status >> 8) & 0xff;
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);

                switch  (status)  {
                default:
                        disp_arg[1] = status;
                        doerror($EH{xmbtq unqueue misc error});
                        continue;
                case  E_SETUP:
                        disp_str = wprog;
                        doerror($EH{xmbtq no unq process});
                        continue;
                case  E_JDFNFND:
                        doerror($EH{xmbtq unq dir not found});
                        continue;
                case  E_JDNOCHDIR:
                        doerror($EH{xmbtq cannot chdir});
                        continue;
                case  E_JDFNOCR:
                        doerror($EH{xmbtq cannot create cmdfile});
                        continue;
                case  E_JDJNFND:
                        doerror($EH{xmbtq unq job not found});
                        continue;
                case  E_CANTDEL:
                        doerror($EH{xmbtq unq cannot del});
                        continue;
                case  E_NOTIMPL:
                        doerror($EH{No XML library});
                        continue;
                }
        }

        /* End of dialog */

        gtk_widget_destroy(dlg);
}


void  cb_freeze(GtkAction *action)
{
        GtkWidget       *dlg, *verbw, *cancw;
        BtjobRef        jp = getselectedjob(BTM_READ);
        int             ishomed = strcmp(gtk_action_get_name(action), "Freeeh") == 0;

        if  (!jp)
                return;

        dlg = start_jobdlg(jp, ishomed? $P{xbtq job freezeh dlgtit}: $P{xbtq job freezec dlgtit}, $P{xbtq job freeze lab});
        verbw = gprompt_checkbutton($P{xbtq freeze verbose setting});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), verbw, FALSE, FALSE, DEF_DLG_VPAD);
        cancw = gprompt_checkbutton($P{xbtq freeze canc setting});
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cancw), TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), cancw, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_widget_show_all(dlg);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                PIDTYPE pid;
                char    abuf[3];

                if  (!execprog)
                        execprog = envprocess(EXECPROG);
                if  (!udprog)
                        udprog = envprocess(DUMPJOB);

                abuf[0] = '-';
                abuf[2] = '\0';
                abuf[1] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cancw))? 'C': 'N';
                if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verbw)))
                        abuf[1] = tolower(abuf[1]);

                if  ((pid = fork()))  {
                        int     status;

                        if  (pid < 0)  {
                                doerror($EH{xmbtq unqueue fork failed});
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
                                if  (status & 0xff)  {
                                        disp_arg[0] = status & 0x7f;
                                        doerror(status & 0x80? $EH{xmbtq unqueue crashed}: $EH{xmbtq unqueue terminated});
                                }
                                status = (status >> 8) & 0xff;
                                disp_arg[0] = jp->h.bj_job;
                                disp_str = qtitle_of(jp);
                                doerror($EH{xmbtq unq cannot create config file});
                        }
                }
                else  {

                        const  char  *argv[5], **ap;

                        /* Child process */

                        Ignored_error = chdir(Curr_pwd);        /* So it picks up the right config file */
                        ap = argv;
                        *ap++ = udprog;
                        *ap++ = abuf;
                        *ap++ = ishomed? "~": Curr_pwd;
                        *ap++ = JOB_NUMBER(jp);
                        *ap = (char *) 0;
                        execv(execprog, (char **) argv);
                        exit(E_SETUP);
                }
        }
        gtk_widget_destroy(dlg);
}

void  cb_createj()
{
        PIDTYPE pid;

        if  (!execprog)
                execprog = envprocess(EXECPROG);

        if  ((pid = fork()) != 0)  {
                int     status;

                if  (pid < 0)
                        return;

#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
        }
        else  {
                Ignored_error = chdir(Curr_pwd);        /* So it picks up the right config file */
                execl(execprog, XBTR_PROGRAM, (char *) 0);
                exit(E_SETUP);
        }
}
#endif /* ! IN_XBTR */
