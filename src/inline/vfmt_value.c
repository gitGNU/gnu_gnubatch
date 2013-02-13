/* vfmt_value.c -- display variable value for variable display

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

VFORMAT(fmt_value)
{
        CBtconRef       cp = &vp->var_value;

        if  (isreadable)  {
#ifdef  CHARSPRINTF
                if  (cp->const_type == CON_STRING)
                        sprintf(bigbuff,
                                       strchr(cp->con_un.con_string, ' ') ||
                                       isdigit(cp->con_un.con_string[0])? "\"%s\"": "%s",
                                       cp->con_un.con_string);
                else
                        sprintf(bigbuff, "%ld", (long) cp->con_un.con_long);
                return  (fmt_t) strlen(bigbuff);
#else
                if  (cp->const_type == CON_STRING)
                        return  (fmt_t) sprintf(bigbuff,
                                                strchr(cp->con_un.con_string, ' ') ||
                                                isdigit(cp->con_un.con_string[0])? "\"%s\"": "%s",
                                                cp->con_un.con_string);
                else
                        return  (fmt_t) sprintf(bigbuff, "%ld", (long) cp->con_un.con_long);
#endif
        }
        return  0;
}
