/* xbtu_cbs.c -- callbacks for gbch-xuser

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
#include "defaults.h"
#include "btmode.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "files.h"
#include "gtk_lib.h"
#include "gtk_modebits.h"
#include "xbtu_ext.h"

static  char    Filename[] = __FILE__;

#define DLG_PRI_PAGE    0
#define DLG_LL_PAGE     1
#define DLG_JMODE_PAGE  2
#define DLG_VMODE_PAGE  3
#define DLG_PRIV_PAGE   4

#define DEF_DLG_VPAD    5
#define DEF_DLG_HPAD    5
#define LAB_PADDING     10
#define NOTE_PADDING    10

#define MAX_UDISP       3

/* Positions (in the motif sense) of users we are thinking of mangling
   If null/zero then we are looking at the default list.  */

static  unsigned        *pendulist;
static  int             pendunum;

#define UGO_NUM         3

struct  dialog_data  {
        GtkWidget  *minpri, *defpri, *maxpri;
        GtkWidget  *maxll, *totll, *specll;
        GtkWidget  *jmodes[UGO_NUM][NUM_JMODEBITS];
        GtkWidget  *vmodes[UGO_NUM][NUM_VMODEBITS];
        GtkWidget  *privs[NUM_PRIVBITS];
};

static int  getselectedusers(const int moan)
{
        GtkTreeSelection *selection;

        /*  Free up last time's */

        if  (pendulist)  {
                free((char *) pendulist);
                pendulist = NULL;
        }

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(uwid));

        if  ((pendunum = gtk_tree_selection_count_selected_rows(selection)) > 0)  {
                GList *pu = gtk_tree_selection_get_selected_rows(selection, NULL);
                GList *nxt;
                unsigned  *n;
                n = pendulist = malloc((unsigned)(pendunum * sizeof(unsigned *)));
                if  (!pendulist)
                        ABORT_NOMEM;
                for  (nxt = g_list_first(pu);  nxt;  nxt = g_list_next(nxt))  {
                        GtkTreeIter  iter;
                        unsigned  indx;
                        if  (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ulist_store), &iter, (GtkTreePath *)(nxt->data)))
                                continue;
                        gtk_tree_model_get(GTK_TREE_MODEL(ulist_store), &iter, INDEX_COL, &indx, -1);
                        *n++ = indx;
                }
                g_list_foreach(pu, (GFunc) gtk_tree_path_free, NULL);
                g_list_free(pu);
                return  1;
        }
        if  (moan)
                doerror($EH{xmbtuser no users selected});
        return  0;
}

GtkWidget *create_pri_dialog(struct dialog_data *ddata, const unsigned minp, const unsigned defp, const unsigned maxp)
{
        GtkWidget  *frame, *vbox, *hbox, *vbox2, *lab;
        char    *pr;

        pr = gprompt($P{xbtuser framelab pris});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD*3);
        vbox2 = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);

        lab = gprompt_label($P{xbtuser pdlg minp});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        lab = gprompt_label($P{xbtuser pdlg defp});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        lab = gprompt_label($P{xbtuser pdlg maxp});
        gtk_box_pack_start(GTK_BOX(vbox2), lab, TRUE, FALSE, 0);

        vbox2 = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
        ddata->minpri = gtk_hscale_new_with_range(1.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->minpri), 0);
        gtk_range_set_value(GTK_RANGE(ddata->minpri), (gdouble) minp);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->minpri, TRUE, TRUE, 0);
        ddata->defpri = gtk_hscale_new_with_range(1.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->defpri), 0);
        gtk_range_set_value(GTK_RANGE(ddata->defpri), (gdouble) defp);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->defpri, TRUE, TRUE, 0);
        ddata->maxpri = gtk_hscale_new_with_range(1.0, 255.0, 1.0);
        gtk_scale_set_digits(GTK_SCALE(ddata->maxpri), 0);
        gtk_range_set_value(GTK_RANGE(ddata->maxpri), (gdouble) maxp);
        gtk_box_pack_start(GTK_BOX(vbox2), ddata->maxpri, TRUE, TRUE, 0);
        return  frame;
}

GtkWidget *create_ll_dialog(struct dialog_data *ddata, const unsigned maxll, const unsigned totll, const unsigned specll)
{
        GtkWidget *frame, *vbox;
        char    *pr;

        pr = gprompt($P{xbtuser framelab lls});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        ddata->maxll = slider_with_buttons(vbox, $P{xbtuser lldlg maxll}, maxll, 0);
        ddata->totll = slider_with_buttons(vbox, $P{xbtuser lldlg totll}, totll, 0);
        ddata->specll = slider_with_buttons(vbox, $P{xbtuser lldlg specll}, specll, 0);
        return  frame;
}

