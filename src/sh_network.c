/* sh_network.c -- scheduler network monitor stuff

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
#include "incl_sig.h"
#include <sys/types.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
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
#ifdef  USING_MMAP
#include <sys/mman.h>
#endif
#include "incl_unix.h"
#include "errnums.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "sh_ext.h"
#include "services.h"

static  char    Filename[] = __FILE__;

/* It seems to take more than one attempt to set up a UDP port at
   times, so....  */

#define UDP_TRIES       3

#ifdef  BUGGY_SIGCLD
extern  int     nchild;
#endif

SHORT   listsock,
        viewsock,
        probesock;
USHORT  lportnum,               /* These are all htons-ified */
        vportnum,
        pportnum;
int     Netsync_req;

unsigned        lumpsize = DEF_LUMPSIZE,
                lumpwait = DEF_LUMPWAIT;

#define INC_REMOTES     4

struct  rem_list  {
        int     rl_nums,                /* Number on list */
                rl_max;                 /* Number allocated */
        struct  remote  **list;
};

struct  rem_list        probed,
                        connected;

static  struct  remote  *hashtab[NETHASHMOD];

PIDTYPE Netm_pid;               /* Process id of net monitor */

#ifdef  USING_FLOCK
#ifdef  USING_MMAP
extern  int     Xshmchan;
#else
extern  int     Xlockfd;
#endif

extern void  setjhold(const int);

static void  setlck(const unsigned startl, const unsigned lng)
{
        struct  flock   lck;
        lck.l_type = F_WRLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = startl;
        lck.l_len = lng;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Xshmchan, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Xlockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        ABORT_NOMEM;
        }
}

static void  unsetlck(const unsigned startl, const unsigned lng)
{
        struct  flock   lck;
        lck.l_type = F_UNLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = startl;
        lck.l_len = lng;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Xshmchan, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Xlockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        ABORT_NOMEM;
        }
}

#define holdjobs()      setjhold(F_RDLCK)
#define unholdjobs()    setjhold(F_UNLCK)
#else

/* Semaphore structures for creating job transfer buffers.
   NB Byebye SEM_UNDO */

static  struct  sembuf
lsem[1] =       {{TQ_INDEX, -1, 0 }},
ssem[2] =       {{TQ_INDEX,  1, 0 },
                 {       0, -1, 0 }},
ussem[1] =      {{       0,  1, 0 }};

static  struct  sembuf
jr[3] = {{      JQ_READING,     1,      0       },
        {       JQ_FIDDLE,      -1,     0       },
        {       JQ_FIDDLE,      1,      0       }},
jru[1] ={{      JQ_READING,     -1,     0       }};

#define holdjobs()      while  (semop(Sem_chan, &jr[0], 3) < 0 && errno == EINTR)
#define unholdjobs()    while  (semop(Sem_chan, &jru[0], 1) < 0 && errno == EINTR)

#endif

static ULONG  getxbuf()
{
        ULONG   result;

#ifdef  USING_FLOCK
        setlck(0, sizeof(ULONG));
#else
        while  (semop(Sem_chan, lsem, sizeof(lsem)/sizeof(struct sembuf)) < 0  &&  errno == EINTR)
                ;
#endif
        result = Xbuffer->Next;
        if  (++Xbuffer->Next >= XBUFJOBS)
                Xbuffer->Next = 0;
#ifdef  USING_MMAP
        msync((char *) Xbuffer, sizeof(ULONG), MS_ASYNC|MS_INVALIDATE);
#endif
#ifdef  USING_FLOCK
        unsetlck(0, sizeof(ULONG));
        setlck((char *) &Xbuffer->Ring[result] - (char *) Xbuffer, sizeof(Btjob));
#else
        ssem[1].sem_num = (short) result + TQ_INDEX + 1;
        while  (semop(Sem_chan, ssem, sizeof(ssem)/sizeof(struct sembuf)) < 0  &&  errno == EINTR)
                ;
#endif
        return  result;
}

static void  freexbuf(ULONG n)
{
#ifdef  USING_FLOCK
        unsetlck((char *) &Xbuffer->Ring[n] - (char *) Xbuffer, sizeof(Btjob));
#else
        ussem[0].sem_num = (short) n + TQ_INDEX + 1;
        while  (semop(Sem_chan, ussem, sizeof(ussem)/sizeof(struct sembuf)) < 0  &&  errno == EINTR)
                ;
#endif
}

/* Allocate a remote structure and copy the passed one (saves time in places).  */

static struct remote *new_remote(const struct remote *rp)
{
        struct  remote  *result;

        if  (!(result = (struct remote *) malloc(sizeof(struct remote))))
                ABORT_NOMEM;

        *result = *rp;
        return  result;
}

inline  void  add_chain(struct remote *rp)
{
        unsigned  hashv = calcnhash(rp->hostid);
        rp->hash_next = hashtab[hashv];
        hashtab[hashv] = rp;
}

/* Free remote structure from chain */

inline  void  free_chain(struct remote *rp)
{
        struct  remote  **rpp, *pp;
        rpp = &hashtab[calcnhash(rp->hostid)];
        while  ((pp = *rpp)  &&  pp != rp)
                rpp = &pp->hash_next;
        if  (pp)
                *rpp = rp->hash_next;
}

/* Allocate and free members of various lists.  */

static void  add_remlist(struct rem_list *rl, struct remote *rp)
{
        if  (rl->rl_nums >= rl->rl_max)  {
                rl->rl_max += INC_REMOTES;
                if  (rl->list)
                        rl->list = (struct remote **) realloc((char *) rl->list, (unsigned)(sizeof(struct remote *) * rl->rl_max));
                else
                        rl->list = (struct remote **) malloc((unsigned)(sizeof(struct remote *) * rl->rl_max));
                if  (!rl->list)
                        ABORT_NOMEM;
        }
        rl->list[rl->rl_nums++] = rp;
}

static void  free_remlist(struct rem_list *rl, struct remote *rp, const int errcode)
{
        int     cnt;

        for  (cnt = rl->rl_nums - 1;  cnt >= 0;  cnt--)
                if  (rl->list[cnt] == rp)  {
                        if  (--rl->rl_nums != cnt)
                                rl->list[cnt] = rl->list[rl->rl_nums];
                        return;
                }

        if  (errcode != 0)  {
                disp_str = rp->hostname;
                nfreport(errcode);
        }
}

/* Return the number of servers we are connected to */

int     get_nservers()
{
        int     result = 0,  cnt;

        for  (cnt = connected.rl_nums - 1;  cnt >= 0;  cnt--)
                if  ((connected.list[cnt]->stat_flags & SF_NOTSERVER) == 0  &&  connected.list[cnt]->is_sync == NSYNC_OK)
                        result++;
        return  result;
}

inline struct remote *find_host(const netid_t netid)
{
        struct  remote  *rp;
        for  (rp = hashtab[calcnhash(netid)];  rp;  rp = rp->hash_next)
                if  (rp->hostid == netid)
                        return  rp;
        return  (struct remote *) 0;
}

inline struct remote *next_samehost(struct remote *rp)
{
        netid_t  netid = rp->hostid;
        for  (rp = rp->hash_next;  rp;  rp = rp->hash_next)
                if  (rp->hostid == netid)
                        return  rp;
        return  (struct remote *) 0;
}

inline struct remote *inl_find_connected(const netid_t netid)
{
        struct  remote  *rp;
        for  (rp = find_host(netid);  rp;  rp = next_samehost(rp))
                if  (rp->stat_flags & SF_CONNECTED)
                        return  rp;
        return  (struct remote *) 0;
}

inline struct remote *next_connected(struct remote *rp)
{
        do   rp = next_samehost(rp);
        while  (rp  &&  !(rp->stat_flags & SF_CONNECTED));
        return  rp;
}

inline struct remote *inl_find_probe(const netid_t netid)
{
        struct  remote  *rp;
        for  (rp = hashtab[calcnhash(netid)];  rp;  rp = rp->hash_next)
                if  (rp->hostid == netid  &&  rp->stat_flags & SF_PROBED)
                        return  rp;
        return  (struct remote *) 0;
}

/* Note that this is only called in respect of machines known to be clients only
   for the benefit of *x machines which might have been servers. */

static void  clientonly(const struct remote *crp)
{
        struct  remote  *rp;
        if  (find_host(crp->hostid))            /* Don't think self is a possibility */
                return;
        rp = new_remote(crp);
        rp->stat_flags = SF_NOTSERVER;
}

/*  Called from outside, find a connection (for manual connect).
    If we have been told it is a client, skip it and find another */

struct  remote *find_connected(const netid_t netid)
{
        struct  remote  *rp = inl_find_connected(netid);
        while  (rp  &&  !(rp->stat_flags & SF_NOTSERVER))
                rp = next_connected(rp);
        return  rp;
}

/* Outside version of find_probe - just calls the inline kind */

struct remote *find_probe(const netid_t netid)
{
        return  inl_find_probe(netid);
}

/* Note "roaming user" - we currently only do anything for clients we absolutely
   know can't ever be servers as well. */

struct remote *alloc_roam(struct remote *rp)
{
        /* Xbnetserv (in tell_sched_roam) stuffs the remote uid in here
           to let us know it's a Win client. The hostid will be zero */

        if  (rp->remuid == UNKNOWN_UID  &&  rp->hostid != 0)
                clientonly(rp);
        return  rp;
}

/* Handle message advising that the caller isn't a server process
   (for when we've got clients speaking as well as btscheds) */

void    set_not_server(ShipcRef req, const int toset)
{
        struct  remote  *rp = inl_find_connected(req->sh_params.hostid);
        while  (rp)  {
                if  (rp->sockfd == req->sh_params.param)  {
                        /* We identify the correct remote by looking at the sock
et descriptor
                        which got passed in sh_params.param (see net_recv) */
                        if  (toset)
                                rp->stat_flags |= SF_NOTSERVER;
                        else
                                rp->stat_flags &= ~SF_NOTSERVER;
                        return;
                }
                rp = next_connected(rp);
        }
}

/* Try to attach to remote machine which may already be running.  */

struct  remote  *conn_attach(struct remote *prp)
{
        int     sk;
        struct  remote  *rp;
        struct  sockaddr_in     sin;

