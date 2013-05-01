/* bq_jlist.c -- job list handling for gbch-q

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
#ifdef  HAVE_TERMIOS_H
#include <termios.h>
#endif
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
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
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
#include "magic_ch.h"
#include "sctrl.h"
#include "statenums.h"
#include "errnums.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "formats.h"
#include "optflags.h"
#include "cfile.h"

/* Maximum size of single variable name before we plug a general bit
   of waffle in the hole.  */

#define MAXCDISP        (JPROG_P - JCOND_P - 1)

static  char    Filename[] = __FILE__;
extern  Shipc   Oreq;
extern  char    *Curr_pwd;

jobno_t Cjobno = -1;

#define ppermitted(flg) (mypriv->btu_priv & flg)

int     Jhline,
        Jeline;

static  char    *more_amsg,
                *more_bmsg,
                *defcondstr,
                *localrun;

char    *exportmark, *clustermark;      /* Actually these are for variables */

static  int     more_above,
                more_below,
                jcnt;

static  HelpaltRef      progresslist,
                        sproglist,
                        killlist;

extern const char * const condname[];
extern const char * const assname[];

#define MAXSTEP 5

char    *job_format, *bigbuff;
#define DEFAULT_FORMAT  "%3n %<7N %7U %13H %14I %3p%5L %5t %9c %<4P"

extern  int     hadrfresh;
extern  SHORT   wh_jtitline;
extern  int     JLINES;

extern  WINDOW  *hjscr,
                *tjscr,
                *jscr,
                *escr,
                *hlpscr,
                *Ew;

#define NULLCP          (char *) 0
#define HELPLESS        ((char **(*)()) 0)

#define MAXTITLE        30

int     format_len(const char *);
char    **listcis(char *);

static  struct  sctrl
 wst_title = { $H{btq wgets title}, HELPLESS, MAXTITLE, 0, -1, MAG_OK, 0L, 0L, NULLCP },
 wnt_pty = { $H{btq wgets prio}, HELPLESS, 3, 0, -1, MAG_P, 1L, 255L, NULLCP },
 wnt_ll = { $H{btq wgets ll}, HELPLESS, 5, 0, -1, MAG_P, 1L, 0x7fffL, NULLCP },
 wst_ci = { $H{btq wgets ci}, listcis, CI_MAXNAME, 0, -1, MAG_P, 0L, 0L, NULLCP };

void    doerror(WINDOW *, int);
void    dochelp(WINDOW *, int);
void    qdojerror(unsigned, BtjobRef);
void    endhe(WINDOW *, WINDOW **);
void    notebgwin(WINDOW *, WINDOW *, WINDOW *);
void    viewfile(BtjobRef);
void    viewhols(WINDOW *);
void    qwjimsg(const unsigned, CBtjobRef);
void    wjmsg(const unsigned, const ULONG);
void    wn_fill(WINDOW *, const int, const struct sctrl *, const LONG);
void    ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
void    offersave(char *, const char *);
int     ciprocess();
int     deljob(BtjobRef);
int     modjob(BtjobRef);
int     ownjob(BtjobRef);
int     grpjob(BtjobRef);
int     wtimes(WINDOW *, BtjobRef, const char *);
int     editjcvars(BtjobRef);
int     editjsvars(BtjobRef);
int     rdalts(WINDOW *, int, int, HelpaltRef, int, int);
int     editmwflags(BtjobRef);
int     dounqueue(BtjobRef);
int     editjargs(BtjobRef);
int     editenvir(BtjobRef);
int     editredir(BtjobRef);
int     editprocenv(BtjobRef);
int     propts();
int     fmtprocess(char **, const char, struct formatdef *, struct formatdef *);
LONG    chk_wnum(WINDOW *, const int, struct sctrl *, const LONG, const int);
const  char  *qtitle_of(CBtjobRef);
char    *wgets(WINDOW *, const int, struct sctrl *, const char *);
char    *chk_wgets(WINDOW *, const int, struct sctrl *, const char *, const int);

/* Allocate "more" messages for jobs */

void  initmoremsgs()
{
        more_amsg = gprompt($P{btq more above});
        more_bmsg = gprompt($P{btq more below});
        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
                print_error($E{Missing alternative code});
        }
        if  (!(sproglist = helprdalt($Q{Set job progress code})))  {
                disp_arg[9] = $Q{Set job progress code};
                print_error($E{Missing alternative code});
        }
        if  (!(killlist = helprdalt($Q{btq kill type})))  {
                disp_arg[9] = $Q{btq kill type};
                print_error($E{Missing alternative code});
        }
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
        wnt_pty.vmax = mypriv->btu_maxp;
        wnt_pty.min = mypriv->btu_minp;
        exportmark = gprompt($P{Variable exported flag});
        clustermark = gprompt($P{Variable clustered flag});
        localrun = gprompt($P{Locally run});
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

