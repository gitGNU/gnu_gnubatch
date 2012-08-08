/* parsehtab.c -- Parese host file for GNUspool for hostedit

   Copyright 2008 Free Software Foundation, Inc.

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
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#include "defaults.h"
#include "networkincl.h"
#include "incl_unix.h"
#include "remote.h"
#include "hostedit.h"

struct	remote	*hostlist;
int	hostnum, hostmax, gsnport;
netid_t	myhostid, gsnid;
enum  IPatype  hadlocaddr = NO_IPADDR;
char	defcluser[UIDSIZE+1];
char	gsnname[HOSTNSIZE+1];

/* Maximum number of bits we are prepared to parse the host file into.  */

#define	MAXPARSE	6
#define	HOSTF_HNAME	0
#define	HOSTF_ALIAS	1
#define	HOSTF_FLAGS	2
#define	HOSTF_TIMEOUT	3

char		hostf_errors = 0;  /* Tell hostfile has errors */

enum  Stype  sort_type = SORT_NONE;

void  memory_out()
{
	fprintf(stderr, "Run out of memory\n");
	exit(255);
}

int ncstrcmp(const char *a, const char *b)
{
	int	ac, bc;

	for  (;;)  {
		ac = toupper(*a);
		bc = toupper(*b);
		if  (ac == 0  ||  bc == 0  ||  ac != bc)
			return  ac - bc;
		a++;
		b++;
	}
}

int  ncstrncmp(const char *a, const char *b, int n)
{
	int	ac, bc;

	while  (--n >= 0)  {
		ac = *a++;
		bc = *b++;
		if  (ac == 0  ||  bc == 0)
			return  ac - bc;
		if  (islower(ac))
			ac += 'A' - 'a';
		if  (islower(bc))
			bc += 'A' - 'a';
		if  (ac != bc)
			return  ac - bc;
	}
	return  0;
}

/* Split string into bits in result using delimiters given.
   Ignore bits after MAXPARSE-1
   Assume string is manglable */

