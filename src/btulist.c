/* btulist.c -- main program for gbch-ulist

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

#define SRT_NONE        0       /* Sort by numeric uid (default) */
#define SRT_USER        1       /* Sort by user name */
#define SRT_GROUP       2       /* Sort by group name */

static  char    alphsort = SRT_NONE,
                defline = 1,
                ulines = 1;
static  char    *defaultname,
                *allname;

struct  perm    {
        char    *string;
        USHORT  flg;                     /* Shorts for now, but we have room for longs in field */
}  ptab[NUM_PRIVBITS] = {
        { (char *) 0, BTM_RADMIN        },
        { (char *) 0, BTM_WADMIN        },
        { (char *) 0, BTM_CREATE        },
        { (char *) 0, BTM_SPCREATE      },
        { (char *) 0, BTM_SSTOP         },
        { (char *) 0, BTM_UMASK         },
        { (char *) 0, BTM_ORP_UG        },
        { (char *) 0, BTM_ORP_UO        },
        { (char *) 0, BTM_ORP_GO        }};

char    *formatstring;
char    sdefaultfmt[] = "%u %g %d %l %m %x %t %s %p";
char    bigbuff[250];

static  char    Filename[] = __FILE__;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

typedef unsigned        fmt_t;

#ifdef  CHARSPRINTF
#define FMT_FUNC(name, def_field, user_field)\
static fmt_t  name(CBtuserRef up, const int fwidth)\
{\
        sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->user_field: (unsigned) Btuhdr.def_field);\
        return  (fmt_t) strlen(bigbuff);\
}
#else
#define FMT_FUNC(name, def_field, user_field)\
static fmt_t  name(CBtuserRef up, const int fwidth)\
{\
        return  (fmt_t) sprintf(bigbuff, "%*u", fwidth, up? (unsigned) up->user_field: (unsigned) Btuhdr.def_field);\
}
#endif

FMT_FUNC(fmt_defpri, btd_defp, btu_defp)
FMT_FUNC(fmt_minpri, btd_minp, btu_minp)
FMT_FUNC(fmt_maxpri, btd_maxp, btu_maxp)
FMT_FUNC(fmt_maxll, btd_maxll, btu_maxll)
FMT_FUNC(fmt_totll, btd_totll, btu_totll)
FMT_FUNC(fmt_specll, btd_spec_ll, btu_spec_ll)

static fmt_t  fmt_priv(CBtuserRef up, const int fwidth)
{
        ULONG   priv = up? up->btu_priv: Btuhdr.btd_priv;

        if  ((priv & ALLPRIVS) == ALLPRIVS)
                return  (fmt_t) strlen(strcpy(bigbuff, allname));
        else  {
                char    *nxt = bigbuff;
                unsigned  pn, totl = 0, lng;
                for  (pn = 0;  pn < NUM_PRIVBITS;  pn++)
                        if  (priv & ptab[pn].flg)  {
                                lng = strlen(strcpy(nxt, ptab[pn].string));
                                nxt += lng;
                                *nxt++ = ' ';
                                totl += lng + 1;
                        }
                if  (totl == 0)
                        return  0;
                *--nxt = '\0';
                return  totl - 1;
        }
}

#include "inline/fmtmode.c"

static fmt_t  fmt_jmode(CBtuserRef up, const int fwidth)
{
        fmt_t  lng = 0;
        if  (up)  {
                lng = fmtmode(lng, "U", up->btu_jflags[0]);
                lng = fmtmode(lng, ",G", up->btu_jflags[1]);
                lng = fmtmode(lng, ",O", up->btu_jflags[2]);
        }
        else  {
                lng = fmtmode(lng, "U", Btuhdr.btd_jflags[0]);
                lng = fmtmode(lng, ",G", Btuhdr.btd_jflags[1]);
                lng = fmtmode(lng, ",O", Btuhdr.btd_jflags[2]);
        }
        return  lng;
}

static fmt_t  fmt_vmode(CBtuserRef up, const int fwidth)
{
        fmt_t  lng = 0;
        if  (up)  {
                lng = fmtmode(lng, "U", up->btu_vflags[0]);
                lng = fmtmode(lng, ",G", up->btu_vflags[1]);
                lng = fmtmode(lng, ",O", up->btu_vflags[2]);
        }
        else  {
                lng = fmtmode(lng, "U", Btuhdr.btd_vflags[0]);
                lng = fmtmode(lng, ",G", Btuhdr.btd_vflags[1]);
                lng = fmtmode(lng, ",O", Btuhdr.btd_vflags[2]);
        }
        return  lng;
}

