/* rbt_job.c -- Pack up and initialise TCP jobs.

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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "files.h"
#include "btrvar.h"
#include "remsubops.h"

static  int     tcpportnum = -1;

extern  uid_t   Repluid;                /* Replacement if requested */
extern  gid_t   Replgid;                /* Replacement if requested */

extern  netid_t Out_host;
extern  char    *Out_interp, *realuname;
extern  char    realgname[];
extern  char    *repluname, *replgname;

int  packjob(struct nijobmsg *dest, CBtjobRef src, const netid_t nhostid)
{
        unsigned        hwm = remsub_packjob(dest, src);
        int             cnt;

        strncpy(dest->ni_hdr.ni_cmdinterp, Out_interp && Out_interp[0]? Out_interp: DEF_CI_NAME, CI_MAXNAME);

        for  (cnt = 0;  cnt < Condcnt;  cnt++)  {
                Nicond  *nic = &dest->ni_hdr.ni_conds[cnt];
                struct  scond   *sic = &Condlist[cnt];
                nic->nic_var.ni_varhost = int2ext_netid_t(sic->vd.hostid);
                /* FIXME - refs to MACHINE */
                strncpy(nic->nic_var.ni_varname, sic->vd.var, BTV_NAME);
                nic->nic_compar = (unsigned char) sic->compar;
                nic->nic_iscrit = sic->vd.crit;
                nic->nic_type = (unsigned char) sic->value.const_type;
                if  (nic->nic_type == CON_STRING)
                        strncpy(nic->nic_un.nic_string, sic->value.con_un.con_string, BTC_VALUE);
                else
                        nic->nic_un.nic_long = htonl(sic->value.con_un.con_long);
        }
        for  (cnt = 0;  cnt < Asscnt;  cnt++)  {
                Niass   *nia = &dest->ni_hdr.ni_asses[cnt];
                struct  Sass    *sia = &Asslist[cnt];
                nia->nia_var.ni_varhost = int2ext_netid_t(sia->vd.hostid);
                strncpy(nia->nia_var.ni_varname, sia->vd.var, BTV_NAME);
                nia->nia_flags = htons(sia->flags);
                nia->nia_op = (unsigned char) sia->op;
                nia->nia_iscrit = sia->vd.crit;
                nia->nia_type = (unsigned char) sia->con.const_type;
                if  (nia->nia_type == CON_STRING)
                        strncpy(nia->nia_un.nia_string, sia->con.con_un.con_string, BTC_VALUE);
                else
                        nia->nia_un.nia_long = htonl(sia->con.con_un.con_long);
        }

        return  hwm;
}

int  remgoutfile(const netid_t hostid, CBtjobRef jb)
{
        int  sock, ret, msgsize;
        struct  nijobmsg        outmsg;

        if  (tcpportnum < 0  &&  (ret = remsub_inittcp(&tcpportnum)) != 0)  {
                print_error(ret);
                return  -1;
        }
        if  ((ret = remsub_opentcp(hostid, tcpportnum, &sock)) != 0) {
                print_error(ret);
                return  -1;
        }
        msgsize = packjob(&outmsg, jb, hostid);

        /* Write message header (with size) followed by job descr */
        if  ((ret = remsub_startjob(sock, msgsize, repluname, replgname)) != 0)  {
                print_error(ret);
                close(sock);
                return  -1;
        }
        if  (!remsub_sock_write(sock, (char *) &outmsg, (int) msgsize))  {
                print_error($E{Trouble with job});
                close(sock);
                return  -1;
        }
        return  sock;
}

LONG  remprocreply(const int sock)
{
        int     errcode;
        ULONG   which;
        struct  client_if       result;

        if  (!remsub_sock_read(sock, (char *) &result, sizeof(result)))  {
                print_error($E{Cant read status result});
                return  0;
        }

        errcode = result.code;
 redo:
        switch  (errcode)  {
        case  XBNQ_OK:
                return  ntohl(result.param);

        case  XBNR_BADCVAR:
                which = ntohl(result.param);
                if  (which >= MAXCVARS)  {
                        print_error($E{Bad condition result});
                        return  0;
                }
                disp_str = Condlist[which].vd.var;
                break;

        case  XBNR_BADAVAR:
                which = ntohl(result.param);
                if  (which >= MAXSEVARS)  {
                        print_error($E{Bad assignment result});
                        return  0;
                }
                disp_str = Asslist[which].vd.var;
                break;

        case  XBNR_BADCI:
                disp_str = Out_interp;

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
                print_error($E{Unknown queue result message});
                return  0;
        }
        disp_arg[1] = Out_host;
        print_error($E{Base for rbtr return errors}+result.code);
        return  0;
}
