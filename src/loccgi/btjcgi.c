/* btjcgi.c -- CGI job list

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
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
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
#include "jvuprocs.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "cfile.h"
#include "files.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "helpalt.h"
#include "formats.h"
#include "optflags.h"
#include "cgiuser.h"
#include "cgifndjb.h"
#include "xihtmllib.h"
#include "listperms.h"

#define BTJLIST_INLINE

#define MAXMODE 16

static  char    Filename[] = __FILE__;

#define IPC_MODE        0600

extern  int     Ctrl_chan;

HelpaltRef      progresslist;

char            *localrun;

unsigned  char  exportflag = BJ_EXPORT|BJ_REMRUNNABLE;

char            sortflag, hadhdrarg;

extern const char *const condname[];
extern const char *const assname[];

char            *formatstring;
char    sdefaultfmt[] = "LN LU LH LI Lp LL Lt Lc LP";

unsigned        jno_width;

char    bigbuff[JOBSPACE];

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  fmt_setup()
{
        if  (!formatstring)
                formatstring = sdefaultfmt;
}

static int  sort_j(BtjobRef *a, BtjobRef *b)
{
        BtjobhRef       aj = &(*a)->h, bj = &(*b)->h;
        TimeconRef      at = &aj->bj_times,
                        bt = &bj->bj_times;

        /* No we can't do subtraction (ever heard of overflow ?)  */

        if  (!at->tc_istime)  {
                if  (!bt->tc_istime)
                        return  aj->bj_job < bj->bj_job? -1: aj->bj_job == bj->bj_job? 0: 1;
                return  -1;
        }
        else  if  (!bt->tc_istime)
                return  1;

        return  at->tc_nexttime < bt->tc_nexttime? -1:
                at->tc_nexttime > bt->tc_nexttime? 1:
                aj->bj_job < bj->bj_job? -1:
                aj->bj_job == bj->bj_job? 0: 1;
}

