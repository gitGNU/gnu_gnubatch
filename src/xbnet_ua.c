/* xbnet_ua.c -- xbnetserv user handling

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
#include <grp.h>
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

static	char	Filename[] = __FILE__;

int	holf_fd = -1;		/* Holiday file */
static	unsigned	holf_mode;

struct pend_job *find_pend(const netid_t whofrom)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (pend_list[cnt].clientfrom == whofrom)
			return  &pend_list[cnt];
	return  (struct pend_job *) 0;
}

struct pend_job *find_j_by_jno(const jobno_t jobno)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (pend_list[cnt].out_f  &&  pend_list[cnt].jobn == jobno)
			return  &pend_list[cnt];
	return  (struct pend_job *) 0;
}

struct pend_job *add_pend(const netid_t whofrom)
{
	int	cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (!pend_list[cnt].out_f)  {
			pend_list[cnt].clientfrom = whofrom;
			pend_list[cnt].prodsent = 0;
			return  &pend_list[cnt];
		}
	return  (struct pend_job *) 0;
}

/* Clean up after decaying job.  */

void  abort_job(struct pend_job *pj)
{
	if  (!pj  ||  !pj->out_f)
		return;
	fclose(pj->out_f);
	pj->out_f = (FILE *) 0;
	unlink(pj->tmpfl);
	pj->clientfrom = 0;
}

static void  udp_send_vec(char *vec, const int size, struct sockaddr_in *sinp)
{
	sendto(uasock, vec, size, 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Similar routine for when we are sending from scratch */

static void  udp_send_to(char *vec, const int size, const netid_t whoto)
{
	int	sockfd, tries;
	struct	sockaddr_in	to_sin, cli_addr;

	BLOCK_ZERO(&to_sin, sizeof(to_sin));
	to_sin.sin_family = AF_INET;
	to_sin.sin_port = uaportnum;
	to_sin.sin_addr.s_addr = whoto;

	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port = 0;

	/* We don't really need the cli_addr but we are obliged to
	   bind something.  The remote uses our standard port.  */

	for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
		if  ((sockfd = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)
			return;
		if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
			close(sockfd);
			return;
		}
		if  (sendto(sockfd, vec, size, 0, (struct sockaddr *) &to_sin, sizeof(to_sin)) >= 0)  {
			close(sockfd);
			return;
		}
		close(sockfd);
	}
}

static void  udp_job_process(const netid_t whofrom, char *pmsg, int datalength, struct sockaddr_in *sinp)
{
	int			ret = 0, tries;
	ULONG			indx;
	struct	hhash		*frp;
	struct	ni_jobhdr	*nih;
	struct	pend_job	*pj;
	struct	client_if	reply;
	Shipc			Oreq;

	if  (!(frp = find_remote(whofrom)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Unknown client");
		ret = XBNR_UNKNOWN_CLIENT;
		goto  senderr;
	}

	if  (datalength < sizeof(struct ni_jobhdr))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "jobproc badhdr");
		ret = XBNR_BAD_JOBDATA;
		goto  senderr;
	}

	pj = find_pend(whofrom);
	nih = (struct ni_jobhdr *) pmsg;
	pmsg += sizeof(struct ni_jobhdr);
	datalength -= sizeof(struct ni_jobhdr);

	switch  (nih->code)  {
	default:
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Unknown cmd");
		reply.code = SV_CL_UNKNOWNC;
		goto  senderr;

	case  CL_SV_STARTJOB:

		/* Start of new job, delete any pending one. add new one */

		abort_job(pj);
		pj = add_pend(whofrom);
		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Nomem queue file");
			reply.code = SV_CL_PEND_FULL;
			goto  senderr;
		}
		pj->timeout = frp->timeout;
		pj->lastaction = time((time_t *) 0);
		pj->joblength = pj->lengthexp = ntohs(nih->joblength);
		if  (pj->lengthexp > sizeof(struct nijobmsg) || pj->lengthexp < (unsigned) datalength)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "addjob buffer err");
			ret = XBNR_BAD_JOBDATA;
			goto  senderr;
		}
		pj->lengthexp -= datalength;
		pj->cpos = (char *) &pj->jobin;
		BLOCK_COPY(pj->cpos, pmsg, datalength);
		pj->cpos += datalength;
		goto  sendok;

	case  CL_SV_CONTJOB:
		if  ((unsigned) datalength > pj->lengthexp)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "contjob buffer");
			ret = XBNR_BAD_JOBDATA;
			goto  senderr;
		}
		BLOCK_COPY(pj->cpos, pmsg, datalength);
		pj->cpos += datalength;
		pj->lengthexp -= datalength;

		if  (pj->lengthexp == 0)  {
			BtuserRef	cli_priv;

			/* Finished with job, construct output job */

			if  ((ret = unpack_job(&pj->jobout, &pj->jobin, pj->joblength, whofrom)) != 0)  {
				reply.param = htonl((LONG) err_which);
				abort_job(pj);
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op(whofrom, "contjob err");
				goto  senderr;
			}

			if  ((ret = convert_username(frp, nih, &pj->jobout, &cli_priv)) != 0)  {
				reply.param = 0;
				abort_job(pj);
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op_name(whofrom, "Contjob convuser", frp->actname);
				goto  senderr;
			}

			/* Stick user and group into job to keep track
			   of them.  (Realuid/realgid get set by
			   convert_username).  */

			pj->jobout.h.bj_mode.o_uid = pj->jobout.h.bj_mode.c_uid = Realuid;
			pj->jobout.h.bj_mode.o_gid = pj->jobout.h.bj_mode.c_gid = Realgid;

			if  ((ret = unpack_cavars(&pj->jobout, &pj->jobin)) != 0)  {
				abort_job(pj);
				goto  senderr;
			}
			if  ((ret = validate_job(&pj->jobout, cli_priv)) != 0)  {
				reply.param = 0;
				abort_job(pj);
				goto  senderr;
			}

			/* Generate job number and output file vaguely from netid etc.  */

			pj->jobn = (ULONG) (ntohl(whofrom) + pj->lastaction) % JOB_MOD;
			pj->out_f = goutfile(&pj->jobn, pj->tmpfl, 1);
			pj->jobout.h.bj_job = pj->jobn;
			pj->jobout.h.bj_time = pj->lastaction;
		}
		goto  sendok;

	case  CL_SV_JOBDATA:
		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Unknown job (job data)");
			ret = SV_CL_UNKNOWNJ;
			goto  senderr;
		}
		if  (!pj->out_f  ||  pj->lengthexp != 0)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Job data buffer");
			ret = SV_CL_BADPROTO;
			goto  senderr;
		}
		pj->lastaction = time((time_t *) 0);
		while  (--datalength >= 0)  {
			if  (putc(*(unsigned char *)pmsg, pj->out_f) == EOF)  {
				abort_job(pj);
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op(whofrom, "File system full");
				ret = XBNR_FILE_FULL;
				goto  senderr;
			}
			pmsg++;
		}
		goto  sendok;

	case  CL_SV_ENDJOB:
		if  (!pj)  {
			ret = SV_CL_UNKNOWNJ;
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Unknown job (end job)");
			goto  senderr;
		}
		if  (!pj->out_f  ||  pj->lengthexp != 0)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Buffer error (end job)");
			ret = SV_CL_BADPROTO;
			goto  senderr;
		}
		JREQ = &Xbuffer->Ring[indx = getxbuf_serv()];
		BLOCK_ZERO(&Oreq, sizeof(Oreq));
		Oreq.sh_mtype = TO_SCHED;
		Oreq.sh_params.mcode = J_CREATE;
		Oreq.sh_params.uuid = pj->jobout.h.bj_mode.o_uid;
		Oreq.sh_params.ugid = pj->jobout.h.bj_mode.o_gid;
		mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
		Oreq.sh_un.sh_jobindex = indx;
		BLOCK_COPY(JREQ, &pj->jobout, sizeof(Btjob));
		JREQ->h.bj_slotno = -1;
