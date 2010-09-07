/* look_host.c -- host name/IP hashing and lookup

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
#include "defaults.h"

static	char	Filename[] = __FILE__;

netid_t	myhostid;		/* Define this here even for non-network version */

#ifdef	NETWORK_VERSION
#include <ctype.h>
#include <sys/types.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "files.h"
#include "network.h"
#include "incl_ugid.h"

extern char *strread(FILE *, const char *);

/* Maximum number of bits we are prepared to parse /etc/Xibatch-hosts into.  */

#define	MAXPARSE	6

/* Structure used to hash host ids and aliases.  */

struct	hhash	{
	struct	hhash	*hh_next,	/* Hash table of host names */
			*hn_next;	/* Hash table of netids */
	netid_t	netid;			/* Net id (in case you didn't realise) */
	char	h_isalias;		/* Is alias - prefer this if poss */
	char	h_name[1];		/* (Start of) host name */
};

static	char	done_hostfile = 0; /* But only for look_host etc */
char		hostf_errors = 0;  /* Tell hostfile has errors */
static	struct	hhash	*hhashtab[NETHASHMOD], *nhashtab[NETHASHMOD];

/* This routine is also used in sh_network and xbnetserv */

unsigned  calcnhash(const netid_t netid)
{
	int	i;
	unsigned  result = 0;

	for  (i = 0;  i < 32;  i += 8)
		result ^= netid >> i;

	return  result % NETHASHMOD;
}

static unsigned  calchhash(const char *hostid)
{
	unsigned  result = 0;
	while  (*hostid)
		result = (result << 1) ^ *hostid++;
	return  result % NETHASHMOD;
}

static void  addtable(const netid_t netid, const char *hname, const int isalias)
{
	struct  hhash  *hp;
	unsigned  hhval = calchhash(hname);
	unsigned  nhval = calcnhash(netid);

	if  (!(hp = (struct hhash *) malloc(sizeof(struct hhash) + (unsigned) strlen(hname))))
		ABORT_NOMEM;
	hp->hh_next = hhashtab[hhval];
	hhashtab[hhval] = hp;
	hp->hn_next = nhashtab[nhval];
	nhashtab[nhval] = hp;
	hp->netid = netid;
	hp->h_isalias = (char) isalias;
	strcpy(hp->h_name, hname);
}

/* Split string into bits in result using delimiters given.  Ignore
   bits after MAXPARSE-1 Assume string is manglable */

