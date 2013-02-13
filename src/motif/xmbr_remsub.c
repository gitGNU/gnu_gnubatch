/* xmbr_remsub.c -- remote submit jobs for gbch-xmr

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
#include <sys/stat.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <Xm/List.h>
#include <Xm/SelectioB.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "q_shm.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "xbnetq.h"
#include "statenums.h"
#include "errnums.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "files.h"
#include "services.h"

static  char    Filename[] = __FILE__;

#define CLOSESOCKET(X)  close(X)
static  SHORT   tcpportnum = -1;

/* We currently use the same names for the TCP and UDP versions */
const   char    TSname[] = GBNETSERV_PORT;
#define Sname   TSname

/***********************************************************************
        UDP Access routines
 ***********************************************************************/

#define RTIMEOUT        5

static int  initsock(int *rsock, const netid_t hostid, struct sockaddr_in *saddr)
{
        int     sockfd;
        SHORT   portnum;
        struct  sockaddr_in     cli_addr;
        struct  servent *sp;

        /* Get port number for this caper */

        if  (!(sp = env_getserv(Sname, IPPROTO_UDP)))  {
                endservent();
                return  $EH{No xbnetserv UDP service};
        }
        portnum = sp->s_port;
        endservent();

        BLOCK_ZERO(saddr, sizeof(struct sockaddr_in));
        saddr->sin_family = AF_INET;
        saddr->sin_addr.s_addr = hostid;
        saddr->sin_port = portnum;
        BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
        cli_addr.sin_family = AF_INET;
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        cli_addr.sin_port = 0;

        /* Save now in case of error.  */

        disp_arg[0] = ntohs(portnum);
        disp_arg[1] = hostid;

        if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                return  $EH{Cannot create UDP access socket};

        if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
                close(sockfd);
                return  $EH{Cannot bind UDP access socket};
        }
        *rsock = sockfd;
        return  0;
}

static  RETSIGTYPE  asig(int n)
{
        return;                 /* Don't do anything just return setting EINTR */
}

static int  udp_enquire(const int sockfd, struct sockaddr_in *saddr, char *outmsg, const int outlen, char *inmsg, const int inlen)
{
#ifdef  STRUCT_SIG
        struct  sigstruct_name  za, zold;
#else
        RETSIGTYPE      (*oldsig)();
#endif
        SOCKLEN_T               repl = sizeof(struct sockaddr_in);
        struct  sockaddr_in     reply_addr;
        if  (sendto(sockfd, outmsg, outlen, 0, (struct sockaddr *) saddr, sizeof(struct sockaddr_in)) < 0)  {
                disp_arg[1] = saddr->sin_addr.s_addr;
                return  $EH{Cannot send UDP packet};
        }
#ifdef  STRUCT_SIG
        za.sighandler_el = asig;
        sigmask_clear(za);
        za.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGALRM, &za, &zold);
#else
        oldsig = signal(SIGALRM, asig);
#endif
        alarm(RTIMEOUT);
        if  (recvfrom(sockfd, inmsg, inlen, 0, (struct sockaddr *) &reply_addr, &repl) <= 0)  {
                disp_arg[1] = saddr->sin_addr.s_addr;
                return  $EH{Cannot receive UDP packet};
        }
        alarm(0);
#ifdef  STRUCT_SIG
        sigact_routine(SIGALRM, &zold, (struct sigstruct_name *) 0);
#else
        signal(SIGALRM, oldsig);
#endif
        return  0;
}

/* Unpack btuser from networked version.  */

static void  unpack_btuser(Btuser *dest, const Btuser *src)
{
        dest->btu_isvalid = src->btu_isvalid;
        dest->btu_minp = src->btu_minp;
        dest->btu_maxp = src->btu_maxp;
        dest->btu_defp = src->btu_defp;
        dest->btu_user = ntohl(src->btu_user);
        dest->btu_maxll = ntohs(src->btu_maxll);
        dest->btu_totll = ntohs(src->btu_totll);
        dest->btu_spec_ll = ntohs(src->btu_spec_ll);
        dest->btu_priv = ntohl(src->btu_priv);
        dest->btu_jflags[0] = ntohs(src->btu_jflags[0]);
        dest->btu_jflags[1] = ntohs(src->btu_jflags[1]);
        dest->btu_jflags[2] = ntohs(src->btu_jflags[2]);
        dest->btu_vflags[0] = ntohs(src->btu_vflags[0]);
        dest->btu_vflags[1] = ntohs(src->btu_vflags[1]);
        dest->btu_vflags[2] = ntohs(src->btu_vflags[2]);
}

