/* xmbq_jlist.c -- job list display for gbch-xmq

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
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
#include "formats.h"
#include "optflags.h"
#include "cfile.h"

static  char    Filename[] = __FILE__;

#ifndef HAVE_XM_SPINB_H
extern void  widdn_cb(Widget, int, XmArrowButtonCallbackStruct *);
extern void  widup_cb(Widget, int, XmArrowButtonCallbackStruct *);
#endif

extern  Shipc   Oreq;

#define ppermitted(flg) (mypriv->btu_priv & flg)

static  char    *defcondstr,
                *localrun;
char            *ccritmark, *cnonavail, *cunread,
                *scritmark, *snonavail, *sunwrite;

char    *exportmark, *clustermark;              /* Actually these are for variables */

static  HelpaltRef      progresslist;

extern const char *const condname[];
extern const char *const assname[];

extern  HelpaltRef      assnames, actnames, stdinnames;
extern  int             redir_actlen, redir_stdinlen;
extern  USHORT          def_assflags;

const char *qtitle_of(CBtjobRef jp)
{
        const   char    *title = title_of(jp);
        const   char    *colp;

        if  (!jobqueue  ||  !(colp = strchr(title, ':')))
                return  title;
        return  colp + 1;
}

#define MALLINIT        10
#define MALLINC         5

char **gen_qlist(char *unused)
{
        int     reread = 0, jcnt, rt;
        unsigned        rcnt, rmax;
        char    **result;
        const   char    *ip;
        char    *saveru = (char *) 0, *saverg = (char *) 0, *savejq = (char *) 0;

        if  (Restru || Restrg || jobqueue)  {
                saveru = Restru;
                saverg = Restrg;
                savejq = jobqueue;
                Restru = (char *) 0;
                Restrg = (char *) 0;
                jobqueue = (char *) 0;
                Last_j_ser = 0;
                rjobfile(1);
                reread++;
        }

        if  ((result = (char **) malloc((unsigned) ((MALLINIT + 1) * sizeof(char *)))) == (char **) 0)
                ABORT_NOMEM;

        result[0] = stracpy("");
        rcnt = 1;
        rmax = MALLINIT;

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                const   char    *tit = title_of(jj_ptrs[jcnt]);
                char    *it;
                unsigned        lng;
                if  (!(ip = strchr(tit, ':')))
                        continue;
                lng = ip - tit + 1;
                if  (!(it = malloc(lng)))
                        ABORT_NOMEM;
                strncpy(it, tit, lng-1);
                it[lng-1] = '\0';
                for  (rt = 0;  rt < rcnt;  rt++)
                        if  (strcmp(it, result[rt]) == 0)  {
                                free(it);
                                goto  clash;
                        }
                if  (rcnt >= rmax)  {
                        rmax += MALLINC;
                        if  ((result = (char **) realloc((char *) result, (rmax+1) * sizeof(char *))) == 0)
                                ABORT_NOMEM;
                }
                result[rcnt++] = it;
        clash:
                ;
        }
        result[rcnt] = (char *) 0;
        if  (reread)  {
                Restru = saveru;
                Restrg = saverg;
                jobqueue = savejq;
                Last_j_ser = 0;
                rjobfile(1);
        }
        return  result;
}

/* Allocate various messages.  */

static  struct  flag_ctrl       {
        int     defcode;                /* Code to look up */
        SHORT   defval;                 /* Default if unreadable */
        USHORT  flag;                   /* Flag involved */
}  flagctrl[] = {
        { $N{Assignment start flag help}, 1, BJA_START },
        { $N{Assignment reverse flag help}, 1, BJA_REVERSE },
        { $N{Assignment exit flag help}, 1, BJA_OK },
        { $N{Assignment error flag help}, 1, BJA_ERROR },
        { $N{Assignment abort flag help}, 1, BJA_ABORT },
        { $N{Assignment cancel flag help}, 0, BJA_CANCEL }
};

#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
HelpaltRef      progresscols;

