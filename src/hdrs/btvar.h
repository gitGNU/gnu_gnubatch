/* btvar.h -- variable structure

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

#define BTV_NAME        19
#define BTV_COMMENT     41

typedef struct  {
        netid_t         hostid;                 /* Originating host id - never zero */
        slotno_t        slotno;                 /* SHM slot number on machine */
}  vident;

typedef struct  {
        ULONG           var_sequence;           /* Change sequence */
        vident          var_id;
        time_t          var_c_time, var_m_time; /* Create/mod time */
        unsigned char var_type;               /* Is it special? */
        unsigned char var_flags;              /* If so, set read-only */
        char            var_name[BTV_NAME+1];   /* Name */
        char            var_comment[BTV_COMMENT+1];     /* User-assigned comment */
        Btmode          var_mode;               /* Permissions */
        Btcon           var_value;              /* Value */
}  Btvar, *BtvarRef;

typedef const   Btvar   *CBtvarRef;

/* Structure for saving job details in file. We must save the full name
   of variables, with the owner and group, as the index can move between
   saves.  (We don't want to hold the names all the time as they can
   change). */

typedef struct  {
        char            sv_name[BTV_NAME+1];
        netid_t         sv_hostid;
        int_ugid_t      sv_uid, sv_gid;
}  Vref;

/* Values for var_type to indicate special (non-zero) */

#define VT_LOADLEVEL    1                       /* Maximum Load Level */
#define VT_CURRLOAD     2                       /* Current load level */
#define VT_LOGJOBS      3                       /* Log jobs */
#define VT_LOGVARS      4                       /* Log vars */
#define VT_MACHNAME     5                       /* Machine name */
#define VT_STARTLIM     6                       /* Max number of jobs to start at once */
#define VT_STARTWAIT    7                       /* Wait time */

/* Values for var_flags */

#define VF_READONLY     0x01
#define VF_STRINGONLY   0x02
#define VF_LONGONLY     0x04
#define VF_EXPORT       0x08                    /* Visible to outside world */
#define VF_SKELETON     0x10                    /* Skeleton variable for offline host */
#define VF_CLUSTER      0x20                    /* Local to machine in conditions/assignments */

#define MACH_SLOT       (-10)                   /* Marker for "machine name" slot. */

/* This value is a nice prime number pulled out of the air  */

#define VAR_HASHMOD     32563

/* This structure is used to link variables on the hash collision
   chain.  Variables for different users with the same name are
   made adjacent if possible. */

struct  Ventry  {
        vhash_t         Vnext;          /* Remember we can't use pointers */
        vhash_t         Vidnext;        /* This is for hashing by ids */
        int             Vused;          /* In use */
        Btvar           Vent;
};

#define vid_hash(vidp) (((((unsigned) (vidp)->hostid) >> 16) ^ ((unsigned) (vidp)->hostid) ^ (((unsigned) ((vidp)->slotno)) >> 4) ^ ((unsigned) ((vidp)->slotno))) % VAR_HASHMOD)
unsigned  calchash(const char *);

#define  VAR_NAME(VP)           host_prefix_str((VP)->var_id.hostid, (VP)->var_name)
