// Copyright (c) Xi Software Ltd. 1993.
//
// packem.cpp: created by John Collins on Tue Aug 24 1993.
//----------------------------------------------------------------------
// DOS/Windows Release 5
//----------------------------------------------------------------------
// Pack and unpack jobs and variables.

#include "stdafx.h"
#include "btqw.h"
#include "netmsg.h"
#include <string.h>

void	jid_pack(jident &dest, const jident &src)
{
	dest.hostid = src.hostid;
	dest.slotno = htonl(src.slotno);
}

void	jid_unpack(jident &dest, const jident &src)
{
	dest.hostid = src.hostid;
	dest.slotno = ntohl(src.slotno);
}

void	vid_pack(vident &dest, const vident &src)
{
	dest.hostid = src.hostid;
	dest.slotno = htonl(src.slotno);
}

void	vid_unpack(vident &dest, const vident &src)
{
	dest.hostid = src.hostid;
	dest.slotno = htonl(src.slotno);
}

inline	int	vid_uplook(const vident &src)
{
	vident	unsw;
	vid_unpack(unsw, src);
	return  Vars().vindex(unsw);
}

void	mode_pack(Btmode &dest, const Btmode &src)
{
	(void) strcpy(dest.o_user, src.o_user);
	(void) strcpy(dest.c_user, src.c_user);
	(void) strcpy(dest.o_group, src.o_group);
	(void) strcpy(dest.c_group, src.c_group);
	dest.u_flags = htons(src.u_flags);
	dest.g_flags = htons(src.g_flags);
	dest.o_flags = htons(src.o_flags);
}

void	mode_unpack(Btmode &dest, const Btmode &src)
{
	strcpy(dest.o_user, src.o_user);
	strcpy(dest.c_user, src.c_user);
	strcpy(dest.o_group, src.o_group);
	strcpy(dest.c_group, src.c_group);
	dest.u_flags = ntohs(src.u_flags);
	dest.g_flags = ntohs(src.g_flags);
	dest.o_flags = ntohs(src.o_flags);
}

void	jobh_pack(jobhnetmsg &dest, const Btjob &src)
{
	memset((void *) &dest, '\0', sizeof(jobhnetmsg));

	// caller sets code and maybe length if this is part of something else
	
	dest.hdr.length = htons(sizeof(jobhnetmsg));
	jid_pack(dest.jid, src);

	dest.nm_progress 	= src.bj_progress;
	dest.nm_pri 		= src.bj_pri;
	dest.nm_jflags 		= src.bj_jflags;
	dest.nm_istime		= src.bj_times.tc_istime;
	dest.nm_mday		= src.bj_times.tc_mday;
	dest.nm_repeat		= src.bj_times.tc_repeat;
	dest.nm_nposs		= src.bj_times.tc_nposs;

	dest.nm_ll			= htons(src.bj_ll);
	dest.nm_umask		= htons(src.bj_umask);
	dest.nm_nvaldays	= htons(src.bj_times.tc_nvaldays);
	dest.nm_deltime		= htons(src.bj_deltime);
	dest.nm_runon		= htons(src.bj_runon);
	dest.nm_autoksig	= htons(src.bj_autoksig);
	
	dest.nm_job			= htonl(src.bj_job);
	dest.nm_time		= htonl(src.bj_time);
	dest.nm_stime		= 0L;
	dest.nm_etime		= 0L;
	dest.nm_pid			= 0;
	dest.nm_lastexit	= 0;
	dest.nm_orighostid	= src.bj_orighostid;
	dest.nm_runhostid	= src.bj_runhostid;
	dest.nm_ulimit		= htonl(src.bj_ulimit);
	dest.nm_nexttime	= htonl(src.bj_times.tc_nexttime);
	dest.nm_rate		= htonl(src.bj_times.tc_rate);
	dest.nm_runtime		= htonl(src.bj_runtime);

	strncpy(dest.nm_cmdinterp, (const char *) src.bj_cmdinterp, CI_MAXNAME);
	dest.nm_exits		= src.bj_exits;
	mode_pack(dest.nm_mode, src.bj_mode);

	for  (unsigned cnt = 0;  cnt < MAXCVARS;  cnt++)  {
		const Jcond  &cr = src.bj_conds[cnt];
		if  (cr.bjc_compar == C_UNUSED)
			break;
		Jncond  &dr = dest.nm_conds[cnt];
		dr.bjnc_compar = cr.bjc_compar;
		dr.bjnc_iscrit = cr.bjc_iscrit;
		if  ((dr.bjnc_type = (unsigned char) cr.bjc_value.const_type) == CON_STRING)
			strncpy(dr.bjnc_un.bjnc_string, cr.bjc_value.con_string, BTC_VALUE);
		else
			dr.bjnc_un.bjnc_long = htonl(cr.bjc_value.con_long);
		vid_pack(dr.bjnc_var, *(Vars()[cr.bjc_varind]));
	}

	for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
		const Jass  &cr = src.bj_asses[cnt];
		if  (cr.bja_op == BJA_NONE)
			break;
		Jnass  &dr = dest.nm_asses[cnt];
		dr.bjna_flags = htons(cr.bja_flags);
		dr.bjna_op = cr.bja_op;
		dr.bjna_iscrit = cr.bja_iscrit;
		if  ((dr.bjna_type = (unsigned char) cr.bja_con.const_type) == CON_STRING)
			strncpy(dr.bjna_un.bjna_string, cr.bja_con.con_string, BTC_VALUE);
		else
			dr.bjna_un.bjna_long = htonl(cr.bja_con.con_long);
		vid_pack(dr.bjna_var, *(Vars()[cr.bja_varind]));
	}
}