static int spliton(char **result, char *string, const char *delims)
{
	int	parsecnt = 1;
	int	resc = 1;

	/* Assumes no leading delimiters */

	result[0] = string;
	while  ((string = strpbrk(string, delims)))  {
		*string++ = '\0';
		while  (*string  &&  strchr(delims, *string))
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

/* Look at string to see if it's an IP address or looks like one as just the
   first digit doesn't work some host names start with that */

int  lookslikeip(const char *addr)
{
	int	dcount = 0, dotcount = 0;

	/* Maximum number of chars is 4 lots of 3 digits = 12 + 3 dots
	   total 15 */

	while  (*addr)  {
		if  (dcount >= 15)
			return  0;
		if  (!isdigit(*addr))  {
			if  (*addr != '.')
				return  0;
			dotcount++;
		}
		addr++;
		dcount++;
	}
	if  (dotcount != 3)
		return  0;
	return  1;
}

/* Get a.b.c.d type IP - trying to future proof */

netid_t  getdottedip(const char *addr)
{
	struct	in_addr	ina_str;
#ifdef	DGAVIION
	netid_t	res;
	ina_str = inet_addr(addr);
	res = ina_str.s_addr;
	return  res != -1? res: 0;
#else
	if  (inet_aton(addr, &ina_str) == 0)
		return  0;
	return  ina_str.s_addr;
#endif
}

netid_t  gsn_getloc(const int sockfd, netid_t servip, const int port)
{
	struct	sockaddr_in  sin, sout;
	int	soutl = sizeof(sout);

	BLOCK_ZERO(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = servip;
	if  (connect(sockfd, (struct sockaddr *) &sin, sizeof(sin)) < 0)
		return  0;
	if  (getsockname(sockfd, (struct sockaddr *) &sout, &soutl) < 0)
		return  0;
	return  sout.sin_addr.s_addr;
}

char	*shortestalias(const struct hostent *hp)
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

/* Get local address for "me". As well as an IP, we recognise the form
   GSN(host,port) to use "getsockname" on a connection to the port. */

static void  get_local_address(char *la)
{
	struct  hostent  *hp;

	/* Case where we want to use getsockname */

	if  (ncstrncmp(la, "gsn(", 4) == 0)  {
		char	*bits[MAXPARSE];
		netid_t	servip, cliip;
		int	portnum, sockfd;

		if  (spliton(bits, la+4, ",)") != 2)  {
			hostf_errors = 1;
			return;
		}
		if  (lookslikeip(bits[0]))  {
			servip = getdottedip(bits[0]);
			if  (servip == 0)  {
				fprintf(stderr, "Invalid IP %s in local address getsockname\n", bits[0]);
				hostf_errors = 1;
				return;
			}
			hadlocaddr = IPADDR_GSN_IP;
		}
		else  {
			if  (!(hp = gethostbyname(bits[0])))  {
				fprintf(stderr, "Invalid host name %s in local address getsockname\n", bits[0]);
				hostf_errors = 1;
				return;
			}
			servip = * (netid_t *) hp->h_addr;
			hadlocaddr = IPADDR_GSN_NAME;
		}

		portnum = atoi(bits[1]);
		if  (portnum <= 0)  {
			fprintf(stderr, "Invalid port number %s in local address getsockname\n", bits[1]);
			hostf_errors = 1;
			return;
		}

		strncpy(gsnname, bits[0], HOSTNSIZE);
		gsnid = servip;
		gsnport = portnum;
		sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		cliip = gsn_getloc(sockfd, gsnid, gsnport);
		if  (cliip == 0)  {
			fprintf(stderr, "Cannot currently fetch IP address via %s:%d\n", gsnname, gsnport);
			hostf_errors = 1;
		}
		else
			myhostid = cliip;
	}
	else  if  (lookslikeip(la))  {
		netid_t  result = getdottedip(la);
		if  (result == 0)  {
			fprintf(stderr, "Invalid IP address %s in local address\n", la);
			hostf_errors = 1;
			return;
		}
		hadlocaddr = IPADDR_IP;
		myhostid = result;
	}
	else  if  (!(hp = gethostbyname(la)))  {
		fprintf(stderr, "Unknown host name %s in local address\n", la);
		hostf_errors = 1;
		return;
	}
	else  {
		hadlocaddr = IPADDR_NAME;
		myhostid = * (netid_t *) hp->h_addr;
	}
}

/* Provide interface like gethostent to the /etc/Xitext-hosts file.
   Side effect of "get_hostfile" is to fill in "dosuser".  */

static	struct	remote	hostresult;

const	char	locaddr[] = "localaddress",
		probestring[] = "probe",
		manualstring[] = "manual",
		dosname[] = "dos",
		clname[] = "client",
		dosuname[] = "dosuser",
		cluname[] = "clientuser",
		extname[] = "external",
		pwcknam[] = "pwchk",
		trustnam[] = "trusted",
		defclient[] = "default";

static	FILE	*hfile;

void  end_hostfile()
{
	if  (hfile  &&  hfile != stdin)  {
		fclose(hfile);
		hfile = (FILE *) 0;
	}
}

struct	remote *get_hostfile(const char *fname)
{
	struct	hostent	*hp;
	int	hadlines = -1;
	char	in_line[200];

	/* If first time round, get "my" host name.  This may be
	   modified later perhaps.  */

	if  (!hfile)  {
		char	myname[256];

		myhostid = 0L;
		myname[sizeof(myname) - 1] = '\0';
		gethostname(myname, sizeof(myname) - 1);
		if  ((hp = gethostbyname(myname)))
			myhostid = *(netid_t *) hp->h_addr;
		if  (!fname  ||  strcmp(fname, "-") == 0)
			hfile = stdin;
		else  {
			hfile = fopen(fname, "r");
			if  (!hfile)
				return  (struct  remote  *) 0;
		}
	}

	/* Loop until we find something interesting */

 next_host:
	while  (fgets(in_line, sizeof(in_line), hfile))  {
		char	*hostp;
		int	lng = strlen(in_line) - 1;
		char	*bits[MAXPARSE];

		if  (in_line[lng] == '\n')
			in_line[lng] = '\0';

		/* Ignore leading white space and skip comment lines
		   starting with # */

		for  (hostp = in_line;  isspace(*hostp);  hostp++)
			;
		if  (!*hostp  ||  *hostp == '#')
			continue;

		/* Split line on white space.
		   Treat file like we used to if we can't find any extra fields.
		   (I.e. host name only, no probe). */

		hadlines++;	/* Started at -1 */

		if  (spliton(bits, hostp, " \t") < 2)  {	/* Only one field */
			char	*alp;

			/* Ignore entry if it is unknown or is "me"
			   This is not the place to have local addresses. */

			if  (ncstrcmp(locaddr, hostp) == 0)
				continue;

			if  (!(hp = gethostbyname(hostp))  ||  *(netid_t *)hp->h_addr == myhostid)
				continue;

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
			   This is for benefit of SP2s etc which have different local addresses.  */

			unsigned  totim = NETTICKLE;
			unsigned  flags = 0;

			if  (ncstrcmp(locaddr, bits[HOSTF_HNAME]) == 0)  {

				/* Ignore this line if it isn't the first "actual" line in the
				   file. ******** REMEMBER TO TELL PEOPLE ******* */

				if  (hadlines > 0)  {
					fprintf(stderr, "Local address line in host file in wrong place\n");
					hostf_errors = 1;
					continue;
				}

				get_local_address(bits[HOSTF_ALIAS]);
				continue;
			}

			/* Alias name of - means no alias This applies to users as well. */

			if  (strcmp(bits[HOSTF_ALIAS], "-") == 0)
				bits[HOSTF_ALIAS] = (char *) 0;

			/* Check timeout time.
			   This applies to users as well but doesn't do anything at present. */

			if  (bits[HOSTF_TIMEOUT]  &&  isdigit(bits[HOSTF_TIMEOUT][0]))  {
				totim = (unsigned) atoi(bits[HOSTF_TIMEOUT]);
				if  (totim == 0  ||  totim > 30000)
					totim = NETTICKLE;
			}

			/* Parse flags */

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
						hostresult.dosuser[0] = '\0';
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
							hostresult.dosuser[0] = '\0';
						else  {
							*ep = '\0';
							bp++;
							if  (ep-bp > HOSTNSIZE)  {
								hostresult.dosuser[HOSTNSIZE] = '\0';
								strncpy(hostresult.dosuser, bp, HOSTNSIZE);
							}
							else
								strcpy(hostresult.dosuser, bp);
						}
						/* Treat specially default client user */
						if  (strcmp(bits[HOSTF_HNAME], defclient) == 0)  {
							strcpy(defcluser, bits[HOSTF_ALIAS]);
							goto  next_host;
						}
						continue;
					}
					if  (ncstrncmp(*fp, dosname, sizeof(dosname)-1) == 0  || ncstrncmp(*fp, clname, sizeof(clname)-1) == 0)  {
						char	*bp, *ep;
						flags |= HT_DOS;
						/* Note that we no longer force on password check if no (user) */
						if  (!(bp = strchr(*fp, '(')) || !(ep = strchr(*fp, ')')))
							hostresult.dosuser[0] = '\0';
						else  {
							*ep = '\0';
							bp++;
							if  (!getpwnam(bp))  {
 								hostresult.dosuser[0] = '\0';
 								flags |= HT_PWCHECK;
								hostf_errors++;
								fprintf(stderr, "Unknown user %s in host file\n", bp);
							}
 							else   if  (ep-bp > UIDSIZE)  {
								hostresult.dosuser[UIDSIZE] = '\0';
								strncpy(hostresult.dosuser, bp, UIDSIZE);
							}
							else
								strcpy(hostresult.dosuser, bp);
						}
						continue;
					}

					/* Cases where the keyword can't be matched find their way here. */

				default:
					hostf_errors = 1;
					fprintf(stderr, "Unknown flags keyword %s in host file\n", *fp);
				}
			}

			if  (flags & HT_ROAMUSER)  {

				/* For roaming users, store the user name and alias in
				   the structure. Xtnetserv can worry about it. */

				strncpy(hostresult.hostname, bits[HOSTF_HNAME], HOSTNSIZE-1);

				if  (bits[HOSTF_ALIAS])
					strncpy(hostresult.alias, bits[HOSTF_ALIAS], HOSTNSIZE-1);
				else
					hostresult.alias[0] = '\0';
				hostresult.hostid = 0;
			}
			else  if  (lookslikeip(hostp))  {

				/* Insist on alias name if given as internet address */

				netid_t  ina = getdottedip(hostp);

				if  (!bits[HOSTF_ALIAS])  {
					hostf_errors = 1;
					fprintf(stderr, "No alias name given with IP address %s\n", hostp);
					continue;
				}

				if  (ina == 0 || ina == myhostid)  {
					hostf_errors = 1;
					fprintf(stderr, "Invalid/duplicated IP address %s in host file\n", hostp);
					continue;
				}
				flags |= HT_HOSTISIP;
				strncpy(hostresult.hostname, bits[HOSTF_ALIAS], HOSTNSIZE-1);
				hostresult.alias[0] = '\0';
				hostresult.hostid = ina;
			}
			else  {

				/* This is a fixed common or garden user.  */

				if  (!(hp = gethostbyname(hostp))  ||  *(netid_t *) hp->h_addr == myhostid)  {
					hostf_errors = 1;
					fprintf(stderr, "Unknown host %s in host file\n", hostp);
					continue;
				}
				strncpy(hostresult.hostname, bits[HOSTF_HNAME], HOSTNSIZE-1);
				if  (bits[HOSTF_ALIAS])
					strncpy(hostresult.alias, bits[HOSTF_ALIAS], HOSTNSIZE-1);
				else
					hostresult.alias[0] = '\0';
				hostresult.hostid = * (netid_t *) hp->h_addr;
			}
			hostresult.ht_flags = flags;
			hostresult.ht_timeout = (USHORT) totim;
		}
		return  &hostresult;
	}
	endhostent();
	return  (struct  remote  *) 0;
}

