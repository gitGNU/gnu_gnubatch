/* mpermitted.c -- see if permissions permit operation

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
#include "incl_ugid.h"
#include "btmode.h"

/* Real[gu]id must be set up right before this is called!  */

int  mpermitted(CBtmodeRef md, const unsigned flag, const ULONG Fileprivs)
{
        USHORT  uf = md->u_flags;
        USHORT  gf = md->g_flags;
        USHORT  of = md->o_flags;

        if  (Fileprivs & BTM_ORP_UG)  {
                uf |= gf;
                gf |= uf;
        }
        if  (Fileprivs & BTM_ORP_UO)  {
                uf |= of;
                of |= uf;
        }
        if  (Fileprivs & BTM_ORP_GO)  {
                gf |= of;
                of |= gf;
        }
        if  (md->o_uid == Realuid)
                return  (uf & flag) == flag;
        if  (md->o_gid == Realgid)
                return  (gf & flag) == flag;
        return  (of & flag) == flag;
}
