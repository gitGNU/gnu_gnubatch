/* xbnetserv.c -- main module for xbnetserv

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
#include "incl_unix.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
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
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "xbapi_int.h"
#include "xbnet_ext.h"
#include "services.h"

static	char	Filename[] = __FILE__;

SHORT	qsock,			/* TCP Socket for accepting queued jobs on */
	uasock,			/* Datagram socket for user access enquiries */
	apirsock;		/* API Request socket */

SHORT	qportnum,		/* Port number for TCP */
	uaportnum,		/* Port number for UDP */
	apirport,		/* Port number for API requests */
	apipport;		/* UDP port number for prompt messages to API */

netid_t	localhostid;		/* IP of "localhost" sometimes different */

SHORT	tcpproto, udpproto;

const	char	Sname[] = GBNETSERV_PORT,
		ASrname[] = DEFAULT_SERVICE,
		ASmname[] = MON_SERVICE;

int	had_alarm, hadrfresh;

int	Ctrl_chan = -1;
#ifndef	USING_FLOCK
int	Sem_chan;
#endif

#ifdef	SHAREDLIBS
#include "helpalt.h"
uid_t		Effuid;
gid_t		Effgid;
char		*Args[1], *exitcodename, *signalname;
HelpaltRef	repunit, ifnposses, days_abbrev;
BtuserRef	mypriv;
#else
ULONG		Dispflags;	/* Resolution only */
#endif
char		*jobqueue;

/* We don't use these fields as the API strips out users and groups
   itself, but we now incorporate the screening in the library
   routine because we want to avoid having so many rjobfiles
   everywhere.  */

char		*Restru,
		*Restrg;

long	mymtype;

static	char	*spdir;

static	char	tmpfl[NAMESIZE + 1];

FILE	*Cfile;

USHORT	err_which;		/* Which we are complaining about */
USHORT	orig_umask;		/* Saved copy of original umask */

uid_t	Daemuid,
	Daemgid,
	Realuid;
gid_t	Realgid;		/* Need to be global to use xbmpermitted from look */

unsigned	myhostl;	/* Length of ... */
char		*myhostname;	/* We send our variables prefixed by this */

BtjobRef	JREQ;

struct	hhash	*nhashtab[NETHASHMOD];
struct	cluhash	*cluhashtab[NETHASHMOD];

extern	char	dosuser[];

struct	pend_job  pend_list[MAX_PEND_JOBS];/* List of pending UDP jobs */

unsigned tracing = 0;
FILE	*tracefile;

void  nomem(const char *fl, const int ln)
{
	fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
	exit(E_NOMEM);
}

unsigned  calc_clu_hash(const char *name)
{
	unsigned  sum = 0;
	while  (*name)
		sum += *name++;
	return  sum % NETHASHMOD;
}

/* Clear details of client "roaming" users if hosts file changes.  */

static void  zap_clu_hash()
{
	unsigned  cnt;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	cluhash  **cpp, *cp;
		cpp = &cluhashtab[cnt];

		/* An item might be on the hash table twice under its
		   own name and the alias name.  First process
		   the "own name" entries.  */

		while  ((cp = *cpp))  {
			if  (--cp->refcnt == 0)  {
				*cpp = cp->next;
				if  (cp->machname)
					free(cp->machname);
				free(cp);
			}
			else
				cpp = &cp->next;
		}

		/* Now repeat for the alias name entries.  */

		cpp = &cluhashtab[cnt];
		while  ((cp = *cpp))  {
			*cpp = cp->alias_next;
			if  (cp->machname)
				free(cp->machname);
			free(cp);
		}
	}
}

/* Add IP address representing "me" to table for benefit of APIs on local host */

static void  addme(const netid_t mid)
{
	unsigned nhval = calcnhash(mid);
	struct	hhash	*hp;
	time_t	now = time((time_t *) 0);

	if  (!(hp = (struct hhash *) malloc(sizeof(struct hhash))))
		ABORT_NOMEM;

	BLOCK_ZERO(hp, sizeof(struct hhash));
	hp->hn_next = nhashtab[nhval];
	nhashtab[nhval] = hp;
	hp->rem.hostid = mid;
	hp->rem.ht_flags = HT_MANUAL|HT_PROBEFIRST|HT_TRUSTED;
	hp->timeout = hp->rem.ht_timeout = 0x7fff;
	hp->lastaction = hp->rem.lastwrite = now;
	hp->rem.n_uid = Daemuid;
	hp->rem.n_gid = Daemgid;
	hp->flags = UAL_OK;
}

/* Read in hosts file and build up interesting stuff */

