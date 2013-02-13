/* bq_viewj.c -- view jobs in gbch-q

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
#include <sys/types.h>
#include <curses.h>
#include <ctype.h>
#include "incl_unix.h"
#include "defaults.h"
#include "incl_net.h"
#include "network.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "btuser.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "netmsg.h"
#include "magic_ch.h"
#include "errnums.h"
#include "statenums.h"
#include "files.h"
#include "optflags.h"
#include "q_shm.h"

void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);
const char *qtitle_of(CBtjobRef);
FILE *net_feed(const int, const netid_t, const jobno_t, const int);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);

static  char    Filename[] = __FILE__;

extern  WINDOW  *escr,
                *hlpscr,
                *jscr,
                *Ew;

#define INITPAGES       20
#define INCPAGES        10

static  LONG    *pagestarts;

static  int     firstrow,
                npages,         /* Allocated amount in pagestarts */
                numpages,       /* Actual number in document */
                pagewidth,
                Colstep;

static  char    *eofmsg,
                *eopmsg;

/* Read file to find where all the pages start.  */

void  scanfile(FILE *fp)
{
        int     curline, curpage, lcnt, ch;

        if  ((pagestarts = (LONG *) malloc(INITPAGES*sizeof(LONG))) == (LONG *) 0)
                ABORT_NOMEM;

        npages = INITPAGES;
        pagewidth = 0;
        curline = 0;
        curpage = 0;
        pagestarts[0] = 0L;
        lcnt = firstrow;

        while  ((ch = getc(fp)) != EOF)  {
                switch  (ch)  {
                default:
                        curline++;
                        break;
                case  '\f':
                case  '\n':
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                        if  (++lcnt < LINES  &&  ch != '\f')
                                break;
                        lcnt = firstrow;
                        if  (++curpage >= npages)  {
                                npages += INCPAGES;
                                pagestarts = (LONG *) realloc((char *)pagestarts, npages*sizeof(LONG));
                                if  (pagestarts == (LONG *) 0)
                                        ABORT_NOMEM;
                        }
                        pagestarts[curpage] = ftell(fp);
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                        break;
                case  '\t':
                        curline = (curline + 8) & ~7;
                        break;
                }
        }
        numpages = curpage;
}

static void  centralise(int row, char *msg)
{
        move(row, (COLS - (int) strlen(msg)) / 2);
        standout();
        addstr(msg);
        standend();
}

/* Display next screenful.  */

static void  displayscr(FILE *fp, int lhcol, int pnum)
{
        int     row, col, wcol, drow, ch;

        row = firstrow;
        col = 0;
        drow = 0;
        move(row, col);
        clrtobot();

        fseek(fp, pagestarts[pnum], 0);

        for  (;;)  {
                ch = getc(fp);

                switch  (ch)  {
                case  EOF:
                        centralise(row, eofmsg);
                        return;

                case  '\n':
                        col = 0;
                        drow = 0;
                        row++;
                        break;

                case  '\f':
                        if  (col > 0)  {
                                col = 0;
                                drow = 0;
                                if  (++row >= LINES)  {
                                        ungetc('\f', fp);
                                        break;
                                }
                        }
                        centralise(row, eopmsg);
                        return;

                case  '\t':
                        col = (col + 8) & ~7;
                        break;

                case  ' ':
                        col++;
                        break;

                default:
                        wcol = col - lhcol;
                        col++;
                        if  (wcol < 0)
                                break;
                        if  (wcol >= COLS)  {
                                if  (!drow)  {
                                        mvaddch(row, COLS-1, '>');
                                        drow++;
                                }
                                break;
                        }

#ifdef  CURSES_SCROLL_BUG
                        /* Yuk!  Supress funnies with bottom rh corner.  */

                        if  (wcol == COLS-1 && row == LINES-1)
                                break;
#endif

                        if  (!isascii(ch))
                                mvaddch(row, wcol, '?');
                        else  if  (iscntrl(ch))  {
                                ch = ch + '@';
                                move(row, wcol);
                                standout();
#ifdef  HAVE_TERMINFO
                                addch((chtype) ch);
#else
                                addch(ch);
#endif
                                standend();
                        }
                        else
#ifdef  HAVE_TERMINFO
                                mvaddch(row, wcol, (chtype) ch);
#else
                                mvaddch(row, wcol, ch);
#endif
                        break;
                }
                if  (row >= LINES)
                        break;
        }
}

static void  domarg(int lhcol)
{
        int     col = 0, k, cy;

        move(firstrow-1, 0);

        do  {
                if  (COLS - col < 5)  {
                        while  (col < COLS)  {
                                addch('.');
                                col++;
                        }
                        return;
                }
                k = lhcol+col+1;
                do  {
                        addch('.');
                        col++;
                        k++;
                }  while  ((k % 5) != 0);
                printw("%d", k);
                getyx(stdscr, cy, col);
        }  while  (cy < firstrow);
}

