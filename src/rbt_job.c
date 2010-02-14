/* rbt_job.c -- Pack up and initialise TCP jobs.

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

#include "config.h"

#include <stdio.h>
#include "incl_unix.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btuser.h"
#include "timecon.h"
#include "btconst.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "xbnetq.h"
#include "ecodes.h"
#include "errnums.h"
#include "files.h"
#include "btrvar.h"
#include "services.h"

static	SHORT	tcpportnum = -1;

const	char	TSname[] = GBNETSERV_PORT;

extern	uid_t	Repluid;		/* Replacement if requested */
extern	gid_t	Replgid;		/* Replacement if requested */

extern	netid_t	Out_host;
extern	char	*Out_interp, *realuname;
extern	char	realgname[];

int  sock_read(const int sock, char *buffer, int nbytes)
{
	while  (nbytes > 0)  {
		int	rbytes = read(sock, buffer, nbytes);
		if  (rbytes <= 0)
			return  0;
		buffer += rbytes;
		nbytes -= rbytes;
	}
	return  1;
}

int  sock_write(const int sock, char *buffer, int nbytes)
{
	while  (nbytes > 0)  {
		int	rbytes = write(sock, buffer, nbytes);
		if  (rbytes < 0)
			return  0;
		buffer += rbytes;
		nbytes -= rbytes;
	}
	return  1;
}

static void  inittcp()
{
	struct	servent	*sp;
	struct	protoent  *pp;
	char	*tcp_protoname;

	if  (!((pp = getprotobyname("tcp"))  || (pp = getprotobyname("TCP"))))  {
		print_error($E{No TCP protocol});
		exit(E_NETERR);
	}
	tcp_protoname = pp->p_name;
	endprotoent();

	/* Get port number for this caper */

	if  (!(sp = env_getserv(TSname, tcp_protoname)))  {
		print_error($E{No xbnetserv TCP service});
		endservent();
		exit(E_NETERR);
	}
	tcpportnum = sp->s_port;
	endservent();
}

