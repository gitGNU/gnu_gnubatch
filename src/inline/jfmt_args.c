/* jfmt_args.c -- format args for job list

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

JFORMAT(fmt_args)
{
        fmt_t   lng = 0;
#ifdef  CHARSPRINTF
        int     cnt;
#endif
        if  (isreadable)  {
                unsigned        ac;
                for  (ac = 0;  ac < jp->h.bj_nargs;  ac++)  {
                        const   char    *arg = ARG_OF(jp, ac);
                        if  (ac != 0)
                                bigbuff[lng++] = ',';
#ifdef  CHARSPRINTF
                        if  (strchr(arg, ' '))
                                sprintf(&bigbuff[lng], "\"%s\"", arg);
                        else
                                strcpy(&bigbuff[lng], arg);
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
#else
                        lng += sprintf(&bigbuff[lng], strchr(arg, ' ')? "\"%s\"": "%s", arg);
#endif
                }
        }

        return  lng;
}
