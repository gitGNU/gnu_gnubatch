/* btjchange.c -- main module for gbch-jchange

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
#include <sys/shm.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include "statenums.h"
#include "ipcstuff.h"
#include "shreq.h"
#include "files.h"
#include "helpalt.h"
#include "helpargs.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "btrvar.h"
#include "spitrouts.h"
#include "optflags.h"
#include "shutilmsg.h"
#include "cgifndjb.h"

#define HTIME   5               /* Forge prompt if one doesn't come */

static  char    Filename[] = __FILE__;

extern  char    **environ, **xenviron;

char    *Curr_pwd;

#define IPC_MODE        0600

ULONG           indx;
ULONG           Saveseq;

char    *sjobqueue;             /* Job queue we are setting */

int     exit_code;

uid_t   New_owner;
gid_t   New_group;

unsigned        defavoid;

extern char **remread_envir(const netid_t);
void  checksetmode(const int, const ushort *, const ushort, USHORT *);

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static void  fixcsvars()
{
        int     nn;

        /* Don't worry if none are specified */

        if  (Condcnt <= 0  &&  Asscnt <= 0)
                return;

        if  (Condasschanges & OF_COND_CHANGES)  {
                for  (nn = 0;  nn < Condcnt;  nn++)  {
                        struct  scond   *sp = &Condlist[nn];
                        JcondRef        jc = &JREQ->h.bj_conds[nn];
                        vhash_t         vp = lookupvar(sp->vd.var, sp->vd.hostid, BTM_READ, &Saveseq);

                        if  (vp < 0)  {
                                disp_str = sp->vd.var;
                                print_error($E{Unreadable variable});
                                exit(E_NOPRIV);
                        }

                        jc->bjc_compar = sp->compar;
                        jc->bjc_varind = vp;
                        jc->bjc_value = sp->value;
                        jc->bjc_iscrit = sp->vd.hostid? sp->vd.crit: 0;
                }
        }

        if  (Condasschanges & OF_ASS_CHANGES)  {
                for  (nn = 0;  nn < Asscnt;  nn++)  {
                        struct  Sass    *sp = &Asslist[nn];
                        JassRef ja = &JREQ->h.bj_asses[nn];
                        vhash_t vp = lookupvar(sp->vd.var, sp->vd.hostid, (unsigned)(sp->op == BJA_ASSIGN? BTM_WRITE: BTM_READ|BTM_WRITE), &Saveseq);

                        if  (vp < 0)  {
                                disp_str = sp->vd.var;
                                print_error($E{Unwritable variable});
                                exit(E_NOPRIV);
                        }

                        ja->bja_flags = sp->flags;
                        ja->bja_op = sp->op;
                        ja->bja_varind = vp;
                        ja->bja_con = sp->con;
                        ja->bja_iscrit = sp->vd.hostid? sp->vd.crit: 0;
                }
        }
}

#define BTJCHANGE_INLINE

OPTION(o_explain)
{
        print_error($E{btjchange explain});
        exit(0);
}

DEOPTION(o_condcrit);
DEOPTION(o_nocondcrit);
DEOPTION(o_asscrit);
DEOPTION(o_noasscrit);
DEOPTION(o_canccond);
DEOPTION(o_condition);
DEOPTION(o_cancset);
DEOPTION(o_flags);
DEOPTION(o_set);
DEOPTION(o_advterr);
DEOPTION(o_noadvterr);
DEOPTION(o_directory);
DEOPTION(o_exits);
DEOPTION(o_noexport);
DEOPTION(o_export);
DEOPTION(o_fullexport);
DEOPTION(o_interpreter);
DEOPTION(o_loadlev);
DEOPTION(o_mail);
DEOPTION(o_write);
DEOPTION(o_cancmailwrite);
DEOPTION(o_normal);
DEOPTION(o_ascanc);
DEOPTION(o_asdone);
DEOPTION(o_priority);
DEOPTION(o_deltime);
DEOPTION(o_runtime);
DEOPTION(o_whichsig);
DEOPTION(o_gracetime);
DEOPTION(o_umask);
DEOPTION(o_ulimit);
DEOPTION(o_title);
DEOPTION(o_cancargs);
DEOPTION(o_argument);
DEOPTION(o_cancio);
DEOPTION(o_io);
DEOPTION(o_queuehost);
DEOPTION(o_mode_job);
DEOPTION(o_notime);
DEOPTION(o_time);
DEOPTION(o_norepeat);
DEOPTION(o_deleteatend);
DEOPTION(o_repeat);
DEOPTION(o_avoiding);
DEOPTION(o_skip);
DEOPTION(o_hold);
DEOPTION(o_resched);
DEOPTION(o_catchup);

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

OPTION(o_jobqueue)              /* We are setting as opposed to displaying */
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_JOBQUEUE_CHANGES;
        if  (sjobqueue)
                free(sjobqueue);
        sjobqueue = (arg[0] && (arg[0] != '-' || arg[1])) ? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

