/* sh_pack.c -- do ntoh/hton stuff for inter-btsched messages

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
#include <sys/types.h>
#include <sys/ipc.h>
#include "incl_unix.h"
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
#include "q_shm.h"
#include "sh_ext.h"

void  jid_pack(jident *dest, CBtjobhRef src)
{
        dest->hostid = int2ext_netid_t(src->bj_hostid);
        dest->slotno = htonl(src->bj_slotno);
}

void  jid_unpack(BtjobhRef dest, const jident *src)
{
        dest->bj_hostid = ext2int_netid_t(src->hostid);
        dest->bj_slotno = ntohl(src->slotno);
}

void  vid_pack(vident *dest, const Btvar *const src)
{
        dest->hostid = int2ext_netid_t(src->var_id.hostid);
        dest->slotno = htonl(src->var_id.slotno);
}

void  vid_unpack(BtvarRef dest, const vident *src)
{
        dest->var_id.hostid = ext2int_netid_t(src->hostid);
        dest->var_id.slotno = htonl(src->slotno);
}

vhash_t  vid_uplook(const vident *src)
{
        vident  hostorder;
        hostorder.hostid = ext2int_netid_t(src->hostid);
        hostorder.slotno = ntohl(src->slotno);
        return  findvar(&hostorder);
}

#ifdef  TRANSLATE_IN_PACK
static vhash_t  myversion(const vhash_t remvh, BtmodeRef jmode, const unsigned permflag)
{
        int     dumm = 0;       /* Dummy result arg passed to findvarbyname */
        vhash_t   retvh;
        Shreq   sr;

        sr.hostid = 0;          /* Must set this! */
        sr.uuid = jmode->o_uid; /* Set these for "visible" call in findvarbyname */
        sr.ugid = jmode->o_gid; /* and following "shmpermitted" */
        if  ((retvh = findvarbyname(&sr, &Var_seg.vlist[remvh].Vent, &dumm)) < 0)
                return  -1;     /* Not found */
        if  (Var_seg.vlist[retvh].Vent.var_type != 0) /* No fiddling with sys vars */
                return  -2;
        return  shmpermitted(&sr, &Var_seg.vlist[retvh].Vent.var_mode, permflag)? retvh: -2;
}
#endif

void  mode_pack(BtmodeRef dest, CBtmodeRef src)
{
        strcpy(dest->o_user, src->o_user);
        strcpy(dest->c_user, src->c_user);
        strcpy(dest->o_group, src->o_group);
        strcpy(dest->c_group, src->c_group);
        dest->u_flags = htons(src->u_flags);
        dest->g_flags = htons(src->g_flags);
        dest->o_flags = htons(src->o_flags);
}

void  mode_unpack(BtmodeRef dest, const Btmode *src)
{
        int_ugid_t      ugid;
        strcpy(dest->o_user, src->o_user);
        strcpy(dest->c_user, src->c_user);
        strcpy(dest->o_group, src->o_group);
        strcpy(dest->c_group, src->c_group);
        dest->o_uid = (ugid = lookup_uname(dest->o_user)) == UNKNOWN_UID? Daemuid: ugid;
        dest->c_uid = (ugid = lookup_uname(dest->c_user)) == UNKNOWN_UID? Daemuid: ugid;
        dest->o_gid = (ugid = lookup_gname(dest->o_group)) == UNKNOWN_GID? Daemgid: ugid;
        dest->c_gid = (ugid = lookup_gname(dest->c_group)) == UNKNOWN_GID? Daemgid: ugid;
        dest->u_flags = ntohs(src->u_flags);
        dest->g_flags = ntohs(src->g_flags);
        dest->o_flags = ntohs(src->o_flags);
}

