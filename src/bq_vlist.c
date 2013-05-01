/* bq_vlist.c -- variable list handling for gbch-q

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
#ifdef  HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "files.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "shreq.h"
#include "btuser.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "statenums.h"
#include "errnums.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "formats.h"
#include "optflags.h"
#include "cfile.h"

#define DEFAULT_FORM1   " %22N %41V %13E"
#define DEFAULT_FORM2   "     %44C %7U %7G %7K"

#define LINESV          3
#define MAXVSTEP        3

static  char    Filename[] = __FILE__;
extern  LONG    Const_val;

extern  Shipc   Oreq;

static  char    Cvar[BTV_NAME+1];

int     Vhline,
        Veline;

static  char    *bigbuff,
                *var1_format,
                *var2_format;

extern  int     VLINES;

extern  WINDOW  *hvscr,
                *tvscr,
                *vscr,
                *escr,
                *hlpscr,
                *Ew;

char    gv_export;
extern  int     hadrfresh;

extern  SHORT   wh_v1titline, wh_v2titline;
extern  char    *exportmark, *clustermark, *Curr_pwd;

#define NULLCP          (char *) 0
#define HELPLESS        ((char **(*)()) 0)

static  struct  sctrl
cv_scomm = { $H{btq new variable comment}, HELPLESS, BTV_COMMENT, 0, 255, MAG_OK, 0L, 0L, NULLCP };
static  SHORT   value_col, value_row;

void  con_refill(WINDOW *, int, int, BtconRef);
void  dochelp(WINDOW *, int);
void  doerror(WINDOW *, int);
void  qdoverror(unsigned, BtvarRef);
void  endhe(WINDOW *, WINDOW **);
void  notebgwin(WINDOW *, WINDOW *, WINDOW *);
void  viewhols(WINDOW *);
void  ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
void  qwvmsg(unsigned, BtvarRef, ULONG);
void  offersave(char *, const char *);
int  createvar();
int  delvar(BtvarRef);
int  assvar(BtvarRef, int, int, int);
int  setconst(int);
int  arithvar(int, BtvarRef);
int  modvar(BtvarRef);
int  ownvar(BtvarRef);
int  grpvar(BtvarRef);
int  renvar(BtvarRef);
int  propts();
int  fmtprocess(char **, const char, struct formatdef *, struct formatdef *);
char *wgets(WINDOW *, const int, struct sctrl *, const char *);
char *chk_wgets(WINDOW *, const int, struct sctrl *, const char *, const int);

/* For job conditions and assignments.
   Check that specified variable name is valid, and if
   so return the index or -1 if not.  */

int  val_var(const char *name, const unsigned modeflag)
{
        int     first = 0, last = Var_seg.nvars, middle, s;
        const   char    *colp;
        BtvarRef        vp;
        netid_t hostid = 0;

        if  ((colp = strchr(name, ':')) != (char *) 0)  {
                char    hname[HOSTNSIZE+1];
                s = colp - name;
                if  (s > HOSTNSIZE)
                        s = HOSTNSIZE;
                strncpy(hname, name, s);
                hname[s] = '\0';
                if  ((hostid = look_int_hostname(hname)) == -1)
                        return  -1;
                colp++;
        }
        else
                colp = name;

        while  (first < last)  {
                middle = (first + last) / 2;
                vp = &vv_ptrs[middle].vep->Vent;
                if  ((s = strcmp(colp, vp->var_name)) == 0)  {
                        if  (vp->var_id.hostid == hostid)  {
                                if  (mpermitted(&vp->var_mode, modeflag, mypriv->btu_priv))
                                        return  middle;
                                return  -1;
                        }
                        if  (vp->var_id.hostid < hostid)
                                first = middle + 1;
                        else
                                last = middle;
                }
                else  if  (s > 0)
                        first = middle + 1;
                else
                        last = middle;
        }
        return  -1;
}

/* Generate matrix of variables accessible via a given mode for job vars.  */

#define INCVLIST        10

