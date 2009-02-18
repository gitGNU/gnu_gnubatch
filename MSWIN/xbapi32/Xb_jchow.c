/* Xb_jchow.c -- Change job owner

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

#include <sys/types.h>
#include <string.h>
#include <winsock.h>
#include "xbapi.h"
#include "xbapi_in.h"
#include "netmsg.h"

extern int	xb_write(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct api_fd *xb_look_fd(const int);

int	xb_jobchown(const int fd, const unsigned flags, const slotno_t slotno, const char *newuser)
{
	int	ret;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;
	struct	jugmsg	res;

	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_JOBCHOWN;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);

	/* Might be worth mentioning the thinking here -
	   We have a "jid" field in "struct jugmsg" which we could use
	   in place of the ones in "msg", but those typically refer to
	   the Owning machine, not the one we are talking to the xbnetserv
	   process of, hence we use the ones in "msg" for consistency.
	   We could equally well define something other than a "struct
	   jugmsg" but this isn't invoked often enough to make it worthwhile. */

	memset(&res, '\0', sizeof(res));
	strncpy(res.newug, newuser, UIDSIZE);
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xb_write(fdp->sockfd, (char *) &res, sizeof(res))))
		return  ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.un.r_reader.seq != 0)
		fdp->jserial = ntohl(msg.un.r_reader.seq);
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  XB_OK;
}