void  allocate_colours(HelpaltRef pcs)
{
        Pixel           origfg;
        int             cnt, rendcnt = 0;
        XmRendition     rend[10];
        Arg             av;
        XmRenderTable   ortab;

        if  (!progresscols)
                return;

        /* First get the original foreground and the rendition table */

        XtVaGetValues(jwid, XmNforeground, &origfg, XmNrenderTable, &ortab, NULL);
        for  (cnt = 0;  cnt < progresscols->numalt;  cnt++)  {
                char    *cnm = progresscols->list[cnt];
                Pixel   newfg;
                char    nbuf[20];

                /* If blank, default background, skip */

                if  (!*cnm)
                        continue;

                /* Gyrations to get pixel value for colour */

                XtVaSetValues(jwid, XtVaTypedArg, XmNforeground, XmRString, cnm, strlen(cnm)+1, NULL);
                XtVaGetValues(jwid, XmNforeground, &newfg, NULL);
                XtSetArg(av, XmNrenditionForeground, newfg);
                sprintf(nbuf, "Statcol%d", progresscols->alt_nums[cnt]);
                rend[rendcnt] = XmRenditionCreate(jwid, nbuf, &av, 1);
                rendcnt++;
        }
        if  (rendcnt > 0)  {
                XmRenderTable  ortc = XmRenderTableCopy(ortab, NULL, 0);
                XmRenderTable  rtab = XmRenderTableAddRenditions(ortc, rend, rendcnt, XmMERGE_REPLACE);
                XtVaSetValues(jwid, XmNforeground, origfg, XmNrenderTable, rtab, NULL);
        }
        freealts(progresscols);
}
#endif

void  initmoremsgs()
{
        int     cnt;

        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
                print_error($E{Missing alternative code});
        }
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
        progresscols = helprdalt($Q{Job progress clr});
#endif
        if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
                disp_arg[9] = $Q{Repeat unit abbrev};
                print_error($E{Missing alternative code});
        }
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                print_error($E{Missing alternative code});
        }
        if  (!(ifnposses = helprdalt($Q{Ifnposses})))  {
                disp_arg[9] = $Q{Ifnposses};
                print_error($E{Missing alternative code});
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        defcondstr = gprompt($P{btq cond abbrev});
        exportmark = gprompt($P{Variable exported flag});
        clustermark = gprompt($P{Variable clustered flag});
        localrun = gprompt($P{Locally run});
        Const_val = helpnstate($N{Initial constant value});
        if  (!(assnames = helprdalt($Q{Assignment names})))  {
                disp_arg[9] = $Q{Assignment names};
                print_error($E{Missing alternative code});
        }
        ccritmark = gprompt($P{Critical condition marker});
        cnonavail = gprompt($P{cond no equiv local var});
        cunread = gprompt($P{unreadable local var});
        scritmark = gprompt($P{Assignment critical});
        snonavail = gprompt($P{ass no equiv local var});
        sunwrite = gprompt($P{unwriteable local var});
        def_assflags = 0;
        for  (cnt = 0;  cnt < XtNumber(flagctrl);  cnt++)  {
                int     nn = helpnstate(flagctrl[cnt].defcode);
                if  ((nn < 0  &&  flagctrl[cnt].defval) ||  nn > 0)
                        def_assflags |= flagctrl[cnt].flag;
        }
        if  (!(actnames = helprdalt($Q{Redirection type names})))  {
                disp_arg[9] = $Q{Redirection type names};
                print_error($E{Missing alternative code});
        }
        else
                redir_actlen = altlen(actnames);
        if  (!(stdinnames = helprdalt($Q{stdio file names})))  {
                disp_arg[9] = $Q{stdio file names};
                print_error($E{Missing alternative code});
        }
        else
                redir_stdinlen = altlen(stdinnames);
}

char    *job_format;
#define DEFAULT_FORMAT  "%3n %<7N %7U %13H %14I %3p%5L %5t %9c %<4P"
static  char            *obuf, *bigbuff;

#define XMBTQ_INLINE
typedef int     fmt_t;

