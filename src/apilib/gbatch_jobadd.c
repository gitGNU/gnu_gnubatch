/* gbatch_jobadd.c -- API function to add a job

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
#include "netmsg.h"

extern int  gbatch_write(const int, char *, unsigned);
extern int  gbatch_rmsg(const struct api_fd *, struct api_msg *);
extern int  gbatch_wmsg(const struct api_fd *, struct api_msg *);
extern unsigned  gbatch_jobswap(struct jobnetmsg *, const apiBtjob *);
extern struct api_fd *gbatch_look_fd(const int);

static void  jabort(const struct api_fd *fdp, const jobno_t jobno)
{
        struct  api_msg msg;
        msg.code = API_DATAABORT;
        msg.un.jobdata.jobno = htonl(jobno);
        gbatch_wmsg(fdp, &msg);
}

FILE *gbatch_jobadd(const int fd, apiBtjob *newjob)
{
        int                     ret;
        unsigned                length;
        PIDTYPE                 cpid;
        jobno_t                 jobno;
        struct  api_fd          *fdp = gbatch_look_fd(fd);
        struct  api_msg         msg;
        struct  jobnetmsg       res;
        int                     pfd[2];

        if  (!fdp)  {
                gbatch_dataerror = XB_INVALID_FD;
                return  (FILE *) 0;
        }

        msg.code = API_JOBADD;
        msg.un.jobdata.jobno = htonl((jobno_t) getpid());
        length = gbatch_jobswap(&res, newjob);

        if  ((ret = gbatch_wmsg(fdp, &msg)))  {
                gbatch_dataerror = ret;
                return  (FILE *) 0;
        }
        if  ((ret = gbatch_write(fdp->sockfd, (char *) &res, length)))  {
                gbatch_dataerror = ret;
                return  (FILE *) 0;
        }
        if  ((ret = gbatch_rmsg(fdp, &msg)))  {
                gbatch_dataerror = ret;
                return  (FILE *) 0;
        }
        if  (msg.retcode != 0)  {
                gbatch_dataerror = (SHORT) ntohs(msg.retcode);
                return  (FILE *) 0;
        }

        /* Return job number in apispq_job if the caller is interested.  */

        newjob->h.bj_job = jobno = ntohl(msg.un.jobdata.jobno);

        /* Ok blast away at the job data */

        if  (pipe(pfd) < 0)  {
                jabort(fdp, jobno);
                gbatch_dataerror = XB_CHILDPROC;
                return  (FILE *) 0;
        }
        if  ((cpid = fork()) != 0)  {
                int     status;
#ifndef HAVE_WAITPID
                PIDTYPE rpid;
#endif
                if  (cpid < 0)  {
                        jabort(fdp, jobno);
                        gbatch_dataerror = XB_CHILDPROC;
                        return  (FILE *) 0;
                }
                close(pfd[0]);
#ifdef  HAVE_WAITPID
                while  (waitpid(cpid, &status, 0) < 0  &&  errno == EINTR)
                        ;
#else
                while  ((rpid = wait(&status)) != cpid  &&  (rpid >= 0 || errno == EINTR))
                        ;
#endif
                if  (status != 0)  {
                        gbatch_dataerror = XB_CHILDPROC;
                        return  (FILE *) 0;
                }
                return fdopen(pfd[1], "w");
        }
        else  {
                /* Child process....
                   Fork again so parent doesn't
                   have to worry about zombies other than me on a
                   Monday morning.  */

                int     bcount;
                char    buffer[XBA_BUFFSIZE];

                close(pfd[1]);

                if  ((cpid = fork()) != 0)
                        exit(cpid < 0? 255: 0);

                msg.code = API_DATAIN;
                msg.un.jobdata.jobno = htonl(jobno);
                while  ((bcount = read(pfd[0], buffer, XBA_BUFFSIZE)) > 0)  {
                        msg.un.jobdata.nbytes = htons(bcount);
                        gbatch_wmsg(fdp, &msg);
                        gbatch_write(fdp->sockfd, buffer, (unsigned) bcount);
                }
                msg.code = API_DATAEND;
                gbatch_wmsg(fdp, &msg);
                exit(0);
        }
}

int  gbatch_jobres(const int fd, jobno_t *jno)
{
        struct  api_fd  *fdp = gbatch_look_fd(fd);
        struct  api_msg msg;

        if  (!fdp)
                return  XB_INVALID_FD;

        gbatch_rmsg(fdp, &msg);
        if  (msg.retcode == 0  &&  jno)  {
                *jno = ntohl(msg.un.jobdata.jobno);
                fdp->jserial = ntohl(msg.un.jobdata.seq);
        }
        return  (SHORT) ntohs(msg.retcode);
}
