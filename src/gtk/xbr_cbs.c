/* xbr_cbs.c -- General callback routines for gbch-xr

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#include <pwd.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
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
#include "ecodes.h"
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
#include "cfile.h"
#include "errnums.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xbr_ext.h"
#include "gtk_lib.h"

void  initcifile()
{
        int     ret;

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
}

void  doinfo(int code)
{
        char    **evec = helpvec(code, 'E'), *newstr;
        GtkWidget               *ew;
        if  (!evec[0])  {
                disp_arg[9] = code;
                free((char *) evec);
                evec = helpvec($E{Missing error code}, 'E');
        }
        newstr = makebigvec(evec);
        ew = gtk_message_dialog_new(GTK_WINDOW(toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", newstr);
        free(newstr);
        evec = helpvec(code, 'H');
        newstr = makebigvec(evec);
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(ew), "%s", newstr);
        free(newstr);
        g_signal_connect_swapped(ew, "response", G_CALLBACK(gtk_widget_destroy), ew);
        gtk_widget_show(ew);
}

struct  editorws  {
        GtkWidget  *inteditw, *editorw, *xtermw;
};

static  void    intedflip(GtkWidget *ww, struct editorws *sp)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ww)))  {
                gtk_widget_set_sensitive(sp->editorw, FALSE);
                gtk_widget_set_sensitive(sp->xtermw, FALSE);
        }
        else  {
                gtk_widget_set_sensitive(sp->editorw, TRUE);
                gtk_widget_set_sensitive(sp->xtermw, TRUE);
        }
}

void  cb_viewopt()
{
        GtkWidget  *dlg, *hbox, *xmlfmt;
        struct  editorws  ews;

        dlg = gprompt_dialog(toplevel, $P{xbtr view opt dlgtit});
        xmlfmt = gprompt_checkbutton($P{xbtr prefer XML format});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), xmlfmt, FALSE, FALSE, DEF_DLG_VPAD);
        if  (xml_format)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xmlfmt), TRUE);
        ews.inteditw = gprompt_checkbutton($P{xbtr use internal editor});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ews.inteditw, FALSE, FALSE, DEF_DLG_VPAD);
        if  (internal_edit)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ews.inteditw), TRUE);
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtr job script editor lab}), FALSE, FALSE, DEF_DLG_HPAD);
        ews.editorw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), ews.editorw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(ews.editorw), editor_name? editor_name: DEFAULT_EDITOR_NAME);
        ews.xtermw = gprompt_checkbutton($P{xbtr editor in xterm});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), ews.xtermw, FALSE, FALSE, DEF_DLG_VPAD);
        if  (xterm_edit)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ews.xtermw), TRUE);

        if  (internal_edit)  {
                gtk_widget_set_sensitive(ews.editorw, FALSE);
                gtk_widget_set_sensitive(ews.xtermw, FALSE);
        }
        g_signal_connect(G_OBJECT(ews.inteditw), "toggled", G_CALLBACK(intedflip), (gpointer) &ews);
        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char     *neweditor = gtk_entry_get_text(GTK_ENTRY(ews.editorw));
                gboolean        newxte = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ews.xtermw));
                gboolean        newie = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ews.inteditw));
                gboolean        newxmlf = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xmlfmt));

                if  (strlen(neweditor) == 0)  {
                        doerror($EH{xbtr editor name null});
                        continue;
                }

                /* FIXME - check on PATH? */

                if  (editor_name)
                        free(editor_name);
                editor_name = stracpy(neweditor);
                xterm_edit = newxte;
                internal_edit = newie;
                xml_format = newxmlf;
                Dirty++;
                break;
        }
        gtk_widget_destroy(dlg);
}

