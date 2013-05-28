/* btvlist.c -- shell-level variable list

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
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "formats.h"
#include "optflags.h"
#include "shutilmsg.h"

static  char    Filename[] = __FILE__;

char    bypassflag;

char    *formatstring;
char    defaultformat[] = "%N %V %E # %C";

#define IPC_MODE        0600

struct  varswanted      {
        netid_t         host;
        char            varname[BTV_NAME+1];
}  *wanted_list;

unsigned        nwanted;

char    bigbuff[BTC_VALUE * 2];

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static void  getwanted(char **argv)
{
        char    **ap;
        struct  varswanted  *wp;
        unsigned  actw = 0;

        for  (ap = argv;  *ap;  ap++)
                nwanted++;

        /* There will be at least one, see call */

        if  (!(wanted_list = (struct varswanted *) malloc(nwanted * sizeof(struct varswanted))))
                ABORT_NOMEM;

        wp = wanted_list;

        for  (ap = argv;  *ap;  ap++)  {
                char    *arg = *ap, *cp;
                if  ((cp = strchr(arg, ':')))  {
                        *cp = '\0';
                        if  ((wp->host = look_int_hostname(arg)) == -1)  {
                                disp_str = arg;
                                print_error($E{Unknown host name});
                                *cp = ':';
                                continue;
                        }
                        *cp = ':';
                        strncpy(wp->varname, cp+1, BTV_NAME);
                        wp->varname[BTV_NAME] = '\0';
                }
                else  {
                        wp->host = 0L;
                        strncpy(wp->varname, arg, BTV_NAME);
                }
                wp->varname[BTV_NAME] = '\0';
                actw++;
                wp++;
        }

        if  (actw == 0)  {
                print_error($E{No valid args to process});
                exit(E_USAGE);
        }
        nwanted = actw;
}

static int  iswanted(CBtvarRef vp)
{
        unsigned        cnt;

        for  (cnt = 0;  cnt < nwanted;  cnt++)
                if  (wanted_list[cnt].host == vp->var_id.hostid  &&
                     strcmp(wanted_list[cnt].varname, vp->var_name) == 0)
                        return  1;
        return  0;
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
        char    *msg;           /* Heading */
        unsigned (*fmt_fn)(CBtvarRef, const int, const int);
};

struct  formatdef
        uppertab[] = { /* A-Z */
        {       0,                      NULLCP, 0               },      /* A */
        {       0,                      NULLCP, 0               },      /* B */
        {       $P{var fmt title}+'C',  NULLCP, fmt_comment     },      /* C */
        {       0,                      NULLCP, 0               },      /* D */
        {       $P{var fmt title}+'E',  NULLCP, fmt_export      },      /* E */
        {       0,                      NULLCP, 0               },      /* F */
        {       $P{var fmt title}+'G',  NULLCP, fmt_group       },      /* G */
        {       0,                      NULLCP, 0               },      /* H */
        {       0,                      NULLCP, 0               },      /* I */
        {       0,                      NULLCP, 0               },      /* J */
        {       $P{var fmt title}+'K',  NULLCP, fmt_cluster     },      /* K */
        {       0,                      NULLCP, 0               },      /* L */
        {       $P{var fmt title}+'M',  NULLCP, fmt_mode        },      /* M */
        {       $P{var fmt title}+'N',  NULLCP, fmt_name        },      /* N */
        {       0,                      NULLCP, 0               },      /* O */
        {       0,                      NULLCP, 0               },      /* P */
        {       0,                      NULLCP, 0               },      /* Q */
        {       0,                      NULLCP, 0               },      /* R */
        {       0,                      NULLCP, 0               },      /* S */
        {       0,                      NULLCP, 0               },      /* T */
        {       $P{var fmt title}+'U',  NULLCP, fmt_user        },      /* U */
        {       $P{var fmt title}+'V',  NULLCP, fmt_value       },      /* V */
        {       0,                      NULLCP, 0               },      /* W */
        {       0,                      NULLCP, 0               },      /* X */
        {       0,                      NULLCP, 0               },      /* Y */
        {       0,                      NULLCP, 0               }       /* Z */
};

/* Display contents of var list */

