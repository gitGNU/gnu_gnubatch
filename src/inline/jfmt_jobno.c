/* jfmt_jobno.c -- format job number display

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

JFORMAT(fmt_jobno)
{
#ifdef  BTJLIST_INLINE
#ifdef  CHARSPRINTF
        if  (jp->h.bj_hostid)
                sprintf(bigbuff, "%s:%.*ld", look_host(jp->h.bj_hostid), jno_width, (long) jp->h.bj_job);
        else
                sprintf(bigbuff, "%.*ld", jno_width, (long) jp->h.bj_job);
        return  (fmt_t) strlen(bigbuff);
#else
        if  (jp->h.bj_hostid)
                return  (fmt_t) sprintf(bigbuff, "%s:%.*ld", look_host(jp->h.bj_hostid), (int) jno_width, (long) jp->h.bj_job);
        else
                return  (fmt_t) sprintf(bigbuff, "%.*ld", (int) jno_width, (long) jp->h.bj_job);
#endif
#else
#ifdef  CHARSPRINTF
        if  (jp->h.bj_hostid)
                sprintf(bigbuff, "%s:%ld", look_host(jp->h.bj_hostid), (long) jp->h.bj_job);
        else
                sprintf(bigbuff, "%ld", (long) jp->h.bj_job);
        return  (fmt_t) strlen(bigbuff);
#else
        if  (jp->h.bj_hostid)
                return  (fmt_t) sprintf(bigbuff, "%s:%ld", look_host(jp->h.bj_hostid), (long) jp->h.bj_job);
        else
                return  (fmt_t) sprintf(bigbuff, "%ld", (long) jp->h.bj_job);
#endif
#endif
}
