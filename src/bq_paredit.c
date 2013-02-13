/* bq_paredit.c -- job parameter editing for gbch-q

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
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "files.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "helpalt.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "optflags.h"

#ifndef _NFILE
#define _NFILE  64
#endif

#define ARGV_P          4
#define ENVN_P          10
#define ENVV_P          4
#define ABUFF_P         4
#define FD1_P           4
#define STDIN_P1        8
#define ACTION_P        20

#define ENVLINES        5
#define REDIRLINES      3

#define DIRLINE         1
#define UMASKLINE       5
#define ULIMITLINE      7
#define NEXITLINE       9
#define EEXITLINE       11
#define EADVLINE        13
#define EXPORTLINE      15
#define DELTLINE        17
#define RUNTLINE        18
#define SIGTLINE        19

#define HELPLESS        ((char **(*)()) 0)

static  struct  sctrl
        wst_val = { $H{btq job arg value}, HELPLESS, 74, 0, ARGV_P+1, MAG_OK, 0L, 0L, (char *) 0 },
        wst_enam = { $H{btq job env name}, HELPLESS, 60, 0, ENVN_P, MAG_OK, 0L, 0L, (char *) 0 },
        wst_eval = { $H{btq job env value}, HELPLESS, 250, 0, ENVV_P, MAG_OK, 0L, 0L, (char *) 0 },
        wst_abuf = { $H{btq job redir fname}, HELPLESS, 250, 0, ABUFF_P, MAG_OK, 0L, 0L, (char *) 0 },
        wst_dir = { $H{btq procp dir}, HELPLESS, 250, 0, 0, MAG_OK, 0L, 0L, (char *) 0 },
        wnt_fd1 = { $H{btq job redir fd1}, HELPLESS, 3, 0, FD1_P, MAG_P, 0L, (LONG) _NFILE-1, (char *) 0 },
        wnt_fd2 = { $H{btq job redir fd2}, HELPLESS, 3, 0, 0, MAG_P, 0L, (LONG) _NFILE-1, (char *) 0 },
        wht_umask = { $H{btq procp umask}, HELPLESS, 3, 0, 0, MAG_P|MAG_OCTAL, 0L, 0777L, (char *) 0 },
        wht_ulimit= { $H{btq procp ulimit}, HELPLESS, 8, 0, 0, MAG_P, 0L, 0x7fffffffL, (char *) 0 },
        wnt_nexit1 = { $H{btq procp nexit}, HELPLESS, 3, 0, 0, MAG_P, 0L, 255L, (char *) 0 },
        wnt_nexit2 = { $H{btq procp nexit}, HELPLESS, 3, 0, 0, MAG_P, 0L, 255L, (char *) 0 },
        wnt_eexit1 = { $H{btq procp eexit}, HELPLESS, 3, 0, 0, MAG_P, 0L, 255L, (char *) 0 },
        wnt_eexit2 = { $H{btq procp eexit}, HELPLESS, 3, 0, 0, MAG_P, 0L, 255L, (char *) 0 },
        wnt_deltime = { $H{btq delete time}, HELPLESS, 5, 0, 0, MAG_P, 0L, 65535L, (char *) 0 },
        wnt_hruntime = { $H{btq run time}, HELPLESS, 2, 0, 0, MAG_P, 0L, 24L, (char *) 0 },
        wnt_mruntime = { $H{btq run time}, HELPLESS, 2, 0, 0, MAG_P, 0L, 59L, (char *) 0 },
        wnt_sruntime = { $H{btq run time}, HELPLESS, 2, 0, 0, MAG_P, 0L, 59L, (char *) 0 },
        wnt_wsig = { $H{btq which signal}, HELPLESS, 2, 0, 0, MAG_P, 0L, (LONG) (NSIG-1), (char *) 0 },
        wnt_mrunon = { $H{btq runon time}, HELPLESS, 3, 0, 0, MAG_P, 0L, 999L, (char *) 0 },
        wnt_srunon = { $H{btq runon time}, HELPLESS, 2, 0, 0, MAG_P, 0L, 59L, (char *) 0 };

extern  WINDOW  *jscr,
                *hlpscr,
                *escr,
                *Ew;

void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  qdojerror(unsigned, BtjobRef);
void  endhe(WINDOW *, WINDOW **);
void  ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
void  wn_fill(WINDOW *, const int, const struct sctrl *, const LONG);
void  wh_fill(WINDOW *, const int, const struct sctrl *, const ULONG);
void  wjmsg(const unsigned, const ULONG);
int  r_max(int, int);
int  rdalts(WINDOW *, int, int, HelpaltRef, int, int);
ULONG  whexnum(WINDOW *, const int, struct sctrl *, const unsigned);
LONG  wnum(WINDOW *, const int, struct sctrl *, const LONG);
const char *qtitle_of(CBtjobRef);
char *wgets(WINDOW *, const int, struct sctrl *, const char *);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);

/* Stuff for editing command arguments.  Use stdscr as we need rather
   a lot of space */

int  editjargs(BtjobRef jp)
{
        int     ch, srow, crow, err_no;
        int     readonly = 0;
        char    moving = 0, copying = 0;
        unsigned  Nargs, Harg, Earg, incr, destpos, sourcepos = 0, retc, argc;
        ULONG   xindx;
        char    **hv, **hvp, *str, *movingmsg, *copyingmsg;
        BtjobRef        bjp;
        char    *argbuf[MAXJARGS];

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return  0;
        }
        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                readonly = 1;

        /* Initialise buffer which gives args */

        for  (Nargs = 0;  Nargs < jp->h.bj_nargs;  Nargs++)
                argbuf[Nargs] = ARG_OF(jp, Nargs);

        /* Get header for job args window, incorporating the job name
           and number.  */

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('P');
        movingmsg = gprompt($P{btq arg moving});
        copyingmsg = gprompt($P{btq arg copying});

        Harg = 0;
        Earg = 0;

 redisp:

        /* Put in heading */

#ifdef  OS_DYNIX
        clear();
#else
        erase();
#endif

        srow = 0;
        for  (hvp = hv;  *hvp;  srow++, hvp++)
                mvwhdrstr(stdscr, srow, 0, *hvp);
        if  (moving)  {
                unsigned  lng = strlen(movingmsg) + 1;
                move(0, COLS - lng);
                standout();
                addstr(movingmsg);
                standend();
        }
        if  (copying)  {
                unsigned  lng = strlen(copyingmsg) + 1;
                move(0, COLS - lng);
                standout();
                addstr(copyingmsg);
                standend();
        }

        for  (argc = Harg, crow = 0;  crow + srow < LINES  &&  argc < Nargs;  crow++, argc++)
                mvprintw(crow+srow, 0, "%3d \'%.74s\'", argc+1, argbuf[argc]);

