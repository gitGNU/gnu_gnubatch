/* hostedit.h -- structure for host details in hostedit/xhostedit

   Copyright 2010 Free Software Foundation, Inc.

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

enum  Stype { SORT_NONE, SORT_HNAME, SORT_IP };
enum  IPatype { NO_IPADDR, IPADDR_IP, IPADDR_NAME, IPADDR_GSN_NAME, IPADDR_GSN_IP };

extern	int	lookslikeip(const char *);
extern	netid_t	getdottedip(const char *);
extern	netid_t gsn_getloc(const int, netid_t, const int);
extern void end_hostfile();
extern void proc_hostfile();
extern void load_hostfile(const char *);
extern void dump_hostfile(FILE *);
extern struct remote *get_hostfile(const char *);
extern	char	*phname(netid_t, const enum IPatype);
extern	void	addhostentry(const struct remote *);
extern	void	sortit();
extern	int	hnameclashes(const char *);
extern  char	*ipclashes(const netid_t);

extern	struct	remote	*hostlist;
extern	int	hostnum, hostmax;
extern	char	defcluser[];
extern	char	gsnname[];
extern	int	gsnport;
extern	netid_t	myhostid, gsnid;
extern	char	hostf_errors;

#define	INITHOSTS	20
#define	INCHOSTS	10

extern	enum  Stype  sort_type;
extern	enum  IPatype  hadlocaddr;
