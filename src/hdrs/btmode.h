/* btmode.h -- define object permissions structure

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

typedef struct  {
        int_ugid_t      o_uid, o_gid,           /* Numeric ones only valid on current machine */
                        c_uid, c_gid;
        char            o_user[UIDSIZE+1],      /* Owner */
                        o_group[UIDSIZE+1],
                        c_user[UIDSIZE+1],      /* Creator */
                        c_group[UIDSIZE+1];
        USHORT          u_flags,                /* Permissions - user */
                        g_flags,                /* Permissions - group */
                        o_flags;                /* Permissions - other */
        USHORT          padding;                /* Pad to long align */
}  Btmode, *BtmodeRef;

typedef const   Btmode  *CBtmodeRef;

/* Mode Flags */

#define BTM_READ_BIT    0
#define BTM_WRITE_BIT   1
#define BTM_SHOW_BIT    2
#define BTM_RDMODE_BIT  3
#define BTM_WRMODE_BIT  4
#define BTM_UTAKE_BIT   5
#define BTM_GTAKE_BIT   6
#define BTM_UGIVE_BIT   7
#define BTM_GGIVE_BIT   8
#define BTM_DELETE_BIT  9
#define BTM_KILL_BIT    10

#define BTM_READ        (1 << BTM_READ_BIT)             /* Read thing */
#define BTM_WRITE       (1 << BTM_WRITE_BIT)            /* Write thing */
#define BTM_SHOW        (1 << BTM_SHOW_BIT)             /* Show it exists */
#define BTM_RDMODE      (1 << BTM_RDMODE_BIT)           /* Read mode */
#define BTM_WRMODE      (1 << BTM_WRMODE_BIT)           /* Write mode */
#define BTM_UTAKE       (1 << BTM_UTAKE_BIT)            /* Assume user */
#define BTM_GTAKE       (1 << BTM_GTAKE_BIT)            /* Assume group */
#define BTM_UGIVE       (1 << BTM_UGIVE_BIT)            /* Give away user */
#define BTM_GGIVE       (1 << BTM_GGIVE_BIT)            /* Give away group */
#define BTM_DELETE      (1 << BTM_DELETE_BIT)           /* Delete it */
#define BTM_KILL        (1 << BTM_KILL_BIT)             /* Kill it (jobs only) */

#define NUM_JMODEBITS   11
#define NUM_VMODEBITS   10

#define VALLMODES       ((1 << NUM_VMODEBITS) - 1)      /* Modes for vars */
#define JALLMODES       ((1 << NUM_JMODEBITS) - 1)      /* Modes for jobs */

/* Privileges */

#define BTM_ORP_UG      (1 << 8)                /* Or user and group permissions */
#define BTM_ORP_UO      (1 << 7)                /* Or user and other permissions */
#define BTM_ORP_GO      (1 << 6)                /* Or group and other permissions */
#define BTM_SSTOP       (1 << 5)                /* Stop the thing */
#define BTM_UMASK       (1 << 4)                /* Change privs */
#define BTM_SPCREATE    (1 << 3)                /* Special create */
#define BTM_CREATE      (1 << 2)                /* Create anything */
#define BTM_RADMIN      (1 << 1)                /* Read admin priv */
#define BTM_WADMIN      (1 << 0)                /* Write admin priv */

#define NUM_PRIVBITS    9
#define ALLPRIVS        ((1 << NUM_PRIVBITS) -1)/* Privs */

/* Options for argument interpretation. */

#define MODE_NONE       0
#define MODE_SET        1
#define MODE_ON         2
#define MODE_OFF        3

extern  int  mpermitted(CBtmodeRef, const unsigned, const ULONG);