#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        Ew = stdscr;
        select_state($S{btq cmd args state});
        crow = Earg - Harg;

 svrefresh:

        move(srow+crow, 0);
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
                err_no = $E{btq args unknown command};
        err:
                doerror(stdscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(stdscr, $H{btq cmd args state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  svrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                if  (++Earg >= Nargs)  {
                        Earg--;
                offe:
                        err_no = $E{btq args off end};
                        goto  err;
                }
                crow++;
                if  (crow + srow >= LINES)  {
                        Harg++;
                        goto  redisp;
                }
                goto  svrefresh;

        case  $K{key cursor up}:
                if  (Earg == 0)  {
                offb:
                        err_no = $E{btq args off beginning};
                        goto  err;
                }
                Earg--;
                if  (--crow < 0)  {
                        Harg--;
                        goto  redisp;
                }
                goto  svrefresh;

        case  $K{key screen down}:
                incr = LINES - srow;
        resd:
                if  (Harg + incr >= Nargs)
                        goto  offe;
                Harg += incr;
                if  (Harg > Earg)
                        Earg = Harg;
                goto  redisp;

        case  $K{key half screen down}:
                incr = (LINES - srow) / 2;
                goto  resd;

        case  $K{key screen up}:
                incr = LINES - srow;
        resu:
                if  (Harg == 0)
                        goto  offb;
                Harg = Harg < incr? 0: Harg - incr;
                if  (Earg - Harg >= LINES - srow)
                        Earg = Harg + LINES - srow - 1;
                goto  redisp;

        case  $K{key half screen up}:
                incr = (LINES - srow) / 2;
                goto  resu;

        case  $K{key top}:
                if  (Earg != Harg)  {
                        Earg = Harg;
                        crow = 0;
                        goto  svrefresh;
                }
                else  if  (Harg == 0)
                        goto  nextin;
                Harg = Earg = 0;
                goto  redisp;

        case  $K{key bottom}:
                incr = Harg + LINES - srow - 1;
                if  (incr > Nargs)
                        incr = Nargs? Nargs - 1: 0;
                if  (Earg != incr)  {
                        Earg = incr;
                        crow = Earg - Harg;
                        goto  svrefresh;
                }
                ch = Nargs - LINES + srow;
                incr = ch < 0? 0: ch;
                if  (Harg == incr)
                        goto  nextin;
                Harg = incr;
                Earg = Harg + LINES - srow - 1;
                if  (Earg > Nargs)
                        Earg = Nargs? Nargs - 1: 0;
                goto  redisp;

        case  $K{key abort}:
        case  $K{key halt}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                free(movingmsg);
                free(copyingmsg);
                return  1;

        case  $K{btq key cedit insert}:
                if  (readonly)  {
                rof:
                        err_no = $E{btq args cannot write job};
                        goto  err;
                }
                destpos = Earg;
                if  (moving)
                        goto  mrest;
                goto  irest;

        case  $K{btq key cedit add}:
                if  (readonly)
                        goto  rof;
                if  (Nargs == 0)
                        destpos = 0;
                else
                        destpos = ++Earg;
                if  ((int) (Earg - Harg) >= LINES - srow)
                        Harg = Earg - LINES + srow + 1;
                if  (moving)  {
                        if  (Earg >= Nargs)
                                Earg = Nargs - 1;
                        goto  mrest;
                }
        irest:
                if  (copying)  {
                        str = argbuf[sourcepos];
                        copying = 0;
                }
                else
                        str = "";
        mfin:
                for  (argc = Nargs;  argc > destpos;  argc--)
                        argbuf[argc] = argbuf[argc-1];
                argbuf[destpos] = str;
                Nargs++;
                goto  commit_ch;
        mrest:
                moving = 0;
                if  (sourcepos == destpos)
                        goto  redisp;
                str = argbuf[sourcepos];
                if  (sourcepos < destpos)  {
                        if  (--destpos == sourcepos)
                                goto  redisp;
                        if  (--Earg < Harg)
                                Harg--;
                        for  (argc = sourcepos;  argc < destpos;  argc++)
                                argbuf[argc] = argbuf[argc+1];
                        argbuf[destpos] = str;
                        goto  commit_ch;
                }
                Nargs--;
                for  (argc = sourcepos;  argc < Nargs;  argc++)
                        argbuf[argc] = argbuf[argc+1];
                goto  mfin;

        case  $K{btq key cedit delete}:
                if  (readonly)
                        goto  rof;
                if  (Nargs == 0)  {
                noargs:
                        err_no = $E{btq args no args yet};
                        goto  err;
                }
                Nargs--;
                for  (argc = Earg;  argc < Nargs;  argc++)
                        argbuf[argc] = argbuf[argc+1];
                goto  commit_ch;

        case  $K{btq key cedit move}:
                if  (readonly)
                        goto  rof;
                if  (moving || copying)  {
                        moving = copying = 0;
                        goto  redisp;
                }
                if  (Nargs == 0)
                        goto  noargs;
                moving = 1;
                sourcepos = Earg;
                goto  redisp;

        case  $K{btq key cedit copy}:
                if  (readonly)
                        goto  rof;
                if  (moving || copying)  {
                        moving = copying = 0;
                        goto  redisp;
                }
                if  (Nargs == 0)
                        goto  noargs;
                copying = 1;
                sourcepos = Earg;
                goto  redisp;

        case  $K{btq key cedit edit}:
                if  (readonly)
                        goto  rof;
                if  (Nargs == 0)
                        goto  noargs;
                if  (moving || copying)  {
                        err_no = $E{btq args finish first};
                        goto  err;
                }
                if  (!(str = wgets(stdscr, crow+srow, &wst_val, argbuf[Earg])))
                        goto  svrefresh;
                argbuf[Earg] = str;
        commit_ch:
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                if  (!repackjob(bjp, jp, (char *) 0, (char *) 0, 0, 0, Nargs, (MredirRef) 0, (MenvirRef) 0, argbuf))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(stdscr, $E{Too many job strings});
                        freexbuf(xindx);
                        goto  redisp;
                }
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        freehelp(hv);
                        free(movingmsg);
                        free(copyingmsg);
                        return  -1;
                }
                freexbuf(xindx);
                for  (Nargs = 0;  Nargs < jp->h.bj_nargs;  Nargs++)
                        argbuf[Nargs] = ARG_OF(jp, Nargs);
                if  (Earg >= Nargs  &&  Earg != 0)  {
                        Earg = Nargs - 1;
                        if  ((int) (Earg - Harg) >= LINES - srow)  {
                                ch = (int) Earg - LINES + srow;
                                Harg = ch < 0? 0: ch;
                        }
                }
                goto  redisp;
        }
}

