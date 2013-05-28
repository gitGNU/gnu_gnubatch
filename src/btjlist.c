/* btjlist.c -- shell-level job list

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
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "helpalt.h"
#include "netmsg.h"
#include "formats.h"
#include "optflags.h"
#include "cgifndjb.h"
#include "shutilmsg.h"

static  char    Filename[] = __FILE__;

#define BTJLIST_INLINE

#define IPC_MODE        0600

#ifndef USING_FLOCK
int     Sem_chan;
#endif

HelpaltRef      progresslist;

char            *localrun;

unsigned  char  exportflag = BJ_REMRUNNABLE;

char            Viewj = 0,
                sortflag,
                fulltimeflag,
                bypassflag;

extern const char *const condname[];
extern const char *const assname[];

char            *formatstring;

struct  jobswanted  *wanted_list;
unsigned        nwanted;

char    sdefaultfmt[] = "%N %U %H %I %p %L %t %c %P",
        fdefaultfmt[] = "%N %U %H %I %p %L %T %c %P";

unsigned        jno_width;

char    bigbuff[JOBSPACE];

FILE *net_feed(const int, const netid_t, const jobno_t, const int);

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static void  getwanted(char **argv)
{
        char    **ap;
        struct  jobswanted  *wp;
        unsigned        actw = 0;

        for  (ap = argv;  *ap;  ap++)
                nwanted++;

        /* There will be at least one, see call */

        if  (!(wanted_list = (struct jobswanted *) malloc(nwanted * sizeof(struct jobswanted))))
                ABORT_NOMEM;

        wp = wanted_list;

        for  (ap = argv;  *ap;  ap++)  {
                int     retc = decode_jnum(*ap, wp);
                if  (retc != 0)
                        print_error(retc);
                else  {
                        actw++;
                        wp++;
                }
        }

        if  (actw == 0)  {
                print_error($E{No valid args to process});
                exit(E_USAGE);
        }
        nwanted = actw;
}

