/* lookup_alt.c -- lookup value corresponding to alt string

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
#include "helpalt.h"
#include "errnums.h"

/* This is a pathetic linear search on the basis that it's not worth
   anything more complicated for the half-dozen or so items max that we use */

int  lookup_alt(const char *val, HelpaltRef tab)
{
        int     i;

        for  (i = 0;  i < tab->numalt;  i++)
                if  (strcmp(val, tab->list[i]) == 0)
                        return  tab->alt_nums[i];
        return  -30000;
}