static char **gen_vars(char *prefix, unsigned mode)
{
        unsigned        vcnt, maxr, countr;
        char    **result;
        int     sfl = 0;

        if  ((result = (char **) malloc((Var_seg.nvars + 1)/2 * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        maxr = (Var_seg.nvars + 1) / 2;
        countr = 0;
        if  (prefix)
                sfl = strlen(prefix);

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                BtvarRef        vp = &vv_ptrs[vcnt].vep->Vent;

                /* Skip ones which don't match the prefix or not allowed.  */

                if  (!mpermitted(&vp->var_mode, mode, mypriv->btu_priv))
                        continue;
                if  (gv_export)  {
                        if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                continue;
                }
                else  if  (vp->var_flags & VF_EXPORT)
                        continue;
                if  (strncmp(vp->var_name, prefix, (unsigned) sfl) != 0)
                        continue;
                if  (countr + 1 >= maxr)  {
                        maxr += INCVLIST;
                        if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
                                ABORT_NOMEM;
                }
                result[countr++] = stracpy(VAR_NAME(vp));
        }

        if  (countr == 0)  {
                free((char *) result);
                return  (char **) 0;
        }

        result[countr] = (char *) 0;
        return  result;
}

char **gen_rvars(char *prefix)
{
        return  gen_vars(prefix, BTM_READ);
}

char **gen_wvars(char *prefix)
{
        return  gen_vars(prefix, BTM_WRITE);
}

/* Find current var in queue and adjust pointers as required.
   The idea is that we try to preserve the position on the screen of
   the current line.
   Option - if scrkeep set, move var but keep the rest.  */

void  cvfind()
{
        int     first, last, middle;

        if  (Dispflags & DF_SCRKEEP)  {
                if  (Vhline >= Var_seg.nvars  &&  (Vhline = Var_seg.nvars - VLINES/LINESV) < 0)
                        Vhline = 0;
                if  (Veline - Vhline >= VLINES/LINESV)
                        Veline = Vhline - VLINES/LINESV - LINESV;
        }
        else  if  (Cvar[0])  {
                int     s, res = Veline;

                if  (res >= Var_seg.nvars)  {
                        if  (Var_seg.nvars == 0)  {
                                Veline = 0;
                                Vhline = 0;
                                return;
                        }
                        res = Var_seg.nvars - 1;
                }

                if  ((s = strcmp(Cvar, vv_ptrs[res].vep->Vent.var_name)) == 0)
                        goto  done;

                if  (s > 0)  {
                        first = res;
                        last = Var_seg.nvars;
                }
                else  {
                        first = 0;
                        last = res;
                }

                while  (first < last)  {
                        middle = (first + last) / 2;
                        if  ((s = strcmp(Cvar, vv_ptrs[middle].vep->Vent.var_name)) == 0)  {
                                res = middle;
                                goto  done;
                        }
                        if  (s > 0)
                                first = middle + 1;
                        else
                                last = middle;
                }

                if  (first >= Var_seg.nvars)  {
                        Veline = Var_seg.nvars - 1;
                        res = Veline - VLINES/LINESV + 1;
                        if  (res < Vhline)
                                Vhline = res;
                        return;
                }

                res = first;
                strcpy(Cvar, vv_ptrs[res].vep->Vent.var_name);

        done:
                /* Move top of screen line up/down queue by same
                   amount as current var.  This code assumes that
                   Veline - Vhline < VLINES/LINESV but that we
                   may wind up with Vhline < 0; */

                Vhline += res - Veline;
                Veline = res;
        }
}

#define BTQ_INLINE
typedef int     fmt_t;

#include "inline/vfmt_comment.c"
#include "inline/vfmt_group.c"
#include "inline/vfmt_export.c"
#include "inline/fmtmode.c"
#include "inline/vfmt_mode.c"
#include "inline/vfmt_name.c"
#include "inline/vfmt_user.c"
#include "inline/vfmt_value.c"

static  struct  formatdef
        uppertab[] = { /* A-Z */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* A */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* B */
        {       $P{var fmt title}+'C',20,NULLCP, NULLCP,fmt_comment     },      /* C */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* D */
        {       $P{var fmt title}+'E',6,NULLCP, NULLCP, fmt_export      },      /* E */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* F */
        {       $P{var fmt title}+'G',UIDSIZE-2,NULLCP, NULLCP,fmt_group},      /* G */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* H */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* I */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* J */
        {       $P{var fmt title}+'K',8,NULLCP, NULLCP, fmt_cluster     },      /* K */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* L */
        {       $P{var fmt title}+'M',10,NULLCP, NULLCP, fmt_mode       },      /* M */
        {       $P{var fmt title}+'N',BTV_NAME-6,NULLCP, NULLCP, fmt_name},     /* N */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* O */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* P */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* Q */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* R */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* S */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* T */
        {       $P{var fmt title}+'U',UIDSIZE-2,NULLCP,NULLCP,fmt_user  },      /* U */
        {       $P{var fmt title}+'V',20,NULLCP, NULLCP,fmt_value       },      /* V */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* W */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* X */
        {       0,      0,              NULLCP, NULLCP, 0               },      /* Y */
        {       0,      0,              NULLCP, NULLCP, 0               }       /* Z */
};

/* Calculate length of format, also used in job list */

int  format_len(const char *fmt)
{
        int     result = 1, nn;

        while  (*fmt)  {
                if  (*fmt++ != '%')  {
                        result++;
                        continue;
                }
                if  (*fmt == '<')
                        fmt++;
                nn = 0;
                do  nn = nn * 10 + *fmt++ - '0';
                while  (isdigit(*fmt));
                result += nn;
                if  (isalpha(*fmt))
                        fmt++;
        }
        return  result;
}

static char *vtitalloc(const char *fmt, const int len, const int vrow)
{
        int     nn;
        char    *result = malloc((unsigned) len), *rp, *mp;
        struct  formatdef       *fp;

        if  (!result)
                ABORT_NOMEM;

        rp = result;

        while  (*fmt)  {
                if  (*fmt != '%')  {
                        *rp++ = *fmt++;
                        continue;
                }
                fmt++;

                /* Get width */

                if  (*fmt == '<')
                        fmt++;
                nn = 0;
                do  nn = nn * 10 + *fmt++ - '0';
                while  (isdigit(*fmt));

                /* Get format char */

                if  (isupper(*fmt))
                        fp = &uppertab[*fmt - 'A'];
                else  {
                        if  (*fmt)
                                fmt++;
                        continue;
                }

                switch  (*fmt++)  {
                case  'V':
                        value_col = (unsigned char) (rp - result);
                        value_row = (SHORT) vrow;
                        break;
                case  'C':
                        cv_scomm.col = (SHORT) (rp - result);
                        break;
                }

                if  (fp->statecode == 0)
                        continue;

                /* Get title message if we don't have it Insert into result */

                if  (!fp->msg)
                        fp->msg = gprompt(fp->statecode);

                mp = fp->msg;
                while  (nn > 0  &&  *mp)  {
                        *rp++ = *mp++;
                        nn--;
                }
                while  (nn > 0)  {
                        *rp++ = ' ';
                        nn--;
                }
        }

        /* Trim trailing spaces */

        for  (rp--;  rp >= result  &&  *rp == ' ';  rp--)
                ;
        *++rp = '\0';
        return  result;
}

static  void    get_vartitle_fmt(char **fmtp, const char *kw, const char *dflt, const int fcode)
{
        char    *fmt = *fmtp, *nfmt;
        if  (fmt)
                return;
        fmt = helpprmpt(fcode);
        nfmt = optkeyword(kw);
        if  (nfmt)  {
                if (fmt)
                        free(fmt);
                fmt = nfmt;
        }
        if  (!fmt)
                fmt = stracpy(dflt);
        *fmtp = fmt;
}

void  get_vartitle(char **t1, char **t2)
{
        int     obufl1, obufl2;

        get_vartitle_fmt(&var1_format, "BTQVAR1FLD", DEFAULT_FORM1, $P{Default var list fmt 1});
        get_vartitle_fmt(&var2_format, "BTQVAR2FLD", DEFAULT_FORM2, $P{Default var list fmt 2});

        /* Initial pass to discover how much space to allocate */

        obufl1 = format_len(var1_format);
        obufl2 = format_len(var2_format);

        /* Allocate space for title and results */

        if  (bigbuff)
                free(bigbuff);
        if  (!(bigbuff = malloc(2*sizeof(Btvar))))
                ABORT_NOMEM;

        /* Possible places for values.  */

        cv_scomm.col = -1;
        value_col = -1;
        value_row = 0;
        *t1 = vtitalloc(var1_format, obufl1, 0);
        *t2 = vtitalloc(var2_format, obufl2, 1);
}

#ifdef  HAVE_TERMINFO
#define DISP_CHAR(w, ch)        waddch(w, (chtype) ch);
#else
#define DISP_CHAR(w, ch)        waddch(w, ch);
#endif

static void  vfillin(BtvarRef vp, const int row, const char *fmt)
{
        char    *lbp;
        struct  formatdef  *fp;
        int     currplace = -1, lastplace, nn, inlen, dummy;
        int     isreadable = mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv);

        wmove(vscr, row, 0);

        while  (*fmt)  {
                if  (*fmt != '%')  {
                        DISP_CHAR(vscr, *fmt);
                        fmt++;
                        continue;
                }
                fmt++;
                lastplace = -1;
                if  (*fmt == '<')  {
                        lastplace = currplace;
                        fmt++;
                }
                nn = 0;
                do  nn = nn * 10 + *fmt++ - '0';
                while  (isdigit(*fmt));
                if  (isupper(*fmt))
                        fp = &uppertab[*fmt - 'A'];
                else  {
                        if  (*fmt)
                                fmt++;
                        continue;
                }
                fmt++;
                if  (!fp->fmt_fn)
                        continue;
                getyx(vscr, dummy, currplace);
                inlen = (fp->fmt_fn)(vp, isreadable, nn);
                lbp = bigbuff;
                if  (inlen > nn  &&  lastplace >= 0)  {
                        wmove(vscr, row, lastplace);
                        nn = currplace + nn - lastplace;
                }
                while  (inlen > 0  &&  nn > 0)  {
                        DISP_CHAR(vscr, *lbp);
                        lbp++;
                        inlen--;
                        nn--;
                }
                if  (nn > 0)  {
                        int     ccol;
                        getyx(vscr, dummy, ccol);
                        wmove(vscr, dummy, ccol+nn);
                }
        }
}

/* Display contents of var file */

static void  vdisplay()
{
        int     row, vcnt;

#if     defined(OS_DYNIX) || defined(CURSES_MEGA_BUG)
        wclear(vscr);
#else
        werase(vscr);
#endif

        /* Better not let too big a gap develop....  */

        if  (Vhline < - MAXVSTEP)
                Vhline = 0;

        if  ((vcnt = Vhline) < 0)  {
                row = - Vhline * LINESV;
                vcnt = 0;
        }
        else
                row = 0;

        for  (;  vcnt < Var_seg.nvars  &&  row < VLINES;  vcnt++, row += LINESV)  {
                BtvarRef  vp = &vv_ptrs[vcnt].vep->Vent;
                vfillin(vp, row, var1_format);
                vfillin(vp, row+1, var2_format);
        }
#ifdef  CURSES_OVERLAP_BUG
        if  (hvscr)  {
                touchwin(hvscr);
                wrefresh(hvscr);
        }
        if  (tvscr)  {
                touchwin(tvscr);
                wrefresh(tvscr);
        }
        touchwin(vscr);
#endif
}

static char *gsearchs(const int isback)
{
        int     row;
        char    *gstr;
        struct  sctrl   ss;
        static  char    *lastmstr;
        static  char    *sforwmsg, *sbackwmsg;

        if  (!sforwmsg)  {
                sforwmsg = gprompt($P{btq search forward});
                sbackwmsg = gprompt($P{btq search backward});
        }

        ss.helpcode = $H{btq search forward};
        gstr = isback? sbackwmsg: sforwmsg;
        ss.helpfn = HELPLESS;
        ss.size = 30;
        ss.retv = 0;
        ss.col = strlen(gstr);
        ss.magic_p = MAG_OK;
        ss.min = 0L;
        ss.vmax = 0L;
        ss.msg = NULLCP;
        row = (Veline - Vhline) * LINESV;
        mvwaddstr(vscr, row, 0, gstr);
        wclrtoeol(vscr);

        if  (lastmstr)  {
                ws_fill(vscr, row, &ss, lastmstr);
                gstr = wgets(vscr, row, &ss, lastmstr);
                if  (!gstr)
                        return  NULLCP;
                if  (gstr[0] == '\0')
                        return  lastmstr;
        }
        else  {
                for  (;;)  {
                        gstr = wgets(vscr, row, &ss, "");
                        if  (!gstr)
                                return  NULLCP;
                        if  (gstr[0])
                                break;
                        doerror(vscr, $E{btq vlist no search string});
                }
        }
        if  (lastmstr)
                free(lastmstr);
        return  lastmstr = stracpy(gstr);
}

static int  smatchit(const char *vstr, const char *mstr)
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

static int  smatch(const int mline, const char *mstr)
{
        CBtvarRef  vp = &vv_ptrs[mline].vep->Vent;
        if  (smatchit(vp->var_name, mstr))
                return  1;
        if  (mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv)  &&  smatchit(vp->var_comment, mstr))
                return  1;
        return  0;
}