static void  checkhlistsize()
{
	if  (hostnum >= hostmax)  {
		if  (hostlist)  {
			hostmax += INCHOSTS;
			hostlist = (struct remote *) realloc((char *) hostlist, (unsigned) hostmax * sizeof(struct remote));
		}
		else  {
			hostmax = INITHOSTS;
			hostlist = (struct remote *) malloc(INITHOSTS * sizeof(struct remote));
		}
		if  (!hostlist)
			memory_out();
	}
}

void  addhostentry(const struct remote *arp)
{
	checkhlistsize();
	hostlist[hostnum++] = *arp;
}

void  load_hostfile(const char *fname)
{
	struct	remote	*inp;

	while  ((inp = get_hostfile(fname)))  {
		checkhlistsize();
		hostlist[hostnum++] = *inp;
	}
	end_hostfile();
}

char *phname(netid_t ipadd, const enum IPatype asip)
{
	if  (asip == IPADDR_IP)  {
		struct	in_addr	ina_str;
		ina_str.s_addr = ipadd;
		return  inet_ntoa(ina_str);
	}
	else  {
		struct  hostent  *hp = gethostbyaddr((char *)&ipadd, sizeof(ipadd), AF_INET);
		return  (char *) (hp? hp->h_name: "<unknown>");
	}
}