static int  iswanted(CBtjobRef jp)
{
        unsigned        cnt;

        for  (cnt = 0;  cnt < nwanted;  cnt++)
                if  (wanted_list[cnt].jno == jp->h.bj_job  &&  wanted_list[cnt].host == jp->h.bj_hostid)
                        return  1;
        return  0;
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

/* Display contents of job file.  */

void  jdisplay()
{
        int     jcnt;
        BtjobRef        jp;
        char    *fp;
        unsigned  pieces, pc, *lengths = (unsigned *) 0, wanted_jobs = 0;
        int     lng;
        BtjobRef  copied_wanted, wp;

        pieces = 0;
        fp = formatstring;
        while  (*fp)  {
                if  (*fp == '%')  {
                        if  (!*++fp)
                                break;
                        if  ((isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)  ||
                             (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn))
                                pieces++;
                }
                fp++;
        }
        if  (pieces &&  !(lengths = (unsigned *) malloc(pieces * sizeof(unsigned))))
                ABORT_NOMEM;
        for  (pc = 0;  pc < pieces;  pc++)
                lengths[pc] = 0;

        /* Initial scan to get job number width */

        jno_width = 0;

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                char    nbuf[12];
                jp = jj_ptrs[jcnt];
                if  (jp->h.bj_job == 0)
                        break;
                if  (jp->h.bj_hostid  &&  (jp->h.bj_jflags & exportflag) == 0)
                        continue;
                if  (nwanted != 0  &&  !iswanted(jp))
                        continue;
#ifdef  CHARSPRINTF
                sprintf(nbuf, "%ld", (long) jp->h.bj_job);
                lng = strlen(nbuf);
#else
                lng = sprintf(nbuf, "%ld", (long) jp->h.bj_job);
#endif
                if  (lng > jno_width)
                        jno_width = lng;

                wanted_jobs++;
        }

        if  (wanted_jobs == 0)  {
                junlock();
                return;
        }

        if  (!(copied_wanted = (BtjobRef) malloc(wanted_jobs * sizeof(Btjob))))
                ABORT_NOMEM;

        wp = copied_wanted;

        /* Second scan to get width of each format
           copy details to butter whilst we are at it */

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                int     isreadable;
                jp = jj_ptrs[jcnt];
                if  (jp->h.bj_job == 0)
                        break;
                if  (jp->h.bj_hostid  &&  (jp->h.bj_jflags & exportflag) == 0)
                        continue;
                if  (nwanted != 0  &&  !iswanted(jp))
                        continue;
                *wp++ = *jp;
                isreadable = mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv);
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)
                                        lng = (uppertab[*fp - 'A'].fmt_fn)(jp, isreadable, 0);
                                else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                        lng = (lowertab[*fp - 'a'].fmt_fn)(jp, isreadable, 0);
                                else  {
                                        fp++;
                                        continue;
                                }
                                if  (lng > lengths[pc])
                                        lengths[pc] = lng;
                                pc++;
                        }
                        fp++;
                }
        }

        /* Now unlock job segment */

        junlock();

        /* Possibly expand columns for header */

        if  (Dispflags & DF_HAS_HDR)  {
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)  {
                                        if  (!uppertab[*fp - 'A'].msg)
                                                uppertab[*fp - 'A'].msg = gprompt(uppertab[*fp - 'A'].statecode);
                                        lng = strlen(uppertab[*fp - 'A'].msg);
                                }
                                else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)  {
                                        if  (!lowertab[*fp - 'a'].msg)
                                                lowertab[*fp - 'a'].msg = gprompt(lowertab[*fp - 'a'].statecode);
                                        lng = strlen(lowertab[*fp - 'a'].msg);
                                }
                                else  {
                                        fp++;
                                        continue;
                                }
                                if  (lng > lengths[pc])
                                        lengths[pc] = lng;
                                pc++;
                        }
                        fp++;
                }

                /* And now output it...  */

                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)  {
                                        fputs(uppertab[*fp - 'A'].msg, stdout);
                                        lng = strlen(uppertab[*fp - 'A'].msg);
                                }
                                else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)  {
                                        fputs(lowertab[*fp - 'a'].msg, stdout);
                                        lng = strlen(lowertab[*fp - 'a'].msg);
                                }
                                else
                                        goto  putit1;
                                if  (pc != pieces - 1)
                                        while  (lng < lengths[pc])  {
                                                putchar(' ');
                                                lng++;
                                        }
                                do  fp++;
                                while  (lengths[pc] == 0  &&  *fp == ' ');
                                pc++;
                                continue;
                        }
                putit1:
                        putchar(*fp);
                        fp++;
                }
                putchar('\n');
        }

        /* Final run-through to output stuff */

        for  (jcnt = 0;  jcnt < (int) wanted_jobs;  jcnt++)  {
                int     isreadable;
                jp = &copied_wanted[jcnt];
                isreadable = mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv);
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                bigbuff[0] = '\0'; /* Zap last thing */
                                if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)
                                        lng = (uppertab[*fp - 'A'].fmt_fn)(jp, isreadable, (int) lengths[pc]);
                                else  if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                        lng = (lowertab[*fp - 'a'].fmt_fn)(jp, isreadable, (int) lengths[pc]);
                                else
                                        goto  putit;
                                fputs(bigbuff, stdout);
                                if  (pc != pieces - 1)
                                        while  (lng < lengths[pc])  {
                                                putchar(' ');
                                                lng++;
                                        }
                                do  fp++;
                                while  (lengths[pc] == 0  &&  *fp == ' ');
                                pc++;
                                continue;
                        }
                putit:
                        putchar(*fp);
                        fp++;
                }
                putchar('\n');
        }
}

