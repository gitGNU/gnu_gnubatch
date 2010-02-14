/* xmbtr.c -- main module for gbch-xmr

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
static	char	rcsid1[] = "@(#) $Id: xmbtr.c,v 1.1 2009/02/18 06:51:32 toadwarble Exp $";		/* We use these in the about message */
static	char	rcsid2[] = "@(#) $Revision: 1.1 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/CascadeB.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/SeparatoGP.h>
#include "incl_sig.h"
#include "incl_net.h"
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "cfile.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "xmmenu.h"

#define	IPC_MODE	0

void  initcifile();

char	*spdir,
	*Curr_pwd;

extern	USHORT	Save_umask;

char	Confvarname[] = "XMBTRCONF";

extern	long	mymtype;

struct	pend_job	default_pend;
Btjob	default_job;

char	xterm_edit;		/* Invoke "xterm" to run editor */

char	*editor_name;	/* Name of favourite editor */

/* X Stuff */

XtAppContext	app;
Display		*dpy;

Widget	toplevel,	/* Main window */
	jtitwid,	/* Job title window */
	jwid;		/* Job scroll list */

static	Widget	panedw,		/* Paned window to stick rest in */
		menubar,	/* Menu */
		toolbar;	/* Optional toolbar */

XtIntervalId	Ptimeout;

typedef	struct	{
	Boolean	toolbar_pres;
	Boolean jobtit_pres;
	Boolean footer_pres;
	Boolean	xterm_edit;
	String	editor_name;
	int	rtime, rint;
}  vrec_t;

static void  cb_about();
static void  cb_quit(Widget, int);
static void  cb_saveopts(Widget);
extern void  initmoremsgs();

static	XtResource	resources[] = {
	{ "toolbarPresent", "ToolbarPresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, toolbar_pres), XtRImmediate, False },
	{ "jtitlePresent", "JtitlePresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, jobtit_pres), XtRImmediate, False },
	{ "footerPresent", "FooterPresent", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, footer_pres), XtRImmediate, False },
	{ "xtermEdit", "XtermEdit", XtRBoolean, sizeof(Boolean),
		  XtOffsetOf(vrec_t, xterm_edit), XtRImmediate, (XtPointer) True },
	{ "editorName", "EditorName", XtRString, sizeof(String),
		  XtOffsetOf(vrec_t, editor_name), XtRString, "vi" },
	{ "repeatTime", "RepeatTime", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, rtime), XtRImmediate, (XtPointer) 500 },
	{ "repeatInt", "RepeatInt", XtRInt, sizeof(int),
		  XtOffsetOf(vrec_t, rint), XtRImmediate, (XtPointer) 100 }};

static	casc_button
opt_casc[] = {
	{	ITEM,	"Viewopts",	cb_viewopt,	0	},
	{	ITEM,	"Saveopts",	cb_saveopts,	0	},
	{	SEP	},
	{	ITEM,	"Selectdir",	cb_direct,	0	},
	{	SEP	},
	{	ITEM,	"Loaddefsc",	cb_loaddefs,	0	},
	{	ITEM,	"Savedefsc",	cb_savedefs,	0	},
	{	SEP	},
	{	ITEM,	"Loaddefsh",	cb_loaddefs,	1	},
	{	ITEM,	"Savedefsh",	cb_savedefs,	1	},
	{	DSEP	},
	{	ITEM,	"Quit",	cb_quit,	0	}},
