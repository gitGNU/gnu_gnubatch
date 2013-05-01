/* xbq_view.c -- script / logfile / holidays for gbch-xq

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
#include "netmsg.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "xbq_ext.h"
#include "gtk_lib.h"

static  char    Filename[] = __FILE__;

#define DEFAULT_VIEW_WIDTH      500
#define DEFAULT_VIEW_HEIGHT     600

#define DEFAULT_SE_WIDTH        400
#define DEFAULT_SE_HEIGHT       600

#define INBUFSIZE       250

struct  view_data  {
        GtkTextBuffer  *fbuf;
        GtkWidget       *dlg;                   /* Dialog containing our view */
        GtkWidget       *scroll, *view;         /* Scroll and TextView */
};

static  char    *lastsearch;
static  int     lastwrap = 0;

static void  cb_vexit(GtkAction *, struct view_data *);
static void  cb_vsearch(GtkAction *, struct view_data *);
static void  cb_vrsearch(GtkAction *, struct view_data *);
static void  clearlog(GtkAction *, GtkWidget *);
static void  cb_quitsel(GtkAction *, GtkWidget *);
FILE *net_feed(const int, const netid_t, const jobno_t, const int);

static GtkActionEntry ventries[] = {
        { "VFileMenu", NULL, "_File"  },
        { "VSearchMenu", NULL, "_Search"  },
        { "ExitV", GTK_STOCK_QUIT, "_Exit and save", "<control>Q", "Exit view", G_CALLBACK(cb_vexit) },
        { "VSearch", NULL, "_Search for ...", NULL, "Search for string", G_CALLBACK(cb_vsearch)  },
        { "VSearchf", NULL, "Search _forward", "F3", "Repeat last search going forward", G_CALLBACK(cb_vrsearch)  },
        { "VSearchb", NULL, "Search _backward", "F4", "Repeat last search going backward", G_CALLBACK(cb_vrsearch)  }  };

