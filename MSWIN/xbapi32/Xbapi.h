/* Xbapi.h -- API library

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

#ifndef	_XBAPI_H
#define	_XBAPI_H

/****************************************************************************
 *	Copied from config.h, defaults.h, bjparam.h, btcon.h, btmode.h,
 *	timecon.h
 ****************************************************************************
 */

/* Set to number of bytes in environment */
#define ENVSIZE 2500

/* Define to set initial job allocation */
#define INITJALLOC 5000

/* Define to compile for network version */
#define NETWORK_VERSION 1

/* Define type for process ids */
#define PIDTYPE pid_t

/* Define type for final arg of accept etc thanq HP */
#define SOCKLEN_T int

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a unsigned.  */
#define SIZEOF_UNSIGNED 4

/* The number of bytes in a unsigned long.  */
#define SIZEOF_UNSIGNED_LONG 4

/* The number of bytes in a unsigned short.  */
#define SIZEOF_UNSIGNED_SHORT 2

#define	LONG	long
#define	ULONG	unsigned long
#define	SHORT	short
#define	USHORT	unsigned short

#define	ROOTID	0

#ifndef	_NFILE
#define	_NFILE	64
#endif

#define	UIDSIZE		11		/* Size of UID field */
#define	HOSTNSIZE	14		/* Host name size */
#define	LICWARN		14		/* Days to warn about licence */

#define	CMODE	0600

typedef	LONG	jobno_t;
typedef LONG    netid_t;
typedef LONG    slotno_t;               /* May be -ve   */
typedef	LONG	vhash_t;

#define	JN_INC	80000			/*  Add this to job no if clashes */

typedef	LONG	int_ugid_t;
typedef	LONG	int_pid_t;

/*	Initial amounts of shared memory to allocate for jobs/vars */

#define	INITNJOBS	INITJALLOC
#define	INITNVARS	50
#define	XBUFJOBS	5			/* Number of slots in transport buff */

/* Amounts to grow by  */

#define	INCNJOBS	(INITJALLOC/2)
#define	INCNVARS	20

#define	NETTICKLE	1000			/* Keep networks alive */

/* Timezone - change this as needed (mostly needed for DOS version). */

#define DEFAULT_TZ	"TZ=GMT0BST"

/* Default maximum load level  */

#define	SYSDF_MAXLL	20000

/* Initial values for start limit and wait */

#define	INIT_STARTLIM	5
#define	INIT_STARTWAIT	30

#define	DEF_LUMPSIZE	20		/* Number of jobs/vars to send at once on startup */
#define	DEF_LUMPWAIT	2		/* Time to wait in each case */

/* Space in a job structure to allow for strings and vectors
   We can be a little bit more profligate than we used to be in
   the days when we passed it in the job. This fell foul of the
   small limit on IPC message buffer size.
   We now pass a pointer in a shared memory segment */

#define	JOBSPACE	(ENVSIZE+500)

#define	CI_MAXNAME	15		/* Max name size cmd interp */
#define	CI_MAXFPATH	75		/* Max path cmd interp */
#define	CI_MAXARGS	27		/* Max args space cmd interp */

#define	MAXCVARS	10		/* Max number of conditions in job */
#define	MAXSEVARS	8		/* Max number of assignments */

/*	These are "as big as they can ever be" */

#define	MAXJREDIRS	JOBSPACE/sizeof(Redir)
#define	MAXJENVIR	JOBSPACE/sizeof(Envir)
#define	MAXJARGS	JOBSPACE/sizeof(Jarg)

/* Condition types. */

#define	C_UNUSED	0
#define	C_EQ		1
#define	C_NE		2
#define	C_LT		3
#define	C_LE		4
#define	C_GT		5
#define	C_GE		6
#define	NUM_CONDTYPES	6

/* ondition critical. */

#define	CCRIT_NORUN	0x01		/*  Do not run if remote unavailable */
#define	CCRIT_NONAVAIL	0x40		/*  Local var not avail */
#define	CCRIT_NOPERM	0x80		/*  Local var not avail */

/* Assignment flags */

#define	BJA_START	0x01
#define	BJA_OK		0x02
#define	BJA_ERROR	0x04
#define	BJA_ABORT	0x08
#define	BJA_CANCEL	0x10
#define	BJA_REVERSE	0x1000		/*  Do the opposite at EOJ */

/* Assignment types */

