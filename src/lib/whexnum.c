/* whexnum.c -- curses routine to get hex and octal numbers

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

#include "config.h"
#include <curses.h>
#include "sctrl.h"
#include "magic_ch.h"
#include "errnums.h"
#include "optflags.h"

void  doerror(WINDOW *, int);
void  dohelp(WINDOW *, struct sctrl *, char *);
void  endhe(WINDOW *, WINDOW **);

extern  WINDOW  *escr,
                *hlpscr,
                *Ew;

void  wh_fill(WINDOW *wp, const int row, const struct sctrl *scp, const ULONG value)
{
        char  *fmt = (scp->magic_p & MAG_OCTAL) ? "%.*lo": "%.*lX";
        mvwprintw(wp, row, scp->col, fmt, (int) scp->size, value);
}

ULONG  whexnum(WINDOW *wp, const int row, struct sctrl *scp, const unsigned exist)
{
        ULONG  result = 0;
        int     ch, chcnt = 0, err_no, err_off = 0, isfirst = 1, base = 4;

        if  (scp->magic_p & MAG_OCTAL)  {
                base--;
                err_off += $S{Wnum octal offset};
        }

        /* Initialise help/error message values and strings.  */

        Ew = wp;
        disp_arg[0] = scp->min;
        disp_arg[1] = scp->vmax;
        disp_arg[2] = exist;
        scp->retv = 0;
        if  ((disp_str = scp->msg) == (char *) 0)
                err_off += $S{Wnum no field name};

        wmove(wp, row, (int) (scp->col + scp->size) - 1);
        wrefresh(wp);

        for  (;;)  {
                do  ch = getkey(scp->magic_p);
                while  (ch == EOF  &&  (hlpscr || escr));
                if  (hlpscr)  {
                        endhe(wp, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(wp, &escr);

                switch  (ch)  {
                case  EOF:
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        wrefresh(wp);
                        continue;

                case  $K{key help}:
                        dohelp(wp, scp, (char *) 0);
                        continue;

                default:
                badch:
                        if  (scp->magic_p & MAG_R)  {
                                wh_fill(wp, row, scp, (ULONG) exist);
                                scp->retv = (SHORT) ch;
                                return  exist;
                        }
                        err_no = isfirst? $E{whexnum unknown command}: $E{whexnum invalid char};
                err:
                        doerror(wp, err_no+err_off);
                        continue;

                case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                        if  (base < 4)
                                goto  badch;
                        isfirst = 0;
                        chcnt++;
                        result = (result << 4) + ch - 'a' + 10;
                        goto  accum;

                case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                        if  (base < 4)
                                goto  badch;
                        isfirst = 0;
                        chcnt++;
                        result = (result << 4) + ch - 'A' + 10;
                        goto  accum;

                case '8':case '9':
                        if  (base < 4)
                                goto  badch;
                case '0':case '1':case '2':case '3':case '4':
                case '5':case '6':case '7':
                        isfirst = 0;
                        chcnt++;
                        result = (result << base) + ch - '0';
        accum:
                        if  (result > scp->vmax)  {
                                err_no = $E{whexnum num too large};
                                goto  ereset;
                        }
        rcnt:
                        wh_fill(wp, row, scp, (ULONG) result);
        cnt:
                        wmove(wp, row, (int) (scp->col + scp->size) - 1);
                        wrefresh(wp);
                        continue;

                case  $K{key eol}:
                        if  (chcnt == 0)
                                return  exist;
                        if  (result >= scp->min)
                                return  result;
                        err_no = $E{whexnum num too small};
                ereset:
                        chcnt = 0;
                        result = 0;
                        wh_fill(wp, row, scp, (ULONG) exist);
                        wmove(wp, row, (int)(scp->col + scp->size) - 1);
                        goto  err;

                case  $K{key kill}:
                        isfirst = 1;
                        chcnt = 0;
                        result = 0;
                        wh_fill(wp, row, scp, (ULONG) exist);
                        goto  cnt;

                case  $K{key erase}:
                        if  (result == 0)
                                continue;
                        result >>= base;
                        if  (result == 0)
                                isfirst = 1;
                        goto  rcnt;

                case  $K{key abort}:
                        wh_fill(wp, row, scp, (ULONG) exist);
                        wrefresh(wp);
                        return  exist;
                }
        }
}
