/* rcgilib.h -- library functions for remote CGI routines

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

struct  jobswanted      {
        slotno_t        slot;           /* Slot number */
        jobno_t         jno;            /* Job number */
        netid_t         host;           /* Host id */
};

struct  var_with_slot  {
        slotno_t        slot;
        apiBtvar        var;
};

extern int  sort_j(apiBtjob *, apiBtjob *);
extern int  sort_v(struct var_with_slot *, struct var_with_slot *);
extern int  numeric(const char *);
extern int  decode_jnum(char *, struct jobswanted *);
extern apiBtvar *find_var(const slotno_t);
extern int  find_var_by_name(const char **, struct var_with_slot *);
extern void  api_open(char *);
extern void  api_readvars(const unsigned);
extern int  rjarg_queue(apiBtjob *, char *);
extern int  rjarg_title(apiBtjob *, char *);
extern void  decode_permflags(USHORT *, char *, const int, const int);

extern  int             xbapi_fd;
extern  apiBtuser       userpriv;

extern  int                     Nvars;
extern  struct  var_with_slot   *var_sl_list;
