/* dumpecrun.c -- dump out various job parameters.

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

void  dumpecrun(FILE *xfl, CBtjobRef jp)
{
        int     hrs, mns, secs;
        spitbtrstr($A{btr arg umask}, xfl, 0);
        fprintf(xfl, "%.3o \\\n", jp->h.bj_umask);
        spitbtrstr($A{btr arg ulimit}, xfl, 0);
        fprintf(xfl, "%ld \\\n", (long) jp->h.bj_ulimit);
        spitbtrstr($A{btr arg exits}, xfl, 0);
        fprintf(xfl, "N%d:%d \\\n", jp->h.bj_exits.nlower, jp->h.bj_exits.nupper);
        spitbtrstr($A{btr arg exits}, xfl, 0);
        fprintf(xfl, "E%d:%d \\\n", jp->h.bj_exits.elower, jp->h.bj_exits.eupper);

        spitbtrstr($A{btr arg deltime}, xfl, 0);
        fprintf(xfl, " %u \\\n", jp->h.bj_deltime);

        spitbtrstr($A{btr arg runtime}, xfl, 0);
        putc(' ', xfl);
        hrs = jp->h.bj_runtime / 3600L;
        mns = (jp->h.bj_runtime % 3600L) / 60;
        secs = jp->h.bj_runtime % 60;
        if  (hrs > 0)
                fprintf(xfl, "%d:", hrs);
        if  (hrs > 0 || mns > 0)
                fprintf(xfl, "%.2d:", mns);

        fprintf(xfl, "%.2d \\\n", secs);

        spitbtrstr($A{btr arg wsig}, xfl, 0);
        fprintf(xfl, " %u \\\n", jp->h.bj_autoksig);

        spitbtrstr($A{btr arg grace}, xfl, 0);
        putc(' ', xfl);
        mns = jp->h.bj_runon / 60;
        secs = jp->h.bj_runon % 60;
        if  (mns > 0)
                fprintf(xfl, "%d:", mns);
        fprintf(xfl, "%.2d \\\n", secs);
}
