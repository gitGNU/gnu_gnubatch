#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"

int		listvar::next()
{
	Btvar	*result;
	while  (result = Vars()[posn])  {
		if  (result->var_mode.mpermitted(perm))
			return  posn++;
		posn++;
	}
	return  -1;
}

int		varlist::vindex(const vident &wi) const
{
	for  (unsigned cnt = 0;	cnt < maxvars;  cnt++)
		if  (list[cnt]  &&  *list[cnt] == wi)
			return  cnt;
	return  -1;
}		

int		varlist::vsortindex(const vident &wi) const
{
	for  (unsigned cnt = 0;	cnt < nvars;  cnt++)
		if  (*sortlist[cnt] == wi)
			return  cnt;
	return  -1;
}		

Btvar	*varlist::operator [] (const unsigned ind) const
{
	if  (ind >= nvars)
		return  NULL;
	return  list[ind];
}                           

Btvar	*varlist::sorted(const unsigned ind) const
{ 
	if  (ind >= nvars)
		return  NULL;
	return  sortlist[ind];
}

void	varlist::checkgrow()
{
	if  (nvars >= maxvars)  {
		if  (list)  {
			Btvar  **oldlist = list;
			Btvar  **oldsortlist = sortlist;
			list = new Btvar *[maxvars + INCVARS];
			sortlist = new Btvar *[maxvars + INCVARS];
			VERIFY(list != NULL && sortlist != NULL);
			memcpy((void *)list, (void *) oldlist, maxvars * sizeof(Btvar *));
			memcpy((void *)sortlist, (void *) oldsortlist, maxvars * sizeof(Btvar *));
			memset((void *) &list[maxvars], '\0', INCVARS * sizeof(Btvar *));
			delete [] oldlist;
			delete [] oldsortlist;
			maxvars += INCVARS;
		}
		else  {
			maxvars = INITVARS;
			list = new Btvar *[INITVARS];
			sortlist = new Btvar *[INITVARS];
			VERIFY(list != NULL && sortlist != NULL);
			memset((void *) list, '\0', INITVARS * sizeof(Btvar *));
		}
    }
}    
	
void	varlist::append(Btvar  *v)
{
	checkgrow();
	unsigned  first = 0, last = nvars;
	while  (first < last)  {
		unsigned  middle = (first + last) / 2;
		int	comp = strcmp(sortlist[middle]->var_name, v->var_name);
		if  (comp == 0)
			comp = sortlist[middle]->hostid < v->hostid? -1:
					sortlist[middle]->hostid > v->hostid? 1:
					sortlist[middle]->slotno < v->slotno? -1: 1;
		if  (comp < 0)
			first = middle + 1;
		else
			last = middle;
	}
	for  (unsigned  cnt = nvars;  cnt > first;  cnt--)
		sortlist[cnt] = sortlist[cnt-1];
	sortlist[first] = v;
	for  (cnt = 0;  cnt < maxvars;  cnt++)
		if  (!list[cnt])
			break;
	list[cnt] = v;
	nvars++;
}

void	varlist::remove(const unsigned where)
{
	Btvar	*v = list[where];
	if  (!v)
		return;
	unsigned  first = 0, last = nvars, res;
	while  (first < last)  {
		unsigned  middle = (first + last) / 2;
		Btvar	&sv = *sortlist[middle];
		int	comp = strcmp(sv.var_name, v->var_name);
		if  (comp == 0)
			comp = sv.hostid < v->hostid? -1:
					sv.hostid > v->hostid? 1:
					sv.slotno < v->slotno? -1:
					sv.slotno > v->slotno? 1: 0;
		if  (comp == 0)  {
			res = middle;
			goto  dun;
		}	
		if  (comp < 0)
			first = middle + 1;
		else
			last = middle;
	}
	//  Huh??
	return;

dun:
	nvars--;
	delete v;
	list[where] = NULL;
	for  (unsigned  cnt = res;  cnt < nvars;  cnt++)
		sortlist[cnt] = sortlist[cnt+1];
	if  (nvars == 0)  {
		delete [] list;
		delete [] sortlist;
		list = sortlist = NULL;
		maxvars = 0;
	}		
}                                                            

