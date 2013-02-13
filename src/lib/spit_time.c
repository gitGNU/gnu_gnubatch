/* spit_time.c -- output time parameters

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
#include "helpargs.h"
#include "helpalt.h"
#include "optflags.h"

int  spit_time(FILE *dest, CTimeconRef tcr, int cancont, const ULONG davset, const int mdset)
{
        int     jn;
        struct  tm      *t = 0; /* Initialise to stop warnings */
        static  const  short  nplookup[] =  {
                $A{btr arg skip},
                $A{btr arg hold},
                $A{btr arg resched},
                $A{btr arg catchup}
        };

        if  (!tcr->tc_istime)
                return  spitoption($A{btr arg notime}, $A{btr arg explain}, dest, ' ', cancont);

        if  (tcr->tc_repeat == TC_DELETE || tcr->tc_repeat == TC_RETAIN)
                cancont = spitoption(tcr->tc_repeat == TC_DELETE? $A{btr arg delete}:
                                $A{btr arg norep}, $A{btr arg explain}, dest, ' ', cancont);

        cancont = spitoption(tcr->tc_nposs > TC_CATCHUP? $A{btr arg skip}:
                                nplookup[tcr->tc_nposs],
                                $A{btr arg explain}, dest, ' ', cancont);

        if  (tcr->tc_nexttime != 0L)  {
                t = localtime(&tcr->tc_nexttime);
                spitoption($A{btr arg time}, $A{btr arg explain}, dest, ' ', 0);
                if  (davset & OF_DATESET)
                        fprintf(dest, " %.2d/%.2d/%.2d,%.2d:%.2d",
                                       t->tm_year % 100, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
                else
                        fprintf(dest, " %.2d:%.2d", t->tm_hour, t->tm_min);
        }
        if  (tcr->tc_repeat >= TC_MINUTES)  {
                spitoption($A{btr arg repeat}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s:%ld",
                               disp_alt((int) (tcr->tc_repeat - TC_MINUTES), repunit),
                               (long) tcr->tc_rate);

                /* Gyrations with day of month.  For jobdump and xmbtr
                   equiv the day of month has already been worked
                   out and we have to convert back. mdset = -1
                   For btr and rbtr mdset is 0 if the guy didn't
                   include it in the arg 1 if he did. In the
                   latter case we reproduce what he put.  */

                if  ((tcr->tc_repeat == TC_MONTHSB || tcr->tc_repeat == TC_MONTHSE) && mdset != 0)  {
                        int     mday = tcr->tc_mday;
                        if  (mdset < 0  &&  tcr->tc_repeat == TC_MONTHSE)  {
                                month_days[1] = t->tm_year % 4 == 0? 29: 28; /* t = localtime above */
                                mday = month_days[t->tm_mon] - tcr->tc_mday;
                                if  (mday <= 0)
                                        mday = 1;
                        }
                        fprintf(dest, ":%d", mday);
                }
        }
        if  (davset & OF_AVOID_CHANGES)  {
                spitoption($A{btr arg avoid}, $A{btr arg explain}, dest, ' ', 0);
                fputs(" -", dest);
                for  (jn = 0;  jn < TC_NDAYS;  jn++)
                        if  (tcr->tc_nvaldays & (1 << jn))
                                fprintf(dest, ",%s", disp_alt(jn, days_abbrev));
        }
        return  0;
}
