/* shreq.h -- Requests to batch scheduler and replies

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

#define SYS_REQ         0               /* System-level requests */
#define SYS_REPLY       0x040           /* Replies regarding system req */
#define JOB_REQ         0x100           /* Requests regarding jobs */
#define JOB_REPLY       0x140           /* Replies regarding jobs */
#define VAR_REQ         0x200           /* Requests regarding vars */
#define VAR_REPLY       0x240           /* Replies regarding vars */
#define NET_REQ         0x300           /* Network request */
#define NET_REPLY       0x340           /* Network reply */

#define J_NOREPLY       0               /* Do not reply to caller */
#define REP_AMPARENT    J_NOREPLY       /* Am parent process - no reply yet */
#define REQ_TYPE        0xFC0           /* Mask for request type */
#define SHREQ_CODE      0x0FFF          /* Mask of child bit */
#define SHREQ_CHILD     0x1000          /* Reply comes from child process */

/* System requests */

#define O_LOGON         (SYS_REQ|1)     /* Log on to system */
#define O_LOGOFF        (SYS_REQ|2)     /* Log off */
#define O_STOP          (SYS_REQ|3)     /* Halt */
#define O_PWCHANGED     (SYS_REQ|4)     /* Password changed */

/* Replies */

#define O_OK            (SYS_REPLY|0)   /* All ok */
#define O_CSTOP         (SYS_REPLY|1)   /* Pass on stop message */
#define O_JREMAP        (SYS_REPLY|2)   /* Job remap message */
#define O_VREMAP        (SYS_REPLY|3)   /* Var remap message */
#define O_NOPERM        (SYS_REPLY|4)   /* Operation not permitted */

/* Job requests */

#define J_CREATE        (JOB_REQ|1)     /* Queue job */
#define J_DELETE        (JOB_REQ|2)     /* Delete job */
#define J_CHANGE        (JOB_REQ|3)     /* Change job */
#define J_CHOWN         (JOB_REQ|4)     /* Change owner */
#define J_CHGRP         (JOB_REQ|5)     /* Change group */
#define J_CHMOD         (JOB_REQ|6)     /* Change modes */
#define J_KILL          (JOB_REQ|7)     /* Kill/abort job */
#define J_FORCE         (JOB_REQ|8)     /* Force job */
#define J_FORCENA       (JOB_REQ|9)     /* Force job no advance time */
#define J_CHANGED       (JOB_REQ|10)    /* Remote wants to change job */
#define J_HCHANGED      (JOB_REQ|11)    /* Remote wants to change job header */
#define J_BCHANGED      (JOB_REQ|12)    /* Remote advises changes to job */
#define J_BHCHANGED     (JOB_REQ|13)    /* Remote advises changes to job header */
#define J_BOQ           (JOB_REQ|14)    /* Remote advises job moved to back of queue */
#define J_BFORCED       (JOB_REQ|15)    /* Remote advises job forced */
#define J_BFORCEDNA     (JOB_REQ|16)    /* Remote advises job forced - no adv time */
#define J_DELETED       (JOB_REQ|17)    /* Remote advises Delete job */
#define J_CHMOGED       (JOB_REQ|18)    /* Remote advises change mode/owner/group */

#define J_STOK          (JOB_REQ|19)    /* Child process - started ok */
#define J_NOFORK        (JOB_REQ|20)    /* Child process - fork fail */
#define J_COMPLETED     (JOB_REQ|21)    /* Child process - job completed */

#define J_PROPOSE       (JOB_REQ|22)    /* Propose to execute remote job */
#define J_PROPOK        (JOB_REQ|23)    /* Proposal OK (internal network message)*/
#define J_CHSTATE       (JOB_REQ|24)    /* Network message - job changed state */
#define J_RNOTIFY       (JOB_REQ|25)    /* Network message - notify status */
#define J_RRCHANGE      (JOB_REQ|26)    /* Remote run changed state */

#define J_RESCHED       (JOB_REQ|27)    /* Network message - do resched after var hacks */
#define J_RESCHED_NS    (JOB_REQ|28)    /* Network message - do resched after var hacks no start */

#define J_DSTADJ        (JOB_REQ|29)    /* Adjust DST */

/* Replies */

