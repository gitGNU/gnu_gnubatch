/* shmdump.c -- dump out shared memory

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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <ctype.h>
#include "incl_unix.h"

#define	DEFAULT_NWORDS	4
#define	MAX_NWORDS	8

unsigned  char	lastbuf[MAX_NWORDS * sizeof(ULONG)];
unsigned  had = 0, hadsame = 0;
unsigned  blksize = DEFAULT_NWORDS;		/* Bumped up to bytes later */

char	npchar = '.';

void	printnext(unsigned addr, unsigned len, unsigned char *b)
{
	unsigned  cnt;

	if  (had  &&  len == blksize  &&  memcmp(b, lastbuf, blksize) == 0)  {
		if  (!hadsame)  {
			hadsame++;
			printf("%.6x -- same ---\n", addr);
		}
		return;
	}
	had++;
	hadsame = 0;
	memcpy(lastbuf, b, blksize);
	printf("%.6x ", addr);
	for  (cnt = 0;  cnt < len;  cnt++)  {
		printf("%.2x", b[cnt]);
		if  ((cnt & (sizeof(ULONG)-1)) == sizeof(ULONG)-1)
			putchar(' ');
	}
	if  (len < blksize)  {
		unsigned  col;
		for  (col = len * 2 + len/sizeof(ULONG);  col < blksize/sizeof(ULONG) * (2*sizeof(ULONG)+1);  col++)
			putchar(' ');
	}
	for  (cnt = 0;  cnt < len;  cnt++)  {
		putchar(isprint(b[cnt])? b[cnt]: npchar);
		if  ((cnt & (sizeof(ULONG)-1)) == (sizeof(ULONG)-1)  &&  cnt != len-1)
			putchar(' ');
	}
	putchar('\n');
}

MAINFN_TYPE	main(int argc, char **argv)
{
	int	c, chan, base;
	unsigned  addr = 0, left;
	unsigned  char  *shp, *nxt;
	extern	char	*optarg;
	extern	int	optind;
	struct  shmid_ds	sbuf;

	while  ((c = getopt(argc, argv, "N:B:")) != EOF)
		switch  (c)  {
		default:
			goto  usage;
		case  'N':
			npchar = optarg[0];
			continue;
		case  'B':
			blksize = atoi(optarg);
			if  (blksize == 0  ||  blksize > MAX_NWORDS)
				goto  usage;
		}

	if  (!argv[optind])  {
	usage:
		fprintf(stderr, "Usage: %s [-N<char>] [-Bn] ipc\n", argv[0]);
		return  1;
	}

	blksize *= sizeof(ULONG);

	base = strtoul(argv[optind], 0, 0);
	if  ((chan = shmget((key_t) base, 0, 0)) < 0  ||  shmctl(chan, IPC_STAT, &sbuf) < 0)  {
		fprintf(stderr, "Cannot open shm key %x\n", base);
		return  2;
	}
	if  ((shp = (unsigned char *) shmat(chan, 0, 0)) == (unsigned char *) -1)  {
		fprintf(stderr, "Could not attach shm key %x\n", chan);
		return  3;
	}

	nxt = shp;
	left = sbuf.shm_segsz;

	while  (left > 0)  {
		unsigned  pce = left > blksize? blksize: left;
		printnext(addr, pce, nxt);
		addr += pce;
		left -= pce;
		nxt += pce;
	}
	printf("%.6x --- end ---\n", addr);
	return  0;
}
