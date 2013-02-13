/* timecon.h -- Time parameters

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

typedef struct  {
        time_t          tc_nexttime;            /* Next time */
        unsigned  char  tc_istime;      /* Time applies */
        unsigned  char  tc_mday;        /* Day of month (for monthb/e) */
        USHORT          tc_nvaldays;    /* Non-valid days bits (1 << day) */
        unsigned  char  tc_repeat;
#define TC_DELETE       0               /* Run once and then delete */
#define TC_RETAIN       1               /* Run once and leave on Q */
#define TC_MINUTES      2               /* Repeat rate - various ints */
#define TC_HOURS        3
#define TC_DAYS         4
#define TC_WEEKS        5
#define TC_MONTHSB      6               /* Months relative to beginning */
#define TC_MONTHSE      7               /* Months relative to end */
#define TC_YEARS        8
        unsigned  char  tc_nposs;       /* If not possible... */
#define TC_SKIP         0               /* Skip it, reschedule next etc */
#define TC_WAIT1        1               /* Wait but do not postpone others */
#define TC_WAITALL      2               /* Wait and postpone others */
#define TC_CATCHUP      3               /* Run one and catch up */
        ULONG   tc_rate;                /* Repeat interval */
}  Timecon, *TimeconRef;

typedef const   Timecon *CTimeconRef;

/* Non-valid day bits are 1 << day: Sun=0 Sat=6 as per localtime */

#define TC_NDAYS        8               /* Types of day */
#define TC_HOLIDAYBIT   (1 << 7)        /* Bit to mark holiday */
#define TC_ALLWEEKDAYS  ((1 << 7) - 1)  /* All weekdays */

/* Size of bitmap vector for holidays */

#define YVECSIZE        ((366 + 7) / 8)

extern time_t   advtime(CTimeconRef);
extern int      repmnthfix(TimeconRef);
extern char     month_days[];
