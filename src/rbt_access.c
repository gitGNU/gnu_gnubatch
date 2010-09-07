/* rbt_access.c -- permissions and such for gbch-rbr

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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "files.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "services.h"

static	char	Filename[] = __FILE__;

const	char	Sname[] = GBNETSERV_PORT;

static	int	udpsock = -1;
static	struct	sockaddr_in	serv_addr, cli_addr;

#define	INITENV	30
#define	INCENV	20

#define	RTIMEOUT	5

static int  initsock(const netid_t hostid)
{
	int	sockfd;
	SHORT	portnum, udpproto;
	struct	servent	*sp;
	struct	protoent  *pp;
	char	*udp_protoname;

	if  (!((pp = getprotobyname("udp"))  || (pp = getprotobyname("UDP"))))  {
		print_error($E{No UDP Protocol});
		exit(E_NETERR);
	}
	udp_protoname = pp->p_name;
	udpproto = pp->p_proto;
	endprotoent();

	/* Get port number for this caper */

	if  (!(sp = env_getserv(Sname, udp_protoname)))  {
		print_error($E{No xbnetserv UDP service});
		endservent();
		exit(E_NETERR);
	}
	portnum = sp->s_port;
	endservent();

	BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
	BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
	serv_addr.sin_family = cli_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = hostid;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = portnum;
	cli_addr.sin_port = 0;

	/* Save now in case of error.  */

	disp_arg[0] = ntohs(portnum);
	disp_arg[1] = hostid;

	if  ((sockfd = socket(AF_INET, SOCK_DGRAM, udpproto)) < 0)  {
		print_error($E{Cannot create UDP access socket});
		exit(E_NETERR);
	}
	if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
		print_error($E{Cannot bind UDP access socket});
		close(sockfd);
		exit(E_NETERR);
	}
	return  sockfd;
}

/* Unpack btuser from networked version.  */

static void  unpack_btuser(Btuser *dest, const Btuser *src)
{
	dest->btu_isvalid = src->btu_isvalid;
	dest->btu_minp = src->btu_minp;
	dest->btu_maxp = src->btu_maxp;
	dest->btu_defp = src->btu_defp;
	dest->btu_user = ntohl(src->btu_user);
	dest->btu_maxll = ntohs(src->btu_maxll);
	dest->btu_totll = ntohs(src->btu_totll);
	dest->btu_spec_ll = ntohs(src->btu_spec_ll);
	dest->btu_priv = ntohl(src->btu_priv);
	dest->btu_jflags[0] = ntohs(src->btu_jflags[0]);
	dest->btu_jflags[1] = ntohs(src->btu_jflags[1]);
	dest->btu_jflags[2] = ntohs(src->btu_jflags[2]);
	dest->btu_vflags[0] = ntohs(src->btu_vflags[0]);
	dest->btu_vflags[1] = ntohs(src->btu_vflags[1]);
	dest->btu_vflags[2] = ntohs(src->btu_vflags[2]);
}

static	RETSIGTYPE  asig(int n)
{
	return;			/* Don't do anything just return setting EINTR */
}

static int  udp_enquire(char *outmsg, const int outlen, char *inmsg, const int inlen)
{
#ifdef	STRUCT_SIG
	struct	sigstruct_name	za, zold;
#else
	RETSIGTYPE	(*oldsig)();
#endif
	int	inbytes;
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;
	if  (sendto(udpsock, outmsg, outlen, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
		disp_arg[1] = serv_addr.sin_addr.s_addr;
		print_error($E{Cannot send UDP packet});
		exit(E_NETERR);
	}
#ifdef	STRUCT_SIG
	za.sighandler_el = asig;
	sigmask_clear(za);
	za.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGALRM, &za, &zold);
#else
	oldsig = signal(SIGALRM, asig);
#endif
	alarm(RTIMEOUT);
	if  ((inbytes = recvfrom(udpsock, inmsg, inlen, 0, (struct sockaddr *) &reply_addr, &repl)) <= 0)  {
		disp_arg[1] = serv_addr.sin_addr.s_addr;
		print_error($E{Cannot receive UDP packet});
		exit(E_NETERR);
	}
	alarm(0);
#ifdef	STRUCT_SIG
	sigact_routine(SIGALRM, &zold, (struct sigstruct_name *) 0);
#else
	signal(SIGALRM, oldsig);
#endif
	return  inbytes;
}