static void  setup_main_files()
{
        int     ret;

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                html_disperror($E{Scheduler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                html_disperror($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

        /* Open the other files. No read yet until the scheduler is
           aware of our existence, which it won't be until we
           send it a message.  */

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                html_disperror(ret);
                exit(E_SETUP);
        }
        openjfile(0, 0);
        rjobfile(1);
        if  (sortflag)
                qsort(QSORTP1 jj_ptrs, Job_seg.njobs, sizeof(BtjobRef), QSORTP4 sort_j);
        openvfile(0, 0);
        rvarfile(1);
}

void  setup_for_jobextract(char *arg, struct jobswanted *jw)
{
        int                     retc;

        setup_main_files();
        if  ((retc = decode_jnum(arg, jw)) != 0)  {
                html_disperror(retc);
                exit(E_USAGE);
        }
        if  (!find_job(jw))  {
                html_out_cparam_file("jobgone", 1, arg);
                exit(E_NOJOB);
        }
}

typedef unsigned        fmt_t;

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
#include "inline/jfmt_export.c"
#include "inline/jfmt_xit.c"
#include "inline/jfmt_times.c"
#include "inline/jfmt_runt.c"
#include "inline/jfmt_hols.c"

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct  formatdef  {
        SHORT   statecode;      /* Code number for heading if applicable */
        char    *msg;           /* Heading */
        unsigned (*fmt_fn)(CBtjobRef, const int, const int);
};

#define NULLCP  (char *) 0

struct  formatdef
        uppertab[] = { /* A-Z */
        {       $P{job fmt title}+'A',  NULLCP, fmt_args        },      /* A */
        {       0,                      NULLCP, 0               },      /* B */
        {       $P{job fmt title}+'C',  NULLCP, fmt_condfull    },      /* C */
        {       $P{job fmt title}+'D',  NULLCP, fmt_dir         },      /* D */
        {       $P{job fmt title}+'E',  NULLCP, fmt_env         },      /* E */
        {       0,                      NULLCP, 0               },      /* F */
        {       $P{job fmt title}+'G',  NULLCP, fmt_group       },      /* G */
        {       $P{job fmt title}+'H',  NULLCP, fmt_title       },      /* H */
        {       $P{job fmt title}+'I',  NULLCP, fmt_interp      },      /* I */
        {       0,                      NULLCP, 0               },      /* J */
        {       0,                      NULLCP, 0               },      /* K */
        {       $P{job fmt title}+'L',  NULLCP, fmt_loadlev     },      /* L */
        {       $P{job fmt title}+'M',  NULLCP, fmt_mode        },      /* M */
        {       $P{job fmt title}+'N',  NULLCP, fmt_jobno       },      /* N */
        {       $P{job fmt title}+'O',  NULLCP, fmt_orighost    },      /* O */
        {       $P{job fmt title}+'P',  NULLCP, fmt_progress    },      /* P */
        {       0,                      NULLCP, 0               },      /* Q */
        {       $P{job fmt title}+'R',  NULLCP, fmt_redirs      },      /* R */
        {       $P{job fmt title}+'S',  NULLCP, fmt_assfull     },      /* S */
        {       $P{job fmt title}+'T',  NULLCP, fmt_timefull    },      /* T */
        {       $P{job fmt title}+'U',  NULLCP, fmt_user        },      /* U */
        {       0,                      NULLCP, 0               },      /* V */
        {       $P{job fmt title}+'W',  NULLCP, fmt_itime       },      /* W */
        {       $P{job fmt title}+'X',  NULLCP, fmt_exits       },      /* X */
        {       $P{job fmt title}+'Y',  NULLCP, fmt_hols        },      /* Y */
        {       0,                      NULLCP, 0               }       /* Z */
},
        lowertab[] = { /* a-z */
        {       $P{job fmt title}+'a',  NULLCP, fmt_avoid       },      /* a */
        {       $P{job fmt title}+'b',  NULLCP, fmt_stime       },      /* b */
        {       $P{job fmt title}+'c',  NULLCP, fmt_cond        },      /* c */
        {       $P{job fmt title}+'d',  NULLCP, fmt_deltime     },      /* d */
        {       $P{job fmt title}+'e',  NULLCP, fmt_export      },      /* e */
        {       $P{job fmt title}+'f',  NULLCP, fmt_etime       },      /* f */
        {       $P{job fmt title}+'g',  NULLCP, fmt_gracetime   },      /* g */
        {       $P{job fmt title}+'h',  NULLCP, fmt_qtit        },      /* h */
        {       $P{job fmt title}+'i',  NULLCP, fmt_pid         },      /* i */
        {       0,                      NULLCP, 0               },      /* j */
        {       $P{job fmt title}+'k',  NULLCP, fmt_autoksig    },      /* k */
        {       $P{job fmt title}+'l',  NULLCP, fmt_runtime     },      /* l */
        {       $P{job fmt title}+'m',  NULLCP, fmt_umask       },      /* m */
        {       0,                      NULLCP, 0               },      /* n */
        {       $P{job fmt title}+'o',  NULLCP, fmt_otime       },      /* o */
        {       $P{job fmt title}+'p',  NULLCP, fmt_prio        },      /* p */
        {       $P{job fmt title}+'q',  NULLCP, fmt_queue       },      /* q */
        {       $P{job fmt title}+'r',  NULLCP, fmt_repeat      },      /* r */
        {       $P{job fmt title}+'s',  NULLCP, fmt_ass         },      /* s */
        {       $P{job fmt title}+'t',  NULLCP, fmt_time        },      /* t */
        {       $P{job fmt title}+'u',  NULLCP, fmt_ulimit      },      /* u */
        {       0,                      NULLCP, 0               },      /* v */
        {       $P{job fmt title}+'w',  NULLCP, fmt_ifnposs     },      /* w */
        {       $P{job fmt title}+'x',  NULLCP, fmt_xit         },      /* x */
        {       $P{job fmt title}+'y',  NULLCP, fmt_sig         },      /* y */
        {       0,                      NULLCP, 0               }       /* z */
};

void  print_hdrfmt(struct formatdef *fp)
{
        if  (!fp->fmt_fn)
                return;
        if  (!fp->msg)
                fp->msg = gprompt(fp->statecode);
        html_fldprint(fp->msg);
}

struct  altype  {
        char    *str;           /* Align string for html */
        char    ch;             /* Align char */
}  altypes[] = {
        {       "left",         'L' },
        {       "right",        'R' },
        {       "center",       'C' }
};

#define NALIGNTYPES (sizeof(altypes) / sizeof(struct altype))

struct  altype  *commonest_align = &altypes[0];

struct altype *lookup_align(const int alch)
{
        int     cnt;
        for  (cnt = 0;  cnt < NALIGNTYPES;  cnt++)
                if  (altypes[cnt].ch == alch)
                        return  &altypes[cnt];
        return  commonest_align;
}

struct  colfmt  {
        struct  formatdef  *form;
        char    *alstr;
};

struct  colfmt  *cflist;
int     ncolfmts, maxcolfmts;

#define INITCF  10
#define INCCF   5

void  find_commonest(char *fp)
{
        int     rvec[NALIGNTYPES];
        int     cnt, mx = 0, ind = 0, fmch;
        struct  altype  *whichalign;
        struct  formatdef  *fd;

        for  (cnt = 0;  cnt < NALIGNTYPES;  cnt++)
                rvec[cnt] = 0;

        if  (!(cflist = (struct colfmt *) malloc(INITCF * sizeof(struct colfmt))))
                ABORT_NOMEM;
        maxcolfmts = INITCF;

        while  (*fp)  {
                while  (*fp  &&  !isalpha(*fp))
                        fp++;
                if  (!*fp)
                        break;
                whichalign = lookup_align(*fp++);
                rvec[whichalign - &altypes[0]]++;
                fmch = *fp++;
                if  (!isalpha(fmch))
                        break;
                fd = isupper(fmch)? &uppertab[fmch - 'A']: &lowertab[fmch - 'a'];
                if  (!fd->fmt_fn)
                        continue;
                if  (ncolfmts >= maxcolfmts)  {
                        maxcolfmts += INCCF;
                        cflist = (struct colfmt *) realloc((char *) cflist, (unsigned) (maxcolfmts * sizeof(struct colfmt)));
                        if  (!cflist)
                                ABORT_NOMEM;
                }
                cflist[ncolfmts].form = fd;
                cflist[ncolfmts].alstr = whichalign->str;
                ncolfmts++;
        }

        for  (cnt = 0;  cnt < NALIGNTYPES;  cnt++)
                if  (mx < rvec[cnt])  {
                        mx = rvec[cnt];
                        ind = cnt;
                }
        commonest_align = &altypes[ind];

        for  (cnt = 0;  cnt < ncolfmts;  cnt++)
                if  (cflist[cnt].alstr == commonest_align->str)
                        cflist[cnt].alstr = (char *) 0;
}

void  startrow()
{
        printf("<tr align=%s>\n", commonest_align->str);
}

void  startcell(const int celltype, const char *str)
{
        if  (str)
                printf("<t%c align=%s>", celltype, str);
        else
                printf("<t%c>", celltype);
}

/* Display contents of job file.  */

void  jdisplay()
{
        int     jcnt, fcnt;
        unsigned  pflgs = 0;

        /* <TABLE> included in pre-list file so as to possibly include formatting elements */

        find_commonest(formatstring);

        /* Possibly insert header */

        if  (Dispflags & DF_HAS_HDR)  {
                /* Possibly insert first header showing options */
                if  (jobqueue || Restru || Restrg || exportflag != (BJ_EXPORT|BJ_REMRUNNABLE))  {
                        startrow();
                        printf("<th colspan=%d align=center>", ncolfmts+1);
                        fputs(gprompt($P{btjcgi restr start}), stdout);
                        html_fldprint(gprompt($P{btjcgi restr view}));
                        if  (jobqueue)  {
                                html_fldprint(gprompt($P{btjcgi restr queue}));
                                html_fldprint(jobqueue);
                                if  (!(Dispflags & DF_SUPPNULL))
                                        html_fldprint(gprompt($P{btjcgi restr null}));
                        }
                        if  (Restru)  {
                                html_fldprint(gprompt($P{btjcgi restr users}));
                                html_fldprint(Restru);
                        }
                        if  (Restrg)  {
                                html_fldprint(gprompt($P{btjcgi restr groups}));
                                html_fldprint(Restrg);
                        }
                        if  (exportflag != (BJ_EXPORT|BJ_REMRUNNABLE))
                                html_fldprint(gprompt(exportflag & BJ_REMRUNNABLE? $P{btjcgi restr plusrr}:
                                                      $P{btjcgi restr loco}));
                        fputs(gprompt($P{btjcgi restr end}), stdout);
                        fputs("</th></tr>\n", stdout);
                }
                startrow();
                startcell('h', commonest_align->str); /* Blank space in place of checkbox */
                fputs("</th>", stdout);
                for  (fcnt = 0;  fcnt < ncolfmts;  fcnt++)  {
                        startcell('h', cflist[fcnt].alstr);
                        print_hdrfmt(cflist[fcnt].form);
                        fputs("</th>", stdout);
                }
                fputs("</tr>\n", stdout);
        }

        /* Final run-through to output stuff */

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                CBtjobRef       jp = jj_ptrs[jcnt];
                int             isreadable = 0;
                unsigned        hval = 0;

                if  (jp->h.bj_job == 0)
                        break;
                if  (jp->h.bj_hostid  &&  (jp->h.bj_jflags & exportflag) == 0)
                        continue;

                if  (mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                        isreadable = 1;
                        hval |= LV_MINEORVIEW|LV_LOCORRVIEW;
                }
                if  (mpermitted(&jp->h.bj_mode, BTM_WRITE, mypriv->btu_priv))
                        hval |= LV_CHANGEABLE;
                if  (mpermitted(&jp->h.bj_mode, BTM_DELETE, mypriv->btu_priv))
                        hval |= LV_DELETEABLE;
                if  (mpermitted(&jp->h.bj_mode, BTM_KILL, mypriv->btu_priv))
                        hval |= LV_KILLABLE;
                if  (mpermitted(&jp->h.bj_mode, BTM_WRMODE, mypriv->btu_priv))
                        hval |= LV_CHMODABLE;
                if  (jp->h.bj_progress >= BJP_STARTUP1)
                        hval |= LV_PROCESS;

                startrow();
                startcell('d', commonest_align->str);
                fmt_jobno(jp, isreadable, 0);
                printf("<input type=checkbox name=jobs value=\"%s,%u\"></td>", bigbuff, hval);

                for  (fcnt = 0;  fcnt < ncolfmts;  fcnt++)  {
                        startcell('d', cflist[fcnt].alstr);
                        bigbuff[0] = '\0';
                        (cflist[fcnt].form->fmt_fn)(jp, isreadable, 0);
                        html_fldprint(bigbuff);
                        fputs("</td>", stdout);
                }
                fputs("</tr>\n", stdout);
        }

        pflgs |= GLV_ACCESSF | GLV_FREEZEF;
        if  (mypriv->btu_priv & BTM_CREATE)
                pflgs |= GLV_ANYCHANGES;
        printf("</table>\n<input type=hidden name=privs value=%u>\n", pflgs);
        printf("<input type=hidden name=prio value=\"%d,%d,%d\">\n",
               mypriv->btu_defp, mypriv->btu_minp, mypriv->btu_maxp);
}

struct  arginterp  {
        char    *argname;
        unsigned  short  flags;
#define AIF_NOARG       0
#define AIF_ARG         1
        int     (*arg_fn)(char *);
};

int  perf_listformat(char *notused)
{
        struct  formatdef   *fp;
        int     lett;
        char    *msg;

        html_out_or_err("listfmt_pre", 1);
        for  (fp = &uppertab[0],  lett = 'A';  fp < &uppertab[26];  lett++, fp++)
                if  (fp->statecode != 0)  {
                        msg = gprompt(fp->statecode + 400);
                        printf("list_format_code(\'%c\', \'L\', \"%s\");\n", lett, msg);
                        free(msg);
                }
        for  (fp = &lowertab[0],  lett = 'a';  fp < &lowertab[26];  lett++, fp++)
                if  (fp->statecode != 0)  {
                        msg = gprompt(fp->statecode + 400);
                        printf("list_format_code(\'%c\', \'L\', \"%s\");\n", lett, msg);
                        free(msg);
                }

        fmt_setup();
        printf("existing_formats(\"%s\");\n", formatstring);
        html_out_param_file("listfmt_post", 0, 0, html_cookexpiry());
        exit(0);
        return  0;
}

extern int  perf_optselect(char *);

int  set_queue(char *arg)
{
        jobqueue = *arg? arg: (char *) 0;
        return  1;
}

int  set_user(char *arg)
{
        Restru = *arg? arg: (char *) 0;
        return  1;
}

int  set_group(char *arg)
{
        Restrg = *arg? arg: (char *) 0;
        return  1;
}

int  set_incnull(char *arg)
{
        switch  (tolower(*arg))  {
        case  'a':case  'y':case  '1':
                Dispflags &= ~DF_SUPPNULL;
                return  1;
        case  'n':case  '0':
                Dispflags |= DF_SUPPNULL;
                return  1;
        default:
                return  0;
        }
}

int  set_header(char *arg)
{
        hadhdrarg = 1;
        switch  (tolower(*arg))  {
        case  'y':case '1':
                Dispflags |= DF_HAS_HDR;
                return  1;
        case  'n':case '0':
                Dispflags &= ~DF_HAS_HDR;
                return  1;
        default:
                return  0;
        }
}

int  set_exported(char *arg)
{
        switch  (tolower(*arg))  {
        case  'r':
                exportflag = BJ_REMRUNNABLE;
                return  1;
        case  'a':
                exportflag = BJ_EXPORT|BJ_REMRUNNABLE;
                return  1;
        case  'l':
                exportflag = 0;
                return  1;
        default:
                return  0;
        }
}

int  set_sort(char *arg)
{
        switch  (tolower(*arg))  {
        case  'y':case '1':
                sortflag = 1;
                return  1;
        case  'n':case '0':
                sortflag = 0;
                return  1;
        default:
                return  0;
        }
}

int  set_format(char *arg)
{
        formatstring = arg;
        return  1;
}

static char *tof(const USHORT x)
{
        return  x? "true": "false";
}

#define QMALLINIT       10
#define QMALLINC        5

char **gen_qlist()
{
        char    **result;
        unsigned  rcnt = 1, rmax = QMALLINIT, jcnt;

        if  (!(result = (char **) malloc((QMALLINIT+1) * sizeof(char *))))
                ABORT_HTML_NOMEM;

        result[0] = stracpy("");

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                const   char    *tit = title_of(jj_ptrs[jcnt]);
                char            *it, *ip;
                unsigned        lng, rt;

                if  (!(ip = strchr(tit, ':')))
                        continue;

                lng = ip - tit + 1;
                if  (!(it = malloc(lng)))
                        ABORT_HTML_NOMEM;

                strncpy(it, tit, lng-1);
                it[lng-1] = '\0';

                for  (rt = 0;  rt < rcnt;  rt++)
                        if  (strcmp(it, result[rt]) == 0)  {
                                free(it);
                                goto  clash;
                        }
                if  (rcnt >= rmax)  {
                        rmax += QMALLINC;
                        if  ((result = (char **) realloc((char *) result, (rmax+1) * sizeof(char *))) == 0)
                                ABORT_NOMEM;
                }
                result[rcnt++] = it;
        clash:
                ;
        }
        result[rcnt] = (char *) 0;
        return  result;
}