static void  viewj(CBtjobRef jp)
{
        FILE            *ifl;
        static  char    *spdir;

        if  (jp->h.bj_hostid)
                ifl = net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job, Job_seg.dptr->js_viewport);
        else  {
                if  (!spdir)
                        spdir = envprocess(SPDIR);
                sprintf(bigbuff, "%s/%s", spdir, mkspid(SPNAM, jp->h.bj_job));
                ifl = fopen(bigbuff, "r");
        }
        if  (!ifl)  {
                disp_arg[0] = jp->h.bj_job;
                if  (jp->h.bj_hostid)  {
                        disp_str = look_host(jp->h.bj_hostid);
                        print_error($E{btjlist cannot view remote job});
                }
                else
                        print_error($E{btjlist cannot view local job});
        }
        else  {
                int     ch;
                while  ((ch = getc(ifl)) != EOF)
                        putchar(ch);
                fclose(ifl);
        }
}

static void  doview()
{
        int     jcnt;
        CBtjobRef       jp;

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                jp = jj_ptrs[jcnt];
                if  (jp->h.bj_job == 0)
                        break;
                if  (jp->h.bj_hostid  &&  (jp->h.bj_jflags & exportflag) == 0)
                        continue;
                if  (nwanted != 0  &&  !iswanted(jp))
                        continue;
                if  (!mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {
                        if  (nwanted == 0)
                                continue;
                        disp_arg[0] = jp->h.bj_job;
                        if  (jp->h.bj_hostid)  {
                                disp_str = look_host(jp->h.bj_hostid);
                                print_error($E{btjlist no perm view remote});
                        }
                        else
                                print_error($E{btjlist no perm view local});
                        continue;
                }
                viewj(jp);
        }
}

OPTION(o_explain)
{
        print_error($E{btjlist explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_local)
{
        exportflag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_remotes)
{
        exportflag = BJ_REMRUNNABLE;
        return  OPTRESULT_OK;
}

OPTION(o_nolocal)
{
        exportflag = BJ_EXPORT|BJ_REMRUNNABLE;
        return  OPTRESULT_OK;
}

OPTION(o_nosort)
{
        sortflag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_sort)
{
        sortflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_shorttimes)
{
        fulltimeflag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_fulltimes)
{
        fulltimeflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_formatstr)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (formatstring)
                free(formatstring);
        formatstring = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_formatdflt)
{
        if  (formatstring)  {
                free(formatstring);
                formatstring = (char *) 0;
        }
        return  OPTRESULT_OK;
}

OPTION(o_viewjob)
{
        Viewj = 1;
        return  OPTRESULT_OK;
}

OPTION(o_noviewjob)
{
        Viewj = 0;
        return  OPTRESULT_OK;
}

OPTION(o_bypass)
{
        if  (mypriv->btu_priv & BTM_WADMIN)
                bypassflag = 1;
        return  OPTRESULT_OK;
}

DEOPTION(o_jobqueue);
DEOPTION(o_justu);
DEOPTION(o_justg);
DEOPTION(o_incnull);
DEOPTION(o_noincnull);
DEOPTION(o_header);
DEOPTION(o_noheader);
DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btjlist arg explain} },
  {  'L', $A{btjlist arg loco} },
  {  'r', $A{btjlist arg execrems} },
  {  'R', $A{btjlist arg allrems} },
  {  'q', $A{btjlist arg queue} },
  {  'n', $A{btjlist arg nosort} },
  {  's', $A{btjlist arg sort} },
  {  'S', $A{btjlist arg stimes} },
  {  'T', $A{btjlist arg fulltimes} },
  {  'F', $A{btjlist arg format} },
  {  'D', $A{btjlist arg deffmt} },
  {  'H', $A{btjlist arg hdr} },
  {  'N', $A{btjlist arg nohdr} },
  {  'u', $A{btjlist arg justu} },
  {  'g', $A{btjlist arg justg} },
  {  'z', $A{btjlist arg nullq} },
  {  'Z', $A{btjlist arg nonullq} },
  {  'B', $A{btjlist arg bypassm} },
  {  'V', $A{btjlist arg view} },
  {  'l', $A{btjlist arg no view} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_local,        o_remotes,      o_nolocal,
o_jobqueue,     o_nosort,       o_sort,         o_shorttimes,
o_fulltimes,    o_formatstr,    o_formatdflt,   o_header,
o_noheader,     o_justu,        o_justg,        o_incnull,
o_noincnull,    o_bypass,       o_viewjob,      o_noviewjob,
o_freezecd,     o_freezehd
};

