/* xbnet_ext.h -- extern definitions for xbnetserv

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

#define	NAMESIZE	14			/* Padding for temp file */

struct	pend_job	{
	netid_t		clientfrom;		/* Who is responsible (or 0) */
	FILE		*out_f;			/* Output file */
	char		tmpfl[NAMESIZE+1];	/* Temporary file */
	USHORT		joblength;		/* Length expected all told */
	USHORT		lengthexp;		/* Length expected (of job descriptor) */
	char		*cpos;			/* Pointer into jobout */
	unsigned  char	prodsent;		/* Sent a prod */
	USHORT		timeout;		/* Timeout value to decide when to enquire death */
	time_t		lastaction;		/* Last message received (for timeout) */
	jobno_t		jobn;			/* Job number we are creating */
	struct nijobmsg	jobin;			/* Job details pending */
	Btjob		jobout;			/* Constructed job */
};

/*  Structure used to hash host ids  */

struct	hhash	{
	struct	hhash	*hn_next;	/* Next in hash chain */
	struct	remote	rem;		/* Remote structure */
	char		*dosname;	/* DOS user name (W95 user name if "roaming") */
	char		*actname;	/* Actual name (W95 user name if not "roaming") */
	USHORT		timeout;	/* Timeout value for seeing if it is asleep */
	USHORT		flags;		/* Status flags same as UAL_OK etc  */
	time_t		lastaction;	/* Last action for timeout */
};

/*  Structure used to hash client user names  */

struct	cluhash  {
	struct	cluhash	*next;		/* Next in hash chain */
	struct	cluhash	*alias_next;	/* Next in alias hash chain */
	unsigned	refcnt;		/* Reference count whilst deallocating */
	char		*machname;	/* Machine name "opt" */
	struct	remote	rem;		/* NB not a "*" */
};

#ifndef	_NFILE
#define	_NFILE	64
#endif

#define	MAX_PEND_JOBS	(_NFILE / 3)

#define	MAXTRIES	5
#define	TRYTIME		20

/* It seems to take more than one attempt to set up a UDP port at times,
   so.... */

#define	UDP_TRIES	3

#define	JOB_MOD	60000			/*  Modulus of job numbers */

extern	uid_t	Daemuid, Realuid;
extern	uid_t	Daemgid, Realgid;
extern	netid_t	myhostid, localhostid;
extern	SHORT	qsock, uasock, apirsock;
extern	SHORT	qportnum, uaportnum, apirport, apipport;
extern	USHORT	err_which;
extern	USHORT	orig_umask;
extern	unsigned  myhostl;
extern	char	*myhostname;
extern	SHORT	tcpproto, udpproto;
extern	int	Ctrl_chan;
#ifndef	USING_FLOCK
extern	int	Sem_chan;
#endif
extern	long	mymtype;
extern	int	had_alarm, hadrfresh;
extern	struct	pend_job	pend_list[];
extern	struct	hhash	*nhashtab[];
extern	struct	cluhash	*cluhashtab[];

extern	unsigned tracing;
extern	FILE	*tracefile;

extern void  trace_op(const int_ugid_t, const char *);
extern void  trace_op_res(const int_ugid_t, const char *, const char *);
extern void  client_trace_op(const netid_t, const char *);
extern void  client_trace_op_name(const netid_t, const char *, const char *);

#define	TRACE_SYSOP		(1 << 0)
#define	TRACE_APICONN		(1 << 1)
#define	TRACE_APIOPSTART	(1 << 2)
#define	TRACE_APIOPEND		(1 << 3)
#define	TRACE_CLICONN		(1 << 4)
#define	TRACE_CLIOPSTART	(1 << 5)
#define	TRACE_CLIOPEND		(1 << 6)

extern	void  abort_job(struct pend_job *);
extern	void  btuser_pack(BtuserRef, BtuserRef);
extern	void  get_hf(const unsigned, char *);
extern	void  put_hf(const unsigned, char *);
extern	void  open_cifile(const unsigned);
extern	void  process_api();
extern	void  process_ua();
extern	void  send_askall();
extern	void  tell_friends(struct hhash *);
extern	void  tell_myself(struct hhash *);
extern	int  tcp_serv_accept(const int, netid_t *);
extern	int  validate_job(BtjobRef, const Btuser *);
extern	int  checkpw(const char *, const char *);
extern	int  convert_username(struct hhash *, struct ni_jobhdr *, BtjobRef, BtuserRef *);
extern	int  chk_vgroup(const int_ugid_t, const char *);

extern	unsigned  process_alarm();
extern	unsigned  unpack_cavars(BtjobRef, const struct nijobmsg *);
extern	unsigned  unpack_job(BtjobRef, const struct nijobmsg *, const unsigned, const netid_t);
extern	unsigned  calc_clu_hash(const char *);

extern	struct hhash *find_remote(const netid_t);
extern	struct pend_job *add_pend(const netid_t);
extern	struct pend_job *find_j_by_jno(const jobno_t);
extern	FILE *goutfile(jobno_t *, char *, const int);

extern	struct cluhash *update_roam_name(struct hhash *, const char *);
extern	struct cluhash *new_roam_name(const netid_t, struct hhash **, const char *);
extern	int  	       update_nonroam_name(struct hhash *, const char *);
