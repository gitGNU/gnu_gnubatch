/* galts.c -- read and check alternatives in Curses clients

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
#include <curses.h>
#include "helpalt.h"
#include "errnums.h"

extern void  doerror(WINDOW *, int);

HelpaltRef  galts(WINDOW *wp, const int state, const int num)
{
        HelpaltRef      a = helprdalt(state);

        if  (a == (HelpaltRef) 0)  {
                disp_arg[9] = state;
                doerror(wp, $E{Missing alternative code});
                return  (HelpaltRef) 0;
        }

        if  (a->numalt != num)  {
                disp_arg[9] = state;
                disp_arg[8] = num;
                disp_arg[7] = a->numalt;
                doerror(wp, $E{Wrong number alternatives});
                freealts(a);
                return  (HelpaltRef) 0;
        }

        return  a;
}