/* Search for string in var name or comment.
   Return 0 - need to redisplay jobs
   (Jhline and Jeline suitably mangled)
   otherwise error code */

static int  dosearch(const int isback)
{
        char    *mstr = gsearchs(isback);
        int     mline;

        if  (!mstr)
                return  0;

        if  (isback)  {
                for  (mline = Veline - 1;  mline >= 0;  mline--)
                        if  (smatch(mline, mstr))
                                goto  gotit;
                for  (mline = Var_seg.nvars - 1;  mline >= Veline;  mline--)
                        if  (smatch(mline, mstr))
                                goto  gotit;
        }
        else  {
                for  (mline = Veline + 1;  (unsigned) mline < Var_seg.nvars;  mline++)
                        if  (smatch(mline, mstr))
                                goto  gotit;
                for  (mline = 0;  mline <= Veline;  mline++)
                        if  (smatch(mline, mstr))
                                goto  gotit;
        }
        return  $E{btq vlist Search string not found};

 gotit:
        Veline = mline;
        if  (Veline < Vhline  ||  (Veline - Vhline) * LINESV >= VLINES)
                Vhline = Veline;
        return  0;
}

static void  var_macro(BtvarRef vp, const int num)
{
        char    *prompt = helpprmpt(num + $P{Var macro}), *str;
        static  char    *execprog;
        PIDTYPE pid;
        int     status, refreshscr = 0;
#ifdef  HAVE_TERMIOS_H
        struct  termios save;
        extern  struct  termios orig_term;
#else
        struct  termio  save;
        extern  struct  termio  orig_term;
#endif

        if  (!prompt)  {
                disp_arg[0] = num;
                doerror(vscr, $E{Macro error});
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
                int     vsy, vsx;
                struct  sctrl   dd;
                wclrtoeol(vscr);
                waddstr(vscr, str);
                getyx(vscr, vsy, vsx);
                dd.helpcode = $H{Var macro};
                dd.helpfn = HELPLESS;
                dd.size = COLS - vsx;
                dd.col = vsx;
                dd.magic_p = MAG_P|MAG_OK;
                dd.min = dd.vmax = 0;
                dd.msg = (char *) 0;
                str = wgets(vscr, vsy, &dd, "");
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
                const  char    *argbuf[3];
                argbuf[0] = str;
                if  (vp)  {
                        argbuf[1] = VAR_NAME(vp);
                        argbuf[2] = (const char *) 0;
                }
                else
                        argbuf[1] = (const char *) 0;
                if  (!refreshscr)  {
                        close(0);
                        close(1);
                        close(2);
                        Ignored_error = dup(dup(open("/dev/null", O_RDWR)));
                }
                Ignored_error = chdir(Curr_pwd);
                execv(execprog, (char **) argbuf);
                exit(255);
        }
        free(prompt);
        if  (pid < 0)  {
                doerror(vscr, $E{Macro fork failed});
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
                        doerror(vscr, $E{Macro command gave signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror(vscr, $E{Macro command error});
                }
        }
}

int  v_process()
{
        int     err_no, i, ret, ch, currow;
        unsigned        retc;
        ULONG           Saveseq;

        char    *str;

 refillall:
        if  (hvscr)  {
                touchwin(hvscr);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(hvscr);
#else
                wrefresh(hvscr);
#endif
        }
        if  (tvscr)  {
                touchwin(tvscr);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(tvscr);
#else
                wrefresh(tvscr);
#endif
        }

 Vreset:
        notebgwin(hvscr, vscr, tvscr);
        Ew = vscr;
        select_state($S{btq var list state});
 Vdisp:
        vdisplay();
#ifdef  HAVE_TERMINFO
        if  (hlpscr)  {
                touchwin(hlpscr);
                wnoutrefresh(vscr);
                wnoutrefresh(hlpscr);
        }
        if  (escr)  {
                touchwin(escr);
                wnoutrefresh(vscr);
                wnoutrefresh(escr);
        }
#else
        if  (hlpscr)  {
                touchwin(hlpscr);
                wrefresh(vscr);
                wrefresh(hlpscr);
        }
        if  (escr)  {
                touchwin(escr);
                wrefresh(vscr);
                wrefresh(escr);
        }
#endif

 Vmove:
        currow = (Veline - Vhline) * LINESV;
        wmove(vscr, currow, 0);
        if  (Var_seg.nvars != 0)
                strcpy(Cvar, vv_ptrs[Veline].vep->Vent.var_name);
        else
                Cvar[0] = '\0';

 Vrefresh:

#ifdef HAVE_TERMINFO
        wnoutrefresh(vscr);
        doupdate();
#else
        wrefresh(vscr);
#endif

nextin:
        if  (hadrfresh)
                return  -1;

        do  ch = getkey(MAG_A|MAG_P);
        while  (ch == EOF  &&  (hlpscr || escr));

        if  (hlpscr)  {
                endhe(vscr, &hlpscr);
                if  (Dispflags & DF_HELPCLR)
                        goto  nextin;
        }
        if  (escr)
                endhe(vscr, &escr);

        switch  (ch)  {
        case  EOF:
                goto  nextin;

        /* Error case - bell character and try again.  */

        default:
                err_no = $E{btq vlist unknown command};
        err:
                doerror(vscr, err_no);
                goto  nextin;

        case  $K{key help}:
                disp_arg[7] = Const_val;
                dochelp(vscr, $H{btq var list state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  Vrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                Veline++;
                if  (Veline >= Var_seg.nvars)  {
                        Veline--;
ev:                     err_no = $E{btq vlist off end};
                        goto  err;
                }

                if  ((currow += LINESV) < VLINES)
                        goto  Vmove;
                Vhline++;
                goto  Vdisp;

        case  $K{key cursor up}:
                if  (Veline <= 0)  {
bv:                     err_no = $E{btq vlist off beginning};
                        goto  err;
                }
                Veline--;
                if  ((currow -= LINESV) >= 0)
                        goto  Vmove;
                Vhline = Veline;
                goto  Vdisp;

        /* Half/Full screen up/down */

        case  $K{key screen down}:
                if  (Vhline + VLINES/LINESV >= Var_seg.nvars)
                        goto  ev;
                Vhline += VLINES/LINESV;
                Veline += VLINES/LINESV;
                if  (Veline >= Var_seg.nvars)
                        Veline = Var_seg.nvars - 1;
        redr:
                vdisplay();
                currow = (Veline - Vhline) * LINESV;
                goto  Vmove;

        case  $K{key half screen down}:
                i = VLINES / LINESV / 2;
                if  (Vhline + i >= Var_seg.nvars)
                        goto  ev;
                Vhline += i;
                if  (Veline < Vhline)
                        Veline = Vhline;
                goto  redr;

        case  $K{key half screen up}:
                if  (Vhline <= 0)
                        goto  bv;
                Vhline -= VLINES / LINESV / 2;
        restu:
                vdisplay();
                if  (Veline - Vhline >= VLINES/LINESV)
                        Veline = Vhline + VLINES/LINESV - 1;
                goto  Vmove;

        case  $K{key screen up}:
                if  (Vhline <= 0)
                        goto  bv;
                Vhline -= VLINES;
                goto  restu;

        case  $K{key top}:
                if  (Vhline == Veline  ||  Veline == 0)  {
                        Vhline = 0;
                        Veline = 0;
                        goto  restu;
                }
                Veline = Vhline < 0?  0:  Vhline;
                goto  Vmove;

        case  $K{key bottom}:
                if  (Veline >= Vhline + VLINES/LINESV - 1  &&  Vhline + VLINES/LINESV < Var_seg.nvars)  {
                        if  (Var_seg.nvars > VLINES/LINESV)
                                Vhline = Var_seg.nvars - VLINES/LINESV;
                        else
                                Vhline = 0;
                        Veline = Var_seg.nvars - 1;
                        goto  restu;
                }
                Veline = Vhline + VLINES/LINESV - 1;
                if  (Veline >= Var_seg.nvars)
                        Veline = Var_seg.nvars - 1;
                goto  Vmove;

        case  $K{key search forward}:
        case  $K{key search backward}:
                if  ((err_no = dosearch(ch == $K{key search backward})) != 0)
                        doerror(vscr, err_no);
                goto  Vdisp;

                /* Go home.  */

        case  $K{key halt}:
                return  0;

        case  $K{key save opts}:
                propts();
                ret = -1;
                goto  endop;

        case  $K{btq vlist key jobs}:
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  1;

        case  $K{btq vlist key holidays}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                viewhols(vscr);
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                goto  refillall;

                /* And now for the stuff to change things.  */

        case  $K{btq vlist key create}:
                if  ((ret = createvar()) > 0)
                        strcpy(Cvar, Oreq.sh_un.sh_var.var_name);
        endop:
                /* Ret should be 0 if no changes to screen (apart from
                   possible error message).
                   Ret should be -1 if screen needs `touching' and resetting.
                   Ret should be 1 if a signal is expected. */

                switch  (ret)  {
                case  0:
#ifdef  CURSES_MEGA_BUG
                        clear();
                        refresh();
                        break;
#else
#ifdef  CURSES_OVERLAP_BUG
                        if  (hvscr)  {
                                touchwin(hvscr);
                                wrefresh(hvscr);
                        }
                        if  (tvscr)  {
                                touchwin(tvscr);
                                wrefresh(tvscr);
                        }
                        touchwin(vscr);
#endif
                        goto  nextin;
#endif
                case  1:
                        if  (hadrfresh)
                                return  -1;
                }

                /* In case a different key state was selected */

                select_state($S{btq var list state});
                Ew = vscr;
#ifdef  CURSES_OVERLAP_BUG
                if  (hvscr)  {
                        touchwin(hvscr);
                        wrefresh(hvscr);
                }
                if  (tvscr)  {
                        touchwin(tvscr);
                        wrefresh(tvscr);
                }
#endif
                touchwin(vscr);
                if  (escr)  {
                        touchwin(escr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(vscr);
                        wnoutrefresh(escr);
#else
                        wrefresh(vscr);
                        wrefresh(escr);
#endif
                }
#ifdef  OS_DYNIX
                goto  Vdisp;
#else
                goto  Vmove;
#endif

        case  $K{btq vlist key delete}:
                if  (Veline >= Var_seg.nvars)  {
nov:                    err_no = $E{btq vlist no vars};
                        goto  err;
                }
                ret = delvar(&vv_ptrs[Veline].vep->Vent);
                goto  endop;

        case  $K{btq vlist key assign}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                ret = assvar(&vv_ptrs[Veline].vep->Vent, (Veline - Vhline) * LINESV + value_row, value_col, $P{btq prompt4 ass});
                goto  endop;

        case  $K{btq vlist key setconst}:
                ret = setconst((Veline - Vhline) * LINESV);
                goto  endop;

        case  $K{btq vlist key add}:
        case  $K{btq vlist key sub}:
        case  $K{btq vlist key mult}:
        case  $K{btq vlist key div}:
        case  $K{btq vlist key mod}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                ret = arithvar(ch, &vv_ptrs[Veline].vep->Vent);
                goto  endop;

        case  $K{btq vlist key chmod}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = modvar(&vv_ptrs[Veline].vep->Vent);
                goto  endop;

        case  $K{btq vlist key chown}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = ownvar(&vv_ptrs[Veline].vep->Vent);
                goto  endop;

        case  $K{btq vlist key chgrp}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = grpvar(&vv_ptrs[Veline].vep->Vent);
                goto  endop;

        case  $K{btq vlist key chcomm}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                str = chk_wgets(vscr, currow + 1, &cv_scomm, vv_ptrs[Veline].vep->Vent.var_comment, $P{btq prompt4 comm});
                if  (str == (char *) 0)
                        goto  Vmove;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                strcpy(Oreq.sh_un.sh_var.var_comment, str);
                qwvmsg(V_CHCOMM, (BtvarRef) 0, Saveseq);
                if  ((retc = readreply()) == V_OK)  {
                        if  (hadrfresh)
                                return  -1;
                        goto  Vreset;
                }
                Ew = vscr;
                select_state($S{btq var list state});
                vdisplay();
                wmove(vscr, currow, 0);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(vscr);
#else
                wrefresh(vscr);
#endif
                qdoverror(retc, &Oreq.sh_un.sh_var);
                goto  Vrefresh;

        case  $K{btq vlist key rename}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                ret = renvar(&vv_ptrs[Veline].vep->Vent);
                goto  endop;

        case  $K{btq vlist key sexport}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                if  (Oreq.sh_un.sh_var.var_flags & VF_EXPORT)
                        goto  nextin;
                Oreq.sh_un.sh_var.var_flags |= VF_EXPORT;
        fset:
                qwvmsg(V_CHFLAGS, (BtvarRef) 0, Saveseq);
                if  ((retc = readreply()) == V_OK)  {
                        if  (hadrfresh)
                                return  -1;
                        goto  Vreset;
                }
                qdoverror(retc, &Oreq.sh_un.sh_var);
                goto  Vrefresh;

        case  $K{btq vlist key usexport}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                if  (!(Oreq.sh_un.sh_var.var_flags & VF_EXPORT))
                        goto  nextin;
                Oreq.sh_un.sh_var.var_flags &= ~VF_EXPORT;
                goto  fset;

        case  $K{btq vlist key togexport}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                Oreq.sh_un.sh_var.var_flags ^= VF_EXPORT;
                goto  fset;

        case  $K{btq vlist key scluster}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                if  (Oreq.sh_un.sh_var.var_flags & VF_CLUSTER)
                        goto  nextin;
                Oreq.sh_un.sh_var.var_flags |= VF_CLUSTER;
                goto  fset;

        case  $K{btq vlist key uscluster}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                if  (!(Oreq.sh_un.sh_var.var_flags & VF_CLUSTER))
                        goto  nextin;
                Oreq.sh_un.sh_var.var_flags &= ~VF_CLUSTER;
                goto  fset;

        case  $K{btq vlist key togcluster}:
                if  (Veline >= Var_seg.nvars)
                        goto  nov;
                Saveseq = vv_ptrs[Veline].vep->Vent.var_sequence;
                Oreq.sh_un.sh_var = vv_ptrs[Veline].vep->Vent;
                Oreq.sh_un.sh_var.var_flags ^= VF_CLUSTER;
                goto  fset;

        case  $K{btq vlist key fmt1}:
                {
                        char    *t1, *t2;
                        ret = fmtprocess(&var1_format, 'Y', uppertab, (struct formatdef *) 0);
                        get_vartitle(&t1, &t2);
                        if  (wh_v1titline >= 0)  {
                                wmove(hvscr, wh_v1titline, 0);
                                wclrtoeol(hvscr);
                                waddstr(hvscr, t1);
                        }
                        free(t1);
                        free(t2);
                        if  (ret)
                                offersave(var1_format, "BTQVAR1FLD");
                        goto  refillall;
                }

        case  $K{btq vlist key fmt2}:
                {
                        char    *t1, *t2;
                        ret = fmtprocess(&var2_format, 'Z', uppertab, (struct formatdef *) 0);
                        get_vartitle(&t1, &t2);
                        if  (wh_v2titline >= 0)  {
                                wmove(hvscr, wh_v2titline, 0);
                                wclrtoeol(hvscr);
                                waddstr(hvscr, t2);
                        }
                        free(t1);
                        free(t2);
                        if  (ret)
                                offersave(var2_format, "BTQVAR2FLD");
                        goto  refillall;
                }

        case  $K{key exec}:  case  $K{key exec}+1:  case  $K{key exec}+2:
        case  $K{key exec}+3:case  $K{key exec}+4:  case  $K{key exec}+5:
        case  $K{key exec}+6:case  $K{key exec}+7:  case  $K{key exec}+8:
        case  $K{key exec}+9:
                var_macro(Veline >= Var_seg.nvars? (BtvarRef) 0: &vv_ptrs[Veline].vep->Vent, ch - $K{key exec});
                vdisplay();
                if  (escr)  {
                        touchwin(escr);
                        wrefresh(escr);
                }
                goto  Vmove;
        }
}
