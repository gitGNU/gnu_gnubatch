/* btvccgi.c -- CGI variable manipulation

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
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "shreq.h"
#include "files.h"
#include "helpargs.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "spitrouts.h"
#include "optflags.h"
#include "shutilmsg.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "cgifndjb.h"

#define IPC_MODE        0

#define SYNCMAX         5       /* Before we give up */

extern  int     Ctrl_chan;

Btvar   newvar;

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
        netid_t         hostid;
};

int  arg_create(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_READ, &Saveseq);

        if  (vp >= 0)  {
                disp_str = aop->varname;
                html_disperror($E{btvcgi var exists});
                exit(E_CLASHES);
        }

        BLOCK_ZERO(&newvar, sizeof(newvar));
        strncpy(newvar.var_name, aop->varname, BTV_NAME);
        newvar.var_mode.u_flags = mypriv->btu_vflags[0];
        newvar.var_mode.g_flags = mypriv->btu_vflags[1];
        newvar.var_mode.o_flags = mypriv->btu_vflags[2];
        if  (signednumeric(aop->arg2))  {
                newvar.var_value.const_type = CON_LONG;
                newvar.var_value.con_un.con_long = atol(aop->arg2);
        }
        else  {
                newvar.var_value.const_type = CON_STRING;
                strncpy(newvar.var_value.con_un.con_string, aop->arg2, BTC_VALUE);
        }
        return  wvmsg(V_CREATE, &newvar, 0);
}

int  arg_delete(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_READ, &Saveseq);
        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd del});
                exit(E_VNOTFND);
        }
        newvar = Var_seg.vlist[vp].Vent;
        return  wvmsg(V_DELETE, &Var_seg.vlist[vp].Vent, Saveseq);
}

int  arg_assign(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_WRITE, &Saveseq);
        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = Var_seg.vlist[vp].Vent;
        if  (signednumeric(aop->arg2))  {
                newvar.var_value.const_type = CON_LONG;
                newvar.var_value.con_un.con_long = atol(aop->arg2);
        }
        else  {
                newvar.var_value.const_type = CON_STRING;
                strncpy(newvar.var_value.con_un.con_string, aop->arg2, BTC_VALUE);
        }
        return  wvmsg(V_ASSIGN, &newvar, Saveseq);
}

int  arg_incr(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_READ|BTM_WRITE, &Saveseq);

        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        if  (Var_seg.vlist[vp].Vent.var_value.const_type != CON_LONG)  {
                disp_str = aop->varname;
                html_disperror($E{btvcgi var non numeric});
                exit(E_USAGE);
        }

        newvar = Var_seg.vlist[vp].Vent;
        newvar.var_value.con_un.con_long += atol(aop->arg2);
        return  wvmsg(V_ASSIGN, &newvar, Saveseq);
}

int  arg_decr(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_READ|BTM_WRITE, &Saveseq);

        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        if  (Var_seg.vlist[vp].Vent.var_value.const_type != CON_LONG)  {
                disp_str = aop->varname;
                html_disperror($E{btvcgi var non numeric});
                exit(E_USAGE);
        }

        newvar = Var_seg.vlist[vp].Vent;
        newvar.var_value.con_un.con_long -= atol(aop->arg2);
        return  wvmsg(V_ASSIGN, &newvar, Saveseq);
}

int  arg_comment(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_WRITE, &Saveseq);
        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = Var_seg.vlist[vp].Vent;
        strncpy(newvar.var_comment, aop->arg2, BTV_COMMENT);
        return  wvmsg(V_CHCOMM, &newvar, Saveseq);
}

int  arg_export(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_WRITE, &Saveseq);
        unsigned  newflgs = 0;
        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd set});
                exit(E_VNOTFND);
        }
        newvar = Var_seg.vlist[vp].Vent;

        switch  (tolower(aop->arg2[0]))  {
        case  'e':
                newflgs = VF_EXPORT;
                break;
        case  'c':
                newflgs = VF_EXPORT|VF_CLUSTER;
                break;
        }
        if  (newflgs == (newvar.var_flags & (VF_EXPORT|VF_CLUSTER)))
                return  -1;
        newvar.var_flags &= ~(VF_EXPORT|VF_CLUSTER);
        newvar.var_flags |= newflgs;
        return  wvmsg(V_CHFLAGS, &newvar, Saveseq);
}

int  arg_perms(struct argop *aop)
{
        ULONG   Saveseq;
        vhash_t vp = lookupvar(aop->varname, aop->hostid, BTM_READ, &Saveseq);
        USHORT  sarr[3], usarr[3];

        if  (vp < 0)  {
                disp_str = aop->varname;
                html_disperror($E{Btvar notfnd mode set});
                exit(E_VNOTFND);
        }

        newvar = Var_seg.vlist[vp].Vent;
        decode_permflags(sarr, aop->arg2, 0, 0);
        decode_permflags(usarr, aop->arg3, 1, 0);
        newvar.var_mode.u_flags |= sarr[0];
        newvar.var_mode.u_flags &= ~usarr[0];
        newvar.var_mode.g_flags |= sarr[1];
        newvar.var_mode.g_flags &= ~usarr[1];
        newvar.var_mode.o_flags |= sarr[2];
        newvar.var_mode.o_flags &= ~usarr[2];
        return  wvmsg(V_CHMOD, &newvar, Saveseq);
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
                if  ((hostid = look_int_hostname(*args)) == -1)  {
                        disp_str = *args;
                        html_disperror($E{Btvar unknown host name});
                        exit(E_USAGE);
                }
                if  (hostid  &&  aop->locovar)  {
                        disp_str = arg;
                        html_disperror($E{btvcgi op local host only});
                        exit(E_USAGE);
                }
                aop->hostid = hostid;
                aop->varname = ++cp;
        }
        else  {
                aop->hostid = 0;
                aop->varname = *args;
        }
        aop->arg2 = args[1];
        aop->arg3 = args[2];

        /* Now proceed with operation */

        if  ((retc = (*aop->arg_fn)(aop)) > 0)  {
                html_disperror(retc);
                exit(E_SETUP);
        }
        if  (retc < 0)          /* Setting flags no change */
                return;

        if  ((retc = readreply()) != V_OK)  {
                html_disperror(doverror(retc, &newvar));
                exit(retc == V_SYNC? E_SYNCFAIL: E_NOPRIV);
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
        newargs = cgi_arginterp(argc, argv, CGI_AI_SUBSID); /* Side effect of cgi_arginterp is to set Realuid */
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        /* Now we want to be Daemuid throughout if possible.  */
        setuid(Daemuid);
        prin_uname(Realuid);    /* Realuid got set by cgi_arginterp */
        Realgid = lastgid;      /* lastgid got set by prin_uname */
        mypriv = getbtuser(Realuid);

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

        openvfile(0, 0);
        rvarfile(1);
        perform_update(newargs);
        html_out_or_err("vopok", 1);
        return  0;
}
