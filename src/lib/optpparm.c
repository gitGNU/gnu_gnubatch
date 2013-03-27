/* optpparm.c -- option handling related to process parameters

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
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "errnums.h"
#include "helpargs.h"
#include "optflags.h"
#include "ecodes.h"

#ifndef _NFILE
#define _NFILE  64
#endif

int             Argcnt,
                Redircnt;

/* Define these here */

char            *Args[MAXJARGS];
Mredir          Redirs[MAXJREDIRS];
Menvir          Envs[MAXJENVIR];

char            *jobqueue,
                *exitcodename,
                *signalname;

BtuserRef       mypriv;

BtmodeRef       Mode_arg;
BtjobRef        JREQ;
netid_t         Out_host;

char            *job_title,
                *job_cwd;

char            Mode_set[3];    /* 0,1,2 = U,G,O (may be vars) */

EOPTION(o_advterr)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_ADVT_CHANGES;
        JREQ->h.bj_jflags &= ~BJ_NOADVIFERR;
        return  OPTRESULT_OK;
}

EOPTION(o_noadvterr)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_ADVT_CHANGES;
        JREQ->h.bj_jflags |= BJ_NOADVIFERR;
        return  OPTRESULT_OK;
}

EOPTION(o_directory)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_DIR_CHANGES;
        if  (job_cwd)
                free(job_cwd);
        job_cwd = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_exits)
{
        int     which = 0;
        unsigned        n1, n2;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_EXIT_CHANGES;

        switch  (*arg)  {
        default:
                break;
        case  'n':case  'N':
                /* which = 0; */
                arg++;
                break;
        case  'e':case  'E':
                which = 1;
                arg++;
                break;
        }
        if  (!isdigit(*arg))
                goto  badarg;
        n1 = 0;
        do  n1 = n1 * 10 + *arg++ - '0';
        while  (isdigit(*arg));
        if  (*arg == ':')  {
                arg++;
                if  (!isdigit(*arg))
                        goto  badarg;
                n2 = 0;
                do  n2 = n2 * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (n1 > n2)  {
                        int     tmp = n1;
                        n1 = n2;
                        n2 = tmp;
                }
        }
        else
                n2 = n1;
        if  (*arg)
                goto  badarg;
        if  (n2 > 255)
                goto  badarg;
        if  (which)  {
                JREQ->h.bj_exits.elower = (unsigned char) n1;
                JREQ->h.bj_exits.eupper = (unsigned char) n2;
        }
        else  {
                JREQ->h.bj_exits.nlower = (unsigned char) n1;
                JREQ->h.bj_exits.nupper = (unsigned char) n2;
        }
        return  OPTRESULT_ARG_OK;
 badarg:
        arg_errnum = $E{Bad exit code spec};
        return  OPTRESULT_ERROR;
}

EOPTION(o_noexport)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_EXPORT_CHANGES;
        JREQ->h.bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);
        return  OPTRESULT_OK;
}

EOPTION(o_export)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_EXPORT_CHANGES;
        JREQ->h.bj_jflags &= ~BJ_REMRUNNABLE;
        JREQ->h.bj_jflags |= BJ_EXPORT;
        return  OPTRESULT_OK;
}

EOPTION(o_fullexport)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_EXPORT_CHANGES;
        JREQ->h.bj_jflags |= BJ_EXPORT|BJ_REMRUNNABLE;
        return  OPTRESULT_OK;
}

EOPTION(o_interpreter)
{
        int     nn;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_INTERP_CHANGES;

        while  (isspace(*arg))
                arg++;

        if  ((nn = validate_ci(arg)) >= 0)  {
                strcpy(JREQ->h.bj_cmdinterp, arg);
                JREQ->h.bj_ll = Ci_list[nn].ci_ll;
                return  OPTRESULT_ARG_OK;
        }

        arg_errnum = $E{Unknown command interp};
        return  OPTRESULT_ERROR;
}