/***********************************************************************
        TCP Routines
 ***********************************************************************

 Find the port number we want to use for TCP ops
 or return an error code */

static int  inittcp()
{
        struct  servent *sp;
        /* Get port number for this caper */
        if  (!(sp = env_getserv(TSname, IPPROTO_TCP)))  {
                endservent();
                return  $EH{No xbnetserv TCP service};
        }
        tcpportnum = sp->s_port;
        endservent();
        return  0;
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

static int  sock_write(const int sock, char *buffer, int nbytes)
{
        while  (nbytes > 0)  {
                int     rbytes = write(sock, buffer, nbytes);
                if  (rbytes < 0)
                        return  0;
                buffer += rbytes;
                nbytes -= rbytes;
        }
        return  1;
}

static void  copyout(FILE *inf, const int sockfd)
{
        int     outbytes, ch;
        struct  ni_jobhdr       hd;
        char    buffer[CL_SV_BUFFSIZE];

        BLOCK_ZERO((char *) &hd, sizeof(hd));
        hd.code = CL_SV_JOBDATA;
        outbytes = 0;

        while  ((ch = getc(inf)) != EOF)  {
                if  (outbytes >= CL_SV_BUFFSIZE)  {
                        hd.joblength = htons(outbytes);
                        sock_write(sockfd, (char *) &hd, sizeof(hd));
                        sock_write(sockfd, buffer, outbytes);
                        outbytes = 0;
                }
                buffer[outbytes++] = (char) ch;
        }
        if  (outbytes > 0)  {
                hd.joblength = htons(outbytes);
                sock_write(sockfd, (char *) &hd, sizeof(hd));
                sock_write(sockfd, buffer, outbytes);
        }
        hd.code = CL_SV_ENDJOB;
        hd.joblength = htons(sizeof(struct ni_jobhdr));
        strncpy(hd.uname, prin_uname(Realuid), UIDSIZE);
        strncpy(hd.gname, prin_gname(Realgid), UIDSIZE);
        sock_write(sockfd, (char *) &hd, sizeof(hd));
}

static unsigned  packjob(struct nijobmsg *dest, CBtjobRef src, const netid_t nhostid)
{
        int             cnt;
        unsigned        ucnt;
        unsigned        hwm;
        JargRef         darg;
        EnvirRef        denv;
        RedirRef        dred;
        const   Jarg    *sarg;
        const   Envir   *senv;
        const   Redir   *sred;

        BLOCK_ZERO((char *) dest, sizeof(struct nijobmsg));

        dest->ni_hdr.ni_progress = src->h.bj_progress;
        dest->ni_hdr.ni_pri = src->h.bj_pri;
        dest->ni_hdr.ni_jflags = src->h.bj_jflags;
        dest->ni_hdr.ni_istime = src->h.bj_times.tc_istime;
        dest->ni_hdr.ni_mday = src->h.bj_times.tc_mday;
        dest->ni_hdr.ni_repeat = src->h.bj_times.tc_repeat;
        dest->ni_hdr.ni_nposs = src->h.bj_times.tc_nposs;

        /* Do the "easy" bits */

        dest->ni_hdr.ni_ll = htons(src->h.bj_ll);
        dest->ni_hdr.ni_umask = htons(src->h.bj_umask);
        dest->ni_hdr.ni_nvaldays = htons(src->h.bj_times.tc_nvaldays);
        dest->ni_hdr.ni_ulimit = htonl(src->h.bj_ulimit);
        dest->ni_hdr.ni_nexttime = htonl(src->h.bj_times.tc_nexttime);
        dest->ni_hdr.ni_rate = htonl(src->h.bj_times.tc_rate);
        dest->ni_hdr.ni_autoksig = htons(src->h.bj_autoksig);
        dest->ni_hdr.ni_runon = htons(src->h.bj_runon);
        dest->ni_hdr.ni_deltime = htons(src->h.bj_deltime);
        dest->ni_hdr.ni_runtime = htonl(src->h.bj_runtime);

        strncpy(dest->ni_hdr.ni_cmdinterp, src->h.bj_cmdinterp, CI_MAXNAME);

        dest->ni_hdr.ni_exits = src->h.bj_exits;

        dest->ni_hdr.ni_mode.u_flags = htons(src->h.bj_mode.u_flags);
        dest->ni_hdr.ni_mode.g_flags = htons(src->h.bj_mode.g_flags);
        dest->ni_hdr.ni_mode.o_flags = htons(src->h.bj_mode.o_flags);

        for  (ucnt = 0, cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                Nicond          *nic;
                BtvarRef        vp;
                CJcondRef       sic = &src->h.bj_conds[cnt];
                if  (sic->bjc_compar == C_UNUSED)
                        continue;
                vp = &Var_seg.vlist[sic->bjc_varind].Vent;
                nic = &dest->ni_hdr.ni_conds[ucnt];
                nic->nic_var.ni_varhost = int2ext_netid_t(vp->var_id.hostid);
                if  (vp->var_id.hostid == 0  &&  vp->var_type == VT_MACHNAME)
                        nic->nic_var.ni_varhost = nhostid;
                strncpy(nic->nic_var.ni_varname, vp->var_name, BTV_NAME);
                nic->nic_compar = sic->bjc_compar;
                nic->nic_iscrit = sic->bjc_iscrit;
                nic->nic_type = (unsigned char) sic->bjc_value.const_type;
                if  (nic->nic_type == CON_STRING)
                        strncpy(nic->nic_un.nic_string, sic->bjc_value.con_un.con_string, BTC_VALUE);
                else
                        nic->nic_un.nic_long = htonl(sic->bjc_value.con_un.con_long);
                ucnt++;
        }
        for  (ucnt = 0, cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                Niass           *nia;
                BtvarRef        vp;
                CJassRef        sia = &src->h.bj_asses[cnt];
                if  (sia->bja_op == BJA_NONE)
                        continue;
                nia = &dest->ni_hdr.ni_asses[ucnt];
                vp = &Var_seg.vlist[sia->bja_varind].Vent;
                nia->nia_var.ni_varhost = int2ext_netid_t(vp->var_id.hostid);
                strncpy(nia->nia_var.ni_varname, vp->var_name, BTV_NAME);
                nia->nia_flags = htons(sia->bja_flags);
                nia->nia_op = sia->bja_op;
                nia->nia_iscrit = sia->bja_iscrit;
                nia->nia_type = (unsigned char) sia->bja_con.const_type;
                if  (nia->nia_type == CON_STRING)
                        strncpy(nia->nia_un.nia_string, sia->bja_con.con_un.con_string, BTC_VALUE);
                else
                        nia->nia_un.nia_long = htonl(sia->bja_con.con_un.con_long);
                ucnt++;
        }

        dest->ni_hdr.ni_nredirs = htons(src->h.bj_nredirs);
        dest->ni_hdr.ni_nargs = htons(src->h.bj_nargs);
        dest->ni_hdr.ni_nenv = htons(src->h.bj_nenv);
        dest->ni_hdr.ni_title = htons(src->h.bj_title);
        dest->ni_hdr.ni_direct = htons(src->h.bj_direct);
        dest->ni_hdr.ni_arg = htons(src->h.bj_arg);
        dest->ni_hdr.ni_redirs = htons(src->h.bj_redirs);
        dest->ni_hdr.ni_env = htons(src->h.bj_env);
        BLOCK_COPY(dest->ni_space, src->bj_space, JOBSPACE);

        /* Cheat by assuming that packjstring put the directory and
           title in last and we can use the offset of that as a
           high water mark.  */

        if  (src->h.bj_title >= 0)  {
                hwm = src->h.bj_title;
                hwm += strlen(&src->bj_space[hwm]) + 1;
        }
        else  if  (src->h.bj_direct >= 0)  {
                hwm = src->h.bj_direct;
                hwm += strlen(&src->bj_space[hwm]) + 1;
        }
        else
                hwm = JOBSPACE;

        hwm += sizeof(struct nijobmsg) - JOBSPACE;

        /* We must swap the argument, environment and redirection
           variable pointers and the arg field in each redirection.  */

        darg = (JargRef) &dest->ni_space[src->h.bj_arg];        /* I did mean src there */
        denv = (EnvirRef) &dest->ni_space[src->h.bj_env];       /* and there */
        dred = (RedirRef) &dest->ni_space[src->h.bj_redirs];    /* and there */
        sarg = (const Jarg *) &src->bj_space[src->h.bj_arg];
        senv = (const Envir *) &src->bj_space[src->h.bj_env];
        sred = (const Redir *) &src->bj_space[src->h.bj_redirs];

        for  (ucnt = 0;  ucnt < src->h.bj_nargs;  ucnt++)  {
                *darg++ = htons(*sarg);
                sarg++;         /* Not falling for htons being a macro!!! */
        }
        for  (ucnt = 0;  ucnt < src->h.bj_nenv;  ucnt++)  {
                denv->e_name = htons(senv->e_name);
                denv->e_value = htons(senv->e_value);
                denv++;
                senv++;
        }
        for  (ucnt = 0;  ucnt < src->h.bj_nredirs; ucnt++)  {
                dred->arg = htons(sred->arg);
                dred++;
                sred++;
        }
        return  hwm;
}

/* Get ourselves an out socket to the server on the remote machine or
   return a suitable error code */

static int  remgoutfile(const netid_t hostid, CBtjobRef jb, int *rsock)
{
        int                     sock;
        int                     msgsize;
        struct  sockaddr_in     sin;
        struct  ni_jobhdr       nih;
        struct  nijobmsg        outmsg;

        if  (tcpportnum < 0  &&  (sock = inittcp()) != 0)
                return  sock;

        if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                return  $EH{Cannot open socket for remote job send};

        /* Set up bits and pieces The port number is set up in the job
           shared memory segment.  */

        sin.sin_family = AF_INET;
        sin.sin_port = tcpportnum;
        BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
        sin.sin_addr.s_addr = hostid;

        if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
                close(sock);
                return  $EH{Cannot connect to remote};
        }

        msgsize = packjob(&outmsg, jb, hostid);
        nih.code = CL_SV_STARTJOB;
        nih.padding = 0;
        nih.joblength = htons(msgsize);
        strcpy(nih.uname, prin_uname(Realuid));
        strcpy(nih.gname, prin_gname(Realgid));

        if  (!sock_write(sock, (char *) &nih, sizeof(nih)))  {
                close(sock);
                return  $EH{Trouble with job header};
        }

        if  (!sock_write(sock, (char *) &outmsg, (int) msgsize))  {
                close(sock);
                return  $EH{Trouble with job};
        }
        *rsock = sock;
        return  0;
}

static int  remprocreply(const int sock, BtjobRef jb)
{
        int     errcode, which;
        struct  client_if       result;

        if  (!sock_read(sock, (char *) &result, sizeof(result)))
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

/***********************************************************************
        Here is the stuff for doing the remote submission.
 ***********************************************************************/

#define INIT_HOSTS      20
#define INC_HOSTS       10

static  unsigned        nhosts, maxhosts;
struct  hmem  {
        char    *name;
        netid_t hid;
} *hlist;

static  int     curr_host = -1; /* host number -1 means none */

struct  remparams  {            /* Info from remote */
        USHORT  umsk;           /* Umask value */
        ULONG   ulmt;           /* Ulimit value */
        char    realgname[UIDSIZE+1];   /* Group name */
        Btuser  uperms;         /* User permissions */
};

static  struct  remparams  defh_parms;  /* Parameters of default host */

static void  ahost(const netid_t nid, char *nam)
{
        if  (nhosts >= maxhosts)  {
                maxhosts += INC_HOSTS;
                if  (!(hlist = (struct hmem *) realloc((char *) hlist, maxhosts * sizeof(struct hmem))))
                        ABORT_NOMEM;
        }
        hlist[nhosts].name = stracpy(nam);
        hlist[nhosts].hid = nid;
        nhosts++;
}

/* For sorting host names into alphabetical order */
static int  s_nh(struct hmem *a, struct hmem *b)
{
        return  strcmp(a->name, b->name);
}

/* Read host file for likely people to speak to */
static void  ghostlist()
{
        struct  remote  *rp;
        extern  char    hostf_errors;

        if  (hlist)
                return;

        if  (!(hlist = (struct hmem *) malloc(INIT_HOSTS*sizeof(struct hmem))))
                ABORT_NOMEM;

        maxhosts = INIT_HOSTS;

        while  ((rp = get_hostfile()))  {
                if  (rp->ht_flags & (HT_DOS|HT_ROAMUSER))
                        continue;
                if  (rp->hostname[0])
                        ahost(rp->hostid, rp->hostname);
                if  (rp->alias[0])
                        ahost(rp->hostid, rp->alias);
        }
        end_hostfile();
        if  (hostf_errors)
                doerror(jwid, $EH{Warn errors in host file});
        if  (nhosts > 1)
                qsort(QSORTP1 hlist, nhosts, sizeof(struct hmem), QSORTP4 s_nh);
}


static  char            *nohmsg;

static  int             new_host;
#define CANCELLED       -3
#define NOT_SELECTED    -2
#define NONE_SELECTED   -1

static void  hselresp(Widget w, int nullok, XmSelectionBoxCallbackStruct *cbs)
{
        int     cnt;
        char    *result;

        switch  (cbs->reason)  {
        case  XmCR_OK:
                if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &result))
                        return;
                if  (!result)
                        return;
                if  (result[0] == '\0'  ||  (nohmsg  &&  strcmp(result, nohmsg) == 0))  {
                        XtFree(result);
                        if  (nullok)  {
                                new_host = NONE_SELECTED;
                                break;
                        }
                        return;
                }
                for  (cnt = 0;  cnt < nhosts;  cnt++)
                        if  (strcmp(result, hlist[cnt].name) == 0)  {
                                XtFree(result);
                                new_host = cnt;
                                XtDestroyWidget(w);
                                return;
                        }
                XtFree(result);
                goto  nom;
        case  XmCR_CANCEL:
                new_host = CANCELLED;
                break;
        case  XmCR_NO_MATCH:
        nom:
                doerror(w, $EH{Invalid host name});
                return;
        }
        XtDestroyWidget(w);
}

static int  choosehost(char *dlgtitle, struct remparams *rp, const int nullok, const int exist)
{
        Widget                  dlg;
        int                     udpsock, ec;
        struct  remparams       possn;
        struct  sockaddr_in     serv_addr;
        struct  ni_jobhdr       enq;
        struct  ua_reply        resp1;
        struct  ua_umlreply     resp2;

        ghostlist();
        if  (nhosts == 0)  {
                doerror(jwid, $EH{No hosts found});
                return  CANCELLED;
        }

        if  (!nullok  &&  (nhosts == 1  || (nhosts == 2 && hlist[0].hid == hlist[1].hid)))  {
                disp_str = hlist[0].name;
                if  (!Confirm(jwid, $PH{OK to use only host}))
                        return  CANCELLED;
                new_host = 0;
                goto  gotit;
        }

        dlg = XmCreateSelectionDialog(jwid, dlgtitle, NULL, 0);
        if  (nullok)  {
                int             cnt, indx;
                XmString        *strlist = (XmString *) XtMalloc((nhosts + 1) * sizeof(XmString));

                if  (!nohmsg)
                        nohmsg = gprompt($P{xmbtr none host});

                strlist[0] = XmStringCreateSimple(nohmsg);

                for  (cnt = 0, indx = 1;  cnt < nhosts;  indx++, cnt++)
                        strlist[indx] = XmStringCreateSimple(hlist[cnt].name);

                XtVaSetValues(dlg,
                              XmNlistItems,     strlist,
                              XmNlistItemCount, (int) nhosts + 1,
                              XmNmustMatch,     True,
                              XmNtextString,    exist >= 0? strlist[exist+1]: strlist[0],
                              NULL);

                for  (cnt = 0;  cnt <= nhosts;  cnt++)
                        XmStringFree(strlist[cnt]);

                XtFree((XtPointer) strlist);
        }
        else  {
                int     cnt;
                XmString                *strlist = (XmString *) XtMalloc(nhosts * sizeof(XmString));

                for  (cnt = 0;  cnt < nhosts;  cnt++)
                        strlist[cnt] = XmStringCreateSimple(hlist[cnt].name);
                if  (exist >= 0)
                        XtVaSetValues(dlg,
                                      XmNlistItems,     strlist,
                                      XmNlistItemCount, (int) nhosts,
                                      XmNmustMatch,     True,
                                      XmNtextString,    strlist[exist],
                                      NULL);
                else
                        XtVaSetValues(dlg,
                                      XmNlistItems,     strlist,
                                      XmNlistItemCount, (int) nhosts,
                                      XmNmustMatch,     True,
                                      NULL);

                for  (cnt = 0;  cnt < nhosts;  cnt++)
                        XmStringFree(strlist[cnt]);

                XtFree((XtPointer) strlist);
        }

        XtUnmanageChild(XmSelectionBoxGetChild(dlg, XmDIALOG_APPLY_BUTTON));
        XtUnmanageChild(XmSelectionBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));

        XtAddCallback(dlg, XmNokCallback, (XtCallbackProc) hselresp, INT_TO_XTPOINTER(nullok));
        XtAddCallback(dlg, XmNcancelCallback, (XtCallbackProc) hselresp, (XtPointer) 0);
        XtAddCallback(dlg, XmNnoMatchCallback, (XtCallbackProc) hselresp, (XtPointer) 0);

        new_host = NOT_SELECTED;
        XtManageChild(dlg);
        XtPopup(XtParent(dlg), XtGrabNone);

        while  (new_host == NOT_SELECTED)
                XtAppProcessEvent(app, XtIMAll);

        if  (new_host < 0)      /* CANCELLED or NONE_SELECTED */
                return  new_host;

 gotit:

        if  ((ec = initsock(&udpsock, hlist[new_host].hid, &serv_addr)) != 0)  {
                doerror(jwid, ec);
                return  CANCELLED;
        }

        BLOCK_ZERO(&enq, sizeof(enq));
        enq.code = CL_SV_UENQUIRY;
        enq.joblength = htons(sizeof(enq));
        strncpy(enq.uname, prin_uname(Realuid), UIDSIZE);
        if  ((ec = udp_enquire(udpsock, &serv_addr, (char *) &enq, sizeof(enq), (char *) &resp1, sizeof(resp1))) != 0)  {
                CLOSESOCKET(udpsock);
                doerror(jwid, ec);
                return  CANCELLED;
        }
        strcpy(possn.realgname, resp1.ua_gname);
        unpack_btuser(&possn.uperms, &resp1.ua_perm);
        if  (!possn.uperms.btu_isvalid)  {
                CLOSESOCKET(udpsock);
                doerror(jwid, $EH{No such user on remote});
                return  CANCELLED;
        }
        enq.code = CL_SV_UMLPARS;
        enq.joblength = htons(sizeof(enq));
        if  ((ec = udp_enquire(udpsock, &serv_addr, (char *) &enq, sizeof(enq), (char *) &resp2, sizeof(resp2))) != 0)  {
                CLOSESOCKET(udpsock);
                doerror(jwid, ec);
                return  CANCELLED;
        }
        possn.umsk = ntohs(resp2.ua_umask);
        possn.ulmt = ntohl(resp2.ua_ulimit);
        CLOSESOCKET(udpsock);
        *rp = possn;
        return  new_host;
}

void  cb_defhost(Widget w, int notused)
{
        int     res = choosehost("defhost", &defh_parms, 1, curr_host);
        if  (res < 0)  {
                if  (res == NONE_SELECTED)
                        curr_host = -1;
                return;
        }
        curr_host = res;
}

void  cb_remsubmit(Widget w, int notused)
{
        int                     indx, ec;
        struct  pend_job        *pj;
        BtjobRef                jreq;
        char                    *path;
        FILE                    *inf;
        int                     outsock = 0;
        struct  remparams       *hp;
        struct  remparams       h_parms;
        extern  char *gen_path(char *, char *);

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];
        if  (pj->nosubmit == 0  &&  !Confirm(w, $PH{xmbtr job unchanged confirm}))
                return;

        jreq = pj->job;
        if  (jreq->h.bj_times.tc_istime  &&  jreq->h.bj_times.tc_nexttime < time((time_t *) 0))  {
                doerror(jwid, $EH{xmbtr cannot submit not future});
                return;
        }

        if  (!pj->jobfile_name)  {
                doerror(w, $EH{xmbtr no job file name});
                return;
        }

        path = gen_path(pj->directory, pj->jobfile_name);
        if  (!f_exists(path))  {
                free(path);
                doerror(w, $EH{xmbtr no such file});
                return;
        }
        if  (!(inf = fopen(path, "r")))  {
                free(path);
                doerror(w, $EH{xmbtr cannot open job file});
                return;
        }
        free(path);

        if  (curr_host < 0)  {
                hp = &h_parms;
                if  ((indx = choosehost("whichhost", hp, 0, -1)) < 0)  {
                        fclose(inf);
                        return;
                }
        }
        else  {
                hp = &defh_parms;
                indx = curr_host;
        }

        if  ((ec = remgoutfile(hlist[indx].hid, jreq, &outsock)) != 0)  {
                doerror(w, ec);
                close(outsock);
                fclose(inf);
                return;
        }
        copyout(inf, outsock);
        fclose(inf);
        if  ((ec = remprocreply(outsock, jreq)) != 0)  {
                disp_arg[1] = hlist[indx].hid;
                doerror(w, ec);
                close(outsock);
                return;
        }
        close(outsock);
        if  (pj->Verbose)  {
                disp_arg[0] = jreq->h.bj_job;
                disp_arg[1] = hlist[indx].hid;
                disp_str = title_of(jreq);
                doinfo(jwid, disp_str[0]? $E{xmbtr remote job created ok title}: $E{xmbtr remote job created ok no title});
        }
        pj->nosubmit = 0;
}
