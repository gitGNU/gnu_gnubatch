/* wtimes.c -- Curses handling of times and dates

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
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "timecon.h"
#include "btmode.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "jvuprocs.h"
#include "helpalt.h"
#include "sctrl.h"
#include "statenums.h"
#include "errnums.h"
#include "magic_ch.h"
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

#ifndef HAVE_TERMINFO
#define chtype  int
#define wattron(wp, attr)       wstandout(wp)
#define wattroff(wp, attr)      wstandend(wp)
#endif

#define DEFAULT_RATE    10

#define SECSPERDAY      (24 * 60 * 60L)

struct  colmarks        {
        int     tim_col;        /* First digit of time */
        int     apr_col;        /* Approve */
        int     wd_col;         /* First char of weekday */
        int     md_col;         /* First digit of month day */
        int     mon_col;        /* First char of month name */
        int     year_col;       /* First digit of year */
        int     ro_col;         /* Column to start repeat options */
        int     int_row;        /* Row of time interval (or option) */
        int     int_col;        /* First digit of time interval */
        int     mday_col;       /* First digit of day number */
        int     dat_col;        /* Col of resulting date -1 if n/a */
        int     adays_col;      /* Col of first day to avoid */
        int     np_col;         /* Column for not poss stuff */
        int     np_sel_row;
};

struct  rowmarks        {
        int     time_row;       /* Where job time goes */
        int     rep_row;        /* Where repetition goes */
        int     avd_row;        /* Avoiding row */
        int     np_row;         /* If not possible.... */
};

extern  WINDOW  *hlpscr, *escr, *Ew;

static  int     changes;

extern  Shipc   Oreq;

/* These prompts are read in the first time this routine is called.  */

static  char    *Stprmpt,       /* `Set time for' prompt */
                *Rpprmpt,       /* `Repeat options' prompt */
                *Dayprmpt,      /* `Day' prompt for months b/e */
                *Avprmpt,       /* `Avoid days' prompt */
                *Ndprmpt,       /* `Not today' prompt */
                *Npprmpt,       /* `Not possible' prompt */
                *Apr_yes,       /* Time set `yes' */
                *Apr_no;        /* Time set `no' */

static  int     wxoffset,
                weekdlen,
                defaultrate = DEFAULT_RATE;

static  unsigned  defavoid;     /* Default days to avoid */

static  HelpaltRef      Roalts,
                        Units,
                        Npalts,
                        Monthnames;

void  endhe(WINDOW *, WINDOW **);
void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  qdojerror(unsigned, BtjobRef);
void  wjmsg(const unsigned, const ULONG);
LONG  wnum(WINDOW *, const int, struct sctrl *, const LONG);

static int  initprompts(WINDOW *wp)
{
        int     ii;

        Stprmpt = gprompt($P{Set time for});
        Rpprmpt = gprompt($P{btq repeat options});
        Roalts = galts(wp, $Q{btq repeat options}, 3);
        Units = galts(wp, $Q{Repeat unit full}, TC_YEARS-TC_MINUTES+1);
        weekdlen = altlen(days_abbrev) + 1;
        Dayprmpt = gprompt($P{Repeat month day});
        Avprmpt = gprompt($P{Wtime avoiding help});
        Ndprmpt = gprompt($P{Wtime not avoid});
        Npprmpt = gprompt($P{Wtime if not poss});
        Npalts = galts(wp, $Q{Wtime if not poss}, 4);
        Monthnames = galts(wp, $Q{Months}, 12);
        Apr_yes = gprompt($P{Do you want to set time});
        Apr_no = gprompt($P{Dont set time});
        if  (!Roalts || !Units || !Npalts || !Monthnames)
                return  0;
        if  ((defaultrate = helpnstate($N{Default number of units})) <= 0)
                defaultrate = DEFAULT_RATE;
        for  (ii = 0;  ii < TC_NDAYS;  ii++)
                if  (helpnstate($N{Base for days to avoid}+ii) > 0)
                        defavoid |= 1 << ii;
        return  1;
}

/* From window size, work out where to put bits & pieces */

static int  initdims(WINDOW *wp, struct rowmarks *rowlist)
{
        int     height, width, datewidth, reqdwidth, spareheight, ll;

        getmaxyx(wp, height, width);

        /* Find required width of time/date field */

        datewidth = 4 + 1 + 1 + weekdlen + 2 + 1 + altlen(Monthnames) + 1 + 5;

        reqdwidth = strlen(Rpprmpt) + altlen(Roalts) + 4 + altlen(Units) + strlen(Dayprmpt) + 3 + datewidth;
        if  ((ll = weekdlen * TC_NDAYS + strlen(Avprmpt)) > reqdwidth)
                reqdwidth = ll;

        spareheight = height - (2       /* Box */
                                + 1     /* Title */
                                + 1     /* Time */
                                + TC_YEARS - TC_DELETE + 1
                                + 1     /* Avoiding */
                                + 4);   /* Skip options */

        if  (spareheight < 0  ||  width < reqdwidth)  {
                doerror(wp, $E{btq time window too small});
                return  0;
        }

        wxoffset = (width - reqdwidth) / 2;

        rowlist->time_row = 2;          /* Follows heading unless.... */
        if  (spareheight > 0)  {
                rowlist->time_row++;
                spareheight--;
        }

        rowlist->rep_row = rowlist->time_row + 1;
        if  (spareheight > 0)  {
                rowlist->rep_row++;
                spareheight--;
        }

        rowlist->avd_row = rowlist->rep_row + TC_YEARS - TC_DELETE + 1;
        if  (spareheight > 0)  {
                rowlist->avd_row++;
                spareheight--;
        }

        rowlist->np_row = rowlist->avd_row + 1;
        if  (spareheight > 0)
                rowlist->np_row++;
        return  1;
}