/* Display a job on the screen.  */

void  viewfile(BtjobRef jp)
{
        FILE    *fp;
        int     kch, pnum, lhcol, row;
        char    **heading, **hv;

        /* Insert now for benefit of error messages */

        disp_arg[0] = jp->h.bj_job;
        disp_str = qtitle_of(jp);

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return;
        }

        /* If it's a remote file slurp it up into a temporary file */

        if  (jp->h.bj_hostid)  {
                FILE    *ifl;
                int     kch;

                if  (!(ifl = net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job, Job_seg.dptr->js_viewport)))  {
                        disp_arg[0] = jp->h.bj_job;
                        disp_str = qtitle_of(jp);
                        disp_str2 = look_host(jp->h.bj_hostid);
                        doerror(jscr, $E{btq viewj cannot open net file});
                        return;
                }
                else  {
                        fp = tmpfile();
                        while  ((kch = getc(ifl)) != EOF)
                                putc(kch, fp);
                        fclose(ifl);
                        rewind(fp);
                }
        }
        else  if  ((fp = fopen(mkspid(SPNAM, jp->h.bj_job), "r")) == (FILE *) 0)  {
                doerror(jscr, $E{btq cannot open file});
                return;
        }

        /* Slurp up heading and prompts inserting job stuff.  */

        heading = helphdr('I');
        count_hv(heading, &firstrow, (int *) 0);
        firstrow++;             /* To allow for ruler */
        eofmsg = gprompt($P{btq view end of file});
        eopmsg = gprompt($P{btq view end of page});
        Colstep = (COLS / (3 * 8)) * 8;

        Ew = stdscr;
        select_state($S{btq view jobs state});

        scanfile(fp);
        clear();
        standout();

        for  (row = 0, hv = heading;  *hv;  row++, hv++)
                mvwhdrstr(stdscr, row, 0, *hv);

        standend();

        lhcol = 0;
        pnum = 0;
        domarg(lhcol);
        displayscr(fp, lhcol, pnum);
#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif

        for  (;;)  {
                move(LINES-1, COLS-1);
                refresh();

                do  kch = getkey(MAG_A|MAG_P);
                while  (kch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(stdscr, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(stdscr, &escr);

                switch  (kch)  {
                default:
                        doerror(stdscr, $E{btq viewj unknown command});

                case  EOF:
                        continue;

                case  $K{key help}:
                        dochelp(stdscr, $H{btq view jobs state});
                        continue;

                case  $K{key halt}:
                        fclose(fp);
                        free((char *) pagestarts);
                        freehelp(heading);
                        free(eofmsg);
                        free(eopmsg);
#ifdef  CURSES_MEGA_BUG
                        clear();
                        refresh();
#endif
                        return;

                case  $K{key guess}:
                case  $K{key eol}:
                case  $K{key cursor down}:
                        if  (pnum >= numpages)  {
                                doerror(stdscr, $E{btq viewj off end});
                                continue;
                        }
                        pnum++;
                        displayscr(fp, lhcol, pnum);
                        continue;

                case  $K{key cursor up}:
                        if  (pnum <= 0)  {
                                doerror(stdscr, $E{btq viewj off beginning});
                                continue;
                        }
                        pnum--;
                        displayscr(fp, lhcol, pnum);
                        continue;

                case  $K{btq key view left}:
                        if  (lhcol <= 0)
                                doerror(stdscr, $E{btq viewj off lhs});
                        else  {
                                lhcol -= Colstep;
                                domarg(lhcol);
                                displayscr(fp, lhcol, pnum);
                        }
                        continue;

                case  $K{btq key view right}:
                        if  (lhcol + Colstep >= pagewidth)
                                doerror(stdscr, $E{btq viewj off rhs});
                        else  {
                                lhcol += Colstep;
                                domarg(lhcol);
                                displayscr(fp, lhcol, pnum);
                        }
                        continue;

                case  $K{key top}:
                        if  (pnum <= 0)
                                continue;
                        pnum = 0;
                        displayscr(fp, lhcol, 0);
                        continue;

                case  $K{key bottom}:
                        if  (pnum >= numpages)
                                continue;
                        pnum = numpages;
                        displayscr(fp, lhcol, pnum);
                        continue;

                case  $K{btq key view lmarg}:
                        if  (lhcol > 0)  {
                                lhcol = 0;
                                domarg(0);
                                displayscr(fp, 0, pnum);
                        }
                        continue;

                case  $K{btq key view rmarg}:
                        if  (lhcol + Colstep < pagewidth)  {
                                lhcol = (pagewidth / Colstep) * Colstep;
                                if  ((pagewidth % Colstep) == 0)  {
                                        lhcol -= Colstep;
                                        if  (lhcol < 0)
                                                lhcol = 0;
                                }
                                domarg(lhcol);
                                displayscr(fp, lhcol, pnum);
                        }
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;
                }
        }
}
