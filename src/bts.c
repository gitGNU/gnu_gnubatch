/* bts.c -- main program for gbch-s

   Copyright 2013 Free Software Foundation, Inc.

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
#include <sys/stat.h>
#include <errno.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpalt.h"
#include "helpargs.h"
#include "cfile.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "jvuprocs.h"
#include "btrvar.h"
#include "spitrouts.h"
#include "optflags.h"
#include "shutilmsg.h"
#include "xbnetq.h"
#include "xmlldsv.h"
#include "remsubops.h"

#ifdef	HAVE_LIBXML2
#define IPC_MODE        0

#ifndef ROOTID
#define ROOTID  0
#endif

#define C_MASK          0377    /* Umask value */
#define NAMESIZE        14      /* Padding for temp file */

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

Shipc           Oreq;
extern  long    mymtype;

static  char    Filename[] = __FILE__;

char    *spdir, *tmpfl;

char    *Curr_pwd;

char    *realuname, *realgname;

struct  pending_job  {
        char    *argfile_name;                          /* Name of argument file */
        char    *spoolfile_name;                        /* Name of spool file in case we have to delete it */
        char    *scriptstring;                          /* Script as a string */
        char    *groupname;                             /* Group name used */
        netid_t out_host;                               /* Where it goes to */
        Btjob   jobdescr;                               /* Actual job descriptor */
        char    subok;                                  /* Submitted OK */
        int     retcode;                                /* Error code */
        int     verbose;                                /* Print message */
};

struct  pending_job  *joblist;
unsigned        num_pending_jobs;

/* Remember the permissions a user has on each remote host encountered */

struct  remperms  {
        netid_t hostid;                                 /* Host id (never local) */
        Btuser  perms;                                  /* Permissions */
        char    *groupname;                             /* Primary group at other end */
};

struct  remperms  *rplist;
unsigned        num_remperms;

int     arg_verbose = -1,               /* -1 as file, 0 quiet, 1 noisy */
        arg_state = -1;                 /* -1 as file, otherwise progress code to force */

netid_t Out_host = -1;                  /* -1 as file, 0 local host, otherwise host IP */

static  int     tcpportnum = -1;