EOPTION(o_loadlev)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_LOADLEV_CHANGES;

        num = atoi(arg);
        if  (num <= 0  ||  num > 0x7fff)  {
                disp_arg[0] = num;
                arg_errnum = $E{Load level out of range};
                return  OPTRESULT_ERROR;
        }

        /* Mypriv won't be set yet in rbtr as we don't know which
           machines privs we are talking about until we have
           finished reading the args.  */

        if  (mypriv  &&  JREQ->h.bj_ll != num  &&  !(mypriv->btu_priv & BTM_SPCREATE))  {
                arg_errnum = $E{No special create};
                return  OPTRESULT_ERROR;
        }
        JREQ->h.bj_ll = (USHORT) num;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_mail)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_MAIL_CHANGES;
        JREQ->h.bj_jflags |= BJ_MAIL;
        return  OPTRESULT_OK;
}

EOPTION(o_write)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_WRITE_CHANGES;
        JREQ->h.bj_jflags |= BJ_WRT;
        return  OPTRESULT_OK;
}

EOPTION(o_cancmailwrite)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_MAIL_CHANGES | OF_WRITE_CHANGES;
        JREQ->h.bj_jflags &= ~(BJ_WRT|BJ_MAIL);
        return  OPTRESULT_OK;
}

EOPTION(o_normal)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_PROGRESS_CHANGES;
        JREQ->h.bj_progress = BJP_NONE;
        return  OPTRESULT_OK;
}

EOPTION(o_ascanc)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_PROGRESS_CHANGES;
        JREQ->h.bj_progress = BJP_CANCELLED;
        return  OPTRESULT_OK;
}

EOPTION(o_asdone)
{
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_PROGRESS_CHANGES;
        JREQ->h.bj_progress = BJP_DONE;
        return  OPTRESULT_OK;
}

EOPTION(o_priority)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_PRIO_CHANGES;

        num = atoi(arg);
        if  (num <= 0 || num > 255)  {
                disp_arg[0] = num;
                arg_errnum = $E{Priority out of range};
                return  OPTRESULT_ERROR;
        }
        JREQ->h.bj_pri = (unsigned char) num;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_deltime)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Bad delete time};
                return  OPTRESULT_ERROR;
        }
        num = atoi(arg);
        if  (num < 0  ||  num > 65535)  {
                arg_errnum = $E{Bad delete time};
                return  OPTRESULT_ERROR;
        }
        JREQ->h.bj_deltime = (USHORT) num;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_DELTIME_SET;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_runtime)
{
        int     hr = 0, mn = 0, sc = 0;
        int     bits;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        for  (bits = 0;  bits < 3;  bits++)  {
                if  (!isdigit(*arg))
                        goto  badrtime;
                do  sc = sc*10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (!*arg)
                        goto  setres;
                if  (*arg++ != ':')
                        goto  badrtime;
                hr = mn;
                mn = sc;
                sc = 0;
        }
 badrtime:
        arg_errnum = $E{Bad run time};
        return  OPTRESULT_ERROR;
 setres:
        if  (mn < 0 || mn > 59 || sc < 0 || sc > 59)
                goto  badrtime;
        JREQ->h.bj_runtime = (hr * 60L + mn) * 60L + sc;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_RUNTIME_SET;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_whichsig)
{
        int     num;

        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))  {
                arg_errnum = $E{Bad signal number};
                return  OPTRESULT_ERROR;
        }
        num = atoi(arg);
        if  (num < 0  ||  num >= NSIG)  {
                arg_errnum = $E{Bad signal number};
                return  OPTRESULT_ERROR;
        }
        JREQ->h.bj_autoksig = (USHORT) num;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_WHICHSIG_SET;
        return  OPTRESULT_ARG_OK;

}

