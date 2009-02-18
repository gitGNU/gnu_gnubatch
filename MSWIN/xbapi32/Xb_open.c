/* Xb_open.c -- Open access to API

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

#include <sys/types.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <winsock.h>
#include "xbapi.h"
#include "xbapi_in.h"

#define	MAXFDS	10

static	unsigned	api_max;
static	struct	api_fd	apilist[MAXFDS];

int		xbapi_dataerror;

int		xb_write(const SOCKET fd, char *buff, unsigned size)
{
	int	obytes;
	while  (size != 0)  {
		if  ((obytes = send(fd, buff, size, 0)) < 0)  {
			int	num = WSAGetLastError();
			return  XB_BADWRITE;
		}
		size -= obytes;
		buff += obytes;
	}
	return  0;
}

int		xb_read(const SOCKET fd, char *buff, unsigned size)
{
	int	ibytes;
	while  (size != 0)  {
		if  ((ibytes = recv(fd, buff, size, 0)) < 0)  {
			int	num = WSAGetLastError();
			return  XB_BADREAD;                        
		}
		size -= ibytes;
		buff += ibytes;
	}
	return  0;
}

int		xb_wmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  xb_write(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

int		xb_rmsg(const struct api_fd *fdp, struct api_msg *msg)
{
	return  xb_read(fdp->sockfd, (char *) msg, sizeof(struct api_msg));
}

struct	api_fd *xb_look_fd(const int fd)
{
	struct	api_fd	*result;

	if  (fd < 0  ||  (unsigned) fd >= api_max)
		return  (struct api_fd *) 0;
	result = &apilist[fd];
	if  (result->sockfd == INVALID_SOCKET)
		return  (struct api_fd *) 0;
	return  result;
}

static	int	getservice(const char *servname, const int proto, const int def_serv)
{
	struct	servent	*sp;
	char	inifilename[_MAX_PATH];
	
	if  (proto == IPPROTO_TCP)  {
		if  (!(sp = getservbyname(servname, "tcp")))
			sp = getservbyname(servname, "TCP");
	}
	else  if  (!(sp = getservbyname(servname, "udp")))
		sp = getservbyname(servname, "UDP");
	
	if  (sp)
		return  ntohs(sp->s_port);
	
	GetProfileString(XI_SOFTWARE, PRODNAME, DEFBASED, inifilename, sizeof(inifilename));
	strcat(inifilename, "\\");
    strcat(inifilename, WININI);
    return  GetPrivateProfileInt(proto == IPPROTO_TCP? "TCP Ports": "UDP Ports", "API", def_serv, inifilename);
}

static	int		open_common(const char *hostname, const char *servname, const char *username)
{
	int		portnum;
	SOCKET	sock;
	unsigned	result;
	struct	api_fd	*ret_fd;
	netid_t	hostid;
	struct	hostent	*hp;
	struct	sockaddr_in	sin;
	WSADATA wd;
	WORD    vr = 0x0101;
	
	WSAStartup(vr, &wd);
    if  (!(hp = gethostbyname(hostname)))
		return  XB_INVALID_HOSTNAME;

	hostid = * (netid_t *) hp->h_addr;
	portnum = getservice(servname? servname: DEFAULT_SERVICE, IPPROTO_TCP, DEF_APITCP_PORT);

	sin.sin_family = AF_INET;
	sin.sin_port = htons((short)portnum);
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = hostid;

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  XB_NOSOCKET;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		closesocket(sock);
		return  XB_NOCONNECT;
	}

	/*
	 *	Allocate ourselves a "file descriptor" and stuff our stuff in it.
	 */

	for  (result = 0;  result < api_max;  result++)
		if  (apilist[result].sockfd == INVALID_SOCKET)
			goto  found;
	if  (++api_max >= MAXFDS)
		return  XB_NOMEM;
 found:
	ret_fd = &apilist[result];
	ret_fd->portnum = (SHORT) portnum;
	ret_fd->sockfd = sock;
	ret_fd->prodfd = INVALID_SOCKET;
	ret_fd->hostid = hostid;
	ret_fd->jserial = 0;
	ret_fd->vserial = 0;
	ret_fd->bufmax = 0;
	ret_fd->buff = (char *) 0;
	strncpy(ret_fd->username, username, UIDSIZE);
	ret_fd->username[UIDSIZE] = '\0';
	return  (int) result;
}

int	xb_open(const char *hostname, const char *servname, const char *username)
{
	int		ret, result;
	struct	api_fd	*ret_fd;
	struct	api_msg	outmsg;

	if  ((result = open_common(hostname, servname, username)) < 0)
		return  result;
	ret_fd = &apilist[result];
	
	outmsg.code = API_SIGNON;
	outmsg.re_reg = 0;
	strcpy(outmsg.un.signon.username, ret_fd->username);
	if  ((ret = xb_wmsg(ret_fd, &outmsg)) || (ret = xb_rmsg(ret_fd, &outmsg)))
		goto  errret;

	ret = (short) ntohs(outmsg.retcode);

	if  (ret != XB_OK)
		goto  errret;

	return  (int) result;
errret:
	xb_close(result);
	return  ret;
}

