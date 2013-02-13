/* helpvec.c -- extract string vector from help file

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
#include <errno.h>
#include "incl_unix.h"
#include "errnums.h"
#include "statenums.h"

static  char    Filename[] = __FILE__;

#define INITLINES       5
#define INCLINES        2

int     save_errno;

extern  int     sys_nerr;

char **helpvec(const int n, const char chr)
{
        int     ch, lnum = 0, totlines = INITLINES, percentflag = 0;
        char    **result;

        save_errno = errno;     /*  Before someone else mangles it  */

        if  ((result = (char **) malloc((INITLINES + 1) * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        fseek(Cfile, 0L, 0);

        for  (;;)  {
                if  ((ch = getc(Cfile)) == EOF)  {
                        result[lnum] = (char *) 0;
                        if  (percentflag)
                                return  mmangle(result);
                        return  result;
                }

                /* If line doesn't start with the char we are looking
                   for forget it.  */

                if  (ch != chr)  {
skipn:                  while  (ch != '\n'  &&  ch != EOF)
                                ch = getc(Cfile);
                        continue;
                }

                /* Read number.  */

                if  (helprdn() != n)  {
                        ch = getc(Cfile);
                        goto  skipn;
                }

                /* Check for terminating colon.  */

                if  ((ch = getc(Cfile)) != ':')
                        goto  skipn;

                /* We got an exact match, so stuff it into our buffer */

                if  (lnum >= totlines)  {
                        totlines += INCLINES;
                        result = (char **) realloc((char *) result,
                                                   (totlines+1) * sizeof(char *));
                        if  (result == (char **) 0)
                                ABORT_NOMEM;
                }

                result[lnum] = help_readl(&percentflag);
                lnum++;
        }
}

/* Get me dimensions.  */

void  count_hv(char **hv, int *rp, int *cp)
{
        int     l, rows = 0, cols = 0;

        if  (hv)  {
                for  (;  *hv;  hv++)  {
                        rows++;
                        if  ((l = strlen(*hv)) > cols)
                                cols = l;
                }
        }
        if  (rp)
                *rp = rows;
        if  (cp)
                *cp = cols;
}

/* Deallocate a help vector */

void  freehelp(char **hv)
{
        char    **hp;

        if  (hv)  {
                for  (hp = hv;  *hp;  hp++)
                        free(*hp);
                free((char *) hv);
        }
}
