/* rbtvccgi.c -- remote CGI variable operations

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

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "gbatch.h"
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_ugid.h"
#include "incl_net.h"
#include "network.h"
#include "statenums.h"
#include "errnums.h"
#include "cfile.h"
#include "ecodes.h"
#include "helpalt.h"
#include "formats.h"
#include "optflags.h"
#include "cgiuser.h"
#include "xihtmllib.h"
#include "listperms.h"
#include "files.h"
#include "rcgilib.h"

apiBtvar        newvar;
int             Nvars;
struct  var_with_slot  *var_sl_list;

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

int  signednumeric(const char *str)
{
        if  (!isdigit(*str)  &&  *str != '-')
                return  0;
        while  (*++str)
                if  (!isdigit(*str))
                        return  0;
        return  1;
}

#define BTVAR_INLINE

struct  argop  {
        const   char    *name;          /* Name of parameter case insens */
        int     (*arg_fn)(struct argop *);
        unsigned  char  locovar;        /* Local vars only */
        unsigned  char  npars;          /* Number of args */
        char            *varname, *arg2, *arg3; /* Supplied args */
};

int  arg_create(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;

        if  (find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{btvcgi var exists});
                exit(E_CLASHES);
        }

        strncpy(newvar.var_name, aop->varname, BTV_NAME);
        newvar.var_mode.u_flags = userpriv.btu_vflags[0];
        newvar.var_mode.g_flags = userpriv.btu_vflags[1];
        newvar.var_mode.o_flags = userpriv.btu_vflags[2];
        if  (signednumeric(aop->arg2))  {
                newvar.var_value.const_type = CON_LONG;
                newvar.var_value.con_un.con_long = atol(aop->arg2);
        }
        else  {
                newvar.var_value.const_type = CON_STRING;
                strncpy(newvar.var_value.con_un.con_string, aop->arg2, BTC_VALUE);
        }
        return  gbatch_varadd(xbapi_fd, &newvar);
}

int  arg_delete(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd del});
                exit(E_VNOTFND);
        }
        return  gbatch_vardel(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot);
}

int  arg_assign(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = vs.var;
        if  (signednumeric(aop->arg2))  {
                newvar.var_value.const_type = CON_LONG;
                newvar.var_value.con_un.con_long = atol(aop->arg2);
        }
        else  {
                newvar.var_value.const_type = CON_STRING;
                strncpy(newvar.var_value.con_un.con_string, aop->arg2, BTC_VALUE);
        }
        return  gbatch_varupd(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot, &newvar);
}

int  arg_incr(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = vs.var;
        if  (newvar.var_value.const_type != CON_LONG)  {
                disp_str = aop->varname;
                html_disperror($E{btvcgi var non numeric});
                exit(E_USAGE);
        }
        newvar.var_value.con_un.con_long += atol(aop->arg2);
        return  gbatch_varupd(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot, &newvar);
}

int  arg_decr(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = vs.var;
        if  (newvar.var_value.const_type != CON_LONG)  {
                disp_str = aop->varname;
                html_disperror($E{btvcgi var non numeric});
                exit(E_USAGE);
        }
        newvar.var_value.con_un.con_long -= atol(aop->arg2);
        return  gbatch_varupd(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot, &newvar);
}

int  arg_comment(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        return  gbatch_varchcomm(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot, aop->arg2);
}

int  arg_export(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;
        unsigned  newflgs = 0;

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = vs.var;

        switch  (tolower(aop->arg2[0]))  {
        case  'e':
                newflgs = VF_EXPORT;
                break;
        case  'c':
                newflgs = VF_EXPORT|VF_CLUSTER;
                break;
        }
        if  (newflgs == (newvar.var_flags & (VF_EXPORT|VF_CLUSTER)))
                return  0;
        newvar.var_flags &= ~(VF_EXPORT|VF_CLUSTER);
        newvar.var_flags |= newflgs;
        return  gbatch_varupd(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot, &newvar);
}

