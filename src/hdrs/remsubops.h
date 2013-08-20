/* remsubops.h -- remote job submission routines.

   Copyright 2013 Free Software Foundation, Inc.

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

#ifndef REMSUBOPS_H_
#define REMSUBOPS_H_

extern  int  remsub_initsock(int *, const netid_t, struct sockaddr_in *);
extern  int  remsub_udp_enquire(const int, struct sockaddr_in *, char *, const int, char *, const int);
extern  void  remsub_unpack_btuser(Btuser *, const Btuser *);
extern  int     remsub_inittcp(int *);
extern  int     remsub_opentcp(const netid_t, const int, int *);
extern  int  remsub_sock_read(const int, char *, int);
extern  int  remsub_sock_write(const int, char *, int);
extern  unsigned        remsub_packjob(struct nijobmsg *, CBtjobRef);
extern  void    remsub_condasses(struct nijobmsg *, CBtjobRef, const netid_t);
extern  int     remsub_startjob(const int, const int, const char *, const char *);
extern  void    remsub_copyout(FILE *, const int, const char *, const char *);
extern  void    remsub_copyout_str(char *, const int, const char *, const char *);
#endif /* REMSUBOPS_H_ */
