/* gbatch_jobdata.c -- API function to fetch job data

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
#include <errno.h>
#include "gbatch.h"
#include "xbapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"
#include "incl_sig.h"

extern int  gbatch_read(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern struct api_fd *gbatch_look_fd(const int);

static void  soakupdata(const struct api_fd *fdp)
{
        int     bcount;
        struct  api_msg msg;
        char    buffer[XBA_BUFFSIZE];

        while  (gbatch_rmsg(fdp, &msg) == 0  &&  msg.code == API_DATAOUT  &&
                (bcount = (SHORT) ntohs(msg.un.jobdata.nbytes)) > 0)
                if  (gbatch_read(fdp->sockfd, buffer, (unsigned) bcount) != 0)
                        return;
}

FILE *gbatch_jobdata(const int fd, const unsigned flags, const slotno_t slotno)
{
        int     ret, ignored;
        PIDTYPE cpid;
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;
        int     pfd[2];

        if  (!fdp)  {
                gbatch_dataerror = XB_INVALID_FD;
                return  (FILE *) 0;
        }
        msg.code = API_JOBDATA;
        msg.un.reader.flags = htonl(flags);
        msg.un.reader.seq = htonl(fdp->jserial);
        msg.un.reader.slotno = htonl(slotno);
        if  ((ret = gbatch_wmsg(fdp, &msg)))  {
                gbatch_dataerror = ret;
                return  (FILE *) 0;
        }
        if  ((ret = gbatch_rmsg(fdp, &msg)))  {
                gbatch_dataerror = ret;
                return  (FILE *) 0;
        }
        if  (msg.un.r_reader.seq != 0)
                fdp->jserial = ntohl(msg.un.jobdata.seq);
        if  (msg.retcode != 0)  {
                gbatch_dataerror = (SHORT) ntohs(msg.retcode);
                return  (FILE *) 0;
        }

        /* Ok blast away at the job data */

        if  (pipe(pfd) < 0)  {
                gbatch_dataerror = XB_CHILDPROC;
                soakupdata(fdp);
                return  (FILE *) 0;
        }
        if  ((cpid = fork()) != 0)  {
                int     status;
                PIDTYPE rpid;

                if  (cpid < 0)  {
                        gbatch_dataerror = XB_CHILDPROC;
                        soakupdata(fdp);
                        return  (FILE *) 0;
                }
                close(pfd[1]);
#ifdef  HAVE_WAITPID
                while  ((rpid = waitpid(cpid, &status, 0)) < 0  &&  errno == EINTR)
                        ;
#else
                while  ((rpid = wait(&status)) != cpid  &&  (rpid >= 0  ||  errno == EINTR))
                        ;
#endif
                if  (rpid < 0  ||  status != 0)  {
                        gbatch_dataerror = XB_CHILDPROC;
                        soakupdata(fdp);
                        return  (FILE *) 0;
                }
                return fdopen(pfd[0], "r");
        }
        else  {
                int     bcount;
                char    buffer[XBA_BUFFSIZE];

                /* Child process....
                   Fork again so parent doesn't have to worry about
                   zombies other than me on a Monday morning.  */

                close(pfd[0]);
                signal(SIGPIPE, SIG_IGN);

                if  ((cpid = fork()) != 0)
                        _exit(cpid < 0? 255: 0);

                while  (gbatch_rmsg(fdp, &msg) == 0  &&  msg.code == API_DATAOUT  &&
                        (bcount = (SHORT) ntohs(msg.un.jobdata.nbytes)) > 0)  {
                        if  (gbatch_read(fdp->sockfd, buffer, (unsigned) bcount) != 0)
                                _exit(0);
                        ignored = write(pfd[1], buffer, bcount);
                }
                close(pfd[1]);
                _exit(0);
                return  0;              /* Silence compilers */
        }
}