char **gen_cilist()
{
        unsigned        cicnt;
        char            **result;
        unsigned        countr;

        if  ((result = (char **) malloc((unsigned) ((Ci_num + 1) * sizeof(char *)))) == (char **) 0)
                ABORT_HTML_NOMEM;

        countr = 0;

        for  (cicnt = 0;  cicnt < Ci_num;  cicnt++)
                if  (Ci_list[cicnt].ci_name[0] != '\0')
                        result[countr++] = stracpy(Ci_list[cicnt].ci_name);

        result[countr] = (char *) 0;
        return  result;
}

/* Generate a list of variables which the user has access to.
   "permflags" gives the required permission (read, write or read/write)
   expr is whether the thing is exported */

#define INCVLIST        10

char **gen_vlist(const USHORT permflags, const int expr)
{
        char    **result;
        unsigned  vcnt, maxr = (Var_seg.nvars + 1) / 2, countr;
        extern  ULONG   Last_v_ser;

        /* We want the sorted list, so we bodge the last read list
           and reread, sorting.
           It didn't seem worth doing this from initially and have all
           sorts of parameters in the setup_... routines */

        Last_v_ser = 0xffffffff;
        rvarlist(1);

        if  ((result = (char **) malloc((maxr + 1) * sizeof(char *))) == (char **) 0)
                ABORT_HTML_NOMEM;

        for  (vcnt = countr = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                BtvarRef        vp = &vv_ptrs[vcnt].vep->Vent;

                /* Skip ones which aren't allowed.  */

                if  (!mpermitted(&vp->var_mode, permflags, mypriv->btu_priv))
                        continue;

                if  (expr)  {
                        if  (!(vp->var_flags & VF_EXPORT)  &&  (vp->var_type != VT_MACHNAME || vp->var_id.hostid))
                                continue;
                }
                else  if  (vp->var_flags & VF_EXPORT)
                        continue;

                if  (countr >= maxr)  {
                        maxr += INCVLIST;
                        if  ((result = (char**) realloc((char *) result, (maxr + 1) * sizeof(char *))) == (char **) 0)
                                ABORT_HTML_NOMEM;
                }

                result[countr++] = stracpy(VAR_NAME(vp));
        }

        result[countr] = (char *) 0;
        return  result;
}