        if  ((sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                return  0;

        sin.sin_family = AF_INET;
        sin.sin_port = lportnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        BLOCK_COPY(&sin.sin_addr, &prp->hostid, sizeof(netid_t));

        if  (connect(sk, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
                close(sk);
                return  0;
        }

        /* If it was a probe, we re-use the struct remote, taking it off the probe list. */

        if  (prp->stat_flags & SF_PROBED)  {
                free_remlist(&probed, prp, $E{Hash function error free_probe});
                rp = prp;
        }
        else  {         /* Otherwise allocate a new one */
                rp = new_remote(prp);
                add_chain(rp);
        }

        rp->sockfd = sk;
        rp->stat_flags = SF_ISCLIENT | SF_CONNECTED;
        add_remlist(&connected, rp);
        rp->is_sync = NSYNC_NONE;
        rp->lastwrite = time((time_t *) 0);
        Netsync_req++;
        return  rp;
}

static int  probe_send(const netid_t hostid, struct netmsg *pmsg)
{
        int     sockfd, tries;
        struct  sockaddr_in     serv_addr, cli_addr;

        BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = hostid;
        serv_addr.sin_port = pportnum;

        BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        cli_addr.sin_port = 0;

        /* We don't really need the cli_addr but we are obliged to
           bind something.  The remote uses our "pportnum".  */

        for  (tries = 0;  tries < UDP_TRIES;  tries++)  {
                if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)  {
                        disp_arg[0] = ntohs(pportnum);
                        nfreport($E{Panic trouble creating probe socket});
                        return  0;
                }
                if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
                        disp_arg[0] = ntohs(pportnum);
                        nfreport($E{Panic trouble binding probe socket});
                        close(sockfd);
                        return  0;
                }
                if  (sendto(sockfd, (char *) pmsg, sizeof(struct netmsg), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) >= 0)  {
                        close(sockfd);
                        return  1;
                }
                close(sockfd);
        }

        /* Failed....  */

        disp_arg[0] = ntohs(pportnum);
        nfreport($E{Panic trouble sending on probe socket});
        return  0;
}

/* Initiate connection by doing UDP probe first.
   The net monitor process deals with the reply, or "nettickle" discovers that
   it's not worth bothering about.  */

void  probe_attach(struct remote *prp)
{
        struct  remote  *rp;
        struct  netmsg  pmsg;

        pmsg.hdr.code = htons(N_CONNECT);
        pmsg.hdr.length = htons(sizeof(struct netmsg));
        pmsg.hdr.hostid = myhostid;
        pmsg.arg = 0;

        /* We always at least send the probe, worry if it is already
           connected later */

        if  (!probe_send(prp->hostid, &pmsg))
                return;

        /* If we did it already, refresh the time and exit */

        if  ((rp = inl_find_probe(prp->hostid)))  {
                rp->lastwrite = time((time_t *) 0);
                return;
        }

        /* See if we think that there is a connection. We may get this wrong if there are clients
           around who haven't said that they are but hopefully we'll push through that change soon.
           If "I" have connected to the other guy already it shouldn't reconnect either. */

        for  (rp = inl_find_connected(prp->hostid);  rp;  rp = next_connected(rp))
                if  ((rp->stat_flags & (SF_ISCLIENT|SF_NOTSERVER)) == 0)
                        return;

        rp = new_remote(prp);
        rp->stat_flags = SF_PROBED|SF_ISCLIENT;
        rp->is_sync = NSYNC_NONE;
        rp->lastwrite = time((time_t *) 0);

        add_remlist(&probed, rp);
        add_chain(rp);
}

void  reply_probe()
{
        netid_t whofrom;
        struct  remote  *rp;
        struct  netmsg  pmsg;
        Shipc   nmsg;
        SOCKLEN_T               repl = sizeof(struct sockaddr_in);
        struct  sockaddr_in     reply_addr;

        if  (recvfrom(probesock, (char *) &pmsg, sizeof(pmsg), 0, (struct sockaddr *) &reply_addr, &repl) < 0)
                return;

        /* Get who it's from from the reply address rather than believing what it says in
           the message packet. If it's "me" ignore it as we could go on forever */

        if  ((whofrom = sockaddr2int_netid_t(&reply_addr)) == 0)
                return;

        /* See if caller has got IP right.
           If it's zeros, it's just to find it out so we don't argue,
           otherwise we make a report */

        if  (pmsg.hdr.hostid != whofrom)  {
                pmsg.hdr.code = N_WRONGIP;
                if  (pmsg.hdr.hostid != 0L)  {
                        struct  in_addr  wip;
                        /* Have to do this as two messages as inet_ntoa overwrites buffer each time */
                        wip.s_addr = pmsg.hdr.hostid;
                        disp_str = inet_ntoa(wip);
                        nfreport($E{Probe incorrect IP 1});
                        wip.s_addr = whofrom;
                        disp_str = inet_ntoa(wip);
                        nfreport($E{Probe incorrect IP 2});
                        /* The other end will have set up its own socket to send back on */
                        pmsg.hdr.hostid = whofrom;
                        probe_send(whofrom, &pmsg);
                }
                else  {
                        /* If it was just an enquiry (0 in the hostid) send it back the way it came */
                        pmsg.hdr.hostid = whofrom;
                        sendto(probesock, (char *) &pmsg, sizeof(struct netmsg), 0, (struct sockaddr *) &reply_addr, sizeof(reply_addr));
                }
                return;
        }

        switch  (ntohs(pmsg.hdr.code))  {
        default:
                return;         /* Forget it */

        case  N_CONNECT:

                /* Probe connect - just bounce it back */

                pmsg.hdr.code = htons(N_CONNOK);
                pmsg.hdr.length = htons(sizeof(struct netmsg));
                pmsg.hdr.hostid = myhostid;
                pmsg.arg = 0;
                probe_send(whofrom, &pmsg);
                return;

        case  N_CONNOK:

                /* Connection ok - forget it if we weren't interested
                   in that processor (possibly because it's ancient).
                   Otherwise we send a message to the scheduler process proper
                   and exit to be regenerated.  */

                if  ((rp = inl_find_probe(whofrom)))  {
                        BLOCK_ZERO(&nmsg, sizeof(nmsg));
                        nmsg.sh_mtype = TO_SCHED;
                        nmsg.sh_params.mcode = N_PCONNOK;
                        nmsg.sh_un.sh_n = *rp;
                        msgsnd(Ctrl_chan, (struct msgbuf *) &nmsg, sizeof(Shreq) + sizeof(struct remote), 0);
                        exit(0);
                }
                return;
        }
}

/* Attach remote, either immediately, or by doing probe operation first.
   Return the struct remote if we get through immediately,
   otherwise null (for the benefit of sendsync) */

struct  remote  *rattach(struct remote *prp)
{
        if  (prp->hostid == 0L  ||  prp->hostid == myhostid)
                return  (struct remote *) 0;
        if  (prp->ht_flags & HT_PROBEFIRST)  {
                probe_attach(prp);
                return  (struct remote *) 0;
        }
        return  conn_attach(prp);
}

/* When we seem to have a duplicate connection.
   This might be because we have another one coming the other way at the same time.
   We might be wrong here until all clients tell us they are clients */

static void dump_connection(int sk, netid_t hostid)
{
        struct  netmsg  rq;
        BLOCK_ZERO(&rq, sizeof(rq));
        rq.hdr.code = htons(N_SHUTHOST);
        rq.hdr.length = htons(sizeof(struct netmsg));
        rq.hdr.hostid = myhostid;
        Ignored_error = write(sk, (char *) &rq, sizeof(rq));
        disp_str = look_host(hostid);
        nfreport($E{Reconnection whilst still connected});
        close(sk);
}

/* Accept connection from new machine */

void  newhost()
{
        int     newsock;
        int     definitely_nonserv = 0;
        netid_t hostid;
        struct  remote  *rp;
        struct  sockaddr_in     sin;
        SOCKLEN_T       sinl;

        sinl = sizeof(sin);
        if  ((newsock = accept(listsock, (struct sockaddr *) &sin, &sinl)) < 0)
                return;

        /* If it's from the local machine, it must be a client thing */

        hostid = sockaddr2int_netid_t(&sin);
        if  (hostid == 0)  {
                rp = (struct remote *) malloc(sizeof(struct remote));
                if  (!rp)
                        ABORT_NOMEM;
                BLOCK_ZERO(rp, sizeof(struct remote));
                rp->ht_flags = HT_MANUAL;
                rp->stat_flags = SF_CONNECTED | SF_NOTSERVER;
        }
        else  {
                /* If we have a probe pending, reject it.
                   It's just possible the other end is trying to call us at exactly the same time.
                   However it's better to just reject at both ends I think */

                if  ((rp = inl_find_probe(hostid)))  {
                        dump_connection(newsock, hostid);
                        free_remlist(&probed, rp, 0);
                        free_chain(rp);
                        free((char *) rp);
                        return;
                }

                /* We might be connected already, as a client and this is a server or
                   vice versa we don't know yet. All we can do at the moment is see if we
                   have a record with it being marked not server without it being connected */

                for  (rp = find_host(hostid);  rp;  rp = next_samehost(rp))
                        if  ((rp->stat_flags & (SF_CONNECTED|SF_NOTSERVER)) == SF_NOTSERVER)  {
                                definitely_nonserv = 1;
                                break;
                        }

                rp = (struct remote *) malloc(sizeof(struct remote));
                if  (!rp)
                        ABORT_NOMEM;
                BLOCK_ZERO(rp, sizeof(struct remote));
                rp->ht_flags = HT_MANUAL;
                rp->stat_flags = SF_CONNECTED;

                /* If this IP is marked as not a server, then set the flag */

                if  (definitely_nonserv)
                        rp->stat_flags |= SF_NOTSERVER;
        }

        rp->sockfd = newsock;
        rp->hostid = hostid;
        rp->is_sync = NSYNC_OK;
        rp->ht_timeout = NETTICKLE;
        rp->lastwrite = time((time_t *) 0);
        add_chain(rp);
        add_remlist(&connected, rp);
}

/* The following are service names so that we can find a suitable port number.
   WARNING: We must run as root if we want to use privileged port numbers < 1024.  */

static  char    *servnames[] = {
        CONNECTPORT_NAME1,
        CONNECTPORT_NAME2,
        CONNECTPORT_NAME3,
        CONNECTPORT_NAME4
};
static  char    *vservnames[] = {
        VIEWPORT_NAME1,
        VIEWPORT_NAME2
};

/* Go down a list of possible port names and return the first port number we find.
   NB Returned in net byte order.
   Return 0 if not found. */

static  int     get_serv_port(char **slist, const int proto, unsigned sn)
{
        struct  servent  *sp;
        char    **slp;

        sn /= sizeof(char *);
        for  (slp = slist;  slp < &slist[sn];  slp++)
                if  ((sp = env_getserv(*slp, proto)))
                        return  sp->s_port;
        return  0;
}

/* Attach hosts if possible */

void  attach_hosts()
{
        struct  remote  *rp;
        char    *ep;
        int     possp;
        extern  char    hostf_errors;

        if  ((ep = envprocess(LUMPSIZE))  &&  (lumpsize = (unsigned) atoi(ep)) == 0)
                lumpsize = DEF_LUMPSIZE;
        if  ((ep = envprocess(LUMPWAIT))  &&  (lumpwait = (unsigned) atoi(ep)) == 0)
                lumpwait = DEF_LUMPWAIT;

        lportnum = pportnum = get_serv_port(servnames, IPPROTO_TCP, sizeof(servnames));
        if  (lportnum == 0)  {
                panic($E{No listening port});
                endservent();
                return;
        }

        /* Get port number for probe port, if not found use the same as above.  */

        possp = get_serv_port(servnames, IPPROTO_UDP, sizeof(servnames));
        if  (possp != 0)
                pportnum = possp;

        /* Get port number for view port, if not found use the connect port + 1 */

        vportnum = get_serv_port(vservnames, IPPROTO_TCP, sizeof(vservnames));
        if  (vportnum == 0)  {
                /* Do this in two steps because sometimes ntohs and htons are asm statements
                   which go wrong if you try to do too much. */
                possp = ntohs((USHORT) lportnum) + 1;
                vportnum = htons(possp);
        }
        endservent();

        /* Now set up "listening" socket */

        if  ((listsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0)  {
                struct  sockaddr_in     sin;
#ifdef  SO_REUSEADDR
                int     on = 1;
                setsockopt(listsock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
                sin.sin_family = AF_INET;
                sin.sin_port = lportnum;
                BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
                sin.sin_addr.s_addr = INADDR_ANY;
                if  (bind(listsock, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(listsock, 5) < 0)  {
                        close(listsock);
                        listsock = -1;
                        disp_arg[0] = ntohs(lportnum);
                        panic($E{Panic cannot create listen socket});
                }
        }

        /* Now set up "viewing/feeding" socket */

        if  ((viewsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) >= 0)  {
                struct  sockaddr_in     sin;
#ifdef  SO_REUSEADDR
                int     on = 1;
                setsockopt(viewsock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
                sin.sin_family = AF_INET;
                sin.sin_port = vportnum;
                BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
                sin.sin_addr.s_addr = INADDR_ANY;
                if  (bind(viewsock, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(viewsock, 5) < 0)  {
                        close(viewsock);
                        viewsock = -1;
                        disp_arg[0] = ntohs(vportnum);
                        panic($E{Panic trouble creating feeder socket});
                }
        }

        /* Now set up Datagram probe socket */

        if  ((probesock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) >= 0)  {
                struct  sockaddr_in     sin;

                sin.sin_family = AF_INET;
                sin.sin_port = pportnum;
                BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
                sin.sin_addr.s_addr = INADDR_ANY;
                if  (bind(probesock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
                        close(probesock);
                        probesock = -1;
                        disp_arg[0] = ntohs(pportnum);
                        panic($E{Panic trouble creating probe uda port});
                }
        }

        /* See comment about this line in creatjfile().  We make it
           work whatever order this routine and creatjfile() are
           called in.  */

        Job_seg.dptr->js_viewport = vportnum;

        /* Now parse host file and attach as appropriate */

        while  ((rp = get_hostfile()))
                if  ((rp->ht_flags & (HT_MANUAL|HT_DOS|HT_ROAMUSER)) == 0)
                        rattach(rp);
        end_hostfile();
        if  (hostf_errors)
                nfreport($E{Warn errors in host file});
}

/* Invoked to shut down connection and deallocate structures */

static void  deallochost(struct remote *rp)
{
        /* Remove traces of jobs and variables on our machine associated with
           the dying machine. */

        if  (rp->hostid)  {             /* Don't remove our own! */
                netlock_hostdied(rp);   /* In case we thought that the machine had the lock */
                net_jclear(rp->hostid); /* Must clean jobs first */
                net_vclear(rp->hostid); /* As need to keep track of vars */
        }

        free_chain(rp);

        if  (rp->stat_flags & SF_CONNECTED)  {
                close(rp->sockfd);
                if  (rp->is_sync != NSYNC_OK)
                        Netsync_req--;
                free_remlist(&connected, rp, 0);
        }
        if  (rp->stat_flags & SF_PROBED)
                free_remlist(&probed, rp, $E{Hash function error free_probe});

        free((char *) rp);
}

/* Write to socket, but if we get some error, treat connection as
   down.  Return 0 in that case so we can break out of loops.  */

static int  chk_write(struct remote *rp, char *buffer, unsigned length)
{
        int     nbytes;
        while  (length != 0)  {
                if  ((nbytes = write(rp->sockfd, buffer, length)) < 0)  {
                        /* Kill off network monitor process.  */
                        kill(Netm_pid, NETSHUTSIG);
                        deallochost(rp);
                        return  0;
                }
                length -= nbytes;
                buffer += nbytes;
        }
        rp->lastwrite = time((time_t *) 0);
        return  1;
}

/* Remove structures associated with host, if sockfd >= 0 then specific
   to that connection. */

void  clearhost(const netid_t netid, const int sockfd)
{
        struct  remote  *rp = inl_find_connected(netid);

        /* If sockfd is positive, we are specifically disconnecting
           an existing connection */

        if  (sockfd >= 0)  {
                while  (rp)  {
                        if  (rp->sockfd == sockfd)  {
                                deallochost(rp);
                                break;
                        }
                        rp = next_connected(rp);
                }
                return;
        }

        /* First go through connected ones */

        while  (rp)  {
                struct  remote  *nxtrp = next_connected(rp);    /* save this cous deallochost zaps the link */
                if  (!(rp->stat_flags & SF_NOTSERVER))          /* This will zap clients that haven't told us they are */
                        deallochost(rp);
                rp = nxtrp;
        }

        /* We expect only one probe though */

        if  ((rp = inl_find_probe(netid)))
                deallochost(rp);
}

/* Read from TCP socket and join together the bits which things
   sometimes get split into.  */

static void  read_sock(struct remote *rp, char *rqb, unsigned size)
{
        Shipc   omsg;

        for  (;;)  {
                int  nbytes = read(rp->sockfd, rqb, size);
                if  (nbytes <= 0)  {
                        if  (nbytes == 0  ||  errno != EINTR)
                                break;
                }
                else  {
                        if  (nbytes == size)
                                return;
                        size -= nbytes;
                        rqb += nbytes;
                }
        }

        /* If we get any kind of error or no bytes are read, then
           treat other end as shut down.  */

        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = N_SHUTHOST;
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_params.param = rp->sockfd;
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq), 0);
        exit(0);
}

/* Use this macro to read from a socket because peek doesn't work on
   so many systems that we have to really read "act" first.  */

#define READ_SOCK(rp, rqb, act) read_sock(rp, sizeof(act) + (char *) &rqb, sizeof(rqb) - sizeof(act))

/* Broadcast a message to all known hosts */

void  broadcast(char *msg, const unsigned size)
{
        int     cnt;

        /* We do this loop backwards so if one of them is found to be
           dead, the "chk_write" logic will move the end one
           which we've done over the top of the one we just tried
           to do.  */

        for  (cnt = connected.rl_nums - 1;  cnt >= 0;  cnt--)
                chk_write(connected.list[cnt], msg, size);
}

void  net_broadcast(const USHORT code)
{
        struct  netmsg  rq;
        BLOCK_ZERO(&rq, sizeof(rq));
        rq.hdr.code = htons(code);
        rq.hdr.length = htons(sizeof(struct netmsg));
        rq.hdr.hostid = myhostid;
        broadcast((char *) &rq, sizeof(rq));
}

void  net_recvlockreq(struct remote *rp, msghdr *hdr)
{
        int     ret;
        struct  netmsg  nm;
        struct  netmsg  onm;

        nm.hdr = *hdr;
        READ_SOCK(rp, nm, msghdr);

        /* NOTE THAT WE ARE NOT BOTHERING WITH BYTE SWAPS!!!!  */

        if  (ntohs(nm.hdr.code) == N_WANTLOCK)  {
                if  (forksafe() != 0)
                        return;
                ret = net_lockreq(rp);
                if  (ret != N_REMLOCK_NONE)  {
                        onm = nm;
                        onm.hdr.hostid = myhostid;
                        onm.hdr.code = htons((USHORT) ret);
                        chk_write(rp, (char *) &onm, sizeof(onm));
                }
                exit(0);
        }
        else
                net_unlockreq(rp);
}

/* Tell our friends byebye */

void  netshut()
{
        struct  netmsg          rq;
        BLOCK_ZERO(&rq, sizeof(rq));
        kill(Netm_pid, NETSHUTSIG);
        rq.hdr.code = htons(N_SHUTHOST);
        rq.hdr.length = htons(sizeof(struct netmsg));
        rq.hdr.hostid = myhostid;
        broadcast((char *) &rq, sizeof(rq));
}

/* Tell one host goodbye */

void  shut_host(const netid_t hostid)
{
        struct  remote  *rp;
        struct  netmsg  rq;

        if  (!(rp = inl_find_connected(hostid)))
                return;

        BLOCK_ZERO(&rq, sizeof(rq));
        rq.hdr.code = htons(N_SHUTHOST);
        rq.hdr.length = htons(sizeof(struct netmsg));
        rq.hdr.hostid = myhostid;

        do  {
                if  (!(rp->stat_flags & SF_NOTSERVER))
                        chk_write(rp, (char *) &rq, sizeof(rq));
                rp = next_connected(rp);
        }  while  (rp);
}

/* Keep connections alive */

unsigned  nettickle()
{
        int             cnt;
        unsigned        result;
        time_t          now;
        struct  netmsg  rq;

        if  (connected.rl_nums <= 0)
                return  0;

        now = time((time_t *) 0);
        result = 0;

        BLOCK_ZERO(&rq, sizeof(rq));
        rq.hdr.code = htons(N_TICKLE);
        rq.hdr.length = htons(sizeof(struct netmsg));
        rq.hdr.hostid = myhostid;

        for  (cnt = connected.rl_nums - 1;  cnt >= 0;  cnt--)  {
                struct  remote  *rp = connected.list[cnt];
                unsigned  tleft = now - rp->lastwrite;

                /* If it was last written twice as long ago, send it a
                   null 'tickle' message.  If it was less than
                   that, but more than the "tickle" time, set
                   result to the shortest time of any connection
                   up to that time */

                if  (tleft >= rp->ht_timeout * 2)
                        chk_write(rp, (char *) &rq, sizeof(rq));
                else  if  (tleft > rp->ht_timeout)  {
                        tleft = rp->ht_timeout*2 - tleft;
                        if  (result == 0  ||  tleft < result)
                                result = tleft;
                }
        }

        /* For any pending operations, abandon any which have not had
           a reply up to the timeout.  */

        for  (cnt = probed.rl_nums - 1;  cnt >= 0;  cnt--)  {
                struct  remote  *rp = probed.list[cnt];
                unsigned  tleft = now - rp->lastwrite;
                if  (tleft > rp->ht_timeout)
                        deallochost(rp);
        }

        return  result;
}

/* Broadcast information about job state changes */

void  job_statbroadcast(BtjobhRef jp)
{
        struct  jobstatmsg  jsm;
        BLOCK_ZERO(&jsm, sizeof(jsm));
        jsm.hdr.code = htons(J_CHSTATE);
        jsm.hdr.length = htons(sizeof(struct jobstatmsg));
        jsm.hdr.hostid = myhostid;
        jid_pack(&jsm.jid, jp);
        jsm.prog = jp->bj_progress;
        jsm.nexttime = htonl((LONG) jp->bj_times.tc_nexttime);
        jsm.lastexit = htons(jp->bj_lastexit);
        jsm.lastpid = htonl(jp->bj_pid);
        jsm.runhost = int2ext_netid_t(jp->bj_runhostid);
        broadcast((char *) &jsm, sizeof(jsm));
}

/* Other end of above routine */

void  job_statrecvbcast(struct remote *rp, msghdr *hdr)
{
        int     jn;
        BtjobhRef       jp;
        struct  jobstatmsg      jsm;
        Shipc   omsg;

        jsm.hdr = *hdr;
        READ_SOCK(rp, jsm, msghdr);

#ifndef WORDS_BIGENDIAN
        jsm.hdr.code = ntohs(hdr->code);
        jsm.hdr.length = ntohs(hdr->length);
        jsm.hdr.pid = ntohl(hdr->pid);
#endif

        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_un.remstat.jid.hostid = ext2int_netid_t(jsm.jid.hostid);
        omsg.sh_un.remstat.jid.slotno = ntohl(jsm.jid.slotno);

        /* Suppress state change messages about jobs we are running,
           to prevent us obliterating the state with some fossil
           state.  This is easier than having the broadcasting
           process avoid sending it.  */

        holdjobs();
        if  ((jn = findj_by_jid(&omsg.sh_un.remstat.jid)) >= 0)  {
                jp = &Job_seg.jlist[jn].j.h;
                if  (jp->bj_progress >= BJP_DONE  &&  jp->bj_progress != BJP_CANCELLED  &&  jp->bj_runhostid == 0)  {
                        unholdjobs();
                        return;
                }
        }
        unholdjobs();

        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jsm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_un.remstat.prog = jsm.prog;
        omsg.sh_un.remstat.nexttime = (time_t) ntohl(jsm.nexttime);
        omsg.sh_un.remstat.lastexit = ntohs(jsm.lastexit);
        omsg.sh_un.remstat.lastpid = ntohl(jsm.lastpid);
        omsg.sh_un.remstat.runhost = ext2int_netid_t(jsm.runhost);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(struct jstatusmsg), 0);
}

/* Invoked by "remote run" to change status of the owning machine's job */

void  job_rrchstat(BtjobhRef jp)
{
        struct  remote  *rp;
        struct  jobstatmsg  jsm;

        /* Use the external version we only want servers */
        if  (!(rp = find_connected(jp->bj_hostid)))
                return;

        BLOCK_ZERO(&jsm, sizeof(jsm));
        jsm.hdr.code = htons(J_RRCHANGE);
        jsm.hdr.length = htons(sizeof(struct jobstatmsg));
        jsm.hdr.hostid = jsm.runhost = myhostid;
        jid_pack(&jsm.jid, jp);
        jsm.prog = jp->bj_progress;
        jsm.nexttime = htonl(jp->bj_times.tc_nexttime);
        jsm.lastexit = htons(jp->bj_lastexit);
        jsm.lastpid = htonl(jp->bj_pid);
        chk_write(rp, (char *) &jsm, sizeof(jsm));
}

/* Used by owning host of job to receive messages about state changes
   in processes running remotely. */

void  job_rrstatrecv(struct remote *rp, msghdr *hdr)
{
        struct  jobstatmsg      jsm;
        Shipc   omsg;

        jsm.hdr = *hdr;
        READ_SOCK(rp, jsm, msghdr);

#ifndef WORDS_BIGENDIAN
        jsm.hdr.code = ntohs(hdr->code);
        jsm.hdr.length = ntohs(hdr->length);
        jsm.hdr.pid = ntohl(hdr->pid);
#endif

        BLOCK_ZERO(&omsg, sizeof(omsg));
        /* omsg.sh_un.remstat.jid.hostid = 0; It has to be my hostid */
        omsg.sh_un.remstat.jid.slotno = ntohl(jsm.jid.slotno);

        /* Avoid checks for jobs we are running since by premise of this routine, we aren't */

        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jsm.hdr.code;
        omsg.sh_params.hostid = omsg.sh_un.remstat.runhost = rp->hostid;
        omsg.sh_un.remstat.prog = jsm.prog;
        omsg.sh_un.remstat.nexttime = ntohl(jsm.nexttime);
        omsg.sh_un.remstat.lastexit = ntohs(jsm.lastexit);
        omsg.sh_un.remstat.lastpid = ntohl(jsm.lastpid);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(struct jstatusmsg), 0);
}

void  job_imessbcast(CBtjobhRef jp, const unsigned mcode, const LONG param)
{
        struct  jobcmsg         jcm;

        BLOCK_ZERO(&jcm, sizeof(jcm));
        jcm.hdr.code = htons((USHORT) mcode);
        jcm.hdr.length = htons(sizeof(jcm));
        jcm.hdr.hostid = myhostid;
        jid_pack(&jcm.jid, jp);
        jcm.param = htonl(param);
        broadcast((char *) &jcm, sizeof(jcm));
}

void  job_imessrecvbcast(struct remote *rp, msghdr *hdr)
{
        struct  jobcmsg jcm;
        Shipc           omsg;

        jcm.hdr = *hdr;
        READ_SOCK(rp, jcm, msghdr);

#ifndef WORDS_BIGENDIAN
        jcm.hdr.code = ntohs(hdr->code);
        jcm.hdr.length = ntohs(hdr->length);
        jcm.hdr.pid = ntohl(hdr->pid);
#endif
        omsg.sh_params.uuid = 0;
        omsg.sh_params.ugid = 0;
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jcm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_params.param = ntohl(jcm.param);
        omsg.sh_un.jobref.hostid = ext2int_netid_t(jcm.jid.hostid);
        omsg.sh_un.jobref.slotno = ntohl(jcm.jid.slotno);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(jident), 0);
}

/* Broadcast information about changes to jobs excluding strings */

void  job_hbroadcast(BtjobhRef jp, const unsigned code)
{
        struct  jobhnetmsg  jhnm;
        jobh_pack(&jhnm, jp);
        jhnm.hdr.code = htons(code);
        jhnm.hdr.hostid = myhostid;
        broadcast((char *) &jhnm, sizeof(jhnm));
}

/* Other end of above routine */


void  job_hrecvbcast(struct remote *rp, msghdr *hdr)
{
        ULONG           indx;
        long            mymtype;
        BtjobRef        Jreq;
        struct  jobhnetmsg      jhnm;
        Shipc           omsg;
        Repmess         rep;

        jhnm.hdr = *hdr;
        READ_SOCK(rp, jhnm, msghdr);

        if  (forksafe() != 0)
                return;

#ifndef WORDS_BIGENDIAN
        jhnm.hdr.code = ntohs(hdr->code);
        jhnm.hdr.length = ntohs(hdr->length);
        jhnm.hdr.pid = ntohl(hdr->pid);
#endif
        Jreq = &Xbuffer->Ring[indx = getxbuf()];
        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jhnm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        mymtype = omsg.sh_params.upid = getpid();
        omsg.sh_un.sh_jobindex = indx;
        jobh_unpack(&Jreq->h, &jhnm);
#ifdef  USING_MMAP
        msync((char *) Xbuffer, sizeof(struct Transportbuf), MS_ASYNC|MS_INVALIDATE);
#endif
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(ULONG), 0);
        msgrcv(Ctrl_chan, (struct msgbuf *) &rep, sizeof(Shreq), mymtype + MTOFFSET, 0);
        freexbuf(indx);
        exit(0);
}

/* Broadcast information about changes to jobs including strings */

void  job_broadcast(BtjobRef jp, const unsigned code)
{
        unsigned        jlen;
        struct  jobnetmsg       jm;
        jlen = job_pack(&jm, jp);
        jm.hdr.hdr.code = htons(code);
        jm.hdr.hdr.hostid = myhostid;
        broadcast((char *) &jm, jlen);
}

/* Other end of above routine */

void  job_recvbcast(struct remote *rp, msghdr *hdr)
{
        ULONG           indx;
        long            mymtype;
        BtjobRef        Jreq;
        struct  jobnetmsg       jnm;
        Shipc           omsg;
        Repmess         rr;

        jnm.hdr.hdr = *hdr;

        /* We need the length here....  */

#ifndef WORDS_BIGENDIAN
        jnm.hdr.hdr.code = ntohs(hdr->code);
        jnm.hdr.hdr.length = ntohs(hdr->length);
        jnm.hdr.hdr.pid = ntohl(hdr->pid);
#endif

        /* Length varies, of course so we spell it out...  */

        read_sock(rp, sizeof(msghdr) + (char *) &jnm, jnm.hdr.hdr.length - sizeof(msghdr));

        if  (forksafe() != 0)
                return;

        Jreq = &Xbuffer->Ring[indx = getxbuf()];
        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jnm.hdr.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        mymtype = omsg.sh_params.upid = getpid();
        omsg.sh_un.sh_jobindex = indx;
        job_unpack(Jreq, &jnm);
#ifdef  USING_MMAP
        msync((char *) Xbuffer, sizeof(struct Transportbuf), MS_ASYNC|MS_INVALIDATE);
#endif
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(ULONG), 0);
        msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype + MTOFFSET, 0);
        freexbuf(indx);
        exit(0);
}