int	xb_login(const char *hostname, const char *servname, const char *username, char *passwd, const int rereg)
{
	int		ret, result;
	struct	api_fd	*ret_fd;
	struct	api_msg	outmsg;
	char	pwbuf[API_PASSWDSIZE+1];

	/*
	 *	Before we do anything, copy argument password to buffer and zap argument
	 *	to make it harder to find password on stack.
	 */

	memset(pwbuf, '\0', sizeof(pwbuf));
	if  (passwd)  {
		int	 cnt = 0;
		while  (*passwd  &&  cnt < API_PASSWDSIZE)  {
			pwbuf[cnt++] = *passwd;
			*passwd++ = '\0';
		}
	}

	if  ((result = open_common(hostname, servname, username)) < 0)
		return  result;
	ret_fd = &apilist[result];

	outmsg.code = API_LOGIN;
	outmsg.re_reg = rereg;
	strcpy(outmsg.un.signon.username, ret_fd->username);	/* ret_fd 'cous it's truncated */
	if  ((ret = xb_wmsg(ret_fd, &outmsg))  ||  (ret = xb_rmsg(ret_fd, &outmsg)))
		goto  errret;

	ret = (short) ntohs(outmsg.retcode);
	
	if  (ret == XB_NO_PASSWD)  {
		if  ((ret = xb_write(ret_fd->sockfd, pwbuf, sizeof(pwbuf))))
			goto  errret;
		
		if  ((ret = xb_rmsg(ret_fd, &outmsg)))
			goto  errret;

		ret = (short) ntohs(outmsg.retcode);

		if  (ret != XB_OK)
			goto  errret;
	}
	else  if  (ret != XB_OK)
		goto  errret;

	if  ((ret = xb_rmsg(ret_fd, &outmsg)))
		goto  errret;

	ret = (short) ntohs(outmsg.retcode);
	if  (ret != XB_OK)
		goto  errret;

	return  result;

errret:
	xb_close(result);
	return  ret;
}

int	xb_newgrp(const int fd, const char *grp)
{
	int	ret;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;
	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_NEWGRP;
	msg.re_reg = 0;
	strncpy(msg.un.signon.username, grp, UIDSIZE);
	msg.un.signon.username[UIDSIZE] = '\0';
	if  ((ret = xb_wmsg(fdp, &msg))  ||  (ret = xb_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (short) ntohs(msg.retcode);
	return  XB_OK;
}

int	xb_procmon(const int fd)
{
	struct	api_fd	*fdp;
	struct	api_msg	imsg;

	if  (!(fdp = xb_look_fd(fd)))
		return  0;

	if  (recvfrom(fdp->prodfd, (char *) &imsg, sizeof(imsg), 0, (struct sockaddr *) 0, (int *) 0) < 0)
		return  0;

	switch  (imsg.code)  {
	default:
		return  0;
	case  API_JOBPROD:
		fdp->jserial = ntohl(imsg.un.r_reader.seq);
		return	XBWINAPI_JOBPROD;
	case  API_VARPROD:
		fdp->vserial = ntohl(imsg.un.r_reader.seq);
		return	XBWINAPI_VARPROD;
	}
}

int	xb_setmon(const int fd, HWND hWnd, UINT wMsg)
{
	int		pportnum, ret;
	SOCKET	sockfd;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	sockaddr_in	sin;
	struct	api_msg	msg;

	if  (!fdp)
		return  XB_INVALID_FD;

	pportnum = getservice(MON_SERVICE, IPPROTO_UDP, DEF_APIUDP_PORT);
	msg.code = API_REQPROD;
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
		
	sin.sin_family = AF_INET;
	sin.sin_port = htons((short) pportnum);
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;

	if  ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return  XB_NOSOCKET;                      
		
	if  (bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		closesocket(sockfd);
		return  XB_NOBIND;
	}
	if  (WSAAsyncSelect(sockfd, hWnd, wMsg, FD_READ) != 0)  {
		closesocket(sockfd);
		return  XB_NOSOCKET;
	}
	fdp->prodfd = sockfd;
	return  XB_OK;
}

void	xb_unsetmon(const int fd)
{
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;
	if  (!fdp  ||  fdp->prodfd == INVALID_SOCKET)
		return;
	closesocket(fdp->prodfd);
	fdp->prodfd = INVALID_SOCKET;
	msg.code = API_UNREQPROD;
	xb_wmsg(fdp, &msg);
}

int	xb_close(const int fd)
{
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	outmsg;

	if  (!fdp)
		return  XB_INVALID_FD;

	if  (fdp->prodfd != INVALID_SOCKET)
		xb_unsetmon(fd);
	outmsg.code = API_SIGNOFF;
	xb_wmsg(fdp, &outmsg);
	closesocket(fdp->sockfd);
	fdp->sockfd = INVALID_SOCKET;
	if  (fdp->bufmax != 0)  {
		fdp->bufmax = 0;
		free(fdp->buff);
		fdp->buff = (char *) 0;
	}
	WSACleanup();
	return  XB_OK;
}