#ifdef	USING_MMAP
		sync_xfermmap();
#endif
		for  (tries = 0;  tries < MSGQ_BLOCKS;  tries++)  {
			if  (msgsnd(Ctrl_chan, (struct msgbuf *)&Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) >= 0)
				goto  sentok;
			sleep(MSGQ_BLOCKWAIT);
		}
		freexbuf_serv(indx);
		abort_job(pj);
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "msg q full");
		ret = XBNR_QFULL;
		goto  senderr;

	sentok:
		freexbuf_serv(indx);
		reply.param = htonl(JREQ->h.bj_job);
		if  ((ret = readreply()) != J_OK)  {
			reply.param = htonl((LONG) ret);
			ret = XBNR_ERR;
			abort_job(pj);
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op_name(whofrom, "Job rejected", frp->actname);
			goto  senderr;
		}

		fclose(pj->out_f);
		pj->out_f = (FILE *) 0;
		pj->clientfrom = 0;
		goto  sendok;

	case  CL_SV_HANGON:
		if  (!pj)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "unknown job (hangon)");
			ret = SV_CL_UNKNOWNJ;
			goto  senderr;
		}
		pj->lastaction = time((time_t *) 0);
		pj->prodsent = 0;
		goto  sendok;
	}

 sendok:
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op_name(whofrom, "Job OK", frp->actname);
	reply.code = SV_CL_ACK;
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
	return;

 senderr:
	reply.code = (unsigned char) ret;
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
}

static	void	udp_send_uglist(struct sockaddr_in *sinp, char **(*ugfn)(const char *))
{
	char	**ul = (*ugfn)((const char *) 0);
	int	rp = 0;
	char	reply[CL_SV_BUFFSIZE];

	if  (ul)  {
		char	**up;
		for  (up = ul;  *up;  up++)  {
			int	lng = strlen(*up) + 1;
			if  (lng + rp > CL_SV_BUFFSIZE)  {
				udp_send_vec(reply, rp, sinp);
				rp = 0;
			}
			strcpy(&reply[rp], *up);
			rp += lng;
			free(*up);
		}
		if  (rp > 0)
			udp_send_vec(reply, rp, sinp);
		free((char *) ul);
	}

	/* Mark end with a null */

	reply[0] = '\0';
	udp_send_vec(reply, 1, sinp);
}

static void  udp_send_vlist(const netid_t whofrom, const char *pmsg, const int datalen, struct sockaddr_in *sinp)
{
	struct	hhash	*frp;
	const	struct	ua_venq	*venq;
	int		rp = 0, nn;
	unsigned	perm;
	BtuserRef	mpriv;
	char	reply[CL_SV_BUFFSIZE];

	if  (datalen != sizeof(struct ua_venq))
		goto  badret;
	if  (!(frp = find_remote(whofrom)))
		goto  badret;

	frp->lastaction = time((time_t *) 0); /* Last activity */
	venq = (const struct ua_venq *) pmsg;
	perm = ntohs(venq->uav_perm);
	if  (frp->rem.ht_flags & HT_DOS)
		Realuid = (uid_t) lookup_uname(frp->dosname);
	else
		Realuid = (uid_t) lookup_uname(venq->uname);
	Realgid = (gid_t) lastgid;
	if  (!(mpriv = getbtuentry(Realuid)))
		goto  badret;
	rvarfile(1);

	/* These aren't sorted, that's the other end's problem.  */

	for  (nn = 0;  nn < VAR_HASHMOD;  nn++)  {
		vhash_t		hp;
		struct	Ventry	*fp;
		BtvarRef	vp;
		for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = fp->Vnext)  {
			fp = &Var_seg.vlist[hp];
			vp = &fp->Vent;
			if  ((vp->var_flags & VF_EXPORT)  &&  mpermitted(&vp->var_mode, perm, 0))  {
				unsigned  lng = strlen(vp->var_name), hlng, tlng;
				char	*hname;
				if  (vp->var_id.hostid)  {
					hname = look_host(vp->var_id.hostid);
					hlng = strlen(hname);
				}
				else  {
					hname = myhostname;
					hlng = myhostl;
				}
				tlng = lng + hlng + 2 + BTV_NAME; /* Colon and null */
				if  (tlng + rp > CL_SV_BUFFSIZE)  {
					udp_send_vec(reply, rp, sinp);
					rp = 0;
				}
				strcpy(&reply[rp], hname);
				rp += hlng;
				reply[rp++] = ':';
				strcpy(&reply[rp], vp->var_name);
				rp += lng + 1;
			}
		}
	}
	if  (rp > 0)
		udp_send_vec(reply, rp, sinp);

 badret:
	/* Mark end with a null */

	reply[0] = '\0';
	udp_send_vec(reply, 1, sinp);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op(whofrom, "send vlist done");
}