/* Display of users and groups - we do a "prin_uname" for each row to
   start with and the result is pointed to by "got_user" (actually it
   is static in prin_uname).  If we are displaying the default row,
   then DEFAULT appears as the user name, unless we aren't displaying
   users, whereupon it goes in the group name field.  We flag the
   latter case by def_in_grp.  */

static  char    *got_user, def_in_grp = 0;

static fmt_t  fmt_user(CBtuserRef up, const int fwidth)
{
        return  (fmt_t) strlen(strcpy(bigbuff, up? got_user : defaultname));
}

static fmt_t  fmt_group(CBtuserRef up, const int fwidth)
{
        return  up? (fmt_t) strlen(strcpy(bigbuff, prin_gname(lastgid))) :
                def_in_grp? (fmt_t) strlen(strcpy(bigbuff, defaultname)): 0;
}

static  fmt_t  fmt_uid(CBtuserRef up, const int fwidth)
{
#ifdef  CHARSPRINTF
        if  (up)
                sprintf(bigbuff, "%*u", fwidth, (unsigned) up->btu_user);
        else
                sprintf(bigbuff, "%*s", fwidth, "-");
        return  (fmt_t) strlen(bigbuff);
#else
        if  (up)
                return  (fmt_t) sprintf(bigbuff, "%*u", fwidth, (unsigned) up->btu_user);
        else
                return  (fmt_t)  sprintf(bigbuff, "%*s", fwidth, "-");
#endif
}

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct  formatdef  {
        SHORT   statecode;      /* Code number for heading if applicable */
        char    *msg;           /* Heading */
        unsigned  (*fmt_fn)(CBtuserRef, const int);
};

#define NULLCP  (char *) 0

struct  formatdef
        lowertab[] = { /* a-z */
        {       0,                              NULLCP, 0               },      /* a */
        {       0,                              NULLCP, 0               },      /* b */
        {       0,                              NULLCP, 0               },      /* c */
        {       $P{Btulist title}+'d'-1,        NULLCP, fmt_defpri      },      /* d */
        {       0,                              NULLCP, 0               },      /* e */
        {       0,                              NULLCP, 0               },      /* f */
        {       $P{Btulist title}+'g'-1,        NULLCP, fmt_group       },      /* g */
        {       0,                              NULLCP, 0               },      /* h */
        {       $P{Btulist title}+'i'-1,        NULLCP, fmt_uid         },      /* i */
        {       $P{Btulist title}+'j'-1,        NULLCP, fmt_jmode       },      /* j */
        {       0,                              NULLCP, 0               },      /* k */
        {       $P{Btulist title}+'l'-1,        NULLCP, fmt_minpri      },      /* l */
        {       $P{Btulist title}+'m'-1,        NULLCP, fmt_maxpri      },      /* m */
        {       0,                              NULLCP, 0               },      /* n */
        {       0,                              NULLCP, 0               },      /* o */
        {       $P{Btulist title}+'p'-1,        NULLCP, fmt_priv        },      /* p */
        {       0,                              NULLCP, 0               },      /* q */
        {       0,                              NULLCP, 0               },      /* r */
        {       $P{Btulist title}+'s'-1,        NULLCP, fmt_specll      },      /* s */
        {       $P{Btulist title}+'t'-1,        NULLCP, fmt_totll       },      /* t */
        {       $P{Btulist title}+'u'-1,        NULLCP, fmt_user        },      /* u */
        {       $P{Btulist title}+'v'-1,        NULLCP, fmt_vmode       },      /* v */
        {       0,                              NULLCP, 0               },      /* w */
        {       $P{Btulist title}+'x'-1,        NULLCP, fmt_maxll       },      /* x */
        {       0,                              NULLCP, 0               },      /* y */
        {       0,                              NULLCP, 0               }       /* z */
};

/* Display contents of user list.  */

