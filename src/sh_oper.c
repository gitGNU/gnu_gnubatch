/* sh_oper.c -- scheduler operator interaction

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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/stat.h>
#include "incl_sig.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btconst.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "q_shm.h"
#include "sh_ext.h"

static  char    Filename[] = __FILE__;

#define OPINIT  5
#define OPINC   3
#define OPWAIT  15
#define OPTRIES 10

static  unsigned        numopers,
                        maxopers;

static  struct  opstr   {
        int_pid_t       pid;
        int_ugid_t      uid,
                        gid;
}  *oplist;

extern  PIDTYPE Xbns_pid;

/* Allocate a structure for an operator.  */

static void  makeop(const int_pid_t pid, const int_ugid_t uid, const int_ugid_t gid)
{
        if  (numopers >= maxopers)  {
                if  (maxopers)  {
                        maxopers += OPINC;
                        oplist = (struct opstr *) realloc((char *) oplist, maxopers*sizeof(struct opstr));
                }
                else  {
                        maxopers = OPINIT;
                        oplist = (struct opstr *) malloc(OPINIT * sizeof(struct opstr));
                }
                if  (oplist == (struct opstr *) 0)
                        ABORT_NOMEM;
        }

        oplist[numopers].pid = pid;
        oplist[numopers].uid = uid;
        oplist[numopers].gid = gid;
        numopers++;
}

/* Delete a structure for an operator.  */

static void  killop(const PIDTYPE pid)
{
        int     i;
        Repmess ipm;

        for  (i = 0;  i < numopers;  i++)
                if  (pid == oplist[i].pid)  {
                        /* Soak up pending messages */
                        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &ipm, sizeof(Shreq), (long) (pid + MTOFFSET), IPC_NOWAIT|MSG_NOERROR) >= 0)
                                ;
                        --numopers;
                        if  (i != numopers)
                                oplist[i] = oplist[numopers];
                        return;
                }
}

/* Find an operator by process id.  */

static int_pid_t  findop(const int_pid_t pid)
{
        int     i;

        for  (i = 0;  i < numopers;  i++)
                if  (oplist[i].pid == pid)
                        return  pid;
        return  0;
}

/* An operator has just signed on - note the fact.  */

int  addoper(ShreqRef rq)
{
        if  (findop(rq->upid) == 0)
                makeop(rq->upid, rq->uuid, rq->ugid);
        return  O_OK;
}

/* Tell operators except the one replied to */

void  tellopers()
{
        int     i = 0;

        qchanges = 0;
        if  (Xbns_pid > 0)
                kill(-Xbns_pid, QRFRESH);
redo:
        for  (;  i < numopers;  i++)  {

                if  (kill((PIDTYPE) oplist[i].pid, QRFRESH) < 0)  {

                        if  (errno == EPERM)
                                panic($E{Panic kill trouble});

                        /* He must have gone away (or setuid bug).  */

                        killop(oplist[i].pid);
                        goto  redo;
                }
        }
}

/* Delete an operator.  */

void  deloper(ShreqRef rq)
{
        if  (findop(rq->upid) == 0)
                return;
        killop(rq->upid);
}

/* Kill off operators.  */

void  killops()
{
        int     i;

        for  (i = 0;  i < numopers;  i++)
                kill((PIDTYPE) oplist[i].pid, SIGTERM);
}

/* Is given uid on list of operators */

int  islogged(uid_t uid)
{
        int     i;

        for  (i = 0;  i < numopers;  i++)
                if  (oplist[i].uid == uid)
                        return  1;
        return  0;
}

/* Check that the proposed mode isn't too restrictive.  Someone must
   be able to see it, and the same someone must be able either to
   delete it or change the mode.  */

int  checkminmode(BtmodeRef mp)
{
        if  (mp->u_flags & BTM_SHOW  &&  mp->u_flags & (BTM_DELETE|BTM_WRMODE))
                return  1;
        if  (mp->g_flags & BTM_SHOW  &&  mp->g_flags & (BTM_DELETE|BTM_WRMODE))
                return  1;
        if  (mp->o_flags & BTM_SHOW  &&  mp->o_flags & (BTM_DELETE|BTM_WRMODE))
                return  1;
        return  0;
}

/* See whether the given user can access the mode (which might be for
   a job or a variable).  */

int  shmpermitted(ShreqRef sr, BtmodeRef md, unsigned flag)
{
        BtuserRef       g;
        USHORT uf = md->u_flags;
        USHORT gf = md->g_flags;
        USHORT of = md->o_flags;

        if  ((md->o_uid == sr->uuid  &&  (uf & flag) == flag) ||
             (md->o_gid == sr->ugid  &&  (gf & flag) == flag) || (of & flag) == flag)
                return  1;

        g = getbtuentry(sr->uuid);

        if  (g->btu_priv & BTM_ORP_UG)  {
                uf |= gf;
                gf |= uf;
        }
        if  (g->btu_priv & BTM_ORP_UO)  {
                uf |= of;
                of |= uf;
        }
        if  (g->btu_priv & BTM_ORP_GO)  {
                gf |= of;
                of |= gf;
        }
        return  (md->o_uid == sr->uuid  &&  (uf & flag) == flag)  ||
                (md->o_gid == sr->ugid  &&  (gf & flag) == flag)  ||  (of & flag) == flag;
}

/* See if the given user can do the given thing.  */

int  ppermitted(uid_t uid, ULONG flag)
{
        BtuserRef  g = getbtuentry(uid);
        return  (g->btu_priv & flag) != 0;
}

/* Initialise default modes for given user variable */

void  initumode(uid_t uid, BtmodeRef mp)
{
        BtuserRef  g = getbtuentry(uid);
        mp->u_flags = g->btu_vflags[0];
        mp->g_flags = g->btu_vflags[1];
        mp->o_flags = g->btu_vflags[2];
}

/* `Force' a message out.  */

void  forcemsg(char *buf, int len)
{
        int     nn;

        if  (msgsnd(Ctrl_chan, (struct msgbuf *) buf, len, IPC_NOWAIT) >= 0)
                return;

        /* Q clogged up....
           See if each process has disappeared by poking it */

        tellopers();
        kill(child_pid, RESCHED);

        /* Try again n times */

        for  (nn = 0;  nn < MSGQ_BLOCKS;  nn++)  {
                sleep(MSGQ_BLOCKWAIT);
                if  (msgsnd(Ctrl_chan, (struct msgbuf *) buf, len, IPC_NOWAIT) >= 0)
                        return;
        }

        nfreport($E{Panic queue clogged up});
}

/* Send a reply to an operator via IPC */

void  sendreply(const int_pid_t pid, const unsigned code)
{
        Repmess ipm;

        /* Code == 0 covers the REP_AMPARENT case - don't reply */

        if  (pid == 0  ||  code == 0)
                return;
        ipm.outmsg.mcode = code & SHREQ_CODE;
        ipm.mm = MTOFFSET + pid;
        forcemsg((char *) &ipm, sizeof(Shreq));
        /* If we did this as a child process, exit now we've done our stuff */
        if  (code & SHREQ_CHILD)
                exit(0);
}
