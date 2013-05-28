/* btcichange.c -- main module for gbch-cichange

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
#include "defaults.h"
#include "incl_ugid.h"
#include "bjparam.h"
#include "cmdint.h"
#include "btmode.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "helpalt.h"
#include "optflags.h"

static  char    Filename[] = __FILE__;

#define BTCICHANGE_INLINE

enum    { UPD_CMD, ADD_CMD, DEL_CMD } cmd_type = UPD_CMD;

char    newname[CI_MAXNAME+1],
        pathname[CI_MAXFPATH+1],
        args[CI_MAXARGS+1];

char    nn_set,
        pn_set,
        args_set;

unsigned        niceval = 1000, llarg = 0;
int             s0flag = -1, expflag = -1;      /* -1 means untouched */

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

OPTION(o_explain)
{
        print_error($E{btcichange explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_addci)
{
        cmd_type = ADD_CMD;
        return  OPTRESULT_OK;
}

OPTION(o_delci)
{
        cmd_type = DEL_CMD;
        return  OPTRESULT_OK;
}

OPTION(o_updci)
{
        cmd_type = UPD_CMD;
        return  OPTRESULT_OK;
}

OPTION(o_nice)
{
        int     num;
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv nice};
                return  OPTRESULT_ERROR;
        }
        num = atoi(arg);
        if  (num < 0  ||  num > 39)  {
                arg_errnum = $E{Btuchange nice range};
                return  OPTRESULT_ERROR;
        }
        niceval = num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_ll)
{
        int     num;
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Btuchange inv ll};
                return  OPTRESULT_ERROR;
        }
        num = (unsigned) atoi(arg);
        if  (num <= 0  ||  num > 65535)  {
                arg_errnum = $E{Btuchange inv ll range};
                return  OPTRESULT_ERROR;
        }
        llarg = num;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_newname)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (arg[0] == '-' && arg[1] == '\0')  {
                newname[0] = '\0';
                nn_set = 0;
        }
        else  {
                strncpy(newname, arg, CI_MAXNAME);
                nn_set = 1;
        }
        return  OPTRESULT_ARG_OK;
}

OPTION(o_path)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (arg[0] != '/')  {
                pathname[0] = '\0';
                pn_set = 0;
        }
        else  {
                strncpy(pathname, arg, CI_MAXFPATH);
                pn_set = 1;
        }
        return  OPTRESULT_ARG_OK;
}

OPTION(o_args)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (arg[0] == ':' && arg[1] == '\0')  {
                args[0] = '\0';
                args_set = 0;
        }
        else  {
                strncpy(args, arg, CI_MAXARGS);
                args_set = 1;
        }
        return  OPTRESULT_ARG_OK;
}

OPTION(o_setarg0)
{
        s0flag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_nosetarg0)
{
        s0flag = 0;
        return  OPTRESULT_OK;
}

OPTION(o_setexp)
{
        expflag = 1;
        return  OPTRESULT_OK;
}

