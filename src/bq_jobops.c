/* bq_jobops.c -- job operations for gbch-q

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
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btuser.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "shreq.h"
#include "q_shm.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "errnums.h"
#include "statenums.h"
#include "helpalt.h"
#include "jvuprocs.h"
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

void    con_refill(WINDOW *, int, int, BtconRef);
void    dochelp(WINDOW *, int);
void    doerror(WINDOW *, int);
void    endhe(WINDOW *, WINDOW **);
void    msg_error();
int     mode_edit(WINDOW *, int, int, const char *, jobno_t, BtmodeRef);
int     r_max(int, int);
int     val_var(const char *, const unsigned);
int     rdalts(WINDOW *, int, int, HelpaltRef, int, int);
int     getval(WINDOW *, int, int, int, BtconRef);
char    **gen_rvars(char *);
char    **gen_wvars(char *);
char    *wgets(WINDOW *, const int, struct sctrl *, const char *);
void    mvwhdrstr(WINDOW *, const int, const int, const char *);

extern  char    gv_export;

extern  WINDOW  *jscr,
                *escr,
                *hlpscr,
                *Ew;

#define NULLCP          (char *) 0
#define HELPLESS        ((char **(*)()) 0)
#define BOXWID          1

#define VSCRIT_P        1       /* Critical marker */
#define VSNOEQUIV_P     2
#define VSNOWRITE_P     3
#define VSNAME_P        5
#define VSOPER_P        7

static  struct  sctrl
  cj_user = { $H{New job owner}, gen_ulist, 10, 0, 0, MAG_P, 0L, 0L, NULLCP },
  cj_group = { $H{New job group}, gen_glist, 10, 0, 0, MAG_P, 0L, 0L, NULLCP },
  ws_cvname = { $H{Comparison var list}, gen_rvars, BTV_NAME, 0, 0, MAG_P, 0L, 0L, NULLCP },
  ws_svname = { $H{Assignment var list}, gen_wvars, BTV_NAME, 0, VSNAME_P, MAG_P, 0L, 0L, NULLCP };

/* Vector of flags for displaying when to assign jobs.  */

static  struct  flag_ctrl       {
        int     defcode;                /* Code to look up */
        SHORT   defval;                 /* Default if unreadable */
        SHORT   defset;                 /* Negative unset, positive set */
        SHORT   disprev;                /* Display reverse if BJA_REVERSE */
        USHORT  flag;           /* Flag involved */
}  flagctrl[] = {
        { $HN{Assignment start flag help}, 1, 0, 0, BJA_START },
        { $HN{Assignment reverse flag help}, 1, 0, 0, BJA_REVERSE },
        { $HN{Assignment exit flag help}, 1, 0, 1, BJA_OK },
        { $HN{Assignment error flag help}, 1, 0, 1, BJA_ERROR },
        { $HN{Assignment abort flag help}, 1, 0, 1, BJA_ABORT },
        { $HN{Assignment cancel flag help}, -1, 0, 1, BJA_CANCEL }
};

#define NFLAGS  (sizeof(flagctrl) / sizeof(struct flag_ctrl))
#define FLAGCWID        8

extern  Shipc   Oreq;
extern  int     Ctrl_chan;

const char *qtitle_of(CBtjobRef jp)
{
        const   char    *title = title_of(jp);
        const   char    *colp;

        if  (!jobqueue  ||  !(colp = strchr(title, ':')))
                return  title;
        return  colp + 1;
}

/* Send job reference only to scheduler */

void  qwjimsg(const unsigned code, CBtjobRef jp)
{
        Oreq.sh_params.mcode = code;
        Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
        Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), 0) < 0)
                msg_error();
}

/* Send job-type message to scheduler */

void  wjmsg(const unsigned code, const ULONG indx)
{
        Oreq.sh_params.mcode = code;
        Oreq.sh_un.sh_jobindex = indx;
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), 0) < 0)
                msg_error();
}

/* Display job-type error message */

void  qdojerror(unsigned retc, BtjobRef jp)
{
        switch  (retc & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                doerror(jscr, $E{Unexpected sched message});
                return;
        case  JOB_REPLY:
                disp_str = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;
                doerror(jscr, (int) ((retc & ~REQ_TYPE) + $E{Base for scheduler job errors}));
                return;
        case  NET_REPLY:
                disp_str = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;
                doerror(jscr, (int) ((retc & ~REQ_TYPE) + $E{Base for scheduler net errors}));
                return;
        }
}

