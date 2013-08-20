/* btq.c -- main program for gbch-q

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
#ifdef M88000
#include <unistd.h>
#endif /* M88000 */
#include <sys/types.h>
#include <errno.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <curses.h>
#ifdef  HAVE_TERMIOS_H
#include <termios.h>
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
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "ecodes.h"
#include "errnums.h"
#include "sctrl.h"
#include "files.h"
#include "statenums.h"
#include "helpargs.h"
#include "cfile.h"
#include "magic_ch.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "q_shm.h"
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

#ifdef  TI_GNU_CC_BUG
int     LINES;                  /* Not defined anywhere without extern */
#endif

#define IPC_MODE        0

#define NULLCH          (char *) 0
#define HELPLESS        (char **(*)()) 0

#define LINESV          3       /* Lines per variable */

#define PROC_DONT_CARE  (-1)    /* Nothing set yet */
#define PROC_JOBS       0       /* Jobs screen NB use ! to switch */
#define PROC_VARS       1       /* Vars screen NB use ! to switch */

#define BOXWID  1

void  cjfind();
void  cvfind();
void  initcifile();
void  initmoremsgs();
void  get_vartitle(char **, char **);
int  j_process();
int  v_process();
char *get_jobtitle();
int  getkey(const unsigned);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);

static  char    Filename[] = __FILE__;

char    *spdir,
        *Curr_pwd;

extern  LONG    Const_val;

WINDOW  *hjscr,
        *jscr,
        *hvscr,
        *vscr,
        *tjscr,
        *tvscr,
        *hlpscr,
        *escr,
        *Ew;

char    Win_setup;
#ifdef	HAVE_LIBXML2
char	XML_jobdump = 1;
#else
char    XML_jobdump;
#endif

SHORT   wh_jtitline, wh_v1titline, wh_v2titline;

SHORT   initscreen = PROC_DONT_CARE;

int     hadrfresh;

int     HJLINES,
        JLINES,
        HVLINES,
        VLINES;

int     Ctrl_chan = -1;

Shipc           Oreq;
extern  long    mymtype;

#ifdef  HAVE_TERMIOS_H
struct  termios orig_term;
#else
struct  termio  orig_term;
#endif

/* If we get a message error die appropriately */

void  msg_error()
{
        if  (Win_setup)  {
                int     save_errno = errno; /* In cases endwin clobbers it */
                clear();
                refresh();
                endwinkeys();
                errno = save_errno;
        }
        print_error(errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error});
        Ctrl_chan = -1;
        exit(E_SETUP);
}

/* Send op-type message to scheduler */

void  womsg(unsigned code)
{
        Oreq.sh_params.mcode = code;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq), 0) < 0)
                msg_error();
}

