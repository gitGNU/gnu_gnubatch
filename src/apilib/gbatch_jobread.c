/* gbatch_jobread.c -- API function to read job

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

#include <stdio.h>
#include <sys/types.h>
#include "gbatch.h"
#include "xbapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "netmsg.h"

extern void  gbatch_mode_unpack(Btmode *, const Btmode *);
extern int  gbatch_read(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

static int  job_readrest(struct api_fd *fdp, apiBtjob *result)
{
        int             ret;
        unsigned        length, cnt;
#ifndef WORDS_BIGENDIAN
        USHORT  *darg;          Envir   *denv;          Redir   *dred;
        const   USHORT *sarg;   const   Envir *senv;    const   Redir *sred;
#endif
        apiJcond        *drc;
        apiJass         *dra;
        struct  jobnetmsg       res;

        if  ((ret = gbatch_read(fdp->sockfd, (char *) &res.hdr.hdr, sizeof(msghdr))) != 0)
                return  ret;

        length = (SHORT) ntohs(res.hdr.hdr.length) - sizeof(msghdr);

        /* Read rest of message...  */

        if  ((ret = gbatch_read(fdp->sockfd, sizeof(msghdr) + (char *) &res, length)) != 0)
                return  ret;

        /* And now do all the byte-swapping */

        BLOCK_ZERO(&result->h, sizeof(apiBtjobh));      /* Needed to init conds/asses */
        result->h.bj_hostid = res.hdr.jid.hostid;
        result->h.bj_slotno = ntohl(res.hdr.jid.slotno);

        result->h.bj_progress = res.hdr.nm_progress;
        result->h.bj_pri      = res.hdr.nm_pri;
        result->h.bj_jflags   = res.hdr.nm_jflags;
        result->h.bj_times.tc_istime= res.hdr.nm_istime;
        result->h.bj_times.tc_mday  = res.hdr.nm_mday;
        result->h.bj_times.tc_repeat= res.hdr.nm_repeat;
        result->h.bj_times.tc_nposs = res.hdr.nm_nposs;

        result->h.bj_ll                 = ntohs(res.hdr.nm_ll);
        result->h.bj_umask              = ntohs(res.hdr.nm_umask);
        result->h.bj_times.tc_nvaldays  = ntohs(res.hdr.nm_nvaldays);
        result->h.bj_autoksig           = ntohs(res.hdr.nm_autoksig);
        result->h.bj_runon              = ntohs(res.hdr.nm_runon);
        result->h.bj_deltime            = ntohs(res.hdr.nm_deltime);
        result->h.bj_lastexit           = ntohs(res.hdr.nm_lastexit);

        result->h.bj_job                = ntohl(res.hdr.nm_job);
        result->h.bj_time               = (time_t) ntohl(res.hdr.nm_time);
        result->h.bj_stime              = (time_t) ntohl(res.hdr.nm_stime);
        result->h.bj_etime              = (time_t) ntohl(res.hdr.nm_etime);
        result->h.bj_pid                = ntohl(res.hdr.nm_pid);
        result->h.bj_orighostid         = res.hdr.nm_orighostid;
        result->h.bj_runhostid          = res.hdr.nm_runhostid;
        result->h.bj_ulimit             = ntohl(res.hdr.nm_ulimit);
        result->h.bj_times.tc_nexttime  = (time_t) ntohl(res.hdr.nm_nexttime);
        result->h.bj_times.tc_rate      = ntohl(res.hdr.nm_rate);
        result->h.bj_runtime            = ntohl(res.hdr.nm_runtime);

        strcpy(result->h.bj_cmdinterp, res.hdr.nm_cmdinterp);
        result->h.bj_exits              = res.hdr.nm_exits;
        gbatch_mode_unpack(&result->h.bj_mode, &res.hdr.nm_mode);

        drc = result->h.bj_conds;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                const Jncond    *cr = &res.hdr.nm_conds[cnt];

                if  (cr->bjnc_compar == C_UNUSED)
                        break;

                drc->bjc_var.slotno = ntohl(cr->bjnc_var.slotno);
                drc->bjc_compar = cr->bjnc_compar;
                drc->bjc_iscrit = cr->bjnc_iscrit;
                if  ((drc->bjc_value.const_type = cr->bjnc_type) == CON_STRING)
                        strncpy(drc->bjc_value.con_un.con_string, cr->bjnc_un.bjnc_string, BTC_VALUE);
                else
                        drc->bjc_value.con_un.con_long = ntohl(cr->bjnc_un.bjnc_long);
                drc++;
        }

        dra = result->h.bj_asses;

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                const Jnass     *cr = &res.hdr.nm_asses[cnt];
                if  (cr->bjna_op == BJA_NONE)
                        break;
                dra->bja_var.slotno = ntohl(cr->bjna_var.slotno);
                dra->bja_flags = ntohs(cr->bjna_flags);
                dra->bja_op = cr->bjna_op;
                dra->bja_iscrit = cr->bjna_iscrit;
                if  ((dra->bja_con.const_type = cr->bjna_type) == CON_STRING)
                        strncpy(dra->bja_con.con_un.con_string, cr->bjna_un.bjna_string, BTC_VALUE);
                else
                        dra->bja_con.con_un.con_long = ntohl(cr->bjna_un.bjna_long);
                dra++;
        }

        result->h.bj_nredirs    = ntohs(res.nm_nredirs);
        result->h.bj_nargs      = ntohs(res.nm_nargs);
        result->h.bj_nenv       = ntohs(res.nm_nenv);
        result->h.bj_title      = ntohs(res.nm_title);
        result->h.bj_direct     = ntohs(res.nm_direct);
        result->h.bj_redirs     = ntohs(res.nm_redirs);
        result->h.bj_env        = ntohs(res.nm_env);
        result->h.bj_arg        = ntohs(res.nm_arg);

        length = (SHORT) ntohs(res.hdr.hdr.length) - (sizeof(struct jobnetmsg) - JOBSPACE);
        if  (length > JOBSPACE)
                length = JOBSPACE;
        BLOCK_COPY(result->bj_space, res.nm_space, length);

#ifndef WORDS_BIGENDIAN

        /* If we are byte-swapping we must swap the argument,
           environment and redirection variable pointers and the
           arg field in each redirection.  */

        darg = (USHORT *) &result->bj_space[result->h.bj_arg];
        denv = (Envir *) &result->bj_space[result->h.bj_env];
        dred = (Redir *) &result->bj_space[result->h.bj_redirs];
        sarg = (const USHORT *) &res.nm_space[result->h.bj_arg]; /* Did mean res!! */
        senv = (const Envir *) &res.nm_space[result->h.bj_env];
        sred = (const Redir *) &res.nm_space[result->h.bj_redirs];

        for  (cnt = 0;  cnt < result->h.bj_nargs;  cnt++)  {
                *darg++ = ntohs(*sarg);
                sarg++; /* Not falling for ntohs being a macro!!! */
        }
        for  (cnt = 0;  cnt < result->h.bj_nenv;  cnt++)  {
                denv->e_name = ntohs(senv->e_name);
                denv->e_value = ntohs(senv->e_value);
                denv++;
                senv++;
        }
        for  (cnt = 0;  cnt < result->h.bj_nredirs; cnt++)  {
                dred->arg = ntohs(sred->arg);
                dred++;
                sred++;
        }
#endif
        return  XB_OK;
}

int     gbatch_jobread(const int fd, const unsigned flags, const slotno_t slotno, apiBtjob *result)
{
        int             ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_JOBREAD;
        msg.un.reader.flags = htonl(flags);
        msg.un.reader.seq = htonl(fdp->jserial);
        msg.un.reader.slotno = htonl(slotno);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.un.r_reader.seq != 0)
                fdp->jserial = ntohl(msg.un.r_reader.seq);
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);

        /* The message is followed by the job details, held in
           jobnetmsg format. We do it that way as we need to hold
           variable refs by slot number etc */

        return  job_readrest(fdp, result);
}

int     gbatch_jobfindslot(const int fd, const unsigned flags, const jobno_t jn, const netid_t nid, slotno_t *slot)
{
        int             ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_FINDJOBSLOT;
        msg.un.jobfind.flags = htonl(flags);
        msg.un.jobfind.netid = nid;
        msg.un.jobfind.jobno = htonl(jn);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.un.r_find.seq != 0)
                fdp->jserial = ntohl(msg.un.r_find.seq);
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        if  (slot)
                *slot = ntohl(msg.un.r_find.slotno);
        return  XB_OK;
}

int     gbatch_jobfind(const int fd, const unsigned flags, const jobno_t jn, const netid_t nid, slotno_t *slot, apiBtjob *jp)
{
        int             ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;

        if  (!fdp)
                return  GBATCH_INVALID_FD;
        msg.code = API_FINDJOB;
        msg.un.jobfind.flags = htonl(flags);
        msg.un.jobfind.netid = nid;
        msg.un.jobfind.jobno = htonl(jn);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.un.r_find.seq != 0)
                fdp->jserial = ntohl(msg.un.r_find.seq);
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        if  (slot)
                *slot = ntohl(msg.un.r_find.slotno);
        return  job_readrest(fdp, jp);
}
