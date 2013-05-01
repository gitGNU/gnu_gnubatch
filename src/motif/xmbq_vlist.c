/* xmbq_vlist.c -- variable list display for gbch-xmq

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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#ifdef HAVE_XM_SPINB_H
#include <Xm/SpinB.h>
#endif
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
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
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbq_ext.h"
#include "helpargs.h"
#include "cfile.h"
#include "formats.h"
#include "optflags.h"

static  char    Filename[] = __FILE__;

static  char    *obuf1, *obuf2, *bigbuff;

extern  char    *exportmark, *clustermark;

#define XMBTQ_INLINE
typedef int     fmt_t;

#include "inline/vfmt_comment.c"
#include "inline/vfmt_group.c"
#include "inline/vfmt_export.c"
#include "inline/fmtmode.c"
#include "inline/vfmt_mode.c"
#include "inline/vfmt_name.c"
#include "inline/vfmt_user.c"
#include "inline/vfmt_value.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct  formatdef  {
        SHORT   statecode;      /* Code number for heading if applicable */
        SHORT   sugg_width;     /* Suggested width */
        char    *msg;           /* Heading */
        char    *explain;       /* More detailed explanation */
        int     (*fmt_fn)(CBtvarRef, const int, const int);
};

extern  char    *job_format;
static  char    *var1_format, *var2_format;
#define DEFAULT_FORM1   " %22N %41V %13E"
#define DEFAULT_FORM2   "     %44C %7U %7G %7K"

#define NULLCP  (char *) 0

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

char *get_vartitle()
{
        int     nn, obufl1, obufl2;
        struct  formatdef       *fp;
        char    *cp, *rp, *result, *mp;

        get_vartitle_fmt(&var1_format, "BTQVAR1FLD", DEFAULT_FORM1, $P{Default var list fmt 1});
        get_vartitle_fmt(&var2_format, "BTQVAR2FLD", DEFAULT_FORM2, $P{Default var list fmt 2});

        /* Initial pass to discover how much space to allocate */

        obufl1 = obufl2 = 1;
        cp = var1_format;
        while  (*cp)  {
                if  (*cp++ != '%')  {
                        obufl1++;
                        continue;
                }
                if  (*cp == '<')
                        cp++;
                nn = 0;
                do  nn = nn * 10 + *cp++ - '0';
                while  (isdigit(*cp));
                obufl1 += nn;
                if  (isalpha(*cp))
                        cp++;
        }
        cp = var2_format;
        while  (*cp)  {
                if  (*cp++ != '%')  {
                        obufl2++;
                        continue;
                }
                if  (*cp == '<')
                        cp++;
                nn = 0;
                do  nn = nn * 10 + *cp++ - '0';
                while  (isdigit(*cp));
                obufl2 += nn;
                if  (isalpha(*cp))
                        cp++;
        }

        /* Allocate space for title result and output buffer */

        result = malloc((unsigned) (obufl1 + 1));
        if  (obuf1)
                free(obuf1);
        if  (obuf2)
                free(obuf2);
        if  (bigbuff)
                free(bigbuff);
        obuf1 = malloc((unsigned) obufl1);
        obuf2 = malloc((unsigned) obufl2);
        bigbuff = malloc(2*sizeof(Btvar));
        if  (!result ||  !obuf1 || !obuf2  ||  !bigbuff)
                ABORT_NOMEM;

        /* Now set up title (from first string) Actually this is a
           waste of time if we aren't actually displaying same,
           but we needed the buffer.  */

        rp = result;
        *rp++ = ' ';
        cp = var1_format;
        while  (*cp)  {
                if  (*cp != '%')  {
                        *rp++ = *cp++;
                        continue;
                }
                cp++;

                /* Get width */

                if  (*cp == '<')
                        cp++;
                nn = 0;
                do  nn = nn * 10 + *cp++ - '0';
                while  (isdigit(*cp));

                /* Get format char */

                if  (isupper(*cp))
                        fp = &uppertab[*cp - 'A'];
                else  {
                        if  (*cp)
                                cp++;
                        continue;
                }

                cp++;
                if  (fp->statecode == 0)
                        continue;

                /* Get title message if we don't have it
                   Insert into result */

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

        *rp = '\0';

        /* We don't trim trailing spaces so that we have enough room
           for what comes under the title.  */

        return  result;
}

static void  vfillin(BtvarRef vp, char *obuf, char *fmt)
{
        char    *cp = fmt, *rp = obuf, *lbp;
        struct  formatdef  *fp;
        int     currplace = -1, lastplace, nn, inlen;
        int     isreadable = mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv);

        while  (*cp)  {
                if  (*cp != '%')  {
                        *rp++ = *cp++;
                        continue;
                }
                cp++;
                lastplace = -1;
                if  (*cp == '<')  {
                        lastplace = currplace;
                        cp++;
                }
                nn = 0;
                do  nn = nn * 10 + *cp++ - '0';
                while  (isdigit(*cp));
                if  (isupper(*cp))
                        fp = &uppertab[*cp - 'A'];
                else  {
                        if  (*cp)
                                cp++;
                        continue;
                }
                cp++;
                if  (!fp->fmt_fn)
                        continue;
                currplace = rp - obuf;
                inlen = (fp->fmt_fn)(vp, isreadable, nn);
                lbp = bigbuff;
                if  (inlen > nn  &&  lastplace >= 0)  {
                        rp = &obuf[lastplace];
                        nn = currplace + nn - lastplace;
                }
                while  (inlen > 0  &&  nn > 0)  {
                        *rp++ = *lbp++;
                        inlen--;
                        nn--;
                }
                while  (nn > 0)  {
                        *rp++ = ' ';
                        nn--;
                }
        }
        *rp = '\0';
}

