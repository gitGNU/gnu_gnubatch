/* xbnet_api.c -- API handling for xbnetserv

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <time.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_sig.h"
#include <errno.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "xbapi_int.h"
#include "xbnet_ext.h"

static  char    Filename[] = __FILE__;

struct  api_status      {
        int             sock;                   /* TCP socket */
        int             prodsock;               /* Socket for refresh messages */
        netid_t         hostid;                 /* Who we are speaking to 0=local host */
        int_ugid_t      realuid;                /* User id in question */
        int_ugid_t      realgid;                /* Group id in question */
        char            *current_queue;         /* Current queue prefix */
        unsigned        current_qlen;           /* Length of that saves calculating each time */
        enum  {  NOT_LOGGED = 0, LOGGED_IN_UNIX = 1, LOGGED_IN_WIN = 2, LOGGED_IN_WINU = 3 }    is_logged;
        ULONG           jser;                   /* Job shm serial */
        ULONG           vser;                   /* Var shm serial */
        Btuser          hispriv;                /* Permissions structure in effect */
        struct  api_msg inmsg;                  /* Input message */
        struct  api_msg outmsg;                 /* Output message */
        struct  sockaddr_in     apiaddr;        /* Address for binding prod socket */
        struct  sockaddr_in     apiret;         /* Address for sending prods */
};

FILE *net_feed(const int, const netid_t, const jobno_t, const int);

/* Initialise status thing so we can have it auto */

static  void    init_status(struct api_status *ap)
{
        BLOCK_ZERO(ap, sizeof(struct api_status));
        ap->sock = ap->prodsock = -1;
}

static int  decodejreply()
{
        switch  (readreply())  {
        default:                return  XB_ERR;                 /* Dunno what that was */
        case  J_OK:             return   XB_OK;
        case  J_NEXIST:         return  XB_UNKNOWN_JOB;
        case  J_VNEXIST:        return  XB_UNKNOWN_VAR;
        case  J_NOPERM:         return  XB_NOPERM;
        case  J_VNOPERM:        return  XB_NOPERM_VAR;
        case  J_NOPRIV:         return  XB_NOPERM;
        case  J_SYSVAR:         return  XB_BAD_AVAR;
        case  J_SYSVTYPE:       return  XB_BAD_AVAR;
        case  J_FULLUP:         return  XB_NOMEM_QF;
        case  J_ISRUNNING:      return  XB_ISRUNNING;
        case  J_REMVINLOCJ:     return  XB_RVAR_LJOB;
        case  J_LOCVINEXPJ:     return  XB_LVAR_RJOB;
        case  J_MINPRIV:        return  XB_MINPRIV;
        }
}

static int  readvreply()
{
        switch  (readreply())  {
        default:                return  XB_ERR;                 /* Dunno what that was */
        case  V_OK:             return  XB_OK;
        case  V_EXISTS:         return  XB_VEXISTS;
        case  V_NEXISTS:        return  XB_UNKNOWN_VAR;
        case  V_CLASHES:        return  XB_VEXISTS;
        case  V_NOPERM:         return  XB_NOPERM;
        case  V_NOPRIV:         return  XB_NOPERM;
        case  V_SYNC:           return  XB_SEQUENCE;
        case  V_SYSVAR:         return  XB_SYSVAR;
        case  V_SYSVTYPE:       return  XB_SYSVTYPE;
        case  V_FULLUP:         return  XB_QFULL;
        case  V_DSYSVAR:        return  XB_DSYSVAR;
        case  V_INUSE:          return  XB_DINUSE;
        case  V_MINPRIV:        return  XB_MINPRIV;
        case  V_DELREMOTE:      return  XB_DELREMOTE;
        case  V_UNKREMUSER:     return  XB_UNKNOWN_USER;
        case  V_UNKREMGRP:      return  XB_UNKNOWN_GROUP;
        case  V_RENEXISTS:      return  XB_VEXISTS;
        case  V_NOTEXPORT:      return  XB_NOTEXPORT;
        case  V_RENAMECLUST:    return  XB_RENAMECLUST;
        }
}

/* This assumes that Real[ug]id is set. We need to revise this sometime */

static void  mode_pack(Btmode *dest, const Btmode *src)
{
        dest->o_uid = htonl(src->o_uid);
        dest->o_gid = htonl(src->o_gid);
        strncpy(dest->o_user, src->o_user, UIDSIZE);
        strncpy(dest->o_group, src->o_group, UIDSIZE);
        if  (mpermitted(src, BTM_RDMODE, 0))  {
                dest->c_uid = htonl(src->c_uid);
                dest->c_gid = htonl(src->c_gid);
                strncpy(dest->c_user, src->c_user, UIDSIZE);
                strncpy(dest->c_group, src->c_group, UIDSIZE);
                dest->u_flags = htons(src->u_flags);
                dest->g_flags = htons(src->g_flags);
                dest->o_flags = htons(src->o_flags);
        }
}

/* Send a message regarding a job operation, and decode result.  */

static int  wjimsg(const unsigned op, const Btjob *jp, const ULONG param)
{
        int             tries;
        Shipc           Oreq;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = op;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_params.param = param;
        Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
        Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
        for  (tries = 1;  tries <= MSGQ_BLOCKS;  tries++)  {
                if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), IPC_NOWAIT) >= 0)
                        return  decodejreply();
                sleep(MSGQ_BLOCKWAIT);
        }
        return  XB_QFULL;
}

/* Ditto for job create/update */

static int  wjmsg(const unsigned op, const ULONG xindx)
{
        int             tries;
        Shipc           Oreq;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = op;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_un.sh_jobindex = xindx;
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        for  (tries = 1;  tries <= MSGQ_BLOCKS;  tries++)  {
                if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) >= 0)  {
                        int  ret  = decodejreply();
                        freexbuf_serv(xindx);
                        return  ret;
                }
                sleep(MSGQ_BLOCKWAIT);
        }
        freexbuf_serv(xindx);
        return  XB_QFULL;
}

static int  wvmsg(const unsigned op, const Btvar *varp, const ULONG seq, const ULONG param)
{
        int     tries;
        Shipc   Oreq;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = op;
        Oreq.sh_params.param = param;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_un.sh_var = *varp;
        Oreq.sh_un.sh_var.var_sequence = seq;
        for  (tries = 1;  tries <= MSGQ_BLOCKS;  tries++)  {
                if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), IPC_NOWAIT) >= 0)
                        return  readvreply();
                sleep(MSGQ_BLOCKWAIT);
        }
        return  XB_QFULL;
}

/* Exit and abort pending jobs */

static void  abort_exit(const int n)
{
        unsigned        cnt;
        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
                struct  pend_job   *pj = &pend_list[cnt];
                if  (pj->out_f)  {
                        fclose(pj->out_f);
                        pj->out_f = (FILE *) 0;
                        unlink(pj->tmpfl);
                }
        }
        exit(n);
}