OPTION(o_resetenv)
{
        Procparchanges |= OF_ENV_CHANGES;
        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        return  OPTRESULT_OK;
}

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btjchange arg explain} },
  {  'm', $A{btjchange arg mail} },
  {  'w', $A{btjchange arg write} },
  {  'h', $A{btjchange arg title} },
  {  'x', $A{btjchange arg nomess} },
  {  'i', $A{btjchange arg interp} },
  {  'p', $A{btjchange arg pri} },
  {  'l', $A{btjchange arg ll} },
  {  'N', $A{btjchange arg norm} },
  {  'C', $A{btjchange arg canc} },
  {  'H', $A{btjchange arg hold} },
  {  'S', $A{btjchange arg skip} },
  {  'R', $A{btjchange arg resched} },
  {  '9', $A{btjchange arg catchup} },
  {  'U', $A{btjchange arg notime} },
  {  'T', $A{btjchange arg time} },
  {  'Z', $A{btjchange arg cancio} },
  {  'I', $A{btjchange arg io} },
  {  'o', $A{btjchange arg norep} },
  {  'r', $A{btjchange arg repeat} },
  {  'd', $A{btjchange arg delete} },
  {  'D', $A{btjchange arg dir} },
  {  'e', $A{btjchange arg cancarg} },
  {  'a', $A{btjchange arg argument} },
  {  'E', $A{btjchange arg resetenv} },
  {  'y', $A{btjchange arg canccond} },
  {  'c', $A{btjchange arg cond} },
  {  'z', $A{btjchange arg cancset} },
  {  'f', $A{btjchange arg setflags} },
  {  's', $A{btjchange arg set} },
  {  'A', $A{btjchange arg avoid} },
  {  'M', $A{btjchange arg mode} },
  {  'u', $A{btjchange arg setu} },
  {  'g', $A{btjchange arg setg} },
  {  'P', $A{btjchange arg umask} },
  {  'L', $A{btjchange arg ulimit} },
  {  'X', $A{btjchange arg exits} },
  {  'j', $A{btjchange arg adverr} },
  {  'J', $A{btjchange arg noadverr} },
  {  'n', $A{btjchange arg loco} },
  {  'F', $A{btjchange arg export} },
  {  'G', $A{btjchange arg fullexport} },
  {  'q', $A{btjchange arg queue} },
  {  't', $A{btjchange arg deltime} },
  {  'Y', $A{btjchange arg runtime} },
  {  'W', $A{btjchange arg wsig} },
  {  '2', $A{btjchange arg grace} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,
o_mail,         o_write,        o_title,        o_cancmailwrite,
o_interpreter,  o_priority,     o_loadlev,      o_mode_job,
o_normal,       o_ascanc,       o_notime,       o_time,
o_norepeat,     o_deleteatend,  o_repeat,       o_avoiding,
o_skip,         o_hold,         o_resched,      o_catchup,
o_canccond,     o_condition,    o_cancset,      o_flags,
o_set,          o_owner,        o_group,        o_cancio,
o_io,           o_directory,    o_cancargs,     o_argument,
o_resetenv,     o_umask,        o_ulimit,       o_exits,
o_advterr,      o_noadvterr,    o_noexport,     o_export,
o_fullexport,   o_condcrit,     o_nocondcrit,   o_asscrit,
o_noasscrit,    o_jobqueue,     o_deltime,      o_runtime,
o_whichsig,     o_gracetime,    o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0, pch = '=', jn;

        fprintf(dest, "%s", name);

        if  (Procparchanges & (OF_MAIL_CHANGES|OF_WRITE_CHANGES))  {
                if  (!(JREQ->h.bj_jflags & (BJ_MAIL|BJ_WRT)))  {
                        cancont = spitoption($A{btjchange arg nomess}, $A{btjchange arg explain}, dest, pch, cancont);
                        pch = ' ';
                }
                if  (JREQ->h.bj_jflags & BJ_MAIL)  {
                        cancont = spitoption($A{btjchange arg mail}, $A{btjchange arg explain}, dest, pch, cancont);
                        pch = ' ';
                }
                if  (JREQ->h.bj_jflags & BJ_WRT)  {
                        cancont = spitoption($A{btjchange arg write}, $A{btjchange arg explain}, dest, pch, cancont);
                        pch = ' ';
                }
        }
        if  (Timechanges & OF_TIMES_CHANGES  &&  !JREQ->h.bj_times.tc_istime)  {
                cancont = spitoption($A{btjchange arg notime}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Timechanges & OF_REPEAT_CHANGES  &&  (JREQ->h.bj_times.tc_repeat == TC_DELETE || JREQ->h.bj_times.tc_repeat == TC_RETAIN))  {
                cancont = spitoption(JREQ->h.bj_times.tc_repeat == TC_DELETE? $A{btjchange arg delete}: $A{btjchange arg norep}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Timechanges & OF_NPOSS_CHANGES)  {
                static  const   short   nplookup[] =  {
                        $A{btjchange arg skip},
                        $A{btjchange arg hold},
                        $A{btjchange arg resched},
                        $A{btjchange arg catchup}
                };
                cancont = spitoption(JREQ->h.bj_times.tc_nposs > TC_CATCHUP?
                                     $A{btjchange arg hold}: nplookup[JREQ->h.bj_times.tc_nposs],
                                     $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Procparchanges & OF_ADVT_CHANGES)  {
                cancont = spitoption(JREQ->h.bj_jflags & BJ_NOADVIFERR? $A{btjchange arg noadverr}: $A{btjchange arg adverr}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Procparchanges & OF_EXPORT_CHANGES)  {
                cancont = spitoption(JREQ->h.bj_jflags & BJ_REMRUNNABLE? $A{btjchange arg fullexport}:
                                     JREQ->h.bj_jflags & BJ_EXPORT? $A{btjchange arg export}:
                                     $A{btjchange arg loco}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Procparchanges & OF_PROGRESS_CHANGES)  {
                cancont = spitoption(JREQ->h.bj_progress == BJP_NONE? $A{btjchange arg norm}: $A{btjchange arg canc}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Procparchanges & OF_IO_CLEAR)  {
                cancont = spitoption($A{btjchange arg cancio}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Procparchanges & OF_ARG_CLEAR)  {
                cancont = spitoption($A{btjchange arg cancarg}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Condasschanges & OF_COND_CHANGES)  {
                cancont = spitoption($A{btjchange arg canccond}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Condasschanges & OF_ASS_CHANGES)  {
                cancont = spitoption($A{btjchange arg cancset}, $A{btjchange arg explain}, dest, pch, cancont);
                pch = ' ';
        }
        if  (Procparchanges & OF_TITLE_CHANGES  &&  job_title  &&  job_title[0])  {
                spitoption($A{btjchange arg title}, $A{btjchange arg explain}, dest, pch, 0);
                putc(' ', dest);
                dumpstr(dest, job_title);
                pch = ' ';
        }
        if  (Procparchanges & OF_JOBQUEUE_CHANGES)  {
                spitoption($A{btjchange arg queue}, $A{btjchange arg explain}, dest, pch, 0);
                if  (sjobqueue)
                        fprintf(dest, " \'%s\'", sjobqueue);
                else
                        fputs(" -", dest);
                pch = ' ';
        }
        if  (Procparchanges & OF_INTERP_CHANGES)  {
                spitoption($A{btjchange arg interp}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %s", JREQ->h.bj_cmdinterp);
                pch = ' ';
        }
        if  (Procparchanges & OF_PRIO_CHANGES)  {
                spitoption($A{btjchange arg pri}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %d", JREQ->h.bj_pri);
                pch = ' ';
        }
        if  (Procparchanges & OF_LOADLEV_CHANGES)  {
                spitoption($A{btjchange arg ll}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %d", JREQ->h.bj_ll);
                pch = ' ';
        }
        if  (Procparchanges & OF_MODE_CHANGES)  {
                char    sep = ' ';
                spitoption($A{btjchange arg mode}, $A{btjchange arg explain}, dest, pch, 0);
                if  (Mode_set[0] != MODE_NONE)  {
                        dumpemode(dest, sep, 'U', Mode_set[0], JREQ->h.bj_mode.u_flags);
                        sep = ',';
                }
                if  (Mode_set[1] != MODE_NONE)  {
                        dumpemode(dest, sep, 'G', Mode_set[1], JREQ->h.bj_mode.g_flags);
                        sep = ',';
                }
                if  (Mode_set[2] != MODE_NONE)
                        dumpemode(dest, sep, 'O', Mode_set[2], JREQ->h.bj_mode.o_flags);
                pch = ' ';
        }
        if  (Condasschanges & OF_COND_CHANGES)  {
                for  (jn = 0;  jn < Condcnt;  jn++)  {
                        if  (Condlist[jn].vd.hostid)  {
                                spitoption(Condlist[jn].vd.crit & CCRIT_NORUN? $A{btjchange arg condcrit}: $A{btjchange arg nocondcrit},
                                          $A{btjchange arg explain}, dest, ' ', 0);
                                spitoption($A{btjchange arg cond}, $A{btjchange arg explain}, dest, pch, 0);
                                fprintf(dest, " %s:%s%s", look_host(Condlist[jn].vd.hostid),
                                               Condlist[jn].vd.var, condname[Condlist[jn].compar-C_EQ]);
                        }
                        else  {
                                spitoption($A{btjchange arg cond}, $A{btjchange arg explain}, dest, pch, 0);
                                fprintf(dest, " %s%s", Condlist[jn].vd.var, condname[Condlist[jn].compar-C_EQ]);
                        }
                        dumpcon(dest, &Condlist[jn].value);
                        pch = ' ';
                }
        }
        if  (Condasschanges & OF_ASS_CHANGES)  {
                for  (jn = 0;  jn < Asscnt;  jn++)  {
                        struct  Sass    *as = &Asslist[jn];
                        if  (as->vd.hostid)
                                spitoption(as->vd.crit & ACRIT_NORUN? $A{btjchange arg asscrit}: $A{btjchange arg noasscrit},
                                                  $A{btjchange arg explain}, dest, ' ', 0);
                        if  (as->op >= BJA_SEXIT)  {
                                spitoption($A{btjchange arg set}, $A{btjchange arg explain}, dest, pch, 0);
                                fprintf(dest, " %s=%s", host_prefix_str(as->vd.hostid, as->vd.var), as->op == BJA_SEXIT? exitcodename: signalname);
                        }
                        else  {
                                if  (as->flags)  {
                                        spitoption($A{btjchange arg setflags}, $A{btjchange arg explain}, dest, pch, 0);
                                        if  (as->flags & BJA_START)
                                                putc('S', dest);
                                        if  (as->flags & BJA_REVERSE)
                                                putc('R', dest);
                                        if  (as->flags & BJA_OK)
                                                putc('N', dest);
                                        if  (as->flags & BJA_ERROR)
                                                putc('E', dest);
                                        if  (as->flags & BJA_ABORT)
                                                putc('A', dest);
                                        if  (as->flags & BJA_CANCEL)
                                                putc('C', dest);
                                }
                                spitoption($A{btjchange arg set}, $A{btjchange arg explain}, dest, pch, 0);
                                fprintf(dest, " %s%s", host_prefix_str(as->vd.hostid, as->vd.var), assname[as->op-BJA_ASSIGN]);
                                dumpcon(dest, &as->con);
                        }
                        pch = ' ';
                }
        }
        if  (Procparchanges & OF_IO_CHANGES)  {
                for  (jn = 0;  jn < Redircnt;  jn++)  {
                        MredirRef       rp = &Redirs[jn];
                        spitoption($A{btjchange arg io}, $A{btjchange arg explain}, dest, pch, 0);
                        pch = ' ';
                        spit_redir(dest, rp->fd, rp->action, rp->un.arg, rp->un.buffer);
                }
        }
        if  (Procparchanges & OF_ARG_CHANGES)  {
                for  (jn = 0;  jn < Argcnt;  jn++)  {
                        spitoption($A{btjchange arg argument}, $A{btjchange arg explain}, dest, pch, 0);
                        putc(' ', dest);
                        dumpstr(dest, Args[jn]);
                }
                pch = ' ';
        }
        if  (Procparchanges & OF_OWNER_CHANGE)  {
                spitoption($A{btjchange arg setu}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %s", prin_uname(New_owner));
                pch = ' ';
        }
        if  (Procparchanges & OF_GROUP_CHANGE)  {
                spitoption($A{btjchange arg setg}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %s", prin_gname(New_group));
                pch = ' ';
        }
        if  (Procparchanges & OF_UMASK_CHANGES)  {
                spitoption($A{btjchange arg umask}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %.3o", JREQ->h.bj_umask);
                pch = ' ';
        }
        if  (Procparchanges & OF_ULIMIT_CHANGES)  {
                spitoption($A{btjchange arg ulimit}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %ld", (long) JREQ->h.bj_ulimit);
                pch = ' ';
        }
        if  (Procparchanges & OF_EXIT_CHANGES)  {
                spitoption($A{btjchange arg exits}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " N%d:%d", JREQ->h.bj_exits.nlower, JREQ->h.bj_exits.nupper);
                pch = ' ';
                spitoption($A{btjchange arg exits}, $A{btjchange arg explain}, dest, ' ', 0);
                fprintf(dest, " E%d:%d", JREQ->h.bj_exits.elower, JREQ->h.bj_exits.eupper);
        }
        if  (Procparchanges & OF_DELTIME_SET) {
                spitoption($A{btjchange arg deltime}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %u", JREQ->h.bj_deltime);
                pch = ' ';
        }
        if  (Procparchanges & OF_RUNTIME_SET) {
                int     hrs, mns, secs;
                spitoption($A{btjchange arg runtime}, $A{btjchange arg explain}, dest, pch, 0);
                putc(' ', dest);
                hrs = JREQ->h.bj_runtime / 3600L;
                mns = (JREQ->h.bj_runtime % 3600L) / 60;
                secs = JREQ->h.bj_runtime % 60;
                if  (hrs > 0)
                        fprintf(dest, "%d:", hrs);
                if  (hrs > 0 || mns > 0)
                        fprintf(dest, "%.2d:", mns);
                fprintf(dest, "%.2d", secs);
                pch = ' ';
        }
        if  (Procparchanges & OF_WHICHSIG_SET)  {
                spitoption($A{btjchange arg wsig}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %u", JREQ->h.bj_autoksig);
                pch = ' ';
        }
        if  (Procparchanges & OF_GRACETIME_SET) {
                int     mns, secs;
                spitoption($A{btjchange arg grace}, $A{btjchange arg explain}, dest, pch, 0);
                putc(' ', dest);
                mns = JREQ->h.bj_runon / 60;
                secs = JREQ->h.bj_runon % 60;
                if  (mns > 0)
                        fprintf(dest, "%d:", mns);
                fprintf(dest, "%.2d", secs);
                pch = ' ';
        }
        if  (Procparchanges & OF_DIR_CHANGES)  {
                spitoption($A{btjchange arg dir}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %s", job_cwd);
                pch = ' ';
        }
        if  (Timechanges & OF_TIMES_CHANGES  &&  JREQ->h.bj_times.tc_istime)  {
                struct  tm      *t = localtime(&JREQ->h.bj_times.tc_nexttime);
                spitoption($A{btjchange arg time}, $A{btjchange arg explain}, dest, pch, 0);
                if  (Timechanges & OF_DATESET)
                        fprintf(dest, " %.2d/%.2d/%.2d,%.2d:%.2d",
                                       t->tm_year % 100, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
                else
                        fprintf(dest, " %.2d:%.2d", t->tm_hour, t->tm_min);
                pch = ' ';
        }
        if  (Timechanges & OF_REPEAT_CHANGES  &&  JREQ->h.bj_times.tc_repeat >= TC_MINUTES)  {
                spitoption($A{btjchange arg repeat}, $A{btjchange arg explain}, dest, pch, 0);
                fprintf(dest, " %s:%ld", disp_alt((int) (JREQ->h.bj_times.tc_repeat - TC_MINUTES), repunit), (long) JREQ->h.bj_times.tc_rate);
                pch = ' ';
                if  (Timechanges & OF_MDSET)
                        fprintf(dest, ":%d", JREQ->h.bj_times.tc_mday);
        }
        if  (Timechanges & OF_AVOID_CHANGES)  {
                spitoption($A{btjchange arg avoid}, $A{btjchange arg explain}, dest, pch, 0);
                fputs(" -", dest);
                for  (jn = 0;  jn < TC_NDAYS;  jn++)
                        if  (JREQ->h.bj_times.tc_nvaldays & (1 << jn))
                                fprintf(dest, ",%s", disp_alt(jn, days_abbrev));
        }
        putc('\n', dest);
}

/* This is the main processing routine.  */

void  process(char **joblist)
{
        char            *jobc, **jobp;
        char            **remenv = (char **) 0;
        netid_t         lastnetid = -1L;
        char            *tp, *dir = (char *) 0, **ap = (char **) 0;
        char            *allocstr = (char *) 0;
        unsigned        nr = 0, na = 0, Envcount = 0;
        MredirRef       rp = (MredirRef) 0;
        MenvirRef       envp = (MenvirRef) 0;
        USHORT          Usave = JREQ->h.bj_mode.u_flags,
                        Gsave = JREQ->h.bj_mode.g_flags,
                        Osave = JREQ->h.bj_mode.o_flags;
        Mredir          Gredirs[MAXJREDIRS];
        char            *Gargs[MAXJARGS];
        Menvir          Genv[MAXJENVIR];

        if  (Procparchanges & OF_DIR_CHANGES)
                dir = job_cwd;
        if  (Procparchanges & OF_ARG_CHANGES)  {
                ap = Gargs;
                if  (Procparchanges & OF_ARG_CLEAR)  {
                        na = Argcnt;
                        BLOCK_COPY(Gargs, Args, sizeof(Gargs));
                }
        }
        if  (Procparchanges & OF_IO_CHANGES)  {
                rp = Gredirs;
                if  (Procparchanges & OF_IO_CLEAR)  {
                        nr = Redircnt;
                        BLOCK_COPY(Gredirs, Redirs, sizeof(Gredirs));
                }
        }

        for  (jobp = joblist;  (jobc = *jobp);  jobp++)  {
                int                     retc;
                struct  jobswanted      jw;
                CBtjobRef               jp;

                if  ((retc = decode_jnum(jobc, &jw)) != 0)  {
                        print_error(retc);
                        exit_code = E_USAGE;
                        continue;
                }

                if  (!find_job(&jw))  {
                        disp_str = jobc;
                        print_error($E{Cannot find job});
                        exit_code = E_NOJOB;
                        continue;
                }

                jp = jw.jp;

                /* Fix the bits we can change with each kind of change */

                JREQ->h.bj_job = jp->h.bj_job;
                JREQ->h.bj_hostid = jp->h.bj_hostid;
                JREQ->h.bj_slotno = jp->h.bj_slotno;

                if  (Procparchanges & OF_MODE_CHANGES)  {
                        checksetmode(0, mypriv->btu_jflags, jp->h.bj_mode.u_flags, &JREQ->h.bj_mode.u_flags);
                        checksetmode(1, mypriv->btu_jflags, jp->h.bj_mode.g_flags, &JREQ->h.bj_mode.g_flags);
                        checksetmode(2, mypriv->btu_jflags, jp->h.bj_mode.o_flags, &JREQ->h.bj_mode.o_flags);
                        if  ((retc = wjxfermsg(J_CHMOD, indx)) != 0)  {
                                disp_str = jobc;
                                print_error(retc);
                                exit_code = E_NOPRIV;
                                continue;
                        }
                        retc = readreply(); /* Wait for reply 'cous JREQ is in xferbuf */
                        JREQ->h.bj_mode.u_flags = Usave;
                        JREQ->h.bj_mode.g_flags = Gsave;
                        JREQ->h.bj_mode.o_flags = Osave;
                        if  (retc != J_OK)  {
                                disp_str = jobc;
                                print_error(dojerror(retc, JREQ));
                                exit_code = E_FALSE;
                                continue;
                        }
                }

                if  (Procparchanges & OF_OWNER_CHANGE)  {
                        wjimsg_param(J_CHOWN, New_owner, jp);
                        if  ((retc = readreply()) != J_OK)  {
                                disp_str = jobc;
                                print_error(dojerror(retc, JREQ));
                                exit_code = E_FALSE;
                                continue;
                        }
                }

                if  (Procparchanges & OF_GROUP_CHANGE)  {
                        wjimsg_param(J_CHGRP, New_group, jp);
                        if  ((retc = readreply()) != J_OK)  {
                                disp_str = jobc;
                                print_error(dojerror(retc, JREQ));
                                exit_code = E_FALSE;
                                continue;
                        }
                }

                if  (!(Anychanges & OF_ANY_DOING_CHANGE))
                        continue;

                if  (!(Procparchanges & OF_ADVT_CHANGES))  {
                        JREQ->h.bj_jflags &= ~BJ_NOADVIFERR;
                        JREQ->h.bj_jflags |= jp->h.bj_jflags & BJ_NOADVIFERR;
                }
                if  (!(Procparchanges & OF_PROGRESS_CHANGES))
                        JREQ->h.bj_progress = jp->h.bj_progress;
                if  (!(Procparchanges & OF_PRIO_CHANGES))
                        JREQ->h.bj_pri = jp->h.bj_pri;
                if  (!(Procparchanges & OF_INTERP_CHANGES))
                        strcpy(JREQ->h.bj_cmdinterp, jp->h.bj_cmdinterp);
                if  (!(Procparchanges & (OF_LOADLEV_CHANGES|OF_INTERP_CHANGES)))
                        JREQ->h.bj_ll = jp->h.bj_ll;
                if  (!(Procparchanges & OF_WRITE_CHANGES))  {
                        JREQ->h.bj_jflags &= ~BJ_WRT;
                        JREQ->h.bj_jflags |= jp->h.bj_jflags & BJ_WRT;
                }
                if  (!(Procparchanges & OF_MAIL_CHANGES))  {
                        JREQ->h.bj_jflags &= ~BJ_MAIL;
                        JREQ->h.bj_jflags |= jp->h.bj_jflags & BJ_MAIL;
                }
                if  (!(Condasschanges & OF_COND_CHANGES))
                        BLOCK_COPY(JREQ->h.bj_conds, jp->h.bj_conds, sizeof(JREQ->h.bj_conds));
                if  (!(Condasschanges & OF_ASS_CHANGES))
                        BLOCK_COPY(JREQ->h.bj_asses, jp->h.bj_asses, sizeof(JREQ->h.bj_asses));
                if  (!(Timechanges & OF_TIMES_CHANGES))  {
                        JREQ->h.bj_times.tc_nexttime = jp->h.bj_times.tc_nexttime;
                        JREQ->h.bj_times.tc_istime = jp->h.bj_times.tc_istime;
                }
                if  (!(Timechanges & OF_AVOID_CHANGES))
                        JREQ->h.bj_times.tc_nvaldays = jp->h.bj_times.tc_nvaldays;
                if  (!(Timechanges & OF_REPEAT_CHANGES))  {
                        JREQ->h.bj_times.tc_repeat = jp->h.bj_times.tc_repeat;
                        JREQ->h.bj_times.tc_rate = jp->h.bj_times.tc_rate;
                        JREQ->h.bj_times.tc_mday = jp->h.bj_times.tc_mday;
                }
                else  {
                        int     ret = repmnthfix(&JREQ->h.bj_times);
                        if  (ret)
                                print_error(ret);
                }
                if  (!(Timechanges & OF_NPOSS_CHANGES))
                        JREQ->h.bj_times.tc_nposs = jp->h.bj_times.tc_nposs;

                if  (!(Procparchanges & OF_UMASK_CHANGES))
                        JREQ->h.bj_umask = jp->h.bj_umask;
                if  (!(Procparchanges & OF_ULIMIT_CHANGES))
                        JREQ->h.bj_ulimit = jp->h.bj_ulimit;
                if  (!(Procparchanges & OF_EXIT_CHANGES))
                        JREQ->h.bj_exits = jp->h.bj_exits;
                if  (!(Procparchanges & OF_EXPORT_CHANGES))  {
                        JREQ->h.bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);
                        JREQ->h.bj_jflags |= jp->h.bj_jflags & (BJ_EXPORT|BJ_REMRUNNABLE);
                }
                if  (!(Procparchanges & OF_DELTIME_SET))
                        JREQ->h.bj_deltime = jp->h.bj_deltime;
                if  (!(Procparchanges & OF_RUNTIME_SET))
                        JREQ->h.bj_runtime = jp->h.bj_runtime;
                if  (!(Procparchanges & OF_WHICHSIG_SET))
                        JREQ->h.bj_autoksig = jp->h.bj_autoksig;
                if  (!(Procparchanges & OF_GRACETIME_SET))
                        JREQ->h.bj_runon = jp->h.bj_runon;
                if  ((Procparchanges & (OF_IO_CHANGES|OF_IO_CLEAR)) == OF_IO_CHANGES)  {
                        unsigned  cnt, rcnt;
                        nr = jp->h.bj_nredirs + Redircnt;
                        if  (nr >= MAXJREDIRS)  {
                                disp_arg[0] = nr;
                                disp_arg[1] = jp->h.bj_nredirs;
                                disp_arg[2] = MAXJREDIRS;
                                disp_arg[3] = jp->h.bj_job;
                                disp_str = title_of(jp);
                                print_error($E{Too many redirections});
                                exit_code = E_LIMIT;
                                continue;
                        }

                        /* Tack new ones onto end of existing */

                        for  (cnt = 0;  cnt < jp->h.bj_nredirs;  cnt++)  {
                                RedirRef        orp = REDIR_OF(jp, cnt);
                                Gredirs[cnt].fd = orp->fd;
                                if  ((Gredirs[cnt].action = orp->action) >= RD_ACT_CLOSE)
                                        Gredirs[cnt].un.arg = orp->arg;
                                else
                                        Gredirs[cnt].un.buffer = (char *) &jp->bj_space[orp->arg];
                        }
                        rcnt = cnt;
                        for  (cnt = 0;  cnt < Redircnt;  rcnt++, cnt++)
                                Gredirs[rcnt] = Redirs[cnt];
                }

                /* Dittoid for arguments */

                if  ((Procparchanges & (OF_ARG_CHANGES|OF_ARG_CLEAR)) == OF_ARG_CHANGES)  {
                        unsigned  cnt, rcnt;
                        na = jp->h.bj_nargs + Argcnt;
                        if  (na >= MAXJARGS)  {
                                disp_arg[0] = na;
                                disp_arg[1] = jp->h.bj_nargs;
                                disp_arg[2] = MAXJARGS;
                                disp_arg[3] = jp->h.bj_job;
                                disp_str = title_of(jp);
                                print_error($E{Too many arguments});
                                exit_code = E_LIMIT;
                                continue;
                        }

                        /* Tack new ones onto end of existing */

                        for  (cnt = 0;  cnt < jp->h.bj_nargs;  cnt++)
                                Gargs[cnt] = (char *) ARG_OF(jp, cnt);
                        rcnt = cnt;
                        for  (cnt = 0;  cnt < Argcnt;  rcnt++, cnt++)
                                Gargs[rcnt] = Args[cnt];
                }

                if  (Procparchanges & OF_ENV_CHANGES  &&  lastnetid != jp->h.bj_hostid)  {
                        char    **ep, **sq_env;

                        /* Free the environment and name copies we
                           created last time.  */

                        if  (remenv)  {
                                for  (ep = remenv;  *ep;  ep++)
                                        free(*ep);
                                free((char *) remenv);
                                remenv = (char **) 0;
                        }
                        while  (Envcount != 0)
                                free(Genv[--Envcount].e_name);

                        /* Envcount now zero of course */
                        envp = Genv;

                        /* For remote jobs, get remote machine's environment.
                           In any case "squash out" static stuff.  */

                        if  (jp->h.bj_hostid)  {
                                remenv = remread_envir(jp->h.bj_hostid);
                                sq_env = squash_envir(remenv, environ);
                        }
                        else
                                sq_env = squash_envir(xenviron, environ);

                        for  (ep = sq_env;  *ep;  ep++)  {
                                char    *eqp, *ncopy;
                                if  (!(eqp = strchr(*ep, '='))) /* Don't understand no = */
                                        continue;
                                if  (Envcount < MAXJENVIR)  {
                                        unsigned  lng = eqp - *ep;
                                        if  ((ncopy = malloc(lng + 1)) == (char *) 0)
                                                ABORT_NOMEM;
                                        BLOCK_COPY(ncopy, *ep, lng);
                                        ncopy[lng] = '\0';
                                        Genv[Envcount].e_name = ncopy;
                                        Genv[Envcount].e_value = eqp + 1;
                                }
                                Envcount++;
                        }
                        if  (Envcount > MAXJENVIR)  {
                                disp_arg[0] = Envcount;
                                disp_arg[1] = MAXJENVIR;
                                print_error($E{Too large environment});
                                Envcount = MAXJENVIR;
                        }

                        /* If 'squash_envir' allocated a new list, delete it */

                        if  (sq_env != environ)
                                free((char *) sq_env);
                }

                tp = (char *) 0;
                if  (Procparchanges & OF_TITLE_CHANGES)  {
                        if  (Procparchanges & OF_JOBQUEUE_CHANGES)  {
                                if  (sjobqueue)  {
                                        allocstr = malloc((unsigned) (strlen(sjobqueue) + strlen(job_title) + 2));
                                        if  (!allocstr)
                                                ABORT_NOMEM;
                                        sprintf(allocstr, "%s:%s", sjobqueue, job_title);
                                        tp = allocstr;
                                }
                                else
                                        tp = job_title;
                        }
                        else  {
                                const   char    *etitle = title_of(jp), *colp;
                                if  ((colp = strchr(etitle, ':')))  {
                                        allocstr = malloc((unsigned) ((colp - etitle) + strlen(job_title) + 2));
                                        if  (!allocstr)
                                                ABORT_NOMEM;
                                        sprintf(allocstr, "%.*s:%s", (int) (colp - etitle), etitle, job_title);
                                        tp = allocstr;
                                }
                                else
                                        tp = job_title;
                        }
                }
                else  if  (Procparchanges & OF_JOBQUEUE_CHANGES)  {
                        const   char    *etitle = title_of(jp);
                        const   char    *colp;
                        if  (sjobqueue)  {
                                if  ((colp = strchr(etitle, ':')))  {
                                        allocstr = malloc((unsigned) (strlen(colp) + strlen(sjobqueue) + 1));
                                        if  (!allocstr)
                                                ABORT_NOMEM;
                                        sprintf(allocstr, "%s%s", sjobqueue, colp);
                                }
                                else  {
                                        allocstr = malloc((unsigned) (strlen(sjobqueue) + strlen(etitle) + 2));
                                        if  (!allocstr)
                                                ABORT_NOMEM;
                                        sprintf(allocstr, "%s:%s", sjobqueue, etitle);
                                }
                                tp = allocstr;
                        }
                        else  if  ((colp = strchr(etitle, ':')))
                                tp = allocstr = stracpy(colp + 1);
                }
                if  (!repackjob(JREQ, jp, dir, tp, nr, Envcount, na, rp, envp, ap))  {
                        disp_arg[3] = jp->h.bj_job;
                        disp_str = title_of(jp);
                        print_error($E{Too many job strings});
                        exit_code = E_LIMIT;
                        continue;
                }
                if  (allocstr)  {
                        free(allocstr);
                        allocstr = (char *) 0;
                }

                /* Remember the host id of the last job we looked at
                   successfully We do this to save regenerating
                   the environment each time.  */

                lastnetid = jp->h.bj_hostid;
                wjxfermsg(J_CHANGE, indx);
                if  ((retc = readreply()) != J_OK)  {
                        disp_str = jobc;
                        print_error(dojerror(retc, JREQ));
                }
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        unsigned        cnt;
        int             ret;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        tzset();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }

#ifndef USING_FLOCK
        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif

        openjfile(0, 0);
        openvfile(0, 0);
        rjobfile(1);
        rvarfile(1);
        initxbuffer(0);
        JREQ = &Xbuffer->Ring[indx = getxbuf()];
        Mode_arg = &JREQ->h.bj_mode;
        BLOCK_ZERO(JREQ, sizeof(Btjob));
        JREQ->h.bj_exits.elower = 1;
        JREQ->h.bj_exits.eupper = 255;
        JREQ->h.bj_jflags = BJ_EXPORT; /* The default case */

        /* Get user control structure for current real user id.  */

        mypriv = getbtuser(Realuid);
        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
        SWAP_TO(Realuid);

        /* Before reading arguments, read in days to avoid */

        for  (cnt = 0;  cnt < TC_NDAYS;  cnt++)
                if  (helpnstate((int) ($N{Base for days to avoid}+cnt)) > 0)
                        defavoid |= 1 << cnt;

        if  ((defavoid & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)  {
                print_error($E{Avoiding all in defaults});
                exit(E_SETUP);
        }

        JREQ->h.bj_times.tc_nvaldays = (USHORT) defavoid;

        if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
                disp_arg[9] = $Q{Repeat unit abbrev};
                print_error($E{Missing alternative code});
        }
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                print_error($E{Missing alternative code});
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        argv = optprocess(argv, Adefs, optprocs, $A{btjchange arg explain}, $A{btjchange arg freeze home}, 0);
        SWAP_TO(Daemuid);

        if  (!(Anychanges & OF_ANY_DOING_SOMETHING))  {
                print_error($E{btjchange no changes});
                exit(E_USAGE);
        }

        /* Here are a few checks which we don't want to do halfway
           through looking at the jobs.  */

        if  (Procparchanges & OF_PRIO_CHANGES  &&
             (JREQ->h.bj_pri < mypriv->btu_minp || JREQ->h.bj_pri > mypriv->btu_maxp))  {
                disp_arg[0] = JREQ->h.bj_pri;
                disp_arg[1] = mypriv->btu_minp;
                disp_arg[2] = mypriv->btu_maxp;
                print_error(mypriv->btu_minp > mypriv->btu_maxp?
                            $E{Cannot use GNUbatch}: $E{Invalid priority});
                exit(E_USAGE);
        }

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  (argv[0] == (char *) 0)  {
                print_error($E{No jobs specified});
                exit(E_USAGE);
        }
        fixcsvars();
        if  (Procparchanges & OF_ENV_CHANGES)
                init_xenv();
        process(&argv[0]);
        return  exit_code;
}
