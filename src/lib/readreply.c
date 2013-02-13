/* readreply.c -- get reply from scheduler message queue

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
#include <errno.h>
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "ecodes.h"
#include "shreq.h"
#include "incl_unix.h"

extern  int     Ctrl_chan;
extern  long    mymtype;

/* Get reply from scheduler */

unsigned  readreply()
{
        Repmess rr;

        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
                if  (errno == EINTR)
                        continue;
                exit(E_SHUTD);
        }

        return  rr.outmsg.mcode;
}