/* Display contents of var file */

void  vdisplay()
{
        BtvarRef        vp;
        int             vcnt, topv = 1, cvarpos = -1, newpos = -1, vlines;
        char            Cvarname[BTV_NAME+1];
        netid_t         Chostno = 0;
        XmString        *elist;

        Cvarname[0] = '\0';

        if  (Var_seg.nvars != 0)  {
                int     *plist, pcnt;

                /* First discover the currently-selected variable and
                   the scroll position, if any.  */

                XtVaGetValues(vwid,
                              XmNtopItemPosition,       &topv,
                              XmNvisibleItemCount,      &vlines,
                              NULL);

                if  (XmListGetSelectedPos(vwid, &plist, &pcnt))  {
                        cvarpos = (plist[0] - 1) / VLINES;
                        XtFree((char *) plist);
                        if  ((unsigned) cvarpos < Var_seg.nvars)  {
                                vp = &vv_ptrs[cvarpos].vep->Vent;
                                strncpy(Cvarname, vp->var_name, BTV_NAME);
                                Cvarname[BTV_NAME] = '\0';
                                Chostno = vp->var_id.hostid;
                        }
                }
                XtVaGetValues(vwid, XmNitems, &elist, NULL);
        }

        XmListDeleteAllItems(vwid);
        rvarlist(1);

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                XmString        str1, str2;
                vp = &vv_ptrs[vcnt].vep->Vent;
                if  (newpos < 0  &&
                     vp->var_id.hostid == Chostno  &&
                     strcmp(Cvarname, vp->var_name) == 0)
                        newpos = vcnt;
                vfillin(vp, obuf1, var1_format);
                vfillin(vp, obuf2, var2_format);
                str1 = XmStringCreateSimple(obuf1);
                XmListAddItem(vwid, str1, 0);
                XmStringFree(str1);
                str2 = XmStringCreateSimple(obuf2);
                XmListAddItem(vwid, str2, 0);
                XmStringFree(str2);
        }

        /* Adjust scrolling */

        if  (newpos >= 0)  {    /* Only gets set if we had one */
                XmListSelectPos(vwid, newpos*VLINES+1, False);
                XmListSelectPos(vwid, newpos*VLINES+2, False);
                if  (cvarpos >= 0  &&  cvarpos != newpos)  {
                        XmListDeselectPos(vwid, cvarpos*VLINES+1);
                        XmListDeselectPos(vwid, cvarpos*VLINES+2);
                }
        }

        if  (!(Dispflags & DF_SCRKEEP))  {
                if  (newpos >= 0)  {
                        topv += (newpos - cvarpos) * VLINES;
                        if  (topv <= 0)
                                topv = 1;
                }
        }
        if  (topv+vlines > Var_seg.nvars * VLINES) /* Shrunk */
                XmListSetBottomPos(vwid, (int) Var_seg.nvars * VLINES);
        else
                XmListSetPos(vwid, topv);
}