OPTION(o_explain)
{
        print_error($E{bts explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_verbose)
{
        arg_verbose = 1;
        return  OPTRESULT_OK;
}

OPTION(o_quiet)
{
        arg_verbose = 0;
        return  OPTRESULT_OK;
}

OPTION(o_verbasfile)
{
        arg_verbose = -1;
        return  OPTRESULT_OK;
}

OPTION(o_queuehost)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (strcmp(arg, "-") == 0)  {
                Out_host = 0;
                return  OPTRESULT_ARG_OK;
        }
        if  ((Out_host = look_int_hostname(arg)) == -1)  {
                print_error($E{Unknown queue host});
                return  OPTRESULT_ERROR;
        }
        return  OPTRESULT_ARG_OK;
}

OPTION(o_hostasfile)
{
        Out_host = -1;
        return  OPTRESULT_OK;
}

OPTION(o_cancelled)
{
        arg_state = BJP_CANCELLED;
        return  OPTRESULT_OK;
}

OPTION(o_normal)
{
        arg_state = BJP_NONE;
        return  OPTRESULT_OK;
}

OPTION(o_stateasfile)
{
        arg_state = -1;
        return  OPTRESULT_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

const   Argdefault      Adefs[] = {
  { '?', $A{bts arg explain} },
  { 'v', $A{bts arg verbose} },
  { 'q', $A{bts arg quiet} },
  { 'f', $A{bts arg verbose as file} },
  { 'Q', $A{bts arg queuehost} },
  { 'F', $A{bts arg host as file} },
  { 'C', $A{bts arg cancelled} },
  { 'N', $A{bts arg normal} },
  { 'S', $A{bts arg state as file} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_verbose,      o_quiet,        o_verbasfile,
o_queuehost,    o_hostasfile,   o_cancelled,    o_normal,
o_stateasfile,
o_freezecd,     o_freezehd
};

#endif  /* HAVE_LIBXML2 */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

#ifdef	HAVE_LIBXML2

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0, verb = $A{bts arg verbose as file}, prog = $A{bts arg state as file};

        if  (arg_verbose >= 0)
                verb = arg_verbose > 0? $A{bts arg verbose}: $A{bts arg quiet};
        if  (arg_state >= 0)
                prog = arg_state == BJP_NONE? $A{bts arg normal}: $A{bts arg cancelled};
        fprintf(dest, "%s", name);
        cancont = spitoption(verb, $A{bts arg explain}, dest, '=', 0);
        cancont = spitoption(prog, $A{bts arg explain}, dest, '-', cancont);
        if  (Out_host == -1)
                spitoption($A{bts arg host as file}, $A{bts arg explain}, dest, '-', cancont);
        else  {
                spitoption($A{bts arg queuehost}, $A{bts arg explain}, dest, '-', 0);
                fprintf(dest, " %s\n", Out_host? look_host(Out_host): "-");
        }
}

static  void    removefiles()
{
        int     cnt;

        SWAP_TO(Daemuid);

        for  (cnt = 0;  cnt < num_pending_jobs;  cnt++)  {
                struct pending_job  *pj = &joblist[cnt];
                if  (pj->spoolfile_name  &&  !pj->subok)
                        Ignored_error = unlink(pj->spoolfile_name);
        }
}

/* On a signal, remove file.  */

RETSIGTYPE  catchit(int n)
{
        removefiles();
        exit(E_SIGNAL);
}

/* We set up to keep ignoring the signals we are already ignoring and
   trap the ones we aren't.  */

static  char    sigstocatch[] = { SIGINT, SIGQUIT, SIGTERM, SIGHUP };
static  char    sig_ignd[sizeof(sigstocatch)];  /*  Ignore these indices */

/* If we have got better signal handling, use it. What a hassle!!!!!  */

void  catchsigs()
{
        int     i;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;

        z.sighandler_el = catchit;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;

        for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
                struct  sigstruct_name  oldsig;
                sigact_routine(sigstocatch[i], &z, &oldsig);
                if  (oldsig.sighandler_el == SIG_IGN)  {
                        sigact_routine(sigstocatch[i], &oldsig, (struct sigstruct_name *) 0);
                        sig_ignd[i] = 1;
                }
        }
#else
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (signal(sigstocatch[i], catchit) == SIG_IGN)  {
                        signal(sigstocatch[i], SIG_IGN);
                        sig_ignd[i] = 1;
                }
        }
#endif
}

static void  holdsigs(const int block)
{
        int     i;
#ifdef  HAVE_SIGACTION
        sigset_t        sset;
        int     which = block? SIG_BLOCK: SIG_UNBLOCK;
        sigemptyset(&sset);
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                sigaddset(&sset, sigstocatch[i]);
        sigprocmask(which, &sset, (sigset_t *) 0);

#elif   defined(STRUCT_SIG)

        int     msk = 0;
        if  (block)
                for  (i = 0;  i < sizeof(sigstocatch);  i++)
                        msk |= sigmask(sigstocatch[i]);
        sigsetmask(msk);

#elif   defined(HAVE_SIGSET)
        if  (block)  {
                for  (i = 0;  i < sizeof(sigstocatch);  i++)
                        if  (!sig_ignd[i])
                                sighold(sigstocatch[i]);
        }
        else  for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (!sig_ignd[i])
                        sigrelse(sigstocatch[i]);
#else
        RETSIGTYPE  s = block? SIG_IGN: catchit;
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (!sig_ignd[i])
                        signal(sigstocatch[i], s);
#endif
}

/* Set up all the scheduler parameters */

static  void    init_batchfiles()
{
        int     ret;
        ULONG   indx;

        /* Get user's privileges to check that he can create a job */

        mypriv = getbtuser(Realuid);

        if  ((mypriv->btu_priv & BTM_CREATE) == 0)  {
                print_error($E{No create perm});
                exit(E_NOPRIV);
        }

        /* Set up file names buffer down from spool directory as we don't want to change directory  */

        spdir = envprocess(SPDIR);
        if  ((tmpfl = malloc((unsigned)(strlen(spdir) + 2 + NAMESIZE))) == (char *) 0)
                ABORT_NOMEM;

        /* Get CI list */

        if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }

        /* See if scheduler is running. */

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

        /* Set up parameters and buffer for scheduler requests */

        initxbuffer(0);
        JREQ = &Xbuffer->Ring[indx = getxbuf()];

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = J_CREATE;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        Oreq.sh_un.sh_jobindex = indx;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
}

