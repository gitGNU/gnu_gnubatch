/* xbnetserv.c -- main module for xbnetserv

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
#include "services.h"

static  char    Filename[] = __FILE__;

SHORT   qsock,                  /* TCP Socket for accepting queued jobs on */
        uasock,                 /* Datagram socket for user access enquiries */
        apirsock;               /* API Request socket */

SHORT   qportnum,               /* Port number for TCP */
        uaportnum,              /* Port number for UDP */
        apirport,               /* Port number for API requests */
        apipport;               /* UDP port number for prompt messages to API */

const   char    Sname[] = GBNETSERV_PORT,
                ASrname[] = DEFAULT_SERVICE,
                ASmname[] = MON_SERVICE;

int     hadrfresh;

int     Ctrl_chan = -1;
unsigned timeouts = NETTICKLE;

/* We don't use these fields as the API strips out users and groups
   itself, but we now incorporate the screening in the library
   routine because we want to avoid having so many rjobfiles
   everywhere.  */

static  char    *spdir;

static  char    tmpfl[NAMESIZE + 1];

int_ugid_t      Defaultuid, Defaultgid;
char            *Defaultuser,
                *Defaultgroup;

USHORT  err_which;              /* Which we are complaining about */
USHORT  orig_umask;             /* Saved copy of original umask */

unsigned        myhostl;        /* Length of ... */
char            *myhostname;    /* We send our variables prefixed by this */

BtjobRef        JREQ;

struct  hhash   *nhashtab[NETHASHMOD];
struct  winuhash *winuhashtab[NETHASHMOD];
struct  alhash  *alhashtab[NETHASHMOD];

extern  char    dosuser[];

struct  pend_job  pend_list[MAX_PEND_JOBS];/* List of pending UDP jobs */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

/* Hash function for windows names limited to WUIDSIZE chars */

unsigned  calc_winu_hash(const char *name)
{
        unsigned  sum = 0;
        int     cnt = WUIDSIZE;
        while  (*name  &&  cnt > 0)  {
                sum += tolower(*name);
                name++;
                cnt--;
        }
        return  sum % NETHASHMOD;
}

/* Look up windows user name in hash table */

struct  winuhash  *lookup_winu(const char *wuname)
{
        struct  winuhash  *wp;

        for  (wp = winuhashtab[calc_winu_hash(wuname)];  wp;  wp = wp->next)
                if  (ncstrncmp(wuname, wp->winname, WUIDSIZE) == 0)
                        return  wp;
        return  (struct winuhash *) 0;
}

/* Allocate a new windows user name structure and return it */

struct  winuhash  *add_winu(const char *wuname)
{
        unsigned  hashv = calc_winu_hash(wuname);
        struct  winuhash  *wp = (struct winuhash *) malloc(sizeof(struct winuhash));

        if  (!wp)
                ABORT_NOMEM;

        wp->next = winuhashtab[hashv];
        winuhashtab[hashv] = wp;
        return  wp;
}

/* Look up a windows name and if it isn't there take it as a UNIX name and try that.
   That's to save lots of fred=fred type entries in the user map file */

struct  winuhash *lookup_winoruu(const char *wuname)
{
        struct  winuhash *wp;
        int_ugid_t  uid, gid;

        if  ((wp = lookup_winu(wuname)))
                return  wp;

        uid = lookup_uname(wuname);
        if  (uid == UNKNOWN_UID)
                return  (struct winuhash *) 0;
        gid = lastgid;

        wp = add_winu(wuname);
        wp->uuid = uid;
        wp->ugid = gid;
        wp->unixname = stracpy(wuname);
        wp->winname = stracpy(wuname);
        wp->unixgroup = stracpy(prin_gname(gid));
        return  wp;
}

/* Parse the Windows user name file */
void    parse_winu_file()
{
        FILE    *fp = fopen(WINUSER_MAP, "r");
        char    lbuf[200];

        if  (!fp)
                return;

        while  (fgets(lbuf, sizeof(lbuf), fp))  {
                struct  winuhash  *wp;
                char  *sp = &lbuf[strlen(lbuf)-1], *cp;
                int_ugid_t  uid, gid;

                /* Zap trailing spaces at the end */

                while  (sp >= &lbuf[0] &&  isspace(*sp))
                        sp--;
                *++sp = '\0';   /* Step back to first space */

                /* Skip over leading spaces at beginning */

                sp = &lbuf[0];
                while  (isspace(*sp))
                        sp++;

                /* Forget lines starting with a # or which don't have a colon in */

                if  (*sp == '#')
                        continue;
                if  (!(cp = strchr(sp, ':')))
                        continue;

                /* Zap the colon, cp points to windows name, sp to UNIX name.
                   Forget ones with unknown UNIX name */

                *cp++ = '\0';
                uid = lookup_uname(sp);
                if  (uid == UNKNOWN_UID)
                        continue;
                gid = lastgid;

                /* If we saw Windows name before, just update details */

                if  (!(wp = lookup_winu(cp)))
                        wp = add_winu(cp);
                wp->winname = stracpy(cp);
                wp->unixname = stracpy(sp);
                wp->unixgroup = stracpy(prin_gname(gid));
                wp->uuid = uid;
                wp->ugid = gid;
        }

        fclose(fp);
}

