%{
/* atcover.y -- queue up gnubatch job like "at" does for "at"

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
static  char    rcsid1[] = "@(#) $Id: atcover.y,v 1.5 2009/02/22 12:49:34 toadwarble Exp $";
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"

#define SECSPERDAY      (24 * 3600)

char    *mailflag = "";
char    cmdbuf[120];
char    verbose, queuelet = 'a';
time_t  now;

char    *daynames[] =  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"  };
char    *monnames[] =  { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"  };

void  yyerror(char *msg)
{
        fprintf(stderr, "%s\n", msg);
}

void  split_time(struct tm *tp, time_t secs)
{
        secs += 30;
        tp->tm_sec = 0;
        secs /= 60;
        tp->tm_min = secs % 60;
        tp->tm_hour = secs / 60;
}

long  genperiod(long per, long num)
{
        struct  tm  *tp;
        struct  tm  orig;
        time_t  res;

        if  (per <= 3600)
                return  per * num;

        if  (per >= SECSPERDAY * 365)  {
                tp = localtime(&now);
                tp->tm_year++;
                return  mktime(tp) - now;
        }

        tp = localtime(&now);
        if  (per <= SECSPERDAY)
                tp->tm_mday += num;
        else  if  (per <= SECSPERDAY * 7)
                tp->tm_mday += num * 7;
        else
                tp->tm_mon += num;

        orig = *tp;
        tp->tm_mon++;
        res = mktime(tp);
        tp = localtime(&res);
        if  (tp->tm_isdst != orig.tm_isdst)
                orig.tm_isdst = tp->tm_isdst;
        return  mktime(&orig) - now;
}
%}

%token  NUMBER WEEKDAY MONTH TODAY TOMORROW NOW AM_PM SPECIALTIME PERIOD TIMEZONE NEXT
%start atspec
%%

atspec: timedatespec
        {  time_t  t = $1;
           struct  tm  *tp = localtime(&t);

           if  (verbose)
                   fprintf(stderr, "%s %s %2d %.2d:%.2d:%.2d %d\n",
                           daynames[tp->tm_wday], monnames[tp->tm_mon], tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, tp->tm_year + 1900);

           if  ($1 < time((time_t *) 0))  {
                   yyerror("Time not in future");
                   YYABORT;
           }

           sprintf(cmdbuf, "gbch-r%s -q \'at%c\' --verbose -T %.2d/%.2d/%.2d,%.2d:%.2d", mailflag, queuelet,
                   tp->tm_year % 100, tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min);
       };


timedatespec:   NOW optincrdecr
                {
                        $$ = now + $2;
                }
                |
                time optincrdecr
                {
                        struct  tm  *tp = localtime(&now);
                        struct  tm  orig = *tp;
                        split_time(&orig, $1);
                        $$ = mktime(&orig) + $2;
                }
                |
                time TIMEZONE optincrdecr
                {
                        struct  tm  *tp = localtime(&now);
                        long    eoff;
                        time_t  res;
#ifdef  HAVE_TM_ZONE
                        eoff = - tp->tm_gmtoff;
#else
                        eoff = timezone;
                        if  (tp->tm_isdst)
                                eoff -= 3600L;
#endif
                        split_time(tp, $1);

                        /* Logic is as follows:
                           Suppose local timezone was 2H East of Greenwich.
                           Then eoff should be set to -2H.
                           We made say 9:00 local time which is 7:00 GMT
                           We need to ADD 2 to it to make the required GMT
                           so we subtract eoff. Do the reverse for the time
                           zone we are going to. */

                        res = mktime(tp) - eoff + $2 * 3600;
                        if  (res < now)
                                res += SECSPERDAY;
                        $$ = res;
                }
                | time date optincrdecr
                {
                        time_t  t = $2;
                        struct  tm  *tp = localtime(&t);
                        struct  tm  orig = *tp;
                        split_time(&orig, $1);
                        $$ = mktime(&orig) + $3;
                }
                | time TIMEZONE date optincrdecr
                {
                        time_t  t = $3;
                        struct  tm  *tp = localtime(&t);
                        long    eoff;
                        split_time(tp, $1);
#ifdef  HAVE_TM_ZONE
                        eoff = - tp->tm_gmtoff;
#else
                        eoff = timezone;
                        if  (tp->tm_isdst)
                                eoff += 3600L;
#endif
                        $$ = mktime(tp) + $4 - eoff + $2 * 3600;
                }
                ;

