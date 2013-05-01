/* btu_propts.c -- program options for gbch-user

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
#include <sys/types.h>
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"
#include "magic_ch.h"
#include "ecodes.h"
#include "helpargs.h"
#include "errnums.h"
#include "statenums.h"
#include "optflags.h"

void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);

#ifndef HAVE_ATEXIT
void  exit_cleanup();
#endif

extern  WINDOW  *escr,
                *hlpscr,
                *Ew;

#define SRT_NONE        0       /* Sort by numeric uid (default) */
#define SRT_USER        1       /* Sort by user name */
#define SRT_GROUP       2       /* Sort by group name */

#define BTU_DISP        0       /* Just display stuff */
#define BTU_UPERM       1       /* Set user modes */
#define BTU_UREAD       2       /* Read users */
#define BTU_UUPD        3       /* Update users */

extern  char    getflags,
                alphsort;

extern  char    *Curr_pwd;

#define NUMPROMPTS      4

struct  ltab    {
        int     helpcode;
        char    *message;
        char    *prompts[NUMPROMPTS];
        char    row, col, size;
        void    (*dfn)(struct ltab *);
        int     (*fn)(struct ltab *);
};

static void  prd_mode(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, lt->prompts[(int) getflags]);
}

static void  prd_sort(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, lt->prompts[(int) alphsort]);
}