/* Stuff for editing environment variables Use stdscr as we need
   rather a lot of space */

int  editenvir(BtjobRef jp)
{
        int     ch, srow, crow, err_no, maxl, step;
        int     readonly = 0;
        char    moving = 0, alloced = 0;
        unsigned  Nenv, Henv, Eenv, incr, destpos, sourcepos = 0, retc, envc;
        ULONG   xindx;
        char    **hv, **hvp, *str, *movingmsg;
        BtjobRef        bjp;
        Menvir          envs[MAXJENVIR], save;

        /* Not needed but compiler may moan at it otherwise */

        save.e_name = 0;
        save.e_value = 0;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return  0;
        }
        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                readonly = 1;

        /* Initialise buffer which gives envs */

        for  (Nenv = 0;  Nenv < jp->h.bj_nenv;  Nenv++)
                ENV_OF(jp, Nenv, envs[Nenv].e_name, envs[Nenv].e_value);

        /* Get header for job args window, incorporating the job name
           and number.  */

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('Q');
        movingmsg = gprompt($P{btq arg moving});

        Henv = 0;
        Eenv = 0;

 redisp:

        /* Put in heading */

#ifdef  OS_DYNIX
        clear();
#else
        erase();
#endif

        srow = 0;
        for  (hvp = hv;  *hvp;  srow++, hvp++)
                mvwhdrstr(stdscr, srow, 0, *hvp);
        step = (LINES-srow) / ENVLINES;
        maxl = step * ENVLINES + srow;

        if  (moving)  {
                unsigned  lng = strlen(movingmsg) + 1;
                move(0, COLS - lng);
                standout();
                addstr(movingmsg);
                standend();
        }

        for  (envc = Henv, crow = 0;  crow + srow < maxl  &&  envc < Nenv;  crow += ENVLINES, envc++)  {
                mvprintw(crow+srow, 0, "%3d", envc+1);
                mvprintw(crow+srow, ENVN_P, "%.60s", envs[envc].e_name);
                ws_fill(stdscr, crow+srow+1, &wst_eval, envs[envc].e_value);
        }

#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        Ew = stdscr;
        select_state($S{btq job env state});
        crow = (Eenv - Henv) * ENVLINES;

 svrefresh:

        move(srow+crow, 0);
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
                err_no = $E{btq env unknown command};
        err:
                doerror(stdscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(stdscr, $H{btq job env state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  svrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                if  (++Eenv >= Nenv)  {
                        Eenv--;
                offe:
                        err_no = $E{btq env off end};
                        goto  err;
                }
                crow += ENVLINES;
                if  (crow + srow >= maxl)  {
                        Henv++;
                        goto  redisp;
                }
                goto  svrefresh;

        case  $K{key cursor up}:
                if  (Eenv == 0)  {
                offb:
                        err_no = $E{btq env off beginning};
                        goto  err;
                }
                Eenv--;
                crow -= ENVLINES;
                if  (crow < 0)  {
                        Henv--;
                        goto  redisp;
                }
                goto  svrefresh;

        case  $K{key screen down}:
                incr = step;
        resd:
                if  (Henv + incr >= Nenv)
                        goto  offe;
                Henv += incr;
                if  (Henv > Eenv)
                        Eenv = Henv;
                goto  redisp;

        case  $K{key half screen down}:
                incr = step / 2;
                goto  resd;

        case  $K{key screen up}:
                incr = step;
        resu:
                if  (Henv == 0)
                        goto  offb;
                Henv = Henv < incr? 0: Henv - incr;
                if  (Eenv - Henv >= step)
                        Eenv = Henv + step - 1;
                goto  redisp;

        case  $K{key half screen up}:
                incr = step / 2;
                goto  resu;

        case  $K{key top}:
                if  (Eenv != Henv)  {
                        Eenv = Henv;
                        crow = 0;
                        goto  svrefresh;
                }
                else  if  (Henv == 0)
                        goto  nextin;
                Henv = Eenv = 0;
                goto  redisp;

        case  $K{key bottom}:
                incr = Henv + step - 1;
                if  (incr > Nenv)
                        incr = Nenv? Nenv - 1: 0;
                if  (Eenv != incr)  {
                        Eenv = incr;
                        crow = (Eenv - Henv) * ENVLINES;
                        goto  svrefresh;
                }
                ch = Nenv - step;
                incr = ch < 0? 0: ch;
                if  (Henv == incr)
                        goto  nextin;
                Henv = incr;
                Eenv = Henv + step - 1;
                if  (Eenv > Nenv)
                        Eenv = Nenv? Nenv - 1: 0;
                goto  redisp;

        case  $K{key abort}:
        case  $K{key halt}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                free(movingmsg);
                return  1;

        case  $K{btq key cedit insert}:
                if  (readonly)  {
                rof:
                        err_no = $E{btq env cannot write job};
                        goto  err;
                }
                destpos = Eenv;
                if  (moving)
                        goto  mrest;
                goto  irest;

        case  $K{btq key cedit add}:
                if  (readonly)
                        goto  rof;
                if  (Nenv == 0)
                        destpos = 0;
                else
                        destpos = ++Eenv;
                if  ((int) (Eenv - Henv) >= step)
                        Henv = Eenv - step + 1;
                if  (moving)  {
                        if  (Eenv >= Nenv)
                                Eenv = Nenv - 1;
                        goto  mrest;
                }
        irest:
                crow = (destpos - Henv) * ENVLINES;
                if  (crow + srow >= maxl)
                        crow -= ENVLINES;
                for  (ch = 0;  ch < ENVLINES;  ch++)  {
                        move(crow+srow+ch, 0);
                        clrtoeol();
                }
                if  (!(str = wgets(stdscr, crow+srow, &wst_enam, "")))
                        goto  redisp;
                save.e_name = stracpy(str);
                if  (!(str = wgets(stdscr, crow+srow+1, &wst_eval, "")))  {
                        free(save.e_name);
                        goto  redisp;
                }
                save.e_value = stracpy(str);
                alloced = 1;
        mfin:
                for  (envc = Nenv;  envc > destpos;  envc--)
                        envs[envc] = envs[envc-1];
                envs[destpos] = save;
                Nenv++;
                goto  commit_ch;
        mrest:
                moving = 0;
                if  (sourcepos == destpos)
                        goto  redisp;
                save = envs[sourcepos];
                if  (sourcepos < destpos)  {
                        if  (--destpos == sourcepos)
                                goto  redisp;
                        if  (--Eenv < Henv)
                                Henv--;
                        for  (envc = sourcepos;  envc < destpos;  envc++)
                                envs[envc] = envs[envc+1];
                        envs[destpos] = save;
                        goto  commit_ch;
                }
                Nenv--;
                for  (envc = sourcepos;  envc < Nenv;  envc++)
                        envs[envc] = envs[envc+1];
                goto  mfin;

        case  $K{btq key cedit delete}:
                if  (readonly)
                        goto  rof;
                if  (Nenv == 0)  {
                noenv:
                        err_no = $E{btq env no env yet};
                        goto  err;
                }
                Nenv--;
                for  (envc = Eenv;  envc < Nenv;  envc++)
                        envs[envc] = envs[envc+1];
                goto  commit_ch;

        case  $K{btq key cedit move}:
                if  (readonly)
                        goto  rof;
                if  (moving)  {
                        moving = 0;
                        goto  redisp;
                }
                if  (Nenv == 0)
                        goto  noenv;
                moving = 1;
                sourcepos = Eenv;
                goto  redisp;

        case  $K{btq key env name}:
                if  (readonly)
                        goto  rof;
                if  (Nenv == 0)
                        goto  noenv;
                if  (moving)  {
                        err_no = $E{btq env finish first};
                        goto  err;
                }
                if  (!(str = wgets(stdscr, crow+srow, &wst_enam, envs[Eenv].e_name)))
                        goto  svrefresh;
                envs[Eenv].e_name = str;
                goto  commit_ch;

        case  $K{btq key env value}:
                if  (readonly)
                        goto  rof;
                if  (Nenv == 0)
                        goto  noenv;
                if  (moving)  {
                        err_no = $E{btq env finish first};
                        goto  err;
                }
                if  (!(str = wgets(stdscr, crow+srow+1, &wst_eval, envs[Eenv].e_value)))
                        goto  svrefresh;
                envs[Eenv].e_value = str;
        commit_ch:
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                if  (!repackjob(bjp, jp, (char *) 0, (char *) 0, 0, Nenv, 0, (MredirRef) 0, envs, (char **) 0))  {
                        if  (alloced)  {
                                free(save.e_name);
                                free(save.e_value);
                                alloced = 0;
                        }
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(stdscr, $E{Too many job strings});
                        freexbuf(xindx);
                        goto  redisp;
                }
                if  (alloced)  {
                        free(save.e_name);
                        free(save.e_value);
                        alloced = 0;
                }
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        freehelp(hv);
                        free(movingmsg);
                        return  -1;
                }
                freexbuf(xindx);
                for  (Nenv = 0;  Nenv < jp->h.bj_nenv;  Nenv++)
                        ENV_OF(jp, Nenv, envs[Nenv].e_name, envs[Nenv].e_value);

                if  (Eenv >= Nenv  &&  Eenv != 0)  {
                        Eenv = Nenv - 1;
                        if  ((int) (Eenv - Henv) >= step)  {
                                ch = (int) Eenv - step;
                                Henv = ch < 0? 0: ch;
                        }
                }
                goto  redisp;
        }
}

