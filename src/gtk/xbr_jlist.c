/* xbr_jlist.c -- job list handling for gbch-xr

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
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
#include "q_shm.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "jvuprocs.h"
#include "xbr_ext.h"
#include "gtk_lib.h"
#include "stringvec.h"
#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

static	char	Filename[] = __FILE__;

#define	NAMESIZE	14	/* Padding for temp file */

#define	C_MASK		037

static	char	*tmpfl;

unsigned		pend_njobs, pend_max;
struct	pend_job	*pend_list;

extern	BtuserRef	mypriv;
extern	char		*Curr_pwd;

char		*execprog, *ldsvprog, *gtkprog;
HelpaltRef	days_abbrev, daynames_full, monnames, repunit, ifnposs_names, repunit_full;
char		*exitcodename, *signalname, *delend_full, *retend_full;
char		*no_title, *no_name;
#ifdef	SUFFIXES
char		*cmd_prefix, *cmd_suffix, *job_prefix, *job_suffix;
#endif

ULONG		default_rate;
unsigned char	default_interval,
		default_nposs;
USHORT		defavoid, def_assflags;

#define	ppermitted(flg)	(mypriv->btu_priv & flg)

HelpaltRef	progresslist;
extern	HelpaltRef	assnames, actnames, stdinnames;

#ifdef	HAVE_SYS_RESOURCE_H
extern	int	Max_files;
#endif

int  jlist_dirty()
{
	int	cnt;

	for  (cnt = 0;  cnt < pend_njobs;  cnt++)
		if  (pend_list[cnt].changes != 0)
			return  1;
	return  0;
}

/* Generate list of queue names in ql */

void  gen_qlist(struct stringvec *ql)
{
	int	jcnt;

	rjobfile(1);

	stringvec_init(ql);
	stringvec_append(ql, "");

	for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
		const	char	*tit = title_of(jj_ptrs[jcnt]);
		const	char	*ip;
		char	*it;
		unsigned	lng;
		if  (!(ip = strchr(tit, ':')))
			continue;
		lng = ip - tit + 1;
		if  (!(it = malloc(lng)))
			ABORT_NOMEM;
		strncpy(it, tit, lng-1);
		it[lng-1] = '\0';
		stringvec_insert_unique(ql, it);
		free(it);
	}

	for  (jcnt = 0;  jcnt < pend_njobs;  jcnt++)
		if  (pend_list[jcnt].jobqueue)
			stringvec_insert_unique(ql, pend_list[jcnt].jobqueue);
}

void  initmoremsgs()
{
	int	n;
#ifdef	HAVE_SYS_RESOURCE_H
	struct	rlimit	cfiles;
	getrlimit(RLIMIT_NOFILE, &cfiles);
	Max_files = cfiles.rlim_cur;
#endif
	if  (!(tmpfl = malloc((unsigned)(strlen(spdir) + 2 + NAMESIZE))))
		ABORT_NOMEM;

	if  (!(progresslist = helprdalt($Q{Job progress code})))  {
		disp_arg[9] = $Q{Job progress code};
		print_error($E{Missing alternative code});
	}
	if  (!(assnames = helprdalt($Q{Assignment names})))  {
		disp_arg[9] = $Q{Assignment names};
		print_error($E{Missing alternative code});
	}
	delend_full = gprompt($P{Delete at end full});
	retend_full = gprompt($P{Retain at end full});
	repunit_full = helprdalt($Q{Repeat unit full});
	ifnposs_names = helprdalt($Q{Wtime if not poss});
	exitcodename = gprompt($P{Assign exit code});
	signalname = gprompt($P{Assign signal});
	no_title = gprompt($P{xbtr no title});
	no_name = gprompt($P{xbtr no file name});
	if  (!(actnames = helprdalt($Q{Redirection type names})))  {
		disp_arg[9] = $Q{Redirection type names};
		print_error($E{Missing alternative code});
	}
	if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
		disp_arg[9] = $Q{Weekdays};
		print_error($E{Missing alternative code});
	}
	daynames_full = helprdalt($Q{Weekdays full});
	monnames = helprdalt($Q{Months full});
	if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
		disp_arg[9] = $Q{Repeat unit abbrev};
		print_error($E{Missing alternative code});
	}
	if  ((n = helpnstate($N{Default repeat alternative})) < 0  ||  n > TC_YEARS)
		n = TC_RETAIN;
	default_interval = (unsigned char) n;
	if  ((n = helpnstate($N{Default number of units})) <= 0)
		n = 10;
	default_rate = (ULONG) n;
	if  ((n = helpnstate($N{Default skip delay option})) < 0  ||  n > TC_CATCHUP)
		n = TC_WAIT1;
	default_nposs = (unsigned char) n;
	defavoid = 0;
	for  (n = 0;  n < TC_NDAYS;  n++)
		if  (helpnstate($N{Base for days to avoid}+n) > 0)
			defavoid |= 1 << n;