defs_casc[] = {
#ifdef	NETWORK_VERSION
	{	ITEM,	"Defhost",	cb_defhost,	0	},
#endif
	{	ITEM,	"Queued",	cb_jqueue,	0	},
	{	ITEM,	"Setrund",	cb_djstate,	BJP_NONE	},
	{	ITEM,	"Setcancd",	cb_djstate,	BJP_CANCELLED	},
	{	ITEM,	"Timed",	cb_stime,	0	},
	{	ITEM,	"Titled",	cb_titprill,	0	},
	{	ITEM,	"Processd",	cb_procpar,	0	},
	{	ITEM,	"Runtimed",	cb_runtime,	0	},
	{	ITEM,	"Maild",	cb_jmail,	0	},
	{	ITEM,	"Permjd",	cb_jperm,	0	},
	{	SEP	},
	{	ITEM,	"Argsd",	cb_jargs,	0	},
	{	ITEM,	"Envd",		cb_jenv,	0	},
	{	ITEM,	"Redirsd",	cb_jredirs,	0	},
	{	SEP	},
	{	ITEM,	"Condsd",	cb_jconds,	0	},
	{	ITEM,	"Assesd",	cb_jass,	0	}},
file_casc[] = {
	{	ITEM,	"New",		cb_jnew,	0	},
	{	ITEM,	"Open",		cb_jopen,	0	},
	{	ITEM,	"Close",	cb_jclosedel,	0	},
	{	ITEM,	"Jobfile",	cb_jcmdfile,	JCMDFILE_JOB	},
	{	ITEM,	"Cmdfile",	cb_jcmdfile,	JCMDFILE_CMD	},
	{	ITEM,	"Save",		cb_jsave,	0	},
	{	ITEM,	"Edit",		cb_edit,	0	},
	{	SEP	},
	{	ITEM,	"Delete",	cb_jclosedel,	1	},
	{	SEP	},
	{	ITEM,	"Submit",	cb_submit,	0	}
#ifdef	NETWORK_VERSION
	,{	ITEM,	"Rsubmit",	cb_remsubmit,	0	}
#endif
},
job_casc[] = {
	{	ITEM,	"Queue",	cb_jqueue,	1	},
	{	ITEM,	"Setrun",	cb_jstate,	BJP_NONE	},
	{	ITEM,	"Setcanc",	cb_jstate,	BJP_CANCELLED	},
	{	ITEM,	"Time",		cb_stime,	1	},
	{	ITEM,	"Title",	cb_titprill,	1	},
	{	ITEM,	"Process",	cb_procpar,	1	},
	{	ITEM,	"Runtime",	cb_runtime,	1	},
	{	ITEM,	"Mail",		cb_jmail,	1	},
	{	ITEM,	"Permj",	cb_jperm,	1	},
	{	SEP	},
	{	ITEM,	"Args",		cb_jargs,	1	},
	{	ITEM,	"Env",		cb_jenv,	1	},
	{	ITEM,	"Redirs",	cb_jredirs,	1	},
	{	SEP	},
	{	ITEM,	"Conds",	cb_jconds,	1	},
	{	ITEM,	"Asses",	cb_jass,	1	}},
help_casc[] = {
	{	ITEM,	"Help",		dohelp,		$H{xmbtr main help}	},
	{	ITEM,	"Helpon",	cb_chelp,	0	},
	{	SEP	},
	{	ITEM,	"About",	cb_about,	0	}};

static	pull_button
	opt_button = {
		"Options",	XtNumber(opt_casc),	$H{xmbtr options menu help},	opt_casc	},
	defs_button = {
		"Defaults",	XtNumber(defs_casc),	$H{xmbtr defaults menu help},	defs_casc	},
	file_button = {
		"File",		XtNumber(file_casc),	$H{xmbtr files menu help},	file_casc	},
	job_button = {
		"Job",		XtNumber(job_casc),	$H{xmbtr jobs menu help},	job_casc	},
	help_button = {
		"Help",		XtNumber(help_casc),	$H{xmbtr help menu help},	help_casc,	1	};

static	pull_button	*menlist[] = {
	&opt_button,	&defs_button,	&file_button,	&job_button,
	&help_button
};

typedef	struct	{
	char	*name;
	void	(*callback)();
	int	callback_data;
	int	helpnum;
}  tool_button;

