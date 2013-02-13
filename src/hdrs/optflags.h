/* optflags.h -- Variables to record which parameters got set

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

extern  ULONG   Anychanges;

#define OF_ANY_DOING_SOMETHING  0x1
#define OF_ANY_DOING_CHANGE     0x2
#define OF_ANY_FREEZE_WANTED    0x4
#define OF_ANY_FREEZE_CURRENT   0x8
#define OF_ANY_FREEZE_HOME      0x10

extern  ULONG   Procparchanges;

#define OF_ADVT_CHANGES         0x1
#define OF_ULIMIT_CHANGES       0x2
#define OF_UMASK_CHANGES        0x4
#define OF_DELTIME_SET          0x8
#define OF_RUNTIME_SET          0x10
#define OF_WHICHSIG_SET         0x20
#define OF_GRACETIME_SET        0x40
#define OF_DIR_CHANGES          0x80
#define OF_EXPORT_CHANGES       0x100
#define OF_EXIT_CHANGES         0x200
#define OF_ARG_CHANGES          0x400
#define OF_ARG_CLEAR            0x800
#define OF_ENV_CHANGES          0x1000
#define OF_IO_CHANGES           0x2000
#define OF_IO_CLEAR             0x4000
#define OF_INTERP_CHANGES       0x8000
#define OF_LOADLEV_CHANGES      0x10000
#define OF_PRIO_CHANGES         0x20000
#define OF_MAIL_CHANGES         0x40000
#define OF_WRITE_CHANGES        0x80000
#define OF_TITLE_CHANGES        0x100000
#define OF_JOBQUEUE_CHANGES     0x200000
#define OF_PROGRESS_CHANGES     0x400000
#define OF_MODE_CHANGES         0x800000
#define OF_OWNER_CHANGE         0x1000000
#define OF_GROUP_CHANGE         0x2000000
#define OF_HOST_CHANGE          0x4000000

extern  ULONG   Condasschanges;

#define OF_COND_CHANGES         0x1
#define OF_ASS_CHANGES          0x2
#define OF_CRIT_COND            0x4
#define OF_CRIT_ASS             0x8

extern  ULONG   Timechanges;

#define OF_TIMES_CHANGES        0x1
#define OF_DATESET              0x2
#define OF_REPEAT_CHANGES       0x4
#define OF_MDSET                0x8
#define OF_AVOID_CHANGES        0x10
#define OF_NPOSS_CHANGES        0x20

extern  ULONG   Dispflags;

#define DF_SUPPNULL             0x1
#define DF_LOCALONLY            0x2
#define DF_HELPCLR              0x4
#define DF_HELPBOX              0x8
#define DF_ERRBOX               0x10
#define DF_SCRKEEP              0x20
#define DF_CONFABORT            0x40
#define DF_HAS_HDR              0x80

extern  char    *Restru,
                *Restrg,
                *jobqueue,
                *job_title,
                *job_cwd,
                *exitcodename,
                *signalname;

extern  char    Mode_set[];

extern  char    *Args[];                /* Define this here for clients */
