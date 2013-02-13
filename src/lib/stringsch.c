/* stringsch.c -- Compare two job structures to see if strings are different.

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
#include <sys/types.h>
#include "defaults.h"
#include "btmode.h"
#include "timecon.h"
#include "btconst.h"
#include "bjparam.h"
#include "btjob.h"
#include "incl_unix.h"

int  stringsch(BtjobRef job1, BtjobRef job2)
{
        char    *d1, *d2;
        unsigned        cnt;

        /* Do the easy bits */

        if  (job1->h.bj_nargs != job2->h.bj_nargs  ||
             job1->h.bj_nenv  != job2->h.bj_nenv   ||
             job1->h.bj_nredirs != job2->h.bj_nredirs ||
             strcmp(title_of(job1), title_of(job2)) != 0)
                return  1;

        d1 = job1->h.bj_direct < 0? "/": &job1->bj_space[job1->h.bj_direct];
        d2 = job2->h.bj_direct < 0? "/": &job2->bj_space[job2->h.bj_direct];
        if  (strcmp(d1, d2) != 0)
                return  1;

        /* Arguments */

        for  (cnt = 0;  cnt < job1->h.bj_nargs;  cnt++)
                if  (strcmp(ARG_OF(job1, cnt), ARG_OF(job2, cnt)) != 0)
                        return  1;

        /* Environment */

        for  (cnt = 0;  cnt < job1->h.bj_nenv;  cnt++)  {
                char    *n1, *n2, *v1, *v2;
                ENV_OF(job1, cnt, n1, v1);
                ENV_OF(job2, cnt, n2, v2);
                if  (strcmp(n1, n2) != 0  ||  strcmp(v1, v2) != 0)
                        return  1;
        }

        /* Redirections */

        for  (cnt = 0;  cnt < job1->h.bj_nredirs;  cnt++)  {
                RedirRef  r1, r2;
                r1 = REDIR_OF(job1, cnt);
                r2 = REDIR_OF(job2, cnt);
                if  (r1->fd != r2->fd  || r1->action != r2->action)
                        return  1;
                if  (r1->action == RD_ACT_CLOSE)
                        continue;
                if  (r1->arg != r2->arg)
                        return  1;
                if  (r1->action >= RD_ACT_CLOSE)
                        continue;
                if  (strcmp(&job1->bj_space[r1->arg], &job2->bj_space[r2->arg]) != 0)
                        return  1;
        }

        /* Gasp....  */

        return  0;
}
