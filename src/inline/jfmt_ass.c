/* jfmt_ass.c -- format assignments for job list

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

JFORMAT(fmt_ass)
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
                        vp = &Var_seg.vlist[ap->bja_varind].Vent;
                        lng += strlen(strcpy(&bigbuff[lng], VAR_NAME(vp)));
                }
        }
        return  lng;
}