char **gen_jobasses(CBtjobRef jp)
{
        char    **result = malloc((MAXSEVARS+1)*sizeof(char *));
        int     cnt;

        if  (!result)
                ABORT_HTML_NOMEM;

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                CJassRef  ja = &jp->h.bj_asses[cnt];
                char    resbuf[HOSTNSIZE+BTV_NAME+BTC_VALUE+30];
                char    *rp = resbuf;
                CBtvarRef       vp;

                if  (ja->bja_op == BJA_NONE)
                        break;

                if  (ja->bja_iscrit & ACRIT_NORUN)  {
                        *rp++ = 'C';
                        *rp++ = '/';
                }
                if  (ja->bja_op < BJA_SEXIT)
#ifdef  CHARSPRINTF
                        rp += strlen(sprintf(rp, "%u", ja->bja_flags));
#else
                        rp += sprintf(rp, "%u", ja->bja_flags);
#endif
                vp = &Var_seg.vlist[ja->bja_varind].Vent;
                rp += strlen(strcpy(rp, VAR_NAME(vp)));
                if  (ja->bja_op >= BJA_SEXIT)  {
                        *rp++ = '=';
                        strcpy(rp, ja->bja_op == BJA_SEXIT? exitcodename: signalname);
                }
                else  {
                        rp += strlen(strcpy(rp, assname[ja->bja_op - BJA_ASSIGN]));
                        if  (ja->bja_con.const_type == CON_LONG)
                                sprintf(rp, "%ld", (long) ja->bja_con.con_un.con_long);
                        else
                                strcpy(rp, ja->bja_con.con_un.con_string);
                }
                result[cnt] = stracpy(resbuf);
        }
        result[cnt] = (char *) 0;
        return  result;
}

