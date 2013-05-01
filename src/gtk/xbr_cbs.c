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

void  cb_viewopt()
{
        GtkWidget  *dlg, *hbox, *editorw, *xtermw;

        dlg = gprompt_dialog(toplevel, $P{xbtr view opt dlgtit});
        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtr job script editor lab}), FALSE, FALSE, DEF_DLG_HPAD);
        editorw = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), editorw, FALSE, FALSE, DEF_DLG_HPAD);
        gtk_entry_set_text(GTK_ENTRY(editorw), editor_name? editor_name: DEFAULT_EDITOR_NAME);
        xtermw = gprompt_checkbutton($P{xbtr editor in xterm});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), xtermw, FALSE, FALSE, DEF_DLG_VPAD);
        if  (xterm_edit)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xtermw), TRUE);

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char     *neweditor = gtk_entry_get_text(GTK_ENTRY(editorw));
                gboolean        newxte = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(xtermw));

                /* Save checking if editor hasn't changed */

                if  (strcmp(neweditor, editor_name) == 0)  {
                        if  ((!newxte && xterm_edit) || (newxte && !xterm_edit))  {
                                xterm_edit = newxte;
                                Dirty++;
                        }
                        break;
                }

                if  (strlen(neweditor) == 0)  {
                        doerror($EH{xbtr editor name null});
                        continue;
                }

                /* FIXME - check on PATH? */

                if  (editor_name)
                        free(editor_name);
                editor_name = stracpy(neweditor);
                xterm_edit = newxte;
                Dirty++;
                break;
        }
        gtk_widget_destroy(dlg);
}

void  loadopts()
{
        char    *arg;

        /* Display options */

        if  ((arg = optkeyword("XBTRXTERMEDIT")))  {
                if  (arg[0])
                        xterm_edit = arg[0] != '0';
                free(arg);
        }

        if  ((arg = optkeyword("XBTREDITOR")))
                editor_name = arg;
        else
                editor_name = stracpy(DEFAULT_EDITOR_NAME); /* Something VIle I suspect */
        close_optfile();
}

void  cb_saveopts()
{
        PIDTYPE pid;
        int     status;

        if  ((pid = fork()) == 0)  {
                char    digbuf[2], *argbuf[10]; /* Remember to increase this if we add stuff */
                char    **ap = argbuf;
                *ap++ = gtkprog; /* Arg 0 is the program we're running */
                *ap++ = "XBTRXTERMEDIT";
                digbuf[0] = xterm_edit? '1': '0';
                digbuf[1] = '\0';
                *ap++ = digbuf;
                *ap++ = "XBTREDITOR";
                *ap++ = editor_name;
                *ap = 0;
                umask(Save_umask);
                execv(execprog, argbuf);
                exit(E_SETUP);
        }

        if  (pid < 0)  {
                doerror($EH{saveopts cannot fork});
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
                disp_arg[0] = status >> 8;
                disp_arg[1] = status & 127;
                doerror((status >> 8) + $EH{saveopts file error});
                return;
        }
        Dirty = 0;
}