/* Send out messages to perform the assignments on the list of jobs */

void  remasses(BtjobhRef jh, const USHORT flag, const USHORT source, const USHORT status)
{
        struct  rassmsg rm;

        BLOCK_ZERO(&rm, sizeof(rm));
        rm.hdr.code = htons(N_RJASSIGN);
        rm.hdr.length = htons(sizeof(rm));
        rm.hdr.hostid = myhostid;
        jid_pack(&rm.jid, jh);
        rm.flags = (unsigned char) flag;
        rm.source = (unsigned char) source;
        rm.status = htons(status);
        broadcast((char *) &rm, sizeof(rm));
}

/* Other end of above routine */

void  recv_remasses(struct remote *rp, msghdr *hdr)
{
        struct  rassmsg rm;
        Shipc   omsg;

        rm.hdr = *hdr;
        READ_SOCK(rp, rm, msghdr);
#ifndef WORDS_BIGENDIAN
        rm.hdr.code = ntohs(hdr->code);
        rm.hdr.length = ntohs(hdr->length);
        rm.hdr.pid = ntohl(hdr->pid);
#endif
        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = rm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_un.remas.jid.hostid = ext2int_netid_t(rm.jid.hostid);
        omsg.sh_un.remas.jid.slotno = ntohl(rm.jid.slotno);
        omsg.sh_un.remas.flags = rm.flags;
        omsg.sh_un.remas.source = rm.source;
        omsg.sh_un.remas.status = ntohs(rm.status);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(struct jremassmsg), 0);
}

