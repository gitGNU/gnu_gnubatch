/* btjob.h -- Details of Batch jobs.

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

#include "btenvir.h"
#include "btredir.h"

typedef struct  {
        unsigned char   bjc_compar;     /*  Comparison as: */
        unsigned char   bjc_iscrit;     /*  Critical */
        vhash_t         bjc_varind;     /*  Index of variable */
        Btcon           bjc_value;      /*  Value to compare */
}  Jcond, *JcondRef;

typedef const   Jcond   *CJcondRef;

/* Assignment structures */

typedef struct  {
        USHORT          bja_flags;      /*  When it applies */
        unsigned  char  bja_op;         /*  What to do */
        unsigned  char  bja_iscrit;     /*  Critical */
        vhash_t         bja_varind;     /*  Variable index */
        Btcon           bja_con;
}  Jass, *JassRef;

typedef const   Jass    *CJassRef;

typedef struct  {
        jobno_t         bj_job;         /*  Job number  */
        jobno_t         bj_sonum;       /*  Standard output id */
        jobno_t         bj_senum;       /*  Standard error id */
        time_t          bj_time;        /*  When originally submitted  */
        time_t          bj_stime;       /*  Time started */
        time_t          bj_etime;       /*  Last end time */
        int_pid_t       bj_pid;         /*  Process id if running */
        netid_t         bj_orighostid;  /*  Original hostid (for remotely submitted jobs) */
        netid_t         bj_hostid;      /*  Host id job belongs to */
        netid_t         bj_runhostid;   /*  Host id job running on (might be different) */
        slotno_t        bj_slotno;      /*  Slot number */

        unsigned  char  bj_progress;    /*  As below */
        unsigned  char  bj_pri;         /*  Priority  */
        SHORT           bj_wpri;        /*  Working priority  */

        USHORT          bj_ll;          /*  Load level */
        USHORT          bj_umask;       /*  Copy of umask */

        USHORT          bj_nredirs,     /*  Number of redirections */
                        bj_nargs,       /*  Number of arguments */
                        bj_nenv;        /*  Environment */

        unsigned  char  bj_jflags;      /*  Flags */
        unsigned  char  bj_jrunflags;   /*  Run flags*/

        SHORT           bj_title;       /*  Name of job (offset) */
        SHORT           bj_direct;      /*  Directory (offset) */

        ULONG           bj_runtime;     /*  Run limit (secs) */
        USHORT          bj_autoksig;    /*  Auto kill sig before size 9s applied */
        USHORT          bj_runon;       /*  Grace period (secs) */
        USHORT          bj_deltime;     /*  Delete job automatically */
        SHORT           bj_padding;     /*  Up to long boundary */

        char            bj_cmdinterp[CI_MAXNAME+1];     /*  Command interpreter (now a string) */
        Btmode          bj_mode;                /*  Permissions */
        Jcond           bj_conds[MAXCVARS];     /*  Conditions */
        Jass            bj_asses[MAXSEVARS];    /*  Assignments */
        Timecon         bj_times;       /*  Time control */
        LONG            bj_ulimit;      /*  Copied ulimit */

        SHORT           bj_redirs;      /* Redirections (offset of vector) */
        SHORT           bj_env;         /* Environment (offset of vector) */
        SHORT           bj_arg;         /* Max args (offset of vector) */
        USHORT          bj_lastexit;    /* Last exit status (as returned by wait) */

        Exits           bj_exits;
}  Btjobh, *BtjobhRef;

typedef const   Btjobh  *CBtjobhRef;

/* We split off the header stuff from the rest
   to avoid uselessly sending all of bj_space down the
   network when we don't have to. */

typedef struct  {
        Btjobh          h;
        char            bj_space[JOBSPACE];     /* Room for chars of char strings */
}  Btjob, *BtjobRef;

typedef const   Btjob   *CBtjobRef;

typedef struct  {
        USHORT  nxt_pid_hash,           /* Next on hash chain by pid */
                prv_pid_hash,           /* Previous on hash chain by pid */
                nxt_jno_hash,           /* Next on hash chain by job number */
                prv_jno_hash,           /* Previous on hash chain by job number */
                nxt_jid_hash,           /* Next on hash chain by jident */
                prv_jid_hash;           /* Previous on hash chain by jident */
        USHORT  q_nxt, q_prv;           /* Job queue location */
        Btjob   j;
}  HashBtjob;

#define JOBHASHMOD      10007           /* Prime number extracted from atmosphere */
#define JOBHASHEND      0xFFFF          /* Denotes end of hash */

/* Buffer for shoving such things around */

struct  Transportbuf    {
        ULONG   Next;                   /* Index of next to use */
        Btjob   Ring[XBUFJOBS];         /* Defined in limits.h */
};

extern  struct  Transportbuf    *Xbuffer;

typedef struct  {
        netid_t         hostid; /* Originating host id - never zero */
        slotno_t        slotno; /* SHM slot number on machine */
}  jident;

extern  int     packjstring(BtjobRef, const char *, const char *, MredirRef, MenvirRef, char **);
extern  int     repackjob(BtjobRef, CBtjobRef, const char *, const char *, const unsigned, const unsigned, const unsigned, MredirRef, MenvirRef, char **);
extern  const char *title_of(CBtjobRef);

#define pid_jhash(pid)  (((unsigned) (pid)) % JOBHASHMOD)
#define jno_jhash(jno)  (((unsigned) (jno)) % JOBHASHMOD)
#define jid_jhash(jidp) (((((unsigned) (jidp)->hostid) >> 16) ^ ((unsigned) (jidp)->hostid) ^ (((unsigned) ((jidp)->slotno)) >> 4) ^ ((unsigned) ((jidp)->slotno))) % JOBHASHMOD)

#define  JOBH_NUMBER(JHP)         host_prefix_long((JHP)->bj_hostid, (LONG) (JHP)->bj_job)
#define  JOB_NUMBER(JP)         JOBH_NUMBER(&((JP)->h))

extern  char    *getscript(BtjobRef, unsigned *);


