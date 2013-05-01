/* sh_ext.h -- btsched extern declarations

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

extern  int     Network_ok;     /* 0=Networking turned off even though code there */

extern  PIDTYPE child_pid;      /* Slave process pid */

extern  int     jqpend,         /* Pending changes to job list */
                vqpend,         /* Pending changes to var list */
                qchanges;       /* Pending changes anywhere - notify people */

extern  unsigned        Startlim,       /* Limit on jobs to start */
                        Startwait,      /* Wait time */
                        being_started;  /* Actually being started */

extern  LONG    Max_ll,         /* Maximum load level */
                Current_ll;     /* Current load level */

extern  int     Ctrl_chan;      /* Msgid for message queue */
#ifndef USING_FLOCK
extern  int     Sem_chan;       /* Semid for semaphore list */
#endif

extern  int             nosetpgrp;
extern  float           pri_decrement;

extern  SHORT   viewsock;

extern  USHORT          lportnum,               /* These are all htons-ified */
                        vportnum,
                        pportnum;

extern  int             Netsync_req;
extern  PIDTYPE         Netm_pid;
extern  slotno_t        machname_slot;/* Slot number of "machine name" variable */

extern  unsigned        lumpsize, lumpwait;

extern void  lockjobs();
extern void  lockvars();
extern void  unlockjobs();
extern void  unlockvars();

