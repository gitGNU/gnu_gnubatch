/* incl_ugid.h -- uid/gid handling defs

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

extern  void    rgrpfile();
extern  void    rpwfile();
extern  void    un_rpwfile();
extern  void    produser();
extern  int     isvuser(const uid_t);
extern  int_ugid_t      lookup_gname(const char *);
extern  int_ugid_t      lookup_uname(const char *);
extern  char    *homedof(const int_ugid_t);
extern  char    *prin_uname(const uid_t);
extern  char    *prin_gname(const gid_t);
extern  char    *unameproc(char *, const char *, const uid_t);
extern  char    *recursive_unameproc(const char *, const char *, const uid_t);
extern  char    **gen_glist(const char *);
extern  char    **gen_ulist(const char *);
#ifdef  HAVE_GETGROUPS
extern  int     get_suppgrps(const uid_t, gid_t **);
extern  char    Requires_suppgrps;
#endif

/* Unknown user and groups */

#define UNKNOWN_UID     (-1)
#define UNKNOWN_GID     (-1)

extern  uid_t   Daemuid,
                Realuid,
                Effuid;
extern  gid_t   Daemgid, Realgid, Effgid;
extern  int_ugid_t      lastgid;

extern  unsigned  Npwusers;

#ifdef  HAVE_SETEUID
#if     defined(NHONSUID) || defined(DEBUG)
#define INIT_DAEMUID    if  ((LONG) (chk_uid = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)\
                                Daemuid = ROOTID;\
                        else  {\
                                Daemuid = chk_uid;\
                                seteuid(Realuid);\
                        }

#define SCRAMBLID_CHECK if  (Realuid != ROOTID  &&  Effuid != Daemuid  &&  Effuid != ROOTID)  {\
                                print_error(8000);\
                                exit(E_SETUP);\
                        }
#else  /* HAVE_SETEUID normal case no debug */
#define INIT_DAEMUID    Daemuid = Effuid; seteuid(Realuid);
#define SCRAMBLID_CHECK
#endif /* No-honour setuid or debug */

#define SWAP_TO(ID) seteuid(ID)

#else  /* !HAVE_SETEUID */
#ifdef  ID_SWAP
#if     defined(NHONSUID) || defined(DEBUG)
#define INIT_DAEMUID    if  ((LONG) (chk_uid = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)\
                                Daemuid = ROOTID;\
                        else  {\
                                Daemuid = chk_uid;\
                                if  (Effuid != ROOTID)\
                                        setuid(Realuid);\
                        }

#define SCRAMBLID_CHECK if  (Realuid != ROOTID  &&  Effuid != Daemuid  &&  Effuid != ROOTID)  {\
                                print_error(8000);\
                                exit(E_SETUP);\
                        }

#define SWAP_TO(ID) if (Daemuid != ROOTID && Realuid != ROOTID && Effuid != ROOTID) setuid(ID)

#else  /* Not no-honour setuid or debug */

#define INIT_DAEMUID    Daemuid = Effuid;if  (Daemuid != ROOTID) setuid(Realuid);
#define SCRAMBLID_CHECK

#define SWAP_TO(ID) if (Daemuid != ROOTID && Realuid != ROOTID) setuid(ID)
#endif /* No-honour setuid or debug */

#else  /* No ID_SWAP */

#define INIT_DAEMUID    Daemuid = Effuid;
#define SCRAMBLID_CHECK
#define SWAP_TO(ID)
#endif
#endif /* ! HAVE_SETEUID */