time:           numerictime
                {
                        $$ = $1;
                }
                | numerictime AM_PM
                {
                        if  ($2 > 0  &&  $1 >= 12 * 3600)  {
                            yyerror("Bad time - must give 0 to 12 with AM/PM");
                            YYABORT;
                        }
                        $$ = $1 + $2;
                }
                | SPECIALTIME
                {
                        $$ = $1;
                }
                ;

numerictime:    NUMBER time_sep NUMBER
                {
                        if  ($1 > 23)  {
                                yyerror("Invalid hours");
                                YYABORT;
                        }
                        if  ($3 > 59)  {
                                yyerror("Invalid minutes");
                                YYABORT;
                        }
                        $$ = ($1 * 60 + $3) * 60;
                }
                ;

time_sep:       ':' | '\'' | ',' | '.' | 'h' ;

date:           MONTH daynum
                {
                        struct  tm  *tp = localtime(&now);
                        int  wmon = $1 - 1;
                        if  (tp->tm_mon > wmon  ||  (tp->tm_mon == wmon  &&  tp->tm_mday > $2))
                                tp->tm_year++;
                        tp->tm_mon = wmon;
                        tp->tm_mday = $2;
                        tp->tm_hour = 12;
                        tp->tm_min = tp->tm_sec = 0;
                        $$ = mktime(tp);
                }
        |
                MONTH daynum ',' yearnum
                {
                        struct  tm  tp;
                        tp.tm_hour = 12;
                        tp.tm_min = tp.tm_sec = tp.tm_isdst = 0;
                        tp.tm_year = $4;
                        tp.tm_mon = $1 - 1;
                        tp.tm_mday = $2;
                        $$ = mktime(&tp);
                }
        |
                WEEKDAY
                {
                        struct  tm  *tp = localtime(&now);
                        if  (tp->tm_wday >= $1)
                                tp->tm_mday += 7;
                        tp->tm_mday += $1 - tp->tm_wday;
                        tp->tm_hour = tp->tm_min = tp->tm_sec = 0;
                        $$ = mktime(tp);
                }
        |
                TODAY
                {
                        $$ = (now / SECSPERDAY) * SECSPERDAY;
                }
        |
                TOMORROW
                {
                        $$ = (now / SECSPERDAY) * SECSPERDAY + SECSPERDAY;
                }
        |
                daynum '-' monnum '-' yearnum
                {
                        struct  tm  tp;
                        tp.tm_hour = tp.tm_min = tp.tm_sec = tp.tm_isdst = 0;
                        tp.tm_year = $5;
                        tp.tm_mon = $3 - 1;
                        tp.tm_mday = $1;
                        $$ = mktime(&tp);
                }
        |
                daynum '.' monnum '.' yearnum
                {
                        struct  tm  tp;
                        tp.tm_hour = 12;
                        tp.tm_min = tp.tm_sec = tp.tm_isdst = 0;
                        tp.tm_year = $5;
                        tp.tm_mon = $3 - 1;
                        tp.tm_mday = $1;
                        $$ = mktime(&tp);
                }
        |
                daynum ',' monnum
                {
                        struct  tm  *tp = localtime(&now);
                        int  wmon = $3 - 1;
                        if  (tp->tm_mon > wmon  ||  (tp->tm_mon == wmon  &&  tp->tm_mday > $1))
                                tp->tm_year++;
                        tp->tm_mon = wmon;
                        tp->tm_mday = $1;
                        tp->tm_hour = tp->tm_min = tp->tm_sec = 0;
                        $$ = mktime(tp);
                }
        |
                daynum MONTH
                {
                        struct  tm  *tp = localtime(&now);
                        int  wmon = $2 - 1;
                        if  (tp->tm_mon > wmon  ||  (tp->tm_mon == wmon  &&  tp->tm_mday > $1))
                                tp->tm_year++;
                        tp->tm_mon = wmon;
                        tp->tm_mday = $1;
                        tp->tm_hour = 12;
                        tp->tm_min = tp->tm_sec = 0;
                        $$ = mktime(tp);
                }
        |
                daynum MONTH yearnum
                {
                        struct  tm  tp;
                        tp.tm_hour = 12;
                        tp.tm_min = tp.tm_sec = tp.tm_isdst = 0;
                        tp.tm_year = $3;
                        tp.tm_mon = $2 - 1;
                        tp.tm_mday = $1;
                        $$ = mktime(&tp);
                }
        |
                monnum '/' daynum '/' yearnum
                {
                        struct  tm  tp;
                        tp.tm_hour = 12;
                        tp.tm_min = tp.tm_sec = tp.tm_isdst = 0;
                        tp.tm_year = $5;
                        tp.tm_mon = $1 - 1;
                        tp.tm_mday = $3;
                        $$ = mktime(&tp);
                }
        ;

