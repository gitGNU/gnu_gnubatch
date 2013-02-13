/* spitbtrstr.c -- Emit command argument (specific to jobdump etc)

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
#include "helpargs.h"

void  spitbtrstr(const int arg, FILE *xfl, const int term)
{
        int     v = arg - $A{btr arg explain};

        if  (optvec[v].isplus)
                fprintf(xfl, " +%s ", optvec[v].aun.string);
        else  if  (optvec[v].aun.letter == 0)
                fprintf(xfl, " +missing-arg-code-%d ", arg);
        else
                fprintf(xfl, " -%c ", optvec[v].aun.letter);
        if  (term)
                fputs("\\\n", xfl);
}
