/* repmnthfix.c -- for fixing up "day of month" in shell commands

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

/*----------------------------------------------------------------------
 * Fix up month day argument which we have just read in
 * Return 0 if OK otherwise error code.
 */

#include "config.h"
#include <sys/types.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "timecon.h"

extern  char    month_days[];

int  repmnthfix(TimeconRef tcr)
{
        struct  tm      *t;
        int     curml;

        if  (tcr->tc_repeat != TC_MONTHSB  &&  tcr->tc_repeat != TC_MONTHSE)
                return  0;

        t = localtime(&tcr->tc_nexttime);
        month_days[1] = t->tm_year % 4 == 0? 29: 28;
        curml = month_days[t->tm_mon];
        if  (tcr->tc_repeat == TC_MONTHSB)  {
                if  (tcr->tc_mday == 0)
                        tcr->tc_mday = t->tm_mday;
                return  0;
        }
        if  (tcr->tc_mday == 0)
                tcr->tc_mday = curml - t->tm_mday;
        else    if  ((int) tcr->tc_mday >= curml)
                tcr->tc_mday = 0;
        else
                tcr->tc_mday = curml - tcr->tc_mday;
        if  (tcr->tc_mday >= 21)
                return  $E{Could repeat endlessly};
        return  0;
}