int  deljob(BtjobRef jp)
{
        static  char    *cnfmsg;
        unsigned        retc;
        WINDOW  *awin;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_DELETE, mypriv->btu_priv))  {
                disp_str = qtitle_of(jp);
                doerror(jscr, $E{btq no delete permission});
                return  0;
        }

        if  (Dispflags & DF_CONFABORT)  {
                int     begy, y, x;
                const   char    *title;

                disp_str = title = qtitle_of(jp);
                disp_arg[0] = jp->h.bj_job;

                getbegyx(jscr, begy, x);
                getyx(jscr, y, x);
                if  (!cnfmsg)
                        cnfmsg = gprompt($P{btq job del state});
                if  (!(awin = newwin(1, 0, begy + y, x)))
                        return  0;
                wprintw(awin, cnfmsg, title);
                wrefresh(awin);
                select_state($S{btq job del state});
                Ew = jscr;

                for  (;;)  {
                        do  x = getkey(MAG_A|MAG_P);
                        while  (x == EOF  &&  (hlpscr || escr));
                        if  (hlpscr)  {
                                endhe(awin, &hlpscr);
                                if  (Dispflags & DF_HELPCLR)
                                        continue;
                        }
                        if  (escr)
                                endhe(awin, &escr);
                        if  (x == $K{key help})  {
                                dochelp(awin, $H{btq job del state});
                                continue;
                        }
                        if  (x == $K{key refresh})  {
                                wrefresh(curscr);
                                wrefresh(awin);
                                continue;
                        }
                        if  (x == $K{Confirm delete OK}  ||  x == $K{Confirm delete cancel})
                                break;
                        doerror(awin, $E{btq job del state});
                }
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                delwin(awin);
                if  (x == $K{Confirm delete cancel})
                        return  -1;
        }

        qwjimsg(J_DELETE, jp);
        if  ((retc = readreply()) != J_OK)  {
                qdojerror(retc, jp);
                return  -1;
        }
        return  1;
}

int  modjob(BtjobRef jp)
{
        int             readwrite;
        unsigned        retc;
        ULONG           xindx;
        BtjobRef        bjp;
        const   char    *title = qtitle_of(jp);
        Btmode          buffm;

        if  (!mpermitted(&jp->h.bj_mode, BTM_RDMODE, mypriv->btu_priv))  {
                disp_str = title;
                doerror(jscr, $E{btq no change mode});
                return  0;
        }
        readwrite = mpermitted(&jp->h.bj_mode, BTM_WRMODE, mypriv->btu_priv);
        buffm = jp->h.bj_mode;
        if  (mode_edit(jscr, readwrite, 1, title, jp->h.bj_job, &buffm) <= 0  ||  !readwrite)
                return  -1;
        bjp = &Xbuffer->Ring[xindx = getxbuf()];
        *bjp = *jp;
        bjp->h.bj_mode = buffm;
        wjmsg(J_CHMOD, xindx);
        if  ((retc = readreply()) != J_OK)  {
                qdojerror(retc, bjp);
                freexbuf(xindx);
                return  -1;
        }
        freexbuf(xindx);
        return  1;
}

int  ownjob(BtjobRef jp)
{
        static  char    *cnfmsg;
        int             begy, y, x;
        int_ugid_t      nu;
        unsigned        retc;
        char            *str;
        const   char    *title = qtitle_of(jp);
        WINDOW  *awin;

        getbegyx(jscr, begy, x);
        getyx(jscr, y, x);
        if  (!cnfmsg)
                cnfmsg = gprompt($P{New job owner});
        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  0;

        wprintw(awin, cnfmsg, title, jp->h.bj_mode.o_user);
        getyx(awin, y, x);
        cj_user.col = (SHORT) x;
        str = wgets(awin, 0, &cj_user, "");
        if  (str == (char *) 0)  {
                delwin(awin);
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  -1;
        }

        delwin(awin);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif

        if  ((nu = lookup_uname(str)) == UNKNOWN_UID)  {
                if  (!isdigit(str[0]))  {
                        doerror(jscr, $E{btq invalid user name});
                        return  -1;
                }
                nu = atol(str);
        }

        Oreq.sh_params.param = nu;
        qwjimsg(J_CHOWN, jp);
        if  ((retc = readreply()) != J_OK)  {
                qdojerror(retc, jp);
                return  -1;
        }
        return  1;
}

int  grpjob(BtjobRef jp)
{
        static  char    *cnfmsg;
        int             begy, y, x;
        int_ugid_t      ng;
        unsigned        retc;
        char            *str;
        const   char    *title = qtitle_of(jp);
        WINDOW  *awin;

        getbegyx(jscr, begy, x);
        getyx(jscr, y, x);
        if  (!cnfmsg)
                cnfmsg = gprompt($P{New job group});
        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  0;
        wprintw(awin, cnfmsg, title, jp->h.bj_mode.o_group);
        getyx(awin, y, x);
        cj_group.col = (SHORT) x;
        str = wgets(awin, 0, &cj_group, "");
        delwin(awin);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        if  (str == (char *) 0)
                return  -1;

        if  ((ng = lookup_gname(str)) == UNKNOWN_GID)  {
                if  (!isdigit(str[0]))  {
                        doerror(jscr, $E{btq invalid group name});
                        return  -1;
                }
                ng = atol(str);
        }

        Oreq.sh_params.param = ng;
        qwjimsg(J_CHGRP, jp);
        if  ((retc = readreply()) != J_OK)  {
                qdojerror(retc, jp);
                return  -1;
        }
        return  1;
}

