/* spit_cond.c -- output condition structure

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
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "helpargs.h"
#include "spitrouts.h"
#include "optflags.h"

void spit_cond(FILE *dest, const unsigned comp, const unsigned crit, const netid_t vhost, char *vname, BtconRef value)
{
        if  (vhost)  {
                spitoption(crit & CCRIT_NORUN? $A{btr arg condcrit}: $A{btr arg nocondcrit},
                                $A{btr arg explain}, dest, ' ', 0);
                spitoption($A{btr arg cond}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s:%s%s", look_host(vhost), vname, condname[comp-C_EQ]);
        }
        else  {
                spitoption($A{btr arg cond}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s%s", vname, condname[comp-C_EQ]);
        }
        dumpcon(dest, value);
}

void  spit_ass(FILE *dest, const unsigned assop, const unsigned flags, const unsigned crit, const netid_t vhost, char *vname, BtconRef value)
{
        if  (vhost)
                spitoption(crit & ACRIT_NORUN? $A{btr arg asscrit}: $A{btr arg noasscrit}, $A{btr arg explain}, dest, ' ', 0);
        if  (assop >= BJA_SEXIT)  {
                spitoption($A{btr arg set}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s=%s", host_prefix_str(vhost, vname),  assop == BJA_SEXIT? exitcodename: signalname);
        }
        else  {
                if  (flags)  {
                        spitoption($A{btr arg setflags}, $A{btr arg explain}, dest, ' ', 0);
                        if  (flags & BJA_START)
                                putc('S', dest);
                        if  (flags & BJA_REVERSE)
                                putc('R', dest);
                        if  (flags & BJA_OK)
                                putc('N', dest);
                        if  (flags & BJA_ERROR)
                                putc('E', dest);
                        if  (flags & BJA_ABORT)
                                putc('A', dest);
                        if  (flags & BJA_CANCEL)
                                putc('C', dest);
                }
                spitoption($A{btr arg set}, $A{btr arg explain}, dest, ' ', 0);
                fprintf(dest, " %s%s", host_prefix_str(vhost, vname), assname[assop-BJA_ASSIGN]);
                dumpcon(dest, value);
        }
}
