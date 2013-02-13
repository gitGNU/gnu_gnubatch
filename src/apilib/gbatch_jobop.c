/* gbatch_jobop.c -- API routine for job operations

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
#include "gbatch.h"
#include "xbapi_int.h"
#include "incl_net.h"

extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_write(const int, char *, unsigned);
extern struct api_fd *gbatch_look_fd(const int);

int     gbatch_jobop(const int fd, const unsigned flags, const slotno_t slotno, const unsigned op, const unsigned param)
{
        int     ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_JOBOP;
        msg.un.jop.flags = htonl(flags);
        msg.un.jop.seq = htonl(fdp->jserial);
        msg.un.jop.slotno = htonl(slotno);
        msg.un.jop.op = htonl(op);
        msg.un.jop.param = htonl(param);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.un.r_reader.seq != 0)
                fdp->jserial = ntohl(msg.un.r_reader.seq);
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        return  XB_OK;
}