#ifdef	SUFFIXES
	job_prefix = helpprmpt($P{Default job file prefix});
	job_suffix = helpprmpt($P{Default job file suffix});
	cmd_prefix = helpprmpt($P{Default cmd file prefix});
	cmd_suffix = helpprmpt($P{Default cmd file suffix});
	if  (!job_prefix)
		job_prefix = "";
	if  (!job_suffix)
		job_suffix = "";
	if  (!cmd_prefix)
		cmd_prefix = "";
	if  (!cmd_prefix)
		cmd_suffix = "";
#endif

	ldsvprog = envprocess(GTKLDSAV);
	execprog = envprocess(EXECPROG);
	gtkprog = envprocess(GTKSAVE);
	Save_umask = umask(C_MASK);
}

/* Update job title display */

void  update_title(struct pend_job *pj)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreeIter  iter, citer;

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		const  char  *ip = title_of(pj->job);
		if  (strlen(ip) == 0)
			ip = no_title;
		gtk_tree_model_sort_convert_iter_to_child_iter(sorted_jlist_store, &citer, &iter);
		gtk_list_store_set(raw_jlist_store, &citer, JLIST_TITLE_COL, ip, -1);
	}
}

void  update_state(struct pend_job *pj)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreeIter  iter, citer;
	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		gtk_tree_model_sort_convert_iter_to_child_iter(sorted_jlist_store, &citer, &iter);
		gtk_list_store_set(raw_jlist_store, &citer, JLIST_PROGRESS_COL, disp_alt(pj->job->h.bj_progress, progresslist), -1);
	}
}

int  getselectedjob(const int moan)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreeIter  iter;

	if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
		guint  seq;
		gtk_tree_model_get(GTK_TREE_MODEL(sorted_jlist_store), &iter, JLIST_SEQ_COL, &seq, -1);
		return  (int) seq;
	}
	if  (moan)
		doerror(pend_njobs != 0? $EH{xmbtq no job selected}: $EH{xmbtq no jobs to select});
	return  -1;
}

struct pend_job *job_or_deflt(GtkAction *action)
{
	const	char	*act = gtk_action_get_name(action);
	if  (act[strlen(act)-1] != 'd')  {
		int	indx;
		if  ((indx = getselectedjob(1)) < 0)
			return  (struct pend_job *) 0;
		return  &pend_list[indx];
	}
	return  &default_pend;
}

#define	INIT_PEND	20
#define	INC_PEND	5

static void  chkalloc()
{
	if  (pend_njobs >= pend_max)  {
		if  (pend_max == 0)  {
			pend_max = INIT_PEND;
			pend_list = (struct pend_job *) malloc(INIT_PEND * sizeof(struct pend_job));
		}
		else  {
			pend_max += INC_PEND;
			pend_list = (struct pend_job *) realloc((char *) pend_list, INC_PEND * sizeof(struct pend_job));
		}
		if  (!pend_list)
			ABORT_NOMEM;
	}
}

