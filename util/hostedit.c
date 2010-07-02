/* hostedit.c -- main module for host file editing

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
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "defaults.h"
#include "incl_unix.h"
#include "networkincl.h"
#include "remote.h"
#include "hostedit.h"

int	main(int argc, char **argv)
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

	sortit();

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