void	jobh_unpack(Btjob &dest, const jobhnetmsg &src)
{
	jid_unpack(dest, src.jid);

	dest.bj_progress 		= src.nm_progress;
	dest.bj_pri 			= src.nm_pri;
	dest.bj_jflags 			= src.nm_jflags;
	dest.bj_jrunflags		= 0;
	dest.bj_times.tc_istime	= src.nm_istime;
	dest.bj_times.tc_mday	= src.nm_mday;
	dest.bj_times.tc_repeat	= src.nm_repeat;
	dest.bj_times.tc_nposs	= src.nm_nposs;

	dest.bj_ll				= ntohs(src.nm_ll);
	dest.bj_umask			= ntohs(src.nm_umask);
	dest.bj_times.tc_nvaldays= ntohs(src.nm_nvaldays);
	dest.bj_deltime			= ntohs(src.nm_deltime);
	dest.bj_runon			= ntohs(src.nm_runon);
	dest.bj_autoksig		= ntohs(src.nm_autoksig);
	dest.bj_lastexit		= ntohs(src.nm_lastexit);

	dest.bj_job				= ntohl(src.nm_job);
	dest.bj_pid				= ntohl(src.nm_pid);
	dest.bj_time			= ntohl(src.nm_time);
	dest.bj_stime			= ntohl(src.nm_stime);
	dest.bj_etime			= ntohl(src.nm_etime);
	dest.bj_orighostid		= src.nm_orighostid;
	dest.bj_runhostid		= src.nm_runhostid;
	dest.bj_ulimit			= ntohl(src.nm_ulimit);
	dest.bj_times.tc_nexttime= ntohl(src.nm_nexttime);
	dest.bj_times.tc_rate	= ntohl(src.nm_rate);
	dest.bj_runtime			= ntohl(src.nm_runtime);

	dest.bj_cmdinterp		= src.nm_cmdinterp;
	dest.bj_exits			= src.nm_exits;
	mode_unpack(dest.bj_mode, src.nm_mode);

	memset(dest.bj_conds, '\0', sizeof(dest.bj_conds));

	Jcond  *drc = dest.bj_conds;

	for  (unsigned cnt = 0;  cnt < MAXCVARS;  cnt++)  {
		const Jncond	&cr = src.nm_conds[cnt];

		if  (cr.bjnc_compar == C_UNUSED)
			break;
		int	wind = vid_uplook(cr.bjnc_var);
		if  (wind < 0)  {
			dest.bj_jrunflags |= BJ_SKELHOLD;
			continue;
		}
		drc->bjc_varind = unsigned(wind);
		drc->bjc_compar = cr.bjnc_compar;
		drc->bjc_iscrit = cr.bjnc_iscrit;
		if  ((drc->bjc_value.const_type = cr.bjnc_type) == CON_STRING)
			strncpy(drc->bjc_value.con_string, cr.bjnc_un.bjnc_string, BTC_VALUE);
		else
			drc->bjc_value.con_long = ntohl(cr.bjnc_un.bjnc_long);
		drc++;
	}

	memset(dest.bj_asses, '\0', sizeof(dest.bj_asses));
	Jass  *dra = dest.bj_asses;

	for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
		const Jnass	&cr = src.nm_asses[cnt];
		if  (cr.bjna_op == BJA_NONE)
			break;
		int  wind = vid_uplook(cr.bjna_var);
		if  (wind < 0)  {
			dest.bj_jrunflags |= BJ_SKELHOLD;
			continue;
		}
		dra->bja_varind = unsigned(wind);
		dra->bja_flags = ntohs(cr.bjna_flags);
		dra->bja_op = cr.bjna_op;
		dra->bja_iscrit = cr.bjna_iscrit;
		if  ((dra->bja_con.const_type = cr.bjna_type) == CON_STRING)
			strncpy(dra->bja_con.con_string, cr.bjna_un.bjna_string, BTC_VALUE);
		else
			dra->bja_con.con_long = ntohl(cr.bjna_un.bjna_long);
		dra++;
	}
}

