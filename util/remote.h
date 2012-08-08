/* remote.h -- structure for host details in hostedit

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

struct	remote	{
	/* NB we assume in various places that HOSTNSIZE > UIDSIZE */
	char	hostname[HOSTNSIZE];	/* Actual host name (alternatively user name) */
	char	alias[HOSTNSIZE];	/* Alias for within software */
#if	HOSTNSIZE > UIDSIZE
	char	dosuser[HOSTNSIZE+1];
#else
	char	dosuser[UIDSIZE+1];
#endif
	netid_t hostid;			/* Host id in network byte order */
	unsigned  short	ht_flags;	/* Host-type flags */
#define	HT_HOSTISIP	(1 << 0)	/* Set to indicate host name given as IP address */
#define	HT_PROBEFIRST	(1 << 1)	/* Probe connection first */
#define	HT_MANUAL	(1 << 2)	/* Manual connection only */
#define	HT_DOS		(1 << 3)	/* DOS or external client system */
#define	HT_PWCHECK	(1 << 4)	/* Check password of user */
#define HT_ROAMUSER	(1 << 5)	/* Roaming user */
#define	HT_TRUSTED	(1 << 6)	/* Trusted host */
	unsigned short	ht_timeout;	/* Timeout value (seconds) */
};
