/* netrouts.cpp -- network handling routines

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
#include <stdlib.h>
#include <io.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "files.h"
#ifdef	BTQW
#include "netmsg.h"
#endif
#include "xbwnetwk.h"
#include "resource.h"

local_params	Locparams;

//  Socket addresses for enquiry port to xbnetserv

sockaddr_in     serv_addr;

// Hashing routines for getting net names from ids and vice versa

remote	*hhashtab[HASHMOD],		// Hash table of host names
		*ahashtab[HASHMOD],		// Hash table of alias names
		*nhashtab[HASHMOD];		// Hash table of netids

inline	unsigned  calcnhash(netid_t netid)
{
	unsigned  result = 0;

	for  (int  i = 0;  i < 32;  i += 8)
		result ^= netid >> i;

	return  result % HASHMOD;
}

inline	unsigned  calchhash(const char *hostid)
{
	unsigned  result = 0;
	while  (*hostid)
		result = (result << 1) ^ *hostid++;
	return  result % HASHMOD;
}

inline	char	*stracpy(const char FAR *s)
{
	char	*result = new char [strlen(s)+1];
	strcpy(result, s);
	return  result;
}	

remote::remote(const netid_t nid, const char FAR *name, const char FAR *alias, const unsigned char flags, const unsigned short timeout) :
	   hostid(nid), ht_flags(flags), ht_timeout(timeout)
{
	hh_next = NULL;
	ha_next = NULL;
	hn_next = NULL;
	h_name = stracpy(name);
	if  (alias && strcmp(name, alias) != 0)
		h_alias = stracpy(alias);
	else
		h_alias = NULL;
	sockfd = INVALID_SOCKET;
	is_sync = NSYNC_NONE;
	lastwrite = 0;
#ifdef	BTQW
	bytesleft = 0;
	byteoffset = 0;
#endif
}                   

void	remote::addhost()
{
	remote	*np, **npp;
	
	if  (!isdigit(h_name[0]))  {
		for  (npp = &hhashtab[calchhash(h_name)]; np = *npp; npp = &np->hh_next)
			;            
		*npp = this;
	}
	if  (h_alias)  {
		for  (npp = &ahashtab[calchhash(h_alias)]; np = *npp; npp = &np->ha_next)
			;
		*npp = this;
	}	
	for  (npp = &nhashtab[calcnhash(hostid)]; np = *npp; npp = &np->hn_next)
		;
	*npp = this;
#ifdef	BTRSETW
	Locparams.lportnum++;
#endif
}	    

#ifdef	BTRSETW
void	remote::delhost()
{
	remote	*np, **npp;
	for  (npp = &hhashtab[calchhash(h_name)];  np = *npp;  npp = &np->hh_next)
		if  (np == this)  {
			*npp = np->hh_next;
			break;
		}
	if  (h_alias)  {
		for  (npp = &ahashtab[calchhash(h_alias)]; np = *npp; npp = &np->ha_next)
		if  (np == this)  {
			*npp = np->ha_next;
			break;
		}
	}
	for  (npp = &nhashtab[calcnhash(hostid)]; np = *npp; npp = &np->hn_next)
		if  (np == this)  {
			*npp = np->hn_next;
			break;
		}                                       
	Locparams.lportnum--;
}

BOOL	clashcheck(const char FAR *name)
{
	remote  *rp;
	for  (rp = hhashtab[calchhash(name)];  rp;  rp = rp->hh_next)
		if  (strcmp(name, rp->h_name) == 0)
			return  TRUE;
	for  (rp = ahashtab[calchhash(name)];  rp;  rp = rp->ha_next)
		if  (strcmp(name, rp->h_alias) == 0)
			return  TRUE;
	return  FALSE;
}	

BOOL	clashcheck(const netid_t hid)
{
	remote  *rp;
	for  (rp = nhashtab[calcnhash(hid)];  rp;  rp = rp->hn_next)
		if  (rp->hostid == hid)
			return  TRUE;
	return  FALSE;
}

//  Get nth host in list (assuming list is small)

remote	*get_nth(const unsigned n)
{
	unsigned  reached = 0;
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)
			if  (reached++ == n)
				return  np;
	return  (remote *) 0;
}
#endif	//  BTRSETW

const char	*look_host(const netid_t nid)
{
	for  (remote *np = nhashtab[calcnhash(nid)]; np;  np = np->hn_next)
		if  (nid == np->hostid)
			return  np->h_alias? np->h_alias: np->h_name;
	if  (nid == Locparams.myhostid)
		return  "(local)";
	return  "unknown";
}

remote	*find_host(const netid_t nid)
{
	for  (remote *np = nhashtab[calcnhash(nid)];  np;  np = np->hn_next)
		if  (nid == np->hostid)
			return  np;
	return  (remote *) 0;
}

#ifndef	BTQW
remote	*find_host(const SOCKET sock)
{
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)
			if  (sock == np->sockfd)
				return  np;
	return  (remote *) 0;
}	
#endif	//!BTQW

netid_t	look_hname(const char *name)
{
	for  (remote *np = hhashtab[calchhash(name)]; np;  np = np->hh_next)
		if  (strcmp(name, np->h_name) == 0)
			return  np->hostid;
	for  (np = ahashtab[calchhash(name)]; np;  np = np->ha_next)
		if  (strcmp(name, np->h_alias) == 0)
			return  np->hostid;
	return  -1L;
}		                                   

//  Windows sockets doesn't provide this routine...
//  Also get services and protocols don't work.

static	netid_t WSgethostid()
{
	char	myname[20];
	gethostname(myname, sizeof(myname));
	hostent FAR *hp;
	hp = gethostbyname(myname);
	return	hp? *(netid_t FAR *) hp->h_addr : -1L;
}

//  Initialise enquiry socket, but we won't necessarily know that
//  we can't talk until we try.

UINT	initenqsocket(const netid_t hostid)
{
	sockaddr_in	cli_addr;

	if  (Locparams.uasocket != INVALID_SOCKET)  {
		closesocket(Locparams.uasocket);
		Locparams.uasocket = INVALID_SOCKET;
	}
	memset((void *) &serv_addr, '\0', sizeof(serv_addr));
	memset((void *) &cli_addr, '\0', sizeof(cli_addr));
	serv_addr.sin_family = cli_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = hostid;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = Locparams.uaportnum;
	SOCKET  sockfd;
	if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
		return  IDP_GBTU_NOSOCK;
	if  (bind(sockfd, (struct sockaddr FAR *) &cli_addr, sizeof(cli_addr)) < 0)  {
		closesocket(sockfd);
		return  IDP_GBTU_NOBIND;
	}
	Locparams.uasocket = sockfd;
	time(&Locparams.tlastop);
	return  0;
}

// Initialise windows.
// Return 0 if OK, otherwise return a string resource ID
// ready for plonking into an AfxMessageBox call.

UINT	winsockstart()
{
	WSADATA wd;
	WORD	vr = 0x0101;	//  Major/minor 1/1
	int rets = WSAStartup(vr, &wd);
	if  (rets != 0)  {
		char	moan[80];
		wsprintf(moan, "Startup gave error %d", rets);
		AfxMessageBox(moan, MB_OK|MB_ICONSTOP);
	}
#ifdef	BTRSETW
	Locparams.lportnum = 0;		// Used as count
#endif
	Locparams.myhostid = WSgethostid();
	UINT  ret = loadhostfile();
	if  (ret != 0)
		return  ret;
	if  (Locparams.servid == 0)
		return  IDP_NO_SERVER;
	return  0;
}

void	winsockend()
{
	WSACleanup();
}