static void  process_hfile()
{
	struct	remote	*rp;
	extern	char	hostf_errors;
	time_t	now = time((time_t *) 0);

	hostf_errors = 0;

	while  ((rp = get_hostfile()))

		if  (rp->ht_flags & HT_ROAMUSER)  {
			struct	cluhash	*cp, **hpp, *hp;
			int_ugid_t	lkuid;

			/* Roaming user - add main and alias name to
			   hash table.  We don't try to interpret
			   the user names at this stage.  */

			if  (!(cp = (struct cluhash *) malloc(sizeof(struct cluhash))))
				ABORT_NOMEM;
			cp->next = cp->alias_next = (struct cluhash *) 0;
			cp->rem = *rp;

			cp->rem.n_uid = Daemuid;
			cp->rem.n_gid = Daemgid;

			/* The machine name (if any) is held in "dosuser".  Please note that this is
			   one of the places where we assume HOSTNSIZE > UIDSIZE */

			cp->machname = dosuser[0]? stracpy(dosuser) : (char *) 0;
			cp->refcnt = 1;	/* For now */

			/* Stick it on the end of the hash chain.
			   Repeat for alias name if applicable.  */

			for  (hpp = &cluhashtab[calc_clu_hash(rp->hostname)]; (hp = *hpp);  hpp = &hp->next)
				;
			*hpp = cp;

			if  ((lkuid = lookup_uname(rp->hostname)) != UNKNOWN_UID)  {
				cp->rem.n_uid = lkuid;
				cp->rem.n_gid = lastgid;
			}
			if  (rp->alias[0])  {
				for  (hpp = &cluhashtab[calc_clu_hash(rp->alias)]; (hp = *hpp);  hpp = &hp->alias_next)
					;
				*hpp = cp;
				cp->refcnt++;
				if  ((lkuid = lookup_uname(rp->alias)) != UNKNOWN_UID)  {
					cp->rem.n_uid = lkuid;
					cp->rem.n_gid = lastgid;
				}
			}
		}
		else  {
			struct	hhash	*hp;
			unsigned  nhval = calcnhash(rp->hostid);

			/* These are "regular" machines.  */

			if  (!(hp = (struct hhash *) malloc(sizeof(struct hhash))))
				ABORT_NOMEM;
			hp->hn_next = nhashtab[nhval];
			nhashtab[nhval] = hp;
			hp->rem = *rp;
			hp->dosname = (char *) 0;
			hp->actname = (char *) 0;
			hp->flags = UAL_OK;
			hp->rem.n_uid = Daemuid;
			hp->rem.n_gid = Daemgid;
			if  (rp->ht_flags & HT_DOS)  {
				int_ugid_t	lkuid;
				hp->dosname = stracpy(dosuser);
				hp->actname = stracpy(dosuser);	/* Saves testing for it */
				if  ((lkuid = lookup_uname(dosuser)) != UNKNOWN_UID)  {
					hp->rem.n_uid = lkuid;
					hp->rem.n_gid = lastgid;
				}
				if  (rp->ht_flags & HT_PWCHECK)
					hp->flags = UAL_NOK;
			}
			hp->timeout = rp->ht_timeout;
			hp->lastaction = now;
		}

	end_hostfile();

	/* Create entries for "me" to allow for API connections from local hosts */

	addme(myhostid);
	addme(htonl(INADDR_LOOPBACK));

	/* This may be a good place to warn people about errors in the host file.  */

	if  (hostf_errors)
		print_error($E{Warn errors in host file});
}

/* Catch hangup signals and re-read hosts file a la mountd */

static RETSIGTYPE  catchhup(int n)
{
	unsigned  cnt;
	struct	hhash	*hp, *np;
#ifdef	UNSAFE_SIGNALS
	signal(n, SIG_IGN);
#endif
	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		for  (hp = nhashtab[cnt];  hp;  hp = np)  {
			if  (hp->dosname)
				free(hp->dosname);
			if  (hp->actname)
				free(hp->actname);
			np = hp->hn_next;
			free((char *) hp);
		}
		nhashtab[cnt] = (struct hhash *) 0;
	}
	zap_clu_hash();
	process_hfile();
	un_rpwfile();
	rpwfile();
	send_askall();
	/* NB Don't think we need a rgrpfile(); here - maybe supp groups sometime?? */
#ifdef	UNSAFE_SIGNALS
	signal(n, catchhup);
#endif
}

struct hhash *find_remote(const netid_t hid)
{
	struct	hhash	*hp;

	for  (hp = nhashtab[calcnhash(hid)];  hp;  hp = hp->hn_next)
		if  (hp->rem.hostid == hid)  {
			time(&hp->lastaction);		/* Remember last action for timeouts */
			return  hp;
		}
	return  (struct  hhash  *) 0;
}

static	char	sigstocatch[] =	{ SIGINT, SIGQUIT, SIGTERM };

/* On a signal, remove file (TCP connection) */

static RETSIGTYPE  catchdel(int n)
{
	unlink(tmpfl);
	exit(E_SIGNAL);
}

/* Main path - remove files pending for UDP */

RETSIGTYPE  catchabort(int n)
{
	int	cnt;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	zign;
	zign.sighandler_el = SIG_IGN;
	sigmask_clear(zign);
	zign.sigflags_el = 0;
	sigact_routine(n, &zign, (struct sigstruct_name *) 0);
#else
	signal(n, SIG_IGN);
#endif
	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		abort_job(&pend_list[cnt]);
	exit(E_SIGNAL);
}

/* Catch alarm signals */

static RETSIGTYPE  catchalarm(int n)
{
#ifdef	UNSAFE_SIGNALS
	signal(n, catchalarm);
#endif
	had_alarm++;
}

/* This notes signals from (presumably) the scheduler.  */

RETSIGTYPE  markit(int sig)
{
#ifdef	UNSAFE_SIGNALS
	signal(sig, markit);
#endif
	hadrfresh++;
}

static	void	catchsigs(void (*catchfn)(int))
{
	int	i;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z, oldz;
	z.sighandler_el = catchfn;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
		sigact_routine(sigstocatch[i], &z, &oldz);
		if  (oldz.sighandler_el == SIG_IGN)
			sigact_routine(sigstocatch[i], &oldz, (struct sigstruct_name *) 0);
	}
