/* gbatch_holread.c -- API function to read holiday file

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
#include "incl_net.h"
#include "incl_unix.h"

extern int  gbatch_read(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

int     gbatch_holread(const int fd, const unsigned flags, int year, unsigned char **hols)
{
        int             ret;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;

        if  (!fdp)
                return  XB_INVALID_FD;
        if  (year < 1990)  {
                if  (year > 90 && year < 200)
                        year += 1900;
                else
                        return  XB_INVALID_YEAR;
        }
        msg.code = API_HOLREAD;
        msg.un.reader.flags = htonl(flags);
        msg.un.reader.slotno = htonl(year);
        if  ((ret = gbatch_wmsg(fdp, &msg)))
                return  ret;
        if  ((ret = gbatch_rmsg(fdp, &msg)))
                return  ret;
        if  (msg.retcode != 0)
                return  (SHORT) ntohs(msg.retcode);

        /* Try to allocate enough space to hold the list.  If we don't
           succeed we'd better carry on reading it so we don't
           get out of sync.  */

        if  (fdp->bufmax < YVECSIZE)  {
                if  (fdp->bufmax != 0)  {
                        free(fdp->buff);
                        fdp->bufmax = 0;
                        fdp->buff = (char *) 0;
                }
                if  (!(fdp->buff = malloc(YVECSIZE)))  {
                        unsigned  char  slurp[YVECSIZE];
                        if  ((ret = gbatch_read(fdp->sockfd, (char *) slurp, sizeof(slurp))))
                                return  ret;
                        return  XB_NOMEM;
                }
                fdp->bufmax = YVECSIZE;
        }

        if  ((ret = gbatch_read(fdp->sockfd, fdp->buff, YVECSIZE)))
                return  ret;

        if  (hols)
                *hols = (unsigned char *) fdp->buff;
        return  XB_OK;
}