static GtkActionEntry slentries[] = {
        { "SEFileMenu", NULL, "_File"  },
        { "Clear", NULL, "_Clear log", NULL, "Clear log file and quit", G_CALLBACK(clearlog)},
        { "Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK(cb_quitsel)}  };

static void  buff_append(GtkTextBuffer *buf, char *tbuf, const int size)
{
        GtkTextIter  iter;
        gtk_text_buffer_get_end_iter(buf, &iter);
        gtk_text_buffer_insert(buf, &iter, tbuf, size);
}

static void  fill_viewbuffer(GtkTextBuffer *fbuf, FILE *inf)
{
        int     ch, cp = 0, colnum = 0;
        char    inbuf[INBUFSIZE+8];

        cp = 0;

        while  ((ch = getc(inf)) != EOF)  {
                switch  (ch)  {
                default:
                        if  (ch & 0x80)  {
                                inbuf[cp++] = 'M';
                                inbuf[cp++] = '-';
                                ch &= 0x7f;
                                colnum += 2;
                        }
                        if  (ch < ' ')  {
                                inbuf[cp++] = '^';
                                ch += ' ';
                                colnum++;
                        }
                        colnum++;
                        inbuf[cp++] = ch;
                        break;
                case  '\n':
                        inbuf[cp++] = ch;
                        colnum = 0;
                case  '\r':
                        break;
                case  '\t':
                        do  {
                                inbuf[cp++] = ' ';
                                colnum++;
                        }  while  ((colnum & 7) != 0);
                        break;
                }

                if  (cp >= INBUFSIZE)  {
                        buff_append(fbuf, inbuf, cp);
                        cp = 0;
                }
        }

        if  (cp > 0)
                buff_append(fbuf, inbuf, cp);
}


static void  vscroll_to(struct view_data *vd, GtkTextIter *posn)
{
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(vd->view), posn, 0.1, FALSE, 0.0, 0.0);
        gtk_text_buffer_place_cursor(vd->fbuf, posn);
}

static void  execute_vsearch(struct view_data *vd, const int isback)
{
        GtkTextIter  current_pos, match_start, buffbeg, buffend;
        GtkTextMark  *cpos;

        /* Kick off by getting current location */

        cpos = gtk_text_buffer_get_insert(vd->fbuf);
        gtk_text_buffer_get_iter_at_mark(vd->fbuf, &current_pos, cpos);

        if  (isback)  {
                if  (gtk_text_iter_backward_search(&current_pos, lastsearch, 0, &match_start, NULL, NULL))  {
                        vscroll_to(vd, &match_start);
                        return;
                }
                if  (lastwrap)  {
                        gtk_text_buffer_get_end_iter(vd->fbuf, &buffend);
                        if  (gtk_text_iter_backward_search(&buffend, lastsearch, 0, &match_start, NULL, NULL))  {
                                vscroll_to(vd, &match_start);
                                return;
                        }
                }
                doerror($EH{xmbtq string not found backw});
        }
        else  {
                int  chars = strlen(lastsearch);
                do  {
                        if  (!gtk_text_iter_forward_char(&current_pos))
                                break;
                        chars--;
                }  while  (chars > 0);

                if  (gtk_text_iter_forward_search(&current_pos, lastsearch, 0, &match_start, NULL, NULL))  {
                        vscroll_to(vd, &match_start);
                        return;
                }
                if  (lastwrap)  {
                        gtk_text_buffer_get_start_iter(vd->fbuf, &buffbeg);
                        if  (gtk_text_iter_forward_search(&buffbeg, lastsearch, 0, &match_start, NULL, NULL))  {
                                vscroll_to(vd, &match_start);
                                return;
                        }
                }
                doerror($EH{xmbtq string not found forw});
        }
}

static void  cb_vsearch(GtkAction *action, struct view_data *vd)
{
        GtkWidget  *dlg, *hbox, *lab, *sstring, *wrapw, *backw;

        dlg = gprompt_dialog(toplevel, $P{xbtq vsearch dlg});

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        lab = gprompt_label($P{xbtq vsearch str});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, DEF_DLG_HPAD);

        sstring = gtk_entry_new();
        if  (lastsearch)
                gtk_entry_set_text(GTK_ENTRY(sstring), lastsearch);
        gtk_box_pack_start(GTK_BOX(hbox), sstring, FALSE, FALSE, DEF_DLG_HPAD);

        wrapw = gprompt_checkbutton($P{xbtq vsearch wrap});
        if  (lastwrap)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrapw), TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), wrapw, FALSE, FALSE, DEF_DLG_VPAD);

        backw = gprompt_checkbutton($P{xbtq vsearch backw});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), backw, FALSE, FALSE, DEF_DLG_VPAD);

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *news = gtk_entry_get_text(GTK_ENTRY(sstring));
                int     isback;
                if  (strlen(news) == 0)  {
                        doerror($EH{xmbtq null search string});
                        continue;
                }
                if  (lastsearch)
                        free(lastsearch);
                lastsearch = stracpy(news);
                lastwrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrapw))? 1: 0;
                isback = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(backw))? 1: 0;
                execute_vsearch(vd, isback);
                break;
        }
        gtk_widget_destroy(dlg);
}

static void  cb_vrsearch(GtkAction *action, struct view_data *vd)
{
        const   char    *act = gtk_action_get_name(action);
        int     isback = strcmp(act, "VSearchb") == 0;

        if  (!lastsearch)  {
                doerror($EH{xmbtq no search string yet});
                return;
        }

        execute_vsearch(vd, isback);
}

static void  cb_vexit(GtkAction *action, struct view_data *vd)
{
        GtkWidget *dlg = vd->dlg;
        free((char *) vd);
        gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_CANCEL);
}