static void  udp_send_cilist(const netid_t whofrom, struct sockaddr_in *sinp)
{
	int		rp = 0;
	unsigned	cnt;
	CmdintRef	ocip;
	struct	hhash	*frp;
	char	reply[CL_SV_BUFFSIZE];

	if  (!(frp = find_remote(whofrom)))
		goto  badret;

	frp->lastaction = time((time_t *) 0);
	rereadcif();

	ocip = (CmdintRef) reply;
	for  (cnt = 0;  cnt < Ci_num;  cnt++)  {
		CmdintRef  ci = &Ci_list[cnt];
		if  (ci->ci_name[0] == '\0')
			continue;
		if  (rp + sizeof(Cmdint) >= CL_SV_BUFFSIZE)  {
			udp_send_vec(reply, rp, sinp);
			rp = 0;
			ocip = (CmdintRef) reply;
		}
		ocip->ci_ll = htons(ci->ci_ll);
		ocip->ci_nice = ci->ci_nice;
		ocip->ci_flags = ci->ci_flags;
		strcpy(ocip->ci_name, ci->ci_name);
		strcpy(ocip->ci_path, ci->ci_path);
		strcpy(ocip->ci_args, ci->ci_args);
		rp += sizeof(Cmdint);
		ocip++;
	}
	if  (rp > 0)
		udp_send_vec(reply, rp, sinp);
 badret:
	/* Mark end with a null */

	reply[0] = '\0';
	udp_send_vec(reply, 1, sinp);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op(whofrom, "send cilist-OK");
}

/* Get a holiday year */

void  get_hf(const unsigned year, char *reply)
{
	if  (holf_fd < 0)  {
		char	*fname = envprocess(HOLFILE);
		holf_fd = open(fname, O_RDONLY);
		holf_mode = O_RDONLY;
		free(fname);
	}

	if  (holf_fd >= 0)  {
		lseek(holf_fd, (long) (year * YVECSIZE), 0);
		read(holf_fd, reply, YVECSIZE);
	}
}

/* Insert a holiday year (currently only called from API) */

void  put_hf(const unsigned year, char *reply)
{
	if  (holf_fd >= 0  &&  holf_mode != O_RDWR)  {
		close(holf_fd);
		holf_fd = -1;
	}

	if  (holf_fd < 0)  {
		char	*fname = envprocess(HOLFILE);
		if  ((holf_fd = open(fname, O_RDWR)) < 0)  {
			holf_fd = open(fname, O_RDWR|O_CREAT, 0644);
#ifdef	HAVE_FCHOWN
			if  (Daemuid != ROOTID)
				fchown(holf_fd, Daemuid, Daemgid);
#else
			if  (Daemuid != ROOTID)
				chown(fname, Daemuid, Daemgid);
#endif
		}
		holf_mode = O_RDWR;
		free(fname);
	}

	if  (holf_fd >= 0)  {
		lseek(holf_fd, (long) (year * YVECSIZE), 0);
		write(holf_fd, reply, YVECSIZE);
	}
}

static void  udp_send_hlist(const netid_t whofrom, const char *pmsg, const int datalen, struct sockaddr_in *sinp)
{
	const	struct	ua_venq	*venq;
	unsigned	year;
	struct	hhash	*frp;
	char	reply[CL_SV_BUFFSIZE];

	/* If read doesn't read anything, just return zeroes */

	BLOCK_ZERO(reply, YVECSIZE);
	if  (datalen != sizeof(struct ua_venq))
		goto  badret;
	if  (!(frp = find_remote(whofrom)))
		goto  badret;

	venq = (const struct ua_venq *) pmsg;
	year = ntohs(venq->uav_perm);
	get_hf(year, reply);
	frp->lastaction = time((time_t *) 0);

 badret:
	udp_send_vec(reply, YVECSIZE, sinp);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op(whofrom, "send hlist done");
}

int  checkpw(const char *name, const char *passwd)
{
	static	char	*btpwnam;
	int		ipfd[2], opfd[2];
	char		rbuf[1];
	PIDTYPE		pid;

	if  (!btpwnam)
		btpwnam = envprocess(BTPWPROG);

	/* Don't bother with error messages, just say no.  */

	if  (pipe(ipfd) < 0)
		return  0;
	if  (pipe(opfd) < 0)  {
		close(ipfd[0]);
		close(ipfd[1]);
		return  0;
	}

	if  ((pid = fork()) == 0)  {
		close(opfd[1]);
		close(ipfd[0]);
		if  (opfd[0] != 0)  {
			close(0);
			dup(opfd[0]);
			close(opfd[0]);
		}
		if  (ipfd[1] != 1)  {
			close(1);
			dup(ipfd[1]);
			close(ipfd[1]);
		}
		execl(btpwnam, btpwnam, name, (char *) 0);
		exit(255);
	}
	close(opfd[0]);
	close(ipfd[1]);
	if  (pid < 0)  {
		close(ipfd[0]);
		close(opfd[1]);
		return  0;
	}
	write(opfd[1], passwd, strlen(passwd));
	rbuf[0] = '\n';
	write(opfd[1], rbuf, sizeof(rbuf));
	close(opfd[1]);
	if  (read(ipfd[0], rbuf, sizeof(rbuf)) != sizeof(rbuf))  {
		close(ipfd[0]);
		return  0;
	}
	close(ipfd[0]);
	return  rbuf[0] == '0'? 1: 0;
}

/* See whether the given user can access the given group.  We DON'T
   use supplementary group login in rpwfile etc as not all
   systems support them and I don't think that we use this
   sufficiently often.  */

int  chk_vgroup(const int_ugid_t uid, const char *gname)
{
	struct	group	*gg = getgrnam(gname);
	char	**mems;
	char	*unam = prin_uname(uid);

	endgrent();

	if  (!gg  ||  !(mems = gg->gr_mem))
		return  0;

	while  (*mems)  {
		if  (strcmp(*mems, unam) == 0)
			return  1;
		mems++;
	}
	return  0;
}

/* Tell scheduler about new user.  */

void  tell_sched_roam(const netid_t netid, const char *unam, const char *gnam)
{
	Shipc	nmsg;

	BLOCK_ZERO(&nmsg, sizeof(nmsg));
	nmsg.sh_mtype = TO_SCHED;
	nmsg.sh_params.mcode = N_ROAMUSER;
	nmsg.sh_params.upid = getpid();
	nmsg.sh_un.sh_n.hostid = netid;
	strncpy(nmsg.sh_un.sh_n.hostname, unam, HOSTNSIZE);
	strncpy(nmsg.sh_un.sh_n.alias, gnam, HOSTNSIZE);
	msgsnd(Ctrl_chan, (struct msgbuf *) &nmsg, sizeof(Shreq) + sizeof(struct remote), 0); /* Not expecting reply */
}