static int  vconfdel(WINDOW *win, int row, int state, char *name)
{
        int     by, ch;
        char    *cnfmsg;
        WINDOW  *awin;

        if  (!(Dispflags & DF_CONFABORT))
                return  1;

        getbegyx(win, by, ch);
        if  ((awin = newwin(1, COLS-ch-2-BOXWID*2, by + row, ch+BOXWID)) == (WINDOW *) 0)
                return  0;
        cnfmsg = gprompt(state);
        wprintw(awin, cnfmsg, name);
        free(cnfmsg);

        wrefresh(awin);
        select_state(state);
        Ew = jscr;

        for  (;;)  {
                do  ch = getkey(MAG_A|MAG_P);
                while  (ch == EOF  &&  (hlpscr || escr));
                if  (hlpscr)  {
                        endhe(win, &hlpscr);
                        touchwin(awin);
                        wrefresh(awin);
                        if  (Dispflags & DF_HELPCLR)
                                continue;
                }
                if  (escr)  {
                        endhe(win, &escr);
                        touchwin(awin);
                        wrefresh(awin);
                }
                if  (ch == $K{key help})  {
                        dochelp(awin, state);
                        continue;
                }
                if  (ch == $K{key refresh})  {
                        wrefresh(curscr);
                        wrefresh(awin);
                        continue;
                }
                if  (ch == $K{btq delete job var OK}  ||  ch == $K{btq delete job var cancel})
                        break;
                doerror(awin, state);
        }
        delwin(awin);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        if  (ch == $K{btq delete job var cancel})
                return  0;
        return  1;
}

/* Edit condition vars for the job.  Return 1 if screen got mangled.  */

int  editjcvars(BtjobRef jp)
{
        static  HelpaltRef      compnames = (HelpaltRef) 0;
        static  int     compnwid, ccritlen, noelen, unreadlen, totlen;
        static  char    *ccritmark, *noequiv, *unread;
        char    **hv, **hvp;
        int     hrows, hcols, wwidth, wheight, jsy, jsx, jmsy, jmsx;
        int     srow, crow, savecrow, ch, err_no, condn, varind;
        int     readonly = 0, changes = 0;
        unsigned        retc;
        ULONG           xindx;
        BtjobRef        bjp;
        char    *str;
        WINDOW  *cw;
        Jcond           buffcond[MAXCVARS];

        /* Copy job structure to request buffer and set up pointer to
           condition var vector */

        BLOCK_COPY(buffcond, jp->h.bj_conds, sizeof(buffcond));
        gv_export = jp->h.bj_jflags & BJ_EXPORT? 1: 0;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return  0;
        }
        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                readonly = 1;

        /* First time round, read prompts */

        if  (!compnames)  {
                if  (!(compnames = galts(jscr, $Q{Comparison names}, 6)))
                        return  0;
                compnwid = altlen(compnames);
                ccritmark = gprompt($P{Critical condition marker});
                noequiv = gprompt($P{cond no equiv local var});
                unread = gprompt($P{unreadable local var});
                ccritlen = strlen(ccritmark);
                noelen = strlen(noequiv);
                unreadlen = strlen(unread);
                totlen = ccritlen + noelen + unreadlen;
                ws_cvname.col = BOXWID+totlen;
        }

        /* Get header for job conditions window, incorporating the job
           name and number.  */

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('C');

        /* Get window width as maximum of width of title and variable
           name/space/condition names/var length Add one on each
           side for box sides.  */

        count_hv(hv, &hrows, &hcols);
        wwidth = BOXWID * 2 +
                r_max(hcols, totlen + BTV_NAME + 1 + compnwid + 1 + BTC_VALUE + 2);

        wheight = hrows + MAXCVARS + BOXWID*2;

        /* Create sub-window to input job variable condition details */

        getbegyx(jscr, jsy, jsx);
        getmaxyx(jscr, jmsy, jmsx);
        if  (jmsx < wwidth  ||  jmsy < wheight)  {
                freehelp(hv);
                doerror(jscr, $E{btq jcond no space for win});
                return  0;
        }

        if  ((cw = newwin(wheight, wwidth, jsy + (jmsy - wheight) / 2, jsx + (jmsx - wwidth) / 2)) == (WINDOW *) 0)  {
                freehelp(hv);
                doerror(jscr, $E{btq jcond cannot create win});
                return  0;
        }

        crow = 0;

 redisp:

        /* Put in heading */

        srow = BOXWID;
        for  (hvp = hv;  *hvp;  srow++, hvp++)
                mvwhdrstr(cw, srow, BOXWID, *hvp);

        for  (ch = 0;  ch < MAXCVARS;  ch++)  {
                JcondRef  bcr = &buffcond[ch];
                BtvarRef        vp;
                if  (bcr->bjc_compar == C_UNUSED)
                        break;
                vp = &Var_seg.vlist[bcr->bjc_varind].Vent;
                if  (bcr->bjc_iscrit & CCRIT_NORUN)
                        mvwaddstr(cw, srow+ch, BOXWID, ccritmark);
                else  {         /* wclrtoel zaps box */
                        int     cnt;
                        for  (cnt = BOXWID; cnt < BOXWID+ccritlen;  cnt++)
                                mvwaddch(cw, srow+ch, cnt, ' ');
                }
                if  (bcr->bjc_iscrit & CCRIT_NONAVAIL)
                        waddstr(cw, noequiv);
                else  {
                        int     cnt;
                        for  (cnt = 0; cnt < noelen;  cnt++)
                                waddch(cw, ' ');
                }
                if  (bcr->bjc_iscrit & CCRIT_NOPERM)
                        waddstr(cw, unread);
                else  {
                        int     cnt;
                        for  (cnt = 0; cnt < unreadlen;  cnt++)
                                waddch(cw, ' ');
                }
                mvwaddstr(cw, srow+ch, BOXWID+totlen, (char *) VAR_NAME(vp));
                mvwaddstr(cw, srow+ch, BOXWID+totlen+BTV_NAME+1, disp_alt((int) bcr->bjc_compar, compnames));
                con_refill(cw, srow+ch, BOXWID+totlen+BTV_NAME+1+compnwid+1, &bcr->bjc_value);
        }

        Ew = cw;
 cvreset:
        select_state($S{btq add cond state});

 cvrefresh:

#ifdef  HAVE_TERMINFO
        box(cw, 0, 0);
#else
        box(cw, '|', '-');
#endif

        wmove(cw, srow+crow, BOXWID);
        wrefresh(cw);

 nextin:

        do  ch = getkey(MAG_A|MAG_P);
        while  (ch == EOF  &&  (hlpscr || escr));

        if  (hlpscr)  {
                endhe(cw, &hlpscr);
                if  (Dispflags & DF_HELPCLR)
                        goto  nextin;
        }
        if  (escr)
                endhe(cw, &escr);

        switch  (ch)  {
        case  EOF:
                goto  nextin;

        /* Error case - bell character and try again.  */

        default:
                err_no = $E{btq jcond unknown command};
        err:
                doerror(cw, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(cw, $H{btq add cond state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  cvrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                if  (buffcond[crow].bjc_compar == C_UNUSED)  {
                nont:
                        err_no = $E{btq no jconds yet};
                        goto  err;
                }
                crow++;
                if  (crow >= MAXCVARS  ||  buffcond[crow].bjc_compar == C_UNUSED)  {
                        crow--;
                        err_no = $E{btq jcond off end};
                        goto  err;
                }
                goto  cvrefresh;

        case  $K{key cursor up}:
                if  (crow <= 0)  {
                        err_no = $E{btq jcond off beginning};
                        goto  err;
                }
                crow--;
                goto  cvrefresh;

        case  $K{key top}:
                crow = 0;
                goto  cvrefresh;

        case  $K{key bottom}:
                for  (crow = MAXCVARS - 1; crow > 0  &&  buffcond[crow].bjc_compar == C_UNUSED;  crow--)
                        ;
                goto  cvrefresh;

        case  $K{key abort}:
                changes = 0;    /* Kludge it */

        case  $K{key halt}:
                delwin(cw);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                if  (!changes)
                        return  -1;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                BLOCK_COPY(bjp->h.bj_conds, buffcond, sizeof(buffcond));
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        return  -1;
                }
                freexbuf(xindx);
                return  1;

        case  $K{btq key cond add}:
                if  (readonly)  {
                rof:
                        err_no = $E{btq cannot write job};
                        goto  err;
                }

                /* Remember where we were in case of error */

                savecrow = crow;

                /* Find first empty slot */

                for  (crow = 0;  crow < MAXCVARS  &&  buffcond[crow].bjc_compar != C_UNUSED;  crow++)
                        ;

                /* Check for possible overflow.  */

                if  (crow >= MAXCVARS)  {
                        err_no = $E{btq jcond too many conds};
                        crow = savecrow;
                        goto  err;
                }

                if  ((str = wgets(cw, srow+crow, &ws_cvname, "")) == (char *) 0)  {
                        crow = savecrow;
                        goto  cvrefresh;
                }

                if  ((varind = val_var(str, BTM_READ)) < 0)  {
                        wmove(cw, srow+crow, BOXWID+totlen);
                        wclrtoeol(cw);
                        crow = savecrow;
                        doerror(cw, $E{btq jcond invalid variable});
                        goto  cvrefresh;
                }
                disp_str = str;
                reset_state();
                if  ((condn = rdalts(cw, srow+crow, BOXWID+totlen+BTV_NAME+1, compnames, -1, $H{Specify cond new cond})) < 0)  {
                        wmove(cw, srow+crow, BOXWID+totlen);
                        wclrtoeol(cw);
                        crow = savecrow;
                        goto  cvreset;
                }
                touchwin(cw);
                mvwaddstr(cw, srow+crow, BOXWID+totlen+BTV_NAME+1, disp_alt(condn, compnames));
                if  (!getval(cw, srow+crow, BOXWID+totlen+BTV_NAME+1+compnwid+1, 0, &buffcond[crow].bjc_value))  {
                        wmove(cw, srow+crow, BOXWID+totlen);
                        wclrtoeol(cw);
                        crow = savecrow;
                        goto  cvreset;
                }
                select_state($S{btq add cond state});
                buffcond[crow].bjc_compar = (SHORT) condn;
                buffcond[crow].bjc_iscrit = 0;
                buffcond[crow].bjc_varind = vv_ptrs[varind].place;
                if  (crow < MAXCVARS - 1)
                        buffcond[crow+1].bjc_compar = C_UNUSED;
                werase(cw);
                changes++;
                goto  redisp;

        case  $K{btq key cond delete}:
                if  (readonly)
                        goto  rof;
                if  (buffcond[crow].bjc_compar == C_UNUSED)
                        goto  nont;
                if  (!vconfdel(cw, srow+crow, $PHES{btq delete cond state}, Var_seg.vlist[buffcond[crow].bjc_varind].Vent.var_name))  {
                        werase(cw);
                        goto  redisp;
                }
                for  (ch = MAXCVARS-1;  ch > crow  &&  buffcond[ch].bjc_compar == C_UNUSED;  ch--)
                        ;
                if  (ch > crow)
                        buffcond[crow] = buffcond[ch];
                else    if  (crow > 0)
                        crow--;
                buffcond[ch].bjc_compar = C_UNUSED;
                werase(cw);
                changes++;
                goto  redisp;

        case  $K{btq key cond chg var}:
                if  (readonly)
                        goto  rof;
                if  (buffcond[crow].bjc_compar == C_UNUSED)
                        goto  nont;
                if  ((str = wgets(cw, srow+crow, &ws_cvname, Var_seg.vlist[buffcond[crow].bjc_varind].Vent.var_name)) == (char *) 0 || str[0] == '\0')
                        goto  cvrefresh;
                if  ((varind = val_var(str, BTM_READ)) < 0)  {
                        mvwaddstr(cw, srow+crow, BOXWID+totlen, Var_seg.vlist[buffcond[crow].bjc_varind].Vent.var_name);
                        disp_str = str;
                        doerror(cw, $E{btq jcond invalid variable});
                        goto  cvrefresh;
                }
                buffcond[crow].bjc_varind = vv_ptrs[varind].place;
                changes++;
                goto  cvrefresh;

        case  $K{btq key cond chg comp}:
                if  (readonly)
                        goto  rof;
                if  (buffcond[crow].bjc_compar == C_UNUSED)
                        goto  nont;
                reset_state();
                if  ((condn = rdalts(cw, srow+crow, BOXWID+totlen+BTV_NAME+1, compnames, buffcond[crow].bjc_compar, $H{Specify cond upd cond})) < 0)  {
                        select_state($S{btq add cond state});
                        touchwin(cw);
                        wrefresh(cw);
                        goto  nextin;
                }
                buffcond[crow].bjc_compar = (SHORT) condn;
                werase(cw);
                select_state($S{btq add cond state});
                changes++;
                goto  redisp;

        case  $K{btq key cond chg const}:
                if  (readonly)
                        goto  rof;
                if  (buffcond[crow].bjc_compar == C_UNUSED)
                        goto  nont;
                getval(cw, srow+crow, BOXWID+totlen+BTV_NAME+1+compnwid+1, 1, &buffcond[crow].bjc_value);
                select_state($S{btq add cond state});
                werase(cw);
                changes++;
                goto  redisp;

        case  $K{btq key cond tog crit}:
                if  (readonly)
                        goto  rof;
                if  (buffcond[crow].bjc_compar == C_UNUSED)
                        goto  nont;
                if  (buffcond[crow].bjc_iscrit & CCRIT_NORUN)
                        buffcond[crow].bjc_iscrit &= ~CCRIT_NORUN;
                else
                        buffcond[crow].bjc_iscrit |= CCRIT_NORUN;
                changes++;
                goto  redisp;
        }
}

static void  flagset(USHORT *flagp, int row, unsigned flag)
{
        int     col, fc, ch;

        select_state($S{btq ass flag state});
        col = VSNAME_P+BTV_NAME+1;
        for  (fc = 0;  fc < NFLAGS;  col += FLAGCWID, fc++)
                if  (flagctrl[fc].flag == flag)
                        goto  found;

        doerror(stdscr, $E{btq jass scrambled flags});
        return;
 found:
        move(row, col);
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
                goto  found;
        default:
                doerror(stdscr, $E{btq jass flag unknown command});
                goto  found;

        case  $K{key help}:
                dochelp(stdscr, flagctrl[fc].defcode);
                goto  found;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  found;

        case  $K{key halt}:
        case  $K{key eol}:
                return;

        case  $K{key abort}:
                return;

        case  $K{btq key ass flag set}:
                *flagp |= flag;
                return;

        case  $K{btq key ass flag unset}:
                *flagp &= ~flag;
                return;

        case  $K{btq key ass flag toggle}:
                *flagp ^= flag;
                return;
        }
}