#else
	for  (i = 0;  i < sizeof(sigstocatch);  i++)
		if  (signal(sigstocatch[i], catchfn) == SIG_IGN)
			signal(sigstocatch[i], SIG_IGN);
#endif
}

static void  openrfile()
{
	/* If message queue does not exist, then the batch scheduler
	   isn't running. I don't think that we want to randomly
	   start it.  */

	if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
		print_error($E{Scheduler not running});
		exit(E_NOTRUN);
	}
#ifndef	USING_FLOCK
	if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, 0)) < 0)  {
		print_error($E{Cannot open semaphore});
		exit(E_SETUP);
	}
#endif
}

static void  lognprocess()
{
	Shipc		Oreq;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
	z.sighandler_el = markit;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
	z.sighandler_el = catchalarm;
	sigact_routine(SIGALRM, &z, (struct sigstruct_name *) 0);
#else
	signal(QRFRESH, markit);
	signal(SIGALRM, catchalarm);
#endif
	BLOCK_ZERO(&Oreq, sizeof(Oreq));
	Oreq.sh_mtype = TO_SCHED;
	Oreq.sh_params.mcode = N_XBNATT;
	Oreq.sh_params.upid = getpid();
	msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq), 0); /* Not expecting reply */
}

/* Unpack a job and see what British Hairyways has broken this time
   Report any errors with the appropriate code */

unsigned unpack_job(BtjobRef to, const struct nijobmsg *from, const unsigned length, const netid_t whofrom)
{
#ifndef	WORDS_BIGENDIAN
	unsigned	cnt;
	JargRef	darg;	EnvirRef denv;	RedirRef dred;
	const	Jarg  *sarg;	const	Envir *senv;	const	Redir *sred;
#endif

	BLOCK_ZERO(to, sizeof(Btjob));
	if  (whofrom != myhostid  &&  whofrom != localhostid)
		to->h.bj_orighostid = whofrom;
	to->h.bj_progress	=	from->ni_hdr.ni_progress;
	to->h.bj_pri		=	from->ni_hdr.ni_pri;
	to->h.bj_jflags		=	from->ni_hdr.ni_jflags;
	to->h.bj_times.tc_istime=	from->ni_hdr.ni_istime;
	to->h.bj_times.tc_mday	=	from->ni_hdr.ni_mday;
	to->h.bj_times.tc_repeat=	from->ni_hdr.ni_repeat;
	to->h.bj_times.tc_nposs	=	from->ni_hdr.ni_nposs;

	to->h.bj_title		=	ntohs(from->ni_hdr.ni_title);
	to->h.bj_direct		=	ntohs(from->ni_hdr.ni_direct);
	to->h.bj_redirs		=	ntohs(from->ni_hdr.ni_redirs);
	to->h.bj_env		=	ntohs(from->ni_hdr.ni_env);
	to->h.bj_arg		=	ntohs(from->ni_hdr.ni_arg);

	to->h.bj_ll		=	ntohs(from->ni_hdr.ni_ll);
	to->h.bj_umask		=	ntohs(from->ni_hdr.ni_umask);
	to->h.bj_times.tc_nvaldays =	ntohs(from->ni_hdr.ni_nvaldays);
	to->h.bj_autoksig	=	ntohs(from->ni_hdr.ni_autoksig);
	to->h.bj_runon		=	ntohs(from->ni_hdr.ni_runon);
	to->h.bj_deltime	=	ntohs(from->ni_hdr.ni_deltime);

	to->h.bj_nredirs	=	ntohs(from->ni_hdr.ni_nredirs);
	to->h.bj_nargs		=	ntohs(from->ni_hdr.ni_nargs);
	to->h.bj_nenv		=	ntohs(from->ni_hdr.ni_nenv);
	to->h.bj_ulimit		=	ntohl(from->ni_hdr.ni_ulimit);
	to->h.bj_times.tc_nexttime =	ntohl(from->ni_hdr.ni_nexttime);
	to->h.bj_times.tc_rate	=	ntohl(from->ni_hdr.ni_rate);
	to->h.bj_runtime	=	ntohl(from->ni_hdr.ni_runtime);

	strcpy(to->h.bj_cmdinterp, from->ni_hdr.ni_cmdinterp);
	if  ((validate_ci(to->h.bj_cmdinterp)) < 0)
		return  XBNR_BADCI;

	to->h.bj_exits		=	from->ni_hdr.ni_exits;

	/* Copy in user and group from protocol later perhaps but
	   maybe replacement user/group Do variables after we've
	   done that.  */

	strncpy(to->h.bj_mode.o_user, from->ni_hdr.ni_mode.o_user, UIDSIZE);
	strncpy(to->h.bj_mode.o_group, from->ni_hdr.ni_mode.o_group, UIDSIZE);
	to->h.bj_mode.u_flags = ntohs(from->ni_hdr.ni_mode.u_flags);
	to->h.bj_mode.g_flags = ntohs(from->ni_hdr.ni_mode.g_flags);
	to->h.bj_mode.o_flags = ntohs(from->ni_hdr.ni_mode.o_flags);

	/* Now for the strings....  */

	BLOCK_COPY(to->bj_space, from->ni_space, length - sizeof(struct nijobhmsg));
#ifndef	WORDS_BIGENDIAN
	darg = (JargRef) &to->bj_space[to->h.bj_arg];
	denv = (EnvirRef) &to->bj_space[to->h.bj_env];
	dred = (RedirRef) &to->bj_space[to->h.bj_redirs];
	sarg = (const Jarg *) &from->ni_space[to->h.bj_arg]; /* Did mean to!! */
	senv = (const Envir *) &from->ni_space[to->h.bj_env];
	sred = (const Redir *) &from->ni_space[to->h.bj_redirs];

	for  (cnt = 0;  cnt < to->h.bj_nargs;  cnt++)  {
		*darg++ = ntohs(*sarg);
		sarg++;	/* Not falling for ntohs being a macro!!! */
	}
	for  (cnt = 0;  cnt < to->h.bj_nenv;  cnt++)  {
		denv->e_name = ntohs(senv->e_name);
		denv->e_value = ntohs(senv->e_value);
		denv++;
		senv++;
	}
	for  (cnt = 0;  cnt < to->h.bj_nredirs; cnt++)  {
		dred->arg = ntohs(sred->arg);
		dred++;
		sred++;
	}
#endif
	return  0;
}