static void  init_palsmsg(struct ua_pal *pm, struct hhash *frp)
{
	BLOCK_ZERO(pm, sizeof(struct ua_pal));
	pm->uap_op = SV_SV_LOGGEDU;
	pm->uap_netid = frp->rem.hostid;
	strncpy(pm->uap_name, frp->actname, UIDSIZE);
	strncpy(pm->uap_grp, prin_gname(frp->rem.n_gid), UIDSIZE);
	strncpy(pm->uap_wname, frp->dosname, UIDSIZE);
}

/* Tell other xbnetservs about new user.  */

void  tell_friends(struct hhash *frp)
{
	unsigned	cnt;
	struct	ua_pal  palsmsg;

	init_palsmsg(&palsmsg, frp);

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	hhash	*hp;
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)
			if  ((hp->rem.ht_flags & (HT_DOS|HT_TRUSTED)) == HT_TRUSTED)  {
				if  (tracing & TRACE_SYSOP)
					client_trace_op_name(hp->rem.hostid, "tell friends", frp->actname);
				udp_send_to((char *) &palsmsg, sizeof(palsmsg), hp->rem.hostid);
			}
	}

	tell_sched_roam(frp->rem.hostid, frp->actname, palsmsg.uap_grp);
}

/* Check password status in login/enquiries on roaming users, and "tell friends" */

static void  set_pwstatus(struct ua_login *inmsg, struct hhash *frp, struct cluhash *cp)
{
	if  (cp->rem.ht_flags & HT_PWCHECK  ||  (cp->machname  &&  ncstrcmp(cp->machname, inmsg->ual_machname) != 0))  {
		frp->flags = UAL_NOK;
		if  (inmsg->ual_op != UAL_LOGIN)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op_name(frp->rem.hostid, "No pw", frp->actname);
			return;
		}
		if  (!checkpw(frp->actname, inmsg->ual_passwd))  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op_name(frp->rem.hostid, "Bad pw", frp->actname);
			frp->flags = UAL_INVP;
			return;
		}
	}
	frp->flags = UAL_OK;
	tell_friends(frp);
}

/* Make name lower case - assumed at most UIDSIZE long, but we don't trust it.  */

static char *lcify(const char *orig)
{
	static	char	result[UIDSIZE+1];
	int	cnt = 0;

	while  (*orig  &&  cnt < UIDSIZE)
		result[cnt++] = tolower(*orig++);
	result[cnt] = '\0';
	return  result;
}

static struct cluhash *look_clu(const char *winname)
{
	struct	cluhash	*cp;
	unsigned  hval = calc_clu_hash(winname);

	for  (cp = cluhashtab[hval];  cp;  cp = cp->next)
		if  (ncstrcmp(cp->rem.hostname, winname) == 0  &&
		     (lookup_uname(cp->rem.hostname) != UNKNOWN_UID  ||  lookup_uname(cp->rem.alias) != UNKNOWN_UID))
			return  cp;
	for  (cp = cluhashtab[hval];  cp;  cp = cp->alias_next)
		if  (ncstrcmp(cp->rem.alias, winname) == 0  &&
		     (lookup_uname(cp->rem.hostname) != UNKNOWN_UID  ||  lookup_uname(cp->rem.alias) != UNKNOWN_UID))
			return  cp;
	/*  If a default user is supplied, use that */
	winname = XBDEFNAME;
	for  (cp = cluhashtab[calc_clu_hash(winname)];  cp;  cp = cp->next)
		if  (ncstrcmp(cp->rem.hostname, winname) == 0  &&  lookup_uname(cp->rem.alias) != UNKNOWN_UID)
			return  cp;
	return  (struct cluhash *) 0;
}

static void  look_roam_ug(struct hhash *frp, struct cluhash *cp)
{
	int_ugid_t	nuid;

	if  ((nuid = lookup_uname(cp->rem.hostname)) != UNKNOWN_UID)  {
		frp->actname = stracpy(cp->rem.hostname);
		frp->rem.n_uid = nuid;
		frp->rem.n_gid = lastgid;
	}
	else  if  ((nuid = lookup_uname(cp->rem.alias)) != UNKNOWN_UID)  {
		frp->actname = stracpy(cp->rem.alias);
		frp->rem.n_uid = nuid;
		frp->rem.n_gid = lastgid;
	}
	else  {
		frp->actname = stracpy(prin_uname(Daemuid));
		frp->rem.n_uid = Daemuid;
		frp->rem.n_gid = Daemgid;
	}
}

struct cluhash *update_roam_name(struct hhash *frp, const char *name)
{
	struct	cluhash	*cp = look_clu(name);

	if  (cp)  {
		free(frp->dosname);
		frp->dosname = stracpy(name);
		free(frp->actname);
		look_roam_ug(frp, cp);
		frp->rem.ht_flags = cp->rem.ht_flags;
		frp->timeout = cp->rem.ht_timeout;
		frp->lastaction = time((time_t *) 0);
	}
	return  cp;
}

struct cluhash *new_roam_name(const netid_t whofrom, struct hhash **frpp, const char *name)
{
	struct	cluhash  *cp;

	if  ((cp = look_clu(name)))  {

		unsigned  nhval;
		struct	hhash	*frp;

		/* Allocate and put a new structure on host hash list. */

		if  (!(frp = (struct hhash *) malloc(sizeof(struct hhash))))
			ABORT_NOMEM;

		BLOCK_ZERO(frp, sizeof(struct hhash));
		nhval = calcnhash(whofrom);
		frp->hn_next = nhashtab[nhval];
		nhashtab[nhval] = frp;
		frp->rem = cp->rem;
		frp->rem.hostid = whofrom;
		frp->timeout = frp->rem.ht_timeout;
		frp->lastaction = time((time_t *) 0);
		frp->dosname = stracpy(name);
		look_roam_ug(frp, cp);
		*frpp = frp;
	}
	return  cp;
}

int  update_nonroam_name(struct hhash *frp, const char *name)
{
	int_ugid_t	nuid;

	free(frp->actname);
	frp->actname = stracpy(name);
	if  ((nuid = lookup_uname(frp->actname)) == UNKNOWN_UID)  {
		if  ((nuid = lookup_uname(lcify(name))) == UNKNOWN_UID)  {
			frp->flags = UAL_INVU;
			return  0;
		}
		free(frp->actname);
		frp->actname = stracpy(lcify(name));
	}
	frp->rem.n_uid = nuid;
	frp->rem.n_gid = lastgid;
	frp->lastaction = time((time_t *) 0);
	return  1;
}