#define	BJA_NONE	0		/*  Does not apply */
#define	BJA_ASSIGN	1		/*  Assign (reverse set zero) */
#define	BJA_INCR	2		/*  Increment by.... */
#define	BJA_DECR	3		/*  Decrement by.... */
#define	BJA_MULT	4		/*  Multiply by.... */
#define	BJA_DIV		5		/*  Divide by.... */
#define	BJA_MOD		6		/*  Remainder by.... */
#define	BJA_SEXIT	7		/*  Set exit code */
#define	BJA_SSIG	8		/*  Set signal number */
#define	NUM_ASSTYPES	8		/*  Number of different types */

/* Assignment critical */

#define	ACRIT_NORUN	0x01		/*  Do not run if remote unavailable */
#define	ACRIT_NONAVAIL	0x40		/*  Unavailable local variable */
#define	ACRIT_NOPERM	0x80		/*  No permissions local variable */

/* Job progress codes */

#define	BJP_NONE	0		/*  Nothing done yet */
#define	BJP_DONE	1		/*  Done once ok */
#define	BJP_ERROR	2		/*  Done but gave error */
#define	BJP_ABORTED	3		/*  Done but aborted by oper */
#define	BJP_CANCELLED	4		/*  Cancelled before it could run */
#define	BJP_STARTUP1	5		/*  Currently starting - phase 1 */
#define	BJP_STARTUP2	6		/*  Currently starting - phase 2 */
#define	BJP_RUNNING	7		/*  Currently running */
#define	BJP_FINISHED	8		/*  Finished ok */

/* Flags for bj_jflags */

#define	BJ_WRT		(1 << 0)	/*  Send message to users terminal  */
#define	BJ_MAIL		(1 << 1)	/*  Mail message to user  */
#define	BJ_NOADVIFERR	(1 << 3)	/*  No advance time if error*/
#define	BJ_EXPORT	(1 << 4)	/*  Job is visible from outside world */
#define	BJ_REMRUNNABLE	(1 << 5)	/*  Job is runnable from outside world */
#define	BJ_CLIENTJOB	(1 << 6) 	/*  From client host */
#define	BJ_ROAMUSER	(1 << 7) 	/*  Roaming user */

/* Flags for bj_jrunflags */

#define	BJ_PROPOSED	(1 << 0)	/*  Remote job proposed - inhibits further propose */
#define	BJ_SKELHOLD	(1 << 1)	/*  Job depends on unaccessible remote var */
#define	BJ_AUTOKILLED	(1 << 2)	/*  Initial kill applied */
#define	BJ_AUTOMURDER	(1 << 3)	/*  Final kill applied */
#define	BJ_HOSTDIED	(1 << 4) 	/*  Murdered because host died */
#define	BJ_FORCE	(1 << 5)	/*  Force job to run NB moved from bj_jflags */
#define	BJ_FORCENA	(1 << 6)	/*  Do not advance time on Force job to run */

/* Exit code range structure */

typedef	struct	{		/* Shuffled round to guarantee 4 byte size */
	unsigned  char	nlower;	/* Normal exit lower range */
	unsigned  char	nupper;	/* Normal exit upper range */
	unsigned  char	elower;	/* Error exit lower range */
	unsigned  char	eupper;	/* Error exit upper range */
}  Exits;

#define	BTC_VALUE	49		/* Maximum string length */

typedef	struct	{
	SHORT	const_type;
#define	CON_NONE	0
#define	CON_LONG	1
#define	CON_STRING	2
	union	{
		char	con_string[BTC_VALUE+1];
		LONG	con_long;
	}  con_un;
}  Btcon, *BtconRef;

typedef	const	Btcon	*CBtconRef;

typedef	struct	{
	int_ugid_t	o_uid, o_gid,		/* Numeric ones only valid on current machine */
			c_uid, c_gid;
	char		o_user[UIDSIZE+1],	/* Owner */
			o_group[UIDSIZE+1],
			c_user[UIDSIZE+1],	/* Creator */
			c_group[UIDSIZE+1];
	USHORT		u_flags,		/* Permissions - user */
			g_flags,		/* Permissions - group */
			o_flags;		/* Permissions - other */
	USHORT		padding;		/* Pad to long align */
}  Btmode, *BtmodeRef;

typedef	const	Btmode	*CBtmodeRef;

/* Flags */

