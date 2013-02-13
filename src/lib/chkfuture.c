/* chkfuture.c -- moan if time not in future (for shell progs)

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
#include "defaults.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "helpalt.h"
#include "errnums.h"
#include "ecodes.h"
#include "incl_unix.h"

void  chkfuture(BtjobRef jp, const int verb)
{
        if  (jp->h.bj_times.tc_istime  &&  jp->h.bj_times.tc_nexttime <= jp->h.bj_time)  {
                if  (jp->h.bj_times.tc_nexttime == 0L)
                        jp->h.bj_times.tc_nexttime = ((time((time_t *) 0) + 59L) / 60L) * 60L;
                else  if  (jp->h.bj_progress == BJP_NONE  &&  Realuid != Daemuid  &&  Realuid != ROOTID)  {
                        if  (jp->h.bj_times.tc_repeat <= TC_RETAIN)  {
                                print_error($E{Time not in future});
                                exit(E_USAGE);
                        }
                        do  jp->h.bj_times.tc_nexttime = advtime(&jp->h.bj_times);
                        while  (jp->h.bj_times.tc_nexttime <= jp->h.bj_time);
                        if  (verb)  {
                                struct  tm      *t = localtime(&jp->h.bj_times.tc_nexttime);
                                int     day = t->tm_mday, mon = t->tm_mon+1;
                                char    tbuf[40];
                                /* More catering for those dyslexic yanks....  */
#ifdef  HAVE_TM_ZONE
                                if  (t->tm_gmtoff <= -4 * 60 * 60)
#else
                                if  (timezone >= 4 * 60 * 60)
#endif
                                {
                                        int     tmp = mon;
                                        mon = day;
                                        day = tmp;
                                }

                                sprintf(tbuf, "%.2d:%.2d %s %.2d/%.2d/%.4d",
                                        t->tm_hour, t->tm_min,
                                        disp_alt(t->tm_wday, days_abbrev),
                                        day, mon, t->tm_year+1900);
                                disp_str = tbuf;
                                print_error($E{Past time incremented});
                        }
                }
        }
}
