/* produser.c -- tell scheduler about changes to user file

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
#include <sys/msg.h>
#include "defaults.h"
#include "files.h"
#include "incl_net.h"
#include "incl_ugid.h"
#include "incl_unix.h"
#include "btmode.h"
#include "btconst.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "network.h"
#include "btjob.h"
#include "cmdint.h"
#include "ipcstuff.h"
#include "shreq.h"

/* After rebuilding btufile, see if scheduler is running and if so
   kick scheduler.  */

void  produser()
{
        int     msgq_id;

        if  ((msgq_id = msgget(MSGID+envselect_value, 0)) >= 0)  {
                Shipc   oreq;
                BLOCK_ZERO(&oreq, sizeof(oreq));
                oreq.sh_mtype = TO_SCHED;
                oreq.sh_params.mcode = O_PWCHANGED;
                oreq.sh_params.uuid = Realuid;
                oreq.sh_params.ugid = Realgid;
                oreq.sh_params.upid = getpid();
                msgsnd(msgq_id, (struct msgbuf *) &oreq, sizeof(Shreq), 0);
        }
}