#define	BTM_READ	(1 << 0)		/* Read thing */
#define	BTM_WRITE	(1 << 1)		/* Write thing */
#define	BTM_SHOW	(1 << 2)		/* Show it exists */
#define	BTM_RDMODE	(1 << 3)		/* Read mode */
#define	BTM_WRMODE	(1 << 4)		/* Write mode */
#define	BTM_UTAKE	(1 << 5)		/* Assume user */
#define	BTM_GTAKE	(1 << 6)		/* Assume group */
#define	BTM_UGIVE	(1 << 7)		/* Give away user */
#define	BTM_GGIVE	(1 << 8)		/* Give away group */
#define	BTM_DELETE	(1 << 9)		/* Delete it */
#define	BTM_KILL	(1 << 10)		/* Kill it (jobs only) */

#define	VALLMODES	0x03FF			/* Modes for vars */
#define	JALLMODES	0x07FF			/* Modes for jobs */

/* Privileges */

#define	BTM_ORP_UG	(1 << 8)		/* Or user and group permissions */
#define	BTM_ORP_UO	(1 << 7)		/* Or user and other permissions */
#define	BTM_ORP_GO	(1 << 6)		/* Or group and other permissions */
#define	BTM_SSTOP	(1 << 5)		/* Stop the thing */
#define	BTM_UMASK	(1 << 4)		/* Change privs */
#define	BTM_SPCREATE	(1 << 3)		/* Special create */
#define	BTM_CREATE	(1 << 2)		/* Create anything */
#define	BTM_RADMIN	(1 << 1)		/* Read admin priv */
#define	BTM_WADMIN	(1 << 0)		/* Write admin priv */

#define	NUM_PRIVBITS	9
#define	ALLPRIVS	((1 << NUM_PRIVBITS) -1)/* Privs */

/* Options for argument interpretation. */

#define	MODE_NONE	0
#define	MODE_SET	1
#define	MODE_ON		2
#define	MODE_OFF	3

typedef	struct	{
	time_t		tc_nexttime;		/* Next time */
	unsigned  char	tc_istime;	/* Time applies */
	unsigned  char  tc_mday;	/* Day of month (for monthb/e) */
	USHORT		tc_nvaldays;	/* Non-valid days bits (1 << day) */
	unsigned  char	tc_repeat;
#define	TC_DELETE	0		/* Run once and then delete */
#define	TC_RETAIN	1		/* Run once and leave on Q */
#define	TC_MINUTES	2		/* Repeat rate - various ints */
#define	TC_HOURS	3
#define	TC_DAYS		4
#define	TC_WEEKS	5
#define	TC_MONTHSB	6		/* Months relative to beginning */
#define	TC_MONTHSE	7		/* Months relative to end */
#define	TC_YEARS	8
	unsigned  char	tc_nposs;	/* If not possible... */
#define	TC_SKIP		0		/* Skip it, reschedule next etc */
#define	TC_WAIT1	1		/* Wait but do not postpone others */
#define	TC_WAITALL	2		/* Wait and postpone others */
#define	TC_CATCHUP	3		/* Run one and catch up */
	ULONG	tc_rate;		/* Repeat interval */
}  Timecon, *TimeconRef;

typedef	const	Timecon	*CTimeconRef;

/* Non-valid day bits are 1 << day: Sun=0 Sat=6 as per localtime */

#define	TC_NDAYS	8		/* Types of day */
#define	TC_HOLIDAYBIT	(1 << 7)	/* Bit to mark holiday */
#define	TC_ALLWEEKDAYS	((1 << 7) - 1)	/* All weekdays */

/* Size of bitmap vector for holidays */

#define	YVECSIZE	((366 + 7) / 8)


/* Error codes */

#define	XBAPI_OK			(0)
#define	XBAPI_INVALID_FD		(-1)
#define	XBAPI_NOMEM		(-2)
#define	XBAPI_INVALID_HOSTNAME	(-3)
#define	XBAPI_INVALID_SERVICE	(-4)
#define	XBAPI_NODEFAULT_SERVICE	(-5)
#define	XBAPI_NOSOCKET		(-6)
#define	XBAPI_NOBIND		(-7)
#define	XBAPI_NOCONNECT		(-8)
#define	XBAPI_BADREAD		(-9)
#define	XBAPI_BADWRITE		(-10)
#define	XBAPI_CHILDPROC		(-11)

