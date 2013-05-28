/* btuser.c -- Main module for gbch-user

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <curses.h>
#include <ctype.h>
#ifdef  HAVE_TERMIOS_H
#include <termios.h>
#endif
#include "incl_unix.h"
#include "incl_sig.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "files.h"
#include "helpargs.h"
#include "cfile.h"
#include "optflags.h"

static  char    Filename[] = __FILE__;

#define BTUSER_INLINE

#ifdef  TI_GNU_CC_BUG
int     LINES;                  /* Not defined anywhere without extern */
#endif

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

#define SRT_NONE        0       /* Sort by numeric uid (default) */
#define SRT_USER        1       /* Sort by user name */
#define SRT_GROUP       2       /* Sort by group name */

#define BTU_DISP        0       /* Just display stuff */
#define BTU_UPERM       1       /* Set user modes */
#define BTU_UREAD       2       /* Read users */
#define BTU_UUPD        3       /* Update users */

#define USNAM_P 0
#define GRPNAM_P        8
#define P_P             47

#define BOXWID  1

void  wn_fill(WINDOW *, const int, const struct sctrl *, const LONG);
int  propts();
LONG  wnum(WINDOW *, const int, struct sctrl *, const LONG);
void  ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
char *wgets(WINDOW *, const int, struct sctrl *, const char *);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);

char    *Curr_pwd;

char    hok,
        getflags = BTU_DISP,
        alphsort = SRT_NONE;

static  int     more_above,
                more_below,
                hchanges,
                Defline;

static  int     modehdlen,              /* Length up to 1st column */
                modecolw,               /* Width of user/group/other cols */
                modejvcol;              /* Width of job/var cols */

static  char    *bs_colon,
                *bs_pno, *bs_pyes,
                *bs_mno, *bs_myes,
                *bs_mjhdr, *bs_mvhdr,
                *bs_mu, *bs_mg, *bs_mo;

WINDOW  *hlscr, *lscr, *tlscr, *hlpscr, *escr, *Ew;
static  int     LLINES;

