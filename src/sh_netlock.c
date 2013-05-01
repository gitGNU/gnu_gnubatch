/* sh_netlock.c -- scheduler variable locking

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

/*
 * This module is exclusively concerned with coordinating network locking
 * around adjusting shared variables.
 */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include "defaults.h"
#include "files.h"
#include "incl_ugid.h"
#include "incl_unix.h"
#include "incl_net.h"
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
#include "ipcstuff.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "sh_ext.h"

#define SETTLETIME      60

#if (defined(OS_LINUX) || defined(OS_BSDI)) && !defined(_SEM_SEMUN_UNDEFINED)
#define my_semun        semun
#else

/* Define a union as defined in semctl(2) to pass the 4th argument of
   semctl. On most machines it is good enough to pass the value
   but on the SPARC and PYRAMID unions are passed differently
   even if all the possible values would fit in an int (as in
   this case).  */

union   my_semun        {
        int                     val;
        struct  semid_ds        *buf;
        ushort                  *array;
};
#endif

/* Semaphore members */

#define NLCK_LCK        0               /* Lock the whole semaphore */
#define NLCK_TYPE       1               /* Current lock type */
#define NLCK_XMITCNT    2               /* Count of people we've asked to lock */

#define NLT_NONE        0               /* Not locked */
#define NLT_SETTING     1               /* Locking in progress */
#define NLT_LOCLOCK     2               /* Local lock */
#define NLT_REMLOCK     3               /* Remote lock */
#define NLT_ERRREC      4               /* Error recovery */

#define IPC_MODE        0600

static  struct  sembuf  lockit  =       {  NLCK_LCK, -1, 0  },
                        unlockit=       {  NLCK_LCK,  1, 0  },
                        waitdone=       {  NLCK_XMITCNT, 0, 0      },
                        waitunlock=     {  NLCK_TYPE, 0, 0         },
                        replied =       {  NLCK_XMITCNT, -1, IPC_NOWAIT    };

static  int     netsem_chan;

