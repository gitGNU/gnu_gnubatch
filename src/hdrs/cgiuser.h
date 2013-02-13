/* cgiuser.h -- remember user for CGI routines

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

struct  cgiuser  {
        int_ugid_t      uid;            /* User id to be used */
        ULONG           key;            /* Random key */
        time_t          alloc;          /* Date/time allocated */
        netid_t         desthost;       /* Host id 0=localhost */
        LONG            reserved[4];    /* Reserve for future expansion */
};

extern  char            *dest_hostname;
extern  netid_t         dest_hostid;

extern  netid_t my_look_hostname(const char *);
extern  char    **cgi_arginterp(const int, char **, const int);

/* Following parameters may be passed as 3rd arg to cgi_arginterp */

#define CGI_AI_SUBSID   1       /* Indicates subsidiary program */
#define CGI_AI_REMHOST  2       /* Indicates using remote host routines */

#define UFILEN_PARAM    "userfile"
#define TOREF_PARAM     "userrefresh"
#define DEFLT_UPARAM    "defltuser"
#define DEFLT_TMOPARAM  "usertimeout"
#define DEFLT_HOSTPARAM "deflthost"

#define DEFLT_TIMEOUT   1