/* Discover remote machine's version of user permissions.
   This is where we discover the machine isn't there */

static  int     getremperms(const netid_t np, BtuserRef *res, char **gname)
{
        unsigned  cnt, ret;
        int     udpsock;
        struct  sockaddr_in  saddr;
        struct   ni_jobhdr  enq;
        struct  ua_reply  resp;
        struct  remperms  *rpp;

        /* Simple search as we're unlikely to have too many */

        for  (cnt = 0;  cnt < num_remperms;  cnt++)
                if  (rplist[cnt].hostid == np)  {
                        *res = &rplist[cnt].perms;
                        *gname = rplist[cnt].groupname;
                        return  0;
                }

        /* Get permissions from server */

        if  ((ret = remsub_initsock(&udpsock, np, &saddr)) != 0)
                return  ret;

        BLOCK_ZERO(&enq, sizeof(enq));
        enq.code = CL_SV_UENQUIRY;
        enq.joblength = htons(sizeof(enq));
        strncpy(enq.uname, realuname, UIDSIZE);
        ret = remsub_udp_enquire(udpsock, &saddr, (char *) &enq, sizeof(enq), (char *) &resp, sizeof(resp));
        close(udpsock);
        if  (ret != 0)
                return  ret;

        num_remperms++;
        if  (rplist)
                rplist = (struct remperms *) realloc((char *) rplist, num_remperms * sizeof(struct remperms));
        else
                rplist = (struct remperms *) malloc(num_remperms * sizeof(struct remperms));
        if  (!rplist)
                ABORT_NOMEM;

        rpp = &rplist[num_remperms-1];
        rpp->hostid = np;
        remsub_unpack_btuser(&rpp->perms, &resp.ua_perm);
        *gname = rpp->groupname = stracpy(resp.ua_gname);
        *res = &rpp->perms;
        return  0;
}

