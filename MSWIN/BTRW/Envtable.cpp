#include "stdafx.h"

#define	INITTABLE	20
#define	INCTABLE	10

void	envtable::add_unixenv(const char *ev)
{
	const  char	 *ep;
	if  (!(ep = strchr(ev, '=')))
		return;
	if  (numenvs >= maxenvs)  {
		if  (maxenvs == 0)  {
			maxenvs = INITTABLE;
			namelist = new char *[maxenvs];
			vallist = new char *[maxenvs];
		}
		else  {
			char	**oldnl = namelist;
			char	**oldvl = vallist;
			unsigned  oldmax = maxenvs;
			maxenvs += INCTABLE;
			namelist = new char *[maxenvs];
			vallist = new char *[maxenvs];
			memcpy(namelist, oldnl, oldmax * sizeof(char *));
			memcpy(vallist, oldvl, oldmax * sizeof(char *));
			delete [] oldnl;
			delete [] oldvl;
		}
	}
	char	*&nam = namelist[numenvs];
	char	*&val = vallist[numenvs];
	numenvs++;
	nam = new char [ep - ev + 1];
	val = new char [strlen(ep)];
	memcpy(nam, ev, ep - ev);
	nam[ep - ev] = '\0';
	strcpy(val, ep+1);
}

const  char	 *envtable::unix_getenv(const char *name)
{
	for  (unsigned cnt = 0;  cnt < numenvs;  cnt++)
		if  (strcmp(name, namelist[cnt]) == 0)
			return  vallist[cnt];
	return  NULL;
}

