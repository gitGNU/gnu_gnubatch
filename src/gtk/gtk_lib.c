/* gtk_lib.c -- routines for GTK programs

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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "defaults.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "incl_unix.h"
#include "gtk_lib.h"
#include "incl_ugid.h"

#define DEF_DLG_VPAD    5
#define DEF_DLG_HPAD    5

static  char    Filename[] = __FILE__;

extern  GtkWidget       *toplevel;

/* Test it's not on before we force it on otherwise we could get an endless loop
   of signals. Used for mode and priv bits in cases where one implies others */

void    setifnotset(GtkWidget *wid)
{
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid), TRUE);
}

/* Ditto for unsetting */

void    unsetifset(GtkWidget *wid)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid)))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wid), FALSE);
}

char    *makebigvec(char **mat)
{
        unsigned  totlen = 0, len;
        char    **ep, *newstr, *pos;

        for  (ep = mat;  *ep;  ep++)
                totlen += strlen(*ep) + 1;

        if  (totlen == 0)
                return  stracpy("");
        newstr = malloc((unsigned) totlen);
        if  (!newstr)
                ABORT_NOMEM;
        pos = newstr;
        for  (ep = mat;  *ep;  ep++)  {
                len = strlen(*ep);
                strcpy(pos, *ep);
                free(*ep);
                pos += len;
                *pos++ = '\n';
        }
        pos[-1] = '\0';
        free((char *) mat);
        return  newstr;
}

void    doerror(int errnum)
{
        char    **evec = helpvec(errnum, 'E'), *newstr;
        GtkWidget  *msgw;
        if  (!evec[0])  {
                disp_arg[9] = errnum;
                free((char *) evec);
                evec = helpvec($EH{Missing error code}, 'E');
        }
        newstr = makebigvec(evec);
        msgw = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", newstr);
        free(newstr);
        evec = helpvec(errnum, 'H');
        if  (evec[0])  {
                newstr = makebigvec(evec);
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msgw), "%s", newstr);
                free(newstr);
        }
        else
                free((char *) evec);
        g_signal_connect_swapped(msgw, "response", G_CALLBACK(gtk_widget_destroy), msgw);
        gtk_widget_show(msgw);
        gtk_dialog_run(GTK_DIALOG(msgw));
}

int     Confirm(int code)
{
        int     ret;
        char    *msg = gprompt(code), *secmsg = makebigvec(helpvec(code, 'H'));
        GtkWidget  *dlg = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", msg);
        free(msg);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), "%s", secmsg);
        free(secmsg);
        ret = gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES;
        gtk_widget_destroy(dlg);
        return  ret;
}

void    gtk_chk_uid()
{
        char    *hd = getenv("HOME");
        struct  stat    sbuf;

        if  (!hd  ||  stat(hd, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR  ||  sbuf.st_uid != Realuid)  {
                print_error($E{Check file setup});
                exit(E_SETUP);
        }
}

GtkWidget       *gprompt_label(const int code)
{
        char    *pr = gprompt(code);
        GtkWidget  *lab = gtk_label_new(pr);
        free(pr);
        return  lab;
}

GtkWidget       *gprompt_button(const int code)
{
        char    *pr = gprompt(code);
        GtkWidget  *butt = gtk_button_new_with_label(pr);
        free(pr);
        return  butt;
}

GtkWidget       *gprompt_checkbutton(const int code)
{
        char    *pr = gprompt(code);
        GtkWidget  *butt = gtk_check_button_new_with_label(pr);
        free(pr);
        return  butt;
}

GtkWidget       *gprompt_radiobutton(const int code)
{
        char    *pr = gprompt(code);
        GtkWidget  *butt = gtk_radio_button_new_with_label(NULL, pr);
        free(pr);
        return  butt;
}

GtkWidget       *gprompt_radiobutton_fromwidget(GtkWidget *w, const int code)
{
        char    *pr = gprompt(code);
        GtkWidget  *butt = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w), pr);
        free(pr);
        return  butt;
}


GtkWidget *gprompt_dialog(GtkWidget *parent, const int code)
{
        GtkWidget  *dlg;
        char    *pr = gprompt(code);

        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(parent),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);
        return  dlg;
}

static  void    slidedown(GtkWidget *slide)
{
        unsigned  val = gtk_range_get_value(GTK_RANGE(slide));
        unsigned  round;

        for  (round = 10;  round < 10000;  round *= 10)  {
                unsigned  rem = val % round;
                if  (rem != 0)  {
                        val -= rem;
                        /* Don't need to check for zero as gtk_range_set_value does that */
                        gtk_range_set_value(GTK_RANGE(slide), (gdouble) val);
                        gtk_widget_queue_draw(GTK_WIDGET(slide));
                        return;
                }
        }
        val -= round;
        gtk_range_set_value(GTK_RANGE(slide), (gdouble) val);
        gtk_widget_queue_draw(GTK_WIDGET(slide));
}

static  void    slideup(GtkWidget *slide)
{
        unsigned  val = gtk_range_get_value(GTK_RANGE(slide));
        unsigned  round;

        for  (round = 10;  round < 10000;  round *= 10)  {
                unsigned  rem = val % round;
                if  (rem != 0)  {
                        val += round - rem;
                        /* Don't need to check for max as gtk_range_set_value does that */
                        gtk_range_set_value(GTK_RANGE(slide), (gdouble) val);
                        gtk_widget_queue_draw(GTK_WIDGET(slide));
                        return;
                }
        }
        val += round;
        gtk_range_set_value(GTK_RANGE(slide), (gdouble) val);
        gtk_widget_queue_draw(GTK_WIDGET(slide));
}

GtkWidget *slider_with_buttons(GtkWidget *vbox, const int code, unsigned initv, const int insens)
{
        GtkWidget  *hbox, *minb, *plusb, *slide;
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label(code), FALSE, FALSE, DEF_DLG_HPAD);
        minb = gtk_button_new_with_label("-");
        gtk_box_pack_start(GTK_BOX(hbox), minb, FALSE, FALSE, DEF_DLG_HPAD);
        plusb = gtk_button_new_with_label("+");
        gtk_box_pack_start(GTK_BOX(hbox), plusb, FALSE, FALSE, DEF_DLG_HPAD);
        slide = gtk_hscale_new_with_range(1.0, 60000.0, 1.0);
        gtk_box_pack_start(GTK_BOX(vbox), slide, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_scale_set_digits(GTK_SCALE(slide), 0);
        gtk_range_set_value(GTK_RANGE(slide), (gdouble) initv);
        gtk_widget_queue_draw(GTK_WIDGET(slide));
        if  (insens)  {
                gtk_widget_set_sensitive(slide, FALSE);
                gtk_widget_set_sensitive(minb, FALSE);
                gtk_widget_set_sensitive(plusb, FALSE);
        }
        else  {
                g_signal_connect_swapped(G_OBJECT(minb), "clicked", G_CALLBACK(slidedown), slide);
                g_signal_connect_swapped(G_OBJECT(plusb), "clicked", G_CALLBACK(slideup), slide);
        }
        return  slide;
}
