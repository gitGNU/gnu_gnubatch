/* xbnet_ua.c -- xbnetserv user handling

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
#include "incl_unix.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <grp.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_sig.h"
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

int     holf_fd = -1;           /* Holiday file */
static  unsigned        holf_mode;

struct  udp_conn  {
        struct  udp_conn  *next;                /* Next in hash chain */
        time_t          lastop;                 /* Time of last op */
        netid_t         hostid;                 /* Hostid involved */
        int_ugid_t      host_uid;               /* User id to distinguish UNIX clients or UNKNOWN_UID */
        int_ugid_t      uid;                    /* User id using */
        int_ugid_t      gid;                    /* Ditto group id */
        Btuser          privs;                  /* Permissions */
        char            *username;              /* UNIX user name */
        char            *groupname;             /* UNIX group name (FIXME) */
};

static  struct  udp_conn        *conn_hash[NETHASHMOD];

/* These are mostly all to do with pending jobs from the old version of the
   Windows client, however we do try to keep track of clients from UNIX hosts.
   In those cases we track the user id (on the host) to try to distinguish different
   users on the machine. Call find_conn with UNKNOWN_UID if we don't need to worry. */

static  struct  udp_conn  *find_conn(const netid_t hostid, const int_ugid_t uid)
{
        struct  udp_conn  *fp;

        for  (fp = conn_hash[calcnhash(hostid)];  fp;  fp = fp->next)
                if  (fp->hostid == hostid  &&  (uid == UNKNOWN_UID || fp->host_uid == uid))
                        return  fp;
        return  (struct udp_conn *) 0;
}

/* Create a new connection structure for UDP ops */

static  struct  udp_conn  *add_conn(const netid_t hostid)
{
        struct  udp_conn *fp = malloc(sizeof(struct udp_conn));
        unsigned  hashv = calcnhash(hostid);
        if  (!fp)
                ABORT_NOMEM;

        fp->next = conn_hash[hashv];
        conn_hash[hashv] = fp;
        fp->hostid = hostid;
        fp->host_uid = UNKNOWN_UID;                       /* Fix this later if not Windows */
        return  fp;
}

static  void    kill_conn(struct udp_conn *fp)
{
        struct  udp_conn  **fpp, *np;

        fpp = &conn_hash[calcnhash(fp->hostid)];

        while  ((np = *fpp))  {
                if  (np == fp)  {
                        *fpp = fp->next;
                        free(fp->username);
                        free(fp->groupname);
                        free((char *) fp);
                        return;
                }
                fpp = &np->next;
        }

        /* "We cannot get here" */
        exit(E_SETUP);
}

/* Locate pending jobs, NB we assume only a few and we assume only from Windows so we
   don't have to worry about multiple concurrent jobs from the same host */

struct pend_job *find_pend(const netid_t whofrom)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (pend_list[cnt].clientfrom == whofrom)
                        return  &pend_list[cnt];
        return  (struct pend_job *) 0;
}

struct pend_job *find_j_by_jno(const jobno_t jobno)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (pend_list[cnt].out_f  &&  pend_list[cnt].jobn == jobno)
                        return  &pend_list[cnt];
        return  (struct pend_job *) 0;
}

struct pend_job *add_pend(const netid_t whofrom)
{
        int     cnt;

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (!pend_list[cnt].out_f)  {
                        pend_list[cnt].clientfrom = whofrom;
                        pend_list[cnt].prodsent = 0;
                        return  &pend_list[cnt];
                }
        return  (struct pend_job *) 0;
}

/* Clean up after decaying job.  */

void  abort_job(struct pend_job *pj)
{
        if  (!pj  ||  !pj->out_f)
                return;
        fclose(pj->out_f);
        pj->out_f = (FILE *) 0;
        unlink(pj->tmpfl);
        pj->clientfrom = 0;
}

static void  udp_send_vec(char *vec, const int size, struct sockaddr_in *sinp)
{
        sendto(uasock, vec, size, 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Similar routine for when we are sending from scratch */

static void  udp_send_to(char *vec, const int size, const netid_t whoto)
{
        int     sockfd, tries;
        struct  sockaddr_in     to_sin, cli_addr;

        BLOCK_ZERO(&to_sin, sizeof(to_sin));
        to_sin.sin_family = AF_INET;
        to_sin.sin_port = uaportnum;
        to_sin.sin_addr.s_addr = whoto;

        BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        cli_addr.sin_port = 0;

        /* We don't really need the cli_addr but we are obliged to
           bind something.  The remote uses our standard port.  */

        for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
                if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                        return;
                if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
                        close(sockfd);
                        return;
                }
                if  (sendto(sockfd, vec, size, 0, (struct sockaddr *) &to_sin, sizeof(to_sin)) >= 0)  {
                        close(sockfd);
                        return;
                }
                close(sockfd);
        }
}