static void  setup_prod(struct api_status *ap)
{
        BLOCK_ZERO(&ap->apiret, sizeof(ap->apiret));
        ap->apiret.sin_family = AF_INET;
        ap->apiret.sin_addr.s_addr = htonl(INADDR_ANY);
        if  ((ap->prodsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                return;
        if  (bind(ap->prodsock, (struct sockaddr *) &ap->apiret, sizeof(ap->apiret)) < 0)  {
                close(ap->prodsock);
                ap->prodsock = -1;
        }
}

static void  unsetup_prod(struct api_status *ap)
{
        if  (ap->prodsock >= 0)  {
                close(ap->prodsock);
                ap->prodsock = -1;
        }
}

static void  proc_refresh(struct api_status *ap)
{
        int     prodj = 0, prodv = 0;

        if  (ap->jser != Job_seg.dptr->js_serial)  {
                ap->jser = Job_seg.dptr->js_serial;
                prodj++;
        }
        if  (ap->vser != Var_seg.dptr->vs_serial)  {
                ap->vser = Var_seg.dptr->vs_serial;
                prodv++;
        }

        if  (ap->prodsock < 0  ||  !(prodj || prodv))
                return;

        BLOCK_ZERO(&ap->apiaddr, sizeof(ap->apiaddr));
        ap->apiaddr.sin_family = AF_INET;
        ap->apiaddr.sin_addr.s_addr = ap->hostid;
        ap->apiaddr.sin_port = apipport;

        if  (prodj)  {
                ap->outmsg.code = API_JOBPROD;
                ap->outmsg.un.r_reader.seq = htonl(ap->jser);
                if  (sendto(ap->prodsock, (char *) &ap->outmsg, sizeof(ap->outmsg), 0, (struct sockaddr *) &ap->apiaddr, sizeof(ap->apiaddr)) < 0)  {
                        close(ap->prodsock);
                        ap->prodsock = -1;
                        return;
                }
        }
        if  (prodv)  {
                ap->outmsg.code = API_VARPROD;
                ap->outmsg.un.r_reader.seq = htonl(ap->vser);
                if  (sendto(ap->prodsock, (char *) &ap->outmsg, sizeof(ap->outmsg), 0, (struct sockaddr *) &ap->apiaddr, sizeof(ap->apiaddr)) < 0)  {
                        close(ap->prodsock);
                        ap->prodsock = -1;
                        return;
                }
        }
}

static void  pushout(struct api_status *ap, char *cbufp, unsigned obytes)
{
        int     xbytes;

        while  (obytes != 0)  {
                if  ((xbytes = write(ap->sock, cbufp, obytes)) < 0)  {
                        if  (errno == EINTR)
                                continue;
                        abort_exit(0);
                }
                cbufp += xbytes;
                obytes -= xbytes;
        }
}

static void  pullin(struct api_status *ap, char *cbufp, unsigned ibytes)
{
        int     xbytes;

        while  (ibytes != 0)  {
                if  ((xbytes = read(ap->sock, cbufp, ibytes)) <= 0)  {
                        if  (xbytes < 0  &&  errno == EINTR)  {
                                while  (hadrfresh)  {
                                        hadrfresh = 0;
                                        proc_refresh(ap);
                                }
                                continue;
                        }
                        abort_exit(0);
                }
                cbufp += xbytes;
                ibytes -= xbytes;
        }
}

static  void    put_reply(struct api_status *ap)
{
        pushout(ap, (char *) &ap->outmsg, sizeof(ap->outmsg));
}

static  void    get_message(struct api_status *ap)
{
        pullin(ap, (char *) &ap->inmsg, sizeof(ap->inmsg));
}

static void  err_result(struct api_status *ap, const int code, const ULONG seq)
{
        ap->outmsg.code = 0;
        ap->outmsg.retcode = htons((SHORT) code);
        ap->outmsg.un.r_reader.seq = htonl(seq);
        put_reply(ap);
}

static void  swapinj(Btjob *to, const struct jobnetmsg *from)
{
        unsigned  cnt;
#ifndef WORDS_BIGENDIAN
        const   Jarg    *farg;  const   Envir   *fenv;  const   Redir   *fred;
        Jarg    *targ;  Envir   *tenv;  Redir   *tred;
#endif
        BLOCK_ZERO(&to->h, sizeof(Btjobh));

        to->h.bj_hostid = from->hdr.jid.hostid;
        to->h.bj_slotno = ntohl(from->hdr.jid.slotno);

        to->h.bj_progress       = from->hdr.nm_progress;
        to->h.bj_pri            = from->hdr.nm_pri;
        to->h.bj_jflags = from->hdr.nm_jflags;
        to->h.bj_times.tc_istime= from->hdr.nm_istime;
        to->h.bj_times.tc_mday  = from->hdr.nm_mday;
        to->h.bj_times.tc_repeat= from->hdr.nm_repeat;
        to->h.bj_times.tc_nposs = from->hdr.nm_nposs;

        to->h.bj_ll                     = ntohs(from->hdr.nm_ll);
        to->h.bj_umask                  = ntohs(from->hdr.nm_umask);
        strcpy(to->h.bj_cmdinterp, from->hdr.nm_cmdinterp);
        to->h.bj_times.tc_nvaldays      = ntohs(from->hdr.nm_nvaldays);
        to->h.bj_autoksig               = ntohs(from->hdr.nm_autoksig);
        to->h.bj_runon                  = ntohs(from->hdr.nm_runon);
        to->h.bj_deltime                = htons(from->hdr.nm_deltime);

        to->h.bj_ulimit                 = ntohl(from->hdr.nm_ulimit);
        to->h.bj_times.tc_nexttime      = (time_t) ntohl(from->hdr.nm_nexttime);
        to->h.bj_times.tc_rate          = ntohl(from->hdr.nm_rate);
        to->h.bj_runtime                = ntohl(from->hdr.nm_runtime);

        to->h.bj_exits                  = from->hdr.nm_exits;
        to->h.bj_mode.u_flags           = ntohs(from->hdr.nm_mode.u_flags);
        to->h.bj_mode.g_flags           = ntohs(from->hdr.nm_mode.g_flags);
        to->h.bj_mode.o_flags           = ntohs(from->hdr.nm_mode.o_flags);

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                const   Jncond  *drc = &from->hdr.nm_conds[cnt];
                Jcond   *cr = &to->h.bj_conds[cnt];
                if  (drc->bjnc_compar == C_UNUSED)
                        break;
                cr->bjc_compar = drc->bjnc_compar;
                cr->bjc_iscrit = drc->bjnc_iscrit;
                cr->bjc_varind = ntohl(drc->bjnc_var.slotno);
                if  ((cr->bjc_value.const_type = drc->bjnc_type) == CON_STRING)
                        strncpy(cr->bjc_value.con_un.con_string, drc->bjnc_un.bjnc_string, BTC_VALUE);
                else
                        cr->bjc_value.con_un.con_long = ntohl(drc->bjnc_un.bjnc_long);
        }

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                const   Jnass   *dra = &from->hdr.nm_asses[cnt];
                Jass    *cr = &to->h.bj_asses[cnt];
                if  (dra->bjna_op == BJA_NONE)
                        break;
                cr->bja_flags = ntohs(dra->bjna_flags);
                cr->bja_op = dra->bjna_op;
                cr->bja_iscrit = dra->bjna_iscrit;
                cr->bja_varind = ntohl(dra->bjna_var.slotno);
                if  ((cr->bja_con.const_type = dra->bjna_type) == CON_STRING)
                        strncpy(cr->bja_con.con_un.con_string, dra->bjna_un.bjna_string, BTC_VALUE);
                else
                        cr->bja_con.con_un.con_long = ntohl(dra->bjna_un.bjna_long);
        }
        to->h.bj_nredirs        = ntohs(from->nm_nredirs);
        to->h.bj_nargs          = ntohs(from->nm_nargs);
        to->h.bj_nenv           = ntohs(from->nm_nenv);
        to->h.bj_title          = ntohs(from->nm_title);
        to->h.bj_direct         = ntohs(from->nm_direct);
        to->h.bj_redirs         = ntohs(from->nm_redirs);
        to->h.bj_env            = ntohs(from->nm_env);
        to->h.bj_arg            = ntohs(from->nm_arg);
        BLOCK_COPY(to->bj_space, from->nm_space, JOBSPACE);

#ifndef WORDS_BIGENDIAN

        /* If we are byte-swapping we must swap the argument,
           environment and redirection variable pointers and the
           arg field in each redirection.  */

        farg = (const Jarg *) &from->nm_space[to->h.bj_arg];
        fenv = (const Envir *) &from->nm_space[to->h.bj_env];
        fred = (const Redir *) &from->nm_space[to->h.bj_redirs];
        targ = (Jarg *) &to->bj_space[to->h.bj_arg];
        tenv = (Envir *) &to->bj_space[to->h.bj_env];
        tred = (Redir *) &to->bj_space[to->h.bj_redirs];

        for  (cnt = 0;  cnt < to->h.bj_nargs;  cnt++)  {
                *targ++ = ntohs(*farg);
                farg++; /* Not falling for ntohs being a macro!!! */
        }
        for  (cnt = 0;  cnt < to->h.bj_nenv;  cnt++)  {
                tenv->e_name = ntohs(fenv->e_name);
                tenv->e_value = ntohs(fenv->e_value);
                tenv++;
                fenv++;
        }
        for  (cnt = 0;  cnt < to->h.bj_nredirs; cnt++)  {
                tred->arg = ntohs(fred->arg);
                tred++;
                fred++;
        }
#endif
}

static int  notinqueue(struct api_status *ap, const Btjob *jp)
{
        const   char    *tit, *cp;

        if  (!ap->current_queue)
                return  0;

        tit = title_of(jp);
        cp = strchr(tit, ':');
        if  (!cp)
                return  0;
        if  ((cp - tit) != ap->current_qlen)
                return  1;

        return  strncmp(tit, ap->current_queue, ap->current_qlen) != 0;
}

static void  reply_joblist(struct api_status *ap)
{
        ULONG           flags = ntohl(ap->inmsg.un.lister.flags);
        unsigned        jind, njobs;
        slotno_t        *rbuf, *rbufp;

        ap->outmsg.code = API_JOBLIST;
        ap->outmsg.retcode = 0;

        rjobfile(0);                    /* param 0 means leave locked */
        njobs = 0;

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                BtjobRef        jp = &jhp->j;

                jind = jhp->q_nxt;

                if  (!mpermitted(&jp->h.bj_mode, BTM_SHOW, 0))
                        continue;
                if  (flags & XB_FLAG_LOCALONLY && jp->h.bj_hostid != 0)
                        continue;
                if  (flags & XB_FLAG_USERONLY && jp->h.bj_mode.o_uid != Realuid)
                        continue;
                if  (flags & XB_FLAG_GROUPONLY && jp->h.bj_mode.o_gid != Realgid)
                        continue;
                if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                        continue;
                njobs++;
        }

        ap->outmsg.un.r_lister.nitems = htonl((ULONG) njobs);
        ap->outmsg.un.r_lister.seq = htonl(Job_seg.dptr->js_serial);
        put_reply(ap);
        if  (njobs == 0)  {
                junlock();
                return;
        }
        if  (!(rbuf = (slotno_t *) malloc(njobs * sizeof(slotno_t))))
                ABORT_NOMEM;
        rbufp = rbuf;

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                HashBtjob       *jhp = &Job_seg.jlist[jind];
                BtjobRef        jp = &jhp->j;
                LONG            nind = jind;

                jind = jhp->q_nxt;

                if  (!mpermitted(&jp->h.bj_mode, BTM_SHOW, 0))
                        continue;
                if  (flags & XB_FLAG_LOCALONLY && jp->h.bj_hostid != 0)
                        continue;
                if  (flags & XB_FLAG_USERONLY && jp->h.bj_mode.o_uid != Realuid)
                        continue;
                if  (flags & XB_FLAG_GROUPONLY && jp->h.bj_mode.o_gid != Realgid)
                        continue;
                if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                        continue;
                *rbufp++ = htonl(nind);
        }
        junlock();

        /* Splat the thing out */

        pushout(ap, (char *) rbuf, sizeof(slotno_t) * njobs);
        free((char *) rbuf);
}

static void  reply_varlist(struct api_status *ap)
{
        ULONG           flags = ntohl(ap->inmsg.un.lister.flags);
        struct  Ventry  *vp, *ve;
        unsigned        numvars = 0;
        slotno_t        *rbuf, *rbufp;

        ap->outmsg.code = API_VARLIST;
        ap->outmsg.retcode = 0;
        rvarfile(0);                    /* And leave locked */
        ve = &Var_seg.vlist[Var_seg.dptr->vs_maxvars];

        for  (vp = &Var_seg.vlist[0]; vp < ve;  vp++)  {
                Btvar   *varp;
                if  (!vp->Vused)
                        continue;
                varp = &vp->Vent;
                if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0))
                        continue;
                if  (flags & XB_FLAG_LOCALONLY  &&  varp->var_id.hostid != 0)
                        continue;
                if  (flags & XB_FLAG_USERONLY  &&  varp->var_mode.o_uid != Realuid)
                        continue;
                if  (flags & XB_FLAG_GROUPONLY  &&  varp->var_mode.o_gid != Realgid)
                        continue;
                numvars++;
        }

        ap->outmsg.un.r_lister.nitems = htonl((ULONG) numvars);
        ap->outmsg.un.r_lister.seq = htonl(Var_seg.dptr->vs_serial);
        put_reply(ap);
        if  (numvars == 0)  {
                vunlock();
                return;
        }
        if  (!(rbuf = (slotno_t *) malloc(numvars * sizeof(slotno_t))))
                ABORT_NOMEM;

        rbufp = rbuf;
        for  (vp = &Var_seg.vlist[0]; vp < ve;  vp++)  {
                Btvar   *varp;
                if  (!vp->Vused)
                        continue;
                varp = &vp->Vent;
                if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0))
                        continue;
                if  (flags & XB_FLAG_LOCALONLY  &&  varp->var_id.hostid != 0)
                        continue;
                if  (flags & XB_FLAG_USERONLY  &&  varp->var_mode.o_uid != Realuid)
                        continue;
                if  (flags & XB_FLAG_GROUPONLY  &&  varp->var_mode.o_gid != Realgid)
                        continue;
                *rbufp++ = htonl((ULONG) (vp - Var_seg.vlist));
        }
        vunlock();

        /* Splat the thing out */

        pushout(ap, (char *) rbuf, sizeof(slotno_t) * numvars);
        free((char *) rbuf);
}

