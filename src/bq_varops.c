/* bq_varops.c -- var operations for gbch-q

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
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "shreq.h"
#include "errnums.h"
#include "statenums.h"
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

void  con_refill(WINDOW *, int, int, BtconRef);
void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  endhe(WINDOW *, WINDOW **);
void  get_vartitle(char **, char **);
void  msg_error();
int  getval(WINDOW *, int, int, int, BtconRef);
int  mode_edit(WINDOW *, int, int, const char *, jobno_t, BtmodeRef);
LONG  wnum(WINDOW *, const int, struct sctrl *, const LONG);
char *wgets(WINDOW *, const int, struct sctrl *, const char *);
void  mvwhdrstr(WINDOW *, const int, const int, const char *);

#define CWHEIGHT        9
#define BOXWID          1

static  char    Filename[] = __FILE__;

static  char    *crtitle,       /* Create variable title */
                *namep,         /* Name prompt */
                *valuep,        /* Value prompt */
                *commp;         /* Comment prompt */

static  int     cprlen;         /* Max length of above 3 prompts  */

#define NULLCP          (char *) 0
#define HELPLESS        ((char **(*)()) 0)

static  struct  sctrl
  cv_sc = { $H{btq new variable name}, HELPLESS, BTV_NAME, 0, 0, MAG_P|MAG_NAME, 0L, 0L, NULLCP },
  cv_comm = { $H{btq new variable comment}, HELPLESS, BTV_COMMENT, 0, 0, MAG_OK, 0L, 0L, NULLCP },
  cv_user = { $H{btq new var user}, gen_ulist, 10, 0, 0, MAG_P, 0L, 0L, NULLCP },
  cv_group = { $H{btq new var group}, gen_glist, 10, 0, 0, MAG_P, 0L, 0L, NULLCP },
  cv_rename = { $H{btq var rename}, HELPLESS, BTV_NAME, 0, 0, MAG_P|MAG_NAME, 0L, 0L, NULLCP },
  cv_const = { $H{btq arith constant}, HELPLESS, 6, 0, 0, MAG_P, 1L, 999999L, NULLCP };

extern  WINDOW  *hvscr,
                *vscr,
                *Ew,
                *hlpscr,
                *escr;

LONG    Const_val = 1L;

extern  Shipc   Oreq;
extern  int     Ctrl_chan;

#define ppermitted(flg) (mypriv->btu_priv & flg)

/* Send var-type message to scheduler */

void  qwvmsg(unsigned code, BtvarRef vp, ULONG Saveseq)
{
        Oreq.sh_params.mcode = code;
        if  (vp)
                Oreq.sh_un.sh_var = *vp;
        Oreq.sh_un.sh_var.var_sequence = Saveseq;
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), 0) < 0)
                msg_error();
}

/* Display var-type error message */

void  qdoverror(unsigned retc, BtvarRef vp)
{
        switch  (retc  & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                doerror(vscr, $E{Unexpected sched message});
                return;
        case  VAR_REPLY:
                disp_str = vp->var_name;
                doerror(vscr, (int) ((retc & ~REQ_TYPE) + $E{Base for scheduler var errors}));
                return;
        case  NET_REPLY:
                disp_str = vp->var_name;
                doerror(vscr, (int) ((retc & ~REQ_TYPE) + $E{Base for scheduler net errors}));
                return;
        }
}

/* Create a variable.  */

