/* spit_pparm.c -- output option values for process parameters

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
#include "helpargs.h"
#include "optflags.h"

void  spit_pparm(FILE *dest, CBtjobhRef jp, const ULONG ppc)
{
        if  (ppc & OF_UMASK_CHANGES)  {
                spitoption($A{btr arg umask}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %.3o", jp->bj_umask);
        }
        if  (ppc & OF_ULIMIT_CHANGES)  {
                spitoption($A{btr arg ulimit}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %ld", (long) jp->bj_ulimit);
        }
        if  (ppc & OF_EXIT_CHANGES)  {
                spitoption($A{btr arg exits}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " N%d:%d", jp->bj_exits.nlower, jp->bj_exits.nupper);
                spitoption($A{btr arg exits}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " E%d:%d", jp->bj_exits.elower, jp->bj_exits.eupper);
        }
        if  (ppc & OF_DELTIME_SET) {
                spitoption($A{btr arg deltime}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", jp->bj_deltime);
        }
        if  (ppc & OF_RUNTIME_SET) {
                int     hrs, mns, secs;
                spitoption($A{btr arg runtime}, $A{btr arg explain}, dest, ' ', 0);
                putc(' ', dest);
                hrs = jp->bj_runtime / 3600L;
                mns = (jp->bj_runtime % 3600L) / 60;
                secs = jp->bj_runtime % 60;
                if  (hrs > 0)
                        fprintf(dest, "%d:", hrs);
                if  (hrs > 0 || mns > 0)
                        fprintf(dest, "%.2d:", mns);
                fprintf(dest, "%.2d", secs);
        }
        if  (ppc & OF_WHICHSIG_SET)  {
                spitoption($A{btr arg wsig}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %u", jp->bj_autoksig);
        }
        if  (ppc & OF_GRACETIME_SET) {
                int     mns, secs;
                spitoption($A{btr arg grace}, $A{btr arg explain}, dest, ' ', 0);
                putc(' ', dest);
                mns = jp->bj_runon / 60;
                secs = jp->bj_runon % 60;
                if  (mns > 0)
                        fprintf(dest, "%d:", mns);
                fprintf(dest, "%.2d", secs);
        }
}
