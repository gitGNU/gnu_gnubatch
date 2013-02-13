/* bq_hols.c -- holiday stuff for gbch-q

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
#ifdef  OS_LINUX                        /* Need this for umask decln */
#include <sys/stat.h>
#endif
#include <curses.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "timecon.h"
#include "btmode.h"
#include "btuser.h"
#include "magic_ch.h"
#include "statenums.h"
#include "optflags.h"

static  HelpaltRef Monthnames;
static  int     monthnlen;

extern  WINDOW  *escr,
                *hlpscr,
                *Ew;

#define ppermitted(flg) (mypriv->btu_priv & flg)

#define CHANGE_MK       16
#define NEXT_MK         1
#define PREV_MK         2

#define DAYSET_FLAG     0x80

#define MWIDTH          (monthnlen + 7*3 + 2)
#define MHEIGHT         5
#define INITDISP        2

#define ISHOL(map, day) map[(day) >> 3] & (1 << (day & 7))
#define SETHOL(map, day)        map[(day) >> 3] |= (1 << (day & 7))

void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);

static void     fillyear(unsigned char *ymap, unsigned char monthd[12][5][7], const int year)
{
        int     startday = (1 + (year - 90) + (year - 89) / 4) % 7;
        int     month, week, yday = 0, mday, wday, daysinmon;
        month_days[1] = year % 4 == 0? 29: 28;

        for  (month = 0;  month < 12;  month++)  {
                daysinmon = month_days[month];
                BLOCK_ZERO(monthd[month][0], 7);
                BLOCK_ZERO(monthd[month][4], 7);
                mday = 1;
                for  (wday = startday;  wday < 7;  wday++)  {
                        monthd[month][0][wday] = mday++;
                        if  (ISHOL(ymap, yday))
                                monthd[month][0][wday] |= DAYSET_FLAG;
                        yday++;
                }
                for  (week = 1;  week < 5;  week++)
                        for  (wday = 0;  wday < 7;  wday++)  {
                                monthd[month][week][wday] = mday++;
                                if  (ISHOL(ymap, yday))
                                        monthd[month][week][wday] |= DAYSET_FLAG;
                                yday++;
                                if  (mday > daysinmon)
                                        goto  dun;
                        }
                for  (wday = 0;  wday < 7;  wday++)  {
                        monthd[month][0][wday] = mday++;
                        if  (ISHOL(ymap, yday))
                                monthd[month][0][wday] |= DAYSET_FLAG;
                        yday++;
                        if  (mday > daysinmon)
                                break;
                }
        dun:
                startday = (startday + daysinmon) % 7;
        }
}

static  void    readyear(unsigned char *ymap, unsigned char monthd[12][5][7], const int year)
{
        int     startday = (1 + (year - 90) + (year - 89) / 4) % 7;
        int     month, week, yday = 0, mday, wday, daysinmon;
        month_days[1] = year % 4 == 0? 29: 28;

        BLOCK_ZERO(ymap, YVECSIZE);
        for  (month = 0;  month < 12;  month++)  {
                daysinmon = month_days[month];
                mday = 1;
                for  (wday = startday;  wday < 7;  wday++)  {
                        if  (monthd[month][0][wday] & DAYSET_FLAG)
                                SETHOL(ymap, yday);
                        mday++;
                        yday++;
                }
                for  (week = 1;  week < 5;  week++)
                        for  (wday = 0;  wday < 7;  wday++)  {
                                if  (monthd[month][week][wday] & DAYSET_FLAG)
                                        SETHOL(ymap, yday);
                                yday++;
                                mday++;
                                if  (mday > daysinmon)
                                        goto  dun;
                        }
                for  (wday = 0;  wday < 7;  wday++)  {
                        if  (monthd[month][0][wday] & DAYSET_FLAG)
                                SETHOL(ymap, yday);
                        yday++;
                        mday++;
                        if  (mday > daysinmon)
                                break;
                }
        dun:
                startday = (startday + daysinmon) % 7;
        }
}