static int  check_valid_job(struct api_status *ap, const ULONG flags, const Btjob *jp)
{
        if  (jp->h.bj_job == 0  ||
             !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
                return  0;

        if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                return  0;

        return  1;
}

static void  job_read_rest(struct api_status *ap, const Btjob *jp)
{
        int             cnt;
#ifndef WORDS_BIGENDIAN
        Jarg    *darg;  Envir   *denv;  Redir   *dred;
        const   Jarg    *sarg;  const   Envir   *senv;  const   Redir   *sred;
#endif
        unsigned        hwm = 1;        /* Send one byte in case memcpy etc choke over 0 bytes */
        struct  jobnetmsg       outjob;

        BLOCK_ZERO(&outjob, sizeof(outjob));
        mode_pack(&outjob.hdr.nm_mode, &jp->h.bj_mode);

        /* These things we copy across even if the user can't read the job.
           M&S fix 28/3/96.  */

        outjob.hdr.jid.hostid = int2ext_netid_t(jp->h.bj_hostid);
        outjob.hdr.jid.slotno = htonl(jp->h.bj_slotno);

        outjob.hdr.nm_progress  = jp->h.bj_progress;
        outjob.hdr.nm_jflags    = jp->h.bj_jflags;
        outjob.hdr.nm_orighostid= int2ext_netid_t(jp->h.bj_orighostid);
        outjob.hdr.nm_runhostid = int2ext_netid_t(jp->h.bj_runhostid);
        outjob.hdr.nm_job       = htonl(jp->h.bj_job);

        if  (mpermitted(&jp->h.bj_mode, BTM_READ, 0))  {
                outjob.hdr.nm_pri       = jp->h.bj_pri;
                outjob.hdr.nm_istime    = jp->h.bj_times.tc_istime;
                outjob.hdr.nm_mday      = jp->h.bj_times.tc_mday;
                outjob.hdr.nm_repeat    = jp->h.bj_times.tc_repeat;
                outjob.hdr.nm_nposs     = jp->h.bj_times.tc_nposs;

                outjob.hdr.nm_ll        = htons(jp->h.bj_ll);
                outjob.hdr.nm_umask     = htons(jp->h.bj_umask);
                strcpy(outjob.hdr.nm_cmdinterp, jp->h.bj_cmdinterp);
                outjob.hdr.nm_nvaldays  = htons(jp->h.bj_times.tc_nvaldays);
                outjob.hdr.nm_autoksig  = htons(jp->h.bj_autoksig);
                outjob.hdr.nm_runon     = htons(jp->h.bj_runon);
                outjob.hdr.nm_deltime   = htons(jp->h.bj_deltime);
                outjob.hdr.nm_lastexit  = htons(jp->h.bj_lastexit);

                outjob.hdr.nm_time      = htonl((LONG) jp->h.bj_time);
                outjob.hdr.nm_stime     = htonl((LONG) jp->h.bj_stime);
                outjob.hdr.nm_etime     = htonl((LONG) jp->h.bj_etime);
                outjob.hdr.nm_pid       = htonl(jp->h.bj_pid);
                outjob.hdr.nm_ulimit    = htonl(jp->h.bj_ulimit);
                outjob.hdr.nm_nexttime  = htonl((LONG) jp->h.bj_times.tc_nexttime);
                outjob.hdr.nm_rate      = htonl(jp->h.bj_times.tc_rate);
                outjob.hdr.nm_runtime   = htonl(jp->h.bj_runtime);

                outjob.hdr.nm_exits     = jp->h.bj_exits;

                for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                        const   Jcond   *drc = &jp->h.bj_conds[cnt];
                        Jncond          *cr  = &outjob.hdr.nm_conds[cnt];
                        if  (drc->bjc_compar == C_UNUSED)
                                break;

                        /* Remember that we are putting the slot number on this machine,
                           not the absolute machine id/slot number on that machine pair.
                           This is because the slot number on this machine is the way
                           in which the user sees the result (for example) of xb_varlist.  */

                        cr->bjnc_var.slotno = htonl(drc->bjc_varind);
                        cr->bjnc_compar = drc->bjc_compar;
                        cr->bjnc_iscrit = drc->bjc_iscrit;
                        if  ((cr->bjnc_type = drc->bjc_value.const_type) == CON_STRING)
                                strncpy(cr->bjnc_un.bjnc_string, drc->bjc_value.con_un.con_string, BTC_VALUE);
                        else
                                cr->bjnc_un.bjnc_long = ntohl(drc->bjc_value.con_un.con_long);
                }

                for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                        const   Jass    *dra = &jp->h.bj_asses[cnt];
                        Jnass   *cr = &outjob.hdr.nm_asses[cnt];
                        if  (dra->bja_op == BJA_NONE)
                                break;
                        cr->bjna_var.slotno = htonl(dra->bja_varind);
                        cr->bjna_flags = htons(dra->bja_flags);
                        cr->bjna_op = dra->bja_op;
                        cr->bjna_iscrit = dra->bja_iscrit;
                        if  ((cr->bjna_type = dra->bja_con.const_type) == CON_STRING)
                                strncpy(cr->bjna_un.bjna_string, dra->bja_con.con_un.con_string, BTC_VALUE);
                        else
                                cr->bjna_un.bjna_long = ntohl(dra->bja_con.con_un.con_long);
                }

                outjob.nm_nredirs       = htons(jp->h.bj_nredirs);
                outjob.nm_nargs         = htons(jp->h.bj_nargs);
                outjob.nm_nenv          = htons(jp->h.bj_nenv);
                outjob.nm_title         = htons(jp->h.bj_title);
                outjob.nm_direct        = htons(jp->h.bj_direct);
                outjob.nm_redirs        = htons(jp->h.bj_redirs);
                outjob.nm_env           = htons(jp->h.bj_env);
                outjob.nm_arg           = htons(jp->h.bj_arg);

                BLOCK_COPY(outjob.nm_space, jp->bj_space, JOBSPACE);

                /* To work out the length, we cheat by assuming that packjstring
                   (and its cousins in xb_strings) put the directory and title in
                   last and we can use the offset of that as a high water mark to
                   give us the length.  */

                if  (jp->h.bj_title >= 0)  {
                        hwm = jp->h.bj_title;
                        hwm += strlen(&jp->bj_space[hwm]) + 1;
                }
                else  if  (jp->h.bj_direct >= 0)  {
                        hwm = jp->h.bj_direct;
                        hwm += strlen(&jp->bj_space[hwm]) + 1;
                }
                else
                        hwm = JOBSPACE;
        }

        hwm += sizeof(struct jobnetmsg) - JOBSPACE;
        outjob.hdr.hdr.length = htons((USHORT) hwm);

#ifndef WORDS_BIGENDIAN

        /* If we are byte-swapping we must swap the argument,
           environment and redirection variable pointers and the
           arg field in each redirection.  */

        darg = (Jarg *) &outjob.nm_space[jp->h.bj_arg]; /* I did mean jp there */
        denv = (Envir *) &outjob.nm_space[jp->h.bj_env];        /* and there */
        dred = (Redir *) &outjob.nm_space[jp->h.bj_redirs]; /* and there */
        sarg = (const Jarg *) &jp->bj_space[jp->h.bj_arg];
        senv = (const Envir *) &jp->bj_space[jp->h.bj_env];
        sred = (const Redir *) &jp->bj_space[jp->h.bj_redirs];

        for  (cnt = 0;  cnt < jp->h.bj_nargs;  cnt++)  {
                *darg++ = htons(*sarg);
                sarg++; /* Not falling for htons being a macro!!! */
        }
        for  (cnt = 0;  cnt < jp->h.bj_nenv;  cnt++)  {
                denv->e_name = htons(senv->e_name);
                denv->e_value = htons(senv->e_value);
                denv++;
                senv++;
        }
        for  (cnt = 0;  cnt < jp->h.bj_nredirs; cnt++)  {
                dred->arg = htons(sred->arg);
                dred++;
                sred++;
        }
#endif
        pushout(ap, (char *) &outjob, hwm);
}

static void  reply_jobread(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btjob                   *jp;

        rjobfile(1);
        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
                err_result(ap, XB_SEQUENCE, Job_seg.dptr->js_serial);
                return;
        }
        if  (slotno >= Job_seg.dptr->js_maxjobs)  {
                err_result(ap, XB_INVALIDSLOT, Job_seg.dptr->js_serial);
                return;
        }
        jp = &Job_seg.jlist[slotno].j;
        if  (check_valid_job(ap, flags, jp))  {
                ap->outmsg.code = API_JOBREAD;
                ap->outmsg.retcode = XB_OK;
                ap->outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                put_reply(ap);
                job_read_rest(ap, jp);
        }
}

static void  reply_jobfind(struct api_status *ap)
{
        jobno_t         jn = ntohl(ap->inmsg.un.jobfind.jobno);
        netid_t         nid = ext2int_netid_t(ap->inmsg.un.jobfind.netid);
        ULONG           flags = ntohl(ap->inmsg.un.jobfind.flags);
        unsigned        jind;
        Btjob           *jp;

        rjobfile(1);
        jind = Job_seg.hashp_jno[jno_jhash(jn)];
        while  (jind != JOBHASHEND)  {
                if  (Job_seg.jlist[jind].j.h.bj_job == jn  &&  Job_seg.jlist[jind].j.h.bj_hostid == nid)
                        goto  gotit;
                jind = Job_seg.jlist[jind].nxt_jno_hash;
        }
        err_result(ap, XB_UNKNOWN_JOB, Job_seg.dptr->js_serial);
        return;
 gotit:
        jp = &Job_seg.jlist[jind].j;
        if  (check_valid_job(ap, flags, jp))  {
                ap->outmsg.code = ap->inmsg.code;
                ap->outmsg.retcode = XB_OK;
                ap->outmsg.un.r_find.seq = htonl(Job_seg.dptr->js_serial);
                ap->outmsg.un.r_find.slotno = htonl((ULONG) jind);
                put_reply(ap);
                if  (ap->inmsg.code == API_FINDJOB)
                        job_read_rest(ap, jp);
        }
}

static void  var_read_rest(struct api_status *ap, const Btvar *varp)
{
        struct  varnetmsg       outvar;

        BLOCK_ZERO(&outvar, sizeof(outvar));
        mode_pack(&outvar.nm_mode, &varp->var_mode);
        outvar.vid.hostid = int2ext_netid_t(varp->var_id.hostid);
        outvar.vid.slotno = htonl(varp->var_id.slotno);
        outvar.nm_type = varp->var_type;
        outvar.nm_flags = varp->var_flags;
        strncpy(outvar.nm_name, varp->var_name, BTV_NAME);
        if  (mpermitted(&varp->var_mode, BTM_READ, 0))  {
                outvar.nm_c_time = htonl(varp->var_c_time);
                strncpy(outvar.nm_comment, varp->var_comment, BTV_COMMENT);
                if  ((outvar.nm_consttype = varp->var_value.const_type) == CON_STRING)
                        strncpy(outvar.nm_un.nm_string, varp->var_value.con_un.con_string, BTC_VALUE);
                else
                        outvar.nm_un.nm_long = htonl(varp->var_value.con_un.con_long);
        }
        pushout(ap, (char *) &outvar, sizeof(outvar));
}