/* Find current job in queue and adjust pointers as required.  The
   idea is that we try to preserve the position on the screen of
   the current line (even if we are currently looking at the
   print file).  Option - if scrkeep set, move job but keep the
   rest.  */

void  cjfind()
{
        int     row;

        if  (Dispflags & DF_SCRKEEP)  {
                if  (Jhline >= Job_seg.njobs  &&  (Jhline = Job_seg.njobs - JLINES) < 0)
                        Jhline = 0;
                if  (Jeline - Jhline >= JLINES)
                        Jeline = Jhline - JLINES - 1;
        }
        else  {
                for  (row = 0;  row < Job_seg.njobs;  row++)  {
                        if  (jj_ptrs[row]->h.bj_job == Cjobno)  {

                                /* Move top of screen line up/down
                                   queue by same amount as
                                   current job.  This code
                                   assumes that Jeline - Jhline <
                                   JLINES but that we may wind up
                                   with Jhline < 0; */

                                Jhline += row - Jeline;
                                Jeline = row;
                                return;
                        }
                }
                if  (Jeline >= Job_seg.njobs)  {
                        if  (Job_seg.njobs == 0)
                                Jhline = Jeline = 0;
                        else  {
                                Jhline -= Jeline - Job_seg.njobs + 1;
                                Jeline = Job_seg.njobs - 1;
                        }
                }
        }
}

#define MALLINIT        10
#define MALLINC         5

char    **gen_qlist(const char * prefix)
{
        int     reread = 0, jcnt, rt;
        unsigned        rcnt, rmax, pl = prefix? strlen(prefix): 0;
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
                if  (strncmp(tit, prefix, pl) != 0)
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

/*      FORMAT FUNCTIONS
        ================        */

#define BTQ_INLINE
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
#include "inline/jfmt_seq.c"
#include "inline/jfmt_export.c"
#include "inline/jfmt_xit.c"
#include "inline/jfmt_times.c"
#include "inline/jfmt_runt.c"
#include "inline/jfmt_hols.c"

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

char    *get_jobtitle()
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

        /* Set columns for these things so we know to generate a
           prompt for editing them.  */

        wst_title.col = wnt_pty.col = wnt_ll.col = wst_ci.col = -1;
        wst_title.size = MAXTITLE;
        wst_ci.size = CI_MAXNAME;
        wnt_pty.size = 3;
        wnt_ll.size = 5;

        obuflen = format_len(job_format);

        /* Allocate space for title and output buffer */

        result = malloc((unsigned) obuflen);
        if  (bigbuff)
                free(bigbuff);
        bigbuff = malloc(JOBSPACE);
        if  (!result ||  !bigbuff)
                ABORT_NOMEM;

        /* Now set up title Actually this is a waste of time if we
           aren't actually displaying same, but we needed the
           buffer.  */

        rp = result;
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

                /* Special kludge for things we set on screen rather
                   than on a sub-window.  */

                switch  (*cp++)  {
                case  'H':
                case  'h':
                        wst_title.col = (SHORT) (rp - result);
                        wst_title.size = (USHORT) nn;
                        break;
                case  'p':
                        wnt_pty.col = (SHORT) (rp - result);
                        wnt_pty.size = (USHORT) nn;
                        break;
                case  'L':
                        wnt_ll.col = (SHORT) (rp - result);
                        wnt_ll.size = (USHORT) nn;
                        break;
                case  'I':
                        wst_ci.col = (SHORT) (rp - result);
                        wst_ci.size = (USHORT) nn;
                        break;
                }

                if  (fp->statecode == 0)
                        continue;

                /* Get title message if we don't have it.
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

        /* Trim trailing spaces */

        for  (rp--;  rp >= result  &&  *rp == ' ';  rp--)
                ;
        *++rp = '\0';

        return  result;
}

#ifdef  HAVE_TERMINFO
#define DISP_CHAR(w, ch)        waddch(w, (chtype) ch);
#else
#define DISP_CHAR(w, ch)        waddch(w, ch);
#endif

/* Display contents of job file.  */

