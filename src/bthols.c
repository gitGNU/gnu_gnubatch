/* bthols.c -- main module for gbch-hols

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
#include <stdio.h>
#include <ctype.h>
#ifdef  OS_LINUX                        /* Need this for umask decln */
#include <sys/stat.h>
#endif
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include "incl_ugid.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpargs.h"
#include "helpalt.h"
#include "optflags.h"
#include "btuser.h"
#include "btmode.h"
#include "timecon.h"
#include "cfile.h"
#include "files.h"

enum  { DISPLAY, SETHOLS } Action = DISPLAY;
char    Clearex = 0;

static  HelpaltRef Monthnames, Monthshort;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

OPTION(o_explain)
{
        print_error($E{bthols explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_clear)
{
        Clearex = 1;
        return  OPTRESULT_OK;
}

OPTION(o_noclear)
{
        Clearex = 0;
        return  OPTRESULT_OK;
}

OPTION(o_display)
{
        Action = DISPLAY;
        return  OPTRESULT_OK;
}

OPTION(o_set)
{
        Action = SETHOLS;
        return  OPTRESULT_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{bthols arg explain} },
  {  'C', $A{bthols arg clear} },
  {  'r', $A{bthols arg noclear} },
  {  'd', $A{bthols arg display} },
  {  's', $A{bthols arg set} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_clear,        o_noclear,      o_display,
o_set,
o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont;

        fprintf(dest, "%s", name);
        cancont = spitoption(Action == SETHOLS? $A{bthols arg set}: $A{bthols arg display}, $A{bthols arg explain}, dest, '=', 0);
        cancont = spitoption(Clearex? $A{bthols arg clear}: $A{bthols arg noclear}, $A{bthols arg explain}, dest, ' ', cancont);
        putc('\n', dest);
}

#define ISHOL(map, day) map[(day) >> 3] & (1 << (day & 7))
#define SETHOL(map, day)        map[(day) >> 3] |= (1 << (day & 7))

void  fillmonths(unsigned char *yearmap, ULONG *monthd, const int yrafter90)
{
        int  jday, mon = 0, mday = 0, lastj = 365;
        month_days[1] = 28;
        if  (yrafter90 % 4 == 2)  {
                month_days[1] = 29;
                lastj++;
        }
        BLOCK_ZERO(monthd, 12 * sizeof(ULONG));
        for  (jday = 0;  jday < lastj;  jday++)  {
                mday++;
                if  (mday > month_days[mon])  {
                        mon++;
                        mday = 1;
                }
                if  (ISHOL(yearmap, jday))
                        monthd[mon] |= 1 << mday;
        }
}

void  fillmap(unsigned char *yearmap, ULONG *monthd, const int yrafter90)
{
        int  jday, mon = 0, mday = 0, lastj = 365;
        if  (yrafter90 % 4 == 2)
                lastj++;
        for  (jday = 0;  jday < lastj;  jday++)  {
                mday++;
                if  (mday > month_days[mon])  {
                        mon++;
                        mday = 1;
                }
                if  (monthd[mon] & (1 << mday))
                        SETHOL(yearmap, jday);
        }
}

int  lookup_mon(const char *mnam)
{
        int     cnt;
        for  (cnt = 0;  cnt < Monthnames->numalt;  cnt++)
                if  (ncstrcmp(mnam, Monthnames->list[cnt]) == 0)
                        return  cnt;
        for  (cnt = 0;  cnt < Monthshort->numalt;  cnt++)
                if  (ncstrcmp(mnam, Monthshort->list[cnt]) == 0)
                        return  cnt;
        return  -1;
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        char    *Curr_pwd = (char *) 0;
        int     whichyear, hfid;
        char    *fname;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        argv = optprocess(argv, Adefs, optprocs, $A{bthols arg explain}, $A{bthols arg freeze home}, 0);

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  (argv[0] == (char *) 0)  {
                print_error($E{Bthols missing args});
                return  E_USAGE;
        }
        if  (!isdigit(argv[0][0]))  {
                print_error($E{Bthols bad arg});
                return  E_USAGE;
        }
        whichyear = atoi(argv[0]);
        if  (whichyear > 100)
                whichyear -= 1900;
        if  (whichyear >= 90)
                whichyear -= 90;
        else
                whichyear += 10;
        if  (whichyear < 0  ||  whichyear >= 48)  {
                disp_arg[0] = atoi(argv[0]);
                print_error($E{Bthols bad year});
                return  E_USAGE;
        }
        argv++;

        if  (argv[0])  {
                disp_str = argv[0];
                if  (Action == SETHOLS)  {
                        if  (!(freopen(argv[0], "r", stdin)))  {
                                print_error($E{Bthols cannot read});
                                return  E_PERM;
                        }
                }
                else  if  (!(freopen(argv[0], "w", stdout)))  {
                        print_error($E{Bthols cannot write});
                        return  E_PERM;
                }
        }

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);
        if  (Action == SETHOLS)  {
                mypriv = getbtuser(Realuid);
                if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                        print_error($E{btq cannot update holidays file});
                        return  E_NOPRIV;
                }
        }

        if  (!(Monthnames = helprdalt($Q{Months full})))  {
                disp_arg[9] = $Q{Months full};
                print_error($E{Missing alternative code});
                return  E_SETUP;
        }
        if  (!(Monthshort = helprdalt($Q{Months})))  {
                disp_arg[9] = $Q{Months};
                print_error($E{Missing alternative code});
                return  E_SETUP;
        }

        fname = envprocess(HOLFILE);

        if  (Action == SETHOLS)  {
                int     errors = 0;
                unsigned  char  yearmap[YVECSIZE];
                ULONG   monthbits[12];
                char    inbuf[200];
                USHORT  oldumask = umask(0);
                hfid = open(fname, O_RDWR|O_CREAT, 0644);
                umask(oldumask);
                free(fname);
                if  (hfid < 0)  {
                        print_error($E{btq cannot create holidays file});
                        return  E_SETUP;
                }
                if  (Clearex)  {
                        BLOCK_ZERO(yearmap, sizeof(yearmap));
                        BLOCK_ZERO(monthbits, sizeof(monthbits));
                        month_days[1] = 28;
                        if  (whichyear % 4 == 2)
                                month_days[1] = 29;
                }
                else  {
                        lseek(hfid, (long) (whichyear * YVECSIZE), 0);
                        if  (read(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))
                                BLOCK_ZERO(yearmap, sizeof(yearmap));
                        fillmonths(yearmap, monthbits, whichyear);
                }
        cont:
                while  (fgets(inbuf, sizeof(inbuf), stdin))  {
                        int     month;
                        char    *cp = strchr(inbuf, ':');
                        if  (!cp)  {
                                disp_str = inbuf;
                                print_error($E{Bthols format error});
                                errors++;
                                continue;
                        }
                        *cp = '\0';
                        if  ((month = lookup_mon(inbuf)) < 0)  {
                                disp_str = inbuf;
                                print_error($E{Bthols invalid month});
                                errors++;
                                continue;
                        }
                        cp++;
                        while  (*cp  &&  *cp != '\n')  {
                                int     day = 0;
                                while  (isspace(*cp))
                                        cp++;
                                if  (isdigit(*cp))  {
                                        do   day = day * 10 + *cp++ - '0';
                                        while  (isdigit(*cp));
                                }
                                if  (day <= 0  ||  day > month_days[month])  {
                                        disp_arg[0] = day;
                                        disp_str = inbuf;
                                        print_error($E{Bthols invalid day in month});
                                        errors++;
                                        goto  cont;
                                }
                                monthbits[month] |= 1 << day;
                        }
                }
                if  (errors > 0)
                        return  E_USAGE;

                fillmap(yearmap, monthbits, whichyear);
                lseek(hfid, (long) (whichyear * YVECSIZE), 0);
                if  (write(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))  {
                        print_error($E{Bthols write error});
                        return  E_SETUP;
                }
                close(hfid);
        }
        else  {
                int     month;
                unsigned  char  yearmap[YVECSIZE];
                ULONG   monthbits[12];
                hfid = open(fname, O_RDONLY);
                free(fname);
                if  (hfid < 0)  {
                        print_error($E{btq cannot read holidays file});
                        return  E_SETUP;
                }
                lseek(hfid, (long) (whichyear * YVECSIZE), 0);
                if  (read(hfid, (char *) yearmap, sizeof(yearmap)) != sizeof(yearmap))
                        BLOCK_ZERO(yearmap, sizeof(yearmap));
                close(hfid);
                fillmonths(yearmap, monthbits, whichyear);
                for  (month = 0;  month < 12;  month++)
                        if  (monthbits[month])  {
                                int     day;
                                printf("%s:", disp_alt(month, Monthnames));
                                for  (day = 1;  day <= 31;  day++)
                                        if  (monthbits[month] & (1 << day))
                                                printf(" %d", day);
                                putchar('\n');
                        }
        }

        return  0;
}