/* Clear hash table for Windows users on SIGHUP */

void    clear_winuhash()
{
        unsigned  hashv;

        for  (hashv = 0;  hashv < NETHASHMOD;  hashv++)  {
                struct  winuhash  *wp = winuhashtab[hashv];
                while  (wp)  {
                        struct  winuhash  *nxt = wp->next;
                        free(wp->winname);
                        free(wp->unixname);
                        free(wp->unixgroup);
                        free((char *) wp);
                        wp = nxt;
                }
                winuhashtab[hashv] = (struct winuhash *) 0;
        }

        /* Forget auto login stuff whilst we are there */
        for  (hashv = 0;  hashv < NETHASHMOD;  hashv++)  {
                struct  alhash  *al = alhashtab[hashv];
                while  (al)  {
                        struct  alhash  *nxt = al->next;
                        free(al->unixname);
                        free(al->unixgroup);
                        free((char *) al);
                        al = nxt;
                }
                alhashtab[hashv] = (struct alhash *) 0;
        }
}

struct  alhash  *find_autoconn(const netid_t h)
{
        struct  alhash  *al;

        for  (al = alhashtab[calcnhash(h)];  al;  al = al->next)
                if  (al->hostid == h)
                        return  al;
        return  (struct alhash *)  0;
}

struct  alhash  *add_autoconn(const netid_t h, const char *uname, const int_ugid_t uid, const int_ugid_t gid)
{
        struct  alhash  *al = find_autoconn(h);

        if  (al)
                free(al->unixname);
        else  {
                unsigned  hashv = calcnhash(h);
                al = (struct alhash *) malloc(sizeof(struct alhash));
                if  (!al)
                        ABORT_NOMEM;
                al->next = alhashtab[hashv];
                alhashtab[hashv] = al;
                al->hostid = h;
        }
        al->unixname = stracpy(uname);
        al->uuid = uid;
        al->ugid = gid;
        al->unixgroup = stracpy(prin_gname(gid));
        return  al;
}

struct  hhash  *lookup_hhash(const netid_t h)
{
        struct  hhash  *hp;

        for  (hp = nhashtab[calcnhash(h)];  hp;  hp = hp->hn_next)
                if  (hp->hostid == h)
                        return  hp;
        return  (struct hhash *)  0;
}

struct  hhash  *add_hhash(const netid_t h)
{
        struct  hhash  *hp = (struct hhash *) malloc(sizeof(struct hhash));
        unsigned  hashv = calcnhash(h);

        if  (!hp)
                ABORT_NOMEM;
        hp->hn_next = nhashtab[hashv];
        nhashtab[hashv] = hp;
        hp->hostid = h;
        hp->isme = 0;
        hp->isclient = 0;
        return  hp;
}

void    forget_hosts()
{
        unsigned  hashv;

        for  (hashv = 0;  hashv < NETHASHMOD;  hashv++)  {
                struct  hhash  *hp, *nxt;
                hp = nhashtab[hashv];
                while  (hp)  {
                        nxt = hp->hn_next;
                        free((char *) hp);
                        hp = nxt;
                }
                nhashtab[hashv] = (struct hhash *) 0;
        }
}

/* Add IP address representing "me" to table for benefit of APIs on local host */

static void  addme(const netid_t mid)
{
        struct  hhash  *hp = add_hhash(mid);
        hp->isme = 1;
}

/* Read in hosts file and build up interesting stuff */

static void  process_hfile()
{
        struct  remote  *rp;
        struct  hhash   *hp;
        extern  char    hostf_errors;

        hostf_errors = 0;

        while  ((rp = get_hostfile()))  {

                /* If it is telling us about a possible client user, note the details as with the user map
                   file, however this overrides whatever was in there and possibly provides for a machine
                   at which a user doesn't need to supply a password */

                   if  (rp->ht_flags & HT_ROAMUSER)  {
                        struct  winuhash  *wp;
                        int_ugid_t  uid = lookup_uname(rp->hostname);
                        int_ugid_t  gid = lastgid;

                        if  (uid == UNKNOWN_UID)
                                continue;

                        if  (!(wp = lookup_winu(rp->alias)))
                                wp = add_winu(rp->alias);

                        wp->uuid = uid;
                        wp->ugid = gid;
                        wp->winname = stracpy(rp->alias);
                        wp->unixname = stracpy(rp->hostname);
                        wp->unixgroup = stracpy(prin_gname(gid));

                        /* Don't worry about usual machines if
                           we are checking anyhow or no machine given */

                        if  (!(rp->ht_flags & HT_PWCHECK)  &&  dosuser[0])  {
                                netid_t  defhost = look_hostname(dosuser);
                                if  (defhost)
                                        add_autoconn(defhost, rp->hostname, uid, gid);
                        }
                        continue;
                }

                /* It might be telling us about a client with a fixed IP address. */

                if  (rp->ht_flags & HT_DOS)  {
                        int_ugid_t  uid;
                        hp = lookup_hhash(rp->hostid);
                        if  (!hp)
                                hp = add_hhash(rp->hostid);
                        hp->isclient = 1;
                        if  (!(rp->ht_flags & HT_PWCHECK)  &&  dosuser[0]  &&  (uid = lookup_uname(dosuser)) != UNKNOWN_UID)
                                add_autoconn(rp->hostid, dosuser, uid, lastgid);
                        continue;
                }

                /* We now don't bother with "trusted" */

                hp = lookup_hhash(rp->hostid);
                if  (!hp)
                        hp = add_hhash(rp->hostid);
        }

        end_hostfile();

        /* Create entries for "me" to allow for API connections from local hosts */

        addme(myhostid);
        addme(htonl(INADDR_LOOPBACK));

        /* This may be a good place to warn people about errors in the host file.  */

        if  (hostf_errors)
                print_error($E{Warn errors in host file});
}

