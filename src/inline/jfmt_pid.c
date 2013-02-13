/* jfmt_pid.c -- format process id for job display

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

JFORMAT(fmt_pid)
{
#ifdef  CHARSPRINTF
        if  (isreadable  &&  jp->h.bj_pid != 0)  {
                sprintf(bigbuff, "%*ld", fwidth, (long) jp->h.bj_pid);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (isreadable  &&  jp->h.bj_pid != 0)
                return  (fmt_t) sprintf(bigbuff, "%*ld", fwidth, (long) jp->h.bj_pid);
#endif
        return  0;
}
