/* setkey.c -- set up key map definitions for Curses clients

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

/* Default amount of time to wait for keys.  */

#define DELAY_TENTHS    3

#include <sys/types.h>
#include <ctype.h>
#include <curses.h>
#ifdef  NO_TERMIO_IN_CURSES
#if     defined(OS_BSDI) || defined(OS_FREEBSD)
#include <termios.h>
#define TCGETA  TIOCGETA
#define TCSETA  TIOCSETA
#define termio  termios
#else
#include <termio.h>
#endif
#endif /* NO_TERMIO_IN_CURSES */
#ifdef  HAVE_TIGETSTR
#include <term.h>
#endif
#if     !defined(HAVE_TERMIO_H) && !defined(TIOCGETP)
#include <sgtty.h>
#endif
#ifdef  HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "keymap.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "errnums.h"

void  insert_global_key(const char *, const int, const int);
void  insert_state_key(const int, const char *, const int, const int);

#define CERR    (-2)
#define ERRK    (-3)

#define MAXDUFF 10
#define CTRLCH(X)       (X & 0x1f)

extern  int     keyerrors;
static  char    *tc_str, duff_str[MAXDUFF];
unsigned  char  Key_delay;

static int  getxdig()
{
        int     ch = getc(Cfile);

        switch  (ch)  {
        case '0':case '1':case '2':case '3':case '4':
        case '5':case '6':case '7':case '8':case '9':
                return  ch - '0';
        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                return  ch - 'a' + 10;
        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                return  ch - 'A' + 10;
        default:
                ungetc(ch, Cfile);
                return  -1;
        }
}

static int  getodig()
{
        int     ch = getc(Cfile);

        switch  (ch)  {
        case '0':case '1':case '2':case '3':case '4':
        case '5':case '6':case '7':
                return  ch - '0';
        default:
                ungetc(ch, Cfile);
                return  -1;
        }
}

static void  skipover(const char *text)
{
        int     ch, sch;

        while  (*text)  {
                ch = sch = getc(Cfile);
                if  (islower(ch))
                        ch += 'A' - 'a';
                if  (ch != *text)  {
                        ungetc(sch, Cfile);
                        return;
                }
                text++;
        }
}

#ifdef  HAVE_TIGETSTR

static int  get_ti_fkey(char *cap)
{
        char    *str;

        if  ((str = tigetstr(cap)) == (char *) 0 || str == (char *) -1)  {
                strncpy(duff_str, cap, MAXDUFF-1);
                return  ERRK;
        }

        tc_str = str;
        return  *tc_str++;
}

#define get_fkey(tiname, tcname)        get_ti_fkey(tiname)
#else

static  char    *tc_area;

static int  get_tc_fkey(char *cap)
{
#ifdef  OLDDYNIX
        /* Old Dynix chokes over tgetstr....  */
        strncpy(duff_str, cap, MAXDUFF-1);
        return  ERRK;
#else
        char    *str;
        extern  char    *tgetstr();

        if  ((str = tgetstr(cap, &tc_area)) == (char *) 0)  {
                strncpy(duff_str, cap, MAXDUFF-1);
                return  ERRK;
        }
        tc_str = str;
        return  *tc_str++;
#endif
}

#define get_fkey(tiname, tcname)        get_tc_fkey(tcname)
#endif

/*      Lookup for special key name.

        \kUP    string in up arrow key
        \kDOWN  string in down arrow key
        \kLEFT  string in left arrow key
        \kRIGHT string in right arrow key
        \kHOME  string in home key
        \kFnn   function key nn
        \kINTR  interrupt key (from termio on entry)
        \kKILL  kill key (ditto)
        \kERASE ERASE key (ditto)
        \kQUIT  quit key (ditto)  */

