/* bq_propts.c -- program options for gbch-q

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
#include <ctype.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
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
#include "files.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "helpargs.h"
#include "errnums.h"
#include "statenums.h"
#include "optflags.h"

void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);
void  ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
void  wn_fill(WINDOW *, const int, const struct sctrl *, const LONG);
LONG  wnum(WINDOW *, const int, struct sctrl *, const LONG);
char *wgets(WINDOW *, const int, struct sctrl *, const char *);
char **gen_qlist(const char *);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);
#ifndef HAVE_ATEXIT
void  exit_cleanup();
#endif

static  char    Filename[] = __FILE__;

extern  WINDOW  *escr,
                *hlpscr,
                *Ew;

#define PROC_DONT_CARE  (-1)    /* Nothing set yet */
#define PROC_JOBS       0       /* Jobs screen NB use ! to switch */
#define PROC_VARS       1       /* Vars screen NB use ! to switch */

extern  SHORT   initscreen;
extern  char    XML_jobdump;

extern  char    *Curr_pwd,
                *spdir;

extern  int     hadrfresh;

extern  ULONG   Last_j_ser,
                Last_v_ser;

#define NUMPROMPTS      3

struct  ltab    {
        int     helpcode;
        char    *message;
        char    *prompts[NUMPROMPTS];
        char    row, col, size;
        void    (*dfn)(struct ltab *);
        int     (*fn)(struct ltab *);
};

static void  prd_queue(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, jobqueue? jobqueue: "");
}

static void  prd_user(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Restru? Restru: "");
}

static void  prd_group(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Restrg? Restrg: "");
}

static void  prd_confabort(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_CONFABORT? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_scrkeep(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_SCRKEEP? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_jfirst(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, initscreen == PROC_DONT_CARE? lt->prompts[2]: initscreen == PROC_VARS? lt->prompts[1]: lt->prompts[0]);
}

static void  prd_localonly(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_LOCALONLY? lt->prompts[1]: lt->prompts[0]);
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

static void  prd_incnull(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, Dispflags & DF_SUPPNULL? lt->prompts[0]: lt->prompts[1]);
}

static  void    prd_xmlfmt(struct ltab *lt)
{
        mvaddstr(lt->row, lt->col, XML_jobdump? lt->prompts[1]: lt->prompts[0]);
}

static int  pro_queue(struct ltab *lt)
{
        char    *str;
        struct  sctrl   wst_queue;

        wst_queue.helpcode = lt->helpcode * 10;
        wst_queue.helpfn = gen_qlist;
        wst_queue.size = lt->size;
        wst_queue.col = lt->col;
        wst_queue.magic_p = MAG_OK|MAG_P|MAG_R|MAG_CRS|MAG_NL;
        wst_queue.msg = (char *) 0;

        str = wgets(stdscr, lt->row, &wst_queue, jobqueue? jobqueue: "");
        if  (str != (char *) 0  &&  strcmp(str, jobqueue? jobqueue: "") != 0)  {
                if  (str[0] == '\0')
                        ws_fill(stdscr, lt->row, &wst_queue, "");
                if  (jobqueue)
                        free(jobqueue);
                jobqueue = (str[0] && (str[0] != '-' || str[1]))? stracpy(str): (char *) 0;
                hadrfresh++;            /*  Force reread jobs/vars */
                Last_j_ser = 0;
                Last_v_ser = 0;
                return  $K{key eol};
        }
        return  wst_queue.retv;
}

static  int     pro_usergroup(struct ltab *lt, char **ugbuf, char **(*genfn)(const char *))
{
        char    *str, *orig = *ugbuf? *ugbuf: "";
        struct  sctrl   wst_user;

        wst_user.helpcode = lt->helpcode * 10;
        wst_user.helpfn = genfn;
        wst_user.size = lt->size;
        wst_user.col = lt->col;
        wst_user.magic_p = MAG_OK|MAG_P|MAG_R|MAG_CRS|MAG_NL;
        wst_user.msg = (char *) 0;

        str = wgets(stdscr, lt->row, &wst_user, orig);
        if  (str != (char *) 0  &&  strcmp(str, orig) != 0)  {
                if  (*ugbuf)  {
                        free(*ugbuf);
                        *ugbuf = (char *) 0;
                }
                if  (str[0] == '\0')
                        ws_fill(stdscr, lt->row, &wst_user, "");
                else
                        *ugbuf = stracpy(str);
                hadrfresh++;            /*  Force reread jobs/vars */
                Last_j_ser = 0;
                Last_v_ser = 0;
                return  $K{key eol};
        }
        return  wst_user.retv;
}