/* Alarm-catching routine for ureply */

static  int     uhadalarm = 0;

static RETSIGTYPE  urephad(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(SIGALRM, urephad);
#endif
        uhadalarm++;
}

/* Despatched message - wait for reply */

static unsigned  ureply(struct remote *rp, CShreqRef sr)
{
        PIDTYPE ret;
        Repmess rep;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
#endif

        if  ((ret = forksafe()) < 0)
                return  N_NOFORK;
        if  (ret != 0)
                return  REP_AMPARENT;

        /* Set alarm in case network dies.
           Wait for reply from net monitor process */

#ifdef  STRUCT_SIG
        z.sighandler_el = urephad;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGALRM, &z, (struct sigstruct_name *) 0);
#else
        signal(SIGALRM, urephad);
#endif
        alarm(rp->ht_timeout);
        if  (msgrcv(Ctrl_chan, (struct msgbuf *) &rep, sizeof(rep) - sizeof(long), sr->upid + NTOFFSET, 0) < 0)  {
                if  (errno != EINTR)
                        return  N_NBADMSGQ;
                return  N_NTIMEOUT;
        }
        alarm(0);       /* Doesn't really matter */
        return  (unsigned) rep.outmsg.param | SHREQ_CHILD;
}

/* Send back message from scheduler process on this machine to the
   network monitor process on the machine which sent the request
   telling it about the outcome.  */