void    read_hfiles()
{
        struct  winuhash  *wp;
        int_ugid_t  uid;

        parse_winu_file();
        process_hfile();
        if  (!(wp = lookup_winu("default"))  || (uid = lookup_uname(wp->unixname)) == UNKNOWN_UID)  {
                Defaultuid = Daemuid;
                Defaultgid = Daemgid;
                Defaultuser = BATCHUNAME;
        }
        else  {
                Defaultuser = wp->unixname;
                Defaultuid = wp->uuid;
                Defaultgid = wp->ugid;
        }

        /* The following may be a movable feast if the group name for the batch
           user isn't defined but I don't think it matters */

        Defaultgroup = prin_gname(Defaultgid);
}

/* Catch hangup signals and re-read hosts file */

static RETSIGTYPE  catchhup(int n)
{
#ifdef  UNSAFE_SIGNALS
        signal(n, SIG_IGN);
#endif
        /* Forget what we learned before and re-read it */

        clear_winuhash();
        forget_hosts();
        un_rpwfile();
        /* NB Don't think we need a rgrpfile(); here - maybe supp groups sometime?? */
        rpwfile();
        read_hfiles();
#ifdef  UNSAFE_SIGNALS
        signal(n, catchhup);
#endif
}

static  char    sigstocatch[] = { SIGINT, SIGQUIT, SIGTERM };

/* On a signal, remove file (TCP connection) */

static RETSIGTYPE  catchdel(int n)
{
        unlink(tmpfl);
        exit(E_SIGNAL);
}

/* Main path - remove files pending for UDP */

RETSIGTYPE  catchabort(int n)
{
        int     cnt;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zign;
        zign.sighandler_el = SIG_IGN;
        sigmask_clear(zign);
        zign.sigflags_el = 0;
        sigact_routine(n, &zign, (struct sigstruct_name *) 0);
#else
        signal(n, SIG_IGN);
#endif
        for  (cnt = 0;  cnt < MAX_PEND_JOBS;  cnt++)
                abort_job(&pend_list[cnt]);
        exit(E_SIGNAL);
}

/* This notes signals from (presumably) the scheduler.  */

RETSIGTYPE  markit(int sig)
{
#ifdef  UNSAFE_SIGNALS
        signal(sig, markit);
#endif
        hadrfresh++;
}

static  void    catchsigs(void (*catchfn)(int))
{
        int     i;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z, oldz;
        z.sighandler_el = catchfn;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        for  (i = 0;  i < sizeof(sigstocatch);  i++)  {
                sigact_routine(sigstocatch[i], &z, &oldz);
                if  (oldz.sighandler_el == SIG_IGN)
                        sigact_routine(sigstocatch[i], &oldz, (struct sigstruct_name *) 0);
        }
#else
        for  (i = 0;  i < sizeof(sigstocatch);  i++)
                if  (signal(sigstocatch[i], catchfn) == SIG_IGN)
                        signal(sigstocatch[i], SIG_IGN);
#endif
}

static void  openrfile()
{
        /* If message queue does not exist, then the batch scheduler
           isn't running. I don't think that we want to randomly
           start it.  */

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }
#ifndef USING_FLOCK
        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, 0)) < 0)  {
                print_error($E{Cannot open semaphore});
                exit(E_SETUP);
        }
#endif
}

static void  lognprocess()
{
        Shipc           Oreq;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
        z.sighandler_el = markit;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(QRFRESH, &z, (struct sigstruct_name *) 0);
#else
        signal(QRFRESH, markit);
#endif
        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = N_XBNATT;
        Oreq.sh_params.upid = getpid();
        msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq), 0); /* Not expecting reply */
}

/* Unpack a job and see what British Hairyways has broken this time
   Report any errors with the appropriate code */