/* Stuff for editing redirections
   Use stdscr as we need rather a lot of space */

int  editredir(BtjobRef jp)
{
        int     ch, srow, crow, err_no, maxl, step;
        int     readonly = 0, stdin_p2;
        char    moving = 0;
        unsigned  Nredir, Hredir, Eredir, incr, destpos, sourcepos = 0, retc, redirc;
        ULONG   xindx;
        int     action, oldaction;
        LONG    num;
        char    **hv, **hvp, *str, *movingmsg;
        HelpaltRef      actnames, stdinnames;
        BtjobRef        bjp;
        Mredir          save;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return  0;
        }
        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                readonly = 1;

        /* Initialise buffer which gives redirs */

        for  (Nredir = 0;  Nredir < jp->h.bj_nredirs;  Nredir++)  {
                RedirRef        rp = REDIR_OF(jp, Nredir);
                Redirs[Nredir].fd = rp->fd;
                if  ((Redirs[Nredir].action = rp->action) >= RD_ACT_CLOSE)
                        Redirs[Nredir].un.arg = rp->arg;
                else
                        Redirs[Nredir].un.buffer = &jp->bj_space[rp->arg];
        }

        /* Get header for redirs window, incorporating the job name
           and number.  */

        if  (!(actnames = helprdalt($Q{Redirection type names})))  {
                disp_arg[9] = $Q{Redirection type names};
        noalt:
                doerror(jscr, $E{Missing alternative code});
                return  0;
        }
        if  (!(stdinnames = helprdalt($Q{stdio file names})))  {
                freealts(actnames);
                disp_arg[9] = $Q{stdio file names};
                goto  noalt;
        }
        stdin_p2  = altlen(actnames) + ACTION_P + 1;
        wnt_fd2.col = (SHORT) stdin_p2;
        stdin_p2 += 4;

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('R');
        movingmsg = gprompt($P{btq arg moving});

        Hredir = 0;
        Eredir = 0;

 redisp:

        /* Put in heading */

#ifdef  OS_DYNIX
        clear();
#else
        erase();
#endif

        srow = 0;
        for  (hvp = hv;  *hvp;  srow++, hvp++)
                mvwhdrstr(stdscr, srow, 0, *hvp);
        step = (LINES-srow) / REDIRLINES;
        maxl = step * REDIRLINES + srow;

        if  (moving)  {
                unsigned  lng = strlen(movingmsg) + 1;
                move(0, COLS - lng);
                standout();
                addstr(movingmsg);
                standend();
        }

        for  (redirc = Hredir, crow = 0;  crow + srow < maxl  &&  redirc < Nredir;  crow += REDIRLINES, redirc++)  {
                MredirRef       rp = &Redirs[redirc];
                mvprintw(crow+srow, 0, "%3d", redirc+1);
                wn_fill(stdscr, crow+srow, &wnt_fd1, (LONG) rp->fd);
                if  (rp->fd <= 2)
                        mvaddstr(crow+srow, STDIN_P1, disp_alt((int) rp->fd, stdinnames));
                mvaddstr(crow+srow, ACTION_P, disp_alt((int) rp->action, actnames));
                if  (rp->action < RD_ACT_CLOSE)
                        ws_fill(stdscr, crow+srow+1, &wst_abuf, rp->un.buffer);
                else  if  (rp->action == RD_ACT_DUP)  {
                        wn_fill(stdscr, crow+srow, &wnt_fd2, (LONG) rp->un.arg);
                        if  (rp->un.arg <= 2)
                                mvaddstr(crow+srow, stdin_p2, disp_alt((int) rp->un.arg, stdinnames));
                }
        }