/* Get details of account for given user, plus group name.  */

BtuserRef  remgetbtuser(const netid_t hostid, char *realuname, char *realgname)
{
	struct	ni_jobhdr	enq;
	struct	ua_reply	resp;
	static	Btuser		result;

	if  (udpsock < 0)
		udpsock = initsock(hostid);
	BLOCK_ZERO(&enq, sizeof(enq));
	enq.code = CL_SV_UENQUIRY;
	enq.joblength = htons(sizeof(enq));
	strncpy(enq.uname, realuname, UIDSIZE);
	udp_enquire((char *) &enq, sizeof(enq), (char *) &resp, sizeof(resp));
	strcpy(realgname, resp.ua_gname);
	unpack_btuser(&result, &resp.ua_perm);
	if  (!result.btu_isvalid)  {
		print_error($E{No such user on remote});
		exit(E_NETERR);
	}
	return	&result;
}

void  remgetuml(const netid_t hostid, USHORT *um, ULONG *ul)
{
	struct	ni_jobhdr	enq;
	struct	ua_umlreply	resp;

	if  (udpsock < 0)
		udpsock = initsock(hostid);
	BLOCK_ZERO(&enq, sizeof(enq));
	enq.code = CL_SV_UMLPARS;
	enq.joblength = htons(sizeof(enq));
	udp_enquire((char *) &enq, sizeof(enq), (char *) &resp, sizeof(resp));
	*um = ntohs(resp.ua_umask);
	*ul = ntohl(resp.ua_ulimit);
}

static int  eread(char *buff, const unsigned bufl)
{
	int	inbytes;
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;
	if  ((inbytes = recvfrom(udpsock, buff, bufl, 0, (struct sockaddr *) &reply_addr, &repl)) <= 0)  {
		print_error($E{Cannot receive UDP packet});
		exit(E_NETERR);
	}
	return  inbytes;
}

char **remread_envir(const netid_t hostid)
{
	int		inbytes;
	unsigned	ecount, emax;
	char		**result, *bufp, *ebuf;
	struct	ni_jobhdr	enq;
	char	buf[CL_SV_BUFFSIZE];

	if  (udpsock < 0)
		udpsock = initsock(hostid);
	BLOCK_ZERO(&enq, sizeof(enq));
	enq.code = CL_SV_ELIST;
	enq.joblength = htons(sizeof(enq));
	inbytes = udp_enquire((char *) &enq, sizeof(enq), buf, sizeof(buf));

	ecount = 0;
	emax = INITENV;
	result = (char **) malloc((INITENV + 1) * sizeof(char *));
	if  (!result)
		ABORT_NOMEM;
	for  (;;)  {
		bufp = buf;
		ebuf = &buf[inbytes];
		if  (bufp[0] == '\0')
			goto  finished;
		if  (ebuf[-1])  {	/*  Var must have been split */
			unsigned	msize = 2*CL_SV_BUFFSIZE;
			unsigned	lump;
			char		*nbuf, *nbp;
			if  (!(nbuf = malloc(2*CL_SV_BUFFSIZE)))
				ABORT_NOMEM;
			BLOCK_COPY(nbuf, buf, CL_SV_BUFFSIZE);
			lump = inbytes;
			for  (;;)  {
				nbp = &nbuf[lump];
				inbytes = eread(nbp, CL_SV_BUFFSIZE);
				if  (!nbp[inbytes-1])
					break;
				msize += CL_SV_BUFFSIZE;
				if  (!(nbuf = realloc(nbuf, msize)))
					ABORT_NOMEM;
				lump += inbytes;
			}
			if  (ecount >= emax)  {
				emax += INCENV;
				result = (char **) realloc((char *)result, (emax+1) * sizeof(char *));
				if  (!result)
					ABORT_NOMEM;
			}
			result[ecount++] = stracpy(nbuf);
			free(nbuf);
		}
		else  {
			while  (bufp < ebuf)  {
				if  (ecount >= emax)  {
					emax += INCENV;
					result = (char **) realloc((char *)result, (emax+1) * sizeof(char *));
					if  (!result)
						ABORT_NOMEM;
				}
				result[ecount++] = stracpy(bufp);
				bufp += strlen(bufp) + 1;
			}
		}
		inbytes = eread(buf, sizeof(buf));
	}
finished:
	result[ecount] = (char *) 0;
	return  result;
}
