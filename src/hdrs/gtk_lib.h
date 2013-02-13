/* gtk_lib.h -- defines for internal GTK library

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

extern  char    *makebigvec(char **);
extern  void    doerror(int);
extern  int     Confirm(int);
extern  void    gtk_chk_uid();

/* Functions to make "gprompt"-stuff easier */

extern  GtkWidget  *gprompt_label(const int);
extern  GtkWidget  *gprompt_button(const int);
extern  GtkWidget  *gprompt_checkbutton(const int);
extern  GtkWidget  *gprompt_radiobutton(const int);
extern  GtkWidget  *gprompt_radiobutton_fromwidget(GtkWidget *, const int);
extern  GtkWidget  *gprompt_dialog(GtkWidget *, const int);
extern  GtkWidget  *slider_with_buttons(GtkWidget *, const int, unsigned, const int);

/* For check buttons where you get a cascade of signals if you set any */

extern  void    setifnotset(GtkWidget *);
extern  void    unsetifset(GtkWidget *);

