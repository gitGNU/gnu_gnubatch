/* gbatch_open.c -- API function to open connection to server

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

#include <stdio.h>
#include <sys/types.h>
#include "gbatch.h"
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <pwd.h>
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <ctype.h>
#include "xbapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"
#include "files.h"
#include "services.h"

#ifndef	_NFILE
#define	_NFILE	64
#endif

#define	MAXFDS	(_NFILE / 3)

static	unsigned	api_max;
static	struct	api_fd	apilist[MAXFDS];
#ifdef	POLLSOCKETS
static	struct	pollfd	flist[MAXFDS];
#endif

int	gbatch_dataerror;

int  gbatch_write(const int fd, char *buff, unsigned size)
{
	int	obytes;
	while  (size != 0)  {
		if  ((obytes = write(fd, buff, size)) < 0)  {
			if  (errno == EINTR)
				continue;
			return  XB_BADWRITE;
		}
		size -= obytes;
		buff += obytes;
	}
	return  0;
}

int  gbatch_read(const int fd, char *buff, unsigned size)
{
	int	ibytes;
	while  (size != 0)  {
		if  ((ibytes = read(fd, buff, size)) < 0)  {
			if  (errno == EINTR)
				continue;
			return  XB_BADREAD;
		}
		size -= ibytes;
		buff += ibytes;
	}
	return  0;
}

int  gbatch_wmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  gbatch_write(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

int  gbatch_rmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  gbatch_read(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

struct api_fd *gbatch_look_fd(const int fd)
{
	struct	api_fd	*result;

	if  (fd < 0  ||  (unsigned) fd >= api_max)
		return  (struct api_fd *) 0;
	result = &apilist[fd];
	if  (result->sockfd < 0)
		return  (struct api_fd *) 0;
	return  result;
}

#if	(defined(FASYNC) && defined(F_SETOWN))
static struct api_fd *find_prod(const int fd)
{
	unsigned  cnt;
	for  (cnt = 0;  cnt < api_max;  cnt++)
		if  (apilist[cnt].prodfd == fd)
			return  &apilist[cnt];
	return  (struct api_fd *) 0;
}
#endif

int  gbatch_open(const char *hostname, const char *servname)
{
	int	portnum, sock, ret;
	unsigned	result;
	struct	api_fd	*ret_fd;
	netid_t	hostid;
	const	char	*serv;
	struct	passwd	*pw;
	struct	hostent	*hp;
	struct	servent	*sp;
	struct	sockaddr_in	sin;
	struct	api_msg	outmsg;
	char	servbuf[sizeof(DEFAULT_SERVICE) + 20];

	hp = gethostbyname(hostname);
	if  (!hp)  {
		endhostent();
		return  XB_INVALID_HOSTNAME;
	}
	hostid = * (netid_t *) hp->h_addr;
	endhostent();

	if  (servname)
		serv = servname;
	else  {
		char	*cp = getenv(ENV_SELECT_VAR);
		strcpy(servbuf, DEFAULT_SERVICE);
		if  (cp  &&  isdigit(*cp))
			strcat(servbuf, cp);
		serv = servbuf;
	}
	if  (!(sp = getservbyname(serv, "tcp")))
		sp = getservbyname(serv, "TCP");
	if  (!sp)  {
		endservent();
		return  servname? XB_INVALID_SERVICE: XB_NODEFAULT_SERVICE;
	}
	portnum = (SHORT) ntohs(sp->s_port);
	endservent();

	/* And now for the groovy socket stuff....  */

	sin.sin_family = AF_INET;
	sin.sin_port = htons(portnum);
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = hostid;


	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  XB_NOSOCKET;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sock);
		return  XB_NOCONNECT;
	}

	/* Allocate ourselves a "file descriptor" and stuff our stuff in it.  */

	for  (result = 0;  result < api_max;  result++)
		if  (apilist[result].sockfd < 0)
			goto  found;
	if  (++api_max >= MAXFDS)
		return  XB_NOMEM;
 found:
	ret_fd = &apilist[result];
	ret_fd->portnum = (SHORT) portnum;
	ret_fd->sockfd = (SHORT) sock;
	ret_fd->prodfd = -1;
	ret_fd->hostid = hostid;
	ret_fd->jobfn = (void (*)()) 0;
	ret_fd->varfn = (void (*)()) 0;
	ret_fd->jserial = 0;
	ret_fd->vserial = 0;
	ret_fd->bufmax = 0;
	ret_fd->queuename = (char *) 0;
	ret_fd->buff = (char *) 0;
	pw = getpwuid(geteuid());
	strncpy(ret_fd->username, pw? pw->pw_name: BATCHUNAME, UIDSIZE);
	ret_fd->username[UIDSIZE] = '\0';
	outmsg.code = API_SIGNON;
	strcpy(outmsg.un.signon.username, ret_fd->username);
	if  ((ret = gbatch_wmsg(ret_fd, &outmsg)))  {
		gbatch_close((int) result);
		return  ret;
	}
	if  ((ret = gbatch_rmsg(ret_fd, &outmsg)))  {
		gbatch_close((int) result);
		return  ret;
	}
	if  (outmsg.retcode != 0)  {
		gbatch_close((int) result);
		return  (SHORT) ntohs(outmsg.retcode);
	}
	return  (int) result;
}