static  int     setup_job(char *jobfile, const int ind)
{
        struct  pending_job  *pj = &joblist[ind];
        BtjobRef        jp  = &pj->jobdescr;
        BtuserRef       mpriv = mypriv;
        char            *scriptstring;
        unsigned        scriptlen = 0;
        int             ret, cinum, fid;

        /* Set up file name argument for messages */

        disp_str = pj->argfile_name = jobfile;

        /* Load the XML file */

        if  ((ret = load_job_xml(jobfile, jp, &scriptstring, &pj->verbose)) != 0)  {
                switch  (ret)  {
                case  XML_INVALID_FORMAT_FILE:
                        ret = $E{Bts invalid format file};
                        break;
                case  XML_INVALID_CONDS:
                        ret = $E{Bts invalid conds};
                        break;
                case  XML_INVALID_ASSES:
                        ret = $E{Bts invalid asses};
                        break;
                case  XML_TOOMANYSTRINGS:
                        ret = $E{Bts too many strings};
                        break;
                default:
                        ret = $E{Bts unknown error};
                        break;
                }
                pj->retcode = ret;
                return  ret;
        }

        if  (scriptstring)
                scriptlen = strlen(scriptstring);

        /* Force on things if we're doing such */

        if  (arg_verbose >= 0)
                pj->verbose = arg_verbose;
        if  (arg_state >= 0)
                jp->h.bj_progress = arg_state;
        if  (Out_host != -1)
                jp->h.bj_hostid = Out_host;

        /* Now get remote permissions if sending to remote */

        if  (jp->h.bj_hostid)  {
                if  ((ret = getremperms(jp->h.bj_hostid, &mpriv, &pj->groupname)) != 0)  {
                        if  (scriptstring)
                                free(scriptstring);
                        return  pj->retcode = ret;
                }
        }

        /*  Fill in any missing or invalid bits of the job
            If no cmd int or unknown one, fill in standard one and reset load level
            otherwise if the user isn't suitably privileged, reset the load level to the standard for the CI
            Note that we use the local version of the command interpreter even for remote jobs on the
            assumption that these are consistent */

        if  (jp->h.bj_cmdinterp[0] == '\0'  ||  (cinum = validate_ci(jp->h.bj_cmdinterp)) < 0)  {
                strcpy(jp->h.bj_cmdinterp, Ci_list[CI_STDSHELL].ci_name);
                jp->h.bj_ll = Ci_list[CI_STDSHELL].ci_ll;
                cinum = CI_STDSHELL;
        }
        else  if  (!(mpriv->btu_priv & BTM_SPCREATE)  ||  jp->h.bj_ll == 0)
                jp->h.bj_ll = Ci_list[cinum].ci_ll;

        /* Check job has a directory */

        if  (jp->h.bj_direct < 0)  {
                if  (scriptstring)
                        free(scriptstring);
                return  pj->retcode = $E{Bts invalid directory};
        }

        /* Validate load level is OK for user */

        if  (jp->h.bj_ll > mpriv->btu_maxll)  {
                if  (scriptstring)
                        free(scriptstring);
                disp_arg[0] = jp->h.bj_ll;
                disp_arg[1] = mpriv->btu_maxll;
                return  pj->retcode = $E{Bts invalid load level};
        }

        /* Set priority in case it didn't get set */

        if  (jp->h.bj_pri == 0)
                jp->h.bj_pri = mpriv->btu_defp;

        /* Check priority is OK for user */

        if  (jp->h.bj_pri < mpriv->btu_minp || jp->h.bj_pri > mpriv->btu_maxp)  {
                if  (scriptstring)
                        free(scriptstring);
                disp_arg[0] = jp->h.bj_pri;
                disp_arg[1] = mpriv->btu_minp;
                disp_arg[2] = mpriv->btu_maxp;
                return  pj->retcode = $E{Bts invalid priority};
        }

        if  ((mpriv->btu_priv & BTM_UMASK) == 0  &&
             (jp->h.bj_mode.u_flags != mpriv->btu_jflags[0] ||
              jp->h.bj_mode.g_flags != mpriv->btu_jflags[1] ||
              jp->h.bj_mode.o_flags != mpriv->btu_jflags[2]))  {
                if  (scriptstring)
                        free(scriptstring);
                return  pj->retcode = $E{Bts cannot set mode};
        }

        /* If sending to remote host, remember the script for later and finish */

        if  (jp->h.bj_hostid)  {
                pj->scriptstring = scriptstring;
                return  0;
        }

        /* Set time in job to now */

        time(&jp->h.bj_time);

        /* Write script to file trying to use previous job number if we can */

        if  (jp->h.bj_job == 0)
                jp->h.bj_job = getpid();

        SWAP_TO(Daemuid);

        for  (;;)  {
                 sprintf(tmpfl, "%s/%s", spdir, mkspid(SPNAM, jp->h.bj_job));
                 if  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0666)) >= 0)
                         break;
                 if  (errno != EEXIST)  {
                        int     saverr = errno;
                        SWAP_TO(Realuid);
                        free(scriptstring);
                        errno = saverr;
                        return pj->retcode = $E{Bts write script fail};
                 }
                 jp->h.bj_job += JN_INC;
        }

        SWAP_TO(Realuid);

        if  (scriptlen != 0)  {
                ret = write(fid, scriptstring, scriptlen);
                if  (ret < 0)  {
                        int     saverr = errno;
                        free(scriptstring);
                        Ignored_error = close(fid);
                        SWAP_TO(Daemuid);
                        Ignored_error = unlink(tmpfl);
                        SWAP_TO(Realuid);
                        errno = saverr;
                        return pj->retcode = $E{Bts write script fail};
                }
        }

        if  (scriptstring)
                free(scriptstring);

        if  (close(fid) < 0)  {
                int     saverr = errno;
                SWAP_TO(Daemuid);
                Ignored_error = unlink(tmpfl);
                SWAP_TO(Realuid);
                errno = saverr;
                return pj->retcode = $E{Bts write script fail};
        }

        /* If set-user id not honoured running as root as we were root, reset the ownership */

#ifdef  NHONSUID
                if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))
                        Ignored_error = chown(tmpfl, Daemuid, Realgid);
#elif   !defined(HAVE_SETEUID)  &&  defined(ID_SWAP)
                if  (Daemuid != ROOTID  &&  Realuid == ROOTID)
                        Ignored_error = chown(tmpfl, Daemuid, Realgid);
#endif

        pj->spoolfile_name = stracpy(tmpfl);
        pj->groupname = realgname;
        return  0;
}

/* Enqueue request to batch scheduler. */

