/* btvar.h -- Batch variable structure

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

struct	vident  {
	netid_t		hostid;					// Originating host id - never zero
	slotno_t	slotno;					// SHM slot number on machine
	int operator == (const vident &oth) { return hostid == oth.hostid && slotno == oth.slotno; }
	vident()	{ hostid = slotno = 0L; }
};

struct	Btvar  :  public  vident  {
	unsigned char	var_type;				// Is it special?
	unsigned char	var_flags; 				// If so, set read-only
	char			var_name[BTV_NAME+1];	// Name
	char			var_comment[BTV_COMMENT+1];	// User-assigned comment
	Btmode			var_mode;				// Permissions
	Btcon			var_value;				// Value
};

//	Values for var_type to indicate special (non-zero)

#define VT_LOADLEVEL		1			// Maximum Load Level
#define VT_CURRLOAD		2			// Current load level
#define VT_LOGJOBS		3			// Log jobs
#define VT_LOGVARS		4			// Log vars
#define	VT_MACHNAME		5			// Machine name
#define	VT_STARTLIM		6			// Max number of jobs to start at once
#define	VT_STARTWAIT		7			// Wait time

//	Values for var_flags

#define	VF_READONLY		0x01
#define	VF_STRINGONLY		0x02
#define	VF_LONGONLY		0x04
#define	VF_EXPORT		0x08		// Visible to outside world
#define	VF_SKELETON		0x10		// Skeleton variable for offline host
#define	VF_CLUSTER		0x20		// Clustered variable
