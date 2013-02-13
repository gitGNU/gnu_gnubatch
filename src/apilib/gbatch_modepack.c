/* gbatch_modepack.c -- API function to pack job/variable permissions

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

void  gbatch_mode_pack(Btmode *dest, const Btmode *src)
{
        strcpy(dest->o_user, src->o_user);
        strcpy(dest->c_user, src->c_user);
        strcpy(dest->o_group, src->o_group);
        strcpy(dest->c_group, src->c_group);
        dest->u_flags = htons(src->u_flags);
        dest->g_flags = htons(src->g_flags);
        dest->o_flags = htons(src->o_flags);
}

void  gbatch_mode_unpack(Btmode *dest, const Btmode *src)
{
        strcpy(dest->o_user, src->o_user);
        strcpy(dest->c_user, src->c_user);
        strcpy(dest->o_group, src->o_group);
        strcpy(dest->c_group, src->c_group);
        dest->u_flags = ntohs(src->u_flags);
        dest->g_flags = ntohs(src->g_flags);
        dest->o_flags = ntohs(src->o_flags);
}
