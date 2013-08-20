/* remsubops.c -- remote job submission operations

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
#include "incl_unix.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "network.h"
#include "defaults.h"
#include "files.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "q_shm.h"
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "remsubops.h"
#include "services.h"

/* We currently use the same names for the TCP and UDP versions */
static   char    TSname[] = GBNETSERV_PORT;
#define Sname   TSname

/***********************************************************************
        UDP Access routines
 ***********************************************************************/

#define RTIMEOUT        5

int  remsub_initsock(int *rsock, const netid_t hostid, struct sockaddr_in *saddr)
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

/* Convert btuser structure */

void  remsub_unpack_btuser(Btuser *dest, const Btuser *src)
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

int     remsub_udp_enquire(const int sockfd, struct sockaddr_in *servaddr, char *outmsg, const int outlen, char *imsg, const int inlen)
{
        SOCKLEN_T       repl = sizeof(struct sockaddr_in);
        fd_set  ready;
        struct  timeval  tv;

        if  (sendto(sockfd, outmsg, outlen, 0, (struct sockaddr *) servaddr, sizeof(struct sockaddr_in)) < 0)  {
                disp_arg[1] = servaddr->sin_addr.s_addr;
                return  $E{Cannot send UDP packet};
        }
        FD_ZERO(&ready);
        FD_SET(sockfd, &ready);
        tv.tv_sec = RTIMEOUT;
        tv.tv_usec = 0;
        if  (select(sockfd+1, &ready, (fd_set *) 0, (fd_set *) 0, &tv) <= 0  ||  recvfrom(sockfd, imsg, inlen, 0, (struct sockaddr *) 0, &repl) != inlen)  {
                disp_arg[1] = servaddr->sin_addr.s_addr;
                return  $E{Cannot receive UDP packet};
        }
        return  0;
}

/***********************************************************************
        TCP Routines
 ***********************************************************************

 Find the port number we want to use for TCP ops
 or return an error code */

int  remsub_inittcp(int *tcpport)
{
        struct  servent *sp;

        /* Get port number for this caper */

        if  (!(sp = env_getserv(TSname, IPPROTO_TCP)))  {
                endservent();
                return  $EH{No xbnetserv TCP service};
        }
        *tcpport = sp->s_port;
        endservent();
        return  0;
}

int     remsub_opentcp(const netid_t nid, const int tcpport, int *sockp)
{
       int                     sock;
       struct  sockaddr_in     sin;

       if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
               return  $EH{Cannot open socket for remote job send};

       BLOCK_ZERO(&sin, sizeof(sin));
       sin.sin_family = AF_INET;
       sin.sin_port = tcpport;
       sin.sin_addr.s_addr = nid;

       if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
               close(sock);
               return  $EH{Cannot connect to remote};
       }
       *sockp = sock;
       return  0;
}

int  remsub_sock_read(const int sock, char *buffer, int nbytes)
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

int  remsub_sock_write(const int sock, char *buffer, int nbytes)
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

/* Pack up job for remote submission apart from conditions, assignments and command interps
   which we do slightly differently in shell routines. */

unsigned        remsub_packjob(struct nijobmsg *dest, CBtjobRef src)
{
        unsigned        ucnt, hwm;
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

        dest->ni_hdr.ni_exits = src->h.bj_exits;

        dest->ni_hdr.ni_mode.u_flags = htons(src->h.bj_mode.u_flags);
        dest->ni_hdr.ni_mode.g_flags = htons(src->h.bj_mode.g_flags);
        dest->ni_hdr.ni_mode.o_flags = htons(src->h.bj_mode.o_flags);


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

/* Copy in ci, conditions and assignments from job */

void    remsub_condasses(struct nijobmsg *dest, CBtjobRef src, const netid_t nhostid)
{
        int             cnt;
        unsigned        ucnt;

        strncpy(dest->ni_hdr.ni_cmdinterp, src->h.bj_cmdinterp, CI_MAXNAME);

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
}

int     remsub_startjob(const int sock, const int msgsize, const char *uname, const char *gname)
{
        struct  ni_jobhdr       nih;

        BLOCK_ZERO(&nih, sizeof(nih));
        nih.code = CL_SV_STARTJOB;
        nih.joblength = htons(msgsize);
        strncpy(nih.uname, uname, UIDSIZE);
        strncpy(nih.gname, gname, UIDSIZE);
        if  (!remsub_sock_write(sock, (char *) &nih, sizeof(nih)))
                return  $EH{Trouble with job header};
        return  0;
}

/* Send job script to remote and mark end */

void    remsub_copyout(FILE *scriptf, const int sockfd, const char *uname, const char *gname)
{
        int     inbytes;
        struct  ni_jobhdr       hd;
        char    buffer[CL_SV_BUFFSIZE];

        BLOCK_ZERO((char *) &hd, sizeof(hd));
        hd.code = CL_SV_JOBDATA;

        while  ((inbytes = fread(buffer, sizeof(char), CL_SV_BUFFSIZE, scriptf)) > 0)  {
                hd.joblength = htons(inbytes);
                remsub_sock_write(sockfd, (char *) &hd, sizeof(hd));
                remsub_sock_write(sockfd, buffer, inbytes);
        }
        hd.code = CL_SV_ENDJOB;
        hd.joblength = htons(sizeof(struct ni_jobhdr));
        strncpy(hd.uname, uname, UIDSIZE);
        strncpy(hd.gname, gname, UIDSIZE);
        remsub_sock_write(sockfd, (char *) &hd, sizeof(hd));
}

/* Send job script string to remote and mark end */

void    remsub_copyout_str(char *script, const int sockfd, const char *uname, const char *gname)
{
        int     lng = strlen(script);
        struct  ni_jobhdr       hd;

        BLOCK_ZERO((char *) &hd, sizeof(hd));
        hd.code = CL_SV_JOBDATA;
        while  (lng > 0)  {
                int     outlng = lng;
                if  (outlng > CL_SV_BUFFSIZE)
                        outlng = CL_SV_BUFFSIZE;
                hd.joblength = htons(outlng);
                remsub_sock_write(sockfd, (char *) &hd, sizeof(hd));
                remsub_sock_write(sockfd, script, outlng);
                lng -= outlng;
                script += outlng;
        }
        hd.code = CL_SV_ENDJOB;
        hd.joblength = htons(sizeof(struct ni_jobhdr));
        strncpy(hd.uname, uname, UIDSIZE);
        strncpy(hd.gname, gname, UIDSIZE);
        remsub_sock_write(sockfd, (char *) &hd, sizeof(hd));
}
