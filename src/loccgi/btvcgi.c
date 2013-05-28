/* btvcgi.c -- Variable CGI routine

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "btconst.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "cfile.h"
#include "files.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "formats.h"
#include "optflags.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "listperms.h"

static  char    Filename[] = __FILE__;

#define MAXMODE 16              /* Max possible in USHORT */

char    hadhdrarg;
char    *formatstring;
char    defaultformat[] = "LN LV LE LC";

#define IPC_MODE        0600

extern  int     Ctrl_chan;

char    bigbuff[BTC_VALUE * 2];

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  fmt_setup()
{
        if  (!formatstring)
                formatstring = defaultformat;
}

#define BTVLIST_INLINE
typedef unsigned        fmt_t;

#include "inline/vfmt_comment.c"
#include "inline/vfmt_group.c"
#include "inline/vfmt_export.c"
#include "inline/fmtmode.c"
#include "inline/vfmt_mode.c"
#include "inline/vfmt_name.c"
#include "inline/vfmt_user.c"
#include "inline/vfmt_value.c"

#define NULLCP  (char *) 0

struct  formatdef  {
        SHORT   statecode;      /* Code number for heading if applicable */
        char    alch;
        char    *msg;           /* Heading */
        unsigned  (*fmt_fn)(CBtvarRef, const int, const int);
};

struct  formatdef
        uppertab[] = { /* A-Z */
        {       0,                      'L',    NULLCP, 0       },      /* A */
        {       0,                      'L',    NULLCP, 0       },      /* B */
        {       $P{var fmt title}+'C',  'L',    NULLCP, fmt_comment     },      /* C */
        {       0,                      'L',    NULLCP, 0       },      /* D */
        {       $P{var fmt title}+'E',  'L',    NULLCP, fmt_export      },      /* E */
        {       0,                      'L',    NULLCP, 0       },      /* F */
        {       $P{var fmt title}+'G',  'L',    NULLCP, fmt_group       },      /* G */
        {       0,                      'L',    NULLCP, 0       },      /* H */
        {       0,                      'L',    NULLCP, 0       },      /* I */
        {       0,                      'L',    NULLCP, 0       },      /* J */
        {       $P{var fmt title}+'K',  'L',    NULLCP, fmt_cluster     },      /* K */
        {       0,                      'L',    NULLCP, 0       },      /* L */
        {       $P{var fmt title}+'M',  'L',    NULLCP, fmt_mode        },      /* M */
        {       $P{var fmt title}+'N',  'L',    NULLCP, fmt_name        },      /* N */
        {       0,                      'L',    NULLCP, 0       },      /* O */
        {       0,                      'L',    NULLCP, 0       },      /* P */
        {       0,                      'L',    NULLCP, 0       },      /* Q */
        {       0,                      'L',    NULLCP, 0       },      /* R */
        {       0,                      'L',    NULLCP, 0       },      /* S */
        {       0,                      'L',    NULLCP, 0       },      /* T */
        {       $P{var fmt title}+'U',  'L',    NULLCP, fmt_user        },      /* U */
        {       $P{var fmt title}+'V',  'L',    NULLCP, fmt_value       },      /* V */
        {       0,                      'L',    NULLCP, 0       },      /* W */
        {       0,                      'L',    NULLCP, 0       },      /* X */
        {       0,                      'L',    NULLCP, 0       },      /* Y */
        {       0,                      'L',    NULLCP, 0       }       /* Z */
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
        {       "left", 'L' },
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
                if  (!isupper(fmch))
                        break;
                fd = &uppertab[fmch - 'A'];
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

/* Display contents of var list */

void  vdisplay()
{
        int     fcnt, vcnt;
        unsigned  pflgs = 0;

        /* <TABLE> included in pre-list file so as to possibly include formatting elements */

        find_commonest(formatstring);

        /* Possibly insert header */

        if  (Dispflags & DF_HAS_HDR)  {
                /* Possibly insert first header showing options */
                if  (Restru || Restrg || (Dispflags & DF_LOCALONLY))  {
                        startrow();
                        printf("<th colspan=%d align=center>", ncolfmts+1);
                        fputs(gprompt($P{btjcgi restr start}), stdout);
                        html_fldprint(gprompt($P{btjcgi restr view}));
                        if  (Restru)  {
                                html_fldprint(gprompt($P{btjcgi restr users}));
                                html_fldprint(Restru);
                        }
                        if  (Restrg)  {
                                html_fldprint(gprompt($P{btjcgi restr groups}));
                                html_fldprint(Restrg);
                        }
                        if  (Dispflags & DF_LOCALONLY)
                                html_fldprint(gprompt($P{btjcgi restr loco}));
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

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                CBtvarRef       vp = &vv_ptrs[vcnt].vep->Vent;
                int             isreadable = 0;
                unsigned        hval = 0;

                if  (vp->var_value.const_type == CON_NONE)
                        continue;
                if  (vp->var_id.hostid  &&  Dispflags & DF_LOCALONLY)
                        continue;
                if  (mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv))  {
                        isreadable = 1;
                        hval |= LV_MINEORVIEW|LV_LOCORRVIEW;
                }
                if  (mpermitted(&vp->var_mode, BTM_WRITE, mypriv->btu_priv))
                        hval |= LV_CHANGEABLE;
                if  (mpermitted(&vp->var_mode, BTM_DELETE, mypriv->btu_priv)  &&  !vp->var_id.hostid)
                        hval |= LV_DELETEABLE;
                if  (mpermitted(&vp->var_mode, BTM_WRMODE, mypriv->btu_priv))
                        hval |= LV_CHMODABLE;

                startrow();
                startcell('d', commonest_align->str);
                fmt_name(vp, isreadable, 0);
                printf("<input type=checkbox name=vars value=\"%s,%u\"></td>", bigbuff, hval);

                for  (fcnt = 0;  fcnt < ncolfmts;  fcnt++)  {
                        startcell('d', cflist[fcnt].alstr);
                        bigbuff[0] = '\0';
                        (cflist[fcnt].form->fmt_fn)(vp, isreadable, 0);
                        html_fldprint(bigbuff);
                        fputs("</td>", stdout);
                }
                fputs("</tr>\n", stdout);
        }

        pflgs |= GLV_ANYCHANGES | GLV_ACCESSF | GLV_FREEZEF;
        printf("</table>\n<input type=hidden name=privs value=%u>\n", pflgs);
}

struct  arginterp  {
        char    *argname;
        unsigned  short  flags;
#define AIF_NOARG       0
#define AIF_ARG         1
        int     (*arg_fn)(char *);
};

static char *tof(const USHORT x)
{
        return  x? "true": "false";
}

int  perf_listperms(char *oarg)
{
        char            *arg = oarg;
        ULONG           Saveseq;
        vhash_t         vp;
        netid_t         hostid = 0;
        BtmodeRef       mp;
        int             modecurr, modenext = -1, modecnt, acnt, listorder[MAXMODE];
        char            *cp;

        /* Open var shm seg because this is done after arg interp */

        setuid(Daemuid);
#ifndef USING_FLOCK
        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                html_disperror($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif
        openvfile(0, 0);
        rvarlist(1);

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
                if  (modenext == $N{Kill mode name})
                        modecnt--;
        }

        /* Decode job name / host name.
           Protest if operation can't be done remotely. */

        if  ((cp = strchr(arg, ':')))  {
                *cp = '\0';
                if  ((hostid = look_int_hostname(arg)) == -1)  {
                        disp_str = arg;
                        html_disperror($E{Btvar unknown host name});
                        exit(E_USAGE);
                }
                *cp++ = ':';
                arg = cp;
        }

        vp = lookupvar(arg, hostid, BTM_READ, &Saveseq);
        if  (vp < 0)  {
                disp_str = oarg;
                html_disperror($E{Btvar notfnd mode set});
                exit(E_VNOTFND);
        }
        mp = &Var_seg.vlist[vp].Vent.var_mode;
        html_out_cparam_file("perm_pre", 1, oarg);
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

int  perf_listformat(char *notused)
{
        struct  formatdef   *fp;
        int     lett;
        char    *msg;

        html_out_or_err("listfmt_pre", 1);
        for  (fp = &uppertab[0],  lett = 'A';  fp < &uppertab[26];  lett++, fp++)
                if  (fp->statecode != 0)  {
                        msg = gprompt(fp->statecode + 200);
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

int  set_loco(char *arg)
{
        switch  (tolower(*arg))  {
        case  'l':case 'y':case '1':
                Dispflags |= DF_LOCALONLY;
                return  1;
        case  'n':case '0':
                Dispflags &= ~DF_LOCALONLY;
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

struct  arginterp  argtypes[] =  {
        {       "format",       AIF_ARG,        set_format      },
        {       "header",       AIF_ARG,        set_header      },
        {       "localonly",    AIF_ARG,        set_loco        },
        {       "user",         AIF_ARG,        set_user        },
        {       "group",        AIF_ARG,        set_group       },
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
        char    **newargs;
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
        SWAP_TO(Daemuid);
        prin_uname(Realuid);    /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;
        mypriv = getbtuser(Realuid);
        SWAP_TO(Realuid);
        hash_hostfile();
        interp_args(newargs);

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                html_disperror($E{Scheduler not running});
                return  E_NOTRUN;
        }

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                html_disperror($E{Cannot open semaphore});
                return  E_SETUP;
        }
#endif

        /* Open the other files. No read yet until the scheduler is
           aware of our existence, which it won't be until we
           send it a message.  */

        openvfile(0, 0);
        if  (!hadhdrarg  &&  html_inibool("headers", 0))
                Dispflags |= DF_HAS_HDR;
        fmt_setup();
        html_output_file("list_preamble", 1);
        rvarlist(0);
        vdisplay();
        vunlock();
        html_output_file("list_postamble", 0);
        return  0;
}
