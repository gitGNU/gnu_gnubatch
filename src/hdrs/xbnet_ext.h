/* xbnet_ext.h -- extern definitions for xbnetserv

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

#define NAMESIZE        14                      /* Padding for temp file */

struct  pend_job        {
        netid_t         clientfrom;             /* Who is responsible (or 0) */
        FILE            *out_f;                 /* Output file */
        char            tmpfl[NAMESIZE+1];      /* Temporary file */
        USHORT          joblength;              /* Length expected all told */
        USHORT          lengthexp;              /* Length expected (of job descriptor) */
        char            *cpos;                  /* Pointer into jobout */
        unsigned  char  prodsent;               /* Sent a prod */
        time_t          lastaction;             /* Last message received (for timeout) */
        jobno_t         jobn;                   /* Job number we are creating */
        struct nijobmsg jobin;                  /* Job details pending */
        Btjob           jobout;                 /* Constructed job */
};

/*  Structure used to hash host ids  */

struct  hhash   {
        struct  hhash   *hn_next;       /* Next in hash chain */
        netid_t         hostid;         /* Remote structure */
        int             isme;           /* Is local host */
        int             isclient;       /* Is windows client*/
};

/* Structure used to map Windows user names to UNIX ones */

struct  winuhash  {
        struct  winuhash  *next;                /* Next in hash chain */
        char            *winname;               /* Windows name */
        char            *unixname;              /* UNIX name */
        char            *unixgroup;             /* Group name primary group */
        int_ugid_t      uuid;                   /* UNIX user id */
        int_ugid_t      ugid;                   /* UNIX primary group id */
};

/* Structure to hold details of auto-login hosts (this should probably be removed) */

struct  alhash  {
        struct  alhash  *next;                  /* Next in hash chain */
        netid_t         hostid;                 /* Host id */
        char            *unixname;              /* Unix user name */
        char            *unixgroup;             /* Unix group name */
        int_ugid_t      uuid;                   /* Unix uid */
        int_ugid_t      ugid;                   /* UNIX primary group id */
};

#ifndef _NFILE
#define _NFILE  64
#endif

#define MAX_PEND_JOBS   (_NFILE / 3)

#define MAXTRIES        5
#define TRYTIME         20

/* It seems to take more than one attempt to set up a UDP port at times,
   so.... */

#define UDP_TRIES       3

#define JOB_MOD 60000                   /*  Modulus of job numbers */

extern  char    *Defaultuser, *Defaultgroup;
extern  int_ugid_t  Defaultuid, Defaultgid;
extern  SHORT   qsock, uasock, apirsock;
extern  SHORT   qportnum, uaportnum, apirport, apipport;
extern  USHORT  err_which;
extern  USHORT  orig_umask;
extern  unsigned  myhostl;
extern  unsigned  timeouts;
extern  char    *myhostname;
extern  int     Ctrl_chan;
extern  long    mymtype;
extern  int     hadrfresh;
extern  struct  pend_job        pend_list[];
extern  struct  hhash   *nhashtab[];

extern  void  abort_job(struct pend_job *);
extern  void  btuser_pack(BtuserRef, BtuserRef);
extern  void  get_hf(const unsigned, char *);
extern  void  put_hf(const unsigned, char *);
extern  void  open_cifile(const unsigned);
extern  void  process_api();
extern  void  process_ua();
extern  void  send_askall();
extern  void  tell_friends(struct hhash *);
extern  void  tell_myself(struct hhash *);
extern  int  tcp_serv_accept(const int, netid_t *);
extern  int  validate_job(BtjobRef, const Btuser *);
extern  int  checkpw(const char *, const char *);
extern  int  convert_username(struct hhash *, struct ni_jobhdr *, BtjobRef, BtuserRef *);
extern  int  chk_vgroup(const char *, const char *);

extern  unsigned  process_alarm();
extern  unsigned  unpack_cavars(BtjobRef, const struct nijobmsg *);
extern  unsigned  unpack_job(BtjobRef, const struct nijobmsg *, const unsigned, const netid_t);
extern  unsigned  calc_clu_hash(const char *);

extern  struct alhash *find_autoconn(const netid_t);
extern  struct winuhash *lookup_winu(const char *);
extern   struct winuhash *lookup_winoruu(const char *);
extern  struct  hhash  *lookup_hhash(const netid_t);
extern  struct pend_job *add_pend(const netid_t);
extern  struct pend_job *find_j_by_jno(const jobno_t);
extern  FILE *goutfile(jobno_t *, char *, const int);