static  int  loc_enqueue(struct pending_job *pj)
{
        unsigned        retc;
        int     blkcount = MSGQ_BLOCKS;

        holdsigs(1);
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        *JREQ = pj->jobdescr;

        SWAP_TO(Daemuid);

        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN  ||  --blkcount <= 0)  {
                        pj->retcode = errno == EAGAIN? $E{IPC msg q full}: $E{IPC msg q error};
                        holdsigs(0);
                        return  pj->retcode;
                }
                sleep(MSGQ_BLOCKWAIT);
        }
        if  ((retc = readreply()) != J_OK)  {
                holdsigs(0);
                if  ((retc & REQ_TYPE) != JOB_REPLY)  { /* Not expecting net errors */
                        disp_arg[0] = retc;
                        return  pj->retcode = $E{Unexpected sched message};
                }
                else
                        return  pj->retcode = (retc & ~REQ_TYPE) + $E{Base for scheduler job errors};
        }

        SWAP_TO(Realuid);

        pj->subok = 1;
        holdsigs(0);
        return  0;
}

static int  remprocreply(const int sock, BtjobRef jb)
{
        int     errcode, which;
        struct  client_if       result;

        if  (!remsub_sock_read(sock, (char *) &result, sizeof(result)))
                return  $EH{Cant read status result};

        errcode = result.code;
 redo:
        switch  (errcode)  {
        case  XBNQ_OK:
                jb->h.bj_job = ntohl(result.param);
                return  0;

        case  XBNR_BADCVAR:
                which = (int) ntohl(result.param);
                if  ((unsigned) which < MAXCVARS)  {
                        int     fw, cnt;
                        for  (fw = -1, cnt = 0;  cnt < MAXCVARS;  cnt++)
                                if  (jb->h.bj_conds[cnt].bjc_compar != C_UNUSED)  { /* Code takes care of possible gaps */
                                        if  (++fw == which)  {
                                                disp_str = Var_seg.vlist[jb->h.bj_conds[cnt].bjc_varind].Vent.var_name;
                                                goto  calced;
                                        }
                                }
                }
                return  $EH{Bad condition result};

        case  XBNR_BADAVAR:
                which = (int) ntohl(result.param);
                if  ((unsigned) which < MAXSEVARS)  {
                        int     fw, cnt;
                        for  (fw = -1, cnt = 0;  cnt < MAXSEVARS;  cnt++)
                                if  (jb->h.bj_asses[cnt].bja_op != BJA_NONE)  { /* Code takes care of possible gaps */
                                        if  (++fw == which)  {
                                                disp_str = Var_seg.vlist[jb->h.bj_asses[cnt].bja_varind].Vent.var_name;
                                                goto  calced;
                                        }
                                }
                }
                return  $EH{Bad assignment result};

        case  XBNR_BADCI:
                disp_str = jb->h.bj_cmdinterp;

        case  XBNR_UNKNOWN_CLIENT:
        case  XBNR_NOT_CLIENT:
        case  XBNR_NOT_USERNAME:
        case  XBNR_NOMEM_QF:
        case  XBNR_NOCRPERM:
        case  XBNR_BAD_PRIORITY:
        case  XBNR_BAD_LL:
        case  XBNR_BAD_USER:
        case  XBNR_FILE_FULL:
        case  XBNR_QFULL:
        case  XBNR_BAD_JOBDATA:
        case  XBNR_UNKNOWN_USER:
        case  XBNR_UNKNOWN_GROUP:
        case  XBNR_NORADMIN:
                break;
        case  XBNR_ERR:
                errcode = ntohl(result.param);
                goto  redo;
        default:
                disp_arg[0] = errcode;
                return  $EH{Unknown queue result message};
        }
 calced:
        return  $EH{Base for rbtr return errors} + result.code;
}

