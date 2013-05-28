/* btvar.c -- main module for gbch-var

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

#define IPC_MODE        0

#define SYNCMAX         5       /* Before we give up */

Shipc           Oreq;
extern  long    mymtype;

ULONG                   Saveseq;

#define COMP_EQ         1
#define COMP_NE         2
#define COMP_LE         3
#define COMP_LT         4
#define COMP_GE         5
#define COMP_GT         6

uid_t   New_owner;
gid_t   New_group;

char    forcestring,
        Createflag,
        Deleteflag,
        Setflag,
        Undeflag,
        Exportflag,             /* 0 unchanged 1 set local 2 set export */
        Clusterflag;            /* Ditto */

/* The following are just to resolve names used in library routines
   which we steal.  */

char    *Curr_pwd;

char    Compvalue[BTC_VALUE+1],
        Undefvalue[BTC_VALUE+1],
        Setvalue[BTC_VALUE+1],
        Comment[BTV_COMMENT+1];

void  checksetmode(const int, const ushort *, const ushort, USHORT *);

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

int  numeric(const char *str)
{
        if  (!isdigit(*str)  &&  *str != '-')
                return  0;
        while  (*++str)
                if  (!isdigit(*str))
                        return  0;
        return  1;
}

#define BTVAR_INLINE

OPTION(o_explain)
{
        print_error($E{btvar explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_cancelall)
{
        forcestring = 0;
        Createflag = 0;
        Deleteflag = 0;
        Setflag = 0;
        Undeflag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_forcestring)
{
        forcestring = 1;
        return  OPTRESULT_OK;
}

OPTION(o_create)
{
        Createflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_delete)
{
        Deleteflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_setvalue)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        strncpy(Setvalue, arg, BTC_VALUE);
        Setvalue[BTC_VALUE] = '\0';
        Setflag = 1;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_undefvalue)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        strncpy(Undefvalue, arg, BTC_VALUE);
        Undefvalue[BTC_VALUE] = '\0';
        Undeflag = 1;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_comment)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        strncpy(Comment, arg, BTV_COMMENT);
        Comment[BTV_COMMENT] = '\0';
        return  OPTRESULT_ARG_OK;
}

OPTION(o_compar)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        return  OPTRESULT_LAST_ARG_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

OPTION(o_resetexport)
{
        Exportflag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_noexport)
{
        Exportflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_export)
{
        Exportflag = 2;
        return  OPTRESULT_OK;
}

OPTION(o_resetcluster)
{
        Clusterflag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_nocluster)
{
        Clusterflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_cluster)
{
        Clusterflag = 2;
        return  OPTRESULT_OK;
}

OPTION(o_owner)
{
        int_ugid_t      nuid;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_OWNER_CHANGE;

        if  ((nuid = lookup_uname(arg)) == UNKNOWN_UID)  {
                arg_errnum = $E{Unknown owner};
                return  OPTRESULT_ERROR;
        }

        New_owner = nuid;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_group)
{
        int_ugid_t      ngid;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_GROUP_CHANGE;

        if  ((ngid = lookup_gname(arg)) == UNKNOWN_GID)  {
                arg_errnum = $E{Unknown group};
                return  OPTRESULT_ERROR;
        }
        New_group = ngid;
        return  OPTRESULT_ARG_OK;
}

