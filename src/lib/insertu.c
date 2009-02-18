/* insertu.c -- insert user permission record into file

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

/* Insert updated user descriptor into file.  */

void  insertu(const int fid, CBtuserRef item)
{
	long	pos;
	BtuserRef	p1, p2, t;
	Btuser	a, b;

	/* If it's below maximum for vector, stuff it in.  */

	if  ((ULONG) item->btu_user < SMAXUID)  {
		pos = sizeof(Btdef) + item->btu_user * sizeof(Btuser);
		goto  done;
	}

	/* Look for the user id.  */

	pos = sizeof(Btdef) + sizeof(Btuser) * SMAXUID;

	lseek(fid, pos, 0);
	while  (read(fid, (char *) &a, sizeof(Btuser)) == sizeof(Btuser))  {
		if  (a.btu_user == item->btu_user)
			break;
		if  ((ULONG) a.btu_user > (ULONG) item->btu_user)
			goto  slide;
		pos += sizeof(Btuser);
	}
	goto  done;

	/* Passed the desired user id, so move all the others down.  */

 slide:
	p1 = &a;
	p2 = &b;

	while  (read(fid, (char *) p2, sizeof(Btuser)) == sizeof(Btuser))  {
		lseek(fid, -(long) sizeof(Btuser), 1);
		write(fid, (char *) p1, sizeof(Btuser));
		t = p1;
		p1 = p2;
		p2 = t;
	}
	write(fid, (char *) p1, sizeof(Btuser));
 done:
	lseek(fid, pos, 0);
	write(fid, (char *) item, sizeof(Btuser));
}
