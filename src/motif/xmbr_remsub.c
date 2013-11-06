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
#include "remsubops.h"
#include "files.h"

static  char    Filename[] = __FILE__;

static  int   tcpportnum = -1;

/***********************************************************************
        UDP Access routines
 ***********************************************************************/

#define RTIMEOUT        5

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

        if  ((ec = remsub_initsock(&udpsock, hlist[new_host].hid, &serv_addr)) != 0)  {
                doerror(jwid, ec);
                return  CANCELLED;
        }

        BLOCK_ZERO(&enq, sizeof(enq));
        enq.code = CL_SV_UENQUIRY;
        enq.joblength = htons(sizeof(enq));
        strncpy(enq.uname, prin_uname(Realuid), UIDSIZE);
        if  ((ec = udp_enquire(udpsock, &serv_addr, (char *) &enq, sizeof(enq), (char *) &resp1, sizeof(resp1))) != 0)  {
                close(udpsock);
                doerror(jwid, ec);
                return  CANCELLED;
        }
        strcpy(possn.realgname, resp1.ua_gname);
        remsub_unpack_btuser(&possn.uperms, &resp1.ua_perm);
        if  (!possn.uperms.btu_isvalid)  {
                close(udpsock);
                doerror(jwid, $EH{No such user on remote});
                return  CANCELLED;
        }
        enq.code = CL_SV_UMLPARS;
        enq.joblength = htons(sizeof(enq));
        if  ((ec = udp_enquire(udpsock, &serv_addr, (char *) &enq, sizeof(enq), (char *) &resp2, sizeof(resp2))) != 0)  {
                close(udpsock);
                doerror(jwid, ec);
                return  CANCELLED;
        }
        possn.umsk = ntohs(resp2.ua_umask);
        possn.ulmt = ntohl(resp2.ua_ulimit);
        close(udpsock);
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
        struct  pend_job        *pj;
        BtjobRef                jreq;
        FILE                    *inf = (FILE *) 0;
        int                     ec, indx, outsock = 0;
        struct  remparams       *hp;
        struct  remparams       h_parms;
        extern  char *gen_path(char *, char *);

        if  (!(pj = sub_check(w)))
                return;

        jreq = pj->job;

        if  (!pj->jobscript)  {
                char  *path = gen_path(pj->directory, pj->jobfile_name);
                SWAP_TO(Realuid);
                inf = fopen(path, "r");
                SWAP_TO(Daemuid);
                free(path);
                if  (!inf)  {
                        doerror(w, $EH{xmbtr cannot open job file});
                        return;
                }
        }

        if  (curr_host < 0)  {
                hp = &h_parms;
                if  ((indx = choosehost("whichhost", hp, 0, -1)) < 0)  {
                        if  (inf)
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
                if  (inf)
                        fclose(inf);
                return;
        }
        if  (inf)  {
                remsub_copyout(inf, outsock, prin_uname(Realuid), hp->realgname);
                fclose(inf);
        }
        else
                remsub_copyout_str(pj->jobscript, outsock, realuname, hp->realgname);

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
                doinfo(w, disp_str[0]? $E{xmbtr remote job created ok title}: $E{xmbtr remote job created ok no title});
        }
        pj->nosubmit = 0;
}
