/* gbatch_putbtd.c -- API function to save user default permissions

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

extern int  gbatch_write(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int  gbatch_putbtd(const int fd, const apiBtdef *res)
{
        int                     ret, cnt;
        struct  api_fd          *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;
        Btdef           buf;

        if  (!fdp)
                return  XB_INVALID_FD;
        msg.code = API_PUTBTD;

        /* And now do all the byte-swapping */

        buf.btd_version = res->btd_version; /* Actually this is ignored */
        buf.btd_minp = res->btd_minp;
        buf.btd_maxp = res->btd_maxp;
        buf.btd_defp = res->btd_defp;
        buf.btd_maxll = htons(res->btd_maxll);
        buf.btd_totll = htons(res->btd_totll);
        buf.btd_spec_ll = htons(res->btd_spec_ll);
        buf.btd_priv = htonl(res->btd_priv);
        for  (cnt = 0;  cnt < 3;  cnt++)  {
                buf.btd_jflags[cnt] = htons(res->btd_jflags[cnt]);
                buf.btd_vflags[cnt] = htons(res->btd_vflags[cnt]);
        }
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_write(fdp->sockfd, (char *) &buf, sizeof(buf))))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);
        return  XB_OK;
}
