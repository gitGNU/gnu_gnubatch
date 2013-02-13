/* bq_miscscr.c -- misc screen ops for gbch-q

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
#include <sys/types.h>
#include "defaults.h"
#include "statenums.h"
#include "errnums.h"
#include "btmode.h"
#include "timecon.h"
#include "btconst.h"
#include "bjparam.h"
#include "btjob.h"
#include "sctrl.h"
#include "magic_ch.h"
#include "helpalt.h"
#include "optflags.h"

#ifdef  OS_BSDI
#define _begy   begy
#define _begx   begx
#define _maxy   maxy
#define _maxx   maxx
#endif

#ifndef getmaxyx
#define getmaxyx(win,y,x)       ((y) = (win)->_maxy, (x) = (win)->_maxx)
#endif
#ifndef getbegyx
#define getbegyx(win,y,x)       ((y) = (win)->_begy, (x) = (win)->_begx)
#endif

#define LBUFSIZE        80

#define NULLCP  (char *) 0

#define BOXWID  1

static  int     modehdlen;              /* Length up to 1st column */

static  char    *bs_mno,
                *bs_myes,
                *bs_mu,
                *bs_mg,
                *bs_mo;

struct  mode    {
        int     number, nextstate;
        char    *string;
        USHORT  flg, sflg, rflg;
}  mtab[] = {
        { $PN{Read mode name}, -1,
                  NULLCP,
                  BTM_READ, BTM_READ|BTM_SHOW, (USHORT) ~(BTM_READ|BTM_WRITE) },
        { $PN{Write mode name}, -1,
                  NULLCP,
                  BTM_WRITE, BTM_READ|BTM_WRITE|BTM_SHOW, (USHORT) ~BTM_WRITE },
        { $PN{Reveal mode name}, -1,
                  NULLCP,
                  BTM_SHOW, BTM_SHOW, (USHORT) ~(BTM_READ|BTM_WRITE|BTM_SHOW) },
        { $PN{Display mode name}, -1,
                  NULLCP,
                  BTM_RDMODE, BTM_RDMODE, (USHORT) ~(BTM_RDMODE|BTM_WRMODE) },
        { $PN{Set mode name}, -1,
                  NULLCP,
                  BTM_WRMODE, BTM_RDMODE|BTM_WRMODE, (USHORT) ~BTM_WRMODE },
        { $PN{Assume owner mode name}, -1,
                  NULLCP,
                  BTM_UTAKE, BTM_UTAKE, (USHORT) ~BTM_UTAKE },
        { $PN{Assume group mode name}, -1,
                  NULLCP,
                  BTM_GTAKE, BTM_GTAKE, (USHORT) ~BTM_GTAKE },
        { $PN{Give owner mode name}, -1,
                  NULLCP,
                  BTM_UGIVE, BTM_UGIVE, (USHORT) ~BTM_UGIVE },
        { $PN{Give group mode name}, -1,
                  NULLCP,
                  BTM_GGIVE, BTM_GGIVE, (USHORT) ~BTM_GGIVE },
        { $PN{Delete mode name}, -1,
                  NULLCP,
                  BTM_DELETE, BTM_DELETE, (USHORT) ~BTM_DELETE },
        { $PN{Kill mode name}, -1,
                  NULLCP,
                  BTM_KILL, BTM_KILL, (USHORT) ~BTM_KILL}};

#define MAXMODE (sizeof(mtab) / sizeof(struct mode))

struct  mode    *mlist[MAXMODE+1];

/* This flag tells us whether we have called the routine to turn the
   error codes into messages or not */

static  char    code_expanded = 0;
static  int     modecolw;

extern  WINDOW  *Ew,
                *hlpscr,
                *escr;

void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);
char *wgets(WINDOW *, const int, struct sctrl *, const char *);
LONG  wnum(WINDOW *, const int, struct sctrl *, const LONG);

int  r_max(int a, int b)
{
        return  a < b? b: a;
}

/* Expand mode etc codes into messages */