/* Check that time is in future */

static int  checkfuture(TimeconRef tc)
{
        return  time((time_t *) 0) <= tc->tc_nexttime;
}

/* For `new' times - fill out structure with defaults taking into
   account defaults specified in the config file.  */

static void  initdefaults(TimeconRef tc)
{
        time_t  now = time((time_t *) 0) + 30;
        struct  tm      *t;

        t = localtime(&now);

        /* The ensuing code is to enable the user to force default
           days and months if (s)he wants by sticking `D's in the
           configuration file. I suspect forcing days is likely
           to be more useful than months, but we may as well be
           consistent.  First do any force day of week adjustments.  */

        if  (days_abbrev->def_alt >= 0)  {
                int     pday = days_abbrev->alt_nums[days_abbrev->def_alt];
                int     cday = t->tm_wday;

                if  (pday != 7) /* Don't get stuck if pratt puts holidays in */
                        while  (cday != pday)  {
                                now += SECSPERDAY;
                                if  (++cday >= 7) /* This doesn't include holidays */
                                        cday = 0;
                        }
                t = localtime(&now);
        }

        if  (Monthnames->def_alt >= 0)  {

                /* Default month set. Get which one. Note that the
                   numbers aren't necessarily the display order.  */

                int     pmonth = Monthnames->alt_nums[Monthnames->def_alt];
                int     cmon = t->tm_mon;
                int     cyear = t->tm_year;

                /* Fix up for leap years as ever.  Oh yes and this
                   program will freak out after 28th Feb 2100
                   because that isn't a leap year.  (I think I
                   won't be around then!!)  */

                month_days[1] = cyear % 4 == 0? 29: 28;
                while  (cmon != pmonth)  {
                        now += month_days[cmon] * SECSPERDAY;
                        if  (++cmon >= 12)  {
                                cmon = 0;
                                cyear++;
                                month_days[1] = cyear % 4 == 0? 29: 28;
                        }
                }
                if  (t->tm_mday > month_days[cmon])
                        now -= (t->tm_mday - month_days[cmon]) * SECSPERDAY;

                t = localtime(&now);
        }

        tc->tc_nexttime = now;

        /* Now set up default repeat parameters */

        tc->tc_rate = defaultrate;
        tc->tc_nvaldays = (USHORT) defavoid;

        if  (Roalts->def_alt >= 0)  {
                tc->tc_repeat = Roalts->alt_nums[Roalts->def_alt];
                if  (tc->tc_repeat >= TC_MINUTES)  {
                        if  (Units->def_alt >= 0)  {
                                tc->tc_repeat += Units->alt_nums[Units->def_alt];
                                if  (tc->tc_repeat == TC_MONTHSB  ||  tc->tc_repeat == TC_MONTHSE)  {
                                        tc->tc_mday = t->tm_mday;
                                        if  (tc->tc_repeat == TC_MONTHSE)
                                                tc->tc_mday = month_days[t->tm_mon] - t->tm_mday;
                                }
                        }
                        else
                                tc->tc_repeat = TC_HOURS;
                }
        }
}

/* Insert time/date at given place and insert into `Tposns' where we
   started the time, day of week, day of month, month and year.
   Return month.  */

static int  tinsert(WINDOW *wp, int row, int col, time_t tim, struct colmarks *tposns)
{
        int     dummy;
        struct  tm      *tc = localtime(&tim);

        tposns->tim_col = col;
        mvwprintw(wp, row, col, "%.2d:%.2d ", tc->tm_hour, tc->tm_min);
        getyx(wp, dummy, tposns->wd_col);
        waddstr(wp, disp_alt((int) tc->tm_wday, days_abbrev));
        waddch(wp, ' ');
        getyx(wp, dummy, tposns->md_col);
        wprintw(wp, "%.2d ", tc->tm_mday);
        getyx(wp, dummy, tposns->mon_col);
        waddstr(wp, disp_alt((int) tc->tm_mon, Monthnames));
        waddch(wp, ' ');
        getyx(wp, dummy, tposns->year_col);
        wprintw(wp, "%d", tc->tm_year + 1900);
        month_days[1] = tc->tm_year % 4 == 0? 29: 28;
        return  tc->tm_mon;
}

/* Display the stuff on the screen */