static  void    displayyear(unsigned char monthd[12][5][7], const int year)
{
        int     quarter, monq, month, wday, week;
        int     crow = INITDISP, ccol;

        erase();

        mvprintw(0, (COLS - 4) / 2, "%d", year + 1900);

        for  (quarter = 0;  quarter < 12;  quarter += 3)  {
                ccol = 0;
                for  (monq = 0;  monq < 3;  monq++)  {
                        month = quarter + monq;
                        mvprintw(crow, ccol, disp_alt(month, Monthnames));
                        ccol += monthnlen;
                        for  (wday = 0;  wday < 7;  wday++)  {
                                int     dayno = monthd[month][0][wday], mday;
                                ccol++;
                                if  ((mday = dayno) != 0)  {
                                        if  (dayno & DAYSET_FLAG)  {
                                                mday &= ~DAYSET_FLAG;
#ifdef  HAVE_TERMINFO
                                                attron(A_REVERSE);
#else
                                                standout();
#endif
                                        }
#ifdef  HAVE_TERMINFO
                                        attron(wday == 0 || wday == 6? A_DIM: A_BOLD);
#endif
                                        mvprintw(crow, ccol, "%2d", mday);
#ifdef  HAVE_TERMINFO
                                        attrset(0);
#else
                                        if  (dayno & DAYSET_FLAG)
                                                standend();
#endif
                                }
                                ccol += 2;
                        }
                        ccol += 2;
                }
                for  (week = 1;  week < 5;  week++)  {
                        crow++;
                        ccol = 0;
                        for  (monq = 0;  monq < 3;  monq++)  {
                                month = quarter + monq;
                                ccol += monthnlen;
                                for  (wday = 0;  wday < 7;  wday++)  {
                                        int     dayno = monthd[month][week][wday], mday;
                                        ccol++;
                                        if  ((mday = dayno) != 0)  {
                                                if  (dayno & DAYSET_FLAG)  {
                                                        mday &= ~DAYSET_FLAG;
#ifdef  HAVE_TERMINFO
                                                        attron(A_REVERSE);
#else
                                                        standout();
#endif
                                                }
#ifdef  HAVE_TERMINFO
                                                attron(wday == 0 || wday == 6? A_DIM: A_BOLD);
#endif
                                                mvprintw(crow, ccol, "%2d", mday);
#ifdef  HAVE_TERMINFO
                                                attrset(0);
#else
                                                if  (dayno & DAYSET_FLAG)
                                                        standend();
#endif
                                        }
                                        ccol += 2;
                                }
                                ccol += 2;
                        }
                }
                crow++;
        }
#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
}

/* Edit year map.
   Return 0 if finished 1 edit next year 2 edit
   previous year.  Add 16 if changes made.  */

static int      edithols(unsigned char monthd[12][5][7], const int year, const int readonly)
{
        int     curmon = 0, curweek, currwday, currdm, ccol, crow, kch;
        int     changes = 0;

        displayyear(monthd, year);

        /* Initialise to point to 1st January */

        for  (currwday = 0;  currwday < 7;  currwday++)
                if  ((currdm = monthd[0][0][currwday] & ~DAYSET_FLAG) == 1)
                        break;
        ccol = monthnlen + currwday * 3 + 2;
        curweek = 0;
        crow = INITDISP;

        for  (;;)  {
                move(crow, ccol);
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
                        doerror(stdscr, $E{btq hols unknown command});

                case  EOF:
                        continue;

                case  $K{key help}:
                        dochelp(stdscr, $H{btq holidays state});
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key halt}:
                        return  changes? CHANGE_MK: 0;

                case  $K{key screen up}:
                case  $K{btq key hols prev}:
                        return  changes?  CHANGE_MK | PREV_MK: PREV_MK;

                case  $K{key screen down}:
                case  $K{btq key hols next}:
                        return  changes?  CHANGE_MK | NEXT_MK: NEXT_MK;

                case  $K{key top}:
                topp:
                        curmon = curweek = 0;
                        for  (currwday = 0;  currwday < 7;  currwday++)
                                if  ((currdm = monthd[0][0][currwday] & ~DAYSET_FLAG) == 1)
                                        break;
                        ccol = monthnlen + currwday * 3 + 2;
                        crow = INITDISP;
                        continue;

                case  $K{key bottom}:
                bott:
                        curmon = 11;
                        curweek = 0;
                        for  (currwday = 0;  currwday < 7;  currwday++)
                                if  ((currdm = monthd[11][0][currwday] & ~DAYSET_FLAG) == 1)
                                        break;
                        ccol = monthnlen + 2 * MWIDTH + currwday * 3 + 2;
                        crow = INITDISP + 3 * MHEIGHT;
                        continue;

                case  $K{key next field}:
                nfld:
                        if  (++curmon >= 12)
                                goto  topp;
                restfld:
                        curweek = 0;
                        for  (currwday = 0;  currwday < 7;  currwday++)
                                if  ((currdm = monthd[curmon][0][currwday] & ~DAYSET_FLAG) == 1)
                                        break;
                        ccol = monthnlen + (curmon % 3) * MWIDTH + currwday * 3 + 2;
                        crow = INITDISP + (curmon / 3) * MHEIGHT;
                        continue;

                case  $K{key previous field}:
                pfld:
                        if  (--curmon < 0)
                                goto  bott;
                        goto  restfld;

                case  $K{key cursor up}:
                        if  (currdm < 8)
                                goto  pfld;
                        currdm -= 7;
                        if  (--curweek < 0)
                                curweek = 4;
                        crow = INITDISP + (curmon / 3) * MHEIGHT + curweek;
                        continue;

                case  $K{key cursor down}:
                        if  (currdm + 7 > month_days[curmon])
                                goto  nfld;
                        currdm += 7;
                        if  (++curweek > 4)
                                curweek = 0;
                        crow = INITDISP + (curmon / 3) * MHEIGHT + curweek;
                        continue;

                case  $K{btq key hols left}:
                        if  (--currdm <= 0)
                                goto  pfld;
                        if  (--currwday < 0)  {
                                currwday = 6;
                                if  (--curweek < 0)
                                        curweek = 4;
                                crow = INITDISP + (curmon / 3) * MHEIGHT + curweek;
                        }
                        ccol = monthnlen + (curmon % 3) * MWIDTH + currwday * 3 + 2;
                        continue;

                case  $K{btq key hols right}:
                        if  (++currdm > month_days[curmon])
                                goto  nfld;
                        if  (++currwday > 6)  {
                                currwday = 0;
                                if  (++curweek > 4)
                                        curweek = 0;
                                crow = INITDISP + (curmon / 3) * MHEIGHT + curweek;
                        }
                        ccol = monthnlen + (curmon % 3) * MWIDTH + currwday * 3 + 2;
                        continue;

                case  $K{btq key hols set}:
                        if  (readonly)  {
                                doerror(stdscr, $E{btq cannot update holidays file});
                                continue;
                        }
                        if  (!(monthd[curmon][curweek][currwday] & DAYSET_FLAG))  {
                                monthd[curmon][curweek][currwday] |= DAYSET_FLAG;
                                changes++;
                                displayyear(monthd, year);
                        }
                        continue;

                case  $K{btq key hols unset}:
                        if  (readonly)  {
                                doerror(stdscr, $E{btq cannot update holidays file});
                                continue;
                        }
                        if  (monthd[curmon][curweek][currwday] & DAYSET_FLAG)  {
                                monthd[curmon][curweek][currwday] &= ~DAYSET_FLAG;
                                changes++;
                                displayyear(monthd, year);
                        }
                        continue;

                case  $K{btq key hols toggle}:
                        if  (readonly)  {
                                doerror(stdscr, $E{btq cannot update holidays file});
                                continue;
                        }
                        monthd[curmon][curweek][currwday] ^= DAYSET_FLAG;
                        changes++;
                        displayyear(monthd, year);
                        continue;
                }
        }
}