void  jobh_pack(struct jobhnetmsg *dest, CBtjobhRef src)
{
        unsigned        cnt;
        Jncond          *drc;
        Jnass           *dra;

        BLOCK_ZERO(dest, sizeof(struct jobhnetmsg));

        /* caller sets code and maybe length if this is part of something else */
        dest->hdr.length = htons(sizeof(struct jobhnetmsg));
        jid_pack(&dest->jid, src);

        dest->nm_progress       = src->bj_progress;
        dest->nm_pri            = src->bj_pri;
        dest->nm_jflags         = src->bj_jflags;
        dest->nm_istime         = src->bj_times.tc_istime;
        dest->nm_mday           = src->bj_times.tc_mday;
        dest->nm_repeat         = src->bj_times.tc_repeat;
        dest->nm_nposs          = src->bj_times.tc_nposs;

        dest->nm_ll             = htons(src->bj_ll);
        dest->nm_umask          = htons(src->bj_umask);
        dest->nm_nvaldays       = htons(src->bj_times.tc_nvaldays);
        dest->nm_autoksig       = htons(src->bj_autoksig);
        dest->nm_runon          = htons(src->bj_runon);
        dest->nm_deltime        = htons(src->bj_deltime);
        dest->nm_lastexit       = htons(src->bj_lastexit);

        dest->nm_job            = htonl(src->bj_job);
        dest->nm_time           = htonl((LONG) src->bj_time);
        dest->nm_stime          = htonl((LONG) src->bj_stime);
        dest->nm_etime          = htonl((LONG) src->bj_etime);
        dest->nm_pid            = htonl(src->bj_pid);
        dest->nm_orighostid     = int2ext_netid_t(src->bj_orighostid);
        dest->nm_runhostid      = int2ext_netid_t(src->bj_runhostid);
        dest->nm_ulimit         = htonl(src->bj_ulimit);
        dest->nm_nexttime       = htonl((LONG) src->bj_times.tc_nexttime);
        dest->nm_rate           = htonl(src->bj_times.tc_rate);

        dest->nm_runtime        = htonl(src->bj_runtime);
        dest->nm_exits          = src->bj_exits;

        strcpy(dest->nm_cmdinterp, src->bj_cmdinterp);
        mode_pack(&dest->nm_mode, &src->bj_mode);

        drc = dest->nm_conds;
        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                CJcondRef  cr = &src->bj_conds[cnt];
                const  Btvar    *vp;
                if  (cr->bjc_compar == C_UNUSED)
                        break;
                drc->bjnc_compar = cr->bjc_compar;
                drc->bjnc_iscrit = cr->bjc_iscrit & ~(CCRIT_NONAVAIL|CCRIT_NOPERM);
                if  ((drc->bjnc_type = cr->bjc_value.const_type) == CON_STRING)
                        strncpy(drc->bjnc_un.bjnc_string, cr->bjc_value.con_un.con_string, BTC_VALUE);
                else
                        drc->bjnc_un.bjnc_long = htonl(cr->bjc_value.con_un.con_long);

                /* Bodge reference to machine name.  We deal with "use
                   local var" at the other end.  */

                vp = &Var_seg.vlist[cr->bjc_varind].Vent;
                if  (vp->var_id.hostid == 0  &&  vp->var_id.slotno == machname_slot)  {
                        drc->bjnc_var.hostid = 0;
                        drc->bjnc_var.slotno = htonl(MACH_SLOT);
                }
                else
                        vid_pack(&drc->bjnc_var, vp);
                drc++;
        }

        dra = dest->nm_asses;
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                CJassRef  cr = &src->bj_asses[cnt];
                if  (cr->bja_op == BJA_NONE)
                        break;
                dra->bjna_flags = htons(cr->bja_flags);
                dra->bjna_op = cr->bja_op;
                dra->bjna_iscrit = cr->bja_iscrit & ~(ACRIT_NONAVAIL|ACRIT_NOPERM);
                if  ((dra->bjna_type = cr->bja_con.const_type) == CON_STRING)
                        strncpy(dra->bjna_un.bjna_string, cr->bja_con.con_un.con_string, BTC_VALUE);
                else
                        dra->bjna_un.bjna_long = htonl(cr->bja_con.con_un.con_long);
                vid_pack(&dra->bjna_var, &Var_seg.vlist[cr->bja_varind].Vent);
                dra++;
        }
}