/* These errors should correspond to xbnetq.h sort of  */
#define	XBAPI_CONVERT_XBNR(code)	(-20-(code))
#define	XBAPI_NOT_USER		(-23)
#define	XBAPI_BAD_CI		(-24)
#define	XBAPI_BAD_CVAR		(-25)
#define	XBAPI_BAD_AVAR		(-26)
#define	XBAPI_NOMEM_QF		(-28)
#define	XBAPI_NOCRPERM		(-29)
#define	XBAPI_BAD_PRIORITY		(-30)
#define	XBAPI_BAD_LL		(-31)
#define	XBAPI_BAD_USER		(-32)
#define	XBAPI_FILE_FULL		(-33)
#define	XBAPI_QFULL		(-34)
#define	XBAPI_BAD_JOBDATA		(-35)
#define	XBAPI_UNKNOWN_USER		(-36)
#define	XBAPI_UNKNOWN_GROUP	(-37)
#define	XBAPI_ERR			(-38)
#define	XBAPI_NORADMIN		(-39)
#define	XBAPI_NOCMODE		(-40)

#define	XBAPI_UNKNOWN_COMMAND	(-41)
#define	XBAPI_SEQUENCE		(-42)
#define	XBAPI_UNKNOWN_JOB		(-43)
#define	XBAPI_UNKNOWN_VAR		(-44)
#define	XBAPI_NOPERM		(-45)
#define	XBAPI_INVALID_YEAR		(-46)
#define	XBAPI_ISRUNNING		(-47)
#define	XBAPI_NOTIMETOA		(-48)
#define	XBAPI_VAR_NULL		(-49)
#define	XBAPI_VAR_CDEV		(-50)
#define	XBAPI_INVALIDSLOT		(-51)
#define	XBAPI_ISNOTRUNNING		(-52)
#define	XBAPI_NOMEMQ		(-53)
#define	XBAPI_NOPERM_VAR		(-54)
#define	XBAPI_RVAR_LJOB		(-55)
#define	XBAPI_LVAR_RJOB		(-56)
#define	XBAPI_MINPRIV		(-57)
#define	XBAPI_SYSVAR		(-58)
#define	XBAPI_SYSVTYPE		(-59)
#define	XBAPI_VEXISTS		(-60)
#define	XBAPI_DSYSVAR		(-61)
#define	XBAPI_DINUSE		(-62)
#define	XBAPI_DELREMOTE		(-63)
#define	XBAPI_NO_PASSWD		(-64)
#define	XBAPI_PASSWD_INVALID	(-65)
#define	XBAPI_BAD_GROUP		(-66)
#define	XBAPI_NOTEXPORT		(-67)
#define	XBAPI_RENAMECLUST	(-68)

/* Flags for accessing things */

#define	XBAPI_FLAG_LOCALONLY	(1 << 0)
#define	XBAPI_FLAG_USERONLY	(1 << 1)
#define	XBAPI_FLAG_GROUPONLY	(1 << 2)
#define	XBAPI_FLAG_QUEUEONLY	(1 << 3)
#define	XBAPI_FLAG_IGNORESEQ	(1 << 4)
#define	XBAPI_FLAG_FORCE		(1 << 5)

/* Miscellaneous codes to do job ops with */

#define	XBAPI_JOP_SETRUN	1
#define	XBAPI_JOP_SETCANC	2
#define	XBAPI_JOP_FORCE		3
#define	XBAPI_JOP_FORCEADV	4
#define	XBAPI_JOP_ADVTIME	5
#define	XBAPI_JOP_KILL		6
#define	XBAPI_JOB_SETDONE	7

#define	XBWINAPI_JOBPROD	1
#define	XBWINAPI_VARPROD	2

typedef	struct	{		/* Format of environment descriptor */
	unsigned  short	e_name;			/* Offset of name */
	unsigned  short	e_value;		/* Offset of value */
}  Envir;

typedef	struct	{		/* Redirection in job buffer */
	unsigned  char	fd;		/* Stream or -ve no more */
	unsigned  char	action;		/* Action */
#define	RD_ACT_RD	1		/* Open for reading */
#define	RD_ACT_WRT	2		/* Open/create for writing */
#define	RD_ACT_APPEND	3		/* Open/create and append write only */
#define	RD_ACT_RDWR	4		/* Open/create for read/write */
#define	RD_ACT_RDWRAPP	5		/* Open/create/read/write/append */
#define	RD_ACT_PIPEO	6		/* Pipe out */
#define	RD_ACT_PIPEI	7		/* Pipe in */
#define	RD_ACT_CLOSE	8		/* Close it (no file) */
#define	RD_ACT_DUP	9		/* Duplicate file descriptor given */

	unsigned  short	arg;		/* Offset or file to dup from */
}  Redir;

