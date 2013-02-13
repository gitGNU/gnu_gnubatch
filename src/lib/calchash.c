/* calchash.c -- hash function for variable names

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
#include <sys/ipc.h>
#include <sys/shm.h>
#include "defaults.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"

unsigned  calchash(const char *cp)
{
        ULONG  result = 0;

        while  (*cp)
                result = (result << 5) ^ *cp++ ^ ((result >> 27) & 0x1f);
        return  result % VAR_HASHMOD;
}