void  jdisplay()
{
        int     row;

#if     defined(OS_DYNIX) || defined(CURSES_MEGA_BUG)
        wclear(jscr);
#else
        werase(jscr);
#endif

        /* Better not let too big a gap develop....  */

        more_above = 0;

        if  (Jhline < - MAXSTEP)
                Jhline = 0;

        if  ((jcnt = Jhline) < 0)  {
                row = - Jhline;
                jcnt = 0;
        }
        else
                row = 0;

        if  (Jhline > 0)  {
                if  (Jhline <= 1)
                        jcnt = Jhline = 0;
                else  {
                        wstandout(jscr);
                        mvwprintw(jscr,
                                         row,
                                         (COLS - (int) strlen(more_amsg))/2,
                                         more_amsg,
                                         Jhline);
                        wstandend(jscr);
                        row++;
                        more_above = 1;
                }
        }

        more_below = 0;

        for  (;  jcnt < Job_seg.njobs  &&  row < JLINES;  jcnt++, row++)  {
                BtjobRef        jp = jj_ptrs[jcnt];
                struct  formatdef  *fp;
                char            *cp = job_format, *lbp;
                int             currplace = -1, lastplace, nn, inlen, dummy;
                int             isreadable;

                if  (jp->h.bj_job == 0)
                        break;

                if  (row == JLINES - 1  &&  jcnt < Job_seg.njobs - 1) {
                        wstandout(jscr);
                        mvwprintw(jscr,
                                         row,
                                         (COLS - (int) strlen(more_bmsg)) / 2,
                                         more_bmsg,
                                         Job_seg.njobs - jcnt);
                        wstandend(jscr);
                        more_below = 1;
                        if  (Jeline >= jcnt)
                                Jeline = jcnt - 1;
                        break;
                }

                isreadable = mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv);

                wmove(jscr, row, 0);

                while  (*cp)  {
                        if  (*cp != '%')  {
                                DISP_CHAR(jscr, *cp);
                                cp++;
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
                        getyx(jscr, dummy, currplace);
                        inlen = (fp->fmt_fn)(jp, isreadable, nn);
                        lbp = bigbuff;
                        if  (inlen > nn  &&  lastplace >= 0)  {
                                wmove(jscr, row, lastplace);
                                nn = currplace + nn - lastplace;
                        }
                        while  (inlen > 0  &&  nn > 0)  {
                                DISP_CHAR(jscr, *lbp);
                                lbp++;
                                inlen--;
                                nn--;
                        }
                        if  (nn > 0)  {
                                int     ccol;
                                getyx(jscr, dummy, ccol);
                                wmove(jscr, dummy, ccol+nn);
                        }
                }
        }
#ifdef  CURSES_OVERLAP_BUG
        if  (hjscr)  {
                touchwin(hjscr);
                wrefresh(hjscr);
        }
        if  (tjscr)  {
                touchwin(tjscr);
                wrefresh(tjscr);
        }
        touchwin(jscr);
#endif
}

static  char  *gsearchs(const int isback)
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
        ss.col = (SHORT) strlen(gstr);
        ss.magic_p = MAG_OK;
        ss.min = 0L;
        ss.vmax = 0L;
        ss.msg = NULLCP;
        row = Jeline-Jhline+more_above;
        mvwaddstr(jscr, row, 0, gstr);
        wclrtoeol(jscr);

        if  (lastmstr)  {
                ws_fill(jscr, row, &ss, lastmstr);
                gstr = wgets(jscr, row, &ss, lastmstr);
                if  (!gstr)
                        return  NULLCP;
                if  (gstr[0] == '\0')
                        return  lastmstr;
        }
        else  {
                for  (;;)  {
                        gstr = wgets(jscr, row, &ss, "");
                        if  (!gstr)
                                return  NULLCP;
                        if  (gstr[0])
                                break;
                        doerror(jscr, $E{btq jlist no search string});
                }
        }
        if  (lastmstr)
                free(lastmstr);
        return  lastmstr = stracpy(gstr);
}

static int  smatch(const int mline, const char *mstr)
{
        const   char    *title;
        const   char    *tp, *mp;

        if  (!mpermitted(&jj_ptrs[mline]->h.bj_mode, BTM_READ, mypriv->btu_priv))
                return  0;

        for  (title = qtitle_of(jj_ptrs[mline]);  *title;  title++)  {
                for  (tp = title, mp = mstr;  *mp;  tp++, mp++)
                        if  (*mp != '.'  &&  toupper(*mp) != toupper(*tp))
                                goto  ng;
                return  1;
        ng:
                ;
        }
        return  0;
}