typedef	struct	{		/* Redirection passed to/from user */
	unsigned  char	fd;
	unsigned  char	action;
	union	{
		unsigned  short	arg;		/* File to dup from */
		const	char	*buffer;
	}  un;
}  apiMredir;

typedef	struct	{		/* Variable identification in conds/asses */
	slotno_t	slotno;			/* Slot number on our host */
}  apiVid;

typedef	struct	{		/* Condition structure */
	unsigned char	bjc_compar;	/*  Comparison as: */
	unsigned char	bjc_iscrit;	/*  Critical */
	apiVid		bjc_var;	/*  Which variable */
	Btcon	bjc_value;		/*  Value to compare */
}  apiJcond;

typedef	struct	{
	unsigned  short	 bja_flags;	/*  When it applies */
	unsigned  char	bja_op;		/*  What to do */
	unsigned  char	bja_iscrit;	/*  Critical */
	apiVid		bja_var;	/*  Which variable */
	Btcon	bja_con;
}  apiJass;

/* This is the job header, not including any strings */

typedef	struct	{
	jobno_t		bj_job;		/*  Job number  */
	jobno_t		bj_sonum;	/*  Standard output id */
	jobno_t		bj_senum;	/*  Standard error id */
	time_t		bj_time;	/*  When originally submitted  */
	time_t		bj_stime;	/*  Time started */
	time_t		bj_etime;	/*  Last end time */
	int_pid_t	bj_pid;		/*  Process id if running */
	netid_t		bj_orighostid;	/*  Original hostid (for remotely submitted jobs) */
	netid_t		bj_hostid;	/*  Host id job belongs to */
	netid_t		bj_runhostid;	/*  Host id job running on (might be different) */
	slotno_t	bj_slotno;	/*  Slot number */

	unsigned  char	bj_progress;	/*  As below */
	unsigned  char  bj_pri;		/*  Priority  */
	SHORT		bj_wpri;	/*  Working priority  */

	USHORT		bj_ll;		/*  Load level */
	USHORT		bj_umask;	/*  Copy of umask */

	USHORT		bj_nredirs,	/*  Number of redirections */
			bj_nargs,	/*  Number of arguments */
			bj_nenv;	/*  Environment */
	unsigned  char	bj_jflags;	/*  Flags */
	unsigned  char	bj_jrunflags;	/*  Run flags*/

	SHORT		bj_title;	/*  Name of job (offset) */
	SHORT		bj_direct;	/*  Directory (offset) */

	ULONG		bj_runtime;	/*  Run limit (secs) */
	USHORT		bj_autoksig;	/*  Auto kill sig before size 9s applied */
	USHORT		bj_runon;	/*  Grace period (secs) */
	USHORT		bj_deltime;	/*  Delete job automatically */
	SHORT		bj_padding;	/*  Up to long boundary */

	char		bj_cmdinterp[CI_MAXNAME+1];	/*  Command interpreter (now a string) */
	Btmode		bj_mode;		/*  Permissions */
	apiJcond	bj_conds[MAXCVARS];	/*  Conditions */
	apiJass		bj_asses[MAXSEVARS];	/*  Assignments */
	Timecon		bj_times;	/*  Time control */
	LONG		bj_ulimit;	/*  Copied ulimit */

	SHORT		bj_redirs;	/* Redirections (offset of vector) */
	SHORT		bj_env;		/* Environment (offset of vector) */
	SHORT		bj_arg;		/* Max args (offset of vector) */
	USHORT		bj_lastexit;	/* Last exit status (as returned by wait) */

	Exits		bj_exits;
}  apiBtjobh;

typedef	struct	{		/* This is the whole lot including the string space */
	apiBtjobh	h;
	char		bj_space[JOBSPACE];	/* Room for chars of char strings */
}  apiBtjob;

typedef	struct	{
	netid_t		hostid;	/* Originating host id - never zero */
	slotno_t	slotno;	/* SHM slot number on machine */
}  jident;

#define	BTV_NAME	19
#define	BTV_COMMENT	41