void  put_jslist(char **param)
{
        char  **pp, *p, *n;
        int     had = 0;

        putchar('[');
        for  (pp = param;  (p = *pp);  pp++)  {
                if  (had)
                        putchar(',');
                putchar('\"');
                for  (n = p;  *n;  n++)  {
                        if  (*n == '\"'  ||  *n == '\\')
                                putchar('\\');
                        putchar(*n);
                }
                putchar('\"');
                free(p);
                had++;
        }
        putchar(']');
        free(param);
}

int  perf_listtime(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        CTimeconRef             tp;
        time_t                  nexttime;
        unsigned  char          mday, repeat, nposs;
        USHORT                  nvaldays;
        ULONG                   rate;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        tp = &jp->h.bj_times;
        mday = 1;
        if  (tp->tc_istime)  {
                nexttime = tp->tc_nexttime;
                repeat = tp->tc_repeat;
                if  ((rate = tp->tc_rate) == TC_MONTHSB  ||  rate == TC_MONTHSE)  {
                        mday = tp->tc_mday;
                        if  (rate == TC_MONTHSE)
                                mday++;
                }
                nvaldays = tp->tc_nvaldays;
                nposs = tp->tc_nposs;
        }
        else  {
                int             cnt, defaultrate;
                HelpaltRef      a;

                nexttime = time((time_t *) 0) + 60L;
                repeat = TC_RETAIN;
                if  ((a = helprdalt($Q{btq repeat options})))  {
                        if  (a->def_alt >= 0)  {
                                int     defrep = a->alt_nums[a->def_alt];
                                freealts(a);
                                if  (defrep > TC_RETAIN)
                                        repeat = repunit->def_alt >= 0?
                                                TC_MINUTES + a->alt_nums[a->def_alt]: TC_HOURS;
                                else
                                        repeat = defrep;
                        }
                        else
                                freealts(a);
                }
                if  ((defaultrate = helpnstate($N{Default number of units})) <= 0)
                        defaultrate = 10;
                rate = defaultrate;
                nvaldays = 0;
                for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                        if  (helpnstate((int) ($N{Base for days to avoid}+cnt)) > 0)
                                nvaldays |= 1 << cnt;
                nposs = TC_WAIT1;
                if  ((a = helprdalt($Q{Wtime if not poss})))  {
                        if  (a->def_alt >= 0)
                                nposs = a->alt_nums[a->def_alt];
                        freealts(a);
                }
        }
        html_out_cparam_file("time_pre", 1, arg);
        printf("xbj_times2(\"%s\",%s,%ld,%u,%lu,%u,%u,%u);\n",
               arg, tof(tp->tc_istime), nexttime, repeat, (unsigned long) rate, mday, nvaldays, nposs);
        html_out_or_err("time_post", 0);
        exit(0);
        return  0;
}

