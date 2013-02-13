/* gprompt.c -- get prompt or bust

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

char *gprompt(const int code)
{
        char    *result;

        if  ((result = helpprmpt(code)) == (char *) 0)  {
                char    **hv;

                disp_arg[9] = code;
                hv = helpvec($E{Missing prompt code}, 'E');

                if  (!*hv)  {
                        free((char *) hv);
                        result = stracpy("Very mangled control file");
                }
                else  {
                        result = hv[0];
                        free((char *) hv);
                }
        }
        return  result;
}
