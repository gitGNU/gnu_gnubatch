/* defs.h -- Defaults and defines

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

#ifndef	_NFILE
#define	_NFILE	64
#endif

#define	UIDSIZE		11		/* Size of UID field */
#define	HOSTNSIZE	14		/* Host name size */

#define	BTV_NAME	19		// Max size of variable name
#define	BTV_COMMENT	41      // Max size of variable comment

#define	CMODE	0600

typedef	long	jobno_t;
typedef long    netid_t;
typedef long    slotno_t;               /* May be -ve   */

#define	JN_INC	80000			/*  Add this to job no if clashes */

/*
 *	We define these as unsigned longs to take care of whatever values
 *	get thrown at as especially by Infernal B*llsh*t Merchants
 */

typedef	long	int_ugid_t;
typedef	long	int_pid_t;

/*
 *	Initial amounts of shared memory to allocate for jobs/vars
 */

#define	INITNJOBS	20
#define	INITNVARS	20
#define	XBUFJOBS	5			/* Number of slots in transport buff */

/*
 *	Amounts to grow by
 */

#define	INCNJOBS	10
#define	INCNVARS	10

#define	NETTICKLE	1000			/* Keep networks alive */

/*
 *	Timezone - change this as needed (mostly needed for DOS version).
 */

#define DEFAULT_TZ	"TZ=GMT0BST"

/*
 *	Default maximum load level
 */

#define	SYSDF_MAXLL	20000

/*
 *	Space in a job structure to allow for strings and vectors
 *	We can be a little bit more profligate than we used to be in
 *	the days when we passed it in the job. This fell foul of the
 *	small limit on IPC message buffer size.
 *	We now pass a pointer in a shared memory segment
 */

#ifndef	ENVSIZE
#define	ENVSIZE		5000
#endif

#define	JOBSPACE	(ENVSIZE + 500)

// Default ports

#define	DEF_LISTEN_PORT	2050
#define	DEF_PROBE_PORT	2050
#define	DEF_FEEDER_PORT	2150
#define	DEF_CLIENT_PORT	2250
#define	DEF_APITCP_PORT 2260
#define	DEF_APIUDP_PORT	2260
