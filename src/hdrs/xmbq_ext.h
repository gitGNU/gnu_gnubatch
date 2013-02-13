/* xmbq_ext.h -- extern defs for gbch-xmq

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

extern  char    *spdir;         /* Spool directory, typically /usr/spool/batch */

extern  int     Ctrl_chan;
extern  Shipc   Oreq;

extern  int             Const_val;
extern  BtjobRef        cjob;
extern  BtvarRef        cvar;

/* X stuff */

extern  Widget  jtitwid,        /* Job title window */
                vtitwid,        /* Var title window */
                jwid,           /* Job scroll list */
                vwid;           /* Variable scroll list */

extern  XtIntervalId    Ptimeout;

#define VLINES          2       /* Number of lines we burn up */

extern void  CreateActionEndDlg(Widget, Widget, XtCallbackProc, int);
extern void  CreateEditDlg(Widget, char *, Widget *, Widget *, Widget *, const int);
extern void  InitsearchDlg(Widget, Widget *, Widget *, Widget *, char *);
extern void  InitsearchOpts(Widget, Widget, int, int, int);
extern void  arrow_incr(Widget, XtIntervalId *);
extern void  arrow_decr(Widget, XtIntervalId *);
extern void  cb_advtime();
extern void  cb_arith(Widget, int);
extern void  cb_defass(Widget);
extern void  cb_defcond(Widget);
extern void  cb_deftime(Widget);
extern void  cb_freeze(Widget, const int);
extern void  cb_hols(Widget);
extern void  cb_interps(Widget);
extern void  cb_jact(Widget, int);
extern void  cb_jargs(Widget);
extern void  cb_jass(Widget);
extern void  cb_jconds(Widget);
extern void  cb_jcreate();
extern void  cb_jenv(Widget);
extern void  cb_jkill(Widget, int);
extern void  cb_jmail(Widget);
extern void  cb_jperm(Widget);
extern void  cb_jredirs(Widget);
extern void  cb_jstate(Widget, int);
extern void  cb_macroj(Widget, int);
extern void  cb_macrov(Widget, int);
extern void  cb_procpar(Widget);
extern void  cb_rsrch(Widget, int);
extern void  cb_runtime(Widget);
extern void  cb_saveformats(Widget, const int);
extern void  cb_setconst(Widget);
extern void  cb_setjdisplay(Widget);
extern void  cb_setvdisplay(Widget, int);
extern void  cb_srchfor(Widget);
extern void  cb_stime(Widget);
extern void  cb_syserr();
extern void  cb_titprill(Widget);
extern void  cb_unqueue(Widget);
extern void  cb_vact(Widget, int);
extern void  cb_vass(Widget);
extern void  cb_vcomm(Widget);
extern void  cb_vcreate(Widget);
extern void  cb_vexport(Widget, int);
extern void  cb_view();
extern void  cb_viewopt(Widget);
extern void  cb_vperm(Widget);
extern void  cb_vrename(Widget);
extern void  qdojerror(unsigned, BtjobRef);
extern void  getitemsel(Widget, int, int, char *, char **(*)(), void (*)());
extern void  msg_error();
extern void  vselect(Widget, int, XmListCallbackStruct *);
extern void  qwjimsg(const unsigned, CBtjobRef);
extern void  wjmsg(const unsigned, const ULONG);
extern int  chk_okcidel(const int);
extern char **gen_rvars(char *);
extern char **gen_wvars(char *);
extern char **gen_rvarse(char *);
extern char **gen_wvarse(char *);
extern char **gen_rvarsa(char *);
extern char **gen_wvarsa(char *);
extern char **listcis(char *);
extern char *get_jobtitle();
extern char *get_vartitle();
extern const char *qtitle_of(CBtjobRef);

extern BtjobRef  getselectedjob(unsigned);
extern BtvarRef  getselectedvar(unsigned);

extern Widget  CreateJeditDlg(Widget, char *, Widget *, Widget *, Widget *);
extern Widget  CreateJtitle(Widget, BtjobRef);
extern Widget  CreateIselDialog(Widget, Widget, char *);
extern Widget  CreateUselDialog(Widget, Widget, char *, int, unsigned);
extern Widget  CreateGselDialog(Widget, Widget, char *, int, unsigned);
