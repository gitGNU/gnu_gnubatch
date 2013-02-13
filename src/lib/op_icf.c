/* op_icf.c -- open message file for internal programs

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
#include "incl_unix.h"
#include "files.h"

FILE *open_icfile()
{
        char    *filename;
        FILE    *res;

        filename = envprocess(INT_CONFIG);
        if  ((res = fopen(filename, "r")) == (FILE *) 0)  {
                fprintf(stderr,
                        "Help cannot open internal config file `%s'\n",
                        filename);
                return  (FILE *) 0;
        }
        free(filename);
        return  res;
}
