/* xbr_ext.h -- External defs for gbch-xr

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
        char            *cmdfile_name;                  /* May be null, will be if XML file */
        char            *jobfile_name;                  /* May be null for new jobs */
        char            *xml_jobfile_name;              /* File name when XML */
        char            *jobscript;                     /* Job script if held in mem */
        char            *jobqueue;                      /* Separate queue */
        char            changes;                        /* Unsaved changes */
        char            nosubmit;
        char            Verbose;
        char            scriptinmem;
        int_ugid_t      userid;
        int_ugid_t      grpid;
        Btjob           *job;
};

#define DEF_DLG_HPAD    5
#define DEF_DLG_VPAD    5
#define DEF_BUTTON_PAD  3

extern  unsigned                pend_njobs, pend_max;
extern  struct  pend_job        *pend_list;
extern  struct  pend_job        default_pend;
extern  Btjob                   default_job;
extern  USHORT                  def_assflags, defavoid;

extern  char  xml_format;     /* Use single XML files */
extern  char  internal_edit;  /* Use internal editor */
extern  char  xterm_edit;     /* Invoke "xterm" to run editor */
extern  char  *editor_name;   /* Name of favourite editor */

extern  char  *spdir;         /* Spool directory, typically /usr/spool/batch */
extern  char  *realuname;      /* Real user name */
extern  int    Ctrl_chan;

/* X stuff */

extern  GtkWidget       *toplevel,
                         *jwid;
extern  GtkListStore            *raw_jlist_store;
extern  GtkTreeModelSort        *sorted_jlist_store;

extern  gint    Mwwidth, Mwheight;

extern  char    *execprog,
                 *gtkprog,
                 *ldsvprog;

extern  USHORT  Save_umask;

#define JLIST_SEQ_COL           0
#define JLIST_PROGRESS_COL      1
#define JLIST_TITLE_COL         2
#define JLIST_JOBFILE_COL       3
#define JLIST_DIRECT_COL        4
#define JLIST_UNSAVED_COL       5

extern  int     Dirty;          /* Unsaved changes */

extern  void    init_defaults();
extern  void    job_initialise(struct pend_job *, char *, char *);
extern  void    load_options();
extern  void    doinfo(int);
extern  void    update_title(struct pend_job *);
extern  void    update_state(struct pend_job *);
extern  int     getselectedjob(const int);
extern  int     job_load(struct pend_job *);
extern  int     xml_job_load(struct pend_job *);
extern  int     job_save(struct pend_job *);
extern  int     job_save_xml(struct pend_job *);
extern  GtkWidget *start_jobdlg(struct pend_job *, const int, const int);
extern  struct  pend_job  *job_or_deflt(GtkAction *);
extern  void    note_changes(struct pend_job *);
extern  int     jlist_dirty();
extern  char    *gen_path(char *, char *);
extern  FILE    *ldsv_open(const char, const char *, const char *);
extern  struct  pend_job        *sub_check();

extern  void     init_hosts_known(char *);
extern  char     *list_hosts_known();
extern   void   set_def_host(char *);
extern  char    *get_def_host();
extern  void    save_state();

#define DEFAULT_EDITOR_NAME     "vi"
