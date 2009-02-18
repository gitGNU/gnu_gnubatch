/* xbnet_api.c -- API handling for xbnetserv

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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <time.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include <errno.h>
#include "incl_unix.h"
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
#include "netmsg.h"
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

static	char	Filename[] = __FILE__;

static	int	prodsock = -1;
static	struct	sockaddr_in	apiaddr;
static	struct	sockaddr_in	apiret;

static	char	*current_queue;
static	unsigned	current_qlen;

FILE *net_feed(const int, const netid_t, const jobno_t);

static int  decodejreply()
{
	switch  (readreply())  {
	default:		return  XB_ERR;			/* Dunno what that was */
	case  J_OK:		return	XB_OK;
	case  J_NEXIST:		return  XB_UNKNOWN_JOB;
	case  J_VNEXIST:	return  XB_UNKNOWN_VAR;
	case  J_NOPERM:		return  XB_NOPERM;
	case  J_VNOPERM:	return  XB_NOPERM_VAR;
	case  J_NOPRIV:		return  XB_NOPERM;
	case  J_SYSVAR:		return  XB_BAD_AVAR;
	case  J_SYSVTYPE:	return  XB_BAD_AVAR;
	case  J_FULLUP:		return  XB_NOMEM_QF;
	case  J_ISRUNNING:	return  XB_ISRUNNING;
	case  J_REMVINLOCJ:	return  XB_RVAR_LJOB;
	case  J_LOCVINEXPJ:	return  XB_LVAR_RJOB;
	case  J_MINPRIV:	return  XB_MINPRIV;
	}
}

static int  readvreply()
{
	switch  (readreply())  {
	default:		return  XB_ERR;			/* Dunno what that was */
	case  V_OK:		return  XB_OK;
	case  V_EXISTS:		return  XB_VEXISTS;
	case  V_NEXISTS:	return  XB_UNKNOWN_VAR;
	case  V_CLASHES:	return  XB_VEXISTS;
	case  V_NOPERM:		return  XB_NOPERM;
	case  V_NOPRIV:		return  XB_NOPERM;
	case  V_SYNC:		return  XB_SEQUENCE;
	case  V_SYSVAR:		return  XB_SYSVAR;
	case  V_SYSVTYPE:	return  XB_SYSVTYPE;
	case  V_FULLUP:		return  XB_QFULL;
	case  V_DSYSVAR:	return  XB_DSYSVAR;
	case  V_INUSE:		return  XB_DINUSE;
	case  V_MINPRIV:	return  XB_MINPRIV;
	case  V_DELREMOTE:	return  XB_DELREMOTE;
	case  V_UNKREMUSER:	return  XB_UNKNOWN_USER;
	case  V_UNKREMGRP:	return  XB_UNKNOWN_GROUP;
	case  V_RENEXISTS:	return  XB_VEXISTS;
	case  V_NOTEXPORT:	return  XB_NOTEXPORT;
	case  V_RENAMECLUST:	return  XB_RENAMECLUST;
	}
}

static void  mode_pack(Btmode *dest, const Btmode *src)
{
	dest->o_uid = htonl(src->o_uid);
	dest->o_gid = htonl(src->o_gid);
	strncpy(dest->o_user, src->o_user, UIDSIZE);
	strncpy(dest->o_group, src->o_group, UIDSIZE);
	if  (mpermitted(src, BTM_RDMODE))  {
		dest->c_uid = htonl(src->c_uid);
		dest->c_gid = htonl(src->c_gid);
		strncpy(dest->c_user, src->c_user, UIDSIZE);
		strncpy(dest->c_group, src->c_group, UIDSIZE);
		dest->u_flags = htons(src->u_flags);
		dest->g_flags = htons(src->g_flags);
		dest->o_flags = htons(src->o_flags);
	}
}

/* Send a message regarding a job operation, and decode result.  */

static  int  wjimsg(const unsigned op, const int_ugid_t uid, const int_ugid_t gid, const Btjob *jp, const ULONG param)
{
	int		tries;
	Shipc		Oreq;

	BLOCK_ZERO(&Oreq, sizeof(Oreq));
	Oreq.sh_mtype = TO_SCHED;
	Oreq.sh_params.mcode = op;
	Oreq.sh_params.uuid = uid;
	Oreq.sh_params.ugid = gid;
	mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
	Oreq.sh_params.param = param;
	Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
	Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
	for  (tries = 1;  tries <= MAXTRIES;  tries++)  {
		if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), IPC_NOWAIT) >= 0)
			return  decodejreply();
		if  (tracing & TRACE_SYSOP)  {
			char  msg[16];
			sprintf(msg, "wjimsg-full-%u", tries);
			trace_op(Realuid, msg);
		}
		sleep(10);
	}
	return  XB_QFULL;
}

/* Ditto for job create/update */

static int wjmsg(const unsigned op, const int_ugid_t uid, const int_ugid_t gid, const ULONG xindx)
{
	int		tries;
	Shipc		Oreq;

	BLOCK_ZERO(&Oreq, sizeof(Oreq));
	Oreq.sh_mtype = TO_SCHED;
	Oreq.sh_params.mcode = op;
	Oreq.sh_params.uuid = uid;
	Oreq.sh_params.ugid = gid;
	mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
	Oreq.sh_un.sh_jobindex = xindx;
#ifdef	USING_MMAP
	sync_xfermmap();
#endif
	for  (tries = 1;  tries <= MAXTRIES;  tries++)  {
		if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) >= 0)  {
			int  ret  = decodejreply();
			freexbuf_serv(xindx);
			return  ret;
		}
		if  (tracing & TRACE_SYSOP)  {
			char  msg[16];
			sprintf(msg, "wjmsg-full-%u", tries);
			trace_op(Realuid, msg);
		}
		sleep(10);
	}
	freexbuf_serv(xindx);
	return  XB_QFULL;
}

static int wvmsg(const unsigned op, const int_ugid_t uid, const int_ugid_t gid, const Btvar *varp, const ULONG seq, const ULONG param)
{
	int	tries;
	Shipc	Oreq;

	BLOCK_ZERO(&Oreq, sizeof(Oreq));
	Oreq.sh_mtype = TO_SCHED;
	Oreq.sh_params.mcode = op;
	Oreq.sh_params.param = param;
	Oreq.sh_params.uuid = uid;
	Oreq.sh_params.ugid = gid;
	mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
	Oreq.sh_un.sh_var = *varp;
	Oreq.sh_un.sh_var.var_sequence = seq;
	for  (tries = 1;  tries <= MAXTRIES;  tries++)  {
		if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), IPC_NOWAIT) >= 0)
			return  readvreply();
		if  (tracing & TRACE_SYSOP)  {
			char  msg[16];
			sprintf(msg, "wvmsg-full-%u", tries);
			trace_op(Realuid, msg);
		}
		sleep(10);
	}
	return  XB_QFULL;
}

/* Exit and abort pending jobs */

static void  abort_exit(const int n)
{
	unsigned	cnt;
	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
		struct	pend_job   *pj = &pend_list[cnt];
		if  (pj->out_f)  {
			fclose(pj->out_f);
			pj->out_f = (FILE *) 0;
			unlink(pj->tmpfl);
		}
	}
	exit(n);
}

static void  setup_prod()
{
	BLOCK_ZERO(&apiret, sizeof(apiret));
	apiret.sin_family = AF_INET;
	apiret.sin_addr.s_addr = htonl(INADDR_ANY);
	apiret.sin_port = 0;
	if  ((prodsock = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)
		return;
	if  (bind(prodsock, (struct sockaddr *) &apiret, sizeof(apiret)) < 0)  {
		close(prodsock);
		prodsock = -1;
	}
}

static void  unsetup_prod()
{
	if  (prodsock >= 0)  {
		close(prodsock);
		prodsock = -1;
	}
}

static void  proc_refresh(const netid_t whofrom)
{
	struct	api_msg	outmsg;
	static	ULONG	jser, vser;
	int	prodj = 0, prodv = 0;

	if  (jser != Job_seg.dptr->js_serial)  {
		jser = Job_seg.dptr->js_serial;
		prodj++;
	}
	if  (vser != Var_seg.dptr->vs_serial)  {
		vser = Var_seg.dptr->vs_serial;
		prodv++;
	}

	if  (prodsock < 0  ||  !(prodj || prodv))
		return;

	BLOCK_ZERO(&apiaddr, sizeof(apiaddr));
	apiaddr.sin_family = AF_INET;
	apiaddr.sin_addr.s_addr = whofrom;
	apiaddr.sin_port = apipport;

	if  (prodj)  {
		outmsg.code = API_JOBPROD;
		outmsg.un.r_reader.seq = htonl(jser);
		if  (sendto(prodsock, (char *) &outmsg, sizeof(outmsg), 0, (struct sockaddr *) &apiaddr, sizeof(apiaddr)) < 0)  {
			close(prodsock);
			prodsock = -1;
			return;
		}
	}
	if  (prodv)  {
		outmsg.code = API_VARPROD;
		outmsg.un.r_reader.seq = htonl(vser);
		if  (sendto(prodsock, (char *) &outmsg, sizeof(outmsg), 0, (struct sockaddr *) &apiaddr, sizeof(apiaddr)) < 0)  {
			close(prodsock);
			prodsock = -1;
			return;
		}
	}
}

static void  pushout(const int sock, char *cbufp, unsigned obytes)
{
	int	xbytes;

	while  (obytes != 0)  {
		if  ((xbytes = write(sock, cbufp, obytes)) <= 0)  {
			if  (xbytes < 0  &&  errno == EINTR)
				continue;
			abort_exit(0);
		}
		cbufp += xbytes;
		obytes -= xbytes;
	}
}

static void  pullin(const int sock, char *cbufp, unsigned ibytes)
{
	int	xbytes;

	while  (ibytes != 0)  {
		if  ((xbytes = read(sock, cbufp, ibytes)) <= 0)  {
			if  (xbytes < 0  &&  errno == EINTR)
				continue;
			abort_exit(0);
		}
		cbufp += xbytes;
		ibytes -= xbytes;
	}
}

static void  err_result(const int sock, const int code, const ULONG seq)
{
	struct	api_msg	outmsg;
	outmsg.code = 0;
	outmsg.retcode = htons((SHORT) code);
	outmsg.un.r_reader.seq = htonl(seq);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
}

static void  swapinj(Btjob *to, const struct jobnetmsg *from)
{
	unsigned  cnt;
#ifndef	WORDS_BIGENDIAN
	const	Jarg	*farg;	const	Envir	*fenv;	const	Redir   *fred;
	Jarg	*targ;	Envir	*tenv;	Redir	*tred;
#endif
	BLOCK_ZERO(&to->h, sizeof(Btjobh));

	to->h.bj_hostid = from->hdr.jid.hostid;
	to->h.bj_slotno = ntohl(from->hdr.jid.slotno);

	to->h.bj_progress	= from->hdr.nm_progress;
	to->h.bj_pri		= from->hdr.nm_pri;
	to->h.bj_jflags	= from->hdr.nm_jflags;
	to->h.bj_times.tc_istime= from->hdr.nm_istime;
	to->h.bj_times.tc_mday	= from->hdr.nm_mday;
	to->h.bj_times.tc_repeat= from->hdr.nm_repeat;
	to->h.bj_times.tc_nposs	= from->hdr.nm_nposs;

	to->h.bj_ll			= ntohs(from->hdr.nm_ll);
	to->h.bj_umask			= ntohs(from->hdr.nm_umask);
	strcpy(to->h.bj_cmdinterp, from->hdr.nm_cmdinterp);
	to->h.bj_times.tc_nvaldays	= ntohs(from->hdr.nm_nvaldays);
	to->h.bj_autoksig		= ntohs(from->hdr.nm_autoksig);
	to->h.bj_runon			= ntohs(from->hdr.nm_runon);
	to->h.bj_deltime		= htons(from->hdr.nm_deltime);

	to->h.bj_ulimit			= ntohl(from->hdr.nm_ulimit);
	to->h.bj_times.tc_nexttime	= (time_t) ntohl(from->hdr.nm_nexttime);
	to->h.bj_times.tc_rate		= ntohl(from->hdr.nm_rate);
	to->h.bj_runtime		= ntohl(from->hdr.nm_runtime);

	to->h.bj_exits			= from->hdr.nm_exits;
	to->h.bj_mode.u_flags		= ntohs(from->hdr.nm_mode.u_flags);
	to->h.bj_mode.g_flags		= ntohs(from->hdr.nm_mode.g_flags);
	to->h.bj_mode.o_flags		= ntohs(from->hdr.nm_mode.o_flags);

	for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
		const	Jncond	*drc = &from->hdr.nm_conds[cnt];
		Jcond	*cr = &to->h.bj_conds[cnt];
		if  (drc->bjnc_compar == C_UNUSED)
			break;
		cr->bjc_compar = drc->bjnc_compar;
		cr->bjc_iscrit = drc->bjnc_iscrit;
		cr->bjc_varind = ntohl(drc->bjnc_var.slotno);
		if  ((cr->bjc_value.const_type = drc->bjnc_type) == CON_STRING)
			strncpy(cr->bjc_value.con_un.con_string, drc->bjnc_un.bjnc_string, BTC_VALUE);
		else
			cr->bjc_value.con_un.con_long = ntohl(drc->bjnc_un.bjnc_long);
	}

	for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
		const	Jnass	*dra = &from->hdr.nm_asses[cnt];
		Jass	*cr = &to->h.bj_asses[cnt];
		if  (dra->bjna_op == BJA_NONE)
			break;
		cr->bja_flags = ntohs(dra->bjna_flags);
		cr->bja_op = dra->bjna_op;
		cr->bja_iscrit = dra->bjna_iscrit;
		cr->bja_varind = ntohl(dra->bjna_var.slotno);
		if  ((cr->bja_con.const_type = dra->bjna_type) == CON_STRING)
			strncpy(cr->bja_con.con_un.con_string, dra->bjna_un.bjna_string, BTC_VALUE);
		else
			cr->bja_con.con_un.con_long = ntohl(dra->bjna_un.bjna_long);
	}
	to->h.bj_nredirs	= ntohs(from->nm_nredirs);
	to->h.bj_nargs		= ntohs(from->nm_nargs);
	to->h.bj_nenv		= ntohs(from->nm_nenv);
	to->h.bj_title		= ntohs(from->nm_title);
	to->h.bj_direct		= ntohs(from->nm_direct);
	to->h.bj_redirs		= ntohs(from->nm_redirs);
	to->h.bj_env		= ntohs(from->nm_env);
	to->h.bj_arg		= ntohs(from->nm_arg);
	BLOCK_COPY(to->bj_space, from->nm_space, JOBSPACE);