int  perf_listconds(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        int                     cnt;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        html_out_cparam_file("conds_pre", 1, arg);
        printf("xbj_conds2(\"%s\",[", arg);
        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                CJcondRef  jc = &jp->h.bj_conds[cnt];
                CBtvarRef       vp;

                if  (jc->bjc_compar == C_UNUSED)
                        break;

                if  (cnt != 0)
                        putchar(',');

                vp = &Var_seg.vlist[jc->bjc_varind].Vent;

                printf("[%d,\"", jc->bjc_iscrit & CCRIT_NORUN? 1: 0);
                printf("%s\",%d,", VAR_NAME(vp), jc->bjc_compar);
                if  (jc->bjc_value.const_type == CON_LONG)
                        printf("%ld]", (long) jc->bjc_value.con_un.con_long);
                else
                        printf("\"%s\"]", jc->bjc_value.con_un.con_string);
        }
        fputs("],", stdout);
        put_jslist(gen_vlist(BTM_READ, jp->h.bj_jflags & (BJ_EXPORT|BJ_REMRUNNABLE)));
        fputs(");\n", stdout);
        html_out_or_err("conds_post", 0);
        exit(0);
        return  0;
}

int  perf_listasses(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        int                     cnt;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        html_out_cparam_file("asses_pre", 1, arg);
        printf("xbj_asses2(\"%s\",[", arg);

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                CJassRef  ja = &jp->h.bj_asses[cnt];
                CBtvarRef       vp;

                if  (ja->bja_op == BJA_NONE)
                        break;

                if  (cnt != 0)
                        putchar(',');

                vp = &Var_seg.vlist[ja->bja_varind].Vent;

                printf("[%d,%u,\"", ja->bja_iscrit & ACRIT_NORUN? 1: 0, ja->bja_flags);
                printf("%s\",%d,", VAR_NAME(vp), ja->bja_op);
                if  (ja->bja_op >= BJA_SEXIT)
                        fputs("0]", stdout);
                else  if  (ja->bja_con.const_type == CON_LONG)
                        printf("%ld]", (long) ja->bja_con.con_un.con_long);
                else
                        printf("\"%s\"]", ja->bja_con.con_un.con_string);
        }

        fputs("],", stdout);
        put_jslist(gen_vlist(BTM_READ|BTM_WRITE, jp->h.bj_jflags & (BJ_EXPORT|BJ_REMRUNNABLE)));
        fputs(");\n", stdout);
        html_out_or_err("asses_post", 0);
        exit(0);
        return  0;
}

