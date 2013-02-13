/* gbatch_getbtd.c -- API function to fetch user default permissions

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
#include "btuser.h"
#include "incl_unix.h"
#include "incl_net.h"

extern int  gbatch_read(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int  gbatch_getbtd(const int fd, apiBtdef *res)
{
        int                     ret, cnt;
        struct  api_fd          *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;
        Btdef           buf;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_GETBTD;
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);

        /* The message is followed by the details.  */

        if  ((ret = gbatch_read(fdp->sockfd, (char *) &buf, sizeof(buf))))
                return  ret;

        /* And now do all the byte-swapping */

        res->btd_version = buf.btd_version;
        res->btd_minp = buf.btd_minp;
        res->btd_maxp = buf.btd_maxp;
        res->btd_defp = buf.btd_defp;
        res->btd_maxll = ntohs(buf.btd_maxll);
        res->btd_totll = ntohs(buf.btd_totll);
        res->btd_spec_ll = ntohs(buf.btd_spec_ll);
        res->btd_priv = ntohl(buf.btd_priv);
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                res->btd_jflags[cnt] = ntohs(buf.btd_jflags[cnt]);
                res->btd_vflags[cnt] = ntohs(buf.btd_vflags[cnt]);
        }
        return  XB_OK;
}
