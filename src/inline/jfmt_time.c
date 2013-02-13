/* jfmt_time.c -- format time for job display

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

JFORMAT(fmt_time)
{
        if  (isreadable  &&  jp->h.bj_times.tc_istime != 0)  {
                time_t  w = jp->h.bj_times.tc_nexttime;
                time_t  now = time((time_t *) 0);
                struct  tm  *t = localtime(&w);
                int     t1 = t->tm_hour, t2 = t->tm_min;
                int     sep = ':';

                if  (w < now || w - now > 24 * 60 * 60)  {
                        t1 = t->tm_mday;
                        t2 = t->tm_mon+1;
#ifdef  HAVE_TM_ZONE
                        if  (t->tm_gmtoff <= -4 * 60 * 60)
#else
                        if  (timezone >= 4 * 60 * 60)
#endif
                        {
                                t1 = t2;
                                t2 = t->tm_mday;
                        }
                        sep = '/';
                }
                sprintf(bigbuff, "%.2d%c%.2d", t1, sep, t2);
                return  5;
        }
        return  0;
}