#include "inline/jfmt_args.c"
#include "inline/jfmt_avoid.c"
#include "inline/jfmt_fcond.c"
#include "inline/jfmt_cond.c"
#include "inline/jfmt_dir.c"
#include "inline/jfmt_env.c"
#include "inline/jfmt_group.c"
#include "inline/jfmt_title.c"
#include "inline/jfmt_interp.c"
#include "inline/jfmt_loadlev.c"
#include "inline/fmtmode.c"
#include "inline/jfmt_mode.c"
#include "inline/jfmt_umask.c"
#include "inline/jfmt_jobno.c"
#include "inline/jfmt_progress.c"
#include "inline/jfmt_prio.c"
#include "inline/jfmt_pid.c"
#include "inline/jfmt_queue.c"
#include "inline/jfmt_qtit.c"
#include "inline/jfmt_redirs.c"
#include "inline/jfmt_repeat.c"
#include "inline/jfmt_ifnposs.c"
#include "inline/jfmt_assfull.c"
#include "inline/jfmt_ass.c"
#include "inline/jfmt_timefull.c"
#include "inline/jfmt_time.c"
#include "inline/jfmt_user.c"
#include "inline/jfmt_ulimit.c"
#include "inline/jfmt_exits.c"
#include "inline/jfmt_orighost.c"
static  int     jcnt;
#include "inline/jfmt_seq.c"
#include "inline/jfmt_export.c"
#include "inline/jfmt_xit.c"
#include "inline/jfmt_times.c"
#include "inline/jfmt_runt.c"
#include "inline/jfmt_hols.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct  formatdef  {
        SHORT   statecode;      /* Code number for heading if applicable */
        SHORT   sugg_width;     /* Suggested width */
        char    *msg;           /* Heading */
        char    *explain;       /* More detailed explanation */
        int     (*fmt_fn)(CBtjobRef, const int, const int);
};

#define NULLCP  (char *) 0