#ifndef	WORDS_BIGENDIAN

	/* If we are byte-swapping we must swap the argument,
	   environment and redirection variable pointers and the
	   arg field in each redirection.  */

	farg = (const Jarg *) &from->nm_space[to->h.bj_arg];
	fenv = (const Envir *) &from->nm_space[to->h.bj_env];
	fred = (const Redir *) &from->nm_space[to->h.bj_redirs];
	targ = (Jarg *) &to->bj_space[to->h.bj_arg];
	tenv = (Envir *) &to->bj_space[to->h.bj_env];
	tred = (Redir *) &to->bj_space[to->h.bj_redirs];

	for  (cnt = 0;  cnt < to->h.bj_nargs;  cnt++)  {
		*targ++ = ntohs(*farg);
		farg++;	/* Not falling for ntohs being a macro!!! */
	}
	for  (cnt = 0;  cnt < to->h.bj_nenv;  cnt++)  {
		tenv->e_name = ntohs(fenv->e_name);
		tenv->e_value = ntohs(fenv->e_value);
		tenv++;
		fenv++;
	}
	for  (cnt = 0;  cnt < to->h.bj_nredirs; cnt++)  {
		tred->arg = ntohs(fred->arg);
		tred++;
		fred++;
	}
#endif
}

static void  reply_joblist(const int sock, const ULONG flags)
{
	unsigned	jind, njobs;
	slotno_t	*rbuf, *rbufp;
	struct	api_msg	outmsg;

	outmsg.code = API_JOBLIST;
	outmsg.retcode = 0;

	rjobfile(0);
	njobs = 0;

	jind = Job_seg.dptr->js_q_head;
	while  (jind != JOBHASHEND)  {
		HashBtjob	*jhp = &Job_seg.jlist[jind];
		BtjobRef	jp = &jhp->j;

		jind = jhp->q_nxt;

		if  (!mpermitted(&jp->h.bj_mode, BTM_SHOW))
			continue;
		if  (flags & XB_FLAG_LOCALONLY && jp->h.bj_hostid != 0)
			continue;
		if  (flags & XB_FLAG_USERONLY && jp->h.bj_mode.o_uid != Realuid)
			continue;
		if  (flags & XB_FLAG_GROUPONLY && jp->h.bj_mode.o_gid != Realgid)
			continue;
		if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
			const	char	*tit = title_of(jp), *cp;
			if  ((cp = strchr(tit, ':'))  &&
			     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
				continue;
		}
		njobs++;
	}

	outmsg.un.r_lister.nitems = htonl((ULONG) njobs);
	outmsg.un.r_lister.seq = htonl(Job_seg.dptr->js_serial);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (njobs == 0)  {
		junlock();
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "joblist", "OK-none");
		return;
	}
	if  (!(rbuf = (slotno_t *) malloc(njobs * sizeof(slotno_t))))
		ABORT_NOMEM;
	rbufp = rbuf;

	jind = Job_seg.dptr->js_q_head;
	while  (jind != JOBHASHEND)  {
		HashBtjob	*jhp = &Job_seg.jlist[jind];
		BtjobRef	jp = &jhp->j;
		LONG		nind = jind;

		jind = jhp->q_nxt;

		if  (!mpermitted(&jp->h.bj_mode, BTM_SHOW))
			continue;
		if  (flags & XB_FLAG_LOCALONLY && jp->h.bj_hostid != 0)
			continue;
		if  (flags & XB_FLAG_USERONLY && jp->h.bj_mode.o_uid != Realuid)
			continue;
		if  (flags & XB_FLAG_GROUPONLY && jp->h.bj_mode.o_gid != Realgid)
			continue;
		if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
			const	char	*tit = title_of(jp), *cp;
			if  ((cp = strchr(tit, ':'))  &&
			     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
				continue;
		}
		*rbufp++ = htonl(nind);
	}
	junlock();

	/* Splat the thing out */

	pushout(sock, (char *) rbuf, sizeof(slotno_t) * njobs);
	free((char *) rbuf);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "joblist", "OK");
}

static void  reply_varlist(const int sock, const ULONG flags)
{
	struct	Ventry	*vp, *ve;
	unsigned	numvars = 0;
	slotno_t	*rbuf, *rbufp;
	struct	api_msg	outmsg;

	outmsg.code = API_VARLIST;
	outmsg.retcode = 0;
	rvarfile(0);
	ve = &Var_seg.vlist[Var_seg.dptr->vs_maxvars];

	for  (vp = &Var_seg.vlist[0]; vp < ve;  vp++)  {
		Btvar	*varp;
		if  (!vp->Vused)
			continue;
		varp = &vp->Vent;
		if  (!mpermitted(&varp->var_mode, BTM_SHOW))
			continue;
		if  (flags & XB_FLAG_LOCALONLY  &&  varp->var_id.hostid != 0)
			continue;
		if  (flags & XB_FLAG_USERONLY  &&  varp->var_mode.o_uid != Realuid)
			continue;
		if  (flags & XB_FLAG_GROUPONLY  &&  varp->var_mode.o_gid != Realgid)
			continue;
		numvars++;
	}

	outmsg.un.r_lister.nitems = htonl((ULONG) numvars);
	outmsg.un.r_lister.seq = htonl(Var_seg.dptr->vs_serial);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (numvars == 0)  {
		vunlock();
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "varlist", "OK-none");
		return;
	}
	if  (!(rbuf = (slotno_t *) malloc(numvars * sizeof(slotno_t))))
		ABORT_NOMEM;

	rbufp = rbuf;
	for  (vp = &Var_seg.vlist[0]; vp < ve;  vp++)  {
		Btvar	*varp;
		if  (!vp->Vused)
			continue;
		varp = &vp->Vent;
		if  (!mpermitted(&varp->var_mode, BTM_SHOW))
			continue;
		if  (flags & XB_FLAG_LOCALONLY  &&  varp->var_id.hostid != 0)
			continue;
		if  (flags & XB_FLAG_USERONLY  &&  varp->var_mode.o_uid != Realuid)
			continue;
		if  (flags & XB_FLAG_GROUPONLY  &&  varp->var_mode.o_gid != Realgid)
			continue;
		*rbufp++ = htonl((ULONG) (vp - Var_seg.vlist));
	}
	vunlock();

	/* Splat the thing out */

	pushout(sock, (char *) rbuf, sizeof(slotno_t) * numvars);
	free((char *) rbuf);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "varlist", "OK");
}

static int check_valid_job(const int sock, const ULONG flags, const Btjob *jp, const char *tmsg)
{
	if  (jp->h.bj_job == 0  ||
	     !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))  {
		err_result(sock, XB_UNKNOWN_JOB, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, tmsg, "unkjob");
		return  0;
	}
	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))  {
			err_result(sock, XB_UNKNOWN_JOB, Job_seg.dptr->js_serial);
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, tmsg, "unkjob");
			return  0;
		}
	}
	return  1;
}

