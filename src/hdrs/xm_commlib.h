/* xm_commlib.h -- common library stuff for Motif libs

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

extern  char            *Curr_pwd;      /* Directory on entry */

extern  XtAppContext    app;
extern  Display         *dpy;

extern  Widget  toplevel;       /* Main window */

extern  XtIntervalId    arrow_timer;
extern  int     arr_rtime, arr_rint;

#ifndef HAVE_XM_SPINB_H
extern  unsigned        arrow_min,
                        arrow_max,
                        arrow_lng;
#endif

#define       CLEAR_ARROW_TIMER       if  (arrow_timer) {\
                                      XtRemoveTimeOut(arrow_timer);\
                                      arrow_timer = (XtIntervalId) 0;\
                                      }

#define WORKW_EDITORW   0
#define WORKW_XTERMW    1

#define WORKW_QTXTW     0       /* Queue text window */
#define WORKW_UTXTW     1       /* User text window */
#define WORKW_GTXTW     2       /* Group text window */
#define WORKW_INCNULL   3       /* Include null toggle */
#define WORKW_LOCO      4       /* Local only */
#define WORKW_CONFA     5       /* Confirm abort */

#define WORKW_TITTW     1       /* Job Title window */
#define WORKW_PRITW     2       /* Priority */
#define WORKW_ITXTW     6       /* Command interpreter */
#define WORKW_LLTXTW    7       /* Load level */
#define WORKW_CINAME    8
#define WORKW_CIPATH    9
#define WORKW_CIARGS    10
#define WORKW_CINICE    11
#define WORKW_CISETARG0 12
#define WORKW_CIEXPAND  13

#define WORKW_DIRTXTW   0       /* Directory */
#define WORKW_ULIMIT    1
#define WORKW_NORML     2
#define WORKW_NORMU     3
#define WORKW_ERRL      4
#define WORKW_ERRU      5
#define WORKW_ADVTERR   6
#define WORKW_NOADVTERR 7
#define WORKW_NOEXPORT  8
#define WORKW_EXPORT    9
#define WORKW_REMRUN    10

#define WORKW_DELTW     1
#define WORKW_RTHOURW   2
#define WORKW_RTMINW    3
#define WORKW_RTSECW    4
#define WORKW_ROMINW    5
#define WORKW_ROSECW    6

#define WORKW_MAIL      0
#define WORKW_WRITE     1

#define WORKW_HTXTW     0       /* Title window */
#define WORKW_NTXTW     1       /* Name window */
#define WORKW_CTXTW     2       /* Comment window */
#define WORKW_VTXTW     3       /* Value window */
#define WORKW_STXTW     4       /* Search text window */
#define WORKW_FORWW     5       /* Forward selection */
#define WORKW_MATCHW    6       /* Match exactly */
#define WORKW_WRAPW     7       /* Wrap around */
#define WORKW_JBUTW     8       /* Search for jobs */
#define WORKW_VBUTW     9       /* Search for vars */

#define WORKW_CONVAL    0       /* Var ops constant value */

#define WORKW_VARNAME   0       /* Var name */
#define WORKW_VARVAL    1       /* Var ops value */
#define WORKW_VARCOMM   2       /* Comment value */
#define WORKW_JVARNAME  3       /* Job var name must be diff from UTXTW etc */

#define WORKW_JARGVAL   1
#define WORKW_JRDFD1    2
#define WORKW_JRDFD2    3
#define WORKW_JRDSTDIN  4

#define WORKW_JCFW      8       /* Job command file */
#define WORKW_JJFW      9       /* Job job file */
#define WORKW_JCONLY    10      /* Copy only */

#define WORKW_IDELETE   0
#define WORKW_IRETAIN   1
#define WORKW_IMINUTES  2
#define WORKW_IHOURS    3
#define WORKW_IDAYS     4
#define WORKW_IWEEKS    5
#define WORKW_IMONTHSB  6
#define WORKW_IMONTHSE  7
#define WORKW_IYEARS    8
#define WORKW_SUNDAY    9
#define WORKW_MONDAY    10
#define WORKW_TUESDAY   11
#define WORKW_WEDNESDAY 12
#define WORKW_THURSDAY  13
#define WORKW_FRIDAY    14
#define WORKW_SATURDAY  15
#define WORKW_HOLIDAY   16
#define WORKW_HOURTW    17
#define WORKW_MINTW     18
#define WORKW_DOWTW     19
#define WORKW_DOMTW     20
#define WORKW_MONTW     21
#define WORKW_YEARTW    22
#define WORKW_RINTERVAL 23
#define WORKW_RMTHDAY   24
#define WORKW_RESULTREP 25
#define WORKW_NPSKIP    26
#define WORKW_NPWAIT1   27
#define WORKW_NPWAITALL 28
#define WORKW_NPCATCHUP 29
#define WORKW_CANC      20
#define WORKW_VERBOSE   21

extern  Widget  workw[];

extern void  cb_chelp(Widget, int, XmAnyCallbackStruct *);
extern void  dohelp(Widget, int);
extern void  doerror(Widget, int);
extern void  getitemsel(Widget, int, int, char *, char **(*)(), void (*)());
extern void  put_textbox_int(Widget, const int, int);
extern void  initcifile();
#if  defined(HAVE_XM_COMBOBOX_H) && !defined(BROKEN_COMBOBOX)
extern void  ilist_cb(Widget, int, XmComboBoxCallbackStruct *);
#else
extern void  ilist_cb(Widget, int, XmSelectionBoxCallbackStruct *);
extern void  getqueuesel(Widget, int);
extern void  getintsel(Widget, int);
extern void  getusersel(Widget, int);
extern void  getgroupsel(Widget, int);
#endif

extern int  Confirm(Widget, int);
extern int  get_textbox_int(Widget);
extern int  sort_qug(char **, char **);

extern char **gen_qlist(char *);
extern char **listcis(char *);
extern char *makebigvec(char **);

extern Widget  CreateArrowPair(char *, Widget, Widget, Widget, XtCallbackProc, XtCallbackProc, int, int);
extern Widget  CreateDselPane(Widget, Widget, char *);
extern Widget  FindWidget(Widget);
extern Widget  GetTopShell(Widget);
extern Widget  place_label_topleft(Widget, char *);
extern Widget  place_label_left(Widget, Widget, char *);
extern Widget  place_label_right(Widget, Widget, char *);
extern Widget  place_label_top(Widget, Widget, char *);
extern Widget  place_label(Widget, Widget, Widget, char *);

#ifdef HAVE_XM_SPINB_H
extern int  get_spinbox_int(Widget);
extern void  put_spinbox_int(Widget, const int);
#define GET_TEXTORSPINBOX_INT(W)        get_spinbox_int(W)
#define PUT_TEXTORSPINBOX_INT(W,V,WD)   put_spinbox_int(W,V)
#else
void  arrow_incr(Widget, XtIntervalId *);
void  arrow_decr(Widget, XtIntervalId *);
#define GET_TEXTORSPINBOX_INT(W)        get_textbox_int(W)
#define PUT_TEXTORSPINBOX_INT(W,V,WD)   put_textbox_int(W,V,WD)
#endif

#define WW(X)   workw[X]