int  gbatch_newgrp(const int fd, const char *name)
{
	int	ret;
	struct	api_fd	*fdp = gbatch_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XB_INVALID_FD;

	BLOCK_ZERO(&msg, sizeof(msg));
	msg.code = API_NEWGRP;
	strncpy(msg.un.signon.username, name, UIDSIZE);
	if  ((ret = gbatch_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = gbatch_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  XB_OK;
}

#if	(defined(FASYNC) && defined(F_SETOWN))
static void  procpoll(int fd)
{
	struct	api_fd	*fdp;
	struct	api_msg	imsg;
	SOCKLEN_T		repl = sizeof(struct sockaddr_in);
	struct	sockaddr_in	reply_addr;

	if  (!(fdp = find_prod(fd)))
		return;

	if  (recvfrom(fdp->prodfd, (char *) &imsg, sizeof(imsg), 0, (struct sockaddr *) &reply_addr, &repl) < 0)
		return;

	switch  (imsg.code)  {
	default:
		return;
	case  API_JOBPROD:
		if  (fdp->jobfn)
			(*fdp->jobfn)((int) (fdp - apilist));
		fdp->jserial = ntohl(imsg.un.r_reader.seq);
		return;
	case  API_VARPROD:
		if  (fdp->varfn)
			(*fdp->varfn)((int) (fdp - apilist));
		fdp->vserial = ntohl(imsg.un.r_reader.seq);
		return;
	}
}

static RETSIGTYPE  catchpoll(int n)
{
	unsigned	cnt;
	int		fd;

#ifdef	POLLSOCKETS
	unsigned	pcnt;

#ifdef	UNSAFE_SIGNALS
	signal(n, catchpoll);
#endif

	for  (pcnt = cnt = 0;  cnt < api_max;  cnt++)
		if  ((fd = apilist[cnt].prodfd) >= 0)  {
			flist[pcnt].fd = fd;
			flist[pcnt].events = POLLIN|POLLPRI|POLLERR;
			flist[pcnt].revents = 0;
			pcnt++;
		}
	while  (poll(flist, pcnt, 0) > 0)  {
		for  (cnt = 0;  cnt < pcnt;  cnt++)
			if  (flist[cnt].revents)
				procpoll(flist[cnt].fd);
	}
#else
	int	highfd = -1, nret;
	fd_set	ready;

#ifdef	UNSAFE_SIGNALS
	signal(n, catchpoll);
#endif

	for  (;;)  {
		FD_ZERO(&ready);
		for  (cnt = 0;  cnt < api_max;  cnt++)  {
			if  ((fd = apilist[cnt].prodfd) >= 0)  {
				FD_SET(fd, &ready);
				if  (fd > highfd)
					highfd = fd;
			}
		}
		nret = select(highfd+1, &ready, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);
		if  (nret <= 0)
			break;
		for  (cnt = 0;  nret > 0  &&  cnt <= (unsigned) highfd;  cnt++)
			if  (FD_ISSET(cnt, &ready))  {
				procpoll(cnt);
				nret--;
			}
	}
#endif
}

static int  setmon(struct api_fd *fdp)
{
	int	sockfd, pportnum, ret;
	const	char	*serv = MON_SERVICE;
	struct	servent	*sp;
#ifdef	STRUCT_SIG
	struct	sigstruct_name	z;
#endif
	int	protonum;
	struct	protoent	*pp;
	struct	sockaddr_in	sin;
	struct	api_msg	msg;

	if  (!(pp = getprotobyname("udp")) && !(pp = getprotobyname("UDP")))  {
		endprotoent();
		return  XB_NODEFAULT_SERVICE;
	}
	protonum = pp->p_proto;
	endprotoent();
	if  (!(sp = getservbyname(serv, "udp")))
		sp = getservbyname(serv, "UDP");
	if  (!sp)  {
		endservent();
		return  XB_NODEFAULT_SERVICE;
	}
	pportnum = sp->s_port;
	endservent();
	msg.code = API_REQPROD;
	if  ((ret = gbatch_wmsg(fdp, &msg)))
		return  ret;
	sin.sin_family = AF_INET;
	sin.sin_port = (SHORT) pportnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;

#ifdef	STRUCT_SIG
	z.sighandler_el = catchpoll;
	sigmask_clear(z);
	z.sigflags_el = SIGVEC_INTFLAG;
	sigact_routine(SIGIO, &z, (struct sigstruct_name *) 0);
#else
	signal(SIGIO, catchpoll);
#endif
	if  ((sockfd = socket(PF_INET, SOCK_DGRAM, protonum)) < 0)
		return  XB_NOSOCKET;
	if  (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sockfd);
		return  XB_NOBIND;
	}
	fdp->prodfd = sockfd;
	if  (fcntl(sockfd, F_SETOWN, getpid()) < 0  ||
	     fcntl(sockfd, F_SETFL, FASYNC) < 0)
		return  XB_NOSOCKET;
	return  XB_OK;
}

static void  unsetmon(struct api_fd *fdp)
{
	struct	api_msg	msg;
	if  (fdp->prodfd < 0)
		return;
	close(fdp->prodfd);
	fdp->prodfd = -1;
	msg.code = API_UNREQPROD;
	gbatch_wmsg(fdp, &msg);
}

int	gbatch_jobmon(const int fd, void (*fn)(const int))
{
	struct	api_fd	*fdp = gbatch_look_fd(fd);

	if  (!fdp)
		return  XB_INVALID_FD;

	if  (fn)  {
		int	ret;
		if  (fdp->prodfd < 0  &&  (ret = setmon(fdp)) != 0)
			return  ret;
	}
	else  if  (fdp->jobfn && !fdp->varfn)
		unsetmon(fdp);
	fdp->jobfn = fn;
	return  XB_OK;
}

int	gbatch_varmon(const int fd, void (*fn)(const int))
{
	struct	api_fd	*fdp = gbatch_look_fd(fd);

	if  (!fdp)
		return  XB_INVALID_FD;

	if  (fn)  {
		int	ret;
		if  (fdp->prodfd < 0  &&  (ret = setmon(fdp)) != 0)
			return  ret;
	}
	else  if  (fdp->varfn && !fdp->jobfn)
		unsetmon(fdp);
	fdp->varfn = fn;
	return  XB_OK;
}
#endif /* Possible to get SIGIO working */

int  gbatch_close(const int fd)
{
	struct	api_fd	*fdp = gbatch_look_fd(fd);
	struct	api_msg	outmsg;

	if  (!fdp)
		return  XB_INVALID_FD;
#if	(defined(FASYNC) && defined(F_SETOWN))
	if  (fdp->prodfd >= 0)  {
		unsetmon(fdp);
		fdp->jobfn = (void (*)()) 0;
		fdp->varfn = (void (*)()) 0;
	}
#endif
	outmsg.code = API_SIGNOFF;
	gbatch_wmsg(fdp, &outmsg);
	close(fdp->sockfd);
	fdp->sockfd = -1;
	if  (fdp->bufmax != 0)  {
		fdp->bufmax = 0;
		free(fdp->buff);
		fdp->buff = (char *) 0;
	}
	if  (fdp->queuename)  {
		free(fdp->queuename);
		fdp->queuename = (char *) 0;
	}
	return  XB_OK;
}