static void  prd_helpclr(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_HELPCLR? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_helpbox(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_HELPBOX? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_errbox(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_ERRBOX? lt->prompts[1]: lt->prompts[0]);
}

static int  pro_bool(struct ltab *lt, unsigned *b)
{
        int     ch;
        unsigned  origb = *b;

        for  (;;)  {
                move(lt->row, lt->col);
                refresh();
                do  ch = getkey(MAG_A|MAG_P);
                while  (ch == EOF  &&  (hlpscr || escr));
                if  (hlpscr)  {
                        endhe(stdscr, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(stdscr, &escr);

                switch  (ch)  {
                default:
                        doerror(stdscr, $E{Screen opts unknown command});

                case  EOF:
                        continue;

                case  $K{key help}:
                        dochelp(stdscr, lt->helpcode * 10);
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        refresh();
                        continue;

                case  $K{key guess}:
                        ++*b;
                        if  (*b >= NUMPROMPTS  ||  lt->prompts[*b] == (char *) 0)
                                *b = 0;
                        clrtoeol();
                        addstr(lt->prompts[*b]);
                        continue;

                case  $K{key erase}:
                        *b = origb;

                case  $K{key halt}:
                case  $K{key cursor down}:
                case  $K{key eol}:
                case  $K{key cursor up}:
                        return  ch;
                }
        }
}

static int  pro_mode(struct ltab *lt)
{
        unsigned  current = getflags;
        int     ch = pro_bool(lt, &current);

        if  (current != getflags)
                getflags = (char) current;
        return  ch;
}

static int  pro_sort(struct ltab *lt)
{
        unsigned  current = alphsort;
        int     ch = pro_bool(lt, &current);

        if  (current != alphsort)
                alphsort = (char) current;
        return  ch;
}

static int  pro_bitflag(struct ltab *lt, const ULONG bitf)
{
        unsigned  current = Dispflags & bitf? 1: 0;
        unsigned  prev = current;
        int     ch = pro_bool(lt, &current);
        if  (current != prev)
                Dispflags ^= bitf;
        return  ch;
}

static int  pro_helpclr(struct ltab *lt)
{
        return  pro_bitflag(lt, DF_HELPCLR);
}

static int  pro_helpbox(struct ltab *lt)
{
        return  pro_bitflag(lt, DF_HELPBOX);
}

static int  pro_errbox(struct ltab *lt)
{
        return  pro_bitflag(lt, DF_ERRBOX);
}

#define NULLCH  (char *) 0

static  struct  ltab  ltab[] = {
        { $PHN{btuser opt mode}, NULLCH, { NULLCH, NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_mode, pro_mode },
        { $PHN{btuser opt sort}, NULLCH, { NULLCH, NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_sort, pro_sort },
        { $PHN{btuser opt helpclr}, NULLCH, { NULLCH, NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_helpclr, pro_helpclr },
        { $PHN{btuser opt helpbox}, NULLCH, { NULLCH, NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_helpbox, pro_helpbox },
        { $PHN{btuser opt errbox}, NULLCH, { NULLCH, NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_errbox, pro_errbox }
};

#define TABNUM  (sizeof(ltab)/sizeof(struct ltab))

static  struct  ltab    *lptrs[TABNUM];
static  int     comeinat;
static  char    **title;

static void  initnames()
{
        struct  ltab    *lt;
        int     i, j, hrows, cols, look4, rowstart, pstart, nextstate[TABNUM];

        title = helphdr('=');
        count_hv(title, &hrows, &cols);

        /* Slurp up standard messages */

        if  ((rowstart = helpnstate($N{btuser opt start row})) <= 0)
                rowstart = $N{btuser opt mode};
        if  ((pstart = helpnstate($N{btuser opt init cursor})) <= 0)
                pstart = rowstart;

        for  (i = 0, lt = &ltab[0]; lt < &ltab[TABNUM]; i++, lt++)  {
                lt->message = gprompt(lt->helpcode);
                lt->col = strlen(lt->message) + 1;
        }

        /* Do this in a second loop to avoid seeking back to beginning */

        for  (i = 0; i < TABNUM; i++)
                nextstate[i] = helpnstate(ltab[i].helpcode);

        /* Do this in a third loop...  */

        for  (i = 0;  i < TABNUM;  i++)  {
                int     j;
                for  (j = 0;  j < NUMPROMPTS;  j++)  {
                        char    *pr = helpprmpt(ltab[i].helpcode * 10 + j);
                        if  (!pr)
                                break;
                        ltab[i].prompts[j] = pr;
                }
        }

        i = 0;
        look4 = rowstart;

        for  (;;)  {
                for  (j = 0;  j < TABNUM;  j++)
                        if  (ltab[j].helpcode == look4)  {
                                lt = &ltab[j];
                                if  (lt->helpcode == pstart)
                                        comeinat = i;
                                lptrs[i] = lt;
                                lt->row = hrows + i;
                                i++;
                                look4 = nextstate[j];
                                goto  dun;
                        }
                disp_arg[9] = look4;
                doerror(stdscr, $E{Missing state code});
                refresh();
                do  i = getkey(MAG_A|MAG_P);
                while   (i == EOF);
#ifndef HAVE_ATEXIT
                exit_cleanup();
#endif
                exit(E_BADCFILE);
        dun:
                if  (look4 < 0)  {
                        if  (i != TABNUM)  {
                                disp_arg[9] = i;
                                doerror(stdscr, $E{Scrambled state code});
                                refresh();
                                do  i = getkey(MAG_A|MAG_P);
                                while   (i == EOF);
#ifndef HAVE_ATEXIT
                                exit_cleanup();
#endif
                                exit(E_BADCFILE);
                        }
                        break;
                }
        }
}

static int  askyorn(int code)
{
        char    *prompt = gprompt(code);
        int     ch;

        select_state($S{Saveopts yorn state});
        clear();
        mvaddstr(LINES/2, 0, prompt);
        addch(' ');
        refresh();
        if  (escr)  {
                touchwin(escr);
                wrefresh(escr);
                refresh();
        }
        for  (;;)  {
                do  ch = getkey(MAG_A|MAG_P);
                while  (ch == EOF  &&  (hlpscr || escr));
                if  (hlpscr)  {
                        endhe(stdscr, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(stdscr, &escr);

                switch  (ch)  {
                default:
                        doerror(stdscr, $E{Saveopts yorn state});

                case  EOF:
                        continue;
                case  $K{key help}:
                        dochelp(stdscr, code);
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        refresh();
                        continue;

                case  $K{key yorn yes}:
                        clear();
                        refresh();
                        return  1;

                case  $K{key yorn no}:
                        clear();
                        refresh();
                        return  0;
                }
        }
}

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0;
        fprintf(dest, "%s", name);

        cancont = spitoption(getflags == BTU_DISP? $A{btuser arg display}:
                             getflags == BTU_UPERM? $A{btuser arg setdef mode} :
                             getflags == BTU_UREAD? $A{btuser arg view users}:
                             $A{btuser arg update users}, $A{btuser arg explain}, dest, '=', cancont);
        cancont = spitoption(alphsort == SRT_USER? $A{btuser arg sort user}:
                             alphsort == SRT_GROUP? $A{btuser arg sort group}:
                             $A{btuser arg sort uid}, $A{btuser arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_HELPCLR? $A{btuser arg losechar}: $A{btuser arg keepchar}, $A{btuser arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_HELPBOX? $A{btuser arg help box}: $A{btuser arg no help box}, $A{btuser arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_ERRBOX? $A{btuser arg error box}: $A{btuser arg no error box}, $A{btuser arg explain}, dest, ' ', cancont);
        putc('\n', dest);
}

static void  ask_build()
{
        int     ret, i;
        static  char    btuser[] = "BTUSER";

        if  (!askyorn($PH{Save parameters}))
                return;

        disp_str = Curr_pwd;
        if  (askyorn($PH{Save in current directory}))  {
                if  ((ret = proc_save_opts(Curr_pwd, btuser, spit_options)) == 0)
                        return;
                doerror(stdscr, ret);
        }
        else  {
                if  (!askyorn($PH{Save in home directory}))
                        return;
                if  ((ret = proc_save_opts((char *) 0, btuser, spit_options)) == 0)
                        return;
                disp_str = "(Home)";
                doerror(stdscr, ret);
        }
        do  i = getkey(MAG_A|MAG_P);
        while   (i == EOF);
}

/* This accepts input from the screen.  */

int  propts()
{
        int     ch, i, whichel;
        struct  ltab    *lt;
        char    **hv;
        static  char    doneinit = 0;

        if  (!doneinit)  {
                doneinit = 1;
                initnames();
        }

        Ew = stdscr;
        whichel = comeinat;
        clear();
        if  (title)  for  (hv = title, i = 0;  *hv;  i++, hv++)
                mvwhdrstr(stdscr, i, 0, *hv);
        for  (i = 0;  i < TABNUM;  i++)  {
                lt = lptrs[i];
                mvaddstr(lt->row, 0, lt->message);
                (*lt->dfn)(lt);
        }
#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        refresh();
        reset_state();

        while  (whichel < TABNUM)  {
                lt = lptrs[whichel];
                ch = (*lt->fn)(lt);
                switch  (ch)  {
                default:
                case  $K{key eol}:
                case  $K{key cursor down}:
                        whichel++;
                        break;
                case  $K{key erase}:
                case  $K{key cursor up}:
                        if  (--whichel < 0)
                                whichel = 0;
                        break;
                case  $K{key halt}:
                        goto  ret;
                }
        }
 ret:
        ask_build();
        if  (escr)  {
                delwin(escr);
                escr = (WINDOW *) 0;
        }
        if  (hlpscr)  {
                delwin(hlpscr);
                hlpscr = (WINDOW *) 0;
        }

#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        return  1;
}