/* Same sort of stuff for setting vars.
   Use stdscr as we need rather a lot of space */

int  editjsvars(BtjobRef jp)
{
        static  HelpaltRef      assnames = (HelpaltRef) 0;
        static  int     assnwid;
        static  char    *setyes, *setno, *scritmark, *noequiv, *unwrite;
        int     ch, srow, crow, savecrow, fc, err_no, hrows, assn, varind;
        int     readonly = 0, changes = 0;
        char    **hv, **hvp, *str;
        unsigned        retc;
        JassRef         sp;
        ULONG           xindx;
        BtjobRef        bjp;
        Jass            buffass[MAXSEVARS];

        BLOCK_COPY(buffass, jp->h.bj_asses, sizeof(buffass));
        gv_export = jp->h.bj_jflags & BJ_EXPORT? 1: 0;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot read job});
                return  0;
        }
        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                readonly = 1;

        /* First time round, read prompts */

        if  (!assnames)  {
                if  (!(assnames = galts(jscr, $Q{Assignment names}, 8)))
                        return  0;
                assnwid = altlen(assnames);
                setyes = gprompt($P{Assignment flag set});
                setno = gprompt($P{Assignment flag unset});
                scritmark = gprompt($P{Assignment critical});
                noequiv = gprompt($P{ass no equiv local var});
                unwrite = gprompt($P{unwriteable local var});

                /* Read default flags */

                for  (ch = 0;  ch < NFLAGS;  ch++)
                        if  ((flagctrl[ch].defset = helpnstate(flagctrl[ch].defcode)) == 0)
                                flagctrl[ch].defset = flagctrl[ch].defval;
        }

        /* Get header for job assigns window, incorporating the job
           name and number.  */

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('A');
        count_hv(hv, &hrows, (int *) 0);

        crow = 0;

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

        for  (ch = 0;  ch < MAXSEVARS;  ch++)  {
                unsigned  fval;
                BtvarRef   vp;
                sp = &buffass[ch];
                if  (sp->bja_op == BJA_NONE)
                        break;
                vp = &Var_seg.vlist[sp->bja_varind].Vent;
                if  (sp->bja_iscrit & ACRIT_NORUN)
                        mvaddstr(srow+ch*2, VSCRIT_P, scritmark);
                if  (sp->bja_iscrit & ACRIT_NONAVAIL)
                        mvaddstr(srow+ch*2, VSNOEQUIV_P, noequiv);
                if  (sp->bja_iscrit & ACRIT_NOPERM)
                        mvaddstr(srow+ch*2, VSNOWRITE_P, unwrite);
                mvaddstr(srow+ch*2, VSNAME_P, (char *) VAR_NAME(vp));
                mvaddstr(srow+ch*2+1, VSOPER_P, disp_alt((int) sp->bja_op, assnames));
                if  (sp->bja_op >= BJA_SEXIT)
                        fval = BJA_OK|BJA_ERROR|BJA_ABORT;
                else  {
                        con_refill(stdscr, srow+ch*2+1, VSOPER_P+1+assnwid, &buffass[ch].bja_con);
                        fval = sp->bja_flags;
                }
                for  (fc = 0;  fc < NFLAGS;  fc++)  {
                        move(srow+ch*2, VSNAME_P+BTV_NAME+1+fc*FLAGCWID);
                        if  (flagctrl[fc].flag & fval)  {
                                if  (fval & BJA_REVERSE  &&  flagctrl[fc].disprev)  {
#ifdef  HAVE_TERMINFO
                                        attron(A_REVERSE);
                                        addstr(setyes);
                                        attroff(A_REVERSE);
#else
                                        standout();
                                        addstr(setyes);
                                        standend();
#endif
                                }
                                else
                                        addstr(setyes);
                        }
                        else
                                addstr(setno);
                }
        }

