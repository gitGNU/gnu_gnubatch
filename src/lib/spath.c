/* spath.c -- get absolute path name from path list

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
#include "incl_unix.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

char *spath(const char *name, const char *curr_dir)
{
        int     lng = strlen(name), lfirst, lngc = strlen(curr_dir);
        char    *pathn = getenv("PATH"), *cp;
        struct  stat    sbuf;
        char    nambuf[PATH_MAX];

        do  {
                cp = strchr(pathn, ':');
                lfirst = cp? cp - pathn: strlen(pathn);
                if  (pathn[0] == '/')  {
                        if  (lfirst + lng + 2 >= PATH_MAX)
                                goto  skipp;
                        strncpy(nambuf, pathn, lfirst);
                        nambuf[lfirst] = '/';
                        strcpy(&nambuf[lfirst + 1], name);
                }
                else  {
                        if  (lngc + lfirst + lng + 3 >= PATH_MAX)
                                goto  skipp;
                        strncpy(nambuf, curr_dir, lngc);
                        nambuf[lngc] = '/';
                        if  (lfirst > 0)  {
                                strncpy(&nambuf[lngc+1], pathn, lfirst);
                                nambuf[lngc+lfirst+1] = '/';
                                strcpy(&nambuf[lngc+lfirst+2], name);
                        }
                        else
                                strcpy(&nambuf[lngc+1], name);
                }
                if  (stat(nambuf, &sbuf) >= 0  &&  (sbuf.st_mode & S_IFMT) == S_IFREG  &&  (sbuf.st_mode & 0111) != 0)
                        return  stracpy(nambuf);
        skipp:
                pathn = cp + 1;
        }  while  (cp);

        return  (char *) 0;
}
