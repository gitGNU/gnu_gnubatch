/* gbatch_gethenv.c -- API function to get host environment

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

char **gbatch_gethenv(const int fd)
{
        int     ret;
        unsigned        nitems, cnt;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;
        char    **result;

        if  (!fdp)  {
                gbatch_dataerror = XB_INVALID_FD;
                return  (char **) 0;
        }

        msg.code = API_SENDENV;
        if  ((ret = gbatch_wmsg(fdp, &msg))  ||  (ret = gbatch_rmsg(fdp, &msg)))  {
                gbatch_dataerror = ret;
                return  (char **) 0;
        }
        if  (msg.retcode != 0)  {
                gbatch_dataerror = (SHORT) ntohs(msg.retcode);
                return  (char **) 0;
        }
        nitems = ntohl(msg.un.r_lister.nitems);
        if  (!(result = (char **) malloc((unsigned)(sizeof(char *) * (nitems + 1)))))  {
                gbatch_dataerror = XB_NOMEM;
                return  (char **) 0;
        }

        /* Maximum length is in seq if we want it.  */

        for  (cnt = 0;  cnt < nitems;  cnt++)  {
                ULONG   le;
                unsigned  lng;
                char    *rp;
                if  ((ret = gbatch_read(fdp->sockfd, (char *) &le, sizeof(le))) != 0)  {
                dataerr:
                        gbatch_dataerror = ret;
                errrest:
                        while  (cnt != 0)
                                free(result[--cnt]);
                        free((char *) result);
                        return  (char **) 0;
                }
                lng = ntohl(le);
                if  (!(rp = malloc(lng+1)))  {
                        gbatch_dataerror = XB_NOMEM;
                        goto  errrest;
                }
                if  ((ret = gbatch_read(fdp->sockfd, rp, lng+1)) != 0)  {
                        free(rp);
                        goto  dataerr;
                }
                result[cnt] = rp;
        }
        result[nitems+1] = (char *) 0;
        return  result;
}