static  void  job_reply(struct sockaddr_in *sinp, const int code)
{
        struct  client_if  reply;
        reply.code = code;
        sendto(uasock, (char *) &reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

static  void  job_reply_param(struct sockaddr_in *sinp, const int code, const LONG param)
{
        struct  client_if  reply;
        reply.code = code;
        reply.param = htonl(param);
        sendto(uasock, (char *) &reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Process job queueing from UDP which we only do for Windows clients. */

static void  udp_job_process(struct sockaddr_in *sinp, char *pmsg, int datalength)
{
        int                     ret = 0, tries;
        netid_t                 whofrom = sockaddr2int_netid_t(sinp);
        ULONG                   indx;
        struct  udp_conn        *fp;
        struct  ni_jobhdr       *nih;
        struct  pend_job        *pj;
        time_t                  now = time((time_t *) 0);
        Shipc                   Oreq;

        /* We are only expecting this from Windows clients, so we reject people we
           don't know or are from UNIX hosts. */

        if  (!(fp = find_conn(whofrom, UNKNOWN_UID))  ||  fp->host_uid != UNKNOWN_UID)  {
                job_reply(sinp, XBNR_UNKNOWN_CLIENT);
                return;
        }

        if  (datalength < sizeof(struct ni_jobhdr))  {
                job_reply(sinp, XBNR_BAD_JOBDATA);
                return;
        }

        pj = find_pend(whofrom);
        fp->lastop = now;

        /* Get rest of message and pointer to header */
        nih = (struct ni_jobhdr *) pmsg;
        pmsg += sizeof(struct ni_jobhdr);
        datalength -= sizeof(struct ni_jobhdr);

        switch  (nih->code)  {
        default:
                job_reply(sinp, SV_CL_UNKNOWNC);
                return;

        case  CL_SV_STARTJOB:

                /* Start of new job, delete any pending one. add new one */

                abort_job(pj);
                pj = add_pend(whofrom);
                if  (!pj)  {
                        job_reply(sinp, SV_CL_PEND_FULL);
                        return;
                }
                pj->lastaction = now;
                pj->joblength = pj->lengthexp = ntohs(nih->joblength);
                if  (pj->lengthexp < (unsigned) datalength)  {
                        job_reply(sinp, XBNR_BAD_JOBDATA);
                        return;
                }
                pj->lengthexp -= datalength;
                pj->cpos = (char *) &pj->jobin;
                BLOCK_COPY(pj->cpos, pmsg, datalength);
                pj->cpos += datalength;
                job_reply(sinp, SV_CL_ACK);
                return;

        case  CL_SV_CONTJOB:
                if  ((unsigned) datalength > pj->lengthexp)  {
                        job_reply(sinp, XBNR_BAD_JOBDATA);
                        return;
                }
                BLOCK_COPY(pj->cpos, pmsg, datalength);
                pj->cpos += datalength;
                pj->lengthexp -= datalength;

                if  (pj->lengthexp == 0)  {

                        /* Finished with job, construct output job */

                        if  ((ret = unpack_job(&pj->jobout, &pj->jobin, pj->joblength, whofrom)) != 0)  {
                                job_reply_param(sinp, ret, err_which);
                                abort_job(pj);
                                return;
                        }

                        /* Stick user and group into job */

                        pj->jobout.h.bj_mode.o_uid = pj->jobout.h.bj_mode.c_uid = fp->uid;
                        pj->jobout.h.bj_mode.o_gid = pj->jobout.h.bj_mode.c_gid = fp->gid;
                        strncpy(pj->jobout.h.bj_mode.o_user, fp->username, UIDSIZE);
                        strncpy(pj->jobout.h.bj_mode.c_user, fp->username, UIDSIZE);
                        strncpy(pj->jobout.h.bj_mode.o_group, fp->groupname, UIDSIZE);
                        strncpy(pj->jobout.h.bj_mode.c_group, fp->groupname, UIDSIZE);

                        if  ((ret = unpack_cavars(&pj->jobout, &pj->jobin)) != 0)  {
                                abort_job(pj);
                                job_reply(sinp, ret);
                                return;
                        }
                        if  ((ret = validate_job(&pj->jobout, &fp->privs)) != 0)  {
                                job_reply_param(sinp, ret, 0);
                                abort_job(pj);
                                return;
                        }

                        /* Generate job number and output file vaguely from netid etc.  */

                        pj->jobn = (ULONG) (ntohl(whofrom) + pj->lastaction) % JOB_MOD;
                        pj->out_f = goutfile(&pj->jobn, pj->tmpfl, 1);
                        pj->jobout.h.bj_job = pj->jobn;
                        pj->jobout.h.bj_time = pj->lastaction;
                }
                job_reply(sinp, SV_CL_ACK);
                return;

        case  CL_SV_JOBDATA:
                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }
                if  (!pj->out_f  ||  pj->lengthexp != 0)  {
                        job_reply(sinp, SV_CL_BADPROTO);
                        return;
                }
                pj->lastaction = now;
                while  (--datalength >= 0)  {
                        if  (putc(*(unsigned char *)pmsg, pj->out_f) == EOF)  {
                                abort_job(pj);
                                job_reply(sinp, XBNR_FILE_FULL);
                                return;
                        }
                        pmsg++;
                }
                job_reply(sinp, SV_CL_ACK);
                return;

        case  CL_SV_ENDJOB:
                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }
                if  (!pj->out_f  ||  pj->lengthexp != 0)  {
                        job_reply(sinp, SV_CL_BADPROTO);
                        return;
                }
                JREQ = &Xbuffer->Ring[indx = getxbuf_serv()];
                BLOCK_ZERO(&Oreq, sizeof(Oreq));
                Oreq.sh_mtype = TO_SCHED;
                Oreq.sh_params.mcode = J_CREATE;
                Oreq.sh_params.uuid = pj->jobout.h.bj_mode.o_uid;
                Oreq.sh_params.ugid = pj->jobout.h.bj_mode.o_gid;
                mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
                Oreq.sh_un.sh_jobindex = indx;
                BLOCK_COPY(JREQ, &pj->jobout, sizeof(Btjob));
                JREQ->h.bj_slotno = -1;
#ifdef  USING_MMAP
                sync_xfermmap();
#endif
                for  (tries = 0;  tries < MSGQ_BLOCKS;  tries++)  {
                        if  (msgsnd(Ctrl_chan, (struct msgbuf *)&Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) >= 0)
                                goto  sentok;
                        sleep(MSGQ_BLOCKWAIT);
                }
                freexbuf_serv(indx);
                abort_job(pj);
                job_reply(sinp, XBNR_QFULL);
                return;

        sentok:
                freexbuf_serv(indx);
                if  ((ret = readreply()) != J_OK)  {
                        job_reply_param(sinp, XBNR_ERR, ret);
                        abort_job(pj);
                        return;
                }

                fclose(pj->out_f);
                pj->out_f = (FILE *) 0;
                pj->clientfrom = 0;
                job_reply_param(sinp, SV_CL_ACK, JREQ->h.bj_job);
                return;

        case  CL_SV_HANGON:
                if  (!pj)  {
                        job_reply(sinp, SV_CL_UNKNOWNJ);
                        return;
                }
                pj->lastaction = now;
                pj->prodsent = 0;
                job_reply(sinp, SV_CL_ACK);
                return;
        }
}

/* Create variable */

static void     udp_crevar_process(struct sockaddr_in *sinp, struct ni_createvar *pmsg, int datalength)
{
        int             tries;
        netid_t         whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn        *fp;
        Shipc                   Oreq;

        if  (!(fp = find_conn(whofrom, UNKNOWN_UID)))  {
                job_reply(sinp, XBNR_UNKNOWN_CLIENT);
                return;
        }

        if  (datalength < sizeof(struct ni_createvar))  {
                job_reply(sinp, XBNR_BAD_JOBDATA);
                return;
        }

        if  ((fp->privs.btu_priv & BTM_CREATE) == 0)  {
                job_reply(sinp, XBNR_NOCRPERM);
                return;
        }

        fp->lastop = time((time_t *) 0);
        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = V_CREATE;
        Oreq.sh_params.uuid = fp->uid;
        Oreq.sh_params.ugid = fp->gid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        strncpy(Oreq.sh_un.sh_var.var_name, pmsg->nicv_name, BTV_NAME);
        strncpy(Oreq.sh_un.sh_var.var_comment, pmsg->nicv_comment, BTV_COMMENT);
        Oreq.sh_un.sh_var.var_flags = pmsg->nicv_flags;
        Oreq.sh_un.sh_var.var_value.const_type = pmsg->nicv_consttype;
        if (pmsg->nicv_consttype == CON_STRING)
                strncpy(Oreq.sh_un.sh_var.var_value.con_un.con_string, pmsg->nicv_string, BTC_VALUE);
        else
                Oreq.sh_un.sh_var.var_value.con_un.con_long = ntohl(pmsg->nicv_long);
        Oreq.sh_un.sh_var.var_mode.u_flags = ntohs(pmsg->u_flags);
        Oreq.sh_un.sh_var.var_mode.g_flags = ntohs(pmsg->g_flags);
        Oreq.sh_un.sh_var.var_mode.o_flags = ntohs(pmsg->o_flags);

        for  (tries = 0;  tries < MSGQ_BLOCKS;  tries++)  {
                 if  (msgsnd(Ctrl_chan, (struct msgbuf *)&Oreq, sizeof(Shreq) + sizeof(Btvar), IPC_NOWAIT) >= 0)  {
                         int  ret = readreply();
                         switch  (ret)  {
                         case  V_OK:
                                 ret = XBNQ_OK;
                                 break;
                         default:
                                 ret = XBNR_ERR;
                                 break;
                         case  V_NOPERM:
                                 ret = XBNR_NOCRPERM;
                                 break;
                         case  V_MINPRIV:
                                 ret = XBNR_MINPRIV;
                                 break;
                         case  V_EXISTS:
                                 ret = XBNR_EXISTS;
                                 break;
                         case  V_NOTEXPORT:
                                 ret = XBNR_NOTEXPORT;
                                 break;
                         case  V_CLASHES:
                                 ret = XBNR_CLASHES;
                                 break;
                         case  V_FULLUP:
                                 ret = XBNR_FILE_FULL;
                                 break;

                         }
                         job_reply(sinp, ret);
                         return;
                 }
                 sleep(MSGQ_BLOCKWAIT);
        }

        /* Dropping out means that the queue was full */

        job_reply(sinp, XBNR_QFULL);
}

/* Delete variable */

static void     udp_delvar_process(struct sockaddr_in *sinp, struct ni_createvar *pmsg, int datalength)
{
        int             tries;
        netid_t         whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn        *fp;
        Shipc                   Oreq;

        if  (!(fp = find_conn(whofrom, UNKNOWN_UID)))  {
                job_reply(sinp, XBNR_UNKNOWN_CLIENT);
                return;
        }

        if  (datalength < sizeof(struct ni_createvar))  {
                job_reply(sinp, XBNR_BAD_JOBDATA);
                return;
        }

        fp->lastop = time((time_t *) 0);

        /* In delete case we only do it on the same host we are running and we
           only give the name. */
        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = V_DELETE;
        Oreq.sh_params.uuid = fp->uid;
        Oreq.sh_params.ugid = fp->gid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        strncpy(Oreq.sh_un.sh_var.var_name, pmsg->nicv_name, BTV_NAME);

        for  (tries = 0;  tries < MSGQ_BLOCKS;  tries++)  {
                 if  (msgsnd(Ctrl_chan, (struct msgbuf *)&Oreq, sizeof(Shreq) + sizeof(Btvar), IPC_NOWAIT) >= 0)  {
                         int  ret = readreply();
                         switch  (ret)  {
                         case  V_OK:
                                 ret = XBNQ_OK;
                                 break;
                         default:
                                 ret = XBNR_ERR;
                                 break;
                         case  V_NOPERM:
                                 ret = XBNR_NOPERM;
                                 break;
                         case  V_DSYSVAR:
                                 ret = XBNR_DSYSVAR;
                                 break;
                         case  V_NEXISTS:
                                 ret = XBNR_NEXISTS;
                                 break;
                         case  V_INUSE:
                                 ret = XBNR_INUSE;
                                 break;
                         }
                         job_reply(sinp, ret);
                         return;
                 }
                 sleep(MSGQ_BLOCKWAIT);
        }

        /* Dropping out means that the queue was full */

        job_reply(sinp, XBNR_QFULL);
}

/* Send end of list marker (also sent to indicate no permission etc) */

static  void    udp_send_end(struct sockaddr_in *sinp)
{
        char    reply = '\0';
        udp_send_vec(&reply, 1, sinp);
}

static  void    udp_send_uglist(struct sockaddr_in *sinp, char **(*ugfn)(const char *))
{
        char    **ul = (*ugfn)((const char *) 0);
        int     rp = 0;
        char    reply[CL_SV_BUFFSIZE];

        if  (ul)  {
                char    **up;
                for  (up = ul;  *up;  up++)  {
                        int     lng = strlen(*up) + 1;
                        if  (lng + rp > CL_SV_BUFFSIZE)  {
                                udp_send_vec(reply, rp, sinp);
                                rp = 0;
                        }
                        strcpy(&reply[rp], *up);
                        rp += lng;
                        free(*up);
                }
                if  (rp > 0)
                        udp_send_vec(reply, rp, sinp);
                free((char *) ul);
        }

        udp_send_end(sinp);
}

static void  udp_send_vlist(struct sockaddr_in *sinp, struct ua_venq *uav, const int datalen)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        int             rp = 0, nn;
        int_ugid_t      ruid = UNKNOWN_UID;     /* Remote user id */
        USHORT          vperm = BTM_SHOW;
        char    reply[CL_SV_BUFFSIZE];

        if  (datalen != sizeof(struct ua_venq))  {
                udp_send_end(sinp);
                return;
        }

        /* If this comes from a UNIX host, we have null in the user name and
           the uid (at the other end) in the group name field */

        if  (uav->uname[0] == '\0')
                ruid = ntohl(uav->uav_un.uav_uid);

        if  (!(fp = find_conn(whofrom, ruid)))  {
                udp_send_end(sinp);
                return;
        }

        /* Get the required permissions from the variable structure */

        vperm |= ntohs(uav->uav_perm);

        /* Have to set globals Realuid and Realgid for benefit of mpermitted */

        Realuid = fp->uid;
        Realgid = fp->gid;
        fp->lastop = time((time_t *) 0); /* Last activity */
        rvarfile(1);

        /* These aren't sorted, that's the other end's problem.  */

        for  (nn = 0;  nn < VAR_HASHMOD;  nn++)  {
                vhash_t         hp;
                struct  Ventry  *ventp;
                BtvarRef        vp;
                unsigned        lng, hlng, tlng;
                char            *hname;

                for  (hp = Var_seg.vhash[nn];  hp >= 0;  hp = ventp->Vnext)  {
                        ventp = &Var_seg.vlist[hp];
                        vp = &ventp->Vent;                 /* Actual variable structure */

                        /* Skip variable if it is not exported and we aren't looking at it from
                           the local machine. (Theory: we can't see variables on other machines which
                           aren't exported) */

                        if  (!(vp->var_flags & VF_EXPORT)  &&  whofrom != 0L)
                                continue;

                        /* Skip variables which the permissions don't let us do the required thing */

                        if  (!mpermitted(&vp->var_mode, vperm, fp->privs.btu_priv))
                                continue;

                        /* Get length of variable name in lng, total length in tlng */

                        lng = strlen(vp->var_name);

                        if  (vp->var_id.hostid)  {
                                hname = look_host(vp->var_id.hostid);
                                hlng = strlen(hname);
                                tlng = hlng + lng + 2;         /* 2 for colon and null */
                        }
                        else  if  (whofrom != 0L)  {
                                hname = myhostname;
                                hlng = myhostl;
                                tlng = myhostl + lng + 2;
                        }
                        else  {
                                hlng = 0;
                                tlng = lng + 1;
                        }

                        /* Spew out what we've got if it's overflowing */

                        if  (tlng + rp > CL_SV_BUFFSIZE)  {
                                udp_send_vec(reply, rp, sinp);
                                rp = 0;
                        }

                        /* Assemble the var */

                        if  (hlng != 0)  {      /* Putting host name in */
                                strcpy(&reply[rp], hname);
                                rp += hlng;
                                reply[rp++] = ':';
                        }
                        strcpy(&reply[rp], vp->var_name);
                        rp += lng + 1;
                }
        }
        if  (rp > 0)
                udp_send_vec(reply, rp, sinp);

        udp_send_end(sinp);
}

static void  udp_send_cilist(struct sockaddr_in *sinp)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn        *fp;
        int             rp = 0;
        unsigned        cnt;
        CmdintRef       ocip;
        char    reply[CL_SV_BUFFSIZE];

        /* We don't really care who uses this */

        if  (!(fp = find_conn(whofrom, UNKNOWN_UID)))  {
                udp_send_end(sinp);
                return;
        }

        /* If this is a UNIX user we might be updating the wrong one here but it probably
           doesn't matter */

        fp->lastop = time((time_t *) 0);
        rereadcif();

        ocip = (CmdintRef) reply;
        for  (cnt = 0;  cnt < Ci_num;  cnt++)  {
                CmdintRef  ci = &Ci_list[cnt];
                if  (ci->ci_name[0] == '\0')
                        continue;
                if  (rp + sizeof(Cmdint) >= CL_SV_BUFFSIZE)  {
                        udp_send_vec(reply, rp, sinp);
                        rp = 0;
                        ocip = (CmdintRef) reply;
                }
                ocip->ci_ll = htons(ci->ci_ll);
                ocip->ci_nice = ci->ci_nice;
                ocip->ci_flags = ci->ci_flags;
                strcpy(ocip->ci_name, ci->ci_name);
                strcpy(ocip->ci_path, ci->ci_path);
                strcpy(ocip->ci_args, ci->ci_args);
                rp += sizeof(Cmdint);
                ocip++;
        }
        if  (rp > 0)
                udp_send_vec(reply, rp, sinp);

        udp_send_end(sinp);
}