/* Unpack condition and assignment vars in a job - done after we get
   the user ids.  Report any errors with the appropriate code */

unsigned  unpack_cavars(BtjobRef to, const struct nijobmsg *from)
{
	unsigned	cnt;
	netid_t		hostid;
	ULONG		Saveseq;

	for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
		const  Nicond	*cf = &from->ni_hdr.ni_conds[cnt];
		JcondRef	ct = &to->h.bj_conds[cnt];
		if  (cf->nic_compar == C_UNUSED)
			break;
		rvarfile(1);
		hostid = cf->nic_var.ni_varhost == myhostid || cf->nic_var.ni_varhost == localhostid? 0: cf->nic_var.ni_varhost;
		if  ((ct->bjc_varind = lookupvar(cf->nic_var.ni_varname, hostid, BTM_READ, &Saveseq)) < 0)  {
			err_which = (USHORT) cnt;
			return  XBNR_BADCVAR;
		}
		ct->bjc_compar = cf->nic_compar;
		ct->bjc_iscrit = cf->nic_iscrit;
		if  ((ct->bjc_value.const_type = cf->nic_type) == CON_STRING)
			strncpy(ct->bjc_value.con_un.con_string, cf->nic_un.nic_string, BTC_VALUE);
		else
			ct->bjc_value.con_un.con_long = ntohl(cf->nic_un.nic_long);
	}
	for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
		const  Niass	*af = &from->ni_hdr.ni_asses[cnt];
		JassRef		at = &to->h.bj_asses[cnt];
		if  (af->nia_op == BJA_NONE)
			break;
		rvarfile(1);
		hostid = af->nia_var.ni_varhost == myhostid || af->nia_var.ni_varhost == localhostid? 0: af->nia_var.ni_varhost;
		if  ((at->bja_varind = lookupvar(af->nia_var.ni_varname, hostid, BTM_READ|BTM_WRITE, &Saveseq)) < 0)  {
			err_which = (USHORT) cnt;
			return  XBNR_BADAVAR;
		}
		at->bja_flags = ntohs(af->nia_flags);
		at->bja_op = af->nia_op;
		at->bja_iscrit = af->nia_iscrit;
		if  ((at->bja_con.const_type = af->nia_type) == CON_STRING)
			strncpy(at->bja_con.con_un.con_string, af->nia_un.nia_string, BTC_VALUE);
		else
			at->bja_con.con_un.con_long = ntohl(af->nia_un.nia_long);
	}
	return  0;
}

/* Pack up a btuser structure */

void  btuser_pack(BtuserRef to, BtuserRef from)
{
	to->btu_isvalid = from->btu_isvalid;
	to->btu_minp = from->btu_minp;
	to->btu_maxp = from->btu_maxp;
	to->btu_defp = from->btu_defp;
	to->btu_user = htonl(from->btu_user);	/* Don't really care about this */
	to->btu_maxll = htons(from->btu_maxll);
	to->btu_totll = htons(from->btu_totll);
	to->btu_spec_ll = htons(from->btu_spec_ll);
	to->btu_priv = htonl(from->btu_priv);
	to->btu_jflags[0] = htons(from->btu_jflags[0]);
	to->btu_jflags[1] = htons(from->btu_jflags[1]);
	to->btu_jflags[2] = htons(from->btu_jflags[2]);
	to->btu_vflags[0] = htons(from->btu_vflags[0]);
	to->btu_vflags[1] = htons(from->btu_vflags[1]);
	to->btu_vflags[2] = htons(from->btu_vflags[2]);
}

