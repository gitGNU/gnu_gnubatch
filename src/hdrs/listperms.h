/* listperms.h -- for CGI programs (shares stuff with GNUspool)

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

#define LV_MINEORVIEW   0x1     /* Job etc is mine or viewable */
#define LV_LOCORRVIEW   0x2     /* Job is local or remote-viewable */
#define LV_DELETEABLE   0x4     /* Job is deleteable */
#define LV_CHANGEABLE   0x8     /* Job is modifyable */
#define LV_CHOWNABLE    0x10    /* Job is chown-able */
#define LV_CHMODABLE    0x20    /* Job is chmod-able */
#define LV_KILLABLE     0x40    /* Job is killable */

#define LV_PROCESS      0x2000  /* Job/printer has process */
#define LV_WARNOFF      0x4000  /* Job requires less warning */
#define LV_PAGES        0x8000  /* Job is paged */

/* General permissions */

#define GLV_ANYCHANGES  0x1     /* May make changes */
#define GLV_FORMCHANGE  0x2     /* Form change */
#define GLV_PTRCHANGE   0x4     /* Ptr change */
#define GLV_PRIOCHANGE  0x8     /* Priority change */
#define GLV_ANYPRIO     0x10    /* Any priority change */
#define GLV_COVER       0x20    /* Override class */
#define GLV_ACCESSF     0x40    /* Access other fields */
#define GLV_FREEZEF     0x80    /* Save format */