static  int     rem_enqueue(struct pending_job *pj)
{
        int     ret, sock, msgsize;
        BtjobRef  jp = &pj->jobdescr;
        struct  nijobmsg        outmsg;

        if  (tcpportnum < 0  &&  (ret = remsub_inittcp(&tcpportnum)) != 0)
                return  pj->retcode = ret;
        if  ((ret = remsub_opentcp(jp->h.bj_hostid, tcpportnum, &sock)) != 0)
                return  pj->retcode = ret;
        msgsize = remsub_packjob(&outmsg, jp);
        remsub_condasses(&outmsg, jp, jp->h.bj_hostid);
        if  ((ret = remsub_startjob(sock, msgsize, realuname, pj->groupname)) != 0)  {
                close(sock);
                return  pj->retcode = ret;
        }
        if  (!remsub_sock_write(sock, (char *) &outmsg, (int) msgsize))  {
                close(sock);
                return  pj->retcode = $EH{Trouble with job};
        }
        remsub_copyout_str(pj->scriptstring, sock, realuname, pj->groupname);
        ret = remprocreply(sock, jp);
        close(sock);
        if  (ret != 0)
                return  pj->retcode = ret;
        pj->subok = 1;
        return  0;
}

static  int     enqueue(struct pending_job *pj)
{
        if  (pj->jobdescr.h.bj_hostid)
                return  rem_enqueue(pj);
        return  loc_enqueue(pj);
}
#endif	/* HAVE_LIBXML2 */

MAINFN_TYPE  main(int argc, char **argv)
{
#ifdef	HAVE_LIBXML2
        static  char    Varname[] = "BTS";
        int  ret, errors = 0;
        unsigned  cnt;
        char    **av;
#endif
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

#ifdef	HAVE_LIBXML2
        argv = optprocess(argv, Adefs, optprocs, $A{bts arg explain}, $A{bts arg freeze home}, 0);

        SWAP_TO(Daemuid);               /* Freeze stuff may assume we're set to that */

#define FREEZE_EXIT
#include "inline/freezecode.c"

        SWAP_TO(Realuid);

        /* We need to put user and group names in packets */

        realuname = prin_uname(Realuid);
        realgname = prin_gname(Realgid);

        for  (av = argv;  *av;  av++)
                if  (access(*av, R_OK) >= 0)
                        num_pending_jobs++;
                else  {
                        errors++;
                        disp_str = *av;
                        if  (errno == ENOENT)
                                print_error($E{Bts cannot open file});
                        else  if  (errno == EACCES)
                                print_error($E{Bts no permission on file});
                        else
                                print_error($E{Bts other error file});
                }

        if  (errors > 0)  {
                disp_arg[0] = errors;
                print_error($E{Bts aborting due to arg errors});
                return  E_NOJOB;
        }

        if  (num_pending_jobs == 0)  {
                print_error($E{Bts no files});
                return  E_USAGE;
        }

        /* Set up vector of pending jobs as we prefer to submit all of them or none of them */

        joblist = (struct pending_job *) malloc((unsigned) (num_pending_jobs * sizeof(struct pending_job)));
        if  (!joblist)
                ABORT_NOMEM;
        BLOCK_ZERO(joblist, num_pending_jobs * sizeof(struct pending_job));

        /* Don't want XML errors on STDERR */

        init_xml();

        /* Swap back to batch user id as we want to check user permissions and set things up properly.
           We need this even if we are sending to remotes only as conds and asses have references to
           the shm segments */

        SWAP_TO(Daemuid);

        init_batchfiles();

        /* Process each job file and make a pending job out of it.
           If we find any errors we don't do any more */

        SWAP_TO(Realuid);
        catchsigs();

        for  (av = argv;  *av;  av++)
                if  ((ret = setup_job(*av, av-argv)) != 0)  {
                        errors++;
                        print_error(ret);
                }

        /* If there were errors, don't do any more */

        if  (errors > 0)  {
                removefiles();
                return  E_NOJOB;
        }

        /* Now submit the jobs */

       for  (cnt = 0;  cnt < num_pending_jobs;  cnt++)  {
                struct  pending_job  *pj = &joblist[cnt];
                if  ((ret = enqueue(pj)) != 0)  {
                        print_error(ret);
                        removefiles();
                        return  E_SHEDERR;
                }
                if  (pj->verbose)  {
                        disp_str = (char *) title_of(&pj->jobdescr);
                        disp_str2 = pj->argfile_name;
                        disp_arg[0] = pj->jobdescr.h.bj_job;
                        fprint_error(stdout, strlen(disp_str) != 0? $E{Job created with name}: $E{Job created without name});
               }
        }

        return  0;
#else
        print_error($E{No XML library});
        return  E_NOTIMPL;
#endif
}