/* Search for string in job title.
   Return 0 - need to redisplay jobs
   (Jhline and Jeline suitably mangled) otherwise error code */

static int  dosearch(const int isback)
{
        char    *mstr = gsearchs(isback);
        int     mline;

        if  (!mstr)
                return  0;

        if  (isback)  {
                for  (mline = Jeline - 1;  mline >= 0;  mline--)
                        if  (smatch(mline, mstr))
                                goto  gotit;
                for  (mline = Job_seg.njobs - 1;  mline >= Jeline;  mline--)
                        if  (smatch(mline, mstr))
                                goto  gotit;
        }
        else  {
                for  (mline = Jeline + 1;  (unsigned) mline < Job_seg.njobs;  mline++)
                        if  (smatch(mline, mstr))
                                goto  gotit;
                for  (mline = 0;  mline <= Jeline;  mline++)
                        if  (smatch(mline, mstr))
                                goto  gotit;
        }
        return  $E{btq jlist search string not found};

 gotit:
        Jeline = mline;
        if  (Jeline < Jhline  ||  Jeline - Jhline + more_above + more_below >= JLINES)
                Jhline = Jeline;
        return  0;
}

static void  job_macro(BtjobRef jp, const int num)
{
        char    *prompt = helpprmpt(num + $P{Job or User macro}), *str;
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
                doerror(jscr, $E{Macro error});
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
                int     jsy, jsx;
                struct  sctrl   dd;
                wclrtoeol(jscr);
                waddstr(jscr, str);
                getyx(jscr, jsy, jsx);
                dd.helpcode = $H{Job or User macro};
                dd.helpfn = HELPLESS;
                dd.size = COLS - jsx;
                dd.col = jsx;
                dd.magic_p = MAG_P|MAG_OK;
                dd.min = dd.vmax = 0;
                dd.msg = (char *) 0;
                str = wgets(jscr, jsy, &dd, "");
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
                const   char    *argbuf[3];
                argbuf[0] = str;
                if  (jp)  {
                        argbuf[1] = JOB_NUMBER(jp);
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
                doerror(jscr, $E{Macro fork failed});
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
                        doerror(jscr, $E{Macro command gave signal});
                }
                else  {
                        disp_arg[0] = (status >> 8) & 255;
                        doerror(jscr, $E{Macro command error});
                }
        }
}

/* This accepts input from the screen.  */