void  jobh_unpack(BtjobhRef dest, const struct jobhnetmsg *src)
{
        unsigned        cnt;
        JcondRef        drc;
        JassRef         dra;

        BLOCK_ZERO(dest, sizeof(Btjobh));

        jid_unpack(dest, &src->jid);

        dest->bj_progress       = src->nm_progress;
        dest->bj_pri            = src->nm_pri;
        dest->bj_jflags = src->nm_jflags;
        dest->bj_times.tc_istime= src->nm_istime;
        dest->bj_times.tc_mday  = src->nm_mday;
        dest->bj_times.tc_repeat= src->nm_repeat;
        dest->bj_times.tc_nposs = src->nm_nposs;

        dest->bj_ll             = ntohs(src->nm_ll);
        dest->bj_umask          = ntohs(src->nm_umask);

        dest->bj_times.tc_nvaldays= ntohs(src->nm_nvaldays);
        dest->bj_autoksig       = ntohs(src->nm_autoksig);
        dest->bj_runon          = ntohs(src->nm_runon);
        dest->bj_deltime        = ntohs(src->nm_deltime);
        dest->bj_lastexit       = htons(src->nm_lastexit);

        dest->bj_job            = ntohl(src->nm_job);
        dest->bj_time           = (time_t) ntohl(src->nm_time);
        dest->bj_stime          = (time_t) ntohl(src->nm_stime);
        dest->bj_etime          = (time_t) ntohl(src->nm_etime);
        dest->bj_pid            = ntohl(src->nm_pid);
        dest->bj_orighostid     = ext2int_netid_t(src->nm_orighostid);
        dest->bj_runhostid      = ext2int_netid_t(src->nm_runhostid);
        dest->bj_ulimit         = ntohl(src->nm_ulimit);
        dest->bj_times.tc_nexttime= (time_t) ntohl(src->nm_nexttime);
        dest->bj_times.tc_rate  = ntohl(src->nm_rate);
        dest->bj_runtime        = ntohl(src->nm_runtime);

        dest->bj_exits          = src->nm_exits;
        strcpy(dest->bj_cmdinterp, src->nm_cmdinterp);

        mode_unpack(&dest->bj_mode, &src->nm_mode);

        drc = dest->bj_conds;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                const Jncond    *cr = &src->nm_conds[cnt];

                if  (cr->bjnc_compar == C_UNUSED)
                        break;

                /* Host id of zero in vident means a special, "the equivalent on this machine", but this is only
                   done this way for MACHINE.  */

                if  (cr->bjnc_var.hostid == 0)  {
                        if  ((LONG) ntohl(cr->bjnc_var.slotno) != MACH_SLOT)
                                continue;
                        drc->bjc_varind = machname_slot;
                }
                else  if  ((drc->bjc_varind = vid_uplook(&cr->bjnc_var)) < 0)  {
                        /* Might have this situation if the variable concerned
                           is on another remote machine we don't know about yet */
                        dest->bj_jrunflags |= BJ_SKELHOLD;
                        continue;
                }

                /* Moved after above to avoid bogus values */
                drc->bjc_compar = cr->bjnc_compar;
                drc->bjc_iscrit = cr->bjnc_iscrit;

                if  ((drc->bjc_value.const_type = cr->bjnc_type) == CON_STRING)
                        strncpy(drc->bjc_value.con_un.con_string, cr->bjnc_un.bjnc_string, BTC_VALUE);
                else
                        drc->bjc_value.con_un.con_long = ntohl(cr->bjnc_un.bjnc_long);
                drc++;
        }

        dra = dest->bj_asses;

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                const Jnass     *cr = &src->nm_asses[cnt];
                if  (cr->bjna_op == BJA_NONE)
                        break;
                if  ((dra->bja_varind = vid_uplook(&cr->bjna_var)) < 0)  {
                        /* Might have this situation if the variable concerned
                           is on another remote machine we don't know about yet */
                        dest->bj_jrunflags |= BJ_SKELHOLD;
                        continue;
                }
                dra->bja_flags = ntohs(cr->bjna_flags);
                dra->bja_op = cr->bjna_op;
                dra->bja_iscrit = cr->bjna_iscrit;
                if  ((dra->bja_con.const_type = cr->bjna_type) == CON_STRING)
                        strncpy(dra->bja_con.con_un.con_string, cr->bjna_un.bjna_string, BTC_VALUE);
                else
                        dra->bja_con.con_un.con_long = ntohl(cr->bjna_un.bjna_long);
                dra++;
        }
}