DEOPTION(o_mode_var);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  { '?', $A{btvar arg explain} },
  { 'X', $A{btvar arg cancel} },
  { 'S', $A{btvar arg fstring} },
  { 'C', $A{btvar arg create} },
  { 'D', $A{btvar arg delete} },
  { 's', $A{btvar arg set} },
  { 'u', $A{btvar arg undefval} },
  { 'c', $A{btvar arg comment} },
  { 'M', $A{btvar arg mode} },
  { 'N', $A{btvar arg resetexp} },
  { 'L', $A{btvar arg loco} },
  { 'E', $A{btvar arg export} },
  { 'o', $A{btvar arg resetclust} },
  { 'k', $A{btvar arg nocluster} },
  { 'K', $A{btvar arg cluster} },
  { 'U', $A{btvar arg setu} },
  { 'G', $A{btvar arg setg} },
  { 'e', $A{btvar arg eq} },
  { 'n', $A{btvar arg ne} },
  { 'g', $A{btvar arg gt} },
  { 'l', $A{btvar arg lt} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_cancelall,    o_forcestring,  o_create,
o_delete,       o_setvalue,     o_undefvalue,   o_comment,
o_resetexport,  o_noexport,     o_export,
o_resetcluster, o_nocluster,    o_cluster,
o_mode_var,     o_owner,        o_group,
o_compar,       o_compar,       o_compar,       o_compar,
o_compar,       o_compar,       o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0, pch = '=';

        fprintf(dest, "%s", name);
        if  (!(forcestring  &&  Createflag  &&  Deleteflag  &&  Setflag  &&  Undeflag))  {
                cancont = spitoption($A{btvar arg cancel}, $A{btvar arg explain}, dest, '=', 0);
                pch = ' ';
        }
        if  (forcestring)  {
                cancont = spitoption($A{btvar arg fstring}, $A{btvar arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Createflag)  {
                cancont = spitoption($A{btvar arg create}, $A{btvar arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Deleteflag)  {
                cancont = spitoption($A{btvar arg delete}, $A{btvar arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        cancont = spitoption(Exportflag > 1? $A{btvar arg export}:
                        Exportflag > 0? $A{btvar arg loco}:
                        $A{btvar arg resetexp}, $A{btvar arg explain}, dest, pch, cancont);
        spitoption(Clusterflag > 1? $A{btvar arg cluster}:
                        Clusterflag > 0? $A{btvar arg nocluster}:
                        $A{btvar arg resetclust}, $A{btvar arg explain}, dest, pch, cancont);
        pch = ' ';
        if  (Setflag)  {
                spitoption($A{btvar arg set}, $A{btvar arg explain}, dest, pch, 0);
                putc(' ', dest);
                dumpstr(dest, Setvalue);
                pch = ' ';
        }
        if  (Undeflag)  {
                spitoption($A{btvar arg undefval}, $A{btvar arg explain}, dest, pch, 0);
                putc(' ', dest);
                dumpstr(dest, Undefvalue);
                pch = ' ';
        }
        if  (Comment[0])  {
                spitoption($A{btvar arg comment}, $A{btvar arg explain}, dest, pch, 0);
                putc(' ', dest);
                dumpstr(dest, Comment);
                pch = ' ';
        }
        if  (Procparchanges & OF_MODE_CHANGES)  {
                spitoption($A{btvar arg mode}, $A{btvar arg explain}, dest, pch, 0);
                if  (Mode_set[0] != MODE_NONE)
                        dumpemode(dest, ' ', 'U', Mode_set[0], mypriv->btu_vflags[0]);
                if  (Mode_set[1] != MODE_NONE)
                        dumpemode(dest, ',', 'G', Mode_set[1], mypriv->btu_vflags[1]);
                if  (Mode_set[2] != MODE_NONE)
                        dumpemode(dest, ',', 'O', Mode_set[2], mypriv->btu_vflags[2]);
                pch = ' ';
        }
        if  (Procparchanges & OF_OWNER_CHANGE)  {
                spitoption($A{btvar arg setu}, $A{btvar arg explain}, dest, pch, 0);
                fprintf(dest, " %s", prin_uname(New_owner));
                pch = ' ';
        }
        if  (Procparchanges & OF_GROUP_CHANGE)  {
                spitoption($A{btvar arg setg}, $A{btvar arg explain}, dest, pch, 0);
                fprintf(dest, " %s", prin_gname(New_group));
                pch = ' ';
        }
        putc('\n', dest);
}

/* See whether the user can access the mode.  (Needed by library function rvfile).  */

int  mpermitted(CBtmodeRef md, const unsigned flag, const ULONG fl)
{
        return  1;              /* Dummy - rely on scheduler */
}

/* Send var-type message to scheduler */

void  wrt_vmsg(unsigned code)
{
        int     blkcount = MSGQ_BLOCKS;

        Oreq.sh_params.mcode = code;
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)  {
                        print_error($E{IPC msg q error});
                        exit(E_SETUP);
                }
                blkcount--;
                if  (blkcount <= 0)  {
                        print_error($E{IPC msg q full});
                        exit(E_SETUP);
                }
                sleep(MSGQ_BLOCKWAIT);
        }
}

int  docompar(int oper, BtvarRef vp)
{
        int     ss;

        if  (vp)  {
                BtconRef        cp = &vp->var_value;

                if  (cp->const_type == CON_LONG)  {
                        LONG    clv = cp->con_un.con_long;
                        LONG    clc = atol(Compvalue);
                        ss = clv == clc? 0: clv < clc ? -1: 1;
                }
                else
                        ss = strcmp(cp->con_un.con_string, Compvalue);
        }
        else  {
                if  (!forcestring  &&  numeric(Undefvalue) && numeric(Compvalue))  {
                        LONG    clv = atol(Undefvalue),
                                clc = atol(Compvalue);
                        ss = clv == clc? 0: clv < clc ? -1: 1;
                }
                else
                        ss = strcmp(Undefvalue, Compvalue);
        }

        switch  (oper)  {
        default:
        case  COMP_EQ:
                return  ss == 0;
        case  COMP_NE:
                return  ss != 0;
        case  COMP_LE:
                return  ss <= 0;
        case  COMP_LT:
                return  ss < 0;
        case  COMP_GE:
                return  ss >= 0;
        case  COMP_GT:
                return  ss > 0;
        }
}

/* Display var-type error message */

int  btv_doverror(unsigned retc, BtvarRef vp)
{
        switch  (retc & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                print_error($E{Unexpected sched message});
                break;
        case  VAR_REPLY:
                disp_str = vp->var_name;
                print_error((int) ((retc & ~REQ_TYPE) + $E{Base for scheduler var errors}));
                break;
        case  NET_REPLY:
                disp_str = vp->var_name;
                print_error((int) ((retc & ~REQ_TYPE) + $E{Base for scheduler net errors}));
                break;
        }
        return  retc == V_SYNC? -1: E_NOPRIV;   /* Might refine this later */
}

/* Export set */

int  doexpset(vhash_t vp)
{
        unsigned        retc, newf;

        if  (vp >= 0)
                Oreq.sh_un.sh_var = Var_seg.vlist[vp].Vent;
        newf = Oreq.sh_un.sh_var.var_flags;
        if  (Exportflag > 0)  {
                if  (Exportflag > 1)
                        newf |= VF_EXPORT;
                else
                        newf &= ~VF_EXPORT;
        }
        if  (Clusterflag > 0)  {
                if  (Clusterflag > 1)
                        newf |= VF_CLUSTER;
                else
                        newf &= ~VF_CLUSTER;
        }
        if  (newf == Oreq.sh_un.sh_var.var_flags)
                return  0;
        Oreq.sh_un.sh_var.var_flags = newf;
        Oreq.sh_un.sh_var.var_sequence = Saveseq++;
        wrt_vmsg(V_CHFLAGS);
        if  ((retc = readreply()) != V_OK)
                return  btv_doverror(retc, &Oreq.sh_un.sh_var);
        return  0;
}

/* Try to set the variable.
   Return something suitable for an exit code. */

int  doset(vhash_t vp)
{
        unsigned        retc;

        if  (vp >= 0)  {
                Oreq.sh_un.sh_var = Var_seg.vlist[vp].Vent;
                Oreq.sh_un.sh_var.var_sequence = Saveseq;
        }
        else  if  (Comment[0])
                strcpy(Oreq.sh_un.sh_var.var_comment, Comment);
        if  (!forcestring  &&  numeric(Setvalue))  {
                Oreq.sh_un.sh_var.var_value.const_type = CON_LONG;
                Oreq.sh_un.sh_var.var_value.con_un.con_long = atol(Setvalue);
        }
        else  {
                Oreq.sh_un.sh_var.var_value.const_type = CON_STRING;
                strcpy(Oreq.sh_un.sh_var.var_value.con_un.con_string, Setvalue);
        }
        if  (vp >= 0)  {
                wrt_vmsg(V_ASSIGN);
                retc = readreply();
                if  (retc != V_OK)
                        return  btv_doverror(retc, &Oreq.sh_un.sh_var);
                Saveseq++;
                if  ((Exportflag > 0 || Clusterflag > 0)  &&  (retc = doexpset((vhash_t) -1)) != 0)
                        return  retc;
                if  (!Comment[0])
                        return  0;
                Oreq.sh_un.sh_var = Var_seg.vlist[vp].Vent;
                Oreq.sh_un.sh_var.var_sequence = Saveseq++;
                strcpy(Oreq.sh_un.sh_var.var_comment, Comment);
                wrt_vmsg(V_CHCOMM);
        }
        else   {
                checksetmode(0, mypriv->btu_vflags, mypriv->btu_vflags[0], &Oreq.sh_un.sh_var.var_mode.u_flags);
                checksetmode(1, mypriv->btu_vflags, mypriv->btu_vflags[1], &Oreq.sh_un.sh_var.var_mode.g_flags);
                checksetmode(2, mypriv->btu_vflags, mypriv->btu_vflags[2], &Oreq.sh_un.sh_var.var_mode.o_flags);
                wrt_vmsg(V_CREATE);
        }
        retc = readreply();
        if  (retc != V_OK)
                return  btv_doverror(retc, &Oreq.sh_un.sh_var);
        return  0;
}

/* Ditto for delete.  */

int  dodelete(vhash_t vp)
{
        unsigned        retc;

        Oreq.sh_un.sh_var = Var_seg.vlist[vp].Vent;
        Oreq.sh_un.sh_var.var_sequence = Saveseq;
        wrt_vmsg(V_DELETE);
        if  ((retc = readreply()) == V_OK)
                return  0;
        return  btv_doverror(retc, &Oreq.sh_un.sh_var);
}

/* Ditto for mode set */

int  domodeset(vhash_t vp)
{
        unsigned        retc;
        BtvarRef        varp = &Var_seg.vlist[vp].Vent;
        USHORT          Usave = Oreq.sh_un.sh_var.var_mode.u_flags,
                        Gsave = Oreq.sh_un.sh_var.var_mode.g_flags,
                        Osave = Oreq.sh_un.sh_var.var_mode.o_flags;
        Oreq.sh_un.sh_var = *varp;
        Oreq.sh_un.sh_var.var_sequence = Saveseq++;
        Oreq.sh_un.sh_var.var_mode.u_flags = Usave;
        Oreq.sh_un.sh_var.var_mode.g_flags = Gsave;
        Oreq.sh_un.sh_var.var_mode.o_flags = Osave;
        checksetmode(0, mypriv->btu_vflags, varp->var_mode.u_flags, &Oreq.sh_un.sh_var.var_mode.u_flags);
        checksetmode(1, mypriv->btu_vflags, varp->var_mode.g_flags, &Oreq.sh_un.sh_var.var_mode.g_flags);
        checksetmode(2, mypriv->btu_vflags, varp->var_mode.o_flags, &Oreq.sh_un.sh_var.var_mode.o_flags);
        wrt_vmsg(V_CHMOD);
        if  ((retc = readreply()) != V_OK)
                return  btv_doverror(retc, &Oreq.sh_un.sh_var);
        return  0;
}

/* Ditto for user set */

int  douset(vhash_t vp)
{
        unsigned        retc;

        Oreq.sh_un.sh_var = Var_seg.vlist[vp].Vent;
        Oreq.sh_un.sh_var.var_sequence = Saveseq++;
        Oreq.sh_params.param = New_owner;
        wrt_vmsg(V_CHOWN);
        if  ((retc = readreply()) != V_OK)
                return  btv_doverror(retc, &Oreq.sh_un.sh_var);
        return  0;
}

/* Ditto for group set */

int  dogset(vhash_t vp)
{
        unsigned        retc;

        Oreq.sh_un.sh_var = Var_seg.vlist[vp].Vent;
        Oreq.sh_un.sh_var.var_sequence = Saveseq++;
        Oreq.sh_params.param = New_group;
        wrt_vmsg(V_CHGRP);
        if  ((retc = readreply()) != V_OK)
                return  btv_doverror(retc, &Oreq.sh_un.sh_var);
        return  0;
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *nam, *vp;
        netid_t         hostid = 0L;
        int     compar = 0, synctries = 0;
        vhash_t         Cvarp;
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

        /* Set up scheduler request main parameters.
           Aaargh need to do this 'ere we parse options.  */

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Mode_arg = &Oreq.sh_un.sh_var.var_mode;

        argv = optprocess(argv, Adefs, optprocs, $A{btvar arg explain}, $A{btvar arg freeze home}, 0);
        SWAP_TO(Daemuid);

#define FREEZE_EXIT
#include "inline/freezecode.c"

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

        openvfile(0, 0);
        rvarfile(1);

        /* Do we have a comparison to do?  */

        if  (argv[0]  &&  (argv[0][0] == '-'  ||  argv[0][0] == '+'))  {
                int     fc, lc;
                fc = argv[0][1];
                lc = argv[0][2];
                switch  (tolower(fc))  {
                default:
                        goto  usage;
                case  'e':
                        if  (lc != '\0'  &&  (tolower(lc) != 'q'  ||  argv[0][3] != '\0'))
                                goto  usage;
                        compar = COMP_EQ;
                        break;
                case  'n':
                        if  (lc != '\0'  &&  (tolower(lc) != 'e'  ||  argv[0][3] != '\0'))
                                goto  usage;
                        compar = COMP_NE;
                        break;
                case  'l':
                        if  (lc == '\0')  {
                                compar = COMP_LT;
                                break;
                        }
                        if  (argv[0][3] != '\0')
                                goto  usage;
                        if  (tolower(lc) == 'e')
                                compar = COMP_LE;
                        else  if  (tolower(lc) == 't')
                                compar = COMP_LT;
                        else
                                goto  usage;
                        break;
                case  'g':
                        if  (lc == '\0')  {
                                compar = COMP_GT;
                                break;
                        }
                        if  (argv[0][3] != '\0')
                                goto  usage;
                        if  (tolower(lc) == 'e')
                                compar = COMP_GE;
                        else  if  (tolower(lc) == 't')
                                compar = COMP_GT;
                        else
                                goto  usage;
                        break;
                }

                /* Protest if nothing to compare with.  */

                if  (!*++argv)
                        goto  usage;

                strncpy(Compvalue, *argv++, BTC_VALUE);
                Compvalue[BTC_VALUE] = '\0';
        }

        /* Protest if no value */

        if  (!*argv)  {
 usage:
                print_error($E{Btvar usage error});
                exit(E_USAGE);
        }

        /* Set in case of winges */

        disp_str = nam = *argv;

        if  (strchr(nam, ':'))  {
                int     cnt;
                char    hostn[HOSTNSIZE+1];

                cnt = 0;
                while  (*nam != ':')  {
                        if  (cnt < HOSTNSIZE)
                                hostn[cnt++] = *nam;
                        nam++;
                }
                hostn[cnt] = '\0';
                if  ((hostid = look_int_hostname(hostn)) == -1)  {
                        print_error($E{Btvar unknown host name});
                        exit(E_USAGE);
                }
                nam++;          /* Past colon */
        }

        if  (!isalpha(nam[0]))  {
        badn:
                print_error($E{Btvar bad variable name});
                exit(E_USAGE);
        }

        /* Validate rest of name */

        for  (vp = nam + 1;  *vp;  vp++)
                if  (!isalnum(*vp) && *vp != '_')
                        goto  badn;

        /* Now what are we supposed to be doing */

        if  (Setflag + Deleteflag + Exportflag + Clusterflag + compar + (int) (Procparchanges & (OF_MODE_CHANGES|OF_OWNER_CHANGE|OF_GROUP_CHANGE)) <= 0)  {
                Cvarp = lookupvar(nam, hostid, BTM_READ, &Saveseq);
                if  (Cvarp < 0)  {
                        if  (!Undeflag)  {
                                print_error($E{Btvar notfnd print});
                                exit(E_VNOTFND);
                        }
                        puts(Undefvalue);
                        exit(0);
                }
                if  (Var_seg.vlist[Cvarp].Vent.var_value.const_type == CON_STRING)
                        puts(Var_seg.vlist[Cvarp].Vent.var_value.con_un.con_string);
                else
                        printf("%ld\n", (long) Var_seg.vlist[Cvarp].Vent.var_value.con_un.con_long);
                exit(0);
        }

        if  ((Setflag || Exportflag || Clusterflag) && Deleteflag)  {
                print_error($E{Btvar set and delete});
                exit(E_USAGE);
        }
        if  ((Procparchanges & (OF_MODE_CHANGES|OF_OWNER_CHANGE|OF_GROUP_CHANGE) || Exportflag || Clusterflag) && Deleteflag)  {
                print_error($E{Btvar mode set and delete});
                exit(E_USAGE);
        }

        /* Do we know about the variable?  */

 tryagain:
        Cvarp = lookupvar(nam, hostid, BTM_READ, &Saveseq);

        if  (compar)  {
                if  (Cvarp < 0)  {
                        if  (!Undeflag)  {
                                print_error($E{Btvar notfnd compare});
                                exit(E_VNOTFND);
                        }
                        if  (!docompar(compar, (BtvarRef) 0))
                                exit(E_FALSE);
                }
                else if  (!docompar(compar, &Var_seg.vlist[Cvarp].Vent))
                        exit(E_FALSE);
        }

        if  (Setflag)  {
                int     ret;
                if  (Cvarp < 0)  {
                        if  (!Createflag)  {
                                print_error($E{Btvar notfnd set});
                                exit(E_VNOTFND);
                        }
                        strncpy(Oreq.sh_un.sh_var.var_name, nam, BTV_NAME);

                        if  (Procparchanges & OF_OWNER_CHANGE)  {
                                if  (Oreq.sh_params.uuid != New_owner &&
                                     (mypriv->btu_priv & BTM_WADMIN) == 0)  {
                                        print_error($E{Cannot set owner});
                                        exit(E_NOPRIV);
                                }
                                Oreq.sh_params.uuid = New_owner;
                        }
                        if  (Procparchanges & OF_GROUP_CHANGE)  {
                                if  (Oreq.sh_params.ugid != New_group  &&
                                     (mypriv->btu_priv & BTM_WADMIN) == 0)  {
                                        print_error($E{Cannot set group});
                                        exit(E_NOPRIV);
                                }
                                Oreq.sh_params.ugid = New_group;
                        }
                        Oreq.sh_un.sh_var.var_name[BTV_NAME] = '\0';
                        if  (Exportflag > 1)  {
                                Oreq.sh_un.sh_var.var_flags = VF_EXPORT;
                                if  (Clusterflag > 1)
                                        Oreq.sh_un.sh_var.var_flags |= VF_CLUSTER;
                        }
                }
                else  {
                        if  (Procparchanges & OF_MODE_CHANGES)  {
                                int     ret = domodeset(Cvarp);
                                if  (ret != 0)
                                        exit(ret);
                        }
                        if  (Exportflag || Clusterflag)  {
                                int     ret = doexpset(Cvarp);
                                if  (ret != 0)
                                        exit(ret);
                        }
                        if  (Procparchanges & OF_OWNER_CHANGE)  {
                                int     ret = douset(Cvarp);
                                if  (ret != 0)
                                        exit(ret);
                        }
                        if  (Procparchanges & OF_GROUP_CHANGE)  {
                                int     ret = dogset(Cvarp);
                                if  (ret != 0)
                                        exit(ret);
                        }
                }
                ret = doset(Cvarp);
                if  (ret < 0)   {
                        /* Sync case do it again */
                        if  (++synctries < SYNCMAX)
                                goto  tryagain;
                        exit(E_SYNCFAIL);
                }
                exit(ret);
        }
        else  {
                if  (Procparchanges & OF_MODE_CHANGES)  {
                        int     ret;
                        if  (Cvarp < 0)  {
                                print_error($E{Btvar notfnd mode set});
                                exit(E_VNOTFND);
                        }
                        ret = domodeset(Cvarp);
                        if  (ret != 0)
                                exit(ret);
                }
                if  (Exportflag || Clusterflag)  {
                        int     ret = doexpset(Cvarp);
                        if  (ret != 0)
                                exit(ret);
                }
                if  (Procparchanges & OF_OWNER_CHANGE)  {
                        int     ret;
                        if  (Cvarp < 0)  {
                                print_error($E{Btvar notfnd set user});
                                exit(E_VNOTFND);
                        }
                        ret = douset(Cvarp);
                        if  (ret != 0)
                                exit(ret);
                }
                if  (Procparchanges & OF_GROUP_CHANGE)  {
                        int     ret;
                        if  (Cvarp < 0)  {
                                print_error($E{Btvar notfnd set group});
                                exit(E_VNOTFND);
                        }
                        ret = dogset(Cvarp);
                        if  (ret != 0)
                                exit(ret);
                }
        }

        if  (Deleteflag)  {
                if  (Cvarp < 0)  {
                        print_error($E{Btvar notfnd del});
                        exit(E_VNOTFND);
                }
                exit(dodelete(Cvarp));
        }

        /* Everything has worked!!!!  */

        return  0;
}