static void  reply_varread(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar           *varp;

        rvarfile(1);
        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)  {
                err_result(ap, XB_SEQUENCE, Var_seg.dptr->vs_serial);
                return;
        }
        if  (slotno >= Var_seg.dptr->vs_maxvars)  {
                err_result(ap, XB_INVALIDSLOT, Var_seg.dptr->vs_serial);
                return;
        }

        if  (!Var_seg.vlist[slotno].Vused)  {
                err_result(ap, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
                return;
        }

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))  {
                err_result(ap, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
                return;
        }
        ap->outmsg.code = API_VARREAD;
        ap->outmsg.retcode = XB_OK;
        ap->outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
        put_reply(ap);
        var_read_rest(ap, varp);
}

static void  reply_varfind(struct api_status *ap)
{
        netid_t nid = ext2int_netid_t(ap->inmsg.un.varfind.netid);
        ULONG   flags = ntohl(ap->inmsg.un.varfind.flags);
        vhash_t vp;
        Btvar   *varp;
        ULONG   Seq;
        char    varname[BTV_NAME+1];

        pullin(ap, varname, sizeof(varname));

        rvarfile(1);

        if  ((vp = lookupvar(varname, nid, BTM_READ, &Seq)) < 0)  {
                err_result(ap, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
                return;
        }
        varp = &Var_seg.vlist[vp].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))  {
                err_result(ap, XB_UNKNOWN_VAR, Var_seg.dptr->vs_serial);
                return;
        }
        ap->outmsg.code = ap->inmsg.code;
        ap->outmsg.retcode = 0;
        ap->outmsg.un.r_find.seq = htonl(Var_seg.dptr->vs_serial);
        ap->outmsg.un.r_find.slotno = htonl((LONG) vp);
        put_reply(ap);
        if  (ap->inmsg.code == API_FINDVAR)
                var_read_rest(ap, varp);
}

static int reply_jobdel(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btjob           *jp;

        rjobfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XB_SEQUENCE;

        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XB_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (check_valid_job(ap, flags, jp))  {
                if  (!mpermitted(&jp->h.bj_mode, BTM_DELETE, 0))
                        return  XB_NOPERM;
                return  wjimsg(J_DELETE, jp, 0);
        }
        return  XB_UNKNOWN_JOB;
}

static int  reply_vardel(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar   *varp;

        rvarfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        if  (!mpermitted(&varp->var_mode, BTM_DELETE, 0))
                return  XB_NOPERM;

        return  wvmsg(V_DELETE, varp, varp->var_sequence, 0);
}

static int  reply_jobop(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.jop.slotno);
        ULONG  seq = ntohl(ap->inmsg.un.jop.seq);
        ULONG  flags = ntohl(ap->inmsg.un.jop.flags);
        ULONG  op = ntohl(ap->inmsg.un.jop.op);
        ULONG  param = ntohl(ap->inmsg.un.jop.param);
        Btjob   *jp, *djp;
        ULONG   xindx, nop;

        rjobfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XB_SEQUENCE;

        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XB_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (jp->h.bj_job == 0  ||
             !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
                return  XB_UNKNOWN_JOB;

        if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                return  XB_UNKNOWN_JOB;

        /* We might have to do different messages depending upon
           whether we are setting progress etc or killing.
           Likewise we may need to access different permissions.  */

        switch  (op)  {
        default:
                return  XB_UNKNOWN_COMMAND;

        case  XB_JOP_SETRUN:
                nop = BJP_NONE;
                goto  setrest;
        case  XB_JOP_SETCANC:
                nop = BJP_CANCELLED;
                goto  setrest;
        case  XB_JOP_SETDONE:
                nop = BJP_DONE;
        setrest:

                /* Run/cancelled/done - we set the progress code in the job
                   structure.  Maybe we ought to make this a
                   separate shreq code sometime if we do it often enough?  */

                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, 0))
                        return  XB_NOPERM;
                if  (jp->h.bj_progress == nop) /* Already set to that, go away */
                        return  XB_OK;
                if  (jp->h.bj_progress >= BJP_STARTUP1)
                        return  XB_ISRUNNING;
                djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
                *djp = *jp;
                djp->h.bj_progress = (unsigned char) nop;
                return  wjmsg(J_CHANGE, xindx);

        case  XB_JOP_FORCE:
        case  XB_JOP_FORCEADV:
                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE|BTM_KILL, 0))
                        return  XB_NOPERM;
                if  (jp->h.bj_progress >= BJP_STARTUP1)
                        return  XB_ISRUNNING;
                return  wjimsg(op == XB_JOP_FORCE? J_FORCENA: J_FORCE, jp, 0);

        case  XB_JOP_ADVTIME:
                if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, 0))
                        return  XB_NOPERM;
                if  (!jp->h.bj_times.tc_istime)
                        return  XB_NOTIMETOA;
                if  (jp->h.bj_progress >= BJP_STARTUP1)
                        return  XB_ISRUNNING;
                djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
                *djp = *jp;
                djp->h.bj_times.tc_nexttime = advtime(&djp->h.bj_times);
                return  wjmsg(J_CHANGE, xindx);

        case  XB_JOP_KILL:
                if  (!mpermitted(&jp->h.bj_mode, BTM_KILL, 0))
                        return  XB_NOPERM;
                if  (jp->h.bj_progress < BJP_STARTUP1)
                        return  XB_ISNOTRUNNING;
                return  wjimsg(J_KILL, jp, param);
        }
}

static void  api_jobstart(struct api_status *ap)
{
        int                     ret;
        unsigned                length;
        struct  pend_job        *pj;
        struct  jobnetmsg       injob;

        ap->outmsg.code = API_JOBADD;
        ap->outmsg.retcode = XB_OK;

        /* Length is variable as all the space may not be filled up
           Read it in even if we intend to reject it as the
           socket will have gunge in it otherwise.  */

        pullin(ap, (char *) &injob.hdr, sizeof(injob.hdr));
        length = ntohs(injob.hdr.hdr.length);
        pullin(ap, sizeof(injob.hdr) + (char *) &injob, length - sizeof(injob.hdr));

        /* If he can't create new entries then he can visit his handy
           local taxidermist */

        if  (!(ap->hispriv.btu_priv & BTM_CREATE))  {
                ap->outmsg.retcode = htons(XB_NOCRPERM);
                put_reply(ap);
                return;
        }

        /* Allocate a pending job structure */

        if  (!(pj = add_pend(ap->hostid)))  {
                ap->outmsg.retcode = htons(XB_NOMEM_QF);
                put_reply(ap);
                return;
        }

        /* Unpack the job */

        swapinj(&pj->jobout, &injob);
        pj->jobout.h.bj_jflags &= ~(BJ_CLIENTJOB|BJ_ROAMUSER);
        if  (ap->is_logged != LOGGED_IN_UNIX)  {
               pj->jobout.h.bj_jflags |= BJ_CLIENTJOB;
               if  (ap->is_logged == LOGGED_IN_WINU)
                       pj->jobout.h.bj_jflags |= BJ_ROAMUSER;
        }
        pj->jobout.h.bj_orighostid = ap->hostid;
#ifdef HAVE_TO_COPY_UNAME
        pj->jobout.h.bj_mode.o_uid = pj->jobout.h.bj_mode.c_uid = ap->realuid;
        pj->jobout.h.bj_mode.o_gid = pj->jobout.h.bj_mode.c_gid = ap->realgid;
        strncpy(pj->jobout.h.bj_mode.o_user, prin_uname(ap->realuid), UIDSIZE);
        strncpy(pj->jobout.h.bj_mode.c_user, pj->jobout.h.bj_mode.o_user, UIDSIZE);
        strncpy(pj->jobout.h.bj_mode.o_group, prin_gname(ap->realgid), UIDSIZE);
        strncpy(pj->jobout.h.bj_mode.c_group, pj->jobout.h.bj_mode.o_group, UIDSIZE);
#endif

        /* Don't bother with user ids in job modes as they get stuffed
           in by the scheduler from the shreq structure.  */

        if  (validate_ci(pj->jobout.h.bj_cmdinterp) < 0)  {
                ap->outmsg.retcode = htons(XB_BAD_CI);
                put_reply(ap);
                abort_job(pj);
                return;
        }

        if  ((ret = validate_job(&pj->jobout, &ap->hispriv)) != 0)  {
                ap->outmsg.retcode = htons((SHORT) XB_CONVERT_XBNR(ret));
                put_reply(ap);
                abort_job(pj);
                return;
        }
        pj->jobn = ntohl(ap->inmsg.un.jobdata.jobno);
        pj->out_f = goutfile(&pj->jobn, pj->tmpfl, 0);
        pj->jobout.h.bj_job = pj->jobn;
        ap->outmsg.un.jobdata.jobno = htonl(pj->jobn);
        put_reply(ap);
}

/* Next lump of a job.  */

static void  api_jobcont(struct api_status *ap)
{
        USHORT                  nbytes = ntohs(ap->inmsg.un.jobdata.nbytes);
        unsigned                cnt;
        unsigned        char    *bp;
        struct  pend_job        *pj;
        char    inbuffer[XBA_BUFFSIZE]; /* XBA_BUFFSIZE always >= nbytes */

        pullin(ap, inbuffer, nbytes);
        if  (!(pj = find_j_by_jno(ntohl(ap->inmsg.un.jobdata.jobno))))
                return;
        bp = (unsigned char *) inbuffer;
        for  (cnt = 0;  cnt < nbytes;  cnt++)  {
                if  (putc(*bp, pj->out_f) == EOF)  {
                        abort_job(pj);
                        return;
                }
                bp++;
        }
}

/* Final lump of job */

static void  api_jobfinish(struct api_status *ap)
{
        jobno_t                 jobno = ntohl(ap->inmsg.un.jobdata.jobno);
        struct  pend_job        *pj;

        ap->outmsg.code = API_DATAEND;
        ap->outmsg.retcode = XB_OK;
        ap->outmsg.un.jobdata.jobno = htonl(jobno);

        if  (!(pj = find_j_by_jno(jobno)))
                ap->outmsg.retcode = htons(XB_UNKNOWN_JOB);
        else  {
                int     ret;
                ULONG   xindx;
                Btjob  *djp;
                djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
                BLOCK_COPY(djp, &pj->jobout, sizeof(Btjob));
                djp->h.bj_slotno = -1;
                time(&djp->h.bj_time);
                ret = wjmsg(J_CREATE, xindx);
                ap->outmsg.retcode = htons((SHORT) ret);
                fclose(pj->out_f);
                pj->out_f = (FILE *) 0;
                if  (ret != XB_OK)  {
                        unlink(pj->tmpfl);
                        ap->outmsg.retcode = htons(ret);
                }
        }
        ap->outmsg.un.jobdata.seq = htonl(Job_seg.dptr->js_serial);
        put_reply(ap);
}

/* Abort a job.
   I think that we'll be extremely lucky if this routine
   ever gets called as application programs don't have a habit of
   politely telling us 'ere they crash in a spectacular heap,
   well ones I've seen anyhow (even ones I write).  */

static void  api_jobabort(struct api_status *ap)
{
        jobno_t         jobno = ntohl(ap->inmsg.un.jobdata.jobno);
        struct  pend_job        *pj;

        ap->outmsg.code = API_DATAABORT;
        ap->outmsg.retcode = XB_OK;
        ap->outmsg.un.jobdata.jobno = htonl(jobno);

        if  (!(pj = find_j_by_jno(jobno)))
                ap->outmsg.retcode = htons(XB_UNKNOWN_JOB);
        else
                abort_job(pj);
        put_reply(ap);
}