static void  job_read_rest(const int sock, const Btjob *jp)
{
	int		cnt;
#ifndef	WORDS_BIGENDIAN
	Jarg	*darg;	Envir	*denv;	Redir	*dred;
	const	Jarg	*sarg;	const	Envir	*senv;	const	Redir	*sred;
#endif
	unsigned	hwm = 1;	/* Send one byte in case memcpy etc choke over 0 bytes */
	struct	jobnetmsg	outjob;

	BLOCK_ZERO(&outjob, sizeof(outjob));
	mode_pack(&outjob.hdr.nm_mode, &jp->h.bj_mode);

	/* These things we copy across even if the user can't read the job.
	   M&S fix 28/3/96.  */

	outjob.hdr.jid.hostid = jp->h.bj_hostid == 0? myhostid: jp->h.bj_hostid;
	outjob.hdr.jid.slotno = htonl(jp->h.bj_slotno);

	outjob.hdr.nm_progress	= jp->h.bj_progress;
	outjob.hdr.nm_jflags	= jp->h.bj_jflags;
	outjob.hdr.nm_orighostid= jp->h.bj_orighostid == 0? myhostid: jp->h.bj_orighostid;
	outjob.hdr.nm_runhostid	= jp->h.bj_runhostid == 0? myhostid: jp->h.bj_runhostid;
	outjob.hdr.nm_job	= htonl(jp->h.bj_job);

	if  (mpermitted(&jp->h.bj_mode, BTM_READ))  {
		outjob.hdr.nm_pri	= jp->h.bj_pri;
		outjob.hdr.nm_istime	= jp->h.bj_times.tc_istime;
		outjob.hdr.nm_mday	= jp->h.bj_times.tc_mday;
		outjob.hdr.nm_repeat	= jp->h.bj_times.tc_repeat;
		outjob.hdr.nm_nposs	= jp->h.bj_times.tc_nposs;

		outjob.hdr.nm_ll	= htons(jp->h.bj_ll);
		outjob.hdr.nm_umask	= htons(jp->h.bj_umask);
		strcpy(outjob.hdr.nm_cmdinterp, jp->h.bj_cmdinterp);
		outjob.hdr.nm_nvaldays	= htons(jp->h.bj_times.tc_nvaldays);
		outjob.hdr.nm_autoksig	= htons(jp->h.bj_autoksig);
		outjob.hdr.nm_runon	= htons(jp->h.bj_runon);
		outjob.hdr.nm_deltime	= htons(jp->h.bj_deltime);
		outjob.hdr.nm_lastexit  = htons(jp->h.bj_lastexit);

		outjob.hdr.nm_time	= htonl((LONG) jp->h.bj_time);
		outjob.hdr.nm_stime	= htonl((LONG) jp->h.bj_stime);
		outjob.hdr.nm_etime	= htonl((LONG) jp->h.bj_etime);
		outjob.hdr.nm_pid	= htonl(jp->h.bj_pid);
		outjob.hdr.nm_ulimit	= htonl(jp->h.bj_ulimit);
		outjob.hdr.nm_nexttime	= htonl((LONG) jp->h.bj_times.tc_nexttime);
		outjob.hdr.nm_rate	= htonl(jp->h.bj_times.tc_rate);
		outjob.hdr.nm_runtime	= htonl(jp->h.bj_runtime);

		outjob.hdr.nm_exits	= jp->h.bj_exits;

		for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
			const	Jcond	*drc = &jp->h.bj_conds[cnt];
			Jncond		*cr  = &outjob.hdr.nm_conds[cnt];
			if  (drc->bjc_compar == C_UNUSED)
				break;

			/* Remember that we are putting the slot number on this machine,
			   not the absolute machine id/slot number on that machine pair.
			   This is because the slot number on this machine is the way
			   in which the user sees the result (for example) of xb_varlist.  */

			cr->bjnc_var.slotno = htonl(drc->bjc_varind);
			cr->bjnc_compar = drc->bjc_compar;
			cr->bjnc_iscrit = drc->bjc_iscrit;
			if  ((cr->bjnc_type = drc->bjc_value.const_type) == CON_STRING)
				strncpy(cr->bjnc_un.bjnc_string, drc->bjc_value.con_un.con_string, BTC_VALUE);
			else
				cr->bjnc_un.bjnc_long = ntohl(drc->bjc_value.con_un.con_long);
		}

		for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
			const	Jass	*dra = &jp->h.bj_asses[cnt];
			Jnass	*cr = &outjob.hdr.nm_asses[cnt];
			if  (dra->bja_op == BJA_NONE)
				break;
			cr->bjna_var.slotno = htonl(dra->bja_varind);
			cr->bjna_flags = htons(dra->bja_flags);
			cr->bjna_op = dra->bja_op;
			cr->bjna_iscrit = dra->bja_iscrit;
			if  ((cr->bjna_type = dra->bja_con.const_type) == CON_STRING)
				strncpy(cr->bjna_un.bjna_string, dra->bja_con.con_un.con_string, BTC_VALUE);
			else
				cr->bjna_un.bjna_long = ntohl(dra->bja_con.con_un.con_long);
		}

		outjob.nm_nredirs	= htons(jp->h.bj_nredirs);
		outjob.nm_nargs		= htons(jp->h.bj_nargs);
		outjob.nm_nenv		= htons(jp->h.bj_nenv);
		outjob.nm_title		= htons(jp->h.bj_title);
		outjob.nm_direct	= htons(jp->h.bj_direct);
		outjob.nm_redirs	= htons(jp->h.bj_redirs);
		outjob.nm_env		= htons(jp->h.bj_env);
		outjob.nm_arg		= htons(jp->h.bj_arg);

		BLOCK_COPY(outjob.nm_space, jp->bj_space, JOBSPACE);

		/* To work out the length, we cheat by assuming that packjstring
		   (and its cousins in xb_strings) put the directory and title in
		   last and we can use the offset of that as a high water mark to
		   give us the length.  */

		if  (jp->h.bj_title >= 0)  {
			hwm = jp->h.bj_title;
			hwm += strlen(&jp->bj_space[hwm]) + 1;
		}
		else  if  (jp->h.bj_direct >= 0)  {
			hwm = jp->h.bj_direct;
			hwm += strlen(&jp->bj_space[hwm]) + 1;
		}
		else
			hwm = JOBSPACE;
	}

	hwm += sizeof(struct jobnetmsg) - JOBSPACE;
	outjob.hdr.hdr.length = htons((USHORT) hwm);

#ifndef	WORDS_BIGENDIAN

	/* If we are byte-swapping we must swap the argument,
	   environment and redirection variable pointers and the
	   arg field in each redirection.  */

	darg = (Jarg *) &outjob.nm_space[jp->h.bj_arg]; /* I did mean jp there */
	denv = (Envir *) &outjob.nm_space[jp->h.bj_env];	/* and there */
	dred = (Redir *) &outjob.nm_space[jp->h.bj_redirs]; /* and there */
	sarg = (const Jarg *) &jp->bj_space[jp->h.bj_arg];
	senv = (const Envir *) &jp->bj_space[jp->h.bj_env];
	sred = (const Redir *) &jp->bj_space[jp->h.bj_redirs];

	for  (cnt = 0;  cnt < jp->h.bj_nargs;  cnt++)  {
		*darg++ = htons(*sarg);
		sarg++;	/* Not falling for htons being a macro!!! */
	}
	for  (cnt = 0;  cnt < jp->h.bj_nenv;  cnt++)  {
		denv->e_name = htons(senv->e_name);
		denv->e_value = htons(senv->e_value);
		denv++;
		senv++;
	}
	for  (cnt = 0;  cnt < jp->h.bj_nredirs; cnt++)  {
		dred->arg = htons(sred->arg);
		dred++;
		sred++;
	}
#endif
	pushout(sock, (char *) &outjob, hwm);
}

static void reply_jobread(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btjob			*jp;
	struct	api_msg		outmsg;

	rjobfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		err_result(sock, XB_SEQUENCE, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobread", "seqerr");
		return;
	}
	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		err_result(sock, XB_INVALIDSLOT, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobread", "invslot");
		return;
	}
	jp = &Job_seg.jlist[slotno].j;
	if  (check_valid_job(sock, flags, jp, "jobread"))  {
		outmsg.code = API_JOBREAD;
		outmsg.retcode = 0;
		outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		job_read_rest(sock, jp);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobread", "OK");
	}
}

static  void  reply_jobfind(const int sock, const unsigned code, const jobno_t jn, const netid_t nid, const ULONG flags)
{
	unsigned	jind;
	Btjob		*jp;

	rjobfile(1);
	jind = Job_seg.hashp_jno[jno_jhash(jn)];
	while  (jind != JOBHASHEND)  {
		if  (Job_seg.jlist[jind].j.h.bj_job == jn  &&  Job_seg.jlist[jind].j.h.bj_hostid == nid)
			goto  gotit;
		jind = Job_seg.jlist[jind].nxt_jno_hash;
	}
	err_result(sock, XB_UNKNOWN_JOB, Job_seg.dptr->js_serial);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "findjob", "unkjob");
	return;
 gotit:
	jp = &Job_seg.jlist[jind].j;
	if  (check_valid_job(sock, flags, jp, "jobfind"))  {
		struct	api_msg		outmsg;
		outmsg.code = code;
		outmsg.retcode = 0;
		outmsg.un.r_find.seq = htonl(Job_seg.dptr->js_serial);
		outmsg.un.r_find.slotno = htonl((ULONG) jind);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (code == API_FINDJOB)
			job_read_rest(sock, jp);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "findjob", "OK");
	}
}

static void  var_read_rest(const int sock, const Btvar *varp)
{
	struct	varnetmsg	outvar;

	BLOCK_ZERO(&outvar, sizeof(outvar));
	mode_pack(&outvar.nm_mode, &varp->var_mode);
	outvar.vid.hostid = varp->var_id.hostid == 0? myhostid: varp->var_id.hostid;
	outvar.vid.slotno = htonl(varp->var_id.slotno);
	outvar.nm_type = varp->var_type;
	outvar.nm_flags = varp->var_flags;
	strncpy(outvar.nm_name, varp->var_name, BTV_NAME);
	if  (mpermitted(&varp->var_mode, BTM_READ))  {
		outvar.nm_c_time = htonl(varp->var_c_time);
		strncpy(outvar.nm_comment, varp->var_comment, BTV_COMMENT);
		if  ((outvar.nm_consttype = varp->var_value.const_type) == CON_STRING)
			strncpy(outvar.nm_un.nm_string, varp->var_value.con_un.con_string, BTC_VALUE);
		else
			outvar.nm_un.nm_long = htonl(varp->var_value.con_un.con_long);
	}
	pushout(sock, (char *) &outvar, sizeof(outvar));
}

static void reply_varread(const int sock, const slotno_t slotno, const ULONG	seq, const ULONG flags)
{
	Btvar			*varp;
	struct	api_msg		outmsg;

	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)  {
		err_result(sock, XB_SEQUENCE, Var_seg.dptr->vs_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "varread", "seq");
		return;
	}
	if  (slotno >= Var_seg.dptr->vs_maxvars)  {
		err_result(sock, XB_INVALIDSLOT, Var_seg.dptr->vs_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "varread", "invslot");
		return;
	}

	if  (!Var_seg.vlist[slotno].Vused)  {
		err_result(sock, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "varread", "unkvar");
		return;
	}

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))  {
		err_result(sock, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "varread", "unkvar");
		return;
	}
	outmsg.code = API_VARREAD;
	outmsg.retcode = 0;
	outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	var_read_rest(sock, varp);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "varread", "OK");
}

static  void  reply_varfind(const int sock, const unsigned code, const netid_t nid, const ULONG flags)
{
	vhash_t	vp;
	Btvar	*varp;
	ULONG	Seq;
	char	varname[BTV_NAME+1];
	struct	api_msg		outmsg;

	pullin(sock, varname, sizeof(varname));

	rvarfile(1);

	if  ((vp = lookupvar(varname, nid, BTM_READ, &Seq)) < 0)  {
		err_result(sock, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "findvar", "unkvar");
		return;
	}
	varp = &Var_seg.vlist[vp].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))  {
		err_result(sock, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "findvar", "unkvar");
		return;
	}
	outmsg.code = code;
	outmsg.retcode = 0;
	outmsg.un.r_find.seq = htonl(Var_seg.dptr->vs_serial);
	outmsg.un.r_find.slotno = htonl((LONG) vp);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (code == API_FINDVAR)
		var_read_rest(sock, varp);
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "findvar", "OK");
}

static void reply_jobdel(const int sock, const slotno_t	slotno, const ULONG seq, const ULONG flags)
{
	Btjob	*jp;

	rjobfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		err_result(sock, XB_SEQUENCE, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "seq");
		return;
	}

	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		err_result(sock, XB_INVALIDSLOT, Job_seg.dptr->js_serial);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "invslot");
		return;
	}

	jp = &Job_seg.jlist[slotno].j;
	if  (check_valid_job(sock, flags, jp, "jobdel"))  {
		int	ret;
		struct	api_msg		outmsg;
		if  (!mpermitted(&jp->h.bj_mode, BTM_DELETE))  {
			err_result(sock, XB_NOPERM, Job_seg.dptr->js_serial);
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobdel", "noperm");
			return;
		}
		if  ((ret = wjimsg(J_DELETE, Realuid, Realgid, jp, 0)) != XB_OK)  {
			err_result(sock, ret, Job_seg.dptr->js_serial);
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobdel", "failed");
			return;
		}
		outmsg.code = API_JOBDEL;
		outmsg.retcode = 0;
		outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdel", "OK");
	}
}