static void  do_logout(struct hhash *frp)
{
	if  (frp->rem.ht_flags & HT_ROAMUSER)  {

		struct  hhash  **hpp, *hp;

		/* Roaming user, get rid of record from hash chain.
		   We "cannot" fall off the bottom of this loop,
		   but we write it securely don't we...  */

		for  (hpp = &nhashtab[calcnhash(frp->rem.hostid)]; (hp = *hpp); hpp = &hp->hn_next)
			if  (hp == frp)  {
				*hpp = hp->hn_next;
				free(frp->actname);
				free(frp->dosname);
				free((char *) frp);
				break;
			}
	}
	else  {

		/* Non roaming-user case, disregard it unless we have
		   a non-standard login, whereupon we fix it back
		   to standard login.  */

		if  (strcmp(frp->actname, frp->dosname) != 0)  {
			free(frp->actname);
			frp->actname = stracpy(frp->dosname);
			frp->flags = frp->rem.ht_flags & HT_PWCHECK? UAL_NOK: UAL_OK;
		}

		/* Worry about that again tomorrow unless we hear back */
		frp->lastaction = time((time_t *) 0) + 3600L * 24;
	}
}

/* Handle logins and initial enquiries, doing as much as possible.  */

static void  udp_login(const netid_t whofrom, struct ua_login *inmsg, const int inlng, struct sockaddr_in *sinp)
{
	struct	ua_login	reply;

	BLOCK_ZERO(&reply, sizeof(reply));	/* Maybe this is paranoid but with passwords kicking about..... */

	if  (inlng != sizeof(struct ua_login))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Login struct err");
		reply.ual_op = SV_CL_BADPROTO;
		goto  sendret;
	}

	if  (inmsg->ual_op == UAL_ENQUIRE  ||  inmsg->ual_op == UAL_LOGIN)  {
		struct	hhash		*frp;
		struct	cluhash		*cp;

		if  ((frp = find_remote(whofrom)))  {

			if  (frp->rem.ht_flags & HT_ROAMUSER)  {

				/* Might have changed in the interim
				   We stored the Windows user as sent in "dosname",
				   the other way round from the non-roaming case */

				if  (inmsg->ual_op == UAL_LOGIN)  {
					if  (strcmp(frp->dosname, inmsg->ual_name) != 0)  { /* Name change */
						if  (!(cp = update_roam_name(frp, inmsg->ual_name)))  {
							if  (tracing & TRACE_CLIOPEND)
								client_trace_op_name(whofrom, "Unknown user", inmsg->ual_name);
							reply.ual_op = XBNR_NOT_USERNAME;
							goto  sendret;
						}
						set_pwstatus(inmsg, frp, cp);
					}
					else  if  (frp->flags != UAL_OK)  {
						if  (!(cp = look_clu(inmsg->ual_name)))  {
							if  (tracing & TRACE_CLIOPEND)
								client_trace_op_name(whofrom, "Unknown client user", inmsg->ual_name);
							reply.ual_op = XBNR_UNKNOWN_CLIENT;
							goto  sendret;
						}
						set_pwstatus(inmsg, frp, cp);
					}
				}
			}
			else  {

				/* If this isn't from a client, I'm confused */

				if  (!(frp->rem.ht_flags & HT_DOS))  {
					if  (tracing & TRACE_CLIOPEND)
						client_trace_op(whofrom, "Client op - not client");
					reply.ual_op = XBNR_NOT_CLIENT;
					goto  sendret;
				}

				/* Not a roaming user, but some species of dos user who must be
				   correctly specified (case insens) in the Windows user in
				   "actname", no opportunity for aliasing here as we've got a real
				   user in "dosname" */

				if  (inmsg->ual_op == UAL_LOGIN)  {
					if  (ncstrcmp(frp->actname, inmsg->ual_name) != 0)  {  /* Name change */
						if  (!update_nonroam_name(frp, inmsg->ual_name))  {
							if  (tracing & TRACE_CLIOPEND)
								client_trace_op_name(whofrom, "invalid user", inmsg->ual_name);
							frp->flags = UAL_INVU;
							goto  sendactname;
						}

						/* Change here - we no longer effectively force on password
						   check if no (dosuser) see also look_host.c */

						if  (frp->rem.ht_flags & HT_PWCHECK  ||
						     (frp->dosname[0]  &&  ncstrcmp(frp->dosname, frp->actname) != 0))  {
							frp->flags = UAL_NOK;		/* Password required */
							if  (inmsg->ual_op == UAL_LOGIN  &&  !checkpw(frp->actname, inmsg->ual_passwd))
								frp->flags = UAL_INVP;
							else
								frp->flags = UAL_OK;
						}
						else
							frp->flags = UAL_OK;
					}
				}
			}
		}
		else  {

			/* We haven't heard of this geyser before.
			   (This only applies to roaming users).  */

			if  (!(cp = new_roam_name(whofrom, &frp, inmsg->ual_name)))  {
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op_name(whofrom, "unknown client uname", inmsg->ual_name);
				reply.ual_op = XBNR_UNKNOWN_CLIENT;
				goto  sendret;
			}
			set_pwstatus(inmsg, frp, cp);
		}

	sendactname:
		strncpy(reply.ual_name, frp->actname, UIDSIZE);
		reply.ual_op = (unsigned char) frp->flags;
	}
	else  {

		struct	hhash		*frp;

		/* Logout case. */

		if  (inmsg->ual_op != UAL_LOGOUT)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Invalid proto logout");
			reply.ual_op = SV_CL_BADPROTO;
			goto  sendret;
		}

		if  (!(frp = find_remote(whofrom)))  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Unknown client");
			reply.ual_op = XBNR_UNKNOWN_CLIENT;
			goto  sendret;
		}

		reply.ual_op = UAL_OK;
		do_logout(frp);
	}

 sendret:
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
}

