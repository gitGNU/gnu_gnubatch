/* jfmt_assfull.c -- format assignments in full for job list

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

JFORMAT(fmt_assfull)
{
        fmt_t  lng = 0;
#ifdef  CHARSPRINTF
        int     cnt;
#endif
        if  (isreadable)  {
                int     uc;
                const   Btvar   *vp;
                const   Jass    *ap;

                for  (uc = 0;  uc < MAXSEVARS;  uc++)  {
                        ap = &jp->h.bj_asses[uc];
                        if  (ap->bja_op == BJA_NONE)
                                break;
                        if  (uc != 0)
                                bigbuff[lng++] = ',';
                        if  (ap->bja_op < BJA_SEXIT)  {
                                if  (ap->bja_flags & BJA_START)
                                        bigbuff[lng++] = 'S';
                                if  (ap->bja_flags & BJA_REVERSE)
                                        bigbuff[lng++] = 'R';
                                if  (ap->bja_flags & BJA_OK)
                                        bigbuff[lng++] = 'N';
                                if  (ap->bja_flags & BJA_ERROR)
                                        bigbuff[lng++] = 'E';
                                if  (ap->bja_flags & BJA_ABORT)
                                        bigbuff[lng++] = 'A';
                                if  (ap->bja_flags & BJA_CANCEL)
                                        bigbuff[lng++] = 'C';
                                bigbuff[lng++] = ':';
                        };
                        vp = &Var_seg.vlist[ap->bja_varind].Vent;
#ifdef  CHARSPRINTF
                        sprintf(&bigbuff[lng], "%s%s", VAR_NAME(vp), assname[ap->bja_op-1]);
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
                        if  (ap->bja_op >= BJA_SEXIT)
                                strcpy(&bigbuff[lng], ap->bja_op == BJA_SEXIT? exitcodename: signalname);
                        else  if  (ap->bja_con.const_type == CON_STRING)
                                sprintf(&bigbuff[lng],
                                               strchr(ap->bja_con.con_un.con_string, ' ') ||
                                               isdigit(ap->bja_con.con_un.con_string[0])?
                                               "\"%s\"": "%s", ap->bja_con.con_un.con_string);
                        else
                                sprintf(&bigbuff[lng], "%ld", (long) ap->bja_con.con_un.con_long);
                        cnt = strlen(&bigbuff[lng]);
                        lng += cnt;
#else
                        lng += sprintf(&bigbuff[lng], "%s%s", VAR_NAME(vp), assname[ap->bja_op-1]);
                        if  (ap->bja_op >= BJA_SEXIT)
                                lng += strlen(strcpy(&bigbuff[lng], ap->bja_op == BJA_SEXIT? exitcodename: signalname));
                        else  if  (ap->bja_con.const_type == CON_STRING)
                                lng += sprintf(&bigbuff[lng],
                                               strchr(ap->bja_con.con_un.con_string, ' ') ||
                                               isdigit(ap->bja_con.con_un.con_string[0])?
                                               "\"%s\"": "%s", ap->bja_con.con_un.con_string);
                        else
                                lng += sprintf(&bigbuff[lng], "%ld", (long) ap->bja_con.con_un.con_long);
#endif
                }
        }
        return  lng;
}
