/* jfmt_exits.c -- format exit codes display

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

JFORMAT(fmt_exits)
{
#ifdef  CHARSPRINTF
        if  (isreadable)  {
                sprintf(bigbuff, "N(%d,%d),E(%d,%d)",
                               jp->h.bj_exits.nlower, jp->h.bj_exits.nupper,
                               jp->h.bj_exits.elower, jp->h.bj_exits.eupper);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (isreadable)
                return  (fmt_t) sprintf(bigbuff, "N(%d,%d),E(%d,%d)",
                                        jp->h.bj_exits.nlower, jp->h.bj_exits.nupper,
                                        jp->h.bj_exits.elower, jp->h.bj_exits.eupper);
#endif
        return  0;
}