static void  append_job(struct pend_job *pj)
{
	GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
	GtkTreeIter	iter, piter;
	const  char	*tit;
	char	*cf, *jf;

	tit = title_of(pj->job);
	if  (strlen(tit) == 0)
		tit = no_title;
	cf = !pj->cmdfile_name || strlen(pj->cmdfile_name) == 0? no_name: pj->cmdfile_name;
	jf = !pj->jobfile_name || strlen(pj->jobfile_name) == 0? no_name: pj->jobfile_name;

	gtk_list_store_append(raw_jlist_store, &iter);
	gtk_list_store_set(raw_jlist_store, &iter,
			   JLIST_SEQ_COL,	pend_njobs,
			   JLIST_PROGRESS_COL,	disp_alt(pj->job->h.bj_progress, progresslist),
			   JLIST_TITLE_COL, 	tit,
			   JLIST_CMDFILE_COL,	cf,
			   JLIST_JOBFILE_COL,	jf,
			   JLIST_DIRECT_COL,	pj->directory,
			   JLIST_UNSAVED_COL,	pj->changes != 0,
			   -1);

	gtk_tree_model_sort_convert_child_iter_to_iter(sorted_jlist_store, &piter, &iter);
	gtk_tree_selection_select_iter(sel, &piter);
}

void  cb_jnew()
{
	struct	pend_job  *pj;

	chkalloc();
	pj = &pend_list[pend_njobs];
	job_initialise(pj, stracpy(Curr_pwd), (char *) 0);
	append_job(pj);
	pend_njobs++;
}

char *gen_path(char *dir, char *fil)
{
	char	*result;
	if  (!(result = malloc((unsigned) (strlen(dir) + strlen(fil) + 2))))
		ABORT_NOMEM;
	sprintf(result, "%s/%s", dir, fil);
	return  result;
}


static gboolean  isit_cmd(const GtkFileFilterInfo *filter_info, gpointer data)
{
	FILE	*fp;
	struct	stat	sbuf;
	char	Pbuf[2000];

	if  (stat(filter_info->filename, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFREG  ||  sbuf.st_size == 0)
		return  FALSE;

	sprintf(Pbuf, "%s -r %s", ldsvprog, filter_info->filename);

	fp = popen(Pbuf, "r");
	if  (!fp)
	     return  FALSE;

	while  (fgets(Pbuf, sizeof(Pbuf), fp))  {
		char	*lp;
		if  (Pbuf[0] == '#')
			continue;
		if  ((lp = strchr(Pbuf, '\n')))
			*lp = '\0';
		if  (strncmp(Pbuf, BTR_PROGRAM " ", sizeof(BTR_PROGRAM)) == 0)  {
			pclose(fp);
			return  TRUE;
		}
		if  (!strchr(Pbuf, '='))  {
			pclose(fp);
			return	FALSE;
		}
	}
	pclose(fp);
	return  FALSE;
}

void  cb_jopen()
{
	GtkWidget  *fsb;
	char	*pr;
	static	GtkFileFilter  *isit_cmd_filt = 0;

	pr = gprompt($P{xbtr open job dlgtit});
	fsb = gtk_file_chooser_dialog_new(pr, GTK_WINDOW(toplevel),
					  GTK_FILE_CHOOSER_ACTION_OPEN,
					  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  NULL);
	free(pr);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsb), Curr_pwd);

	if  (!isit_cmd_filt)  {
		pr = gprompt($P{xbtr poss job cmd files});
		isit_cmd_filt = gtk_file_filter_new();
		gtk_file_filter_set_name(isit_cmd_filt, pr);
		free(pr);
		gtk_file_filter_add_custom(isit_cmd_filt, GTK_FILE_FILTER_FILENAME, (GtkFileFilterFunc) isit_cmd, NULL, NULL);
	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fsb), isit_cmd_filt);
	g_object_ref(G_OBJECT(isit_cmd_filt));

	while  (gtk_dialog_run(GTK_DIALOG(fsb)) == GTK_RESPONSE_ACCEPT)  {
		char  *pathname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsb));
		char  *sp = strrchr(pathname, '/');
		char  *dirname = pathname;
		char  *filename = sp+1;
		int	cnt, ret;

		if  (sp)  {
			if  (sp == pathname)
				dirname = "/";
			*sp = '\0';
		}
		else  {
			dirname = Curr_pwd;
			filename = pathname;
		}
		for  (cnt = 0;  cnt < pend_njobs;  cnt++)  {
			if  (pend_list[cnt].directory  &&  strcmp(dirname, pend_list[cnt].directory) == 0  &&
			     pend_list[cnt].cmdfile_name  &&  strcmp(filename, pend_list[cnt].cmdfile_name) == 0)  {
				doerror($EH{xmbtr file already loaded});
				g_free(pathname);
				goto  endl;
			}
		}

		chkalloc();
		job_initialise(&pend_list[pend_njobs], stracpy(dirname), stracpy(filename));

		/* Carry on even if we have an error.  We may as well include the thing.  */

		if  ((ret = job_load(&pend_list[pend_njobs])))
			doerror(ret);
		else
			pend_list[pend_njobs].changes = 0;

		append_job(&pend_list[pend_njobs]);
		pend_njobs++;
		break;
	endl:
		;
	}
	gtk_widget_destroy(fsb);
}

