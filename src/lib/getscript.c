/* getscript.c -- get job script as a string

   Copyright 2013 Free Software Foundation, Inc.

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

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "netmsg.h"
#include "files.h"

#define  BUFFSIZE       1024

extern  FILE *net_feed(const int, const netid_t, const jobno_t, const int);

/* Get script string from named file */

char    *getscript_file(FILE *ifl, unsigned *lp)
{
        unsigned  rsize = 0;
        char    *result = (char *) 0;
        unsigned  nb;
        char    inbuf[BUFFSIZE];

        while  ((nb = fread(inbuf, sizeof(char), sizeof(inbuf), ifl)) != 0)  {
                if  (result)  {
                        char  *newres = realloc(result, rsize + nb + 1);
                        if  (!newres)  {
                                free(result);
                                result = stracpy("Too big");
                                rsize = strlen(result);
                                break;
                        }
                        result = newres;
                        BLOCK_COPY(result + rsize, inbuf, nb);
                        rsize += nb;
                }
                else  {
                        result = malloc(nb + 1);
                        if  (!result)  {
                                result = stracpy("Too big");
                                rsize = strlen(result);
                                break;
                        }
                        BLOCK_COPY(result, inbuf, nb);
                        rsize = nb;
                }
        }

        *lp = rsize;
        if  (result)
                result[rsize] = '\0';
        return  result;
}

/* Assume that we are "cd-ed" to the spool directory and also that we have access */

char    *getscript(BtjobRef jp, unsigned *lp)
{
        FILE    *ifl;
        char    *result;

        /* Fetch from remote if applicable otherwise from local spool directory */

        if  (jp->h.bj_hostid)
                ifl = net_feed(FEED_JOB, jp->h.bj_hostid, jp->h.bj_job, Job_seg.dptr->js_viewport);
        else
                ifl = fopen(mkspid(SPNAM, jp->h.bj_job), "r");

        if  (!ifl)
                return  (char *) 0;

        result = getscript_file(ifl, lp);
        fclose(ifl);
        return  result;
}