void  viewhols(WINDOW *inwin)
{
        int     readonly, year, hfid, ret;
        time_t  now = time((time_t *) 0);
        struct  tm  *tp = localtime(&now);
        char    *fname = envprocess(HOLFILE);
        unsigned  char  yearmap[YVECSIZE];
        unsigned  char  monthd[12][5][7];

        year = tp->tm_year;

        if  (!Monthnames)  {
                Monthnames = galts(inwin, $Q{Months}, 12);
                monthnlen = altlen(Monthnames);
        }

        if  ((readonly = !ppermitted(BTM_WADMIN)))  {
                hfid = open(fname, O_RDONLY);
                free(fname);
                if  (hfid < 0)  {
                        doerror(inwin, $E{btq cannot read holidays file});
                        return;
                }
        }
        else  {
                USHORT  oldumask = umask(0);
                hfid = open(fname, O_RDWR|O_CREAT, 0644);
#ifndef HAVE_FCHOWN
                if  (Realuid == ROOTID)
                        Ignored_error = chown(fname, Daemuid, Realgid);
#endif
                umask(oldumask);
                free(fname);
                if  (hfid < 0)  {
                        doerror(inwin, $E{btq cannot create holidays file});
                        return;
                }
#ifdef  HAVE_FCHOWN
                if  (Realuid == ROOTID)
                        Ignored_error = fchown(hfid, Daemuid, Realgid);
#endif
        }

        lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
        if  (read(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))
                BLOCK_ZERO(yearmap, sizeof(yearmap));

        Ew = stdscr;
        select_state($S{btq holidays state});

        for  (;;)  {
                fillyear(yearmap, monthd, year);
                ret = edithols(monthd, year, readonly);
                if  (ret & CHANGE_MK)  {
                        readyear(yearmap, monthd, year);
                        lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
                        Ignored_error = write(hfid, (char *) yearmap, sizeof(yearmap));
                        ret &= ~CHANGE_MK;
                }
                if  (ret == 0)  {
                        close(hfid);
                        return;
                }
                if  (ret == PREV_MK)  {
                        if  (year > 90)
                                year--;
                }
                else  if  (year < 199)
                        year++;
                lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
                if  (read(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))
                        BLOCK_ZERO(yearmap, sizeof(yearmap));
        }
}