#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        Ew = stdscr;
        select_state($S{btq job redir state});
        crow = (Eredir - Hredir) * REDIRLINES;

 svrefresh:

        move(srow+crow, 0);
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
                err_no = $E{btq redir unknown command};
        err:
                doerror(stdscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(stdscr, $H{btq job redir state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  svrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                if  (++Eredir >= Nredir)  {
                        Eredir--;
                offe:
                        err_no = $E{btq redir off end of list};
                        goto  err;
                }
                crow += REDIRLINES;
                if  (crow + srow >= maxl)  {
                        Hredir++;
                        goto  redisp;
                }
                goto  svrefresh;

        case  $K{key cursor up}:
                if  (Eredir == 0)  {
                offb:
                        err_no = $E{btq redir off beginning};
                        goto  err;
                }
                Eredir--;
                crow -= REDIRLINES;
                if  (crow < 0)  {
                        Hredir--;
                        goto  redisp;
                }
                goto  svrefresh;

        case  $K{key screen down}:
                incr = step;
        resd:
                if  (Hredir + incr >= Nredir)
                        goto  offe;
                Hredir += incr;
                if  (Hredir > Eredir)
                        Eredir = Hredir;
                goto  redisp;

        case  $K{key half screen down}:
                incr = step / 2;
                goto  resd;

        case  $K{key screen up}:
                incr = step;
        resu:
                if  (Hredir == 0)
                        goto  offb;
                Hredir = Hredir < incr? 0: Hredir - incr;
                if  (Eredir - Hredir >= step)
                        Eredir = Hredir + step - 1;
                goto  redisp;

        case  $K{key half screen up}:
                incr = step / 2;
                goto  resu;

        case  $K{key top}:
                if  (Eredir != Hredir)  {
                        Eredir = Hredir;
                        crow = 0;
                        goto  svrefresh;
                }
                else  if  (Hredir == 0)
                        goto  nextin;
                Hredir = Eredir = 0;
                goto  redisp;

        case  $K{key bottom}:
                incr = Hredir + step - 1;
                if  (incr > Nredir)
                        incr = Nredir? Nredir - 1: 0;
                if  (Eredir != incr)  {
                        Eredir = incr;
                        crow = (Eredir - Hredir) * REDIRLINES;
                        goto  svrefresh;
                }
                ch = Nredir - step;
                incr = ch < 0? 0: ch;
                if  (Hredir == incr)
                        goto  nextin;
                Hredir = incr;
                Eredir = Hredir + step - 1;
                if  (Eredir > Nredir)
                        Eredir = Nredir? Nredir - 1: 0;
                goto  redisp;

        case  $K{key abort}:
        case  $K{key halt}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                free(movingmsg);
                freealts(actnames);
                freealts(stdinnames);
                return  1;

        case  $K{btq key cedit insert}:
                if  (readonly)  {
                rof:
                        err_no = $E{btq redir cannot write job};
                        goto  err;
                }
                destpos = Eredir;
                if  (moving)
                        goto  mrest;
                goto  irest;

        case  $K{btq key cedit add}:
                if  (readonly)
                        goto  rof;
                if  (Nredir == 0)
                        destpos = 0;
                else
                        destpos = ++Eredir;
                if  ((int) (Eredir - Hredir) >= step)
                        Hredir = Eredir - step + 1;
                if  (moving)  {
                        if  (Eredir >= Nredir)
                                Eredir = Nredir - 1;
                        goto  mrest;
                }
        irest:
                crow = (destpos - Hredir) * REDIRLINES;
                if  (crow + srow >= maxl)
                        crow -= REDIRLINES;
                for  (ch = 0;  ch < REDIRLINES;  ch++)  {
                        move(crow+srow+ch, 0);
                        clrtoeol();
                }
                if  ((num = wnum(stdscr, crow+srow, &wnt_fd1, 1L)) < 0L)  {
                        if  (num == -2L)
                                save.fd = 1;
                        else
                                goto  iclear;
                }
                else
                        save.fd = (unsigned  char) num;
                if  (save.fd <= 2)
                        mvaddstr(crow+srow, STDIN_P1, disp_alt((int) save.fd, stdinnames));
                if  ((action = rdalts(stdscr, crow+srow+1, 0, actnames, -1, $Q{Redirection type names})) < 0)
                        goto  iclear;
                move(crow+srow+1, 0);
                clrtoeol();
                mvaddstr(crow+srow, ACTION_P, disp_alt(action, actnames));
                if  ((save.action = (unsigned char) action) < RD_ACT_CLOSE)  {
                        if  (!(str = wgets(stdscr, crow+srow+1, &wst_abuf, "")))
                                goto  iclear;
                        save.un.buffer = str;
                }
                else  if  (action == RD_ACT_DUP)  {
                        int     exist = save.fd-1;
                        if  (exist < 0)
                                exist = save.fd+1;
                        if  ((num = wnum(stdscr, crow+srow, &wnt_fd2, (LONG) exist)) < 0)  {
                                if  (num != -2L)
                                        goto  iclear;
                                save.un.arg = (USHORT) exist;
                        }
                        else
                                save.un.arg = (USHORT) num;
                }
        mfin:
                for  (redirc = Nredir;  redirc > destpos;  redirc--)
                        Redirs[redirc] = Redirs[redirc-1];
                Redirs[destpos] = save;
                Nredir++;
                goto  commit_ch;
        mrest:
                moving = 0;
                if  (sourcepos == destpos)
                        goto  redisp;
                save = Redirs[sourcepos];
                if  (sourcepos < destpos)  {
                        if  (--destpos == sourcepos)
                                goto  redisp;
                        if  (--Eredir < Hredir)
                                Hredir--;
                        for  (redirc = sourcepos;  redirc < destpos;  redirc++)
                                Redirs[redirc] = Redirs[redirc+1];
                        Redirs[destpos] = save;
                        goto  commit_ch;
                }
                Nredir--;
                for  (redirc = sourcepos;  redirc < Nredir;  redirc++)
                        Redirs[redirc] = Redirs[redirc+1];
                goto  mfin;
        iclear:
                if  (Eredir >= Nredir)
                        Eredir = Nredir == 0? 0: Nredir - 1;
                goto  redisp;

        case  $K{btq key cedit delete}:
                if  (readonly)
                        goto  rof;
                if  (Nredir == 0)  {
                noredir:
                        err_no = $E{btq redir no redir yet};
                        goto  err;
                }
                Nredir--;
                for  (redirc = Eredir;  redirc < Nredir;  redirc++)
                        Redirs[redirc] = Redirs[redirc+1];
                goto  commit_ch;

        case  $K{btq key cedit move}:
                if  (readonly)
                        goto  rof;
                if  (moving)  {
                        moving = 0;
                        goto  redisp;
                }
                if  (Nredir == 0)
                        goto  noredir;
                moving = 1;
                sourcepos = Eredir;
                goto  redisp;

        case  $K{btq key cedit redir num}:
                if  (readonly)
                        goto  rof;
                if  (Nredir == 0)
                        goto  noredir;
                if  (moving)  {
                        err_no = $E{btq redir finish first};
                        goto  err;
                }
                if  ((num = wnum(stdscr, crow+srow, &wnt_fd1, (LONG) Redirs[Eredir].fd)) < 0)  {
                        if  (num == -2L)
                                goto  commit_ch;
                        goto  svrefresh;
                }
                Redirs[Eredir].fd = (unsigned char) num;
                goto  commit_ch;

        case  $K{btq key cedit redir act}:
                if  (readonly)
                        goto  rof;
                if  (Nredir == 0)
                        goto  noredir;
                if  (moving)  {
                        err_no = $E{btq redir finish first};
                        goto  err;
                }
                save = Redirs[Eredir];
                if  ((action = rdalts(stdscr, crow+srow+1, 0, actnames, (int) save.action, $H{Redirection type names})) < 0)
                        goto  svrefresh;
                if  (action == save.action)
                        goto  redisp;
                move(crow+srow+1, 0);
                clrtoeol();
                move(crow+srow, ACTION_P);
                clrtoeol();
                mvaddstr(crow+srow, ACTION_P, disp_alt(action, actnames));
                oldaction = save.action;
                save.action = (unsigned char) action;
                if  (action < RD_ACT_CLOSE)  {
                        if  (oldaction < RD_ACT_CLOSE)
                                goto  commit_ch0;
                        save.un.buffer = "";
                }
                else  {
                        if  (oldaction >= RD_ACT_CLOSE)
                                goto  commit_ch0;
                        save.un.arg = save.fd == 0? 1: save.fd-1;
                        wn_fill(stdscr, crow+srow, &wnt_fd2, (LONG) save.un.arg);
                }
                goto  getname;

        case  $K{btq key cedit redir file}:
                if  (readonly)
                        goto  rof;
                if  (Nredir == 0)
                        goto  noredir;
                if  (moving)  {
                        err_no = $E{btq redir finish first};
                        goto  err;
                }
                save = Redirs[Eredir];
                oldaction = save.action;
        getname:
                if  (save.action < RD_ACT_CLOSE)  {
                        if  (!(str = wgets(stdscr, crow+srow+1, &wst_abuf, save.un.buffer)))  {
                                save.action = (unsigned char) oldaction;
                                goto  svrefresh;
                        }
                        save.un.buffer = str;
                }
                else  if  (save.action == RD_ACT_DUP)  {
                        if  ((num = wnum(stdscr, crow+srow, &wnt_fd2, (LONG) save.un.arg)) < 0)  {
                                if  (num == -2L)
                                        goto  commit_ch0;
                                save.action = (unsigned char) oldaction;
                                goto  redisp;
                        }
                        save.un.arg = (USHORT) num;
                }
        commit_ch0:
                Redirs[Eredir] = save;
        commit_ch:
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                if  (!repackjob(bjp, jp, (char *) 0, (char *) 0, Nredir, 0, 0, Redirs, (MenvirRef) 0, (char **) 0))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(stdscr, $E{Too many job strings});
                        freexbuf(xindx);
                        goto  redisp;
                }
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        freehelp(hv);
                        free(movingmsg);
                        freealts(actnames);
                        freealts(stdinnames);
                        return  -1;
                }
                freexbuf(xindx);
                for  (Nredir = 0;  Nredir < jp->h.bj_nredirs;  Nredir++)  {
                        RedirRef        rp = REDIR_OF(jp, Nredir);
                        Redirs[Nredir].fd = rp->fd;
                        if  ((Redirs[Nredir].action = rp->action) >= RD_ACT_CLOSE)
                                Redirs[Nredir].un.arg = rp->arg;
                        else
                                Redirs[Nredir].un.buffer = &jp->bj_space[rp->arg];
                }

                if  (Eredir >= Nredir  &&  Eredir != 0)  {
                        Eredir = Nredir - 1;
                        if  ((int) (Eredir - Hredir) >= step)  {
                                ch = (int) Eredir - step;
                                Hredir = ch < 0? 0: ch;
                        }
                }
                goto  redisp;
        }
}