static void  quoteput(FILE *dest, char *str)
{
        if  (str)
                fprintf(dest, " \'%s\'", str);
        else
                fputs(" -", dest);
}

void  spit_options(FILE *dest, const char *name)
{
        int     cancont;

        fprintf(dest, "%s", name);
        cancont = spitoption(exportflag & BJ_EXPORT? $A{btjlist arg allrems}:
                             exportflag & BJ_REMRUNNABLE? $A{btjlist arg execrems}:
                             $A{btjlist arg loco}, $A{btjlist arg explain}, dest, '=', 0);
        cancont = spitoption(sortflag ? $A{btjlist arg sort}: $A{btjlist arg nosort}, $A{btjlist arg explain}, dest, ' ', cancont);
        cancont = spitoption(fulltimeflag ? $A{btjlist arg fulltimes}: $A{btjlist arg stimes}, $A{btjlist arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_HAS_HDR? $A{btjlist arg hdr}: $A{btjlist arg nohdr}, $A{btjlist arg explain}, dest, ' ', cancont);
        cancont = spitoption(Dispflags & DF_SUPPNULL? $A{btjlist arg nonullq}: $A{btjlist arg nullq}, $A{btjlist arg explain}, dest, ' ', cancont);
        cancont = spitoption(Viewj? $A{btjlist arg view}: $A{btjlist arg no view}, $A{btjlist arg explain}, dest, ' ', cancont);
        if  (formatstring)  {
                spitoption($A{btjlist arg format}, $A{btjlist arg explain}, dest, ' ', 0);
                fprintf(dest, " \"%s\"", formatstring);
        }
        else
                spitoption($A{btjlist arg deffmt}, $A{btjlist arg explain}, dest, ' ', cancont);

        spitoption($A{btjlist arg queue}, $A{btjlist arg explain}, dest, ' ', 0);
        quoteput(dest, jobqueue);
        spitoption($A{btjlist arg justu}, $A{btjlist arg explain}, dest, ' ', 0);
        quoteput(dest, Restru);
        spitoption($A{btjlist arg justg}, $A{btjlist arg explain}, dest, ' ', 0);
        quoteput(dest, Restrg);
        putc('\n', dest);
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

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *Curr_pwd = (char *) 0;
        int     ret;
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
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        tzset();
        SWAP_TO(Daemuid);
        mypriv = getbtuser(Realuid);
        SWAP_TO(Realuid);
        hash_hostfile();
        argv = optprocess(argv, Adefs, optprocs, $A{btjlist arg explain}, $A{btjlist arg freeze home}, 0);

#include "inline/freezecode.c"

        if  (argv[0])
                getwanted(argv);

        if  (Anychanges & OF_ANY_FREEZE_WANTED)
                exit(0);

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

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

        /* Open the other files. No read yet until the scheduler is
           aware of our existence, which it won't be until we
           send it a message.  */

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
        openjfile(0, 0);
        rjobfile(0);            /* NB Leave it locked!!! */
        if  (sortflag)
                qsort(QSORTP1 jj_ptrs, Job_seg.njobs, sizeof(BtjobRef), QSORTP4 sort_j);

        openvfile(0, 0);
        rvarfile(0);

        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
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
        localrun = gprompt($P{Locally run});
        if  (!formatstring)
                formatstring = fulltimeflag? fdefaultfmt: sdefaultfmt;
        if  (Viewj)
                doview();
        else
                jdisplay();
        return  0;
}