static int  pro_user(struct ltab *lt)
{
        return  pro_usergroup(lt, &Restru, gen_ulist);
}

static int  pro_group(struct ltab *lt)
{
        return  pro_usergroup(lt, &Restrg, gen_glist);
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

static int  pro_jfirst(struct ltab *lt)
{
        unsigned  current = initscreen == PROC_DONT_CARE? 2: initscreen == PROC_VARS? 1: 0;
        unsigned  orig = current;
        int     ch = pro_bool(lt, &current);

        if  (current != orig)
                initscreen = current > 1? PROC_DONT_CARE: current? PROC_VARS: PROC_JOBS;
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

static int  pro_localonly(struct ltab *lt)
{
        ULONG  orig = Dispflags & DF_LOCALONLY;
        int     ch = pro_bitflag(lt, DF_LOCALONLY);
        if  ((Dispflags & DF_LOCALONLY) != orig)  {
                Last_j_ser = Last_v_ser = 0; /* Force reread */
                hadrfresh++;
        }
        return  ch;
}

static int  pro_confabort(struct ltab *lt)
{
        return  pro_bitflag(lt, DF_CONFABORT);
}

static int  pro_scrkeep(struct ltab *lt)
{
        return  pro_bitflag(lt, DF_SCRKEEP);
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

static int  pro_incnull(struct ltab *lt)
{
        unsigned  current = Dispflags & DF_SUPPNULL? 0: 1;
        unsigned  prev = current;
        int     ch = pro_bool(lt, &current);

        if  (current != prev)  {
                Dispflags ^= DF_SUPPNULL;
                hadrfresh++;            /*  Force reread jobs/vars */
                Last_j_ser = 0;
                Last_v_ser = 0;
        }
        return  ch;
}

static  int  pro_xmlfmt(struct ltab *lt)
{
        unsigned  current = XML_jobdump;
        unsigned  prev = current;
        int     ch = pro_bool(lt, &current);
        if  (current != prev)
                XML_jobdump = current? 1: 0;
        return  ch;
}

#define NULLCH  (char *) 0

static  struct  ltab  ltab[] = {
        { $PNH{btq opt queue}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0,28, prd_queue, pro_queue },
        { $PNH{btq opt incnull}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_incnull, pro_incnull },
        { $PNH{btq opt onlyuser}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0,UIDSIZE, prd_user, pro_user },
        { $PNH{btq opt onlygroup}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0,UIDSIZE, prd_group, pro_group },
        { $PNH{btq opt confdel}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_confabort, pro_confabort },
        { $PNH{btq opt curs}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_scrkeep, pro_scrkeep },
        { $PNH{btq opt loco}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_localonly, pro_localonly },
        { $PNH{btq opt helpclr}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_helpclr, pro_helpclr },
        { $PNH{btq opt helpbox}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_helpbox, pro_helpbox },
        { $PNH{btq opt errbox}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_errbox, pro_errbox },
        { $PNH{btq opt initscr}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_jfirst, pro_jfirst },
        { $PNH{btq opt xml fmt}, NULLCH, { NULLCH, NULLCH, NULLCH }, 0, 0, 0, prd_xmlfmt, pro_xmlfmt }
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

        if  ((rowstart = helpnstate($N{btq opt start row})) <= 0)
                rowstart = $N{btq opt queue};
        if  ((pstart = helpnstate($N{btq opt init cursor})) <= 0)
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

int  askyorn(const int code)
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

static void  quoteput(FILE *dest, char *str)
{
        if  (str)
                fprintf(dest, " \'%s\'", str);
        else
                fputs(" -", dest);
}

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0;
        fprintf(dest, "%s", name);

        cancont = spitoption(Dispflags & DF_HELPBOX? $A{btq arg help box}: $A{btq arg no help hox}, $A{btq arg explain}, dest, '=', cancont);
        cancont = spitoption(Dispflags & DF_ERRBOX? $A{btq arg error box}: $A{btq arg no error box}, $A{btq arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_HELPCLR? $A{btq arg losechar}: $A{btq arg keepchar}, $A{btq arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_CONFABORT? $A{btq arg confdel}: $A{btq arg noconfdel}, $A{btq arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_SCRKEEP? $A{btq arg cursor keep}: $A{btq arg cursor follow}, $A{btq arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_LOCALONLY? $A{btq arg local}: $A{btq arg network}, $A{btq arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_SUPPNULL? $A{btq arg no nullqs}: $A{btq arg nullqs}, $A{btq arg explain}, dest, ' ', cancont);
        cancont = spitoption(XML_jobdump? $A{btq arg XML fmt}: $A{btq arg no XML fmt}, $A{btq arg explain}, dest, ' ', cancont);
        if  (initscreen != PROC_DONT_CARE)
                cancont = spitoption(initscreen == PROC_VARS? $A{btq arg vars screen}: $A{btq arg jobs screen}, $A{btq arg explain}, dest, ' ', cancont);
        spitoption($A{btq arg jobqueue}, $A{btq arg explain}, dest, ' ', 0);
        quoteput(dest, jobqueue);
        spitoption($A{btq arg justu}, $A{btq arg explain}, dest, ' ', 0);
        quoteput(dest, Restru);
        spitoption($A{btq arg justg}, $A{btq arg explain}, dest, ' ', 0);
        quoteput(dest, Restrg);
        putc('\n', dest);
}

static void  ask_build()
{
        int     ret, i;
        static  char    btq[] = "BTQ";

        if  (!askyorn($PH{Save parameters}))
                return;

        disp_str = Curr_pwd;
        if  (askyorn($PH{Save in current directory}))  {
                if  ((ret = proc_save_opts(Curr_pwd, btq, spit_options)) == 0)
                        return;
        }
        else  {
                disp_str = "(Home)";
                if  (!askyorn($PH{Save in home directory}))
                        return;
                if  ((ret = proc_save_opts((const char *) 0, btq, spit_options)) == 0)
                        return;
        }
        doerror(stdscr, ret);
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

/* Generate help message for format codes */

static  struct  formatdef       *codeshelp_u,
                                *codeshelp_l;

static char **codeshelp(char *prefixnotused)
{
        unsigned  codecount = 1, cnt;
        struct  formatdef       *fp;
        char    **result, **rp, *msg;
        unsigned        lng;

        if  (codeshelp_u)
                for  (cnt = 0;  cnt < 26;  cnt++)
                        if  (codeshelp_u[cnt].statecode != 0)
                                codecount++;
        if  (codeshelp_l)
                for  (cnt = 0;  cnt < 26;  cnt++)
                        if  (codeshelp_l[cnt].statecode != 0)
                                codecount++;

        if  (!(result = (char **) malloc(codecount * sizeof(char *))))
                ABORT_NOMEM;

        rp = result;
        if  (codeshelp_u)
                for  (cnt = 0;  cnt < 26;  cnt++)  {
                        fp = &codeshelp_u[cnt];
                        if  (fp->statecode == 0)
                                continue;
                        if  (!fp->explain)
                                fp->explain = gprompt(fp->statecode + 200);
                        lng = strlen(fp->explain) + 4;
                        if  (!(msg = malloc(lng)))
                                ABORT_NOMEM;
                        sprintf(msg, "%c  %s", cnt + 'A', fp->explain);
                        *rp++ = msg;
                }
        if  (codeshelp_l)
                for  (cnt = 0;  cnt < 26;  cnt++)  {
                        fp = &codeshelp_l[cnt];
                        if  (fp->statecode == 0)
                                continue;
                        if  (!fp->explain)
                                fp->explain = gprompt(fp->statecode + 200);
                        lng = strlen(fp->explain) + 4;
                        if  (!(msg = malloc(lng)))
                                ABORT_NOMEM;
                        sprintf(msg, "%c  %s", cnt + 'a', fp->explain);
                        *rp++ = msg;
                }
        *rp = (char *) 0;
        return  result;
}

#define LTAB_P          1
#define FMTWID_P        6
#define CODE_P          11
#define EXPLAIN_P       14

static  struct  sctrl   wns_wid = { $H{btq fmt field width}, ((char **(*)()) 0), 3, 0, FMTWID_P, MAG_P, 1L, 100L, (char *) 0 },
                        wss_code = { $H{btq fmt fmt code}, codeshelp, 1, 0, CODE_P, MAG_P, 0L, 0L, (char *) 0 },
                        wss_sep = { $H{btq fmt sep val}, ((char **(*)()) 0), 40, 0, 1, MAG_OK, 0L, 0L, (char *) 0 };

struct  formatrow       {
        char    *f_field;               /* field explain or sep */
        USHORT          f_length;       /* length of field */
        unsigned  char  f_issep;        /* 0=field 1=sep */
        unsigned  char  f_flag;         /* 1=shift left */
        char            f_code;         /* field code */
};

static  struct  formatrow  *flist;
static  int             f_num;
static  unsigned        f_max;
static  char            *ltabmk;

#define INITNUM         20
#define INCNUM          10

static void  conv_fmt(const char *fmt, struct formatdef *utab, struct formatdef *ltab)
{
        struct  formatrow       *fr;

        f_num = 0;
        f_max = INITNUM;
        if  (!(flist = (struct formatrow *) malloc(INITNUM * sizeof(struct formatrow))))
                ABORT_NOMEM;

        while  (*fmt)  {
                if  (f_num >= (int) f_max)  {
                        f_max += INCNUM;
                        if  (!(flist = (struct formatrow *) realloc((char *) flist, (unsigned) (f_max * sizeof(struct formatrow)))))
                                ABORT_NOMEM;
                }
                fr = &flist[f_num];

                if  (*fmt != '%')  {
                        const   char    *fmtp = fmt;
                        char    *nfld;
                        fr->f_issep = 1;
                        fr->f_length = 0;
                        fr->f_flag = 0;
                        do      fr->f_length++;
                        while  (*++fmt  &&  *fmt != '%');
                        if  (!(nfld = malloc((unsigned) (fr->f_length + 1))))
                                ABORT_NOMEM;
                        strncpy(nfld, fmtp, (unsigned) fr->f_length);
                        nfld[fr->f_length] = '\0';
                        fr->f_field = nfld;
                }
                else  {
                        USHORT                  nn;
                        struct  formatdef       *fp;

                        fr->f_issep = 0;
                        fr->f_flag = 0;
                        if  (*++fmt == '<')  {
                                fmt++;
                                fr->f_flag = 1;
                        }
                        nn = 0;
                        do  nn = nn * 10 + *fmt++ - '0';
                        while  (isdigit(*fmt));
                        fr->f_length = nn;

                        if  (isupper(*fmt))
                                fp = &utab[*fmt - 'A'];
                        else  if  (ltab  &&  islower(*fmt))
                                fp = &ltab[*fmt - 'a'];
                        else  {
                                if  (*fmt)
                                        fmt++;
                                continue;
                        }
                        fr->f_code = *fmt++;
                        if  (!fp->explain)
                                fp->explain = gprompt(fp->statecode + 200);
                        fr->f_field = fp->explain;
                }
                f_num++;
        }
}

static char *unconv_fmt()
{
        int     cnt;
        char    *cp;
        struct  formatrow  *fr = flist;
        char    cbuf[256];

        cp = cbuf;
        for  (cnt = 0;  cnt < f_num;  fr++, cnt++)  {
                if  (fr->f_issep)  {
                        const   char    *fp;
                        unsigned        lng;
                        if  ((cp - cbuf) + fr->f_length >= sizeof(cbuf) - 1)
                                break;
                        fp = fr->f_field;
                        lng = fr->f_length;
                        while  (lng != 0)  {
                                *cp++ = *fp++;
                                lng--;
                        }
                }
                else  {
                        if  ((cp - cbuf) + 1 + 1 + 3 + 1 + 1 >= sizeof(cbuf))
                                break;
                        *cp++ = '%';
                        if  (fr->f_flag)
                                *cp++ = '<';
#ifdef  CHARSPRINTF
                        sprintf(cp, "%u%c", fr->f_length, fr->f_code);
                        cp += strlen(cp);
#else
                        cp += sprintf(cp, "%u%c", fr->f_length, fr->f_code);
#endif
                }
        }
        *cp = '\0';
        return  stracpy(cbuf);
}

static void  freeflist()
{
        int     cnt;

        for  (cnt = 0;  cnt < f_num;  cnt++)
                if  (flist[cnt].f_issep)
                        free((char *) flist[cnt].f_field);
        free((char *) flist);
}

#ifdef  HAVE_TERMINFO
#define DISP_CHAR(w, ch)        waddch(w, (chtype) ch);
#else
#define DISP_CHAR(w, ch)        waddch(w, ch);
#endif

static void  fmtdisplay(char **fmt_hdr, int start)
{
        char                    **hv;
        int                     rr;

        if  (!ltabmk)
                ltabmk = gprompt($P{Left tab field});
#ifdef  OS_DYNIX
        clear();
#else
        erase();
#endif

        for  (rr = 0, hv = fmt_hdr;  *hv;  rr++, hv++)
                mvwhdrstr(stdscr, rr, 0, *hv);

        for  (;  start < f_num  &&  rr < LINES;  start++, rr++)  {
                struct  formatrow       *fr = &flist[start];

                if  (fr->f_issep)  {
                        int     cnt = fr->f_length;
                        const   char    *cp = fr->f_field;
                        move(rr, 0);
                        DISP_CHAR(stdscr, '\"');
                        while  (cnt > 0)  {
                                DISP_CHAR(stdscr, *cp);
                                cp++;
                                cnt--;
                        }
                        DISP_CHAR(stdscr, '\"');
                }
                else  {
                        if  (fr->f_flag)
                                mvaddstr(rr, LTAB_P, ltabmk);
                        wn_fill(stdscr, rr, &wns_wid, (int) fr->f_length);
                        move(rr, CODE_P);
                        DISP_CHAR(stdscr, fr->f_code);
                        mvaddstr(rr, EXPLAIN_P, (char *) fr->f_field);
                }
        }
#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
}

/* View/edit formats */

int  fmtprocess(char **fmt, const char hch, struct formatdef *utab, struct formatdef *ltab)
{
        int     ch, err_no, hrows, tilines, start, srow, currow, incr, cnt, insertwhere, changes = 0;
        char    **fmt_hdr;

        /* Save these for help function */

        codeshelp_u = utab;
        codeshelp_l = ltab;

        fmt_hdr = helphdr(hch);
        count_hv(fmt_hdr, &hrows, &err_no);
        tilines = LINES - hrows;
        conv_fmt(*fmt, utab, ltab);

        start = 0;
        srow = 0;

        Ew = stdscr;
        select_state($S{btq select format state});
 Fdisp:
        fmtdisplay(fmt_hdr, start);
 Fmove:
        currow = srow - start + hrows;
        wmove(stdscr, currow, 0);

 Frefresh:

        refresh();

 nextin:
        do  ch = getkey(MAG_A|MAG_P);
        while  (ch == EOF  &&  (hlpscr || escr));

        if  (hlpscr)  {
                endhe(stdscr, &hlpscr);
                if  (Dispflags & DF_HELPCLR)
                        goto  nextin;
        }
        if  (escr)
                endhe(stdscr, &escr);

        switch  (ch)  {
        case  EOF:
                goto  nextin;
        default:
                err_no = $E{btq format unknown command};
        err:
                doerror(stdscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(stdscr, $H{btq select format state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  Frefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                srow++;
                if  (srow >= f_num)  {
                        srow--;
ev:                     err_no = $E{btq format off end};
                        goto  err;
                }
                currow++;
                if  (currow - hrows < tilines)
                        goto  Fmove;
                start++;
                goto  Fdisp;

        case  $K{key cursor up}:
                if  (srow <= 0)  {
bv:                     err_no = $E{btq format off beginning};
                        goto  err;
                }
                srow--;
                if  (srow >= start)
                        goto  Fmove;
                start = srow;
                goto  Fdisp;

        /* Half/Full screen up/down */

        case  $K{key screen down}:
                incr = tilines;
        gotitd:
                if  (srow + incr >= f_num)
                        goto  ev;
                start += incr;
                srow += incr;
                goto  Fdisp;

        case  $K{key half screen down}:
                incr = tilines / 2;
                goto  gotitd;

        case  $K{key half screen up}:
                incr = tilines / 2;
                goto  gotitu;

        case  $K{key screen up}:
                incr = tilines;
                if  (srow - incr < 0)
                        goto  bv;
        gotitu:
                start -= incr;
                srow -= incr;
                goto  Fdisp;

        case  $K{key top}:
                if  (srow == start)  {
                        srow = start = 0;
                        goto  Fdisp;
                }
                srow = start;
                goto  Fmove;

        case  $K{key bottom}:
                incr = tilines - 1;
                if  (start + incr >= f_num)
                        incr = f_num - start - 1;
                if  (srow < start + incr)  {
                        srow = start + incr;
                        goto  Fmove;
                }
                srow = f_num - 1;
                if  (srow < 0)
                        srow = 0;
                start = tilines >= f_num? 0: f_num - tilines;
                goto  Fdisp;

        case  $K{key halt}:

                /* May need to restore clobbered heading */

                if  (changes)  {
                        free(*fmt);
                        *fmt = unconv_fmt();
                }
                freeflist();
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  changes;

        case  $K{btq key fmt delete}:
                if  (f_num <= 0)  {
none2:                  err_no = $E{btq format no formats};
                        goto  err;
                }
                f_num--;
                if  (flist[srow].f_issep)
                        free((char *) flist[srow].f_field);
                if  (srow >= f_num)  {
                        srow--;
                        if  (srow < start)
                                start = srow;
                        if  (srow < 0)
                                start = srow = 0;
                }
                else  for  (cnt = srow;  cnt < f_num;  cnt++)
                        flist[cnt] = flist[cnt+1];
                changes++;
                goto  Fdisp;

        case  $K{btq key fmt width}:
                if  (f_num <= 0)
                        goto  none2;
                if  (flist[srow].f_issep)  {
        issep:
                        err_no = $E{btq format is separator};
                        goto  err;
                }
        {
                LONG  res = wnum(stdscr, currow, &wns_wid, (LONG) flist[srow].f_length);
                if  (res >= 0L)
                        flist[srow].f_length = (USHORT) res;
                changes++;
                goto  Fmove;
        }

        case  $K{btq key fmt toggle left tab}:
                if  (f_num <= 0)
                        goto  none2;
                if  (flist[srow].f_issep)
                        goto  issep;
                move(currow, LTAB_P);
                if  (flist[srow].f_flag)  {
                        flist[srow].f_flag = 0;
                        for  (incr = strlen(ltabmk);  incr > 0;  incr--)
                                DISP_CHAR(stdscr, ' ');
                }
                else  {
                        flist[srow].f_flag = 1;
                        addstr(ltabmk);
                }
                changes++;
                goto  Fmove;

        case  $K{btq key fmt code}:
                if  (f_num <= 0)
                        goto  none2;
                if  (flist[srow].f_issep)
                        goto  issep;
                {
                        char    *resc, fld[2];
                        struct  formatrow       *fr = &flist[srow];
                        struct  formatdef       *fp;
                        fld[0] = fr->f_code;
                        fld[1] = '\0';
                        if  (!(resc = wgets(stdscr, currow, &wss_code, fld)))  {
                                move(currow, EXPLAIN_P);
                                clrtoeol();
                                addstr(fr->f_field);
                                goto  Fmove;
                        }
                        if  (utab  &&  isupper(resc[0])  &&  utab[resc[0] - 'A'].statecode != 0)
                                fp = &utab[resc[0] - 'A'];
                        else  if  (ltab  &&  islower(resc[0])  &&  ltab[resc[0] - 'a'].statecode != 0)
                                fp = &ltab[resc[0] - 'a'];
                        else  {
                                move(currow, CODE_P);
                                DISP_CHAR(stdscr, fld[0]);
                                move(currow, 0);
                                err_no = $E{btq format invalid code};
                                goto  err;
                        }
                        fr->f_code = resc[0];
                        if  (!fp->explain)
                                fp->explain = gprompt(fp->statecode + 200);
                        fr->f_field = fp->explain;
                        move(currow, EXPLAIN_P);
                        clrtoeol();
                        addstr(fp->explain);
                }
                changes++;
                goto  Fmove;

        case  $K{btq key fmt separator edit}:
                if  (f_num <= 0)
                        goto  none2;
                if  (!flist[srow].f_issep)  {
                        err_no = $E{btq format is not separator};
                        goto  err;
                }
                {
                        char    *resc;
                        int     len;
                        if  (!(resc = wgets(stdscr, currow, &wss_sep, flist[srow].f_field)))
                                goto  Fmove;
                        free((char *) flist[srow].f_field);
                        if  ((len = strlen(resc)) <= 0)
                                flist[srow].f_field = stracpy(" ");
                        else  {
                                if  (resc[len-1] == '\"')
                                        resc[--len] = '\0';
                                flist[srow].f_field = stracpy(resc);
                        }
                        move(currow, 0);
                        clrtoeol();
                        printw("\"%s\"", flist[srow].f_field);
                        flist[srow].f_length = (USHORT) len;
                        changes++;
                        goto  Fmove;
                }

        case  $K{btq key fmt new sep before}:
                insertwhere = srow;
                goto  iseprest;
        case  $K{btq key fmt new sep after}:
                insertwhere = srow + 1;
        iseprest:
                clrtoeol();
                refresh();
        {
                char    *resc, *fres;
                int     len;
                struct  formatrow       *fr;
                if  (!(resc = wgets(stdscr, currow, &wss_sep, "")))
                        goto  Fdisp;
                if  ((len = strlen(resc)) <= 0)  {
                        fres = stracpy(" ");
                        len = 1;
                }
                else  {
                        if  (resc[len-1] == '\"')
                                resc[--len] = '\0';
                        fres = stracpy(resc);
                }
                if  (f_num >= (int) f_max)  {
                        f_max += INCNUM;
                        if  (!(flist = (struct formatrow *) realloc((char *) flist, (unsigned) (f_max * sizeof(struct formatrow)))))
                                ABORT_NOMEM;
                }
                for  (cnt = f_num-1;  cnt >= insertwhere;  cnt--)
                        flist[cnt+1] = flist[cnt];
                f_num++;
                fr = &flist[insertwhere];
                fr->f_issep = 1;
                fr->f_length = (USHORT) len;
                fr->f_flag = 0;
                fr->f_field = fres;
                srow = insertwhere;
                if  (srow >= start + tilines)
                        start = srow - tilines + 1;
                changes++;
                goto   Fdisp;
        }
        case  $K{btq key fmt new field before}:
                insertwhere = srow;
                goto  ifldrest;
        case  $K{btq key fmt new field after}:
                insertwhere = srow + 1;
        ifldrest:
                clrtoeol();
                refresh();
        {
                char    *resc;
                int     code;
                LONG    res;
                struct  formatdef       *fp;
                struct  formatrow       *fr;
                if  (!(resc = wgets(stdscr, currow, &wss_code, "")))  {
                        move(currow, 0);
                        clrtoeol();
                        goto  Fdisp;
                }
                code = resc[0];
                if  (utab  &&  isupper(code)  &&  utab[code - 'A'].statecode != 0)
                        fp = &utab[code - 'A'];
                else  if  (ltab  &&  islower(code)  &&  ltab[code - 'a'].statecode != 0)
                        fp = &ltab[code - 'a'];
                else  {
                        fmtdisplay(fmt_hdr, start);
                        move(currow, 0);
                        err_no = $E{btq format invalid code};
                        goto  err;
                }
                wn_fill(stdscr, currow, &wns_wid, (long) fp->sugg_width);
                res = wnum(stdscr, currow, &wns_wid, (long) fp->sugg_width);
                if  (res < 0L)  {
                        if  (res != -2L)
                                goto  Fdisp;
                        res = fp->sugg_width;
                }
                if  (f_num >= (int) f_max)  {
                        f_max += INCNUM;
                        if  (!(flist = (struct formatrow *) realloc((char *) flist, (unsigned) (f_max * sizeof(struct formatrow)))))
                                ABORT_NOMEM;
                }
                for  (cnt = f_num-1;  cnt >= insertwhere;  cnt--)
                        flist[cnt+1] = flist[cnt];
                f_num++;
                fr = &flist[insertwhere];
                fr->f_issep = 0;
                fr->f_length = (SHORT) res;
                fr->f_flag = 0;
                fr->f_code = (char) code;
                if  (!fp->explain)
                        fp->explain = gprompt(fp->statecode + 200);
                fr->f_field = fp->explain;
                srow = insertwhere;
                if  (srow >= start + tilines)
                        start = srow - tilines + 1;
                changes++;
                goto   Fdisp;
        }
        }
}

static  char    *wfmt;

static void  make_confline(FILE *fp, const char *vname)
{
        fprintf(fp, "%s=%s\n", vname, wfmt);
}

void  offersave(char *fmt, const char *varname)
{
        if  (!askyorn($PH{Save format codes}))
                return;

        /* Save this in static location for proc_save_opts/make_confline to use */
         wfmt = fmt;
         disp_str = Curr_pwd;
         if  (askyorn($P{Save in current directory}))
                 proc_save_opts(Curr_pwd, varname, make_confline);
         else  if  (askyorn($P{Save in home directory}))
                 proc_save_opts((const char *) 0, varname, make_confline);
 }
