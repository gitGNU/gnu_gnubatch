/* Xb_cirea.c -- Read command interpreter

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

int	xb_ciread(const int fd, const unsigned flags, int *nci, Cmdint **cis)
{
	int		ret;
	unsigned	numcis;
	struct	api_fd	*fdp = xb_look_fd(fd);
	struct	api_msg	msg;

	if  (!fdp)
		return  XB_INVALID_FD;

	msg.code = API_CIREAD;
	msg.un.lister.flags = htonl(flags);
	if  ((ret = xb_wmsg(fdp, &msg)))
		return  ret;
	if  ((ret = xb_rmsg(fdp, &msg)))
		return  ret;
	if  (msg.retcode != 0)
		return  (SHORT) ntohs(msg.retcode);

	/* Get number of cis */

	numcis = (unsigned) ntohl(msg.un.r_lister.nitems);
	
	if  (nci)
		*nci = (int) numcis;

	/* Try to allocate enough space to hold the list.
	   If we don't succeed we'd better carry on reading it
	   so we don't get out of sync. */

	if  (numcis != 0)  {
		unsigned  nbytes = numcis * sizeof(Cmdint), cnt;
		Cmdint	*cilist;		
		if  (nbytes > fdp->bufmax)  {
			if  (fdp->bufmax != 0)  {
				free(fdp->buff);
				fdp->bufmax = 0;
				fdp->buff = (char *) 0;
			}
			if  (!(fdp->buff = malloc(nbytes)))  {
				unsigned  cnt;
				for  (cnt = 0;  cnt < numcis;  cnt++)  {
					Cmdint	slurp;
					if  ((ret = xb_read(fdp->sockfd, (char *) &slurp, sizeof(slurp))))
						return  ret;
				}
				return  XB_NOMEM;
			}
			fdp->bufmax = nbytes;
		}

		if  ((ret = xb_read(fdp->sockfd, fdp->buff, nbytes)))
			return  ret;

		cilist = (Cmdint *) fdp->buff;
		for  (cnt = 0;  cnt < numcis;  cilist++, cnt++)
			cilist->ci_ll = ntohs(cilist->ci_ll);
	}

	/* Set up answer */

	if  (cis)
		*cis = (Cmdint *) fdp->buff;
	return  XB_OK;
}
