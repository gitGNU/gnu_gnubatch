/* gbatch_putbtu.c -- API function to save user permissions

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
#include "incl_unix.h"
#include "incl_net.h"
#include "btuser.h"
#include "xbnetq.h"

extern int  gbatch_write(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int  gbatch_putbtu(const int fd, const char *username, const apiBtuser *res)
{
        int     ret, cnt;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;
        struct  ua_reply        buf;

        if  (!fdp)
                return  XB_INVALID_FD;
        BLOCK_ZERO(&msg, sizeof(msg));
        msg.code = API_PUTBTU;
        if  (username  &&  username[0])
                strncpy(msg.un.us.username, username, UIDSIZE);

        /* And now do all the byte-swapping */

        buf.ua_perm.btu_user = 0;
        buf.ua_perm.btu_minp = res->btu_minp;
        buf.ua_perm.btu_maxp = res->btu_maxp;
        buf.ua_perm.btu_defp = res->btu_defp;
        buf.ua_perm.btu_maxll = htons(res->btu_maxll);
        buf.ua_perm.btu_totll = htons(res->btu_totll);
        buf.ua_perm.btu_spec_ll = htons(res->btu_spec_ll);
        buf.ua_perm.btu_priv = htonl(res->btu_priv);

        for  (cnt = 0;  cnt < 3;  cnt++)  {
                buf.ua_perm.btu_jflags[cnt] = htons(res->btu_jflags[cnt]);
                buf.ua_perm.btu_vflags[cnt] = htons(res->btu_vflags[cnt]);
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
