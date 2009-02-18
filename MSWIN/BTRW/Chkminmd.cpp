#include "stdafx.h"

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