int  createvar()
{
        int     wwid, vsx, vsy, vmsx, vmsy;
        unsigned        retc;
        WINDOW  *cw;
        char    *str;

        /* Can the geyser do it?  */

        if  (!ppermitted(BTM_CREATE))  {
                doerror(vscr, $E{btq vlist cannot create});
                return  0;
        }

        /* First time round, read prompts */

        if  (!crtitle)  {
                int     l;
                crtitle = gprompt($P{btq create new var});
                namep = gprompt($P{btq new variable name});
                valuep = gprompt($P{btq new variable value});
                commp = gprompt($P{btq new variable comment});
                cprlen = strlen(crtitle);
                if  ((l = strlen(valuep)) > cprlen)
                        cprlen = l;
                if  ((l = strlen(commp)) > cprlen)
                        cprlen = l;
        }

        /* Allow for max string size (plus 2 quotes) or comment length
           whichever is the greater. Use C preprocessor to work
           this out.  */

#if     BTC_VALUE + 2 > BTV_COMMENT
        wwid = cprlen + 1 + 2*BOXWID + BTC_VALUE + 2;
#else
        wwid = cprlen + 1 + BTV_COMMENT + 2*BOXWID;
#endif

        /* Create sub-window to input variable creation details */

        getbegyx(vscr, vsy, vsx);
        getmaxyx(vscr, vmsy, vmsx);
        if  (vmsx < wwid  ||  vmsy < CWHEIGHT)  {
                doerror(vscr, $E{btq vlist no space create win});
                return  0;
        }

        if  ((cw = newwin(CWHEIGHT, wwid, vsy + (vmsy - CWHEIGHT) / 2, vsx + (vmsx - wwid) / 2)) == (WINDOW *) 0)  {
                doerror(vscr, $E{btq vlist cannot create win});
                return  0;
        }

#ifdef  HAVE_TERMINFO
        box(cw, 0, 0);
#else
        box(cw, '|', '-');
#endif

        mvwaddstr(cw, BOXWID, BOXWID + (wwid - (int) strlen(crtitle)) / 2, crtitle);
        mvwaddstr(cw, 2+BOXWID, BOXWID, namep);
        mvwaddstr(cw, 4+BOXWID, BOXWID, valuep);
        mvwaddstr(cw, 6+BOXWID, BOXWID, commp);

        /* Read name field */

        cv_sc.col = cprlen + 2;
        if  (!(str = wgets(cw, 2+BOXWID, &cv_sc, ""))  ||  str[0] == '\0')  {
                delwin(cw);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  -1;
        }

        Oreq.sh_un.sh_var.var_type = 0;
        Oreq.sh_un.sh_var.var_flags = 0;

        strcpy(Oreq.sh_un.sh_var.var_name, str);
        disp_str = Oreq.sh_un.sh_var.var_name;  /* In case of error */

        reset_state();

        if  (!getval(cw, 4+BOXWID, cprlen + 2, 0, &Oreq.sh_un.sh_var.var_value))  {
                delwin(cw);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  -1;
        }

        cv_comm.col = cprlen + 2;
        if  (!(str = wgets(cw, 6+BOXWID, &cv_comm, "")))  {
                delwin(cw);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  -1;
        }

        Oreq.sh_un.sh_var.var_mode.u_flags = mypriv->btu_vflags[0];
        Oreq.sh_un.sh_var.var_mode.g_flags = mypriv->btu_vflags[1];
        Oreq.sh_un.sh_var.var_mode.o_flags = mypriv->btu_vflags[2];

        delwin(cw);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        strcpy(Oreq.sh_un.sh_var.var_comment, str);
        qwvmsg(V_CREATE, (BtvarRef) 0, 0L);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  delvar(BtvarRef vp)
{
        static  char    *cnfmsg;
        unsigned        retc;
        ULONG   Saveseq = vp->var_sequence;
        WINDOW  *awin;

        /* Can the geyser do it?  */

        if  (!mpermitted(&vp->var_mode, BTM_DELETE, mypriv->btu_priv))  {
                disp_str = vp->var_name;
                doerror(vscr, $E{btq vlist cannot delete});
                return  0;
        }

        if  (Dispflags & DF_CONFABORT)  {
                int     begy, y, x;
                getbegyx(vscr, begy, x);
                getyx(vscr, y, x);
                if  (!cnfmsg)
                        cnfmsg = gprompt($P{btq var del state});
                if  (!(awin = newwin(1, 0, begy + y, x)))
                        return  0;
                wprintw(awin, cnfmsg, vp->var_name);
                wrefresh(awin);
                select_state($S{btq var del state});
                Ew = vscr;
                disp_str = vp->var_name;

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
                                dochelp(awin, $H{btq var del state});
                                continue;
                        }
                        if  (x == $K{key refresh})  {
                                wrefresh(curscr);
                                wrefresh(awin);
                                continue;
                        }
                        if  (x == $K{Confirm delete OK}  ||  x == $K{Confirm delete cancel})
                                break;
                        doerror(vscr, $E{btq var del state});
                }
                delwin(awin);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                if  (x == $K{Confirm delete cancel})
                        return  -1;
        }

        qwvmsg(V_DELETE, vp, Saveseq);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  assvar(BtvarRef vp, int row, int col, int prompt)
{
        Btcon   newval;
        unsigned        retc;
        ULONG   Saveseq = vp->var_sequence;

        /* We'll want this if we have an error */

        disp_str = vp->var_name;

        if  (!mpermitted(&vp->var_mode, BTM_WRITE, mypriv->btu_priv))  {
                doerror(vscr, $E{btq vlist cannot assign});
                return  0;
        }

        reset_state();
        newval = vp->var_value;
        if  (col < 0)  {
                int     y, x, ret;
                WINDOW  *awin;
                char    *prmpt;
                getbegyx(vscr, y, x);
                if  (!(awin = newwin(1, 0, y + row, 0)))
                        return  -1;
                prmpt = gprompt(prompt);
                waddstr(awin, prmpt);
                free(prmpt);
                getyx(awin, y, x);
                con_refill(awin, 0, x, &newval);
                ret = getval(awin, 0, x, 0, &newval);
                delwin(awin);
                touchwin(vscr);
                wrefresh(vscr);
                if  (!ret)
                        return  -1;
        }
        else  if  (!getval(vscr, row, col, 1, &newval))
                return  -1;
        Oreq.sh_un.sh_var = *vp;
        Oreq.sh_un.sh_var.var_value = newval;
        qwvmsg(V_ASSIGN, (BtvarRef) 0, Saveseq);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  setconst(int row)
{
        static  char    *constp;
        WINDOW  *awin;
        LONG    ret;
        int     y, x;

        if  (!constp)  {
                constp = gprompt($P{btq arith constant});
                cv_const.col = strlen(constp);
        }
        getbegyx(vscr, y, x);
        if  (!(awin = newwin(1, 0, y + row, 0)))
                ABORT_NOMEM;
        wprintw(awin, "%s%*d", constp, cv_const.size, Const_val);
        reset_state();
        ret = wnum(awin, 0, &cv_const, Const_val);
        delwin(awin);
        if  (ret > 0)  {
                Const_val = ret;
                if  (hvscr)  {
                        char    **hv, **hc, *vt1, *vt2;
                        get_vartitle(&vt1, &vt2);
                        disp_arg[7] = Const_val;
                        werase(hvscr);
                        if  ((hv = helphdr('V')))  {
                                for  (row = 0, hc = hv;  *hc;  row++, hc++)  {
                                        char  *lin = *hc;
                                        if  ((lin[0] == '1' || lin[0] == '2') && lin[1] == '\0')
                                                mvwhdrstr(hvscr, row, 0, lin[0] == '1'? vt1: vt2);
                                        else
                                                mvwhdrstr(hvscr, row, 0, lin);
                                        free(lin);
                                }
                                free((char *) hv);
                        }
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hvscr);
#else
                        wrefresh(hvscr);
#endif
                        free(vt1);
                        free(vt2);
                }
        }
        return  -1;
}