unsigned unpack_job(BtjobRef to, const struct nijobmsg *from, const unsigned length, const netid_t whofrom)
{
#ifndef WORDS_BIGENDIAN
        unsigned        cnt;
        JargRef darg;   EnvirRef denv;  RedirRef dred;
        const   Jarg  *sarg;    const   Envir *senv;    const   Redir *sred;
#endif

        BLOCK_ZERO(to, sizeof(Btjob));
        to->h.bj_orighostid     =       whofrom;
        to->h.bj_progress       =       from->ni_hdr.ni_progress;
        to->h.bj_pri            =       from->ni_hdr.ni_pri;
        to->h.bj_jflags         =       from->ni_hdr.ni_jflags;
        to->h.bj_times.tc_istime=       from->ni_hdr.ni_istime;
        to->h.bj_times.tc_mday  =       from->ni_hdr.ni_mday;
        to->h.bj_times.tc_repeat=       from->ni_hdr.ni_repeat;
        to->h.bj_times.tc_nposs =       from->ni_hdr.ni_nposs;

        to->h.bj_title          =       ntohs(from->ni_hdr.ni_title);
        to->h.bj_direct         =       ntohs(from->ni_hdr.ni_direct);
        to->h.bj_redirs         =       ntohs(from->ni_hdr.ni_redirs);
        to->h.bj_env            =       ntohs(from->ni_hdr.ni_env);
        to->h.bj_arg            =       ntohs(from->ni_hdr.ni_arg);

        to->h.bj_ll             =       ntohs(from->ni_hdr.ni_ll);
        to->h.bj_umask          =       ntohs(from->ni_hdr.ni_umask);
        to->h.bj_times.tc_nvaldays =    ntohs(from->ni_hdr.ni_nvaldays);
        to->h.bj_autoksig       =       ntohs(from->ni_hdr.ni_autoksig);
        to->h.bj_runon          =       ntohs(from->ni_hdr.ni_runon);
        to->h.bj_deltime        =       ntohs(from->ni_hdr.ni_deltime);

        to->h.bj_nredirs        =       ntohs(from->ni_hdr.ni_nredirs);
        to->h.bj_nargs          =       ntohs(from->ni_hdr.ni_nargs);
        to->h.bj_nenv           =       ntohs(from->ni_hdr.ni_nenv);
        to->h.bj_ulimit         =       ntohl(from->ni_hdr.ni_ulimit);
        to->h.bj_times.tc_nexttime =    ntohl(from->ni_hdr.ni_nexttime);
        to->h.bj_times.tc_rate  =       ntohl(from->ni_hdr.ni_rate);
        to->h.bj_runtime        =       ntohl(from->ni_hdr.ni_runtime);

        strcpy(to->h.bj_cmdinterp, from->ni_hdr.ni_cmdinterp);
        if  ((validate_ci(to->h.bj_cmdinterp)) < 0)
                return  XBNR_BADCI;

        to->h.bj_exits          =       from->ni_hdr.ni_exits;

        /* Copy in user and group from protocol later perhaps but
           maybe replacement user/group Do variables after we've
           done that.  */

        strncpy(to->h.bj_mode.o_user, from->ni_hdr.ni_mode.o_user, UIDSIZE);
        strncpy(to->h.bj_mode.o_group, from->ni_hdr.ni_mode.o_group, UIDSIZE);
        to->h.bj_mode.u_flags = ntohs(from->ni_hdr.ni_mode.u_flags);
        to->h.bj_mode.g_flags = ntohs(from->ni_hdr.ni_mode.g_flags);
        to->h.bj_mode.o_flags = ntohs(from->ni_hdr.ni_mode.o_flags);

        /* Now for the strings....  */

        BLOCK_COPY(to->bj_space, from->ni_space, length - sizeof(struct nijobhmsg));
#ifndef WORDS_BIGENDIAN
        darg = (JargRef) &to->bj_space[to->h.bj_arg];
        denv = (EnvirRef) &to->bj_space[to->h.bj_env];
        dred = (RedirRef) &to->bj_space[to->h.bj_redirs];
        sarg = (const Jarg *) &from->ni_space[to->h.bj_arg]; /* Did mean to!! */
        senv = (const Envir *) &from->ni_space[to->h.bj_env];
        sred = (const Redir *) &from->ni_space[to->h.bj_redirs];

        for  (cnt = 0;  cnt < to->h.bj_nargs;  cnt++)  {
                *darg++ = ntohs(*sarg);
                sarg++; /* Not falling for ntohs being a macro!!! */
        }
        for  (cnt = 0;  cnt < to->h.bj_nenv;  cnt++)  {
                denv->e_name = ntohs(senv->e_name);
                denv->e_value = ntohs(senv->e_value);
                denv++;
                senv++;
        }
        for  (cnt = 0;  cnt < to->h.bj_nredirs; cnt++)  {
                dred->arg = ntohs(sred->arg);
                dred++;
                sred++;
        }
#endif
        return  0;
}

/* Unpack condition and assignment vars in a job - done after we get
   the user ids.  Report any errors with the appropriate code */