int  perf_listtp(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        html_out_cparam_file("tpl_pre", 1, arg);
        printf("xbj_titprill2(\"%s\",\"%s\",[%d,%d,%d,%d],%d,\"%s\",%s,",
               arg, title_of(jp), jp->h.bj_pri, mypriv->btu_defp,
               mypriv->btu_minp, mypriv->btu_maxp,
               jp->h.bj_ll, jp->h.bj_cmdinterp, tof(mypriv->btu_priv & BTM_SPCREATE));
        put_jslist(gen_qlist());
        putchar(',');
        put_jslist(gen_cilist());
        fputs(");\n", stdout);
        html_out_or_err("tpl_post", 0);
        exit(0);
        return  0;
}

int  perf_listargs(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        char                    **args;
        unsigned                acnt;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        if  (!(args = (char **) malloc((jp->h.bj_nargs + 1) * sizeof(char *))))
                ABORT_HTML_NOMEM;
        for  (acnt = 0;  acnt < jp->h.bj_nargs;  acnt++)
                args[acnt] = stracpy(ARG_OF(jp, acnt));
        args[jp->h.bj_nargs] = (char *) 0;
        html_out_cparam_file("args_pre", 1, arg);
        printf("xbj_args2(\"%s\",", arg);
        put_jslist(args);
        fputs(");\n", stdout);
        html_out_or_err("args_post", 0);
        exit(0);
        return  0;
}

int  perf_listio(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        unsigned                rcnt;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        html_out_cparam_file("io_pre", 1, arg);
        printf("xbj_io2(\"%s\",[", arg);
        for  (rcnt = 0;  rcnt < jp->h.bj_nredirs;  rcnt++)  {
                CRedirRef rp = REDIR_OF(jp, rcnt);
                if  (rcnt != 0)
                        putchar(',');
                printf("[%d,%u,", rp->fd, rp->action);
                if  (rp->action >= RD_ACT_CLOSE)
                        printf("%d]", rp->action == RD_ACT_CLOSE? 0: rp->arg);
                else
                        printf("\'%s\']", &jp->bj_space[rp->arg]);
        }
        fputs("]);\n", stdout);
        html_out_or_err("io_post", 0);
        exit(0);
        return  0;
}

int  perf_listprocp(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        html_out_cparam_file("procp_pre", 1, arg);
        printf("xbj_procpar2(\"%s\", \"%s\",%d,%ld,[%d,%d,%d,%d],%s,%s,%s);\n",
               arg,
               jp->h.bj_direct < 0? "":  &jp->bj_space[jp->h.bj_direct],
               jp->h.bj_umask, (long) jp->h.bj_ulimit,
               jp->h.bj_exits.nlower, jp->h.bj_exits.nupper,
               jp->h.bj_exits.elower, jp->h.bj_exits.eupper,
               tof(jp->h.bj_jflags & BJ_NOADVIFERR),
               tof(jp->h.bj_jflags & BJ_EXPORT),
               tof(jp->h.bj_jflags & BJ_REMRUNNABLE));
        html_out_or_err("procp_post", 0);
        exit(0);
        return  0;
}

static int  do_create_func(const int fromfile)
{
        setup_main_files();
        html_out_or_err("create_pre", 1);
        fputs("xbj_create2(", stdout);
        put_jslist(gen_qlist());
        printf(",\"%s\",", homedof(Realuid));
        put_jslist(gen_cilist());
        printf(", %s);\n", fromfile? "true": "false");
        html_out_or_err(fromfile? "create_postfl": "create_post", 0);
        exit(0);
        return  0;
}

int  perf_listcreate(char *notused)
{
        do_create_func(0);
        return  0;
}

int  perf_listcreatef(char *notused)
{
        do_create_func(1);
        return  0;
}