void  wtdisplay(WINDOW *wp, TimeconRef tc, struct rowmarks *rowlist, struct colmarks *tposns)
{
        int     repeating = 0, whichrow, whichcol, rc, dummy, ii, monthn;

        werase(wp);
#ifdef  HAVE_TERMINFO
        box(wp, 0, 0);
#else
        box(wp, '|', '-');
#endif
        mvwprintw(wp, 1, wxoffset, "%s %s ", Stprmpt, disp_str);
        getyx(wp, dummy, tposns->apr_col);
        waddstr(wp, tc->tc_istime? Apr_yes: Apr_no);
        if  (tc->tc_istime == 0)
                return;
        monthn = tinsert(wp, rowlist->time_row, wxoffset, tc->tc_nexttime, tposns);

        /* Now insert repeat times */

        mvwprintw(wp, rowlist->rep_row, wxoffset, "%s: ", Rpprmpt);
        getyx(wp, whichrow, whichcol);

        tposns->ro_col = whichcol;

        for  (rc = 0;  rc < Roalts->numalt;  rc++)  {
                if  (Roalts->alt_nums[rc] >= TC_MINUTES)  {
                        int     un;

                        for  (un = 0;  un < Units->numalt;  un++)  {
                                wmove(wp, whichrow, whichcol);
                                if  (Units->alt_nums[un] == tc->tc_repeat - TC_MINUTES)  {
                                        struct  colmarks        dumms;
                                        time_t  foreg;

                                        /* Mark that we are definitely
                                           repeating this one.  */

                                        repeating = 1;

                                        /* Ok this is the one we're
                                           repeating so insert `every'
                                           or equivalent number etc.  */

                                        wattron(wp, A_UNDERLINE);
                                        wprintw(wp, "%s ", Roalts->list[rc]);
                                        getyx(wp, tposns->int_row, tposns->int_col);
                                        wprintw(wp, "%3d %s", tc->tc_rate, Units->list[un]);
                                        if  (tc->tc_repeat == TC_MONTHSB  ||  tc->tc_repeat == TC_MONTHSE)  {
                                                int     mday = tc->tc_mday;
                                                wprintw(wp, " %s ", Dayprmpt);
                                                getyx(wp, dummy, tposns->mday_col);
                                                if  (tc->tc_repeat == TC_MONTHSE)
                                                        mday = month_days[monthn] - mday;
                                                wprintw(wp, "%2d", mday);
                                        }
                                        wattroff(wp, A_UNDERLINE);
                                        getyx(wp, dummy, tposns->dat_col);
                                        tposns->dat_col++;
                                        foreg = advtime(tc);
                                        tinsert(wp, dummy, tposns->dat_col, foreg, &dumms);
                                }
                                else
                                        waddstr(wp, Units->list[un]);
                                whichrow++;
                        }
                }
                else  {
                        wmove(wp, whichrow, whichcol);
                        if  (Roalts->alt_nums[rc] == tc->tc_repeat)  {
                                wattron(wp, A_UNDERLINE);
                                waddstr(wp, Roalts->list[rc]);
                                wattroff(wp, A_UNDERLINE);
                                tposns->dat_col = -1;
                                tposns->int_row = whichrow;
                        }
                        else
                                waddstr(wp, Roalts->list[rc]);
                        whichrow++;
                }
        }

        if  (repeating)  {
                mvwprintw(wp, rowlist->avd_row, wxoffset, "%s", Avprmpt);
                getyx(wp, dummy, tposns->adays_col);
                tposns->adays_col++;

                for  (ii = 0;  ii < TC_NDAYS;  ii++)
                        mvwaddstr(wp,
                                         rowlist->avd_row,
                                         tposns->adays_col + weekdlen*ii,
                                         (tc->tc_nvaldays & (1 << ii)) ?
                                         disp_alt(ii, days_abbrev):
                                         Ndprmpt);

                mvwprintw(wp, rowlist->np_row, wxoffset, "%s: ", Npprmpt);
                getyx(wp, whichrow, tposns->np_col);
                for  (rc = 0;  rc < Npalts->numalt;  rc++)  {
                        if  (tc->tc_nposs == Npalts->alt_nums[rc])  {
                                getyx(wp, tposns->np_sel_row, dummy);
                                wattron(wp, A_UNDERLINE);
                                waddstr(wp, Npalts->list[rc]);
                                wattroff(wp, A_UNDERLINE);
                        }
                        else
                                waddstr(wp, Npalts->list[rc]);
                        whichrow++;
                        wmove(wp, whichrow, tposns->np_col);
                }
        }
}

/* Get digits of time.
   Return:     -1 go back to previous field
                0 abort
                1 ok
                2 ok and don't bother with next field */