daynum:         NUMBER
                {
                        if  ($1 <= 0  ||  $1 > 31)  {
                                yyerror("Invalid day number");
                                YYABORT;
                        }
                        $$ = $1;
                }
                ;

monnum:         NUMBER
                {
                        if  ($1 <= 0  ||  $1 > 12)  {
                                yyerror("Invalid month number");
                                YYABORT;
                        }
                        $$ = $1;
                }
                ;

yearnum:        NUMBER
                {
                        if  ($1 > 2000)  {
                                if  ($1 > 20037)  {
                                        yyerror("Invalid year number");
                                        YYABORT;
                                }
                                $$ = $1 - 1900;
                        }
                        else  {
                                if  ($1 <= 0  ||  $1 > 37)  {
                                        yyerror("Invalid year number");
                                        YYABORT;
                                }
                                $$ = $1 + 100;
                        }
                }
                ;

optincrdecr:
                /* empty */
                {
                        $$ = 0;
                }
        |
                '+' NUMBER PERIOD
                {
                        $$ = genperiod($3, $2);
                }
        |
                '-' NUMBER PERIOD
                {
                        $$ = -$2 * $3;
                }
        |
                NEXT PERIOD
                {
                        $$ = genperiod($2, 1);
                }
        |
                NEXT WEEKDAY
                {
                        struct  tm  *tp = localtime(&now);
                        time_t  res;
                        if  (tp->tm_wday >= $2)
                                tp->tm_mday += 7;
                        tp->tm_mday += $2 - tp->tm_wday;
                        res = mktime(tp);
                        $$ = res - now;
                }
                ;
%%

char    **yyalist, *yylarg;
int     yylap, yylahead;

int     yylgetch()
{
        if  (yylahead)  {
                int     ret = yylahead;
                yylahead = 0;
                return  ret;
        }
        if  (yylarg[yylap] != '\0')
                return  yylarg[yylap++];
        yylarg = *++yyalist;
        if  (!yylarg)
                return  EOF;
        yylap = 0;
        return  ' ';
}

