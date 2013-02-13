/* bjparam.h -- Various system parameters and constants mostly for jobs

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

#define CI_MAXNAME      15              /* Max name size cmd interp */
#define CI_MAXFPATH     75              /* Max path cmd interp */
#define CI_MAXARGS      27              /* Max args space cmd interp */

#define MAXCVARS        10              /* Max number of conditions in job */
#define MAXSEVARS       8               /* Max number of assignments */

/*      These are "as big as they can ever be" */

#define MAXJREDIRS      JOBSPACE/sizeof(Redir)
#define MAXJENVIR       JOBSPACE/sizeof(Envir)
#define MAXJARGS        JOBSPACE/sizeof(Jarg)

/*      Condition types. */

#define C_UNUSED        0
#define C_EQ            1
#define C_NE            2
#define C_LT            3
#define C_LE            4
#define C_GT            5
#define C_GE            6
#define NUM_CONDTYPES   6

/*      Condition critical.*/

#define CCRIT_NORUN     0x01            /*  Do not run if remote unavailable */
#define CCRIT_NONAVAIL  0x40            /*  Local var not avail */
#define CCRIT_NOPERM    0x80            /*  Local var not avail */

/*      Assignment flags */

#define BJA_START       0x01
#define BJA_OK          0x02
#define BJA_ERROR       0x04
#define BJA_ABORT       0x08
#define BJA_CANCEL      0x10
#define BJA_REVERSE     0x1000          /*  Do the opposite at EOJ */

/*      Assignment types  */

#define BJA_NONE        0               /*  Does not apply */
#define BJA_ASSIGN      1               /*  Assign (reverse set zero) */
#define BJA_INCR        2               /*  Increment by.... */
#define BJA_DECR        3               /*  Decrement by.... */
#define BJA_MULT        4               /*  Multiply by.... */
#define BJA_DIV         5               /*  Divide by.... */
#define BJA_MOD         6               /*  Remainder by.... */
#define BJA_SEXIT       7               /*  Set exit code */
#define BJA_SSIG        8               /*  Set signal number */
#define NUM_ASSTYPES    8               /*  Number of different types */

/*      Assignment critical */

#define ACRIT_NORUN     0x01            /*  Do not run if remote unavailable */
#define ACRIT_NONAVAIL  0x40            /*  Unavailable local variable */
#define ACRIT_NOPERM    0x80            /*  No permissions local variable */

/*      Job progress codes */

#define BJP_NONE        0               /*  Nothing done yet */
#define BJP_DONE        1               /*  Done once ok */
#define BJP_ERROR       2               /*  Done but gave error */
#define BJP_ABORTED     3               /*  Done but aborted by oper */
#define BJP_CANCELLED   4               /*  Cancelled before it could run */
#define BJP_STARTUP1    5               /*  Currently starting - phase 1 */
#define BJP_STARTUP2    6               /*  Currently starting - phase 2 */
#define BJP_RUNNING     7               /*  Currently running */
#define BJP_FINISHED    8               /*  Finished ok */

/*      Flags for bj_jflags  */

#define BJ_WRT          (1 << 0)        /*  Send message to users terminal  */
#define BJ_MAIL         (1 << 1)        /*  Mail message to user  */
#define BJ_NOADVIFERR   (1 << 3)        /*  No advance time if error*/
#define BJ_EXPORT       (1 << 4)        /*  Job is visible from outside world */
#define BJ_REMRUNNABLE  (1 << 5)        /*  Job is runnable from outside world */
#define BJ_CLIENTJOB    (1 << 6)        /*  From client host */
#define BJ_ROAMUSER     (1 << 7)        /*  Roaming user */

/*      Flags for bj_jrunflags */

#define BJ_PROPOSED     (1 << 0)        /*  Remote job proposed - inhibits further propose */
#define BJ_SKELHOLD     (1 << 1)        /*  Job depends on unaccessible remote var */
#define BJ_AUTOKILLED   (1 << 2)        /*  Initial kill applied */
#define BJ_AUTOMURDER   (1 << 3)        /*  Final kill applied */
#define BJ_HOSTDIED     (1 << 4)        /*  Murdered because host died */
#define BJ_FORCE        (1 << 5)        /*  Force job to run NB moved from bj_jflags */
#define BJ_FORCENA      (1 << 6)        /*  Do not advance time on Force job to run */
#define BJ_PENDKILL     (1 << 7)        /*  Pending kill */

/*      Exit code range structure */

typedef struct  {               /* Shuffled round to guarantee 4 byte size */
        unsigned  char  nlower; /* Normal exit lower range */
        unsigned  char  nupper; /* Normal exit upper range */
        unsigned  char  elower; /* Error exit lower range */
        unsigned  char  eupper; /* Error exit upper range */
}  Exits;
