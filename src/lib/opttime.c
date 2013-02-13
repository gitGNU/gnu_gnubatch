/* opttime.c -- option handling for time arguments

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
#include <sys/types.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "btmode.h"
#include "timecon.h"
#include "btconst.h"
#include "bjparam.h"
#include "btjob.h"
#include "errnums.h"
#include "helpargs.h"
#include "helpalt.h"
#include "optflags.h"

extern  BtjobRef        JREQ;

/* Define these here */

HelpaltRef      repunit, days_abbrev, ifnposses;

EOPTION(o_notime)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_TIMES_CHANGES;
        JREQ->h.bj_times.tc_istime = 0;
        JREQ->h.bj_times.tc_nexttime = 0;
        return  OPTRESULT_OK;
}

EOPTION(o_time)
{
        time_t  now = time((time_t *) 0);
        struct  tm      *tn = localtime(&now);
        int     year, month, day, hour, min, num, num2, i;
        time_t  result, testit;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_TIMES_CHANGES;
        year = tn->tm_year;
        month = tn->tm_mon + 1;
        day = tn->tm_mday;
        hour = tn->tm_hour;
        min = tn->tm_min;

        while  (isspace(*arg))
                arg++;

        if  (!isdigit(*arg))
                goto  badtime;

        num = 0;
        do      num = num * 10 + *arg++ - '0';
        while  (isdigit(*arg));

        if  (*arg == '/')  {    /* It's a date I think */
                Timechanges |= OF_DATESET;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                num2 = 0;
                do      num2 = num2 * 10 + *arg++ - '0';
                while  (isdigit(*arg));

                if  (*arg == '/')  { /* First digits were year */
                        if  (num > 1900)
                                year = num - 1900;
                        else  if  (num > 110)
                                goto  badtime;
                        else  if  (num < 90)
                                year = num + 100;
                        else
                                year = num;
                        arg++;
                        if  (!isdigit(*arg))
                                goto  badtime;
                        month = num2;
                        day = 0;
                        do      day = day * 10 + *arg++ - '0';
                        while  (isdigit(*arg));
                }
                else  {         /* Day/month or Month/day
                                   Decide by which side of the Atlantic */
#ifdef  HAVE_TM_ZONE
                        if  (tn->tm_gmtoff <= -4 * 60 * 60)
#else
                        if  (timezone >= 4 * 60 * 60)
#endif
                        {
                                month = num;
                                day = num2;
                        }
                        else  {
                                month = num2;
                                day = num;
                        }
                        if  (month < tn->tm_mon + 1  ||
                             (month == tn->tm_mon + 1 && day < tn->tm_mday))
                                year++;
                }
                if  (*arg == '\0')
                        goto  finish;
                if  (*arg != ',')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                hour = 0;
                do      hour = hour * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (*arg != ':')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                min = 0;
                do      min = min * 10 + *arg++ - '0';
                while  (isdigit(*arg));
        }
        else  {

                /* If tomorrow advance date */

                Timechanges &= ~OF_DATESET;
                hour = num;
                if  (*arg != ':')
                        goto  badtime;
                arg++;
                if  (!isdigit(*arg))
                        goto  badtime;
                min = 0;
                do      min = min * 10 + *arg++ - '0';
                while  (isdigit(*arg));

                if  (hour < tn->tm_hour  ||  (hour == tn->tm_hour && min <= tn->tm_min))  {
                        day++;
                        month_days[1] = year % 4 == 0? 29: 28;
                        if  (day > month_days[month-1])  {
                                day = 1;
                                if  (++month > 12)  {
                                        month = 1;
                                        year++;
                                }
                        }
                }
        }

        while  (isspace(*arg))
                arg++;

        if  (*arg != '\0')  {
                disp_arg[0] = *arg;
                arg_errnum = $E{Invalid char in time};
                return  OPTRESULT_ERROR;
        }

 finish:
        if  (month > 12  || hour > 23  || min > 59)
                goto  badtime;

        month_days[1] = year % 4 == 0? 29: 28;
        month--;
        year -= 70;
        if  (day > month_days[month])
                goto  badtime;

        result = year * 365;
        if  (year > 2)
                result += (year + 1) / 4;

        for  (i = 0;  i < month;  i++)
                result += month_days[i];
        result = (result + day - 1) * 24;

        /* Build it up once as at 12 noon and work out timezone shift from that */

        testit = (result + 12) * 60 * 60;
        tn = localtime(&testit);
        result = ((result + hour + 12 - tn->tm_hour) * 60 + min) * 60;
        JREQ->h.bj_times.tc_nexttime = result;
        JREQ->h.bj_times.tc_istime = 1;
        return  OPTRESULT_ARG_OK;

 badtime:
        arg_errnum = $E{Bad time spec};
        return  OPTRESULT_ERROR;
}

