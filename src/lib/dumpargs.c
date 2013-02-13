/* dumpargs.c -- dump out job arguments

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
#include "defaults.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"

void  spitbtrstr(const int, FILE *, const int);

void  dumpargs(FILE *xfl, CBtjobRef jp)
{
        int     cnt;

        spitbtrstr($A{btr arg cancarg}, xfl, 1);
        for  (cnt = 0;  cnt < jp->h.bj_nargs;  cnt++)  {
                const   char    *cp = ARG_OF(jp, cnt);

                spitbtrstr($A{btr arg argument}, xfl, 0);
                putc('\'', xfl);
                while  (*cp)  {
                        if  (*cp == '\'')
                                fputs("\'\\\'", xfl);
                        putc(*cp, xfl);
                        cp++;
                }
                fputs("\' \\\n", xfl);
        }
}