static int  gettdigs(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     row, col, dignum, coladd, ch, err_no, dig, ctim, newtim;
        struct  tm      *t = localtime(&tc->tc_nexttime);

        row = rm->time_row;
        col = cm->tim_col;
        dignum = 0;
        coladd = 0;

        for  (;;)  {

                wmove(wp, row, col+coladd);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        err_no = $E{btq hours unknown command};
                err:
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, dignum < 2? $H{wtime digits hours}: $H{wtime digits mins});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:
                case  $K{key cursor down}:
                        if  (checkfuture(tc))
                                return  2;
                notfut:
                        err_no = $E{btq time not in future};
                        goto  err;

                case  $K{key halt}:
                        if  (checkfuture(tc))
                                return  3;
                        goto  notfut;

                case  $K{key cursor up}:
                        return  -1;

                case  $K{key previous field}:
                        return  -1;

                case  $K{key next field}:
                        return  1;

                case  $K{btq key time left}:
                        if  (dignum <= 0)
                                return  -1;
                        dignum--;
                        if  (--coladd == 2)
                                coladd--;
                        continue;

                case  $K{btq key time right}:
                        if  (dignum >= 3)
                                return  1;
                        dignum++;
                        if  (++coladd == 2)
                                coladd++;
                        continue;

                case '0':case '1':case '2':case '3':case '4':
                case '5':case '6':case '7':case '8':case '9':

                        dig = ch - '0';

                        /* What happens next depends on whether we are
                           at the first or second digit of the
                           hours / minutes fields */

                        switch  (dignum)  {
                        case  0:        /* Tens of hours */
                                if  (dig > 2)  {
                                thbd:
                                        err_no = $E{btq bad hours digit};
                                        goto  err;
                                }
                                ctim = t->tm_hour;
                                newtim = ctim % 10 + dig * 10;
                                if  (newtim > 20)  {
                                        newtim = 20;
                                        waddch(wp, (chtype) ch);
                                        waddch(wp, '0');
                                }
                                else
                                        waddch(wp, (chtype) ch);
                                changes++;
                                coladd++;
                                dignum++;
                                tc->tc_nexttime += (newtim - ctim) * 60 * 60;
                                t->tm_hour = newtim;
                                continue;

                        case  1:        /* Hours */
                                if  (t->tm_hour >= 20  &&  dig > 3)
                                        goto  thbd;
                                ctim = t->tm_hour;
                                newtim = (ctim / 10) * 10 + dig;
                                waddch(wp, (chtype) ch);
                                changes++;
                                coladd += 2;
                                dignum++;
                                tc->tc_nexttime += (newtim - ctim) * 60 * 60;
                                t->tm_hour = newtim;
                                continue;

                        case  2:        /* Tens of minutes */
                                if  (dig > 5)  {
                                        err_no = $E{btq bad mins digit};
                                        goto  err;
                                }
                                ctim = t->tm_min;
                                newtim = ctim % 10 + dig * 10;
                                waddch(wp, (chtype) ch);
                                changes++;
                                coladd++;
                                dignum++;
                                tc->tc_nexttime += (newtim - ctim) * 60;
                                t->tm_min = newtim;
                                continue;

                        case  3:        /* Minutes  */
                                ctim = t->tm_min;
                                newtim = (ctim / 10) * 10 + dig;
                                waddch(wp, (chtype) ch);
                                changes++;
                                tc->tc_nexttime += (newtim - ctim) * 60;
                                t->tm_min = newtim;
                                return  1;
                        }

                case  $K{btq key time incr}:

                        /* Increment case follows through entire date
                           if necessary.  */

                        switch  (dignum)  {
                        case  3:
                                tc->tc_nexttime += 60;
                                break;
                        case  2:
                                tc->tc_nexttime += 60 * 10;
                                break;
                        case  1:
                                tc->tc_nexttime += 60 * 60;
                                break;
                        case  0:
                                tc->tc_nexttime += 60 * 60 * 12;
                                break;
                        }

                        /* Warning: This will break if the positions
                           of hours and minutes digits change.
                           If they do you will need to
                           reinitialise `row' and `col' from
                           rm->time_row and cm->tim_col
                           respectively.  */

                        t = localtime(&tc->tc_nexttime);
                        changes++;
                        wtdisplay(wp, tc, rm, cm);
                        continue;

                case  $K{btq key time decr}:

                        switch  (dignum)  {
                        case  3:
                                tc->tc_nexttime -= 60;
                                break;
                        case  2:
                                tc->tc_nexttime -= 60 * 10;
                                break;
                        case  1:
                                tc->tc_nexttime -= 60 * 60;
                                break;
                        case  0:
                                tc->tc_nexttime -= 60 * 60 * 12;
                                break;
                        }

                        /* Warning: See previous warning for increment case.  */

                        t = localtime(&tc->tc_nexttime);
                        changes++;
                        wtdisplay(wp, tc, rm, cm);
                        continue;
                }
        }
}

static int  getdw(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     row, col, ch, err_no;

        row = rm->time_row;
        col = cm->wd_col;

        for  (;;)  {

                wmove(wp, row, col);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        err_no = $E{btq weekday unknown command};
                err:
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{wtime day of week});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:
                case  $K{key cursor down}:
                        if  (checkfuture(tc))
                                return  2;
                notfut:
                        err_no = $E{btq time not in future};
                        goto  err;

                case  $K{key halt}:
                        if  (checkfuture(tc))
                                return  3;
                        goto  notfut;

                case  $K{key cursor up}:
                        return  -1;

                case  $K{btq key time left}:
                case  $K{key previous field}:
                        return  -1;

                case  $K{btq key time right}:
                case  $K{key next field}:
                        return  1;

                case  $K{btq key time incr}:
                        changes++;
                        tc->tc_nexttime += SECSPERDAY;
                        wtdisplay(wp, tc, rm, cm);
                        continue;

                case  $K{btq key time decr}:
                        changes++;
                        tc->tc_nexttime -= SECSPERDAY;
                        wtdisplay(wp, tc, rm, cm);
                        continue;
                }
        }
}