static int  klookup()
{
        int     ch;

        ch = getc(Cfile);

        switch  (ch)  {
        default:
                duff_str[0] = '\\';
                duff_str[1] = 'k';
                duff_str[2] = (char) ch;
                duff_str[3] = '\0';
                return  ERRK;

        case 'U':case 'u':
                skipover("P");
                return  get_fkey("kcuu1", "ku");

        case 'D':case 'd':
                skipover("OWN");
                return  get_fkey("kcud1", "kd");

        case 'L':case 'l':
                skipover("EFT");
                return  get_fkey("kcub1", "kl");

        case 'R':case 'r':
                skipover("IGHT");
                return  get_fkey("kcuf1", "kr");

        case 'H':case 'h':
                skipover("OME");
                return  get_fkey("khome", "kh");

        case 'F':case 'f':
                {
                        int     fn;
                        char    fkb[10];

                        ch = getc(Cfile);
                        if  (!isdigit(ch))
                                return  CERR;

                        fn = ch - '0';
                        for  (;;)  {
                                ch = getc(Cfile);
                                if  (!isdigit(ch))
                                        break;
                                fn = fn * 10 + ch - '0';
                        }
                        ungetc(ch, Cfile);
#ifdef  HAVE_TIGETSTR
                        sprintf(fkb, "kf%d", fn);
                        return  get_ti_fkey(fkb);
#else
                        if  (fn >= 10)  {
                                sprintf(duff_str, "k%d", fn);
                                return  ERRK;
                        }
                        sprintf(fkb, "k%d", fn);
                        return  get_tc_fkey(fkb);
#endif
                }

        case 'I':case 'i':
                skipover("NTR");
#ifdef  HAVE_TERMIO_H
                {
                        struct  termio  t;
                        ioctl(0, TCGETA, &t);
                        return  t.c_cc[VINTR];
                }
#else
                {
                        struct  tchars  t;
                        ioctl(0, TIOCGETC, &t);
                        return  t.t_intrc;
                }
#endif

        case 'K':case 'k':
                skipover("ILL");
#ifdef  HAVE_TERMIO_H
                {
                        struct  termio  t;
                        ioctl(0, TCGETA, &t);
                        return  t.c_cc[VKILL];
                }
#else
                {
                        struct  sgttyb  t;
                        ioctl(0, TIOCGETP, &t);
                        return  t.sg_kill;
                }
#endif

        case 'E':case 'e':
                skipover("RASE");
#ifdef  HAVE_TERMIO_H
                {
                        struct  termio  t;
                        ioctl(0, TCGETA, &t);
                        return  t.c_cc[VERASE];
                }
#else
                {
                        struct  sgttyb  t;
                        ioctl(0, TIOCGETP, &t);
                        return  t.sg_erase;
                }
#endif


        case 'Q':case 'q':
                skipover("UIT");
#ifdef  HAVE_TERMIO_H
                {
                        struct  termio  t;
                        ioctl(0, TCGETA, &t);
                        return  t.c_cc[VQUIT];
                }
#else
                {
                        struct  tchars  t;
                        ioctl(0, TIOCGETC, &t);
                        return  t.t_quitc;
                }
#endif
        }
}

/* Read a character delimited by a (non-escaped) ',', \n or EOF
   Interpret ^a etc as control-A etc \r \b \t \n \e have
   usual meaning, \xXX for hex chars \nnn for octal \\
   for \ \, for , \s for space.  Also provide: \kname as above.  */

static int  esc_getc()
{
        int     ch, res;

        if  (tc_str && *tc_str)
                return  *tc_str++;

        for  (;;)  {

                ch = getc(Cfile);

                switch  (ch)  {
                default:
                        if  (ch >= ' ' && ch <= '~')
                                return  ch;
                        return  CERR;

                case  '\n':
                case  ',':
                        ungetc(ch, Cfile);
                case  EOF:
                        return  EOF;

                case  '^':
                        ch = getc(Cfile);
                        if  (ch == '^'  ||  ch == ',')
                                return  ch;
                        if  (ch < ' ' || ch > '~')
                                return  CERR;
                        return  ch & 0x1f;

                case  '\\':
                        ch = getc(Cfile);
                        switch  (ch)  {
                        default:
                                return  CERR;

                        case  '\\':
                        case  ',':
                        case  '^':
                                return  ch;

                        case  '\n':
                                continue;

                        case  'n':case  'N':    return  '\n';
                        case  'r':case  'R':    return  '\r';
                        case  't':case  'T':    return  '\t';
                        case  'v':case  'V':    return  '\v';
                        case  's':case  'S':    return  ' ';
                        case  'b':case  'B':    return  CTRLCH('h');
                        case  'e':case  'E':    return  '\033';

                        case  'x':case  'X':
                                if  ((res = getxdig()) < 0)
                                        return  CERR;
                                if  ((ch = getxdig()) < 0)
                                        return  res;
                                return  (res << 4) + ch;

                        case  '0':case '1':case '2':case '3':
                        case  '4':case '5':case '6':case '7':

                                res = ch - '0';
                                while  ((ch = getodig()) >= 0)
                                        res = (res << 3) + ch;
                                return  res;

                        case  'k':case  'K':
                                return  klookup();
                        }
                }
        }
}

/* Remember the sequence to reset the "application cursor keys" */
static  char    *arrow_treset;

/* Yes that's right some stdio.h-es don't have putchar in */

static int  func_putchar(int ch)
{
        return  putc(ch, stdout);
}

/* Main routine to set up key sequences for various states from config file.  */

