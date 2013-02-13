/* jfmt_timefull.c -- format time in full for job display

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

JFORMAT(fmt_timefull)
{
        if  (isreadable  &&  jp->h.bj_times.tc_istime != 0)  {
                time_t  w = jp->h.bj_times.tc_nexttime;
                struct  tm  *t = localtime(&w);
                int     day = t->tm_mday, mon = t->tm_mon+1;
#ifdef  HAVE_TM_ZONE
                if  (t->tm_gmtoff <= -4 * 60 * 60)
#else
                if  (timezone >= 4 * 60 * 60)
#endif
                {
                        day = mon;
                        mon = t->tm_mday;
                }
#ifdef  CHARSPRINTF
                sprintf(bigbuff, "%.2d/%.2d/%.4d %.2d:%.2d",
                        day, mon, t->tm_year+1900, t->tm_hour, t->tm_min);
                return  (fmt_t) strlen(bigbuff);
#else
                return  (fmt_t) sprintf(bigbuff, "%.2d/%.2d/%.4d %.2d:%.2d",
                                           day, mon, t->tm_year+1900, t->tm_hour, t->tm_min);
#endif
        }
        return  0;
}