void  exit_cleanup()
{
        if  (Win_setup)  {
                clear();
                refresh();
                endwinkeys();
        }
        if  (Ctrl_chan >= 0)
                womsg(O_LOGOFF);
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

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        if  (Win_setup)  {
                clear();
                printw("%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
                refresh();
                endwinkeys();
                Win_setup = 0;
        }
        else
                fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
#ifndef HAVE_ATEXIT
        exit_cleanup();
#endif
        exit(E_NOMEM);
}

/* Initialise screen for curses */

void  screeninit()
{
        int     i, hflines;
        char    **hvi, **hj, **hv, **hf;
        char    *jtitle, *vtit1, *vtit2;
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
#else
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

        /* Set up windows.
           NB we allow for no headers if we want.  */

        Const_val = helpnstate($N{Initial constant value});
        if  (Const_val <= 0 || Const_val > 999999L)
                Const_val = 1;

        /* Get job title string from job display format, we may want it */

        jtitle = get_jobtitle();
        get_vartitle(&vtit1, &vtit2);
        wh_jtitline = wh_v1titline = wh_v2titline = -1;

        hj = helphdr('J');
        count_hv(hj, &HJLINES, &i);
        hf = helphdr('F');
        count_hv(hf, &hflines, &i);
        JLINES = LINES - HJLINES - hflines;
        if  (hflines > 0)  {
                if  ((tjscr = newwin(hflines, 0, HJLINES+JLINES, 0)) == (WINDOW *) 0)
                        ABORT_NOMEM;
                for  (i = 0, hvi = hf;  *hvi;  i++, hvi++)  {
                        mvwhdrstr(tjscr, i, 0, *hvi);
                        free(*hvi);
                }
                free((char *) hf);
        }
        disp_arg[7] = Const_val;
        hv = helphdr('V');
        count_hv(hv, &HVLINES, &i);
        hf = helphdr('G');
        count_hv(hf, &hflines, &i);
        VLINES = LINES - HVLINES - hflines;
        if  (JLINES <= 0  ||  VLINES <= 0)
                ABORT_NOMEM;
        if  (hflines > 0)  {
                if  ((tvscr = newwin(hflines, 0, HVLINES+VLINES, 0)) == (WINDOW *) 0)
                        ABORT_NOMEM;
                for  (i = 0, hvi = hf;  *hvi;  i++, hvi++)  {
                        mvwhdrstr(tvscr, i, 0, *hvi);
                        free(*hvi);
                }
                free((char *) hf);
        }
        if  (HVLINES > 0)  {
                char    *lin;
                if  ((hvscr = newwin(HVLINES, 0, 0, 0)) == (WINDOW *) 0)
                        ABORT_NOMEM;
                for  (i = 0, hvi = hv;  (lin = *hvi);  i++, hvi++)  {
                        if  (lin[1] == '\0'  &&  (lin[0] == '1' || lin[0] == '2'))  {
                                if  (lin[0] == '1')
                                        mvwaddstr(hvscr, wh_v1titline = (SHORT) i, 0, vtit1);
                                else
                                        mvwaddstr(hvscr, wh_v2titline = (SHORT) i, 0, vtit2);
                        }
                        else
                                mvwhdrstr(hvscr, i, 0, lin);
                        free(lin);
                }
                free((char *) hv);
        }

        if  ((vscr = newwin(VLINES, 0, HVLINES, 0)) == (WINDOW *) 0)
                ABORT_NOMEM;

        /* Round to multiple.
           We allocated the window first to avoid an undefined gap at the bottom */

        VLINES = (VLINES / LINESV) * LINESV;

        if  (HJLINES > 0)  {
                if  ((hjscr = newwin(HJLINES, 0, 0, 0)) == (WINDOW *) 0)
                        ABORT_NOMEM;
                for  (i = 0, hvi = hj;  *hvi;  i++, hvi++)  {
                        if  (hvi[0][0] == 'j'  &&  hvi[0][1] == '\0')
                                mvwaddstr(hjscr, wh_jtitline = (SHORT) i, 0, jtitle);
                        else
                                mvwhdrstr(hjscr, i, 0, *hvi);
                        free(*hvi);
                }
                free((char *) hj);
        }

        free(jtitle);
        free(vtit1);
        free(vtit2);
        jscr = newwin(JLINES, 0, HJLINES, 0);
        Win_setup = 1;
}

/* Pointers to window structures for error/help routines to reinstate
   screen when finished with message.  */

static  WINDOW  *bgwin1, *bgwin2, *bgwin3;

/* Note background window(s) */

void  notebgwin(WINDOW *hwin, WINDOW *win, WINDOW *twin)
{
        bgwin1 = hwin;
        bgwin2 = win;
        bgwin3 = twin;
}

/* Generate help message.  */

void  dohelp(WINDOW *owin, struct sctrl *scp, char *prefix)
{
        char    **hv, **examples = (char **) 0;
        int     hrows, hcols, erows, ecols, cols, rows;
        int     begy, cy, cx, startrow, startcol, i, j, k, l, edrows, edcols;
        int     rpadding, icpadding;

        if  (*(hv = helpvec(scp->helpcode, 'H')) == (char *) 0)  {
                free((char *) hv);
                disp_arg[9] = scp->helpcode;
                hv = helpvec($E{Missing help code}, 'E');
        }

        if  (scp->helpfn)
                examples = (*scp->helpfn)(prefix);

        count_hv(hv, &hrows, &hcols);
        count_hv(examples, &erows, &ecols);
        cols = hcols;
        edcols = 1;
        edrows = erows;
        rpadding = 0;
        icpadding = 1;

        if  (ecols > cols)
                cols = ecols;
        else  if  ((edcols = hcols / (ecols + 1)) <= 0)
                        edcols = 1;
        else  {
                if  (edcols > erows)
                        edcols = erows;
                if  ((i = edcols - 1) > 0)  {
                        edrows = (erows - 1) / edcols + 1;
                        rpadding = cols - edcols * ecols;
                        icpadding = rpadding / i;
                        if  (icpadding > 5)
                                icpadding = 5;
                        rpadding = (cols - ecols * edcols - icpadding * i) / 2;
                }
        }

        rows = hrows + edrows;
        if  (Dispflags & DF_HELPBOX)  {
                rows += 2 * BOXWID;
                cols += 2 * BOXWID;
        }

        if  (rows >= LINES)  {
                edrows -= rows - LINES + 1;
                rows = LINES-1;
        }

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
                        mvwaddstr(hlpscr, i+BOXWID, BOXWID, hv[i]);

                for  (i = 0;  i < edrows;  i++)  {
                        int     ccol = rpadding + BOXWID;
                        wmove(hlpscr, i + hrows + BOXWID, ccol);
                        for  (j = 0;  j < edcols - 1;  j++)  {
                                if  ((k = i + j*edrows) < erows)
                                        waddstr(hlpscr, examples[k]);
                                ccol += ecols + icpadding;
                                wmove(hlpscr, i + hrows + BOXWID, ccol);
                        }
                        if  ((k = i + (edcols - 1) * edrows) < erows)
                                waddstr(hlpscr, examples[k]);
                }
        }
        else  {
                wstandout(hlpscr);

                for  (i = 0;  i < hrows;  i++)  {
                        mvwaddstr(hlpscr, i, 0, hv[i]);
                        for  (l = strlen(hv[i]);  l < cols;  l++)
                                waddch(hlpscr, ' ');
                }

                for  (i = 0;  i < edrows;  i++)  {
                        wmove(hlpscr, i+hrows, 0);
                        for  (cx = 0;  cx < rpadding;  cx++)
                                waddch(hlpscr, ' ');
                        for  (j = 0;;  j++)  {
                                if  ((k = i + j*edrows) < erows)  {
                                        waddstr(hlpscr, examples[k]);
                                        k = strlen(examples[k]);
                                }
                                else
                                        k = 0;
                                for  (;  k < ecols;  k++)
                                        waddch(hlpscr, ' ');
                                if  (j >= edcols - 1)  {
                                        for  (k = (edcols - 1) * (ecols + icpadding) + ecols;  k < cols;  k++)
                                                waddch(hlpscr, ' ');
                                        break;
                                }
                                for  (k = 0;  k < icpadding;  k++)
                                        waddch(hlpscr, ' ');
                        }
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

/* Get rid of a help or error message and restore cursor.  We need to
   work out what we mangled and restore it.  */

void  endhe(WINDOW *owin, WINDOW **wpp)
{
        delwin(*wpp);   /* Byebye */
        *wpp = (WINDOW *) 0;
#ifdef HAVE_TERMINFO
        if  (bgwin1  &&  bgwin1 != owin)  {
                touchwin(bgwin1);
                wnoutrefresh(bgwin1);
        }
        if  (bgwin2  &&  bgwin2 != owin)  {
                touchwin(bgwin2);
                wnoutrefresh(bgwin2);
        }
        if  (bgwin3  &&  bgwin3 != owin)  {
                touchwin(bgwin3);
                wnoutrefresh(bgwin3);
        }
        touchwin(owin);
        wnoutrefresh(owin);
        doupdate();
#else
        if  (bgwin1  &&  bgwin1 != owin)  {
                touchwin(bgwin1);
                wrefresh(bgwin1);
        }
        if  (bgwin2  &&  bgwin2 != owin)  {
                touchwin(bgwin2);
                wrefresh(bgwin2);
        }
        if  (bgwin3  &&  bgwin3 != owin)  {
                touchwin(bgwin3);
                wrefresh(bgwin3);
        }
        touchwin(owin);
        wrefresh(owin);
#endif
}

/* Generate error message avoiding (if possible) current cursor
   position.  */

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
                rows += BOXWID * 2;
                cols += BOXWID * 2;
        }

        /* Silly person might make error messages too big.  */

        if  (cols > COLS)  {
                ecols -= cols - COLS;
                cols = COLS;
        }

        /* Find absolute cursor position and try to create window
           avoiding it. This is done differently from help
           messages - we try to use opposite bit of the screen.  */

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
                        mvwaddstr(escr, i+BOXWID, BOXWID, ev[i]);
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

/* This notes signals from (presumably) the scheduler.  */

RETSIGTYPE  markit(int sig)
{
#ifdef  UNSAFE_SIGNALS
        signal(sig, markit);
#endif

        if  (sig != QRFRESH)  {
#ifndef HAVE_ATEXIT
                exit_cleanup();
#endif
                exit(E_SIGNAL);
        }
        hadrfresh++;
}

/* This accepts input from the screen.  */

void  process(int invl)
{
        int     res;

#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
        z.sighandler_el = markit;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
#else
        /* signal is #defined as sigset on suitable systems */
        signal(QRFRESH, markit);
#endif

        womsg(O_LOGON);
        readreply();

        for  (;;)  {
                do  {
                        hadrfresh = 0;
                        rjobfile(1);
                        rvarlist(1);
                        cjfind();
                        cvfind();
                }  while  (hadrfresh);

                for  (;;)  {
                        if  (invl == PROC_DONT_CARE)  {
                                if  (Job_seg.njobs != 0)
                                        invl = 0;
                                else
                                        invl = 1;
                        }
                        if  (invl)
                                res = v_process();
                        else
                                res = j_process();

                        /* Res = 0, quit, -1 refresh, 1 other screen */

                        if  (res < 0)
                                break;

                        if  (res == 0)  {
#ifndef HAVE_ATEXIT
                                exit_cleanup();
#endif
                                exit(0);
                        }
                        invl = !invl;
                }
        }
}

#define BTQ_INLINE

OPTION(o_explain)
{
        print_error($E{btq explain});
        exit(0);
}

DEOPTION(o_confabort);
DEOPTION(o_noconfabort);

OPTION(o_jfirst)
{
        initscreen = PROC_JOBS;
        return  OPTRESULT_OK;
}

OPTION(o_vfirst)
{
        initscreen = PROC_VARS;
        return  OPTRESULT_OK;
}

OPTION(o_xmlfmt)
{
        XML_jobdump = 1;
        return  OPTRESULT_OK;
}

OPTION(o_noxmlfmt)
{
        XML_jobdump = 0;
        return  OPTRESULT_OK;
}

DEOPTION(o_incnull);
DEOPTION(o_noincnull);
DEOPTION(o_localonly);
DEOPTION(o_nolocalonly);
DEOPTION(o_scrkeep);
DEOPTION(o_noscrkeep);
DEOPTION(o_confabort);
DEOPTION(o_noconfabort);
DEOPTION(o_justu);
DEOPTION(o_justg);
DEOPTION(o_jobqueue);
DEOPTION(o_helpclr);
DEOPTION(o_nohelpclr);
DEOPTION(o_helpbox);
DEOPTION(o_nohelpbox);
DEOPTION(o_errbox);
DEOPTION(o_noerrbox);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
        {  '?', $A{btq arg explain} },
        {  'j', $A{btq arg jobs screen} },
        {  'v', $A{btq arg vars screen} },
        {  'a', $A{btq arg confdel} },
        {  'A', $A{btq arg noconfdel} },
        {  's', $A{btq arg cursor keep} },
        {  'N', $A{btq arg cursor follow} },
        {  'h', $A{btq arg losechar} },
        {  'H', $A{btq arg keepchar} },
        {  'b', $A{btq arg help box} },
        {  'B', $A{btq arg no help hox} },
        {  'e', $A{btq arg error box} },
        {  'E', $A{btq arg no error box} },
        {  'l', $A{btq arg local} },
        {  'r', $A{btq arg network} },
        {  'q', $A{btq arg jobqueue} },
        {  'u', $A{btq arg justu} },
        {  'g', $A{btq arg justg} },
        {  'z', $A{btq arg nullqs} },
        {  'Z', $A{btq arg no nullqs} },
        {  'X', $A{btq arg XML fmt} },
        {  'D', $A{btq arg no XML fmt} },
        { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_confabort,    o_noconfabort,  o_scrkeep,
o_noscrkeep,    o_jfirst,       o_vfirst,       o_nohelpclr,
o_helpclr,      o_helpbox,      o_nohelpbox,    o_errbox,
o_noerrbox,     o_localonly,    o_nolocalonly,  o_jobqueue,
o_justu,        o_justg,        o_incnull,      o_noincnull,
o_xmlfmt,       o_noxmlfmt
};

char    Cvarname[] = "BTQCONF";

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

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
        Cfile = open_cfile(Cvarname, "btq.help");
        SCRAMBLID_CHECK
        tzset();
        Dispflags = DF_CONFABORT; /* Set as default */
        argv = optprocess(argv, Adefs, optprocs, $A{btq arg explain}, $A{btq arg no XML fmt}, 1);

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        SWAP_TO(Daemuid);

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                disp_str = spdir;
                print_error($E{Cannot change directory});
                exit(E_NOCHDIR);
        }

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

#ifdef  HAVE_TERMIOS_H
        tcgetattr(0, &orig_term);
#else
        ioctl(0, TCGETA, &orig_term);
#endif

#ifdef  OS_ARIX

        /* On the ARIX curses breaks if you invoke it after attaching
           shared memory segments.  However doing it this way
           round means that error messages will have a bad format */

        setupkeys();
        screeninit();
#endif

        /* Set up scheduler request main parameters.  */

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        mypriv = getbtuser(Realuid);

        openjfile(1, 0);
        openvfile(1, 0);
        initmoremsgs();
        initxbuffer(1);
        initcifile();
#ifdef  HAVE_ATEXIT
        atexit(exit_cleanup);
#endif

#ifndef OS_ARIX
        setupkeys();
        screeninit();
#endif
        /* Off we go.  */

        process(initscreen);

        exit(0);
}