void  vselect(Widget w, int data, XmListCallbackStruct *cbs)
{
        int     itemnum = cbs->item_position;
        itemnum = (itemnum - 1) / VLINES;
        XmListDeselectAllItems(w);
        XmListSelectPos(w, itemnum*VLINES+1, False);
        XmListSelectPos(w, itemnum*VLINES+2, False);
}

BtvarRef  getselectedvar(unsigned perm)
{
        int     *plist, pcnt;

        if  (XmListGetSelectedPos(vwid, &plist, &pcnt))  {
                int     cvarpos = (plist[0] - 1) / VLINES;
                XtFree((char *) plist);
                if  ((unsigned) cvarpos < Var_seg.nvars)  {
                        BtvarRef  vp = &vv_ptrs[cvarpos].vep->Vent;
                        if  (!mpermitted(&vp->var_mode, perm, mypriv->btu_priv))  {
                                disp_str = vp->var_name;
                                disp_str2 = vp->var_mode.o_user;
                                doerror(vwid, $EH{xmbtq no var access perm});
                                return  NULL;
                        }
                        return  vp;
                }
        }
        doerror(vwid, Var_seg.nvars != 0? $EH{xmbtq no var selected}: $EH{xmbtq no vars to select});
        return  NULL;
}

/* For job conditions and assignments.  Check that specified variable
   name is valid, and if so return the strchr or -1 if not.  */

