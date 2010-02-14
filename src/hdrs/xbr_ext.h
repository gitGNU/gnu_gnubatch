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

struct	pend_job	{
	char		*directory;
	char		*cmdfile_name;
	char		*jobfile_name;
	char		*jobqueue;
	int		changes;
	int		nosubmit;
	char		Verbose;
	int_ugid_t	userid;
	int_ugid_t	grpid;
	Btjob		*job;
};

#define	DEF_DLG_HPAD	5
#define	DEF_DLG_VPAD	5
#define	DEF_BUTTON_PAD	3

extern	unsigned		pend_njobs, pend_max;
extern	struct	pend_job	*pend_list;
extern	struct	pend_job	default_pend;
extern	Btjob			default_job;
extern	USHORT			def_assflags, defavoid;

extern	char	xterm_edit;	/* Invoke "xterm" to run editor */
extern	char	*editor_name;	/* Name of favourite editor */

extern	char	*spdir;		/* Spool directory, typically /usr/spool/batch */

extern	int	Ctrl_chan;

/* X stuff */

extern	GtkWidget	*toplevel,
			*jwid;
extern	GtkListStore		*raw_jlist_store;
extern	GtkTreeModelSort	*sorted_jlist_store;

extern	char	*execprog,
		*gtkprog,
		*ldsvprog;

extern	USHORT	Save_umask;

#define	JLIST_SEQ_COL		0
#define	JLIST_PROGRESS_COL	1
#define	JLIST_TITLE_COL		2
#define	JLIST_CMDFILE_COL	3
#define	JLIST_JOBFILE_COL	4
#define	JLIST_DIRECT_COL	5
#define	JLIST_UNSAVED_COL	6

extern	int	Dirty;		/* Unsaved changes */

extern	void	init_defaults();
extern	void	job_initialise(struct pend_job *, char *, char *);
extern	void	load_options();
extern	void	doinfo(int);
extern	void	update_title(struct pend_job *);
extern	void	update_state(struct pend_job *);
extern	int	getselectedjob(const int);
extern	int	job_load(struct pend_job *);
extern	int  	job_save(struct pend_job *);
extern	GtkWidget *start_jobdlg(struct pend_job *, const int, const int);
extern	struct  pend_job  *job_or_deflt(GtkAction *);
extern	void	note_changes(struct pend_job *);
extern	int	jlist_dirty();
extern	char 	*gen_path(char *, char *);

#define	DEFAULT_EDITOR_NAME	"vi"
