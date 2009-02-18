/* Xb_setqu.c -- Set queue prefix

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
#include <malloc.h>
#include <winsock.h>
#include "xbapi.h"
#include "xbapi_in.h"

extern int	xb_write(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct api_fd *xb_look_fd(const int);

int	xb_setqueue(const int fd, const char *queuename)
{
	int	ret;
	unsigned	length;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_SETQUEUE;
	length = queuename && queuename[0]? strlen(queuename) + 1: 0;
	msg.un.queuelength = htons((USHORT) length);
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
	if  (length != 0  &&  (ret = xb_write(fdp->sockfd, (char *) queuename, length)))
		return  ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
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
		memcpy(fdp->queuename, queuename, length);
	}
	return  XB_OK;
}