int     yylex()
{
        int     ch, cnt;
        char    nbuf[20];

        do  ch = yylgetch();
        while  (ch == ' ');

        if  (isalpha(ch))  {
                cnt = 1;
                nbuf[0] = tolower(ch);
                for  (;;)  {
                        ch = yylgetch();
                        if  (!isalpha(ch))
                                break;
                        if  (cnt < 20)
                                nbuf[cnt++] = tolower(ch);
                }
                nbuf[cnt] = '\0';
                yylahead = ch;
                switch  (nbuf[0])  {
                case  'a':
                        if  (strcmp(nbuf, "am") == 0)  {
                                yylval = 0;
                                return  AM_PM;
                        }
                        if  (strcmp(nbuf, "apr") == 0)  {
                                yylval = 4;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "april") == 0)  {
                                yylval = 4;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "aug") == 0)  {
                                yylval = 8;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "august") == 0)  {
                                yylval = 8;
                                return  MONTH;
                        }
                case  'b':
                        if  (strcmp(nbuf, "bst") == 0)  {
                                yylval = -1;
                                return  TIMEZONE;
                        }
                case  'd':
                        if  (strcmp(nbuf, "day") == 0)  {
                                yylval = SECSPERDAY;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "days") == 0)  {
                                yylval = SECSPERDAY;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "dec") == 0)  {
                                yylval = 12;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "december") == 0)  {
                                yylval = 12;
                                return  MONTH;
                        }
                case  'f':
                        if  (strcmp(nbuf, "friday") == 0)  {
                                yylval = 5;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "fri") == 0)  {
                                yylval = 5;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "feb") == 0)  {
                                yylval = 2;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "february") == 0)  {
                                yylval = 2;
                                return  MONTH;
                        }
                case  'g':
                        if  (strcmp(nbuf, "gmt") == 0)  {
                                yylval = 0;
                                return  TIMEZONE;
                        }
                case  'h':
                        if  (strcmp(nbuf, "h") == 0)
                                return  'h';
                        if  (strcmp(nbuf, "hour") == 0)  {
                                yylval = 3600;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "hours") == 0)  {
                                yylval = 3600;
                                return  PERIOD;
                        }
                case  'j':
                        if  (strcmp(nbuf, "jan") == 0)  {
                                yylval = 1;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "january") == 0)  {
                                yylval = 1;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "jun") == 0)  {
                                yylval = 6;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "june") == 0)  {
                                yylval = 6;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "jul") == 0)  {
                                yylval = 7;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "july") == 0)  {
                                yylval = 7;
                                return  MONTH;
                        }
                case  'm':
                        if  (strcmp(nbuf, "minute") == 0)  {
                                yylval = 60;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "min") == 0)  {
                                yylval = 60;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "minutes") == 0)  {
                                yylval = 60;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "mins") == 0)  {
                                yylval = 60;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "month") == 0)  {
                                yylval = 30 * SECSPERDAY;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "months") == 0)  {
                                yylval = 30 * SECSPERDAY;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "mar") == 0)  {
                                yylval = 3;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "march") == 0)  {
                                yylval = 3;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "may") == 0)  {
                                yylval = 5;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "midday") == 0)  {
                                yylval = SECSPERDAY / 2;
                                return  SPECIALTIME;
                        }
                        if  (strcmp(nbuf, "midnight") == 0)  {
                                yylval = 0;
                                return  SPECIALTIME;
                        }
                        if  (strcmp(nbuf, "monday") == 0)  {
                                yylval = 1;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "mon") == 0)  {
                                yylval = 1;
                                return  WEEKDAY;
                        }
                case  'n':
                        if  (strcmp(nbuf, "next") == 0)
                                return  NEXT;
                        if  (strcmp(nbuf, "now") == 0)
                                return  NOW;
                        if  (strcmp(nbuf, "noon") == 0)  {
                                yylval = SECSPERDAY / 2;
                                return  SPECIALTIME;
                        }
                        if  (strcmp(nbuf, "nov") == 0)  {
                                yylval = 11;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "november") == 0)  {
                                yylval = 11;
                                return  MONTH;
                        }
                case  'o':
                        if  (strcmp(nbuf, "oct") == 0)  {
                                yylval = 10;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "october") == 0)  {
                                yylval = 10;
                                return  MONTH;
                        }
                case  'p':
                        if  (strcmp(nbuf, "pm") == 0)  {
                                yylval = SECSPERDAY/2;
                                return  AM_PM;
                        }
                case  's':
                        if  (strcmp(nbuf, "sunday") == 0)  {
                                yylval = 0;
                                return  WEEKDAY;
                                }
                        if  (strcmp(nbuf, "sun") == 0)  {
                                yylval = 0;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "saturday") == 0)  {
                                yylval = 6;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "sat") == 0)  {
                                yylval = 6;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "sep") == 0)  {
                                yylval = 9;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "sept") == 0)  {
                                yylval = 9;
                                return  MONTH;
                        }
                        if  (strcmp(nbuf, "september") == 0)  {
                                yylval = 9;
                                return  MONTH;
                        }
                case  't':
                        if  (strcmp(nbuf, "today") == 0)
                                return  TODAY;
                        if  (strcmp(nbuf, "tomorrow") == 0)
                                return  TOMORROW;
                        if  (strcmp(nbuf, "tuesday") == 0)  {
                                yylval = 2;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "thursday") == 0)  {
                                yylval = 4;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "tues") == 0)  {
                                yylval = 2;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "thurs") == 0)  {
                                yylval = 4;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "tue") == 0)  {
                                yylval = 2;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "thur") == 0)  {
                                yylval = 4;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "teatime") == 0)  {
                                yylval = 16 * 3600;
                                return  SPECIALTIME;
                        }
                case  'u':
                        if  (strcmp(nbuf, "utc") == 0)  {
                                yylval = 0;
                                return  TIMEZONE;
                        }
                case  'w':
                        if  (strcmp(nbuf, "week") == 0)  {
                                yylval = SECSPERDAY * 7;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "weeks") == 0)  {
                                yylval = SECSPERDAY * 7;
                                return  PERIOD;
                        }
                        if  (strcmp(nbuf, "wednesday") == 0)  {
                                yylval = 3;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "wed") == 0)  {
                                yylval = 3;
                                return  WEEKDAY;
                        }
                        if  (strcmp(nbuf, "weds") == 0)  {
                                yylval = 3;
                                return  WEEKDAY;
                        }
                case  'y':
                        if  (strcmp(nbuf, "year") == 0)  {
                                yylval = SECSPERDAY * 365;
                                return  PERIOD;
                        }

                        if  (strcmp(nbuf, "years") == 0)  {
                                yylval = SECSPERDAY * 365;
                                return  PERIOD;
                        }
                }
                return  '%';
        }
        if  (isdigit(ch))  {
                cnt = 1;
                nbuf[0] = ch;
                for  (;;)  {
                        ch = yylgetch();
                        if  (!isdigit(ch))
                                break;
                        if  (cnt < 20)
                                nbuf[cnt++] = ch;
                }
                nbuf[cnt] = '\0';
                yylahead = ch;
                yylval = atoi(nbuf);
                return  NUMBER;
        }
        return  ch;
}

