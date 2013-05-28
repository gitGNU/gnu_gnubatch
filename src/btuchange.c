/* btuchange.c -- main program for gbch-uchange

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
#include <sys/types.h>
#include <ctype.h>
#include "defaults.h"
#include "files.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpargs.h"
#include "incl_unix.h"
#include "cfile.h"
#include "optflags.h"

char    *Curr_pwd;

BtuserRef       ulist;

static  char    set_default,
                copyall,
                priv_setting,
                abs_priv,
                jmode_setting,
                vmode_setting,
                JVmode_set[6];

static  unsigned  char  min_p,
                        max_p,
                        def_p;

static  USHORT          maxll,
                        totll,
                        specll;

static  USHORT          jvmodes[6];     /* Job modes are []s 3 4 5 */

static  ULONG   set_flags = 0,
                reset_flags = ALLPRIVS;

struct  perm    {
        int     number;
        char    *string;
        USHORT  flg, sflg, rflg; /* Shorts for now, but we have room for longs in field */
}  ptab[] = {
        { $P{Read adm abbr},            (char *) 0, BTM_RADMIN, BTM_RADMIN, (USHORT) ~(BTM_RADMIN|BTM_WADMIN) },
        { $P{Write adm abbr},           (char *) 0, BTM_WADMIN, ALLPRIVS, (USHORT) ~BTM_WADMIN },
        { $P{Create entry abbr},        (char *) 0, BTM_CREATE, BTM_CREATE, (USHORT) ~(BTM_CREATE|BTM_SPCREATE)},
        { $P{Special create abbr},      (char *) 0, BTM_SPCREATE, BTM_CREATE|BTM_SPCREATE, (USHORT) ~BTM_SPCREATE},
        { $P{Stop sched abbr},  (char *) 0, BTM_SSTOP, BTM_SSTOP, (USHORT) ~BTM_SSTOP},
        { $P{Change default modes abbr},(char *) 0, BTM_UMASK, BTM_UMASK, (USHORT) ~(BTM_UMASK|BTM_WADMIN)},
        { $P{Combine user group abbr},  (char *) 0, BTM_ORP_UG, BTM_ORP_UG, (USHORT) ~BTM_ORP_UG},
        { $P{Combine user other abbr},  (char *) 0, BTM_ORP_UO, BTM_ORP_UO, (USHORT) ~BTM_ORP_UO},
        { $P{Combine group other abbr}, (char *) 0, BTM_ORP_GO, BTM_ORP_GO, (USHORT) ~BTM_ORP_GO}};

static  char    *allname;

#define MAXPERM (sizeof (ptab)/sizeof(struct perm))

extern void  dumpemode(FILE *, const char, const char, const int, unsigned);

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

/* Expand privilege codes into messages */

static  void  expcodes()
{
        int     i;

        allname = gprompt($P{Btulist all name});
        for  (i = 0;  i < MAXPERM;  i++)
                ptab[i].string = gprompt(ptab[i].number);
}