static int  expcodes(WINDOW *win)
{
        int     i, j, look4, modestart;

        if  (code_expanded)
                return  1;

        modestart = helpnstate($N{Modes initial row});

        for  (i = 0;  i < MAXMODE;  i++)  {
                mtab[i].string = gprompt(mtab[i].number);
                modehdlen = r_max(modehdlen, strlen(mtab[i].string));
        }

        for  (i = 0;  i < MAXMODE;  i++)
                mtab[i].nextstate = helpnstate(mtab[i].number);

        /* Get messages and prompts */

        bs_mno = gprompt($P{Btuser mode no});
        bs_myes = gprompt($P{Btuser mode yes});
        bs_mu = gprompt($P{Btuser mode user hdr});
        bs_mg = gprompt($P{Btuser mode group hdr});
        bs_mo = gprompt($P{Btuser mode other hdr});

        /* Allow for space after header.
           Get dimensions */

        modehdlen++;
        modecolw = r_max(strlen(bs_mno), strlen(bs_myes));
        modecolw = r_max(modecolw, strlen(bs_mu));
        modecolw = r_max(modecolw, strlen(bs_mg));
        modecolw = r_max(modecolw, strlen(bs_mo)) + 1;

        /* Now build list giving desired order */

        i = 0;
        look4 = modestart;
        for  (;;)  {
                for  (j = 0;  j < MAXMODE;  j++)
                        if  (mtab[j].number == look4)  {
                                mlist[i] = &mtab[j];
                                i++;
                                look4 = mtab[j].nextstate;
                                goto  dunm;
                        }
                disp_arg[9] = look4;
                doerror(win, $E{Missing state code});
                return  0;
        dunm:
                if  (look4 < 0)  {
                        if  (i != MAXMODE)  {
                                doerror(win, $E{Scrambled state code});
                                return  0;
                        }
                        break;
                }
        }

        code_expanded = 1;
        return  1;
}