#define J_OK            (JOB_REPLY|0)   /* Ok */
#define J_NEXIST        (JOB_REPLY|1)   /* Job does not exist */
#define J_VNEXIST       (JOB_REPLY|2)   /* Job variable does not exist */
#define J_NOPERM        (JOB_REPLY|3)   /* Operation not permitted */
#define J_VNOPERM       (JOB_REPLY|4)   /* Operation not permitted on var */
#define J_NOPRIV        (JOB_REPLY|5)   /* Operation not privileged */
#define J_SYSVAR        (JOB_REPLY|6)   /* Affects system var incorrectly */
#define J_SYSVTYPE      (JOB_REPLY|7)   /* Affects system var incorrectly */
#define J_FULLUP        (JOB_REPLY|8)   /* Choc-a-bloc */
#define J_ISRUNNING     (JOB_REPLY|9)   /* Job running */
#define J_REMVINLOCJ    (JOB_REPLY|10)  /* Remote variable in local job */
#define J_LOCVINEXPJ    (JOB_REPLY|11)  /* Local variable in export job */
#define J_MINPRIV       (JOB_REPLY|12)  /* Too few privs */
#define J_ISSKEL        (JOB_REPLY|13)  /* Incomplete job cannot do it */

#define J_START         (JOB_REPLY|16)  /* Start job */

/* Variable related requests */

#define V_CREATE        (VAR_REQ|1)     /* Create variable */
#define V_DELETE        (VAR_REQ|2)     /* Delete variable */
#define V_ASSIGN        (VAR_REQ|3)     /* Assign variable */
#define V_CHOWN         (VAR_REQ|4)     /* Change owner */
#define V_CHGRP         (VAR_REQ|5)     /* Change group */
#define V_CHMOD         (VAR_REQ|6)     /* Change modes */
#define V_CHCOMM        (VAR_REQ|7)     /* Change comment */
#define V_NEWNAME       (VAR_REQ|8)     /* Change name */
#define V_CHFLAGS       (VAR_REQ|9)     /* Change flags */

#define V_DELETED       (VAR_REQ|16)    /* Broadcast variable deleted */
#define V_ASSIGNED      (VAR_REQ|17)    /* Broadcast variable assigned */
#define V_CHMOGED       (VAR_REQ|18)    /* Broadcast variable change mode/owner/group */
#define V_RENAMED       (VAR_REQ|19)    /* Broadcast variable change name */

/* Replies to above */

#define V_OK            (VAR_REPLY|0)   /* OK */
#define V_EXISTS        (VAR_REPLY|1)   /* Variable exists */
#define V_NEXISTS       (VAR_REPLY|2)   /* Variable does not exist */
#define V_CLASHES       (VAR_REPLY|3)   /* Variable clashes with similar */
#define V_NOPERM        (VAR_REPLY|4)   /* Operation not permitted */
#define V_NOPRIV        (VAR_REPLY|5)   /* Operation not privileged */
#define V_SYNC          (VAR_REPLY|6)   /* Conflict with another user */
#define V_SYSVAR        (VAR_REPLY|7)   /* Clashes with system variable */
#define V_SYSVTYPE      (VAR_REPLY|8)   /* System variable wrong type */
#define V_FULLUP        (VAR_REPLY|9)   /* Choc-a-bloc */
#define V_DSYSVAR       (VAR_REPLY|10)  /* Attempt to delete system var */
#define V_INUSE         (VAR_REPLY|11)  /* Variable in use */
#define V_MINPRIV       (VAR_REPLY|12)  /* Too few privs */
#define V_DELREMOTE     (VAR_REPLY|13)  /* Deleting remote var */
#define V_UNKREMUSER    (VAR_REPLY|14)  /* Unknown remote user */
#define V_UNKREMGRP     (VAR_REPLY|15)  /* Unknown remote group */
#define V_RENEXISTS     (VAR_REPLY|16)  /* Variable exists trying to rename */
#define V_NOTEXPORT     (VAR_REPLY|17)  /* Variable is not exported */
#define V_RENAMECLUST   (VAR_REPLY|18)  /* Attempt to rename cluster var */

/* Network requests */

