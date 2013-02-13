/* jfmt_hols.c -- format holidays display

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

#define ISHOL(map, day) map[(day) >> 3] & (1 << (day & 7))

JFORMAT(fmt_hols)
{
        int     lng = 0;

        if  (isreadable  &&  jp->h.bj_times.tc_istime != 0  &&  (jp->h.bj_times.tc_nvaldays & TC_HOLIDAYBIT))  {
                time_t  now  =  time((time_t *) 0);
                struct  tm  *tp = localtime(&now);
                static  int     hfid = -1;
                int     ydays = tp->tm_year % 4 == 0? 366: 365, syday, yday, mon, day;
                unsigned  char  yearmap[YVECSIZE];

                if  (hfid < 0)  {
                        char  *fname = envprocess(HOLFILE);
                        hfid = open(fname, O_RDONLY);
                        free(fname);
                        if  (hfid < 0)
                                return  0;
                }

                lseek(hfid, (long) (tp->tm_year - 90) * YVECSIZE, 0);
                if  (read(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))
                        return  0;
                for  (syday = yday = tp->tm_yday;  yday < ydays;  now += 3600*24, yday++)
                        if  (ISHOL(yearmap, yday))  {
                                tp = localtime(&now);
                                mon = tp->tm_mon+1;
                                day = tp->tm_mday;
#ifdef  HAVE_TM_ZONE
                                if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
                                if  (timezone >= 4 * 60 * 60)
#endif
                                {
                                        day = mon;
                                        mon = tp->tm_mday;
                                }
                                if  (lng > 0)
                                        bigbuff[lng++] = ',';
#ifdef  CHARSPRINTF
                                sprintf(&bigbuff[lng], "%.2d/%.2d", day, mon);
                                lng += strlen(&bigbuff[lng]);
#else
                                lng += sprintf(&bigbuff[lng], "%.2d/%.2d", day, mon);
#endif
                        }

                if  (read(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))
                        return  lng;

                for  (yday = 0;  yday < syday; now += 3600*24, yday++)
                        if  (ISHOL(yearmap, yday))  {
                                tp = localtime(&now);
                                mon = tp->tm_mon+1;
                                day = tp->tm_mday;
#ifdef  HAVE_TM_ZONE
                                if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
                                        if  (timezone >= 4 * 60 * 60)
#endif
                                {
                                        day = mon;
                                        mon = tp->tm_mday;
                                }
                                if  (lng > 0)
                                        bigbuff[lng++] = ',';
#ifdef  CHARSPRINTF
                                sprintf(&bigbuff[lng], "%.2d/%.2d", day, mon);
                                lng += strlen(&bigbuff[lng]);
#else
                                lng += sprintf(&bigbuff[lng], "%.2d/%.2d", day, mon);
#endif
                        }
        }
        return  lng;
}
