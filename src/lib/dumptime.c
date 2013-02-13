/* dumptime.c -- dump time

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
#include "helpalt.h"
#include "timecon.h"

void  spitbtrstr(const int, FILE *, const int);

void  dumptime(FILE *ofl, CTimeconRef tcr)
{
        struct  tm  *t;
        int     nd;
        static  const   unsigned  short lookupnp[] =  {
                $A{btr arg skip},
                $A{btr arg hold},
                $A{btr arg resched},
                $A{btr arg catchup}
        };

        if  (tcr->tc_istime == 0)  {
                spitbtrstr($A{btr arg notime}, ofl, 1);
                return;
        }

        t = localtime(&tcr->tc_nexttime);
        spitbtrstr($A{btr arg time}, ofl, 0);
        fprintf(ofl, "%.2d/%.2d/%.2d,%.2d:%.2d \\\n",
                       t->tm_year % 100,
                       t->tm_mon + 1,
                       t->tm_mday,
                       t->tm_hour,
                       t->tm_min);

        spitbtrstr($A{btr arg avoid}, ofl, 0);
        putc('-', ofl);

        for  (nd = 0;  nd < TC_NDAYS;  nd++)
                if  (tcr->tc_nvaldays & (1 << nd))
                        fprintf(ofl, ",%s", disp_alt(nd, days_abbrev));
        fputs(" \\\n", ofl);

        if  (tcr->tc_repeat < TC_MINUTES)
                spitbtrstr(tcr->tc_repeat == TC_DELETE?
                           $A{btr arg delete}: $A{btr arg norep}, ofl, 1);
        else  {
                spitbtrstr($A{btr arg repeat}, ofl, 0);
                if  (tcr->tc_repeat == TC_MONTHSB)
                        fprintf(ofl, "\'%s:%ld:%d\' \\\n", disp_alt((int) (tcr->tc_repeat - TC_MINUTES), repunit), (long) tcr->tc_rate, tcr->tc_mday);
                else  if  (tcr->tc_repeat == TC_MONTHSE)  {
                        int     mday;
                        month_days[1] = t->tm_year % 4 == 0? 29: 28;
                        mday = month_days[t->tm_mon] - tcr->tc_mday;
                        if  (mday <= 0)
                                mday = 1;
                        fprintf(ofl, "\'%s:%ld:%d\' \\\n", disp_alt((int) (tcr->tc_repeat - TC_MINUTES), repunit), (long) tcr->tc_rate, mday);
                }
                else
                        fprintf(ofl, "\'%s:%ld\' \\\n", disp_alt((int) (tcr->tc_repeat - TC_MINUTES), repunit), (long) tcr->tc_rate);
        }

        spitbtrstr(tcr->tc_nposs > TC_CATCHUP? $A{btr arg skip}: lookupnp[tcr->tc_nposs], ofl, 1);
}