#ifdef  CURSES_OVERLAP_BUG
        touchwin(stdscr);
#endif
        Ew = stdscr;
        select_state($S{btq add ass state});

 svrefresh:

        move(srow+crow*2, 0);
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

        /* Error case - bell character and try again.  */

        default:
                err_no = $E{btq jass unknown command};
        err:
                doerror(stdscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(stdscr, $H{btq add ass state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  svrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                if  (buffass[crow].bja_op == BJA_NONE)  {
                nont:
                        err_no = $E{btq no jass yet};
                        goto  err;
                }
                crow++;
                if  (crow >= MAXSEVARS  ||  buffass[crow].bja_op == BJA_NONE)  {
                        crow--;
                        err_no = $E{btq jass off end};
                        goto  err;
                }
                goto  svrefresh;

        case  $K{key cursor up}:
                if  (crow <= 0)  {
                        err_no = $E{btq jass off beginning};
                        goto  err;
                }
                crow--;
                goto  svrefresh;

        case  $K{key top}:
                crow = 0;
                goto  svrefresh;

        case  $K{key bottom}:
                for  (crow = MAXSEVARS - 1; crow > 0  &&  buffass[crow].bja_op == BJA_NONE;  crow--)
                        ;
                goto  svrefresh;

        case  $K{key abort}:
                changes = 0;    /* Kludge it */

        case  $K{key halt}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                if  (!changes)
                        return  -1;
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                BLOCK_COPY(bjp->h.bj_asses, buffass, sizeof(buffass));
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        return  -1;
                }
                freexbuf(xindx);
                return  1;

        case  $K{btq key ass add}:

                if  (readonly)  {
                rof:
                        err_no = $E{btq cannot write job};
                        goto  err;
                }

                /* Remember where we were in case of error */

                savecrow = crow;

                /* Find first empty slot */

                for  (crow = 0;  crow < MAXSEVARS  &&  buffass[crow].bja_op != BJA_NONE;  crow++)
                        ;

                /* Check for possible overflow.  */

                if  (crow >= MAXSEVARS)  {
                        err_no = $E{btq jass too many};
                        crow = savecrow;
                        goto  err;
                }

                if  ((str = wgets(stdscr, srow+crow*2, &ws_svname, "")) == (char *) 0)  {
                        crow = savecrow;
                        goto  svrefresh;
                }
                disp_str = str;
                if  ((varind = val_var(str, BTM_WRITE)) < 0)  {
                        move(srow+crow*2, 0);
                        clrtoeol();
                        crow = savecrow;
                        doerror(stdscr, $E{btq jass bad variable});
                        goto  svrefresh;
                }
                if  ((assn = rdalts(stdscr, srow+crow*2+1, VSOPER_P, assnames, -1, $H{New assignment op help})) < 0)  {
clrl2:                  move(srow+crow*2, 0);
                        clrtoeol();
                        move(srow+crow*2+1, 0);
                        clrtoeol();
                        crow = savecrow;
                        goto  svrefresh;
                }
                touchwin(stdscr);
                mvaddstr(srow+crow*2+1, VSOPER_P, disp_alt(assn, assnames));
                if  (assn < BJA_SEXIT)  {
                        if  (!getval(stdscr, srow+crow*2+1, VSOPER_P+1+assnwid, 0, &buffass[crow].bja_con))  {
                                select_state($S{btq add ass state});
                                goto  clrl2;
                        }
                }
                else  {
                        buffass[crow].bja_con.const_type = CON_LONG;
                        buffass[crow].bja_con.con_un.con_long = 0L;
                }

                select_state($S{btq add ass state});
                buffass[crow].bja_flags = 0;

                for  (fc = 0;  fc < NFLAGS;  fc++)
                        if  (flagctrl[fc].defset > 0)
                                buffass[crow].bja_flags |= flagctrl[fc].flag;

                buffass[crow].bja_op = (USHORT) assn;
                buffass[crow].bja_varind = vv_ptrs[varind].place;
                buffass[crow].bja_iscrit = 0;
                if  (crow < MAXSEVARS - 1)
                        buffass[crow+1].bja_op = BJA_NONE;
                changes++;
                goto  redisp;

        case  $K{btq key ass delete}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (!vconfdel(stdscr, srow+crow*2, $PHES{btq delete ass state}, Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name))
                        goto  redisp;
                for  (ch = MAXSEVARS-1;  ch > crow  &&  buffass[ch].bja_op == BJA_NONE;  ch--)
                        ;
                if  (ch > crow)
                        buffass[crow] = buffass[ch];
                else    if  (crow > 0)
                        crow--;
                buffass[ch].bja_op = BJA_NONE;
                changes++;
                goto  redisp;

        case  $K{btq key ass chg var}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  ((str = wgets(stdscr, srow+crow*2, &ws_svname, Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name)) == (char *) 0 || str[0] == '\0')
                        goto  svrefresh;
                if  ((varind = val_var(str, BTM_WRITE)) < 0)  {
                        mvaddstr(srow+crow*2, VSNAME_P, Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name);
                        disp_str = str;
                        doerror(stdscr, $E{btq jass bad variable});
                        goto  svrefresh;
                }
                buffass[crow].bja_varind = vv_ptrs[varind].place;
                changes++;
                goto  nextin;

        case  $K{btq key ass chg ass}:
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                if  ((assn = rdalts(stdscr, srow+crow*2+1, VSOPER_P, assnames, (int) buffass[crow].bja_op, $H{Upd assignment op help})) < 0)  {
                        touchwin(stdscr);
                        refresh();
                        goto  nextin;
                }
                buffass[crow].bja_op = (USHORT) assn;
                changes++;
                goto  redisp;

        case  $K{btq key ass chg const}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)  {
                notecerr:
                        err_no = $E{btq not valid for exit codes};
                        goto  err;
                }
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                getval(stdscr, srow+crow*2+1, VSOPER_P+1+assnwid, 1, &buffass[crow].bja_con);
                changes++;
                goto  redisp;

        case  $K{btq key ass flg start}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)
                        goto  notecerr;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                flagset(&buffass[crow].bja_flags, srow+crow*2, BJA_START);
                changes++;
                goto  redisp;

        case  $K{btq key ass flg rev}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)
                        goto  notecerr;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                flagset(&buffass[crow].bja_flags, srow+crow*2, BJA_REVERSE);
                goto  redisp;

        case  $K{btq key ass flg norm}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)
                        goto  notecerr;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                flagset(&buffass[crow].bja_flags, srow+crow*2, BJA_OK);
                changes++;
                goto  redisp;

        case  $K{btq key ass flg err}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)
                        goto  notecerr;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                flagset(&buffass[crow].bja_flags, srow+crow*2, BJA_ERROR);
                changes++;
                goto  redisp;

        case  $K{btq key ass flg abort}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)
                        goto  notecerr;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                flagset(&buffass[crow].bja_flags, srow+crow*2, BJA_ABORT);
                changes++;
                goto  redisp;

        case  $K{btq key ass flg canc}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_op >= BJA_SEXIT)
                        goto  notecerr;
                disp_str = Var_seg.vlist[buffass[crow].bja_varind].Vent.var_name;
                flagset(&buffass[crow].bja_flags, srow+crow*2, BJA_CANCEL);
                changes++;
                goto  redisp;

        case  $K{btq key ass tog crit}:
                if  (readonly)
                        goto  rof;
                if  (buffass[crow].bja_op == BJA_NONE)
                        goto  nont;
                if  (buffass[crow].bja_iscrit & ACRIT_NORUN)
                        buffass[crow].bja_iscrit &= ~ACRIT_NORUN;
                else
                        buffass[crow].bja_iscrit |= ACRIT_NORUN;
                changes++;
                goto  redisp;
        }
}

