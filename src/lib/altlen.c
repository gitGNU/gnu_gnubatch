/* altlen.c -- Give the maximum field width of a helpalt structure.

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
#include "incl_unix.h"
#include "helpalt.h"

int  altlen(CHelpaltRef a)
{
        int     ik, result = 0, lk;

        if  (!a)
                return  0;

        for  (ik = 0;  ik < a->numalt;  ik++)
                if  ((lk = strlen(a->list[ik])) > result)
                        result = lk;
        return  result;
}