/* Add a groovy new variable */

static int  reply_varadd(struct api_status *ap)
{
        struct  varnetmsg       invar;
        Btvar   rvar;

        pullin(ap, (char *) &invar, sizeof(invar));
        BLOCK_ZERO(&rvar, sizeof(rvar));

        rvar.var_type = invar.nm_type;
        rvar.var_flags = invar.nm_flags;
        strncpy(rvar.var_name, invar.nm_name, BTV_NAME);
        strncpy(rvar.var_comment, invar.nm_comment, BTV_COMMENT);
        rvar.var_mode.u_flags = ntohs(invar.nm_mode.u_flags);
        rvar.var_mode.g_flags = ntohs(invar.nm_mode.g_flags);
        rvar.var_mode.o_flags = ntohs(invar.nm_mode.o_flags);
        if  ((rvar.var_value.const_type = invar.nm_consttype) == CON_STRING)
                strncpy(rvar.var_value.con_un.con_string, invar.nm_un.nm_string, BTC_VALUE);
        else
                rvar.var_value.con_un.con_long = ntohl(invar.nm_un.nm_long);

        if  ((ap->hispriv.btu_priv & BTM_CREATE) == 0)
                return  XBNR_NOCRPERM;

        if  (!(ap->hispriv.btu_priv & BTM_UMASK)  &&
             (rvar.var_mode.u_flags != ap->hispriv.btu_vflags[0] ||
              rvar.var_mode.g_flags != ap->hispriv.btu_vflags[1] ||
              rvar.var_mode.o_flags != ap->hispriv.btu_vflags[2]))
                return  XBNR_NOCMODE;

        return  wvmsg(V_CREATE, &rvar, 0, 0);
}

static int  reply_jobupd(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        int             cinum;
        unsigned        length;
        ULONG           xindx;
        Btjob           *jp, *djp;
        struct  jobnetmsg       injob;
        Btjob                   rjob;

        /* Length is variable as per jobadd.  */

        pullin(ap, (char *) &injob.hdr, sizeof(injob.hdr));
        length = ntohs(injob.hdr.hdr.length);
        pullin(ap, sizeof(injob.hdr) + (char *) &injob, length - sizeof(injob.hdr));
        swapinj(&rjob, &injob);

        rjobfile(1);
        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XB_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (jp->h.bj_job == 0  ||  !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
                return  XB_UNKNOWN_JOB;

        if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                return  XB_UNKNOWN_JOB;

        if  (!mpermitted(&jp->h.bj_mode, BTM_WRITE, 0))
                return  XB_NOPERM;

        if  (jp->h.bj_progress >= BJP_STARTUP1)
                return  XB_ISRUNNING;

        /* The scheduler ignores attempts to change the mode and
           ownership etc so we won't bother to look.  We will
           validate the priority and load levels though.  */

        if  (rjob.h.bj_pri < ap->hispriv.btu_minp  ||  rjob.h.bj_pri > ap->hispriv.btu_maxp)
                return  XB_BAD_PRIORITY;

        if  ((cinum = validate_ci(rjob.h.bj_cmdinterp)) < 0)
                return  XB_BAD_CI;

        /* Validate load level */

        if  (rjob.h.bj_ll == 0  ||  rjob.h.bj_ll > ap->hispriv.btu_maxll)
                return  XBNR_BAD_LL;
        if  (!(ap->hispriv.btu_priv & BTM_SPCREATE) && rjob.h.bj_ll != Ci_list[cinum].ci_ll)
                return  XBNR_BAD_LL;

        /* Copy across bits which the scheduler uses to identify the job */

        rjob.h.bj_slotno = jp->h.bj_slotno;
        rjob.h.bj_hostid = jp->h.bj_hostid;
        rjob.h.bj_job = jp->h.bj_job;

        djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
        BLOCK_COPY(djp, &rjob, sizeof(Btjob));
        return  wjmsg(J_CHANGE, xindx);
}

static int  reply_varupd(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        unsigned        wflags = BTM_WRITE;
        Btvar           *varp, rvar;
        struct  varnetmsg  invar;
        ULONG           Saveseq;

        pullin(ap, (char *) &invar, sizeof(invar));
        BLOCK_ZERO(&rvar, sizeof(rvar));
        rvarfile(1);
        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;

        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;

        rvar = *varp;
        rvar.var_flags = invar.nm_flags & (VF_EXPORT|VF_CLUSTER);
        if  ((rvar.var_value.const_type = invar.nm_consttype) == CON_STRING)
                strncpy(rvar.var_value.con_un.con_string, invar.nm_un.nm_string, BTC_VALUE);
        else
                rvar.var_value.con_un.con_long = ntohl(invar.nm_un.nm_long);

        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        if  ((varp->var_flags ^ rvar.var_flags) & (VF_EXPORT|VF_CLUSTER))
                wflags |= BTM_DELETE;
        if  (rvar.var_value.const_type == varp->var_value.const_type)  {
                if  (rvar.var_value.const_type == CON_LONG)  {
                        if  (rvar.var_value.con_un.con_long == varp->var_value.con_un.con_long)
                                wflags &= ~BTM_WRITE;
                }
                else  if  (strcmp(rvar.var_value.con_un.con_string, varp->var_value.con_un.con_string) == 0)
                        wflags &= ~BTM_WRITE;
        }

        if  (wflags == 0)       /* Nothing doing forget it */
                return  XB_OK;

        if  (!mpermitted(&varp->var_mode, wflags, 0))
                return  XB_NOPERM;

        Saveseq = varp->var_sequence;
        if  (wflags & BTM_DELETE)  {
                int     ret = wvmsg(V_CHFLAGS, &rvar, Saveseq, 0);
                if  (ret != XB_OK)
                        return  ret;
                Saveseq++;
        }
        if  (wflags & BTM_WRITE)
                return  wvmsg(V_ASSIGN, &rvar, Saveseq, 0);
        return  XB_OK;
}

static int  reply_varchcomm(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar           *varp, rvar;
        struct  varnetmsg  invar;

        pullin(ap, (char *) &invar, sizeof(invar));
        rvarfile(1);
        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        if  (!mpermitted(&varp->var_mode, BTM_WRITE, 0))
                return  XB_NOPERM;

        rvar = *varp;
        strcpy(rvar.var_comment, invar.nm_comment);
        return  wvmsg(V_CHCOMM, &rvar, varp->var_sequence, 0);
}

static int  reply_varrename(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar   *varp;
        char    nbuf[BTV_NAME+1];
        Shipc   Oreq;

        pullin(ap, (char *) nbuf, sizeof(nbuf));
        rvarfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        if  (!mpermitted(&varp->var_mode, BTM_DELETE, 0))
                return  XB_NOPERM;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = V_NEWNAME;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_un.sh_rn.sh_ovar = *varp;
        Oreq.sh_un.sh_rn.sh_ovar.var_sequence = varp->var_sequence;
        strcpy(Oreq.sh_un.sh_rn.sh_rnewname, nbuf);
        msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar) + strlen(nbuf) + 1, 0);
        return readvreply();
}

static void  api_jobdata(struct api_status *ap)
{
        slotno_t  slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG     seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG     flags = ntohl(ap->inmsg.un.reader.flags);
        int     inbp, ch;
        FILE    *jfile;
        Btjob   *jp;
        char    buffer[XBA_BUFFSIZE];

        ap->outmsg.code = API_JOBDATA;
        ap->outmsg.retcode = XB_OK;

        rjobfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)  {
                ap->outmsg.retcode = htons(XB_SEQUENCE);
                put_reply(ap);
                return;
        }
        if  (slotno >= Job_seg.dptr->js_maxjobs)  {
                ap->outmsg.retcode = htons(XB_INVALIDSLOT);
                put_reply(ap);
                return;
        }

        jp = &Job_seg.jlist[slotno].j;

        if  (jp->h.bj_job == 0  ||  !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))  {
                ap->outmsg.retcode = htons(XB_UNKNOWN_JOB);
                put_reply(ap);
                return;
        }

        if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))  {
                ap->outmsg.retcode = htons(XB_UNKNOWN_JOB);
                put_reply(ap);
                return;
        }

        if  (!mpermitted(&jp->h.bj_mode, BTM_READ, 0))  {
                ap->outmsg.retcode = htons(XB_NOPERM);
                put_reply(ap);
                return;
        }

        jfile = jp->h.bj_hostid? net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job, Job_seg.dptr->js_viewport): fopen(mkspid(SPNAM, jp->h.bj_job), "r");
        if  (!jfile)  {
                ap->outmsg.retcode = htons(XB_UNKNOWN_JOB);
                put_reply(ap);
                return;
        }

        /* Say ok */

        put_reply(ap);

        /* Read the file and splat it out.  */

        ap->outmsg.code = API_DATAOUT;
        ap->outmsg.un.jobdata.jobno = htonl(jp->h.bj_job);
        inbp = 0;

        while  ((ch = getc(jfile)) != EOF)  {
                buffer[inbp++] = (char) ch;
                if  (inbp >= sizeof(buffer))  {
                        ap->outmsg.un.jobdata.nbytes = htons(inbp);
                        put_reply(ap);
                        pushout(ap, buffer, inbp);
                        inbp = 0;
                }
        }

        fclose(jfile);
        if  (inbp > 0)  {
                ap->outmsg.un.jobdata.nbytes = htons(inbp);
                put_reply(ap);
                pushout(ap, buffer, inbp);
                inbp = 0;
        }

        /* Mark end of data */

        ap->outmsg.code = API_DATAEND;
        put_reply(ap);
}