static int  getdm(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     row, col, dignum, ch, err_no, dig, newday, cday, cmon;
        struct  tm      *t = localtime(&tc->tc_nexttime);

        cday = t->tm_mday;
        cmon = t->tm_mon;
        row = rm->time_row;
        col = cm->md_col;       /* NB This may move!!! */
        dignum = 1;

        /* Check leap year stuff is ok */

        month_days[1] = t->tm_year % 4 == 0? 29: 28;

        for  (;;)  {

                wmove(wp, row, col+dignum);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        err_no = $E{btq monthday unknown command};
                err:
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{wtime day of mon});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:
                case  $K{key cursor down}:
                        if  (checkfuture(tc))
                                return  2;
                notfut:
                        err_no = $E{btq time not in future};
                        goto  err;

                case  $K{key halt}:
                        if  (checkfuture(tc))
                                return  3;
                        goto  notfut;

                case  $K{key cursor up}:
                        return  -1;

                case  $K{btq key time left}:
                        if  (dignum > 0)  {
                                dignum = 0;
                                continue;
                        }

                case  $K{key previous field}:
                        return  -1;

                case  $K{btq key time right}:
                        if  (dignum <= 0)  {
                                dignum++;
                                continue;
                        }

                case  $K{key next field}:
                        return  1;

                case '0':case '1':case '2':case '3':case '4':
                case '5':case '6':case '7':case '8':case '9':

                        dig = ch - '0';

                        /* What happens next depends on whether we are
                           at the first or second digit of the
                           day of month field */

                        if  (dignum == 0)  {
                                if  (dig > 3)  {
                                        err_no = $E{btq monthday bad digit};
                                        goto  err;
                                }
                                newday = cday % 10 + dig * 10;
                                dignum++;
                        }
                        else
                                newday = (cday / 10) * 10 + dig;

                        if  (newday > month_days[cmon])
                                newday = month_days[cmon];
                        tc->tc_nexttime += (newday - cday) * SECSPERDAY;
                        t = localtime(&tc->tc_nexttime);
                        cday = t->tm_mday;
                        cmon = t->tm_mon;
                        changes++;
                        break;

                case  $K{btq key time incr}:
                        tc->tc_nexttime += (dignum == 0)? 10 * SECSPERDAY: SECSPERDAY;
                        t = localtime(&tc->tc_nexttime);
                        cday = t->tm_mday;
                        cmon = t->tm_mon;
                        changes++;
                        break;

                case  $K{btq key time decr}:
                        tc->tc_nexttime -= (dignum == 0)? 10 * SECSPERDAY: SECSPERDAY;
                        t = localtime(&tc->tc_nexttime);
                        cday = t->tm_mday;
                        cmon = t->tm_mon;
                        changes++;
                        break;
                }

                wtdisplay(wp, tc, rm, cm);
                col = cm->md_col;
        }
}

static int  getmon(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     row, col, ch, err_no;
        struct  tm      *t = localtime(&tc->tc_nexttime);

        row = rm->time_row;
        col = cm->mon_col;      /* NB This may move!!! */

        for  (;;)  {

                wmove(wp, row, col);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        err_no = $E{btq month unknown command};
                err:
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{wtime set month});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:
                case  $K{key cursor down}:
                        if  (checkfuture(tc))
                                return  2;
                notfut:
                        err_no = $E{btq time not in future};
                        goto  err;

                case  $K{key halt}:
                        if  (checkfuture(tc))
                                return  3;
                        goto  notfut;

                case  $K{key cursor up}:
                        return  -1;

                case  $K{btq key time left}:
                case  $K{key previous field}:
                        return  -1;

                case  $K{btq key time right}:
                case  $K{key next field}:
                        return  1;

                case  $K{btq key time incr}:
                        month_days[1] = t->tm_year % 4 == 0? 29: 28;
                        tc->tc_nexttime += month_days[t->tm_mon] * SECSPERDAY;
                        if  (t->tm_mon < 11  &&  t->tm_mday > month_days[t->tm_mon+1])
                                tc->tc_nexttime -= (t->tm_mday - month_days[t->tm_mon+1]) * SECSPERDAY;
                        t = localtime(&tc->tc_nexttime);
                        changes++;
                        break;

                case  $K{btq key time decr}:
                        month_days[1] = t->tm_year % 4 == 0? 29: 28;
                        ch = month_days[t->tm_mon == 0? 11: t->tm_mon-1];
                        if  (ch < t->tm_mday)
                                ch = t->tm_mday;
                        tc->tc_nexttime -= ch * SECSPERDAY;
                        t = localtime(&tc->tc_nexttime);
                        changes++;
                        break;
                }

                wtdisplay(wp, tc, rm, cm);
                col = cm->mon_col;
        }
}

