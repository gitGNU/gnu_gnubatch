/* jfmt_avoid.c -- format avoid days for job list

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

JFORMAT(fmt_avoid)
{
        fmt_t  lng = 0;

        if  (isreadable  &&  jp->h.bj_times.tc_istime != 0)  {
                int     nd, had = 0;
                for  (nd = 0;  nd < TC_NDAYS;  nd++)  {
                        if  (jp->h.bj_times.tc_nvaldays & (1 << nd))  {
                                if  (had)
                                        bigbuff[lng++] = ',';
                                had++;
                                lng += strlen(strcpy(&bigbuff[lng], disp_alt(nd, days_abbrev)));
                        }
                }
        }
        return  lng;
}
