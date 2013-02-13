/* xbnetq.h -- Structure for sending jobs stuff on TCP/UDP interface

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

typedef struct  {
        char    ni_varname[BTV_NAME+1]; /* Name of var */
        netid_t ni_varhost;             /* Host it is on */
}  Nivar;

typedef struct  {
        Nivar   nic_var;                /* Variable we are looking for */
        unsigned char   nic_compar;     /* Comparison */
        unsigned char   nic_iscrit;     /* Critical */
        unsigned char   nic_type;       /* Type of constant */
        unsigned char   nic_padding[1]; /* Pad out to 4 bytes */
        union   {
                char    nic_string[BTC_VALUE+3];
                LONG    nic_long;
        }  nic_un;
}  Nicond;

typedef struct  {
        Nivar           nia_var;        /* Which Variable */
        USHORT          nia_flags;      /* When it applies */
        unsigned  char  nia_op;         /* What to do */
        unsigned  char  nia_iscrit;     /* Critical */
        unsigned  char  nia_type;       /* Type of constant */
        unsigned  char  nia_padding[3]; /* Pad  out to 4 bytes */
        union   {
                char    nia_string[BTC_VALUE+3];
                LONG    nia_long;
        }  nia_un;
}  Niass;

struct  nijobhmsg  {                    /* Structure passed for job create */
        unsigned  char  ni_progress;    /* Only NONE or CANCELLED */
        unsigned  char  ni_pri;
        unsigned  char  ni_jflags;
        unsigned  char  ni_istime;

        unsigned  char  ni_mday;
        unsigned  char  ni_repeat;
        unsigned  char  ni_nposs;
        unsigned  char  ni_padding0;

        SHORT           ni_title,
                        ni_direct,
                        ni_redirs,
                        ni_env,
                        ni_arg;
        USHORT          ni_ll,
                        ni_umask,
                        ni_nvaldays,
                        ni_nredirs,
                        ni_nargs,
                        ni_nenv,
                        ni_autoksig,
                        ni_runon,
                        ni_deltime;

        LONG            ni_ulimit,
                        ni_nexttime;

        ULONG           ni_rate,
                        ni_runtime;

        char            ni_cmdinterp[CI_MAXNAME+1]; /* Pass as char string to save sending ints about */

        Exits           ni_exits;
        Btmode          ni_mode;

        Nicond          ni_conds[MAXCVARS];
        Niass           ni_asses[MAXSEVARS];
};

struct  nijobmsg  {             /* Whole shooting match */
        struct  nijobhmsg       ni_hdr;
        char            ni_space[JOBSPACE]; /* Not necessarily as big as that */
};

struct  client_if       {
        unsigned  char  code;                   /* 0 ok or error code */
        unsigned  char  resvd[3];               /* Padding */
        LONG            param;                  /* Job number/sub-error code/which is wrong */
};

/* OK (or not) */

#define XBNQ_OK         0

/* Error codes */

#define XBNR_UNKNOWN_CLIENT     1
#define XBNR_NOT_CLIENT         2
#define XBNR_NOT_USERNAME       3
#define XBNR_BADCI              4
#define XBNR_BADCVAR            5
#define XBNR_BADAVAR            6
#define XBNR_NOMEM_QF           8
#define XBNR_NOCRPERM           9
#define XBNR_BAD_PRIORITY       10
#define XBNR_BAD_LL             11
#define XBNR_BAD_USER           12
#define XBNR_FILE_FULL          13
#define XBNR_QFULL              14
#define XBNR_BAD_JOBDATA        15
#define XBNR_UNKNOWN_USER       16
#define XBNR_UNKNOWN_GROUP      17
#define XBNR_ERR                18
#define XBNR_NORADMIN           19
#define XBNR_NOCMODE            20
#define	 XBNR_MINPRIV		 21
#define XBNR_EXISTS		 22
#define XBNR_NOTEXPORT		 23
#define XBNR_CLASHES		 24
#define	 XBNR_DSYSVAR		 25
#define	 XBNR_NOPERM		 26
#define	 XBNR_NEXISTS		 27
#define	 XBNR_INUSE		 28


/* UDP interface for RECEIVING data from client.
   The interface which we listen on to accept jobs and enquiries. */

#define CL_SV_UENQUIRY          0       /* Request for permissions (single byte) */
#define CL_SV_STARTJOB          1       /* Start job */
#define CL_SV_CONTJOB           2       /* Continue job */
#define CL_SV_JOBDATA           3       /* Job data */
#define CL_SV_ENDJOB            4       /* End of last job */
#define CL_SV_HANGON            5       /* Hang on for next block of data */

struct  ni_jobhdr       {
        unsigned  char  code;           /* One of above codes */
        unsigned  char  padding;        /* Pad to SHORT align */
        USHORT          joblength;      /* Length of job descriptor */
        char    uname[UIDSIZE+1];       /* User name submitted by */
        char    gname[UIDSIZE+1];       /* Group name submitted by */
};

#define	 CL_SV_CREATEVAR	7	/* Create variable */
#define	 CL_SV_DELETEVAR	8	/* Delete variable */

/* Structure for creating and deleting variables from clients.
   For deleting we only worry about the variable name */