/* Get a holiday year */

void  get_hf(const unsigned year, char *reply)
{
        if  (holf_fd < 0)  {
                char    *fname = envprocess(HOLFILE);
                holf_fd = open(fname, O_RDONLY);
                holf_mode = O_RDONLY;
                free(fname);
        }

        if  (holf_fd >= 0)  {
                lseek(holf_fd, (long) (year * YVECSIZE), 0);
                Ignored_error = read(holf_fd, reply, YVECSIZE);
        }
}

/* Insert a holiday year (currently only called from API) */

void  put_hf(const unsigned year, char *reply)
{
        if  (holf_fd >= 0  &&  holf_mode != O_RDWR)  {
                close(holf_fd);
                holf_fd = -1;
        }

        if  (holf_fd < 0)  {
                char    *fname = envprocess(HOLFILE);
                if  ((holf_fd = open(fname, O_RDWR)) < 0)  {
                        holf_fd = open(fname, O_RDWR|O_CREAT, 0644);
#ifdef  HAVE_FCHOWN
                        if  (Daemuid != ROOTID)
                                Ignored_error = fchown(holf_fd, Daemuid, Daemgid);
#else
                        if  (Daemuid != ROOTID)
                                Ignored_error = chown(fname, Daemuid, Daemgid);
#endif
                }
                holf_mode = O_RDWR;
                free(fname);
        }

        if  (holf_fd >= 0)  {
                lseek(holf_fd, (long) (year * YVECSIZE), 0);
                Ignored_error = write(holf_fd, reply, YVECSIZE);
        }
}

