/* btmode.h -- permissions

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

struct  Btmode	{
	int_ugid_t	o_uid, o_gid, c_uid, c_gid;	//  Dummies in DOS vn
	char		o_user[UIDSIZE+1], 	// Owner
			o_group[UIDSIZE+1],
			c_user[UIDSIZE+1],	// Creator
			c_group[UIDSIZE+1];
	unsigned  short	u_flags,		// Permissions - user
	 		g_flags,		// Permissions - group
	 		o_flags;		// Permissions - other
	unsigned  short	dummy;
	BOOL	mpermitted(const unsigned) const;
};

//	Flags

#define	BTM_READ	(1 << 0)		// Read thing
#define	BTM_WRITE	(1 << 1)		// Write thing
#define	BTM_SHOW	(1 << 2)		// Show it exists
#define	BTM_RDMODE	(1 << 3)		// Read mode
#define	BTM_WRMODE	(1 << 4)		// Write mode
#define	BTM_UTAKE	(1 << 5)		// Assume user
#define	BTM_GTAKE	(1 << 6)		// Assume group
#define	BTM_UGIVE	(1 << 7)		// Give away user
#define	BTM_GGIVE	(1 << 8)		// Give away group
#define	BTM_DELETE	(1 << 9)		// Delete it
#define	BTM_KILL	(1 << 10)		// Kill it (jobs only)

#define	VALLMODES	0x03FF	 		// Modes for vars
#define	JALLMODES	0x07FF	 		// Modes for jobs

//	Privileges


#define	BTM_ORP_UG	(1 << 8)		// Or user and group permissions
#define	BTM_ORP_UO	(1 << 7)		// Or user and other permissions
#define	BTM_ORP_GO	(1 << 6)		// Or group and other permissions
#define	BTM_SSTOP	(1 << 5)		// Stop the thing
#define	BTM_UMASK	(1 << 4)		// Change privs
#define	BTM_SPCREATE	(1 << 3)		// Special create
#define	BTM_CREATE	(1 << 2)		// Create anything
#define	BTM_RADMIN	(1 << 1)		// Read admin priv
#define	BTM_WADMIN	(1 << 0)		// Write admin priv

#define	NUM_PRIVBITS	9
#define	ALLPRIVS	((1 << NUM_PRIVBITS) -1)// Privs

int	checkminmode(const unsigned, const unsigned, const unsigned);