OPTION(o_explain)
{
        print_error($E{btuchange explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_defaults)
{
        set_default = 1;
        return  OPTRESULT_OK;
}

OPTION(o_nodefaults)
{
        set_default = 0;
        return  OPTRESULT_OK;
}

OPTION(o_copyall)
{
        copyall = 1;
        return  OPTRESULT_OK;
}

OPTION(o_nocopyall)
{
        copyall = 0;
        return  OPTRESULT_OK;
}

OPTION(o_rebuild)
{
        return  OPTRESULT_OK;
}

OPTION(o_norebuild)
{
        return  OPTRESULT_OK;
}

OPTION(o_minpri)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv pri};
                return  OPTRESULT_ERROR;
        }
        num = atoi(arg);
        if  (num <= 0  || num > 255)  {
                arg_errnum = $E{Btuchange inv pri range};
                return  OPTRESULT_ERROR;
        }
        min_p = (unsigned char) num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_maxpri)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv pri};
                return  OPTRESULT_ERROR;
        }
        num = atoi(arg);
        if  (num <= 0  || num > 255)  {
                arg_errnum = $E{Btuchange inv pri range};
                return  OPTRESULT_ERROR;
        }
        max_p = (unsigned char) num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_defpri)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv pri};
                return  OPTRESULT_ERROR;
        }
        num = atoi(arg);
        if  (num <= 0  || num > 255)  {
                arg_errnum = $E{Btuchange inv pri range};
                return  OPTRESULT_ERROR;
        }
        def_p = (unsigned char) num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_maxll)
{
        unsigned  num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv ll};
                return  OPTRESULT_ERROR;
        }
        num = (unsigned) atoi(arg);
        if  (num == 0  ||  num > 65535)  {
                arg_errnum = $E{Btuchange inv ll range};
                return  OPTRESULT_ERROR;
        }
        maxll = (USHORT) num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_totll)
{
        unsigned  num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv ll};
                return  OPTRESULT_ERROR;
        }
        num = (unsigned) atoi(arg);
        if  (num == 0  ||  num > 65535)  {
                arg_errnum = $E{Btuchange inv ll range};
                return  OPTRESULT_ERROR;
        }
        totll = (USHORT) num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_specll)
{
        unsigned  num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv ll};
                return  OPTRESULT_ERROR;
        }
        num = (unsigned) atoi(arg);
        if  (num == 0  ||  num > 65535)  {
                arg_errnum = $E{Btuchange inv ll range};
                return  OPTRESULT_ERROR;
        }
        specll = (USHORT) num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_priv)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        /* Start from scratch as we don't want to be confusebd */

        set_flags = 0;
        reset_flags = ALLPRIVS;

        /* Accept flags as hex for benefit of conversion from old
           versions in btuconv */

        if  (arg[0] == '0' && toupper(arg[1]) == 'X')  {
                arg += 2;
                while  (*arg)
                        if  (isdigit(*arg))
                                set_flags = (set_flags << 4) + *arg++ - '0';
                        else  if  (isupper(*arg))
                                set_flags = (set_flags << 4) + *arg++ - 'A' + 10;
                        else  if  (islower(*arg))
                                set_flags = (set_flags << 4) + *arg++ - 'a' + 10;
                        else  {
                                arg_errnum = $E{Btuchange inv priv};
                                return  OPTRESULT_ERROR;
                        }
                priv_setting++;
                abs_priv++;
                return  OPTRESULT_ARG_OK;
        }

        while  (*arg)  {
                int  isminus = 0, ac = 0, pc;
                char    abuf[20];
                if  (*arg == '-')  {
                        isminus++;
                        arg++;
                }
                do  if  (ac < sizeof(abuf)-1)
                        abuf[ac++] = *arg++;
                while  (*arg && *arg != ',');
                abuf[ac] = '\0';
                for  (pc = 0;  pc < MAXPERM;  pc++)
                        if  (ncstrcmp(abuf, ptab[pc].string) == 0)
                                goto  gotit;
                if  (ncstrcmp(abuf, allname) == 0  &&  !isminus)  {
                        set_flags = ALLPRIVS;
                        goto  nxtcomm;
                }
                disp_str = abuf;
                arg_errnum = $E{Btuchange inv priv};
                return  OPTRESULT_ERROR;
        gotit:
                if  (isminus)
                        reset_flags &= ptab[pc].rflg;
                else
                        set_flags |= ptab[pc].sflg;
        nxtcomm:
                priv_setting++;
                abs_priv = 0;
                if  (*arg == ',')
                        arg++;
        }
        return  OPTRESULT_ARG_OK;
}

static void  btu_chksetmode(const int modenum, USHORT *flgs)
{
        USHORT  mflags = jvmodes[modenum];

        switch  (JVmode_set[modenum])  {
        case  MODE_NONE:
                return;
        case  MODE_SET:
                *flgs = mflags;
                return;
        case  MODE_ON:
                *flgs |= mflags;
                return;
        case  MODE_OFF:
                *flgs &= ~mflags;
                return;
        }
}