static int  getyr(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     row, col, ch, err_no;
        struct  tm      *t = localtime(&tc->tc_nexttime);

        row = rm->time_row;
        col = cm->year_col+3;   /* NB This may move!!! */

        for  (;;)  {
                wmove(wp, row, col);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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

                default:
                        err_no = $E{btq year unknown command};
                err:
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{wtime set year});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:
                case  $K{key cursor down}:
                        if  (checkfuture(tc))
                                return  2;
                notfut:
                        err_no = $E{btq time not in future};
                        goto  err;

                case  $K{key halt}:
                        if  (checkfuture(tc))
                                return  3;
                        goto  notfut;

                case  $K{key cursor up}:
                        return  -1;

                case  $K{key previous field}:
                case  $K{btq key time left}:
                        return  -1;

                case  $K{key next field}:
                case  $K{btq key time right}:
                        return  1;

                        /* Increment and decrement year.  All this
                           messing around with leap years gives
                           me a whole new dimension of sympathy
                           for Brutus & Cassius.  */

                case  $K{btq key time incr}:
                        tc->tc_nexttime += 365 * SECSPERDAY;
                        if  (t->tm_mon > 1)  {
                                if  (t->tm_year % 4 == 3)
                                        tc->tc_nexttime += SECSPERDAY;
                        }
                        else  {
                                if  (t->tm_mon == 1)  {
                                        if  (t->tm_mday < 29)
                                                tc->tc_nexttime += SECSPERDAY;
                                }
                                else  if  (t->tm_year % 4 == 0)
                                        tc->tc_nexttime += SECSPERDAY;
                        }
                        t = localtime(&tc->tc_nexttime);
                        changes++;
                        break;

                case  $K{btq key time decr}:
                        if  (t->tm_year <= 90)  {
                                err_no = $E{btq stoneage};
                                goto  err;
                        }
                        tc->tc_nexttime -= 365 * SECSPERDAY;

                        if  (t->tm_mon > 1)  {
                                if  (t->tm_year % 4 == 0)
                                        tc->tc_nexttime -= SECSPERDAY;
                        }
                        else  {
                                if  (t->tm_mon == 1)  {
                                        if  (t->tm_mday == 29)
                                                tc->tc_nexttime -= SECSPERDAY;
                                }
                                else  if  (t->tm_year % 4 == 1)
                                        tc->tc_nexttime -= SECSPERDAY;
                        }
                        t = localtime(&tc->tc_nexttime);
                        changes++;
                        break;
                }

                wtdisplay(wp, tc, rm, cm);
                col = cm->year_col + 3;
        }
}

/* The following routines manipulate parts of the display.
   Return values are:   -1 go back to previous bit
                        0 abort
                        1 ok */

static int  dateaccept(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     i, ret;
        static  int     (*fns[])() =
                { gettdigs, getdw, getdm, getmon, getyr };

        i = 0;
        while  (i < sizeof(fns)/sizeof(int (*)()))  {
                ret = (*fns[i])(wp, tc, rm, cm);
                if  (ret == 0)
                        return  0;
                if  (ret >= 2)
                        return  ret - 1;
                if  (ret < 0)  {
                        if  (i <= 0)
                                return  -1;
                        i--;
                }
                else
                        i++;
        }
        return  1;
}

