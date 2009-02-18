/* userops.cpp -- Set user options

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

#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#ifdef	BTQW
#include "netmsg.h"
#endif
#include "xbwnetwk.h"
#include "ulist.h"
#ifdef	BTQW
#include "btqw.h"
#endif
#ifdef	BTRSETW
#include "btrsetw.h"
#endif
#ifdef	BTRW
#include "btrw.h"
#endif
#include "loginhost.h"

#ifdef	_DEBUG
#define	UDP_WAITTIME	300
#else
#define UDP_WAITTIME    5
#endif
#define	UDP_TRIES		3

extern	sockaddr_in	serv_addr;

static  void	 unpack_btuser(Btuser FAR &dest, const Btuser &src)
{
	dest.btu_isvalid = src.btu_isvalid;
	dest.btu_minp = src.btu_minp;
	dest.btu_maxp = src.btu_maxp;
	dest.btu_defp = src.btu_defp;
	dest.btu_user = ntohl((unsigned long) src.btu_user);
	dest.btu_maxll = ntohs((unsigned short) src.btu_maxll);
	dest.btu_totll = ntohs((unsigned short) src.btu_totll);
	dest.btu_spec_ll = ntohs((unsigned short) src.btu_spec_ll);
	dest.btu_priv = ntohl(src.btu_priv);
	dest.btu_jflags[0] = ntohs((unsigned short) src.btu_jflags[0]);
	dest.btu_jflags[1] = ntohs((unsigned short) src.btu_jflags[1]);
	dest.btu_jflags[2] = ntohs((unsigned short) src.btu_jflags[2]);
	dest.btu_vflags[0] = ntohs((unsigned short) src.btu_vflags[0]);
	dest.btu_vflags[1] = ntohs((unsigned short) src.btu_vflags[1]);
	dest.btu_vflags[2] = ntohs((unsigned short) src.btu_vflags[2]);
}

const  int	udp_sendrecv(char FAR *sendbuf, char FAR *recvbuf, const int sendsize, const int recvsize)
{
	time(&Locparams.tlastop);

	if  (sendto(Locparams.uasocket, sendbuf, sendsize, 0, (struct sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		return  IDP_GBTU_NOSEND;

	fd_set	rfds;
	FD_ZERO(&rfds);
	FD_SET(Locparams.uasocket, &rfds);
	timeval	tv;
	tv.tv_sec = UDP_WAITTIME;
	tv.tv_usec = 0;
	if  (select(FD_SETSIZE, &rfds, (fd_set FAR *) 0, (fd_set FAR *) 0, &tv) <= 0)
		return  IDP_GBTU_NORECV;
	if  (recvfrom(Locparams.uasocket, recvbuf, recvsize, 0, (sockaddr FAR *) 0, (int FAR *) 0) <= 0)
		return  IDP_GBTU_NORECV;
	return  0;
}

const	int		xb_enquire(CString &username, CString &mymachname, CString &resname)
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_ENQUIRE;
	strncpy(enq.ual_name, (const char *) username, UIDSIZE);
	strncpy(enq.ual_machname, (const char *) mymachname, HOSTNSIZE);
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;
	if  (reply.ual_op == UAL_OK)  {
		resname = reply.ual_name;
		return  0;
	}
	switch  (reply.ual_op)  {
	default:					return  IDP_XBENQ_UNKNOWN;
	case  XBNR_NOT_CLIENT:
	case  XBNR_UNKNOWN_CLIENT:	return  IDP_XBENQ_BADCLIENT;
	case  XBNR_NOT_USERNAME:
	case  XBNR_UNKNOWN_USER:
	case  UAL_INVU:				return  IDP_XBENQ_UNKNOWNU;
	case  UAL_INVP:
	case  UAL_NOK:				return  IDP_XBENQ_PASSREQ;
	}
}

const	int		xb_login(CString &username, CString &mymachname, const char *passwd, CString &resname)
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_LOGIN;
	strncpy(enq.ual_name, (const char *) username, UIDSIZE);
	strncpy(enq.ual_machname, (const char *) mymachname, HOSTNSIZE);
	strncpy(enq.ual_passwd, passwd, UA_PASSWDSZ);
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;
	if  (reply.ual_op == UAL_OK)  {
		resname = reply.ual_name;
		return  0;
	}
	switch  (reply.ual_op)  {
	default:					return  IDP_XBENQ_UNKNOWN;
	case  XBNR_NOT_CLIENT:
	case  XBNR_UNKNOWN_CLIENT:	return  IDP_XBENQ_BADCLIENT;
	case  XBNR_NOT_USERNAME:
	case  XBNR_UNKNOWN_USER:
	case  UAL_INVU:				return  IDP_XBENQ_UNKNOWNU;
	case  UAL_INVP:
	case  UAL_NOK:				return  IDP_XBENQ_BADPASSWD;
	}
}

const  int	xb_newgrp(CString &grp)
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_NEWGRP;
	strncpy(enq.ual_name, (const char *) grp, UIDSIZE);
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;

	switch  (reply.ual_op)  {
	case  UAL_OK:				return  0;
	default:					return  IDP_XBENQ_UNKNOWN;
	case  XBNR_NOT_CLIENT:
	case  XBNR_UNKNOWN_CLIENT:	return  IDP_XBENQ_BADCLIENT;
	case  XBNR_UNKNOWN_GROUP:
	case  UAL_INVG:				return  IDP_XBENQ_UNKNOWNG;
	case  UAL_INVU:				return  IDP_XBENQ_UNKNOWNU;
	case  UAL_INVP:
	case  UAL_NOK:				return  IDP_XBENQ_BADPASSWD;
	}
}

const  int	xb_logout()
{
	ua_login	enq, reply;
	memset(&enq, '\0', sizeof(enq));
	enq.ual_op = UAL_LOGOUT;
	int  ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &reply, sizeof(enq), sizeof(reply));
	if  (ret != 0)
		return  ret;
	if  (reply.ual_op != UAL_OK)
		return  IDP_XBENQ_UNKNOWN;
	return  0;
}

const  int	getbtuser(Btuser FAR &mpriv, CString &realuname, CString &realgname)
{
	ni_jobhdr	uenq;
	uenq.code = CL_SV_UENQUIRY;
	uenq.joblength = htons(sizeof(uenq));
	uenq.uname[0] = '\0';
	ua_reply	resp;
	int		ret = udp_sendrecv((char FAR *) &uenq, (char FAR *) &resp, sizeof(uenq), sizeof(resp));
	if  (ret == 0)  {
		if  (!resp.ua_perm.btu_isvalid)
			return	IDP_GBTU_NVALID;
		unpack_btuser(mpriv, resp.ua_perm);
		realuname = resp.ua_uname;
		realgname = resp.ua_gname;
	}
	return	ret;
}

static	BOOL	relogin()
{
#ifdef	BTQW
	CBtqwApp  &ma = *((CBtqwApp *)AfxGetApp());
#endif
#ifdef	BTRSETW
	CBtrsetwApp  &ma = *((CBtrsetwApp *)AfxGetApp());
#endif
#ifdef	BTRW
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
#endif
	int	ret;
	CString	newuser;

	if  ((ret = xb_enquire(ma.m_winuser, ma.m_winmach, newuser)) != 0)  {

		if  (ret != IDP_XBENQ_PASSREQ)
			return  FALSE;            

		CLoginHost	dlg;
		dlg.m_unixhost = look_host(Locparams.servid);
		dlg.m_clienthost = ma.m_winmach;
		dlg.m_username = ma.m_winuser;

		int	cnt = 0;
		
		for  (;;)  {
			
			if  (dlg.DoModal() != IDOK)
				return  FALSE;

			if  ((ret = xb_login(dlg.m_username, ma.m_winmach, (const char *) dlg.m_passwd, newuser)) == 0)
				break;

			if  (ret != IDP_XBENQ_BADPASSWD || cnt >= 2)  {
		    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				return  FALSE;
			}

			if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)
				return  FALSE;
			cnt++;
		}
	}

    if  (getbtuser(ma.m_mypriv, ma.m_username, ma.m_groupname) != 0)
		return  FALSE;
	return  TRUE;
}

void	refreshconn()
{
	if  (time(NULL) < Locparams.tlastop + Locparams.servtimeout/2)
		return;

	char	msg = CL_SV_KEEPALIVE, repl;
	int		ret = udp_sendrecv((char FAR *) &msg, (char FAR *) &repl, sizeof(msg), sizeof(repl));
	if  (ret == 0  &&  repl == 0)
		return;

	//  He may want to give up now

	if  (AfxMessageBox(IDP_CONNDIED, MB_ICONSTOP|MB_YESNO) != IDYES)
		goto  giveup;
	
	if  (initenqsocket(Locparams.servid) != 0)
		goto  giveup;

	if  (relogin())
		return;

giveup:
	AfxGetMainWnd()->SendMessage(WM_CLOSE);
}

void	UUGList::init(const unsigned char type, const char FAR *prefx)
{
	if  (prefx)  {
		preflen = strlen(prefx);
		strncpy(prefix, prefx, UIDSIZE);
		prefix[UIDSIZE] = '\0';
	}
	else  {
		prefix[0] = '\0';     
		preflen = 0;
	}

	if  (Locparams.servid == 0 || Locparams.uasocket == INVALID_SOCKET)  {
		nbytes = pos = 0;
		return;
	}

	char	abyte[1];
	abyte[0] = type;

	time(&Locparams.tlastop);
	
	if  (sendto(Locparams.uasocket, abyte, sizeof(abyte), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		nbytes = pos = 0;
	else  {		
		pos = 0;
		nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
	}	
}

UUGList::~UUGList()
{
}		

const  char  FAR  *UUGList::next()
{
	if  (Locparams.uasocket == INVALID_SOCKET  ||  nbytes <= 0)
		return  NULL;
	for  (;;)  {
		if  (pos >= nbytes)  {
			nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
			pos = 0;
			if  (nbytes <= 0 || buffer[0] == '\0')
				return  NULL;
		}                                                   
		char  *res = &buffer[pos];
		size_t	len = strlen(res) + 1;
		pos += len;
		if  (strncmp(res, prefix, preflen) == 0)
			return  res;
	}	
}

//  Not really connected but a suitable place to put it.
//  Send me a list of holidays for the specified year

unsigned  char	*gethvec(const int  year)
{
	ua_venq	req;
	req.uav_code = CL_SV_HLIST;
	req.uav_perm = htons(year - 1990);
	unsigned  char  *result = new unsigned char [YVECSIZE];
	int  ret = udp_sendrecv((char FAR *) &req, (char FAR *) result, sizeof(req), YVECSIZE);
	if  (ret != 0)  {
		delete [] result;
		return  NULL;
	}
	return  result;
}