static void  udp_send_hlist(struct sockaddr_in *sinp, const char *pmsg, const int datalen)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn        *fp;
        char    reply[CL_SV_BUFFSIZE];

        /* If read doesn't read anything, just return zeroes */

        BLOCK_ZERO(reply, YVECSIZE);
        if  (datalen == sizeof(struct ua_venq)  &&  (fp = find_conn(whofrom, UNKNOWN_UID)))  {
                const   struct  ua_venq *venq = (const struct ua_venq *) pmsg;
                unsigned  year = ntohs(venq->uav_perm);
                get_hf(year, reply);
                fp->lastop = time((time_t *) 0);
        }
        udp_send_end(sinp);
}

int  checkpw(const char *name, const char *passwd)
{
        static  char    *btpwnam;
        int             ipfd[2], opfd[2];
        char            rbuf[1];
        PIDTYPE         pid;

        if  (!btpwnam)
                btpwnam = envprocess(BTPWPROG);

        /* Don't bother with error messages, just say no.  */

        if  (pipe(ipfd) < 0)
                return  0;
        if  (pipe(opfd) < 0)  {
                close(ipfd[0]);
                close(ipfd[1]);
                return  0;
        }

        if  ((pid = fork()) == 0)  {
                close(opfd[1]);
                close(ipfd[0]);
                if  (opfd[0] != 0)  {
                        close(0);
                        Ignored_error = dup(opfd[0]);
                        close(opfd[0]);
                }
                if  (ipfd[1] != 1)  {
                        close(1);
                        Ignored_error = dup(ipfd[1]);
                        close(ipfd[1]);
                }
                execl(btpwnam, btpwnam, name, (char *) 0);
                exit(255);
        }
        close(opfd[0]);
        close(ipfd[1]);
        if  (pid < 0)  {
                close(ipfd[0]);
                close(opfd[1]);
                return  0;
        }
        Ignored_error = write(opfd[1], passwd, strlen(passwd));
        rbuf[0] = '\n';
        Ignored_error = write(opfd[1], rbuf, sizeof(rbuf));
        close(opfd[1]);
        if  (read(ipfd[0], rbuf, sizeof(rbuf)) != sizeof(rbuf))  {
                close(ipfd[0]);
                return  0;
        }
        close(ipfd[0]);
        return  rbuf[0] == '0'? 1: 0;
}