int  val_var(const char *name, const unsigned modeflag)
{
        int     first = 0, last = Var_seg.nvars, middle, s;
        const   char    *colp;
        BtvarRef        vp;
        netid_t hostid = 0;

        if  (!name)
                return  -1;

        if  ((colp = strchr(name, ':')) != (char *) 0)  {
                char    hname[HOSTNSIZE+1];
                s = colp - name;
                if  (s > HOSTNSIZE)
                        s = HOSTNSIZE;
                strncpy(hname, name, s);
                hname[s] = '\0';
                if  ((hostid = look_hostname(hname)) == 0)
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

static char **gen_vars(int isexport, unsigned mode)
{
        char            **result;
        unsigned        vcnt, maxr, countr;

        if  ((result = (char **) malloc((Var_seg.nvars + 1)/2 * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        maxr = (Var_seg.nvars + 1) / 2;
        countr = 0;

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                BtvarRef        vp = &vv_ptrs[vcnt].vep->Vent;

                /* Skip ones which are not allowed.  */

                if  (!mpermitted(&vp->var_mode, mode, mypriv->btu_priv))
                        continue;
                if  (isexport >= 0)  {
                        if  (isexport)  {
                                if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                        continue;
                        }
                        else
                                if  (vp->var_flags & VF_EXPORT)
                                        continue;
                }
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
        return  gen_vars(0, BTM_READ);
}

char **gen_wvars(char *prefix)
{
        return  gen_vars(0, BTM_WRITE);
}

char **gen_rvarse(char *prefix)
{
        return  gen_vars(1, BTM_READ);
}

char **gen_wvarse(char *prefix)
{
        return  gen_vars(1, BTM_WRITE);
}

char **gen_rvarsa(char *prefix)
{
        return  gen_vars(-1, BTM_READ);
}

char **gen_wvarsa(char *prefix)
{
        return  gen_vars(-1, BTM_WRITE);
}

/* Stuff to edit formats */

Widget  sep_valw;
static  Widget  listw, eatw;
static  int     whicline;
static  unsigned        char    wfld, isinsert;
static  char    **editing;

static void  fillvdisplist()
{
        char    *cp, *lbp;
        int             nn;
        XmString        str;
        struct  formatdef       *fp;

        cp = *editing;
        while  (*cp)  {
                lbp = bigbuff;
                if  (*cp != '%')  {
                        *lbp++ = '\"';
                        do      *lbp++ = *cp++;
                        while  (*cp  &&  *cp != '%');
                        *lbp++ = '\"';
                        *lbp = '\0';
                }
                else  {
                        cp++;
                        *lbp = ' ';
                        if  (*cp == '<')
                                *lbp = *cp++;
                        lbp++;
                        *lbp++ = ' ';
                        nn = 0;
                        do  nn = nn * 10 + *cp++ - '0';
                        while  (isdigit(*cp));
                        if  (isupper(*cp))
                                fp = &uppertab[*cp - 'A'];
                        else  {
                                if  (*cp)
                                        cp++;
                                continue;
                        }
                        *lbp++ = *cp++;
                        if  (fp->statecode == 0)
                                continue;
                        sprintf(lbp, " %3d ", nn);
                        lbp += 5;
                        if  (!fp->explain)
                                fp->explain = gprompt(fp->statecode+200);
                        strcpy(lbp, fp->explain);
                }
                str = XmStringCreateSimple(bigbuff);
                XmListAddItem(listw, str, 0);
                XmStringFree(str);
        }
}

static void  fld_turn(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)  {
                struct  formatdef  *fp;
                wfld = (unsigned char) n;
                fp = &uppertab[n - 'A'];
                if  (fp->fmt_fn)
                        PUT_TEXTORSPINBOX_INT(sep_valw, fp->sugg_width, 3);
        }
}

static void  endnewedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char    *lbp;
                int     nn;
                XmString        str;
                if  (wfld > 127)
                        return;
                nn = GET_TEXTORSPINBOX_INT(sep_valw);
                if  (nn <= 0)
                        return;
                lbp = bigbuff;
                *lbp++ = XmToggleButtonGadgetGetState(eatw)? '<': ' ';
                sprintf(lbp, " %c %3d ", (char) wfld, nn);
                lbp += 7;
                strcpy(lbp, uppertab[wfld - 'A'].explain);
                str = XmStringCreateSimple(bigbuff);
                if  (isinsert)
                        XmListAddItem(listw, str, whicline <= 0? 0: whicline);
                else
                        XmListReplaceItemsPos(listw, &str, 1, whicline);
                XmStringFree(str);
        }
        XtDestroyWidget(GetTopShell(w));
}

#ifndef HAVE_XM_SPINB_H
void  widup_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_max = 255;
                arrow_lng = 3;
                arrow_incr(sep_valw, NULL);
        }
        else
                CLEAR_ARROW_TIMER
}

void  widdn_cb(Widget w, int subj, XmArrowButtonCallbackStruct *cbs)
{
        if  (cbs->reason == XmCR_ARM)  {
                arrow_min = 1;
                arrow_lng = 3;
                arrow_decr(sep_valw, NULL);
        }
        else
                CLEAR_ARROW_TIMER
}
#endif