int  arg_perms(struct argop *aop)
{
        char    *vname = aop->varname;
        struct  var_with_slot vs;
        USHORT  sarr[3], usarr[3];

        if  (!find_var_by_name((const char **) &vname, &vs))  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd mode set});
                exit(E_VNOTFND);
        }

        newvar = vs.var;
        decode_permflags(sarr, aop->arg2, 0, 0);
        decode_permflags(usarr, aop->arg3, 1, 0);
        newvar.var_mode.u_flags |= sarr[0];
        newvar.var_mode.u_flags &= ~usarr[0];
        newvar.var_mode.g_flags |= sarr[1];
        newvar.var_mode.g_flags &= ~usarr[1];
        newvar.var_mode.o_flags |= sarr[2];
        newvar.var_mode.o_flags &= ~usarr[2];
        return  gbatch_varchmod(xbapi_fd, GBATCH_FLAG_IGNORESEQ, vs.slot, &newvar.var_mode);
}

struct  argop   aolist[] =  {
        { "assign", arg_assign, 0, 2  },
        { "incr", arg_incr, 0, 2  },
        { "decr", arg_decr, 0, 2  },
        { "create", arg_create, 1, 2  },
        { "delete", arg_delete, 1, 1  },
        { "comment", arg_comment, 0, 2  },
        { "export", arg_export, 1, 2  },
        { "perms", arg_perms, 0, 3  }
};

void  perform_update(char **args)
{
        char    *arg = *args++, **ap, *cp;
        struct  argop   *aop;
        unsigned  nargs = 0;
        int     retc;

        if  (!arg)
                return;

        for  (aop = aolist;  aop < &aolist[sizeof(aolist)/sizeof(struct argop)];  aop++)
                if  (ncstrcmp(aop->name, arg) == 0)
                        goto  found;
        if  (html_out_cparam_file("badcarg", 1, arg))
                exit(E_USAGE);
        html_error(arg);
        exit(E_SETUP);
 found:
        for  (ap = args;  *ap;  ap++)
                nargs++;

        if  (nargs != aop->npars)  {
                disp_str = arg;
                disp_arg[0] = aop->npars;
                disp_arg[1] = nargs;
                html_disperror($E{btvcgi wrong number args});
                exit(E_USAGE);
        }

        /* Decode job name / host name.
           Protest if operation can't be done remotely. */

        if  ((cp = strchr(*args, ':')))  {
                netid_t hostid;
                *cp = '\0';
                if  (!(hostid = my_look_hostname(*args)))  {
                        disp_str = *args;
                        html_disperror($E{Btvar unknown host name});
                        exit(E_USAGE);
                }
                hostid = ext2int_netid_t(hostid);
                if  (hostid  &&  aop->locovar)  {
                        disp_str = arg;
                        html_disperror($E{btvcgi op local host only});
                        exit(E_USAGE);
                }
        }
        aop->varname = args[0];
        aop->arg2 = args[1];
        aop->arg3 = args[2];

        /* Now proceed with operation */

        if  ((retc = (*aop->arg_fn)(aop)) < 0)  {
                html_disperror($E{Base for API errors} + retc);
                exit(E_PERM);
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *realuname, **newargs;
        int_ugid_t      chku;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        html_openini();
        hash_hostfile();
        Effuid = geteuid();
        Effgid = getegid();
        if  ((chku = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)
                Daemuid = ROOTID;
        else
                Daemuid = chku;
        newargs = cgi_arginterp(argc, argv, CGI_AI_REMHOST|CGI_AI_SUBSID); /* Side effect of cgi_arginterp is to set Realuid */
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        realuname = prin_uname(Realuid);        /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;
        setgid(Realgid);
        setuid(Realuid);
        api_open(realuname);
        perform_update(newargs);
        html_out_or_err("vopok", 1);
        return  0;
}