static int  reply_vardel(
	 const slotno_t slotno,
	 const ULONG seq,
	 const ULONG flags)
{
	Btvar	*varp;

	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	if  (!mpermitted(&varp->var_mode, BTM_DELETE))
		return  XB_NOPERM;

	return  wvmsg(V_DELETE, Realuid, Realgid, varp, varp->var_sequence, 0);
}

static int reply_jobop(const slotno_t slotno, const ULONG seq, const ULONG flags, const ULONG op, const ULONG param)
{
	Btjob	*jp, *djp;
	ULONG	xindx, nop;

	rjobfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
		return  XB_SEQUENCE;

	if  (slotno >= Job_seg.dptr->js_maxjobs)
		return  XB_INVALIDSLOT;

	jp = &Job_seg.jlist[slotno].j;
	if  (jp->h.bj_job == 0  ||
	     !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
		return  XB_UNKNOWN_JOB;

	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
			return  XB_UNKNOWN_JOB;
	}

	/* We might have to do different messages depending upon
	   whether we are setting progress etc or killing.
	   Likewise we may need to access different permissions.  */

	switch  (op)  {
	default:
		return  XB_UNKNOWN_COMMAND;

	case  XB_JOP_SETRUN:
		nop = BJP_NONE;
		goto  setrest;
	case  XB_JOP_SETCANC:
		nop = BJP_CANCELLED;
		goto  setrest;
	case  XB_JOP_SETDONE:
		nop = BJP_DONE;
	setrest:

		/* Run/cancelled/done - we set the progress code in the job
		   structure.  Maybe we ought to make this a
		   separate shreq code sometime if we do it often enough?  */

		if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE))
			return  XB_NOPERM;
		if  (jp->h.bj_progress == nop) /* Already set to that, go away */
			return  XB_OK;
		if  (jp->h.bj_progress >= BJP_STARTUP1)
			return  XB_ISRUNNING;
		djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
		*djp = *jp;
		djp->h.bj_progress = (unsigned char) nop;
		return  wjmsg(J_CHANGE, Realuid, Realgid, xindx);

	case  XB_JOP_FORCE:
	case  XB_JOP_FORCEADV:
		if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE|BTM_KILL))
			return  XB_NOPERM;
		if  (jp->h.bj_progress >= BJP_STARTUP1)
			return  XB_ISRUNNING;
		return  wjimsg(op == XB_JOP_FORCE? J_FORCENA: J_FORCE, Realuid, Realgid, jp, 0);

	case  XB_JOP_ADVTIME:
		if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE))
			return  XB_NOPERM;
		if  (!jp->h.bj_times.tc_istime)
			return  XB_NOTIMETOA;
		if  (jp->h.bj_progress >= BJP_STARTUP1)
			return  XB_ISRUNNING;
		djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
		*djp = *jp;
		djp->h.bj_times.tc_nexttime = advtime(&djp->h.bj_times);
		return  wjmsg(J_CHANGE, Realuid, Realgid, xindx);

	case  XB_JOP_KILL:
		if  (!mpermitted(&jp->h.bj_mode, BTM_KILL))
			return  XB_NOPERM;
		if  (jp->h.bj_progress < BJP_STARTUP1)
			return  XB_ISNOTRUNNING;
		return  wjimsg(J_KILL, Realuid, Realgid, jp, param);
	}
}

static void api_jobstart(const int sock, struct	hhash *frp, const Btuser *mpriv, const jobno_t jobno)
{
	int			ret;
	unsigned		length;
	netid_t			whofrom = frp->rem.hostid;
	struct	pend_job	*pj;
	struct	api_msg		outmsg;
	struct	jobnetmsg	injob;

	outmsg.code = API_JOBADD;
	outmsg.retcode = 0;

	/* Length is variable as all the space may not be filled up
	   Read it in even if we intend to reject it as the
	   socket will have gunge in it otherwise.  */

	pullin(sock, (char *) &injob.hdr, sizeof(injob.hdr));
	length = ntohs(injob.hdr.hdr.length);
	pullin(sock, sizeof(injob.hdr) + (char *) &injob, length - sizeof(injob.hdr));

	/* If he can't create new entries then he can visit his handy
	   local taxidermist */

	if  (!(mpriv->btu_priv & BTM_CREATE))  {
		outmsg.retcode = htons(XB_NOCRPERM);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobadd", "nocrperm");
		return;
	}

	/* Allocate a pending job structure */

	if  (!(pj = add_pend(whofrom)))  {
		outmsg.retcode = htons(XB_NOMEM_QF);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobadd", "nomem");
		return;
	}

	/* Unpack the job */

	swapinj(&pj->jobout, &injob);
	pj->jobout.h.bj_jflags &= ~(BJ_CLIENTJOB|BJ_ROAMUSER);
	if  (frp->rem.ht_flags & HT_DOS)  {
		pj->jobout.h.bj_jflags |= BJ_CLIENTJOB;
		if  (frp->rem.ht_flags & HT_ROAMUSER)
			pj->jobout.h.bj_jflags |= BJ_ROAMUSER;
	}
	if  (whofrom != myhostid  &&  whofrom != localhostid)
		pj->jobout.h.bj_orighostid = whofrom;

	/* Don't bother with user ids in job modes as they get stuffed
	   in by the scheduler from the shreq structure.  */

	if  (validate_ci(pj->jobout.h.bj_cmdinterp) < 0)  {
		outmsg.retcode = htons(XB_BAD_CI);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		abort_job(pj);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobadd", "badci");
		return;
	}

	if  ((ret = validate_job(&pj->jobout, mpriv)) != 0)  {
		outmsg.retcode = htons((SHORT) XB_CONVERT_XBNR(ret));
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		abort_job(pj);
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobadd", "invjob");
		return;
	}
	pj->jobn = jobno;
	pj->out_f = goutfile(&pj->jobn, pj->tmpfl, 0);
	pj->jobout.h.bj_job = pj->jobn;
	outmsg.un.jobdata.jobno = htonl(pj->jobn);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobadd", "OK");
}

/* Next lump of a job.  */

static void  api_jobcont(const int sock, const jobno_t jobno, const USHORT nbytes)
{
	unsigned		cnt;
	unsigned	char	*bp;
	struct	pend_job	*pj;
	char	inbuffer[XBA_BUFFSIZE];	/* XBA_BUFFSIZE always >= nbytes */

	pullin(sock, inbuffer, nbytes);
	if  (!(pj = find_j_by_jno(jobno)))
		return;
	bp = (unsigned char *) inbuffer;
	for  (cnt = 0;  cnt < nbytes;  cnt++)  {
		if  (putc(*bp, pj->out_f) == EOF)  {
			abort_job(pj);
			return;
		}
		bp++;
	}
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "datain", "OK");
}

/* Final lump of job */

static void  api_jobfinish(const int sock, const jobno_t jobno)
{
	struct	pend_job	*pj;
	struct	api_msg		outmsg;

	outmsg.code = API_DATAEND;
	outmsg.retcode = 0;
	outmsg.un.jobdata.jobno = htonl(jobno);

	if  (!(pj = find_j_by_jno(jobno)))
		outmsg.retcode = htons(XB_UNKNOWN_JOB);
	else  {
		ULONG	xindx;
		Btjob  *djp;
		djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
		BLOCK_COPY(djp, &pj->jobout, sizeof(Btjob));
		djp->h.bj_slotno = -1;
		time(&djp->h.bj_time);
		outmsg.retcode = htons((SHORT) wjmsg(J_CREATE, Realuid, Realgid, xindx));
		fclose(pj->out_f);
		pj->out_f = (FILE *) 0;
		if  (outmsg.retcode != 0)
			unlink(pj->tmpfl);
	}
	outmsg.un.jobdata.seq = htonl(Job_seg.dptr->js_serial);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "dataend", "OK");
}

/* Abort a job.
   I think that we'll be extremely lucky if this routine
   ever gets called as application programs don't have a habit of
   politely telling us 'ere they crash in a spectacular heap,
   well ones I've seen anyhow (even ones I write).  */

static void  api_jobabort(const int sock, const jobno_t jobno)
{
	struct	pend_job	*pj;
	struct	api_msg		outmsg;

	outmsg.code = API_DATAABORT;
	outmsg.retcode = 0;
	outmsg.un.jobdata.jobno = htonl(jobno);

	if  (!(pj = find_j_by_jno(jobno)))
		outmsg.retcode = htons(XB_UNKNOWN_JOB);
	else
		abort_job(pj);
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobabort", "OK");
}

/* Add a groovy new variable */

static int  reply_varadd(const int sock, const Btuser *priv)
{
	struct	varnetmsg	invar;
	Btvar	rvar;

	pullin(sock, (char *) &invar, sizeof(invar));
	BLOCK_ZERO(&rvar, sizeof(rvar));
	rvar.var_type = invar.nm_type;
	rvar.var_flags = invar.nm_flags;
	strncpy(rvar.var_name, invar.nm_name, BTV_NAME);
	strncpy(rvar.var_comment, invar.nm_comment, BTV_COMMENT);
	rvar.var_mode.u_flags = ntohs(invar.nm_mode.u_flags);
	rvar.var_mode.g_flags = ntohs(invar.nm_mode.g_flags);
	rvar.var_mode.o_flags = ntohs(invar.nm_mode.o_flags);
	if  ((rvar.var_value.const_type = invar.nm_consttype) == CON_STRING)
		strncpy(rvar.var_value.con_un.con_string, invar.nm_un.nm_string, BTC_VALUE);
	else
		rvar.var_value.con_un.con_long = ntohl(invar.nm_un.nm_long);

	if  ((priv->btu_priv & BTM_CREATE) == 0)
		return  XBNR_NOCRPERM;

	if  (!(priv->btu_priv & BTM_UMASK)  &&
	     (rvar.var_mode.u_flags != priv->btu_vflags[0] ||
	      rvar.var_mode.g_flags != priv->btu_vflags[1] ||
	      rvar.var_mode.o_flags != priv->btu_vflags[2]))
		return  XBNR_NOCMODE;

	return  wvmsg(V_CREATE, Realuid, Realgid, &rvar, 0, 0);
}