void  setupkeys()
{
        int     ch, minus, kvalue, kstate, length;
        char    *kd, tbuf[MAXTBUF];
        char    *arrow_tset;
#ifdef  HAVE_TIGETSTR
        setupterm((char *) 0, 2, (int *) 0);
#ifdef  TI_GNU_CC_BUG
        LINES = tigetnum("lines");
#endif
        if  ((arrow_tset = tigetstr("smkx")) == (char *) -1)
                arrow_tset = (char *) 0;
        if  ((arrow_treset = tigetstr("rmkx")) == (char *) -1)
                arrow_treset = (char *) 0;
#else
#ifndef OLDDYNIX
        char    tca[1024], resb[1024];

        tc_area = resb;
        tgetent(tca, getenv("TERM"));
        arrow_tset = tgetstr("ks", &tc_area);
        arrow_treset = tgetstr("ke", &tc_area);
#endif
#endif
        /* Set up application keypad if applicable */

        if  (arrow_treset)
                arrow_treset = stracpy(arrow_treset);
        if  (arrow_tset)
                tputs(arrow_tset, 1, func_putchar);

        /* Pass 1, set up global keys.  */

        fseek(Cfile, 0L, 0);

        for  (;;)  {
                ch = getc(Cfile);
                if  (ch == EOF)
                        break;

                if  (ch != 'K')  {
                skipn:
                        while  (ch != '\n'  &&  ch != EOF)
                                ch = getc(Cfile);
                        continue;
                }

                ch = getc(Cfile);
                if  (ch == '-')  {
                        minus = 1;
                        kvalue = 0;
                }
                else  {
                        if  (!isdigit(ch))
                                goto  skipn;
                        kvalue = ch - '0';
                        minus = 0;
                }
                for  (;;)  {
                        ch = getc(Cfile);
                        if  (!isdigit(ch))
                                break;
                        kvalue = kvalue * 10 + ch - '0';
                }
                if  (minus)
                        kvalue = -kvalue;
                if  (ch != ':')
                        goto  skipn;
                do  {
                        length = 0;
                        while  ((ch = esc_getc()) >= 0)  {
                                tbuf[length++] = (char) ch;
                                if  (length >= MAXTBUF)  {
                                        print_error($E{Key string too long});
                                        exit(E_BADCFILE);
                                }
                        }
                        if  (ch == CERR)  {
                                print_error($E{Key string bad syntax});
                                exit(E_BADCFILE);
                        }
                        if  (ch == ERRK)  {
                                LONG    pl = ftell(Cfile);

                                disp_str = duff_str;
                                print_error($E{Unknown key name});
                                fseek(Cfile, (long) pl, 0);

                                /* Reset in case halfway through function key.  */

                                tc_str = (char *) 0;
                                sleep(3);
                                goto  skipn;
                        }
                        insert_global_key(tbuf, length, kvalue);
                        ch = getc(Cfile);
                }  while  (ch == ',');
        }

        /* Pass 2, set up state specific keys.  */

        fseek(Cfile, 0L, 0);

        for  (;;)  {
                ch = getc(Cfile);
                if  (ch == EOF)
                        break;

                if  (ch == '-')  {
                        kstate = 0;
                        minus = 1;
                }
                else  {
                        if  (!isdigit(ch))  {
                        skipn2:
                                while  (ch != '\n'  &&  ch != EOF)
                                        ch = getc(Cfile);
                                continue;
                        }
                        kstate = ch - '0';
                        minus = 0;
                }
                for  (;;)  {
                        ch = getc(Cfile);
                        if  (!isdigit(ch))
                                break;
                        kstate = kstate * 10 + ch - '0';
                }
                if  (minus)
                        kstate = -kstate;
                if  (ch != 'K')
                        goto  skipn2;

                ch = getc(Cfile);
                if  (ch == '-')  {
                        minus = 1;
                        kvalue = 0;
                }
                else  {
                        if  (!isdigit(ch))
                                goto  skipn2;
                        kvalue = ch - '0';
                        minus = 0;
                }
                for  (;;)  {
                        ch = getc(Cfile);
                        if  (!isdigit(ch))
                                break;
                        kvalue = kvalue * 10 + ch - '0';
                }
                if  (minus)
                        kvalue = -kvalue;
                if  (ch != ':')
                        goto  skipn2;
                do  {
                        length = 0;
                        while  ((ch = esc_getc()) >= 0)  {
                                tbuf[length++] = (char) ch;
                                if  (length >= MAXTBUF)  {
                                        print_error($E{Key string too long});
                                        exit(E_BADCFILE);
                                }
                        }
                        if  (ch == CERR)  {
                                print_error($E{State key string bad syntax});
                                exit(E_BADCFILE);
                        }
                        if  (ch == ERRK)  {
                                disp_str = duff_str;
                                print_error($E{Unknown key name});
                                /* Reset in case halfway through function key.  */
                                tc_str = (char *) 0;
                                goto  skipn2;
                        }
                        insert_state_key(kstate, tbuf, length, kvalue);
                        ch = getc(Cfile);
                }  while  (ch == ',');
        }
        if  (keyerrors)  {
                disp_arg[0] = keyerrors;
                print_error($E{Key setup errors aborting});
                exit(E_BADCFILE);
        }

#if     defined(HAVE_TIGETSTR) && !defined(BUGGY_DELCURTERM)

        /* This routine is broken on some systems, notably Bull DPX/2
           If omitted, then the `setupterm' call's effects won't
           be deallocated.  */

        del_curterm(cur_term);
#endif
        /* Set up key delay to keep John Pinner happy.  Hi! If you're
           watching!!!!  */

        if  ((kd = getenv("KEYDELAY")) == (char *) 0  ||
             (Key_delay = atoi(kd)) == 0)
                Key_delay = DELAY_TENTHS;
}

void  endwinkeys()
{
        endwin();
        if  (arrow_treset)
                tputs(arrow_treset, 1, func_putchar);
}
