/* sh_netfeed.c -- scheduler network shunting

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
#include "incl_sig.h"
#include <sys/types.h>
#include <sys/ipc.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
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
#include "files.h"
#include "q_shm.h"
#include "sh_ext.h"

/* Send a file down a socket. Delete when done if del is set.  */

static  void  feed_pr(const jobno_t jobno, const char *prefix, const int sock, const int del)
{
        int     ffd, bytes;
        char    *fname;
        char    buffer[1024];

        if  ((ffd = open(fname = mkspid(prefix, jobno), O_RDONLY)) >= 0)  {
                while  ((bytes = read(ffd, buffer, sizeof(buffer))) > 0)
                        Ignored_error = write(sock, buffer, (unsigned) bytes);
                close(ffd);
                if  (del)
                        unlink(fname);
        }
}

/* Process request from another machine to read job / stdout / stderr file */

void  feed_req()
{
        int             sock;
        PIDTYPE         ret;
        struct  feeder  rq;
        SOCKLEN_T       sinl;
        struct  sockaddr  sin;

        sinl = sizeof(sin);
        if  ((sock = accept(viewsock, &sin, &sinl)) < 0)
                return;

        if  (read(sock, (char *) &rq, sizeof(rq)) != sizeof(rq))  {
                close(sock);
                return;
        }
        if  ((ret = forksafe()) < 0)
                panic($E{Cannot fork});
        if  (ret != 0)  {
                close(sock);
                return;
        }

        /* We are now a separate process */

        switch  (rq.fdtype)  {
        case  FEED_JOB:
        default:
                feed_pr(ntohl(rq.jobno), SPNAM, sock, 0);
                exit(0);
        case  FEED_SO:
                feed_pr(ntohl(rq.jobno), SONAM, sock, 1);
                exit(0);
        case  FEED_SE:
                feed_pr(ntohl(rq.jobno), SENAM, sock, 1);
                exit(0);
        }
}
