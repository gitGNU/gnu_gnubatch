/* bgtkldsv.c -- GTK program file manipulation

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
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include "defaults.h"
#include "incl_unix.h"
#include "files.h"
#include "ecodes.h"

#define	BUFFSIZE	256

int  readfile(char *fl)
{
	FILE  *inf = fopen(fl, "r");
	size_t	nb;
	char	buf[1024];

	if  (!inf)
		return  E_NOJOB;

	while  ((nb = fread(buf, 1, sizeof(buf), inf))  !=  0)
		fwrite(buf, 1, nb, stdout);
	fclose(inf);
	return  0;
}

int	writefile(char *fl)
{
	FILE  *outf = fopen(fl, "w");
	size_t	nb;
	char	buf[1024];
	int	um = umask(0);
	umask(um);

	if  (!outf)
		return  E_NOJOB;
	while  ((nb = fread(buf, 1, sizeof(buf), stdin))  !=  0)
		fwrite(buf, 1, nb, outf);
	/* This is to try to turn on executable bits */
#ifdef	HAVE_FCHMOD
	fchmod(fileno(outf), 0777 & ~um);
#endif
	fclose(outf);
#ifndef	HAVE_FCHMOD
	chmod(fl, 0777 & ~um);
#endif
	return  0;
}

/*  Arguments are:
    -w create/open file for writing (we read from pipe) / -r open file for reading (we write to pipe)
    file name -d just delete.
    Run as root to switch to appropriate User ID */

MAINFN_TYPE  main(int argc, char **argv)
{
	struct  passwd  *pw = getpwnam(BATCHUNAME);
	uid_t  duid = ROOTID, myuid = getuid(), mygid = getgid();

	versionprint(argv, "$Revision: 1.6 $", 1);

	/* Get batch uid */

	if  (pw)
		duid = pw->pw_uid;
	endpwent();

	/* If the real user id is "batch" this is either because we have invoked the
	   original ui program as batch or because we have invoked a GTK program which
	   switched the real uid to batch. In such cases we fish the uid out of the
	   home directory and use that.

	   Refuse to work if no home directory or it looks strange */

	if  (myuid == duid)  {
		char	*homed = getenv("HOME");
		struct  stat  sbuf;

		if  (!homed  ||  stat(homed, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)
			exit(E_SETUP);

		myuid = sbuf.st_uid;
		mygid = sbuf.st_gid;
	}

	/* Switch completely to the user id and do the bizniz. */

	setgid(mygid);
	setuid(myuid);

	/* If we haven't got the right arguments then just quit.
	   This is only meant to be run by xbtr.
	   Maybe one day we'll have a more sophisticated routine. */

	if  (argc != 3  ||  argv[1][0] != '-')
		return  E_USAGE;

	switch  (argv[1][1])  {
	default:
		return  E_USAGE;
	case  'r':
		return  readfile(argv[2]);
	case  'w':
		return  writefile(argv[2]);
	case  'd':
		return  unlink(argv[2]) >= 0? 0: E_NOJOB;
	}
}