//  Pack up a whole job

unsigned job_pack(jobnetmsg &dest, const Btjob &src)
{
	jobh_pack(dest.hdr, src);
	dest.nm_nredirs		= htons(src.bj_nredirs);
	dest.nm_nargs		= htons(src.bj_nargs);
	dest.nm_nenv		= htons(src.bj_nenv);
	
	//  Pack arguments into vector
	
	unsigned  short	*alist = (unsigned short *) dest.nm_space;
	dest.nm_arg = 0;
	Envir	*elist = (Envir *) &alist[src.bj_nargs];
	dest.nm_env = htons((char *) elist - dest.nm_space);
	Redir	*rlist = (Redir *) &elist[src.bj_nenv];     
	dest.nm_redirs = htons((char *) rlist - dest.nm_space);
	char	*spacep = (char *) &rlist[src.bj_nredirs];
	unsigned  hwm = spacep - dest.nm_space;	
	
	for  (unsigned  cnt = 0; cnt < src.bj_nargs; cnt++)  {
		unsigned  alen = src.bj_arg[cnt].GetLength() + 1;
		if  (alen + hwm > JOBSPACE)
			return  sizeof(jobnetmsg) + 1;
		alist[cnt] = htons(hwm);
		strcpy(spacep, (const char *) src.bj_arg[cnt]);
		spacep += alen;
		hwm += alen;
	}
	
	for  (cnt = 0;  cnt < src.bj_nenv;  cnt++)  {
		unsigned  nlen = src.bj_env[cnt].e_name.GetLength() + 1;
		unsigned  vlen = src.bj_env[cnt].e_value.GetLength() + 1;
		if  (nlen + vlen + hwm > JOBSPACE)
			return  sizeof(jobnetmsg) + 1;
		elist[cnt].e_name = htons(hwm);
		strcpy(spacep, (const char *) src.bj_env[cnt].e_name);
		spacep += nlen;
		hwm += nlen;
		elist[cnt].e_value = htons(hwm);
		strcpy(spacep, (const char *) src.bj_env[cnt].e_value);
		spacep += vlen;
		hwm += vlen;
	}
	
	for  (cnt = 0;  cnt < src.bj_nredirs; cnt++)  {
		rlist[cnt].fd = src.bj_redirs[cnt].fd;
		rlist[cnt].action = src.bj_redirs[cnt].action;
		if  (rlist[cnt].action <= RD_ACT_PIPEI)  {
			unsigned  nlen = src.bj_redirs[cnt].buffer.GetLength() + 1;
			if  (nlen + hwm > JOBSPACE)
				return  sizeof(jobnetmsg) + 1;
			rlist[cnt].arg = htons(hwm);
			strcpy(spacep, (const char *) src.bj_redirs[cnt].buffer);
			spacep += nlen;
			hwm += nlen;
		}
		else
			rlist[cnt].arg = htons(src.bj_redirs[cnt].arg);
	}
	
	if  (src.bj_direct.IsEmpty())
		dest.nm_direct = htons(-1);
	else  {
		unsigned  dlen = src.bj_direct.GetLength() + 1;
		if  (dlen + hwm > JOBSPACE)
			return  sizeof(jobnetmsg) + 1;
		dest.nm_direct = htons(hwm);
		strcpy(spacep, (const char *) src.bj_direct);
		spacep += dlen;
		hwm += dlen;
	}
	
	if  (src.bj_title.IsEmpty())
		dest.nm_title = htons(-1);
	else  {
		unsigned  tlen = src.bj_title.GetLength() + 1;
		if  (tlen + hwm > JOBSPACE)
			return  sizeof(jobnetmsg) + 1;
		dest.nm_title = htons(hwm);
		strcpy(spacep, (const char *) src.bj_title);
		spacep += tlen;
		hwm += tlen;
	}
    
    hwm += sizeof(struct jobnetmsg) - JOBSPACE;
	dest.hdr.hdr.length = htons((unsigned short) hwm);
	return  hwm;
}

