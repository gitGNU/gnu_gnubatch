/* calcharge.c -- calculate "fee" from charge log file

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
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "btuser.h"
#include "files.h"
#include "incl_unix.h"

double  calccharge(const int_ugid_t uid)
{
	int			fd;
	LONG			had = 0;
	char			*fname = envprocess(CHFILE);
	double			result = 0.0;
	struct	btcharge	rec;

	fd = open(fname, O_RDONLY);
	free(fname);
	if  (fd < 0)
		return  0;

	while  (read(fd, (char *) &rec, sizeof(rec)) == sizeof(rec))
		switch  (rec.btch_what)  {
		case  BTCH_RECORD:	/* Record left by btshed */
			if  (rec.btch_user == uid)  {
				double	res = rec.btch_pri;
				res /= U_DF_DEFP;
				result += res * res * (double) rec.btch_ll * rec.btch_runtime / 3600.0;
				had++;
			}
			break;

		case  BTCH_FEE:			/* Impose fee */
			if  (rec.btch_user == uid)  {
				result += rec.btch_runtime;
				had++;
			}
			break;

		case  BTCH_FEEALL:		/* Impose fee to all current */
			if  (had)
				result += rec.btch_runtime;
			break;

		case  BTCH_CONSOL:		/* Consolidation of previous charges */
			if  (rec.btch_user == uid)  {
				result = rec.btch_runtime;
				had++;
			}
			break;

		case  BTCH_ZERO:		/* Zero record for given user */
			if  (rec.btch_user == uid)  {
				result = 0.0;
				had++;
			}
			break;

		case  BTCH_ZEROALL:		/* Zero record for all users */
			result = 0.0;
			break;
		}

	close(fd);
	return  result;
}