static void  udp_newgrp(const netid_t whofrom, struct ua_login *inmsg, const int inlng, struct sockaddr_in *sinp)
{
	struct	hhash		*frp;
	int_ugid_t		ngid;
	BtuserRef		hispriv;
	struct	ua_login	reply;

	BLOCK_ZERO(&reply, sizeof(reply));	/* Maybe this is paranoid but with passwords kicking about..... */

	if  (inlng != sizeof(struct ua_login))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Newgrp buffer");
		reply.ual_op = SV_CL_BADPROTO;
		goto  sendret;
	}

	if  (!(frp = find_remote(whofrom)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Newgrp unknown client");
		reply.ual_op = XBNR_UNKNOWN_CLIENT;
		goto  sendret;
	}

	/* If this isn't from a client, I'm confused */

	if  (!(frp->rem.ht_flags & HT_DOS))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Newgrp not client");
		reply.ual_op = XBNR_NOT_CLIENT;
		goto  sendret;
	}

	/* This is where we bung the result in */

	if  ((reply.ual_op = frp->flags) != UAL_OK)
		goto  sendret;

	/* Now look at the group name */

	if  ((ngid = lookup_gname(inmsg->ual_name)) == UNKNOWN_GID)  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op_name(whofrom, "Newgrp unknown group", inmsg->ual_name);
		reply.ual_op = XBNR_UNKNOWN_GROUP;
		goto  sendret;
	}

	/* See if the user can do anything, in which case we allow any group */

	if  (!(hispriv = getbtuentry(frp->rem.n_uid)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op_name(whofrom, "Newgrp bad user", prin_uname(frp->rem.n_uid));
		reply.ual_op = XBNR_BAD_USER;
		goto  sendret;
	}
	if  (!(hispriv->btu_priv & BTM_WADMIN) && !chk_vgroup(frp->rem.n_uid, inmsg->ual_name))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op_name(whofrom, "Newgrp invalid group", inmsg->ual_name);
		reply.ual_op = UAL_INVG;
		goto  sendret;
	}

	/*	Do the biz...	*/

	if  (ngid != frp->rem.n_gid)  {
		frp->rem.n_gid = ngid;
		if  (frp->rem.ht_flags & HT_ROAMUSER)
			tell_friends(frp);
	}

	if  (tracing & TRACE_CLIOPEND)
		client_trace_op_name(whofrom, "Newgrp OK", inmsg->ual_name);

 sendret:
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
}

/* Original kind of enquiry for user permissions.  */

static void  udp_send_perms(const netid_t whofrom, const char *pmsg, const int datalen, struct sockaddr_in *sinp)
{
	int		ret;
	int_ugid_t	nuid;
	struct	hhash	*frp;
	BtuserRef	btuser;
	const	struct	ni_jobhdr	*uenq;
	struct	ua_reply  rep;

	BLOCK_ZERO(&rep, sizeof(rep));

	/* If I don't know who the client is, reject it regardless.  */

	if  (!(frp = find_remote(whofrom)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "Sendperms unknown client");
		ret = XBNR_UNKNOWN_CLIENT;
		goto  badret;
	}

	/* Do this now we'll want it later anyhow.  */

	Realuid = frp->rem.n_uid;
	Realgid = frp->rem.n_gid;
	if  (!(btuser = getbtuentry(Realuid)))  {
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op_name(whofrom, "Sendperms bad user", prin_uname(Realuid));
		ret = XBNR_BAD_USER;
		goto  badret;
	}

	/* For compatibility with some older versions, on Windows
	   machines where the message isn't the usual length,
	   bypass the checking.  */

	if  (datalen < sizeof(struct ni_jobhdr))  {
		if  (!(frp->rem.ht_flags & HT_DOS))  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Sendperms not client");
			ret = SV_CL_BADPROTO;
			goto  badret;
		}
		goto  xmit;
	}


	uenq = (const struct ni_jobhdr *) pmsg;

	/* Windows users now have roaming users to worry about.
	   However we keep the current uid in the remote structure.  */

	if  (frp->rem.ht_flags & HT_DOS)  {
		if  (frp->flags != UAL_OK)  {
			if  (tracing & TRACE_CLIOPEND)
				client_trace_op(whofrom, "Sendperms not logged in");
			ret = frp->flags;
			goto  badret;
		}

		if  (uenq->uname[0])  {
			if  ((nuid = lookup_uname(uenq->uname)) == UNKNOWN_UID)  {
				if  (tracing & TRACE_CLIOPEND)
					client_trace_op_name(whofrom, "Unknown user", uenq->uname);
				ret = XBNR_NOT_USERNAME;
				goto  badret;
			}
			if  (nuid != Realuid)  {
				if  (!(btuser->btu_priv & BTM_RADMIN))  {
					if  (tracing & TRACE_CLIOPEND)
						client_trace_op_name(whofrom, "No read admin", prin_uname(Realuid));
					ret = XBNR_NORADMIN;
					goto  badret;
				}
				Realuid = (uid_t) nuid;
				Realgid = (gid_t) lastgid;
				if  (!(btuser = getbtuentry(Realuid)))  {
					if  (tracing & TRACE_CLIOPEND)
						client_trace_op_name(whofrom, "Unknown user", prin_uname(Realuid));
					ret = XBNR_UNKNOWN_USER;
					goto  badret;
				}
			}
		}
	}

 xmit:
	strcpy(rep.ua_uname, prin_uname(Realuid));
	strcpy(rep.ua_gname, prin_gname(Realgid));
	btuser_pack(&rep.ua_perm, btuser);
	udp_send_vec((char *) &rep, sizeof(struct ua_reply), sinp);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op_name(whofrom, "sendperms-OK", rep.ua_uname);
	return;
 badret:
	rep.ua_perm.btu_user = htonl((LONG) ret);
	udp_send_vec((char *) &rep, sizeof(struct ua_reply), sinp);
}

static void  udp_send_umlpars(const netid_t whofrom, struct sockaddr_in *sinp)
{
	struct	ua_umlreply	reply;
	char	badrep[1];

	if  (!find_remote(whofrom))
		goto  badret;
	reply.ua_umask = htons(orig_umask);
	reply.ua_padding = 0;
	reply.ua_ulimit = 0L;
	udp_send_vec((char *)&reply, sizeof(reply), sinp);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op(whofrom, "senduml");
	return;
 badret:
	/* Mark end with a null */

	badrep[0] = '\0';
	udp_send_vec(badrep, 1, sinp);
}