static void  mreply(struct remote *rp, const int_pid_t pid, const ULONG mcode)
{
        struct  netmsg          reply;
        BLOCK_ZERO(&reply, sizeof(reply));
        reply.hdr.code = htons(N_REQREPLY);
        reply.hdr.length = htons(sizeof(struct netmsg));
        reply.hdr.hostid = myhostid;
        reply.hdr.pid = htonl(pid); /* This gives the originating
                                        (sched child) process on remote */
        reply.arg = htonl(mcode);
        chk_write(rp, (char *) &reply, sizeof(reply));
}

/* Send update request about a job to the owning machine */

unsigned  job_sendupdate(BtjobRef oldjob, BtjobRef newjob, ShreqRef sr, const unsigned code)
{
        struct  remote  *rp;

        /* Make sure that these are known quantities */

        newjob->h.bj_job = oldjob->h.bj_job;
        newjob->h.bj_hostid = oldjob->h.bj_hostid;
        newjob->h.bj_slotno = oldjob->h.bj_slotno;

        if  (!(rp = inl_find_connected(newjob->h.bj_hostid)))
                return  N_HOSTOFFLINE;

        if  (code == J_HCHANGED)  {
                struct  jobhnetmsg      jhm;
                jobh_pack(&jhm, &newjob->h);
                jhm.hdr.code = htons(code);
                jhm.hdr.hostid = myhostid;
                jhm.hdr.pid = htonl(sr->upid);
                strncpy(jhm.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
                strncpy(jhm.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
                if  (!chk_write(rp, (char *) &jhm, sizeof(jhm)))
                        return  N_HOSTDIED;
        }
        else  {
                unsigned  jlen;
                struct  jobnetmsg       jnm;
                jlen = job_pack(&jnm, newjob);
                jnm.hdr.hdr.code = htons(code);
                jnm.hdr.hdr.hostid = myhostid;
                jnm.hdr.hdr.pid = htonl(sr->upid);
                strncpy(jnm.hdr.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
                strncpy(jnm.hdr.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
                if  (!chk_write(rp, (char *) &jnm, jlen))
                        return  N_HOSTDIED;
        }

        /* Dispose of reply */

        return  ureply(rp, sr);
}

/* Other end of above routine header version */

void  job_recvhupdate(struct remote *rp, msghdr *hdr)
{
        ULONG           indx;
        long            mymtype;
        BtjobRef        Jreq;
        struct  jobhnetmsg      jhnm;
        Shipc           omsg;
        Repmess         rr;

        jhnm.hdr = *hdr;
        READ_SOCK(rp, jhnm, msghdr);

        if  (forksafe() != 0)
                return;

#ifndef WORDS_BIGENDIAN
        jhnm.hdr.code = ntohs(hdr->code);
        jhnm.hdr.length = ntohs(hdr->length);
        jhnm.hdr.pid = ntohl(hdr->pid);
#endif

        /* Generate scheduler message out of this lot */

        Jreq = &Xbuffer->Ring[indx = getxbuf()];
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jhnm.hdr.code;
        mymtype = omsg.sh_params.upid = getpid();
        if  (rp->ht_flags & HT_ROAMUSER)  {
                omsg.sh_params.uuid = rp->n_uid;
                omsg.sh_params.ugid = rp->n_gid;
        }
        else  {
                omsg.sh_params.uuid = lookup_uname(jhnm.hdr.muser);
                omsg.sh_params.ugid = lookup_gname(jhnm.hdr.mgroup);
        }
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_un.sh_jobindex = indx;
        jobh_unpack(&Jreq->h, &jhnm);
#ifdef  USING_MMAP
        msync((char *) Xbuffer, sizeof(struct Transportbuf), MS_ASYNC|MS_INVALIDATE);
#endif
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(ULONG), 0);

        /* Get reply and make into a network reply */

        msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype + MTOFFSET, 0);
        freexbuf(indx);
        mreply(rp, jhnm.hdr.pid, rr.outmsg.mcode);
        exit(0);
}

/* Other end of send update routine w.s.m. version */

void  job_recvupdate(struct remote *rp, msghdr *hdr)
{
        ULONG           indx;
        long            mymtype;
        BtjobRef        Jreq;
        struct  jobnetmsg       jnm;
        Shipc           omsg;
        Repmess         rep;

        jnm.hdr.hdr = *hdr;
#ifndef WORDS_BIGENDIAN
        jnm.hdr.hdr.code = ntohs(hdr->code);
        jnm.hdr.hdr.length = ntohs(hdr->length);
        jnm.hdr.hdr.pid = ntohl(hdr->pid);
#endif

        read_sock(rp, sizeof(msghdr) + (char *) &jnm, jnm.hdr.hdr.length - sizeof(msghdr));

        if  (forksafe() != 0)
                return;

        /* Generate scheduler message out of this lot */

        Jreq = &Xbuffer->Ring[indx = getxbuf()];
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jnm.hdr.hdr.code;
        mymtype = omsg.sh_params.upid = getpid();
        if  (rp->ht_flags & HT_ROAMUSER)  {
                omsg.sh_params.uuid = rp->n_uid;
                omsg.sh_params.ugid = rp->n_gid;
        }
        else  {
                omsg.sh_params.uuid = lookup_uname(jnm.hdr.hdr.muser);
                omsg.sh_params.ugid = lookup_gname(jnm.hdr.hdr.mgroup);
        }
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_un.sh_jobindex = indx;
        job_unpack(Jreq, &jnm);
#ifdef  USING_MMAP
        msync((char *) Xbuffer, sizeof(struct Transportbuf), MS_ASYNC|MS_INVALIDATE);
#endif
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(ULONG), 0);
        msgrcv(Ctrl_chan, (struct msgbuf *) &rep, sizeof(Shreq), mymtype + MTOFFSET, 0);
        freexbuf(indx);
        mreply(rp, jnm.hdr.hdr.pid, rep.outmsg.mcode);
        exit(0);
}

/* Send update of modes only.
   Other end catches and processes reply using job_recvhupdate above */

unsigned  job_sendmdupdate(BtjobRef oldjob, BtjobRef newjob, ShreqRef sr)
{
        struct  remote  *rp;
        struct  jobhnetmsg      jhm;

        if  (!(rp = inl_find_connected(oldjob->h.bj_hostid)))
                return  N_HOSTOFFLINE;

        /* There's a certain amount of duplication, especially of
           prin_uname etc here but this doesn't happen often
           enough to make a special meal of it.  */

        jobh_pack(&jhm, &oldjob->h);
        mode_pack(&jhm.nm_mode, &newjob->h.bj_mode);
        jhm.hdr.code = htons((USHORT) sr->mcode);
        jhm.hdr.hostid = myhostid;
        jhm.hdr.pid = htonl(sr->upid);
        strncpy(jhm.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
        strncpy(jhm.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
        if  (!chk_write(rp, (char *) &jhm, sizeof(jhm)))
                return  N_HOSTDIED;
        return  ureply(rp, sr);
}

/* Send update of user or group only */

unsigned  job_sendugupdate(BtjobRef oldjob, ShreqRef sr)
{
        struct  remote  *rp;
        struct  jugmsg  jm;

        if  (!(rp = inl_find_connected(oldjob->h.bj_hostid)))
                return  N_HOSTOFFLINE;

        jm.hdr.code = htons((USHORT) sr->mcode);
        jm.hdr.length = htons(sizeof(jm));
        jm.hdr.hostid = myhostid;
        jm.hdr.pid = htonl(sr->upid);
        strncpy(jm.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
        strncpy(jm.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
        jid_pack(&jm.jid, &oldjob->h);
        strncpy(jm.newug, sr->mcode == J_CHOWN? prin_uname((uid_t) sr->param):
                                                       prin_gname((gid_t) sr->param), UIDSIZE);
        jm.newug[UIDSIZE] = '\0';
        if  (!chk_write(rp, (char *) &jm, sizeof(jm)))
                return  N_HOSTDIED;
        return  ureply(rp, sr);
}

/* Process that at the remote end */

void  job_recvugupdate(struct remote *rp, msghdr *hdr)
{
        long            mymtype;
        struct  jugmsg  jugm;
        Shipc           omsg;
        Repmess         rep;

        jugm.hdr = *hdr;
        READ_SOCK(rp, jugm, msghdr);

        if  (forksafe() != 0)
                return;

#ifndef WORDS_BIGENDIAN
        jugm.hdr.code = ntohs(hdr->code);
        jugm.hdr.length = ntohs(hdr->length);
        jugm.hdr.pid = ntohl(hdr->pid);
#endif

        /* Generate scheduler message out of this lot */

        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jugm.hdr.code;
        mymtype = omsg.sh_params.upid = getpid();
        if  (rp->ht_flags & HT_ROAMUSER)  {     /* Check this FIXME */
                omsg.sh_params.uuid = rp->n_uid;
                omsg.sh_params.ugid = rp->n_gid;
        }
        else  {
                omsg.sh_params.uuid = lookup_uname(jugm.hdr.muser);
                omsg.sh_params.ugid = lookup_gname(jugm.hdr.mgroup);
        }
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_params.param = jugm.hdr.code == J_CHOWN? lookup_uname(jugm.newug) : lookup_gname(jugm.newug);
        omsg.sh_un.jobref.hostid = ext2int_netid_t(jugm.jid.hostid);
        omsg.sh_un.jobref.slotno = ntohl(jugm.jid.slotno);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(jident), 0);
        msgrcv(Ctrl_chan, (struct msgbuf *) &rep, sizeof(Shreq), mymtype + MTOFFSET, 0);
        mreply(rp, jugm.hdr.pid, rep.outmsg.mcode);
        exit(0);
}

/* Transmit a message about a job to a specific machine.  This is
   delete/force to the originating machine, abort to a machine
   running a job.  */

unsigned  job_message(const netid_t hostid, CBtjobhRef jp, CShreqRef sr)
{
        struct  remote          *rp;
        struct  jobcmsg         jcm;

        if  (!(rp = find_connected(hostid)))
                return  N_HOSTOFFLINE;
        jcm.hdr.code = htons((USHORT) sr->mcode);
        jcm.hdr.length = htons(sizeof(jcm));
        jcm.hdr.hostid = myhostid;
        jcm.hdr.pid = htonl(sr->upid);
        strncpy(jcm.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
        strncpy(jcm.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
        jid_pack(&jcm.jid, jp);
        jcm.param = htonl(sr->param);
        chk_write(rp, (char *) &jcm, sizeof(jcm));
        return  ureply(rp, sr);
}

/* Internal version of above - proposals and charges */

void  job_imessage(const netid_t hostid, CBtjobhRef jp, const unsigned mcode, const LONG param)
{
        struct  remote          *rp;
        struct  jobcmsg         jcm;

        if  (!(rp = find_connected(hostid)))
                return;
        BLOCK_ZERO(&jcm, sizeof(jcm));
        jcm.hdr.code = htons((USHORT) mcode);
        jcm.hdr.length = htons(sizeof(jcm));
        jcm.hdr.hostid = myhostid;
        jid_pack(&jcm.jid, jp);
        jcm.param = htonl(param);
        chk_write(rp, (char *) &jcm, sizeof(jcm));
}

/* Receive above two routines' efforts at the other end.  */

void  job_recvmessage(struct remote *rp, msghdr *hdr)
{
        long            mymtype;
        struct  jobcmsg jcm;
        Shipc           omsg;

        jcm.hdr = *hdr;
        READ_SOCK(rp, jcm, msghdr);

#ifndef WORDS_BIGENDIAN
        jcm.hdr.code = ntohs(hdr->code);
        jcm.hdr.length = ntohs(hdr->length);
        jcm.hdr.pid = ntohl(hdr->pid);
#endif

        /* If not expecting a reply, as with propose, just launch message */

        if  (jcm.hdr.pid != 0)  {
                if  (forksafe() != 0)
                        return;
                mymtype = omsg.sh_params.upid = getpid();
                omsg.sh_params.uuid = lookup_uname(jcm.hdr.muser);
                omsg.sh_params.ugid = lookup_gname(jcm.hdr.mgroup);
        }
        else  {
                mymtype = 0;
                omsg.sh_params.uuid = 0;
                omsg.sh_params.ugid = 0;
        }
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jcm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_params.param = ntohl(jcm.param);
        omsg.sh_un.jobref.hostid = ext2int_netid_t(jcm.jid.hostid);
        omsg.sh_un.jobref.slotno = ntohl(jcm.jid.slotno);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(jident), 0);
        if  (mymtype)  {
                Repmess rr;
                msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype + MTOFFSET, 0);
                mreply(rp, jcm.hdr.pid, rr.outmsg.mcode);
                exit(0);
        }
}

void  job_sendnote(CBtjobRef jp, const int mcode, const jobno_t sout, const jobno_t serr)
{
        struct  remote          *rp;
        struct  jobnotmsg       jnm;

        if  (!(rp = find_connected(jp->h.bj_hostid)))
                return;
        BLOCK_ZERO(&jnm, sizeof(jnm));
        jnm.hdr.code = htons((USHORT) J_RNOTIFY);
        jnm.hdr.length = htons(sizeof(jnm));
        jnm.hdr.hostid = myhostid;
        jid_pack(&jnm.jid, &jp->h);
        jnm.msgcode = htons((SHORT) mcode);
        jnm.sout = htonl(sout);
        jnm.serr = htonl(serr);
        chk_write(rp, (char *) &jnm, sizeof(jnm));
}

/* Receive above routine's efforts at the other end.  */

void  job_recvnote(struct remote *rp, msghdr *hdr)
{
        struct  jobnotmsg       jnm;
        Shipc           omsg;

        jnm.hdr = *hdr;
        READ_SOCK(rp, jnm, msghdr);

#ifndef WORDS_BIGENDIAN
        jnm.hdr.code = ntohs(hdr->code);
        jnm.hdr.length = ntohs(hdr->length);
#endif

        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = jnm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_un.remnote.jid.hostid = ext2int_netid_t(jnm.jid.hostid);
        omsg.sh_un.remnote.jid.slotno = ntohl(jnm.jid.slotno);
        omsg.sh_un.remnote.msg = ntohs(jnm.msgcode);
        omsg.sh_un.remnote.sout = ntohl(jnm.sout);
        omsg.sh_un.remnote.serr = ntohl(jnm.serr);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(struct jremnotemsg), 0);
}

void  sync_single(const netid_t hostid, const slotno_t slot)
{
        struct  remote  *rp;
        if  ((rp = find_connected(hostid)))  {
                struct  netmsg  rq;
                BLOCK_ZERO(&rq, sizeof(rq));
                rq.hdr.code = htons(N_SYNCSINGLE);
                rq.hdr.length = htons(sizeof(struct netmsg));
                rq.hdr.hostid = myhostid;
                rq.arg = htonl(slot);
                chk_write(rp, (char *) &rq, sizeof(rq));
        }
}

void  send_single_jobhdr(const netid_t hostid, const slotno_t slot)
{
        struct  remote  *rp;
        struct  jobhnetmsg      jhm;
        BtjobhRef       jp;

        if  (slot < 0  ||  slot >= Job_seg.dptr->js_maxjobs)
                return;
        jp = &Job_seg.jlist[slot].j.h;
        if  (jp->bj_job == 0)
                return;
        if  (!(rp = find_connected(hostid)))
                return;
        jobh_pack(&jhm, jp);
        jhm.hdr.code = htons(J_HCHANGED);
        jhm.hdr.hostid = myhostid;
        jhm.hdr.pid = htonl(getpid());
        strncpy(jhm.hdr.muser, jp->bj_mode.o_user, UIDSIZE);
        strncpy(jhm.hdr.mgroup, jp->bj_mode.o_group, UIDSIZE);
        chk_write(rp, (char *) &jhm, sizeof(jhm));
}

/* Broadcast case for variables */

void  var_broadcast(BtvarRef vp, const unsigned code)
{
        struct  varnetmsg       vm;
        var_pack(&vm, vp);
        vm.hdr.code = htons(code);
        vm.hdr.hostid = myhostid;
        broadcast((char *) &vm, sizeof(struct varnetmsg));
}

/* Unscramble that at t'other end */

void  var_recvbcast(struct remote *rp, msghdr *hdr)
{
        struct  varnetmsg       vnm;
        Shipc           omsg;

        vnm.hdr = *hdr;
        READ_SOCK(rp, vnm, msghdr);
#ifndef WORDS_BIGENDIAN
        vnm.hdr.code = ntohs(hdr->code);
        vnm.hdr.length = ntohs(hdr->length);
        vnm.hdr.pid = ntohl(hdr->pid);
#endif
        BLOCK_ZERO(&omsg, sizeof(omsg));
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = vnm.hdr.code;
        omsg.sh_params.hostid = rp->hostid;
        var_unpack(&omsg.sh_un.sh_var, &vnm);
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(Btvar), 0);
}

/* Send update request about a variable to the owning machine.
   Applies to assignments or comment change */

unsigned  var_sendupdate(BtvarRef oldvar, BtvarRef newvar, ShreqRef sr)
{
        struct  remote  *rp;
        struct  varnetmsg       vm;

        var_pack(&vm, oldvar);
        vm.hdr.code = htons((USHORT) sr->mcode);
        vm.hdr.hostid = myhostid;

        /* Bits of duplication here.....  */

        switch  (sr->mcode)  {
        case  V_ASSIGN:
                if  ((vm.nm_consttype = (unsigned char) newvar->var_value.const_type) == CON_STRING)
                        strncpy(vm.nm_un.nm_string, newvar->var_value.con_un.con_string, BTC_VALUE);
                else
                        vm.nm_un.nm_long = htonl(newvar->var_value.con_un.con_long);
                break;
        case  V_CHCOMM:
                strncpy(vm.nm_comment, newvar->var_comment, BTV_COMMENT);
                break;
        case  V_CHMOD:
                mode_pack(&vm.nm_mode, &newvar->var_mode);
                break;
        }

        if  (!(rp = find_connected(oldvar->var_id.hostid)))
                return  N_HOSTOFFLINE;

        vm.hdr.pid = htonl(sr->upid);
        strncpy(vm.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
        strncpy(vm.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
        if  (!chk_write(rp, (char *) &vm, sizeof(vm)))
                return  N_HOSTDIED;
        return  ureply(rp, sr);
}

/* Unscramble that at t'other end */

void  var_recvupdate(struct remote *rp, msghdr *hdr)
{
        long            mymtype;
        struct  varnetmsg       vnm;
        Shipc           omsg;
        Repmess         rr;

        vnm.hdr = *hdr;
        READ_SOCK(rp, vnm, msghdr);
        if  (forksafe() != 0)
                return;
#ifndef WORDS_BIGENDIAN
        vnm.hdr.code = ntohs(hdr->code);
        vnm.hdr.length = ntohs(hdr->length);
        vnm.hdr.pid = ntohl(hdr->pid);
#endif
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = vnm.hdr.code;
        mymtype = omsg.sh_params.upid = getpid();
        if  (rp->ht_flags & HT_ROAMUSER)  {             /* FIXME check this */
                omsg.sh_params.uuid = rp->n_uid;
                omsg.sh_params.ugid = rp->n_gid;
        }
        else  {
                omsg.sh_params.uuid = lookup_uname(vnm.hdr.muser);
                omsg.sh_params.ugid = lookup_gname(vnm.hdr.mgroup);
        }
        omsg.sh_params.hostid = rp->hostid;
        var_unpack(&omsg.sh_un.sh_var, &vnm);
        if  (rp->hostid == 0)                           /* Stop messages about sequence error from clients on same machine */
                omsg.sh_un.sh_var.var_sequence = 0x7fffffff;
        msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(Btvar), 0);
        msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype + MTOFFSET, 0);
        mreply(rp, vnm.hdr.pid, rr.outmsg.mcode);
        exit(0);
}

/* Send update of user or group only */

unsigned  var_sendugupdate(BtvarRef oldvar, ShreqRef sr)
{
        struct  remote  *rp;
        struct  vugmsg  vm;

        if  (!(rp = find_connected(oldvar->var_id.hostid)))
                return  N_HOSTOFFLINE;

        vm.hdr.code = htons((USHORT) sr->mcode);
        vm.hdr.length = htons(sizeof(vm));
        vm.hdr.hostid = myhostid;
        vm.hdr.pid = htonl(sr->upid);
        strncpy(vm.hdr.muser, prin_uname(sr->uuid), UIDSIZE);
        strncpy(vm.hdr.mgroup, prin_gname(sr->ugid), UIDSIZE);
        vid_pack(&vm.vid, oldvar);
        strncpy(vm.newug, sr->mcode == V_CHOWN? prin_uname((uid_t) sr->param):
                                                       prin_gname((gid_t) sr->param), UIDSIZE);
        vm.newug[UIDSIZE] = '\0';
        if  (!chk_write(rp, (char *) &vm, sizeof(vm)))
                return  N_HOSTDIED;
        return  ureply(rp, sr);
}

/* Unscramble that at t'other end */

void  var_recvugupdate(struct remote *rp, msghdr *hdr)
{
        vhash_t         wvar;
        long            mymtype;
        struct  vugmsg  vnm;
        Shipc           omsg;
        Repmess         rr;

        vnm.hdr = *hdr;
        READ_SOCK(rp, vnm, msghdr);
        if  (forksafe() != 0)
                return;
#ifndef WORDS_BIGENDIAN
        vnm.hdr.code = ntohs(hdr->code);
        vnm.hdr.length = ntohs(hdr->length);
        vnm.hdr.pid = ntohl(hdr->pid);
#endif
        omsg.sh_mtype = TO_SCHED;
        omsg.sh_params.mcode = vnm.hdr.code;
        mymtype = omsg.sh_params.upid = getpid();
        if  (rp->ht_flags & HT_ROAMUSER)  {             /* FIXME check this */
                omsg.sh_params.uuid = rp->n_uid;
                omsg.sh_params.ugid = rp->n_gid;
        }
        else  {
                omsg.sh_params.uuid = lookup_uname(vnm.hdr.muser);
                omsg.sh_params.ugid = lookup_gname(vnm.hdr.mgroup);
        }
        omsg.sh_params.hostid = rp->hostid;
        omsg.sh_params.param = vnm.hdr.code == V_CHOWN? lookup_uname(vnm.newug): lookup_gname(vnm.newug);
        if  ((wvar = vid_uplook(&vnm.vid)) < 0)
                mreply(rp, vnm.hdr.pid, V_NEXISTS);
        else  {
                omsg.sh_un.sh_var = Var_seg.vlist[wvar].Vent;
                msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq) + sizeof(Btvar), 0);
                msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype + MTOFFSET, 0);
                mreply(rp, vnm.hdr.pid, rr.outmsg.mcode);
        }
        exit(0);
}