static  struct  formatdef
        uppertab[] = { /* A-Z */
        {       $P{job fmt title}+200+'A',      10,     NULLCP, NULLCP, fmt_args        },      /* A */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* B */
        {       $P{job fmt title}+200+'C',      20,     NULLCP, NULLCP, fmt_condfull    },      /* C */
        {       $P{job fmt title}+200+'D',      20,     NULLCP, NULLCP, fmt_dir         },      /* D */
        {       $P{job fmt title}+200+'E',      30,     NULLCP, NULLCP, fmt_env         },      /* E */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* F */
        {       $P{job fmt title}+200+'G',      7,      NULLCP, NULLCP, fmt_group       },      /* G */
        {       $P{job fmt title}+200+'H',      15,     NULLCP, NULLCP, fmt_title       },      /* H */
        {       $P{job fmt title}+200+'I',      8,      NULLCP, NULLCP, fmt_interp      },      /* I */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* J */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* K */
        {       $P{job fmt title}+200+'L',      5,      NULLCP, NULLCP, fmt_loadlev     },      /* L */
        {       $P{job fmt title}+200+'M',      10,     NULLCP, NULLCP, fmt_mode        },      /* M */
        {       $P{job fmt title}+200+'N',      8,      NULLCP, NULLCP, fmt_jobno       },      /* N */
        {       $P{job fmt title}+200+'O',      8,      NULLCP, NULLCP, fmt_orighost    },      /* O */
        {       $P{job fmt title}+200+'P',      6,      NULLCP, NULLCP, fmt_progress    },      /* P */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* Q */
        {       $P{job fmt title}+200+'R',      20,     NULLCP, NULLCP, fmt_redirs      },      /* R */
        {       $P{job fmt title}+200+'S',      20,     NULLCP, NULLCP, fmt_assfull     },      /* S */
        {       $P{job fmt title}+200+'T',      18,     NULLCP, NULLCP, fmt_timefull    },      /* T */
        {       $P{job fmt title}+200+'U',      7,      NULLCP, NULLCP, fmt_user        },      /* U */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* V */
        {       $P{job fmt title}+200+'W',      5,      NULLCP, NULLCP, fmt_itime       },      /* W */
        {       $P{job fmt title}+200+'X',      10,     NULLCP, NULLCP, fmt_exits       },      /* X */
        {       $P{job fmt title}+200+'Y',      20,     NULLCP, NULLCP, fmt_hols        },      /* Y */
        {       0,                              0,      NULLCP, NULLCP, 0               }       /* Z */
},
        lowertab[] = { /* a-z */
        {       $P{job fmt title}+200+'a',      10,     NULLCP, NULLCP, fmt_avoid       },      /* a */
        {       $P{job fmt title}+200+'b',      5,      NULLCP, NULLCP, fmt_stime       },      /* b */
        {       $P{job fmt title}+200+'c',      10,     NULLCP, NULLCP, fmt_cond        },      /* c */
        {       $P{job fmt title}+200+'d',      5,      NULLCP, NULLCP, fmt_deltime     },      /* d */
        {       $P{job fmt title}+200+'e',      10,     NULLCP, NULLCP, fmt_export      },      /* e */
        {       $P{job fmt title}+200+'f',      5,      NULLCP, NULLCP, fmt_etime       },      /* f */
        {       $P{job fmt title}+200+'g',      5,      NULLCP, NULLCP, fmt_gracetime   },      /* g */
        {       $P{job fmt title}+200+'h',      10,     NULLCP, NULLCP, fmt_qtit        },      /* h */
        {       $P{job fmt title}+200+'i',      5,      NULLCP, NULLCP, fmt_pid         },      /* i */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* j */
        {       $P{job fmt title}+200+'k',      2,      NULLCP, NULLCP, fmt_autoksig    },      /* k */
        {       $P{job fmt title}+200+'l',      8,      NULLCP, NULLCP, fmt_runtime     },      /* l */
        {       $P{job fmt title}+200+'m',      3,      NULLCP, NULLCP, fmt_umask       },      /* m */
        {       $P{job fmt title}+200+'n',      3,      NULLCP, NULLCP, fmt_seq         },      /* n */
        {       $P{job fmt title}+200+'o',      5,      NULLCP, NULLCP, fmt_otime       },      /* o */
        {       $P{job fmt title}+200+'p',      3,      NULLCP, NULLCP, fmt_prio        },      /* p */
        {       $P{job fmt title}+200+'q',      6,      NULLCP, NULLCP, fmt_queue       },      /* q */
        {       $P{job fmt title}+200+'r',      10,     NULLCP, NULLCP, fmt_repeat      },      /* r */
        {       $P{job fmt title}+200+'s',      10,     NULLCP, NULLCP, fmt_ass         },      /* s */
        {       $P{job fmt title}+200+'t',      5,      NULLCP, NULLCP, fmt_time        },      /* t */
        {       $P{job fmt title}+200+'u',      10,     NULLCP, NULLCP, fmt_ulimit      },      /* u */
        {       0,                              0,      NULLCP, NULLCP, 0               },      /* v */
        {       $P{job fmt title}+200+'w',      6,      NULLCP, NULLCP, fmt_ifnposs     },      /* w */
        {       $P{job fmt title}+200+'x',      3,      NULLCP, NULLCP, fmt_xit         },      /* x */
        {       $P{job fmt title}+200+'y',      3,      NULLCP, NULLCP, fmt_sig         },      /* y */
        {       0,                              0,      NULLCP, NULLCP, 0               }       /* z */
};

