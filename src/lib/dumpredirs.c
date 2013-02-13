/* dumpredirs.c -- dump redirections

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
#include <sys/types.h>
#include "defaults.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"

void  spitbtrstr(const int, FILE *, const int);

void  dumpredirs(FILE *xfl, CBtjobRef jp)
{
        int     cnt;

        spitbtrstr($A{btr arg cancio}, xfl, 1);
        for  (cnt = 0;  cnt < jp->h.bj_nredirs;  cnt++)  {
                RedirRef        rp = REDIR_OF(jp, cnt);

                spitbtrstr($A{btr arg io}, xfl, 0);
                putc('\'', xfl);

                switch  (rp->action)  {
                case  RD_ACT_RD:
                        if  (rp->fd != 0)
                                fprintf(xfl, "%d", rp->fd);
                        putc('<', xfl);
                        break;
                case  RD_ACT_WRT:
                        if  (rp->fd != 1)
                                fprintf(xfl, "%d", rp->fd);
                        putc('>', xfl);
                        break;
                case  RD_ACT_APPEND:
                        if  (rp->fd != 1)
                                fprintf(xfl, "%d", rp->fd);
                        fputs(">>", xfl);
                        break;
                case  RD_ACT_RDWR:
                        if  (rp->fd != 0)
                                fprintf(xfl, "%d", rp->fd);
                        fputs("<>", xfl);
                        break;
                case  RD_ACT_RDWRAPP:
                        if  (rp->fd != 0)
                                fprintf(xfl, "%d", rp->fd);
                        fputs("<>>", xfl);
                        break;
                case  RD_ACT_PIPEO:
                        if  (rp->fd != 1)
                                fprintf(xfl, "%d", rp->fd);
                        putc('|', xfl);
                        break;
                case  RD_ACT_PIPEI:
                        if  (rp->fd != 0)
                                fprintf(xfl, "%d", rp->fd);
                        fputs("<|", xfl);
                        break;
                case  RD_ACT_CLOSE:
                        if  (rp->fd != 1)
                                fprintf(xfl, "%d", rp->fd);
                        fputs(">&-\' \\\n", xfl);
                        continue;
                case  RD_ACT_DUP:
                        if  (rp->fd != 1)
                                fprintf(xfl, "%d", rp->fd);
                        fprintf(xfl, ">&%d\' \\\n", rp->arg);
                        continue;
                }
                fprintf(xfl, "%s\' \\\n", &jp->bj_space[rp->arg]);
        }
}