EOPTION(o_norepeat)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_REPEAT_CHANGES;
        JREQ->h.bj_times.tc_repeat = TC_RETAIN;
        JREQ->h.bj_times.tc_istime = 1;
        return  OPTRESULT_OK;
}

EOPTION(o_deleteatend)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_REPEAT_CHANGES;
        JREQ->h.bj_times.tc_repeat = TC_DELETE;
        JREQ->h.bj_times.tc_istime = 1;
        return  OPTRESULT_OK;
}

EOPTION(o_repeat)
{
        ULONG   num;
        int     nn;
        Timecon ntim;
        static  ULONG   maxnums[] = { 527040, 8784, 1000, 520, 50, 50, 10 };

        if  (!arg)
                return  OPTRESULT_MISSARG;

        ntim = JREQ->h.bj_times;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_REPEAT_CHANGES;

        while  (isspace(*arg))
                arg++;

        if  (isalpha(*arg))  {
                char    cbuf[30];
                int     np = 0;
                do  {
                        if  (np < 29)
                                cbuf[np++] = *arg++;
                }  while  (isalpha(*arg));

                cbuf[np] = '\0';
                if  (*arg++ != ':')
                        goto  badrep;
                for  (nn = 0;  nn < repunit->numalt;  nn++)
                        if  (ncstrcmp(cbuf, repunit->list[nn]) == 0)  {
                                ntim.tc_repeat = repunit->alt_nums[nn] + TC_MINUTES;
                                goto  gotunit;
                        }
                goto  badrep;
        }

        if  (repunit->def_alt < 0)
                ntim.tc_repeat = TC_HOURS;
        else
                ntim.tc_repeat = repunit->alt_nums[repunit->def_alt] + TC_MINUTES;

 gotunit:
        if  (!isdigit(*arg))
                goto  badrep;
        num = 0;
        do      num = num*10 + *arg++ - '0';
        while  (isdigit(*arg));

        if  (num > maxnums[ntim.tc_repeat - TC_MINUTES])
                goto  badrep;
        ntim.tc_istime = 1;
        ntim.tc_rate = num;
        ntim.tc_mday = 0;               /* Fill it in later */
        Timechanges &= ~OF_MDSET;

        if  (*arg == ':')  {
                arg++;
                num = 0;
                do      num = num*10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (num == 0  || num > 31)
                        goto  badrep;
                ntim.tc_mday = (unsigned  char) num;
                Timechanges |= OF_MDSET;
        }
        if  (*arg)
                goto  badrep;
        JREQ->h.bj_times = ntim;
        return  OPTRESULT_ARG_OK;

 badrep:
        arg_errnum = $E{Bad repeat};
        return  OPTRESULT_ERROR;
}

EOPTION(o_avoiding)
{
        int     nn;
        unsigned        res = 0;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_AVOID_CHANGES;

        while  (isspace(*arg))
                arg++;

        if  (*arg == ',')  {
                res = JREQ->h.bj_times.tc_nvaldays;
                arg++;
        }
        else  if  (*arg == '-')  {
                if  (*++arg == '\0')
                        goto  done;
                if  (*arg != ',')
                        goto  badarg;
                arg++;
        }

        do  {
                int     np = 0;
                char    cbuf[30];

                if  (!isalpha(*arg))
                        goto  badarg;

                do  {
                        if  (np < 29)
                                cbuf[np++] = *arg++;
                }  while  (isalpha(*arg));

                cbuf[np] = '\0';
                for  (nn = 0;  nn < days_abbrev->numalt;  nn++)
                        if  (ncstrcmp(cbuf, days_abbrev->list[nn]) == 0)  {
                                res |= 1 << days_abbrev->alt_nums[nn];
                                goto  gotday;
                        }
                goto  badarg;

        gotday:
                ;
        }  while  (*arg++ == ',');

 done:
        if  ((res & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)  {
                arg_errnum = $E{Setting avoid all};
                return  OPTRESULT_ERROR;
        }

        JREQ->h.bj_times.tc_nvaldays = (USHORT) res;
        return  OPTRESULT_ARG_OK;

 badarg:
        arg_errnum = $E{Bad avoid arg};
        return  OPTRESULT_ERROR;
}

EOPTION(o_skip)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_NPOSS_CHANGES;
        JREQ->h.bj_times.tc_nposs = TC_SKIP;
        return  OPTRESULT_OK;
}

EOPTION(o_hold)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_NPOSS_CHANGES;
        JREQ->h.bj_times.tc_nposs = TC_WAIT1;
        return  OPTRESULT_OK;
}

EOPTION(o_resched)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_NPOSS_CHANGES;
        JREQ->h.bj_times.tc_nposs = TC_WAITALL;
        return  OPTRESULT_OK;
}

EOPTION(o_catchup)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Timechanges |= OF_NPOSS_CHANGES;
        JREQ->h.bj_times.tc_nposs = TC_CATCHUP;
        return  OPTRESULT_OK;
}