void  cb_view()
{
        BtjobRef  cj = getselectedjob(BTM_READ);
        FILE    *infile;
        char    *pr;
        GtkWidget       *hbox, *lab;
        GtkActionGroup *actions;
        GtkUIManager    *vui;
        GString *labp;
        GError          *err;
        PangoFontDescription *font_desc;
        GtkTextIter     biter;
        struct  view_data       *vd;

        if  (!cj)
                return;

        /* Check if remote host and grab it suitably */

        if  (cj->h.bj_hostid)  {
                if  (!(infile = net_feed(FEED_JOB, cj->h.bj_hostid, cj->h.bj_job, Job_seg.dptr->js_viewport)))  {
                        disp_arg[0] = cj->h.bj_job;
                        disp_str = qtitle_of(cj);
                        disp_str2 = look_host(cj->h.bj_hostid);
                        doerror($EH{xmbtq cannot open net file});
                        return;
                }
        }
        else  if  (!(infile = fopen(mkspid(SPNAM, cj->h.bj_job), "r")))  {
                disp_arg[0] = cj->h.bj_job;
                disp_str = qtitle_of(cj);
                doerror($EH{xmbtq cannot open job file});
                return;
        }

        vd = (struct view_data *) malloc(sizeof(struct view_data));
        if  (!vd)
                ABORT_NOMEM;

        vd->fbuf = gtk_text_buffer_new(NULL);
        fill_viewbuffer(vd->fbuf, infile);
        fclose(infile);

        /* Move cursor to start for benefit of searches */

        gtk_text_buffer_get_start_iter(vd->fbuf, &biter);
        gtk_text_buffer_place_cursor(vd->fbuf, &biter);

        if  (gtk_text_buffer_get_char_count(vd->fbuf) <= 0)  {
                doerror($EH{xmbtq empty job file});
                g_object_unref(G_OBJECT(vd->fbuf));
                free((char *) vd);
                return;
        }

        pr = gprompt($P{xbtq view dlg});
        vd->dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
        free(pr);
        gtk_window_set_default_size(GTK_WINDOW(vd->dlg), DEFAULT_VIEW_WIDTH, DEFAULT_VIEW_HEIGHT);

        actions = gtk_action_group_new("VJActions");
        gtk_action_group_add_actions(actions, ventries, G_N_ELEMENTS(ventries), vd);
        vui = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(vui, actions, 0);
        g_object_unref(G_OBJECT(actions));
        gtk_window_add_accel_group(GTK_WINDOW(vd->dlg), gtk_ui_manager_get_accel_group(vui));
        pr = envprocess(XBTQVIEW_MENU);
        gtk_ui_manager_add_ui_from_file(vui, pr, &err);
        free(pr);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), gtk_ui_manager_get_widget(vui, "/ViewMenu"), FALSE, FALSE, 0);
        g_object_unref(G_OBJECT(vui));

        pr = gprompt($P{xbtq view job});
        labp = g_string_new(NULL);
        g_string_printf(labp, "%s %s ", pr, JOB_NUMBER(cj));
        free(pr);

        if  (strlen(qtitle_of(cj)) == 0)  {
                pr = gprompt($P{xbtq view no title});
                g_string_append(labp, pr);
                free(pr);
        }
        else
                g_string_append(labp, qtitle_of(cj));

        lab = gtk_label_new(labp->str);
        g_string_free(labp, TRUE);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), lab, FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), hbox, FALSE, FALSE, 0);

        lab = gprompt_label($P{xbtq view user});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);
        lab = gtk_label_new(cj->h.bj_mode.o_user);
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);
        lab = gprompt_label($P{xbtq view group});
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);
        lab = gtk_label_new(cj->h.bj_mode.o_group);
        gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 0);

        vd->view = gtk_text_view_new_with_buffer(vd->fbuf);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(vd->view), FALSE);
        g_object_unref(G_OBJECT(vd->fbuf));
        if  ((font_desc = pango_font_description_from_string("fixed")))  {
                gtk_widget_modify_font(vd->view, font_desc);
                pango_font_description_free(font_desc);
        }
        vd->scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(vd->scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vd->scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(vd->scroll), vd->view);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vd->dlg)->vbox), vd->scroll, TRUE, TRUE, 0);
        gtk_widget_show_all(vd->dlg);
        g_signal_connect_swapped(vd->dlg, "response", G_CALLBACK(gtk_widget_destroy), vd->dlg);
}

/*
 ******************************************************************************
 *
 *                      System error log file viewing
 *
 ******************************************************************************
 */

static void  cb_quitsel(GtkAction *action, GtkWidget *dlg)
{
        gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
}

static void  clearlog(GtkAction *action, GtkWidget *dlg)
{
        if  (Confirm($PH{xmbtq confirm delete log file}))
                close(open(REPFILE, O_TRUNC|O_WRONLY));
        gtk_dialog_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
}