/* Stuff for editing process environment Use stdscr as we need rather
   a lot of space */

int  editprocenv(BtjobRef jp)
{
        int     ch, err_no, col, srow, etpcol;
        int     readonly = 0;
        LONG    num1, num2, num3;
        ULONG   unum, xindx;
        unsigned  retc;
        char    **hv, **hvp, *str, *expstr = (char *) 0;
        char    *currdir;
        char    *dirprompt, *umaskprompt, *ulimitprompt, *normalprompt, *errorprompt;
        char    *etprmpt, *deltprmpt, *runtprmpt, *wsigprmpt, *runonprmpt;
        HelpaltRef      erradv, exporttyp;
        BtjobRef        bjp;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return  0;
        }
        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                readonly = 1;

        /* Get header for procenv window, incorporating the job name
           and number.  */

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('T');
        dirprompt = gprompt($P{btq procp dir});
        umaskprompt = gprompt($P{btq procp umask});
        ulimitprompt = gprompt($P{btq procp ulimit});
        normalprompt = gprompt($P{btq procp nexit});
        errorprompt = gprompt($P{btq procp eexit});
        if  (!(erradv = helprdalt($Q{btq advance time opts})))  {
                disp_arg[9] = $Q{btq advance time opts};
                doerror(jscr, $E{Missing alternative code});
                return  0;
        }
        etprmpt = gprompt($P{btq export opts});
        if  (!(exporttyp = helprdalt($Q{btq export opts})))  {
                disp_arg[9] = $Q{btq export opts};
                doerror(jscr, $E{Missing alternative code});
                return  0;
        }
        deltprmpt = gprompt($P{btq delete time});
        runtprmpt = gprompt($P{btq run time});
        wsigprmpt = gprompt($P{btq which signal});
        runonprmpt = gprompt($P{btq runon time});
        col = r_max(r_max(r_max(r_max((int) strlen(dirprompt), (int) strlen(umaskprompt)),
                                r_max((int) strlen(ulimitprompt), (int) strlen(normalprompt))),
                          r_max(r_max((int) strlen(errorprompt), (int) strlen(etprmpt)),
                                r_max((int) strlen(deltprmpt), (int) strlen(runtprmpt)))),
                    r_max((int) strlen(wsigprmpt), (int) strlen(runonprmpt)));

        wst_dir.col = (SHORT) col;
        wht_umask.col = (SHORT) col;
        wht_ulimit.col = (SHORT) col;
        wnt_nexit1.col = (SHORT) col;
        wnt_nexit2.col = (SHORT) col + 5;
        wnt_eexit1.col = (SHORT) col;
        wnt_eexit2.col = (SHORT) col + 5;
        wnt_deltime.col = (SHORT) col;
        wnt_hruntime.col = (SHORT) col;
        wnt_mruntime.col = (SHORT) col + 3;
        wnt_sruntime.col = (SHORT) col + 6;
        wnt_wsig.col = (SHORT) col;
        wnt_mrunon.col = (SHORT) (col*2 + 3);
        wnt_srunon.col = (SHORT) wnt_mrunon.col + 4;

 redisp:
        /* Put in heading */