GtkWidget *create_jmode_dialog(struct dialog_data *ddata, USHORT *modes)
{
        GtkWidget *frame;
        char    *pr;

        pr = gprompt($P{xbtuser framelab jmode});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        setup_jmodebits(frame, ddata->jmodes, modes[0], modes[1], modes[2]);
        return  frame;
}

GtkWidget *create_vmode_dialog(struct dialog_data *ddata, USHORT *modes)
{
        GtkWidget *frame;
        char    *pr;

        pr = gprompt($P{xbtuser framelab vmode});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        setup_vmodebits(frame, ddata->vmodes, modes[0], modes[1], modes[2]);
        return  frame;
}

static GtkWidget *findprivwid(const ULONG priv)
{
        int     cnt;
        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)
                if  (privnames[cnt].priv_flag & priv)
                        return  privnames[cnt].priv_widget;
        return  0;
}

void  priv_flip(GtkWidget *src, gpointer privnum)
{
        struct  privabbrev  *pp = &privnames[GPOINTER_TO_INT(privnum)];

        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(src)))  {
                if  (pp->priv_flag & BTM_SPCREATE)
                        setifnotset(findprivwid(BTM_CREATE));
                else  if  (pp->priv_flag & BTM_WADMIN)  {
                        int  cnt;
                        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)
                                setifnotset(privnames[cnt].priv_widget);
                }
        }
        else  {
                if  (pp->priv_flag & BTM_CREATE)
                        unsetifset(findprivwid(BTM_SPCREATE));
                else  if  (pp->priv_flag & (BTM_UMASK|BTM_RADMIN))
                        unsetifset(findprivwid(BTM_WADMIN));
        }
}

void  priv_setdef(struct dialog_data *ddata)
{
        int     cnt;

        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)
                if  (Btuhdr.btd_priv & privnames[cnt].priv_flag)
                        setifnotset(ddata->privs[cnt]);
                else
                        unsetifset(ddata->privs[cnt]);
}

GtkWidget *create_priv_dialog(struct dialog_data *ddata, ULONG priv, const int isuser)
{
        GtkWidget  *frame, *vbox, *button;
        char    *pr;
        int     cnt;

        pr = gprompt($P{xbtuser framelab privs});
        frame = gtk_frame_new(pr);
        free(pr);
        gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 1.0);
        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)  {
                button = gtk_check_button_new_with_label(privnames[cnt].priv_name);
                gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
                if  (privnames[cnt].priv_flag & priv)
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
                ddata->privs[cnt] = button;
                privnames[cnt].priv_widget = button;
                if  (privnames[cnt].priv_flag & (BTM_SPCREATE|BTM_WADMIN|BTM_CREATE|BTM_UMASK|BTM_RADMIN))
                        g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(priv_flip), GINT_TO_POINTER(cnt));
        }

        if  (isuser)  {
                pr = gprompt($P{xbtuser priv setdef});
                button = gtk_button_new_with_label(pr);
                free(pr);
                g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(priv_setdef), ddata);
                gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
        }

        return  frame;
}

GtkWidget *user_lab(const int isusers, const char *msg)
{
        GtkWidget  *lab;
        int     cnt, mxu = pendunum;
        GString *buf;

        if  (mxu > MAX_UDISP)
                mxu = MAX_UDISP;
        if  (!isusers)
                return gprompt_label($P{xbtuser deflab});

        buf = g_string_new(msg);
        for  (cnt = 0;  cnt < mxu;  cnt++)  {
                if  (cnt != 0)
                        g_string_append_c(buf, ',');
                g_string_append(buf, prin_uname(ulist[pendulist[cnt]].btu_user));
        }

        if  (mxu != pendunum)
                g_string_append(buf, "...");

        lab = gtk_label_new(buf->str);
        g_string_free(buf, TRUE);

        return  lab;
}