static void  udp_send_elist(const netid_t whofrom, struct sockaddr_in *sinp)
{
	int		rp = 0;
	char		**ep, *ei;
	char	reply[CL_SV_BUFFSIZE];
	extern	char	**xenviron;

	if  (!find_remote(whofrom))
		goto  badret;

	for  (ep = xenviron;  (ei = *ep);  ep++)  {
		unsigned  lng = strlen(ei) + 1;
		if  (lng + rp > CL_SV_BUFFSIZE)  {
			udp_send_vec(reply, rp, sinp);
			rp = 0;

			/* If variable is longer than CL_SV_BUFFSIZE
			   (maybe we should increase this but I'm
			   scared of people with their titchy max
			   UDP xfers) split up into chunks.  */

			if  (lng > CL_SV_BUFFSIZE)  {
				do  {
					udp_send_vec(ei, CL_SV_BUFFSIZE, sinp);
					ei += CL_SV_BUFFSIZE;
					lng -= CL_SV_BUFFSIZE;
				}  while  (lng > CL_SV_BUFFSIZE);
				if  (lng != 0)
					udp_send_vec(ei, (int) lng, sinp);
				continue;
			}
		}
		strcpy(&reply[rp], ei);
		rp += lng;
	}
	if  (rp > 0)
		udp_send_vec(reply, rp, sinp);
 badret:
	/* Mark end with a null */

	reply[0] = '\0';
	udp_send_vec(reply, 1, sinp);
	if  (tracing & TRACE_CLIOPEND)
		client_trace_op(whofrom, "send env");
}

static void  note_roamer(const netid_t whofrom, struct ua_pal *inmsg, const int inlng)
{
	struct	hhash	*sender, *client;

	/* Ignore message if invalid length, unknown host or not
	   trusted host wot's sending it.  */

	if  (inlng != sizeof(struct ua_pal)  ||
	     ((whofrom != myhostid  &&  whofrom != localhostid  &&
	      !((sender = find_remote(whofrom)) && sender->rem.ht_flags & HT_TRUSTED))))
		return;

	if  ((client = find_remote(inmsg->uap_netid)))  {

		int_ugid_t	nuid;

		/* We've met this machine before, but possibly with a
		   different user. */

		if  (!(client->rem.ht_flags & HT_ROAMUSER))	/* Huh??? */
			return;
		if  (strcmp(client->dosname, inmsg->uap_wname) == 0)
			return;
		free(client->actname);
		free(client->dosname);
		client->actname = stracpy(inmsg->uap_name);
		client->dosname = stracpy(inmsg->uap_wname);
		if  ((nuid = lookup_uname(inmsg->uap_name)) == UNKNOWN_UID)  {
			client->rem.n_uid = Daemuid;
			client->rem.n_gid = Daemgid;
		}
		else  {
			client->rem.n_uid = nuid;
			client->rem.n_gid = lastgid;
		}
		client->lastaction = time((time_t *) 0);
		if  (tracing & TRACE_SYSOP)
			client_trace_op_name(whofrom, "Note roam user", client->actname);
	}
	else  {
		/* Ignore it unless we know about the windows user */

		if  (!new_roam_name(inmsg->uap_netid, &client, inmsg->uap_wname))
			return;
		client->flags = UAL_OK;
		if  (tracing & TRACE_SYSOP)
			client_trace_op_name(whofrom, "Note new roam user", inmsg->uap_wname);
	}

	tell_sched_roam(inmsg->uap_netid, client->actname, prin_gname(client->rem.n_gid));
}

/* Called from API when re-register option set.  */

void  tell_myself(struct hhash *frp)
{
	struct	ua_pal  palsmsg;
	init_palsmsg(&palsmsg, frp);
	udp_send_to((char *) &palsmsg, sizeof(palsmsg), myhostid);
}

/* This is mainly for dosbtwrite */

static void  answer_asku(const netid_t whofrom, struct ua_pal *inmsg, const int inlng, struct sockaddr_in *sinp)
{
	int	nu = 0, cnt;
	struct	hhash	*hp;
	struct	ua_asku_rep	reply;

	BLOCK_ZERO(&reply, sizeof(reply));

	if  (inlng != sizeof(struct ua_pal)  ||
	     (whofrom != myhostid && whofrom != localhostid  &&  !((hp = find_remote(whofrom))  &&  hp->rem.ht_flags & HT_TRUSTED)))
		goto  dun;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)  {
			if  (!(hp->rem.ht_flags & HT_DOS))
				continue;
			if  (hp->flags != UAL_OK)
				continue;
			if  (hp->rem.ht_flags & HT_ROAMUSER)  {
				if  (strcmp(hp->actname, inmsg->uap_name) != 0)
					continue;
			}
			else  if  (strcmp(hp->dosname, inmsg->uap_name) != 0)
				continue;
			reply.uau_ips[nu++] = hp->rem.hostid;
			if  (nu >= UAU_MAXU)
				goto  dun;
		}
	}
 dun:
	reply.uau_n = htons(nu);
	udp_send_vec((char *) &reply, sizeof(reply), sinp);
	if  (tracing & TRACE_SYSOP)
		client_trace_op_name(whofrom, "asku", inmsg->uap_name);
}

static void  answer_askall(const netid_t whofrom, struct ua_pal *inmsg, const int inlng)
{
	int	cnt;
	struct	hhash	*hp;
	struct	ua_pal	reply;

	BLOCK_ZERO(&reply, sizeof(reply));
	reply.uap_op = SV_SV_LOGGEDU;

	if  (inlng != sizeof(struct ua_pal)  ||
	     (whofrom != myhostid && whofrom != localhostid  &&
	      !((hp = find_remote(whofrom))  &&  hp->rem.ht_flags & HT_TRUSTED)))
		return;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)  {
			if  (!(hp->rem.ht_flags & HT_ROAMUSER))
				continue;
			if  (hp->flags != UAL_OK)
				continue;
			reply.uap_netid = hp->rem.hostid;
			strncpy(reply.uap_name, hp->actname, UIDSIZE);
			strncpy(reply.uap_grp, prin_gname(hp->rem.n_gid), UIDSIZE);
			strncpy(reply.uap_wname, hp->dosname, UIDSIZE);
			udp_send_to((char *) &reply, sizeof(reply), whofrom);
		}
	}
	if  (tracing & TRACE_SYSOP)
		client_trace_op(whofrom, "askall done");
}

void  send_askall()
{
	int	cnt;
	struct	ua_pal	msg;

	BLOCK_ZERO(&msg, sizeof(msg));
	msg.uap_op = SV_SV_ASKALL;

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	hhash	*hp;
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)
			if  ((hp->rem.ht_flags & (HT_DOS|HT_TRUSTED)) == HT_TRUSTED)
				udp_send_to((char *) &msg, sizeof(msg), hp->rem.hostid);
	}
}

