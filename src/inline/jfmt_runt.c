/* jfmt_runt.c -- format runtime for job display

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

JFORMAT(fmt_deltime)
{
#ifdef  CHARSPRINTF
        if  (isreadable  &&  jp->h.bj_deltime != 0)  {
                sprintf(bigbuff, "%*u", fwidth, jp->h.bj_deltime);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (isreadable  &&  jp->h.bj_deltime != 0)
                return  (fmt_t) sprintf(bigbuff, "%*u", fwidth, jp->h.bj_deltime);
#endif
        return  0;
}

JFORMAT(fmt_runtime)
{
#ifdef  CHARSPRINTF
        if  (isreadable  &&  jp->h.bj_runtime != 0)  {
                unsigned  long  hrs, mns, secs;
                hrs = jp->h.bj_runtime / 3600L;
                mns = jp->h.bj_runtime % 3600L;
                secs = mns % 60L;
                mns /= 60L;
                if  (hrs != 0)  {
                        int     resw = fwidth - 6;
                        sprintf(bigbuff, "%*lu:%.2u:%.2u", resw < 0? 0: resw,
                                       hrs, (unsigned) mns, (unsigned) secs);
                }
                else  if  (mns != 0)  {
                        int     resw = fwidth - 3;
                        sprintf(bigbuff, "%*lu:%.2u", resw < 0? 0: resw, mns, (unsigned) secs);
                }
                else
                        sprintf(bigbuff, "%*lu", fwidth, secs);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (isreadable  &&  jp->h.bj_runtime != 0)  {
                unsigned  long  hrs, mns, secs;
                hrs = jp->h.bj_runtime / 3600L;
                mns = jp->h.bj_runtime % 3600L;
                secs = mns % 60L;
                mns /= 60L;
                if  (hrs != 0)  {
                        int     resw = fwidth - 6;
                        return  (fmt_t) sprintf(bigbuff, "%*lu:%.2u:%.2u", resw < 0? 0: resw,
                                       hrs, (unsigned) mns, (unsigned) secs);
                }
                else  if  (mns != 0)  {
                        int     resw = fwidth - 3;
                        return  (fmt_t) sprintf(bigbuff, "%*lu:%.2u", resw < 0? 0: resw, mns, (unsigned) secs);
                }
                else
                        return  (fmt_t) sprintf(bigbuff, "%*lu", fwidth, secs);
        }
#endif
        return  0;
}

JFORMAT(fmt_autoksig)
{
#ifdef  CHARSPRINTF
        if  (isreadable  &&  jp->h.bj_runtime != 0)  {
                sprintf(bigbuff, "%*u", fwidth, jp->h.bj_autoksig);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (isreadable  &&  jp->h.bj_runtime != 0)
                return  (fmt_t) sprintf(bigbuff, "%*u", fwidth, jp->h.bj_autoksig);
#endif
        return  0;
}

JFORMAT(fmt_gracetime)
{
#ifdef  CHARSPRINTF
        if  (isreadable  &&  jp->h.bj_runtime != 0  &&  jp->h.bj_runon != 0)  {
                unsigned  mns, secs;
                mns = jp->h.bj_runon / 60;
                secs = jp->h.bj_runon % 60;
                if  (mns != 0)  {
                        int     resw = fwidth - 3;
                        sprintf(bigbuff, "%*u:%.2u", resw < 0? 0: resw, mns, secs);
                }
                else
                        sprintf(bigbuff, "%*u", fwidth, secs);
                return  (fmt_t) strlen(bigbuff);
        }
#else
        if  (isreadable  &&  jp->h.bj_runtime != 0  &&  jp->h.bj_runon != 0)  {
                unsigned  mns, secs;
                mns = jp->h.bj_runon / 60;
                secs = jp->h.bj_runon % 60;
                if  (mns != 0)  {
                        int     resw = fwidth - 3;
                        return  (fmt_t) sprintf(bigbuff, "%*u:%.2u", resw < 0? 0: resw, mns, secs);
                }
                else
                        return  (fmt_t) sprintf(bigbuff, "%*u", fwidth, secs);
        }
#endif
        return  0;
}