int  j_process()
{
        int     num, err_no, i, incr, ret, ch, currow;
        unsigned        retc;
        ULONG           xindx;
        char    *str;
        const   char    *title;
        BtjobRef        jp, djp;

 restartj:
#ifdef CURSES_MEGA_BUG
        clear();
        refresh();
#endif
        if  (hjscr)  {
                touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(hjscr);
#else
                wrefresh(hjscr);
#endif
        }
        if  (tjscr)  {
                touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                wnoutrefresh(tjscr);
#else
                wrefresh(tjscr);
#endif
        }

        notebgwin(hjscr, jscr, tjscr);
        Ew = jscr;
        select_state($S{btq job list state});
 Jdisp:
        jdisplay();
#ifdef  HAVE_TERMINFO
        if  (hlpscr)  {
                touchwin(hlpscr);
                wnoutrefresh(jscr);
                wnoutrefresh(hlpscr);
        }
        if  (escr)  {
                touchwin(escr);
                wnoutrefresh(jscr);
                wnoutrefresh(escr);
        }
#else
        if  (hlpscr)  {
                touchwin(hlpscr);
                wrefresh(jscr);
                wrefresh(hlpscr);
        }
        if  (escr)  {
                touchwin(escr);
                wrefresh(jscr);
                wrefresh(escr);
        }
#endif

 Jmove:
        currow = Jeline - Jhline + more_above;
        wmove(jscr, currow, 0);
        Cjobno = Job_seg.njobs != 0? jj_ptrs[Jeline]->h.bj_job: -1;

 Jrefresh:

#ifdef  HAVE_TERMINFO
        wnoutrefresh(jscr);
        doupdate();
#else
        wrefresh(jscr);
#endif

 nextin:
        if  (hadrfresh)
                return  -1;

        do  ch = getkey(MAG_A|MAG_P);
        while  (ch == EOF  &&  (hlpscr || escr));

        if  (hlpscr)  {
                endhe(jscr, &hlpscr);
                if  (Dispflags & DF_HELPCLR)
                        goto  nextin;
        }
        if  (escr)
                endhe(jscr, &escr);

        switch  (ch)  {
        case  EOF:
                goto  nextin;

        /* Error case - bell character and try again.  */

        default:
                err_no = $E{btq jlist unknown command};
        err:
                doerror(jscr, err_no);
                goto  nextin;

        case  $K{key help}:
                dochelp(jscr, $H{btq job list state});
                goto  nextin;

        case  $K{key refresh}:
                wrefresh(curscr);
                goto  Jrefresh;

        /* Move up or down.  */

        case  $K{key cursor down}:
                Jeline++;
                if  (Jeline >= Job_seg.njobs)  {
                        Jeline--;
ej:                     err_no = $E{btq jlist off end};
                        goto  err;
                }

                if  (++currow < JLINES - more_below)
                        goto  Jmove;
                Jhline++;
                if  (!more_above)
                        Jhline++;
                goto  Jdisp;

        case  $K{key cursor up}:
                if  (Jeline <= 0)  {
bj:                     err_no = $E{btq jlist off beginning};
                        goto  err;
                }
                Jeline--;
                if  (--currow >= more_above)
                        goto  Jmove;
                Jhline = Jeline;
                if  (more_above  &&  Jeline == 1)
                        Jhline = 0;
                goto  Jdisp;

        /* Half/Full screen up/down */

        case  $K{key screen down}:
                if  (Jhline + JLINES - more_above >= Job_seg.njobs)
                        goto  ej;
                Jhline += JLINES - more_above - more_below;
                Jeline += JLINES - more_above - more_below;
                if  (Jeline >= Job_seg.njobs)
                        Jeline = Job_seg.njobs - 1;
        redr:
                jdisplay();
                currow = Jeline - Jhline + more_above;
                if  (more_below  &&  currow > JLINES-2)
                        Jeline--;
                goto  Jmove;

        case  $K{key half screen down}:
                i = (JLINES - more_above - more_below) / 2;
                if  (Jhline + i >= Job_seg.njobs)
                        goto  ej;
                Jhline += i;
                if  (Jeline < Jhline)
                        Jeline = Jhline;
                goto  redr;

        case  $K{key half screen up}:
                if  (Jhline <= 0)
                        goto  bj;
                Jhline -= (JLINES - more_above - more_below) / 2;
        restu:
                if  (Jhline == 1)
                        Jhline = 0;
                jdisplay();
                if  (Jeline - Jhline >= JLINES - more_above - more_below)
                        Jeline = Jhline + JLINES - more_above - more_below - 1;
                goto  Jmove;

        case  $K{key screen up}:
                if  (Jhline <= 0)
                        goto  bj;
                Jhline -= JLINES - more_above - more_below;
                goto  restu;

        case  $K{key top}:
                if  (Jhline != Jeline  &&  Jeline != 0)  {
                        Jeline = Jhline < 0?  0: Jhline;
                        goto  Jmove;
                }
                Jhline = 0;
                Jeline = 0;
                goto  Jdisp;

        case  $K{key bottom}:
                if  (Job_seg.njobs == 0)
                        goto  nextin;
                incr = Jhline + JLINES - more_above - more_below - 1;
                if  (Jeline < incr  &&  incr < Job_seg.njobs - 1)  {
                        Jeline = incr;
                        goto  Jmove;
                }
                if  (Job_seg.njobs > JLINES)
                        Jhline = Job_seg.njobs - JLINES + 1;
                else
                        Jhline = 0;
                Jeline = Job_seg.njobs - 1;
                goto  Jdisp;

        case  $K{key search forward}:
        case  $K{key search backward}:
                if  ((err_no = dosearch(ch == $K{key search backward})) != 0)
                        doerror(jscr, err_no);
                goto  Jdisp;

        case  $K{btq jlist key vars}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                return  1;

                /* Go home.  */

        case  $K{key halt}:
                return  0;

        case  $K{key save opts}:
                propts();
                goto  refall;

        case  $K{btq jlist key setcmdint}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                str = chk_wgets(jscr, currow, &wst_ci, jj_ptrs[Jeline]->h.bj_cmdinterp, $P{btq prompt4 ci});
                if  (str == (char *) 0)
                        goto  Jmove;
                for  (i = 0;  i < Ci_num;  i++)
                        if  (strcmp(Ci_list[i].ci_name, str) == 0)
                                goto  gotci;
                disp_str = str;
                err_no = $E{btq jlist bad command interp};
                wmove(jscr, currow, 0);
                goto  err;
        gotci:
                djp = &Xbuffer->Ring[xindx = getxbuf()];
                *djp = *jj_ptrs[Jeline];
                Oreq.sh_un.sh_jobindex = xindx;
                strcpy(djp->h.bj_cmdinterp, Ci_list[i].ci_name);
                djp->h.bj_ll = Ci_list[i].ci_ll;
                goto  doch;

        case  $K{btq jlist key cmdint}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = ciprocess();
                goto  endop;

        case  $K{btq jlist key delete}:
                if  (Jeline >= Job_seg.njobs)  {
                noj:    err_no = $E{btq no jobs to edit};
                        goto  err;
                }
                ret = deljob(jj_ptrs[Jeline]);
        endop:
                /* Ret should be 0 if no changes to screen (apart from possible error message).
                   Ret should be -1 if screen needs `touching' and resetting.
                   Ret should be 1 if a signal is expected.  */

                switch  (ret)  {
                case  0:
#ifdef  CURSES_MEGA_BUG
                        clear();
                        refresh();
                        break;
#else
#ifdef  CURSES_OVERLAP_BUG
                        if  (hjscr)  {
                                touchwin(hjscr);
                                wrefresh(hjscr);
                        }
                        if  (tjscr)  {
                                touchwin(tjscr);
                                wrefresh(tjscr);
                        }
                        touchwin(jscr);
#endif
                        goto  nextin;
#endif
                case  1:
                        if  (hadrfresh)
                                return  -1;
                }

                /* In case a different key state was selected */

        fixwin:
                select_state($S{btq job list state});
                Ew = jscr;
#ifdef  CURSES_OVERLAP_BUG
                if  (hjscr)  {
                        touchwin(hjscr);
                        wrefresh(hjscr);
                }
                if  (tjscr)  {
                        touchwin(tjscr);
                        wrefresh(tjscr);
                }
#endif
                touchwin(jscr);
                if  (escr)  {
                        touchwin(escr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(jscr);
                        wnoutrefresh(escr);
#else
                        wrefresh(jscr);
                        wrefresh(escr);
#endif
                }
#ifdef  OS_DYNIX
                goto  Jdisp;
#else
                goto  Jmove;
#endif

        case  $K{btq jlist key chmod}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = modjob(jj_ptrs[Jeline]);
                goto  endop;

        case  $K{btq jlist key chown}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                ret = ownjob(jj_ptrs[Jeline]);
                goto  endop;

        case  $K{btq jlist key chgrp}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                ret = grpjob(jj_ptrs[Jeline]);
                goto  endop;

        case  $K{btq jlist key prio}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                if  (!mpermitted(&jj_ptrs[Jeline]->h.bj_mode, BTM_WRITE, mypriv->btu_priv))  {
                        err_no = $E{btq cannot write job};
                        goto  err;
                }
                num = chk_wnum(jscr, currow, &wnt_pty, (LONG) jj_ptrs[Jeline]->h.bj_pri, $P{btq prompt4 pri});
                if  (num < 0)
                        goto  Jmove;

                djp = &Xbuffer->Ring[xindx = getxbuf()];
                *djp = *jj_ptrs[Jeline];
                Oreq.sh_un.sh_jobindex = xindx;
                djp->h.bj_pri = (unsigned char) num;
        doch:
                wjmsg(J_CHANGE, xindx);
                if  ((retc = readreply()) != J_OK)
                        qdojerror(retc, djp);
                freexbuf(xindx);
                goto  Jmove;

        case  $K{btq jlist key ll}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                jp = jj_ptrs[Jeline];
                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))  {
                        err_no = $E{btq cannot write job};
                        goto  err;
                }
                if  (!ppermitted(BTM_SPCREATE))  {
                        err_no = $E{btq no special create permission};
                        goto  err;
                }
                num = chk_wnum(jscr, currow, &wnt_ll, (LONG) jp->h.bj_ll, $P{btq prompt4 ll});
                if  (num < 0)
                        goto  Jmove;
                djp = &Xbuffer->Ring[xindx = getxbuf()];
                *djp = *jp;
                Oreq.sh_un.sh_jobindex = xindx;
                djp->h.bj_ll = (USHORT) num;
                goto  doch;

        case  $K{btq jlist key title}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                jp = jj_ptrs[Jeline];
                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))  {
                        err_no = $E{btq cannot write job};
                        goto  err;
                }
                str = chk_wgets(jscr, currow, &wst_title, qtitle_of(jp), $P{btq prompt4 title});
                if  (str == (char *) 0)
                        goto  Jmove;
                if  (jobqueue)  {
                        char    *ntit;
                        if  (!(ntit = malloc((unsigned) (strlen(jobqueue) + strlen(str) + 2))))
                                ABORT_NOMEM;
                        sprintf(ntit, "%s:%s", jobqueue, str);
                        str = ntit;
                }
                djp = &Xbuffer->Ring[xindx = getxbuf()];
                *djp = *jp;
                if  (!repackjob(djp, jp, (char *) 0, str, 0, 0, 0, (MredirRef) 0, (MenvirRef) 0, (char **) 0))  {
                        err_no = $E{Too many job strings};
                        if  (jobqueue)
                                free(str);
                        wmove(jscr, currow, 0);
                        goto  err;
                }
                if  (jobqueue)
                        free(str);
                goto  doch;

        case  $K{btq jlist key time}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                jp = jj_ptrs[Jeline];
                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))  {
                        err_no = $E{btq cannot write job};
                        goto  err;
                }
                title = qtitle_of(jp);
                ret = wtimes(jscr, jp, title);
                if  (ret != 0)  {
                        jdisplay();
                        wmove(jscr, currow, 0);
                }
                goto  endop;

        case  $K{btq jlist key advnext}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                jp = jj_ptrs[Jeline];
                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))  {
                        err_no = $E{btq cannot write job};
                        goto  err;
                }
                if  (!jp->h.bj_times.tc_istime)  {
                        disp_arg[0] = jp->h.bj_job;
                        disp_str = qtitle_of(jp);
                        err_no = $E{btq no time to advance};
                        goto  err;
                }
                djp = &Xbuffer->Ring[xindx = getxbuf()];
                *djp = *jp;
                Oreq.sh_un.sh_jobindex = xindx;
                djp->h.bj_times.tc_nexttime = advtime(&djp->h.bj_times);
                goto  doch;

        case  $K{btq jlist key view}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                if  (!mpermitted(&jj_ptrs[Jeline]->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                        err_no = $E{btq cannot read job};
                        goto  err;
                }
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                viewfile(jj_ptrs[Jeline]);
        refallcob:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
        refall:
                if  (hjscr)  {
                        touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hjscr);
#else
                        wrefresh(hjscr);
#endif
                }
                if  (tjscr)  {
                        touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(tjscr);
#else
                        wrefresh(tjscr);
#endif
                }
                goto  fixwin;

        case  $K{btq jlist key holidays}:
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                viewhols(jscr);
                goto  refallcob;

        case  $K{btq jlist key cvars}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                ret = editjcvars(jj_ptrs[Jeline]);
                goto  endop;

        case  $K{btq jlist key svars}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = editjsvars(jj_ptrs[Jeline]);
                if  (hjscr)  {
                        touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hjscr);
