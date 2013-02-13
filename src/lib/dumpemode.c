/* dumpemode.c -- dump "extended" modes

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
#include "defaults.h"
#include "btmode.h"

void  dumpemode(FILE *outf, const char sep, const char prefix, const int typ, unsigned md)
{
        fprintf(outf, "%c%c:", sep, prefix);
        if  (typ == MODE_ON)
                putc('+', outf);
        else  if  (typ == MODE_OFF)
                putc('-', outf);
        if  (md & BTM_READ)
                putc('R', outf);
        if  (md & BTM_WRITE)
                putc('W', outf);
        if  (md & BTM_SHOW)
                putc('S', outf);
        if  (md & BTM_RDMODE)
                putc('M', outf);
        if  (md & BTM_WRMODE)
                putc('P', outf);
        if  (md & BTM_UGIVE)
                putc('U', outf);
        if  (md & BTM_UTAKE)
                putc('V', outf);
        if  (md & BTM_GGIVE)
                putc('G', outf);
        if  (md & BTM_GTAKE)
                putc('H', outf);
        if  (md & BTM_DELETE)
                putc('D', outf);
        if  (md & BTM_KILL)
                putc('K', outf);
}