void  cb_jclosedel(GtkAction *action)
{
	int		indx, isdel;
	unsigned	cnt;
	struct		pend_job	*pj;
	GtkTreeIter	iter;

	if  ((indx = getselectedjob(1)) < 0)
		return;

	/* indx should be the right place in raw_jlist_store */

	isdel = strcmp(gtk_action_get_name(action), "Delete") == 0;

	pj = &pend_list[indx];
	if  (pj->changes > 0  &&  !Confirm(isdel? $PH{xmbtr confirm delete}: $PH{xmbtr confirm close}))
		return;

	if  (isdel)  {
		GString	*cmdname = g_string_new(NULL);

		if  (pj->jobfile_name)  {
			g_string_printf(cmdname, "%s -d %s/%s", ldsvprog, pj->directory, pj->jobfile_name);
			if  (system(cmdname->str) != 0)
				doerror($EH{xmbtr cannot delete job file});
		}
		if  (pj->cmdfile_name)  {
			g_string_printf(cmdname, "%s -d %s/%s", ldsvprog, pj->directory, pj->cmdfile_name);
			if  (system(cmdname->str) != 0)
				doerror($EH{xmbtr cannot delete cmd file});
		}
		g_string_free(cmdname, TRUE);
	}

	if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, indx))
		gtk_list_store_remove(raw_jlist_store, &iter);

	free((char *) pj->job);
	free(pj->directory);
	if  (pj->jobfile_name)
		free(pj->jobfile_name);
	if  (pj->cmdfile_name)
		free(pj->cmdfile_name);
	pend_njobs--;

	/* Move up all the jobs below that and change the index */

	for  (cnt = indx;  cnt < pend_njobs;  cnt++)  {
		pend_list[cnt] = pend_list[cnt+1];
		if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, cnt))
			gtk_list_store_set(raw_jlist_store, &iter, JLIST_SEQ_COL, cnt, -1);
	}
}

void  cb_jcmdfile(GtkAction *action)
{
	int		isjobfile = strcmp(gtk_action_get_name(action), "Jobfile") == 0;
	int		indx;
	GtkWidget	*fsb;
	char		*pr;
	struct	pend_job	*pj;

	if  ((indx = getselectedjob(1)) < 0)
		return;

	pj = &pend_list[indx];

	pr = gprompt(isjobfile? $P{xbtr set job file dlgtit}: $P{xbtr set cmd file dlgtit});
	fsb = gtk_file_chooser_dialog_new(pr, GTK_WINDOW(toplevel),
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  NULL);
	free(pr);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fsb), TRUE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsb), pj->directory);
	if  (gtk_dialog_run(GTK_DIALOG(fsb)) == GTK_RESPONSE_ACCEPT)  {
		char  *pathname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsb));
		char  *sp = strrchr(pathname, '/');
		char  *dirname = pathname;
		char  *filename = sp+1;
		int	col;
		GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
		GtkTreeIter	iter, citer;

		if  (sp)  {
			if  (sp == pathname)
				dirname = "/";
			*sp = '\0';
		}
		else  {
			dirname = pj->directory;
			filename = pathname;
		}
		if  (strcmp(pj->directory, dirname) != 0)  {
			free(pj->directory);
			pj->directory = stracpy(dirname);
		}
		if  (isjobfile)  {
			if  (pj->jobfile_name)
				free(pj->jobfile_name);
			pj->jobfile_name = stracpy(filename);
			col = JLIST_JOBFILE_COL;
		}
		else  {
			if  (pj->cmdfile_name)
				free(pj->cmdfile_name);
			pj->cmdfile_name = stracpy(filename);
			col = JLIST_CMDFILE_COL;
		}
		if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
			gtk_tree_model_sort_convert_iter_to_child_iter(sorted_jlist_store, &citer, &iter);
			gtk_list_store_set(raw_jlist_store, &citer, JLIST_DIRECT_COL, dirname, col, filename, -1);
		}
		g_free(pathname);
	}
	gtk_widget_destroy(fsb);
}

