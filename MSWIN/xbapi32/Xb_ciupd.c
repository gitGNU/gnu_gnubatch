/* Xb_ciupd.c -- Update command interpreter

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

extern int	xb_write(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct api_fd *xb_look_fd(const int);

int	xb_ciupd(const int fd, const unsigned flags, const unsigned slot, const Cmdint *newci)
{
	int		ret;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;
	Cmdint		nci;

	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_CIUPD;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.slotno = htonl(slot);
	memset(&nci, '\0', sizeof(nci));
	nci.ci_ll = htons(newci->ci_ll);
	nci.ci_nice = newci->ci_nice;
	nci.ci_flags = newci->ci_flags;
	strncpy(nci.ci_name, newci->ci_name, CI_MAXNAME);
	strncpy(nci.ci_path, newci->ci_path, CI_MAXFPATH);
	strncpy(nci.ci_args, newci->ci_args, CI_MAXARGS);
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xb_write(fdp->sockfd, (char *) &nci, sizeof(nci))))
		return  ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);
	return  XB_OK;
}