/* Set mail/write flags for job */

int  editmwflags(BtjobRef jp)
{
        static  char    *mmsg, *wmsg, *setyes, *setno;
        static  int     msgmax, setmax;
        char            **hv, **hvp;
        int             hrows, hcols, wwidth, wheight, jsy, jsx, jmsy, jmsx;
        int             crow, srow, ch, err_no;
        unsigned        wrt, mail;
        unsigned        retc;
        WINDOW          *cw;
        ULONG           xindx;
        BtjobRef        bjp;

        wrt = (jp->h.bj_jflags & BJ_WRT) != 0;
        mail = (jp->h.bj_jflags & BJ_MAIL) != 0;

        /* Can the geyser do it?  */

        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))  {
                doerror(jscr, $E{btq cannot write job});
                return  0;
        }

        /* First time round, read prompts */

        if  (!mmsg)  {
                mmsg = gprompt($P{btq mail msg prompt});
                wmsg = gprompt($P{btq wrt msg prompt});
                setyes = gprompt($P{btq mw msg yes});
                setno = gprompt($P{btq mw msg no});
                msgmax = r_max(strlen(mmsg), strlen(wmsg));
                setmax = r_max(strlen(setyes), strlen(setno));
        }

        /* Get header for job assigns window, incorporating the job
           name and number.  */

        disp_str = qtitle_of(jp);
        disp_arg[0] = jp->h.bj_job;
        hv = helphdr('W');

        /* Get window width as maximum of width of title and mail or
           write/space/flag Add one on each side for box sides.  */

        count_hv(hv, &hrows, &hcols);
        wwidth = BOXWID * 2 + r_max(hcols, msgmax + 1 + setmax);
        wheight = hrows + 2 + BOXWID*2;

        /* Create sub-window to input job variable condition details */

        getbegyx(jscr, jsy, jsx);
        getmaxyx(jscr, jmsy, jmsx);
        if  (jmsx < wwidth  ||  jmsy < wheight)  {
                freehelp(hv);
                doerror(jscr, $E{btq mwflags no space win});
                return  0;
        }

        if  ((cw = newwin(wheight, wwidth, jsy + (jmsy - wheight) / 2, jsx + (jmsx - wwidth) / 2)) == (WINDOW *) 0)  {
                freehelp(hv);
                doerror(jscr, $E{btq mwflags no create win});
                return  0;
        }

        crow = 0;

 redisp:

        /* Put in heading */

        werase(cw);
