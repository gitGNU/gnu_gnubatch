/* gbatch_jobchmod.c -- API function to change job permissions

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
#include "incl_unix.h"
#include "netmsg.h"

extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_write(const int, char *, unsigned);
extern void  gbatch_mode_pack(Btmode *, const Btmode *);
extern struct api_fd *gbatch_look_fd(const int);

int     gbatch_jobchmod(const int fd, const unsigned flags, const slotno_t slotno, const Btmode *newmode)
{
        int     ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;
        struct  jobhnetmsg      res;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_JOBCHMOD;
        msg.un.reader.flags = htonl(flags);
        msg.un.reader.seq = htonl(fdp->jserial);
        msg.un.reader.slotno = htonl(slotno);
        BLOCK_ZERO(&res, sizeof(res)); /* All this is a bit wasteful really but who cares */
        gbatch_mode_pack(&res.nm_mode, newmode);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_write(fdp->sockfd, (char *) &res, sizeof(res))))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.un.r_reader.seq != 0)
                fdp->jserial = ntohl(msg.un.r_reader.seq);
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        return  XB_OK;
}
