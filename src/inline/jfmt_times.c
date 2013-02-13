/* jfmt_times.c -- format various times for job display

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

static fmt_t  ptimes(const time_t w)
{
        if  (w != 0)  {
                time_t  now = time((time_t *) 0);
                struct  tm  *t = localtime(&w);
                int     t1 = t->tm_hour, t2 = t->tm_min;
                int     sep = ':';

                now = (now / (24L * 60L * 60L)) * 24L * 60L * 60L;

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

JFORMAT(fmt_otime)
{
        if  (!isreadable)
                return  0;
        return  ptimes(jp->h.bj_time);
}
JFORMAT(fmt_stime)
{
        if  (!isreadable)
                return  0;
        return  ptimes(jp->h.bj_stime);
}
JFORMAT(fmt_etime)
{
        if  (!isreadable)
                return  0;
        return  ptimes(jp->h.bj_etime);
}
JFORMAT(fmt_itime)
{

        switch  (jp->h.bj_progress)  {
        default:
                return  0;

        case  BJP_FINISHED:
        case  BJP_ERROR:
        case  BJP_ABORTED:
                return  fmt_etime(jp, isreadable, fwidth);
        case  BJP_STARTUP1:
        case  BJP_STARTUP2:
        case  BJP_RUNNING:
                return  fmt_stime(jp, isreadable, fwidth);
        case  BJP_DONE:
                if  (jp->h.bj_etime)
                        return  fmt_etime(jp, isreadable, fwidth);
        case  BJP_CANCELLED:
                return  fmt_time(jp, isreadable, fwidth);
        case  BJP_NONE:
                if  (!jp->h.bj_times.tc_istime  &&  jp->h.bj_times.tc_nexttime < time((time_t *) 0))
                        return  fmt_etime(jp, isreadable, fwidth);
                return  fmt_time(jp, isreadable, fwidth);
        }
}