char *get_jobtitle()
{
        int     nn, obuflen;
        struct  formatdef       *fp;
        char    *cp, *rp, *result, *mp;

        if  (!job_format)  {
                /* Need to get job format,
                   First get version out of message file.
                   Then possibly replace it with a version saved in a config file.
                   If we don't find it anywhere, use the default. */

                char    *njf;
                job_format = helpprmpt($P{Default job list format});
                njf = optkeyword("BTQJOBFLD");
                if  (njf)  {
                        if  (job_format)
                                free(job_format);
                        job_format = njf;
                }
                if  (!job_format)
                        job_format = stracpy(DEFAULT_FORMAT);
        }

        /* Initial pass to discover how much space to allocate */

        obuflen = 1;
        cp = job_format;
        while  (*cp)  {
                if  (*cp++ != '%')  {
                        obuflen++;
                        continue;
                }
                if  (*cp == '<')
                        cp++;
                nn = 0;
                do  nn = nn * 10 + *cp++ - '0';
                while  (isdigit(*cp));
                obuflen += nn;
                if  (isalpha(*cp))
                        cp++;
        }

        /* Allocate space for title result and output buffer */

        result = malloc((unsigned) (obuflen + 1));
        if  (obuf)
                free(obuf);
        if  (bigbuff)
                free(bigbuff);
        obuf = malloc((unsigned) obuflen);
        bigbuff = malloc(JOBSPACE);
        if  (!result ||  !obuf  ||  !bigbuff)
                ABORT_NOMEM;

        /* Now set up title
           Actually this is a waste of time if we aren't actually displaying
           same, but we needed the buffer.  */

        rp = result;
        *rp++ = ' ';
        cp = job_format;
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
                else  if  (islower(*cp))
                        fp = &lowertab[*cp - 'a'];
                else  {
                        if  (*cp)
                                cp++;
                        continue;
                }

                cp++;
                if  (fp->statecode == 0)
                        continue;

                /* Get title message
                   if we don't have it Insert into result */

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

/* Display contents of job file.  */

void  jdisplay()
{
        int     topj = 1, cjobpos = -1, newpos = -1, jlines;
        jobno_t         Cjobno = -1;
        netid_t         Chostno = 0;
        XmString        *elist;

        if  (Job_seg.njobs != 0)  {
                int     *plist, pcnt;

                /* First discover the currently-selected job and the
                   scroll position, if any.  */

                XtVaGetValues(jwid,
                              XmNtopItemPosition,       &topj,
                              XmNvisibleItemCount,      &jlines,
                              NULL);

                if  (XmListGetSelectedPos(jwid, &plist, &pcnt)  &&  pcnt > 0)  {
                        cjobpos = plist[0] - 1;
                        XtFree((char *) plist);
                        if  ((unsigned) cjobpos < Job_seg.njobs && jj_ptrs[cjobpos]->h.bj_job != 0)  {
                                Cjobno = jj_ptrs[cjobpos]->h.bj_job;
                                Chostno = jj_ptrs[cjobpos]->h.bj_hostid;
                        }
                }
                XtVaGetValues(jwid, XmNitems, &elist, NULL);
        }

        XmListDeleteAllItems(jwid);
        rjobfile(1);

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                XmString        str;
                BtjobRef        jp = jj_ptrs[jcnt];
                struct  formatdef  *fp;
                char            *cp = job_format, *rp = obuf, *lbp;
                int             currplace = -1, lastplace, nn, inlen;
                int             isreadable = mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv);
#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
                char            nbuf[20];
#endif
                if  (jp->h.bj_job == Cjobno  &&  jp->h.bj_hostid == Chostno)
                        newpos = jcnt;

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

                        /* Get format char */

                        if  (isupper(*cp))
                                fp = &uppertab[*cp - 'A'];
                        else  if  (islower(*cp))
                                fp = &lowertab[*cp - 'a'];
                        else  {
                                if  (*cp)
                                        cp++;
                                continue;
                        }
                        cp++;
                        if  (!fp->fmt_fn)
                                continue;
                        currplace = rp - obuf;
                        inlen = (fp->fmt_fn)(jp, isreadable, nn);
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

#if defined(HAVE_XMRENDITION) && !defined(BROKEN_RENDITION)
                sprintf(nbuf, "Statcol%d", jp->h.bj_progress);
                str = XmStringGenerate(obuf, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, nbuf);
#else
                str = XmStringCreateLocalized(obuf);
#endif
                XmListAddItem(jwid, str, 0);
                XmStringFree(str);
        }

        /* Adjust scrolling */

        if  (!(Dispflags & DF_SCRKEEP))  {
                if  (newpos >= 0)  {
                        topj += newpos - cjobpos;
                        if  (topj <= 0)
                                topj = 1;
                }
        }
        if  (topj+jlines > Job_seg.njobs) /* Shrunk */
                XmListSetBottomPos(jwid, (int) Job_seg.njobs);
        else
                XmListSetPos(jwid, topj);

        if  (newpos >= 0)       /* Only gets set if we had one */
                XmListSelectPos(jwid, newpos+1, False);
}