int  perf_listperms(char *arg)
{
        struct  jobswanted      jw;
        CBtjobRef               jp;
        CBtmodeRef              mp;
        int                     modecurr, modenext = -1, modecnt, acnt, listorder[MAXMODE];

        setup_for_jobextract(arg, &jw);
        jp = jw.jp;
        mp = &jp->h.bj_mode;
        if  (!mpermitted(mp, BTM_RDMODE|BTM_WRMODE, mypriv->btu_priv))  {
                disp_str = arg;
                html_disperror($E{btjcgi no mode perm});
                exit(E_NOPRIV);
        }

        /* Protect against errors (loops) in data - acnt gives the number
           we've looked at, modenct the number excluding kill */
#ifdef  DONT_DEFINE             /* Force inclusion of them all */
$P{Read mode name}      $N{Read mode name}
$P{Write mode name}     $N{Write mode name}
$P{Reveal mode name}    $N{Reveal mode name}
$P{Display mode name}   $N{Display mode name}
$P{Set mode name}       $N{Set mode name}
$P{Assume owner mode name}      $N{Assume owner mode name}
$P{Assume group mode name}      $N{Assume group mode name}
$P{Give owner mode name}        $N{Give owner mode name}
$P{Give group mode name}        $N{Give group mode name}
$P{Delete mode name}    $N{Delete mode name}
$P{Kill mode name}      $N{Kill mode name}
#endif
        for  (acnt = modecnt = 0, modecurr = $N{Modes initial row};
              acnt < MAXMODE;
              modecurr = modenext, modecnt++, acnt++)  {
                modenext = helpnstate(modecurr);
                if  (modenext < 0)
                        break;
                listorder[modecnt] = modenext;
        }

        html_out_cparam_file("perm_pre", 1, arg);
        for  (acnt = 0;  acnt < modecnt;  acnt++)  {
                int     n = listorder[acnt];
                char    *msg = gprompt(n);
                USHORT  flg;
                n -= $N{Read mode name};
                flg = 1 << n;
                printf("btperm_checkboxes(\'%s\', \'%c\', %s, %s, %s);\n",
                       msg, "RWSMPUVGHDK"[n],
                       tof(mp->u_flags & flg),
                       tof(mp->g_flags & flg),
                       tof(mp->o_flags & flg));
                free(msg);
        }
        html_out_or_err("perm_post", 0);
        exit(0);
        return  0;
}

struct  arginterp  argtypes[] =  {
        {       "format",       AIF_ARG,        set_format      },
        {       "header",       AIF_ARG,        set_header      },
        {       "queue",        AIF_ARG,        set_queue       },
        {       "incnull",      AIF_ARG,        set_incnull     },
        {       "user",         AIF_ARG,        set_user        },
        {       "group",        AIF_ARG,        set_group       },
        {       "export",       AIF_ARG,        set_exported    },
        {       "sort",         AIF_ARG,        set_sort        },
        {       "create",       AIF_NOARG,      perf_listcreate },
        {       "createf",      AIF_NOARG,      perf_listcreatef },
        {       "listtime",     AIF_ARG,        perf_listtime   },
        {       "listconds",    AIF_ARG,        perf_listconds  },
        {       "listasses",    AIF_ARG,        perf_listasses  },
        {       "listtitprill", AIF_ARG,        perf_listtp     },
        {       "listprocpar",  AIF_ARG,        perf_listprocp  },
        {       "listargs",     AIF_ARG,        perf_listargs   },
        {       "listio",       AIF_ARG,        perf_listio     },
        {       "listperms",    AIF_ARG,        perf_listperms  },
        {       "listopts",     AIF_NOARG,      perf_optselect  },
        {       "listformat",   AIF_NOARG,      perf_listformat }
};

int  perf_optselect(char *notused)
{
        html_out_param_file("setopts", 1, 0, html_cookexpiry());
        exit(0);
        return  0;
}

void  interp_args(char **args)
{
        char    **ap, *arg, *cp = (char *) 0;
        int     cnt;

        for  (ap = args;  (arg = *ap);  ap++)  {
                if  ((cp = strchr(arg, ':')) || (cp = strchr(arg, '=')))
                        *cp++ = '\0';
                for  (cnt = 0;  cnt < sizeof(argtypes)/sizeof(struct arginterp);  cnt++)  {
                        if  (ncstrcmp(arg, argtypes[cnt].argname) == 0)  {
                                if  ((cp  &&  argtypes[cnt].flags == AIF_NOARG)  ||
                                     (!cp  &&  argtypes[cnt].flags == AIF_ARG)  ||
                                     !(argtypes[cnt].arg_fn)(cp))
                                        goto  badarg;
                                goto  ok;
                        }
                }
        badarg:
                html_out_or_err("badargs", 1);
                exit(E_USAGE);
        ok:
                ;
        }
}


/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char            **newargs;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        html_openini();
        newargs = cgi_arginterp(argc, argv, 0); /* Side effect of cgi_arginterp is to set Realuid */
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        tzset();
        SWAP_TO(Daemuid);
        prin_uname(Realuid);    /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;
        mypriv = getbtuser(Realuid);
        SWAP_TO(Realuid);
        hash_hostfile();
        interp_args(newargs);
        setup_main_files();

        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
                html_disperror($E{Missing alternative code});
                return  E_SETUP;
        }
        if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
                disp_arg[9] = $Q{Repeat unit abbrev};
                html_disperror($E{Missing alternative code});
                return  E_SETUP;
        }
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                html_disperror($E{Missing alternative code});
                return  E_SETUP;
        }
        if  (!(ifnposses = helprdalt($Q{Ifnposses})))  {
                disp_arg[9] = $Q{Ifnposses};
                print_error($E{Missing alternative code});
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        localrun = gprompt($P{Locally run});
        if  (!hadhdrarg  &&  html_inibool("headers", 0))
                Dispflags |= DF_HAS_HDR;
        fmt_setup();
        html_output_file("list_preamble", 1);
        jdisplay();
        html_output_file("list_postamble", 0);
        return  0;
}