void  cb_jsave()
{
	int	indx, ret;
	struct	pend_job	*pj;
	char	*jfpath;

	if  ((indx = getselectedjob(1)) < 0)
		return;
	pj = &pend_list[indx];
	if  (pj->changes <= 0  &&  !Confirm($PH{xmbtr file save confirm}))
		return;
	if  (!pj->jobfile_name)  {
		doerror($EH{xmbtr no job file name});
		return;
	}
	if  (!pj->cmdfile_name)  {
		doerror($EH{xmbtr no cmd file name});
		return;
	}

	jfpath = gen_path(pj->directory, pj->jobfile_name);
	ret = access(jfpath, F_OK);
	free(jfpath);
	if  (ret < 0)  {
		doerror($EH{xbtr no job file set up});
		return;
	}
	if  ((ret = job_save(pj)))
		doerror(ret);
	else  {
		GtkTreeIter	iter;
		pj->changes = 0;
		if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, indx))
			gtk_list_store_set(raw_jlist_store, &iter, JLIST_UNSAVED_COL, FALSE, -1);
	}
}

void  note_changes(struct pend_job *pj)
{
	GtkTreeIter	iter;

	pj->changes++;
	pj->nosubmit++;
	if  (pj == &default_pend)
		return;
	if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, pj - pend_list))
		gtk_list_store_set(raw_jlist_store, &iter, JLIST_UNSAVED_COL, TRUE, -1);
}

void  cb_edit()
{
	int	indx;
	struct	pend_job	*pj;
	char	*path;
	PIDTYPE	pid;

	if  ((indx = getselectedjob(1)) < 0)
		return;
	pj = &pend_list[indx];
	if  (!pj->directory || !pj->jobfile_name)  {
		doerror($EH{xmbtr no job file name});
		return;
	}
	path = gen_path(pj->directory, pj->jobfile_name);

	if  ((pid = fork()) != 0)  {
		int	status;
		free(path);
		if  (pid < 0)  {
			doerror($EH{xmbtr cannot fork for editor});
			return;
		}
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, &status, 0) < 0)
			;
#else
		while  (wait(&status) != pid)
			;
#endif
		if  (status != 0)  {
			disp_str = editor_name;
			doerror($EH{xmbtr cannot execute editor});
		}
		return;
	}

	umask((int) Save_umask);
	chdir(pj->directory);
	if  (xterm_edit)  {
		char	*termname = envprocess("${XTERM:-xterm}");
		execl(execprog, termname, "-e", editor_name, path, (char *) 0);
	}
	else
		execl(execprog, editor_name, path, (char *) 0);
	exit(255);
}

void  cb_direct()
{
	GtkWidget	*dlg;
	char		*pr;

	pr = gprompt($P{xbtr select working dir});
	dlg = gtk_file_chooser_dialog_new(pr,
					  NULL,
					  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					  GTK_STOCK_OK, GTK_RESPONSE_OK,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  NULL);
	free(pr);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), Curr_pwd);
	if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
		gchar  *selected = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		free(Curr_pwd);
		Curr_pwd = stracpy(selected);
		g_free(selected);
	}
	gtk_widget_destroy(dlg);
}

/* Generate output file name */

