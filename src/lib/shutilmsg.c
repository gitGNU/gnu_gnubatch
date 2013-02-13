/* shutilmsg.c -- error handling for shell programs

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
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "errnums.h"
#include "shreq.h"
#include "shutilmsg.h"

#ifdef  USING_MMAP
void  sync_xfermmap();
#endif

int     Ctrl_chan;                              /* Define this here */
long    mymtype;                                /* Define this here */

/* Send params job message to scheduler */

int  wjimsg_param(const unsigned code, const LONG param, CBtjobRef jp)
{
        Shipc   Oreq;
        int     blkcount = MSGQ_BLOCKS;

        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        Oreq.sh_params.mcode = code;
        Oreq.sh_params.param = param;
        Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
        Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)
                        return  $E{IPC msg q error};
                blkcount--;
                if  (blkcount <= 0)
                        return  $E{IPC msg q full};
                sleep(MSGQ_BLOCKWAIT);
        }
        return  0;
}

int  wjimsg(const unsigned code, CBtjobRef jp)
{
        return  wjimsg_param(code, 0L, jp);
}


/* Obtain send job message to scheduler using transfer buffer in *JREQ */

int  wjxfermsg(const unsigned code, const ULONG indx)
{
        Shipc           Oreq;
        int     blkcount = MSGQ_BLOCKS;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        Oreq.sh_params.mcode = code;
        Oreq.sh_un.sh_jobindex = indx;
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)
                        return  $E{IPC msg q error};
                blkcount--;
                if  (blkcount <= 0)
                        return  $E{IPC msg q full};
                sleep(MSGQ_BLOCKWAIT);
        }
        return  0;
}

/* Send var-type message to scheduler */

int  wvmsg(unsigned code, CBtvarRef vp, const ULONG seq)
{
        Shipc           Oreq;
        int     blkcount = MSGQ_BLOCKS;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        Oreq.sh_params.mcode = code;
        Oreq.sh_un.sh_var = *vp;
        Oreq.sh_un.sh_var.var_sequence = seq;
        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(Btvar), IPC_NOWAIT) < 0)  {
                if  (errno != EAGAIN)
                        return  $E{IPC msg q error};
                blkcount--;
                if  (blkcount <= 0)
                        return  $E{IPC msg q full};
                sleep(MSGQ_BLOCKWAIT);
        }
        return  0;
}

/* Display job-type error message */

int  dojerror(const unsigned retc, CBtjobRef jp)
{
        switch  (retc & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                return  $E{Unexpected sched message};
        case  JOB_REPLY:
                disp_str = title_of(jp);
                disp_arg[0] = jp->h.bj_job;
                return  (retc & ~REQ_TYPE) + $E{Base for scheduler job errors};
        case  NET_REPLY:
                disp_str = title_of(jp);
                disp_arg[0] = jp->h.bj_job;
                return  (retc & ~REQ_TYPE) + $E{Base for scheduler net errors};
        }
}

/* Display var-type error message */

int  doverror(unsigned retc, BtvarRef vp)
{
        switch  (retc & REQ_TYPE)  {
        default:
                disp_arg[0] = retc;
                return  $E{Unexpected sched message};
        case  VAR_REPLY:
                disp_str = vp->var_name;
                return  (int) ((retc & ~REQ_TYPE) + $E{Base for scheduler var errors});
        case  NET_REPLY:
                disp_str = vp->var_name;
                return  (int) ((retc & ~REQ_TYPE) + $E{Base for scheduler net errors});
        }
}