static void  sem_op(struct sembuf *SB)
{
        for  (;;)  {
                if  (semop(netsem_chan, SB, 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                panic($E{Semaphore error probably undo});
        }
}

void  init_remotelock()
{
        union   my_semun        ms;
        ushort                  array[3];

        if  ((netsem_chan = semget(NETSEMID+envselect_value, 3, IPC_MODE|IPC_CREAT)) < 0)  {
                print_error($E{Panic cannot create network semaphore});
                do_exit(E_SETUP);
        }
        if  (Daemuid != ROOTID)  {
                struct  semid_ds  sb;
                ms.buf = &sb;
                if  (semctl(netsem_chan, 0, IPC_STAT, ms) < 0)  {
                        print_error($E{Panic cannot reset network semaphore});
                        do_exit(E_SETUP);
                }
                sb.sem_perm.uid = Daemuid;
                sb.sem_perm.gid = Daemgid;
                if  (semctl(netsem_chan, 0, IPC_SET, ms) < 0)  {
                        print_error($E{Panic cannot reset network semaphore});
                        do_exit(E_SETUP);
                }
        }

        array[NLCK_LCK] = 1;
        array[NLCK_TYPE] = NLT_NONE;
        array[NLCK_XMITCNT] = 0;
        ms.array = array;
        if  (semctl(netsem_chan, 0, SETALL, ms) < 0)  {
                print_error($E{Panic cannot reset network semaphore});
                do_exit(E_SETUP);
        }
}

void  end_remotelock()
{
        union   my_semun        ms;
        BLOCK_ZERO(&ms, sizeof(ms));
        semctl(netsem_chan, 0, IPC_RMID, ms);
}

void  get_remotelock()
{
        union   my_semun        ms;
        int     num_remhosts = get_nservers();

        ms.val = 0;

        /* If we haven't got any remotes to talk to, forget it.  */

        if  (num_remhosts == 0)
                return;

        for  (;;)  {

                /* Wait until lock is free */

                sem_op(&lockit);

                switch  (semctl(netsem_chan, NLCK_TYPE, GETVAL, ms))  {
                case  NLT_ERRREC:
                        /* Error recovery - wait for life to get quieter and then reset.  */
                        sem_op(&unlockit);
                        sleep(SETTLETIME);
                        sem_op(&lockit);
                        if  (semctl(netsem_chan, NLCK_TYPE, GETVAL, ms) == NLT_ERRREC)  {
                                ms.val = NLT_NONE;
                                semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                        }
                        sem_op(&unlockit);
                        continue;

                default:                /* Treat as unlocked */
                case  NLT_NONE:
                        ms.val = NLT_SETTING;
                        semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                        ms.val = num_remhosts;
                        semctl(netsem_chan, NLCK_XMITCNT, SETVAL, ms);
                        sem_op(&unlockit);
                        net_broadcast(N_WANTLOCK);
                waitrest:
                        sem_op(&waitdone);
                        continue;

                case  NLT_SETTING:
                        /* Setting in progress - probably by another task.
                           Just wait for the hosts to respond.
                           Someone will win!!  */

                        if  (semctl(netsem_chan, NLCK_XMITCNT, GETVAL, ms) != 0)  {
                                sem_op(&unlockit);
                                goto  waitrest;
                        }

                        /* All done - we call the thing locked */

                        ms.val = NLT_LOCLOCK;
                        semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                        sem_op(&unlockit);
                        return;

                case  NLT_LOCLOCK:
                case  NLT_REMLOCK:
                        /* Someone has beaten us to it.  Unlock the
                           semaphore itself and wait until the
                           thing is free, then go round again.  */
                        sem_op(&unlockit);
                        sem_op(&waitunlock);
                        continue;
                }
        }
}

void  lose_remotelock()
{
        union   my_semun        ms;

        ms.val = 0;
        if  (get_nservers() <= 0)
                return;
        sem_op(&lockit);
        if  (semctl(netsem_chan, NLCK_TYPE, GETVAL, ms) == NLT_LOCLOCK)  {
                ms.val = NLT_NONE;
                semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
        }
        sem_op(&unlockit);
        net_broadcast(N_UNLOCK);
}

/* This routine is called by the network monitor process to handle
   replies to lock requests. Currently we knock one off the count
   of machines we are waiting to hear from.  */

void  net_replylock()
{
        sem_op(&replied);
}

/* Process incoming lock requests from network */

int  net_lockreq(struct remote *rp)
{
        int     ret = N_REMLOCK_OK;
        union   my_semun        ms;

        ms.val = 0;

        /* First hang on till ready */

 restart:
        sem_op(&lockit);
        switch  (semctl(netsem_chan, NLCK_TYPE, GETVAL, ms))  {
        case    NLT_SETTING:
                /* We have a conflict here. The other end is trying to
                   lock at the same time as we are trying to do
                   the same. Resolve the dispute by the purely
                   arbitrary criterion of the hostid being less
                   taking the priority.  We should have sent out
                   a lock request to it, which will be handled at
                   the other end the opposite way by this if
                   statement */
                if  (myhostid < rp->hostid)  {
                        sem_op(&replied);       /* Cous other end doesn't reply */
                        sem_op(&unlockit);
                        return  N_REMLOCK_PRIO;
                }
                ret = N_REMLOCK_NONE;   /* Don't reply as other end rejects our lock request */

        case    NLT_NONE:
                /* No problems - set lock */
                ms.val = NLT_REMLOCK;
                semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                sem_op(&unlockit);
                Var_seg.dptr->vs_lockid = rp->hostid;
                return  ret;

        case    NLT_LOCLOCK:

                /* Local lock. Tough luck. Wait for operation to complete.  */

                sem_op(&unlockit);
                sem_op(&waitunlock);
                goto  restart;

        case    NLT_REMLOCK:

                /* Already have remote lock.
                   2 or more other machines must be fighting it out.
                   Just work out who the winner is and adjust if necessary */

                sem_op(&unlockit);
                if  (Var_seg.dptr->vs_lockid  &&  Var_seg.dptr->vs_lockid > rp->hostid)
                        Var_seg.dptr->vs_lockid = rp->hostid;   /* rp is the winner!!! */
                return  N_REMLOCK_OK;

        default:
        case  NLT_ERRREC:

                /* What do we do here?  Dunno let the other end worry about it.  */
                sem_op(&unlockit);
                return  N_REMLOCK_OK;
        }
}

/* Process incoming unlock requests. No reply is generated.  */

void  net_unlockreq(struct remote *rp)
{
        union   my_semun        ms;
        ms.val = 0;

        sem_op(&lockit);
        if  (semctl(netsem_chan, NLCK_TYPE, GETVAL, ms) == NLT_REMLOCK  &&  Var_seg.dptr->vs_lockid == rp->hostid)  {
                ms.val = NLT_NONE;
                semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                sem_op(&unlockit); /* W/o further ado */
                Var_seg.dptr->vs_lockid = 0;
        }
        else
                sem_op(&unlockit);
}

/* Host died - tidy up locks if we can */

void  netlock_hostdied(struct remote *rp)
{
        union   my_semun        ms;

        ms.val = 0;
        sem_op(&lockit);
        switch  (semctl(netsem_chan, NLCK_TYPE, GETVAL, ms))  {
        case  NLT_SETTING:
                ms.val = NLT_ERRREC;
                semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                ms.val = 0;
                semctl(netsem_chan, NLCK_XMITCNT, SETVAL, ms);
                sem_op(&unlockit);
                return;
        case  NLT_REMLOCK:
                if  (Var_seg.dptr->vs_lockid == rp->hostid)  {
                        ms.val = NLT_NONE;
                        semctl(netsem_chan, NLCK_TYPE, SETVAL, ms);
                        sem_op(&unlockit);
                        Var_seg.dptr->vs_lockid = 0;
                        return;
                }

        default:
        case  NLT_NONE:
        case  NLT_LOCLOCK:
        case  NLT_ERRREC:
                sem_op(&unlockit);
                return;
        }
}
