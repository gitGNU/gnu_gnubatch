/* gbatch_varrename.c -- API function to rename variable

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
extern struct api_fd *gbatch_look_fd(const int);

int     gbatch_varrename(const int fd, const unsigned flags, const slotno_t slotno, const char *newname)
{
        int     ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;
        char    nbuf[BTV_NAME+1];

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_VARRENAME;
        msg.un.reader.flags = htonl(flags);
        msg.un.reader.seq = htonl(fdp->vserial);
        msg.un.reader.slotno = htonl(slotno);
        strncpy(nbuf, newname, BTV_NAME);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_write(fdp->sockfd, nbuf, sizeof(nbuf))))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.un.r_reader.seq != 0)
                fdp->vserial = ntohl(msg.un.r_reader.seq);
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        return  XB_OK;
}
