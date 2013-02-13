/* gbatch_getbtu.c -- API function to get user permissions

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

extern int  gbatch_read(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int     gbatch_getbtu(const int fd, const char *username, char *groupname, apiBtuser *res)
{
        int     ret, cnt;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;
        struct  ua_reply        buf;

        if  (!fdp)
                return  XB_INVALID_FD;
        BLOCK_ZERO(&msg, sizeof(msg));
        msg.code = API_GETBTU;
        if  (username  &&  username[0])
                strncpy(msg.un.us.username, username, UIDSIZE);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);

        /* The message is followed by the details.  */

        if  ((ret = gbatch_read(fdp->sockfd, (char *) &buf, sizeof(buf))))
                return  ret;

        if  (groupname)  {
                strncpy(groupname, buf.ua_gname, UIDSIZE);
                groupname[UIDSIZE] = '\0';
        }

        /* And now do all the byte-swapping */

        res->btu_user = ntohl(buf.ua_perm.btu_user);
        res->btu_minp = buf.ua_perm.btu_minp;
        res->btu_maxp = buf.ua_perm.btu_maxp;
        res->btu_defp = buf.ua_perm.btu_defp;
        res->btu_maxll = ntohs(buf.ua_perm.btu_maxll);
        res->btu_totll = ntohs(buf.ua_perm.btu_totll);
        res->btu_spec_ll = ntohs(buf.ua_perm.btu_spec_ll);
        res->btu_priv = ntohl(buf.ua_perm.btu_priv);

        for  (cnt = 0;  cnt < 3;  cnt++)  {
                res->btu_jflags[cnt] = ntohs(buf.ua_perm.btu_jflags[cnt]);
                res->btu_vflags[cnt] = ntohs(buf.ua_perm.btu_vflags[cnt]);
        }
        return  XB_OK;
}