void  udisplay(CBtuserRef ul, const unsigned nu)
{
        CBtuserRef      up, ep = &ul[nu];
        char    *fp;
        unsigned  pieces, pc, *lengths = (unsigned *) 0;
        int     lng, wantug = 0;

        pieces = 0;
        fp = formatstring;
        while  (*fp)  {
                if  (*fp == '%')  {
                        if  (!*++fp)
                                break;
                        if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                pieces++;
                }
                fp++;
        }
        if  (pieces  &&  !(lengths = (unsigned *) malloc(pieces * sizeof(unsigned))))
                ABORT_NOMEM;
        for  (pc = 0;  pc < pieces;  pc++)
                lengths[pc] = 0;

        /* First scan to see if we need to get user, we still need to
           do a prin_uname to get the group field if we don't want
           users and we put DEFAULT as the group name in that case.
           Set def_in_grp if we are not displaying the user name.  */

        fp = formatstring;
        while  (*fp)  {
                if  (*fp++ == '%')  {
                        if  (*fp == 'u')  {
                                def_in_grp = 0;
                                wantug++;
                                break;          /* All we need to know */
                        }
                        if  (*fp == 'g')  {
                                def_in_grp = 1; /* For now */
                                wantug++;
                                fp++;
                        }
                        else if  (*fp)          /* Careful in case we have % at end or %%g */
                                fp++;
                }
        }

        /* Second scan to get width of each format */

        if  (defline)  {
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                        lng = (lowertab[*fp - 'a'].fmt_fn)((CBtuserRef) 0, 0);
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
        if  (ulines)  {
                for  (up = ul;  up < ep;  up++)  {
                        if  (wantug)
                                got_user = prin_uname((uid_t) up->btu_user); /* NB sets lastgid */
                        fp = formatstring;
                        pc = 0;
                        while  (*fp)  {
                                if  (*fp == '%')  {
                                        if  (!*++fp)
                                                break;
                                        if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                                lng = (lowertab[*fp - 'a'].fmt_fn)(up, 0);
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
        }

        /* Possibly expand columns for header */

        if  (Dispflags & DF_HAS_HDR)  {
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)  {
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
                                if  (!(islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn))
                                        goto  putit1;
                                fputs(lowertab[*fp - 'a'].msg, stdout);
                                lng = strlen(lowertab[*fp - 'a'].msg);
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

        if  (defline)  {
                fp = formatstring;
                pc = 0;
                while  (*fp)  {
                        if  (*fp == '%')  {
                                if  (!*++fp)
                                        break;
                                bigbuff[0] = '\0'; /* Zap last thing */
                                if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                        lng = (lowertab[*fp - 'a'].fmt_fn)((CBtuserRef) 0, (int) lengths[pc]);
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

        if  (ulines)  {
                for  (up = ul;  up < ep;  up++)  {
                        if  (wantug)
                                got_user = prin_uname((uid_t) up->btu_user); /* NB sets lastgid */
                        fp = formatstring;
                        pc = 0;
                        while  (*fp)  {
                                if  (*fp == '%')  {
                                        if  (!*++fp)
                                                break;
                                        bigbuff[0] = '\0'; /* Zap last thing */
                                        if  (islower(*fp)  &&  lowertab[*fp - 'a'].fmt_fn)
                                                lng = (lowertab[*fp - 'a'].fmt_fn)(up, (int) lengths[pc]);
                                        else
                                                goto  putit2;
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
                        putit2:
                                putchar(*fp);
                                fp++;
                        }
                        putchar('\n');
                }
        }
}

OPTION(o_explain)
{
        print_error($E{btulist explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_usort)
{
        alphsort = SRT_USER;
        return  OPTRESULT_OK;
}

OPTION(o_gsort)
{
        alphsort = SRT_GROUP;
        return  OPTRESULT_OK;
}

OPTION(o_nsort)
{
        alphsort = SRT_NONE;
        return  OPTRESULT_OK;
}

OPTION(o_defline)
{
        defline = 1;
        return  0;
}

OPTION(o_nodefline)
{
        defline = 0;
        return  0;
}

OPTION(o_ulines)
{
        ulines = 1;
        return  0;
}

OPTION(o_noulines)
{
        ulines = 0;
        return  0;
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

DEOPTION(o_header);
DEOPTION(o_noheader);
DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

static  const   Argdefault  Adefs[] = {
        {  '?', $A{btulist arg explain} },
        {  'u', $A{btulist arg sort user} },
        {  'g', $A{btulist arg sort group} },
        {  'n', $A{btulist arg sort uid} },
        {  'd', $A{btulist default line}},
        {  's', $A{btulist no default line}},
        {  'U', $A{btulist user lines}},
        {  'S', $A{btulist no user lines}},
        {  'F', $A{btulist format}      },
        {  'D', $A{btulist default format}},
        {  'H', $A{btulist header}      },
        {  'N', $A{btulist no header}   },
        { 0, 0 }
};

optparam  optprocs[] = {
o_explain,      o_usort,        o_gsort,        o_nsort,
o_defline,      o_nodefline,    o_ulines,       o_noulines,
o_formatstr,    o_formatdflt,   o_header,       o_noheader,
o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0;
        fprintf(dest, "%s", name);
        cancont = spitoption(alphsort == SRT_USER? $A{btulist arg sort user}:
                             alphsort == SRT_GROUP? $A{btulist arg sort group}:
                             $A{btulist arg sort uid}, $A{btulist arg explain}, dest, '=', cancont);
        cancont = spitoption(Dispflags & DF_HAS_HDR? $A{btulist header}:
                             $A{btulist no header},
                             $A{btulist arg explain}, dest, ' ', cancont);
        cancont = spitoption(defline? $A{btulist default line}:
                             $A{btulist no default line},
                             $A{btulist arg explain}, dest, ' ', cancont);
        cancont = spitoption(ulines? $A{btulist user lines}:
                             $A{btulist no user lines},
                             $A{btulist arg explain}, dest, ' ', cancont);
        if  (formatstring)  {
                spitoption($A{btulist format}, $A{btulist arg explain}, dest, ' ', 0);
                fprintf(dest, " \"%s\"", formatstring);
                cancont = 0;
        }
        else
                cancont = spitoption($A{btulist default format}, $A{btulist arg explain}, dest, ' ', cancont);
        putc('\n', dest);
}

int  sort_u(BtuserRef a, BtuserRef b)
{
        return  strcmp(prin_uname((uid_t) a->btu_user), prin_uname((uid_t) b->btu_user));
}

int  sort_g(BtuserRef a, BtuserRef b)
{
        gid_t   ga, gb;
        char    *au, *bu;

        au = prin_uname((uid_t) a->btu_user);           /*  Sets lastgid */
        ga = lastgid;
        bu = prin_uname((uid_t) b->btu_user);
        gb = lastgid;
        if  (ga == gb)
                return  strcmp(au, bu);
        return  strcmp(prin_gname((gid_t) ga), prin_gname((gid_t) gb));
}

MAINFN_TYPE  main(int argc, char **argv)
{
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        BtuserRef       mypriv, ulist = (BtuserRef) 0;
        unsigned        nusers = 0, pn;

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
        argv = optprocess(argv, Adefs, optprocs, $A{btulist arg explain}, $A{btulist arg freeze home}, 0);
        SWAP_TO(Daemuid);

#include "inline/freezecode.c"

        if  (Anychanges & OF_ANY_FREEZE_WANTED)
                exit(0);

        if  (!(defline || ulines))  {
                print_error($E{btulist nothing to do});
                exit(E_USAGE);
        }
        defaultname = gprompt($P{Btulist default name});
        allname = gprompt($P{Btulist all name});
        for  (pn = 0;  pn < NUM_PRIVBITS;  pn++)
                ptab[pn].string = gprompt($P{Read adm abbr}+pn);

        mypriv = getbtuentry(Realuid);

        if  (!(mypriv->btu_priv & BTM_RADMIN))  {
                print_error($E{No read admin file priv});
                exit(E_NOPRIV);
        }
        if  (ulines)  {
                if  (*argv)  {
                        char  **av = argv;
                        BtuserRef  up;
                        nusers = 1;
                        while  (*++av)
                                nusers++;
                        ulist = (BtuserRef) malloc(nusers * sizeof(Btuser));
                        if  (!ulist)
                                ABORT_NOMEM;
                        up = ulist;
                        for  (av = argv;  *av;  av++)  {
                                char    *uname = *av;
                                int_ugid_t  uid;
                                if  (isdigit(uname[0]))
                                        uid = atoi(uname);
                                else  if   ((uid = lookup_uname(uname)) == UNKNOWN_UID)  {
                                        nusers--;
                                        disp_str = uname;
                                        print_error($E{Unknown owner});
                                        continue;
                                }
                                *up++ = *getbtuentry(uid);
                        }
                }
                else  {
                        ulist = getbtulist();
                        nusers = Npwusers;
                }
                if  (alphsort == SRT_USER)
                        qsort(QSORTP1 ulist, nusers, sizeof(Btuser), QSORTP4 sort_u);
                else  if  (alphsort == SRT_GROUP)
                        qsort(QSORTP1 ulist, nusers, sizeof(Btuser), QSORTP4 sort_g);
        }
        if  (!formatstring)
                formatstring = sdefaultfmt;
        udisplay(ulist, nusers);
        exit(0);
}