typedef	struct	{
	netid_t		hostid;			/* Originating host id - never zero */
	slotno_t	slotno;			/* SHM slot number on machine */
}  vident;

typedef	struct	{
	ULONG		var_sequence;		/* Change sequence */
	vident		var_id;
	time_t	var_c_time, var_m_time;		/* Create/mod time */
	unsigned char	var_type;		/* Is it special? */
	unsigned char	var_flags;		/* If so, set read-only */
	char	var_name[BTV_NAME+1];		/* Name */
	char	var_comment[BTV_COMMENT+1];	/* User-assigned comment */
	Btmode	var_mode;			/* Permissions */
	Btcon	var_value;			/* Value */
}  apiBtvar;

/* Values for var_type to indicate special (non-zero) */

#define VT_LOADLEVEL	1			/* Maximum Load Level */
#define VT_CURRLOAD	2			/* Current load level */
#define VT_LOGJOBS	3			/* Log jobs */
#define VT_LOGVARS	4			/* Log vars */
#define	VT_MACHNAME	5			/* Machine name */
#define	VT_STARTLIM	6			/* Max number of jobs to start at once */
#define	VT_STARTWAIT	7			/* Wait time */

/* Values for var_flags */

#define	VF_READONLY	0x01
#define	VF_STRINGONLY	0x02
#define	VF_LONGONLY	0x04
#define	VF_EXPORT	0x08			/* Visible to outside world */
#define	VF_SKELETON	0x10			/* Skeleton variable for offline host */
#define	VF_CLUSTER	0x20			/* Local to machine in conditions/assignments */

typedef	struct	{		/* Default strucure */
	unsigned  char	btd_version;	/* Major Xi-Batch version number */
	unsigned  char	btd_minp,	/* Minimum priority  */
			btd_maxp,	/* Maximum priority  */
			btd_defp;	/* Default priority  */

	time_t		btd_lastp;	/* Last read password file */

	ULONG		btd_priv;	/* Privileges */
	USHORT		btd_maxll;	/* Max load level */
	USHORT		btd_totll;	/* Max total load level */
	USHORT		btd_spec_ll;	/* Non-std jobs load level */
	USHORT		btd_jflags[3];	/* Flags for jobs */
	USHORT		btd_vflags[3];	/* Flags for variables */
}  apiBtdef;

typedef	struct	{
	unsigned  char	btu_isvalid,	/* Valid user id */
			btu_minp,	/* Minimum priority  */
			btu_maxp,	/* Maximum priority  */
			btu_defp;	/* Default priority  */
	int_ugid_t	btu_user;	/* User id */

	ULONG		btu_priv;	/* Privileges */

	USHORT		btu_maxll;	/* Max load level */
	USHORT		btu_totll;	/* Max total load level */
	USHORT		btu_spec_ll;	/* Non-std jobs load level */
	USHORT		btu_jflags[3];	/* Flags for jobs */
	USHORT		btu_vflags[3];	/* Flags for variables */
}  apiBtuser;

typedef	struct	{		/* Command interpreter */
	USHORT		ci_ll;		/*  Default load level */
	unsigned  char  ci_nice;	/*  Nice value */
	unsigned  char	ci_flags;	/*  Flags for cmd int */
	char	ci_name[CI_MAXNAME+1];	/*  Name (e.g. shell) */
	char	ci_path[CI_MAXFPATH+1];	/*  Path name */
	char	ci_args[CI_MAXARGS+1];	/*  Arg list */
}  Cmdint;

#define	CI_STDSHELL	0				/* Standard shell */
#define	CIF_SETARG0	(1 << 0)	/* Flag to set arg0 from job title */
#define	CIF_INTERPARGS	(1 << 1)	/* Flag to set to interpolate $1 etc args ourselves */