/* Look around for remote machines we haven't got details of jobs or variables of.  */

void  netsync()
{
        int     rpcnt;

        for  (rpcnt = connected.rl_nums - 1;  rpcnt >= 0;  rpcnt--)  {
                struct  remote  *rp = connected.list[rpcnt];
                if  (rp->is_sync == NSYNC_NONE)  {
                        struct  netmsg  rq;
                        BLOCK_ZERO(&rq, sizeof(rq));
                        rq.hdr.code = htons(N_REQSYNC);
                        rq.hdr.length = htons(sizeof(rq));
                        rq.hdr.hostid = myhostid;
                        chk_write(rp, (char *) &rq, sizeof(rq));
                        rp->is_sync = NSYNC_REQ;
                        /* Return so we don't get indigestion with
                           everyone talking at once.  */
                        return;
                }
        }
}

/* Find the person we are talking to. We distinguish multiple connections
   on the same host by looking at the socket fd. */

struct  remote  *find_sync(const netid_t netid, const int sockfd)
{
        struct  remote  *rp;
        for  (rp = inl_find_connected(netid);  rp;  rp = next_connected(rp))
                if  (rp->sockfd == sockfd)
                        return  rp;
        return  (struct remote *) 0;
}

/* Send sync to given host.
   We use this in three basic places,
   1. If we do a "manual" connect to another server
   2. If we get a reply to a "probe" from another server
   3. If another server or client which connected to us wants to sync or resync.
   Return 1 if it's OK else 0 */

