/* dumpconds.c -- dump out condition values.

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
#include <ctype.h>
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "q_shm.h"
#include "spitrouts.h"
#include "optflags.h"

void  spitbtrstr(const int, FILE *, const int);

void  dumpconds(FILE *ofl, JcondRef jcp)
{
        int     jn;

        spitbtrstr($A{btr arg canccond}, ofl, 1);

        for  (jn = 0;  jn < MAXCVARS;  jcp++, jn++)  {
                BtvarRef        vp;
                if  (jcp->bjc_compar == C_UNUSED)
                        return;
                vp = &Var_seg.vlist[jcp->bjc_varind].Vent;
                if  (vp->var_id.hostid)  {
                        spitbtrstr(jcp->bjc_iscrit & CCRIT_NORUN? $A{btr arg condcrit}: $A{btr arg nocondcrit}, ofl, 0);
                        spitbtrstr($A{btr arg cond}, ofl, 0);
                        fprintf(ofl, "\'%s:%s%s", look_host(vp->var_id.hostid), vp->var_name, condname[jcp->bjc_compar - C_EQ]);
                }
                else  {
                        spitbtrstr($A{btr arg cond}, ofl, 0);
                        fprintf(ofl, "\'%s%s", vp->var_name, condname[jcp->bjc_compar - C_EQ]);
                }
                if  (jcp->bjc_value.const_type == CON_LONG)
                        fprintf(ofl, "%ld\' \\\n", (long) jcp->bjc_value.con_un.con_long);
                else  {
                        if  (isdigit(jcp->bjc_value.con_un.con_string[0]))
                                putc(':', ofl);
                        fprintf(ofl, "%s\' \\\n", jcp->bjc_value.con_un.con_string);
                }
        }
}

void  dumpasses(FILE *ofl, JassRef jap)
{
        int     jn;

        spitbtrstr($A{btr arg cancset}, ofl, 1);

        for  (jn = 0;  jn < MAXSEVARS;  jap++, jn++)  {
                BtvarRef        vp;
                if  (jap->bja_op == BJA_NONE)
                        return;
                spitbtrstr($A{btr arg setflags}, ofl, 0);
                if  (jap->bja_flags)  {
                        if  (jap->bja_flags & BJA_START)
                                putc('S', ofl);
                        if  (jap->bja_flags & BJA_REVERSE)
                                putc('R', ofl);
                        if  (jap->bja_flags & BJA_OK)
                                putc('N', ofl);
                        if  (jap->bja_flags & BJA_ERROR)
                                putc('E', ofl);
                        if  (jap->bja_flags & BJA_ABORT)
                                putc('A', ofl);
                        if  (jap->bja_flags & BJA_CANCEL)
                                putc('C', ofl);
                }
                else
                        putc('-', ofl);

                vp = &Var_seg.vlist[jap->bja_varind].Vent;
                if  (vp->var_id.hostid)
                        spitbtrstr(jap->bja_iscrit & ACRIT_NORUN? $A{btr arg asscrit} : $A{btr arg noasscrit}, ofl, 0);
                spitbtrstr($A{btr arg set}, ofl, 0);
                if  (jap->bja_op >= BJA_SEXIT)  {
                        char    *msg = jap->bja_op == BJA_SEXIT? exitcodename: signalname;
                        fprintf(ofl, "\'%s=%s\' \\\n", VAR_NAME(vp), msg);
                }
                else  {
                        fprintf(ofl, "\'%s%s", VAR_NAME(vp), assname[jap->bja_op-BJA_ASSIGN]);
                        if  (jap->bja_con.const_type == CON_LONG)
                                fprintf(ofl, "%ld\' \\\n", (long) jap->bja_con.con_un.con_long);
                        else  {
                                if  (isdigit(jap->bja_con.con_un.con_string[0]))
                                        putc(':', ofl);
                                fprintf(ofl, "%s\' \\\n", jap->bja_con.con_un.con_string);
                        }
                }
        }
}
