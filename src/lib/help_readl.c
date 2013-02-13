/* help_readl.c -- read line from help file

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
#include "incl_unix.h"
#include "errnums.h"

static  char    Filename[] = __FILE__;

char    *help_readl(int *percentflag)
{
        long    w = ftell(Cfile);
        unsigned        l = 1;          /*  Final null  */
        int     ch;
        char    *result, *rp;

        while  ((ch = getc(Cfile)) != '\n' && ch != EOF)
                l++;

        fseek(Cfile, w, 0);
        if  ((rp = result = malloc(l)) == (char *) 0)
                ABORT_NOMEM;
        while  ((ch = getc(Cfile)) != '\n' && ch != EOF)
                if  ((*rp++ = (char) ch) == '%')
                        *percentflag |= 1;
        *rp = '\0';
        return  result;
}
