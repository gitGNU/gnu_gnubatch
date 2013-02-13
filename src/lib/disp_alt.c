/* disp_alt.c -- spew the string out of a "Helpalt" structure

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

char *disp_alt(int val, HelpaltRef tab)
{
        int     i;
        char    **emess;

        for  (i = 0;  i < tab->numalt;  i++)
                if  (val == tab->alt_nums[i])
                        return  tab->list[i];

        disp_arg[9] = val;
        emess = helpvec($E{Scrambled state code}, 'E');
        if  (emess[0] == (char *) 0)  {
                free((char *) emess);
                disp_arg[9] = $E{Scrambled state code};
                emess = helpvec($E{Missing error code}, 'E');
        }
        return  emess[0]?  emess[0]: "Total cockup";
}
