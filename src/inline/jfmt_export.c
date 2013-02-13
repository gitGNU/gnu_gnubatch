/* jfmt_export.c -- format export display

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

JFORMAT(fmt_export)
{
        static  char    *exportname, *remrunname;

        if  (!exportname)  {
                exportname = gprompt($P{Exportable flag});
                remrunname = gprompt($P{Remote runnable flag});
        }
        if  (jp->h.bj_jflags & BJ_REMRUNNABLE)
                return  (fmt_t) strlen(strcpy(bigbuff, remrunname));
        else    if  (jp->h.bj_jflags & BJ_EXPORT)
                return  (fmt_t) strlen(strcpy(bigbuff, exportname));
        else  {
                bigbuff[0] = '\0';
                return  0;
        }
}
