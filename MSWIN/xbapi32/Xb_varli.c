/* Xb_varli.c -- List variables

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

extern int	xb_read(const SOCKET, char *, unsigned),
		xb_rmsg(const struct api_fd *, struct api_msg *),
		xb_wmsg(const struct api_fd *, struct api_msg *);

extern struct api_fd *xb_look_fd(const int);

int	xb_varlist(const int fd, const unsigned flags, int *np, slotno_t **slots)
{
	int		ret;
	unsigned	numvars;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XB_INVALID_FD;
	msg.code = API_VARLIST;
	msg.un.lister.flags = htonl(flags);
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* Get number of variables */

	fdp->vserial = ntohl(msg.un.r_lister.seq);
	numvars = (unsigned) ntohl(msg.un.r_lister.nitems);
	if  (np)
		*np = (int) numvars;

	/* Try to allocate enough space to hold the list.
	   If we don't succeed we'd better carry on reading it
	   so we don't get out of sync. */

	if  (numvars != 0)  {
		unsigned  nbytes = numvars * sizeof(slotno_t), cnt;
		slotno_t  *sp;
		if  (nbytes > fdp->bufmax)  {
			if  (fdp->bufmax != 0)  {
				free(fdp->buff);
				fdp->bufmax = 0;
				fdp->buff = (char *) 0;
			}
			if  (!(fdp->buff = malloc(nbytes)))  {
				unsigned  cnt;
				for  (cnt = 0;  cnt < numvars;  cnt++)  {
					ULONG  slurp;
					if  ((ret = xb_read(fdp->sockfd, (char *) &slurp, sizeof(slurp))))
						return  ret;
				}
				return  XB_NOMEM;
			}
			fdp->bufmax = nbytes;
		}
		if  ((ret = xb_read(fdp->sockfd, fdp->buff, nbytes)))
			return  ret;
		sp = (slotno_t *) fdp->buff;
		for  (cnt = 0;  cnt < numvars;  cnt++)  {
			*sp = ntohl(*sp);
			sp++;
		}
	}

	/* Set up answer */

	if  (slots)
		*slots = (slotno_t *) fdp->buff;
	return  XB_OK;
}