int  mode_edit(WINDOW *win, int readwrite, int isjob, const char *descr, jobno_t jobno, BtmodeRef mp)
{
        char    **hv, **hp;
        int     hht, hwd, mstate, bx, by, mx, my, srow, currow, actrow, cnt;
        int     ugo_which, ch, err_no;
        int     changes = 0;
        WINDOW  *mw;
        struct  mode    *mlp;

        if  (!expcodes(win))
                return  0;

        /* Read in header to find out how big to make the window */

        disp_str = descr;
        disp_arg[0] = jobno;
        disp_arg[1] = mp->o_uid;
        disp_arg[2] = mp->o_gid;

        hv = helphdr(isjob? 'M': 'N');
        count_hv(hv, &hht, &hwd);

        if  (hwd < modecolw*3 + modehdlen)
                hwd = modecolw*3 + modehdlen;

        /* Allow for the number of modes + heading. One less mode for variables.  */

        hht += MAXMODE + 2*BOXWID;
        if  (isjob)  {
                hht++;
                mstate = $SH{btq job modes state};
        }
        else
                mstate = $SH{btq var modes state};

        hwd += 2 * BOXWID;

        getbegyx(win, by, bx);
        getmaxyx(win, my, mx);

        if  (my < hht  ||  mx < hwd)  {
                doerror(win, $E{btq modes no window space});
                return  0;
        }

        if  ((mw = newwin(hht, hwd, by + (my - hht) / 2, bx + (mx - hwd) / 2)) == (WINDOW *) 0)  {
                doerror(win, $E{btq modes cannot create mode window});
                return  0;
        }

        srow = 1;
        for  (hp = hv;  *hp;  srow++, hp++)
                mvwaddstr(mw, srow, BOXWID, *hp);

        mvwaddstr(mw, srow, modehdlen + BOXWID, bs_mu);
        mvwaddstr(mw, srow, modehdlen+BOXWID+modecolw, bs_mg);
        mvwaddstr(mw, srow, modehdlen+BOXWID+modecolw*2, bs_mo);
        srow++;

        select_state(mstate);

        currow = 0;
        actrow = 0;             /* For vars - actual screen row */
        if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                currow++;
        ugo_which = 0;

 fixmode:

        /* Put out current values of flags */

        bx = 0;
        for  (cnt = 0;  cnt < MAXMODE;  cnt++)  {
                mlp = &mtab[cnt];
                if  (!isjob  &&  !(mlp->flg & VALLMODES))
                        continue;
                mvwaddstr(mw, srow + bx, BOXWID, mlp->string);
                wmove(mw, srow + bx, BOXWID + modehdlen);
                wclrtoeol(mw);
                waddstr(mw, mp->u_flags & mlp->flg? bs_myes: bs_mno);
                mvwaddstr(mw, srow + bx, BOXWID + modehdlen + modecolw,
                                 mp->g_flags & mlp->flg? bs_myes: bs_mno);
                mvwaddstr(mw, srow + bx, BOXWID + modehdlen + modecolw*2,
                                 mp->o_flags & mlp->flg? bs_myes: bs_mno);
                bx++;
        }

#ifdef  HAVE_TERMINFO
        box(mw, 0, 0);
#else
        box(mw, '|', '-');
#endif

        for  (;;)  {
                wmove(mw, srow + actrow, BOXWID + modehdlen + modecolw * ugo_which);
                wrefresh(mw);

                do  ch = getkey(MAG_P|MAG_A);
                while  (ch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(mw, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(mw, &escr);

                switch  (ch)  {
                default:
                        err_no = $E{btq modes unknown command};
                err:
                        doerror(mw, err_no);

                case  EOF:
                        continue;

                case  $K{key help}:
                        dochelp(mw, mstate);
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key halt}:
                finish:
                        delwin(mw);
#ifdef  CURSES_MEGA_BUG
                        clear();
                        refresh();
#endif
                        return  changes? 1: -1;

                case  $K{key eol}:

                        /* Go onto next column */

                        if  (++ugo_which < 3)
                                continue;

                        ugo_which = 0;

                        if  (currow >= MAXMODE - 1)
                                goto  finish;

                case  $K{key cursor down}:
                        currow++;
                        actrow++;
                        if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                                currow++;
                        if  (currow >= MAXMODE)  {
                                actrow--;
                                currow--;
                                if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                                        currow--;
                                err_no = $E{btq modes off end};
                                goto  err;
                        }
                        continue;

                case  $K{key cursor up}:
                        if  (actrow <= 0)  {
                                err_no = $E{btq modes off beginning};
                                goto  err;
                        }
                        actrow--;
                        currow--;
                        if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                                currow--;
                        continue;

                case  $K{key top}:
                        currow = 0;
                        if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                                currow++;
                        actrow = 0;
                        ugo_which = 0;
                        continue;

                case  $K{key bottom}:
                        actrow = currow = MAXMODE - 1;
                        if  (!isjob)  {
                                if  (!(mlist[currow]->flg & VALLMODES))
                                        currow--;
                                actrow--;
                        }
                        ugo_which = 0;
                        continue;

                case  $K{btq key mode left}:
                        if  (ugo_which <= 0)  {
                                err_no = $E{btq modes off lhs};
                                goto  err;
                        }
                        ugo_which--;
                        continue;

                case  $K{btq key mode right}:
                        ugo_which++;
                        if  (ugo_which >= 3)  {
                                ugo_which--;
                                err_no = $E{btq modes off rhs};
                                goto  err;
                        }
                        continue;

                case  $K{btq key mode set}:
                strue:
                        if  (!readwrite)  {
                                err_no = $E{btq modes cannot write};
                                goto  err;
                        }
                        switch  (ugo_which)  {
                        default:
                                mp->u_flags |= mlist[currow]->sflg;
                                break;
                        case  1:
                                mp->g_flags |= mlist[currow]->sflg;
                                break;
                        case  2:
                                mp->o_flags |= mlist[currow]->sflg;
                                break;
                        }
                fixn:
                        changes++;
                        if  (++ugo_which < 3)
                                goto  fixmode;
                        ugo_which = 0;
                        currow++;
                        actrow++;
                        if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                                currow++;
                        if  (currow >= MAXMODE)  {
                                actrow = 0;
                                currow = 0;
                                if  (!isjob  &&  !(mlist[currow]->flg & VALLMODES))
                                        currow++;
                        }
                        goto  fixmode;

                case  $K{btq key mode unset}:
                sfalse:
                        if  (!readwrite)  {
                                err_no = $E{btq modes cannot write};
                                goto  err;
                        }
                        switch  (ugo_which)  {
                        default:
                                mp->u_flags &= mlist[currow]->rflg;
                                break;
                        case  1:
                                mp->g_flags &= mlist[currow]->rflg;
                                break;
                        case  2:
                                mp->o_flags &= mlist[currow]->rflg;
                                break;
                        }
                        goto  fixn;

                case  $K{btq key mode toggle}:
                        switch  (ugo_which)  {
                        default:
                                if  (mp->u_flags & mlist[currow]->flg)
                                        goto  sfalse;
                                goto  strue;
                        case  1:
                                if  (mp->g_flags & mlist[currow]->flg)
                                        goto  sfalse;
                                goto  strue;
                        case  2:
                                if  (mp->o_flags & mlist[currow]->flg)
                                        goto  sfalse;
                                goto  strue;
                        }
                }
        }
}

/* Routine for `getval' clear field to blanks */

void  con_clrfld(WINDOW *win, int row, int col)
{
        int     kk;

        wmove(win, row, col);
        for  (kk = 0;  kk < BTC_VALUE+2;  kk++)
                waddch(win, ' ');
        wmove(win, row, col);
}

void  con_refill(WINDOW *win, int row, int col, BtconRef cp)
{
        wmove(win, row, col);
        if  (cp->const_type == CON_LONG)
                wprintw(win, "%ld", (long) cp->con_un.con_long);
        else
                wprintw(win, "\"%s\"", cp->con_un.con_string);
}

/* Get a string or numeric value for create or assign */

int  getval(WINDOW *win, int row, int col, int update, BtconRef cp)
{
        int     ch, posn = 0, icol, is_num, is_string, err_no;
        unsigned  magcode;
        char    result[BTC_VALUE+1];

        Ew = win;

        wmove(win, row, col);
        wrefresh(win);
        reset_state();

        magcode = MAG_P;
        icol = col;
        is_num = 0;
        is_string = 0;

        for  (;;)  {
                do  ch = getkey(magcode);
                while  (ch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(win, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(win, &escr);

                switch  (ch)  {
                case  EOF:
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        wrefresh(win);
                        continue;

                case  $K{key help}:
                        dochelp(win, update? $H{btq get upd numeric}: $H{btq get numeric});
                        continue;

                case  '\'':
                case  '\"':
                        if  (is_num)  {
                        notdig:
                                err_no = $E{btq invalid string char in number};
                                goto  err;
                        }
                        if  (is_string)
                                goto  stuffch;

                        con_clrfld(win, row, col);
                        waddch(win, '\"');
                        icol++;
                        is_string = 1;
                        magcode = 0;
                        wrefresh(win);
                        continue;

                case '-':
                        if  (is_num)
                                goto  notdig; /* Must have had it
                                                 or missed the chance */

                case '0':case '1':case '2':case '3':case '4':
                case '5':case '6':case '7':case '8':case '9':

                        if  (is_string)
                                goto  stuffch;  /* Digit in string */

                        if  (!is_num)  {        /* Start of number */
                                con_clrfld(win, row, col);
                                is_num = 1;
                        }

#ifdef  HAVE_TERMINFO
                        waddch(win, (chtype) ch);
#else
                        waddch(win, ch);
#endif
                        result[posn++] = (char) ch;
                        icol++;
                        wrefresh(win);
                        continue;

                default:
                        if  (!isascii(ch))  {
                                err_no = posn != 0?
                                        $E{btq invalid char in string}: $E{btq invalid command expecting string};
                        err:
                                doerror(win, err_no);
                                continue;
                        }

                        if  (is_num)
                                goto  notdig;

                        if  (!is_string)  {
                                con_clrfld(win, row, col);
                                waddch(win, '\"');
                                icol++;
                                is_string = 1;
                                magcode = 0;
                        }
                stuffch:
                        if  (posn >= BTC_VALUE)  {
                                err_no = $E{wgets string too long};
                                goto  err;
                        }
#ifdef  HAVE_TERMINFO
                        waddch(win, (chtype) ch);
#else
                        waddch(win, ch);
#endif
                        icol++;
                        result[posn++] = (char) ch;
                        wrefresh(win);
                        continue;

                case  $K{key kill}:
                zapper:
                        posn = 0;
                        icol = col;
                        is_num = 0;
                        is_string = 0;
                        magcode = MAG_P;
                        wmove(win, row, col);
                        con_clrfld(win, row, col);
                        if  (update)
                                con_refill(win, row, col, cp);
                        wmove(win, row, col);
                        wrefresh(win);
                        continue;

                case  $K{key erase}:
                        if  (posn <= 1)
                                goto  zapper;
                        posn--;
                        mvwaddch(win, row, --icol, ' ');
                        wmove(win, row, icol);
                        wrefresh(win);
                        continue;

                case  $K{key eol}:
                        result[posn] = '\0';
                        if  (is_string)  {
                                strcpy(cp->con_un.con_string, result);
                                cp->const_type = CON_STRING;
                        }
                        else  {
                                /* If nothing there retain type but
                                   set 0 or null */
                                if  (posn <= 0)  {
                                        if  (cp->const_type == CON_STRING)
                                                cp->con_un.con_string[0] = '\0';
                                        else  {
                                                cp->con_un.con_long = 0L;
                                                cp->const_type = CON_LONG;
                                        }
                                }
                                else  {
                                        cp->con_un.con_long = atol(result);
                                        cp->const_type = CON_LONG;
                                }
                        }
                        return  1;

                case  $K{key abort}:
                        con_clrfld(win, row, col);
                        if  (update)
                                con_refill(win, row, col, cp);
                        wrefresh(win);
                        return  0;
                }
        }
}

int  rdalts(WINDOW *win, int row, int col, HelpaltRef altlist, int existing, int helpcode)
{
        WINDOW  *awin;
        int     by, bx, mx, defn, an, dummy, LHx, cpos, ch;
        int     hadch = 0;
        char    inbuf[LBUFSIZE];

        getbegyx(win, by, bx);
        getmaxyx(win, dummy, mx);
        if  ((awin = newwin(1, mx-bx-col-2, by+row, bx+col)) == (WINDOW *) 0)  {
                doerror(win, $E{btq alts no window space});
                return  -1;
        }

        defn = altlist->def_alt;
        ch = '[';
        for  (an = 0;  an < altlist->numalt;  an++)  {
                waddch(awin, ch);
                ch = ' ';
                waddstr(awin, altlist->list[an]);
                if  (altlist->alt_nums[an] == existing)
                        defn = an;
        }
        waddstr(awin, "]? ");
        getyx(awin, dummy, LHx);
        if  (defn >= 0)  {
                waddstr(awin, altlist->list[defn]);
                strcpy(inbuf, altlist->list[defn]);
                getyx(awin, dummy, cpos);
                cpos -= LHx;
        }
        else
                cpos = 0;

        for  (;;)  {
                wrefresh(awin);
                do  ch = getkey(MAG_P);
                while  (ch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(win, &hlpscr);
                        touchwin(awin);
                        wrefresh(awin);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)  {
                        endhe(win, &escr);
                        touchwin(awin);
                        wrefresh(awin);
                }

                switch  (ch)  {
                case  EOF:
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                default:
                        if  (!isprint(ch))
                                continue;
                        if  (hadch <= 0)  {
                                int     countc = 0, wn = -1;
                                if  (isupper(ch))
                                        ch = tolower(ch);
                                for  (an = 0;  an < altlist->numalt;  an++)  {
                                        int     fc = altlist->list[an][0];
                                        if  (isupper(fc))
                                                fc = tolower(fc);
                                        if  (fc == ch)  {
                                                wn = altlist->alt_nums[an];
                                                countc++;
                                        }
                                }
                                if  (countc == 1)  {
                                        delwin(awin);
#ifdef  CURSES_MEGA_BUG
                                        clear();
                                        refresh();
#endif
                                        return  wn;
                                }
                        }
                        hadch++;
                        if  (cpos < LBUFSIZE)  {
                                inbuf[cpos++] = (char) ch;
#ifdef  HAVE_TERMINFO
                                waddch(awin, (chtype) ch);
#else
                                waddch(awin, ch);
#endif
                        }
                        continue;

                case  $K{key erase}:
                        if  (cpos <= 0)
                                continue;
                        cpos--;
                        wmove(awin, 0, cpos + LHx);
                        waddch(awin, ' ');
                        wmove(awin, 0, cpos + LHx);
                        continue;

                case  $K{key next field}:
                        if  (++defn >= altlist->numalt)
                                defn = 0;
                cont_field:
                        strcpy(inbuf, altlist->list[defn]);
                        wmove(awin, 0, LHx);
                        wclrtoeol(awin);
                        waddstr(awin, inbuf);
                        getyx(awin, dummy, cpos);
                        cpos -= LHx;
                        continue;

                case  $K{key previous field}:
                        if  (--defn < 0)
                                defn = altlist->numalt - 1;
                        goto  cont_field;

                case  $K{key eol}:
                        if  (cpos <= 0)  {
                                delwin(awin);
#ifdef  CURSES_MEGA_BUG
                                clear();
                                refresh();
#endif
                                return  -1;
                        }
                        inbuf[cpos] = '\0';
                        for  (an = 0;  an < altlist->numalt;  an++)
                                if  (strcmp(altlist->list[an], inbuf) == 0)  {
                                        delwin(awin);
#ifdef  CURSES_MEGA_BUG
                                        clear();
                                        refresh();
#endif
                                        return  altlist->alt_nums[an];
                                }

                        doerror(awin, $E{btq alts unknown alternative});
                        continue;

                case  $K{key help}:
                        dochelp(win, helpcode);
                        touchwin(awin);
                        touchwin(hlpscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(awin);
                        wnoutrefresh(hlpscr);
                        doupdate();
#else
                        wrefresh(awin);
                        wrefresh(hlpscr);
#endif
                        continue;

                case  $K{key abort}:
                        delwin(awin);
#ifdef  CURSES_MEGA_BUG
                        clear();
                        refresh();
#endif
                        return  -1;
                }
        }
}

/* Variants of wgets and wnum for when we are possibly not displaying
   the relevant item.  */

char  *chk_wgets(WINDOW *w, const int row, struct sctrl *sc, const char *existing, const int promptcode)
{
        int     begy, y, x;
        char    *prompt, *ret;
        WINDOW  *awin;

        /* If we are displaying it in the job/variable list, just
           carry on as usual.  */

        if  (sc->col >= 0)
                return  wgets(w, row, sc, existing);

        getbegyx(w, begy, x);
        getyx(w, y, x);

        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  (char *) 0;

        /* Put out the prompt code and the existing string in a 1-line
           window for the purpose, and invoke wgets on that.  */

        prompt = gprompt(promptcode);
        waddstr(awin, prompt);
        free(prompt);
        getyx(awin, y, x);
        sc->col = (SHORT) x;                    /* Patch to after prompt */
        waddstr(awin, (char *) existing);
        ret = wgets(awin, 0, sc, existing);
        sc->col = -1;                           /* Patch back */

        delwin(awin);

        touchwin(w);
        wrefresh(w);
        return  ret;
}

LONG  chk_wnum(WINDOW *w, const int row, struct sctrl *sc, const LONG existing, const int promptcode)
{
        int     begy, y, x;
        char    *prompt;
        LONG    ret;
        WINDOW  *awin;

        if  (sc->col >= 0)
                return  wnum(w, row, sc, existing);

        getbegyx(w, begy, x);
        getyx(w, y, x);

        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  -1L;

        prompt = gprompt(promptcode);
        waddstr(awin, prompt);
        free(prompt);
        getyx(awin, y, x);
        sc->col = (SHORT) x;
        wprintw(awin, "%*d", (int) sc->size, existing);
        ret = wnum(awin, 0, sc, existing);
        sc->col = -1;
        delwin(awin);
        touchwin(w);
        wrefresh(w);
        return  ret;
}