/* See whether the given user can access the given group.  We DON'T
   use supplementary group login in rpwfile etc as not all
   systems support them and I don't think that we use this
   sufficiently often.  */

int  chk_vgroup(const char *uname, const char *gname)
{
        struct  group   *gg = getgrnam(gname);
        char    **mems;

        endgrent();

        if  (!gg  ||  !(mems = gg->gr_mem))
                return  0;

        while  (*mems)  {
                if  (strcmp(*mems, uname) == 0)
                        return  1;
                mems++;
        }
        return  0;
}

/* Tell scheduler about new user.  */

void  tell_sched_roam(const netid_t netid, const int_ugid_t ruid, const char *unam, const char *gnam)
{
        Shipc   nmsg;

        BLOCK_ZERO(&nmsg, sizeof(nmsg));
        nmsg.sh_mtype = TO_SCHED;
        nmsg.sh_params.mcode = N_ROAMUSER;
        nmsg.sh_params.upid = getpid();
        nmsg.sh_un.sh_n.hostid = netid;
        nmsg.sh_un.sh_n.remuid = ruid;
        if  (ruid == UNKNOWN_UID)
                nmsg.sh_un.sh_n.ht_flags = HT_DOS;
        nmsg.sh_un.sh_n.ht_flags |= HT_ROAMUSER;
        strncpy(nmsg.sh_un.sh_n.hostname, unam, HOSTNSIZE);
        strncpy(nmsg.sh_un.sh_n.alias, gnam, HOSTNSIZE);
        msgsnd(Ctrl_chan, (struct msgbuf *) &nmsg, sizeof(Shreq) + sizeof(struct remote), 0); /* Not expecting reply */
}

/* Bad return from enquiry - we return the errors in the user id field of
   the permissions structure, which is otherwise useless.  */

static  void  enq_badret(struct sockaddr_in *sinp, const int code)
{
        struct  ua_reply  rep;
        /* This turns off btu_isvalid too so other end knows we're complaining,
           we just plonk the error code in the user field */
        BLOCK_ZERO(&rep, sizeof(rep));
        rep.ua_perm.btu_user = htonl((LONG) code);
        udp_send_vec((char *) &rep, sizeof(struct ua_reply), sinp);
}