EOPTION(o_gracetime)
{
        int     mn = 0, sc = 0;
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (!isdigit(*arg))
                goto  badrtime;
        do  sc = sc*10 + *arg++ - '0';
        while  (isdigit(*arg));
        if  (!*arg)
                goto  setres;
        if  (*arg++ != ':')
                goto  badrtime;
        mn = sc;
        sc = 0;
        do  sc = sc*10 + *arg++ - '0';
        while  (isdigit(*arg));
        if  (!*arg)
                goto  setres;
 badrtime:
        arg_errnum = $E{Bad grace time};
        return  OPTRESULT_ERROR;
 setres:
        if  (sc < 0 || sc > 59)
                goto  badrtime;
        sc += mn * 60;
        if  (sc < 0 || sc > 65535)
                goto  badrtime;
        JREQ->h.bj_runon = (USHORT) sc;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_GRACETIME_SET;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_umask)
{
        unsigned  result = 0;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        while  (*arg)  {
                if  (*arg < '0' || *arg > '7')  {
                        arg_errnum = $E{Bad umask};
                        return  OPTRESULT_ERROR;
                }
                result = (result << 3) + *arg++ - '0';
        }
        if  (result > 0777)  {
                arg_errnum = $E{Bad umask};
                return  OPTRESULT_ERROR;
        }
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_UMASK_CHANGES;
        JREQ->h.bj_umask = (USHORT) result;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_ulimit)
{
        LONG  result;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        result = atol(arg);
        if  (result < 0)  {
                arg_errnum = $E{Bad ulimit};
                return  OPTRESULT_ERROR;
        }
        JREQ->h.bj_ulimit = result;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_ULIMIT_CHANGES;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_title)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_TITLE_CHANGES;
        Dispflags |= DF_HAS_HDR;
        if  (job_title)
                free(job_title);
        job_title = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_cancargs)
{
        unsigned  cnt;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_ARG_CLEAR | OF_ARG_CHANGES;

        for  (cnt = 0;  cnt < Argcnt;  cnt++)
                free(Args[cnt]);
        Argcnt = 0;
        return  OPTRESULT_OK;
}

EOPTION(o_argument)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_ARG_CHANGES;

        if  (Argcnt >= MAXJARGS)  {
                arg_errnum = $E{Too many arguments};
                return  OPTRESULT_ERROR;
        }
        else  {
                Args[Argcnt] = stracpy(arg);
                Argcnt++;
        }
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_cancio)
{
        unsigned  cnt;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_IO_CLEAR | OF_IO_CHANGES;

        for  (cnt = 0;  cnt < Redircnt;  cnt++)
                if  (Redirs[cnt].action < RD_ACT_CLOSE)  {
                        free(Redirs[cnt].un.buffer);
                        Redirs[cnt].un.buffer = (char *) 0;
                }
        Redircnt = 0;
        return  OPTRESULT_OK;
}

EOPTION(o_io)
{
        int     whichfd = -1, action;
        MredirRef       curre;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_IO_CHANGES;

        if  (Redircnt >= MAXJREDIRS)  {
                arg_errnum = $E{Redirection max exceeded};
                return  OPTRESULT_ERROR;
        }
        curre = &Redirs[Redircnt];

        /* Slurp up any leading space */

        while  (isspace(*arg))
                arg++;

        /* Any leading digit is a file descriptor */

        if  (isdigit(*arg))  {
                whichfd = 0;
                do      whichfd = whichfd * 10 + *arg++ - '0';
                while  (isdigit(*arg));

                if  (whichfd < 0 || whichfd >= _NFILE)  {
                        disp_arg[0] = whichfd;
                        arg_errnum = $E{File descriptor out of range};
                        return  OPTRESULT_ERROR;
                }
        }

        switch  (*arg++)  {
        default:
                goto  badarg;

                /* Some sort of input */

        case  '<':

                if  (whichfd < 0)
                        whichfd = 0;

                action = RD_ACT_RD;

                switch  (*arg)  {
                default:
                        goto  rfname;
                case  '&':
                        goto  amper;
                case  '|':
                        arg++;
                        action = RD_ACT_PIPEI;
                        goto  rfname;
                case  '>':
                        action = RD_ACT_RDWR;
                        if  (*++arg == '>')  {
                                arg++;
                                action = RD_ACT_RDWRAPP;
                        }
                        if  (*arg == '&')
                                goto  amper;
                        goto  rfname;
                }

                /* Some sort of output */

        case  '>':

                if  (whichfd < 0)
                        whichfd = 1;

                action = RD_ACT_WRT;

                switch  (*arg)  {
                default:
                        goto  rfname;

                case  '&':
                        goto  amper;
                case  '|':
                        arg++;
                        action = RD_ACT_PIPEO;
                        goto  rfname;
                case  '>':
                        action = RD_ACT_APPEND;
                        if  (*++arg == '<')  {
                                arg++;
                                action = RD_ACT_RDWRAPP;
                        }
                        if  (*arg == '&')
                                goto  amper;
                        goto  rfname;
                }

        case  '|':
                if  (whichfd < 0)
                        whichfd = 1;
                action = RD_ACT_PIPEO;
                goto  rfname;
        }

 amper:
        if  (*++arg == '-')  {
                action = RD_ACT_CLOSE;
                arg++;
        }
        else  {
                int     dupfd = 0;
                action = RD_ACT_DUP;
                if  (!isdigit(*arg))
                        goto  badarg;
                do  dupfd = dupfd * 10 + *arg++ - '0';
                while  (isdigit(*arg));
                if  (dupfd < 0 || dupfd >= _NFILE)  {
                        disp_arg[0] = dupfd;
                        arg_errnum = $E{File descriptor out of range};
                        return  OPTRESULT_ERROR;
                }
                curre->un.arg = (USHORT) dupfd;
        }
        while  (isspace(*arg))
                arg++;
        if  (*arg)
                goto  badarg;
        curre->fd = (unsigned char) whichfd;
        curre->action = (unsigned char) action;
        Redircnt++;
        return  OPTRESULT_ARG_OK;

 rfname:
        while  (isspace(*arg))
                arg++;
        if  (!*arg)
                goto  badarg;
        curre->un.buffer = stracpy(arg);
        curre->fd = (unsigned char) whichfd;
        curre->action = (unsigned char) action;
        Redircnt++;
        return  OPTRESULT_ARG_OK;

 badarg:
        arg_errnum = $E{Bad redirection};
        return  OPTRESULT_ERROR;
}