static	tool_button
	toollist1[] = {
	{	"Open",		cb_jopen,	0,		$H{xmbtr open job button help}	},
	{	"Edit",		cb_edit,	0,		$H{xmbtr edit job button help}	},
	{	"Srun",		cb_jstate,	BJP_NONE,	$H{xmbtr set runnable button help}	},
	{	"Scanc",	cb_jstate,	BJP_CANCELLED,	$H{xmbtr set canc button help}	},
	{	"Time",		cb_stime,	1,		$H{xmbtr set time button help}	},
	{	"Cond",		cb_jconds,	1,		$H{xmbtr set conds button help}	},
	{	"Ass",		cb_jass,	1,		$H{xmbtr set asses button help}	},
	{	"Submit",	cb_submit,	0,		$H{xmbtr submit button help}	}
#ifdef	NETWORK_VERSION
	,{	"Rsubmit",	cb_remsubmit,	0,		$H{xmbtr remote submit button help}	}
#endif
	};

#if	defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)

/* Define our own bcopy and bzero because X uses these in places and
   we don't want to include some -libucb which pulls in funny
   sprintfs etc */

void  bcopy(void *from, void *to, unsigned count)
{
	memcpy(to, from, count);
}
void  bzero(void *to, unsigned count)
{
	memset(to, '\0', count);
}
#endif

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
	fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
	exit(E_NOMEM);
}

/* Don't put exit as a callback or we'll get some weird exit code based on a Widget pointer.  */

static void  cb_quit(Widget w, int n)
{
	exit(n);
}

static void  dumpbool(FILE *tf, char *name, const int value)
{
	fprintf(tf, "%s.%s:\t%s\n", progname, name, value? "True": "False");
}

static void  cb_saveopts(Widget w)
{
	char	*srfile, *hf;
	FILE	*inf, *tf;
	int	ch;
	unsigned	oldumask;
	Dimension	wid;
	SHORT	items;
	time_t	now;
	struct	tm	*tp;

	if  (!Confirm(w, $PH{Confirm write options}))
		return;

	hf = envprocess("$HOME/XI");
	if  (!(inf = fopen(hf, "r")))  {
		srfile = XtResolvePathname(dpy, "app-defaults", NULL, NULL, NULL, NULL, 0, NULL);
		if  (!(inf = fopen(srfile, "r")))  {
			doerror(w, $EH{xmbtq cannot open appdefaults});
			XtFree(srfile);
			return;
		}
		XtFree(srfile);
	}
	tf = tmpfile();
	while  ((ch = getc(inf)) != EOF)
		putc(ch, tf);
	fclose(inf);
	time(&now);
	tp = localtime(&now);
	fprintf(tf, "\n!! %s User-defined options %.2d:%.2d:%.2d %.2d/%.2d/%.2d\n\n",
		progname,
		tp->tm_hour, tp->tm_min, tp->tm_sec,
		tp->tm_year % 100, tp->tm_mon+1, tp->tm_mday);
	dumpbool(tf, "xtermEdit", xterm_edit);
	fprintf(tf, "%s.editorName: %s\n", progname, editor_name);
	XtVaGetValues(jwid, XmNwidth, &wid, XmNvisibleItemCount, &items, NULL);
	fprintf(tf, "%s*jlist.width: %ld\n", progname, (long) wid);
	fprintf(tf, "%s*jlist.visibleItemCount: %ld\n", progname, (long) items);
	SWAP_TO(Realuid);
	oldumask = umask((int) Save_umask);
	if  (!(inf = fopen(hf, "w")))  {
		SWAP_TO(Daemuid);
		doerror(w, $EH{xmbtq cannot create app resource});
		return;
	}
	umask((int) oldumask);
	SWAP_TO(Daemuid);
	free(hf);
	rewind(tf);
	while  ((ch = getc(tf)) != EOF)
		putc(ch, inf);
	fclose(tf);
	fclose(inf);
}

