#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "btqw.h"
#include <string.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

void    endsync(const netid_t);

inline	HWND	mainhWnd()
{
	return  AfxGetApp()->m_pMainWnd->m_hWnd;
}                                               

inline	const char	*appuser()
{
	return  ((CBtqwApp *)AfxGetApp())->m_username;
}

inline  const char  *appgroup()
{
	return  ((CBtqwApp *)AfxGetApp())->m_groupname;
}

netmsg::netmsg(const unsigned code)
{
	memset((void *) this, '\0', sizeof(netmsg));
	hdr.code = htons(code);
	hdr.length = htons(sizeof(netmsg));
	hdr.hostid = Locparams.myhostid;
}	

// Get hold of connection to given TCP port.
// Port number is already htons-ified

static  SOCKET  tcp_connect(const netid_t h, const short port)
{
	SOCKET	sk;
	sockaddr_in	sin;
	
	if  ((sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		return INVALID_SOCKET;

	sin.sin_family = AF_INET;
	sin.sin_port = port;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = h;

	if  (connect(sk, (sockaddr *) &sin, sizeof(sin)) != 0)  {
		closesocket(sk);
		return INVALID_SOCKET;
	}
	return  sk;
}

//  Send a probe - if we don't succeed generate a string ID ready for
//  a message box.                 

#define	UDP_TRIES	3

static	UINT	probe_send(const netid_t h, netmsg &msg)
{
	short	tries;
	SOCKET	sockfd;
	sockaddr_in	serv_addr, cli_addr;

	memset((void *) &serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = h;
	serv_addr.sin_port = Locparams.pportnum;

	memset((void *)&cli_addr, '\0', sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//	We don't really need the cli_addr but we are obliged to bind something.
	//	The remote uses our "pportnum".

	for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
		if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
			return  IDP_PROBE_SOCKERR;
		if  (bind(sockfd, (sockaddr *) &cli_addr, sizeof(cli_addr)) != 0)  {
			closesocket(sockfd);
			return	IDP_PROBE_BINDERR;
		}
		if  (sendto(sockfd, (char *) &msg, sizeof(msg), 0, (sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)  {
			closesocket(sockfd);
			return  0;
		}
		closesocket(sockfd);
	}
    
    if  (((CBtqwApp *) AfxGetApp())->m_warnoffline)
		return  IDP_PROBE_SENDERR;
	return  0;
}                               

// Receive probe from a Unix host

static	int	probe_recv(netmsg &msg)
{
	if  (recvfrom(Locparams.probesock, (char *) &msg, sizeof(netmsg), 0, (sockaddr *) 0, (int *) 0) <= 0)
		return  -1;
	return  0;
}

//	Try to attach to remote machine which may already be running.

void	remote::conn_attach()
{
	SOCKET	sk;
	if  ((sk = tcp_connect(hostid, Locparams.lportnum)) != INVALID_SOCKET)  {
		current_q.alloc(this);
		if  (WSAAsyncSelect(sk, mainhWnd(), WM_NETMSG_ARRIVED, FD_READ) != 0)  {
			int  code = WSAGetLastError();
			AfxMessageBox(IDP_ASYNCSEL, MB_OK|MB_ICONSTOP);
		}	
		sockfd = sk;
		ht_flags |= HT_ISCLIENT;
		is_sync = NSYNC_NONE;
		lastwrite = time((time_t *) 0);
		Locparams.Netsync_req++;
	}	
}

//	Initiate connection by doing UDP probe first.

void	remote::probe_attach()
{
	netmsg	pmsg(N_CONNECT);
	UINT	pres = probe_send(hostid, pmsg);
	if  (pres != 0)  {
		AfxMessageBox(pres, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	pending_q.alloc(this);
	is_sync = NSYNC_NONE;
	ht_flags |= HT_ISCLIENT;
	lastwrite = time((time_t *) 0);
}

//  If someone else speaks to us, generate an appropriate response.

void  reply_probe()
{
	netmsg	inmsg;
	if  (probe_recv(inmsg) != 0)
		return;
	netid_t  whofrom = inmsg.hdr.hostid;
	if  (whofrom == 0L  ||	whofrom == Locparams.myhostid)
		return;

	switch  (ntohs(inmsg.hdr.code))  {
	default:
		return;		// Forget it

	case  N_CONNECT:

		//	Probe connect - just bounce it back
	{
		netmsg	outmsg(N_CONNOK);
		probe_send(whofrom, outmsg);
		return;
	}

	case  N_CONNOK:

		//	Connection ok - forget it if we weren't interested
		//	in that processor (possibly because it's ancient).

		remote	*rp;
		if  (rp = pending_q.find(whofrom))  {
			rp->conn_attach();
			pending_q.free(rp);
			netsync();
		}	
		return;
	}
}

//	Attach remote, either immediately, or by doing probe operation first

void	remote::rattach()
{
	if  (hostid == 0L  ||  hostid == Locparams.myhostid)
		return;
	if  (ht_flags & HT_PROBEFIRST)
		probe_attach();
	else
		conn_attach();
}

//	Attach hosts if possible

void	attach_hosts()
{
	extern	remote	*nhashtab[];
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)
			np->rattach();
}

static	void	deallochost(remote  *rp)
{
	Jobs().net_jclear(rp->hostid);
	Vars().net_vclear(rp->hostid);
//	WSAAsyncSelect(rp->sockfd, mainhWnd(), 0, 0);
	closesocket(rp->sockfd);
	if  (rp->is_sync != NSYNC_OK)
		Locparams.Netsync_req--;
	current_q.free(rp);
}

//	Write to socket, but if we get some error, treat connection
//	as down.  Return 0 in that case so we can break out of loops.

static	int	chk_write(remote *rp, char *buffer, const int length)
{
	int	 lleft = length, nbytes;
	do  {
		if  ((nbytes = send(rp->sockfd, buffer, lleft, 0)) < 0)  {
			int  code = WSAGetLastError();
			deallochost(rp);
			return  0;
		}
		lleft -= nbytes;
		buffer += nbytes;
	}  while  (lleft > 0);
	rp->lastwrite = time((time_t *) 0);
	return  1;
}

void	clearhost(const netid_t netid)
{
	remote	*rp;
	if  (rp = pending_q.find(netid))
		pending_q.free(rp);
	if  (rp = current_q.find(netid))
		deallochost(rp);
}

//	Broadcast a message to all known hosts

void	broadcast(char *msg, const int size)
{
	current_q.setlast();
	remote	*rp;
	while  (rp = current_q.prev())
		chk_write(rp, msg, size);
}

void	net_recvlockreq(remote *rp)
{
	netmsg	&nm = *(netmsg *) rp->buffer;

	//  Just reply OK to everything as we have nothing to lock

	if  (ntohs(nm.hdr.code) ==  N_WANTLOCK)  {
		netmsg  onm(N_REMLOCK_OK);
		chk_write(rp, (char *) &onm, sizeof(onm));
	}	
}

//	Tell our friends byebye

void	netshut()
{
	netmsg  rq(N_SHUTHOST);
	broadcast((char *) &rq, sizeof(rq));
	current_q.setlast();
	remote	*rp;
	while  (rp = current_q.prev())
		deallochost(rp);
}

//	Tell one host goodbye

void	shut_host(netid_t hostid)
{
	remote	*rp;
	if  (rp = current_q.find(hostid))  {
		netmsg	rq(N_SHUTHOST);
		chk_write(rp, (char *) &rq, sizeof(rq));
	}	
}

//	Job status changes.

void	job_statrecvbcast(remote *rp)
{
	jobstatmsg	&jsm = *(jobstatmsg *)rp->buffer;
    jident	ji;
    jid_unpack(ji, jsm.jid);
	Jobs().statchjob(ji, jsm.runhost, ntohl(jsm.nexttime), ntohl(jsm.lastpid), ntohs(jsm.lastexit), jsm.prog);
}

//  Receive messages about jobs being forced, deleted or boq-ified

void	job_imessrecvbcast(remote *rp)
{
	jobcmsg	&jcm = *(jobcmsg *) rp->buffer;
    jident	ji;
    jid_unpack(ji, jcm.jid);
    switch  (ntohs(jcm.hdr.code))  {
	case  J_BOQ:
		Jobs().boqjob(ji);		
        break;
	case  J_DELETED:
		Jobs().deletedjob(ji);		
        break;
	case  J_BFORCED:
		Jobs().forced(ji, 1);		
        break;
	case  J_BFORCEDNA:
		Jobs().forced(ji, 0);		
        break;
	}
}

//  Receive broadcast about a job not including strings

void	job_hrecvbcast(remote *rp)
{
	jobhnetmsg	&jhnm = *(jobhnetmsg *)rp->buffer;
	jhnm.hdr.code = ntohs(jhnm.hdr.code);
	Btjob  Jreq;
	jobh_unpack(Jreq, jhnm);
	Jobs().changedjobhdr(Jreq);
}

//	Handle broadcasts about changes to jobs including
//  Job strings.

void	job_recvbcast(remote *rp)
{
	jobnetmsg	&jnm = *(jobnetmsg *)rp->buffer;
	Btjob	Jreq;
	job_unpack(Jreq, jnm);
	switch  (ntohs(jnm.hdr.hdr.code))  {
	case  J_CREATE:
		Jobs().createdjob(Jreq);
		break;
	case  J_BCHANGED:
		Jobs().changedjob(Jreq);
		break;                                         
	}	
}

//	Receive details of remote assignments.

void	recv_remasses(remote *rp)
{
	rassmsg  &rm = *(rassmsg *)rp->buffer;
	jident  ji;
	jid_unpack(ji, rm.jid);
	Jobs().remassjob(ji, unsigned(rm.flags), unsigned(ntohs(rm.status)));
}

//	Send update request about a job to the owning machine

void	job_sendupdate(const Btjob &newjob, const unsigned code)
{
	remote	*rp;

	if  (!(rp = current_q.find(newjob.hostid)))
		return;		//	N_HOSTOFFLINE??

	if  (code == J_HCHANGED)  {
		jobhnetmsg	jhm;
		jobh_pack(jhm, newjob);
		jhm.hdr.code = htons(code);
		jhm.hdr.hostid = Locparams.myhostid;
		jhm.hdr.pid = 0L;
		strcpy(jhm.hdr.muser, appuser());
		strcpy(jhm.hdr.mgroup, appgroup());
		chk_write(rp, (char *) &jhm, sizeof(jhm));
		//  Host died???
	}
	else  {
		jobnetmsg	jnm;
		unsigned  jlen = job_pack(jnm, newjob);
		jnm.hdr.hdr.code = htons(code);
		jnm.hdr.hdr.hostid = Locparams.myhostid;
		jnm.hdr.hdr.pid = 0L;
		strcpy(jnm.hdr.hdr.muser, appuser());
		strcpy(jnm.hdr.hdr.mgroup, appgroup());
		chk_write(rp, (char *) &jnm, jlen);
		//  Host died???
	}
}

//	Other end of above routine header version
//  Dummy routine - no jobs on DOS vn.

//void	job_recvhupdate(remote *rp)
//{
//}

//	Other end of send update routine w.s.m. version
//  Dummy routine - no jobs on DOS vn.

void	job_recvupdate(remote *rp)
{
}

//	Send update of modes only

void	job_sendmdupdate(const Btjob &oldjob, const Btjob &newjob)
{
	remote	*rp;

	if  (!(rp = current_q.find(oldjob.hostid)))
		return;		//	N_HOSTOFFLINE??

	jobhnetmsg	jhm;
	
	//	There's a certain amount of duplication here but this doesn't happen
	//  often enough to make a special meal of it.

	jobh_pack(jhm, oldjob);
	mode_pack(jhm.nm_mode, newjob.bj_mode);
	jhm.hdr.code = htons(J_CHMOD);
	jhm.hdr.hostid = Locparams.myhostid;
	jhm.hdr.pid = 0L;
	strcpy(jhm.hdr.muser, appuser());
	strcpy(jhm.hdr.mgroup, appgroup());
	chk_write(rp, (char *) &jhm, sizeof(jhm));
	//  Host died??
}

//	Send update of user or group only

void	job_sendugupdate(const Btjob &oldjob, const unsigned op, const CString &newog)
{
	remote	*rp;

	if  (!(rp = current_q.find(oldjob.hostid)))
		return;		//	N_HOSTOFFLINE??

    jugmsg	jm;
	jm.hdr.code = htons((unsigned short) op);
	jm.hdr.length = htons(sizeof(jm));
	jm.hdr.hostid = Locparams.myhostid;
	jm.hdr.pid = 0L;
	strcpy(jm.hdr.muser, appuser()); 
	strcpy(jm.hdr.mgroup, appgroup());
	jid_pack(jm.jid, oldjob);
	strncpy(jm.newug, (const char *) newog, UIDSIZE);
	jm.newug[UIDSIZE] = '\0';
	chk_write(rp, (char *) &jm, sizeof(jm));
	// Worry about dying host???
}

//	Process that at the remote end
//  Dummy routine only.

void	job_recvugupdate(remote *rp)
{
}

//	Transmit a message about a job to a specific machine.
//	This is delete/force to the originating machine, abort to a machine
//	running a job.

void  job_message(const Btjob &jp, const unsigned op, const unsigned long arg)
{
	remote		*rp;

	if  (!(rp = current_q.find(jp.hostid)))
		return;  //	N_HOSTOFFLINE ??
	jobcmsg		jcm;
	jcm.hdr.code = htons((unsigned short) op);
	jcm.hdr.length = htons(sizeof(jcm));
	jcm.hdr.hostid = Locparams.myhostid;
	jcm.hdr.pid = htonl(1L); 	//  Psuedo-pid as non-internal to scheduler
	strcpy(jcm.hdr.muser, appuser()); 
	strcpy(jcm.hdr.mgroup, appgroup());
	jid_pack(jcm.jid, jp);
	jcm.param = htonl(arg);
	chk_write(rp, (char *) &jcm, sizeof(jcm));
}

void	sync_single(const Btjob &jp)
{
	remote	*rp;
	if  (!(rp = current_q.find(jp.hostid)))
		return;
	netmsg	rq(N_SYNCSINGLE);
	rq.arg = htonl(jp.slotno);
	chk_write(rp, (char *) &rq, sizeof(rq));
}

//	Receive above routine's efforts at the other end.
//  Dummy routine as we don't have any jobs to do it with.

void	job_recvmessage(remote *rp)
{
}

//	Receive remote notify (dummy routine)

void	job_recvnote(remote *rp)
{
}

//	Unscramble variable broadcast.

void	var_recvbcast(remote *rp)
{
	varnetmsg	&vnm = *(varnetmsg *) rp->buffer;
	Btvar	invar;
	var_unpack(invar, vnm);
		
	switch  (ntohs(vnm.hdr.code))  {
	case  V_CREATE:
		Vars().createdvar(invar);
		return;
	case  V_ASSIGNED:
		Vars().assignedvar(invar);
		return;		
	case  V_CHMOGED:
		Vars().chmogedvar(invar);
		return;		
	case  V_DELETED:
		Vars().deletedvar(invar);
		return;		
	case  V_RENAMED:
		Vars().renamedvar(invar, invar.var_name);
		return;
	}		
}

//	Send update request about a variable to the owning machine
//	Applies to assignments, comment change or chmod

void	var_sendupdate(const Btvar &oldvar, const Btvar &newvar, const unsigned op)
{
	varnetmsg	vm;

	var_pack(vm, oldvar);
	vm.hdr.code = htons(op);
	vm.hdr.hostid = Locparams.myhostid;

	//	Bits of duplication here.....

	switch  (op)  {
	case  V_ASSIGN:
		if  ((vm.nm_consttype = (unsigned char) newvar.var_value.const_type) == CON_STRING)
			strncpy(vm.nm_un.nm_string, newvar.var_value.con_string, BTC_VALUE);
		else
			vm.nm_un.nm_long = htonl(newvar.var_value.con_long);
		break;
	case  V_CHCOMM:
		strncpy(vm.nm_comment, newvar.var_comment, BTV_COMMENT);
		break;
	case  V_CHMOD:
		mode_pack(vm.nm_mode, newvar.var_mode);
		break;
	}
    
    remote  *rp;
	if  (!(rp = current_q.find(oldvar.hostid)))
		return;	//  Display offline??

	vm.hdr.pid = 0;
	strcpy(vm.hdr.muser, appuser());
	strcpy(vm.hdr.mgroup, appgroup());
	chk_write(rp, (char *) &vm, sizeof(vm));	//  Display died??
}

//	Unscramble that at t'other end
//  Dummy routine only.

void	var_recvupdate(remote *rp)
{
}

//	Send update of variable user or group only

void  var_sendugupdate(const Btvar &oldvar, const unsigned code, const CString &newug)
{
	remote	*rp;

	if  (!(rp = current_q.find(oldvar.hostid)))
		return;

	vugmsg	vm;
	vm.hdr.code = htons((unsigned short) code);
	vm.hdr.length = htons(sizeof(vm));
	vm.hdr.hostid = Locparams.myhostid;
	vm.hdr.pid = 0;
	strcpy(vm.hdr.muser, appuser());
	strcpy(vm.hdr.mgroup, appgroup());
	vid_pack(vm.vid, oldvar);
	strncpy(vm.newug, (const char *) newug, UIDSIZE);
	vm.newug[UIDSIZE] = '\0';
	chk_write(rp, (char *) &vm, sizeof(vm));
}

//	Unscramble that at t'other end (dummy routine)
//  We shouldn't get here but do something sensible if we do

void	var_recvugupdate(remote *rp)
{
}

//	Send list of command interpreters
//  This is a dummy routine because we don't have any
//  currently.

int	ci_send(remote *rp)
{
	cimsg	cm;

	memset((void *) &cm, '\0', sizeof(cm));
	cm.hdr.code = htons(N_CISEND);
	cm.hdr.length = htons((unsigned short) sizeof(cm));
	cm.hdr.hostid = Locparams.myhostid;
	cm.numb = 0L;
	if  (!chk_write(rp, (char *) &cm, sizeof(cm)))
			return  0;
	return  1;
}

//	Look around for remote machines we haven't got details of jobs
//  or variables of.

void	netsync(void)
{
	remote	*rp;

	current_q.setfirst();
	while  (rp = current_q.next())
		if  (rp->is_sync == NSYNC_REQ)
			return;
	current_q.setfirst();
	while  (rp = current_q.next())  {
		if  (rp->is_sync == NSYNC_NONE)  {
			netmsg	rq(N_REQSYNC);
			if  (chk_write(rp, (char *) &rq, sizeof(rq)))
				rp->is_sync = NSYNC_REQ;
			//	Return so we don't get indigestion
		 	//	with everyone talking at once.
			return;
		}
	}
}                                     

void	send_endsync(const netid_t hostid)
{
	remote	*rp;
	if  ((rp = current_q.find(hostid)))  {
		netmsg	rq(N_ENDSYNC);
		(void) chk_write(rp, (char *) &rq, sizeof(rq));
	}
}

void	endsync(const netid_t netid)
{
	remote	*rp;

	if  ((rp = current_q.find(netid))  &&  rp->is_sync != NSYNC_OK)  {
		rp->is_sync = NSYNC_OK;
		Locparams.Netsync_req--;
		((CBtqwApp *)AfxGetApp())->m_appjoblist.deskel_jobs(netid);
	}
	if  (Locparams.Netsync_req > 0)
		netsync();
}

void	net_recv(remote *rp)
{
	netmsg	&imsg = *(netmsg *) rp->buffer;
	msghdr	&hdr = imsg.hdr;

	imsg.hdr.code = ntohs(hdr.code);
	imsg.hdr.length = ntohs(hdr.length);
	imsg.hdr.pid = ntohl(hdr.pid);
	imsg.arg = ntohl(imsg.arg);

	switch  (imsg.hdr.code)  {
	case  N_TICKLE:
		return;
	case  N_SHUTHOST:
		clearhost(imsg.hdr.hostid);
		return;
	case  N_REQSYNC:
		send_endsync(imsg.hdr.hostid);
		return;
	case  N_ENDSYNC:
		endsync(imsg.hdr.hostid);
		return;
	case  N_REQREPLY:
		((CBtqwApp *)AfxGetApp())->m_pMainWnd->PostMessage(WM_NETMSG_MESSAGE, WPARAM(imsg.arg));
		return;
	}
}

void	remote_recv(remote *rp)
{
	//  Read in the message header which may come in several bits
	
	if  (rp->byteoffset < sizeof(msghdr))  {
		if  (rp->bytesleft == 0)
			rp->bytesleft = sizeof(msghdr);
		int	nbytes = recv(rp->sockfd, &rp->buffer[rp->byteoffset], (int) rp->bytesleft, 0);
		if  (nbytes <= 0)
			return;                                 
		rp->bytesleft -= nbytes;
		rp->byteoffset += nbytes;
		return;
	}
	
	//  We previously read the header, so read the rest of the message
	//  and dispose of it when it is complete.
	
	msghdr	&hdr = *(msghdr *)rp->buffer;
	if  (rp->bytesleft == 0)
		rp->bytesleft = ntohs(hdr.length) - sizeof(msghdr);
	
	int	nbytes = recv(rp->sockfd, &rp->buffer[rp->byteoffset], (int) rp->bytesleft, 0);
	if  (nbytes <= 0)
		return;
	rp->bytesleft -= nbytes;
	rp->byteoffset += nbytes;
	if  (rp->bytesleft != 0)
		return;					//  And get another message
    rp->byteoffset = 0;			//  Before we forget

	switch  (ntohs(hdr.code))  {
	case  N_TICKLE:
	case  N_SHUTHOST:
	case  N_REQSYNC:
	case  N_ENDSYNC:
	case  N_REQREPLY:
		net_recv(rp);
		return;

	case  N_WANTLOCK:
	case  N_UNLOCK:
		net_recvlockreq(rp);
		return;

	case  N_RJASSIGN:
		recv_remasses(rp);
		return;

	case  N_REMLOCK_OK:
	case  N_REMLOCK_PRIO:
		return;

	case  J_CREATE:
	case  J_BCHANGED:
		job_recvbcast(rp);
		return;

	case  J_CHOWN:
	case  J_CHGRP:
		job_recvugupdate(rp);
		return;

	case  N_DOCHARGE:
	case  J_DELETE:
	case  J_KILL:
	case  J_FORCE:
	case  J_FORCENA:
		job_recvmessage(rp);
		return;
    
    case  J_CHANGED:
		job_recvupdate(rp);
		return;

	case  J_HCHANGED:
	case  J_CHMOD:
		job_hrecvbcast(rp);	// Bodge for single sync case
		return;

	case  J_BHCHANGED:
	case  J_CHMOGED:
		job_hrecvbcast(rp);
		return;

	case  J_BOQ:
	case  J_DELETED:
	case  J_BFORCED:
	case  J_BFORCEDNA:
	case  J_PROPOSE:
	case  J_PROPOK:
		job_imessrecvbcast(rp);
		return;

	case  J_CHSTATE:
		job_statrecvbcast(rp);
		return;

	case  J_RNOTIFY:
		job_recvnote(rp);
		return;

	case  V_CREATE:		// This is a broadcast if it gets here
	case  V_ASSIGNED:	// Broadcast of result of assign
	case  V_CHMOGED:	// Broadcast of result of chmod/chown/chgrp
	case  V_DELETED:	// Broadcast of result of delete
	case  V_RENAMED:	// Broadcast of result of rename
		var_recvbcast(rp);
		return;

	case  V_ASSIGN:		// Request to assign
	case  V_CHMOD:		// Request to chmod
	case  V_CHCOMM:		// Request to change comment
		var_recvupdate(rp);
		return;

	case  V_CHOWN:		// Passed on request to chown/chgrp
	case  V_CHGRP:
		var_recvugupdate(rp);
		return;
	}
}

// Accept messages passed on from mainframe.cpp

void	net_recvmsg(WPARAM sockfd, LPARAM code)
{
	unsigned  eventcode = WSAGETSELECTEVENT(code);
	if  (eventcode & FD_CLOSE)  {
		remote	*rp = find_host(SOCKET(sockfd));
		if  (rp)
			deallochost(rp);
		return;
	}
	if  (eventcode & FD_READ)  {
		remote  *rp;
		if	(rp = find_host(SOCKET(sockfd)))
				remote_recv(rp);
	}
}			

void	net_recvprobe(WPARAM, LPARAM)
{
#ifdef	FOOOO   
	fd_set	fds;
	timeval  tot;
	memset((void *) &tot, '\0', sizeof(tot));
	FD_ZERO(&fds);
	FD_SET(Locparams.probesock, &fds);
	do  {
#endif
		reply_probe();
#ifdef	FOOOO
		FD_ZERO(&fds);
		FD_SET(Locparams.probesock, &fds);
	}  while  (select(FD_SETSIZE, &fds, NULL, NULL, &tot) > 0);	
#endif
}