void  cb_syserr()
{
        char    *pr;
        FILE    *infile;
        GtkTextBuffer  *fbuf;
        GtkTextMark     *mark;
        GtkActionGroup *actions;
        GtkUIManager    *seui;
        GtkWidget       *dlg, *view, *scroll;
        GError          *err;
        GtkTextIter     iter;
        PangoFontDescription *font_desc;

        if  (!(infile = fopen(REPFILE, "r")))  {
                doerror($EH{xmbtq cannot open syslog});
                return;
        }

        fbuf = gtk_text_buffer_new(NULL);
        fill_viewbuffer(fbuf, infile);
        fclose(infile);

        /* If we didn't actually read anything, ditch it */

        if  (gtk_text_buffer_get_char_count(fbuf) <= 0)  {
                doerror($EH{xmbtq cannot open syslog});
                g_object_unref(G_OBJECT(fbuf));
                return;
        }

        pr = gprompt($P{xbtq syserr dlg});
        dlg = gtk_dialog_new_with_buttons(pr, GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
        free(pr);
        gtk_window_set_default_size(GTK_WINDOW(dlg), DEFAULT_SE_WIDTH, DEFAULT_SE_HEIGHT);

        actions = gtk_action_group_new("SEActions");
        gtk_action_group_add_actions(actions, slentries, G_N_ELEMENTS(slentries), dlg);
        seui = gtk_ui_manager_new();
        gtk_ui_manager_insert_action_group(seui, actions, 0);
        g_object_unref(G_OBJECT(actions));
        gtk_window_add_accel_group(GTK_WINDOW(dlg), gtk_ui_manager_get_accel_group(seui));
        pr = envprocess(XBTQSEL_MENU);
        gtk_ui_manager_add_ui_from_file(seui, pr, &err);
        free(pr);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), gtk_ui_manager_get_widget(seui, "/SELMenu"), FALSE, FALSE, 0);
        g_object_unref(G_OBJECT(seui));
        view = gtk_text_view_new_with_buffer(fbuf);
        gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
        g_object_unref(G_OBJECT(fbuf));
        if  ((font_desc = pango_font_description_from_string("fixed")))  {
                gtk_widget_modify_font(view, font_desc);
                pango_font_description_free(font_desc);
        }
        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), view);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, 0);

        gtk_text_buffer_get_end_iter(fbuf, &iter);
        mark = gtk_text_buffer_create_mark(fbuf, NULL, &iter, TRUE);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(view), mark, 0.0, FALSE, 0.0, 1.0);
        gtk_widget_show_all(dlg);
        g_signal_connect_swapped(dlg, "response", G_CALLBACK(gtk_widget_destroy), dlg);
}

/*********************************************************************************

        H O L I D A Y   L I S T

**********************************************************************************/

#define ppermitted(flg) (mypriv->btu_priv & flg)

#define ISHOL(str, day)         (str)->yearmap[(day) >> 3] & (1 << ((day) & 7))
#define SETHOL(str, day)        (str)->yearmap[(day) >> 3] |= (1 << ((day) & 7))
#define UNSETHOL(str, day)      (str)->yearmap[(day) >> 3] &= ~((1 << ((day) & 7)))
#define FLIPHOL(str, day)       (str)->yearmap[(day) >> 3] ^= (1 << ((day) & 7))

struct  holdata  {
        GtkWidget       *calw;
        int             readonly;
        int             changes;
        int             mapyear;
        int             holfd;
        unsigned  char  yearmap[YVECSIZE];
};

static void  savechanges(struct holdata *hdata)
{
        if  (hdata->changes <= 0)
                return;
        hdata->changes = 0;
        if  (hdata->mapyear < 1990)
                return;
        lseek(hdata->holfd, (long) ((hdata->mapyear - 1990) * YVECSIZE), 0);
        Ignored_error = write(hdata->holfd, (char *) hdata->yearmap, YVECSIZE);
}

static void  loadyear(struct holdata *hdata, const int year)
{
        if  (hdata->mapyear == year)
                return;
        savechanges(hdata);
        hdata->mapyear = year;
        lseek(hdata->holfd, (long) ((year - 1990) * YVECSIZE), 0);
        if  (read(hdata->holfd, (char *) hdata->yearmap, YVECSIZE) != YVECSIZE)
                BLOCK_ZERO(hdata->yearmap, YVECSIZE);
}