struct	ni_createvar {
	unsigned  char	 code;				/* Code which is always CL_SV_CREATEVAR */
	unsigned  char	 padding;
	USHORT		 msglength;			/* Length which we include for consistency */
	LONG		 nicv_long;			/* Value as long */
	USHORT		 u_flags, g_flags, o_flags;	/* Permisions as per mode */
	unsigned  char   nicv_flags;			/* Possible export or cluster */
	unsigned  char   nicv_consttype;		/* Constant type */
	char		 nicv_string[BTC_VALUE+1];	/* Value as string */
	char             nicv_name[BTV_NAME+1];   	/* Name */
	char             nicv_comment[BTV_COMMENT+1];   /* User-assigned comment */
};

#define CL_SV_ULIST             10      /* Send list of valid users */
#define CL_SV_VLIST             11      /* Send list of valid variables */
#define CL_SV_CILIST            12      /* Send list of command interpreters */
#define CL_SV_HLIST             13      /* Send list of command interpreters */
#define CL_SV_GLIST             14      /* Send list of valid groups */
#define CL_SV_UMLPARS           15      /* Send umask and ulimit */
#define CL_SV_ELIST             16      /* Send environment variables */

#define SV_CL_ACK               0       /* Acknowledge job data (single byte) */
#define SV_CL_TOENQ             20      /* Are you still there? (single byte) */
#define SV_CL_PEND_FULL         21      /* Queue of pending jobs full */
#define SV_CL_UNKNOWNC          22      /* Unknown command */
#define SV_CL_BADPROTO          23      /* Something wrong protocol */
#define SV_CL_UNKNOWNJ          24      /* Out of sequence job */

struct  ua_venq         {               /* Variables or holidays */
        char            uav_code;       /* CL_SV_VLIST or CL_SV_HLIST*/
        char            uav_padding;
        USHORT          uav_perm;       /* Permission flags or year after 1990 */
        char    uname[UIDSIZE+1];       /* User name submitted by or null if sending uid */
        union   {
            char        gname[UIDSIZE+1];/* Group name submitted by */
            int_ugid_t  uav_uid;        /* User id on sending host */
        } uav_un;
};

/* Reply to request for permissions */

struct  ua_reply        {
        char    ua_uname[UIDSIZE+1];
        char    ua_gname[UIDSIZE+1];
        Btuser  ua_perm;
};

#define UA_PASSWDSZ     31

/* ua_login structure has now been enhanced to get the machine name
   in as well.
   We try to support the old procedure (apart from password check)
   as much as possible. */

struct  ua_login        {
        unsigned  char  ual_op;                         /* Operation/result as below */
        unsigned  char  ual_fill;                       /* Filler */
        USHORT          ual_fill1;                      /* Filler */
        char            ual_name[WUIDSIZE+1];           /* User or default user */
        char            ual_passwd[UA_PASSWDSZ+1];      /* Password */
        union   {
            /* We don't actually use the machine name any more but we keep the field
               to make the structure the same length as old versions of things were expecting. */
            char        ual_machname[HOSTNSIZE+2];
            int_ugid_t  ual_uid;                        /* User ID to distinguish UNIX clients */
        }  ua_un;
};

#define UAL_LOGIN       30      /* Log in with user name & password */
#define UAL_LOGOUT      31      /* Log out */
#define UAL_ENQUIRE     32      /* Enquire about user id */
#define UAL_OK          33      /* Logged in ok */
#define UAL_NOK         34      /* Not logged in yet */
#define UAL_INVU        35      /* Not logged in, invalid user */
#define UAL_INVP        36      /* Not logged in, invalid passwd */
#define UAL_NEWGRP      37      /* New group */
#define UAL_INVG        38      /* Invalid group */
#define UAL_ULOGIN      39      /* Log in as UNIX user */
#define UAL_UENQUIRE    40      /* Enquire about user from UNIX */

struct  ua_pal  {               /* Talk to friends */
        unsigned  char  uap_op;         /* Msg - see below */
        unsigned  char  uap_fill;       /* Filler */
        USHORT          uap_fill1;      /* Filler */
        netid_t         uap_netid;      /* IP we'return talking about */
        char            uap_name[UIDSIZE+1];    /* Unix end user */
        char            uap_grp[UIDSIZE+1];     /* Unix end group */
        char            uap_wname[WUIDSIZE+1];  /* Windross server */
};

#define UAU_MAXU        20      /* Limit on number times one user logged in */

struct  ua_asku_rep  {
        USHORT          uau_n;          /* Number of people */
        USHORT          uau_fill;       /* Filler */
        netid_t         uau_ips[UAU_MAXU];
};

#define SV_SV_LOGGEDU   50      /* Confirm OK to other servers */
#define SV_SV_ASKU      51      /* Ask other servers about specific user */
#define SV_SV_ASKALL    52      /* Ask other servers about all users */

#define CL_SV_KEEPALIVE 70      /* Keep connection alive */

/* Reply to request for umask and ulimit */

struct  ua_umlreply     {
        USHORT          ua_umask;       /* Umask */
        USHORT          ua_padding;     /* To long boundary */
        ULONG           ua_ulimit;      /* Ulimit */
};

#define CL_SV_BUFFSIZE  256             /* Buffer for data client/server INCREASE ME!!! */
