#include "stdafx.h"
#include <stdlib.h>
#include <io.h>
#include <iostream.h>
#include <fstream.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "files.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"

remote_queue	current_q, pending_q;

extern	local_params	Locparams;

extern	char	FAR	basedir[];

remote  *find_host(const SOCKET sock)
{
	return  current_q.find(sock);
}

unsigned	remote_queue::alloc(remote *rp)
{
	for  (unsigned  number = 0;  number < max;  number++)
		if  (!list[number])  {
			list[number] = rp;
			return  number;
		}
	unsigned  oldmax = max;
	max += INC_REMOTES;
	remote  **newlist = new remote * [max];
	if  (oldmax > 0)  {
		memcpy((void *) newlist, (void *) list, oldmax * sizeof(remote *));
		delete [] list;
	}
	memset((void *) &newlist[oldmax], '\0', INC_REMOTES * sizeof(remote *));
	list = newlist;
	list[oldmax] = rp;
	return  oldmax;
}

remote	*remote_queue::find(const netid_t netid)
{
	for  (unsigned  cnt = 0;  cnt < max;  cnt++)
		if  (list[cnt] && list[cnt]->hostid == netid)
			return  list[cnt];
	return  (remote *) 0;
}

remote  *remote_queue::find(const SOCKET sockfd)
{
	for  (unsigned  cnt = 0;  cnt < max;  cnt++)
		if  (list[cnt] && list[cnt]->sockfd == sockfd)
			return  list[cnt];
	return  (remote *) 0;
}

unsigned	remote_queue::index(const remote * const rp)
{
	for  (unsigned  cnt = 0;  cnt < max;  cnt++)
		if  (list[cnt] == rp)
			return  cnt;
	return  max;
}

void	remote_queue::free(const remote * const rp)
{
	unsigned  ind = index(rp);
	if  (ind < max)
		list[ind] = (remote *) 0;	
}

remote	*remote_queue::operator [] (const unsigned ind)
{
	if  (ind < max)
		return  list[ind];
	return  (remote *) 0;
}	

UINT	initsockets()
{
	sockaddr_in	sin;	
	if  ((Locparams.probesock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return  IDP_SK_CCPROBESOCK;
	sin.sin_family = AF_INET;
	sin.sin_port = Locparams.pportnum;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = INADDR_ANY;
	if  (bind(Locparams.probesock, (sockaddr *) &sin, sizeof(sin)) < 0)  {
		closesocket(Locparams.probesock);
		Locparams.probesock = INVALID_SOCKET;
		return  IDP_SK_CCPROBESOCK;
	}
	return  0;
}
						