static void  newrout(Widget w, int isnew)
{
        Widget  ae_shell, panew, formw, prevleft, fldrc;
        int     *plist, cnt, wotc = 255;
        char    *txt;

        whicline = -1;
        isinsert = isnew;
        if  (XmListGetSelectedPos(listw, &plist, &cnt)  &&  cnt > 0)  {
                whicline = plist[0];
                XtFree((char *) plist);
                if  (!isnew)  {
                        XmStringTable   strlist;
                        XtVaGetValues(listw, XmNitems, &strlist, NULL);
                        XmStringGetLtoR(strlist[whicline-1], XmSTRING_DEFAULT_CHARSET, &txt);
                        if  (*txt == '\"')  {
                                XtFree(txt);
                                return;
                        }
                }
        }
        else  if  (!isnew)
                return;

        CreateEditDlg(w, "vfldedit", &ae_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "width");
#ifdef HAVE_XM_SPINB_H
        prevleft = XtVaCreateManagedWidget("widsp",
                                           xmSpinBoxWidgetClass,        formw,
                                           XmNtopAttachment,            XmATTACH_FORM,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               prevleft,
                                           NULL);

        sep_valw = XtVaCreateManagedWidget("wid",
                                           xmTextFieldWidgetClass,      prevleft,
                                           XmNmaximumValue,             255,
                                           XmNminimumValue,             1,
                                           XmNspinBoxChildType,         XmNUMERIC,
#ifdef  BROKEN_SPINBOX
                                           XmNpositionType,             XmPOSITION_INDEX,
                                           XmNposition,                 10-1,
#else
                                           XmNposition,                 10,
#endif
                                           XmNcolumns,                  3,
                                           XmNeditable,                 False,
                                           XmNcursorPositionVisible,    False,
                                           NULL);

#else
        prevleft = sep_valw = XtVaCreateManagedWidget("wid",
                                                      xmTextFieldWidgetClass,           formw,
                                                      XmNcolumns,                       3,
                                                      XmNmaxWidth,                      3,
                                                      XmNcursorPositionVisible,         False,
                                                      XmNtopAttachment,                 XmATTACH_FORM,
                                                      XmNleftAttachment,                XmATTACH_WIDGET,
                                                      XmNleftWidget,                    prevleft,
                                                      NULL);

        prevleft = CreateArrowPair("wid", formw, (Widget) 0, prevleft, (XtCallbackProc) widup_cb, (XtCallbackProc) widdn_cb, 1, 1);

#endif /* ! HAVE_XM_SPINB_H */

        eatw = XtVaCreateManagedWidget("useleft",
                                       xmToggleButtonGadgetClass,       formw,
                                       XmNborderWidth,                  0,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_WIDGET,
                                       XmNleftWidget,                   prevleft,
                                       NULL);

        fldrc = XtVaCreateManagedWidget("fldtype",
                                        xmRowColumnWidgetClass,         formw,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   sep_valw,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  2,
                                        XmNisHomogeneous,               True,
                                        XmNentryClass,                  xmToggleButtonGadgetClass,
                                        XmNradioBehavior,               True,
                                        NULL);

        if  (isnew)
                PUT_TEXTORSPINBOX_INT(sep_valw, 10, 3);
        else  {
                char    *cp = txt;
                int     nn;
                if  (*cp++ == '<')
                        XmToggleButtonGadgetSetState(eatw, True, False);
                while  (isspace(*cp))
                        cp++;
                wotc = *cp++;
                while  (isspace(*cp))
                        cp++;
                nn = 0;
                do  nn = nn * 10 + *cp++ - '0';
                while  (isdigit(*cp));
                PUT_TEXTORSPINBOX_INT(sep_valw, nn, 3);
                XtFree(txt);
        }

        wfld = 255;
        for  (cnt = 0;  cnt < 26;  cnt++)  {
                Widget  wc;
                struct  formatdef  *fp = &uppertab[cnt];
                if  (fp->statecode == 0)
                        continue;
                if  (!fp->explain)
                        fp->explain = gprompt(fp->statecode + 200);
                sprintf(bigbuff, "%c: %s", cnt + 'A', fp->explain);
                wc = XtVaCreateManagedWidget(bigbuff,
                                             xmToggleButtonGadgetClass, fldrc,
                                             XmNborderWidth,            0,
                                             NULL);
                if  (cnt + 'A' == wotc)  {
                        wfld = cnt + 'A';
                        XmToggleButtonGadgetSetState(wc, True, False);
                }
                XtAddCallback(wc, XmNvalueChangedCallback, (XtCallbackProc) fld_turn, INT_TO_XTPOINTER(cnt + 'A'));
        }

        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewedit, $H{xmbtq new var field});
}