void  loadopts()
{
        char    *arg;

        /* Display options */

        if  ((arg = optkeyword("XBTRMWWIDTH")))  {
                if  (arg[0])
                        Mwwidth = atoi(arg);
                free(arg);
        }
        if  ((arg = optkeyword("XBTRMWHEIGHT")))  {
                if  (arg[0])
                        Mwheight = atoi(arg);
                free(arg);
        }
        if  ((arg = optkeyword("XBTRXMLFMT")))  {
                if  (arg[0])
                        xml_format = arg[0] != '0';
                free(arg);
        }
        if  ((arg = optkeyword("XBTRINTERNALEDIT")))  {
                if  (arg[0])
                        internal_edit = arg[0] != '0';
                free(arg);
        }
        if  ((arg = optkeyword("XBTRXTERMEDIT")))  {
                if  (arg[0])
                        xterm_edit = arg[0] != '0';
                free(arg);
        }

        if  ((arg = optkeyword("XBTREDITOR")))
                editor_name = arg;
        else
                editor_name = stracpy(DEFAULT_EDITOR_NAME); /* Something VIle I suspect */

        /* Set up list of hosts we know about */
        arg = optkeyword("XBTRHOSTLIST");
        init_hosts_known(arg);
        if  (arg)
                free(arg);
        if  ((arg = optkeyword("XBTRDEFHOST")))
                set_def_host(arg);
        close_optfile();
}

static  int     state_wait(PIDTYPE pid)
{
        int     status;

        if  (pid < 0)  {
                doerror($EH{saveopts cannot fork});
                return  0;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif
        if  (status != 0)  {
                if  ((status & 127) != 0)  {
                        disp_arg[0] = status & 127;
                        doerror($EH{saveopts crashed});
                }
                else  {
                        int     msg;

                        disp_arg[0] = status >> 8;
                        switch  (disp_arg[0])  {
                        default:
                                msg = $EH{saveopts unknown exit};
                                break;
                        case  E_NOMEM:
                                msg = $EH{saveopts no memory};
                                break;
                        case  E_SETUP:
                                msg = $EH{saveopts bad setup};
                                break;
                        case  E_NOPRIV:
                                msg = $EH{saveopts no write};
                                break;
                        case  E_USAGE:
                                msg = $EH{saveopts usage};
                                break;
                        case  E_BTEXEC2:
                                msg = $EH{saveopts no exec};
                                break;
                        }
                        doerror(msg);
                }
                return  0;
        }

        return  1;
}

void  save_state()
{
        PIDTYPE pid;

        gtk_window_get_size(GTK_WINDOW(toplevel), &Mwwidth, &Mwheight);

        if  ((pid = fork()) == 0)  {
                char    wbuf[20], hbuf[20], *argbuf[12]; /* Remember to increase this if we add stuff */
                char    **ap = argbuf, *hl;
                *ap++ = gtkprog; /* Arg 0 is the program we're running */
                *ap++ = "XBTRMWWIDTH";
                sprintf(wbuf, "%d", Mwwidth);
                *ap++ = wbuf;
                *ap++ = "XBTRMWHEIGHT";
                sprintf(hbuf, "%d", Mwheight);
                *ap++ = hbuf;
                hl = list_hosts_known();
                if  (hl  &&  strlen(hl) != 0)  {
                        *ap++ = "XBTRHOSTLIST";
                        *ap++ = hl;
                }
                hl = get_def_host();
                if  (hl && strlen(hl) != 0)  {
                        *ap++ = "XBTRDEFHOST";
                        *ap++ = hl;
                }
                *ap = 0;
                umask(Save_umask);
                execv(execprog, argbuf);
                exit(E_SETUP);
        }
        state_wait(pid);
}

void  cb_saveopts()
{
        PIDTYPE pid;

        gtk_window_get_size(GTK_WINDOW(toplevel), &Mwwidth, &Mwheight);

        if  ((pid = fork()) == 0)  {
                char    digbuf1[2], digbuf2[2], digbuf3[2], *argbuf[12]; /* Remember to increase this if we add stuff */
                char    **ap = argbuf;
                *ap++ = gtkprog; /* Arg 0 is the program we're running */
                *ap++ = "XBTRINTERNALEDIT";
                digbuf1[0] = internal_edit? '1': '0';
                digbuf1[1]= '\0';
                *ap++ = digbuf1;
                *ap++ = "XBTRXTERMEDIT";
                digbuf2[0] = xterm_edit? '1': '0';
                digbuf2[1] = '\0';
                *ap++ = digbuf2;
                *ap++ = "XBTRXMLFMT";
                digbuf3[0] = xml_format? '1': '0';
                digbuf3[1] = '\0';
                *ap++ = digbuf3;
                *ap++ = "XBTREDITOR";
                *ap++ = editor_name;
                *ap = 0;
                umask(Save_umask);
                execv(execprog, argbuf);
                exit(E_SETUP);
        }

        if  (state_wait(pid))
                Dirty = 0;
}