static int  roaccept(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     ch, err_no, on, un;
        ULONG   oldval;
        unsigned        oldmday;
        LONG    reti;
        int     nrows = Roalts->numalt + Units->numalt - 1;
        int     indx = cm->int_row - rm->rep_row;
        int     cursrow = cm->int_row;
        const   char    *savestr;
        SHORT   *rvals = Roalts->alt_nums;
        SHORT   *uvals = Units->alt_nums;
        static  struct  sctrl   wnsc = { $H{Repeat unit full}, (char **(*)()) 0, 3, 0, 0, MAG_P, 1L, 999L, (char *) 0 },
                wnday = { $H{Repeat month day}, (char **(*)()) 0, 2, 0, 0, MAG_P, 1L, 31L, (char *) 0 };

        for  (;;)  {
                wmove(wp, cursrow, cm->ro_col);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
                while  (ch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(wp, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(wp, &escr);

                switch  (ch)  {
                case   EOF:
                        continue;
                default:
                        err_no = $E{btq repeat unknown command};
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{btq repeat options});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:

                        /* If it isn't current, select it otherwise
                           take it as the choice and quit */

                        if  (cursrow == cm->int_row)
                                return  1;

                        un = 0;
                        for  (on = 0;  on < Roalts->numalt;  on++)  {
                                if  (rvals[on] >= TC_MINUTES)  {
                                        if  (indx - on < Units->numalt)
                                                goto  setinc;
                                        else
                                                un = Units->numalt;
                                }
                                if  (on + un >= indx)
                                        break;
                        }

                        /* Delete or retain case */

                        tc->tc_repeat = rvals[on+un];
                        changes++;
                        wtdisplay(wp, tc, rm, cm);
                        continue;

                setinc:
                        /* Other case - reset count as well.  */

                        oldval = tc->tc_repeat;
                        oldmday = tc->tc_mday;
                        tc->tc_repeat = uvals[indx-on] + TC_MINUTES;
                        changes++;
                numset:
                        if  (tc->tc_repeat == TC_MONTHSB  ||  tc->tc_repeat == TC_MONTHSE)  {
                                int     mday = oldmday, mnl;
                                struct  tm  *tt = localtime(&tc->tc_nexttime);
                                month_days[1] = tt->tm_year % 4 == 0? 29: 28;
                                mnl = month_days[tt->tm_mon];
                                if  (oldval == TC_MONTHSE)  {
                                        if  (mday >= mnl)
                                                mday = mnl - 1;
                                        else
                                                mday = mnl - mday;
                                }
                                else  if  (oldval == TC_MONTHSB)  {
                                        if  (mday > mnl)
                                                mday = mnl;
                                }
                                else
                                        mday = tt->tm_mday;
                                if  (tc->tc_repeat == TC_MONTHSE)
                                        mday = mnl - mday;
                                tc->tc_mday = (unsigned char) mday;
                        }
                        wtdisplay(wp, tc, rm, cm);
                        wnsc.col = cm->int_col;
                        savestr = disp_str;
                        wnsc.msg = wnday.msg = Units->list[indx-on];
                        wattron(wp, A_UNDERLINE);
                        reti = wnum(wp, cm->int_row, &wnsc, (LONG) tc->tc_rate);
                        if  (reti == -1L)  {    /* Aborted */
                                tc->tc_repeat = (unsigned char) oldval;
                                tc->tc_mday = (unsigned char) oldmday;
                        }
                        else  {
                                if  (reti > 0L)
                                        tc->tc_rate = reti;
                                if  (tc->tc_repeat == TC_MONTHSB  ||  tc->tc_repeat == TC_MONTHSE)  {
                                        int     mday = tc->tc_mday;
                                        struct  tm  *tt = localtime(&tc->tc_nexttime);
                                        month_days[1] = tt->tm_year % 4 == 0? 29: 28;
                                        if  (tc->tc_repeat == TC_MONTHSE)
                                                mday = month_days[tt->tm_mon] - mday;
                                        else  if  (mday > month_days[tt->tm_mon])
                                                mday = month_days[tt->tm_mon];
                                        wnday.col = cm->mday_col;
                                        wnday.vmax = month_days[tt->tm_mon];
                                        reti = wnum(wp, cm->int_row, &wnday, (LONG) mday);
                                        if  (reti == -1L)  {    /* Aborted */
                                                tc->tc_repeat = (unsigned char) oldval;
                                                tc->tc_mday = (unsigned char) oldmday;
                                        }
                                        else  {
                                                if  (reti < 0)
                                                        reti = mday;
                                                if  (tc->tc_repeat == TC_MONTHSE)
                                                        tc->tc_mday = month_days[tt->tm_mon] - reti;
                                                else
                                                        tc->tc_mday = (unsigned char) reti;
                                        }
                                }
                        }
                        disp_str = savestr;
                        wattroff(wp, A_UNDERLINE);
                        wtdisplay(wp, tc, rm, cm);
                        changes++;
                        continue;

                case  $K{btq key time incr}:
                case  $K{btq key time decr}:
                        oldval = tc->tc_repeat;
                        oldmday = tc->tc_mday;
                        for  (on = 0;  on < Units->numalt;  on++)
                                if  (oldval - TC_MINUTES == uvals[on])  {
                                        on = indx - on;
                                        goto  numset;
                                }
                        on = indx;
                        goto  numset;

                case  $K{key halt}:
                        return  2;

                case  $K{btq key time left}:
                case  $K{key cursor up}:
                        if  (indx <= 0)
                                indx = nrows;
                        indx--;
                        cursrow = rm->rep_row + indx;
                        continue;

                case  $K{btq key time right}:
                case  $K{key cursor down}:
                        if  (++indx >= nrows)
                                indx = 0;
                        cursrow = rm->rep_row + indx;
                        continue;

                case  $K{key previous field}:
                        return  -1;

                case  $K{key next field}:
                        return  1;
                }
        }

}

static int  atallaccept(WINDOW *wp, TimeconRef tc, struct colmarks *cm)
{
        int     ch;

        for  (;;)  {
                wmove(wp, 1, cm->apr_col);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        doerror(wp, $E{btq timecon unknown command});
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{Do you want to set time});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key halt}:
                        return  2;

                case  $K{key eol}:
                case  $K{key next field}:
                case  $K{btq key time right}:
                case  $K{key cursor down}:
                        return  1;

                case  $K{btq key time set}:
                        if  (!tc->tc_istime)  {
                                tc->tc_istime = 1;
                                changes++;
                        }
                        return  1;

                case  $K{btq key time unset}:
                        if  (tc->tc_istime)  {
                                tc->tc_istime = 0;
                                changes++;
                        }
                        return  1;

                case  $K{btq key time toggle}:
                        tc->tc_istime = !tc->tc_istime;
                        changes++;
                        return  1;
                }
        }
}

static int  avdaccept(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     ch, err_no, dayno = 0;
        int     cursrow = rm->avd_row, curscol = cm->adays_col;

        for  (;;)  {
                wmove(wp, cursrow, curscol);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        err_no = $E{btq avoid days unknown command};
                err:
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{Wtime avoiding help});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key halt}:
                        if  ((tc->tc_nvaldays & TC_ALLWEEKDAYS) == (unsigned) TC_ALLWEEKDAYS)  {
                                err_no = $E{btq avoid days cannot have all set};
                                goto  err;
                        }
                        return  2;

                case  $K{key previous field}:
                        if  (dayno <= 0)
                                return  -1;

                case  $K{btq key time left}:
                        if  (--dayno < 0)
                                dayno = TC_NDAYS-1;
                        curscol = cm->adays_col + dayno*weekdlen;
                        continue;

                case  $K{key eol}:
                        if  ((tc->tc_nvaldays & TC_ALLWEEKDAYS) == (unsigned) TC_ALLWEEKDAYS)  {
                                err_no = $E{btq avoid days cannot have all set};
                                goto  err;
                        }
                        return  1;

                case  $K{key next field}:
                        if  (dayno >= TC_NDAYS-1)  {
                                if  ((tc->tc_nvaldays & TC_ALLWEEKDAYS) == (unsigned) TC_ALLWEEKDAYS)  {
                                        err_no = $E{btq avoid days cannot have all set};
                                        goto  err;
                                }
                                return  1;
                        }

                case  $K{btq key time right}:
                mvr:
                        if  (++dayno >= TC_NDAYS)
                                dayno = 0;
                        curscol = cm->adays_col + dayno*weekdlen;
                        continue;

                case  $K{btq key time set}:
                        if  (tc->tc_nvaldays & (1 << dayno))
                                goto  mvr;
                        tc->tc_nvaldays |= 1 << dayno;
                        changes++;
                        wtdisplay(wp, tc, rm, cm);
                        goto  mvr;

                case  $K{btq key time unset}:
                        if  (!(tc->tc_nvaldays & (1 << dayno)))
                                goto  mvr;
                        tc->tc_nvaldays &= ~(1 << dayno);
                        changes++;
                        wtdisplay(wp, tc, rm, cm);
                        goto  mvr;

                case  $K{btq key time toggle}:
                        tc->tc_nvaldays ^= 1 << dayno;
                        changes++;
                        wtdisplay(wp, tc, rm, cm);
                        goto  mvr;
                }
        }
}

