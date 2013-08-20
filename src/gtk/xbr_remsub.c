/* xbr_remsub.c -- remote host submission for gbch-xr

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
#include <gtk/gtk.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
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
#include "xbr_ext.h"
#include "gtk_lib.h"
#include "remsubops.h"
#include "stringvec.h"
#include "files.h"

#define RTIMEOUT        5
static  int   tcpportnum = -1;

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

/* Get ourselves an out socket to the server on the remote machine or
   return a suitable error code */

static int  remgoutfile(const netid_t hostid, CBtjobRef jb, int *rsock)
{
        int     sock, ret, msgsize;
        struct  nijobmsg        outmsg;

        if  (tcpportnum < 0  &&  (ret = remsub_inittcp(&tcpportnum)) != 0)
                return  ret;

        if  ((ret = remsub_opentcp(hostid, tcpportnum, &sock)) != 0)
                return  ret;

        msgsize = remsub_packjob(&outmsg, jb);
        remsub_condasses(&outmsg, jb, hostid);

        if  ((ret = remsub_startjob(sock, msgsize, prin_uname(Realuid), prin_gname(Realgid))) != 0)  {
                close(sock);
                return  ret;
        }
        if  (!remsub_sock_write(sock, (char *) &outmsg, (int) msgsize))  {
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

        if  (!remsub_sock_read(sock, (char *) &result, sizeof(result)))
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

struct  stringvec  hlist;

static  char    *curr_host;

struct  remparams  {            /* Info from remote */
        netid_t nid;            /* Network id */
        USHORT  umsk;           /* Umask value */
        ULONG   ulmt;           /* Ulimit value */
        char    realgname[UIDSIZE+1];   /* Group name */
        Btuser  uperms;         /* User permissions */
};

struct  remparams  curr_host_params;

/* Get all the information for the specified host.
   Return 0 if OK otherwise the error code. */

static  int     get_host_data(const char *hostname, struct remparams *rp)
{
        netid_t  nid;
        int     udpsock, ret;
        struct  sockaddr_in     serv_addr;
        struct  ni_jobhdr       enq;
        struct  ua_reply        resp1;
        struct  ua_umlreply     resp2;

        if  ((nid = look_int_hostname(hostname)) == -1)
                return  $EH{Invalid host name};

        if  ((ret = remsub_initsock(&udpsock, nid, &serv_addr)) != 0)
                return  ret;

        BLOCK_ZERO(&enq, sizeof(enq));
        enq.code = CL_SV_UENQUIRY;
        enq.joblength = htons(sizeof(enq));
        strncpy(enq.uname, realuname, UIDSIZE);

        /* Get group name and permissions */

        if  ((ret = udp_enquire(udpsock, &serv_addr, (char *) &enq, sizeof(enq), (char *) &resp1, sizeof(resp1))) != 0)  {
                close(udpsock);
                return  ret;
        }
        strcpy(rp->realgname, resp1.ua_gname);
        remsub_unpack_btuser(&rp->uperms, &resp1.ua_perm);
        if  (!rp->uperms.btu_isvalid)  {
                close(udpsock);
                return  $EH{No such user on remote};
        }

        /* Get umask and limit */

        enq.code = CL_SV_UMLPARS;
        enq.joblength = htons(sizeof(enq));
        if  ((ret = udp_enquire(udpsock, &serv_addr, (char *) &enq, sizeof(enq), (char *) &resp2, sizeof(resp2))) != 0)  {
                close(udpsock);
                return  ret;
        }

        rp->umsk = ntohs(resp2.ua_umask);
        rp->ulmt = ntohl(resp2.ua_ulimit);
        rp->nid = nid;
        close(udpsock);
        return  0;
}

/* Invoked on startup to set the current host name.
   Also look up parameters and silently delete it if we can't see it. */

void    set_def_host(char *h)
{
        if  (get_host_data(h, &curr_host_params) == 0)  {
                curr_host = h;
                stringvec_insert_unique(&hlist, h);
        }
        else
                free(h);                /* Needed deallocating */
}

/* Read host file for likely people to speak to (called on startup) */

void  init_hosts_known(char *hl)
{
        struct  remote  *rp;

        if  (hl)
                stringvec_split_sorted(&hlist, hl, ',');
        else
                stringvec_init(&hlist);

        while  ((rp = get_hostfile()))  {
                if  (rp->ht_flags & (HT_DOS|HT_ROAMUSER))
                        continue;
                if  (rp->hostname[0])
                        stringvec_insert_unique(&hlist, rp->hostname);
                if  (rp->alias[0])
                        stringvec_insert_unique(&hlist, rp->alias);
        }
        end_hostfile();
}

/* Get list of hosts as comma-separated list */

char    *list_hosts_known()
{
        return  stringvec_join(&hlist, ',');
}

char    *get_def_host()
{
        return  curr_host;
}

void  cb_remsubmit()
{
        struct  pend_job        *pj;
        GtkWidget               *dlg, *hostw;
        int                    cnt, sel;
        gint                    dlgres;
        FILE                    *inf = (FILE *) 0;
        int                     outsock = 0;

        if  (!(pj = sub_check()))
                return;

        if  (!pj->jobscript  &&   !(inf = ldsv_open('r', pj->directory, pj->jobfile_name)))  {
                doerror($EH{xmbtr cannot open job file});
                return;
        }

        /* Set up dialog and initialise the entry with the last host used if possible */

        dlg = gprompt_dialog(toplevel, $P{xbtr remote host dlgtit});
        hostw = gtk_combo_box_text_new_with_entry();
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hostw, FALSE, FALSE, DEF_DLG_VPAD);

        sel = -1;       /* Look for current entry */

        for  (cnt = 0;  cnt < stringvec_count(hlist);  cnt++)  {
                char    *ent = stringvec_nth(hlist, cnt);
                if  (curr_host  &&  strcmp(ent, curr_host) == 0)
                        sel = cnt;
                gtk_combo_box_append_text(GTK_COMBO_BOX(hostw), ent);
        }
        if  (sel >= 0)
                gtk_combo_box_set_active(GTK_COMBO_BOX(hostw), sel);

        gtk_widget_show_all(dlg);

        /* Get required host from dialog */

        while  ((dlgres = gtk_dialog_run(GTK_DIALOG(dlg))) == GTK_RESPONSE_OK)  {
                char    *shost = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(hostw));
                int     ec;
                struct  remparams       h_parms;

                /* If it's the same one as we had last time, we can drop out */

                if  (curr_host  &&  strcmp(shost, curr_host) == 0)  {
                        g_free(shost);
                        break;
                }

                if  ((ec = get_host_data(shost, &h_parms)))  {
                        g_free(shost);
                        doerror(ec);
                        continue;
                }

                if  (curr_host)
                        free(curr_host);

                stringvec_insert_unique(&hlist, shost);

                /* Possibly this is unnecessary but the result of gtk_combo_box_get_active_text has to be
                   freed with g_free, it says which might not be the same as ordinary free */

                curr_host = stracpy(shost);
                g_free(shost);
                curr_host_params = h_parms;
                break;
        }

        gtk_widget_destroy(dlg);

        /* Drop out if user cancelled it */

        if  (dlgres != GTK_RESPONSE_OK)  {
                if  (inf)
                        pclose(inf);
                return;
        }

        if  ((cnt = remgoutfile(curr_host_params.nid, pj->job, &outsock)) != 0)  {
                doerror(cnt);
                close(outsock);
                if  (inf)
                        fclose(inf);
                return;
        }

        if  (inf)  {
                remsub_copyout(inf, outsock, realuname, curr_host_params.realgname);
                pclose(inf);
        }
        else
                remsub_copyout_str(pj->jobscript, outsock, realuname, curr_host_params.realgname);

        if  ((cnt = remprocreply(outsock, pj->job)) != 0)  {
                disp_arg[1] = curr_host_params.nid;
                doerror(cnt);
                close(outsock);
                return;
        }
        close(outsock);
        if  (pj->Verbose)  {
                disp_arg[0] = pj->job->h.bj_job;
                disp_arg[1] = curr_host_params.nid;
                disp_str = title_of(pj->job);
                doinfo(disp_str[0]? $E{xmbtr remote job created ok title}: $E{xmbtr remote job created ok no title});
        }
        pj->nosubmit = 0;
}