static int  reply_jobchmod(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btjob           *jp, *djp;
        ULONG           xindx;
        struct  jobhnetmsg      injob;

        pullin(ap, (char *) &injob, sizeof(injob));

        rjobfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XB_SEQUENCE;

        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XB_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (jp->h.bj_job == 0  ||
             !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
                return  XB_UNKNOWN_JOB;

        if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                return  XB_UNKNOWN_JOB;

        if  (!mpermitted(&jp->h.bj_mode, BTM_WRMODE, 0))
                return  XB_NOPERM;

        djp = &Xbuffer->Ring[xindx = getxbuf_serv()];
        *djp = *jp;
        djp->h.bj_mode.u_flags = ntohs(injob.nm_mode.u_flags);
        djp->h.bj_mode.g_flags = ntohs(injob.nm_mode.g_flags);
        djp->h.bj_mode.o_flags = ntohs(injob.nm_mode.o_flags);
        return  wjmsg(J_CHMOD, xindx);
}

static int  reply_varchmod(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar           *varp, rvar;
        struct  varnetmsg       invar;

        pullin(ap, (char *) &invar, sizeof(invar));

        rvarfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        if  (!mpermitted(&varp->var_mode, BTM_WRMODE, 0))
                return  XB_NOPERM;

        rvar = *varp;
        rvar.var_mode.u_flags = ntohs(invar.nm_mode.u_flags);
        rvar.var_mode.g_flags = ntohs(invar.nm_mode.g_flags);
        rvar.var_mode.o_flags = ntohs(invar.nm_mode.o_flags);
        return  wvmsg(V_CHMOD, &rvar, rvar.var_sequence, 0);
}

static int  reply_jobchown(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btjob           *jp;
        int_ugid_t      nuid;
        struct  jugmsg  injob;

        pullin(ap, (char *) &injob, sizeof(injob));

        if  ((nuid = lookup_uname(injob.newug)) == UNKNOWN_UID)
                return  XB_UNKNOWN_USER;

        rjobfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XB_SEQUENCE;

        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XB_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (jp->h.bj_job == 0  ||
             !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
                return  XB_UNKNOWN_JOB;

        if  (flags & XB_FLAG_QUEUEONLY && notinqueue(ap, jp))
                return  XB_UNKNOWN_JOB;

        return  wjimsg(J_CHOWN, jp, (ULONG) nuid);
}

static int  reply_varchown(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar           *varp;
        int_ugid_t      nuid;
        struct  vugmsg  invar;

        pullin(ap, (char *) &invar, sizeof(invar));
        if  ((nuid = lookup_uname(invar.newug)) == UNKNOWN_UID)
                return  XB_UNKNOWN_USER;

        rvarfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        return  wvmsg(V_CHOWN, varp, varp->var_sequence, (ULONG) nuid);
}

static int  reply_jobchgrp(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btjob           *jp;
        int_ugid_t      ngid;
        struct  jugmsg  injob;

        pullin(ap, (char *) &injob, sizeof(injob));
        if  ((ngid = lookup_gname(injob.newug)) == UNKNOWN_GID)
                return  XB_UNKNOWN_GROUP;

        rjobfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Job_seg.dptr->js_serial)
                return  XB_SEQUENCE;

        if  (slotno >= Job_seg.dptr->js_maxjobs)
                return  XB_INVALIDSLOT;

        jp = &Job_seg.jlist[slotno].j;
        if  (jp->h.bj_job == 0  ||
             !mpermitted(&jp->h.bj_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  jp->h.bj_hostid != 0)  ||
             ((flags & XB_FLAG_USERONLY)  &&  jp->h.bj_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  jp->h.bj_mode.o_gid != Realgid))
                return  XB_UNKNOWN_JOB;

        if  (flags & XB_FLAG_QUEUEONLY  && notinqueue(ap, jp))
                return  XB_UNKNOWN_JOB;

        return  wjimsg(J_CHGRP, jp, (ULONG) ngid);
}

static int  reply_varchgrp(struct api_status *ap)
{
        slotno_t        slotno = ntohl(ap->inmsg.un.reader.slotno);
        ULONG           seq = ntohl(ap->inmsg.un.reader.seq);
        ULONG           flags = ntohl(ap->inmsg.un.reader.flags);
        Btvar           *varp;
        int_ugid_t      ngid;
        struct  vugmsg  invar;

        pullin(ap, (char *) &invar, sizeof(invar));
        if  ((ngid = lookup_gname(invar.newug)) == UNKNOWN_GID)
                return  XB_UNKNOWN_GROUP;

        rvarfile(1);

        if  (!(flags & XB_FLAG_IGNORESEQ)  &&  seq != Var_seg.dptr->vs_serial)
                return  XB_SEQUENCE;
        if  (slotno >= Var_seg.dptr->vs_maxvars)
                return  XB_INVALIDSLOT;
        if  (!Var_seg.vlist[slotno].Vused)
                return  XB_UNKNOWN_VAR;

        varp = &Var_seg.vlist[slotno].Vent;
        if  (!mpermitted(&varp->var_mode, BTM_SHOW, 0)  ||
             ((flags & XB_FLAG_LOCALONLY)  &&  varp->var_id.hostid != 0) ||
             ((flags & XB_FLAG_USERONLY)  &&  varp->var_mode.o_uid != Realuid)  ||
             ((flags & XB_FLAG_GROUPONLY)  &&  varp->var_mode.o_gid != Realgid))
                return  XB_UNKNOWN_VAR;

        return  wvmsg(V_CHGRP, varp, varp->var_sequence, (ULONG) ngid);
}

static int  reply_ciadd(struct api_status *ap)
{
        unsigned        nsel;
        Cmdint          rci, inci;

        pullin(ap, (char *) &inci, sizeof(inci));

        if  (!(ap->hispriv.btu_priv & BTM_SPCREATE))
                return  XB_NOPERM;

        BLOCK_ZERO(&rci, sizeof(rci));
        rci.ci_ll = ntohs(inci.ci_ll);
        rci.ci_nice = inci.ci_nice;
        rci.ci_flags = inci.ci_flags;
        strncpy(rci.ci_name, inci.ci_name, CI_MAXNAME);
        strncpy(rci.ci_path, inci.ci_path, CI_MAXFPATH);
        strncpy(rci.ci_args, inci.ci_args, CI_MAXARGS);
        if  (rci.ci_name[0] == '\0' || rci.ci_path[0] == '\0')
                return  XB_BAD_CI;

        if  (rci.ci_ll == 0)
                rci.ci_ll = ap->hispriv.btu_spec_ll;

        if  (validate_ci(rci.ci_name) == 0)
                return  XB_BAD_CI;
        for  (nsel = 0;  nsel < Ci_num;  nsel++)
                if  (Ci_list[nsel].ci_name[0] == '\0')
                        goto  dun;
        Ci_num++;
        if  (!(Ci_list = (CmdintRef) realloc((char *) Ci_list, (unsigned) (Ci_num * sizeof(Cmdint)))))
                ABORT_NOMEM;
 dun:
        lseek(Ci_fd, (long) (nsel * sizeof(Cmdint)), 0);
        Ignored_error = write(Ci_fd, (char *) &rci, sizeof(rci));
        Ci_list[nsel] = rci;
        ap->outmsg.un.r_reader.seq = htonl(nsel);
        return  XB_OK;
}

#ifndef WORDS_BIGENDIAN
#define CIBLOCKSIZE     10
#endif

static void  reply_ciread(struct api_status *ap)
{
        open_ci(O_RDWR);
        ap->outmsg.code = API_CIREAD;
        ap->outmsg.retcode = XB_OK;
        ap->outmsg.un.r_lister.nitems = htonl((ULONG) Ci_num);
        ap->outmsg.un.r_lister.seq = 0;
        put_reply(ap);
#ifdef WORDS_BIGENDIAN
        if  (Ci_num > 0)
                pushout(ap, (char *) Ci_list, (unsigned) (Ci_num * sizeof(Cmdint)));
#else
        if  (Ci_num > 0)  {
                Cmdint  ciblk[CIBLOCKSIZE];
                Cmdint  *fp = Ci_list, *tp;
                Cmdint  *fe = &Ci_list[Ci_num], *te = &ciblk[CIBLOCKSIZE];
                do  {
                        tp = ciblk;
                        do  {
                                *tp = *fp;
                                tp->ci_ll = htons(fp->ci_ll);
                                tp++;   fp++;
                        }  while  (fp < fe  &&  tp < te);

                        pushout(ap, (char *) ciblk, (char *) tp - (char *)ciblk);
                }  while  (fp < fe);
        }
#endif
}

static int  reply_ciupd(struct api_status *ap)
{
        unsigned        slot = ntohl(ap->inmsg.un.reader.slotno);
        unsigned        cnt;
        Cmdint          rci, inci;

        pullin(ap, (char *) &inci, sizeof(inci));
        if  (!(ap->hispriv.btu_priv & BTM_SPCREATE))
                return  XB_NOPERM;
        BLOCK_ZERO(&rci, sizeof(rci));
        rci.ci_ll = ntohs(inci.ci_ll);
        rci.ci_nice = inci.ci_nice;
        rci.ci_flags = inci.ci_flags;
        strncpy(rci.ci_name, inci.ci_name, CI_MAXNAME);
        strncpy(rci.ci_path, inci.ci_path, CI_MAXFPATH);
        strncpy(rci.ci_args, inci.ci_args, CI_MAXARGS);

        if  (rci.ci_name[0] == '\0' || rci.ci_path[0] == '\0'  || slot >= Ci_num  ||  Ci_list[slot].ci_name[0] == '\0')
                return  XB_BAD_CI;

        rereadcif();

        for  (cnt = 0;  cnt < Ci_num;  cnt++)
                if  (cnt != slot  &&  strcmp(Ci_list[cnt].ci_name, rci.ci_name) == 0)
                        return  XB_BAD_CI;

        if  (rci.ci_ll == 0)
                rci.ci_ll = ap->hispriv.btu_spec_ll;

        lseek(Ci_fd, (long) (slot * sizeof(Cmdint)), 0);
        Ignored_error = write(Ci_fd, (char *) &rci, sizeof(rci));
        Ci_list[slot] = rci;
        return  XB_OK;
}

static int  reply_cidel(struct api_status *ap)
{
        unsigned        slot = ntohl(ap->inmsg.un.reader.slotno);

        if  (!(ap->hispriv.btu_priv & BTM_SPCREATE))
                return  XB_NOPERM;

        rereadcif();
        if  (slot == CI_STDSHELL  ||  slot >= Ci_num  ||  Ci_list[slot].ci_name[0] == '\0')
                return  XB_BAD_CI;
        Ci_list[slot].ci_name[0] = '\0';
        lseek(Ci_fd, (long) (slot * sizeof(Cmdint)), 0);
        Ignored_error = write(Ci_fd, (char *) &Ci_list[slot], sizeof(Cmdint));
        return  XB_OK;
}

static void  reply_holread(struct api_status *ap)
{
        unsigned  year = ntohl(ap->inmsg.un.reader.slotno);
        char    rep[YVECSIZE];

        BLOCK_ZERO(rep, sizeof(rep));
        ap->outmsg.retcode = XB_OK;
        if  (year >= 1990  &&  year < 2100)  {
                get_hf(year-1990, rep);
                put_reply(ap);
                pushout(ap, rep, sizeof(rep));
        }
        else  {
                ap->outmsg.retcode = htons(XB_INVALID_YEAR);
                put_reply(ap);
        }
}

static int  reply_holupd(struct api_status *ap)
{
        unsigned  year = ntohl(ap->inmsg.un.reader.slotno);
        char    inyear[YVECSIZE];
        pullin(ap, inyear, YVECSIZE);
        if  (year < 1990  ||  year >= 2100)
                return  XB_INVALID_YEAR;
        /* Don't we want to check this guy can do it??? */
        put_hf(year-1990, inyear);
        return  XB_OK;
}

static int  reply_setqueue(struct api_status *ap)
{
        unsigned  length = ntohs(ap->inmsg.un.queuelength);

        if  (length == 0)  {
                if  (ap->current_queue)  {
                        free(ap->current_queue);
                        ap->current_queue = (char *) 0;
                }
                ap->current_qlen = 0;
        }
        else  {
                char  *newqueue = malloc(length); /* Includes final null */
                if  (!newqueue)  {
                        unsigned  nbytes = length;
                        char    stuff[100];

                        /* Slurp up stuff from socket */

                        while  (nbytes != 0)  {
                                int     nb = nbytes < sizeof(stuff)? nbytes: sizeof(stuff);
                                pullin(ap, stuff, (unsigned) nb);
                                nbytes -= nb;
                        }
                        return  XB_NOMEMQ;
                }
                pullin(ap, newqueue, length);
                if  (ap->current_queue)
                        free(ap->current_queue);
                ap->current_queue = newqueue;
                ap->current_qlen = length - 1;
        }
        return  XB_OK;
}

static  void    reply_sendenv(struct api_status *ap)
{
        char    **ep;
        ULONG   ecount = 0;
        unsigned  maxlng = 0;
        extern  char    **xenviron;

        for  (ep = xenviron;  *ep;  ep++)  {
                unsigned  lng = strlen(*ep);
                ecount++;
                if  (lng > maxlng)
                        maxlng = lng;
        }

        ap->outmsg.retcode = XB_OK;
        ap->outmsg.un.r_lister.nitems = htonl(ecount);
        ap->outmsg.un.r_lister.seq = htonl((ULONG) maxlng);
        put_reply(ap);
        for  (ep = xenviron;  *ep;  ep++)  {
                unsigned  lng = strlen(*ep);
                ULONG   le = htonl((ULONG) lng);
                pushout(ap, (char *) &le, sizeof(le));
                pushout(ap, *ep, lng + 1);
        }
}

static  void    reply_getbtu(struct api_status *ap)
{
        int_ugid_t      ouid = ap->realuid, ogid = ap->realgid;
        BtuserRef       mpriv;
        struct  ua_reply  outpriv;

        if  (ap->inmsg.un.us.username[0])  {
                if  ((ouid = lookup_uname(ap->inmsg.un.us.username)) == UNKNOWN_UID)  {
                        ap->outmsg.retcode = htons(XB_UNKNOWN_USER);
                        put_reply(ap);
                        return;
                }
                if  (ouid != Realuid  &&  !(ap->hispriv.btu_priv & BTM_RADMIN))  {
                        ap->outmsg.retcode = htons(XB_NOPERM);
                        put_reply(ap);
                        return;
                }
                ogid = lastgid;
        }

        /* Still re-read it in case something changed */

        mpriv = getbtuentry(ouid);
        if  (ouid == Realuid)
                ap->hispriv = *mpriv;
        btuser_pack(&outpriv.ua_perm, mpriv);
        strcpy(outpriv.ua_uname, prin_uname((uid_t) ouid));
        strcpy(outpriv.ua_gname, prin_gname((gid_t) ogid));
        ap->outmsg.retcode = XB_OK;
        put_reply(ap);
        pushout(ap, (char *) &outpriv, sizeof(outpriv));
}

static  void    reply_getbtd(struct api_status *ap)
{
        int     cnt;
        Btdef   outbthdr;

        BLOCK_ZERO(&outbthdr, sizeof(outbthdr));
        outbthdr.btd_version = Btuhdr.btd_version;
        outbthdr.btd_minp = Btuhdr.btd_minp;
        outbthdr.btd_maxp = Btuhdr.btd_maxp;
        outbthdr.btd_defp = Btuhdr.btd_defp;
        outbthdr.btd_maxll = htons(Btuhdr.btd_maxll);
        outbthdr.btd_totll = htons(Btuhdr.btd_totll);
        outbthdr.btd_spec_ll = htons(Btuhdr.btd_spec_ll);
        outbthdr.btd_priv = htonl(Btuhdr.btd_priv);
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                outbthdr.btd_jflags[cnt] = htons(Btuhdr.btd_jflags[cnt]);
                outbthdr.btd_vflags[cnt] = htons(Btuhdr.btd_vflags[cnt]);
        }
        ap->outmsg.retcode = XB_OK;
        put_reply(ap);
        pushout(ap, (char *) &outbthdr, sizeof(outbthdr));
}

