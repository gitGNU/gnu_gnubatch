/* ipcstuff.h -- IPC ids and such

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

#define IPC_ID(appl, no)        (('X'<<24)|('i'<<16)|((appl)<<12)|no)
#define XBT_ID(no)              IPC_ID(11, no)  /* Added 8 to application number for net batch */

#define MSGID                   XBT_ID(0)
#define SEMID                   XBT_ID(1)
#define SHMID                   XBT_ID(2)       /*  A base  */
#define NETSEMID                XBT_ID(3)
#define TRANSHMID               XBT_ID(0x100)   /*  Transport buffer */
#define FMSHMID                 XBT_ID(0x200)   /*  Data for filemon */
#define FMSEMID                 XBT_ID(0x201)   /*  Semaphore for filemon */
#define SEMNUMS                 5               /*  + XBUFJOBS */
#define MAXSHMS                 100             /*  Before wrapping  */
#define JSHMOFF                 0               /*  Offset on base for jobs */
#define VSHMOFF                 1               /*  Offset on base for vars */
#define SHMINC                  2               /*  Number to increment by */
#define ENV_INC_MULT            1024            /*  Multiply env no by this */

#define MSGBUFFSIZE             2000            /*  Max size of message*/

/* Indices into semaphore array for various ops */

#define JQ_FIDDLE               0
#define JQ_READING              1
#define VQ_FIDDLE               2
#define VQ_READING              3
#define TQ_INDEX                4               /* Lock index into transport buffer */

#define QREWRITE                300             /* Seconds to queue rewrite */

/* Some funny machines (e.g. Sequent) don't define this */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif
