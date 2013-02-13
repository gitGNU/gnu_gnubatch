/* gbatch_jobswap.c -- API function to do [hn]to[nh][sl]-ing

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
#include <errno.h>
#include "gbatch.h"
#include "xbapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "netmsg.h"

extern void  gbatch_mode_pack(Btmode *, const Btmode *);

unsigned  gbatch_jobswap(struct jobnetmsg *dest, const apiBtjob *src)
{
        unsigned        hwm, cnt;
#ifndef WORDS_BIGENDIAN
        USHORT  *darg;  Envir   *denv;  Redir   *dred;
        const   USHORT *sarg;   const   Envir   *senv;  const   Redir   *sred;
#endif
        Jncond          *drc;
        Jnass           *dra;

        BLOCK_ZERO(&dest->hdr, sizeof(struct jobhnetmsg));

        dest->hdr.jid.hostid = src->h.bj_hostid;
        dest->hdr.jid.slotno = htonl(src->h.bj_slotno);

        dest->hdr.nm_progress   = src->h.bj_progress;
        dest->hdr.nm_pri        = src->h.bj_pri;
        dest->hdr.nm_jflags     = src->h.bj_jflags;
        dest->hdr.nm_istime     = src->h.bj_times.tc_istime;
        dest->hdr.nm_mday       = src->h.bj_times.tc_mday;
        dest->hdr.nm_repeat     = src->h.bj_times.tc_repeat;
        dest->hdr.nm_nposs      = src->h.bj_times.tc_nposs;

        dest->hdr.nm_ll         = htons(src->h.bj_ll);
        dest->hdr.nm_umask      = htons(src->h.bj_umask);
        dest->hdr.nm_nvaldays   = htons(src->h.bj_times.tc_nvaldays);
        dest->hdr.nm_autoksig   = htons(src->h.bj_autoksig);
        dest->hdr.nm_runon      = htons(src->h.bj_runon);
        dest->hdr.nm_deltime    = htons(src->h.bj_deltime);

        dest->hdr.nm_job        = htonl(src->h.bj_job);
        dest->hdr.nm_time       = htonl((LONG) src->h.bj_time);
        dest->hdr.nm_stime      = htonl((LONG) src->h.bj_stime);
        dest->hdr.nm_etime      = htonl((LONG) src->h.bj_etime);
        dest->hdr.nm_pid        = htonl(src->h.bj_pid);
        dest->hdr.nm_orighostid = src->h.bj_orighostid;
        dest->hdr.nm_runhostid  = src->h.bj_runhostid;
        dest->hdr.nm_ulimit     = htonl(src->h.bj_ulimit);
        dest->hdr.nm_nexttime   = htonl((LONG) src->h.bj_times.tc_nexttime);
        dest->hdr.nm_rate       = htonl(src->h.bj_times.tc_rate);
        dest->hdr.nm_runtime    = htonl(src->h.bj_runtime);

        strcpy(dest->hdr.nm_cmdinterp, src->h.bj_cmdinterp);
        dest->hdr.nm_exits      = src->h.bj_exits;
        gbatch_mode_pack(&dest->hdr.nm_mode, &src->h.bj_mode);

        drc = dest->hdr.nm_conds;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                const   apiJcond        *cr = &src->h.bj_conds[cnt];
                if  (cr->bjc_compar == C_UNUSED)
                        continue;
                drc->bjnc_compar = cr->bjc_compar;
                drc->bjnc_iscrit = cr->bjc_iscrit;
                if  ((drc->bjnc_type = cr->bjc_value.const_type) == CON_STRING)
                        strncpy(drc->bjnc_un.bjnc_string, cr->bjc_value.con_un.con_string, BTC_VALUE);
                else
                        drc->bjnc_un.bjnc_long = htonl(cr->bjc_value.con_un.con_long);
                drc->bjnc_var.slotno = htonl(cr->bjc_var.slotno);
                drc++;
        }

        dra = dest->hdr.nm_asses;
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                const   apiJass *cr = &src->h.bj_asses[cnt];
                if  (cr->bja_op == BJA_NONE)
                        continue;
                dra->bjna_flags = htons(cr->bja_flags);
                dra->bjna_op = cr->bja_op;
                dra->bjna_iscrit = cr->bja_iscrit;
                if  ((dra->bjna_type = cr->bja_con.const_type) == CON_STRING)
                        strncpy(dra->bjna_un.bjna_string, cr->bja_con.con_un.con_string, BTC_VALUE);
                else
                        dra->bjna_un.bjna_long = htonl(cr->bja_con.con_un.con_long);
                dra->bjna_var.slotno = htonl(cr->bja_var.slotno);
                dra++;
        }
        dest->nm_nredirs        = htons(src->h.bj_nredirs);
        dest->nm_nargs          = htons(src->h.bj_nargs);
        dest->nm_nenv           = htons(src->h.bj_nenv);
        dest->nm_title          = htons(src->h.bj_title);
        dest->nm_direct         = htons(src->h.bj_direct);
        dest->nm_redirs         = htons(src->h.bj_redirs);
        dest->nm_env            = htons(src->h.bj_env);
        dest->nm_arg            = htons(src->h.bj_arg);
        BLOCK_COPY(dest->nm_space, src->bj_space, JOBSPACE);

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
        hwm += sizeof(struct jobnetmsg) - JOBSPACE;
        dest->hdr.hdr.length = htons((USHORT) hwm);

#ifndef WORDS_BIGENDIAN

        /* If we are byte-swapping we must swap the argument,
           environment and redirection variable pointers and the
           arg field in each redirection.  */

        darg = (USHORT *) &dest->nm_space[src->h.bj_arg]; /* I did mean src there */
        denv = (Envir *) &dest->nm_space[src->h.bj_env];        /* and there */
        dred = (Redir *) &dest->nm_space[src->h.bj_redirs]; /* and there */
        sarg = (const USHORT *) &src->bj_space[src->h.bj_arg];
        senv = (const Envir *) &src->bj_space[src->h.bj_env];
        sred = (const Redir *) &src->bj_space[src->h.bj_redirs];

        for  (cnt = 0;  cnt < src->h.bj_nargs;  cnt++)  {
                *darg++ = htons(*sarg);
                sarg++; /* Not falling for htons being a macro!!! */
        }
        for  (cnt = 0;  cnt < src->h.bj_nenv;  cnt++)  {
                denv->e_name = htons(senv->e_name);
                denv->e_value = htons(senv->e_value);
                denv++;
                senv++;
        }
        for  (cnt = 0;  cnt < src->h.bj_nredirs; cnt++)  {
                dred->arg = htons(sred->arg);
                dred++;
                sred++;
        }
#endif
        return  hwm;
}