OPTION(o_nosetexp)
{
        expflag = 0;
        return  OPTRESULT_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btcichange arg explain} },
  {  'A', $A{btcichange arg add} },
  {  'D', $A{btcichange arg delete} },
  {  'U', $A{btcichange arg update} },
  {  'N', $A{btcichange arg nice} },
  {  'l', $A{btcichange arg ll} },
  {  'n', $A{btcichange arg newname} },
  {  'p', $A{btcichange arg path} },
  {  'a', $A{btcichange arg args} },
  {  't', $A{btcichange arg a0title} },
  {  'i', $A{btcichange arg a0name} },
  {  'u', $A{btcichange arg noexpand} },
  {  'e', $A{btcichange arg expand} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_addci,        o_delci,        o_updci,
o_nice,         o_ll,           o_newname,      o_path,
o_args,         o_setarg0,      o_nosetarg0,    o_nosetexp,
o_setexp,       o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont;

        fprintf(dest, "%s", name);
        cancont = spitoption(cmd_type == ADD_CMD? $A{btcichange arg add}:
                             cmd_type == DEL_CMD? $A{btcichange arg delete}: $A{btcichange arg update}, $A{btcichange arg explain}, dest, '=', 0);
        if  (s0flag >= 0)
                cancont = spitoption(s0flag>0? $A{btcichange arg a0title}: $A{btcichange arg a0name}, $A{btcichange arg explain}, dest, ' ', cancont);
        if  (expflag >= 0)
                cancont = spitoption(expflag>0? $A{btcichange arg expand}: $A{btcichange arg noexpand}, $A{btcichange arg explain}, dest, ' ', cancont);

        if  (niceval < 40)  {
                spitoption($A{btcichange arg nice}, $A{btcichange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", niceval);
        }

        if  (llarg != 0)  {
                spitoption($A{btcichange arg ll}, $A{btcichange arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", llarg);
        }
        if  (cmd_type == UPD_CMD)  {
                spitoption($A{btcichange arg newname}, $A{btcichange arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", nn_set? newname: "-");
        }
        spitoption($A{btcichange arg path}, $A{btcichange arg explain}, dest, ' ', 0);
        fprintf(dest, " %s", pn_set? pathname: "-");
        spitoption($A{btcichange arg args}, $A{btcichange arg explain}, dest, ' ', 0);
        fprintf(dest, " %s\n", args_set? args: ":");
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int             ci_ind, ind2;
        char            *ciname;
        char            *Curr_pwd = (char *) 0;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        CmdintRef       Cientry;
        int             ret;

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
        argv = optprocess(argv, Adefs, optprocs, $A{btcichange arg explain}, $A{btcichange arg freeze home}, 0);

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  (argv[0] == (char *) 0)  {
                print_error($E{Btcichange missing args});
                exit(E_USAGE);
        }

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);
        mypriv = getbtuser(Realuid);
        if  (!(mypriv->btu_priv & BTM_SPCREATE))  {
                print_error($E{No upd ci perm});
                exit(E_NOPRIV);
        }

        if  ((ret = open_ci(O_RDWR)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }

        disp_str = ciname = argv[0];

        switch  (cmd_type)  {
        case  UPD_CMD:
        default:
                if  ((ci_ind = validate_ci(ciname)) < 0)  {
                        print_error($E{Btcichange unknown ci});
                        exit(E_USAGE);
                }
                Cientry = &Ci_list[ci_ind];
                if  (nn_set)  {
                        if  ((ind2 = validate_ci(newname)) >= 0  &&  ind2 != ci_ind)  {
                                disp_str2 = newname;
                                print_error($E{Btcichange name clash});
                                exit(E_CLASHES);
                        }
                        strcpy(Cientry->ci_name, newname);
                }
                if  (llarg != 0)
                        Cientry->ci_ll = llarg;
                if  (niceval < 40)
                        Cientry->ci_nice = (unsigned char) niceval;

                /* Leave it alone unless actually changing it... */

                Cientry->ci_flags &= CIF_SETARG0 | CIF_INTERPARGS;
                if  (s0flag >= 0)  {
                        if  (s0flag > 0)
                                Cientry->ci_flags |= CIF_SETARG0;
                        else
                                Cientry->ci_flags &= ~CIF_SETARG0;
                }
                if  (expflag >= 0)  {
                        if  (expflag > 0)
                                Cientry->ci_flags |= CIF_INTERPARGS;
                        else
                                Cientry->ci_flags &= ~CIF_INTERPARGS;
                }
                if  (pn_set)
                        strcpy(Cientry->ci_path, pathname);
                if  (args_set)
                        strcpy(Cientry->ci_args, args);
                lseek(Ci_fd, (long) (ci_ind * sizeof(Cmdint)), 0);
                Ignored_error = write(Ci_fd, (char *) Cientry, sizeof(Cmdint));
                break;

        case  DEL_CMD:
                if  ((ci_ind = validate_ci(ciname)) < 0)  {
                        print_error($E{Btcichange unknown ci});
                        exit(E_USAGE);
                }
                if  (ci_ind == CI_STDSHELL)  {
                        print_error($E{Btcichange del def});
                        exit(E_USAGE);
                }
                Ci_list[ci_ind].ci_name[0] = '\0';
                lseek(Ci_fd, (long) (ci_ind * sizeof(Cmdint)), 0);
                Ignored_error = write(Ci_fd, (char *) &Ci_list[ci_ind], sizeof(Cmdint));
                break;

        case  ADD_CMD:
                if  (validate_ci(ciname) >= 0)  {
                        print_error($E{Btcichange new name clash});
                        exit(E_CLASHES);
                }
                if  (!pn_set)  {
                        print_error($E{Btcichange no path name});
                        exit(E_USAGE);
                }

                /* Find a hole, otherwise grow the list.  */

                for  (ci_ind = 0;  ci_ind < Ci_num;  ci_ind++)
                        if  (Ci_list[ci_ind].ci_name[0] == '\0')
                                goto  gotit;
                Ci_num++;
                if  (!(Ci_list = (CmdintRef) realloc((char *) Ci_list, Ci_num * sizeof(Cmdint))))
                        ABORT_NOMEM;
        gotit:
                Cientry = &Ci_list[ci_ind];
                strcpy(Cientry->ci_name, ciname);
                strcpy(Cientry->ci_path, pathname);
                if  (args_set)
                        strcpy(Cientry->ci_args, args);
                else
                        Cientry->ci_args[0] = '\0';
                Cientry->ci_ll = llarg != 0? llarg: mypriv->btu_spec_ll;
                Cientry->ci_nice = niceval < 40? niceval: DEF_CI_NICE;
                Cientry->ci_flags = 0;
                if  (s0flag > 0)
                        Cientry->ci_flags |= CIF_SETARG0;
                if  (expflag > 0)
                        Cientry->ci_flags |= CIF_INTERPARGS;
                lseek(Ci_fd, (long) (ci_ind * sizeof(Cmdint)), 0);
                Ignored_error = write(Ci_fd, (char *) &Ci_list[ci_ind], sizeof(Cmdint));
                break;
        }

        return  0;
}