extern void  adjust_ll(const LONG);
extern void  back_of_queue(const unsigned);
extern void  cannot_start(const unsigned);
extern void  childproc();
extern void  completed_job(const unsigned);
extern void  creatjfile(LONG);
extern void  creatvfile(LONG, const LONG);
extern void  deloper(ShreqRef);
extern void  do_exit(const int) NORETURN_FUNC;
extern void  flushlogs(const int);
extern void  forcemsg(char *, int);
extern void  haltall();
extern void  initcifile();
extern void  initlog();
extern void  initmsgs();
extern void  initumode(uid_t, BtmodeRef);
extern void  jassvar(const vhash_t, BtconRef, unsigned, BtjobRef);
extern void  jbabort(BtjobhRef, int);
extern void  jopvar(const vhash_t, BtconRef, int, unsigned, BtjobRef);
extern void  killops();
extern void  logjob(BtjobRef, unsigned, netid_t, int_ugid_t, int_ugid_t);
extern void  logvar(BtvarRef, unsigned, unsigned, netid_t, int_ugid_t, int_ugid_t, BtjobRef);
extern void  murder(BtjobhRef);
extern void  nfreport(int);
extern void  niceend(int) NORETURN_FUNC;
extern void  notify(BtjobRef, const int, const int);
extern void  openjlog(char *, BtmodeRef);
extern void  openvlog(char *, BtmodeRef);
extern void  panic(int);
extern void  rewrjq();
extern void  rewrvf();
extern void  sendreply(const int_pid_t, const unsigned);
extern void  set_not_server(ShipcRef, const int);
extern void  setasses(BtjobRef, unsigned, unsigned, unsigned, const netid_t);
extern void  started_job(const unsigned);
extern void  tellchild(const unsigned, const ULONG);
extern void  tellopers();
extern void  tellsched(const unsigned, const ULONG);
extern int  addoper(ShreqRef);
extern int  checkminmode(BtmodeRef);
extern int  chjob(ShreqRef, BtjobRef);
extern int  clockvars();
extern int  deljob(ShreqRef, jident *);
extern int  doabort(ShreqRef, jident *);
extern int  doforce(ShreqRef, jident *, const int);
extern int  dstadj(ShreqRef, struct adjstr *);
extern int  enqueue(ShreqRef, BtjobRef);
extern int  findj_by_jid(jident *);
extern int  get_nservers();
extern int  gshmchan(struct btshm_info *, const int);
extern int  isit_dos(const netid_t);
extern int  islogged(uid_t);
extern int  job_chgrp(ShreqRef, jident *);
extern int  job_chmod(ShreqRef, BtjobRef);
extern int  job_chown(ShreqRef, jident *);
extern int  shmpermitted(ShreqRef, BtmodeRef, unsigned);
extern int  ppermitted(uid_t, ULONG);
extern int  reqdforjobs(const vhash_t, const int);
extern int  var_assign(ShreqRef, BtvarRef);
extern int  var_chgrp(ShreqRef, BtvarRef);
extern int  var_chmod(ShreqRef, BtvarRef);
extern int  var_chown(ShreqRef, BtvarRef);
extern int  var_chcomm(ShreqRef, BtvarRef);
extern int  var_chflags(ShreqRef, BtvarRef);
extern int  var_create(ShreqRef, BtvarRef);
extern int  var_delete(ShreqRef, BtvarRef);
extern int  var_rename(ShreqRef, BtvarRef, char *);
extern int  varcomp(JcondRef, BtjobhRef);
extern unsigned  resched();
extern PIDTYPE  forksafe();
extern vhash_t  findvar(const vident *);
extern vhash_t  lookvar(Vref *);
extern BtvarRef  locvarind(ShreqRef, const vhash_t);
extern void  attach_hosts();
extern void  ci_broadcast();
extern void  ci_delhost(const unsigned);
extern void  ci_remaphost(const unsigned, const unsigned, CCmdintRef);
extern void  clearhost(const netid_t, const int);
extern void  deskeletonise(const vhash_t);
extern void  deskel_jobs(const netid_t);
extern void  end_remotelock();
extern void  endsync(const netid_t);
extern void  feed_req();
extern void  forced(jident *, const int);
extern void  get_remotelock();
extern void  init_remotelock();
extern void  initsvmachine();
extern void  jid_pack(jident *, CBtjobhRef);
extern void  job_broadcast(BtjobRef, const unsigned);
extern void  job_deleted(jident *);
extern void  job_hbroadcast(BtjobhRef, const unsigned);
extern void  job_imessage(const netid_t, CBtjobhRef, const unsigned, const LONG);
extern void  job_imessbcast(CBtjobhRef, const unsigned, const LONG);
extern void  job_rrchstat(BtjobhRef);
extern void  job_sendnote(CBtjobRef, const int, const jobno_t, const jobno_t);
extern void  job_statbroadcast(BtjobhRef);
extern void  job_unpack(BtjobRef, const struct jobnetmsg *);
extern void  jobh_pack(struct jobhnetmsg *, CBtjobhRef);
extern void  jobh_unpack(BtjobhRef, const struct jobhnetmsg *);
extern void  lose_remotelock();
extern void  mode_pack(BtmodeRef, CBtmodeRef);
extern void  net_broadcast(const USHORT);
extern void  net_initjsync();
extern void  net_initvsync();
extern void  net_jclear(const netid_t);
extern void  net_replylock();
extern void  net_unlockreq(struct remote *);
extern void  netlock_hostdied(struct remote *);
extern void  netmonitor();
extern void  netshut();
extern void  netsync();
extern void  net_vclear(const netid_t);
extern void  newhost();
extern void  propok(jident *);
extern void  reply_propose(ShreqRef, jident *);
extern void  rem_notify(BtjobRef, const netid_t, const struct jremnotemsg *);
extern void  remasses(BtjobhRef, const USHORT, const USHORT, const USHORT);
extern void  rrstatchange(const netid_t, struct jstatusmsg *);
extern void  send_endsync(struct remote *);
extern void  shut_host(const netid_t);
extern void  statchange(struct jstatusmsg *);
extern void  sync_single(const netid_t, const slotno_t);
extern void  send_single_jobhdr(const netid_t, const slotno_t);
extern void  var_broadcast(BtvarRef, const unsigned);
extern void  var_pack(struct varnetmsg *, CBtvarRef);
extern void  var_remassign(BtvarRef);
extern void  var_remchmog(BtvarRef);
extern void  var_remchname(BtvarRef);
extern void  var_remdelete(BtvarRef);
extern void  var_unpack(BtvarRef, const struct varnetmsg *);
extern void  vid_pack(vident *, const Btvar * const);
extern int  is_skeleton(const vhash_t);
extern int  net_lockreq(struct remote *);
extern int  remchjob(ShreqRef, BtjobRef);
extern int  remjchmog(BtjobRef);
extern int  sendsync(struct remote *);
extern unsigned  ci_addhost(const unsigned, CCmdintRef);
extern unsigned  job_message(const netid_t, CBtjobhRef, CShreqRef);
extern unsigned  job_pack(struct jobnetmsg *, CBtjobRef);
extern unsigned  job_sendmdupdate(BtjobRef, BtjobRef, ShreqRef);
extern unsigned  job_sendugupdate(BtjobRef, ShreqRef);
extern unsigned  job_sendupdate(BtjobRef, BtjobRef, ShreqRef, const unsigned);
extern unsigned  nettickle();
extern unsigned  var_sendugupdate(BtvarRef, ShreqRef);
extern unsigned  var_sendupdate(BtvarRef, BtvarRef, ShreqRef);
extern vhash_t  myvariable(BtvarRef, BtmodeRef, const unsigned);
extern vhash_t  vid_uplook(const vident *);
extern char **envhandle(BtjobRef);
extern struct remote *find_connected(const netid_t);
extern struct remote *find_probe(const netid_t);
extern struct remote *find_sync(const netid_t, const int);
extern struct remote *alloc_roam(struct remote *);
extern struct remote *conn_attach(struct remote *);
extern struct remote *rattach(struct remote *);