static  int     reply_putbtu(struct api_status *ap)
{
        int_ugid_t      ouid = ap->realuid;
        Btuser          rbtu;
        BtuserRef       mpriv;
        int             cnt;
        struct  ua_reply        buf;

        pullin(ap, (char *) &buf, sizeof(buf));

        if  (ap->inmsg.un.us.username[0])  {
                if  ((ouid = lookup_uname(ap->inmsg.un.us.username)) == UNKNOWN_UID)
                        return  XB_UNKNOWN_USER;

                /* Disallow if different uid and no WADMIN or in any case if no UMASK priv */

                if  ((ouid != Realuid  && !(ap->hispriv.btu_priv & BTM_WADMIN)) || !(ap->hispriv.btu_priv & (BTM_UMASK|BTM_WADMIN)))
                        return  XB_NOPERM;
        }

        rbtu.btu_user = ouid;
        rbtu.btu_minp = buf.ua_perm.btu_minp;
        rbtu.btu_maxp = buf.ua_perm.btu_maxp;
        rbtu.btu_defp = buf.ua_perm.btu_defp;
        rbtu.btu_maxll = ntohs(buf.ua_perm.btu_maxll);
        rbtu.btu_totll = ntohs(buf.ua_perm.btu_totll);
        rbtu.btu_spec_ll = ntohs(buf.ua_perm.btu_spec_ll);
        rbtu.btu_priv = ntohl(buf.ua_perm.btu_priv);

        for  (cnt = 0;  cnt < 3;  cnt++)  {
                rbtu.btu_jflags[cnt] = ntohs(buf.ua_perm.btu_jflags[cnt]);
                rbtu.btu_vflags[cnt] = ntohs(buf.ua_perm.btu_vflags[cnt]);
        }

        if  (rbtu.btu_minp == 0 || rbtu.btu_maxp == 0 || rbtu.btu_defp == 0)
                return  XB_BAD_PRIORITY;

        if  (rbtu.btu_maxll == 0 || rbtu.btu_totll == 0 || rbtu.btu_spec_ll == 0)
                return  XB_BAD_LL;

        /* In case something has changed */

        if  (ouid == Realuid)  {
                ap->hispriv = *getbtuentry(ouid);
                if  (!(ap->hispriv.btu_priv & BTM_WADMIN))  {

                        /* Disallow everything except def prio and flags */

                        if  (rbtu.btu_minp != ap->hispriv.btu_minp ||
                             rbtu.btu_maxp != ap->hispriv.btu_maxp  ||
                             rbtu.btu_maxll != ap->hispriv.btu_maxll  ||
                             rbtu.btu_totll != ap->hispriv.btu_totll  ||
                             rbtu.btu_spec_ll != ap->hispriv.btu_spec_ll ||
                             rbtu.btu_priv != ap->hispriv.btu_priv)
                                return  XB_NOPERM;

                        if  (!(ap->hispriv.btu_priv & BTM_UMASK))  {
                                for  (cnt = 0;  cnt < 3;  cnt++)  {
                                        if  (rbtu.btu_jflags[cnt] != ap->hispriv.btu_jflags[cnt])
                                                return  XB_NOPERM;
                                        if  (rbtu.btu_vflags[cnt] != ap->hispriv.btu_vflags[cnt])
                                                return  XB_NOPERM;
                                }
                        }
                }
        }

        mpriv = getbtuentry(ouid);
        mpriv->btu_minp = rbtu.btu_minp;
        mpriv->btu_maxp = rbtu.btu_maxp;
        mpriv->btu_defp = rbtu.btu_defp;
        mpriv->btu_maxll = rbtu.btu_maxll;
        mpriv->btu_totll = rbtu.btu_totll;
        mpriv->btu_spec_ll = rbtu.btu_spec_ll;
        mpriv->btu_priv = rbtu.btu_priv;
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                mpriv->btu_jflags[cnt] = rbtu.btu_jflags[cnt];
                mpriv->btu_vflags[cnt] = rbtu.btu_vflags[cnt];
        }
        putbtuentry(mpriv);
        if  (ouid == Realuid)
                ap->hispriv = *mpriv;
        return  XB_OK;
}

static  int    reply_putbtd(struct api_status *ap)
{
        int     cnt;
        Btdef   inbthdr, rbthdr;

        pullin(ap, (char *) &inbthdr, sizeof(inbthdr));
        if  (!(ap->hispriv.btu_priv & BTM_WADMIN))
                return  XB_NOPERM;

        BLOCK_ZERO(&rbthdr, sizeof(rbthdr));
        rbthdr.btd_minp = inbthdr.btd_minp;
        rbthdr.btd_maxp = inbthdr.btd_maxp;
        rbthdr.btd_defp = inbthdr.btd_defp;
        rbthdr.btd_maxll = ntohs(inbthdr.btd_maxll);
        rbthdr.btd_totll = ntohs(inbthdr.btd_totll);
        rbthdr.btd_spec_ll = ntohs(inbthdr.btd_spec_ll);
        rbthdr.btd_priv = ntohl(inbthdr.btd_priv);
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                rbthdr.btd_jflags[cnt] = ntohs(inbthdr.btd_jflags[cnt]);
                rbthdr.btd_vflags[cnt] = ntohs(inbthdr.btd_vflags[cnt]);
        }
        if  (rbthdr.btd_minp == 0 || rbthdr.btd_maxp == 0 || rbthdr.btd_defp == 0)
                return  XB_BAD_PRIORITY;
        if  (rbthdr.btd_maxll == 0 || rbthdr.btd_totll == 0 || rbthdr.btd_spec_ll == 0)
                return  XB_BAD_LL;

        /* Re-read file to get locking open */

        ap->hispriv = *getbtuentry(Realuid);
        Btuhdr.btd_minp = rbthdr.btd_minp;
        Btuhdr.btd_maxp = rbthdr.btd_maxp;
        Btuhdr.btd_defp = rbthdr.btd_defp;
        Btuhdr.btd_maxll = rbthdr.btd_maxll;
        Btuhdr.btd_totll = rbthdr.btd_totll;
        Btuhdr.btd_spec_ll = rbthdr.btd_spec_ll;
        Btuhdr.btd_priv = rbthdr.btd_priv;
        Btuhdr.btd_version = GNU_BATCH_MAJOR_VERSION;
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                Btuhdr.btd_jflags[cnt] = rbthdr.btd_jflags[cnt];
                Btuhdr.btd_vflags[cnt] = rbthdr.btd_vflags[cnt];
        }
        putbtuhdr();
        return  XB_OK;
}

void  process_pwchk(struct api_status *ap)
{
        char    pwbuf[API_PASSWDSIZE+1];

        ap->outmsg.code = ap->inmsg.code;
        strncpy(ap->outmsg.un.signon.username, ap->inmsg.un.signon.username, WUIDSIZE);
        ap->outmsg.retcode = htons(XB_NO_PASSWD);
        put_reply(ap);
        pullin(ap, pwbuf, sizeof(pwbuf));
        if  (!checkpw(prin_uname(ap->realuid), pwbuf))  {
                err_result(ap, XB_PASSWD_INVALID, 0);
                abort_exit(0);
        }
}