#ifdef  OS_DYNIX
        clear();
#else
        erase();
#endif

        srow = 0;
        for  (hvp = hv;  *hvp;  srow++, hvp++)
                mvwhdrstr(stdscr, srow, 0, *hvp);
        mvaddstr(srow+DIRLINE, 0, dirprompt);
        mvaddstr(srow+UMASKLINE, 0, umaskprompt);
        mvaddstr(srow+ULIMITLINE, 0, ulimitprompt);
        mvaddstr(srow+NEXITLINE, 0, normalprompt);
        mvaddstr(srow+EEXITLINE, 0, errorprompt);
        currdir = jp->h.bj_direct < 0? "/": &jp->bj_space[jp->h.bj_direct];
        ws_fill(stdscr, srow+DIRLINE, &wst_dir, currdir);
        wh_fill(stdscr, srow+UMASKLINE, &wht_umask, (ULONG) jp->h.bj_umask);
        wh_fill(stdscr, srow+ULIMITLINE, &wht_ulimit, (ULONG) jp->h.bj_ulimit);
        wn_fill(stdscr, srow+NEXITLINE, &wnt_nexit1, (LONG) jp->h.bj_exits.nlower);
        wn_fill(stdscr, srow+NEXITLINE, &wnt_nexit2, (LONG) jp->h.bj_exits.nupper);
        wn_fill(stdscr, srow+EEXITLINE, &wnt_eexit1, (LONG) jp->h.bj_exits.elower);
        wn_fill(stdscr, srow+EEXITLINE, &wnt_eexit2, (LONG) jp->h.bj_exits.eupper);
        mvaddstr(srow+EADVLINE, 0, disp_alt(jp->h.bj_jflags & BJ_NOADVIFERR? 1: 0, erradv));
        mvaddstr(srow+EXPORTLINE, 0, etprmpt);
        getyx(stdscr, etpcol, ch);
        addstr(disp_alt((int) ((jp->h.bj_jflags >> 4) & 3), exporttyp));
        mvaddstr(srow+DELTLINE, 0, deltprmpt);
        wn_fill(stdscr, srow+DELTLINE, &wnt_deltime, (LONG) jp->h.bj_deltime);
        mvaddstr(srow+RUNTLINE, 0, runtprmpt);
        wn_fill(stdscr, srow+RUNTLINE, &wnt_hruntime, (LONG) (jp->h.bj_runtime / 3600L));
        wn_fill(stdscr, srow+RUNTLINE, &wnt_mruntime, (LONG) (jp->h.bj_runtime % 3600L) / 60L);
        wn_fill(stdscr, srow+RUNTLINE, &wnt_sruntime, (LONG) (jp->h.bj_runtime % 60L));
        mvaddstr(srow+SIGTLINE, 0, wsigprmpt);
        wn_fill(stdscr, srow+SIGTLINE, &wnt_wsig, (LONG) jp->h.bj_autoksig);
        mvaddstr(srow+SIGTLINE, wnt_wsig.col + 3, runonprmpt);
        wn_fill(stdscr, srow+SIGTLINE, &wnt_mrunon, (LONG) (jp->h.bj_runon / 60));
        wn_fill(stdscr, srow+SIGTLINE, &wnt_srunon, (LONG) (jp->h.bj_runon % 60));
