/* network.h -- Network processing

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

/* Remote lock return codes */

#define CLOCK_NOFORK    (-1)
#define CLOCK_MADECHILD 0
#define CLOCK_AMCHILD   1
#define CLOCK_OK        2

struct  remote  {
        char    hostname[HOSTNSIZE];    /* Actual host name (alternatively user name) */
        char    alias[HOSTNSIZE];       /* Alias for it (alternatively group name) */
        SHORT   sockfd;                 /* Socket fd to talk to it */
        netid_t  hostid;                /* Host id in network byte order */
        struct  remote  *hash_next;     /* Other remotes in same hash bucket */
        USHORT  ht_flags;               /* Host-type flags */
#define HT_PROBEFIRST   (1 << 1)        /* Probe connection first */
#define HT_MANUAL       (1 << 2)        /* Manual connection only */
#define HT_DOS          (1 << 3)        /* DOS or external client system */
#define HT_PWCHECK      (1 << 4)        /* Check password of user */
#define HT_ROAMUSER     (1 << 5)        /* Roaming user */
#define HT_TRUSTED      (1 << 6)        /* Trusted host */
        USHORT          stat_flags;     /* State flags in scheduler */
#define SF_ISCLIENT     (1 << 0)        /* Set to indicate "I" am client */
#define SF_PROBED       (1 << 1)        /* Set to indicate probe sent */
#define SF_CONNECTED    (1 << 2)        /* Connection complete */
#define SF_NOTSERVER    (1 << 3)        /* Not a server so don't expect printers etc */
        enum  sync_state { NSYNC_NONE = 0, NSYNC_REQ = 1, NSYNC_OK = 2 } is_sync;
        USHORT          ht_timeout;     /* Timeout value (seconds) */
        time_t          lastwrite;      /* When last done */
        int_ugid_t      n_uid;          /* User id for roamuser case */
        int_ugid_t      n_gid;          /* Group id for roamuser case */
        int_ugid_t      remuid;         /* Remote user id for multiple instances */
};

/* Size of hash table used in various places */

#define NETHASHMOD      97              /* Not very big prime number */

/* Fields in hosts file lines */

#define HOSTF_HNAME     0
#define HOSTF_ALIAS     1
#define HOSTF_FLAGS     2
#define HOSTF_TIMEOUT   3

#define NETSHUTSIG      SIGHUP

extern  netid_t         myhostid;

/* Hash function for netid_ts */

extern  unsigned        calcnhash(const netid_t);

/* Functions for manipulating host file */

extern  void            end_hostfile();
extern  struct  remote  *get_hostfile();
extern  void            hash_hostfile();

extern  char             *get_myhostname();                     /* Get my hostname which we standardise */
extern  char            *look_host(const netid_t);              /* Look up external host id */
extern  char             *look_int_host(const netid_t);         /* Look up internal host id */
extern   const  char     *host_prefix_str(const netid_t, const char *);         /* Generate prefix with hostid:name */
extern   const  char     *host_prefix_long(const netid_t, const LONG);                  /* Generate prefix with hostid:value */

extern  netid_t         look_hostname(const char *);            /* Look up host name and return external id */
extern   netid_t         look_int_hostname(const char *);       /* Look up host name and reteurn internal id */

/* We now have routines for this to provide for IPv6 conversion later */

extern   netid_t  sockaddr2int_netid_t(struct sockaddr_in *);
extern   netid_t  ext2int_netid_t(const netid_t);
extern   netid_t  int2ext_netid_t(const netid_t);
