/* gtk_modebits.c -- fiddle with permission bits under GTK

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

#define DEF_DLG_VPAD    5
#define DEF_DLG_HPAD    5

#ifdef  PLEASE_DO_NOT_DEFINE_ME

/* There is a bug in helpparse which loses the effect of COPY now and then.
   Until I fix it we need to quote everything. */

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

void    forceoff(GtkWidget *butt, GtkWidget *otherbutt)
{
        if  (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(butt))  &&  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(otherbutt)))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(otherbutt), FALSE);
}

void    forceon(GtkWidget *butt, GtkWidget *otherbutt)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(butt))  &&  !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(otherbutt)))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(otherbutt), TRUE);
}

void    setup_jmodebits(GtkWidget *container, GtkWidget *jmodes[3][NUM_JMODEBITS], USHORT um, USHORT gm, USHORT om)
{
        GtkWidget  *tab;
        int     bcnt;

        tab = gtk_table_new(NUM_JMODEBITS, 4, FALSE);
        gtk_container_add(GTK_CONTAINER(container), tab);

        for  (bcnt = 0;  bcnt < NUM_JMODEBITS;  bcnt++)  {
                GtkWidget  *butt;
                gtk_table_attach_defaults(GTK_TABLE(tab), gprompt_label($P{Read mode name} + bcnt), 0, 1, bcnt, bcnt+1);
                butt = gtk_check_button_new_with_label("U");
                jmodes[0][bcnt] = butt;
                if  (um & (1 << bcnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(tab), butt, 1, 2, bcnt, bcnt+1);
                butt = gtk_check_button_new_with_label("G");
                jmodes[1][bcnt] = butt;
                if  (gm & (1 << bcnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(tab), butt, 2, 3, bcnt, bcnt+1);
                butt = gtk_check_button_new_with_label("O");
                jmodes[2][bcnt] = butt;
                if  (om & (1 << bcnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(tab), butt, 3, 4, bcnt, bcnt+1);
        }

        for  (bcnt = 0;  bcnt < 3;  bcnt++)  {

                /* Set reveal off to force off read
                   Set read off to force off write
                   Set read on to force on reveal
                   Set write on to force on read
                   Set read mode off to force off write mode
                   Set write mode on to force on read mode */

                g_signal_connect(jmodes[bcnt][BTM_SHOW_BIT], "toggled", G_CALLBACK(forceoff), (gpointer) jmodes[bcnt][BTM_READ_BIT]);
                g_signal_connect(jmodes[bcnt][BTM_READ_BIT], "toggled", G_CALLBACK(forceoff), (gpointer) jmodes[bcnt][BTM_WRITE_BIT]);
                g_signal_connect(jmodes[bcnt][BTM_READ_BIT], "toggled", G_CALLBACK(forceon), (gpointer) jmodes[bcnt][BTM_SHOW_BIT]);
                g_signal_connect(jmodes[bcnt][BTM_WRITE_BIT], "toggled", G_CALLBACK(forceon), (gpointer) jmodes[bcnt][BTM_READ_BIT]);
                g_signal_connect(jmodes[bcnt][BTM_RDMODE_BIT], "toggled", G_CALLBACK(forceoff), (gpointer) jmodes[bcnt][BTM_WRMODE_BIT]);
                g_signal_connect(jmodes[bcnt][BTM_WRMODE_BIT], "toggled", G_CALLBACK(forceon), (gpointer) jmodes[bcnt][BTM_RDMODE_BIT]);
        }
}

void    setup_vmodebits(GtkWidget *container, GtkWidget *vmodes[3][NUM_VMODEBITS], USHORT um, USHORT gm, USHORT om)
{
        GtkWidget  *tab;
        int     bcnt;

        tab = gtk_table_new(NUM_VMODEBITS, 4, FALSE);
        gtk_container_add(GTK_CONTAINER(container), tab);

        for  (bcnt = 0;  bcnt < NUM_VMODEBITS;  bcnt++)  {
                GtkWidget  *butt;
                gtk_table_attach_defaults(GTK_TABLE(tab), gprompt_label($P{Read mode name} + bcnt), 0, 1, bcnt, bcnt+1);
                butt = gtk_check_button_new_with_label("U");
                vmodes[0][bcnt] = butt;
                if  (um & (1 << bcnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(tab), butt, 1, 2, bcnt, bcnt+1);
                butt = gtk_check_button_new_with_label("G");
                vmodes[1][bcnt] = butt;
                if  (gm & (1 << bcnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(tab), butt, 2, 3, bcnt, bcnt+1);
                butt = gtk_check_button_new_with_label("O");
                vmodes[2][bcnt] = butt;
                if  (om & (1 << bcnt))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(butt), TRUE);
                gtk_table_attach_defaults(GTK_TABLE(tab), butt, 3, 4, bcnt, bcnt+1);
        }

        for  (bcnt = 0;  bcnt < 3;  bcnt++)  {

                /* Set reveal off to force off read
                   Set read off to force off write
                   Set read on to force on reveal
                   Set write on to force on read
                   Set read mode off to force off write mode
                   Set write mode on to force on read mode */

                g_signal_connect(vmodes[bcnt][BTM_SHOW_BIT], "toggled", G_CALLBACK(forceoff), (gpointer) vmodes[bcnt][BTM_READ_BIT]);
                g_signal_connect(vmodes[bcnt][BTM_READ_BIT], "toggled", G_CALLBACK(forceoff), (gpointer) vmodes[bcnt][BTM_WRITE_BIT]);
                g_signal_connect(vmodes[bcnt][BTM_READ_BIT], "toggled", G_CALLBACK(forceon), (gpointer) vmodes[bcnt][BTM_SHOW_BIT]);
                g_signal_connect(vmodes[bcnt][BTM_WRITE_BIT], "toggled", G_CALLBACK(forceon), (gpointer) vmodes[bcnt][BTM_READ_BIT]);
                g_signal_connect(vmodes[bcnt][BTM_RDMODE_BIT], "toggled", G_CALLBACK(forceoff), (gpointer) vmodes[bcnt][BTM_WRMODE_BIT]);
                g_signal_connect(vmodes[bcnt][BTM_WRMODE_BIT], "toggled", G_CALLBACK(forceon), (gpointer) vmodes[bcnt][BTM_RDMODE_BIT]);
        }
}

void    read_jmodes(GtkWidget *jmodes[3][NUM_JMODEBITS], USHORT *um, USHORT *gm, USHORT *om)
{
        int     bcnt;

        *um = *gm = *om = 0;

        for  (bcnt = 0;  bcnt < NUM_JMODEBITS;  bcnt++)  {
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jmodes[0][bcnt])))
                        *um |= 1 << bcnt;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jmodes[1][bcnt])))
                        *gm |= 1 << bcnt;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jmodes[2][bcnt])))
                        *om |= 1 << bcnt;
        }
}

void    read_vmodes(GtkWidget *vmodes[3][NUM_VMODEBITS], USHORT *um, USHORT *gm, USHORT *om)
{
        int     bcnt;

        *um = *gm = *om = 0;

        for  (bcnt = 0;  bcnt < NUM_VMODEBITS;  bcnt++)  {
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vmodes[0][bcnt])))
                        *um |= 1 << bcnt;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vmodes[1][bcnt])))
                        *gm |= 1 << bcnt;
                if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vmodes[2][bcnt])))
                        *om |= 1 << bcnt;
        }
}
