/* wgets.c -- curses routine to get a string value

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
#include <ctype.h>
#include "incl_unix.h"
#include "sctrl.h"
#include "magic_ch.h"
#include "errnums.h"
#include "optflags.h"

void  dohelp(WINDOW *, struct sctrl *, char *);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);

extern  WINDOW  *escr,
                *hlpscr,
                *Ew;

#define MAXSTR  255

static  char    result[MAXSTR+1];

void    ws_fill( WINDOW *wp, const int row, const struct sctrl *scp, const char *value)
{
        int     lng = scp->size;
        if  (lng + (int) scp->col >= COLS)
                lng = COLS;
        mvwprintw(wp, row, scp->col, "%-*s", lng, value);
}

char    *wgets(WINDOW *wp, const int row, struct sctrl *scp, const char *exist)
{
        int     posn = 0, optline = 0, rowinc, colinc, hadch = 0, ch, err_no;
        char    **optvec = (char **) 0;

        Ew = wp;
        disp_str = scp->msg;

        wmove(wp, row, (int) scp->col);
        wrefresh(wp);

        for  (;;)  {
                do  ch = getkey(scp->magic_p);
                while  (ch == EOF  &&  (hlpscr || escr));
                if  (hlpscr)  {
                        endhe(wp, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)  {
                        endhe(wp, &escr);
                        disp_str = scp->msg;
                }

                switch  (ch)  {
                case  EOF:
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        wrefresh(wp);
                        continue;

                case  $K{key help}:
                        result[posn] = '\0';
                        dohelp(wp, scp, result);
                        continue;

                case  $K{key guess}:
                        result[posn] = '\0';
                        if  (!optvec)  {
                                if  (scp->helpfn == (char **(*)()) 0)
                                        goto  unknc;
                                optvec = (*scp->helpfn)(result, 0);
                                if  (!optvec)  {
                                nonef:
                                        err_no = $E{wgets no defaults};
                                        disp_str = result;
                                        goto  err;
                                }
                                if  (!optvec[0])  {
                                        free((char *) optvec);
                                        optvec = (char **) 0;
                                        goto  nonef;
                                }
                                optline = 0;
                        }
                        else
                                optline++;

                        if  (optvec[optline] == (char *) 0)
                                optline = 0;

                        /* Remember what we had before....  */

                        strcpy(result, optvec[optline]);
                        posn = strlen(result);
                        hadch++;
                        ws_fill(wp, row, scp, result);
                        if  (posn > (int) scp->size)  { /* Obscurity for field codes */
                                colinc = scp->col + scp->size;
                                wclrtoeol(wp);
                        }
                        else
                                colinc = scp->col + posn;
                        rowinc = colinc / COLS;
                        colinc %= COLS;
                        wmove(wp, row+rowinc, colinc);
                        wrefresh(wp);
                        continue;

                case  $K{key cursor up}:
                case  $K{key cursor down}:
                case  $K{key halt}:
                        if  (scp->magic_p & MAG_CRS)  {
                                ws_fill(wp, row, scp, exist);
                                scp->retv = (SHORT) ch;
                                return  (char *) 0;
                        }

                default:
                        if  (!isascii(ch))
                                goto  unknc;

                        if  (isprint(ch)  &&  (scp->magic_p & MAG_OK))
                                goto  stuffch;

                        /* When we merge libraries remember to add '.'
                           and '-' as option for spq */

                        if  (isalpha(ch) || ch == '_' || ch == ':')
                                goto  stuffch;
                        if  (isdigit(ch) && (!(scp->magic_p & MAG_NAME) || posn > 0))
                                goto  stuffch;

                        if  (scp->magic_p & MAG_R)  {
                                ws_fill(wp, row, scp, exist);
                                scp->retv = (SHORT) ch;
                                return  (char *) 0;
                        }
                unknc:
                        err_no = posn == 0? $E{wgets unknown command}: $E{wgets invalid char};
                err:
                        doerror(wp, err_no);
                        continue;

                stuffch:
                        hadch++;
                        if  (posn >= (int) scp->size || posn >= MAXSTR)  {
                                err_no = $E{wgets string too long};
                                goto  err;
                        }

                        if  (posn <= 0)  {      /*  Clear it  */
                                if  ((int) (scp->size + scp->col) >= COLS)  {
                                        wmove(wp, row, (int) scp->col);
                                        wclrtoeol(wp);
                                }
                                else  {
                                        ws_fill(wp, row, scp, "");
                                        wmove(wp, row, (int) scp->col);
                                }
                        }
                        result[posn++] = (char) ch;
#ifdef  HAVE_TERMINFO
                        waddch(wp, (chtype) ch);
#else
                        waddch(wp, ch);
#endif
                finch:
                        wrefresh(wp);
                        if  (optvec)  {
                                freehelp(optvec);
                                optvec = (char **) 0;
                        }
                        continue;

                case  $K{key kill}:
                        posn = 0;
                        ws_fill(wp, row, scp, exist);
                        wmove(wp, row, (int) scp->col);
                        goto  finch;

                case  $K{key erase}:
                        if  (posn <= 0)
                                continue;
                        colinc = scp->col + --posn;
                        rowinc = colinc / COLS;
                        colinc %= COLS;
                        mvwaddch(wp, row+rowinc, colinc, ' ');
                        wmove(wp, row+rowinc, colinc);
                        goto  finch;

                case  $K{key eol}:
                        if  (optvec)
                                freehelp(optvec);
                        scp->retv = $K{key eol};
                        if  (!hadch  &&  scp->magic_p & MAG_NL)
                                return  (char *) 0;
                        result[posn] = '\0';
                        return  result;

                case  $K{key abort}:
                        ws_fill(wp, row, scp, exist);
                        wrefresh(wp);
                        if  (optvec)
                                freehelp(optvec);
                        scp->retv = 0;
                        return  (char *) 0;
                }
        }
}
