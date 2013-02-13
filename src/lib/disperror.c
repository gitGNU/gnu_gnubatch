/* disperror.c -- shell level error print

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

void  fprint_error(FILE *fp, int Errnum)
{
        char    **emess = helpvec(Errnum, 'E'), **ep;

        if  (emess[0] == (char *) 0)  {
                free((char *) emess);
                disp_arg[9] = Errnum;
                emess = helpvec($E{Missing error code}, 'E');
        }
        for  (ep = emess;  *ep;  ep++)  {
                fprintf(fp, "%s\n", *ep);
                free(*ep);
        }
        free((char *) emess);
}

void  print_error(int Errnum)
{
        fprint_error(stderr, Errnum);
}
