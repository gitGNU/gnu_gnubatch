#include "stdafx.h"
#include <string.h>
#include <ctype.h>

BOOL  Smstr(const CString &str, const char *comp, const BOOL exact)
{
	int	 l = str.GetLength(), cl = strlen(comp);
	if  (cl < l)
		return  FALSE;

	if  (exact)  {
		if  (cl != l)
			return  FALSE;
		for  (int  cnt = 0;  cnt < l;  cnt++)
			if  (str[cnt] != '.' && toupper(str[cnt]) != toupper(comp[cnt]))
				return  FALSE;
		return  TRUE;
	}
	else  {
		int	 resid = cl - l;
		for  (int bcnt = 0;  bcnt <= resid;  bcnt++)  {
			for  (int  cnt = 0;  cnt < l ;  cnt++)  {
				if  (str[cnt] != '.' && toupper(str[cnt]) != toupper(comp[bcnt+cnt]))
					goto  notmatch;
			}
			return  TRUE;
		notmatch:
			;
		}
		return  FALSE;
	} 
}	