#ifdef  HAVE_TERMINFO
        box(cw, 0, 0);
#else
        box(cw, '|', '-');
#endif

        srow = BOXWID;
        for  (hvp = hv;  *hvp;  srow++, hvp++)
                mvwhdrstr(cw, srow, BOXWID, *hvp);

        mvwaddstr(cw, srow, BOXWID, mmsg);
        mvwaddstr(cw, srow+1, BOXWID, wmsg);
        mvwaddstr(cw, srow, BOXWID+msgmax+1, mail? setyes: setno);
        mvwaddstr(cw, srow+1, BOXWID+msgmax+1, wrt? setyes: setno);
        Ew = cw;
        select_state($S{btq set message flags});

 mwrefresh:

        wmove(cw, srow+crow, BOXWID+1+msgmax);
        wrefresh(cw);

 nextin:

        do  ch = getkey(MAG_A|MAG_P);
        while  (ch == EOF  &&  (hlpscr || escr));

        if  (hlpscr)  {
                endhe(cw, &hlpscr);
                if  (Dispflags & DF_HELPCLR)
                        goto  nextin;
        }
        if  (escr)
                endhe(cw, &escr);

        switch  (ch)  {
        case  EOF:
                goto  nextin;

        /* Error case - bell character and try again.  */

        default:
                err_no = $E{btq set message flags};
                doerror(cw, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(cw, $H{btq set message flags});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  mwrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
        case  $K{key cursor up}:
                crow = crow? 0: 1;
                goto  mwrefresh;

        case  $K{key top}:
                crow = 0;
                goto  mwrefresh;

        case  $K{key bottom}:
                crow = 1;
                goto  mwrefresh;

        case  $K{key eol}:
                if  (crow == 0)  {
                        crow = 1;
                        goto  mwrefresh;
                }

        case  $K{key halt}:
                delwin(cw);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                bjp = &Xbuffer->Ring[xindx = getxbuf()];
                *bjp = *jp;
                bjp->h.bj_jflags &= ~(BJ_MAIL|BJ_WRT);
                if  (mail)
                        bjp->h.bj_jflags |= BJ_MAIL;
                if  (wrt)
                        bjp->h.bj_jflags |= BJ_WRT;
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)  {
                        qdojerror(retc, bjp);
                        freexbuf(xindx);
                        return  -1;
                }
                freexbuf(xindx);
                return  1;

        case  $K{key abort}:
                delwin(cw);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                freehelp(hv);
                return  -1;

        case  $K{btq key set msg flag}:
                if  (crow)  {
                        wrt = 1;
                        crow = 0;
                }
                else  {
                        mail = 1;
                        crow = 1;
                }
                goto  redisp;

        case  $K{btq key unset msg flag}:
                if  (crow)  {
                        wrt = 0;
                        crow = 0;
                }
                else  {
                        mail = 0;
                        crow = 1;
                }
                goto  redisp;

        case  $K{btq key toggle msg flag}:
                if  (crow)  {
                        wrt = !wrt;
                        crow = 0;
                }
                else  {
                        mail = !mail;
                        crow = 1;
                }
                goto  redisp;
        }
}
