/* dumpcon.c -- dump out "constant" values.

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
#include "btconst.h"

void  dumpstr(FILE *, char *);

void  dumpcon(FILE *dest, BtconRef con)
{
        if  (con->const_type == CON_STRING)  {
                char    *str = con->con_un.con_string;
                if  (isdigit(str[0]))
                        putc(':', dest);
                dumpstr(dest, str);
        }
        else
                fprintf(dest, "%ld", (long) con->con_un.con_long);
}
