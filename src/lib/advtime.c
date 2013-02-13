/* advtime.c -- advance to next time as per repeat option

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
#include <sys/types.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "defaults.h"
#include "files.h"
#include "timecon.h"
#include "incl_unix.h"

/* People who use this are at liberty to reset February as required */

char    month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

#define SECSPERDAY      (24 * 60 * 60L)

static  int     holf_fd = -1;

/* Decide whether given day is a holiday */

static int  is_holiday(time_t when)
{
        struct  tm      *tp;
        unsigned  char  holmap[YVECSIZE];
        if  (holf_fd < 0)  {
                char    *fname = envprocess(HOLFILE);
                holf_fd = open(fname, O_RDONLY);
                free(fname);
                if  (holf_fd < 0)
                        return  0;
                fcntl(holf_fd, F_SETFD, 1);
        }

        /* We choose not to understand years before 1990 */

        tp = localtime(&when);
        if  (tp->tm_year < 90)
                return  0;

        /* Offset in file is 366/8 bytes times year after 1990 */

        lseek(holf_fd, (long) ((tp->tm_year - 90) * YVECSIZE), 0);
        if  (read(holf_fd, (char *) holmap, sizeof(holmap)) != sizeof(holmap))
                return  0;
        return  holmap[tp->tm_yday / 8] & (1 << (tp->tm_yday & 7));
}

/* Advance one time unit.  */

time_t  advtime(CTimeconRef tc)
{
        time_t          newtime = tc->tc_nexttime;
        unsigned        nvdays = tc->tc_nvaldays, mday = tc->tc_mday;
        ULONG           amt = tc->tc_rate;
        int     dw, mon, nly;
        struct  tm      *btime = localtime(&newtime);

        /* Safety.  */

        if  ((nvdays & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)
                nvdays = 0;

        switch  (tc->tc_repeat)  {
        default:
                return  newtime;

        case  TC_MINUTES:
                newtime += amt * 60;
                break;

        case  TC_HOURS:
                newtime += amt * 60 * 60;
                break;

        case  TC_DAYS:
                newtime += amt * SECSPERDAY;
                break;

        case  TC_WEEKS:
                newtime += amt * 7 * SECSPERDAY;
                break;

        case  TC_MONTHSB:
                nly = btime->tm_year % 4;
                month_days[1] = nly? 28: 29;
                mon = btime->tm_mon;
                newtime += ((int) mday - btime->tm_mday) * SECSPERDAY;
                btime->tm_mday = mday;
                while  (amt != 0)  {
                        newtime += month_days[mon] * SECSPERDAY;
                        if  (++mon >= 12)  {
                                mon = 0;
                                nly = (nly + 1) % 4;
                                month_days[1] = nly? 28: 29;
                        }
                        amt--;
                }
                if  (btime->tm_mday > month_days[mon])
                        newtime -= (btime->tm_mday - month_days[mon]) * SECSPERDAY;
                break;

        case  TC_MONTHSE:
                nly = btime->tm_year % 4;
                month_days[1] = nly? 28: 29;
                mon = btime->tm_mon;
                while  (amt != 0)  {
                        newtime += month_days[mon] * SECSPERDAY;
                        if  (++mon >= 12)  {
                                mon = 0;
                                nly = (nly + 1) % 4;
                                month_days[1] = nly? 28: 29;
                        }
                        amt--;
                }
                newtime -= (btime->tm_mday - month_days[mon] + mday) * SECSPERDAY;

                /* If day unacceptable count backwards */

                if  (nvdays == 0)
                        return  newtime;

                btime = localtime(&newtime);
                dw = btime->tm_wday;

                for  (;;)  {
                        while  (nvdays & (1 << dw))  {
                                if  (--dw < 0)
                                        dw = 6;
                                newtime -= SECSPERDAY;
                        }
                        if  (!(nvdays & TC_HOLIDAYBIT)  ||  !is_holiday(newtime))
                                return  newtime;
                        if  (--dw < 0)
                                dw = 6;
                        newtime -= SECSPERDAY;
                }

        case  TC_YEARS:
                newtime += amt * 365 * SECSPERDAY;
                newtime += SECSPERDAY * (amt / 4);
                dw = btime->tm_year % 4;

                /* See if we're straddling Feb 29 at beginning.  */

                if  (dw == 0)  {
                        if  (btime->tm_mon < 1  || (btime->tm_mon == 1 && btime->tm_mday < 29))
                                newtime += SECSPERDAY;
                        break;
                }

                /* See if we're straddling at end */

                if  ((btime->tm_year + amt) % 4 == 0)  {
                        if  (btime->tm_mon > 1)
                                newtime += SECSPERDAY;
                        break;
                }

                if  ((btime->tm_year + amt) % 4 < dw)
                        newtime += SECSPERDAY;
                break;
        }

        /* Now adjust if day of week is unacceptable.  For everything
           except Month relative to end move forwards.  We must
           use the localtime routine to make sure we handle
           timezones etc correctly.  */

        if  (nvdays == 0)
                return  newtime;

        btime = localtime(&newtime);
        dw = btime->tm_wday;

        for  (;;)  {
                while  (nvdays & (1 << dw))  {
                        if  (++dw >= 7)
                                dw = 0;
                        newtime += SECSPERDAY;
                }
                if  (!(nvdays & TC_HOLIDAYBIT)  ||  !is_holiday(newtime))
                        return  newtime;
                if  (++dw >= 7)
                        dw = 0;
                newtime += SECSPERDAY;
        }
}