static void  cb_about()
{
	Widget		dlg;
	char	buf[sizeof(rcsid1) + sizeof(rcsid2) + 2];
	sprintf(buf, "%s\n%s", rcsid1, rcsid2);
	dlg = XmCreateInformationDialog(jwid, "about", NULL, 0);
	XtVaSetValues(dlg,
		      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		      XtVaTypedArg, XmNmessageString, XmRString, buf, strlen(buf),
		      NULL);
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
	XtManageChild(dlg);
	XtPopup(XtParent(dlg), XtGrabNone);
}

/* Signals are errors Suppress final message....  */

static RETSIGTYPE  catchit(int n)
{
	Ctrl_chan = -1;
	exit(E_SIGNAL);
}

Widget  BuildPulldown(Widget menub, pull_button *item)
{
	int	cnt;
	Widget	pulldown, cascade, button;

	pulldown = XmCreatePulldownMenu(menub, "pulldown", NULL, 0);
	cascade = XtVaCreateManagedWidget(item->pull_name, xmCascadeButtonWidgetClass, menub,
					  XmNsubMenuId, pulldown, NULL);

	if  (item->helpnum != 0)
		XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(item->helpnum));

	for  (cnt = 0;  cnt < item->nitems;  cnt++)  {
		char	sname[20];
		casc_button	*cb = &item->items[cnt];
		switch  (cb->type)  {
		case  SEP:
			sprintf(sname, "separator%d", cnt);
			button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown, NULL);
			continue;
		case  DSEP:
			sprintf(sname, "separator%d", cnt);
			button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown,
							 XmNseparatorType, XmDOUBLE_LINE, NULL);
			continue;
		case  ITEM:
			button = XtVaCreateManagedWidget(cb->name, xmPushButtonGadgetClass, pulldown, NULL);
			if  (cb->callback)
				XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) cb->callback, INT_TO_XTPOINTER(cb->callback_data));
			continue;
		}
	}
	if  (item->ishelp)
		return  cascade;
	return  NULL;
}

