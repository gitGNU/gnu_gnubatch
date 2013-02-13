/* xbapi_int.h -- internal structures for API

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

struct  api_fd  {
        SHORT           portnum;        /* Port number local byte order */
        SHORT           sockfd;         /* Socket fd */
        SHORT           prodfd;         /* "Prod" socket fd */
        netid_t         hostid;         /* Host we are talking to net byte order */
#ifdef  PYRAMID
/* (Pyramid has bug in C compiler affecting pointers to functions with const parameters) */
        void            (*jobfn)(int);          /* Function to invoke on jobs change */
        void            (*varfn)(int);          /* Function to invoke on vars change */
#else
        void            (*jobfn)(const int);    /* Function to invoke on jobs change */
        void            (*varfn)(const int);    /* Function to invoke on vars change */
#endif
        ULONG           jserial;        /* Serial of jobs */
        ULONG           vserial;        /* Serial of vars */
        unsigned        bufmax;         /* Size of Buffer allocated for list operations */
        char            *queuename;     /* Queue prefix */
        char            *buff;          /* The buffer itself */
};

struct  api_msg {
        unsigned  char  code;           /* Code number see below */
#define API_SIGNON      0
#define API_SIGNOFF     1
#define API_SETQUEUE    2
#define API_JOBLIST     3
#define API_VARLIST     4
#define API_JOBREAD     5
#define API_VARREAD     6
#define API_JOBDEL      7
#define API_VARDEL      8
#define API_JOBOP       9
#define API_JOBADD      10
#define API_DATAIN      11
#define API_DATAEND     12
#define API_DATAABORT   13
#define API_VARADD      14
#define API_JOBUPD      15
#define API_VARUPD      16
#define API_JOBDATA     17
#define API_DATAOUT     18
#define API_JOBCHMOD    19
#define API_JOBCHOWN    20
#define API_JOBCHGRP    21
#define API_VARCHCOMM   22
#define API_VARCHMOD    23
#define API_VARCHOWN    24
#define API_VARCHGRP    25
#define API_VARRENAME   26
#define API_GETBTU      27
#define API_GETBTD      28
#define API_PUTBTU      29
#define API_PUTBTD      30
#define API_CIADD       31
#define API_CIREAD      32
#define API_CIUPD       33
#define API_CIDEL       34
#define API_HOLREAD     35
#define API_HOLUPD      36
#define API_JOBPROD     37
#define API_VARPROD     38
#define API_REQPROD     39
#define API_UNREQPROD   40
#define API_SENDENV     41
#define API_FINDJOBSLOT 42
#define API_FINDVARSLOT 43
#define API_FINDJOB     44
#define API_FINDVAR     45

#define API_LOGIN       50
#define API_NEWGRP      51
#define API_LOCALLOGIN  52
#define API_WLOGIN      53

        char    re_reg;                 /* Re-register on login */
        SHORT   retcode;                /* Error return/0 */
        union  {
                USHORT          queuelength;
                struct  {
                        char    username[WUIDSIZE+1];
                }  signon;
                /* For logins from local host - may specify alternative user */
                struct  {
                        int_ugid_t      fromuser;
                        int_ugid_t      touser;
                }  local_signon;

                struct  {
                        ULONG   flags;
                }  lister;
                struct  {
                        ULONG   nitems;         /* Number of jobs or printers */
                        ULONG   seq;            /* Sequence number */
                }  r_lister;
                struct  {
                        ULONG   flags;
                        ULONG   seq;            /* Sequence number */
                        slotno_t        slotno; /* Slot number */
                }  reader;
                struct  {
                        ULONG   flags;
                        ULONG  seq;             /* Sequence number */
                        slotno_t        slotno; /* Slot number */
                        ULONG   op;
                        ULONG   param;          /* Parameter (just sig no at pres) */
                }  jop;
                struct  {
                        ULONG   seq;
                }  r_reader;
                struct  {
                        char    username[UIDSIZE+1];
                }  us;
                struct  {
                        jobno_t         jobno;
                        ULONG           seq;
                        USHORT          nbytes;
                }  jobdata;
                struct  {
                        ULONG   flags;
                        netid_t netid;
                        jobno_t jobno;
                }  jobfind;
                struct  {
                        ULONG   flags;
                        netid_t netid;
                }  varfind;     /* Followed by var name */
                struct  {
                        ULONG           seq;
                        slotno_t        slotno;
                }  r_find;      /* Possibly followed by job or var */
        }  un;
};