#ifdef	__cplusplus
extern	"C"	{
#endif
extern	int	xb_open(const char *, const char *, const char *);
extern	int	xb_login(const char *, const char *, const char *, char *, const int);
extern	int	xb_newgrp(const int, const char *);
extern	int	xb_close(const int);
extern	int	xb_setqueue(const int, const char *);
extern	int	xb_ciadd(const int, const unsigned, const Cmdint *, unsigned *);
extern	int	xb_cidel(const int, const unsigned, const unsigned);
extern	int	xb_ciread(const int, const unsigned, int *, Cmdint * *);
extern	int	xb_ciupd(const int, const unsigned, const unsigned, const Cmdint *);
extern	int	xb_holread(const int, const unsigned, int, unsigned char * *);
extern	int	xb_holupd(const int, const unsigned, int, unsigned char *);
extern	int	xb_getbtd(const int, apiBtdef *);
extern	int	xb_getbtu(const int, char *, char *, apiBtuser *);
extern	int	xb_jobres(const int, jobno_t *);
extern	int	xb_jobchgrp(const int, const unsigned, const slotno_t, const char *);
extern	int	xb_jobchmod(const int, const unsigned, const slotno_t, const Btmode *);
extern	int	xb_jobchown(const int, const unsigned, const slotno_t, const char *);
extern	int	xb_jobdel(const int, const unsigned, const slotno_t);
extern	int	xb_jobfind(const int, const unsigned, const jobno_t, const netid_t, slotno_t *, apiBtjob *);
extern	int	xb_jobfindslot(const int, const unsigned, const jobno_t, const netid_t, slotno_t *);
extern	int	xb_joblist(const int, const unsigned, int *, slotno_t * *);
extern	int	xb_jobop(const int, const unsigned, const slotno_t, const unsigned, const unsigned);
extern	int	xb_jobread(const int, const unsigned, const slotno_t, apiBtjob *);
extern	int	xb_jobupd(const int, const unsigned, const slotno_t, apiBtjob *);
extern	int	xb_putbtd(const int, const apiBtdef *);
extern	int	xb_putbtu(const int, const char *, const apiBtuser *);
extern	int	xb_unpackenv(const apiBtjob *, const unsigned, const char * *, const char * *);
extern	int	xb_puttitle(const int, apiBtjob *, const char *);
extern	int	xb_putdirect(apiBtjob *, const char *);
extern	int	xb_putarg(apiBtjob *, const unsigned, const char *);
extern	int	xb_delarg(apiBtjob *, const unsigned);
extern	int	xb_putarglist(apiBtjob *, const char * *);
extern	int	xb_putenv(apiBtjob *, const char *);
extern	int	xb_delenv(apiBtjob *, const char *);
extern	int	xb_putelist(apiBtjob *, const char * *);
extern	int	xb_putredir(apiBtjob *, const unsigned, const apiMredir *);
extern	int	xb_delredir(apiBtjob *, const unsigned);
extern	int	xb_putredirlist(apiBtjob *, const apiMredir *, const unsigned);
extern	int	xb_varadd(const int, const apiBtvar *);
extern	int	xb_varchcomm(const int, const unsigned, const slotno_t, const char *);
extern	int	xb_varchgrp(const int, const unsigned, const slotno_t, const char *);
extern	int	xb_varchmod(const int, const unsigned, const slotno_t, const Btmode *);
extern	int	xb_varchown(const int, const unsigned, const slotno_t, const char *);
extern	int	xb_vardel(const int, const unsigned, const slotno_t);
extern	int	xb_varfind(const int, const unsigned, const char *, const netid_t, slotno_t *, apiBtvar *);
extern	int	xb_varfindslot(const int, const unsigned, const char *, const netid_t, slotno_t *);
extern	int	xb_varlist(const int, const unsigned, int *, slotno_t * *);
extern	int	xb_varread(const int, const unsigned, const slotno_t, apiBtvar *);
extern	int	xb_varrename(const int, const unsigned, const slotno_t, const char *);
extern	int	xb_varupd(const int, const unsigned, const slotno_t, const apiBtvar *);
extern	int	xb_procmon(const int);
extern	int	xb_setmon(const int, HWND, UINT);
		
extern	void	xb_unsetmon(const int);

extern	const char *xb_gettitle(const int, const apiBtjob *);
extern	const char *xb_getdirect(const apiBtjob *);
extern	const char *xb_getarg(const apiBtjob *, const unsigned);
extern	const char *xb_getenv(const apiBtjob *, const char *);
extern	const char **xb_getenvlist(const apiBtjob *);

extern	const char **xb_gethenv(const int);

extern	const	apiMredir *xb_getredir(const apiBtjob *, const unsigned);

extern	int	xb_jobadd(const int, const int, int (*)(int,void*,unsigned), apiBtjob *);
extern	int	xb_jobdata(const int, const int, int (*)(int,void*,unsigned), const unsigned, const slotno_t);
#ifdef	__cplusplus
}
#endif

extern	int	xbapi_dataerror;

#endif	/*_XBAPI_H*/