static void  markmonth(struct holdata *hdata)
{
        int     mon = GTK_CALENDAR(hdata->calw)->month;
        int     daysinmon, yday, cnt;
        struct  tm      mkt;

        mkt.tm_hour = 12;
        mkt.tm_min = mkt.tm_sec = mkt.tm_isdst = 0;
        mkt.tm_year = hdata->mapyear - 1900;
        mkt.tm_mon = mon;
        mkt.tm_mday = 1;
        if  (mktime(&mkt) == (time_t) -1)
                return;

        yday = mkt.tm_yday;

        daysinmon = month_days[mon];
        if  (mon == 1)  {
                daysinmon = 28;
                /* Might as well get leap years right */
                if  (((hdata->mapyear % 4) == 0  &&  (hdata->mapyear % 100) != 0) ||  (hdata->mapyear % 400) == 0)
                        daysinmon = 29;
        }

        for  (cnt = 0;  cnt < daysinmon;  cnt++)
                if  (ISHOL(hdata, yday+cnt))
                        gtk_calendar_mark_day(GTK_CALENDAR(hdata->calw), cnt+1);
}

static void  hol_month_changed(struct holdata *hdata)
{
        guint   year, month, day;
        gtk_calendar_get_date(GTK_CALENDAR(hdata->calw), &year, &month, &day);
        loadyear(hdata, year);
        gtk_calendar_clear_marks(GTK_CALENDAR(hdata->calw));
        markmonth(hdata);
}

static void  hol_toggle(struct holdata *hdata)
{
        guint   year, month, day;
        struct  tm      mkt;

        gtk_calendar_get_date(GTK_CALENDAR(hdata->calw), &year, &month, &day);
        mkt.tm_hour = 12;
        mkt.tm_min = mkt.tm_sec = mkt.tm_isdst = 0;
        mkt.tm_year = year - 1900;
        mkt.tm_mon = month;
        mkt.tm_mday = day;
        if  (mktime(&mkt) == (time_t) -1)
                return;
        FLIPHOL(hdata, mkt.tm_yday);
        if  (ISHOL(hdata, mkt.tm_yday))
                gtk_calendar_mark_day(GTK_CALENDAR(hdata->calw), day);
        else
                gtk_calendar_unmark_day(GTK_CALENDAR(hdata->calw), day);
        hdata->changes++;
}

void  cb_hols()
{
        GtkWidget       *dlg;
        char    *fname = envprocess(HOLFILE), *pr;
        time_t  now = time((time_t *) 0);
        struct  tm  *tp = localtime(&now);
        struct  holdata hdata;

        hdata.mapyear = -1;
        hdata.changes = 0;

        if  ((hdata.readonly = !ppermitted(BTM_WADMIN)))  {
                hdata.holfd = open(fname, O_RDONLY);
                free(fname);
                if  (hdata.holfd < 0)  {
                        doerror($EH{xmbtq cannot open hols});
                        return;
                }
        }
        else  {
                USHORT  oldumask = umask(0);
                hdata.holfd = open(fname, O_RDWR|O_CREAT, 0644);
#ifndef HAVE_FCHOWN
                if  (Realuid == ROOTID)
                        Ignored_error = chown(fname, Daemuid, Realgid);
#endif
                umask(oldumask);
                free(fname);
                if  (hdata.holfd < 0)  {
                        doerror($EH{xmbtq cannot create hols});
                        return;
                }
#ifdef  HAVE_FCHOWN
                if  (Realuid == ROOTID)
                        Ignored_error = fchown(hdata.holfd, Daemuid, Realgid);
#endif
        }

        pr = gprompt($P{xbtq hols dlgtit});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          NULL);
        free(pr);

        hdata.calw = gtk_calendar_new();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hdata.calw, FALSE, FALSE, DEF_DLG_VPAD);
        loadyear(&hdata, tp->tm_year + 1900);
        markmonth(&hdata);

        g_signal_connect_swapped(G_OBJECT(hdata.calw), "month-changed", G_CALLBACK(hol_month_changed), (gpointer) &hdata);
        if  (!hdata.readonly)
                g_signal_connect_swapped(G_OBJECT(hdata.calw), "day-selected-double-click", G_CALLBACK(hol_toggle), (gpointer) &hdata);

        gtk_widget_show_all(dlg);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        savechanges(&hdata);
        close(hdata.holfd);
}