static void  endnewsepedit(Widget w, int data)
{
        if  (data)  {           /* OK pressed */
                char            *txt;
                XmString        str;
                XtVaGetValues(sep_valw, XmNvalue, &txt, NULL);
                sprintf(bigbuff, "\"%s\"", txt[0] == '\0'? " ": txt);
                XtFree(txt);
                str = XmStringCreateSimple(bigbuff);
                if  (isinsert)
                        XmListAddItem(listw, str, whicline <= 0? 0: whicline);
                else
                        XmListReplaceItemsPos(listw, &str, 1, whicline);
                XmStringFree(str);
        }
        XtDestroyWidget(GetTopShell(w));
}

static void  newseprout(Widget w, int isnew)
{
        Widget  ae_shell, panew, formw, prevleft;
        int     *plist, pcnt;
        char    *txt;

        whicline = -1;
        isinsert = isnew;

        if  (XmListGetSelectedPos(listw, &plist, &pcnt)  &&  pcnt > 0)  {
                whicline = plist[0];
                XtFree((char *) plist);
                if  (!isnew)  {
                        XmStringTable   strlist;
                        XtVaGetValues(listw, XmNitems, &strlist, NULL);
                        XmStringGetLtoR(strlist[whicline-1], XmSTRING_DEFAULT_CHARSET, &txt);
                        if  (*txt != '\"')  {
                                XtFree(txt);
                                return;
                        }
                }
        }
        CreateEditDlg(w, "vsepedit", &ae_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "value");
        prevleft = sep_valw = XtVaCreateManagedWidget("val",
                                                      xmTextFieldWidgetClass,   formw,
                                                      XmNtopAttachment,         XmATTACH_FORM,
                                                      XmNleftAttachment,        XmATTACH_WIDGET,
                                                      XmNleftWidget,            prevleft,
                                                      XmNrightAttachment,       XmATTACH_FORM,
                                                      NULL);

        if  (!isnew)  {
                char    *cp = txt;
                char    *lbp = bigbuff;
                if  (*cp == '\"')
                        cp++;
                do  *lbp++ = *cp++;
                while  (*cp  &&  *cp != '\"'  &&  cp[1]);
                *lbp = '\0';
                XmTextSetString(sep_valw, bigbuff);
                XtFree(txt);
        }

        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewsepedit, $H{xmbtq new var separator});
}

static void  delrout()
{
        int     *plist, pcnt;

        if  (XmListGetSelectedPos(listw, &plist, &pcnt)  &&  pcnt > 0)  {
                int     which = plist[0];
                XtFree((char *) plist);
                XmListDeletePos(listw, which);
        }
}

