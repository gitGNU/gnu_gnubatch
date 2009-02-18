#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#include "btqw.h"

BOOL	Btmode::mpermitted(const unsigned flag) const
{               
	CBtqwApp  *ma = ((CBtqwApp *)AfxGetApp());
	unsigned short uf = u_flags, gf = g_flags, of = o_flags;
	if  (ma->m_mypriv.btu_priv & BTM_ORP_UG)  {
		uf |= gf;
		gf |= uf;
	}
	if  (ma->m_mypriv.btu_priv & BTM_ORP_UO)  {
		uf |= of;
		of |= uf;
	}
	if  (ma->m_mypriv.btu_priv & BTM_ORP_GO)  {
		gf |= of;
		of |= gf;
	}
	if  (strcmp(ma->m_username, o_user) == 0)
		return  (uf & flag) == flag;
	if  (strcmp(ma->m_groupname, o_group) == 0)
		return  (gf & flag) == flag;
	return  (of & flag) == flag;
}
	
int		checkminmode(const unsigned um, const unsigned gm, const unsigned om)
{
	if  (um & BTM_SHOW  &&  um & (BTM_DELETE|BTM_WRMODE))
		return  1;
	if  (gm & BTM_SHOW  &&  gm & (BTM_DELETE|BTM_WRMODE))
		return  1;
	if  (om & BTM_SHOW  &&  om & (BTM_DELETE|BTM_WRMODE))
		return  1;           
	return  0;
}
	
		