struct  perm    {
        int     number, nextstate;
        char    *string, *abbrev;
        ULONG   flg, sflg, rflg;
}  ptab[] = {
        { $PN{Read adm full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_RADMIN, BTM_RADMIN, (ULONG) ~(BTM_RADMIN|BTM_WADMIN) },
        { $PN{Write adm full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_WADMIN, ALLPRIVS, (ULONG) ~BTM_WADMIN },
        { $PN{Create entry full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_CREATE, BTM_CREATE, (ULONG) ~(BTM_CREATE|BTM_SPCREATE)},
        { $PN{Special create full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_SPCREATE, BTM_CREATE|BTM_SPCREATE, (ULONG) ~BTM_SPCREATE},
        { $PN{Stop sched full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_SSTOP, BTM_SSTOP, (ULONG) ~BTM_SSTOP},
        { $PN{Change default modes full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_UMASK, BTM_UMASK, (ULONG) ~(BTM_UMASK|BTM_WADMIN)},
        { $PN{Combine user group full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_ORP_UG, BTM_ORP_UG, (ULONG) ~BTM_ORP_UG},
        { $PN{Combine user other full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_ORP_UO, BTM_ORP_UO, (ULONG) ~BTM_ORP_UO},
        { $PN{Combine group other full}, -1,
                  (char *) 0, (char *) 0,
                  BTM_ORP_GO, BTM_ORP_GO, (ULONG) ~BTM_ORP_GO}};

#define MAXPERM (sizeof(ptab) / sizeof(struct perm))

struct  perm    *plist[MAXPERM+1];

struct  mode    {
        int     number, nextstate;
        char    *string;
        USHORT  flg, sflg, rflg;
}  mtab[] = {
        { $PN{Read mode name}, -1,
                  (char *) 0,
                  BTM_READ, BTM_READ|BTM_SHOW, (USHORT) ~(BTM_READ|BTM_WRITE) },
        { $PN{Write mode name}, -1,
                  (char *) 0,
                  BTM_WRITE, BTM_READ|BTM_WRITE|BTM_SHOW, (USHORT) ~BTM_WRITE },
        { $PN{Reveal mode name}, -1,
                  (char *) 0,
                  BTM_SHOW, BTM_SHOW, (USHORT) ~(BTM_READ|BTM_WRITE|BTM_SHOW) },
        { $PN{Display mode name}, -1,
                  (char *) 0,
                  BTM_RDMODE, BTM_RDMODE, (USHORT) ~(BTM_RDMODE|BTM_WRMODE) },
        { $PN{Set mode name}, -1,
                  (char *) 0,
                  BTM_WRMODE, BTM_RDMODE|BTM_WRMODE, (USHORT) ~BTM_WRMODE },
        { $PN{Assume owner mode name}, -1,
                  (char *) 0,
                  BTM_UTAKE, BTM_UTAKE, (USHORT) ~BTM_UTAKE },
        { $PN{Assume group mode name}, -1,
                  (char *) 0,
                  BTM_GTAKE, BTM_GTAKE, (USHORT) ~BTM_GTAKE },
        { $PN{Give owner mode name}, -1,
                  (char *) 0,
                  BTM_UGIVE, BTM_UGIVE, (USHORT) ~BTM_UGIVE },
        { $PN{Give group mode name}, -1,
                  (char *) 0,
                  BTM_GGIVE, BTM_GGIVE, (USHORT) ~BTM_GGIVE },
        { $PN{Delete mode name}, -1,
                  (char *) 0,
                  BTM_DELETE, BTM_DELETE, (USHORT) ~BTM_DELETE },
        { $PN{Kill mode name}, -1,
                  (char *) 0,
                  BTM_KILL, BTM_KILL, (USHORT) ~BTM_KILL}};

#define MAXMODE (sizeof(mtab) / sizeof(struct mode))

struct  mode    *mlist[MAXMODE+1];

#ifdef  DO_NOT_DEFINE
$P{Read adm abbr}
$P{Write adm abbr}
$P{Create entry abbr}
$P{Special create abbr}
$P{Stop sched abbr}
$P{Change default modes abbr}
$P{Combine user group abbr}
$P{Combine user other abbr}
$P{Combine group other abbr}
#endif

/* This flag tells us whether we have called the routine to turn the
   error codes into messages or not */

static  char    code_expanded = 0, Win_setup;
static  char    *more_amsg, *more_bmsg;

/* Tables for use by wnum */

#define HELPLESS        ((char **(*)()) 0)
#define NULLCH          ((char *) 0)

static  struct  sctrl
  wn_udp = { $H{btuser user def prio help}, HELPLESS, 3, 0, 16, MAG_P, 1L, 255L, NULLCH },
  wn_ddp = { $H{btuser sys def prio help}, HELPLESS, 3, 0, 16, MAG_P, 1L, 255L, NULLCH },
  wn_ulp = { $H{btuser user min prio help}, HELPLESS, 3, 0, 20, MAG_P, 1L, 255L, NULLCH },
  wn_dlp = { $H{btuser sys min prio help}, HELPLESS, 3, 0, 20, MAG_P, 1L, 255L, NULLCH },
  wn_uhp = { $H{btuser user max prio help}, HELPLESS, 3, 0, 24, MAG_P, 1L, 255L, NULLCH },
  wn_dhp = { $H{btuser sys max prio help}, HELPLESS, 3, 0, 24, MAG_P, 1L, 255L, NULLCH },
  wn_umaxll = { $H{btuser user max ll help}, HELPLESS, 5, 0, 28, MAG_P, 1L, 32767L, NULLCH },
  wn_dmaxll = { $H{btuser sys max ll help}, HELPLESS, 5, 0, 28, MAG_P, 1L, 32767L, NULLCH },
  wn_utotll = { $H{btuser user tot ll help}, HELPLESS, 5, 0, 34, MAG_P, 1L, 32767L, NULLCH },
  wn_dtotll = { $H{btuser sys tot ll help}, HELPLESS, 5, 0, 34, MAG_P, 1L, 32767L, NULLCH },
  wn_uspecll = { $H{btuser user spec ll help}, HELPLESS, 5, 0, 40, MAG_P, 1L, 32767L, NULLCH },
  wn_dspecll = { $H{btuser sys spec ll help}, HELPLESS, 5, 0, 40, MAG_P, 1L, 32767L, NULLCH };

#ifdef  HAVE_TERMIOS_H
struct  termios orig_term;
#else
struct  termio  orig_term;
#endif

void  exit_cleanup()
{
        if  (Win_setup)  {
                clear();
                refresh();
                endwinkeys();
        }
}

/* Deal with signals.  */

RETSIGTYPE  catchit(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(n, SIG_IGN);
#endif
#ifndef HAVE_ATEXIT
        exit_cleanup();
#endif
        exit(E_SIGNAL);
}

/* Initialise screen for curses */

void  screeninit()
{
#if     defined(TOWER) || defined(OS_DYNIX)
        struct  termio  aswas, asis;
#endif
#ifdef  STRUCT_SIG
        struct  sigstruct_name  ze;
        ze.sighandler_el = catchit;
        sigmask_clear(ze);
        ze.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGINT, &ze, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &ze, (struct sigstruct_name *) 0);
        sigact_routine(SIGHUP, &ze, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &ze, (struct sigstruct_name *) 0);
#ifndef DEBUG
        sigact_routine(SIGBUS, &ze, (struct sigstruct_name *) 0);
        sigact_routine(SIGSEGV, &ze, (struct sigstruct_name *) 0);
        sigact_routine(SIGILL, &ze, (struct sigstruct_name *) 0);
        sigact_routine(SIGFPE, &ze, (struct sigstruct_name *) 0);
#endif
#else  /* !STRUCT_SIG */
        signal(SIGINT, catchit);
        signal(SIGQUIT, catchit);
        signal(SIGHUP, catchit);
        signal(SIGTERM, catchit);
#ifndef DEBUG
        signal(SIGBUS, catchit);
        signal(SIGSEGV, catchit);
        signal(SIGILL, catchit);
        signal(SIGFPE, catchit);
#endif
#endif

#if     defined(TOWER) || defined(OS_DYNIX)
        ioctl(0, TCGETA, &aswas);
#endif
#ifdef M88000
        if  (sysconf(_SC_JOB_CONTROL) < 0)
                fprintf(stderr, "WARNING: No job control\n");
#endif
        initscr();
        raw();
        nonl();
        noecho();

#if     defined(TOWER) || defined(OS_DYNIX)

        /* Restore the port's hardware to what it was before curses
           did its dirty deed.  */

        ioctl(0, TCGETA, &asis);
        asis.c_cflag = aswas.c_cflag;
        ioctl(0, TCSETA, &asis);
#endif
        Win_setup = 1;
}

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        if  (Win_setup)  {
                clear();
                printw("%s:Mem alloc fault:%s line %d", progname, fl, ln);
                refresh();
                endwinkeys();
                Win_setup = 0;
        }
        else
                fprintf(stderr, "%s:Mem alloc fault:%s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static int  r_max(int a, int b)
{
        return  a > b? a: b;
}

/* Expand privilege etc codes into messages */

static void  expcodes()
{
        int     i, j, look4, permstart, modestart;

        if  (code_expanded)
                return;

        permstart = helpnstate($N{Privs initial row});
        modestart = helpnstate($N{Modes initial row});

        for  (i = 0;  i < MAXPERM;  i++)
                ptab[i].string = gprompt(ptab[i].number);
        for  (i = 0;  i < MAXPERM;  i++)
                ptab[i].abbrev = gprompt(ptab[i].number+$S{Privs abbrev});
        for  (i = 0;  i < MAXPERM;  i++)
                ptab[i].nextstate = helpnstate(ptab[i].number);
        for  (i = 0;  i < MAXMODE;  i++)  {
                mtab[i].string = gprompt(mtab[i].number);
                modehdlen = r_max(modehdlen, strlen(mtab[i].string));
        }
        for  (i = 0;  i < MAXMODE;  i++)
                mtab[i].nextstate = helpnstate(mtab[i].number);

        /* Get messages and prompts */

        bs_colon = gprompt($P{Btuser priv colon});
        bs_pno = gprompt($P{Btuser priv no});
        bs_pyes = gprompt($P{Btuser priv yes});

        bs_mno = gprompt($P{Btuser mode no});
        bs_myes = gprompt($P{Btuser mode yes});
        bs_mjhdr = gprompt($P{Btuser job mode hdr});
        bs_mvhdr = gprompt($P{Btuser var mode hdr});
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
        modejvcol = r_max(modecolw*3, r_max(strlen(bs_mjhdr), strlen(bs_mvhdr))) + 1;

        /* Now build list giving desired order */

        i = 0;
        look4 = permstart;
        for  (;;)  {
                for  (j = 0;  j < MAXPERM;  j++)
                        if  (ptab[j].number == look4)  {
                                plist[i] = &ptab[j];
                                i++;
                                look4 = ptab[j].nextstate;
                                goto  dun;
                        }
                disp_arg[9] = look4;
                print_error($E{Missing state code});
                exit(E_BADCFILE);
        dun:
                if  (look4 < 0)  {
                        if  (i != MAXPERM)  {
                                print_error($E{Scrambled state code});
                                exit(E_BADCFILE);
                        }
                        break;
                }
        }

        /* Repeat all that gunge for modes */

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
                print_error($E{Missing state code});
                exit(E_BADCFILE);
        dunm:
                if  (look4 < 0)  {
                        if  (i != MAXMODE)  {
                                print_error($E{Scrambled state code});
                                exit(E_BADCFILE);
                        }
                        break;
                }
        }

        code_expanded = 1;
}

/* Generate help message.  */

void  dohelp(WINDOW *owin, struct sctrl *scp, char *prefix)
{
        char    **hv, **examples = (char **) 0;
        int     hrows, hcols, erows, ecols, cols, rows;
        int     begy, cy, cx, startrow, startcol, i, l;

        if  (*(hv = helpvec(scp->helpcode, 'H')) == (char *) 0)  {
                free((char *) hv);
                disp_arg[9] = scp->helpcode;
                hv = helpvec($E{Missing help code}, 'E');
        }

        if  (scp->helpfn)
                examples = (*scp->helpfn)(prefix, 0);

        count_hv(hv, &hrows, &hcols);
        count_hv(examples, &erows, &ecols);
        cols = hcols;

        if  (ecols > cols)
                cols = ecols;

        rows = hrows + erows;
        if  (Dispflags & DF_HELPBOX)  {
                rows += 2 * BOXWID;
                cols += 2 * BOXWID;
        }
        if  (rows > LINES)
                rows = LINES;
        if  (cols > COLS)
                cols = LINES;

        /* Find absolute cursor position and try to create window
           avoiding it.  */

        getbegyx(owin, begy, cx);
        getyx(owin, cy, cx);
        cy += begy;
        if  ((startrow = cy - rows/2) < 0)
                startrow = 0;
        else  if  (startrow + rows > LINES)
                startrow = LINES - rows;
        if  ((startcol = cx - cols/2) < 0)
                startcol = 0;
        else  if  (startcol + cols > COLS)
                startcol = COLS - cols;

        if  (cx + cols + 2 < COLS)
                startcol = COLS - cols - 1;
        else  if  (cx - cols - 1 >= 0)
                startcol = cx - cols - 1;
        else  if  (cy + rows + 2 < LINES)
                startrow = cy + 2;
        else  if  (cy - rows - 1 >= 0)
                startrow = cy - rows - 1;

        if  ((hlpscr = newwin(rows <= 0? 1: rows, cols, startrow, startcol)) == (WINDOW *) 0)
                ABORT_NOMEM;

        if  (Dispflags & DF_HELPBOX)  {
#ifdef  HAVE_TERMINFO
                box(hlpscr, 0, 0);
#else
                box(hlpscr, '|', '-');
#endif
                for  (i = 0;  i < hrows;  i++)
                        mvwaddstr(hlpscr, i + BOXWID, BOXWID, hv[i]);
                for  (i = 0;  i < erows;  i++)
                        mvwaddstr(hlpscr, i + hrows + BOXWID, BOXWID, examples[i]);
        }
        else  {
                wstandout(hlpscr);

                for  (i = 0;  i < hrows;  i++)  {
                        mvwaddstr(hlpscr, i, 0, hv[i]);
                        for  (l = strlen(hv[i]);  l < cols;  l++)
                                waddch(hlpscr, ' ');
                }
                for  (i = 0;  i < erows;  i++)  {
                        mvwaddstr(hlpscr, i+hrows, 0, examples[i]);
                        for  (l = strlen(hv[i]);  l < cols;  l++)
                                waddch(hlpscr, ' ');
                }
        }
        freehelp(hv);
        freehelp(examples);

#ifdef HAVE_TERMINFO
        wnoutrefresh(hlpscr);
        wnoutrefresh(owin);
        doupdate();
#else
        wrefresh(hlpscr);
        wrefresh(owin);
#endif
}

/* Bodge the above for when we don't have a specific thing to do other
   than display a message code.  */

void  dochelp(WINDOW *wp, int code)
{
        struct  sctrl   xx;
        xx.helpcode = code;
        xx.helpfn = HELPLESS;

        dohelp(wp, &xx, NULLCH);
}

void  endhe(WINDOW *owin, WINDOW **wpp)
{
        delwin(*wpp);
        *wpp = (WINDOW *) 0;
        touchwin(owin);
        if  (owin != stdscr)  {
                WINDOW  *unwin = hlscr;
                if  (tlscr)  {
                        touchwin(tlscr);
#ifdef HAVE_TERMINFO
                        wnoutrefresh(tlscr);
#else
                        wrefresh(tlscr);
#endif
                }
                if  (owin == hlscr)
                        unwin = lscr;
                touchwin(unwin);
#ifdef HAVE_TERMINFO
                wnoutrefresh(unwin);
#else
                wrefresh(unwin);
#endif
        }
#ifdef HAVE_TERMINFO
        wnoutrefresh(owin);
        doupdate();
#else
        wrefresh(owin);
#endif
}

/* Generate error message avoiding (if possible) current cursor position.  */

void  doerror(WINDOW *wp, int Errnum)
{
        char    **ev;
        int     erows, ecols, rows, cols;
        int     begy, cy, startrow, startcol, i, l;

#ifdef  HAVE_TERMINFO
        flash();
#else
        putchar('\007');
#endif

        if  (*(ev = helpvec(Errnum, 'E')) == (char *) 0)  {
                free((char *) ev);
                disp_arg[9] = Errnum;
                ev = helpvec($E{Missing error code}, 'E');
        }

        count_hv(ev, &erows, &ecols);
        rows = erows;
        cols = ecols;

        if  (Dispflags & DF_ERRBOX)  {
                rows += 2 * BOXWID;
                cols += 2 * BOXWID;
        }
        if  (cols > COLS)  {
                ecols -= cols - COLS;
                cols = COLS;
        }

        /* Find absolute cursor position and try to create window
           avoiding it.  */

        getbegyx(wp, begy, i);
        getyx(wp, cy, i);
        cy += begy;
        if  (cy >= LINES/2)
                startrow = 0;
        else
                startrow = LINES - rows;
        startcol = (COLS - cols) / 2;

        if  ((escr = newwin(rows <= 0? 1: rows, cols, startrow, startcol)) == (WINDOW *) 0)
                ABORT_NOMEM;

        if  (Dispflags & DF_ERRBOX)  {
#ifdef  HAVE_TERMINFO
                box(escr, 0, 0);
#else
                box(escr, '|', '-');
#endif
                for  (i = 0;  i < erows;  i++)
                        mvwaddstr(escr, i + BOXWID, BOXWID, ev[i]);
        }
        else  {
                wstandout(escr);

                for  (i = 0;  i < erows;  i++)  {
                        mvwaddstr(escr, i, 0, ev[i]);
                        for  (l = strlen(ev[i]);  l < ecols;  l++)
                                waddch(escr, ' ');
                }
        }
        freehelp(ev);

#ifdef HAVE_TERMINFO
        wnoutrefresh(escr);
        wnoutrefresh(wp);
        doupdate();
#else
        wrefresh(escr);
        wrefresh(wp);
#endif
}

/* Display and update modes
   Return non-zero if changes made
   First arg is the user list if setting default and isindivuser is 0
   Otherwise user we're looking at */

int  mprocess(BtuserRef userlist, int readwrite, int isindivuser)
{
        struct  mode    *mp;
        int  ch, currow;
        USHORT  *resj, *resv;
        char    **hv, **hvi, *smsg;
        int     srow, job_var, ugo_which, changes = 0;
        int     err_no, mstate;

        currow = 0;
        job_var = 0;
        ugo_which = 0;

 redraw:
        Ew = stdscr;
        clear();

        if  (isindivuser)  {
                disp_arg[0] = userlist->btu_user;
                disp_str = prin_uname((uid_t) userlist->btu_user);
                disp_arg[1] = lastgid;
                disp_str2 = prin_gname((gid_t) lastgid);
                hv = helphdr('R');
                resj = userlist->btu_jflags;
                resv = userlist->btu_vflags;
                mstate = readwrite? $S{btuser edit modes}: $S{btuser view modes};
        }
        else  {
                hv = helphdr('Q');
                resj = Btuhdr.btd_jflags;
                resv = Btuhdr.btd_vflags;
                mstate = readwrite? $S{btuser edit defmodes}: $S{btuser view defmodes};
        }

        /* Put up heading, and at the same time count the lines into `srow'.  */

        for  (srow = 0, hvi = hv;  *hvi;  srow++, hvi++)  {
                mvwhdrstr(stdscr, srow, 0, *hvi);
                free(*hvi);
        }
        free((char *) hv);

        /* Put out "Jobs" ... "Vars" type header */

        mvaddstr(srow, modehdlen, bs_mjhdr);
        mvaddstr(srow, modehdlen+modejvcol, bs_mvhdr);
        srow++;

        /* Put out "User/group/others" headers */

        mvaddstr(srow, modehdlen, bs_mu);
        mvaddstr(srow, modehdlen+modecolw, bs_mg);
        mvaddstr(srow, modehdlen+modecolw*2, bs_mo);
        mvaddstr(srow, modehdlen+modejvcol, bs_mu);
        mvaddstr(srow, modehdlen+modejvcol+modecolw, bs_mg);
        mvaddstr(srow, modehdlen+modejvcol+modecolw*2, bs_mo);
        srow += 2;

        select_state(mstate);

 fixmode:

        /* Put out current values of flags */

        for  (err_no = 0;  err_no < MAXMODE;  err_no++)  {
                mp = mlist[err_no];
                mvaddstr(srow + err_no, 0, mp->string);
                clrtoeol();
                for  (ch = 0;  ch < 3;  ch++)  {
                        mvaddstr(srow + err_no,
                                        modehdlen+modecolw*ch,
                                        resj[ch] & mp->flg? bs_myes: bs_mno);

                        /* Not quite as many var modes */

                        if  (mp->flg & VALLMODES)
                                mvaddstr(srow + err_no,
                                                modejvcol+modehdlen+modecolw*ch,
                                                resv[ch] & mp->flg? bs_myes: bs_mno);
                }
        }

        for  (;;)  {
                move(srow + currow, modehdlen + job_var + modecolw * ugo_which);
                refresh();

                do  ch = getkey(MAG_P|MAG_A);
                while  (ch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(stdscr, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(stdscr, &escr);

                switch  (ch)  {
                case  EOF:
                        continue;
                default:
                badc:
                        err_no = $E{btuser mode unknown command};
                err:
                        doerror(stdscr, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(stdscr, mstate);
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key halt}:
                fin:
                        clear();
#ifdef CURSES_MEGA_BUG
                        refresh();
#endif

                        /* If just a common or garden user, return
                           number of changes */

                        if  (isindivuser)
                                return  changes;

                        /* If nothing done, don't ask stupid questions.  */

                        if  (changes <= 0)
                                return  0;

                        smsg = gprompt($P{Btuser q modes copy over});
                        standout();
                        mvaddstr(LINES/2, (COLS - (int) strlen(smsg))/2, smsg);
                        free(smsg);
                        standend();
                        refresh();
                        do  {
                                ch = getkey(MAG_A|MAG_P);
                                if  (ch == $K{key help})
                                        dochelp(stdscr, $H{Btuser q modes copy over});
                        }  while  (ch != $K{btuser key set mode} && ch != $K{btuser key unset mode});

                        if  (ch == $K{btuser key set mode})  {
                                for  (currow = 0;  currow < Npwusers;  currow++)
                                        if  (userlist[currow].btu_user != Realuid)
                                                for  (ugo_which = 0;  ugo_which < 3;  ugo_which++)  {
                                                        userlist[currow].btu_jflags[ugo_which] = resj[ugo_which];
                                                        userlist[currow].btu_vflags[ugo_which] = resv[ugo_which];
                                                }
                        }
                        return  changes;

                case  $K{key save opts}:
                        propts();
                        goto  redraw;

                case  $K{key eol}:

                        /* Go onto next column */

                        if  (++ugo_which < 3)
                                continue;
                        ugo_which = 0;
                        if  (job_var <= 0  &&  mlist[currow]->flg & VALLMODES)  {
                                job_var = modejvcol;
                                continue;
                        }
                        job_var = 0;
                        if  (currow >= MAXMODE - 1)
                                goto  fin;

                case  $K{key cursor down}:
                        currow++;
                        if  (currow >= MAXMODE)  {
                                currow--;
                                err_no = $E{btuser mode off end};
                                goto  err;
                        }
                        if  (job_var  &&  !(mlist[currow]->flg & VALLMODES))
                                job_var = 0;
                        continue;

                case  $K{key cursor up}:
                        if  (currow <= 0)  {
                                err_no = $E{btuser mode off beg};
                                goto  err;
                        }
                        currow--;
                        if  (job_var  &&  !(mlist[currow]->flg & VALLMODES))
                                job_var = 0;
                        continue;

                case  $K{key top}:
                        currow = 0;
                        job_var = 0;
                        ugo_which = 0;
                        continue;

                case  $K{key bottom}:
                        currow = MAXMODE - 1;
                        job_var = 0;
                        ugo_which = 0;
                        continue;

                case  $K{btuser key mode left}:
                        if  (ugo_which <= 0)  {
                                if  (job_var <= 0)  {
                                        err_no = $E{btuser mode off left};
                                        goto  err;
                                }
                                job_var = 0;
                                ugo_which = 2;
                                continue;
                        }
                        ugo_which--;
                        continue;

                case  $K{btuser key mode right}:
                        ugo_which++;
                        if  (ugo_which >= 3)  {
                                if  (job_var > 0  || !(mlist[currow]->flg & VALLMODES))  {
                                        job_var = 0;
                                        ugo_which--;
                                        err_no = $E{btuser mode off right};
                                        goto  err;
                                }
                                ugo_which = 0;
                                job_var = modejvcol;
                                continue;
                        }
                        continue;

                case  $K{btuser key mode jobs}:
                        job_var = 0;
                        continue;

                case  $K{btuser key mode vars}:
                        if  (mlist[currow]->flg & VALLMODES)
                                job_var = modejvcol;
                        continue;

                case  $K{btuser key set mode}:
                        if  (!readwrite)  {
                        readonly:
                                err_no = $E{btuser mode read only};
                                goto  err;
                        }
                strue:
                        changes++;
                        if  (job_var)
                                resv[ugo_which] |= mlist[currow]->sflg;
                        else
                                resj[ugo_which] |= mlist[currow]->sflg;
                fixn:
                        if  (++ugo_which < 3)
                                goto  fixmode;
                        ugo_which = 0;
                        if  (job_var == 0 && mlist[currow]->flg & VALLMODES)  {
                                job_var = modejvcol;
                                goto  fixmode;
                        }
                        job_var = 0;
                        if  (++currow >= MAXMODE)
                                currow = 0;
                        goto  fixmode;

                case  $K{btuser key unset mode}:
                        if  (!readwrite)
                                goto  readonly;
                sfalse:
                        changes++;
                        if  (job_var)
                                resv[ugo_which] &= mlist[currow]->rflg;
                        else
                                resj[ugo_which] &= mlist[currow]->rflg;
                        goto  fixn;

                case  $K{btuser key toggle mode}:
                        if  (!readwrite)
                                goto  readonly;
                        if  (job_var)  {
                                if  (resv[ugo_which] & mlist[currow]->flg)
                                     goto  sfalse;
                        }
                        else  if  (resj[ugo_which] & mlist[currow]->flg)
                                goto  sfalse;
                        goto  strue;

                case  $K{btuser key setdef mode}:
                        if  (!readwrite)
                                goto  readonly;
                        if  (!isindivuser)
                                goto  badc;

                        resj[0] = Btuhdr.btd_jflags[0];
                        resj[1] = Btuhdr.btd_jflags[1];
                        resj[2] = Btuhdr.btd_jflags[2];
                        resv[0] = Btuhdr.btd_vflags[0];
                        resv[1] = Btuhdr.btd_vflags[1];
                        resv[2] = Btuhdr.btd_vflags[2];
                        changes++;
                        goto  fixmode;
                }
        }
}

/* Display and update privileges
   Return non-zero if changes made */

int  pprocess(BtuserRef userlist, int readwrite, int isindivuser)
{
        struct  perm    *pp;
        int  ch, currow;
        ULONG   res;
        char    **hv, **hvi, *uname, *smsg;
        int     srow, erow, mstate, changes = 0;
        int     err_no;
        char    pcol[MAXPERM];

        Ew = stdscr;
        clear();

        if  (isindivuser)  {
                disp_str = uname = prin_uname((uid_t) userlist->btu_user);
                disp_arg[0] = userlist->btu_user;
                disp_arg[1] = lastgid;
                disp_str2 = prin_gname((gid_t) lastgid);
                hv = helphdr('N');
                res = userlist->btu_priv;
                smsg = gprompt($P{btuser privs state});
                mstate = readwrite? $S{btuser privs state} : $S{btuser view privs};
        }
        else  {
                hv = helphdr('M');
                res = Btuhdr.btd_priv;
                smsg = gprompt($P{btuser defprivs state});
                uname = "";
                mstate = readwrite? $S{btuser defprivs state} : $S{btuser view defprivs};
        }

        /* Put up heading, and at the same time count the lines into `srow'.  */

        for  (srow = 0, hvi = hv;  *hvi;  srow++, hvi++)  {
                mvwhdrstr(stdscr, srow, 0, *hvi);
                free(*hvi);
        }
        free((char *) hv);

        /* Put out current values of flags */

        for  (currow = 0;  currow < MAXPERM;  currow++)  {
                pp = plist[currow];
                mvprintw(srow + currow, 0, smsg, uname);
                addstr(pp->string);
                addstr(bs_colon);
                getyx(stdscr, erow, pcol[currow]);
                addstr(res & pp->flg? bs_pyes: bs_pno);
        }
#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        erow = currow;                  /* Where we got to - relative end */
        free(smsg);

        select_state(mstate);

        currow = 0;

        for  (;;)  {
                move(srow + currow, pcol[currow]);
                refresh();

                do  ch = getkey(MAG_P|MAG_A);
                while  (ch == EOF  &&  (hlpscr || escr));

                if  (hlpscr)  {
                        endhe(stdscr, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(stdscr, &escr);

                switch  (ch)  {
                case  EOF:
                        continue;
                default:
                badc:
                        err_no = $E{btuser priv unknown command};
                err:
                        doerror(stdscr, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(stdscr, mstate);
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key halt}:
                fin:
                        /* If just a common or garden user, return number of changes */

                        if  (isindivuser)  {
                                userlist->btu_priv = (ULONG) res;
                                return  changes;
                        }

                        /* If nothing done, don't ask stupid questions.  */

                        if  (changes <= 0)
                                return  0;

                        smsg = gprompt($P{Btuser q privs copy over});
                        clear();
                        standout();
                        mvaddstr(LINES/2, (COLS - (int) strlen(smsg))/2, smsg);
                        free(smsg);
                        standend();
                        refresh();
                        do  {
                                ch = getkey(MAG_A|MAG_P);
                                if  (ch == $K{key help})
                                        dochelp(stdscr, $H{Btuser q privs copy over});
                        }  while  (ch != $K{Btuser key set priv} && ch != $K{Btuser key unset priv});

                        if  (ch == $K{Btuser key set priv})
                                for  (currow = 0;  currow < (int) Npwusers;  currow++)
                                        if  (userlist[currow].btu_user != Realuid)
                                                userlist[currow].btu_priv = (ULONG) res;
                        Btuhdr.btd_priv = (ULONG) res;
                        return  changes;

                case  $K{key eol}:
                        if  (++currow >= erow)
                                goto  fin;
                        continue;

                case  $K{key cursor down}:
                        if  (++currow >= erow)  {
                                currow--;
                                err_no = $E{btuser priv off end};
                                goto  err;
                        }
                        continue;

                case  $K{key cursor up}:
                        if  (currow <= 0)  {
                                err_no = $E{btuser priv off beg};
                                goto  err;
                        }
                        currow--;
                        continue;

                case  $K{key top}:
                        currow = 0;
                        continue;

                case  $K{key bottom}:
                        currow = erow - 1;
                        continue;

                case  $K{Btuser key set priv}:
                        if  (!readwrite)  {
                        readonly:
                                err_no = $E{btuser priv read only};
                                goto  err;
                        }
                strue:
                        changes++;
                        res |= plist[currow]->sflg;
                        goto  fixperm;

                case  $K{Btuser key unset priv}:
                        if  (!readwrite)
                                goto  readonly;
                sfalse:
                        changes++;
                        res &= plist[currow]->rflg;
                        goto  fixperm;

                case  $K{Btuser key toggle priv}:
                        if  (!readwrite)
                                goto  readonly;
                        if  (res & plist[currow]->flg)
                                goto  sfalse;
                        goto  strue;

                case  $K{Btuser key setdef priv}:
                        if  (!readwrite)
                                goto  readonly;
                        if  (!isindivuser)
                                goto  badc;
                        res = Btuhdr.btd_priv;
                        changes++;
                fixperm:
                        for  (ch = 0;  ch < MAXPERM;  ch++)  {
                                move(srow+ch, pcol[ch]);
                                clrtoeol();
                                addstr(plist[ch]->flg & res? bs_pyes: bs_pno);
                        }
                        continue;
                }
        }
}

void  copyu(BtuserRef up)
{
        up->btu_minp = Btuhdr.btd_minp;
        up->btu_maxp = Btuhdr.btd_maxp;
        up->btu_defp = Btuhdr.btd_defp;
        up->btu_maxll = Btuhdr.btd_maxll;
        up->btu_totll = Btuhdr.btd_totll;
        up->btu_spec_ll = Btuhdr.btd_spec_ll;
        up->btu_jflags[0] = Btuhdr.btd_jflags[0];
        up->btu_jflags[1] = Btuhdr.btd_jflags[1];
        up->btu_jflags[2] = Btuhdr.btd_jflags[2];
        up->btu_vflags[0] = Btuhdr.btd_vflags[0];
        up->btu_vflags[1] = Btuhdr.btd_vflags[1];
        up->btu_vflags[2] = Btuhdr.btd_vflags[2];
}

/* Spit out a prompt for a search string */

static  char *gsearchs(const int isback, const int row)
{
        char    *gstr;
        struct  sctrl   ss;
        static  char    *lastmstr;
        static  char    *sforwmsg, *sbackwmsg;

        if  (!sforwmsg)  {
                sforwmsg = gprompt($P{btuser search forward});
                sbackwmsg = gprompt($P{btuser search backward});
        }

        ss.helpcode = $H{btuser search forward};
        gstr = isback? sbackwmsg: sforwmsg;
        ss.helpfn = HELPLESS;
        ss.size = 30;
        ss.retv = 0;
        ss.col = (SHORT) strlen(gstr);
        ss.magic_p = MAG_OK;
        ss.min = 0L;
        ss.vmax = 0L;
        ss.msg = (char *) 0;
        mvwaddstr(lscr, row, 0, gstr);
        wclrtoeol(lscr);

        if  (lastmstr)  {
                ws_fill(lscr, row, &ss, lastmstr);
                gstr = wgets(lscr, row, &ss, lastmstr);
                if  (!gstr)
                        return  (char *) 0;
                if  (gstr[0] == '\0')
                        return  lastmstr;
        }
        else  {
                for  (;;)  {
                        gstr = wgets(lscr, row, &ss, "");
                        if  (!gstr)
                                return  (char *) 0;
                        if  (gstr[0])
                                break;
                        doerror(lscr, $E{btuser search backward});
                }
        }
        if  (lastmstr)
                free(lastmstr);
        return  lastmstr = stracpy(gstr);
}

/* Match a job string "vstr" against a pattern string "mstr" */

static  int  smatchit(const char *vstr, const char *mstr)
{
        const   char    *tp, *mp;
        while  (*vstr)  {
                tp = vstr;
                mp = mstr;
                while  (*mp)  {
                        if  (*mp != '.'  &&  toupper(*mp) != toupper(*tp))
                                goto  ng;
                        mp++;
                        tp++;
                }
                return  1;
        ng:
                vstr++;
        }
        return  0;
}

/* Only match user name for now, but write like this to allow for
   future e-x-p-a-n-s-i-o-n.  */

static  int  smatch(BtuserRef uwho, const char *mstr)
{
        return  smatchit(prin_uname((uid_t) uwho->btu_user), mstr);
}

/* Search for string in user name
   Return 0 - found otherwise return error code */

static  int  dosearch(BtuserRef ulist, const int isback, int *Tup, int *Cup)
{
        char    *mstr;
        int     mline, Current_user = *Cup, row = *Tup - Current_user + more_above;

        mstr = gsearchs(isback, row);

        if  (!mstr)
                return  0;

        if  (isback)  {
                for  (mline = Current_user - 1;  mline >= 0;  mline--)
                        if  (smatch(&ulist[mline], mstr))
                                goto  gotit;
                for  (mline = Npwusers - 1;  mline >= Current_user;  mline--)
                        if  (smatch(&ulist[mline], mstr))
                                goto  gotit;
        }
        else  {
                for  (mline = Current_user + 1;  (unsigned) mline < Npwusers;  mline++)
                        if  (smatch(&ulist[mline], mstr))
                                goto  gotit;
                for  (mline = 0;  mline <= Current_user;  mline++)
                        if  (smatch(&ulist[mline], mstr))
                                goto  gotit;
        }
        return  $E{btuser search not found};

 gotit:
        Current_user = mline;
        row = Current_user - *Tup;
        if  (row < 0  ||  row + more_above + more_below >= LLINES)
                *Tup = Current_user;
        *Cup = Current_user;
        return  0;
}

/* Initialise header for list.  */

void  linithdr()
{
        char    **hv, **hvi, **hf;
        int     i, hlines, hflines;

        /* Get title for main list */

        hv = helphdr('L');
        count_hv(hv, &hlines, &i);
        if  (hlines <= 0)
                hlines = 1;

        hf = helphdr('F');
        count_hv(hf, &hflines, &i);
        hlscr = newwin(hlines, 0, 0, 0);
        LLINES = LINES - hlines - hflines;
        lscr = newwin(LLINES, 0, hlines, 0);
        if  (!hlscr  ||  !lscr)
                ABORT_NOMEM;
        if  (hflines > 0)  {
                if  (!(tlscr = newwin(hflines, 0, LINES - hflines, 0)))
                        ABORT_NOMEM;
                for  (i = 0, hvi = hf;  *hvi;  i++, hvi++)  {
                        mvwhdrstr(tlscr, i, 0, *hvi);
                        free(*hvi);
                }
                free((char *) hf);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(tlscr);
#else
                wrefresh(tlscr);
#endif
        }

        /* Find where "default" message lives */

        Defline = hlines - 1;
        for  (i = Defline;  i >= 0;  i--)
                if  ((int) strlen(hv[i]) > USNAM_P)  {
                        Defline = i;
                        break;
                }

        /* Put up header stuff */

        for  (i = 0, hvi = hv;  *hvi;  i++, hvi++)  {
                mvwhdrstr(hlscr, i, 0, *hvi);
                free(*hvi);
        }

        free((char *) hv);
        more_amsg = gprompt($P{btuser more above});
        more_bmsg = gprompt($P{btuser more below});
}

/* Fill up the screen.  */

void  ldisplay(BtuserRef ulist, int start)
{
        int     row, endl, i;
        BtuserRef       up;

        if  (!hok)  {
                wclear(lscr);
                wmove(hlscr, Defline, P_P);
                wclrtoeol(hlscr);
                wn_fill(hlscr, Defline, &wn_ddp, (LONG) Btuhdr.btd_defp);
                wn_fill(hlscr, Defline, &wn_dlp, (LONG) Btuhdr.btd_minp);
                wn_fill(hlscr, Defline, &wn_dhp, (LONG) Btuhdr.btd_maxp);
                wn_fill(hlscr, Defline, &wn_dmaxll, (LONG) Btuhdr.btd_maxll);
                wn_fill(hlscr, Defline, &wn_dtotll, (LONG) Btuhdr.btd_totll);
                wn_fill(hlscr, Defline, &wn_dspecll, (LONG) Btuhdr.btd_spec_ll);
                if  (Btuhdr.btd_priv)  {
                        struct  perm    **ppp, *pp;
                        int     h = 0;

                        wmove(hlscr, Defline, P_P);
                        for  (ppp = plist; (pp = *ppp);  ppp++)
                                if  (pp->flg & Btuhdr.btd_priv)  {
                                        if  (h)
                                                waddch(hlscr, '|');
                                        waddstr(hlscr, pp->abbrev);
                                        h++;
                                }
                        }

#ifdef HAVE_TERMINFO
                wnoutrefresh(hlscr);
#else
                wrefresh(hlscr);
#endif
                hok = 1;
        }
        else
#ifdef  OS_DYNIX
                wclear(lscr);
#else
                werase(lscr);
#endif

        more_above = 0;
        row = 0;

        if  (start > 0) {
                wstandout(lscr);
                mvwprintw(lscr, row, (COLS-(int) strlen(more_amsg))/2, more_amsg, start);
                wstandend(lscr);
                row++;
                more_above = 1;
        }

        more_below = 0;
        endl = LLINES;

        if  ((int) Npwusers - start > endl - row)  {
                more_below = 1;
                wstandout(lscr);
                mvwprintw(lscr, endl-1, (COLS - (int) strlen(more_bmsg))/2, more_bmsg, (int) Npwusers + row - start - endl + 1);
                wstandend(lscr);
                endl--;
        }

        for  (i = start;  i < (int) Npwusers  &&  row < endl;  row++, i++)  {
                up = &ulist[i];
                mvwaddstr(lscr, row, USNAM_P, prin_uname((uid_t) up->btu_user));
                mvwaddstr(lscr, row, GRPNAM_P, prin_gname((gid_t) lastgid));
                wn_fill(lscr, row, &wn_udp, (LONG) up->btu_defp);
                wn_fill(lscr, row, &wn_ulp, (LONG) up->btu_minp);
                wn_fill(lscr, row, &wn_uhp, (LONG) up->btu_maxp);
                wn_fill(lscr, row, &wn_umaxll, (LONG) up->btu_maxll);
                wn_fill(lscr, row, &wn_utotll, (LONG) up->btu_totll);
                wn_fill(lscr, row, &wn_uspecll, (LONG) up->btu_spec_ll);

                if  (up->btu_priv)  {
                        struct  perm    **ppp, *pp;
                        int     h = 0;

                        wmove(lscr, row, P_P);
                        for  (ppp = plist; (pp = *ppp);  ppp++)
                                if  (pp->flg & up->btu_priv)  {
                                        if  (h)
                                                waddch(lscr, '|');
                                        waddstr(lscr, pp->abbrev);
                                        h++;
                                }
                        }
        }
#ifdef  CURSES_OVERLAP_BUG
        touchwin(hlscr);
        wrefresh(hlscr);
        if  (tlscr)  {
                touchwin(tlscr);
                wrefresh(tlscr);
        }
        touchwin(lscr);
#endif
}

static void  user_macro(BtuserRef ulist, const int up, const int num)
{
        char    *prompt = helpprmpt(num + $P{Job or User macro}), *str;
        static  char    *execprog;
        PIDTYPE pid;
        int     status, refreshscr = 0;
#ifdef  HAVE_TERMIOS_H
        struct  termios save;
#else
        struct  termio  save;
#endif

        if  (!prompt)  {
                disp_arg[0] = num;
                doerror(lscr, $E{Macro error});
                return;
        }
        if  (!execprog)
                execprog = envprocess(EXECPROG);

        str = prompt;
        if  (*str == '!')  {
                str++;
                refreshscr++;
        }

        if  (num == 0)  {
                int     usy, usx;
                struct  sctrl   dd;
                wclrtoeol(lscr);
                waddstr(lscr, str);
                getyx(lscr, usy, usx);
                dd.helpcode = $H{Job or User macro};
                dd.helpfn = HELPLESS;
                dd.size = COLS - usx;
                dd.col = usx;
                dd.magic_p = MAG_P|MAG_OK;
                dd.min = dd.vmax = 0;
                dd.msg = (char *) 0;
                str = wgets(lscr, usy, &dd, "");
                if  (!str || str[0] == '\0')  {
                        free(prompt);
                        return;
                }
                if  (*str == '!')  {
                        str++;
                        refreshscr++;
                }
        }

        if  (refreshscr)  {
#ifdef  HAVE_TERMIOS_H
                tcgetattr(0, &save);
                tcsetattr(0, TCSADRAIN, &orig_term);
#else
                ioctl(0, TCGETA, &save);
                ioctl(0, TCSETAW, &orig_term);
#endif
        }

        if  ((pid = fork()) == 0)  {
                char    *argbuf[3];
                argbuf[0] = str;
                argbuf[1] = prin_uname((uid_t) ulist[up].btu_user);
                argbuf[2] = (char *) 0;
                if  (!refreshscr)  {
                        close(0);
                        close(1);
                        close(2);
                        Ignored_error = dup(dup(open("/dev/null", O_RDWR)));
                }
                execv(execprog, argbuf);
                exit(255);
        }
        free(prompt);
        if  (pid < 0)  {
                doerror(lscr, $E{Macro fork failed});
                return;
        }
#ifdef  HAVE_WAITPID
        while  (waitpid(pid, &status, 0) < 0)
                ;
#else
        while  (wait(&status) != pid)
                ;
#endif

        if  (refreshscr)  {
#ifdef  HAVE_TERMIOS_H
                tcsetattr(0, TCSADRAIN, &save);
#else
                ioctl(0, TCSETAW, &save);
#endif
                wrefresh(curscr);
        }

        if  (status != 0)  {
                if  (status & 255)  {
                        disp_arg[0] = status & 255;
                        doerror(lscr, $E{Macro command gave signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror(lscr, $E{Macro command error});
                }
        }
}

/* This accepts input from the screen for the overall list.
   Return 1 if changes made, otherwise 0.  */

int  lprocess(BtuserRef ulist, int readwrite)
{
        int     changes = 0, ch, err_no;
        int     Start, Row, u_p, mstate, incr;
        LONG    num;
        double  fnum;
        BtuserRef       up;
        static  char    *cch;

        Ew = lscr;
        Start = 0;
        Row = 0;
        mstate = readwrite? $SH{btuser interactive state}: $SH{btuser interactive view};

 refill:
        select_state(mstate);
 refill2:
        ldisplay(ulist, Start);

        for  (;;)  {
                u_p = Row - Start + more_above;
                wmove(lscr, u_p, 0);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(lscr);
                doupdate();
#else
                wrefresh(lscr);
#endif
                do  ch = getkey(MAG_A|MAG_P);
                while  (ch == EOF  &&  (hlpscr || escr));

        gotit:
                if  (hlpscr)  {
                        endhe(lscr, &hlpscr);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)
                        endhe(lscr, &escr);

                switch  (ch)  {
                case  EOF:
                        continue;
                default:
                        err_no = $E{btuser list unknown command};
                err:
                        doerror(lscr, err_no);
                        continue;

                case  $K{key help}:
                        dochelp(lscr, mstate);
                        continue;

                case  $K{key refresh}:
                        wrefresh(curscr);
                        continue;

                case  $K{key halt}:
                        clear();
                        return  changes;

                case  $K{key save opts}:
                        propts();
#ifndef CURSES_OVERLAP_BUG
                        touchwin(hlscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hlscr);
#else
                        wrefresh(hlscr);
#endif
                        if  (tlscr)  {
                                touchwin(tlscr);
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(tlscr);
#else
                                wrefresh(tlscr);
#endif
                        }
#endif
                        goto  refill;

                case  $K{key cursor down}:
                        Row++;
                        if  (Row >= (int) Npwusers)  {
                                Row--;
el:                             err_no = $E{btuser list off end};
                                goto  err;
                        }
                        u_p++;
                        if  (u_p >= LLINES - more_below)  {
                                if  (++Start <= 2)
                                        Start++;
                                goto  refill2;
                        }
                        continue;

                case  $K{key cursor up}:
                        if  (Row <= 0)  {
bl:                             err_no = $E{btuser list off beg};
                                goto  err;
                        }
                        Row--;
                        if  (Row < Start)  {
                                Start = Row;
                                if (Start == 1)
                                        Start = 0;
                                goto  refill2;
                        }
                        continue;

                case  $K{key top}:
                        if  (Row != Start  &&  Row != 0)  {
                                Row = Start < 0? 0: Start;
                                continue;
                        }
                        Start = 0;
                        Row = 0;
                        goto  refill2;

                case  $K{key bottom}:
                        incr = Start + LLINES - more_above - more_below - 1;
                        if  (Row < incr  &&  incr < (int) Npwusers - 1)  {
                                Row = incr;
                                continue;
                        }
                        if  ((int) Npwusers > LLINES)  {
                                Start = (int) Npwusers - LLINES + 1;
                                Row = Start + LLINES - 2;
                        }
                        else  {
                                Start = 0;
                                Row = (int) Npwusers - 1;
                        }
                        goto  refill2;

                case  $K{key screen down}:
                        incr = LLINES - more_above - more_below;
                drest:
                        if  (Start + incr >= (int) Npwusers)
                                goto  el;
                        Start += incr;
                        if  (Row < Start)
                                Row = Start;
                        goto  refill2;

                case  $K{key half screen down}:
                        incr = (LLINES - more_above - more_below) / 2;
                        goto  drest;

                case  $K{key screen up}:
                        incr = LLINES - more_above - more_below;
                urest:
                        if  (Start < incr)
                                goto  bl;
                        Start -= incr;
                        if  (Start == 1)
                                Start = 0;
                        incr = LLINES - 2;
                        if  (Start > 0)
                                incr--;
                        if  (Row - Start > incr)
                                Row = Start + incr;
                        goto  refill2;

                case  $K{key half screen up}:
                        incr = (LLINES - more_above - more_below) / 2;
                        goto  urest;

                case  $K{btuser key user def pri}:
                        if  (!readwrite)  {
                readonly:       err_no = $E{btuser read only};
                                goto  err;
                        }
                        up = &ulist[Row];
                        wn_udp.msg = prin_uname((uid_t) up->btu_user);
                        num = wnum(lscr, u_p, &wn_udp, (LONG) up->btu_defp);
                        if  (num > 0L)  {
                                changes++;
                                up->btu_defp = (unsigned char) num;
                        }
                        continue;

                case  $K{btuser key sys def pri}:
                        if  (!readwrite)
                                goto  readonly;
                        num = wnum(hlscr, Defline, &wn_ddp, (LONG) Btuhdr.btd_defp);
                        if  (num > 0L)  {
                                hchanges++;
                                hok = 0;
                                Btuhdr.btd_defp = (unsigned char) num;
                        }
                        continue;

                case  $K{btuser key user min pri}:
                        if  (!readwrite)
                                goto  readonly;
                        up = &ulist[Row];
                        wn_ulp.msg = prin_uname((uid_t) up->btu_user);
                        num = wnum(lscr, u_p, &wn_ulp, (LONG) up->btu_minp);
                        if  (num > 0L)  {
                                changes++;
                                up->btu_minp = (unsigned char) num;
                        }
                        continue;

                case  $K{btuser key def min pri}:
                        if  (!readwrite)
                                goto  readonly;
                        num = wnum(hlscr, Defline, &wn_dlp, (LONG) Btuhdr.btd_minp);
                        if  (num > 0L)  {
                                hchanges++;
                                hok = 0;
                                Btuhdr.btd_minp = (unsigned char) num;
                        }
                        continue;

                case  $K{btuser key user max pri}:
                        if  (!readwrite)
                                goto  readonly;
                        up = &ulist[Row];
                        wn_uhp.msg = prin_uname((uid_t) up->btu_user);
                        num = wnum(lscr, u_p, &wn_uhp, (LONG) up->btu_maxp);
                        if  (num > 0L)  {
                                changes++;
                                up->btu_maxp = (unsigned char) num;
                        }
                        continue;

                case  $K{btuser key def max pri}:
                        if  (!readwrite)
                                goto  readonly;
                        num = wnum(hlscr, Defline, &wn_dhp, (LONG) Btuhdr.btd_maxp);
                        if  (num > 0L)  {
                                hchanges++;
                                hok = 0;
                                Btuhdr.btd_maxp = (unsigned char) num;
                        }
                        continue;

                case  $K{btuser key user max ll}:
                        if  (!readwrite)
                                goto  readonly;
                        up = &ulist[Row];
                        wn_umaxll.msg = prin_uname((uid_t) up->btu_user);
                        num = wnum(lscr, u_p, &wn_umaxll, (LONG) up->btu_maxll);
                        if  (num > 0L)  {
                                changes++;
                                up->btu_maxll = (USHORT) num;
                        }
                        continue;

                case  $K{btuser key def max ll}:
                        if  (!readwrite)
                                goto  readonly;
                        num = wnum(hlscr, Defline, &wn_dmaxll, (LONG) Btuhdr.btd_maxll);
                        if  (num > 0L)  {
                                hchanges++;
                                hok = 0;
                                Btuhdr.btd_maxll = (USHORT) num;
                        }
                        continue;

                case  $K{btuser key user tot ll}:
                        if  (!readwrite)
                                goto  readonly;
                        up = &ulist[Row];
                        wn_utotll.msg = prin_uname((uid_t) up->btu_user);
                        num = wnum(lscr, u_p, &wn_utotll, (LONG) up->btu_totll);
                        if  (num > 0L)  {
                                changes++;
                                up->btu_totll = (USHORT) num;
                        }
                        continue;

                case  $K{btuser key def tot ll}:
                        if  (!readwrite)
                                goto  readonly;
                        num = wnum(hlscr, Defline, &wn_dtotll, (LONG) Btuhdr.btd_totll);
                        if  (num > 0L)  {
                                hchanges++;
                                hok = 0;
                                Btuhdr.btd_totll = (USHORT) num;
                        }
                        continue;

                case  $K{btuser key user spec ll}:
                        if  (!readwrite)
                                goto  readonly;
                        up = &ulist[Row];
                        wn_uspecll.msg = prin_uname((uid_t) up->btu_user);
                        num = wnum(lscr, u_p, &wn_uspecll, (LONG) up->btu_spec_ll);
                        if  (num > 0L)  {
                                changes++;
                                up->btu_spec_ll = (USHORT) num;
                        }
                        continue;

                case  $K{btuser key def spec ll}:
                        if  (!readwrite)
                                goto  readonly;
                        num = wnum(hlscr, Defline, &wn_dspecll, (LONG) Btuhdr.btd_spec_ll);
                        if  (num > 0L)  {
                                hchanges++;
                                hok = 0;
                                Btuhdr.btd_spec_ll = (USHORT) num;
                        }
                        continue;

                case  $K{btuser key user modes}:
                        up = &ulist[Row];
                        changes += mprocess(up, readwrite, 1);
#ifndef CURSES_OVERLAP_BUG
                        touchwin(hlscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hlscr);
#else
                        wrefresh(hlscr);
#endif
                        if  (tlscr)  {
                                touchwin(tlscr);
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(tlscr);
#else
                                wrefresh(tlscr);
#endif
                        }
#endif
                        goto  refill;

                case  $K{btuser key def modes}:
                        ch = mprocess(ulist, readwrite, 0);
                        if  (tlscr)  {
                                touchwin(tlscr);
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(tlscr);
#else
                                wrefresh(tlscr);
#endif
                        }
                        touchwin(hlscr);
                        if  (ch)  {
                                hchanges++;
                                hok = 0;
                        }
                        else   {
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(hlscr);
#else
                                wrefresh(hlscr);
#endif
                        }
                        goto  refill;

                case  $K{btuser key user priv}:
                        up = &ulist[Row];
                        changes += pprocess(up, readwrite, 1);
                        touchwin(hlscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hlscr);
#else
                        wrefresh(hlscr);
#endif
                        if  (tlscr)  {
                                touchwin(tlscr);
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(tlscr);
#else
                                wrefresh(tlscr);
#endif
                        }
                        goto  refill;

                case  $K{btuser key def priv}:
                        ch = pprocess(ulist, readwrite, 0);
                        if  (tlscr)  {
                                touchwin(tlscr);
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(tlscr);
#else
                                wrefresh(tlscr);
#endif
                        }
                        touchwin(hlscr);
                        if  (ch)  {
                                hchanges++;
                                hok = 0;
                        }
                        else  {
#ifdef  HAVE_TERMINFO
                                wnoutrefresh(hlscr);
#else
                                wrefresh(hlscr);
#endif
                        }
                        goto  refill;

                case  $K{btuser key copy user}:
                        if  (!readwrite)
                                goto  readonly;
                        copyu(&ulist[Row]);
                        changes++;
                        goto  refill2;

                case  $K{btuser key copy all}:
                        if  (!readwrite)
                                goto  readonly;
                        for  (ch = 0;  ch < (int) Npwusers;  ch++)
                                copyu(&ulist[ch]);
                        changes++;
                        goto  refill2;

                case  $K{btuser key charge}:
                        if  (!readwrite)
                                goto  readonly;
                        up = &ulist[Row];
                        fnum = 0;
                        if  (!cch)
                                cch = gprompt($P{Btuser charge is});
                        mvwprintw(lscr, u_p, 0, cch, prin_uname((uid_t) up->btu_user), fnum);
                        wrefresh(lscr);
                        ch = getkey(MAG_A|MAG_P);
                        if  (Dispflags & DF_HELPCLR)
                                goto  refill2;
                        ldisplay(ulist, Start);
                        wmove(lscr, u_p, 0);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(lscr);
                        doupdate();
#else
                        wrefresh(lscr);
#endif
                        goto  gotit;

                case  $K{btuser key exec}:    case  $K{btuser key exec} + 1:case  $K{btuser key exec} + 2:
                case  $K{btuser key exec} + 3:case  $K{btuser key exec} + 4:case  $K{btuser key exec} + 5:
                case  $K{btuser key exec} + 6:case  $K{btuser key exec} + 7:case  $K{btuser key exec} + 8:
                case  $K{btuser key exec} + 9:
                        user_macro(ulist, Row, ch - $K{btuser key exec});
                        ldisplay(ulist, Start);
                        if  (escr)  {
                                touchwin(escr);
                                wrefresh(escr);
                        }
                        continue;

                case  $K{key search forward}:
                case  $K{key search backward}:
                        if  ((err_no = dosearch(ulist, ch == $K{key search backward}, &Start, &Row)) != 0)  {
                                doerror(lscr, err_no);
                                ldisplay(ulist, Start);
                                touchwin(escr);
                                wrefresh(escr);
                                continue;
                        }
                        goto  refill2;
                }
        }
}

/* Basic display-only.  */

void  display_info(BtuserRef item)
{
        char    *bs_mdu[3], *bs_mdfsep, *bs_mdssep, *z;
        char    *mj, *mv;
        struct  mode    **mpp, *mp;

        disp_arg[0] = item->btu_user;
        disp_arg[1] = Realgid;
        disp_arg[2] = item->btu_minp;
        disp_arg[3] = item->btu_maxp;
        disp_arg[4] = item->btu_defp;
        disp_arg[5] = item->btu_maxll;
        disp_arg[6] = item->btu_totll;
        disp_arg[7] = item->btu_spec_ll;
        /* disp_float = 0;  Was charge now not used */
        fprint_error(stdout, $E{btuser simple display});

        if  (item->btu_priv)  {
                struct  perm    **ppp, *pp;
                char    *pf, *pn;

                pf = gprompt($P{btuser privilege header});
                pn = gprompt($P{btuser privilege next});
                z = pf;

                for  (ppp = plist; (pp = *ppp);  ppp++)
                        if  (pp->flg & item->btu_priv)  {
                                printf("%s%s", z, pp->string);
                                z = pn;
                        }
                putchar('\n');
                free(pf);
                free(pn);
        }

        mj = gprompt($P{btuser disp mode jobs});
        mv = gprompt($P{btuser disp mode vars});
        bs_mdu[0] = gprompt($P{btuser disp mode user});
        bs_mdu[1] = gprompt($P{btuser disp mode group});
        bs_mdu[2] = gprompt($P{btuser disp mode others});
        bs_mdfsep = gprompt($P{btuser disp mode sep1});
        bs_mdssep = gprompt($P{btuser disp mode sep2});

        for  (mpp = mlist;  (mp = *mpp);  mpp++)  {
                int     i;
                unsigned  fl = 0;

                for  (i = 0;  i < 3;  i++)
                        fl |= item->btu_jflags[i];

                if  (!(mp->flg & fl))
                        continue;

                z = bs_mdfsep;
                printf("%s%s", mj, mp->string);

                for  (i = 0;  i < 3;  i++)
                        if  (item->btu_jflags[i] & mp->flg)  {
                                printf("%s%s", z, bs_mdu[i]);
                                z = bs_mdssep;
                        }

                putchar('\n');
        }

        for  (mpp = mlist;  (mp = *mpp);  mpp++)  {
                int     i;
                unsigned  fl = 0;

                for  (i = 0;  i < 3;  i++)
                        fl |= item->btu_vflags[i];

                if  (!(mp->flg & fl))
                        continue;

                z = bs_mdfsep;
                printf("%s%s", mv, mp->string);

                for  (i = 0;  i < 3;  i++)
                        if  (item->btu_vflags[i] & mp->flg)  {
                                printf("%s%s", z, bs_mdu[i]);
                                z = bs_mdssep;
                        }

                putchar('\n');
        }
}

int  sort_u(BtuserRef a, BtuserRef b)
{
        return  strcmp(prin_uname((uid_t) a->btu_user), prin_uname((uid_t) b->btu_user));
}

int  sort_g(BtuserRef a, BtuserRef b)
{
        gid_t   ga, gb;
        char    *au, *bu;

        au = prin_uname((uid_t) a->btu_user);           /*  Sets lastgid */
        ga = lastgid;
        bu = prin_uname((uid_t) b->btu_user);
        gb = lastgid;
        if  (ga == gb)
                return  strcmp(au, bu);
        return  strcmp(prin_gname((gid_t) ga), prin_gname((gid_t) gb));
}

int  sort_id(BtuserRef a, BtuserRef b)
{
        return  (ULONG) a->btu_user > (ULONG) b->btu_user? 1: (ULONG) a->btu_user < (ULONG) b->btu_user? -1: 0;
}

OPTION(o_explain)
{
        print_error($E{btuser explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_display)
{
        getflags = BTU_DISP;
        return  OPTRESULT_OK;
}

OPTION(o_setmodes)
{
        getflags = BTU_UPERM;
        return  OPTRESULT_OK;
}

OPTION(o_view)
{
        getflags = BTU_UREAD;
        return  OPTRESULT_OK;
}

OPTION(o_setall)
{
        getflags = BTU_UUPD;
        return  OPTRESULT_OK;
}

OPTION(o_usort)
{
        alphsort = SRT_USER;
        return  OPTRESULT_OK;
}

OPTION(o_gsort)
{
        alphsort = SRT_GROUP;
        return  OPTRESULT_OK;
}

OPTION(o_nosort)
{
        alphsort = SRT_NONE;
        return  OPTRESULT_OK;
}

DEOPTION(o_helpclr);
DEOPTION(o_nohelpclr);
DEOPTION(o_helpbox);
DEOPTION(o_nohelpbox);
DEOPTION(o_errbox);
DEOPTION(o_noerrbox);

/* Defaults and proc table for arg interp.  */

const   Argdefault  Adefs[] = {
        { '?', $A{btuser arg explain} },
        { 'd', $A{btuser arg display} },
        { 'm', $A{btuser arg setdef mode} },
        { 'v', $A{btuser arg view users} },
        { 'i', $A{btuser arg update users} },
        { 'u', $A{btuser arg sort user} },
        { 'g', $A{btuser arg sort group} },
        { 'n', $A{btuser arg sort uid} },
        { 'h', $A{btuser arg losechar} },
        { 'H', $A{btuser arg keepchar} },
        { 'b', $A{btuser arg help box} },
        { 'B', $A{btuser arg no help box} },
        { 'e', $A{btuser arg error box} },
        { 'E', $A{btuser arg no error box} },
        { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_display,      o_setmodes,     o_view,
o_setall,       o_usort,        o_gsort,        o_nosort,
o_helpclr,      o_nohelpclr,    o_helpbox,      o_nohelpbox,
o_errbox,       o_noerrbox
};

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        BtuserRef       ulist;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile("BTUSERCONF", "btuser.help");
        SCRAMBLID_CHECK
        argv = optprocess(argv, Adefs, optprocs, $A{btuser arg explain}, $A{btuser arg no error box}, 1);
        expcodes();

        SWAP_TO(Daemuid);

        mypriv = getbtuentry(Realuid);          /* Always returns something or bombs */

        switch  (getflags)  {
        default:
        case  BTU_DISP:
                display_info(mypriv);
                return  0;

        case  BTU_UPERM:
                if  (!(mypriv->btu_priv & BTM_UMASK))  {
                        print_error($E{No set default modes});
                        return  E_PERM;
                }
#ifdef  HAVE_TERMIOS_H
                tcgetattr(0, &orig_term);
#else
                ioctl(0, TCGETA, &orig_term);
#endif
                setupkeys();
                screeninit();
#ifdef  HAVE_ATEXIT
                atexit(exit_cleanup);
#endif
                if  (mprocess(mypriv, 1, 1))
                        putbtuentry(mypriv);
                break;

        case  BTU_UREAD:
                if  (!(mypriv->btu_priv & BTM_RADMIN))  {
                        print_error($E{No read admin file priv});
                        return  E_PERM;
                }
                ulist = getbtulist();
                if  (Npwusers == 0)  {
                        print_error($E{btuser no users});
                        return  E_SETUP;
                }
                switch  (alphsort)  {
                case  SRT_USER:
                        qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_u);
                        break;
                case  SRT_GROUP:
                        qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_g);
                        break;
                }
#ifdef  HAVE_TERMIOS_H
                tcgetattr(0, &orig_term);
#else
                ioctl(0, TCGETA, &orig_term);
#endif
                setupkeys();
                screeninit();
                linithdr();
#ifdef  HAVE_ATEXIT
                atexit(exit_cleanup);
#endif
                lprocess(ulist, 0);
                break;

        case  BTU_UUPD:
                if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                        print_error($E{No write admin file priv});
                        exit(E_PERM);
                }
#ifdef  HAVE_TERMIOS_H
                tcgetattr(0, &orig_term);
#else
                ioctl(0, TCGETA, &orig_term);
#endif
                setupkeys();
                screeninit();
                linithdr();
#ifdef  HAVE_ATEXIT
                atexit(exit_cleanup);
#endif
                ulist = getbtulist();
                if  (Npwusers == 0)  {
                        print_error($E{btuser no users});
                        return  E_SETUP;
                }
                switch  (alphsort)  {
                case  SRT_USER:
                        qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_u);
                        break;
                case  SRT_GROUP:
                        qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_g);
                        break;
                }
                if  (lprocess(ulist, 1)  ||  hchanges)  {
                        qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_id);
                        putbtulist(ulist);
                }
                break;
        }

        return  0;
}