void	job_unpack(Btjob &dest, const jobnetmsg &src)
{
	jobh_unpack(dest, src.hdr);

	dest.bj_nredirs	= ntohs(src.nm_nredirs);
	dest.bj_nargs	= ntohs(src.nm_nargs);
	dest.bj_nenv	= ntohs(src.nm_nenv);

	int	 wp;
	
	if  ((wp = ntohs(src.nm_title)) >= 0)
		dest.bj_title = &src.nm_space[wp];
		
	if  ((wp = ntohs(src.nm_direct)) >= 0)
		dest.bj_direct = &src.nm_space[wp];

	if  (dest.bj_nargs != 0)  {
		const unsigned  short  *alist = (const unsigned short *) &src.nm_space[ntohs(src.nm_arg)];
		dest.bj_arg = new CString [dest.bj_nargs];
		for  (unsigned  cnt = 0;  cnt < dest.bj_nargs;  cnt++)
			dest.bj_arg[cnt] = &src.nm_space[ntohs(alist[cnt])];
	}
	
	if  (dest.bj_nenv != 0)  {
		const Envir *elist = (const Envir *) &src.nm_space[ntohs(src.nm_env)];
		dest.bj_env = new Menvir [dest.bj_nenv];
		for  (unsigned cnt = 0;  cnt < dest.bj_nenv;  cnt++)  {
			dest.bj_env[cnt].e_name = &src.nm_space[ntohs(elist[cnt].e_name)];
			dest.bj_env[cnt].e_value = &src.nm_space[ntohs(elist[cnt].e_value)];
		}
	}		
		
	if  (dest.bj_nredirs != 0)  {
		const Redir  *rlist = (const Redir *) &src.nm_space[ntohs(src.nm_redirs)];
		dest.bj_redirs = new Mredir [dest.bj_nredirs];
		for  (unsigned  cnt = 0;  cnt < dest.bj_nredirs;  cnt++)  {
			dest.bj_redirs[cnt].fd = rlist[cnt].fd;
			dest.bj_redirs[cnt].action = rlist[cnt].action;
			if  (rlist[cnt].action <= RD_ACT_PIPEI)
				dest.bj_redirs[cnt].buffer = &src.nm_space[ntohs(rlist[cnt].arg)];
			else
				dest.bj_redirs[cnt].arg = ntohs(rlist[cnt].arg);
		}
	}
}

void	var_pack(varnetmsg &dest, const Btvar &src)
{
	memset((void *) &dest, '\0', sizeof(varnetmsg));
	dest.hdr.length = htons(sizeof(varnetmsg));
	vid_pack(dest.vid, src);
	dest.nm_type = (unsigned char) src.var_type;
	dest.nm_flags = (unsigned char) src.var_flags;
	strncpy(dest.nm_name, src.var_name, BTV_NAME);
	strncpy(dest.nm_comment, src.var_comment, BTV_COMMENT);
	mode_pack(dest.nm_mode, src.var_mode);
	if  ((dest.nm_consttype = (unsigned char) src.var_value.const_type) == CON_STRING)
		(void) strncpy(dest.nm_un.nm_string, src.var_value.con_string, BTC_VALUE);
	else
		dest.nm_un.nm_long = htonl(src.var_value.con_long);
}

void	var_unpack(Btvar &dest, const varnetmsg &src)
{
	vid_unpack(dest, src.vid);
	dest.var_type = src.nm_type;
	dest.var_flags = src.nm_flags;
	strncpy(dest.var_name, src.nm_name, BTV_NAME);
	strncpy(dest.var_comment, src.nm_comment, BTV_COMMENT);
	mode_unpack(dest.var_mode, src.nm_mode);
	if  ((dest.var_value.const_type = src.nm_consttype) == CON_STRING)
		(void) strncpy(dest.var_value.con_string, src.nm_un.nm_string, BTC_VALUE);
	else
		dest.var_value.con_long = ntohl(src.nm_un.nm_long);
}
