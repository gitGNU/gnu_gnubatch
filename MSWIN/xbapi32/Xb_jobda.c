/* Xb_jobda.c -- Get job data

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
#include <io.h>
#include "xbapi.h"
#include "xbapi_in.h"

extern int	xb_read(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);

extern struct api_fd *xb_look_fd(const int);

int	xb_jobdata(const int fd,
		   const int outfile,
		   int (*func)(int,void*,unsigned),
		   const unsigned flags,
		   const slotno_t slotno)
{
	int		ret, bcount;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;
	char	buffer[XBA_BUFFSIZE];

	if  (!fdp)
		return  xbapi_dataerror = XB_INVALID_FD;

	msg.code = API_JOBDATA;
	msg.un.reader.flags = htonl(flags);
	msg.un.reader.seq = htonl(fdp->jserial);
	msg.un.reader.slotno = htonl(slotno);

	if  ((ret = xb_wmsg(fdp, &msg)))
		return  xbapi_dataerror = ret;

	if  ((ret = xb_rmsg(fdp, &msg)))
		return  xbapi_dataerror = ret;

	if  (msg.un.r_reader.seq != 0)
		fdp->jserial = ntohl(msg.un.jobdata.seq);

	if  (msg.retcode != 0)
		return  xbapi_dataerror = (SHORT) ntohs(msg.retcode);
    
	for  (;;)  {
		if  ((ret = xb_rmsg(fdp, &msg)) != 0)
			return  xbapi_dataerror = ret;
		if  (msg.code != API_DATAOUT)
			break;
		if  ((bcount = ntohs(msg.un.jobdata.nbytes)) <= 0)
			break;
		if  ((ret = xb_read(fdp->sockfd, buffer, (unsigned) bcount)) != 0)
			return  xbapi_dataerror = ret;
		(*func)(outfile, buffer, (unsigned) bcount);
	}
	
	return  XB_OK;
}
