/* jfmt_repeat.c -- format repeat for job display

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

JFORMAT(fmt_repeat)
{
        if  (isreadable  &&  jp->h.bj_times.tc_istime != 0)  {
                static  char    *rdeletemsg, *rretainmsg;

                if  (!rdeletemsg)  {
                        rdeletemsg = gprompt($P{Delete at end abbrev});
                        rretainmsg = gprompt($P{Retain at end abbrev});
                }

                if  (jp->h.bj_times.tc_repeat < TC_MINUTES)
                        return  (fmt_t) strlen(strcpy(bigbuff,
                                                         jp->h.bj_times.tc_repeat == TC_DELETE?
                                                                rdeletemsg: rretainmsg));
#ifdef  CHARSPRINTF
                if  (jp->h.bj_times.tc_repeat == TC_MONTHSB  ||  jp->h.bj_times.tc_repeat == TC_MONTHSE)  {
                        int  mday = jp->h.bj_times.tc_mday;
                        if  (jp->h.bj_times.tc_repeat == TC_MONTHSE)  {
                                static  char    month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
                                struct  tm      *t = localtime(&jp->h.bj_times.tc_nexttime);
                                month_days[1] = t->tm_year % 4 == 0? 29: 28;
                                mday = month_days[t->tm_mon] - jp->h.bj_times.tc_mday;
                                if  (mday <= 0)
                                        mday = 1;
                        }
                        sprintf(bigbuff, "%s:%ld:%d",
                                       disp_alt((int) (jp->h.bj_times.tc_repeat - TC_MINUTES), repunit),
                                       (long) jp->h.bj_times.tc_rate, mday);
                }
                else
                        sprintf(bigbuff, "%s:%ld",
                                       disp_alt((int) (jp->h.bj_times.tc_repeat - TC_MINUTES), repunit),
                                       (long) jp->h.bj_times.tc_rate);
                return  (fmt_t) strlen(bigbuff);
#else
                if  (jp->h.bj_times.tc_repeat == TC_MONTHSB  ||  jp->h.bj_times.tc_repeat == TC_MONTHSE)  {
                        int  mday = jp->h.bj_times.tc_mday;
                        if  (jp->h.bj_times.tc_repeat == TC_MONTHSE)  {
                                static  char    month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
                                struct  tm      *t = localtime(&jp->h.bj_times.tc_nexttime);
                                month_days[1] = t->tm_year % 4 == 0? 29: 28;
                                mday = month_days[t->tm_mon] - jp->h.bj_times.tc_mday;
                                if  (mday <= 0)
                                        mday = 1;
                        }
                        return  (fmt_t) sprintf(bigbuff, "%s:%ld:%d",
                                                disp_alt((int) (jp->h.bj_times.tc_repeat - TC_MINUTES), repunit),
                                                (long) jp->h.bj_times.tc_rate, mday);
                }
                return  (fmt_t) sprintf(bigbuff, "%s:%ld",
                                        disp_alt((int) (jp->h.bj_times.tc_repeat - TC_MINUTES), repunit),
                                        (long) jp->h.bj_times.tc_rate);
#endif
        }
        return  0;
}
