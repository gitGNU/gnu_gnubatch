/* cgifndjb.h -- remember jobs wanted for CGI routines

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
        jobno_t         jno;            /* Job number */
        netid_t         host;           /* Host id */
        CBtjobRef       jp;             /* Job structure pointer */
};

extern  int     decode_jnum(char *, struct jobswanted *);
extern  const   HashBtjob *find_job(struct jobswanted *);
extern  void    decode_permflags(USHORT *, char *, const int, const int);
