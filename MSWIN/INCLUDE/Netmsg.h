/* netmsg.h -- Network messages

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

#ifdef	__cplusplus
struct	feeder	{
	char	fdtype;		/* Type of file required */
#define	FEED_JOB	0	/* Feed job file */
#define	FEED_SO		1	/* Feed standard output file - delete when done */
#define	FEED_SE		2	/* Feed standard error file - delete when done */
	char	resvd[3];	/* Pad out to 4 bytes */
	jobno_t	 jobno;		/* Jobnumber net byte order */
	feeder(const char type, const jobno_t jn) : fdtype(type), jobno(jn) { }
};
#endif

/*
 *	General messages. Some of the hideous contortions we go
 *	to are to ensure that every long is on a 4-byte boundary
 *	etc and every structure is a multiple of 4 bytes long
 *	so we can send packets between machines without worrying about what
 *	the C compiler gets up to.
 */

typedef	struct	{
	unsigned  short	code;			/* Code number */
	unsigned  short	length;			/* Message length */
	netid_t		hostid;			/* Sender host id net byte order */
	int_pid_t	pid;			/* Sender process id - 0 if internal */
	char		muser[UIDSIZE+1], 	/* User */
			mgroup[UIDSIZE+1]; 	/* Group */
}  msghdr;

struct	netmsg	{
	msghdr		hdr;	// Header
	long		arg;	// Argument
#ifdef	__cplusplus
	netmsg()	{	}
	netmsg(const unsigned);
#endif
};

typedef	struct	{
	vident		bjnc_var; 	/*  Variable id */
	unsigned char	bjnc_compar;	/*  Comparison */
	unsigned char	bjnc_iscrit; 	/*  Critical */
	unsigned char	bjnc_type; 	/*  Type of constant */
	unsigned char	bjnc_padding[1];/*  Pad out to 4 bytes*/
	union	{
		char	bjnc_string[BTC_VALUE+3];
		long	bjnc_long;
	}  bjnc_un;
}  Jncond;

typedef	struct	{
	vident		bjna_var; 	/*  Variable id */
	unsigned  short	bjna_flags;	/*  When it applies */
	unsigned  char	bjna_op;	/*  What to do */
	unsigned  char	bjna_iscrit; 	/*  Critical */
	unsigned  char	bjna_type;	/*  Type of constant */
	unsigned  char	bjna_padding[3];/*  Pad  out to 4 bytes */
	union	{
		char	bjna_string[BTC_VALUE+3];
		long	bjna_long;
	}  bjna_un;
}  Jnass;

/*
 *	Messages regarding jobs....
 *	C++ derived classes wouldn't half come in handy here!
 */

struct	jobstatmsg  {		// State change
	msghdr		hdr;		// Header
	jident		jid;		// Job identifier
	unsigned  short	lastexit;	// Exit code
	unsigned  char	prog;	// Progress code
	unsigned  char	padding[1]; /* Reserved - make long */
	time_t		nexttime; 	// Next time
	long		lastpid;	// Process id where applicable
	netid_t		runhost;	// Host running
};

/*
 *	Message for changing uid or group.
 *	The code in hdr.code says which
 */

struct	jugmsg	{
	msghdr		hdr;
	jident		jid;
	char		newug[UIDSIZE+1];
};

/*
 *	Job messages for where strings haven't changed
 */

struct	jobhnetmsg  {
	msghdr		hdr;		/* Header */
	jident		jid;

	unsigned  char	nm_progress;
	unsigned  char  nm_pri;
	unsigned  char	nm_jflags;
	unsigned  char	nm_istime;
	unsigned  char  nm_mday;
	unsigned  char	nm_repeat;
	unsigned  char	nm_nposs;
	unsigned  char	nm_padding;

	unsigned short	nm_ll;
	unsigned short	nm_umask;
	unsigned short	nm_nvaldays;
	unsigned short	nm_autoksig;
	unsigned short	nm_runon;
	unsigned short	nm_deltime;
	unsigned short	nm_lastexit;
	unsigned short	nm_padding2;

	jobno_t		nm_job;
	long		nm_time;
	long		nm_stime;
	long		nm_etime;
	int_pid_t	nm_pid;
	netid_t		nm_orighostid;
	netid_t		nm_runhostid;
	long		nm_ulimit;
	long		nm_nexttime;
	unsigned  long	nm_rate;
	unsigned  long	nm_runtime;

	char		nm_cmdinterp[CI_MAXNAME+1];
	Exits		nm_exits;
	Btmode		nm_mode;

	Jncond		nm_conds[MAXCVARS];
	Jnass		nm_asses[MAXSEVARS];
};

struct	jobnetmsg  {		/* Whole shooting match */
	struct	jobhnetmsg	hdr;
	unsigned short	nm_nredirs,
			nm_nargs,
			nm_nenv;
	short		nm_title,
			nm_direct,
			nm_redirs,
			nm_env,
			nm_arg;
	char		nm_space[JOBSPACE];
};

struct	rassmsg	{		/* Tell other end to do remote assigns */
	msghdr		hdr;
	jident		jid;
	unsigned char	flags;	/* We only use the bottom 8 bits - no BJA_REVERSE used */
	unsigned char	source;	/* This can fit in an unsigned char */
	unsigned short	status;	/* This can't */
};

struct	jobcmsg	{		/* Job control message */
	msghdr		hdr;
	jident		jid;
	unsigned  long	param;
};

struct	jobnotmsg  {		/* Job notify message */
	msghdr		hdr;
	jident		jid;
	short		msgcode;/* Message code */
	short		padding;/* Padding to 4 bytes */
	jobno_t		sout;	/* Standard out file number */
	jobno_t		serr;	/* Standard error file number */
};

struct	varnetmsg  {
	msghdr		hdr;
	vident		vid;
	long		nm_c_time; 		/* Variable create time */
	unsigned  char	nm_type;		/* Variable type */
	unsigned  char	nm_flags;		/* Export should be set! */
	char		nm_name[BTV_NAME+1];	/* Variable name or new name */
	char		nm_comment[BTV_COMMENT+1]; /* Comment field We should now be long-aligned */
	Btmode		nm_mode;
	unsigned  char	nm_consttype;		/* Constant type */
	unsigned  char	nm_padding[3];		/* Pad out to long */
	union	{
		char	nm_string[BTC_VALUE+3];	/* Ensure long-align */
		long	nm_long;
	}  nm_un;
};

/*
 *	Message for changing uid or group.
 *	The code in hdr.code says which
 */

struct	vugmsg	{
	msghdr		hdr;
	vident		vid;
	char		newug[UIDSIZE+1];
};

/*
 *	Message format for sending command interpreter list
 *	(Is this needed????)
 */

struct	cimsg	{
	msghdr		hdr;
	unsigned long	numb;
	Cmdint		ci[1];	/* As long as it needs to be */
};

#ifdef	__cplusplus
void	mode_pack(Btmode &, const Btmode &);
void	jid_pack(jident &, const jident &);
void	jid_unpack(jident &, const jident &);
void	vid_pack(vident &, const vident &);
void	jobh_pack(jobhnetmsg &, const Btjob &);
void	jobh_unpack(Btjob &, const jobhnetmsg &);
unsigned job_pack(jobnetmsg &, const Btjob &);
void	job_unpack(Btjob &, const jobnetmsg &);
void	var_pack(varnetmsg &, const Btvar &);
void	var_unpack(Btvar &, const varnetmsg &);
void	sync_single(const Btjob &);
#endif 

extern	void	net_recvmsg(WPARAM, LPARAM);