int  sendsync(struct remote *rp)
{
        int             vn;
        unsigned        jind, jlen;
        unsigned  lumpcount = 0;
        struct  jobnetmsg  jm;
        struct  varnetmsg  vm;

        for  (vn = 0;  vn < VAR_HASHMOD;  vn++)  {
                vhash_t hp;
                for  (hp = Var_seg.vhash[vn];  hp >= 0;  hp = Var_seg.vlist[hp].Vnext)  {
                        BtvarRef  vp = &Var_seg.vlist[hp].Vent;
                        if  (vp->var_id.hostid == 0  &&  vp->var_flags & VF_EXPORT)  {
                                var_pack(&vm, vp);
                                /* Have to put this in each time as var_pack zeroes whole message */
                                vm.hdr.code = htons(V_CREATE);
                                vm.hdr.hostid = myhostid;
                                if  (!chk_write(rp, (char *) &vm, sizeof(vm)))
                                        return  0;
                                if  ((++lumpcount % lumpsize) == 0)
                                        sleep(lumpwait);
                        }
                }
        }

        jind = Job_seg.dptr->js_q_head;
        while  (jind != JOBHASHEND)  {
                BtjobRef  jp = &Job_seg.jlist[jind].j;
                if  (jp->h.bj_hostid == 0  &&  jp->h.bj_jflags & BJ_EXPORT)  {
                        jlen = job_pack(&jm, jp);
                        /* Have to put this in each time as job_pack zeroes whole message */
                        jm.hdr.hdr.code = htons(J_CREATE);
                        jm.hdr.hdr.hostid = myhostid;
                        if  (!chk_write(rp, (char *) &jm, jlen))
                                return  0;
                        if  ((++lumpcount % lumpsize) == 0)
                                sleep(lumpwait);
                }
                jind = Job_seg.jlist[jind].q_nxt;
        }

        return  1;
}

/* Send end of sync message */

void  send_endsync(struct remote *rp)
{
        struct  netmsg  rq;
        BLOCK_ZERO(&rq, sizeof(rq));
        rq.hdr.code = htons(N_ENDSYNC);
        rq.hdr.length = htons(sizeof(struct netmsg));
        rq.hdr.hostid = myhostid;
        chk_write(rp, (char *) &rq, sizeof(rq));
}

/* Note we had a successful end sync from the other end */

void  endsync(const netid_t netid)
{
        struct  remote  *rp;

        if  ((rp = find_connected(netid))  &&  rp->is_sync != NSYNC_OK)  {
                rp->is_sync = NSYNC_OK;
                Netsync_req--;
        }
        if  (Netsync_req > 0)
                netsync();
}

/* Network request reception */

