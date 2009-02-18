/* Xb_jchgr.c -- change job group

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

int	xb_jobchgrp(const int fd, const unsigned flags, const slotno_t slotno, const char *newgroup)
{
	int	ret;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;
	struct	jugmsg	res;

	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_JOBCHGRP;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);
	/*	See comments in xb_jobchown about usage of fields. */
	memset((void *) &res, '\0', sizeof(res));
	strncpy(res.newug, newgroup, UIDSIZE);
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