void  dump_hostfile(FILE *outf)
{
	int	cnt;
	time_t	t = time((time_t *) 0);
	struct	tm	*tp = localtime(&t);

	fprintf(outf, "# Host file created on %.2d/%.2d/%.2d at %.2d:%.2d:%.2d\n\n",
		tp->tm_mday, tp->tm_mon+1, tp->tm_year%100,
		tp->tm_hour, tp->tm_min, tp->tm_sec);

	if  (hadlocaddr != NO_IPADDR)  {
		fprintf(outf, "%s\t", locaddr);
		switch  (hadlocaddr)  {
		default:
		case  IPADDR_NAME:
		case  IPADDR_IP:
			fprintf(outf, "%s\n\n", phname(myhostid, hadlocaddr));
			break;
		case  IPADDR_GSN_NAME:
			fprintf(outf, "GSN(%s,%d)\n\n", gsnname, gsnport);
			break;
		case  IPADDR_GSN_IP:
			fprintf(outf, "GSN(%s,%d)\n\n", phname(gsnid, IPADDR_IP), gsnport);
			break;
		}
	}

	for  (cnt = 0;  cnt < hostnum;  cnt++)  {
		struct	remote	*hlp = &hostlist[cnt];
		if  (hlp->ht_flags & HT_DOS)  {
			if  (hlp->ht_flags & HT_ROAMUSER)
				fprintf(outf, "%s\t%s\t%s", hlp->hostname, hlp->alias[0]? hlp->alias: "-", cluname);
			else  if  (hlp->ht_flags & HT_HOSTISIP)
				fprintf(outf, "%s\t%s\t%s", phname(hlp->hostid, IPADDR_IP), hlp->hostname, clname);
			else
				fprintf(outf, "%s\t%s\t%s", hlp->hostname, hlp->alias[0]? hlp->alias: "-", clname);
			if  (hlp->dosuser[0])
				fprintf(outf, "(%s)", hlp->dosuser);
			if  (hlp->ht_flags & HT_PWCHECK)
				fprintf(outf, ",%s", pwcknam);
		}
		else  {
			int	had = '\t';
			if  (hlp->ht_flags & HT_HOSTISIP)
				fprintf(outf, "%s\t%s", phname(hlp->hostid, IPADDR_IP), hlp->hostname);
			else
				fprintf(outf, "%s\t%s", hlp->hostname, hlp->alias[0]? hlp->alias: "-");
			if  (hlp->ht_flags & HT_PROBEFIRST)  {
				fprintf(outf, "%c%s", had, probestring);
				had = ',';
			}
			if  (hlp->ht_flags & HT_MANUAL)  {
				fprintf(outf, "%c%s", had, manualstring);
				had = ',';
			}
			if  (hlp->ht_flags & HT_TRUSTED)  {
				fprintf(outf, "%c%s", had, trustnam);
				had = ',';
			}
		}
		if  (hlp->ht_timeout != NETTICKLE)
			fprintf(outf, "\t%u", hlp->ht_timeout);
		putc('\n', outf);
	}
	if  (defcluser[0])
		fprintf(outf, "%s\t%s\t%s\n", defclient, defcluser, cluname);
}