BtjobRef  getselectedjob(unsigned perm)
{
        int     *plist, pcnt;

        if  (XmListGetSelectedPos(jwid, &plist, &pcnt)  &&  pcnt > 0)  {
                BtjobRef  result = jj_ptrs[plist[0] - 1];
                XtFree((char *) plist);
                if  (!mpermitted(&result->h.bj_mode, perm, mypriv->btu_priv))  {
                        disp_arg[0] = result->h.bj_job;
                        disp_str = qtitle_of(result);
                        disp_str2 = result->h.bj_mode.o_user;
                        doerror(jwid, $EH{xmbtq job op not permitted});
                        return  NULL;
                }
                return  result;
        }
        doerror(jwid, Job_seg.njobs != 0? $EH{xmbtq no job selected}: $EH{xmbtq no jobs to select});
        return  NULL;
}

/* Check ok to delete command interpreter (not referred to in any job).  */

int  chk_okcidel(const int n)
{
        unsigned  jind = Job_seg.dptr->js_q_head;
        const   char    *ciname = Ci_list[n].ci_name;

        while  (jind != JOBHASHEND)  {
                if  (strcmp(Job_seg.jlist[jind].j.h.bj_cmdinterp, ciname) == 0)
                        return  0;
                jind = Job_seg.jlist[jind].q_nxt;
        }
        return  1;
}

extern  Widget  sep_valw;
static  Widget  listw, eatw;
static  int     whicline;
static  unsigned        char    wfld, isinsert;

static void  filljdisplist()
{
        char    *cp, *lbp;
        int             nn;
        XmString        str;
        struct  formatdef       *fp;

        cp = job_format;
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
                        else  if  (islower(*cp))
                                fp = &lowertab[*cp - 'a'];
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
                if  (isupper(n))
                        fp = &uppertab[n - 'A'];
                else
                        fp = &lowertab[n - 'a'];
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
                strcpy(lbp, isupper(wfld)? uppertab[wfld - 'A'].explain: lowertab[wfld - 'a'].explain);
                str = XmStringCreateSimple(bigbuff);
                if  (isinsert)
                        XmListAddItem(listw, str, whicline <= 0? 0: whicline);
                else
                        XmListReplaceItemsPos(listw, &str, 1, whicline);
                XmStringFree(str);
        }
        XtDestroyWidget(GetTopShell(w));
}

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

        CreateEditDlg(w, "jfldedit", &ae_shell, &panew, &formw, 3);
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
                                           XmNcolumns,                  3,
                                           XmNeditable,                 False,
                                           XmNcursorPositionVisible,    False,

#ifdef  BROKEN_SPINBOX
                                           XmNpositionType,             XmPOSITION_INDEX,
                                           XmNposition,                 10-1,
#else
                                           XmNposition,                 10,
#endif
                                           NULL);
#else  /* ! HAVE_XM_SPINB_H */
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
#endif
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
#ifdef HAVE_XM_SPINB_H
                                        XmNtopWidget,                   prevleft,
#else
                                        XmNtopWidget,                   sep_valw,