static int  slurp_mode(const char *arg, const unsigned flagoff)
{
        int             isuser, isgroup, isoth, ch;
        int             mode_type = MODE_SET;
        unsigned        wmode;
        const   char    *startit;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        if  (arg[0] == '0' &&  toupper(arg[1]) == 'X')  {
                int     cnt;
                arg += 2;
                for  (cnt = 0;  *arg  &&  cnt < 3;  cnt++)  {
                        unsigned  set_flags = 0;
                        while  (*arg  &&  *arg != ',')  {
                                if  (isdigit(*arg))
                                        set_flags = (set_flags << 4) + *arg++ - '0';
                                else  if  (isupper(*arg))
                                        set_flags = (set_flags << 4) + *arg++ - 'A' + 10;
                                else  if  (islower(*arg))
                                        set_flags = (set_flags << 4) + *arg++ - 'a' + 10;
                                else
                                        arg++;
                        }
                        if  (*arg)
                                arg++;
                        JVmode_set[cnt + flagoff] = MODE_SET;
                        jvmodes[cnt + flagoff] = set_flags;
                }

                return  OPTRESULT_ARG_OK;
        }

        do  {
                isuser = isgroup = isoth = 0;
                mode_type = MODE_SET;
                wmode = 0;

                startit = arg;

                while  ((ch = toupper(*arg)) == 'U'  ||  ch == 'G'  ||  ch == 'O')  {
                        if  (ch == 'U')
                                isuser = 1;
                        else  if  (ch == 'G')
                                isgroup = 1;
                        else
                                isoth = 1;
                        arg++;
                }
                if  (ch != ':')  {
                        isuser = isgroup = isoth = 1;
                        arg = startit;
                }
                else
                        arg++;
                if  ((ch = *arg) == '+')  {
                        mode_type = MODE_ON;
                        arg++;
                }
                else  if  (ch == '-')  {
                        mode_type = MODE_OFF;
                        arg++;
                }
                else  if  (ch == '=')
                        arg++;

                while  (isalpha(ch = *arg))  {
                        arg++;
                        switch  (ch)  {
                        default:        goto  badmode;
                        case  'R':
                                if  (mode_type == MODE_OFF)
                                        wmode |= BTM_READ|BTM_WRITE;
                                else
                                        wmode |= BTM_SHOW|BTM_READ;
                                break;

                        case  'W':
                                if  (mode_type == MODE_OFF)
                                        wmode |= BTM_WRITE;
                                else
                                        wmode |= BTM_SHOW|BTM_READ|BTM_WRITE;
                                break;

                        case  'S':
                                if  (mode_type == MODE_OFF)
                                        wmode |= BTM_SHOW|BTM_READ|BTM_WRITE;
                                else
                                        wmode |= BTM_SHOW;
                                break;

                        case  'M':
                                if  (mode_type == MODE_OFF)
                                        wmode |= BTM_RDMODE|BTM_WRMODE;
                                else
                                        wmode |= BTM_RDMODE;
                                break;

                        case  'P':
                                if  (mode_type == MODE_OFF)
                                        wmode |= BTM_WRMODE;
                                else
                                        wmode |= BTM_RDMODE|BTM_WRMODE;
                                break;

                        case  'U':      wmode |= BTM_UGIVE;     break;
                        case  'V':      wmode |= BTM_UTAKE;     break;
                        case  'G':      wmode |= BTM_GGIVE;     break;
                        case  'H':      wmode |= BTM_GTAKE;     break;
                        case  'D':      wmode |= BTM_DELETE;    break;
                        case  'K':      if  (flagoff >= 3)  wmode |= BTM_KILL;  break;
                        }
                }

                if  (ch != '\0'  &&  ch != ',')
                        goto  badmode;

                if  (isuser)  {
                        JVmode_set[flagoff + 0] = (char) mode_type;
                        jvmodes[flagoff + 0] = (USHORT) wmode;
                }
                if  (isgroup)  {
                        JVmode_set[flagoff + 1] = (char) mode_type;
                        jvmodes[flagoff + 1] = (USHORT) wmode;
                }
                if  (isoth)  {
                        JVmode_set[flagoff + 2] = (char) mode_type;
                        jvmodes[flagoff + 2] = (USHORT) wmode;
                }

        }  while  (*arg++ == ',');

        return  OPTRESULT_ARG_OK;

 badmode:
        arg_errnum = $E{Bad mode string};
        return  OPTRESULT_ERROR;
}