static void  endvdisp(Widget w, int data)
{
        if  (data)  {           /* OK Pressed */
                XmStringTable   strlist;
                int     numstrs, cnt;
                char    *cp, *txt, *ip;
                XtVaGetValues(listw, XmNitems, &strlist, XmNitemCount, &numstrs, NULL);
                cp = bigbuff;
                for  (cnt = 0;  cnt < numstrs;  cnt++)  {
                        XmStringGetLtoR(strlist[cnt], XmSTRING_DEFAULT_CHARSET, &txt);
                        ip = txt;
                        if  (*ip == '\"')  {    /* Separator */
                                ip++;
                                do  *cp++ = *ip++;
                                while  (*ip  &&  *ip != '\"'  &&  ip[1]);
                        }
                        else  {
                                int     wf;
                                *cp++ = '%';
                                if  (*ip == '<')
                                        *cp++ = *ip++;
                                while  (isspace(*ip))
                                        ip++;
                                wf = *ip;
                                do  ip++;
                                while  (isspace(*ip));
                                while  (isdigit(*ip))
                                        *cp++ = *ip++;
                                *cp++ = (char) wf;
                        }
                        XtFree(txt);
                }
                *cp = '\0';
                if  (*editing)
                        free(*editing);
                *editing = stracpy(bigbuff);
                txt = get_vartitle();
                if  (vtitwid)  {
                        XmString  str = XmStringCreateSimple(txt);
                        XtVaSetValues(vtitwid, XmNlabelString, str, NULL);
                        XmStringFree(str);
                }
                free(txt);
                Last_v_ser = 0;
                vdisplay();
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_setvdisplay(Widget parent, int data)
{
        Widget  vd_shell, panew, vdispform, neww, editw, newsepw, editsepw, delw;
        Arg             args[6];
        int             n;

        CreateEditDlg(parent, "Vdisp", &vd_shell, &panew, &vdispform, 5);

        neww = XtVaCreateManagedWidget("Newfld",
                                       xmPushButtonGadgetClass, vdispform,
                                       XmNshowAsDefault,        True,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,            0,
                                       XmNbottomOffset,         0,
                                       XmNtopAttachment,        XmATTACH_FORM,
                                       XmNleftAttachment,       XmATTACH_POSITION,
                                       XmNleftPosition,         0,
                                       NULL);

        editw = XtVaCreateManagedWidget("Editfld",
                                        xmPushButtonGadgetClass,        vdispform,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                2,
                                        NULL);

        newsepw = XtVaCreateManagedWidget("Newsep",
                                          xmPushButtonGadgetClass,      vdispform,
                                          XmNshowAsDefault,             False,
                                          XmNdefaultButtonShadowThickness,      1,
                                          XmNtopOffset,                 0,
                                          XmNbottomOffset,              0,
                                          XmNtopAttachment,             XmATTACH_FORM,
                                          XmNleftAttachment,            XmATTACH_POSITION,
                                          XmNleftPosition,              4,
                                          NULL);

        editsepw = XtVaCreateManagedWidget("Editsep",
                                           xmPushButtonGadgetClass,             vdispform,
                                           XmNshowAsDefault,                    False,
                                           XmNdefaultButtonShadowThickness,     1,
                                           XmNtopOffset,                        0,
                                           XmNbottomOffset,                     0,
                                           XmNtopAttachment,                    XmATTACH_FORM,
                                           XmNleftAttachment,                   XmATTACH_POSITION,
                                           XmNleftPosition,                     6,
                                           NULL);

        delw = XtVaCreateManagedWidget("Delete",
                                       xmPushButtonGadgetClass, vdispform,
                                       XmNshowAsDefault,                False,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,                    0,
                                       XmNbottomOffset,                 0,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_POSITION,
                                       XmNleftPosition,                 8,
                                       NULL);

        XtAddCallback(neww, XmNactivateCallback, (XtCallbackProc) newrout, (XtPointer) 1);
        XtAddCallback(editw, XmNactivateCallback, (XtCallbackProc) newrout, (XtPointer) 0);
        XtAddCallback(newsepw, XmNactivateCallback, (XtCallbackProc) newseprout, (XtPointer) 1);
        XtAddCallback(editsepw, XmNactivateCallback, (XtCallbackProc) newseprout, (XtPointer) 0);
        XtAddCallback(delw, XmNactivateCallback, (XtCallbackProc) delrout, (XtPointer) 0);
        n = 0;
        XtSetArg(args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
        XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNtopWidget, neww); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        listw = XmCreateScrolledList(vdispform, "Vdisplist", args, n);
        editing = data? &var2_format: &var1_format;
        fillvdisplist();
        XtManageChild(listw);
        XtManageChild(vdispform);
        CreateActionEndDlg(vd_shell, panew, (XtCallbackProc) endvdisp, $H{xmbtq vlist fmt dialog});
}

static  char    *outformat;

static void  make_confline(FILE *fp, const char *vname)
{
        fprintf(fp, "%s=%s\n", vname, outformat);
}

void  cb_saveformats(Widget parent, const int ish)
{
        char    *dir = Curr_pwd;
        int     ret;

        disp_str = dir;
        if  (ish)  {
                dir = (char *) 0;
                disp_str = "(Home)";
        }

        outformat = job_format;
        if  ((ret = proc_save_opts(dir, "BTQJOBFLD", make_confline)) != 0)  {
                doerror(parent, ret);
                return;
        }

        outformat = var1_format;
        if  ((ret = proc_save_opts(dir, "BTQVAR1FLD", make_confline)) != 0)
                doerror(parent, ret);

        outformat = var2_format;
        if  ((ret = proc_save_opts(dir, "BTQVAR2FLD", make_confline)) != 0)
                 doerror(parent, ret);
}