/* Respond to keep alive messages - we rely on "find_remote" updating
   the last access time.  */

static void  tickle(const netid_t whofrom, struct sockaddr_in *sinp)
{
	struct	hhash	*frp = find_remote(whofrom);
	char	repl = XBNQ_OK;
	if  (frp)  {
		udp_send_vec(&repl, sizeof(repl), sinp);
		if  (tracing & TRACE_CLIOPEND)
			client_trace_op(whofrom, "tickle");
	}
}

void  process_ua()
{
	int	datalength;
	netid_t	whofrom;
	LONG	pmsgl[CL_SV_BUFFSIZE/sizeof(LONG)]; /* Force to long */
	char	*pmsg = (char *) pmsgl;
	struct	sockaddr_in	*sinp;
#ifdef	STRUCT_SIG
	struct	sigstruct_name  zch;
#endif
	struct	sockaddr_in	sin;
	SOCKLEN_T		sinl = sizeof(sin);
	sinp = &sin;

#ifdef	STRUCT_SIG
	zch.sighandler_el = SIG_IGN;
	sigmask_clear(zch);
	zch.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(QRFRESH, &zch, (struct sigstruct_name *) 0);
#else
	signal(QRFRESH, SIG_IGN);
#endif
	if  ((datalength = recvfrom(uasock, pmsg, sizeof(pmsgl), 0, (struct sockaddr *) sinp, &sinl)) < 0)
		return;
	whofrom = sin.sin_addr.s_addr;

	switch  (pmsg[0])  {
	case  UAL_ENQUIRE:
	case  UAL_LOGIN:
	case  UAL_LOGOUT:
	case  UAL_OK:		/* Actually these are redundant */
	case  UAL_NOK:
		udp_login(whofrom, (struct ua_login *) pmsg, datalength, sinp);
		return;
	case  UAL_NEWGRP:
	case  UAL_INVG:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "newgrp");
		udp_newgrp(whofrom, (struct ua_login *) pmsg, datalength, sinp);
		return;
	case  CL_SV_UENQUIRY:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendperms");
		udp_send_perms(whofrom, pmsg, datalength, sinp);
		return;
	case  CL_SV_ULIST:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendulist");
		udp_send_uglist(sinp, gen_ulist);
		return;
	case  CL_SV_GLIST:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendglist");
		udp_send_uglist(sinp, gen_glist);
		return;
	case  CL_SV_VLIST:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendvlist");
		udp_send_vlist(whofrom, pmsg, datalength, sinp);
		return;
	case  CL_SV_CILIST:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendcilist");
		udp_send_cilist(whofrom, sinp);
		return;
	case  CL_SV_HLIST:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendhlist");
		udp_send_hlist(whofrom, pmsg, datalength, sinp);
		return;
	case  CL_SV_UMLPARS:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "senduml");
		udp_send_umlpars(whofrom, sinp);
		return;
	case  CL_SV_ELIST:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "sendelist");
		udp_send_elist(whofrom, sinp);
		return;
	case  SV_SV_LOGGEDU:
		if  (tracing & TRACE_SYSOP)
			client_trace_op(whofrom, "noteroam");
		note_roamer(whofrom, (struct ua_pal *) pmsg, datalength);
		return;
	case  SV_SV_ASKU:
		if  (tracing & TRACE_SYSOP)
			client_trace_op(whofrom, "asku");
		answer_asku(whofrom, (struct ua_pal *) pmsg, datalength, sinp);
		return;
	case  SV_SV_ASKALL:
		if  (tracing & TRACE_SYSOP)
			client_trace_op(whofrom, "askall");
		answer_askall(whofrom, (struct ua_pal *) pmsg, datalength);
		return;
	case  CL_SV_KEEPALIVE:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "keepalive");
		tickle(whofrom, sinp);
		return;
	default:
		if  (tracing & TRACE_CLIOPSTART)
			client_trace_op(whofrom, "data");
		udp_job_process(whofrom, pmsg, datalength, sinp);
		return;
	}
}

/* Tell the client we think it should wake up */

static void  send_prod(struct pend_job *pj)
{
	int	sockfd, tries;
	char	prodit[1];
	struct	sockaddr_in	serv_addr, cli_addr;

	prodit[0] = SV_CL_TOENQ;
	BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = pj->clientfrom;
	serv_addr.sin_port = uaportnum;

	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port = 0;

	/* We don't really need the cli_addr but we are obliged to
	   bind something.  The remote uses our standard port.  */

	for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
		if  ((sockfd = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)
			return;
		if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
			close(sockfd);
			return;
		}
		if  (sendto(sockfd, prodit, sizeof(prodit), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)  {
			close(sockfd);
			return;
		}
		close(sockfd);
	}
	if  (tracing & TRACE_SYSOP)
		client_trace_op(pj->clientfrom, "Prod");
}

/* See which UDP ports seem to have dried up Return time of next alarm.  */

unsigned  process_alarm()
{
	time_t	now = time((time_t *) 0);
	unsigned  mintime = 0, nexttime;
	int	prodtime, killtime, cnt;

	for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
		if  (pend_list[cnt].clientfrom != 0L)  {
			struct	pend_job  *pj = &pend_list[cnt];
			prodtime = pj->lastaction + pj->timeout - now;
			killtime = prodtime + pj->timeout;
			if  (killtime < 0)  {
				abort_job(pj); /* Must have died */
				continue;
			}
			if  (prodtime <= 0)  {
				send_prod(pj);
				nexttime = killtime;
			}
			else
				nexttime = prodtime;
			if  (nexttime != 0  &&  (mintime == 0 || mintime > nexttime))
				mintime = nexttime;
		}

	/* Apply timeouts to stale connections.  */

	for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
		struct	hhash	*hp;
	redohash:		/* Hash chain gets mangled by do_logout */
		for  (hp = nhashtab[cnt];  hp;  hp = hp->hn_next)
			if  (hp->rem.ht_flags & HT_DOS)  {
				long	tdiff = (long) (hp->lastaction + hp->timeout) - now;
				if  (tdiff <= 0)  {
					if  (tracing & TRACE_SYSOP)
						client_trace_op_name(hp->rem.hostid, "Force logout", hp->actname);
					do_logout(hp);
					goto  redohash;
				}
				else  if  (mintime == 0 || mintime > (unsigned) tdiff)
					mintime = (unsigned) tdiff;
			}
	}
	return  mintime;
}