int  packjob(struct nijobmsg *dest, CBtjobRef src)
{
	int		cnt;
	unsigned	ucnt;
	unsigned	hwm;
	JargRef		darg;
	EnvirRef	denv;
	RedirRef	dred;
	const	Jarg	*sarg;
	const	Envir	*senv;
	const	Redir	*sred;

	BLOCK_ZERO((char *) dest, sizeof(struct nijobmsg));

	dest->ni_hdr.ni_progress = src->h.bj_progress;
	dest->ni_hdr.ni_pri = src->h.bj_pri;
	dest->ni_hdr.ni_jflags = src->h.bj_jflags;
	dest->ni_hdr.ni_istime = src->h.bj_times.tc_istime;
	dest->ni_hdr.ni_mday = src->h.bj_times.tc_mday;
	dest->ni_hdr.ni_repeat = src->h.bj_times.tc_repeat;
	dest->ni_hdr.ni_nposs = src->h.bj_times.tc_nposs;

	/* Do the "easy" bits */

	dest->ni_hdr.ni_ll = htons(src->h.bj_ll);
	dest->ni_hdr.ni_umask = htons(src->h.bj_umask);
	dest->ni_hdr.ni_nvaldays = htons(src->h.bj_times.tc_nvaldays);
	dest->ni_hdr.ni_ulimit = htonl(src->h.bj_ulimit);
	dest->ni_hdr.ni_nexttime = htonl(src->h.bj_times.tc_nexttime);
	dest->ni_hdr.ni_rate = htonl(src->h.bj_times.tc_rate);
	dest->ni_hdr.ni_autoksig = htons(src->h.bj_autoksig);
	dest->ni_hdr.ni_runon = htons(src->h.bj_runon);
	dest->ni_hdr.ni_deltime	= htons(src->h.bj_deltime);
	dest->ni_hdr.ni_runtime	= htonl(src->h.bj_runtime);

	strncpy(dest->ni_hdr.ni_cmdinterp, Out_interp && Out_interp[0]? Out_interp: DEF_CI_NAME, CI_MAXNAME);

	dest->ni_hdr.ni_exits = src->h.bj_exits;

	dest->ni_hdr.ni_mode.u_flags = htons(src->h.bj_mode.u_flags);
	dest->ni_hdr.ni_mode.g_flags = htons(src->h.bj_mode.g_flags);
	dest->ni_hdr.ni_mode.o_flags = htons(src->h.bj_mode.o_flags);

	for  (cnt = 0;  cnt < Condcnt;  cnt++)  {
		Nicond	*nic = &dest->ni_hdr.ni_conds[cnt];
		struct	scond	*sic = &Condlist[cnt];
		nic->nic_var.ni_varhost = sic->vd.hostid == 0? myhostid: sic->vd.hostid;
		strncpy(nic->nic_var.ni_varname, sic->vd.var, BTV_NAME);
		nic->nic_compar = (unsigned char) sic->compar;
		nic->nic_iscrit = sic->vd.crit;
		nic->nic_type = (unsigned char) sic->value.const_type;
		if  (nic->nic_type == CON_STRING)
			strncpy(nic->nic_un.nic_string, sic->value.con_un.con_string, BTC_VALUE);
		else
			nic->nic_un.nic_long = htonl(sic->value.con_un.con_long);
	}
	for  (cnt = 0;  cnt < Asscnt;  cnt++)  {
		Niass	*nia = &dest->ni_hdr.ni_asses[cnt];
		struct	Sass	*sia = &Asslist[cnt];
		nia->nia_var.ni_varhost = sia->vd.hostid == 0? myhostid: sia->vd.hostid;
		strncpy(nia->nia_var.ni_varname, sia->vd.var, BTV_NAME);
		nia->nia_flags = htons(sia->flags);
		nia->nia_op = (unsigned char) sia->op;
		nia->nia_iscrit = sia->vd.crit;
		nia->nia_type = (unsigned char) sia->con.const_type;
		if  (nia->nia_type == CON_STRING)
			strncpy(nia->nia_un.nia_string, sia->con.con_un.con_string, BTC_VALUE);
		else
			nia->nia_un.nia_long = htonl(sia->con.con_un.con_long);
	}

	dest->ni_hdr.ni_nredirs = htons(src->h.bj_nredirs);
	dest->ni_hdr.ni_nargs = htons(src->h.bj_nargs);
	dest->ni_hdr.ni_nenv = htons(src->h.bj_nenv);
	dest->ni_hdr.ni_title = htons(src->h.bj_title);
	dest->ni_hdr.ni_direct = htons(src->h.bj_direct);
	dest->ni_hdr.ni_arg = htons(src->h.bj_arg);
	dest->ni_hdr.ni_redirs = htons(src->h.bj_redirs);
	dest->ni_hdr.ni_env = htons(src->h.bj_env);
	BLOCK_COPY(dest->ni_space, src->bj_space, JOBSPACE);

	/* Cheat by assuming that packjstring put the directory and
	   title in last and we can use the offset of that as a
	   high water mark.  */

	if  (src->h.bj_title >= 0)  {
		hwm = src->h.bj_title;
		hwm += strlen(&src->bj_space[hwm]) + 1;
	}
	else  if  (src->h.bj_direct >= 0)  {
		hwm = src->h.bj_direct;
		hwm += strlen(&src->bj_space[hwm]) + 1;
	}
	else
		hwm = JOBSPACE;

	hwm += sizeof(struct nijobmsg) - JOBSPACE;

	/* We must swap the argument, environment and redirection
	   variable pointers and the arg field in each redirection.  */

	darg = (JargRef) &dest->ni_space[src->h.bj_arg];	/* I did mean src there */
	denv = (EnvirRef) &dest->ni_space[src->h.bj_env];	/* and there */
	dred = (RedirRef) &dest->ni_space[src->h.bj_redirs];	/* and there */
	sarg = (const Jarg *) &src->bj_space[src->h.bj_arg];
	senv = (const Envir *) &src->bj_space[src->h.bj_env];
	sred = (const Redir *) &src->bj_space[src->h.bj_redirs];

	for  (ucnt = 0;  ucnt < src->h.bj_nargs;  ucnt++)  {
		*darg++ = htons(*sarg);
		sarg++;		/* Not falling for htons being a macro!!! */
	}
	for  (ucnt = 0;  ucnt < src->h.bj_nenv;  ucnt++)  {
		denv->e_name = htons(senv->e_name);
		denv->e_value = htons(senv->e_value);
		denv++;
		senv++;
	}
	for  (ucnt = 0;  ucnt < src->h.bj_nredirs; ucnt++)  {
		dred->arg = htons(sred->arg);
		dred++;
		sred++;
	}
	return  hwm;
}

