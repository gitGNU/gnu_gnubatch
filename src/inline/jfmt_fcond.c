/* jfmt_fcond.c -- format full condition display

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

JFORMAT(fmt_condfull)
{
        fmt_t  lng = 0;
#ifdef  CHARSPRINTF
        int     cnt;
#endif
        if  (isreadable)  {
                int     uc;
                const  Btvar    *vp;
                const  Jcond    *cp;
                for  (uc = 0;  uc < MAXCVARS;  uc++)  {
                        cp = &jp->h.bj_conds[uc];
                        if  (cp->bjc_compar == C_UNUSED)
                                break;
                        if  (uc != 0)
                                bigbuff[lng++] = ',';
                        vp = &Var_seg.vlist[cp->bjc_varind].Vent;
#ifdef  CHARSPRINTF
                        sprintf(&bigbuff[lng], "%s%s", VAR_NAME(vp), condname[cp->bjc_compar-1]);
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
                        if  (cp->bjc_value.const_type == CON_STRING)
                                sprintf(&bigbuff[lng],
                                               strchr(cp->bjc_value.con_un.con_string, ' ') ||
                                               isdigit(cp->bjc_value.con_un.con_string[0])?
                                               "\"%s\"": "%s", cp->bjc_value.con_un.con_string);
                        else
                                sprintf(&bigbuff[lng], "%ld", (long) cp->bjc_value.con_un.con_long);
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
#else
                        lng += sprintf(&bigbuff[lng], "%s%s", VAR_NAME(vp), condname[cp->bjc_compar-1]);
                        if  (cp->bjc_value.const_type == CON_STRING)
                                lng += sprintf(&bigbuff[lng],
                                               strchr(cp->bjc_value.con_un.con_string, ' ') ||
                                               isdigit(cp->bjc_value.con_un.con_string[0])?
                                               "\"%s\"": "%s", cp->bjc_value.con_un.con_string);
                        else
                                lng += sprintf(&bigbuff[lng], "%ld", (long) cp->bjc_value.con_un.con_long);
#endif
                }
        }

        return  lng;
}
