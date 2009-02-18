//  $Header: /sources/gnubatch/gnubatch/MSWIN/BTQW/CMDINT.CPP,v 1.1 2009/02/18 06:51:33 toadwarble Exp $
//	$Log: CMDINT.CPP,v $
//	Revision 1.1  2009/02/18 06:51:33  toadwarble
//	Initial revision
//
//	Revision 1.1.1.1  2009/02/17 15:11:09  jmc
//	Initial release of GNUbatch
//

#include "stdafx.h"
#include "defs.h"
#include "bjparam.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "cmdint.h"
#include "netmsg.h"
#include "network.h"
#include "resource.h"
	
short	ci_index(const netid_t hostid, const CString &name)
{
	ci_map  *rp;
	if  (!(rp = look_ci_host(hostid)))
		return  -1;
	for  (unsigned  cnt = 0;  cnt < rp->ci_num;  cnt++)
		if  (name == rp->ci_list[cnt].ci_name)
			return  cnt;
	return  -1;
}	

void	addcilist(const netid_t hostid, const unsigned num, Cmdint *cilist)
{
	ci_map	*rp;
	
	if  (rp = look_ci_host(hostid))
		delete [] rp->ci_list;     
	else  {
		rp = new ci_map;
		rp->ci_next = chain;
		rp->ci_hostid = hostid;
		chain = rp;
	}		
	rp->ci_num = num;
	rp->ci_list = cilist;
}

void	ci_delhost(const netid_t hostid)
{
	ci_map	*rp, **rpp;
	
	for  (rpp = &chain;  rp = *rpp;  rpp = &rp->ci_next)
		if  (rp->ci_hostid == hostid)  {
			*rpp = rp->ci_next;
			delete [] rp->ci_list;
			delete rp;
			return;
		}	
}

CIList::CIList(const netid_t hostid)
{
	where = cilnum = 0;
	ci_map	*rp = look_ci_host(hostid);
	if  (rp)  {
		cilnum = rp->ci_num;
		cilist = new Cmdint [cilnum];
		for  (unsigned cnt = 0;  cnt < cilnum;  cnt++)
			cilist[cnt] = rp->ci_list[cnt];
	}	
}

Cmdint	*CIList::next()
{
	if  (where >= cilnum)
		return  NULL;
	return  &cilist[where++];
}
