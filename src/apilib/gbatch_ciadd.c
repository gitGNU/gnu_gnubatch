/* xb_ciadd.c -- create new command interpreter

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

extern int  gbatch_write(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int     gbatch_ciadd(const int fd, const unsigned flags, const Cmdint *newci, unsigned *res)
{
        int             ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;
        Cmdint          nci;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_CIADD;
        msg.un.lister.flags = htonl(flags);
        BLOCK_ZERO(&nci, sizeof(nci));
        nci.ci_ll = htons(newci->ci_ll);
        nci.ci_nice = newci->ci_nice;
        nci.ci_flags = newci->ci_flags;
        strncpy(nci.ci_name, newci->ci_name, CI_MAXNAME);
        strncpy(nci.ci_path, newci->ci_path, CI_MAXFPATH);
        strncpy(nci.ci_args, newci->ci_args, CI_MAXARGS);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_write(fdp->sockfd, (char *) &nci, sizeof(nci))))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        if  (res)
                *res = ntohl(msg.un.r_reader.seq);
        return  XB_OK;
}