void  net_recv(struct remote *rp, msghdr *hdr)
{
        struct  netmsg  imsg;
        Shipc   omsg;

        imsg.hdr = *hdr;
        READ_SOCK(rp, imsg, msghdr);

#ifndef WORDS_BIGENDIAN
        imsg.hdr.code = ntohs(hdr->code);
        imsg.hdr.length = ntohs(hdr->length);
        imsg.hdr.pid = ntohl(hdr->pid);
        imsg.arg = ntohl(imsg.arg);
#endif

        /* All these messages are internally generated so we don't send any reply back. */

        BLOCK_ZERO(&omsg, sizeof(omsg));
        switch  (imsg.hdr.code)  {
        case  N_TICKLE:
                return;
        case  N_SHUTHOST:
        case  N_REQSYNC:
        case  N_ENDSYNC:
                omsg.sh_mtype = TO_SCHED;
                omsg.sh_params.mcode = imsg.hdr.code;
                omsg.sh_params.hostid = rp->hostid;
                omsg.sh_params.param = rp->sockfd;
                msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq), 0);
                if  (imsg.hdr.code == N_SHUTHOST)
                        exit(0);
                return;
        case  N_REQREPLY:
        case  N_SYNCSINGLE:
                omsg.sh_mtype = TO_SCHED;
                omsg.sh_params.mcode = imsg.hdr.code;
                omsg.sh_params.hostid = rp->hostid;
                omsg.sh_params.upid = imsg.hdr.pid;
                omsg.sh_params.param = imsg.arg;
                msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq), 0);
                return;
        case  N_SETNOTSERVER:
        case  N_SETISSERVER:
                omsg.sh_mtype = TO_SCHED;
                omsg.sh_params.mcode = imsg.hdr.code;
                omsg.sh_params.hostid = rp->hostid;
                omsg.sh_params.upid = imsg.hdr.pid;
                omsg.sh_params.param = rp->sockfd;
                msgsnd(Ctrl_chan, (struct msgbuf *) &omsg, sizeof(Shreq), 0);
                return;
        }
}

void  remote_recv(struct remote *rp)
{
        msghdr  hdr;

        read_sock(rp, (char *) &hdr, sizeof(hdr));

        switch  (ntohs(hdr.code))  {
        case  N_TICKLE:
        case  N_SHUTHOST:
        case  N_REQSYNC:
        case  N_ENDSYNC:
        case  N_REQREPLY:
        case  N_SYNCSINGLE:
        case  N_SETNOTSERVER:
        case  N_SETISSERVER:
                net_recv(rp, &hdr);
                return;

        case  N_WANTLOCK:
        case  N_UNLOCK:
                net_recvlockreq(rp, &hdr);
                return;

        case  N_RJASSIGN:
                recv_remasses(rp, &hdr);
                return;

        case  N_REMLOCK_OK:
        case  N_REMLOCK_PRIO:
                {
                        struct  netmsg  nm;
                        READ_SOCK(rp, nm, msghdr);
                        net_replylock();
                }
                return;

        case  J_CREATE:
        case  J_BCHANGED:
                job_recvbcast(rp, &hdr);
                return;

        case  J_CHOWN:
        case  J_CHGRP:
                job_recvugupdate(rp, &hdr);
                return;

        case  N_DOCHARGE:
        case  J_DELETE:
        case  J_KILL:
        case  J_FORCE:
        case  J_FORCENA:
                job_recvmessage(rp, &hdr);
                return;

        case  J_CHANGED:
                job_recvupdate(rp, &hdr);
                return;

        case  J_HCHANGED:
        case  J_CHMOD:
                job_recvhupdate(rp, &hdr);
                return;

        case  J_BHCHANGED:
        case  J_CHMOGED:
                job_hrecvbcast(rp, &hdr);
                return;

        case  J_BOQ:
        case  J_DELETED:
        case  J_BFORCED:
        case  J_BFORCEDNA:
        case  J_PROPOSE:
        case  J_PROPOK:
                job_imessrecvbcast(rp, &hdr);
                return;

        case  J_RRCHANGE:
                job_rrstatrecv(rp, &hdr);
                return;

        case  J_CHSTATE:
                job_statrecvbcast(rp, &hdr);
                return;

        case  J_RNOTIFY:
                job_recvnote(rp, &hdr);
                return;

        case  V_CREATE:         /* This is a broadcast if it gets here */
        case  V_ASSIGNED:       /* Broadcast of result of assign */
        case  V_CHMOGED:        /* Broadcast of result of chmod/chown/chgrp */
        case  V_DELETED:        /* Broadcast of result of delete */
        case  V_RENAMED:        /* Broadcast of result of rename */
        case  V_CHFLAGS:        /* Broadcast of result of change flags (cluster) */
                var_recvbcast(rp, &hdr);
                return;

        case  V_ASSIGN:         /* Request to assign */
        case  V_CHMOD:          /* Request to chmod */
        case  V_CHCOMM:         /* Request to change comment */
                var_recvupdate(rp, &hdr);
                return;

        case  V_CHOWN:          /* Passed on request to chown/chgrp */
        case  V_CHGRP:
                var_recvugupdate(rp, &hdr);
                return;
        }
}

static  int     count_catch_shut = 0;

void  exec_catchshut()
{
        Shipc   reply;
        BLOCK_ZERO(&reply, sizeof(reply));
        reply.sh_mtype = TO_SCHED;
        reply.sh_params.mcode = N_ABORTHOST;
        msgsnd(Ctrl_chan, (struct msgbuf *) &reply, sizeof(Shreq), 0);
        exit(0);
}

/* Catch termination signals sent when we think something has gone down.
   We now avoid SEM_UNDOs so we set a flag and catch it in the main loop. */

RETSIGTYPE  catchshut(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(NETSHUTSIG, SIG_IGN);
#endif
        count_catch_shut++;
}

static RETSIGTYPE  stop_mon(int signum)
{
        Shipc   oreq;

#ifdef  UNSAFE_SIGNALS
        signal(signum, SIG_IGN);
#endif

        BLOCK_ZERO(&oreq, sizeof(oreq));
        oreq.sh_mtype = TO_SCHED;
        oreq.sh_params.mcode = O_STOP;
        oreq.sh_params.uuid = getuid();
        oreq.sh_params.ugid = getgid();
        oreq.sh_params.upid = getpid();
        oreq.sh_params.param = signum == SIGTERM? $E{Scheduler killed}: $E{Scheduler net core dump};
        msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(Shreq), 0);
        exit(0);
}

/* This monitors incoming data from various remote hosts by forking
   off a process and listening on the various hosts.  If it finds
   a suitable candidate, the main scheduler path gets sent a
   message on the message channel. In the case where a new
   machine arrives we exit because our forked-off process has
   garbage hosts in it, so we can be born again another day.
   Hopefully this won't happen too often.  */

void  netmonitor()
{
        int     nret, rpcnt, highfd;
        fd_set  setupset, ready;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
#endif

        if  ((Netm_pid = fork()) != 0)  {
#ifdef  BUGGY_SIGCLD
                PIDTYPE pid2;
#endif
                if  (Netm_pid < 0)
                        panic($E{Cannot fork});
#ifdef  BUGGY_SIGCLD
                /* Contortions to avoid leaving zombie processes */

                if  ((pid2 = fork()) != 0)  {
                        if  (pid2 < 0)  {
                                kill(Netm_pid, SIGKILL);
                                panic($E{Cannot fork});
                        }
                        exit(0);
                }

                /* The main btsched process is now a sibling of the
                   net monitor process...  */

                nchild = 0;
#endif
                return;
        }

#ifdef  STRUCT_SIG
        z.sighandler_el = stop_mon;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#ifndef DEBUG
        sigact_routine(SIGFPE, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGBUS, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGSEGV, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGILL, &z, (struct sigstruct_name *) 0);
#ifdef  SIGSYS
        sigact_routine(SIGSYS, &z, (struct sigstruct_name *) 0);
#endif /* SIGSYS */
#endif /* !DEBUG */
#else  /* !STRUCT_SIG */
        signal(SIGTERM, stop_mon);
#ifndef DEBUG
        signal(SIGFPE, stop_mon);
        signal(SIGBUS, stop_mon);
        signal(SIGSEGV, stop_mon);
        signal(SIGILL, stop_mon);
#ifdef  SIGSYS
        signal(SIGSYS, stop_mon);
#endif /* SIGSYS */
#endif /* !DEBUG */
#endif /* !STRUCT_SIG */

        highfd = listsock;
        if  (viewsock > highfd)
                highfd = viewsock;
        if  (probesock > highfd)
                highfd = probesock;
        FD_ZERO(&setupset);
        FD_SET(listsock, &setupset);
        FD_SET(viewsock, &setupset);
        FD_SET(probesock, &setupset);

        for  (rpcnt = 0;  rpcnt < connected.rl_nums;  rpcnt++)  {
                int     sock = connected.list[rpcnt]->sockfd;
                if  (sock > 0)  {
                        if  (sock > highfd)
                                highfd = sock;
                        FD_SET(sock, &setupset);
                }
        }

        /* Set signal to note shutdown messages */

#ifdef  STRUCT_SIG
        z.sighandler_el = catchshut;
        sigact_routine(NETSHUTSIG, &z, (struct sigstruct_name *) 0);
#else
        signal(NETSHUTSIG, catchshut);
#endif

        /* We are now in the (possibly painfully) created net monitor
           process.
           We slurp up messages and put on message queue. */

        for  (;;)  {

                if  (count_catch_shut > 0)
                        exec_catchshut();

                ready = setupset;

                if  ((nret = select(highfd+1, &ready, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0)) < 0)  {
                        if  (errno == EINTR)
                                continue;
                        panic($E{Panic poll/select error in netmonitor process});
                }

                if  (FD_ISSET(viewsock, &ready))  {
                        feed_req();
                        if  (--nret <= 0)
                                continue;
                }

                for  (rpcnt = 0;  rpcnt < connected.rl_nums;  rpcnt++)  {
                        struct  remote  *rp = connected.list[rpcnt];
                        if  (FD_ISSET(rp->sockfd, &ready))  {
                                remote_recv(rp);
                                --nret;
                        }
                }

                if  (nret <= 0)
                        continue;

                if  (FD_ISSET(probesock, &ready))  {
                        reply_probe();
                        if  (--nret <= 0)
                                continue;
                }
                if  (FD_ISSET(listsock, &ready))  {
                        Shipc   reply;
                        BLOCK_ZERO(&reply, sizeof(reply));
                        reply.sh_mtype = TO_SCHED;
                        reply.sh_params.mcode = N_NEWHOST;
                        msgsnd(Ctrl_chan, (struct msgbuf *) &reply, sizeof(Shreq), 0);
                        exit(0);
                }
        }
}