static FILE *goutfile(jobno_t *jn)
{
	FILE	*res;
	int	fid;

	for  (;;)  {
		sprintf(tmpfl, "%s/%s", spdir, mkspid(SPNAM, *jn));
		if  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0666)) >= 0)
			break;
		*jn += JN_INC;
	}
	if  ((res = fdopen(fid, "w")) == (FILE *) 0)  {
		unlink(tmpfl);
		ABORT_NOMEM;
	}
	return  res;
}

void  cb_submit()
{
	int			indx, ch;
	ULONG			jindx;
	struct	pend_job	*pj;
	BtjobRef		jreq;
	char			*path;
	FILE			*outf, *inf;
	GString			*readcmd;
	static	jobno_t		jn;
	Shipc			Oreq;
	Repmess			rr;
	extern	long		mymtype;

	if  ((indx = getselectedjob(1)) < 0)
		return;
	pj = &pend_list[indx];
	if  (pj->nosubmit == 0  &&  !Confirm($PH{xmbtr job unchanged confirm}))
		return;
	if  (pj->job->h.bj_times.tc_istime  &&  pj->job->h.bj_times.tc_nexttime < time((time_t *) 0))  {
		doerror($EH{xmbtr cannot submit not future});
		return;
	}
	if  (!pj->jobfile_name)  {
		doerror($EH{xmbtr no job file name});
		return;
	}
	path = gen_path(pj->directory, pj->jobfile_name);
	if  (access(path, F_OK))  {
		free(path);
		doerror($EH{xmbtr no such file});
		return;
	}

	readcmd = g_string_new(NULL);
	g_string_printf(readcmd, "%s -r %s", ldsvprog, path);
	inf = popen(readcmd->str, "r");
	g_string_free(readcmd, TRUE);
	free(path);

	if  (!inf)  {
		doerror($EH{xmbtr cannot open job file});
		return;
	}

	jn = jn? jn+1: getpid();
	outf = goutfile(&jn);

	while  ((ch = getc(inf)) != EOF)  {
		if  (putc(ch, outf) == EOF)  {
			unlink(tmpfl);
			pclose(inf);
			fclose(outf);
			doerror($EH{No room for job});
			return;
		}
	}

	fclose(outf);
	if  (pclose(inf) != 0)  {
		unlink(tmpfl);
		doerror($EH{xmbtr cannot open job file});
		return;
	}

	jreq = &Xbuffer->Ring[jindx = getxbuf()];
	BLOCK_COPY(jreq, pj->job, sizeof(Btjob));
	jreq->h.bj_slotno = -1;
	jreq->h.bj_job = jn;
	time(&jreq->h.bj_time);

	Oreq.sh_mtype = TO_SCHED;
	Oreq.sh_params.mcode = J_CREATE;
	Oreq.sh_params.uuid = pj->userid;
	Oreq.sh_params.ugid = pj->grpid;
	Oreq.sh_params.hostid = 0;
	Oreq.sh_params.param = 0;
	Oreq.sh_un.sh_jobindex = jindx;
	Oreq.sh_params.upid = getpid();
#ifdef	USING_MMAP
	sync_xfermmap();
#endif
	if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) < 0)  {
		int	savee = errno;
		unlink(tmpfl);
		freexbuf(jindx);
		errno = savee;
		doerror(savee == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
		return;
	}

	while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
		if  (errno == EINTR)
			continue;
		freexbuf(jindx);
		doerror($E{Error on IPC});
		return;
	}

	freexbuf(jindx);

	if  (rr.outmsg.mcode != J_OK) {
		unlink(tmpfl);
		if  ((rr.outmsg.mcode & REQ_TYPE) != JOB_REPLY)  { /* Not expecting net errors */
			disp_arg[0] = rr.outmsg.mcode;
			doerror($EH{Unexpected sched message});
		}
		else  {
			disp_str = title_of(pj->job);
			doerror((int) ((rr.outmsg.mcode & ~REQ_TYPE) + $EH{Base for scheduler job errors}));
		}
		return;
	}

	if  (pj->Verbose)  {
		disp_arg[0] = jn;
		disp_str = title_of(pj->job);
		doinfo(disp_str[0]? $E{xmbtr job created ok title}: $E{xmbtr job created ok no title});
	}
	pj->nosubmit = 0;
}