GtkWidget *create_dialog(struct dialog_data *ddata, const int wpage, const int isusers)
{
        GtkWidget  *dlg, *lab, *notebook, *pridlg, *lldlg, *jmdlg, *vmdlg, *privdlg;

        dlg = gprompt_dialog(toplevel, isusers? $P{xbtuser udlgtit}: $P{xbtuser ddlgtit});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), user_lab(isusers, "Editing user params: "), FALSE, FALSE, LAB_PADDING);
        notebook = gtk_notebook_new();
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

        if  (isusers)  {
                BtuserRef  up = &ulist[pendulist[0]];
                pridlg = create_pri_dialog(ddata, up->btu_minp, up->btu_defp, up->btu_maxp);
                lldlg = create_ll_dialog(ddata, up->btu_maxll, up->btu_totll, up->btu_spec_ll);
                jmdlg = create_jmode_dialog(ddata, up->btu_jflags);
                vmdlg = create_vmode_dialog(ddata, up->btu_vflags);
                privdlg = create_priv_dialog(ddata, up->btu_priv, 1);
        }
        else  {
                pridlg = create_pri_dialog(ddata, Btuhdr.btd_minp, Btuhdr.btd_defp, Btuhdr.btd_maxp);
                lldlg = create_ll_dialog(ddata, Btuhdr.btd_maxll, Btuhdr.btd_totll, Btuhdr.btd_spec_ll);
                jmdlg = create_jmode_dialog(ddata, Btuhdr.btd_jflags);
                vmdlg = create_vmode_dialog(ddata, Btuhdr.btd_vflags);
                privdlg = create_priv_dialog(ddata, Btuhdr.btd_priv, 0);
        }

        /* Set up tab names for notebook */

        lab = gprompt_label($P{xbtuser frametab pri});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), pridlg, lab);

        lab = gprompt_label($P{xbtuser frametab ll});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), lldlg, lab);

        lab = gprompt_label($P{xbtuser frametab jmode});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), jmdlg, lab);

        lab = gprompt_label($P{xbtuser frametab vmode});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vmdlg, lab);

        lab = gprompt_label($P{xbtuser frametab privs});
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privdlg, lab);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), notebook, TRUE, TRUE, NOTE_PADDING);
        gtk_widget_show_all(dlg);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), wpage);
        return  dlg;
}

ULONG  get_priv_from_widgets(struct dialog_data *ddata)
{
        ULONG  result = 0;
        int     cnt;

        for  (cnt = 0;  cnt < NUM_PRIVBITS;  cnt++)
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(privnames[cnt].priv_widget)))
                        result |= privnames[cnt].priv_flag;
        return  result;
}

static int  extract_ddata_defaults(struct dialog_data *ddata)
{
        int     minp, defp, maxp;
        unsigned  maxl, totl, specl;

        minp = gtk_range_get_value(GTK_RANGE(ddata->minpri));
        defp = gtk_range_get_value(GTK_RANGE(ddata->defpri));
        maxp = gtk_range_get_value(GTK_RANGE(ddata->maxpri));

        /* Check those make sense */
        if  (minp > maxp)  {
                doerror($EH{xmbtuser minp gt maxp});
                return  0;
        }
        if  (defp < minp)  {
                doerror($EH{xmbtuser defp lt minp});
                return  0;
        }
        if  (defp > maxp)  {
                doerror($EH{xmbtuser defp gt maxp});
                return  0;
        }

        maxl = gtk_range_get_value(GTK_RANGE(ddata->maxll));
        totl = gtk_range_get_value(GTK_RANGE(ddata->totll));
        specl = gtk_range_get_value(GTK_RANGE(ddata->specll));

        if  (maxl > totl)  {
                doerror($EH{xmbtuser maxll gt totll});
                return  0;
        }
        if  (specl > maxl)  {
                doerror($EH{xmbtuser specll gt maxll});
                return  0;
        }
        if  (specl > totl)  {
                doerror($EH{xmbtuser specll gt totll});
                return  0;
        }

        Btuhdr.btd_minp = minp;
        Btuhdr.btd_maxp = maxp;
        Btuhdr.btd_defp = defp;
        Btuhdr.btd_maxll =  maxl;
        Btuhdr.btd_totll = totl;
        Btuhdr.btd_spec_ll = specl;

        read_jmodes(ddata->jmodes, &Btuhdr.btd_jflags[UGO_USER], &Btuhdr.btd_jflags[UGO_GROUP], &Btuhdr.btd_jflags[UGO_OTHER]);
        read_vmodes(ddata->vmodes, &Btuhdr.btd_vflags[UGO_USER], &Btuhdr.btd_vflags[UGO_GROUP], &Btuhdr.btd_vflags[UGO_OTHER]);
        Btuhdr.btd_priv = get_priv_from_widgets(ddata);
        hchanges++;
        defdisplay();
        return  1;
}