int  arithvar(int key, BtvarRef vp)
{
        ULONG   Saveseq = vp->var_sequence;
        BtconRef   cvalue = &vp->var_value;
        LONG    newval;
        unsigned        retc;

        disp_str = vp->var_name;
        if  (!mpermitted(&vp->var_mode, BTM_WRITE, mypriv->btu_priv))  {
                doerror(vscr, $E{btq vlist cannot assign});
                return  0;
        }
        if  (cvalue->const_type != CON_LONG)  {
                doerror(vscr, $E{btq vlist not arithmetic});
                return  0;
        }
        newval = cvalue->con_un.con_long;

        switch  (key)  {
        case  $K{btq vlist key add}:    newval += Const_val;    break;
        case  $K{btq vlist key sub}:    newval -= Const_val;    break;
        case  $K{btq vlist key mult}:
                if  (Const_val <= 1L)
                        return  0;
                newval *= Const_val;
                break;
        case  $K{btq vlist key div}:
                if  (Const_val <= 1L)
                        return  0;
                newval /= Const_val;
                break;
        case  $K{btq vlist key mod}:
                if  (Const_val <= 1L)
                        newval = 0L;
                else
                        newval %= Const_val;
                break;
        }

        Oreq.sh_un.sh_var = *vp;
        Oreq.sh_un.sh_var.var_value.con_un.con_long = newval;
        qwvmsg(V_ASSIGN, (BtvarRef) 0, Saveseq);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  modvar(BtvarRef vp)
{
        int             readwrite;
        unsigned        retc;
        ULONG           Saveseq = vp->var_sequence;

        if  (!mpermitted(&vp->var_mode, BTM_RDMODE, mypriv->btu_priv))  {
                disp_str = vp->var_name;
                doerror(vscr, $E{btq vlist cannot change mode});
                return  0;
        }

        readwrite = mpermitted(&vp->var_mode, BTM_WRMODE, mypriv->btu_priv);

        Oreq.sh_un.sh_var = *vp;
        if  (mode_edit(vscr, readwrite, 0, vp->var_name, (jobno_t) 0, &Oreq.sh_un.sh_var.var_mode) <= 0  ||  !readwrite)
                return  -1;

        qwvmsg(V_CHMOD, (BtvarRef) 0, Saveseq);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  ownvar(BtvarRef vp)
{
        static  char    *cnfmsg;
        int             begy, y, x;
        int_ugid_t      nu;
        unsigned        retc;
        ULONG           Saveseq = vp->var_sequence;
        char            *str;
        WINDOW          *awin;

        getbegyx(vscr, begy, x);
        getyx(vscr, y, x);
        if  (!cnfmsg)
                cnfmsg = gprompt($P{btq new var user});
        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  0;
        wprintw(awin, cnfmsg, vp->var_name, vp->var_mode.o_user);
        getyx(awin, y, x);
        cv_user.col = (SHORT) x;
        str = wgets(awin, 0, &cv_user, "");
        delwin(awin);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        if  (str == (char *) 0)
                return  -1;

        if  ((nu = lookup_uname(str)) == UNKNOWN_UID)  {
                if  (!isdigit(str[0]))  {
                        doerror(vscr, $E{btq vlist invalid user name});
                        return  -1;
                }
                nu = atol(str);
        }

        Oreq.sh_params.param = nu;
        qwvmsg(V_CHOWN, vp, Saveseq);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  grpvar(BtvarRef vp)
{
        static  char    *cnfmsg;
        int             begy, y, x;
        int_ugid_t      ng;
        unsigned        retc;
        ULONG           Saveseq = vp->var_sequence;
        char            *str;
        WINDOW          *awin;

        getbegyx(vscr, begy, x);
        getyx(vscr, y, x);
        if  (!cnfmsg)
                cnfmsg = gprompt($P{btq new var group});
        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  0;
        wprintw(awin, cnfmsg, vp->var_name, vp->var_mode.o_group);
        getyx(awin, y, x);
        cv_group.col = (SHORT) x;
        str = wgets(awin, 0, &cv_group, "");
        delwin(awin);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        if  (str == (char *) 0)
                return  -1;

        if  ((ng = lookup_gname(str)) == UNKNOWN_GID)  {
                if  (!isdigit(str[0]))  {
                        doerror(vscr, $E{btq vlist invalid group name});
                        return  -1;
                }
                ng = atol(str);
        }

        Oreq.sh_params.param = ng;
        qwvmsg(V_CHGRP, vp, Saveseq);
        if  ((retc = readreply()) != V_OK)  {
                qdoverror(retc, &Oreq.sh_un.sh_var);
                return  -1;
        }
        return  1;
}

int  renvar(BtvarRef vp)
{
        static  char    *cnfmsg;
        int             begy, y, x;
        unsigned        retc;
        char    *str;
        WINDOW  *awin;
        ULONG   Saveseq = vp->var_sequence;

        /* Can the geyser do it?  */

        if  (!mpermitted(&vp->var_mode, BTM_DELETE, mypriv->btu_priv))  {
                disp_str = vp->var_name;
                doerror(vscr, $E{btq vlist Cannot rename});
                return  0;
        }

        getbegyx(vscr, begy, x);
        getyx(vscr, y, x);
        if  (!cnfmsg)
                cnfmsg = gprompt($P{btq var rename});
        if  (!(awin = newwin(1, 0, begy + y, x)))
                return  0;
        wprintw(awin, cnfmsg, vp->var_name);
        getyx(awin, y, x);
        cv_rename.col = (SHORT) x;
        str = wgets(awin, 0, &cv_rename, "");
        delwin(awin);
#ifdef  CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        if  (str == (char *) 0 || str[0] == '\0')
                return  1;

        Oreq.sh_params.mcode = V_NEWNAME;
        Oreq.sh_un.sh_rn.sh_ovar = *vp;
        Oreq.sh_un.sh_rn.sh_ovar.var_sequence = Saveseq;
        strcpy(Oreq.sh_un.sh_rn.sh_rnewname, str);
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar) + strlen(str) + 1, 0) < 0)
                msg_error();
        if  ((retc = readreply()) != V_OK)  {
                disp_str2 = Oreq.sh_un.sh_rn.sh_rnewname;
                qdoverror(retc, &Oreq.sh_un.sh_rn.sh_ovar);
                return  -1;
        }
        return  1;
}
