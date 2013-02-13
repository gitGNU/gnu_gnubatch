/* jfmt_env.c -- format environment display

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

JFORMAT(fmt_env)
{
        fmt_t  lng = 0;
#ifdef  CHARSPRINTF
        int     cnt;
#endif
        if  (isreadable)  {
                unsigned        ec;
                for  (ec = 0;  ec < jp->h.bj_nenv;  ec++)  {
                        char    *name, *value;

                        ENV_OF(jp, ec, name, value);
                        if  (ec != 0)
                                bigbuff[lng++] = ',';
#ifdef  CHARSPRINTF
                        sprintf(&bigbuff[lng], strchr(value, ' ')? "%s=\"%s\"": "%s=%s", name, value);
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
#else
                        lng += sprintf(&bigbuff[lng], strchr(value, ' ')? "%s=\"%s\"": "%s=%s", name, value);
#endif
                }
        }
        return  lng;
}
