/* jfmt_progress.c -- format job progress for job display

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

JFORMAT(fmt_progress)
{
#ifdef  CHARSPRINTF
        if  (jp->h.bj_progress == BJP_RUNNING  &&  jp->h.bj_runhostid != jp->h.bj_hostid)  {
                if  (jp->h.bj_runhostid == 0)
                        strcpy(bigbuff, localrun);
                else
                        sprintf(bigbuff, "%s:%s", disp_alt((int) jp->h.bj_progress, progresslist),
                                       look_host(jp->h.bj_runhostid));
        }
        else
                strcpy(bigbuff, disp_alt((int) jp->h.bj_progress, progresslist));

        return  (fmt_t) strlen(bigbuff);
#else
        if  (jp->h.bj_progress == BJP_RUNNING  &&  jp->h.bj_runhostid != jp->h.bj_hostid)  {
                if  (jp->h.bj_runhostid == 0)
                        return  (fmt_t) strlen(strcpy(bigbuff, localrun));
                return  (fmt_t) sprintf(bigbuff, "%s:%s", disp_alt((int) jp->h.bj_progress, progresslist),
                                           look_host(jp->h.bj_runhostid));
        }
        else
                return  (fmt_t) strlen(strcpy(bigbuff, disp_alt((int) jp->h.bj_progress, progresslist)));
#endif
}
