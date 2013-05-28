/* xfilemon.c -- main module for gbch-xfilemon

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
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <gtk/gtk.h>
#include "defaults.h"
#include "filemon.h"
#include "ecodes.h"
#include "cfile.h"
#include "incl_unix.h"
#include "helpargs.h"
#include "helpalt.h"
#include "statenums.h"
#include "errnums.h"
#include "files.h"
#include "gtk_lib.h"

#define DEF_DLG_VPAD    5
#define DEF_DLG_HPAD    5

static  char    Filename[] = __FILE__;

char    *Curr_pwd;

GtkWidget       *toplevel;

HelpaltRef      montypes, filetypes;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

#include "inline/btfmadefs.c"

char *gen_option(const int arg)
{
        int     v = arg - $A{btfilemon arg explain};

        if  (optvec[v].isplus)  {
                char    *r = malloc((unsigned) (strlen(optvec[v].aun.string) + 2));
                if  (!r)
                        ABORT_NOMEM;
                sprintf(r, "+%s", optvec[v].aun.string);
                return  r;
        }
        else  if  (optvec[v].aun.letter == 0)
                return  "+???";
        else  {
                char    *r = malloc(3);
                if  (!r)
                        ABORT_NOMEM;
                sprintf(r, "-%c", optvec[v].aun.letter);
                return  r;
        }
}

#define TBG_STATE(W)    XmToggleButtonGadgetGetState(W)

struct  dialog_data  {
        GtkWidget       *dir_w,
                        *typem_w,
                        *inclold_w,
                        *recurse_w,
                        *followl_w,
                        *timeout_w,
                        *patttype_w,
                        *filename_w,
                        *contrun_w,
                        *daem_w,
                        *polltime_w,
                        *command_w,
                        *isshell_w;
};

void  select_dir(struct dialog_data *ddata)
{
        char            *pr;
        GtkWidget       *fsd;

        pr = gprompt($P{xfilemon select dir dlgtit});
        fsd = gtk_file_chooser_dialog_new(pr,
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          GTK_STOCK_OK,         GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,     GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fsd), gtk_entry_get_text(GTK_ENTRY(ddata->dir_w)));
        if  (gtk_dialog_run(GTK_DIALOG(fsd)) == GTK_RESPONSE_OK)  {
                gchar  *selected = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsd));
                gtk_entry_set_text(GTK_ENTRY(ddata->dir_w), selected);
                g_free(selected);
        }
        gtk_widget_destroy(fsd);
}

void  select_pattype(struct dialog_data *ddata)
{
        gtk_widget_set_sensitive(ddata->filename_w, gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->patttype_w)) > 0);
}

#define CHECK_TEST(w)   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ddata->w))

void  gen_process(struct dialog_data *ddata)
{
        HelpargRef      helpa;
        const  gchar    *dirv, *filv, *scriptv;
        int             whichsct, whichpt;
        char            **ap;
        char            *argvec[32], grotv[16], polltv[16];

        dirv = gtk_entry_get_text(GTK_ENTRY(ddata->dir_w));
        filv = gtk_entry_get_text(GTK_ENTRY(ddata->filename_w));
        scriptv = gtk_entry_get_text(GTK_ENTRY(ddata->command_w));
        whichsct = gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->typem_w));
        whichpt = gtk_combo_box_get_active(GTK_COMBO_BOX(ddata->patttype_w));

        if  (strlen(dirv) == 0)  {
                doerror($EH{xfilemon no directory});
                return;
        }
        if  (strlen(scriptv) == 0)  {
                doerror($EH{xfilemon no script});
                return;
        }
        if  (whichpt != 0  &&  strlen(filv) == 0)  {
                doerror($EH{xfilemon no pattern});
                return;
        }

        sprintf(grotv, "%d", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->timeout_w)));
        sprintf(polltv, "%d", gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ddata->polltime_w)));

        helpa = helpargs(Adefs, $A{btfilemon arg explain}, $A{btfilemon arg freeze home});
        makeoptvec(helpa, $A{btfilemon arg explain}, $A{btfilemon arg freeze home});

        ap = argvec;
        *ap++ = "btfilemon";
        *ap++ = gen_option($A{btfilemon arg run monitor});
        *ap++ = gen_option(CHECK_TEST(daem_w)? $A{btfilemon arg daemon}: $A{btfilemon arg nodaemon});

        switch  (filetypes->alt_nums[whichpt])  {
        case  WFL_ANY_FILE:
                *ap++ = gen_option($A{btfilemon arg anyfile});
                break;
        case  WFL_PATT_FILE:
                *ap++ = gen_option($A{btfilemon arg pattern file});
                *ap++ = (char *) filv;
                break;
        case  WFL_SPEC_FILE:
                *ap++ = gen_option($A{btfilemon arg given file});
                *ap++ = (char *) filv;
                break;
        }

        switch  (montypes->alt_nums[whichsct])  {
        case  WF_APPEARS:
                *ap++ = gen_option($A{btfilemon arg arrival});
                break;
        case  WF_REMOVED:
                *ap++ = gen_option($A{btfilemon arg file removed});
                break;
        case  WF_STOPSGROW:
                *ap++ = gen_option($A{btfilemon arg grow time});
                *ap++ = grotv;
                break;
        case  WF_STOPSWRITE:
                *ap++ = gen_option($A{btfilemon arg mod time});
                *ap++ = grotv;
                break;
        case  WF_STOPSCHANGE:
                *ap++ = gen_option($A{btfilemon arg change time});
                *ap++ = grotv;
                break;
        case  WF_STOPSUSE:
                *ap++ = gen_option($A{btfilemon arg access time});
                *ap++ = grotv;
                break;
        }

        *ap++ = gen_option($A{btfilemon arg poll time});
        *ap++ = polltv;
        *ap++ = gen_option(CHECK_TEST(contrun_w)? $A{btfilemon arg cont found}: $A{btfilemon arg halt found});
        *ap++ = gen_option(CHECK_TEST(inclold_w)? $A{btfilemon arg include existing}: $A{btfilemon arg ignore existing});
        *ap++ = gen_option(CHECK_TEST(recurse_w)? $A{btfilemon arg recursive}: $A{btfilemon arg nonrecursive});
        *ap++ = gen_option(CHECK_TEST(followl_w)? $A{btfilemon arg follow links}: $A{btfilemon arg no links});
        *ap++ = gen_option($A{btfilemon arg directory});
        *ap++ = (char *) dirv;
        *ap++ = gen_option(CHECK_TEST(isshell_w)? $A{btfilemon arg command}: $A{btfilemon arg script file});
        *ap++ = (char *) scriptv;
        *ap = (char *) 0;
        execvp("btfilemon", argvec);
        exit(E_SETUP);
}

GtkWidget *combo_from_helpalt(HelpaltRef ha)
{
        GtkWidget  *result = gtk_combo_box_new_text();
        int     cnt, sel = 0;

        for  (cnt = 0;  cnt < ha->numalt;  cnt++)
                gtk_combo_box_append_text(GTK_COMBO_BOX(result), ha->list[cnt]);
        if  (ha->def_alt > 0)
                sel = ha->def_alt;
        gtk_combo_box_set_active(GTK_COMBO_BOX(result), sel);
        return  result;
}

void  create_dialog(struct dialog_data *ddata)
{
        GtkWidget  *window, *vbox, *hbox, *bbox, *button;
        GtkAdjustment   *adj;
        char            *pr;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        toplevel = (GtkWidget *) window;
        pr = gprompt($P{xfilemon app title});
        gtk_window_set_title(GTK_WINDOW(window), pr);
        free(pr);

        gtk_container_set_border_width(GTK_CONTAINER(window), 5);
        g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
        g_signal_connect(window, "delete-event", G_CALLBACK(gtk_false), NULL);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

        vbox = gtk_vbox_new(FALSE, DEF_DLG_VPAD);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        /* First row is directory to scan */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xfilemon directory}), FALSE, FALSE, DEF_DLG_HPAD);
        ddata->dir_w = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(ddata->dir_w), Curr_pwd);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->dir_w, FALSE, FALSE, DEF_DLG_HPAD);
        button = gprompt_button($P{xfilemon choose});
        gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, DEF_DLG_HPAD);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK(select_dir), (gpointer) ddata);

        /* Now have type of event and timeout time */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xfilemon file action}), FALSE, FALSE, 0);

        ddata->typem_w = combo_from_helpalt(montypes);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->typem_w, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xfilemon file timeout}), FALSE, FALSE, DEF_DLG_HPAD);

        adj = (GtkAdjustment *) gtk_adjustment_new(20.0, 1.0, 99999.0, 1.0, 10.0, 0.0);
        ddata->timeout_w = gtk_spin_button_new(adj, 0.0, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->timeout_w, FALSE, FALSE, DEF_DLG_HPAD);

        /* Now have checkboxes include old, recurse and follow links */

        ddata->inclold_w = gprompt_checkbutton($P{xfilemon include});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->inclold_w, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->recurse_w = gprompt_checkbutton($P{xfilemon recursive});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->recurse_w, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->followl_w = gprompt_checkbutton($P{xfilemon links});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->followl_w, FALSE, FALSE, DEF_DLG_VPAD);

        /* File pattern type and box for pattern */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xfilemon file names}), FALSE, FALSE, DEF_DLG_HPAD);
        ddata->patttype_w = combo_from_helpalt(filetypes);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->patttype_w, FALSE, FALSE, DEF_DLG_HPAD);
        ddata->filename_w = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), ddata->filename_w, FALSE, FALSE, DEF_DLG_HPAD);
        if  (filetypes->def_alt == 0)
                gtk_widget_set_sensitive(ddata->filename_w, FALSE);
        g_signal_connect_swapped(ddata->patttype_w, "changed", G_CALLBACK(select_pattype), (gpointer) ddata);

        /* Continue running and daemon flags */

        ddata->contrun_w = gprompt_checkbutton($P{xfilemon continue running});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->contrun_w, FALSE, FALSE, DEF_DLG_VPAD);
        ddata->daem_w = gprompt_checkbutton($P{xfilemon daemon});
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->daem_w), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), ddata->daem_w, FALSE, FALSE, DEF_DLG_VPAD);

        /* Poll time */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xfilemon poll time}), FALSE, FALSE, DEF_DLG_HPAD);
        adj = (GtkAdjustment *) gtk_adjustment_new(20.0, 1.0, 99999.0, 1.0, 10.0, 0.0);
        ddata->polltime_w = gtk_spin_button_new(adj, 0.0, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ddata->polltime_w, FALSE, FALSE, DEF_DLG_HPAD);

        /* Command */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xfilemon command}), FALSE, FALSE, DEF_DLG_HPAD);
        ddata->command_w = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), ddata->command_w, FALSE, FALSE, DEF_DLG_HPAD);

        /* Shell or file */

        ddata->isshell_w = gprompt_checkbutton($P{xfilemon cscmd});
        gtk_box_pack_start(GTK_BOX(vbox), ddata->isshell_w, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddata->isshell_w), TRUE);

        /* Stuff at bottom */

        bbox = gtk_hbutton_box_new();
        gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, DEF_DLG_VPAD*2);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_EDGE);
        button = gtk_button_new_with_label("OK");
        gtk_container_add(GTK_CONTAINER(bbox), button);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK(gen_process), (gpointer) ddata);
        button = gtk_button_new_with_label("Cancel");
        gtk_container_add(GTK_CONTAINER(bbox), button);
        g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);
        gtk_widget_show_all(toplevel);
}

MAINFN_TYPE  main(int argc, char **argv)
{
        struct  dialog_data     ddata;

        versionprint(argv, "$Revision: 1.9 $", 0);

        gtk_init(&argc, &argv);
        init_mcfile();

        Cfile = open_cfile("FILEMONCONF", "filemon.help");

        if  (!(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        montypes = helprdalt($Q{xfilemon file action});
        filetypes = helprdalt($Q{xfilemon file names});

        if  (!montypes)  {
                disp_arg[9] = $Q{xfilemon file action};
                print_error($E{Missing alternative code});
                return  E_SETUP;
        }
        if  (!filetypes)  {
                disp_arg[9] = $Q{xfilemon file names};
                print_error($E{Missing alternative code});
                return  E_SETUP;
        }

        create_dialog(&ddata);
        gtk_main();
        return  0;              /* Shut up compiler */
}