static  int  reply_jobupd(const int sock, const Btuser *mperm, const slotno_t slotno, const ULONG	seq, const ULONG flags)
{
	int		cinum;
	unsigned	length;
	ULONG		xindx;
	Btjob		*jp, *djp;
	struct	jobnetmsg	injob;
	Btjob			rjob;

	/* Length is variable as per jobadd.  */

	pullin(sock, (char *) &injob.hdr, sizeof(injob.hdr));
	length = ntohs(injob.hdr.hdr.length);
	pullin(sock, sizeof(injob.hdr) + (char *) &injob, length - sizeof(injob.hdr));
	swapinj(&rjob, &injob);

	rjobfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Job_seg.dptr->js_maxjobs)
		return  XB_INVALIDSLOT;

	jp = &Job_seg.jlist[slotno].j;
	if  (jp->h.bj_job == 0  ||  !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
		return  XB_UNKNOWN_JOB;

	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
			return  XB_UNKNOWN_JOB;
	}

	if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE))
		return  XB_NOPERM;

	if  (jp->h.bj_progress >= BJP_STARTUP1)
		return  XB_ISRUNNING;

	/* The scheduler ignores attempts to change the mode and
	   ownership etc so we won't bother to look.  We will
	   validate the priority and load levels though.  */

	if  (rjob.h.bj_pri < mperm->btu_minp  ||  rjob.h.bj_pri > mperm->btu_maxp)
		return  XB_BAD_PRIORITY;

	if  ((cinum = validate_ci(rjob.h.bj_cmdinterp)) < 0)
		return  XB_BAD_CI;

	/* Validate load level */

	if  (rjob.h.bj_ll == 0  ||  rjob.h.bj_ll > mperm->btu_maxll)
		return  XBNR_BAD_LL;
	if  (!(mperm->btu_priv & BTM_SPCREATE) && rjob.h.bj_ll != Ci_list[cinum].ci_ll)
		return  XBNR_BAD_LL;

	/* Copy across bits which the scheduler uses to identify the job */

	rjob.h.bj_slotno = jp->h.bj_slotno;
	rjob.h.bj_hostid = jp->h.bj_hostid;
	rjob.h.bj_job = jp->h.bj_job;

	djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
	BLOCK_COPY(djp, &rjob, sizeof(Btjob));
	return  wjmsg(J_CHANGE, Realuid, Realgid, xindx);
}

static int reply_varupd(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	unsigned	wflags = BTM_WRITE;
	Btvar	*varp, rvar;
	struct	varnetmsg	invar;
	ULONG		Saveseq;

	pullin(sock, (char *) &invar, sizeof(invar));
	BLOCK_ZERO(&rvar, sizeof(rvar));
	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;

	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;

	rvar = *varp;
	rvar.var_flags = invar.nm_flags & (VF_EXPORT|VF_CLUSTER);
	if  ((rvar.var_value.const_type = invar.nm_consttype) == CON_STRING)
		strncpy(rvar.var_value.con_un.con_string, invar.nm_un.nm_string, BTC_VALUE);
	else
		rvar.var_value.con_un.con_long = ntohl(invar.nm_un.nm_long);

	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	if  ((varp->var_flags ^ rvar.var_flags) & (VF_EXPORT|VF_CLUSTER))
		wflags |= BTM_DELETE;
	if  (rvar.var_value.const_type == varp->var_value.const_type)  {
		if  (rvar.var_value.const_type == CON_LONG)  {
			if  (rvar.var_value.con_un.con_long == varp->var_value.con_un.con_long)
				wflags &= ~BTM_WRITE;
		}
		else  if  (strcmp(rvar.var_value.con_un.con_string, varp->var_value.con_un.con_string) == 0)
			wflags &= ~BTM_WRITE;
	}

	if  (wflags == 0)	/* Nothing doing forget it */
		return  XB_OK;

	if  (!mpermitted(&varp->var_mode, wflags))
		return  XB_NOPERM;

	Saveseq = varp->var_sequence;
	if  (wflags & BTM_DELETE)  {
		int	ret = wvmsg(V_CHFLAGS, Realuid, Realgid, &rvar, Saveseq, 0);
		if  (ret != XB_OK)
			return  ret;
		Saveseq++;
	}
	if  (wflags & BTM_WRITE)
		return  wvmsg(V_ASSIGN, Realuid, Realgid, &rvar, Saveseq, 0);
	return  XB_OK;
}

static int reply_varchcomm(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btvar	*varp, rvar;
	struct	varnetmsg	invar;

	pullin(sock, (char *) &invar, sizeof(invar));
	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	if  (!mpermitted(&varp->var_mode, BTM_WRITE))
		return  XB_NOPERM;

	rvar = *varp;
	strcpy(rvar.var_comment, invar.nm_comment);
	return  wvmsg(V_CHCOMM, Realuid, Realgid, &rvar, varp->var_sequence, 0);
}

static int reply_varrename(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btvar	*varp;
	char	nbuf[BTV_NAME+1];
	Shipc	Oreq;

	pullin(sock, (char *) nbuf, sizeof(nbuf));
	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	if  (!mpermitted(&varp->var_mode, BTM_DELETE))
		return  XB_NOPERM;

	BLOCK_ZERO(&Oreq, sizeof(Oreq));
	Oreq.sh_mtype = TO_SCHED;
	Oreq.sh_params.mcode = V_NEWNAME;
	Oreq.sh_params.uuid = Realuid;
	Oreq.sh_params.ugid = Realgid;
	mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
	Oreq.sh_un.sh_rn.sh_ovar = *varp;
	Oreq.sh_un.sh_rn.sh_ovar.var_sequence = varp->var_sequence;
	strcpy(Oreq.sh_un.sh_rn.sh_rnewname, nbuf);
	msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar) + strlen(nbuf) + 1, 0);
	return  readvreply();
}

static void api_jobdata(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	int	inbp, ch;
	FILE	*jfile;
	Btjob	*jp;
	struct	api_msg	outmsg;
	char	buffer[XBA_BUFFSIZE];

	outmsg.code = API_JOBDATA;
	outmsg.retcode = 0;

	rjobfile(1);

	outmsg.un.jobdata.seq = htonl(Job_seg.dptr->js_serial);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
		outmsg.retcode = htons(XB_SEQUENCE);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "seq");
		return;
	}
	if  (slotno >= Job_seg.dptr->js_maxjobs)  {
		outmsg.retcode = htons(XB_INVALIDSLOT);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "invslot");
		return;
	}

	jp = &Job_seg.jlist[slotno].j;

	if  (jp->h.bj_job == 0  ||  !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))  {
		outmsg.retcode = htons(XB_UNKNOWN_JOB);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "unkjob");
		return;
	}

	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))  {
			outmsg.retcode = htons(XB_UNKNOWN_JOB);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "jobdata", "unkjob");
			return;
		}
	}

	if  (!mpermitted(&jp->h.bj_mode, BTM_READ))  {
		outmsg.retcode = htons(XB_NOPERM);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "noperm");
		return;
	}

	jfile = jp->h.bj_hostid? net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job): fopen(mkspid(SPNAM, jp->h.bj_job), "r");
	if  (!jfile)  {
		outmsg.retcode = htons(XB_UNKNOWN_JOB);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "jobdata", "unkjob");
		return;
	}

	/* Say ok */

	pushout(sock, (char *) &outmsg, sizeof(outmsg));

	/* Read the file and splat it out.  */

	outmsg.code = API_DATAOUT;
	outmsg.un.jobdata.jobno = htonl(jp->h.bj_job);
	inbp = 0;

	while  ((ch = getc(jfile)) != EOF)  {
		buffer[inbp++] = (char) ch;
		if  (inbp >= sizeof(buffer))  {
			outmsg.un.jobdata.nbytes = htons(inbp);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pushout(sock, buffer, inbp);
			inbp = 0;
		}
	}

	fclose(jfile);
	if  (inbp > 0)  {
		outmsg.un.jobdata.nbytes = htons(inbp);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		pushout(sock, buffer, inbp);
		inbp = 0;
	}

	/* Mark end of data */

	outmsg.code = API_DATAEND;
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
	if  (tracing & TRACE_APIOPEND)
		trace_op_res(Realuid, "jobdata", "OK");
}

static int reply_jobchmod(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btjob		*jp, *djp;
	ULONG		xindx;
	struct	jobhnetmsg	injob;

	pullin(sock, (char *) &injob, sizeof(injob));

	rjobfile(1);
	/* We didn't forget tracing here we do it in the main routine */
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
		return  XB_SEQUENCE;

	if  (slotno >= Job_seg.dptr->js_maxjobs)
		return  XB_INVALIDSLOT;

	jp = &Job_seg.jlist[slotno].j;
	if  (jp->h.bj_job == 0  ||
	     !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
		return  XB_UNKNOWN_JOB;

	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
			return  XB_UNKNOWN_JOB;
	}

	if  (!mpermitted(&jp->h.bj_mode, BTM_WRMODE))
		return  XB_NOPERM;

	djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
	*djp = *jp;
	djp->h.bj_mode.u_flags = ntohs(injob.nm_mode.u_flags);
	djp->h.bj_mode.g_flags = ntohs(injob.nm_mode.g_flags);
	djp->h.bj_mode.o_flags = ntohs(injob.nm_mode.o_flags);
	return  wjmsg(J_CHMOD, Realuid, Realgid, xindx);
}

static int reply_varchmod(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btvar	*varp, rvar;
	struct	varnetmsg	invar;

	pullin(sock, (char *) &invar, sizeof(invar));

	rvarfile(1);
	/* We didn't forget tracing here we do it in the main routine */
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	if  (!mpermitted(&varp->var_mode, BTM_WRMODE))
		return  XB_NOPERM;

	rvar = *varp;
	rvar.var_mode.u_flags = ntohs(invar.nm_mode.u_flags);
	rvar.var_mode.g_flags = ntohs(invar.nm_mode.g_flags);
	rvar.var_mode.o_flags = ntohs(invar.nm_mode.o_flags);
	return  wvmsg(V_CHMOD, Realuid, Realgid, &rvar, rvar.var_sequence, 0);
}

static int reply_jobchown(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btjob		*jp;
	int_ugid_t	nuid;
	struct	jugmsg	injob;

	pullin(sock, (char *) &injob, sizeof(injob));

	/* We didn't forget tracing here we do it in the main routine */
	if  ((nuid = lookup_uname(injob.newug)) == UNKNOWN_UID)
		return  XB_UNKNOWN_USER;
	rjobfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
		return  XB_SEQUENCE;

	if  (slotno >= Job_seg.dptr->js_maxjobs)
		return  XB_INVALIDSLOT;

	jp = &Job_seg.jlist[slotno].j;
	if  (jp->h.bj_job == 0  ||
	     !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
		return  XB_UNKNOWN_JOB;

	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
			return  XB_UNKNOWN_JOB;
	}

	return  wjimsg(J_CHOWN, Realuid, Realgid, jp, (ULONG) nuid);
}

static int reply_varchown(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btvar		*varp;
	int_ugid_t	nuid;
	struct	vugmsg	invar;

	pullin(sock, (char *) &invar, sizeof(invar));
	/* We didn't forget tracing here we do it in the main routine */
	if  ((nuid = lookup_uname(invar.newug)) == UNKNOWN_UID)
		return  XB_UNKNOWN_USER;

	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	return  wvmsg(V_CHOWN, Realuid, Realgid, varp, varp->var_sequence, (ULONG) nuid);
}

