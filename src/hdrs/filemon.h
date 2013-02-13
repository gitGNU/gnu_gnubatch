/* filemon.h -- definitions for file monitoring

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

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

enum  wot_prog  {
        WP_SETMONITOR,          /* Default mode - set monitor */
        WP_LISTMONS,            /* List monitor progs */
        WP_KILLMON,             /* Kill monitor prog */
        WP_KILLALL              /* Kill all progs */
};

enum  wot_mode  {
        WM_STOP_FOUND,          /* Stop as soon as file found */
        WM_CONT_FOUND           /* Continue looking for other files */
};

enum  wot_form  {
        WF_APPEARS,             /* Ready when a file appears */
        WF_STOPSGROW,           /* Ready when file stops growing */
        WF_STOPSWRITE,          /* Ready when file stops being written */
        WF_STOPSCHANGE,         /* Ready when file stops being changed */
        WF_STOPSUSE,            /* Ready when file stops being accessed */
        WF_REMOVED              /* Ready when file removed */
};

enum  wot_file  {
        WFL_ANY_FILE,           /* Ready when any file appears/changes */
        WFL_PATT_FILE,          /* Ready when file matching pattern */
        WFL_SPEC_FILE           /* Ready when specified file appears */
};

enum  inc_exist  {
        IE_IGNORE_EXIST,        /* Ignore existing files in search */
        IE_INCL_EXIST           /* Include existing files in search */
};

#define DEF_GROW_TIME   20      /* Default grow time */
#define DEF_POLL_TIME   20      /* Default poll time */
#define MAX_POLL_TIME   3600    /* Maximum poll time */

#define MAX_FM_PROCS    50      /* Maximum number we cope with */

struct  fm_shm_entry  {
        PIDTYPE         fm_pid;         /* Process ID of entry or zero */
        int_ugid_t      fm_uid;         /* Uid of process owner */
        enum  wot_form  fm_typ;         /* Type of monitoring */
        char    fm_path[PATH_MAX];      /* Directory */
};