void  execrest(const char *prog, const char *arg1, char **args)
{
        unsigned  n = 3;
        char  **ap = args, **argl;
        while  (*ap)  {
                n++;
                ap++;
        }
        ap = argl = (char **) malloc(n);
        if  (!ap)
                exit(255);
        *ap++ = (char *) prog;
        *ap++ = (char *) arg1;
        while  (*args)
                *ap++ = *args++;
        *ap++ = 0;
        execvp(prog, argl);
        exit(255);
}

MAINFN_TYPE  main(int argc, char **argv)
{
        extern  int     optind;
        extern  char    *optarg;
        char    *progname;
        int     ch;
        int     catjobs = 0, isdel = 0, islist = 0, isbatch = 0, listq = 0;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        /* Generate --help and --version messages */

        if  (argv[1])  {
                if  (strcmp(argv[1], "--help") == 0)  {
                        fputs("This is a program intended to replace the functionality of at(1) using GNUbatch\n"
                              "Please see at(1) for details of arguments\n", stdout);
                        return  0;
                }
                if  (strcmp(argv[1], "--version") == 0)  {
                        printf("This is %s, a component of GNUbatch version %s\n", progname, GNU_BATCH_VERSION_STRING);
                        fputs("Copyright (C) 2009 Free Software Foundation, Inc.\n"
                              "This is free software; see the source for copying conditions.  There is NO\n"
                              "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n", stdout);
                        return  0;
                }
        }

        if  (strcmp(progname, "atq") == 0)
                islist++;
        else  if  (strcmp(progname, "atrm") == 0)
                isdel++;
        else  if  (strcmp(progname, "batch") == 0)
                isbatch++;

        while  ((ch = getopt(argc, argv, "cdf:lmq:vV")) != EOF)
                switch  (ch)  {
                case  '?':
                        fprintf(stderr, "Usage: %s [-cdlmvV] [-q queue] [-f file] args... \n", progname);
                        return  1;
                case  'c':
                        catjobs++;
                        break;
                case  'd':
                        isdel++;
                        break;
                case  'f':
                        if  (!freopen(optarg, "r", stdin))  {
                                fprintf(stderr, "%s: Cannot open %s\n", progname, optarg);
                                return  2;
                        }
                        break;
                case  'l':
                        islist++;
                        break;
                case  'm':
                        mailflag = " -m";
                        break;
                case  'q':
                        listq++;
                        if  (!isalpha(optarg[0]))  {
                                fprintf(stderr, "Queue name not alphabetic\n");
                                return  3;
                        }
                        queuelet = tolower(optarg[0]);
                        break;
                case  'v':
                        verbose++;
                        break;
                case  'V':
                {
                        char    *cp;
                        for  (cp = rcsid1;  *cp  &&  *cp != '$';  cp++)
                                ;
                        if  (!*cp)
                                break;
                        for  (cp++;  *cp  &&  *cp != '$';  cp++)
                                putc(*cp, stderr);
                        putc('\n', stderr);
                        break;
                }
                }

        if  (islist)  {
                int     uid = getuid();
                char    restru[30];
                restru[0] = '\0';
                if  (uid != 0)  {
                        struct  passwd  *pw = getpwuid(uid);
                        if  (pw)
                                sprintf(restru, " -u %s", pw->pw_name);
                }
                sprintf(cmdbuf, "gbch-jlist -NLsZ%s -q \'at%c\' -F \'%%N %%T %%q %%U\'|sed -e \'s/at\\([a-z]\\)/\\1/\'", restru, listq? queuelet: '?');
                return  system(cmdbuf) >> 8;
        }
        else  if  (catjobs)
                execrest("gbch-jlist", "-V", &argv[optind]);
        else  if  (isdel)
                execrest("gbch-jdel", "-yd", &argv[optind]);
        else  if  (isbatch)  {
                if  (argv[optind])  {
                        fprintf(stderr, "%s: not expecting any more arguments\n", progname);
                        return  3;
                }
                sprintf(cmdbuf, "gbch-r --verbose -q \'at%c\' %s", queuelet, mailflag);
                return  system(cmdbuf) >> 8;
        }

        now = time(0);

        yyalist = &argv[optind];
        if  (!(yylarg = yyalist[0]))  {
                fprintf(stderr, "%s: Missing argument(s)\n", progname);
                return  4;
        }
        if  (yyparse())
                return 10;
        else
                return  system(cmdbuf) >> 8;
}