static int reply_jobchgrp(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btjob		*jp;
	int_ugid_t	ngid;
	struct	jugmsg	injob;

	pullin(sock, (char *) &injob, sizeof(injob));

	/* We didn't forget tracing here we do it in the main routine */
	if  ((ngid = lookup_gname(injob.newug)) == UNKNOWN_GID)
		return  XB_UNKNOWN_GROUP;
	rjobfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
		return  XB_SEQUENCE;

	if  (slotno >= Job_seg.dptr->js_maxjobs)
		return  XB_INVALIDSLOT;

	jp = &Job_seg.jlist[slotno].j;
	if  (jp->h.bj_job == 0  ||
	     !mpermitted(&jp->h.bj_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
	     ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
		return  XB_UNKNOWN_JOB;

	if  (flags & XB_FLAG_QUEUEONLY && current_queue)  {
		const	char	*tit = title_of(jp), *cp;
		if  ((cp = strchr(tit, ':'))  &&
		     ((cp - tit) != current_qlen || strncmp(tit, current_queue, current_qlen) != 0))
			return  XB_UNKNOWN_JOB;
	}

	return  wjimsg(J_CHGRP, Realuid, Realgid, jp, (ULONG) ngid);
}

static int reply_varchgrp(const int sock, const slotno_t slotno, const ULONG seq, const ULONG flags)
{
	Btvar		*varp;
	int_ugid_t	ngid;
	struct	vugmsg	invar;

	pullin(sock, (char *) &invar, sizeof(invar));
	/* We didn't forget tracing here we do it in the main routine */
	if  ((ngid = lookup_gname(invar.newug)) == UNKNOWN_GID)
		return  XB_UNKNOWN_GROUP;

	rvarfile(1);
	if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
		return  XB_SEQUENCE;
	if  (slotno >= Var_seg.dptr->vs_maxvars)
		return  XB_INVALIDSLOT;
	if  (!Var_seg.vlist[slotno].Vused)
		return  XB_UNKNOWN_VAR;

	varp = &Var_seg.vlist[slotno].Vent;
	if  (!mpermitted(&varp->var_mode, BTM_SHOW)  ||
	     ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
	     ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
	     ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
		return  XB_UNKNOWN_VAR;

	return  wvmsg(V_CHGRP, Realuid, Realgid, varp, varp->var_sequence, (ULONG) ngid);
}

static int  reply_ciadd(const int sock, const Btuser *mpriv, unsigned *res)
{
	unsigned	nsel;
	Cmdint		rci, inci;

	pullin(sock, (char *) &inci, sizeof(inci));
	/* We didn't forget tracing here we do it in the main routine */
	if  (!(mpriv->btu_priv & BTM_SPCREATE))
		return  XB_NOPERM;
	BLOCK_ZERO(&rci, sizeof(rci));
	rci.ci_ll = ntohs(inci.ci_ll);
	rci.ci_nice = inci.ci_nice;
	rci.ci_flags = inci.ci_flags;
	strncpy(rci.ci_name, inci.ci_name, CI_MAXNAME);
	strncpy(rci.ci_path, inci.ci_path, CI_MAXFPATH);
	strncpy(rci.ci_args, inci.ci_args, CI_MAXARGS);
	if  (rci.ci_name[0] == '\0' || rci.ci_path[0] == '\0')
		return  XB_BAD_CI;

	if  (rci.ci_ll == 0)
		rci.ci_ll = mpriv->btu_spec_ll;

	if  (validate_ci(rci.ci_name) == 0)
		return  XB_BAD_CI;
	for  (nsel = 0;  nsel < Ci_num;  nsel++)
		if  (Ci_list[nsel].ci_name[0] == '\0')
			goto  dun;
	Ci_num++;
	if  (!(Ci_list = (CmdintRef) realloc((char *) Ci_list, (unsigned) (Ci_num * sizeof(Cmdint)))))
		ABORT_NOMEM;
 dun:
	lseek(Ci_fd, (long) (nsel * sizeof(Cmdint)), 0);
	write(Ci_fd, (char *) &rci, sizeof(rci));
	Ci_list[nsel] = rci;
	*res = nsel;
	return  XB_OK;
}

#ifndef WORDS_BIGENDIAN
#define	CIBLOCKSIZE	10
#endif

static void  reply_ciread(const int sock)
{
	struct	api_msg	outmsg;

	open_ci(O_RDWR);
	outmsg.code = API_CIREAD;
	outmsg.retcode = XB_OK;
	outmsg.un.r_lister.nitems = htonl((ULONG) Ci_num);
	outmsg.un.r_lister.seq = 0;
	pushout(sock, (char *) &outmsg, sizeof(outmsg));
#ifdef WORDS_BIGENDIAN
	if  (Ci_num > 0)
		pushout(sock, (char *) Ci_list, (unsigned) (Ci_num * sizeof(Cmdint)));
#else
	if  (Ci_num > 0)  {
		Cmdint	ciblk[CIBLOCKSIZE];
		Cmdint	*fp = Ci_list, *tp;
		Cmdint	*fe = &Ci_list[Ci_num], *te = &ciblk[CIBLOCKSIZE];
		do  {
			tp = ciblk;
			do  {
				*tp = *fp;
				tp->ci_ll = htons(fp->ci_ll);
				tp++;	fp++;
			}  while  (fp < fe  &&  tp < te);

			pushout(sock, (char *) ciblk, (char *) tp - (char *)ciblk);
		}  while  (fp < fe);
	}
#endif
}

static int  reply_ciupd(const int sock, const Btuser *mpriv, const unsigned slot)
{
	unsigned	cnt;
	Cmdint	rci, inci;

	pullin(sock, (char *) &inci, sizeof(inci));
	/* We didn't forget tracing here we do it in the main routine */
	if  (!(mpriv->btu_priv & BTM_SPCREATE))
		return  XB_NOPERM;
	BLOCK_ZERO(&rci, sizeof(rci));
	rci.ci_ll = ntohs(inci.ci_ll);
	rci.ci_nice = inci.ci_nice;
	rci.ci_flags = inci.ci_flags;
	strncpy(rci.ci_name, inci.ci_name, CI_MAXNAME);
	strncpy(rci.ci_path, inci.ci_path, CI_MAXFPATH);
	strncpy(rci.ci_args, inci.ci_args, CI_MAXARGS);

	if  (rci.ci_name[0] == '\0' || rci.ci_path[0] == '\0'  || slot >= Ci_num  ||  Ci_list[slot].ci_name[0] == '\0')
		return  XB_BAD_CI;

	rereadcif();

	for  (cnt = 0;  cnt < Ci_num;  cnt++)
		if  (cnt != slot  &&  strcmp(Ci_list[cnt].ci_name, rci.ci_name) == 0)
			return  XB_BAD_CI;

	if  (rci.ci_ll == 0)
		rci.ci_ll = mpriv->btu_spec_ll;

	lseek(Ci_fd, (long) (slot * sizeof(Cmdint)), 0);
	write(Ci_fd, (char *) &rci, sizeof(rci));
	Ci_list[slot] = rci;
	return  XB_OK;
}

static int  reply_cidel(const Btuser *mpriv, const unsigned slot)
{
	if  (!(mpriv->btu_priv & BTM_SPCREATE))
		return  XB_NOPERM;

	rereadcif();
	if  (slot == CI_STDSHELL  ||  slot >= Ci_num  ||  Ci_list[slot].ci_name[0] == '\0')
		return  XB_BAD_CI;
	Ci_list[slot].ci_name[0] = '\0';
	lseek(Ci_fd, (long) (slot * sizeof(Cmdint)), 0);
	write(Ci_fd, (char *) &Ci_list[slot], sizeof(Cmdint));
	return  XB_OK;
}

static void  reply_holread(const int sock, const unsigned year)
{
	struct	api_msg	outmsg;
	char	rep[YVECSIZE];

	BLOCK_ZERO(rep, sizeof(rep));
	outmsg.retcode = XB_OK;
	if  (year >= 1990  &&  year < 2100)  {
		get_hf(year-1990, rep);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		pushout(sock, rep, sizeof(rep));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "holread", "OK");
	}
	else  {
		outmsg.retcode = htons(XB_INVALID_YEAR);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, "holread", "invyear");
	}
}

static int  reply_holupd(const int sock, const unsigned year)
{
	char	inyear[YVECSIZE];
	pullin(sock, inyear, YVECSIZE);
	if  (year < 1990  ||  year >= 2100)
		return  XB_INVALID_YEAR;
	put_hf(year-1990, inyear);
	return  XB_OK;
}

static int  reply_setqueue(const int sock, const unsigned length)
{
	if  (length == 0)  {
		if  (current_queue)  {
			free(current_queue);
			current_queue = (char *) 0;
		}
		current_qlen = 0;
	}
	else  {
		char  *newqueue = malloc(length); /* Includes final null */
		if  (!newqueue)  {
			unsigned  nbytes = length;
			char	stuff[100];

			/* Slurp up stuff from socket */

			while  (nbytes != 0)  {
				int	nb = nbytes < sizeof(stuff)? nbytes: sizeof(stuff);
				pullin(sock, stuff, (unsigned) nb);
				nbytes -= nb;
			}
			return  XB_NOMEMQ;
		}
		pullin(sock, newqueue, length);
		if  (current_queue)
			free(current_queue);
		current_queue = newqueue;
		current_qlen = length - 1;
	}
	return  XB_OK;
}

void  process_api()
{
	int		sock, inbytes, cnt, ret;
	PIDTYPE		pid;
	netid_t		whofrom;
	struct	hhash	*frp;
	Btuser		*mpriv;
	Btuser		hispriv;
	struct	api_msg	inmsg;
	struct	api_msg	outmsg;
	const	char	*tcode = "";

	if  ((sock = tcp_serv_accept(apirsock, &whofrom)) < 0)
		return;

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
	/* Make the process the grandchild so we don't have to worry
	   about waiting for it later.  */

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

	/* We are now a separate process...
	   Clean up irrelevant stuff to do with pending UDP jobs.
	   See what the guy wants.
	   At this stage we only want to "login".  */

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
		struct  pend_job  *pj = &pend_list[cnt];
		if  (pj->out_f)  {
			fclose(pj->out_f);
			pj->out_f = (FILE *) 0;
		}
		pj->clientfrom = 0;
	}

	while  ((inbytes = read(sock, (char *) &inmsg, sizeof(inmsg))) != sizeof(inmsg))  {
		if  (inbytes >= 0  ||  errno != EINTR)
			abort_exit(0);
		while  (hadrfresh)  {
			hadrfresh = 0;
			proc_refresh(whofrom);
		}
	}

	frp = find_remote(whofrom);

	/* If this is from a DOS user reject it if he hasn't logged
	   in.  Support "roaming" client users, by a login
	   protocol.  This is only available from clients.  */

	if  (inmsg.code == API_LOGIN)  {
		if  (frp)  {
			if  (!(frp->rem.ht_flags & HT_DOS))  {
				if  (tracing & TRACE_APICONN)
					trace_op(ROOTID, "api-badlogin");
				err_result(sock, XB_UNKNOWN_COMMAND, 0);
				abort_exit(0);
			}
			if  (frp->rem.ht_flags & HT_ROAMUSER)  {
				if  (strcmp(frp->dosname, inmsg.un.signon.username) != 0)  { /* Name change */
					struct	cluhash	 *cp;
					if  (!(cp = update_roam_name(frp, inmsg.un.signon.username)))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-loginr-unkuser", inmsg.un.signon.username);
						err_result(sock, XB_UNKNOWN_USER, 0);
						abort_exit(0);
					}

					/* Set UAL_NOK if we need a password or if there is a default machine name
					   which is not the same one as we are talking about. The code below will
					   check the password */

					frp->flags = (cp->rem.ht_flags & HT_PWCHECK  ||
						      (cp->machname  &&  look_hostname(cp->machname) != whofrom)) ? UAL_NOK: UAL_OK;
				}
			}
			else  {
				/* Non roam case we've seen the machine but we need to check the user name makes sense */

				if  (ncstrcmp(frp->actname, inmsg.un.signon.username) != 0)  {
					if  (!update_nonroam_name(frp, inmsg.un.signon.username))  {
						err_result(sock, XB_UNKNOWN_USER, 0);
						abort_exit(0);
					}
					frp->flags = (frp->rem.ht_flags & HT_PWCHECK  ||
						      (frp->dosname[0]  &&  ncstrcmp(frp->dosname, frp->actname) != 0))? UAL_NOK: UAL_OK;
				}
			}
		}
		else  {
			struct	cluhash  *cp;

			if  (!(cp = new_roam_name(whofrom, &frp, inmsg.un.signon.username)))  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "api-login-um-unkuser", inmsg.un.signon.username);
				err_result(sock, XB_UNKNOWN_USER, 0);
				abort_exit(0);
			}
			frp->flags = (cp->rem.ht_flags & HT_PWCHECK  ||
				      (cp->machname  &&  look_hostname(cp->machname) != whofrom)) ? UAL_NOK: UAL_OK;
		}

		/* Now for any password-checking.  */

		outmsg.code = inmsg.code;
		strncpy(outmsg.un.signon.username, frp->actname, UIDSIZE);

		if  (frp->flags != UAL_OK)  {
			char	pwbuf[API_PASSWDSIZE+1];
			outmsg.retcode = htons(XB_NO_PASSWD);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pullin(sock, pwbuf, sizeof(pwbuf));
			frp->flags = checkpw(frp->actname, pwbuf)? UAL_OK: UAL_INVP;
			if  (frp->flags != UAL_OK)  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "Loginfail", frp->actname);
				err_result(sock, XB_PASSWD_INVALID, 0);
				abort_exit(0);
			}
		}
		outmsg.retcode = XB_OK;
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (inmsg.re_reg  &&  frp->rem.ht_flags & HT_ROAMUSER)
			tell_myself(frp);
	}
	else  {			/* Not starting with login */
		if  (inmsg.code != API_SIGNON)  {
			if  (tracing & TRACE_APICONN)
				trace_op(ROOTID, "Loginseq");
			err_result(sock, XB_SEQUENCE, 0);
			abort_exit(0);
		}
		if  (!frp)  {
			if  (tracing & TRACE_APICONN)
				trace_op(ROOTID, "Signonunku");
			err_result(sock, XB_UNKNOWN_USER, 0);
			abort_exit(0);
		}
		if  (frp->rem.ht_flags & HT_DOS)  {

			/* Possibly let a roaming user change his name.  */

			if  (frp->rem.ht_flags & HT_ROAMUSER)  {
				if  (ncstrcmp(inmsg.un.signon.username, frp->actname) != 0)  {
					struct	cluhash		*cp;
					if  (!(cp = update_roam_name(frp, inmsg.un.signon.username)))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-signon-unkuser", inmsg.un.signon.username);
						err_result(sock, XB_NOT_USER, 0);
						abort_exit(0);
					}
					/* We didn't forget to tell_friends here - not a login, just a different user */
					frp->flags = cp->rem.ht_flags & HT_PWCHECK? UAL_NOK: UAL_OK;
				}
			}
			else  {
				if  (ncstrcmp(frp->actname, inmsg.un.signon.username) != 0)  {
					if  (!update_nonroam_name(frp, inmsg.un.signon.username))  {
						if  (tracing & TRACE_APICONN)
							trace_op_res(ROOTID, "api-signonnr-unkuser", inmsg.un.signon.username);
						err_result(sock, XB_NOT_USER, 0);
						abort_exit(0);
					}
					frp->flags = (frp->rem.ht_flags & HT_PWCHECK  ||
						      (frp->dosname[0]  &&
						       ncstrcmp(frp->dosname, frp->actname) != 0))? UAL_NOK : UAL_OK;
				}
			}

			if  (frp->flags != UAL_OK)  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "Signonfail", frp->actname);
				err_result(sock, frp->flags == UAL_NOK? XB_NO_PASSWD: frp->flags == UAL_INVU? XB_UNKNOWN_USER: XB_PASSWD_INVALID, 0);
				abort_exit(0);
			}
		}
		else  {
			int_ugid_t	whofuser;

			if  ((whofuser = lookup_uname(inmsg.un.signon.username)) == UNKNOWN_UID)  {
				if  (tracing & TRACE_APICONN)
					trace_op_res(ROOTID, "Signonfail", inmsg.un.signon.username);
				err_result(sock, XB_UNKNOWN_USER, 0);
				abort_exit(0);
			}
			frp->rem.n_uid = whofuser;
			frp->rem.n_gid = lastgid;
		}
	}

	Realuid = frp->rem.n_uid;
	Realgid = frp->rem.n_gid;
	if  (tracing & TRACE_APICONN)
		trace_op_res(Realuid, "Login-OK", frp->rem.hostname);

	/* Need to make a copy as subsequent calls overwrite static
	   space in getbtuentry */

	if  (!(mpriv = getbtuentry(Realuid)))  {
		if  (tracing & TRACE_APICONN)
			trace_op_res(Realuid, "unkuid", frp->rem.hostname);
		err_result(sock, XB_UNKNOWN_USER, 0);
		abort_exit(0);
	}
	Fileprivs = mpriv->btu_priv;
	hispriv = *mpriv;

	/* Ok we made it */

	outmsg.code = inmsg.code;
	outmsg.retcode = 0;
	pushout(sock, (char *) &outmsg, sizeof(outmsg));

	/* So do main loop waiting for something to happen */

 restart:
	for  (;;)  {
		while  (hadrfresh)  {
			hadrfresh = 0;
			proc_refresh(whofrom);
		}
		if  ((inbytes = read(sock, (char *) &inmsg, sizeof(inmsg))) != sizeof(inmsg))  {
			if  (inbytes >= 0  ||  errno != EINTR)
				abort_exit(0);
			goto  restart;
		}
		switch  (outmsg.code = inmsg.code)  {
		default:
			ret = XB_UNKNOWN_COMMAND;
			break;

		case  API_SIGNOFF:
			if  (tracing & TRACE_APICONN)
				trace_op(Realuid, "logoff");
			abort_exit(0);

		case  API_NEWGRP:
		{
			int_ugid_t	ngid;

			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "newgrp");

			if  (!(hispriv.btu_priv & BTM_WADMIN)  &&  !chk_vgroup(Realuid, inmsg.un.signon.username))  {
				ret = XB_BAD_GROUP;
				if  (tracing & TRACE_APIOPEND)
					trace_op_res(Realuid, "newgrp", "badgrp");
				break;
			}
			if  ((ngid = lookup_gname(inmsg.un.signon.username)) == UNKNOWN_GID)  {
				ret = XB_UNKNOWN_GROUP;
				if  (tracing & TRACE_APIOPEND)
					trace_op_res(Realuid, "newgrp", "unkgrp");
				break;
			}
			Realgid = frp->rem.n_gid = ngid;
			ret = XB_OK;
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "newgrp", "OK");
			break;
		}

		case  API_JOBLIST:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "joblist");
			reply_joblist(sock, ntohl(inmsg.un.lister.flags));
			continue;

		case  API_VARLIST:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "varlist");
			reply_varlist(sock, ntohl(inmsg.un.lister.flags));
			continue;

		case  API_JOBREAD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobread");
			reply_jobread(sock,
				      ntohl(inmsg.un.reader.slotno),
				      ntohl(inmsg.un.reader.seq),
				      ntohl(inmsg.un.reader.flags));
			continue;

		case  API_FINDJOBSLOT:
		case  API_FINDJOB:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "findjob");
			reply_jobfind(sock,
				      inmsg.code,
				      ntohl(inmsg.un.jobfind.jobno),
				      inmsg.un.jobfind.netid == myhostid? 0: inmsg.un.jobfind.netid,
				      ntohl(inmsg.un.jobfind.flags));
			continue;

		case  API_VARREAD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "varread");
			reply_varread(sock,
				      ntohl(inmsg.un.reader.slotno),
				      ntohl(inmsg.un.reader.seq),
				      ntohl(inmsg.un.reader.flags));
			continue;

		case  API_FINDVARSLOT:
		case  API_FINDVAR:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "findvar");
			reply_varfind(sock,
				      inmsg.code,
				      inmsg.un.varfind.netid == myhostid? 0: inmsg.un.varfind.netid,
				      ntohl(inmsg.un.varfind.flags));
			continue;

		case  API_JOBDEL:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobdel");
			reply_jobdel(sock,
				     ntohl(inmsg.un.reader.slotno),
				     ntohl(inmsg.un.reader.seq),
				     ntohl(inmsg.un.reader.flags));
			continue;

		case  API_VARDEL:
			tcode = "vardel";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_vardel(ntohl(inmsg.un.reader.slotno),
					   ntohl(inmsg.un.reader.seq),
					   ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_JOBOP:
			tcode = "jobop";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_jobop(ntohl(inmsg.un.jop.slotno),
					  ntohl(inmsg.un.jop.seq),
					  ntohl(inmsg.un.jop.flags),
					  ntohl(inmsg.un.jop.op),
					  ntohl(inmsg.un.jop.param));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_JOBADD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobadd");
			api_jobstart(sock, frp, &hispriv, ntohl(inmsg.un.jobdata.jobno));
			continue;
		case  API_DATAIN:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "datain");
			api_jobcont(sock, ntohl(inmsg.un.jobdata.jobno), ntohs(inmsg.un.jobdata.nbytes));
			continue;
		case  API_DATAEND:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "dataend");
			api_jobfinish(sock, ntohl(inmsg.un.jobdata.jobno));
			continue;
		case  API_DATAABORT: /* We'll be lucky if we get this */
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "dataabort");
			api_jobabort(sock, ntohl(inmsg.un.jobdata.jobno));
			continue;

		case  API_VARADD:
			tcode = "varadd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varadd(sock, &hispriv);
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_JOBUPD:
			tcode = "jobupd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_jobupd(sock,
					   &hispriv,
					   ntohl(inmsg.un.reader.slotno),
					   ntohl(inmsg.un.reader.seq),
					   ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_VARUPD:
			tcode = "varupd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varupd(sock,
					   ntohl(inmsg.un.reader.slotno),
					   ntohl(inmsg.un.reader.seq),
					   ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_VARCHCOMM:
			tcode = "varcomm";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varchcomm(sock,
					      ntohl(inmsg.un.reader.slotno),
					      ntohl(inmsg.un.reader.seq),
					      ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_JOBDATA:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "jobdata");
			api_jobdata(sock,
				    ntohl(inmsg.un.reader.slotno),
				    ntohl(inmsg.un.reader.seq),
				    ntohl(inmsg.un.reader.flags));
			continue;

		case  API_JOBCHMOD:
			tcode = "jobchmod";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_jobchmod(sock,
					     ntohl(inmsg.un.reader.slotno),
					     ntohl(inmsg.un.reader.seq),
					     ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_VARCHMOD:
			tcode = "varchmod";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varchmod(sock,
					     ntohl(inmsg.un.reader.slotno),
					     ntohl(inmsg.un.reader.seq),
					     ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_JOBCHOWN:
			tcode = "jobchown";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_jobchown(sock,
					     ntohl(inmsg.un.reader.slotno),
					     ntohl(inmsg.un.reader.seq),
					     ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_VARCHOWN:
			tcode = "varchown";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varchown(sock,
					     ntohl(inmsg.un.reader.slotno),
					     ntohl(inmsg.un.reader.seq),
					     ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_JOBCHGRP:
			tcode = "jobchgrp";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_jobchgrp(sock,
					     ntohl(inmsg.un.reader.slotno),
					     ntohl(inmsg.un.reader.seq),
					     ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
			break;

		case  API_VARCHGRP:
			tcode = "varchgrp";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varchgrp(sock,
					     ntohl(inmsg.un.reader.slotno),
					     ntohl(inmsg.un.reader.seq),
					     ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_VARRENAME:
			tcode = "varrename";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_varrename(sock,
					      ntohl(inmsg.un.reader.slotno),
					      ntohl(inmsg.un.reader.seq),
					      ntohl(inmsg.un.reader.flags));
			outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
			break;

		case  API_CIADD:
		{
			unsigned  res = 0;
			tcode = "ciadd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_ciadd(sock, &hispriv, &res);
			outmsg.un.r_reader.seq = htonl(res);
			break;
		}

		case  API_CIREAD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ciread");
			reply_ciread(sock);
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "ciread", "OK");
			continue;

		case  API_CIUPD:
			tcode = "ciupd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "ciupd");
			ret = reply_ciupd(sock, &hispriv, ntohl(inmsg.un.reader.slotno));
			break;

		case  API_CIDEL:
			tcode = "cidel";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "cidel");
			ret = reply_cidel(&hispriv, ntohl(inmsg.un.reader.slotno));
			break;

		case  API_HOLREAD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "holread");
			reply_holread(sock, ntohl(inmsg.un.reader.slotno));
			continue;

		case  API_HOLUPD:
			tcode = "holupd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_holupd(sock, ntohl(inmsg.un.reader.slotno));
			break;

		case  API_SETQUEUE:
			tcode = "setq";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			ret = reply_setqueue(sock, ntohs(inmsg.un.queuelength));
			break;

		case  API_REQPROD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "onmon");
			setup_prod();
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "onmon", "OK");
			continue;

		case  API_UNREQPROD:
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "offmon");
			unsetup_prod();
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "offmon", "OK");
			continue;

		case  API_SENDENV:
		{
			char	**ep;
			ULONG	ecount = 0;
			unsigned  maxlng = 0;
			extern	char	**xenviron;
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "sendenv");
			for  (ep = xenviron;  *ep;  ep++)  {
				unsigned  lng = strlen(*ep);
				ecount++;
				if  (lng > maxlng)
					maxlng = lng;
			}
			outmsg.retcode = 0;
			outmsg.un.r_lister.nitems = htonl(ecount);
			outmsg.un.r_lister.seq = htonl((ULONG) maxlng);
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			for  (ep = xenviron;  *ep;  ep++)  {
				unsigned  lng = strlen(*ep);
				ULONG	le = htonl((ULONG) lng);
				pushout(sock, (char *) &le, sizeof(le));
				pushout(sock, *ep, lng + 1);
			}
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "sendenv", "OK");
			continue;
		}

		case  API_GETBTU:
		{
			int_ugid_t	ouid, ogid;
			struct	ua_reply  outpriv;
			tcode = "getbtu";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);
			if  ((ouid = lookup_uname(inmsg.un.us.username)) == UNKNOWN_UID)  {
				ret = XB_UNKNOWN_USER;
				break;
			}
			if  (ouid != Realuid  &&  !(hispriv.btu_priv & BTM_RADMIN))  {
				ret = XB_NOPERM;
				break;
			}
			ogid = lastgid;

			/* Still re-read it in case something changed */

			if  (!(mpriv = getbtuentry(ouid)))  {
				ret = XB_UNKNOWN_USER;
				break;
			}
			btuser_pack(&outpriv.ua_perm, mpriv);
			strcpy(outpriv.ua_uname, prin_uname((uid_t) ouid));
			strcpy(outpriv.ua_gname, prin_gname((gid_t) ogid));
			outmsg.retcode = 0;
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pushout(sock, (char *) &outpriv, sizeof(outpriv));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, tcode, "OK");
			continue;
		}
		case  API_GETBTD:
		{
			Btdef	outbthdr;
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, "getbtd");
			BLOCK_ZERO(&outbthdr, sizeof(outbthdr));
			outbthdr.btd_version = Btuhdr.btd_version;
			outbthdr.btd_minp = Btuhdr.btd_minp;
			outbthdr.btd_maxp = Btuhdr.btd_maxp;
			outbthdr.btd_defp = Btuhdr.btd_defp;
			outbthdr.btd_maxll = htons(Btuhdr.btd_maxll);
			outbthdr.btd_totll = htons(Btuhdr.btd_totll);
			outbthdr.btd_spec_ll = htons(Btuhdr.btd_spec_ll);
			outbthdr.btd_priv = htonl(Btuhdr.btd_priv);
			for  (cnt = 0;  cnt < 3;  cnt++)  {
				outbthdr.btd_jflags[cnt] = htons(Btuhdr.btd_jflags[cnt]);
				outbthdr.btd_vflags[cnt] = htons(Btuhdr.btd_vflags[cnt]);
			}
			outmsg.retcode = 0;
			pushout(sock, (char *) &outmsg, sizeof(outmsg));
			pushout(sock, (char *) &outbthdr, sizeof(outbthdr));
			if  (tracing & TRACE_APIOPEND)
				trace_op_res(Realuid, "getbtd", "OK");
			continue;
		}

		case  API_PUTBTU:
		{
			int_ugid_t	ouid;
			Btuser		rbtu;
			struct	ua_reply	buf;

			tcode = "putbtu";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);

			pullin(sock, (char *) &buf, sizeof(buf));

			if  ((ouid = lookup_uname(inmsg.un.us.username)) == UNKNOWN_UID)  {
				ret = XB_UNKNOWN_USER;
				break;
			}
			if  ((ouid != Realuid  && !(hispriv.btu_priv & BTM_WADMIN)) || !(hispriv.btu_priv & (BTM_UMASK|BTM_WADMIN)))  {
				ret = XB_NOPERM;
				break;
			}

			rbtu.btu_user = ouid;
			rbtu.btu_minp = buf.ua_perm.btu_minp;
			rbtu.btu_maxp = buf.ua_perm.btu_maxp;
			rbtu.btu_defp = buf.ua_perm.btu_defp;
			rbtu.btu_maxll = ntohs(buf.ua_perm.btu_maxll);
			rbtu.btu_totll = ntohs(buf.ua_perm.btu_totll);
			rbtu.btu_spec_ll = ntohs(buf.ua_perm.btu_spec_ll);
			rbtu.btu_priv = ntohl(buf.ua_perm.btu_priv);

			for  (cnt = 0;  cnt < 3;  cnt++)  {
				rbtu.btu_jflags[cnt] = ntohs(buf.ua_perm.btu_jflags[cnt]);
				rbtu.btu_vflags[cnt] = ntohs(buf.ua_perm.btu_vflags[cnt]);
			}

			if  (rbtu.btu_minp == 0 || rbtu.btu_maxp == 0 || rbtu.btu_defp == 0)  {
				ret = XB_BAD_PRIORITY;
				break;
			}
			if  (rbtu.btu_maxll == 0 || rbtu.btu_totll == 0 || rbtu.btu_spec_ll == 0)  {
				ret = XB_BAD_LL;
				break;
			}

			/* In case something has changed */

			if  (ouid == Realuid)  {
				if  (!(mpriv = getbtuentry(ouid)))  {
					ret = XB_UNKNOWN_USER;
					break;
				}
				hispriv = *mpriv;
				if  (!(hispriv.btu_priv & BTM_WADMIN))  {

					/* Disallow everything except def prio and flags */

					if  (rbtu.btu_minp != hispriv.btu_minp ||
					     rbtu.btu_maxp != hispriv.btu_maxp  ||
					     rbtu.btu_maxll != hispriv.btu_maxll  ||
					     rbtu.btu_totll != hispriv.btu_totll  ||
					     rbtu.btu_spec_ll != hispriv.btu_spec_ll ||
					     rbtu.btu_priv != hispriv.btu_priv)  {
					badp:
						ret = XB_NOPERM;
						break;
					}
					if  (!(hispriv.btu_priv & BTM_UMASK))  {
						for  (cnt = 0;  cnt < 3;  cnt++)  {
							if  (rbtu.btu_jflags[cnt] != hispriv.btu_jflags[cnt])
								goto  badp;
							if  (rbtu.btu_vflags[cnt] != hispriv.btu_vflags[cnt])
								goto  badp;
						}
					}
				}
			}
			if  (!(mpriv = getbtuentry(ouid)))  {
				ret = XB_UNKNOWN_USER;
				break;
			}

			mpriv->btu_minp = rbtu.btu_minp;
			mpriv->btu_maxp = rbtu.btu_maxp;
			mpriv->btu_defp = rbtu.btu_defp;
			mpriv->btu_maxll = rbtu.btu_maxll;
			mpriv->btu_totll = rbtu.btu_totll;
			mpriv->btu_spec_ll = rbtu.btu_spec_ll;
			mpriv->btu_priv = rbtu.btu_priv;
			for  (cnt = 0;  cnt < 3;  cnt++)  {
				mpriv->btu_jflags[cnt] = rbtu.btu_jflags[cnt];
				mpriv->btu_vflags[cnt] = rbtu.btu_vflags[cnt];
			}
			putbtuentry(mpriv);
			if  (ouid == Realuid)
				hispriv = *mpriv;
			ret = XB_OK;
			break;
		}

		case  API_PUTBTD:
		{
			Btdef	inbthdr, rbthdr;

			tcode = "putbtd";
			if  (tracing & TRACE_APIOPSTART)
				trace_op(Realuid, tcode);

			pullin(sock, (char *) &inbthdr, sizeof(inbthdr));
			if  (!(hispriv.btu_priv & BTM_WADMIN))  {
				ret = XB_NOPERM;
				break;
			}
			BLOCK_ZERO(&rbthdr, sizeof(rbthdr));
			rbthdr.btd_minp = inbthdr.btd_minp;
			rbthdr.btd_maxp = inbthdr.btd_maxp;
			rbthdr.btd_defp = inbthdr.btd_defp;
			rbthdr.btd_maxll = ntohs(inbthdr.btd_maxll);
			rbthdr.btd_totll = ntohs(inbthdr.btd_totll);
			rbthdr.btd_spec_ll = ntohs(inbthdr.btd_spec_ll);
			rbthdr.btd_priv = ntohl(inbthdr.btd_priv);
			for  (cnt = 0;  cnt < 3;  cnt++)  {
				rbthdr.btd_jflags[cnt] = ntohs(inbthdr.btd_jflags[cnt]);
				rbthdr.btd_vflags[cnt] = ntohs(inbthdr.btd_vflags[cnt]);
			}
			if  (rbthdr.btd_minp == 0 || rbthdr.btd_maxp == 0 || rbthdr.btd_defp == 0)  {
				ret = XB_BAD_PRIORITY;
				break;
			}
			if  (rbthdr.btd_maxll == 0 || rbthdr.btd_totll == 0 || rbthdr.btd_spec_ll == 0)  {
				ret = XB_BAD_LL;
				break;
			}

			/* Re-read file to get locking open */

			if  (!(mpriv = getbtuentry(Realuid)))  {
				ret = XB_UNKNOWN_USER;
				break;
			}
			hispriv = *mpriv;
			Btuhdr.btd_minp = rbthdr.btd_minp;
			Btuhdr.btd_maxp = rbthdr.btd_maxp;
			Btuhdr.btd_defp = rbthdr.btd_defp;
			Btuhdr.btd_maxll = rbthdr.btd_maxll;
			Btuhdr.btd_totll = rbthdr.btd_totll;
			Btuhdr.btd_spec_ll = rbthdr.btd_spec_ll;
			Btuhdr.btd_priv = rbthdr.btd_priv;
			Btuhdr.btd_version = GNU_BATCH_MAJOR_VERSION;
			for  (cnt = 0;  cnt < 3;  cnt++)  {
				Btuhdr.btd_jflags[cnt] = rbthdr.btd_jflags[cnt];
				Btuhdr.btd_vflags[cnt] = rbthdr.btd_vflags[cnt];
			}
			putbtulist((Btuser *) 0, 0, 1);
			ret = XB_OK;
			break;
		}
		}
		outmsg.retcode = htons(ret);
		pushout(sock, (char *) &outmsg, sizeof(outmsg));
		if  (tracing & TRACE_APIOPEND)
			trace_op_res(Realuid, tcode, ret == XB_OK? "OK": "Failed");
	}
}