static  void  bad_log(struct sockaddr_in *sinp, const int code)
{
        struct  ua_login  reply;
        BLOCK_ZERO(&reply, sizeof(reply));
        reply.ual_op = code;
        sendto(uasock, (char *) &reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

/* Original kind of enquiry for user permissions.  */

static void  do_enquiry(struct sockaddr_in *sinp, const char *pmsg, const int datalen)
{
        const   struct  ni_jobhdr *uenq = (const struct ni_jobhdr *) pmsg;
        struct  ua_reply  rep;
        int_ugid_t        luid;
        netid_t whofrom;
        struct  udp_conn  *fp;
        struct  alhash    *ap;

        /* We recognise Rspr and UNIX-based clients because they put the relevant user
           name in the request, but Windows clients just have a null name field */

        if  (uenq->uname[0])  {
                luid = lookup_uname(uenq->uname);
                if  (luid == UNKNOWN_UID)  {
                        enq_badret(sinp, XBNR_NOT_USERNAME);
                        return;
                }
                strncpy(rep.ua_uname, uenq->uname, UIDSIZE);
                strncpy(rep.ua_gname, prin_gname(lastgid), UIDSIZE);
                btuser_pack(&rep.ua_perm, getbtuentry(luid));
                /* Other end knows that everything is OK because btu_isvalid is set in ua_perm */
                udp_send_vec((char *) &rep, sizeof(struct ua_reply), sinp);
                return;
        }

        /* So it's some kind of Windows client */

        whofrom = sockaddr2int_netid_t(sinp);
        if  ((fp = find_conn(whofrom, UNKNOWN_UID)))  {
                strncpy(rep.ua_uname, fp->username, UIDSIZE);
                strncpy(rep.ua_gname, fp->groupname, UIDSIZE);
                btuser_pack(&rep.ua_perm, &fp->privs);
                udp_send_vec((char *) &rep, sizeof(struct ua_reply), sinp);
                return;
        }

        ap = find_autoconn(whofrom);
        if  (!ap)  {
                enq_badret(sinp, XBNR_UNKNOWN_CLIENT);
                return;
        }

        strncpy(rep.ua_uname, ap->unixname, UIDSIZE);
        strncpy(rep.ua_gname, ap->unixgroup, UIDSIZE);
        btuser_pack(&rep.ua_perm, getbtuentry(ap->uuid));
        udp_send_vec((char *) &rep, sizeof(struct ua_reply), sinp);
}

/* Return OK login and UNIX user name we're in as */

static  void    logret_ok(struct sockaddr_in *sinp, const char *uname)
{
        struct  ua_login  reply;

        BLOCK_ZERO(&reply, sizeof(reply));
        reply.ual_op = UAL_OK;
        strncpy(reply.ual_name, uname, sizeof(reply.ual_name)-1);
        sendto(uasock, (char *) &reply, sizeof(reply), 0, (struct sockaddr *) sinp, sizeof(struct sockaddr_in));
}

static  void    udp_uenquire(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        int_ugid_t      uuid, ruid;

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        ruid = ntohl(inmsg->ua_un.ual_uid);

        if  ((fp = find_conn(whofrom, ruid)))  {

                /* Existing connection.
                   If it's for a different user, reject unless it's on "my" machine.
                   Thinks: is this OK for Batch or is it a back door? */

                if  (ncstrcmp(fp->username, inmsg->ual_name) != 0)  {

                        if  (whofrom != 0)  {
                                bad_log(sinp, XBNR_NOT_USERNAME);
                                return;
                        }

                        if  ((uuid = lookup_uname(inmsg->ual_name)) == UNKNOWN_UID)  {
                                bad_log(sinp, XBNR_NOT_USERNAME);
                                return;
                        }
                        fp->uid = uuid;
                        fp->gid = lastgid;
                        free(fp->username);
                        free(fp->groupname);
                        fp->username = stracpy(inmsg->ual_name);
                        fp->groupname = stracpy(prin_gname(fp->gid));
                        fp->privs = *getbtuentry(uuid);
                }
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                tell_sched_roam(whofrom, ruid, fp->username, fp->groupname);
                return;
        }

        /* Create standard connection only from same host */

        if  (whofrom != 0)  {
                bad_log(sinp, UAL_INVP);
                return;
        }

        if  ((uuid = lookup_uname(inmsg->ual_name)) == UNKNOWN_UID)  {
                bad_log(sinp, XBNR_NOT_USERNAME);
                return;
        }

        /* Create connection for that person */

        fp = add_conn(whofrom);
        fp->host_uid = ruid;
        fp->lastop = time((time_t *) 0);
        fp->uid = uuid;
        fp->gid = lastgid;
        fp->username = stracpy(inmsg->ual_name);
        fp->groupname = stracpy(prin_gname(fp->gid));
        fp->privs = *getbtuentry(uuid);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, ruid, fp->username, fp->groupname);
}

/* This is the parallel version for Windows clients. We don't have to worry about multiple
   users on the same machine */

void  udp_enquire(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);       /* We don't anticipate this to be "me" */
        struct  udp_conn  *fp;
        struct  alhash  *wp;

        /* NB the size of the login structure may change (longer Win user name) so
           this will cream out old clients */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        /* See if it is someone we know, if so refresh the access time and return the details */

        if  ((fp = find_conn(whofrom, UNKNOWN_UID)))  {
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                tell_sched_roam(whofrom, UNKNOWN_UID, fp->username, fp->groupname);
                return;
        }

        /* See if it is someone we connect automatically */

        if  (!(wp = find_autoconn(whofrom)))  {
                bad_log(sinp, UAL_INVP);
                return;
        }

        /* Create connection for that person */

        fp = add_conn(whofrom);
        fp->lastop = time((time_t *) 0);
        fp->uid = wp->uuid;
        fp->gid = wp->ugid;
        fp->username = stracpy(wp->unixname);
        fp->groupname = stracpy(wp->unixgroup);
        fp->privs = *getbtuentry(fp->uid);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, UNKNOWN_UID, fp->username, fp->groupname);
}

void  udp_ulogin(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);                /* Might be "me" */
        struct  udp_conn  *fp;
        int_ugid_t  luid, ruid;

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        ruid = ntohl(inmsg->ua_un.ual_uid);

        /* See if someone is already logged in and report success if it is the same person
           as before. Refresh the activity time. NB "whofrom" might be "me".
           Refresh privileges in case the guy is logging in again after they've
           been changed. */

        if  ((fp = find_conn(whofrom, ruid))  &&  ncstrcmp(fp->username, inmsg->ual_name) == 0)  {
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                fp->privs = *getbtuentry(fp->uid);
                tell_sched_roam(whofrom, ruid, fp->username, fp->groupname);
                return;
        }

        /* Check password, if not OK, kill any existing connection */

        if  ((luid = lookup_uname(inmsg->ual_name)) == UNKNOWN_UID  ||  !checkpw(inmsg->ual_name, inmsg->ual_passwd))  {
                bad_log(sinp, UAL_INVP);
                if  (fp)
                        kill_conn(fp);
                return;
        }

        /* Reset user name and ID of existing connection or allocate a new one */

        if  (fp)  {
                free(fp->username);
                free(fp->groupname);
        }
        else
                fp = add_conn(whofrom);

        fp->lastop = time((time_t *) 0);
        fp->uid = luid;
        fp->host_uid = ruid;
        fp->gid = lastgid;
        fp->username = stracpy(inmsg->ual_name);
        fp->groupname = stracpy(prin_gname(fp->gid));
        fp->privs = *getbtuentry(luid);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, ruid, fp->username, fp->groupname);
}

