/* versionprint.c -- interpret "--version" argument

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
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"

/* This is my version number */

const   char    GNUbatch_version[] = GNU_BATCH_VERSION_STRING;

/* Translate $Revision: 1.whetever $ string into just version number */

static  const   char  *xlate_vers(const char *progvers, char *buf)
{
        const  char  *cp = strchr(progvers, ':');
        char    *rbuf = buf;
        if  (!cp)
                return  "(Initial version)";
        do  cp++;
        while  (isspace(*cp));
        do  *rbuf++ = *cp++;
        while  (*cp && !isspace(*cp)  &&  *cp != '$');
        *rbuf = '\0';
        return  buf;
}

void    versionprint(char **argv, const char *progvers, const int internal)
{
        if  (argv[1])  {
                const  char  *arg = argv[1];

                if  (strcmp(arg, "--version") == 0)  {
                        const  char  *prog = argv[0], *cp, *pv;
                        char    pvers[20];
                        if  ((cp = strrchr(prog, '/')))
                                prog = cp + 1;
                        pv = xlate_vers(progvers, pvers);
                        fprintf(stderr, "This is %s version %s, a component of GNUbatch version %s\n", prog, pv, GNUbatch_version);
                        fputs("Copyright (C) 2009-2013 Free Software Foundation, Inc.\n"
                              "This is free software; see the source for copying conditions.  There is NO\n"
                              "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n", stderr);
                        exit(0);
                }
                if  (internal  &&  strcmp(arg, "--help") == 0)  {
                        const  char  *prog = argv[0], *cp, *pv;
                        char    pvers[20];
                        if  ((cp = strrchr(prog, '/')))
                                prog = cp + 1;
                        pv = xlate_vers(progvers, pvers);
                        fprintf(stderr, "This is %s version %s,\nan internal component of GNUbatch version %s\n", prog, pv, GNUbatch_version);
                        fputs("This program is not intended to be run other than internally\nPlease do not run it manually\n", stderr);
                        exit(0);
                }
        }
}
