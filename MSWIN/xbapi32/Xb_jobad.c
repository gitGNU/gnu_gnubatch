/* Xb_jobad.c -- Add job

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
#include <process.h>
#include <io.h>
#include "xbapi.h"
#include "xbapi_in.h"
#include "netmsg.h"

extern int	xb_write(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);

extern unsigned xb_jobswap(struct jobnetmsg *, const apiBtjob *);

extern struct api_fd *xb_look_fd(const int);

static	jobno_t	gen_jobno()
{
	static	char	doneit = 0;
	static	jobno_t	result;
	
	if  (!doneit)  {
		doneit = 1;
		result = ((unsigned long) getpid()) % 0x7ffffU;
	}
	else
		result++;
	return  result;
}

int	xb_jobadd(const int fd, 
		  const int	infile,
		  int	(*func)(int,void*,unsigned),
		  apiBtjob *newjob)
{
	int			ret, bcount;
	unsigned		length;
	jobno_t			jobno;
	struct	api_fd		*fdp = xb_look_fd(fd);
	struct	api_msg		msg;
	struct	jobnetmsg	res;
	char	buffer[XBA_BUFFSIZE];

	if  (!fdp)
		return  xbapi_dataerror = XB_INVALID_FD;

	msg.code = API_JOBADD;
	msg.un.jobdata.jobno = htonl(gen_jobno());
	length = xb_jobswap(&res, newjob);

	if  ((ret = xb_wmsg(fdp, &msg)))
		return  xbapi_dataerror = ret;

	if  ((ret = xb_write(fdp->sockfd, (char *) &res, length)))
		return  xbapi_dataerror = ret;

	if  ((ret = xb_rmsg(fdp, &msg)))
		return  xbapi_dataerror = ret;

	if  (msg.retcode != 0)
		return  xbapi_dataerror = (SHORT) ntohs(msg.retcode);

	/* Return job number in bj_job if the caller is interested. */

	newjob->h.bj_job = jobno = ntohl(msg.un.jobdata.jobno);
	msg.code = API_DATAIN;
	msg.un.jobdata.jobno = htonl(jobno);
	while  ((bcount = (*func)(infile, buffer, XBA_BUFFSIZE)) > 0)  {
		msg.un.jobdata.nbytes = htons((short)bcount);
		if  ((ret = xb_wmsg(fdp, &msg))  ||
			 (ret = xb_write(fdp->sockfd, buffer, (unsigned) bcount)))
			 return  xbapi_dataerror = ret;
	}
	msg.code = API_DATAEND;
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  xbapi_dataerror = ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
		return  xbapi_dataerror = ret;
	return  xbapi_dataerror = ntohs(msg.retcode);
}