/* Log in as a Windows user */

void  udp_login(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        struct  winuhash  *wn;
        char    *luname, *lgname;
        int_ugid_t  luid, lgid;

        /* First check the message size */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        /* See what the Unix name is corresponding to that windows name. (Possibly let in a Windows user of the same name).
           Use the default user name if needed (should not be a security risk with the passwords set correctly) */

        if  ((wn = lookup_winoruu(inmsg->ual_name)))  {
                luname = wn->unixname;
                luid = wn->uuid;
                lgid = wn->ugid;
                lgname = prin_gname(wn->ugid);
        }
        else  {
                luname = Defaultuser;
                lgname = Defaultgroup;
                luid = Defaultuid;
                lgid = Defaultgid;
        }
        /* If we were logged in OK as that guy before, just refresh
           and say OK. NB "whofrom" might be me again.
           However we refresh the privileges in case those have changed. */

        if  ((fp = find_conn(whofrom, UNKNOWN_UID)) && fp->uid == luid)  {
                fp->lastop = time((time_t *) 0);
                logret_ok(sinp, fp->username);
                fp->privs = *getbtuentry(luid);
                tell_sched_roam(whofrom, UNKNOWN_UID, fp->username, fp->groupname);
                return;
        }

        /* Check the password and cancel existing login if wrong */

        if  (!checkpw(luname, inmsg->ual_passwd))  {
                bad_log(sinp, UAL_INVP);
                if  (fp)
                        kill_conn(fp);
                return;
       }

        /* Reset user name and ID of existing connection or allocate a new one */

        if  (fp)  {
                free(fp->username);
                free(fp->groupname);
        }
        else
                fp = add_conn(whofrom);
        fp->lastop = time((time_t *) 0);
        fp->uid = luid;
        fp->gid = lgid;
        fp->privs = *getbtuentry(luid);
        fp->username = stracpy(luname);
        fp->groupname = stracpy(lgname);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, UNKNOWN_UID, fp->username, fp->groupname);
}

/* Log out whoever is logged in, no reply */

static  void  udp_logout(struct sockaddr_in *sinp, struct ua_login *inmsg, int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn  *fp;
        int_ugid_t      ruid = UNKNOWN_UID;     /* Remote user id */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

       /* If this comes from a UNIX host, we have NON null in the filler and
           the uid (at the other end) in the group name field. We do it this way
           as existing Windows clients zero the whole thing and just put in UAL_LOGOUT */

        if  (inmsg->ual_fill != '\0')
                ruid = ntohl(inmsg->ua_un.ual_uid);

        if  ((fp = find_conn(whofrom, ruid)))  {
                tell_sched_roam(whofrom, ruid, "", "");
                kill_conn(fp);
        }
}

static void  udp_newgrp(struct sockaddr_in *sinp, struct ua_login *inmsg, const int inlng)
{
        netid_t whofrom = sockaddr2int_netid_t(sinp);
        struct  udp_conn        *fp;
        int_ugid_t              ngid;
        int_ugid_t      ruid = UNKNOWN_UID;     /* Remote user id */

        if  (inlng != sizeof(struct ua_login))  {
                bad_log(sinp, SV_CL_BADPROTO);
                return;
        }

        /* If this comes from a UNIX host, we have NON null in the filler and
           the uid (at the other end) in the group name field. We do it this way
           as existing Windows clients zero the whole thing and just put in UAL_LOGOUT */

        if  (inmsg->ual_fill != '\0')
                ruid = ntohl(inmsg->ua_un.ual_uid);

        if  (!(fp = find_conn(whofrom, ruid)))  {
                bad_log(sinp, XBNR_UNKNOWN_CLIENT);
                return;
        }

        /* Now look at the group name */

        if  ((ngid = lookup_gname(inmsg->ual_name)) == UNKNOWN_GID)  {
                bad_log(sinp, XBNR_UNKNOWN_GROUP);
                return;
        }

        /* Allow any group if he's super-dooper */

        if  (!(fp->privs.btu_priv & BTM_WADMIN) && !chk_vgroup(fp->username, inmsg->ual_name))  {
                bad_log(sinp, UAL_INVG);
                return;
        }

        /* Turn off the existing one */
        tell_sched_roam(whofrom, ruid, "","");
        fp->gid = ngid;
        free(fp->groupname);
        fp->groupname = stracpy(prin_gname(ngid));
        fp->privs = *getbtuentry(fp->uid);
        logret_ok(sinp, fp->username);
        tell_sched_roam(whofrom, ruid, fp->username, fp->groupname);
}

/* Lies I know but just say zero to ulimit */

static void  udp_send_umlpars(struct sockaddr_in *sinp)
{
        struct  ua_umlreply     reply;
        reply.ua_umask = htons(orig_umask);
        reply.ua_padding = 0;
        reply.ua_ulimit = 0L;
        udp_send_vec((char *)&reply, sizeof(reply), sinp);
}

static void  udp_send_elist(struct sockaddr_in *sinp)
{
        int             rp = 0;
        char            **ep, *ei;
        char    reply[CL_SV_BUFFSIZE];
        extern  char    **xenviron;

        for  (ep = xenviron;  (ei = *ep);  ep++)  {
                unsigned  lng = strlen(ei) + 1;
                if  (lng + rp > CL_SV_BUFFSIZE)  {
                        udp_send_vec(reply, rp, sinp);
                        rp = 0;

                        /* If variable is longer than CL_SV_BUFFSIZE
                           (maybe we should increase this but I'm
                           scared of people with their titchy max
                           UDP xfers) split up into chunks.  */

                        if  (lng > CL_SV_BUFFSIZE)  {
                                do  {
                                        udp_send_vec(ei, CL_SV_BUFFSIZE, sinp);
                                        ei += CL_SV_BUFFSIZE;
                                        lng -= CL_SV_BUFFSIZE;
                                }  while  (lng > CL_SV_BUFFSIZE);
                                if  (lng != 0)
                                        udp_send_vec(ei, (int) lng, sinp);
                                continue;
                        }
                }
                strcpy(&reply[rp], ei);
                rp += lng;
        }

        if  (rp > 0)
                udp_send_vec(reply, rp, sinp);

        udp_send_end(sinp);
}