#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        Ew = stdscr;
        select_state($S{btq process environment});

 svrefresh:

        move(LINES-1, COLS-1);
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
                err_no = $E{btq proce unknown command};
        err:
                doerror(stdscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(stdscr, $H{btq process environment});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  svrefresh;

        case  $K{key abort}:
        case  $K{key halt}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                free(dirprompt);
                free(umaskprompt);
                free(ulimitprompt);
                free(normalprompt);
                free(errorprompt);
                free(etprmpt);
                free(deltprmpt);
                free(runtprmpt);
                free(wsigprmpt);
                free(runonprmpt);
                freealts(erradv);
                return  1;

        case  $K{btq pe key directory}:
                if  (readonly)  {
                rof:
                        err_no = $E{btq proce cannot write job};
                        goto  err;
                }
                if  (!(str = wgets(stdscr, srow+DIRLINE, &wst_dir, currdir)))
                        goto  svrefresh;
                if  (strchr(str, '$'))  {
                        char    *restr = envprocess(str);
                        if  (!restr)  {
                                disp_str = str;
                                err_no = $E{btq proce invalid directory name};
                                goto  svrefresh;
                        }
                        expstr = restr;
                }
                if  (strchr(str, '~'))  {
                        char    *restr = unameproc(expstr? expstr: str, currdir, Realuid);
                        if  (expstr)
                                free(expstr);
                        expstr = restr;
                }
                disp_str = str;
                if  (expstr)  {
                        if  (expstr[0] != '/')  {
                                disp_str2 = expstr;
                                doerror(stdscr, $E{btq proce expanded directory not absolute});
                                free(expstr);
                                expstr = (char *) 0;
                                goto  svrefresh;
                        }
                        free(expstr);
                        expstr = (char *) 0;
                }
                else  if  (str[0] != '/')  {
                        doerror(stdscr, $E{btq proce directory not absolute});
                        goto  svrefresh;
                }
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                if  (!repackjob(bjp, jp, str, (char *) 0, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        disp_arg[3] = bjp->h.bj_job;
                        disp_str = qtitle_of(bjp);
                        doerror(stdscr, $E{Too many job strings});
                        freexbuf(xindx);
                        goto  redisp;
                }
        doout:
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        freehelp(hv);
                        free(dirprompt);
                        free(umaskprompt);
                        free(ulimitprompt);
                        free(normalprompt);
                        free(errorprompt);
                        free(etprmpt);
                        free(deltprmpt);
                        free(runtprmpt);
                        free(wsigprmpt);
                        free(runonprmpt);
                        freealts(erradv);
                        return  -1;
                }
                freexbuf(xindx);
                goto  redisp;

        case  $K{btq pe key umask}:
                if  (readonly)
                        goto  rof;
                if  ((unum = whexnum(stdscr, srow+UMASKLINE, &wht_umask, (unsigned) jp->h.bj_umask)) == (ULONG) jp->h.bj_umask)
                        goto  svrefresh;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_umask = (USHORT) unum;
                goto  doout;

        case  $K{btq pe key ulimit}:
                if  (readonly)
                        goto  rof;
                if  ((unum = whexnum(stdscr, srow+ULIMITLINE, &wht_ulimit, (unsigned) jp->h.bj_ulimit)) == (ULONG) jp->h.bj_ulimit)
                        goto  svrefresh;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_ulimit = unum;
                goto  doout;

        case  $K{btq pe key nexit}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = wnum(stdscr, srow+NEXITLINE, &wnt_nexit1, (LONG) jp->h.bj_exits.nlower)) < 0)  {
                        if  (num1 != -2L)
                                goto  svrefresh;
                        num1 = jp->h.bj_exits.nlower;
                }
                if  ((num2 = wnum(stdscr, srow+NEXITLINE, &wnt_nexit2, (LONG) jp->h.bj_exits.nupper)) < 0)  {
                        if  (num2 != -2L)
                                goto  redisp;
                        num2 = jp->h.bj_exits.nupper;
                }
                if  (num1 > num2)  {
                        LONG    tmp = num1;
                        num1 = num2;
                        num2 = tmp;
                }
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_exits.nlower = (unsigned char) num1;
                bjp->h.bj_exits.nupper = (unsigned char) num2;
                goto  doout;

        case  $K{btq pe key eexit}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = wnum(stdscr, srow+EEXITLINE, &wnt_eexit1, (LONG) jp->h.bj_exits.elower)) < 0)  {
                        if  (num1 != -2L)
                                goto  svrefresh;
                        num1 = jp->h.bj_exits.elower;
                }
                if  ((num2 = wnum(stdscr, srow+EEXITLINE, &wnt_eexit2, (LONG) jp->h.bj_exits.eupper)) < 0)  {
                        if  (num2 != -2L)
                                goto  redisp;
                        num2 = jp->h.bj_exits.eupper;
                }
                if  (num1 > num2)  {
                        LONG    tmp = num1;
                        num1 = num2;
                        num2 = tmp;
                }
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_exits.elower = (unsigned char) num1;
                bjp->h.bj_exits.eupper = (unsigned char) num2;
                goto  doout;

        case $K{btq pe key adverr}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = rdalts(stdscr, srow + EADVLINE, 0, erradv, -1, $H{btq advance time opts})) < 0)
                        goto  svrefresh;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                if  (num1)
                        bjp->h.bj_jflags |= BJ_NOADVIFERR;
                else
                        bjp->h.bj_jflags &= ~BJ_NOADVIFERR;
                goto  doout;

        case  $K{btq pe key export}:
                if  (readonly)
                        goto  rof;
                ch = rdalts(stdscr, srow + EXPORTLINE, etpcol, exporttyp, (int) ((jp->h.bj_jflags >> 4) & 3), $H{btq export opts});
                if  (ch < 0)
                        goto  svrefresh;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);
                bjp->h.bj_jflags |= (unsigned char) (ch << 4);
                goto  doout;

        case  $K{btq pe key deltime}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = wnum(stdscr, srow+DELTLINE, &wnt_deltime, (LONG) jp->h.bj_deltime)) < 0)
                        goto  svrefresh;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_deltime = (USHORT) num1;
                goto  doout;

        case  $K{btq pe key runtime}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = wnum(stdscr, srow+RUNTLINE, &wnt_hruntime, (LONG) (jp->h.bj_runtime / 3600L))) < 0)  {
                        if  (num1 != -2L)
                                goto  svrefresh;
                        num1 = jp->h.bj_runtime / 3600L;
                }
                if  ((num2 = wnum(stdscr, srow+RUNTLINE, &wnt_mruntime, (LONG) (jp->h.bj_runtime % 3600L) / 60L)) < 0)  {
                        if  (num2 != -2L)
                                goto  redisp;
                        num2 = jp->h.bj_runtime % 3600L / 60L;
                }
                if  ((num3 = wnum(stdscr, srow+RUNTLINE, &wnt_sruntime, (LONG) (jp->h.bj_runtime % 60L))) < 0)  {
                        if  (num3 != -2L)
                                goto  redisp;
                        num3 = jp->h.bj_runtime % 60L;
                }
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_runtime = (num1 * 60L + num2) * 60L + num3;
                goto  doout;

        case  $K{btq pe key killsig}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = wnum(stdscr, srow+SIGTLINE, &wnt_wsig, (LONG) jp->h.bj_autoksig)) < 0)
                        goto  svrefresh;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_autoksig = (USHORT) num1;
                goto  doout;

        case  $K{btq pe key runon}:
                if  (readonly)
                        goto  rof;
                if  ((num1 = wnum(stdscr, srow+SIGTLINE, &wnt_mrunon, (LONG) (jp->h.bj_runon / 60))) < 0)  {
                        if  (num1 != -2L)
                                goto  svrefresh;
                        num1 = jp->h.bj_runon / 60;
                }
                if  ((num2 = wnum(stdscr, srow+SIGTLINE, &wnt_srunon, (LONG) (jp->h.bj_runon % 60))) < 0)  {
                        if  (num2 != -2L)
                                goto  redisp;
                        num2 = jp->h.bj_runon % 60;
                }
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_runon = (USHORT)(num1 * 60L + num2);
                goto  doout;
        }
}