int sort_rout(struct remote *a, struct remote *b)
{
	if  (a->ht_flags & HT_ROAMUSER)
		return  b->ht_flags & HT_ROAMUSER? strcmp(a->hostname, b->hostname): 1;
	if  (b->ht_flags & HT_ROAMUSER)
		return  -1;
	if  (sort_type == SORT_IP)  {
		netid_t  na = ntohl(a->hostid), nb = ntohl(b->hostid);
		return  na < nb? -1: na == nb? 0: 1;
	}
	if  (a->ht_flags & HT_DOS)
		return  b->ht_flags & HT_DOS? strcmp(a->hostname, b->hostname): 1;
	if  (b->ht_flags & HT_DOS)
		return  -1;
	return  strcmp(a->hostname, b->hostname);
}

void	sortit()
{
	if  (sort_type != SORT_NONE)
		qsort(QSORTP1 hostlist, (unsigned) hostnum, sizeof(struct remote), QSORTP4 sort_rout);
}

int  hnameclashes(const char *newname)
{
	int	cnt;

	for  (cnt = 0;  cnt < hostnum;  cnt++)  {
		struct	remote	*rp = &hostlist[cnt];
		if  (rp->ht_flags & HT_ROAMUSER)
			continue;
		if  (strcmp(rp->hostname, newname) == 0  ||  strcmp(rp->alias, newname) == 0)
			return  1;
	}
	return  0;
}

char	*ipclashes(const netid_t newip)
{
	int	cnt;
	for  (cnt = 0;  cnt < hostnum;  cnt++)  {
		struct	remote	*rp = &hostlist[cnt];
		if  (rp->ht_flags & HT_ROAMUSER)
			continue;
		if  (rp->hostid == newip)
			return  rp->hostname;
	}
	return  (char *) 0;
}