unsigned  unpack_cavars(BtjobRef to, const struct nijobmsg *from)
{
        unsigned        cnt;
        netid_t         hostid;
        ULONG           Saveseq;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                const  Nicond   *cf = &from->ni_hdr.ni_conds[cnt];
                JcondRef        ct = &to->h.bj_conds[cnt];
                if  (cf->nic_compar == C_UNUSED)
                        break;
                rvarfile(1);
                hostid = ext2int_netid_t(cf->nic_var.ni_varhost);
                if  ((ct->bjc_varind = lookupvar(cf->nic_var.ni_varname, hostid, BTM_READ, &Saveseq)) < 0)  {
                        err_which = (USHORT) cnt;
                        return  XBNR_BADCVAR;
                }
                ct->bjc_compar = cf->nic_compar;
                ct->bjc_iscrit = cf->nic_iscrit;
                if  ((ct->bjc_value.const_type = cf->nic_type) == CON_STRING)
                        strncpy(ct->bjc_value.con_un.con_string, cf->nic_un.nic_string, BTC_VALUE);
                else
                        ct->bjc_value.con_un.con_long = ntohl(cf->nic_un.nic_long);
        }
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                const  Niass    *af = &from->ni_hdr.ni_asses[cnt];
                JassRef         at = &to->h.bj_asses[cnt];
                if  (af->nia_op == BJA_NONE)
                        break;
                rvarfile(1);
                hostid = ext2int_netid_t(af->nia_var.ni_varhost);
                if  ((at->bja_varind = lookupvar(af->nia_var.ni_varname, hostid, BTM_READ|BTM_WRITE, &Saveseq)) < 0)  {
                        err_which = (USHORT) cnt;
                        return  XBNR_BADAVAR;
                }
                at->bja_flags = ntohs(af->nia_flags);
                at->bja_op = af->nia_op;
                at->bja_iscrit = af->nia_iscrit;
                if  ((at->bja_con.const_type = af->nia_type) == CON_STRING)
                        strncpy(at->bja_con.con_un.con_string, af->nia_un.nia_string, BTC_VALUE);
                else
                        at->bja_con.con_un.con_long = ntohl(af->nia_un.nia_long);
        }
        return  0;
}

/* Pack up a btuser structure */

void  btuser_pack(BtuserRef to, BtuserRef from)
{
        to->btu_isvalid = from->btu_isvalid;
        to->btu_minp = from->btu_minp;
        to->btu_maxp = from->btu_maxp;
        to->btu_defp = from->btu_defp;
        to->btu_user = htonl(from->btu_user);   /* Don't really care about this */
        to->btu_maxll = htons(from->btu_maxll);
        to->btu_totll = htons(from->btu_totll);
        to->btu_spec_ll = htons(from->btu_spec_ll);
        to->btu_priv = htonl(from->btu_priv);
        to->btu_jflags[0] = htons(from->btu_jflags[0]);
        to->btu_jflags[1] = htons(from->btu_jflags[1]);
        to->btu_jflags[2] = htons(from->btu_jflags[2]);
        to->btu_vflags[0] = htons(from->btu_vflags[0]);
        to->btu_vflags[1] = htons(from->btu_vflags[1]);
        to->btu_vflags[2] = htons(from->btu_vflags[2]);
}