static int  spliton(char **result, char *string, const char *delims)
{
	int	parsecnt = 1, resc = 1;

	/* Assumes no leading delimiters */

	result[0] = string;
	while  ((string = strpbrk(string, delims)))  {
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

static char *shortestalias(const struct hostent *hp)
{
	char	**ap, *which = (char *) 0;
	int	minlen = 1000, ln;

	for  (ap = hp->h_aliases;  *ap;  ap++)
		if  ((ln = strlen(*ap)) < minlen)  {
			minlen = ln;
			which = *ap;
		}
	if  (minlen < (int) strlen(hp->h_name))
		return  which;
	return  (char *) 0;
}

/* Provide interface like gethostent to the /etc/Xibatch-hosts file.
   Side effect of "get_hostfile" is to fill in "dosuser".  */

static	struct	remote	hostresult;

static	const	char	locaddr[] = "localaddress",
			probestring[] = "probe",
			manualstring[] = "manual",
			dosname[] = "dos",
			clname[] = "client",
			dosuname[] = "dosuser",
			cluname[] = "clientuser",
			extname[] = "external",
			pwcknam[] = "pwchk",
			trustnam[] = "trusted";

static	FILE	*hfile;

/* For "roaming" people the dosuser array holds the host name */

#if	HOSTNSIZE > UIDSIZE
char	dosuser[HOSTNSIZE+1];
#else
char	dosuser[UIDSIZE+1];
#endif

void  end_hostfile()
{
	if  (hfile)  {
		fclose(hfile);
		hfile = (FILE *) 0;
	}
}

struct remote *get_hostfile()
{
	char	*line;
	struct	hostent	*hp;
	int	hadlines = -1;

	/* If first time round, get "my" host name.  This may be
	   modified later perhaps.  */

	if  (!hfile)  {
		char	*hfn;
		char	myname[256];

		myhostid = 0L;
		myname[sizeof(myname) - 1] = '\0';
		gethostname(myname, sizeof(myname) - 1);
		if  ((hp = gethostbyname(myname)))
			myhostid = *(netid_t *) hp->h_addr;
		hfn = envprocess(HOSTFILE);
		hfile = fopen(hfn, "r");
		free(hfn);
		if  (!hfile)
			return  (struct  remote  *) 0;
	}

	/* Loop until we find something interesting */

	while  ((line = strread(hfile, "\n")))  {
		char	*hostp;
		char	*bits[MAXPARSE];

		/* Ignore leading white space and skip comment lines starting with # */

		for  (hostp = line;  isspace(*hostp);  hostp++)
			;
		if  (!*hostp  ||  *hostp == '#')  {
			free(line);
			continue;
		}

		/* Split line on white space.
		   Treat file like we used to if we can't find any extra fields.
		   (I.e. host name only, no probe). */

		hadlines++;	/* Started at -1 */

		if  (spliton(bits, hostp, " \t") < 2)  {	/* Only one field */
			char	*alp;

			/* Ignore entry if it is unknown or is "me"
			   This is not the place to have local addresses.  */

			if  (ncstrcmp(locaddr, hostp) == 0)  {
				free(line);
				continue;
			}

			if  (!(hp = gethostbyname(hostp))  ||  *(netid_t *)hp->h_addr == myhostid)  {
				free(line);
				continue;
			}

			/* Copy system host name in as host name.
			   Choose shortest alias name as alias if available */

			strncpy(hostresult.hostname, hp->h_name, HOSTNSIZE-1);
			if  ((alp = shortestalias(hp)))
				strncpy(hostresult.alias, alp, HOSTNSIZE-1);
			else
				hostresult.alias[0] = '\0';
			hostresult.ht_flags = 0;
			hostresult.ht_timeout = NETTICKLE;
			hostresult.hostid = * (netid_t *) hp->h_addr;
		}
		else  {

			/* More than 2 fields, check for local address
			   This is for benefit of SP2s etc which have different local addresses. */

			unsigned  totim = NETTICKLE;
			unsigned  flags = 0;

			if  (ncstrcmp(locaddr, bits[HOSTF_HNAME]) == 0)  {

				/* Ignore this line if it isn't the first "actual" line
				   in the file. ******** REMEMBER TO TELL PEOPLE ******* */

				if  (hadlines > 0)  {
					hostf_errors = 1;
					free(line);
					continue;
				}

				if  (isdigit(bits[HOSTF_ALIAS][0]))  {
#ifdef	DGAVIION
					struct	in_addr	ina_str;
					ina_str = inet_addr(bits[HOSTF_ALIAS]);
					myhostid = ina_str.s_addr;
#else
					myhostid = inet_addr(bits[HOSTF_ALIAS]);
#endif
				}
				else  if  (!(hp = gethostbyname(bits[HOSTF_ALIAS])))  {
					if  (myhostid == 0L)  {
						free(line);
						hostf_errors = 1;
						continue;
					}
				}
				else
					myhostid = * (netid_t *) hp->h_addr;
				free(line);
				continue;
			}

			/* Alias name of - means no alias This applies
			   to users as well.  */

			if  (strcmp(bits[HOSTF_ALIAS], "-") == 0)
				bits[HOSTF_ALIAS] = (char *) 0;

			/* Check timeout time.  This applies to users
			   as well but doesn't do anything at present.  */

			if  (bits[HOSTF_TIMEOUT]  &&  isdigit(bits[HOSTF_TIMEOUT][0]))  {
				totim = (unsigned) atoi(bits[HOSTF_TIMEOUT]);
				if  (totim == 0  ||  totim > 30000)
					totim = NETTICKLE;
			}

			/* Parse flags.  */

			if  (bits[HOSTF_FLAGS])  {
				char	**fp, *bitsf[MAXPARSE];

				spliton(bitsf, bits[HOSTF_FLAGS], ",");

				for  (fp = bitsf;  *fp;  fp++)  switch  (**fp)  {
				case  'p':
				case  'P':
					if  (ncstrcmp(*fp, probestring) == 0)  {
						flags |= HT_PROBEFIRST;
						continue;
					}
					if  (ncstrcmp(*fp, pwcknam) == 0)  {
						flags |= HT_PWCHECK;
						continue;
					}
				case  'M':
				case  'm':
					if  (ncstrcmp(*fp, manualstring) == 0)  {
						flags |= HT_MANUAL;
						continue;
					}
				case  'E':
				case  'e':
					if  (ncstrcmp(*fp, extname) == 0)  {
						/* Cheat for now, maybe split to HT_EXTERNAL??? */
						flags |= HT_DOS;
						strcpy(dosuser, BATCHUNAME);
						continue;
					}
				case  'T':
				case  't':
					if  (ncstrcmp(*fp, trustnam) == 0)  {
						flags |= HT_TRUSTED;
						continue;
					}
				case  'C':
				case  'c':
				case  'D':
				case  'd':
					/* Do longer name first (using ncstrncmp 'cous might have ( on end) */
					if  (ncstrncmp(*fp, dosuname, sizeof(dosuname)-1) == 0  ||  ncstrncmp(*fp, cluname, sizeof(cluname)-1) == 0)  {
						char	*bp, *ep;
						flags |= HT_DOS|HT_ROAMUSER;
						if  (!(bp = strchr(*fp, '(')) || !(ep = strchr(*fp, ')')))
							dosuser[0] = '\0';
						else  {
							*ep = '\0';
							bp++;
							if  (ep-bp > HOSTNSIZE)  {
								dosuser[HOSTNSIZE] = '\0';
								strncpy(dosuser, bp, HOSTNSIZE);
							}
							else
								strcpy(dosuser, bp);
						}
						continue;
					}
					if  (ncstrncmp(*fp, dosname, sizeof(dosname)-1) == 0  || ncstrncmp(*fp, clname, sizeof(clname)-1) == 0)  {
						char	*bp, *ep;
						flags |= HT_DOS;
						/* Note that we no longer force on password check if no (user) */
						if  (!(bp = strchr(*fp, '(')) || !(ep = strchr(*fp, ')')))
							dosuser[0] = '\0';
						else  {
							*ep = '\0';
							bp++;
							if  (lookup_uname(bp) == UNKNOWN_UID)  {
								dosuser[0] = '\0';
								flags |= HT_PWCHECK;	/* Unknown user we do though */
							}
							else  if  (ep-bp > UIDSIZE)  {
								dosuser[UIDSIZE] = '\0';
								strncpy(dosuser, bp, UIDSIZE);
							}
							else
								strcpy(dosuser, bp);
						}
						continue;
					}

					/* Cases where the keyword can't be matched find
					   their way here.  */

				default:
					hostf_errors = 1;
				}
			}

			if  (flags & HT_ROAMUSER)  {

				/* For roaming users, store the user name and alias in the
				   structure. Xtnetserv can worry about it.  */

				strncpy(hostresult.hostname, bits[HOSTF_HNAME], HOSTNSIZE-1);

				if  (bits[HOSTF_ALIAS])
					strncpy(hostresult.alias, bits[HOSTF_ALIAS], HOSTNSIZE-1);
				else
					hostresult.alias[0] = '\0';
				hostresult.hostid = 0;
			}
			else  if  (isdigit(*hostp))  {

				/* Insist on alias name if given as internet address
				   DG Aviions seem to use different inet_addr's to the
				   rest of the universe.  */

				netid_t  ina;
#ifdef	DGAVIION
				struct	in_addr	ina_str;
#endif
				if  (!bits[HOSTF_ALIAS])  {
					hostf_errors = 1;
					free(line);
					continue;
				}
#ifdef	DGAVIION
				ina_str = inet_addr(hostp);
				ina = ina_str.s_addr;
#else
				ina = inet_addr(hostp);
#endif
				if  (ina == -1L || ina == myhostid)  {
					hostf_errors = 1;
					free(line);
					continue;
				}
				strncpy(hostresult.hostname, bits[HOSTF_ALIAS], HOSTNSIZE-1);
				hostresult.alias[0] = '\0';
				hostresult.hostid = ina;
			}
			else  {

				/* This is a fixed common or garden user.  */

				if  (!(hp = gethostbyname(hostp))  ||  *(netid_t *) hp->h_addr == myhostid)  {
					hostf_errors = 1;
					free(line);
					continue;
				}
				strncpy(hostresult.hostname, bits[HOSTF_HNAME], HOSTNSIZE-1);
				if  (bits[HOSTF_ALIAS])
					strncpy(hostresult.alias, bits[HOSTF_ALIAS], HOSTNSIZE-1);
				else
					hostresult.alias[0] = '\0';
				hostresult.hostid = * (netid_t *) hp->h_addr;
			}
			hostresult.ht_flags = (unsigned char) flags;
			hostresult.ht_timeout = (USHORT) totim;
		}
		free(line);
		return  &hostresult;
	}
	endhostent();
	return  (struct  remote  *) 0;
}

void  hash_hostfile()
{
	struct	remote	*lhost;
	done_hostfile = 1;

	while  ((lhost = get_hostfile()))  {
		if  (lhost->ht_flags & HT_ROAMUSER)
			continue;
		addtable(lhost->hostid, lhost->hostname, 0);
		if  (lhost->alias[0])
			addtable(lhost->hostid, lhost->alias, 1);
	}
	end_hostfile();
}

char *look_hostid(const netid_t netid)
{
	struct  hhash  *hp, *hadname = (struct hhash *) 0;
	struct	hostent	*dbhost;

	if  (!done_hostfile)
		hash_hostfile();

	/* Prefer alias name if we can find it.  */

	for  (hp = nhashtab[calcnhash(netid)];  hp;  hp = hp->hn_next)  {
		if  (hp->netid == netid)  {
			if  (hp->h_isalias)
				return  hp->h_name;
			hadname = hp;
		}
	}
	if  (hadname)
		return  hadname->h_name;


	/* Run through system hosts file.  Note aliases, preferring
	   the shortest one if there are several */

	if  ((dbhost = gethostbyaddr((char *) &netid, sizeof(netid), AF_INET)))  {
		char	*alp = shortestalias(dbhost);
		addtable(netid, dbhost->h_name, 0);
		endhostent();
		if  (alp)  {
			addtable(netid, alp, 1);
			return  alp;
		}
		return  (char *) dbhost->h_name; /* Cast because some declare this as const char * */
	}

	endhostent();
	return  (char *) 0;
}

char *look_host(const netid_t netid)
{
	char	*res = look_hostid(netid);

	return  res? res: "Unknown";
}

netid_t  look_hostname(const char *name)
{
	netid_t  netid;
	struct  hhash  *hp;
	struct	hostent	*dbhost;

	if  (!done_hostfile)
		hash_hostfile();

	/* Recent change - decode IP addresses */

	if  (isdigit(name[0]))  {
		netid_t  ina;
#ifdef	DGAVIION
		struct	in_addr	ina_str;
		ina_str = inet_addr(name);
		ina = ina_str.s_addr;
#else
		ina = inet_addr(name);
#endif
		return  ina == -1L || ina == myhostid? 0L: ina;
	}

	for  (hp = hhashtab[calchhash(name)];  hp;  hp = hp->hh_next)  {
		if  (strcmp(hp->h_name, name) == 0)
			return  hp->netid;
	}

	if  ((dbhost = gethostbyname(name)) && strcmp(name, dbhost->h_name) == 0)  {
		netid = *(netid_t *) dbhost->h_addr;
		addtable(netid, dbhost->h_name, 0);
		endhostent();
		return  netid;
	}

	endhostent();
	return  0L;
}

#else

/* Dummy version to keep linker happy */

void  end_hostfile()
{
	return;
}

FUNCDE0(void, hash_hostfile)
{
	return;
}

char *look_host(const netid_t netid)
{
	return  "Unknown";
}

netid_t  look_hostname(const char *name)
{
	return  0L;
}
#endif
