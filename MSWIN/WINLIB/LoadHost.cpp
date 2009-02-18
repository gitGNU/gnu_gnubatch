/* loadhost.cpp -- load host table

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

extern	char	basedir[];

//	Maximum number of bits we are prepared to parse \xibatch\hosts into.

#define	MAXPARSE	6

//	Split string into bits in result using delimiters given.
//	Ignore bits after MAXPARSE-1
//	Assume string is manglable

static	int	spliton(char **result, char *string, const char *delims)
{
	int	parsecnt = 1, resc = 1;

	//	Assumes no leading delimiters

	result[0] = string;
	while  (string = strpbrk(string, delims))  {
		*string++ = '\0';
		while  (strchr(delims, *string))
			string++;
		if  (!*string)
			break;
		result[parsecnt] = string;
		++resc;
		if  (++parsecnt >= MAXPARSE-1)
			break;
	}
	while  (parsecnt < MAXPARSE)
		result[parsecnt++] = (char *) 0;
	return  resc;
}

static  char FAR *shortestalias(hostent FAR *hp)
{
	char	FAR *which = (char FAR *) 0;
	int	minlen = 1000, ln;

	for  (char FAR * FAR *ap = hp->h_aliases; *ap; ap++)
		if  ((ln = strlen(*ap)) < minlen)  {
			minlen = ln;
			which = *ap;
		}
	if  (minlen < int(strlen(hp->h_name)))
		return  which;
	return	(char FAR *) 0;
}

//  Open host file and initialise list.
//  Return 0 - OK
//  Otherwise resource ID of error string.
//  Nasty side effect - bodge Locparams.servid

UINT	loadhostfile()
{
	ifstream  hfile;
	char	hpath[_MAX_PATH];
	strcpy(hpath, basedir);
	strcat(hpath, HOSTFILE);
	
	hfile.open(hpath, ios::in | ios::nocreate);
	if  (!hfile.good())
		return	IDP_NO_HOSTFILE;

	//	Loop until we find something interesting

	char	line[100];
	do  {
		hfile.getline(line, sizeof(line));

		//	Ignore leading white space and skip comment lines starting with #

		for  (char  *hostp = line;  isspace(*hostp);  hostp++)
			;
		if  (!*hostp  ||  *hostp == '#')
			continue;

		//	Split line on white space.

		char	*bits[MAXPARSE];                      
		remote	*newrem;
		
		if  (spliton(bits, hostp, " \t") < 2)  {
			hostent  FAR *hp;                         
			netid_t	nid;                              
			
			//	Reject entry if it is unknown or is "me"

			if  (!(hp = gethostbyname(bits[HOSTF_HNAME]))  ||
			    (nid = *(netid_t FAR *) hp->h_addr) == 0L  ||
			    nid == -1L  ||
			    nid == Locparams.myhostid)
				return  IDP_HF_INVALIDHOST;
			
			newrem = new remote(nid, hp->h_name, shortestalias(hp));
		}	
		else  {
			unsigned  totim = NETTICKLE;
			unsigned  char  serv_flag = 0;
			char	FAR *alias = NULL;

			//	Alias name of - means no alias

			if  (strcmp(bits[HOSTF_ALIAS], "-") != 0)
				alias = bits[HOSTF_ALIAS];

			//	Check timeout time

			if  (bits[HOSTF_TIMEOUT])  {
				if  (!isdigit(bits[HOSTF_TIMEOUT][0]))
					return  IDP_HF_BADTIMEOUT;
				long  tot = atol(bits[HOSTF_TIMEOUT]);
				if  (tot <= 0 || tot > 30000)
					return  IDP_HF_BADTIMEOUT;
				totim = (unsigned) tot;
			}

			//	Parse flags - currently only "server" and "probe" are supported

			if  (bits[HOSTF_FLAGS])  {
				char	*bitsf[MAXPARSE];
				spliton(bitsf, bits[HOSTF_FLAGS], ",");
				for  (char **fp = bitsf;  *fp;  fp++)
					if  (_stricmp(*fp, "server") == 0)
						serv_flag |= HT_SERVER;
					else  if  (_stricmp(*fp, "probe") == 0)
						serv_flag |= HT_PROBEFIRST;
			}

			hostent  FAR *hp;
			netid_t	nid;
			
			//  Allow host name to be given as internet address
			
			if  (isdigit(bits[HOSTF_HNAME][0]))  {
				serv_flag |= HT_NAMEALIAS;
				nid = inet_addr(hostp);
				if  (nid == -1L	|| nid == 0L || nid == Locparams.myhostid)
					continue;
				hp = gethostbyaddr((char *)&nid, sizeof(long), AF_INET);
				if  (!hp)  {
					if  (!alias)
						return  IDP_HF_NOALIAS;
					newrem = new remote(nid, alias, NULL, serv_flag, totim);
				}
				else
					newrem = new remote(nid, hp->h_name, alias? alias: shortestalias(hp), serv_flag, totim);
			}
			else  {
				//	Reject entry if it is unknown or is "me"

				if  (!(hp = gethostbyname(bits[HOSTF_HNAME]))  ||
				    (nid = *(netid_t FAR *) hp->h_addr) == 0L  ||
				    nid == -1L  ||
			    	nid == Locparams.myhostid)
					return  IDP_HF_INVALIDHOST;
					
				newrem = new remote(nid, bits[HOSTF_HNAME], alias? alias: shortestalias(hp), serv_flag, totim);
			}          
			
			//  NASTY SIDE EFFECT HERE!!!
			
			if  (serv_flag & HT_SERVER)  {
				Locparams.servid = nid;
				Locparams.servtimeout = newrem->ht_timeout;
			}
		}
		newrem->addhost();
	}  while  (hfile.good());
                                       
	return	0;
}

#ifdef	BTRSETW
extern	remote	*hhashtab[],		// Hash table of host names
				*ahashtab[],		// Hash table of alias names
				*nhashtab[];		// Hash table of netids

void	savehostfile()
{
	ofstream  hfile;
	char	hpath[_MAX_PATH];
	strcpy(hpath, basedir);
	strcat(hpath, HOSTFILE);
	hfile.open(hpath, ios::out);
	if  (!hfile.good())
		return;
	for  (unsigned  cnt = 0;  cnt < HASHMOD;  cnt++)
		for  (remote *np = nhashtab[cnt];  np;  np = np->hn_next)  {
			const char *an = np->aliasname();

			//  Reproduce the format we encountered when we read the file

			if  (np->ht_flags & HT_NAMEALIAS)  {
				in_addr ina = *((in_addr *) &np->hostid);
				hfile << inet_ntoa(ina) << '\t';
				if  (an  &&  *an)
					hfile << an;
				else
					hfile << np->hostname();
			}
			else  {
				hfile << np->hostname() << '\t';
				if  (an  &&  *an)
					hfile << an;
				else
					hfile << '-';
			}
			hfile << '\t';
			if  (np->ht_flags & HT_PROBEFIRST)  {
				hfile << "probe";
				if  (np->ht_flags & HT_SERVER)
					hfile << ",server";
			}
			else  if (np->ht_flags & HT_SERVER)
				hfile << "server";
			else
				hfile << '-';
			hfile << '\t' << np->ht_timeout << endl;
		}
}
#endif	// BTRSETW