#define API_PASSWDSIZE  127

#define DEFAULT_SERVICE API_DEFAULT_SERVICE
#define MON_SERVICE     API_MON_SERVICE
#define XBA_BUFFSIZE    256

/*ERRSTART*/

/* Error codes */

#define XB_OK                   (0)
#define XB_INVALID_FD           (-1)
#define XB_NOMEM                (-2)
#define XB_INVALID_HOSTNAME     (-3)
#define XB_INVALID_SERVICE      (-4)
#define XB_NODEFAULT_SERVICE    (-5)
#define XB_NOSOCKET             (-6)
#define XB_NOBIND               (-7)
#define XB_NOCONNECT            (-8)
#define XB_BADREAD              (-9)
#define XB_BADWRITE             (-10)
#define XB_CHILDPROC            (-11)

/* These errors should correspond to xbnetq.h sort of  */
#define XB_CONVERT_XBNR(code)   (-20-(code))
#define XB_NOT_USER             (-23)
#define XB_BAD_CI               (-24)
#define XB_BAD_CVAR             (-25)
#define XB_BAD_AVAR             (-26)
#define XB_NOMEM_QF             (-28)
#define XB_NOCRPERM             (-29)
#define XB_BAD_PRIORITY         (-30)
#define XB_BAD_LL               (-31)
#define XB_BAD_USER             (-32)
#define XB_FILE_FULL            (-33)
#define XB_QFULL                (-34)
#define XB_BAD_JOBDATA          (-35)
#define XB_UNKNOWN_USER         (-36)
#define XB_UNKNOWN_GROUP        (-37)
#define XB_ERR                  (-38)
#define XB_NORADMIN             (-39)
#define XB_NOCMODE              (-40)

#define XB_UNKNOWN_COMMAND      (-41)
#define XB_SEQUENCE             (-42)
#define XB_UNKNOWN_JOB          (-43)
#define XB_UNKNOWN_VAR          (-44)
#define XB_NOPERM               (-45)
#define XB_INVALID_YEAR         (-46)
#define XB_ISRUNNING            (-47)
#define XB_NOTIMETOA            (-48)
#define XB_VAR_NULL             (-49)
#define XB_VAR_CDEV             (-50)
#define XB_INVALIDSLOT          (-51)
#define XB_ISNOTRUNNING         (-52)
#define XB_NOMEMQ               (-53)
#define XB_NOPERM_VAR           (-54)
#define XB_RVAR_LJOB            (-55)
#define XB_LVAR_RJOB            (-56)
#define XB_MINPRIV              (-57)
#define XB_SYSVAR               (-58)
#define XB_SYSVTYPE             (-59)
#define XB_VEXISTS              (-60)
#define XB_DSYSVAR              (-61)
#define XB_DINUSE               (-62)
#define XB_DELREMOTE            (-63)

#define XB_NO_PASSWD            (-64)
#define XB_PASSWD_INVALID       (-65)
#define XB_BAD_GROUP            (-66)

#define XB_NOTEXPORT            (-67)
#define XB_RENAMECLUST          (-68)

/* Flags for accessing things */

#define XB_FLAG_LOCALONLY       (1 << 0)
#define XB_FLAG_USERONLY        (1 << 1)
#define XB_FLAG_GROUPONLY       (1 << 2)
#define XB_FLAG_QUEUEONLY       (1 << 3)
#define XB_FLAG_IGNORESEQ       (1 << 4)
#define XB_FLAG_FORCE           (1 << 5)

/* Miscellaneous codes to do job ops with */

#define XB_JOP_SETRUN           1
#define XB_JOP_SETCANC          2
#define XB_JOP_FORCE            3
#define XB_JOP_FORCEADV         4
#define XB_JOP_ADVTIME          5
#define XB_JOP_KILL             6
#define XB_JOP_SETDONE          7

/*ERREND*/