int  tcp_serv_open(SHORT portnum)
{
        int     result;
        struct  sockaddr_in     sin;
#ifdef  SO_REUSEADDR
        int     on = 1;
#endif
        sin.sin_family = AF_INET;
        sin.sin_port = portnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        sin.sin_addr.s_addr = INADDR_ANY;

        if  ((result = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                return  -1;
#ifdef  SO_REUSEADDR
        setsockopt(result, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
#endif
        if  (bind(result, (struct sockaddr *) &sin, sizeof(sin)) < 0  ||  listen(result, 5) < 0)  {
                close(result);
                return  -1;
        }
        return  result;
}

int  tcp_serv_accept(const int msock, netid_t *whofrom)
{
        int     sock;
        SOCKLEN_T            sinl;
        struct  sockaddr_in  sin;

        sinl = sizeof(sin);
        if  ((sock = accept(msock, (struct sockaddr *) &sin, &sinl)) < 0)
                return  -1;
        *whofrom = sockaddr2int_netid_t(&sin);
        return  sock;
}

int  udp_serv_open(SHORT portnum)
{
        int     result;
        struct  sockaddr_in     sin;
        sin.sin_family = AF_INET;
        sin.sin_port = portnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        sin.sin_addr.s_addr = INADDR_ANY;

        /* Open Datagram socket for user access stuff */

        if  ((result = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                return  -1;

        if  (bind(result, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
                close(result);
                return  -1;
        }
        return  result;
}

/* Set up network stuff - a UDP port to receive/send enquiries and UDP
   jobs on and a TCP port for receiving spr files.  */

static int  init_network()
{
        struct  servent *sp;

        if  (!(sp = env_getserv(Sname, IPPROTO_TCP)))  {
                disp_str = (char *) Sname;
                disp_str2 = "tcp";
                print_error($E{Netconn no service name});
                return  0;
        }

        /* Shhhhhh....  I know this should be network byte order, but
           lets leave it alone for now.  */

        qportnum = sp->s_port;
        if  (!(sp = env_getserv(Sname, IPPROTO_UDP)))  {
                disp_str = (char *) Sname;
                disp_str2 = "udp";
                print_error($E{Netconn no service name});
                return  0;
        }

        uaportnum = sp->s_port;

        if  (!(sp = env_getserv(ASrname, IPPROTO_TCP)))  {
                disp_str = (char *) ASrname;
                disp_str2 = "tcp";
                print_error($E{Netconn no API req port});
                return  0;
        }
        apirport = sp->s_port;

        if  (!(sp = env_getserv(ASmname, IPPROTO_UDP)))  {
                disp_str = (char *) ASmname;
                disp_str2 = "udp";
                print_error($E{Netconn no API prompt port});
                return  0;
        }
        apipport = sp->s_port;

        endservent();

        if  ((qsock = tcp_serv_open(qportnum)) < 0)  {
                disp_arg[0] = ntohs(qportnum);
                print_error($E{Netconn no open TCP});
                return  0;
        }

        if  ((uasock = udp_serv_open(uaportnum)) < 0)  {
                disp_arg[0] = ntohs(uaportnum);
                print_error($E{Netconn no open UDP});
                return  0;
        }

        if  ((apirsock = tcp_serv_open(apirport)) < 0)  {
                disp_arg[0] = ntohs(apirport);
                print_error($E{Netconn cannot open apisock});
                return  0;
        }
        return  1;
}

/* Generate output file name */

FILE *goutfile(jobno_t *jnp, char *tmpfl, const int isudp)
{
        FILE    *res;
        int     fid;
        for  (;;)  {
                strcpy(tmpfl, mkspid(SPNAM, *jnp));
                if  ((fid = open(tmpfl, isudp? O_RDWR|O_CREAT|O_EXCL: O_WRONLY|O_CREAT|O_EXCL, 0400)) >= 0)
                        break;
                *jnp += JN_INC;
        }
        if  (!isudp)
                catchsigs(catchdel);

        if  ((res = fdopen(fid, isudp? "w+": "w")) == (FILE *) 0)  {
                unlink(tmpfl);
                ABORT_NOMEM;
        }
        return  res;
}

static int  sock_read(const int sock, char *buffer, int nbytes)
{
        while  (nbytes > 0)  {
                int     rbytes = read(sock, buffer, nbytes);
                if  (rbytes <= 0)
                        return  0;
                buffer += rbytes;
                nbytes -= rbytes;
        }
        return  1;
}

/* Copy to output file.  (NB empty files are ok) */

static int  copyout(const int sock, FILE *outf)
{
        struct  ni_jobhdr       hd;
        char    buffer[CL_SV_BUFFSIZE];

        while  (sock_read(sock, (char *) &hd, sizeof(hd)) &&  hd.code == CL_SV_JOBDATA)  {
                int     inbytes, obytes = ntohs(hd.joblength);
                if  (!sock_read(sock, buffer, obytes))
                        break;
                for  (inbytes = 0;  inbytes < obytes;  inbytes++)
                        if  (putc(buffer[inbytes], outf) == EOF)  {
                                fclose(outf);
                                return  XBNR_FILE_FULL;
                        }
        }
        if  (fclose(outf) != EOF)
                return  0;
        return  XBNR_FILE_FULL;
}

static void  tcpreply(const int sock, const int code, const LONG param)
{
        int     nbytes, rbytes;
        char    *res;
        struct  client_if       result;

        result.code = (unsigned char) code;
        result.param = htonl(param);
        res = (char *) &result;
        nbytes = sizeof(result);
        do  {
                if  ((rbytes = write(sock, res, (unsigned) nbytes)) < 0)
                        return;
                res += rbytes;
                nbytes -= rbytes;
        }  while  (nbytes > 0);
}

int  validate_job(BtjobRef jp, const Btuser *userpriv)
{
        int     cinum;

        /* Can the geyser do anything at all */

        if  ((userpriv->btu_priv & BTM_CREATE) == 0)
                return  XBNR_NOCRPERM;

        /* Validate priority */

        if  (jp->h.bj_pri < userpriv->btu_minp  ||  jp->h.bj_pri > userpriv->btu_maxp)
                return  XBNR_BAD_PRIORITY;

        /* Check that we are happy about the job modes */

        if  (!(userpriv->btu_priv & BTM_UMASK)  &&
             (jp->h.bj_mode.u_flags != userpriv->btu_jflags[0] ||
              jp->h.bj_mode.g_flags != userpriv->btu_jflags[1] ||
              jp->h.bj_mode.o_flags != userpriv->btu_jflags[2]))
                return  XBNR_NOCMODE;

        /* Validate load level */

        if  ((cinum = validate_ci(jp->h.bj_cmdinterp)) < 0)
                return  XBNR_BADCI;

        if  (jp->h.bj_ll == 0)
                jp->h.bj_ll = Ci_list[cinum].ci_ll;
        else  {
                if  (jp->h.bj_ll > userpriv->btu_maxll)
                        return  XBNR_BAD_LL;
                if  (!(userpriv->btu_priv & BTM_SPCREATE) && jp->h.bj_ll != Ci_list[cinum].ci_ll)
                        return  XBNR_BAD_LL;
        }
        return  0;
}

/* In case where we have a UNIX user name passed to us, convert user name
   appropriately */

int    convert_unix_username(struct ni_jobhdr *nih, BtjobRef jp, BtuserRef *userprivp)
{
        char    *repu, *repg;
        int_ugid_t  nuid, ngid, possug;
        BtuserRef  mp;

        if  ((nuid = lookup_uname(nih->uname)) == UNKNOWN_UID)
                return  XBNR_UNKNOWN_USER;

        ngid = lastgid;

        Realuid = (uid_t) nuid;
        Realgid = (gid_t) ngid;

        if  ((ngid = lookup_gname(nih->gname)) != UNKNOWN_GID)
                Realgid = (gid_t) ngid;

        /* If we have a replacement user/group, they'll be in
                   the o_user/o_group fields.  */

        repu = jp->h.bj_mode.o_user;
        repg = jp->h.bj_mode.o_group;

         /* Need the current value of the user privs as it might have changed.  */

        if  (!(mp = getbtuentry(Realuid)))
                return  XBNR_BAD_USER;

        *userprivp = mp;

        /* If user or group has changed, we need the permission.  */

        if  (repu[0] && (possug = lookup_uname(repu)) != UNKNOWN_UID)
                nuid = possug;
        if  (repg[0] && (possug = lookup_gname(repg)) != UNKNOWN_GID)
                ngid = possug;
        if  (nuid != Realuid  ||  ngid != Realgid)  {
                if  (!(mp->btu_priv & BTM_WADMIN))
                        return  XBNR_BAD_USER;
                Realuid = nuid;
                Realgid = ngid;
        }
        return  0;
}

/* Process requests to enqueue file */

static void  process_q()
{
        int             sock, ret, tries;
        unsigned        joblength;
        ULONG           indx;
        netid_t         whofrom;
        PIDTYPE         pid;
        jobno_t         jn;
        BtuserRef       userpriv;
        FILE            *outf;
        Shipc           Oreq;
        struct  ni_jobhdr       nih;
        struct  nijobmsg        inj;
#ifdef  HAVE_SIGACTION
        sigset_t        sset;
        sigemptyset(&sset);
        sigaddset(&sset, QRFRESH);
        sigprocmask(SIG_BLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
        int     masked = siggetmask();
        sigblock(masked | sigmask(QRFRESH));
#else
        RETSIGTYPE  (*oldsig)(int);
#ifdef  SIG_HOLD
        oldsig = signal(QRFRESH, SIG_HOLD);
#else
        oldsig = signal(QRFRESH, SIG_IGN);
#endif
#endif
        if  ((sock = tcp_serv_accept(qsock, &whofrom)) < 0)  {
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }

        if  (!sock_read(sock, (char *) &nih, sizeof(nih)))  {
                close(sock);
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }

        if  ((pid = fork()) < 0)  {
                print_error($E{Cannot fork});
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }
#ifndef BUGGY_SIGCLD
        if  (pid != 0)  {
                close(sock);
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                return;
        }
#else
        /* Make the process the grandchild so we don't have to worry about waiting for it later.  */

        if  (pid != 0)  {
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, (int *) 0, 0) < 0  &&  errno == EINTR)
                        ;
#else
                PIDTYPE wpid;
                while  ((wpid = wait((int *) 0)) != pid  &&  (wpid >= 0 || errno == EINTR))
                        ;
#endif
#ifdef  HAVE_SIGACTION
                sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif defined(HAVE_SIGVEC) || defined(HAVE_SIGVECTOR)
                sigblock(masked);
#else
                signal(QRFRESH, oldsig);
#endif
                close(sock);
                return;
        }
        if  (fork() != 0)
                _exit(0);
#endif
        joblength = ntohs(nih.joblength);
        if  (joblength < sizeof(struct nijobhmsg) || joblength > sizeof(struct nijobmsg))  {
                tcpreply(sock, XBNR_BAD_JOBDATA, 0);
                _exit(0);
        }

        /* Slurp in rest of job header */

        sock_read(sock, (char *) &inj, (int) joblength);

        JREQ = &Xbuffer->Ring[indx = getxbuf_serv()];

        /* Find out if host is client (only from UNIX though, use UDP for
           Windows)
           isclient = (frp = lookup_hhash(whofrom))  &&  frp->isclient;
           Don't think we need this right now */

        if  ((ret = unpack_job(JREQ, &inj, joblength, whofrom)) != 0)  {
                freexbuf_serv(indx);
                tcpreply(sock, ret, err_which);
                _exit(0);
        }
        if  ((ret = convert_unix_username(&nih, JREQ, &userpriv)) != 0)  {
                freexbuf_serv(indx);
                tcpreply(sock, ret, 0);
                _exit(0);
        }
        if  ((ret = unpack_cavars(JREQ, &inj)) != 0)  {
                freexbuf_serv(indx);
                tcpreply(sock, ret, err_which);
                _exit(0);
        }
        if  ((ret = validate_job(JREQ, userpriv)) != 0)  {
                freexbuf_serv(indx);
                tcpreply(sock, XBNR_ERR, ret);
                _exit(0);
        }

        /* Kick off with current pid as job number.  */

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        mymtype = MTOFFSET + (jn = Oreq.sh_params.upid = getpid());
        Oreq.sh_params.mcode = J_CREATE;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        outf = goutfile(&jn, tmpfl, 0);
        JREQ->h.bj_job = jn;
        time(&JREQ->h.bj_time);

        if  ((ret = copyout(sock, outf)) != 0)  {
                tcpreply(sock, ret, 0);
                unlink(tmpfl);
                freexbuf_serv(indx);
                _exit(0);
        }

        Oreq.sh_un.sh_jobindex = indx;
        JREQ->h.bj_slotno = -1;
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        for  (tries = 0;  tries < MSGQ_BLOCKS;  tries++)  {
                if  (msgsnd(Ctrl_chan, (struct msgbuf *)&Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) >= 0)  {
                        freexbuf_serv(indx);
                        if  ((ret = readreply()) != J_OK)  {
                                unlink(tmpfl);
                                tcpreply(sock, XBNR_ERR, ret);
                                _exit(0);
                        }
                        tcpreply(sock, XBNQ_OK, jn);
                        _exit(0);
                }
                sleep(MSGQ_BLOCKWAIT);
        }
        freexbuf_serv(indx);
        unlink(tmpfl);
        tcpreply(sock, XBNR_QFULL, 0);
        _exit(0);
}

static void  process()
{
        int     highfd;
        fd_set  ready;
        struct  timeval  alrm;

        alrm.tv_usec = 0;

        highfd = qsock;
        if  (uasock > highfd)
                highfd = uasock;
        if  (apirsock > highfd)
                highfd = apirsock;

        for  (;;)  {

                /* If no timeouts in progress, don't bother with alarm */

                struct  timeval  *ap = (struct timeval *) 0;
                int  nret;
                unsigned  nexttime = process_alarm();

                if  (nexttime != 0)  {
                        alrm.tv_sec = nexttime;
                        ap = &alrm;
                }

                FD_ZERO(&ready);
                FD_SET(qsock, &ready);
                FD_SET(uasock, &ready);
                FD_SET(apirsock, &ready);

                if  ((nret = select(highfd+1, &ready, (fd_set *) 0, (fd_set *) 0, ap)) < 0)
                        if  (errno != EINTR)
                                exit(0);

                while  (nret > 0)  {
                        if  (FD_ISSET(apirsock, &ready))  {
                                process_api();
                                nret--;
                                continue;
                        }
                        if  (FD_ISSET(uasock, &ready))  {
                                process_ua();
                                nret--;
                                continue;
                        }
                        if  (FD_ISSET(qsock, &ready))  {
                                process_q();
                                nret--;
                                continue;
                        }
                }
        }
}

/* Ye olde main routine.
   I don't expect any arguments & will ignore
   any the fool gives me, apart from remembering my name.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int_ugid_t      chku;
        char    *trf;
#ifndef DEBUG
        PIDTYPE pid;
#endif
#ifdef  STRUCT_SIG
        struct  sigstruct_name  zign;
#endif

        versionprint(argv, "$Revision: 1.9 $", 1);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        init_xenv();

        if  ((Cfile = open_icfile()) == (FILE *) 0)
                exit(E_NOCONFIG);

        fcntl(fileno(Cfile), F_SETFD, 1);

        if  ((chku = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)  {
                Daemuid = ROOTID;
                Daemgid = getgid();
        }
        else  {
                Daemuid = chku;
                Daemgid = lastgid;
        }

        /* Revert to spooler user (we are setuser to root) */

#ifdef  SCO_SECURITY
        setluid(Daemuid);
        setuid(Daemuid);
#else
        setuid(Daemuid);
#endif

        orig_umask = umask(0);

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)  {
                print_error($E{Cannot change directory});
                return  E_SETUP;
        }

        /* Set timeout value */

        trf = envprocess(XBTIMEOUTS);
        timeouts = atoi(trf);
        free(trf);
        if (timeouts == 0)
                timeouts = NETTICKLE;

        /* Initial processing of host file */

        read_hfiles();

#ifdef  STRUCT_SIG
        zign.sighandler_el = catchhup;
        sigmask_clear(zign);
        zign.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGHUP, &zign, (struct sigstruct_name *) 0);
#else
        signal(SIGHUP, catchhup);
#endif
        myhostname = get_myhostname();
        myhostl = strlen(myhostname);
        openrfile();
        openjfile(1, 1);
        openvfile(1, 1);
        if  (open_ci(O_RDWR) != 0)  {
                print_error($E{Panic cannot open CI file});
                return  E_SETUP;
        }
        if  (!init_network())  {
                print_error($E{Netconn failure abort});
                exit(E_NETERR);
        }

#ifndef DEBUG

        while  ((pid = fork()) < 0)  {
                print_error($E{Cannot fork});
                sleep(30);
        }
        if  (pid != 0)
                return  0;

        /*      Don't bother with suppressing setpgrp because I don't think anything
                depends on it and because hangup signals would be misinterpreted. */

#ifdef  SETPGRP_VOID
        setpgrp();
#else
        setpgrp(0, getpid());
#endif
        catchsigs(catchabort);
#endif /* !DEBUG */
#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        zign.sighandler_el = SIG_IGN;
#ifdef  SA_NOCLDWAIT
        zign.sigflags_el |= SA_NOCLDWAIT;
#endif
        sigact_routine(SIGCLD, &zign, (struct sigstruct_name *) 0);
#else  /* !STRUCT_SIG */
        signal(SIGCLD, SIG_IGN);
#endif /* !STRUCT_SIG */
#endif /* !BUGGY_SIGCLD */

        lognprocess();
        initxbuffer(1);
        process();
        return  0;              /* Shut up compiler */
}