unsigned  job_pack(struct jobnetmsg *dest, CBtjobRef src)
{
        unsigned  hwm;          /* High water mark in jobspace to minimise message length */
#ifndef WORDS_BIGENDIAN
        unsigned  cnt;
        JargRef darg;   EnvirRef denv;  RedirRef dred;
        const   Jarg    *sarg;  const   Envir   *senv;  const   Redir   *sred;
#endif

        jobh_pack(&dest->hdr, &src->h);
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

        darg = (JargRef) &dest->nm_space[src->h.bj_arg]; /* I did mean src there */
        denv = (EnvirRef) &dest->nm_space[src->h.bj_env];       /* and there */
        dred = (RedirRef) &dest->nm_space[src->h.bj_redirs]; /* and there */
        sarg = (const Jarg *) &src->bj_space[src->h.bj_arg];
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

void  job_unpack(BtjobRef dest, const struct jobnetmsg *src)
{
        unsigned  hwm = ntohs(src->hdr.hdr.length) - sizeof(struct jobhnetmsg);
#ifndef WORDS_BIGENDIAN
        unsigned  cnt;
        JargRef darg;   EnvirRef denv;  RedirRef dred;
        const   Jarg  *sarg;    const   Envir *senv;    const   Redir *sred;
#endif
        jobh_unpack(&dest->h, &src->hdr);

        dest->h.bj_nredirs      = ntohs(src->nm_nredirs);
        dest->h.bj_nargs        = ntohs(src->nm_nargs);
        dest->h.bj_nenv         = ntohs(src->nm_nenv);
        dest->h.bj_title        = ntohs(src->nm_title);
        dest->h.bj_direct       = ntohs(src->nm_direct);
        dest->h.bj_redirs       = ntohs(src->nm_redirs);
        dest->h.bj_env          = ntohs(src->nm_env);
        dest->h.bj_arg          = ntohs(src->nm_arg);

        BLOCK_COPY(dest->bj_space, src->nm_space, hwm > JOBSPACE? JOBSPACE: hwm);

#ifndef WORDS_BIGENDIAN

        /* If we are byte-swapping we must swap the argument,
           environment and redirection variable pointers and the
           arg field in each redirection.  */

        darg = (JargRef) &dest->bj_space[dest->h.bj_arg];
        denv = (EnvirRef) &dest->bj_space[dest->h.bj_env];
        dred = (RedirRef) &dest->bj_space[dest->h.bj_redirs];
        sarg = (const Jarg *) &src->nm_space[dest->h.bj_arg]; /* Did mean dest!! */
        senv = (const Envir *) &src->nm_space[dest->h.bj_env];
        sred = (const Redir *) &src->nm_space[dest->h.bj_redirs];

        for  (cnt = 0;  cnt < dest->h.bj_nargs;  cnt++)  {
                *darg++ = ntohs(*sarg);
                sarg++; /* Not falling for ntohs being a macro!!! */
        }
        for  (cnt = 0;  cnt < dest->h.bj_nenv;  cnt++)  {
                denv->e_name = ntohs(senv->e_name);
                denv->e_value = ntohs(senv->e_value);
                denv++;
                senv++;
        }
        for  (cnt = 0;  cnt < dest->h.bj_nredirs; cnt++)  {
                dred->arg = ntohs(sred->arg);
                dred++;
                sred++;
        }
#endif
}

void  var_pack(struct varnetmsg *dest, CBtvarRef src)
{
        BLOCK_ZERO(dest, sizeof(struct varnetmsg));
        dest->hdr.length = htons(sizeof(struct varnetmsg));
        vid_pack(&dest->vid, src);
        dest->nm_type = (unsigned char) src->var_type;
        dest->nm_flags = (unsigned char) src->var_flags;
        strncpy(dest->nm_name, src->var_name, BTV_NAME);
        strncpy(dest->nm_comment, src->var_comment, BTV_COMMENT);
        mode_pack(&dest->nm_mode, &src->var_mode);
        if  ((dest->nm_consttype = (unsigned char) src->var_value.const_type) == CON_STRING)
                strncpy(dest->nm_un.nm_string, src->var_value.con_un.con_string, BTC_VALUE);
        else
                dest->nm_un.nm_long = htonl(src->var_value.con_un.con_long);
}

void  var_unpack(BtvarRef dest, const struct varnetmsg *src)
{
        BLOCK_ZERO(dest, sizeof(Btvar));
        vid_unpack(dest, &src->vid);
        dest->var_type = src->nm_type;
        dest->var_flags = src->nm_flags;
        strncpy(dest->var_name, src->nm_name, BTV_NAME);
        strncpy(dest->var_comment, src->nm_comment, BTV_COMMENT);
        mode_unpack(&dest->var_mode, &src->nm_mode);
        if  ((dest->var_value.const_type = src->nm_consttype) == CON_STRING)
                strncpy(dest->var_value.con_un.con_string, src->nm_un.nm_string, BTC_VALUE);
        else
                dest->var_value.con_un.con_long = ntohl(src->nm_un.nm_long);
}