static void  setup_menus()
{
	int			cnt;
	XtWidgetGeometry	size;
	Widget			helpw;

	menubar = XmCreateMenuBar(panedw, "menubar", NULL, 0);

	/* Get rid of resize button for menubar */

	size.request_mode = CWHeight;
	XtQueryGeometry(menubar, NULL, &size);
	XtVaSetValues(menubar, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

	for  (cnt = 0;  cnt < XtNumber(menlist);  cnt++)
		if  ((helpw = BuildPulldown(menubar, menlist[cnt])))
			XtVaSetValues(menubar, XmNmenuHelpWidget, helpw, NULL);

	XtManageChild(menubar);
}

static void  setup_toolbar()
{
	int			cnt;
	XtWidgetGeometry	size;

	toolbar = XtVaCreateManagedWidget("toolbar", xmRowColumnWidgetClass, panedw,
					  XmNorientation,	XmHORIZONTAL,
					  XmNpacking,		XmPACK_COLUMN,
					  XmNnumColumns,	1,	/* Meaning rows */
					  NULL);

	/* Fix size to avoid having a resize - the heights are a bit arbitrary.  */

	size.request_mode = CWHeight;
	XtQueryGeometry(toolbar, NULL, &size);
	XtVaSetValues(toolbar, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

	/* Set up buttons */

	for  (cnt = 0;  cnt < XtNumber(toollist1);  cnt++)  {
		Widget  w = XtVaCreateManagedWidget(toollist1[cnt].name, xmPushButtonWidgetClass, toolbar, NULL);
		if  (toollist1[cnt].callback)
			XtAddCallback(w,
				      XmNactivateCallback,
				      (XtCallbackProc) toollist1[cnt].callback,
				      INT_TO_XTPOINTER(toollist1[cnt].callback_data));
		if  (toollist1[cnt].helpnum != 0)
			XtAddCallback(w, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(toollist1[cnt].helpnum));
	}
}

static Widget  maketitle(char *tname)
{
	Widget			labv;
	XtWidgetGeometry	size;

	labv = XtVaCreateManagedWidget(tname, xmLabelWidgetClass, panedw, NULL);
	size.request_mode = CWHeight;
	XtQueryGeometry(labv, NULL, &size);
	XtVaSetValues(labv, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
	return  labv;
}

#include "xmbtr.bm"

static void  wstart(int argc, char **argv)
{
	vrec_t	vrec;
	Pixmap	bitmap;

	toplevel = XtVaAppInitialize(&app, "GBATCH", NULL, 0, &argc, argv, NULL, NULL);
	XtGetApplicationResources(toplevel, &vrec, resources, XtNumber(resources), NULL, 0);
	bitmap = XCreatePixmapFromBitmapData(dpy = XtDisplay(toplevel),
					     RootWindowOfScreen(XtScreen(toplevel)),
					     xmbtr_bits, xmbtr_width, xmbtr_height, 1, 0, 1);
	XtVaSetValues(toplevel, XmNiconPixmap, bitmap, NULL);

	/* Set up parameters from resources */

	if  (vrec.editor_name[0])  {
		editor_name = stracpy(vrec.editor_name);
		xterm_edit = vrec.xterm_edit? 1: 0;
	}
	else  {
		editor_name = stracpy("vi");
		xterm_edit = 1;
	}
	arr_rtime = vrec.rtime;
	arr_rint = vrec.rint;

	/* Now to create all the bits of the application */

	panedw = XtVaCreateWidget("layout", xmPanedWindowWidgetClass, toplevel, NULL);

	setup_menus();

	if  (vrec.toolbar_pres)
		setup_toolbar();

	if  (vrec.jobtit_pres)
		jtitwid = maketitle("jtitle");

	jwid = XmCreateScrolledList(panedw, "jlist", NULL, 0);
	XtAddCallback(jwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtr jlist help});
	XtManageChild(jwid);

	if  (vrec.footer_pres)
		maketitle("footer");

	XtManageChild(panedw);
	XtRealizeWidget(toplevel);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
#endif

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	init_xenv();
	Realuid = getuid();
	Realgid = getgid();
	Effuid = geteuid();
	Effgid = getegid();
	INIT_DAEMUID
	Cfile = open_cfile(Confvarname, "xmbtr.help");
	SCRAMBLID_CHECK
	tzset();

	/* If we haven't got a directory, use the current */

	if  (!Curr_pwd)  {
		if  ((Curr_pwd = getenv("PWD")))
			Curr_pwd = stracpy(Curr_pwd);
		else
			Curr_pwd = runpwd();
	}

	spdir = envprocess(SPDIR);
	initmoremsgs();

	/* Initialise default job.  We are generally happy about things being zero.  */

	SWAP_TO(Daemuid);
	if  (chdir(spdir) < 0)  {
		disp_str = spdir;
		print_error($E{Cannot change directory});
		exit(E_NOCHDIR);
	}
	initcifile();
	mypriv = getbtuser(Realuid);
	init_defaults();
	SWAP_TO(Realuid);
	load_options();
	SWAP_TO(Daemuid);

	if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
		print_error($E{Scheduler not running});
		exit(E_NOTRUN);
	}
	mymtype = MTOFFSET + getpid();

#ifndef	USING_FLOCK
	/* Set up semaphores */

	if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif

	openjfile(1, 0);
	openvfile(1, 0);
	initxbuffer(1);
	wstart(argc, argv);
	if  (default_job.h.bj_times.tc_istime  &&  default_job.h.bj_times.tc_nexttime < time((time_t *) 0))
		doinfo(jwid, $E{xmbtr default not future});
#ifdef	STRUCT_SIG
	z.sighandler_el = catchit;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGINT, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGQUIT, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGHUP, &z, (struct sigstruct_name *) 0);
	sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#else
	signal(SIGINT, catchit);
	signal(SIGQUIT, catchit);
	signal(SIGHUP, catchit);
	signal(SIGTERM, catchit);
#endif
	XtAppMainLoop(app);
	return  0;		/* Shut up compilers */
}