static int  extract_ddata_users(struct dialog_data *ddata)
{
        int     minp, defp, maxp, cnt;
        unsigned  maxl, totl, specl;
        USHORT  jum, jgm, jom, vum, vgm, vom;
        ULONG   npriv;

        minp = gtk_range_get_value(GTK_RANGE(ddata->minpri));
        defp = gtk_range_get_value(GTK_RANGE(ddata->defpri));
        maxp = gtk_range_get_value(GTK_RANGE(ddata->maxpri));

        /* Check those make sense */
        if  (minp > maxp)  {
                doerror($EH{xmbtuser minp gt maxp});
                return  0;
        }
        if  (defp < minp)  {
                doerror($EH{xmbtuser defp lt minp});
                return  0;
        }
        if  (defp > maxp)  {
                doerror($EH{xmbtuser defp gt maxp});
                return  0;
        }

        maxl = gtk_range_get_value(GTK_RANGE(ddata->maxll));
        totl = gtk_range_get_value(GTK_RANGE(ddata->totll));
        specl = gtk_range_get_value(GTK_RANGE(ddata->specll));

        if  (maxl > totl)  {
                doerror($EH{xmbtuser maxll gt totll});
                return  0;
        }
        if  (specl > maxl)  {
                doerror($EH{xmbtuser specll gt maxll});
                return  0;
        }
        if  (specl > totl)  {
                doerror($EH{xmbtuser specll gt totll});
                return  0;
        }

        read_jmodes(ddata->jmodes, &jum, &jgm, &jom);
        read_vmodes(ddata->vmodes, &vum, &vgm, &vom);
        npriv = get_priv_from_widgets(ddata);

        uchanges++;

        for  (cnt = 0;  cnt < pendunum;  cnt++)  {
                BtuserRef  up = &ulist[pendulist[cnt]];
                up->btu_minp = minp;
                up->btu_maxp = maxp;
                up->btu_defp = defp;
                up->btu_maxll =  maxl;
                up->btu_totll = totl;
                up->btu_spec_ll = specl;
                up->btu_jflags[UGO_USER] = jum;
                up->btu_jflags[UGO_GROUP] = jgm;
                up->btu_jflags[UGO_OTHER] = jom;
                up->btu_vflags[UGO_USER] = vum;
                up->btu_vflags[UGO_GROUP] = vgm;
                up->btu_vflags[UGO_OTHER] = vom;
                up->btu_priv = npriv;
        }

        update_selected_users();
        return  1;
}

void    cb_options(const int isusers, const int wpage)
{
        struct  dialog_data     ddata;
        GtkWidget  *dlg;
        int     ret = 0;

        if  (isusers  &&  !getselectedusers(1))
                return;

        dlg = create_dialog(&ddata, wpage, isusers);
        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK  &&  !ret)  {
                if  (isusers)
                        ret = extract_ddata_users(&ddata);
                else
                        ret = extract_ddata_defaults(&ddata);
        }
        gtk_widget_destroy(dlg);
}

void  cb_pris(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_PRI_PAGE);
}

void  cb_loadlev(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_LL_PAGE);
}

void  cb_jmode(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_JMODE_PAGE);
}

void  cb_vmode(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_VMODE_PAGE);
}

void  cb_priv(GtkAction *action)
{
        cb_options(gtk_action_get_name(action)[0] != 'D', DLG_PRIV_PAGE);
}

static void  copyu(BtuserRef n)
{
        n->btu_defp = Btuhdr.btd_defp;
        n->btu_minp = Btuhdr.btd_minp;
        n->btu_maxp = Btuhdr.btd_maxp;
        n->btu_maxll = Btuhdr.btd_maxll;
        n->btu_totll = Btuhdr.btd_totll;
        n->btu_spec_ll = Btuhdr.btd_spec_ll;
}

void  cb_copyall()
{
        unsigned  cnt;
        for  (cnt = 0;  cnt < Npwusers;  cnt++)
                copyu(&ulist[cnt]);
        update_all_users();
        uchanges++;
}

void  cb_copydef()
{
        int     cnt;
        if  (!getselectedusers(1))
                return;
        for  (cnt = 0;  cnt < pendunum;  cnt++)
                copyu(&ulist[pendulist[cnt]]);
        update_selected_users();
        uchanges++;
}