void  vdisplay()
{
        int     vcnt;
        BtvarRef        vp;
        char    *fp;
        unsigned  pieces, pc, *lengths = (unsigned *) 0, number_wanted = 0;
        int     lng;
        BtvarRef  copied_wanted, wp;

        pieces = 0;
        fp = formatstring;
        while  (*fp)  {
                if  (*fp == '%')  {
                        if  (!*++fp)
                                break;
                        if  ((isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn))
                                pieces++;
                }
                fp++;
        }
        if  (pieces &&  !(lengths = (unsigned *) malloc(pieces * sizeof(unsigned))))
                ABORT_NOMEM;
        for  (pc = 0;  pc < pieces;  pc++)
                lengths[pc] = 0;

        if  (Var_seg.nvars == 0)  {
                vunlock();
                return;
        }
        if  (!(copied_wanted = (BtvarRef) malloc(Var_seg.nvars * sizeof(Btjob))))
                ABORT_NOMEM;

        wp = copied_wanted;

        /* Initial scan to get width of each format */

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                int     isreadable;
                vp = &vv_ptrs[vcnt].vep->Vent;
                if  (vp->var_value.const_type == CON_NONE)
                        continue;
                if  (vp->var_id.hostid  &&  Dispflags & DF_LOCALONLY)
                        continue;
                if  (nwanted != 0  &&  !iswanted(vp))
                        continue;
                isreadable = mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv);
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)
                                        lng = (uppertab[*fp - 'A'].fmt_fn)(vp, isreadable, 0);
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
                *wp++ = *vp;;
                number_wanted++;
        }

        vunlock();
        if  (number_wanted == 0)
                return;

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
                                if  (!(isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn))
                                        goto  putit1;
                                fputs(uppertab[*fp - 'A'].msg, stdout);
                                lng = strlen(uppertab[*fp - 'A'].msg);
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

        for  (vcnt = 0;  vcnt < number_wanted;  vcnt++)  {
                int     isreadable;
                vp = &copied_wanted[vcnt];
                isreadable = mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv);
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                bigbuff[0] = '\0'; /* Zap last thing */
                                if  (isupper(*fp)  &&  uppertab[*fp - 'A'].fmt_fn)
                                        lng = (uppertab[*fp - 'A'].fmt_fn)(vp, isreadable, (int) lengths[pc]);
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

OPTION(o_explain)
{
        print_error($E{btvlist explain});
        exit(0);
        return  0;              /* Silence compilers */
}

DEOPTION(o_localonly);
DEOPTION(o_nolocalonly);

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

OPTION(o_bypass)
{
        if  (mypriv->btu_priv & BTM_WADMIN)
                bypassflag = 1;
        return  OPTRESULT_OK;
}

DEOPTION(o_justu);
DEOPTION(o_justg);
DEOPTION(o_header);
DEOPTION(o_noheader);
DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btvlist arg explain} },
  {  'L', $A{btvlist arg loco} },
  {  'R', $A{btvlist arg inclrems} },
  {  'F', $A{btvlist arg format} },
  {  'D', $A{btvlist arg deffmt} },
  {  'H', $A{btvlist arg hdr} },
  {  'N', $A{btvlist arg nohdr} },
  {  'u', $A{btvlist arg justu} },
  {  'g', $A{btvlist arg justg} },
  {  'B', $A{btvlist arg bypassm} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,
o_localonly,    o_nolocalonly,  o_formatstr,    o_formatdflt,
o_header,       o_noheader,     o_justu,        o_justg,
o_bypass,       o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont;

        fprintf(dest, "%s", name);
        cancont = spitoption(Dispflags & DF_LOCALONLY? $A{btvlist arg loco}: $A{btvlist arg inclrems}, $A{btvlist arg explain}, dest, '=', 0);
        cancont = spitoption(Dispflags & DF_HAS_HDR? $A{btvlist arg hdr}: $A{btvlist arg nohdr}, $A{btvlist arg explain}, dest, ' ', cancont);
        if  (formatstring)  {
                spitoption($A{btvlist arg format}, $A{btvlist arg explain}, dest, ' ', 0);
                fprintf(dest, " \"%s\"", formatstring);
        }
        else
                spitoption($A{btvlist arg deffmt}, $A{btvlist arg explain}, dest, ' ', cancont);
        spitoption($A{btvlist arg justu}, $A{btvlist arg explain}, dest, ' ', 0);
        if  (Restru)
                fprintf(dest, " \'%s\'", Restru);
        else
                fputs(" -", dest);
        spitoption($A{btvlist arg justg}, $A{btvlist arg explain}, dest, ' ', 0);
        if  (Restrg)
                fprintf(dest, " \'%s\'", Restrg);
        else
                fputs(" -", dest);
        putc('\n', dest);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *Curr_pwd = (char *) 0;
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
        SWAP_TO(Daemuid);
        mypriv = getbtuser(Realuid);
        SWAP_TO(Realuid);
        hash_hostfile();
        argv = optprocess(argv, Adefs, optprocs, $A{btvlist arg explain}, $A{btvlist arg freeze home}, 0);

#include "inline/freezecode.c"

        if  (argv[0])
                getwanted(argv);

        if  (Anychanges & OF_ANY_FREEZE_WANTED)
                return  0;

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                return  E_NOTRUN;
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

        openvfile(0, 0);
        if  (!formatstring)
                formatstring = defaultformat;
        rvarlist(0);
        vdisplay();
        return  0;
}