#define N_SHUTHOST      (NET_REQ|0)     /* Tell everyone we're dying */
#define N_ABORTHOST     (NET_REQ|1)     /* Worked out that someone has died */
#define N_NEWHOST       (NET_REQ|2)     /* New host arrived (internal) */
#define N_TICKLE        (NET_REQ|3)     /* Check still alive */
#define N_CONNECT       (NET_REQ|4)     /* Attempt connection */
#define N_DISCONNECT    (NET_REQ|5)     /* Attempt disconnection */
#define N_PCONNOK       (NET_REQ|6)     /* Probe connect ok (internal) */
#define N_REQSYNC       (NET_REQ|7)     /* Request sync */
#define N_ENDSYNC       (NET_REQ|8)     /* End sync */
#define N_REQREPLY      (NET_REQ|9)     /* Reply to user's request */
#define N_DOCHARGE      (NET_REQ|10)    /* Send charge record */
#define N_WANTLOCK      (NET_REQ|11)    /* Want remote lock */
#define N_UNLOCK        (NET_REQ|12)    /* Want remote unlock */
#define N_SYNCSINGLE    (NET_REQ|13)    /* Sync a single job */
#define N_RJASSIGN      (NET_REQ|15)    /* Network message - given job remote assigns */
#define N_XBNATT        (NET_REQ|17)    /* Request to attach xbnetserv process */
#define N_ROAMUSER      (NET_REQ|18)    /* Note user logged in to xbnetserv */
#define N_SETNOTSERVER  (NET_REQ|19)    /* Network message - set not server */
#define N_SETISSERVER   (NET_REQ|20)    /* Network message - unset not server */

#define N_CONNOK        (NET_REPLY|0)   /* Connection ok */
#define N_REMLOCK_NONE  (NET_REPLY|1)   /* Lock - no reply  */
#define N_REMLOCK_OK    (NET_REPLY|2)   /* Lock - done ok */
#define N_REMLOCK_PRIO  (NET_REPLY|3)   /* Lock - other machine has priority */
#define N_NOFORK        (NET_REPLY|4)   /* User reply - no fork */
#define N_NBADMSGQ      (NET_REPLY|5)   /* User reply - bad message queue */
#define N_NTIMEOUT      (NET_REPLY|6)   /* User reply - network timeout */
#define N_HOSTOFFLINE   (NET_REPLY|7)   /* User reply - host off line */
#define N_HOSTDIED      (NET_REPLY|8)   /* User reply - host died */
#define N_CONNFAIL      (NET_REPLY|9)   /* User reply - connection failed */
#define N_WRONGIP       (NET_REPLY|10)  /* Wrong IP address */

/* Message packet to/from scheduler */

#define TO_SCHED        1               /* Message type field to sched */
#define CHILD           2               /* To child `exec' process */
#define MTOFFSET        0x40000         /* Add to pid to get unique type */
#define NTOFFSET        0x80000         /* Add to pid to get unique type net processes */

typedef struct  {
        ULONG           mcode;          /* Message code as above (not the long at the start) */
        int_ugid_t      uuid,           /* Calling user uid these are internal to m/c */
                        ugid;           /* Calling user gid these are internal to m/c */
        int_pid_t       upid;           /* Calling user pid */
        netid_t         hostid;         /* Calling host id 0 means this one */
        ULONG           param;          /* Parameter new mode etc */
}  Shreq, *ShreqRef;

typedef const   Shreq   *CShreqRef;

/* Structure for remote status change.
   This is only used internally not on the network where we use
   struct jobstatmsg */

struct  jstatusmsg      {
        jident          jid;
        time_t          nexttime;
        USHORT          lastexit;
        int_pid_t       lastpid;
        netid_t         runhost;
        unsigned  char  prog;
};

/* Structure for remote assign */

struct  jremassmsg      {
        jident          jid;
        USHORT          flags;
        USHORT          source;
        USHORT          status;
};

struct  jremnotemsg     {
        jident          jid;
        SHORT           msg;
        jobno_t         sout;
        jobno_t         serr;
};

typedef struct  {
        long    sh_mtype;               /* NB this really must be l.c. long */
        Shreq   sh_params;
        union   {
                ULONG   sh_jobindex; /* Index of job in xferbuffer */
                jident  jobref;              /* Ditto when single param */
                Btvar   sh_var;
                struct  {
                        Btvar   sh_ovar;
                        char    sh_rnewname[BTV_NAME+1];
                }  sh_rn;
                struct  adjstr {             /* Sweeping DST adjustment */
                        time_t  sh_starttime;
                        time_t  sh_endtime;
                        LONG    sh_adjust;
                }  sh_adj;
                struct  remote          sh_n;
                struct  jstatusmsg      remstat;
                struct  jremassmsg      remas;
                Cmdint                  cilist[1]; /* List of command interps */
                struct  jremnotemsg     remnote;
        }  sh_un;
}  Shipc, *ShipcRef;

typedef const   Shipc   *CShipcRef;

/* Reply structure for simple codes */

typedef struct  {
        long    mm;                     /* This MUST be a l.c. long */
        Shreq   outmsg;
}  Repmess;

#define RESCHED SIGUSR1                 /* To child process to start job */
#define QRFRESH SIGUSR2                 /* To btq process to note change */

extern unsigned  readreply();