static  void    signon_ok(struct api_status *ap)
{
        int_ugid_t      uuid;

        if  ((uuid = lookup_uname(ap->inmsg.un.signon.username)) == UNKNOWN_UID)  {
                err_result(ap, XB_UNKNOWN_USER, 0);
                abort_exit(0);
        }
        ap->realuid = uuid;
        ap->realgid = lastgid;
        ap->hispriv = *getbtuentry(ap->realuid);
        ap->is_logged = LOGGED_IN_UNIX;
}

static  void  process_wlogin(struct api_status *ap)
{
        struct  winuhash  *wp = lookup_winoruu(ap->inmsg.un.signon.username);

        if  (wp)  {
                ap->realuid = wp->uuid;
                ap->realgid = wp->ugid;
        }
        else  {
                ap->realuid = Defaultuid;
                ap->realgid = Defaultgid;
        }
        process_pwchk(ap);
        ap->hispriv = *getbtuentry(ap->realuid);
        ap->is_logged = LOGGED_IN_WIN;
}

/* This is where we sign on without a password */

static  void  process_signon(struct api_status *ap)
{
        struct  hhash   *hp;
        struct  winuhash  *wp;
        struct  alhash  *alu;

        if  (!ap->hostid)  {            /* Local login, we believe the user name given */
                signon_ok(ap);
                return;
        }

        /* OK - we might be relying on it being a Windows user, or a UNIX-user from another host.
           See if we know the host - if not a Windows machine, accept the connection. */

        hp = lookup_hhash(ap->hostid);
        if  (hp  &&  !hp->isclient)  {
                signon_ok(ap);
                return;
        }
        /* See if we know the Windows user name */

        if  (!(wp = lookup_winoruu(ap->inmsg.un.signon.username)))  {
                err_result(ap, XB_UNKNOWN_USER, 0);
                return;
        }

        /* Check if it's an auto-login user and it matches */

        if  (!(alu = find_autoconn(ap->hostid))  ||  wp->uuid != alu->uuid)  {
                err_result(ap, XB_PASSWD_INVALID, 0);
                return;
        }
        ap->realuid = alu->uuid;
        ap->realgid = alu->ugid;
        ap->hispriv = *getbtuentry(ap->realuid);
        ap->is_logged = LOGGED_IN_WINU;
}

/* Login - presume it's a UNIX source unless we know better */

static  void    process_login(struct api_status *ap)
{
        struct  hhash   *hp;

        if  (!(ap->hostid  &&  (((hp = lookup_hhash(ap->hostid))  &&  hp->isclient)  ||  find_autoconn(ap->hostid))))  {
                /* We think it's a UNIX host */
                int_ugid_t  uuid;

                if  ((uuid = lookup_uname(ap->inmsg.un.signon.username)) == UNKNOWN_UID)  {
                        err_result(ap, XB_UNKNOWN_USER, 0);
                        abort_exit(0);
                }
                ap->realuid = uuid;
                ap->realgid = lastgid;
                process_pwchk(ap);
                ap->hispriv = *getbtuentry(ap->realuid);
                ap->is_logged = LOGGED_IN_UNIX;
        }
        else
                process_wlogin(ap);
}

static  void    process_locallogin(struct api_status *ap)
{
        int_ugid_t  fromuid, touid, lgid;
        Btuser   *mpriv;

        if  (ap->hostid)  {
                err_result(ap, XB_UNKNOWN_USER, 0);
                abort_exit(0);
        }

        fromuid = ntohl(ap->inmsg.un.local_signon.fromuser);
        touid = ntohl(ap->inmsg.un.local_signon.touser);
        if  (!isvuser(fromuid))  {
                err_result(ap, XB_UNKNOWN_USER, 0);
                abort_exit(0);
        }
        lgid = lastgid;         /* We fixed prin_uname to set this */
        if  (fromuid != touid)  {
                if  (!isvuser(touid))  {
                        err_result(ap, XB_UNKNOWN_USER, 0);
                        abort_exit(0);
                }
                lgid = lastgid;
        }
        mpriv = getbtuentry(fromuid);
        if  (fromuid != touid  &&  !(mpriv->btu_priv & BTM_WADMIN))  {
                err_result(ap, XB_NOPERM, 0);
                abort_exit(0);
        }
        ap->realuid = touid;
        ap->realgid = lgid;
        ap->hispriv = *mpriv;
        ap->is_logged = LOGGED_IN_UNIX;
}

void  process_api()
{
        struct  api_status      apistat;
        int             cnt, ret;
        PIDTYPE         pid;

        init_status(&apistat);

        if  ((apistat.sock = tcp_serv_accept(apirsock, &apistat.hostid)) < 0)
                return;

        if  ((pid = fork()) < 0)  {
                print_error($E{Cannot fork});
                return;
        }

#ifndef BUGGY_SIGCLD
        if  (pid != 0)  {
                close(apistat.sock);
                return;
        }
#else
        /* Make the process the grandchild so we don't have to worry
           about waiting for it later.  */

        if  (pid != 0)  {
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
                        ;
#else
                PIDTYPE wpid;
                while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
                        ;
#endif
                close(apistat.sock);
                return;
        }
        if  (fork() != 0)
                exit(0);
#endif

        /* We are now a separate process...
           Clean up irrelevant stuff to do with pending UDP jobs.
           See what the guy wants.
           At this stage we only want to "login".  */

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)  {
                struct  pend_job  *pj = &pend_list[cnt];
                if  (pj->out_f)  {
                        fclose(pj->out_f);
                        pj->out_f = (FILE *) 0;
                }
                pj->clientfrom = 0;
        }

        /* We need to log in first before we get reminders about jobs and variables,
           so we don't need to worry, register it after we logged in */

        get_message(&apistat);

        switch  (apistat.inmsg.code)  {
        default:
                err_result(&apistat, XB_SEQUENCE, 0);
                abort_exit(0);

        case  API_WLOGIN:
                process_wlogin(&apistat);
                break;

        case  API_LOCALLOGIN:
                process_locallogin(&apistat);
                break;

        case  API_LOGIN:
                process_login(&apistat);
                break;

        case  API_SIGNON:
                process_signon(&apistat);
                break;
        }

        /* We need these copied across for priv checking to work.
           One day we'll do this properly */

        Realuid = apistat.realuid;
        Realgid = apistat.realgid;

        /* Ok we made it */

        apistat.outmsg.code = apistat.inmsg.code;
        apistat.outmsg.retcode = XB_OK;
        put_reply(&apistat);

        /* So do main loop waiting for something to happen */

        for  (;;)  {
                while  (hadrfresh)  {
                        hadrfresh = 0;
                        proc_refresh(&apistat);
                }

                get_message(&apistat);

                switch  (apistat.outmsg.code = apistat.inmsg.code)  {
                default:
                        ret = XB_UNKNOWN_COMMAND;
                        break;

                case  API_SIGNOFF:
                        abort_exit(0);

                case  API_NEWGRP:
                {
                        int_ugid_t      ngid;

                        if  (!(apistat.hispriv.btu_priv & BTM_WADMIN)  &&
                                !chk_vgroup(prin_uname(Realuid), apistat.inmsg.un.signon.username))  {
                                ret = XB_BAD_GROUP;
                                break;
                        }
                        if  ((ngid = lookup_gname(apistat.inmsg.un.signon.username)) == UNKNOWN_GID)  {
                                ret = XB_UNKNOWN_GROUP;
                                break;
                        }
                        apistat.realgid = Realgid = ngid;
                        ret = XB_OK;
                        break;
                }

                case  API_JOBLIST:
                        reply_joblist(&apistat);
                        continue;

                case  API_VARLIST:
                        reply_varlist(&apistat);
                        continue;

                case  API_JOBREAD:
                        reply_jobread(&apistat);
                        continue;

                case  API_FINDJOBSLOT:
                case  API_FINDJOB:
                        reply_jobfind(&apistat);
                        continue;

                case  API_VARREAD:
                        reply_varread(&apistat);
                        continue;

                case  API_FINDVARSLOT:
                case  API_FINDVAR:
                        reply_varfind(&apistat);
                        continue;

                case  API_JOBDEL:
                        ret = reply_jobdel(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                        break;

                case  API_VARDEL:
                        ret = reply_vardel(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_JOBOP:
                        ret = reply_jobop(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                        break;

                case  API_JOBADD:
                        api_jobstart(&apistat);
                        continue;

                case  API_DATAIN:
                        api_jobcont(&apistat);
                        continue;

                case  API_DATAEND:
                        api_jobfinish(&apistat);
                        continue;

                case  API_DATAABORT: /* We'll be lucky if we get this */
                        api_jobabort(&apistat);
                        continue;

                case  API_VARADD:
                        ret = reply_varadd(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_JOBUPD:
                        ret = reply_jobupd(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                        break;

                case  API_VARUPD:
                        ret = reply_varupd(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_VARCHCOMM:
                        ret = reply_varchcomm(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_JOBDATA:
                        api_jobdata(&apistat);
                        continue;

                case  API_JOBCHMOD:
                        ret = reply_jobchmod(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                        break;

                case  API_VARCHMOD:
                        ret = reply_varchmod(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_JOBCHOWN:
                        ret = reply_jobchown(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                        break;

                case  API_VARCHOWN:
                        ret = reply_varchown(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_JOBCHGRP:
                        ret = reply_jobchgrp(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Job_seg.dptr->js_serial);
                        break;

                case  API_VARCHGRP:
                        ret = reply_varchgrp(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_VARRENAME:
                        ret = reply_varrename(&apistat);
                        apistat.outmsg.un.r_reader.seq = htonl(Var_seg.dptr->vs_serial);
                        break;

                case  API_CIADD:
                        ret = reply_ciadd(&apistat);
                        break;

                case  API_CIREAD:
                        reply_ciread(&apistat);
                        continue;

                case  API_CIUPD:
                        ret = reply_ciupd(&apistat);
                        break;

                case  API_CIDEL:
                        ret = reply_cidel(&apistat);
                        break;

                case  API_HOLREAD:
                        reply_holread(&apistat);
                        continue;

                case  API_HOLUPD:
                        ret = reply_holupd(&apistat);
                        break;

                case  API_SETQUEUE:
                        ret = reply_setqueue(&apistat);
                        break;

                case  API_REQPROD:
                        setup_prod(&apistat);
                        continue;

                case  API_UNREQPROD:
                        unsetup_prod(&apistat);
                        continue;

                case  API_SENDENV:
                        reply_sendenv(&apistat);
                        continue;

                case  API_GETBTU:
                        reply_getbtu(&apistat);
                        continue;

                case  API_GETBTD:
                        reply_getbtd(&apistat);
                        continue;

                case  API_PUTBTU:
                        ret = reply_putbtu(&apistat);
                        break;

                case  API_PUTBTD:
                        ret = reply_putbtd(&apistat);
                        break;
                }

                apistat.outmsg.retcode = htons(ret);
                put_reply(&apistat);
        }
}
