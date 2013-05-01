/* xmbr_ext.h -- External symbols for xmbtr

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

struct  pend_job        {
        char            *directory;
        char            *cmdfile_name;
        char            *jobfile_name;
        char            *jobqueue;
        int             changes;
        int             nosubmit;
        char            Verbose;
        int_ugid_t      userid;
        int_ugid_t      grpid;
        Btjob           *job;
};

#define JCMDFILE_JOB    1
#define JCMDFILE_CMD    2

extern  unsigned                pend_njobs, pend_max;
extern  struct  pend_job        *pend_list;
extern  struct  pend_job        default_pend, *cjob;
extern  Btjob                   default_job;
extern  USHORT                  def_assflags, defavoid;

extern  char    xterm_edit;     /* Invoke "xterm" to run editor */
extern  char    *editor_name;   /* Name of favourite editor */

extern  char    *spdir;         /* Spool directory, typically /usr/spool/batch */

extern  int     Ctrl_chan;

/* X stuff */

extern  Widget  jtitwid,        /* Job title window */
                jwid;           /* Job scroll list */

extern  XtIntervalId    Ptimeout;

extern void  cb_defhost(Widget, int);
extern void  cb_direct(Widget, int);
extern void  cb_djstate(Widget, int);
extern void  cb_jargs(Widget, int);
extern void  cb_jass(Widget, int);
extern void  cb_jclosedel(Widget, int);
extern void  cb_jcmdfile(Widget, int);
extern void  cb_jconds(Widget, int);
extern void  cb_jdstate(Widget, int);
extern void  cb_edit(Widget, int);
extern void  cb_jenv(Widget, int);
extern void  cb_jmail(Widget, int);
extern void  cb_jnew(Widget, int);
extern void  cb_jopen(Widget, int);
extern void  cb_jperm(Widget, int);
extern void  cb_jqueue(Widget, int);
extern void  cb_jredirs(Widget, int);
extern void  cb_jsave(Widget, int);
extern void  cb_jstate(Widget, int);
extern void  cb_loaddefs(Widget, int);
extern void  cb_procpar(Widget, int);
extern void  cb_remsubmit(Widget, int);
extern void  cb_runtime(Widget, int);
extern void  cb_savedefs(Widget, int);
extern void  cb_stime(Widget, int);
extern void  cb_submit(Widget, int);
extern void  cb_titprill(Widget, int);
extern void  cb_viewopt(Widget, int);
extern void  CreateActionEndDlg(Widget, Widget, XtCallbackProc, int);
extern void  CreateEditDlg(Widget, char *, Widget *, Widget *, Widget *, const int);
extern void  doinfo(Widget, int);
extern void  init_defaults();
extern void  isit_cmdfile(Widget, XtPointer);
extern void  job_initialise(struct pend_job *, char *, char *);
extern void  load_options();

extern int  f_exists(const char *);
extern int  getselectedjob(const int);
extern int  job_load(struct pend_job *);
extern int  job_save(struct pend_job *);

extern char **gen_rvars(char *);
extern char **gen_wvars(char *);
extern char **gen_rvarse(char *);
extern char **gen_wvarse(char *);
extern char **gen_rvarsa(char *);
extern char **gen_wvarsa(char *);
extern char **listcis(char *);

extern XmString  jdisplay(struct pend_job *);

extern Widget  CreateIselDialog(Widget, Widget, char *);
extern Widget  CreateQselDialog(Widget, Widget, char *, int);
extern Widget  CreateGselDialog(Widget, Widget, char *, int, unsigned);
extern Widget  CreateUselDialog(Widget, Widget, char *, int, unsigned);

extern struct pend_job *job_or_deflt(const int);

#define DEFAULT_EDITOR_NAME     "vi"