static int  skaccept(WINDOW *wp, TimeconRef tc, struct rowmarks *rm, struct colmarks *cm)
{
        int     ch, err_no;
        int     nrows = Npalts->numalt;
        int     indx = cm->np_sel_row - rm->np_row;
        int     cursrow = cm->np_sel_row;
        SHORT   *rvals = Npalts->alt_nums;

        for  (;;)  {

                wmove(wp, cursrow, cm->np_col);
                wrefresh(wp);

                do  ch = getkey(MAG_A|MAG_P);
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
                default:
                        err_no = $E{btq skip unknown command};
                        doerror(wp, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(wp, $H{Wtime if not poss});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key abort}:
                        return  0;

                case  $K{key eol}:

                        /* If it isn't current, select it otherwise
                           take it as the choice and quit */

                        if  (cursrow != cm->np_sel_row)  {
                                tc->tc_nposs = rvals[indx];
                                changes++;
                                wtdisplay(wp, tc, rm, cm);
                                /* That resets cm->np_sel_row */
                                continue;
                        }

                case  $K{key halt}:
                        return  1;

                case  $K{btq key time left}:
                case  $K{key cursor up}:
                        if  (indx <= 0)
                                indx = nrows;
                        indx--;
                        cursrow = rm->np_row + indx;
                        continue;

                case  $K{btq key time right}:
                case  $K{key cursor down}:
                        if  (++indx >= nrows)
                                indx = 0;
                        cursrow = rm->np_row + indx;
                        continue;

                case  $K{key previous field}:
                        return  -1;

                case  $K{key next field}:
                        return  1;
                }
        }
}

int  wtimes(WINDOW *wp, BtjobRef jp, const char *msg)
{
        int             ret, hadbefore;
        unsigned        retc;
        ULONG           xindx;
        BtjobRef        bjp;
        struct  rowmarks        rowlist;
        struct  colmarks        collist;
        Timecon res;

        disp_str = msg;
        res = jp->h.bj_times;
        disp_arg[0] = jp->h.bj_job;
        hadbefore = res.tc_istime;
        changes = 0;

        if  (!Stprmpt  &&  !initprompts(wp))
                return  0;

        if  (!initdims(wp, &rowlist))
                return  0;

        /* Splurge it out on the screen */

        wtdisplay(wp, &res, &rowlist, &collist);
        select_state($S{btq wtime state});
        Ew = wp;

 atallagain:
        if  ((ret = atallaccept(wp, &res, &collist)) <= 0)
                return  -1;
        if  (res.tc_istime == 0)
                goto  done;
        if  (!hadbefore)
                initdefaults(&res);
        else  {
                if  (ret > 1)
                        goto  done;
                if  (res.tc_rate == 0)
                        res.tc_rate = defaultrate;
        }
        wtdisplay(wp, &res, &rowlist, &collist);

 dateagain:
        ret = dateaccept(wp, &res, &rowlist, &collist);
        if  (ret == 0)
                return  -1;
        if  (ret < 0)
                goto  atallagain;
        if  (ret > 1)
                goto  done;
 roagain:
        ret = roaccept(wp, &res, &rowlist, &collist);
        if  (ret == 0)
                return  -1;
        if  (ret < 0)
                goto  dateagain;

        if  (res.tc_repeat >= TC_MINUTES  &&  ret < 2)  {

                /* Redisplay it in the correct form At the same time
                   this will put up the skip options again.  */

                wtdisplay(wp, &res, &rowlist, &collist);
        avdagain:
                ret = avdaccept(wp, &res, &rowlist, &collist);
                if  (ret == 0)
                        return  -1;
                if  (ret < 0)
                        goto  roagain;
                if  (ret > 1)
                        goto  done;
                ret = skaccept(wp, &res, &rowlist, &collist);
                if  (ret == 0)
                        return  -1;
                if  (ret < 0)
                        goto  avdagain;
        }

 done:
        if  (!changes)
                return  -1;

        bjp = &Xbuffer->Ring[xindx = getxbuf()];
        *bjp = *jp;
        bjp->h.bj_times = res;
        wjmsg(J_CHANGE, xindx);
        if  ((retc = readreply()) != J_OK)  {
                qdojerror(retc, bjp);
                freexbuf(xindx);
                return  -1;
        }
        freexbuf(xindx);
        return  1;
}