#endif
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNrightAttachment,             XmATTACH_FORM,
                                        XmNpacking,                     XmPACK_COLUMN,
                                        XmNnumColumns,                  3,
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

        for  (cnt = 0;  cnt < 26;  cnt++)  {
                Widget  wc;
                struct  formatdef  *fp = &lowertab[cnt];
                if  (fp->statecode == 0)
                        continue;
                if  (!fp->explain)
                        fp->explain = gprompt(fp->statecode + 200);
                sprintf(bigbuff, "%c: %s", cnt + 'a', fp->explain);
                wc = XtVaCreateManagedWidget(bigbuff,
                                             xmToggleButtonGadgetClass, fldrc,
                                             XmNborderWidth,            0,
                                             NULL);
                if  (cnt + 'a' == wotc)  {
                        wfld = cnt + 'a';
                        XmToggleButtonGadgetSetState(wc, True, False);
                }
                XtAddCallback(wc, XmNvalueChangedCallback, (XtCallbackProc) fld_turn, INT_TO_XTPOINTER(cnt + 'a'));
        }

        XtManageChild(formw);
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewedit, $H{xmbtq new job field});
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
        CreateEditDlg(w, "jsepedit", &ae_shell, &panew, &formw, 3);
        prevleft = place_label_topleft(formw, "value");
        prevleft = sep_valw = XtVaCreateManagedWidget("val",
                                                  xmTextFieldWidgetClass,       formw,
                                                  XmNtopAttachment,             XmATTACH_FORM,
                                                  XmNleftAttachment,            XmATTACH_WIDGET,
                                                  XmNleftWidget,                prevleft,
                                                  XmNrightAttachment,           XmATTACH_FORM,
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
        CreateActionEndDlg(ae_shell, panew, (XtCallbackProc) endnewsepedit, $H{xmbtq new job separator});
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

static void  endjdisp(Widget w, int data)
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
                if  (job_format)
                        free(job_format);
                job_format = stracpy(bigbuff);
                txt = get_jobtitle();
                if  (jtitwid)  {
                        XmString  str = XmStringCreateSimple(txt);
                        XtVaSetValues(jtitwid, XmNlabelString, str, NULL);
                        XmStringFree(str);
                }
                free(txt);
                Last_j_ser = 0;
                jdisplay();
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_setjdisplay(Widget parent)
{
        Widget  jd_shell, panew, jdispform, neww, editw, newsepw, editsepw, delw;
        Arg             args[6];
        int             n;

        CreateEditDlg(parent, "Jdisp", &jd_shell, &panew, &jdispform, 5);
        neww = XtVaCreateManagedWidget("Newfld",
                                       xmPushButtonGadgetClass, jdispform,
                                       XmNshowAsDefault,        True,
                                       XmNdefaultButtonShadowThickness, 1,
                                       XmNtopOffset,            0,
                                       XmNbottomOffset,         0,
                                       XmNtopAttachment,        XmATTACH_FORM,
                                       XmNleftAttachment,       XmATTACH_POSITION,
                                       XmNleftPosition,         0,
                                       NULL);

        editw = XtVaCreateManagedWidget("Editfld",
                                        xmPushButtonGadgetClass,        jdispform,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                2,
                                        NULL);

        newsepw = XtVaCreateManagedWidget("Newsep",
                                          xmPushButtonGadgetClass,      jdispform,
                                          XmNshowAsDefault,             False,
                                          XmNdefaultButtonShadowThickness,      1,
                                          XmNtopOffset,                 0,
                                          XmNbottomOffset,              0,
                                          XmNtopAttachment,             XmATTACH_FORM,
                                          XmNleftAttachment,            XmATTACH_POSITION,
                                          XmNleftPosition,              4,
                                          NULL);

        editsepw = XtVaCreateManagedWidget("Editsep",
                                           xmPushButtonGadgetClass,             jdispform,
                                           XmNshowAsDefault,                    False,
                                           XmNdefaultButtonShadowThickness,     1,
                                           XmNtopOffset,                        0,
                                           XmNbottomOffset,                     0,
                                           XmNtopAttachment,                    XmATTACH_FORM,
                                           XmNleftAttachment,                   XmATTACH_POSITION,
                                           XmNleftPosition,                     6,
                                           NULL);

        delw = XtVaCreateManagedWidget("Delete",
                                       xmPushButtonGadgetClass, jdispform,
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
        listw = XmCreateScrolledList(jdispform, "Jdisplist", args, n);
        filljdisplist();
        XtManageChild(listw);
        XtManageChild(jdispform);
        CreateActionEndDlg(jd_shell, panew, (XtCallbackProc) endjdisp, $H{xmbtq jlist fmt dialog});
}