EOPTION(o_queuehost)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        Procparchanges |= OF_HOST_CHANGE; /* Won't bother with Anychanges - only in r?btr */

        if  (strcmp(arg, "-") == 0)  {
                Out_host = 0L;
                return  OPTRESULT_ARG_OK;
        }
        if  ((Out_host = look_int_hostname(arg)) == -1)  {
                arg_errnum = $E{Unknown queue host};
                return  OPTRESULT_ERROR;
        }
        return  OPTRESULT_ARG_OK;
}

void  checksetmode(const int modenum, const ushort *plist, const ushort srcmode, USHORT *flgs)
{
        USHORT  mflags = 0;

        if  (plist)
                mflags = plist[modenum];
        switch  (Mode_set[modenum])  {
        case  MODE_NONE:
                *flgs = srcmode;
                return;
        case  MODE_ON:
                *flgs |= srcmode;
        case  MODE_SET:
                break;
        case  MODE_OFF:
                *flgs = srcmode & ~*flgs;
                break;
        }
        if  (plist  &&  (mypriv->btu_priv & BTM_UMASK) == 0  &&  *flgs != mflags)  {
                print_error($E{Cannot respecify mode});
                exit(E_USAGE);
        }
}

int  parse_mode_arg(const char *arg, const int isjob)
{
        int     isuser, isgroup, isoth, ch;
        int     mode_type = MODE_SET;
        unsigned        wmode;
        const   char    *startit;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_MODE_CHANGES;

        while  (isspace(*arg))
                arg++;

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
                        case  'K':
                                if  (isjob)  {
                                        wmode |= BTM_KILL;
                                        break;
                                }
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
                        }
                }

                if  (ch != '\0'  &&  ch != ',')
                        goto  badmode;

                if  (isuser)  {
                        Mode_set[0] = (char) mode_type;
                        Mode_arg->u_flags = (USHORT) wmode;
                }
                if  (isgroup)  {
                        Mode_set[1] = (char) mode_type;
                        Mode_arg->g_flags = (USHORT) wmode;
                }
                if  (isoth)  {
                        Mode_set[2] = (char) mode_type;
                        Mode_arg->o_flags = (USHORT) wmode;
                }

        }  while  (*arg++ == ',');

        return  OPTRESULT_ARG_OK;

 badmode:
        arg_errnum = $E{Bad mode string};
        return  OPTRESULT_ERROR;
}

EOPTION(o_mode_job)
{
        return  parse_mode_arg(arg, 1);
}

EOPTION(o_mode_var)
{
        return  parse_mode_arg(arg, 0);
}
