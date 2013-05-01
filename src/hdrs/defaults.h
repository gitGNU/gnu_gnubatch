/* defaults.h -- Various default values

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

#define UIDSIZE         11              /* Size of UID field */
#define WUIDSIZE        23              /* Size of Windows name field */
#define HOSTNSIZE       30              /* Host name size */

#define CMODE   0600

typedef LONG    jobno_t;
typedef LONG    netid_t;
typedef LONG    slotno_t;               /* May be -ve   */
typedef LONG    vhash_t;

#define JN_INC  80000                   /*  Add this to job no if clashes */

/*      We define these as longs to take care of whatever values
        get thrown at as especially by Infernal B*llsh*t Merchants */

typedef LONG    int_ugid_t;
typedef LONG    int_pid_t;

/*      Initial amounts of shared memory to allocate for jobs/vars */

#define INITNJOBS       INITJALLOC
#define INITNVARS       50
#define XBUFJOBS        (NUMSEMA4-5)            /* Number of slots in transport buff */

/*      Amounts to grow by  */

#define INCNJOBS        (INITJALLOC/2)
#define INCNVARS        20

#define NETTICKLE       1000                    /* Keep networks alive */

#define RECURSE_MAX     10              /* Maximum level of recursive expansion of $-style constructs */

/*      Timezone - change this as needed (mostly needed for DOS version). */

#define DEFAULT_TZ      "TZ=GMT0BST"

/*      Default maximum load level */

#define SYSDF_MAXLL     20000

/*      Initial values for start limit and wait  */

#define INIT_STARTLIM   15
#define INIT_STARTWAIT  30

#define DEF_LUMPSIZE    20              /* Number of jobs/vars to send at once on startup */
#define DEF_LUMPWAIT    2               /* Time to wait in each case */

#define MSGQ_BLOCKS     30              /* Number of times we try message queue */
#define MSGQ_BLOCKWAIT  10              /* Sleep time between message queue tries */

/*      Space in a job structure to allow for strings and vectors
        We can be a little bit more profligate than we used to be in
        the days when we passed it in the job. This fell foul of the
        small limit on IPC message buffer size.
        We now pass a pointer in a shared memory segment
*/

#ifdef  ENVSIZE
#define JOBSPACE        (ENVSIZE+500)
#endif