#else
                        wrefresh(hjscr);
#endif
                }
                if  (tjscr)  {
                        touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(tjscr);
#else
                        wrefresh(tjscr);
#endif
                }
                goto  endop;

        case  $K{btq jlist key setstatus}:
        case  $K{btq jlist key setrunn}:
        case  $K{btq jlist key setcanc}:
        case  $K{btq jlist key goj}:
        case  $K{btq jlist key gojna}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                if  (!mpermitted(&jj_ptrs[Jeline]->h.bj_mode,
                                 (unsigned) (ch == $K{btq jlist key goj} || ch == $K{btq jlist key gojna}? (BTM_WRITE|BTM_KILL): BTM_WRITE),
                                 mypriv->btu_priv))  {
                        err_no = $E{btq cannot write job};
                        goto  err;
                }
                if  (ch == $K{btq jlist key setstatus})  { /* CHECKME CHECKME (position??) */
                        if  ((num = rdalts(jscr, currow, 20, sproglist, -1, $H{Set job progress code})) < 0)
                                goto  Jdisp;
                        if  (num == jj_ptrs[Jeline]->h.bj_progress)
                                goto  Jdisp;
                }
                else  if  (ch == $K{btq jlist key setcanc})
                        num = BJP_CANCELLED;
                else
                        num = BJP_NONE;

                if  (num != jj_ptrs[Jeline]->h.bj_progress)  {
                        djp = &Xbuffer->Ring[xindx = getxbuf()];
                        *djp = *jj_ptrs[Jeline];
                        djp->h.bj_progress = (unsigned char) num;
                        wjmsg(J_CHANGE, xindx);
                        retc = readreply();
                        if  (retc != J_OK)
                                qdojerror(retc, djp);
                        freexbuf(xindx);
                        goto  Jmove;
                }
                if  (ch == $K{btq jlist key goj} || ch == $K{btq jlist key gojna})  {
                        /* Ignore error because the job might have started */
                        qwjimsg(ch == $K{btq jlist key goj}? J_FORCE: J_FORCENA, jj_ptrs[Jeline]);
                        readreply();
                }
                goto  Jmove;

        case  $K{btq jlist key kill}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                if  (!mpermitted(&jj_ptrs[Jeline]->h.bj_mode, BTM_KILL, mypriv->btu_priv))  {
                        err_no = $E{btq jlist cannot kill};
                        goto  err;
                }
                /* CHECKME CHECKME - positon 4???? */
                if  ((num = rdalts(jscr, currow, 4, killlist, -1, $QH{btq kill type})) < 0)
                        goto  Jdisp;

                Oreq.sh_params.param = num;
                qwjimsg(J_KILL, jj_ptrs[Jeline]);
                if  ((retc = readreply()) != J_OK)
                        qdojerror(retc, jj_ptrs[Jeline]);
                goto  Jdisp;

        case  $K{btq jlist key mwflags}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                ret = editmwflags(jj_ptrs[Jeline]);
                goto  endop;

        case  $K{btq jlist key unqueue}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
                ret = dounqueue(jj_ptrs[Jeline]);
                goto  endop;

        case  $K{btq jlist key edargs}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
#ifdef  CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = editjargs(jj_ptrs[Jeline]);
                if  (hjscr)  {
                        touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hjscr);
#else
                        wrefresh(hjscr);
#endif
                }
                if  (tjscr)  {
                        touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(tjscr);
#else
                        wrefresh(tjscr);
#endif
                }
                goto  endop;

        case  $K{btq jlist key edenv}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = editenvir(jj_ptrs[Jeline]);
                if  (hjscr)  {
                        touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hjscr);
#else
                        wrefresh(hjscr);
#endif
                }
                if  (tjscr)  {
                        touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(tjscr);
#else
                        wrefresh(tjscr);
#endif
                }
                goto  endop;

        case  $K{btq jlist key edredir}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = editredir(jj_ptrs[Jeline]);
                if  (hjscr)  {
                        touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hjscr);
#else
                        wrefresh(hjscr);
#endif
                }
                if  (tjscr)  {
                        touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(tjscr);
#else
                        wrefresh(tjscr);
#endif
                }
                goto  endop;

        case  $K{btq jlist key procenv}:
                if  (Jeline >= Job_seg.njobs)
                        goto  noj;
#ifdef CURSES_MEGA_BUG
                clear();
                refresh();
#endif
                ret = editprocenv(jj_ptrs[Jeline]);
                if  (hjscr)  {
                        touchwin(hjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(hjscr);
#else
                        wrefresh(hjscr);
#endif
                }
                if  (tjscr)  {
                        touchwin(tjscr);
#ifdef  HAVE_TERMINFO
                        wnoutrefresh(tjscr);
#else
                        wrefresh(tjscr);
#endif
                }
                goto  endop;

        case  $K{btq jlist key setformat}:
                ret = fmtprocess(&job_format, 'X', uppertab, lowertab);
                str = get_jobtitle();
                if  (wh_jtitline >= 0)  {
                        wmove(hjscr, wh_jtitline, 0);
                        wclrtoeol(hjscr);
                        waddstr(hjscr, str);
                }
                free(str);
                if  (ret)
                        offersave(job_format, "BTQJOBFLD");
                goto  restartj;

        case  $K{key exec}:  case  $K{key exec}+1:case  $K{key exec}+2:
        case  $K{key exec}+3:case  $K{key exec}+4:case  $K{key exec}+5:
        case  $K{key exec}+6:case  $K{key exec}+7:case  $K{key exec}+8:
        case  $K{key exec}+9:
                job_macro(Jeline >= Job_seg.njobs? (BtjobRef) 0: jj_ptrs[Jeline], ch - $K{key exec});
                jdisplay();
                if  (escr)  {
                        touchwin(escr);
                        wrefresh(escr);
                }
                goto  Jmove;
        }
}
