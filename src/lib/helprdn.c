/* helprdn.c -- read number from help file

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
#include "errnums.h"

int  helprdn()
{
        int     ch, result = 0, minus = 0;

        if  ((ch = getc(Cfile)) == '-')  {
                minus = 1;
                ch = getc(Cfile);
        }

        while  (isdigit(ch))  {
                result = result * 10 + ch - '0';
                ch = getc(Cfile);
        }
        ungetc(ch, Cfile);
        return  minus? -result: result;
}
