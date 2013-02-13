/* q_shm.h -- format of structures for remembering shm info

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

struct  jshm_hdr        {
        USHORT          js_type;        /*  Indicate it's a job segment */
        SHORT           js_viewport;    /*  Internet Port for viewing jobs   */
        unsigned        js_nxtid;       /*  Set to next id if new allocated */
        unsigned        js_njobs;       /*  Number of jobs in queue  */
        unsigned        js_maxjobs;     /*  Max number  */
        USHORT          js_freech;      /*  Free chain */
        time_t          js_lastwrite;   /*  Last written out to file */
        ULONG           js_serial;      /*  Incremented every time written */
        USHORT          js_q_head,      /*  Head of job queue */
                        js_q_tail;      /*  Tail of job queue */
};

struct  vshm_hdr        {
        USHORT          vs_type;        /*  Indicate it's a var segment */
        USHORT          vs_info;        /*  Info to pass to user */
        unsigned        vs_nxtid;       /*  Set to next id if new allocated */
        unsigned        vs_nvars;       /*  Number of vars */
        unsigned        vs_maxvars;     /*  Max number  */
        vhash_t         vs_freech;      /*  Free chain -1 if none */
        time_t          vs_lastwrite;   /*  Last written out to file */
        ULONG           vs_serial;      /*  Incremented every time written */
        netid_t         vs_lockid;      /*  Host id of remote locker */
};

#define TY_ISJOB        0
#define TY_ISVAR        1

struct  btshm_info  {
#ifdef  USING_MMAP
        int             mmfd;           /* File descriptor */
#else
#ifdef  USING_FLOCK
        int             lockfd;         /* Lock file fd */
#endif
        int             base,           /* Key base for allocating shared memory segs */
                        chan;           /* Current id */
#endif
        char            *seg;           /* Current base address from shmat or mmap */
        ULONG           segsize;        /* Size of segment / file */
        ULONG           reqsize;        /* Requested segment size */
};

struct  jshm_info  {
        struct  btshm_info      inf;    /* As above */
        struct  jshm_hdr        *dptr;  /* Pointer to jshm_hdr (pointer into segment) */
        USHORT          	*hashp_pid;     /* Hash by pids */
        USHORT          	*hashp_jno;     /* Hash by job numbers */
        USHORT          	*hashp_jid;     /* Hash by job idents */
        HashBtjob               *jlist; /* Vector of job pointers */
        unsigned                njobs,  /* Number we can see */
                                Njobs;  /* Number there really are */
};

struct  vshm_info  {
        struct  btshm_info     inf;    /* As above */
        struct  vshm_hdr       *dptr;  /* Pointer to vshm_hdr (pointer into segment) */
        vhash_t                *vhash; /* Hash by var name */
        vhash_t                *vidhash;       /* Hash by var id */
        struct  Ventry         *vlist; /* List of variables */
        unsigned               nvars,  /* Variables we are looking at */
                               Nvars;  /* Maximum possible variables */
};

extern  struct  jshm_info       Job_seg;
extern  struct  vshm_info       Var_seg;