OPTION(o_jmode)
{
        jmode_setting++;
        return  slurp_mode(arg, 3);
}

OPTION(o_vmode)
{
        vmode_setting++;
        return  slurp_mode(arg, 0);
}

OPTION(o_dumppw)
{
        return  OPTRESULT_OK;
}

OPTION(o_aswaspw)
{
        return  OPTRESULT_OK;
}

OPTION(o_killpw)
{
        return  OPTRESULT_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

static  const   Argdefault      Adefs[] = {
  {  '?', $A{btuchange arg explain} },
  {  'D', $A{btuchange arg setdefs} },
  {  'u', $A{btuchange arg setusers} },
  {  'A', $A{btuchange arg copydefs} },
  {  's', $A{btuchange arg nocopydefs} },
  {  'l', $A{btuchange arg minpri} },
  {  'd', $A{btuchange arg defpri} },
  {  'm', $A{btuchange arg maxpri} },
  {  'M', $A{btuchange arg maxll} },
  {  'T', $A{btuchange arg totll} },
  {  'S', $A{btuchange arg specll} },
  {  'p', $A{btuchange arg privs} },
  {  'J', $A{btuchange arg jobmode} },
  {  'V', $A{btuchange arg varmode} },
  {  'R', $A{btuchange arg rebuild} },
  {  'N', $A{btuchange arg norebuild} },
  {  'X', $A{btuchange dump pw}  },
  {  'Y', $A{btuchange aswas pw} },
  {  'Z', $A{btuchange kill pw}  },
  {  0, 0 }
};

optparam  optprocs[] = {
o_explain,      o_defaults,     o_nodefaults,   o_copyall,
o_nocopyall,    o_minpri,       o_defpri,       o_maxpri,
o_maxll,        o_totll,        o_specll,       o_priv,
o_jmode,        o_vmode,        o_rebuild,      o_norebuild,
o_dumppw,       o_aswaspw,      o_killpw,
o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0;
        fprintf(dest, "%s", name);

        cancont = spitoption(set_default? $A{btuchange arg setdefs}: $A{btuchange arg setusers}, $A{btuchange arg explain}, dest, '=', cancont);
        cancont = spitoption(copyall? $A{btuchange arg copydefs}: $A{btuchange arg nocopydefs}, $A{btuchange arg explain}, dest, ' ', cancont);
        if  (min_p != 0)  {
                spitoption($A{btuchange arg minpri}, $A{btuchange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", min_p);
        }
        if  (def_p != 0)  {
                spitoption($A{btuchange arg defpri}, $A{btuchange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", def_p);
        }
        if  (max_p != 0)  {
                spitoption($A{btuchange arg maxpri}, $A{btuchange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", max_p);
        }
        if  (maxll != 0)  {
                spitoption($A{btuchange arg maxll}, $A{btuchange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", maxll);
        }
        if  (totll != 0)  {
                spitoption($A{btuchange arg totll}, $A{btuchange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", totll);
        }
        if  (specll != 0)  {
                spitoption($A{btuchange arg specll}, $A{btuchange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", specll);
        }
        if  (priv_setting)  {
                spitoption($A{btuchange arg privs}, $A{btuchange arg explain}, dest, ' ', 0);

                if  (abs_priv)
                        fprintf(dest, "0x%lx", (unsigned long) set_flags);
                else  {
                        int     ch = ' ', cnt;
                        unsigned  cflags = set_flags;

                        for  (cnt = 0;  cnt < MAXPERM;  cnt++)
                                if  (cflags & ptab[cnt].flg)  {
                                        fprintf(dest, "%c%s", ch, ptab[cnt].string);
                                        cflags &= ~ ptab[cnt].sflg;
                                        ch = ',';
                                }
                        cflags = reset_flags;
                        for  (cnt = 0;  cnt < MAXPERM;  cnt++)
                                if  (!(cflags & ptab[cnt].flg))  {
                                        fprintf(dest, "%c-%s", ch, ptab[cnt].string);
                                        cflags |= ~ ptab[cnt].rflg;
                                        ch = ',';
                                }
                }
        }
        if  (jmode_setting)  {
                char    sep = ' ';
                spitoption($A{btuchange arg jobmode}, $A{btuchange arg explain}, dest, ' ', 0);
                if  (JVmode_set[3] != MODE_NONE)  {
                        dumpemode(dest, sep, 'U', JVmode_set[3], jvmodes[3]);
                        sep = ',';
                }
                if  (JVmode_set[4] != MODE_NONE)  {
                        dumpemode(dest, sep, 'G', JVmode_set[4], jvmodes[4]);
                        sep = ',';
                }
                if  (JVmode_set[5] != MODE_NONE)
                        dumpemode(dest, sep, 'O', JVmode_set[5], jvmodes[5]);
        }
        if  (vmode_setting)  {
                char    sep = ' ';
                spitoption($A{btuchange arg varmode}, $A{btuchange arg explain}, dest, ' ', 0);
                if  (JVmode_set[0] != MODE_NONE)  {
                        dumpemode(dest, sep, 'U', JVmode_set[0], jvmodes[0]);
                        sep = ',';
                }
                if  (JVmode_set[1] != MODE_NONE)  {
                        dumpemode(dest, sep, 'G', JVmode_set[1], jvmodes[1]);
                        sep = ',';
                }
                if  (JVmode_set[2] != MODE_NONE)
                        dumpemode(dest, sep, 'O', JVmode_set[2], jvmodes[2]);
        }
        putc('\n', dest);
}

BtuserRef  find_user(const char *uname)
{
        int_ugid_t      uid;
        int     first, last, middle;

        if  ((uid = lookup_uname(uname)) == UNKNOWN_UID)
                return  (BtuserRef) 0;

        first = 0;
        last = Npwusers;

        while  (first < last)  {
                middle = (first + last) / 2;
                if  (ulist[middle].btu_user == uid)
                        return  &ulist[middle];
                if  ((ULONG) ulist[middle].btu_user < (ULONG) uid)
                        first = middle + 1;
                else
                        last = middle;
        }
        return  (BtuserRef) 0;
}

static void  u_update(BtuserRef up, const int metoo)
{
        if  (min_p != 0)
                up->btu_minp = min_p;
        if  (max_p != 0)
                up->btu_maxp = max_p;
        if  (def_p != 0)
                up->btu_defp = def_p;
        if  (maxll != 0)
                up->btu_maxll = maxll;
        if  (totll != 0)
                up->btu_totll = totll;
        if  (specll != 0)
                up->btu_spec_ll = specll;
        if  (priv_setting  &&  (metoo || up->btu_user != Realuid))  {
                if  (abs_priv)
                        up->btu_priv = set_flags;
                else  {
                        up->btu_priv |= set_flags;
                        up->btu_priv &= reset_flags;
                }
        }
        if  (jmode_setting)  {
                btu_chksetmode(3, &up->btu_jflags[0]);
                btu_chksetmode(4, &up->btu_jflags[1]);
                btu_chksetmode(5, &up->btu_jflags[2]);
        }
        if  (vmode_setting)  {
                btu_chksetmode(0, &up->btu_vflags[0]);
                btu_chksetmode(1, &up->btu_vflags[1]);
                btu_chksetmode(2, &up->btu_vflags[2]);
        }
}

static void  copy_all()
{
        BtuserRef  up, ue = &ulist[Npwusers];
        for  (up = ulist;  up < ue;  up++)  {
                up->btu_minp = Btuhdr.btd_minp;
                up->btu_maxp = Btuhdr.btd_maxp;
                up->btu_defp = Btuhdr.btd_defp;
                up->btu_maxll = Btuhdr.btd_maxll;
                up->btu_totll = Btuhdr.btd_totll;
                up->btu_spec_ll = Btuhdr.btd_spec_ll;
                if  (up->btu_user != Realuid)
                        up->btu_priv = Btuhdr.btd_priv;
                up->btu_jflags[0] = Btuhdr.btd_jflags[0];
                up->btu_jflags[1] = Btuhdr.btd_jflags[1];
                up->btu_jflags[2] = Btuhdr.btd_jflags[2];
                up->btu_vflags[0] = Btuhdr.btd_vflags[0];
                up->btu_vflags[1] = Btuhdr.btd_vflags[1];
                up->btu_vflags[2] = Btuhdr.btd_vflags[2];
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int             nerrors = 0;
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
        expcodes();
        SCRAMBLID_CHECK
        argv = optprocess(argv, Adefs, optprocs, $A{btuchange arg explain}, $A{btuchange arg freeze home}, 0);
        SWAP_TO(Daemuid);

#define FREEZE_EXIT
#include "inline/freezecode.c"

        mypriv = getbtuentry(Realuid);

        if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                print_error($E{No write admin file priv});
                return  E_NOPRIV;
        }
        ulist = getbtulist();

        if  (set_default)  {
                if  (*argv != (char *) 0)  {
                        print_error($E{Btuchange unexp arg});
                        return  E_USAGE;
                }
                if  (min_p != 0)
                        Btuhdr.btd_minp = min_p;
                if  (max_p != 0)
                        Btuhdr.btd_maxp = max_p;
                if  (def_p != 0)
                        Btuhdr.btd_defp = def_p;
                if  (maxll != 0)
                        Btuhdr.btd_maxll = maxll;
                if  (totll != 0)
                        Btuhdr.btd_totll = totll;
                if  (specll != 0)
                        Btuhdr.btd_spec_ll = specll;
                if  (priv_setting)  {
                        if  (abs_priv)
                                Btuhdr.btd_priv = set_flags;
                        else  {
                                Btuhdr.btd_priv |= set_flags;
                                Btuhdr.btd_priv &= reset_flags;
                        }
                }
                if  (jmode_setting)  {
                        btu_chksetmode(3, &Btuhdr.btd_jflags[0]);
                        btu_chksetmode(4, &Btuhdr.btd_jflags[1]);
                        btu_chksetmode(5, &Btuhdr.btd_jflags[2]);
                }
                if  (vmode_setting)  {
                        btu_chksetmode(0, &Btuhdr.btd_vflags[0]);
                        btu_chksetmode(1, &Btuhdr.btd_vflags[1]);
                        btu_chksetmode(2, &Btuhdr.btd_vflags[2]);
                }
                if  (copyall)
                        copy_all();
                putbtulist(ulist);
        }
        else  {
                if  (copyall)
                        copy_all();
                if  (!*argv)  {
                        BtuserRef  up, ue = &ulist[Npwusers];
                        for  (up = ulist;  up < ue;  up++)
                                u_update(up, 0);
                }
                else  {
                        char    **ap;
                        for  (ap = argv;  *ap;  ap++)  {
                                BtuserRef  up = find_user(*ap);
                                if  (!up)  {
                                        disp_str = *ap;
                                        print_error($E{Btuchange unexp user});
                                        nerrors++;
                                }
                                else
                                        u_update(up, 1);
                        }
                }
                putbtulist(ulist);
        }
        return  nerrors > 0? E_FALSE: E_TRUE;
}