/* This is mainly for dosbtwrite */

static void  answer_asku(struct sockaddr_in *sinp, struct ua_pal *inmsg, const int inlng)
{
        int     nu = 0;
        struct  ua_asku_rep     reply;

        BLOCK_ZERO(&reply, sizeof(reply));

        if  (inlng == sizeof(struct ua_pal))  {
                int     cnt;

                for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
                        struct  udp_conn  *hp;

                        for  (hp = conn_hash[cnt];  hp;  hp = hp->next)  {
                                if  (strcmp(hp->username, inmsg->uap_name) == 0)  {
                                        reply.uau_ips[nu++] = hp->hostid;
                                        if  (nu >= UAU_MAXU)
                                                goto  dun;
                                }
                        }
                }
        }
 dun:
        reply.uau_n = htons(nu);
        udp_send_vec((char *) &reply, sizeof(reply), sinp);
}

/* Respond to keep alive messages assuming from Windows.  */

static void  tickle(struct sockaddr_in *sinp)
{
        struct  udp_conn  *fp = find_conn(sockaddr2int_netid_t(sinp), UNKNOWN_UID);
        char    repl = XBNQ_OK;
        if  (fp)
                fp->lastop = time((time_t *) 0);
        udp_send_vec(&repl, sizeof(repl), sinp);
}

void  process_ua()
{
        int     datalength;
        LONG    pmsgl[CL_SV_BUFFSIZE/sizeof(LONG)]; /* Force to long */
        char    *pmsg = (char *) pmsgl;
        struct  sockaddr_in     sin;
        SOCKLEN_T               sinl = sizeof(sin);

        while  ((datalength = recvfrom(uasock, pmsg, sizeof(pmsgl), 0, (struct sockaddr *) &sin, &sinl)) < 0)
                if  (errno != EINTR)
                        return;

        switch  (pmsg[0])  {
        default:
                job_reply(&sin, SV_CL_UNKNOWNC);
                return;

        case  CL_SV_UENQUIRY:
                do_enquiry(&sin, pmsg, datalength);
                return;

        case  UAL_UENQUIRE:
                udp_uenquire(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_ENQUIRE:
                udp_enquire(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_ULOGIN:
                udp_ulogin(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_LOGIN:
                udp_login(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  UAL_LOGOUT:
                udp_logout(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  CL_SV_STARTJOB:
        case  CL_SV_CONTJOB:
        case  CL_SV_JOBDATA:
        case  CL_SV_ENDJOB:
        case  CL_SV_HANGON:
                udp_job_process(&sin, pmsg, datalength);
                return;

        case  CL_SV_CREATEVAR:
                udp_crevar_process(&sin, (struct ni_createvar *) pmsg, datalength);
                return;

        case  CL_SV_DELETEVAR:
                udp_delvar_process(&sin, (struct ni_createvar *) pmsg, datalength);
                return;

        case  UAL_NEWGRP:
        case  UAL_INVG:
                udp_newgrp(&sin, (struct ua_login *) pmsg, datalength);
                return;

        case  CL_SV_ULIST:
                udp_send_uglist(&sin, gen_ulist);
                return;

        case  CL_SV_GLIST:
                udp_send_uglist(&sin, gen_glist);
                return;

        case  CL_SV_VLIST:
                udp_send_vlist(&sin, (struct ua_venq *) pmsg, datalength);
                return;

        case  CL_SV_CILIST:
                udp_send_cilist(&sin);
                return;

        case  CL_SV_HLIST:
                udp_send_hlist(&sin, pmsg, datalength);
                return;

        case  CL_SV_UMLPARS:
                udp_send_umlpars(&sin);
                return;

        case  CL_SV_ELIST:
                udp_send_elist(&sin);
                return;

        case  SV_SV_LOGGEDU:
        case  SV_SV_ASKALL:
                return;

        case  SV_SV_ASKU:
                answer_asku(&sin, (struct ua_pal *) pmsg, datalength);
                return;

        case  UAL_OK:           /* Actually these are redundant */
        case  UAL_NOK:
        case  CL_SV_KEEPALIVE:
                tickle(&sin);
                return;
        }
}

/* Tell the client we think it should wake up */

static void  send_prod(struct pend_job *pj)
{
        char    prodit = SV_CL_TOENQ;
        udp_send_to(&prodit, sizeof(prodit), pj->clientfrom);
}

/* See which UDP ports seem to have dried up Return time of next alarm.  */

unsigned  process_alarm()
{
        time_t  now = time((time_t *) 0);
        unsigned  mintime = 0, nexttime;
        int     prodtime, killtime, cnt;

        /* Scan list of jobs being added to see half-baked ones */

        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                if  (pend_list[cnt].clientfrom != 0L)  {
                        struct  pend_job  *pj = &pend_list[cnt];
                        prodtime = pj->lastaction + timeouts - now;
                        killtime = prodtime + timeouts;
                        if  (killtime < 0)  {
                                abort_job(pj); /* Must have died */
                                continue;
                        }
                        if  (prodtime <= 0)  {
                                send_prod(pj);
                                nexttime = killtime;
                        }
                        else
                                nexttime = prodtime;
                        if  (nexttime != 0  &&  (mintime == 0 || mintime > nexttime))
                                mintime = nexttime;
                }

        /* Apply timeouts to stale connections.  */

        for  (cnt = 0;  cnt < NETHASHMOD;  cnt++)  {
                struct  udp_conn        **hpp, *hp;

            restartch:

            	/* Have to restart chain at the beginning as freeing hp below clobbers pointer */

                hpp = &conn_hash[cnt];

                while  ((hp = *hpp))  {
                        long  tdiff = (long) (hp->lastop + timeouts) - now;
                        if  (tdiff <= 0)  {
                        	/* Take off chain and free stuff */
                                *hpp = hp->next;
                                free(hp->username);
                                free(hp->groupname);
                                free((char *) hp);
                                goto  restartch;
                        }
                        if  (mintime == 0 || mintime > (unsigned) tdiff)
                        	mintime = (unsigned) tdiff;
                        hpp = &hp->next;
                }
        }

        return  mintime;
}
