/* gbatch_setqueue.c -- API function to set a job queue name

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
#include <errno.h>
#include "gbatch.h"
#include "xbapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"

extern int  gbatch_write(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int  gbatch_setqueue(const int fd, const char *queuename)
{
        int     ret;
        unsigned        length;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_SETQUEUE;
        length = queuename && queuename[0]? strlen(queuename) + 1: 0;
        msg.un.queuelength = htons((USHORT) length);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  (length != 0  &&  (ret = gbatch_write(fdp->sockfd, (char *) queuename, length)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        if  (fdp->queuename)  {
                free(fdp->queuename);
                fdp->queuename = (char *) 0;
        }
        if  (length != 0)  {
                if  (!(fdp->queuename = malloc(length)))
                        return  XB_NOMEM;
                BLOCK_COPY(fdp->queuename, queuename, length);
        }
        return  XB_OK;
}