int  tcp_serv_open(SHORT portnum)
{
	int	result;
	struct	sockaddr_in	sin;
#if	defined(SO_REUSEADDR)
	int	on = 1;
#endif
	sin.sin_family = AF_INET;
	sin.sin_port = portnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;

	if  ((result = socket(PF_INET, SOCK_STREAM, tcpproto)) < 0)
		return  -1;
#ifdef	SO_REUSEADDR
	setsockopt(result, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
	if  (bind(result, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(result, 5) < 0)  {
		close(result);
		return  -1;
	}
	return  result;
}

int  tcp_serv_accept(const int msock, netid_t *whofrom)
{
	int	sock;
	SOCKLEN_T	     sinl;
	struct	sockaddr_in  sin;

	sinl = sizeof(sin);
	if  ((sock = accept(msock, (struct sockaddr *) &sin, &sinl)) < 0)
		return  -1;
	*whofrom = sin.sin_addr.s_addr;
	return  sock;
}

int  udp_serv_open(SHORT portnum)
{
	int	result;
	struct	sockaddr_in	sin;
	sin.sin_family = AF_INET;
	sin.sin_port = portnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;

	/* Open Datagram socket for user access stuff */

	if  ((result = socket(PF_INET, SOCK_DGRAM, udpproto)) < 0)
		return  -1;

	if  (bind(result, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(result);
		return  -1;
	}
	return  result;
}

/* Set up network stuff - a UDP port to receive/send enquiries and UDP
   jobs on and a TCP port for receiving spr files.  */

static int  init_network()
{
	struct	hostent	*hp;
	struct	servent	*sp;
	struct	protoent  *pp;
	char	*tcp_protoname,
		*udp_protoname;

	/* Get id of local host if different */

	if  ((hp = gethostbyname("localhost")))
		localhostid = *(netid_t *) hp->h_addr;

	/* Get TCP/UDP protocol names */

	if  (!((pp = getprotobyname("tcp"))  || (pp = getprotobyname("TCP"))))  {
		print_error($E{Netconn no TCP abort});
		return  0;
	}
	tcp_protoname = stracpy(pp->p_name);
	tcpproto = pp->p_proto;
	if  (!((pp = getprotobyname("udp"))  || (pp = getprotobyname("UDP"))))  {
		print_error($E{Netconn no UDP abort});
		return  0;
	}
	udp_protoname = stracpy(pp->p_name);
	udpproto = pp->p_proto;
	endprotoent();

	if  (!(sp = env_getserv(Sname, tcp_protoname)))  {
		disp_str = (char *) Sname;
		disp_str2 = tcp_protoname;
		print_error($E{Netconn no service name});
		return  0;
	}

	/* Shhhhhh....  I know this should be network byte order, but
	   lets leave it alone for now.  */

	qportnum = sp->s_port;
	if  (!(sp = env_getserv(Sname, udp_protoname)))  {
		disp_str = (char *) Sname;
		disp_str2 = udp_protoname;
		print_error($E{Netconn no service name});
		return  0;
	}

	uaportnum = sp->s_port;

	if  (!(sp = env_getserv(ASrname, tcp_protoname)))  {
		disp_str = (char *) ASrname;
		disp_str2 = tcp_protoname;
		print_error($E{Netconn no API req port});
		apirport = 0;
	}
	else
		apirport = sp->s_port;
	if  (!(sp = env_getserv(ASmname, udp_protoname)))  {
		disp_str = (char *) ASmname;
		disp_str2 = udp_protoname;
		print_error($E{Netconn no API prompt port});
		apipport = 0;
	}
	else
		apipport = sp->s_port;
	free(tcp_protoname);
	free(udp_protoname);
	endservent();

	if  ((qsock = tcp_serv_open(qportnum)) < 0)  {
		disp_arg[0] = ntohs(qportnum);
		print_error($E{Netconn no open TCP});
		return  0;
	}

	if  ((uasock = udp_serv_open(uaportnum)) < 0)  {
		disp_arg[0] = ntohs(uaportnum);
		print_error($E{Netconn no open UDP});
		return  0;
	}

	if  (apirport)
		apirsock = tcp_serv_open(apirport);
	return  1;
}

/* Generate output file name */

FILE *goutfile(jobno_t *jnp, char *tmpfl, const int isudp)
{
	FILE	*res;
	int	fid;
	for  (;;)  {
		strcpy(tmpfl, mkspid(SPNAM, *jnp));
		if  ((fid = open(tmpfl, isudp? O_RDWR|O_CREAT|O_EXCL: O_WRONLY|O_CREAT|O_EXCL, 0400)) >= 0)
			break;
		*jnp += JN_INC;
	}
	if  (!isudp)
		catchsigs(catchdel);

	if  ((res = fdopen(fid, isudp? "w+": "w")) == (FILE *) 0)  {
		unlink(tmpfl);
		ABORT_NOMEM;
	}
	return  res;
}

static int  sock_read(const int sock, char *buffer, int nbytes)
{
	while  (nbytes > 0)  {
		int	rbytes = read(sock, buffer, nbytes);
		if  (rbytes <= 0)
			return  0;
		buffer += rbytes;
		nbytes -= rbytes;
	}
	return  1;
}

/* Copy to output file.  (NB empty files are ok) */

static int  copyout(const int sock, FILE *outf)
{
	struct	ni_jobhdr	hd;
	char	buffer[CL_SV_BUFFSIZE];

	while  (sock_read(sock, (char *) &hd, sizeof(hd)) &&  hd.code == CL_SV_JOBDATA)  {
		int	inbytes, obytes = ntohs(hd.joblength);
		if  (!sock_read(sock, buffer, obytes))
			break;
		for  (inbytes = 0;  inbytes < obytes;  inbytes++)
			if  (putc(buffer[inbytes], outf) == EOF)  {
				fclose(outf);
				return  XBNR_FILE_FULL;
			}
	}
	if  (fclose(outf) != EOF)
		return  0;
	return  XBNR_FILE_FULL;
}

static void  tcpreply(const int sock, const int code, const LONG param)
{
	int	nbytes, rbytes;
	char	*res;
	struct	client_if	result;

	result.code = (unsigned char) code;
	result.param = htonl(param);
	res = (char *) &result;
	nbytes = sizeof(result);
	do  {
		if  ((rbytes = write(sock, res, (unsigned) nbytes)) < 0)
			return;
		res += rbytes;
		nbytes -= rbytes;
	}  while  (nbytes > 0);
}

int  validate_job(BtjobRef jp, const Btuser *mypriv)
{
	int	cinum;

	/* Can the geyser do anything at all */

	if  ((mypriv->btu_priv & BTM_CREATE) == 0)
		return  XBNR_NOCRPERM;

	/* Validate priority */

	if  (jp->h.bj_pri < mypriv->btu_minp  ||  jp->h.bj_pri > mypriv->btu_maxp)
		return  XBNR_BAD_PRIORITY;

	/* Check that we are happy about the job modes */

	if  (!(mypriv->btu_priv & BTM_UMASK)  &&
	     (jp->h.bj_mode.u_flags != mypriv->btu_jflags[0] ||
	      jp->h.bj_mode.g_flags != mypriv->btu_jflags[1] ||
	      jp->h.bj_mode.o_flags != mypriv->btu_jflags[2]))
		return  XBNR_NOCMODE;

	/* Validate load level */

	if  ((cinum = validate_ci(jp->h.bj_cmdinterp)) < 0)
		return  XBNR_BADCI;

	if  (jp->h.bj_ll == 0)
		jp->h.bj_ll = Ci_list[cinum].ci_ll;
	else  {
		if  (jp->h.bj_ll > mypriv->btu_maxll)
			return  XBNR_BAD_LL;
		if  (!(mypriv->btu_priv & BTM_SPCREATE) && jp->h.bj_ll != Ci_list[cinum].ci_ll)
			return  XBNR_BAD_LL;
	}
	return  0;
}

/* Here is where we manhandle the user id.  We leave the
   eventually-decided user/group in Realuid/Realgid.  Return 0 if
   OK otherwise the error code.  */

int convert_username(struct hhash *frp, struct ni_jobhdr *nih, BtjobRef jp, BtuserRef *myprivp)
{
	char		*repu, *repg;
	int_ugid_t	nuid, ngid, possug;
	BtuserRef	mp;

	if  (frp->rem.ht_flags & HT_DOS)  {

		if  (frp->rem.ht_flags & HT_ROAMUSER)
			jp->h.bj_jflags |= BJ_ROAMUSER;

		Realuid = (uid_t) frp->rem.n_uid;
		Realgid = (gid_t) frp->rem.n_gid;

		/* Possibly we have a replacement user/group in the header */

		repu = nih->uname;
		repg = nih->gname;
	}
	else  {			/* Unix end - transmogrify user name */
		if  ((nuid = lookup_uname(nih->uname)) == UNKNOWN_UID)
			return  XBNR_UNKNOWN_USER;

		Realuid = (uid_t) nuid;
		Realgid = (gid_t) lastgid;

		if  ((ngid = lookup_gname(nih->gname)) != UNKNOWN_GID)
			Realgid = (gid_t) ngid;

		/* If we have a replacement user/group, they'll be in
		   the o_user/o_group fields.  */

		repu = jp->h.bj_mode.o_user;
		repg = jp->h.bj_mode.o_group;
	}

	/* Need the current value of the user privs as it might have changed.  */

	if  (!(mp = getbtuentry(Realuid)))
		return  XBNR_BAD_USER;

	*myprivp = mp;

	nuid = Realuid;
	ngid = Realgid;

	/* If user or group has changed, we need the permission.  */

	if  (repu[0] && (possug = lookup_uname(repu)) != UNKNOWN_UID)
		nuid = possug;
	if  (repg[0] && (possug = lookup_gname(repg)) != UNKNOWN_GID)
		ngid = possug;

	if  (nuid != Realuid  ||  ngid != Realgid)  {
		if  (!(mp->btu_priv & BTM_WADMIN))
			return  XBNR_BAD_USER;
		Realuid = nuid;
		Realgid = ngid;
	}

	return  0;
}

/* Process requests to enqueue file */

static void  process_q()
{
	int		sock, ret, tries;
	unsigned	joblength;
	ULONG		indx;
	netid_t		whofrom;
	PIDTYPE		pid;
	jobno_t		jn;
	struct	hhash	*frp;
	BtuserRef	mypriv;
	FILE		*outf;
	Shipc		Oreq;
	struct	ni_jobhdr	nih;
	struct	nijobmsg	inj;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zch;
	zch.sighandler_el = SIG_IGN;
	sigmask_clear(zch);
	zch.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(QRFRESH, &zch, (struct sigstruct_name *) 0);
#else
	signal(QRFRESH, SIG_IGN);
#endif

	if  ((sock = tcp_serv_accept(qsock, &whofrom)) < 0)
		return;

	if  (!sock_read(sock, (char *) &nih, sizeof(nih)))  {
		close(sock);
		return;
	}

	if  ((pid = fork()) < 0)  {
		print_error($E{Cannot fork});
		return;
	}
#ifndef	BUGGY_SIGCLD
	if  (pid != 0)  {
		close(sock);
		return;
	}
#else
	/* Make the process the grandchild so we don't have to worry about waiting for it later.  */

	if  (pid != 0)  {
#ifdef	HAVE_WAITPID
		while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
			;
#else
		PIDTYPE	wpid;
		while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
			;
#endif
		close(sock);
		return;
	}
	if  (fork() != 0)
		exit(0);
#endif
	joblength = ntohs(nih.joblength);
	if  (joblength < sizeof(struct nijobhmsg) || joblength > sizeof(struct nijobmsg))  {
		tcpreply(sock, XBNR_BAD_JOBDATA, 0);
		exit(0);
	}

	/* Slurp in rest of job header */

	sock_read(sock, (char *) &inj, (int) joblength);

	JREQ = &Xbuffer->Ring[indx = getxbuf_serv()];

	if  (!(frp = find_remote(whofrom)))  {
		freexbuf_serv(indx);
		tcpreply(sock, XBNR_UNKNOWN_CLIENT, 0);
		exit(0);
	}
	if  ((ret = unpack_job(JREQ, &inj, joblength, frp->rem.hostid)) != 0)  {
		freexbuf_serv(indx);
		tcpreply(sock, ret, err_which);
		exit(0);
	}
	if  ((ret = convert_username(frp, &nih, JREQ, &mypriv)) != 0)  {
		freexbuf_serv(indx);
		tcpreply(sock, ret, 0);
		exit(0);
	}
	if  ((ret = unpack_cavars(JREQ, &inj)) != 0)  {
		freexbuf_serv(indx);
		tcpreply(sock, ret, err_which);
		exit(0);
	}
	if  ((ret = validate_job(JREQ, mypriv)) != 0)  {
		freexbuf_serv(indx);
		tcpreply(sock, XBNR_ERR, ret);
		exit(0);
	}

	/* Kick off with current pid as job number.  */

	BLOCK_ZERO(&Oreq, sizeof(Oreq));
	Oreq.sh_mtype = TO_SCHED;
	mymtype = MTOFFSET + (jn = Oreq.sh_params.upid = getpid());
	Oreq.sh_params.mcode = J_CREATE;
	Oreq.sh_params.uuid = Realuid;
	Oreq.sh_params.ugid = Realgid;
	outf = goutfile(&jn, tmpfl, 0);
	JREQ->h.bj_job = jn;
	time(&JREQ->h.bj_time);

	if  ((ret = copyout(sock, outf)) != 0)  {
		tcpreply(sock, ret, 0);
		close(sock);
		unlink(tmpfl);
		freexbuf_serv(indx);
		exit(0);
	}

	Oreq.sh_un.sh_jobindex = indx;
	JREQ->h.bj_slotno = -1;
#ifdef	USING_MMAP
	sync_xfermmap();
#endif
	for  (tries = 0;  tries < MAXTRIES;  tries++)  {
		if  (msgsnd(Ctrl_chan, (struct msgbuf *)&Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) >= 0)  {
			freexbuf_serv(indx);
			if  ((ret = readreply()) != J_OK)  {
				unlink(tmpfl);
				close(sock);
				tcpreply(sock, XBNR_ERR, ret);
				exit(0);
			}
			tcpreply(sock, XBNQ_OK, jn);
			close(sock);
			exit(0);
		}
		sleep(TRYTIME);
	}
	freexbuf_serv(indx);
	unlink(tmpfl);
	tcpreply(sock, XBNR_QFULL, 0);
	exit(0);
}

static void  process()
{
	int	nret;
	unsigned  nexttime;
#ifdef	POLLSOCKETS
	int	npoll = apirsock >= 0? 3: 2;
	struct	pollfd	fdlist[3];

	fdlist[0].fd = qsock;
	fdlist[1].fd = uasock;
	fdlist[2].fd = apirsock;
	fdlist[0].events = fdlist[1].events = fdlist[2].events = POLLIN|POLLPRI|POLLERR;
	fdlist[0].revents = fdlist[1].revents = fdlist[2].revents = 0;

	for  (;;)  {

		alarm(nexttime = process_alarm());

		/* Treat an error as an indication to stop (probably signal).  */

		if  ((nret = poll(fdlist, npoll, -1)) < 0)  {
			if  (errno == EINTR)  {
				if  (had_alarm)  {
					had_alarm = 0;
					alarm(nexttime = process_alarm());
				}
				hadrfresh = 0;
				continue;
			}
			exit(0);
		}

		if  (nexttime != 0)
			alarm(0);

		if  (fdlist[0].revents)  {
			process_q();
			if  (--nret <= 0)
				continue;
		}
		if  (fdlist[1].revents)  {
			process_ua();
			if  (--nret <= 0)
				continue;
		}
		if  (fdlist[2].revents)
			process_api();
	}
#else
	int	highfd;
	fd_set	ready;

	highfd = qsock;
	if  (uasock > highfd)
		highfd = uasock;
	if  (apirsock > highfd)
		highfd = apirsock;
	for  (;;)  {

		alarm(nexttime = process_alarm());

		FD_ZERO(&ready);
		FD_SET(qsock, &ready);
		FD_SET(uasock, &ready);
		if  (apirsock >= 0)
			FD_SET(apirsock, &ready);

		if  ((nret = select(highfd+1, &ready, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0)) < 0)  {
			if  (errno == EINTR)  {
				if  (had_alarm)  {
					had_alarm = 0;
					alarm(nexttime = process_alarm());
				}
				hadrfresh = 0;
				continue;
			}
			exit(0);
		}
		if  (nexttime != 0)
			alarm(0);

		if  (FD_ISSET(qsock, &ready))  {
			process_q();
			if  (--nret <= 0)
				continue;
		}

		if  (FD_ISSET(uasock, &ready))  {
			process_ua();
			if  (--nret <= 0)
				continue;
		}

		if  (apirsock >= 0  &&  FD_ISSET(apirsock, &ready))
			process_api();
	}
#endif		/* !POLLSOCKETS */
}

void  trace_dtime(char *buf)
{
	time_t  now = time(0);
	struct  tm  *tp = localtime(&now);
	int	mon = tp->tm_mon+1, mday = tp->tm_mday;
#ifdef	HAVE_TM_ZONE
	if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
	if  (timezone >= 4 * 60 * 60)
#endif
	{
		mday = mon;
		mon = tp->tm_mday;
	}
	sprintf(buf, "%.2d/%.2d/%.2d|%.2d:%.2d:%.2d", mday, mon, tp->tm_year%100, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

void  trace_op(const int_ugid_t uid, const char *op)
{
	char	tbuf[20];
	trace_dtime(tbuf);
	fprintf(tracefile, "%s|%.5d|%s|%s\n", tbuf, getpid(), prin_uname(uid), op);
	fflush(tracefile);
}

void  trace_op_res(const int_ugid_t uid, const char *op, const char *res)
{
	char	tbuf[20];
	trace_dtime(tbuf);
	fprintf(tracefile, "%s|%.5d|%s|%s|%s\n", tbuf, getpid(), prin_uname(uid), op, res);
	fflush(tracefile);
}

void  client_trace_op(const netid_t nid, const char *op)
{
	char	tbuf[20];
	trace_dtime(tbuf);
	fprintf(tracefile, "%s|%.5d|client:%s|%s\n", tbuf, getpid(), look_host(nid), op);
	fflush(tracefile);
}

void  client_trace_op_name(const netid_t nid, const char *op, const char *uid)
{
	char	tbuf[20];
	trace_dtime(tbuf);
	fprintf(tracefile, "%s|%.5d|client:%s|%s|%s\n", tbuf, getpid(), look_host(nid), op, uid);
	fflush(tracefile);
}

/* Ye olde main routine.
   I don't expect any arguments & will ignore
   any the fool gives me, apart from remembering my name.  */

MAINFN_TYPE  main(int argc, char **argv)
{
	int_ugid_t	chku;
	char	*trf;
#ifndef	DEBUG
	PIDTYPE	pid;
#endif
#ifdef	STRUCT_SIG
	struct	sigstruct_name	zign;
#endif

	versionprint(argv, "$Revision: 1.2 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	init_xenv();

	if  ((Cfile = open_icfile()) == (FILE *) 0)
		exit(E_NOCONFIG);

	fcntl(fileno(Cfile), F_SETFD, 1);

	if  ((chku = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)  {
		Daemuid = ROOTID;
		Daemgid = getgid();
	}
	else  {
		Daemuid = chku;
		Daemgid = lastgid;
	}

	/* Revert to spooler user (we are setuser to root) */

#ifdef	SCO_SECURITY
	setluid(Daemuid);
	setuid(Daemuid);
#else
	setuid(Daemuid);
#endif

	orig_umask = umask(0);

	spdir = envprocess(SPDIR);
	if  (chdir(spdir) < 0)  {
		print_error($E{Cannot change directory});
		return  E_SETUP;
	}

	/* Set up tracing perhaps */

	trf = envprocess(XBNETTRACE);
	tracing = atoi(trf);
	free(trf);
	if  (tracing)  {
		trf = envprocess(XBNETTRFILE);
		tracefile = fopen(trf, "a");
		free(trf);
		if  (!tracefile)
			tracing = 0;
	}

	/* Initial processing of host file */

	process_hfile();
#ifdef	STRUCT_SIG
	zign.sighandler_el = catchhup;
	sigmask_clear(zign);
	zign.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGHUP, &zign, (struct sigstruct_name *) 0);
#else
	signal(SIGHUP, catchhup);
#endif
	myhostname = look_host(myhostid);
	myhostl = strlen(myhostname);
	openrfile();
	openjfile(1, 1);
	openvfile(1, 1);
	if  (open_ci(O_RDWR) != 0)  {
		print_error($E{Panic cannot open CI file});
		return  E_SETUP;
	}
	if  (!init_network())  {
		print_error($E{Netconn failure abort});
		exit(E_NETERR);
	}

#ifndef	DEBUG

	while  ((pid = fork()) < 0)  {
		print_error($E{Cannot fork});
		sleep(30);
	}
	if  (pid != 0)
		return  0;

	/*	Don't bother with suppressing setpgrp because I don't think anything
		depends on it and because hangup signals would be misinterpreted. */

#ifdef	SETPGRP_VOID
	setpgrp();
#else
	setpgrp(0, getpid());
#endif
	catchsigs(catchabort);
#endif /* !DEBUG */
#ifdef	STRUCT_SIG
	zign.sighandler_el = catchalarm;
	sigact_routine(SIGALRM, &zign, (struct sigstruct_name *) 0);
#ifndef	BUGGY_SIGCLD
	zign.sighandler_el = SIG_IGN;
#ifdef	SA_NOCLDWAIT
	zign.sigflags_el |= SA_NOCLDWAIT;
#endif
	sigact_routine(SIGCLD, &zign, (struct sigstruct_name *) 0);
#endif /* !BUGGY_SIGCLD */
#else  /* !STRUCT_SIG */
	signal(SIGALRM, catchalarm);
#ifndef	BUGGY_SIGCLD
	signal(SIGCLD, SIG_IGN);
#endif /* !BUGGY_SIGCLD */
#endif /* !STRUCT_SIG */

	lognprocess();
	initxbuffer(1);
	send_askall();
	process();
	return  0;		/* Shut up compiler */
}
