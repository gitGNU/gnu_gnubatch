/* cgifndjb.c -- CGI function to find job

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

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "errnums.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "cgifndjb.h"

int  numeric(const char *x)
{
        while  (*x)  {
                if  (!isdigit(*x))
                        return  0;
                x++;
        }
        return  1;
}

int  decode_jnum(char *jnum, struct jobswanted *jwp)
{
        char    *cp;

        if  ((cp = strchr(jnum, ':')))  {
                *cp = '\0';
                if  ((jwp->host = look_int_hostname(jnum)) == -1)  {
                        *cp = ':';
                        disp_str = jnum;
                        return  $E{Unknown host name};
                }
                *cp++ = ':';
        }
        else  {
                jwp->host = 0L;
                cp = jnum;
        }
        if  (!numeric(cp)  ||  (jwp->jno = (jobno_t) atol(cp)) == 0)  {
                disp_str = jnum;
                return  $E{Not numeric argument};
        }
        return  0;
}

const HashBtjob *find_job(struct jobswanted *jw)
{
        LONG  jind;

        jlock();
        jind = Job_seg.hashp_jno[jno_jhash(jw->jno)];

        while  (jind != JOBHASHEND)  {
                const  HashBtjob  *hjp = &Job_seg.jlist[jind];
                if  (hjp->j.h.bj_job == jw->jno  &&  hjp->j.h.bj_hostid == jw->host)  {
                        junlock();
                        jw->jp = &hjp->j;
                        return  hjp;
                }
                jind = hjp->nxt_jno_hash;
        }
        junlock();
        jw->jp = (CBtjobRef) 0;
        return  (HashBtjob *) 0;
}

/* Decode permission flags in CGI routines */

void  decode_permflags(USHORT *arr, char *str, const int off, const int isjob)
{
        USHORT  *ap;

        arr[0] = arr[1] = arr[2] = 0;

        while  (*str)  {
                switch  (*str++)  {
                default:        return;
                case  'U':case  'u':    ap = &arr[0];  break;
                case  'G':case  'g':    ap = &arr[1];  break;
                case  'O':case  'o':    ap = &arr[2];  break;
                }
                switch  (*str++)  {
                default:        return;
                case  'R':case  'r':
                        if  (off)
                                *ap |= BTM_READ|BTM_WRITE;
                        else
                                *ap |= BTM_SHOW|BTM_READ;
                        break;

                case  'W':case  'w':
                        if  (off)
                                *ap |= BTM_WRITE;
                        else
                                *ap |= BTM_SHOW|BTM_READ|BTM_WRITE;
                        break;

                case  'S':case  's':
                        if  (off)
                                *ap |= BTM_SHOW|BTM_READ|BTM_WRITE;
                        else
                                *ap |= BTM_SHOW;
                        break;

                case  'M':case  'm':
                        if  (off)
                                *ap |= BTM_RDMODE|BTM_WRMODE;
                        else
                                *ap |= BTM_RDMODE;
                        break;

                case  'P':case  'p':
                        if  (off)
                                *ap |= BTM_WRMODE;
                        else
                                *ap |= BTM_RDMODE|BTM_WRMODE;
                        break;

                case  'U':case  'u':    *ap |= BTM_UGIVE;       break;
                case  'V':case  'v':    *ap |= BTM_UTAKE;       break;
                case  'G':case  'g':    *ap |= BTM_GGIVE;       break;
                case  'H':case  'h':    *ap |= BTM_GTAKE;       break;
                case  'D':case  'd':    *ap |= BTM_DELETE;      break;

                case  'K':case  'k':
                        if  (isjob)  {
                                *ap |= BTM_KILL;
                                break;
                        }
                        return;
                }
        }
}
