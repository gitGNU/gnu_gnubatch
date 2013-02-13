/* helphdr.c -- get header (for Curses programs)

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
#include "statenums.h"

static  char    Filename[] = __FILE__;

#define MAXLINES        10

char **helphdr(const int chr)
{
        char    **result;
        int     lnum, maxl = 0, ch, perc = 0, hadeof = 0;

        if  ((result = (char **) malloc((MAXLINES+1) * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;
        for  (lnum = 0;  lnum < MAXLINES+1;  lnum++)
                result[lnum] = (char *) 0;

        for  (;;)  {
                if  ((ch = getc(Cfile)) == EOF)  {
                        if  (hadeof)
                                goto  dun;
                        fseek(Cfile, 0L, 0);
                        hadeof++;
                        continue;
                }

                if  (ch == chr  ||  ch == chr - 'A' + 'a')  {
                        lnum = helprdn();
                        if  ((ch = getc(Cfile)) != ':')
                                goto  skipl;
                        if  (lnum <= 0 || lnum > MAXLINES)
                                goto  skipl;
                        if  (lnum > maxl)
                                maxl = lnum;
                        result[lnum - 1] = help_readl(&perc);
                        continue;
                }
        skipl:
                while  (ch != '\n' && ch != EOF)
                        ch = getc(Cfile);
                continue;
        }
dun:
        for  (lnum = 0;  lnum < maxl;  lnum++)
                if  (!result[lnum])
                        result[lnum] = stracpy("");
        if  (perc)
                result = mmangle(result);
        return  result;
}
