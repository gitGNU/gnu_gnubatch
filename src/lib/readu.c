/* readu.c -- read user permissions from file

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
#include "incl_unix.h"
#include "defaults.h"
#include "btuser.h"

/* Find user descriptor in file */

int  readu(const int fid, const int_ugid_t uid, BtuserRef item)
{
	/* If it's below the magic number at which we store them as a
	   vector, jump to the right place and go home.  */

	if  ((ULONG) uid < SMAXUID)  {
		lseek(fid, (long)(sizeof(Btdef) + uid * sizeof(Btuser)), 0);
		if  (read(fid, (char *) item, sizeof(Btuser)) == sizeof(Btuser)  &&  item->btu_isvalid)
			return  1;
		return  0;
	}

	/* Otherwise seek to right place.  */

	lseek(fid, (long)(sizeof(Btdef) + sizeof(Btuser) * SMAXUID), 0);
	while  (read(fid, (char *)item, sizeof(Btuser)) == sizeof(Btuser))  {
		if  ((ULONG) item->btu_user > (ULONG) uid)
			break;
		if  (item->btu_user == uid)
			return  item->btu_isvalid;
	}
	return  0;
}
