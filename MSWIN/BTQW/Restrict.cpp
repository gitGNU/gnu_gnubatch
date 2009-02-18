#include "stdafx.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"
#include <string.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static	BOOL	ematch(const char *pattern, const char *value)
{
	int		cnt;
	BOOL	nott;
	const	char	*cp;

	for  (;;)  {

		switch  (*pattern)  {
		case  '\0':
			if  (*value == '\0' || *value == ':')
				return  TRUE;
			return  FALSE;
			
		default:
			if  (*pattern != *value)
				return  FALSE;
			pattern++;
			value++;
			continue;

		case  '?':
			if  (*value == '\0' ||  *value == ':')
				return  FALSE;
			pattern++;
			value++;
			continue;

		case  '*':
			pattern++;
			cp = strchr(value, ':');
			for  (cnt = cp? cp - value: strlen(value); cnt >= 0;  cnt--)
				if  (ematch(pattern, value+cnt))
					return  TRUE;
			return  FALSE;

		case  '[':
			if  (*value == '\0' ||  *value == ':')
				return  FALSE;
			nott = FALSE;
			if  (*++pattern == '!')  {
				nott = TRUE;
				pattern++;
			}

			//	Safety in case pattern truncated

			if  (*pattern == '\0')
				return  FALSE;

			do  {
				int  lrange, hrange;

				//	Initialise limits of range

				lrange = hrange = *pattern++;
				if  (*pattern == '-')  {
					hrange = *++pattern;

					if  (hrange == 0) // Safety in case trunacated
						return  FALSE;

					//	Be relaxed about backwards ranges

					if  (hrange < lrange)  {
						int	tmp = hrange;
						hrange = lrange;
						lrange = tmp;
					}
					pattern++;		// Past rhs of range
				}

				//	If value matches, and we are excluding range, then
				//	pattern doesn't and we quit.
				//	Otherwise we skip to the end.

				if  (*value >= lrange  &&  *value <= hrange)  {
					if  (nott)
						return  FALSE;
					while  (*pattern  &&  *pattern != ']')
						pattern++;
					if  (*pattern == '\0') // Safety
						return  FALSE;
					pattern++;
					goto  endpat;
				}

			}  while  (*pattern  &&  *pattern != ']');

			if  (*pattern == '\0') // Safety
				return  FALSE;

			while  (*pattern++ != ']')
				;
			if  (!nott)
				return  FALSE;
		endpat:
			value++;
			continue;
		}
	}
}

static	BOOL	qmatch(CString &pattern, const char *value)
{
	BOOL	res;
	int		cp;
	CString	pp = pattern;

	do  {
		//	Allow for ,-separated alternatives.
		//	There isn't a potential bug here with [,.] because
		//	we only allow names with alphanumerics and _ in.
		
		cp = pp.Find(',');
		if  (cp >= 0)  {
			res = ematch((const char *) pp.Left(cp), value);
			pp = pp.Mid(cp+1);
		}
		else
			res = ematch((const char *) pp, value);
		if  (res)
			return  TRUE;
	}  while  (cp >= 0);

	//	Not found...
	return  FALSE;
}

restrictdoc::restrictdoc()
{
	onlyl = NO;
	confd = ALWAYS;
}

int		restrictdoc::visible(const Btjob &q)
{            
	if  (!q.bj_mode.mpermitted(BTM_SHOW))
		return  0;
	if  (onlyl == YES  &&  q.hostid != Locparams.servid)
		return  0;
	if  (!user.IsEmpty()  &&  !qmatch(user, q.bj_mode.o_user))
		return  0;
	if  (!group.IsEmpty()  &&  !qmatch(group, q.bj_mode.o_group))
		return  0;
	if  (!queuename.IsEmpty())  {
	  	if  (q.bj_title.Find(':') < 0)
	  		return  incnull;
	  	if  (!qmatch(queuename, q.bj_title))
  			return  0;
	}
	return  1;
}

int		restrictdoc::visible(const Btvar &v)
{
	if  (!v.var_mode.mpermitted(BTM_SHOW))
		return  0;
	if  (onlyl == YES  &&  v.hostid != Locparams.servid)
		return  0;
	if  (!user.IsEmpty()  &&  !qmatch(user, v.var_mode.o_user))
		return  0;
	if  (!group.IsEmpty()  &&  !qmatch(group, v.var_mode.o_group))
		return  0;
	return  1;
}