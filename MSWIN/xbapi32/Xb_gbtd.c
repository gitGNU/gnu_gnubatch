/* Xb_gbtd.c -- Get user permission defaults

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

extern int	xb_read(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);
			
extern struct api_fd *xb_look_fd(const int);

int	xb_getbtd(const int fd, apiBtdef  *res)
{
	int			ret, cnt;
	struct	api_fd		*fdp = xb_look_fd(fd);
	struct	api_msg		msg;
	apiBtdef		buf;

	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_GETBTD;
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* The message is followed by the details. */

	if  ((ret = xb_read(fdp->sockfd, (char *) &buf, sizeof(buf))))
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
