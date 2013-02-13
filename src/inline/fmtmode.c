/* fmtmode.c -- format mode display

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

static fmt_t  fmtmode(unsigned lng, const char *prefix, const unsigned md)
{
#ifdef  CHARSPRINTF
        int     cnt;
        sprintf(&bigbuff[lng], "%s:", prefix);
        cnt = strlen(&bigbuff[lng]);
        lng += cnt;
#else
        lng += sprintf(&bigbuff[lng], "%s:", prefix);
#endif
        if  (md & BTM_READ)
                bigbuff[lng++] = 'R';
        if  (md & BTM_WRITE)
                bigbuff[lng++] = 'W';
        if  (md & BTM_SHOW)
                bigbuff[lng++] = 'S';
        if  (md & BTM_RDMODE)
                bigbuff[lng++] = 'M';
        if  (md & BTM_WRMODE)
                bigbuff[lng++] = 'P';
        if  (md & BTM_UGIVE)
                bigbuff[lng++] = 'U';
        if  (md & BTM_UTAKE)
                bigbuff[lng++] = 'V';
        if  (md & BTM_GGIVE)
                bigbuff[lng++] = 'G';
        if  (md & BTM_GTAKE)
                bigbuff[lng++] = 'H';
        if  (md & BTM_DELETE)
                bigbuff[lng++] = 'D';
        if  (md & BTM_KILL)
                bigbuff[lng++] = 'K';
        bigbuff[lng] = '\0';
        return  lng;
}