int  remgoutfile(const netid_t hostid, CBtjobRef jb)
{
	int			sock;
	int			msgsize;
	struct	sockaddr_in	sin;
	struct	ni_jobhdr	nih;
	struct	nijobmsg	outmsg;

	if  (tcpportnum < 0)
		inittcp();

	if  ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return  -1;

	/* Set up bits and pieces.
	   The port number is set up in the job shared memory segment.  */

	sin.sin_family = AF_INET;
	sin.sin_port = tcpportnum;
	BLOCK_ZERO(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = hostid;

	if  (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		close(sock);
		return  -1;
	}

	msgsize = packjob(&outmsg, jb);
	nih.code = CL_SV_STARTJOB;
	nih.padding = 0;
	nih.joblength = htons(msgsize);
	strcpy(nih.uname, realuname);
	strcpy(nih.gname, realgname);
	if  (Realuid != Repluid)
		strcpy(outmsg.ni_hdr.ni_mode.o_user, prin_uname(Repluid));
	if  (Realgid != Replgid)
		strcpy(outmsg.ni_hdr.ni_mode.o_group, prin_gname(Replgid));

	if  (!sock_write(sock, (char *) &nih, sizeof(nih)))  {
		print_error($E{Trouble with job header});
		close(sock);
		return  -1;
	}

	if  (!sock_write(sock, (char *) &outmsg, (int) msgsize))  {
		print_error($E{Trouble with job});
		close(sock);
		return  -1;
	}

	return  sock;
}

LONG  remprocreply(const int sock)
{
	int	errcode;
	ULONG	which;
	struct	client_if	result;

	if  (!sock_read(sock, (char *) &result, sizeof(result)))  {
		print_error($E{Cant read status result});
		return  0;
	}

	errcode = result.code;
 redo:
	switch  (errcode)  {
	case  XBNQ_OK:
		return  ntohl(result.param);

	case  XBNR_BADCVAR:
		which = ntohl(result.param);
		if  (which >= MAXCVARS)  {
			print_error($E{Bad condition result});
			return  0;
		}
		disp_str = Condlist[which].vd.var;
		break;

	case  XBNR_BADAVAR:
		which = ntohl(result.param);
		if  (which >= MAXSEVARS)  {
			print_error($E{Bad assignment result});
			return  0;
		}
		disp_str = Asslist[which].vd.var;
		break;

	case  XBNR_BADCI:
		disp_str = Out_interp;

	case  XBNR_UNKNOWN_CLIENT:
	case  XBNR_NOT_CLIENT:
	case  XBNR_NOT_USERNAME:
	case  XBNR_NOMEM_QF:
	case  XBNR_NOCRPERM:
	case  XBNR_BAD_PRIORITY:
	case  XBNR_BAD_LL:
	case  XBNR_BAD_USER:
	case  XBNR_FILE_FULL:
	case  XBNR_QFULL:
	case  XBNR_BAD_JOBDATA:
	case  XBNR_UNKNOWN_USER:
	case  XBNR_UNKNOWN_GROUP:
	case  XBNR_NORADMIN:
		break;
	case  XBNR_ERR:
		errcode = ntohl(result.param);
		goto  redo;
	default:
		disp_arg[0] = errcode;
		print_error($E{Unknown queue result message});
		return  0;
	}
	disp_arg[1] = Out_host;
	print_error($E{Base for rbtr return errors}+result.code);
	return  0;
}