//  These routines are called in response to network operations

static	void	poke(const UINT msg, const unsigned indx = 0)
{
	CWnd  *maw = AfxGetApp()->m_pMainWnd;
	if  (maw)
		maw->SendMessageToDescendants(msg, indx);
}	
		
void	varlist::net_vclear(const netid_t hostid)
{
	int		  changes = 0;
	for  (int  cnt = int(maxvars) - 1;  cnt >= 0;  cnt--)  
	    if  (list[cnt]  &&  list[cnt]->hostid == hostid)  {
	        remove(cnt);
	        changes++;                         
	    }               
	if	(changes)
		poke(WM_NETMSG_VREVISED);
}

void	varlist::createdvar(const Btvar &vq)
{
	Btvar	*newv = new Btvar;
	*newv = vq;
	append(newv);
	poke(WM_NETMSG_VADD);
}

void	varlist::deletedvar(const vident &vi)
{
	int	 where = vindex(vi);
	if  (where < 0)
		return;
	remove(where);
	poke(WM_NETMSG_VDEL);
}

void	varlist::assignedvar(const Btvar &vq)
{
	int	 where = vindex(vq);
	if  (where < 0)
		return;
	Btvar  *v = list[where];
	v->var_value = vq.var_value;
	strcpy(v->var_comment, vq.var_comment);
	poke(WM_NETMSG_VCHANGE, unsigned(where));
}

void	varlist::chmogedvar(const Btvar &vq)
{
	int	 where = vindex(vq);
	if  (where < 0)
		return;
	list[where]->var_mode = vq.var_mode;
	poke(WM_NETMSG_VCHANGE, unsigned(where));
}

void	varlist::renamedvar(const vident &vi, const char *newname)
{
	int	where = vindex(vi);
	if  (where < 0)
		return;
	Btvar	*newv = new Btvar;
	*newv = *list[where];
	remove(where);
	strcpy(newv->var_name, newname);
	append(newv);
	poke(WM_NETMSG_VREVISED);
}	
	
// Transmit ops
	
void	varlist::assignvar(const Btvar &vq)
{
	int	 where = vindex(vq);
	if  (where < 0)
		return;
	var_sendupdate(*list[where], vq, V_ASSIGN);
}

void	varlist::chcommvar(const Btvar &vq)
{
	int	 where = vindex(vq);
	if  (where < 0)
		return;
	var_sendupdate(*list[where], vq, V_CHCOMM);
}
	
void	varlist::chogvar(const vident &vi, const unsigned op, const CString &newog)
{
	int	 where = vindex(vi);
	if  (where < 0)
		return;
	var_sendugupdate(*list[where], op, newog);
}

void	varlist::chmodvar(const Btvar &vq)
{
	int	 where = vindex(vq);
	if  (where < 0)
		return;
	var_sendupdate(*list[where], vq, V_CHMOD);
}	

void	jopvar(const unsigned ind, const Btcon &con, const unsigned op)
{
	Btvar  *vp = Vars()[ind];
	if  (!vp)
		return;
	switch  (op)  {
	case  BJA_ASSIGN:
		vp->var_value = con;
		break;
	case  BJA_INCR:
		vp->var_value.con_long += con.con_long;
		break;
	case  BJA_DECR:
		vp->var_value.con_long -= con.con_long;
		break;
	case  BJA_MULT:
		vp->var_value.con_long *= con.con_long;
		break;
	case  BJA_DIV:
		if  (con.con_long != 0)
			vp->var_value.con_long /= con.con_long;
		break;
	case  BJA_MOD:
		if  (con.con_long != 0)
			vp->var_value.con_long %= con.con_long;
		break;
	}
	poke(WM_NETMSG_VCHANGE, ind);
}
