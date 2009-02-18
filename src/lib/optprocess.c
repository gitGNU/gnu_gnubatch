/* optprocess.c -- process options to shell commands

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

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "optflags.h"
#include "errnums.h"

#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

static	char	Filename[] = __FILE__;

/* Construct environment variable name from program name */

char	*make_varname()
{
	char  *varname = malloc((unsigned) strlen(progname) + 1), *vp;
	const  char  *pp;

	if  (!varname)
		ABORT_NOMEM;

	pp = progname;
	vp = varname;

	while  (*pp)  {
		int  ch = *pp++;
		if  (!isalpha(ch))
			*vp++ = '_';
		else
			*vp++ = toupper(ch);
	}
	*vp = '\0';
	return  varname;
}

char   **optprocess(char **argv, const Argdefault *defaultopts, optparam * const optlist, const int minstate, const int maxstate, const int keepargs)
{
	char	*loclist = envprocess(CONFIGPATH), *nxt, *name;
	const	char	*configname = USER_CONFIG;
	HelpargRef	avec = helpargs(defaultopts, minstate, maxstate);
	int	hadargv = 0;
	char	*Varname = make_varname();

	nxt = loclist;
	for  (;;)  {
		char	*colp, *dirname;

		if  ((colp = strchr(nxt, ':')))
			*colp = '\0';

		if  (nxt[0] == '-'  &&  nxt[1] == '\0')  {	/* Process Command Line Options */
			if  (!hadargv)
				argv = doopts(&argv[0], avec, optlist, minstate);
			hadargv++;
		}
		else  if  (*nxt == '\0'  ||  (nxt[0] == '!' && nxt[1] == '\0'))
			doenv(getenv(Varname), avec, optlist, minstate);
		else  {
			char	cfilename[PATH_MAX];
			if  (strchr(nxt, '~'))  {
				if  (!(dirname = unameproc(nxt, ".", Realuid)))
					goto  donxt;
				if  (strchr(dirname, '$'))  {
					char	*tmp = envprocess(dirname);
					free(dirname);
					dirname = tmp;
				}
				sprintf(cfilename, "%s/%s", dirname, configname);
				free(dirname);
			}
			else  if  (strchr(nxt, '$'))  {
				dirname = envprocess(nxt);
				sprintf(cfilename, "%s/%s", dirname, configname);
				free(dirname);
			}
			else
				sprintf(cfilename, "%s/%s", nxt, configname);

			if  ((name = rdoptfile(cfilename, Varname)))  {
				doenv(name, avec, optlist, minstate);
				free(name);
			}
		}
	donxt:
		if  (!colp)
			break;
		*colp++ = ':';
		nxt = colp;
	}
	close_optfile();

	if  (keepargs || Anychanges & OF_ANY_FREEZE_WANTED)
		makeoptvec(avec, minstate, maxstate);
	freehelpargs(avec);
	if  (!hadargv)
		argv++;
	return  argv;
}
