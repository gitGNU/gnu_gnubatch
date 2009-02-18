/* hostedit.c -- main module for host file editing

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
#include <fcntl.h>
#include <time.h>
#include "defaults.h"
#include "incl_unix.h"
#include "networkincl.h"
#include "remote.h"

extern	void	proc_hostfile(), end_hostfile();
extern struct remote *get_hostfile(const char *);

extern	const	char	locaddr[], probestring[], manualstring[], dosname[],
			clname[], dosuname[], cluname[], extname[], pwcknam[],
			trustnam[], defclient[];

struct	remote	*hostlist;
int	hostnum, hostmax;

extern	int	hadlocaddr;
extern	char	defcluser[];
extern	netid_t	myhostid;
extern	char	hostf_errors;

#define	INITHOSTS	20
#define	INCHOSTS	10

enum  { SORT_NONE, SORT_HNAME, SORT_IP } sort_type;

void	memory_out()
{
	fprintf(stderr, "Run out of memory\n");
	exit(255);
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

void	addhostentry(struct remote *arp)
{
	checkhlistsize();
	hostlist[hostnum++] = *arp;
}

void	load_hostfile(const char *fname)
{
	struct	remote	*inp;

	while  ((inp = get_hostfile(fname)))  {
		checkhlistsize();
		hostlist[hostnum++] = *inp;
	}
	end_hostfile();
}

char  *phname(netid_t ipadd, const int asip)
{
	if  (asip)  {
		struct	in_addr	ina_str;
		ina_str.s_addr = ipadd;
		return  inet_ntoa(ina_str);
	}
	else  {
		struct  hostent  *hp = gethostbyaddr((char *)&ipadd, sizeof(ipadd), AF_INET);
		return  (char *) (hp? hp->h_name: "<unknown>");
	}
}

void	dump_hostfile(FILE *outf)
{
	int	cnt;
	time_t	t = time((time_t *) 0);
	struct	tm	*tp = localtime(&t);

	fprintf(outf, "# Host file created on %.2d/%.2d/%.2d at %.2d:%.2d:%.2d\n\n",
		tp->tm_mday, tp->tm_mon+1, tp->tm_year%100,
		tp->tm_hour, tp->tm_min, tp->tm_sec);

	if  (hadlocaddr)
		fprintf(outf, "%s\t%s\n\n", locaddr, phname(myhostid, hadlocaddr == 1));

	for  (cnt = 0;  cnt < hostnum;  cnt++)  {
		struct	remote	*hlp = &hostlist[cnt];
		if  (hlp->ht_flags & HT_DOS)  {
			if  (hlp->ht_flags & HT_ROAMUSER)
				fprintf(outf, "%s\t%s\t%s", hlp->hostname, hlp->alias[0]? hlp->alias: "-", cluname);
			else  if  (hlp->ht_flags & HT_HOSTISIP)
				fprintf(outf, "%s\t%s\t%s", phname(hlp->hostid, 1), hlp->hostname, clname);
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
				fprintf(outf, "%s\t%s", phname(hlp->hostid, 1), hlp->hostname);
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

int	sort_rout(struct remote *a, struct remote *b)
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

MAINFN_TYPE	main(int argc, char **argv)
{
	int	ch, outfd, inplace = 0;
	FILE	*outfil;
	char	*inf = (char *) 0, *outf = (char *) 0;
	extern	char	*optarg;
	extern	int	optind;

	while  ((ch = getopt(argc, argv, "Io:s:")) != EOF)  {
		switch  (ch)  {
		default:
			fprintf(stderr, "Usage: %s [-I] [-o file] [file]\n", argv[0]);
			return  1;
		case  'I':
			inplace++;
			continue;
		case  'o':
			outf = optarg;
			continue;
		case  's':
			switch  (optarg[0])  {
			default:
				sort_type = SORT_NONE;
				continue;
			case  'h':
				sort_type = SORT_HNAME;
				continue;
			case  'i':
				sort_type = SORT_IP;
				continue;
			}
		}
	}

	if  (argv[optind])
		inf = argv[optind];
	else  if  (inplace)  {
		fprintf(stderr, "-I option requires file arg\n");
		return  1;
	}

	if  (outf)  {
		if  (inplace)  {
			fprintf(stderr, "-I option and -o %s option are not compatible\n", outf);
			return  1;
		}
		if  (!freopen(outf, "w", stdout))  {
			fprintf(stderr, "Cannot open output file %s\n", outf);
			return  2;
		}
	}

	load_hostfile(inf);
	if  (hostf_errors)  {
		fprintf(stderr, "Warning: There were error(s) in your host file!\n");
		fflush(stderr);
		sleep(2);
	}

	if  (inplace  &&  !freopen(inf, "w", stdout))  {
		fprintf(stderr, "Cannot reopen input file %s for writing\n", inf);
		return  5;
	}

	if  (sort_type != SORT_NONE)
		qsort(QSORTP1 hostlist, (unsigned) hostnum, sizeof(struct remote), QSORTP4 sort_rout);

	outfd = dup(1);
	if  (!(outfil = fdopen(outfd, "w")))  {
		fprintf(stderr, "Cannot dup output file\n");
		return  3;
	}
	close(0);
	close(1);
	close(2);
	if  (dup(dup(open("/dev/tty", O_RDWR))) < 0)
		exit(99);
	proc_hostfile();
	dump_hostfile(outfil);
	return  0;
}
