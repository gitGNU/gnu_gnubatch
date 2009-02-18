#include "stdafx.h"
#include <iostream.h>
#include <fstream.h>
#include <ctype.h>
#include "xbwnetwk.h"
#include "btrw.h"
#include "netmsg.h"

static	void	unpickvar(const CString &var, netid_t &hid, char *name)
{
	int  ep;
	if  ((ep = var.Find(':')) < 0)  {
		strncpy(name, (const char *) var, BTV_NAME);
		hid = Locparams.servid;
	}
	else  {
		hid = look_hname((const char *) var.Left(ep));
		if  (hid == -1L)
			hid = Locparams.servid;
		strncpy(name, (const char *) var.Right(var.GetLength() - ep - 1), BTV_NAME);
	}
}

int	 Btjob::packjob(nijobmsg &Jreq, const CString &title)  const
{
	memset((void *) &Jreq, '\0', sizeof(Jreq));
	
	Jreq.ni_hdr.ni_progress = bj_progress;
	Jreq.ni_hdr.ni_pri = bj_pri;
	Jreq.ni_hdr.ni_jflags = bj_jflags;
	Jreq.ni_hdr.ni_istime = bj_times.tc_istime;
	Jreq.ni_hdr.ni_mday = bj_times.tc_mday;
	Jreq.ni_hdr.ni_repeat = bj_times.tc_repeat;
	Jreq.ni_hdr.ni_nposs = bj_times.tc_nposs;

	//  Do the "easy" bits
	    
    Jreq.ni_hdr.ni_ll = htons(bj_ll);
    Jreq.ni_hdr.ni_umask = htons(bj_umask);
	Jreq.ni_hdr.ni_nvaldays = htons(bj_times.tc_nvaldays);
	Jreq.ni_hdr.ni_deltime = htons(bj_deltime);
	Jreq.ni_hdr.ni_autoksig = htons(bj_autoksig);
	Jreq.ni_hdr.ni_runon = htons(bj_runon);
	
	Jreq.ni_hdr.ni_ulimit = htonl(bj_ulimit);
	Jreq.ni_hdr.ni_nexttime = htonl(bj_times.tc_nexttime);
    Jreq.ni_hdr.ni_rate = htonl(bj_times.tc_rate);
    Jreq.ni_hdr.ni_runtime = htonl(bj_runtime);
    
    strncpy(Jreq.ni_hdr.ni_cmdinterp, (const char *) bj_cmdinterp, CI_MAXNAME);

	Jreq.ni_hdr.ni_exits = bj_exits;

	Jreq.ni_hdr.ni_mode.u_flags = htons(bj_mode.u_flags);
	Jreq.ni_hdr.ni_mode.g_flags = htons(bj_mode.g_flags);
	Jreq.ni_hdr.ni_mode.o_flags = htons(bj_mode.o_flags);
	
	for  (unsigned  cnt = 0;; cnt++)  {
		const  Jcond	&jic = bj_conds[cnt];
		if  (jic.bjc_compar == C_UNUSED)
			break;
		Nicond	&nic = Jreq.ni_hdr.ni_conds[cnt];
		nic.nic_compar = jic.bjc_compar;
		unpickvar(jic.bjc_var, nic.nic_var.ni_varhost, nic.nic_var.ni_varname);
		nic.nic_iscrit = jic.bjc_iscrit;
		nic.nic_type = (unsigned char) jic.bjc_value.const_type;
		if  (nic.nic_type == CON_STRING)
			strncpy(nic.nic_un.nic_string, jic.bjc_value.con_string, BTC_VALUE);
		else
			nic.nic_un.nic_long = htonl(jic.bjc_value.con_long);
	}
	for  (cnt = 0;;  cnt++)  {
		const  Jass	&jia = bj_asses[cnt];
		if  (jia.bja_op == BJA_NONE)
			break;
		Niass	&nia = Jreq.ni_hdr.ni_asses[cnt];
		unpickvar(jia.bja_var, nia.nia_var.ni_varhost, nia.nia_var.ni_varname);
		nia.nia_flags = htons(jia.bja_flags);
		nia.nia_op = jia.bja_op;
		nia.nia_iscrit = jia.bja_iscrit;
		nia.nia_type = (unsigned char) jia.bja_con.const_type;
		if  (nia.nia_type == CON_STRING)
			strncpy(nia.nia_un.nia_string, jia.bja_con.con_string, BTC_VALUE);
		else
			nia.nia_un.nia_long = htonl(jia.bja_con.con_long);
	}

	Jreq.ni_hdr.ni_nredirs = htons(bj_nredirs);
	Jreq.ni_hdr.ni_nargs = htons(bj_nargs);
	Jreq.ni_hdr.ni_nenv = htons(bj_nenv);
	
	//  Pack arguments into vector
	
	unsigned  short	*alist = (unsigned short *) Jreq.ni_space;
	Jreq.ni_hdr.ni_arg = 0;
	Envir	*elist = (Envir *) &alist[bj_nargs];
	Jreq.ni_hdr.ni_env = htons((char *) elist - Jreq.ni_space);
	Redir	*rlist = (Redir *) &elist[bj_nenv];     
	Jreq.ni_hdr.ni_redirs = htons((char *) rlist - Jreq.ni_space);
	char	*spacep = (char *) &rlist[bj_nredirs];
	unsigned  hwm = spacep - Jreq.ni_space;	
	
	for  (cnt = 0; cnt < bj_nargs; cnt++)  {
		unsigned  alen = bj_arg[cnt].GetLength() + 1;
		if  (alen + hwm > JOBSPACE)
			return  -1;
		alist[cnt] = htons(hwm);
		strcpy(spacep, (const char *) bj_arg[cnt]);
		spacep += alen;
		hwm += alen;
	}
	
	for  (cnt = 0;  cnt < bj_nenv;  cnt++)  {
		unsigned  nlen = bj_env[cnt].e_name.GetLength() + 1;
		unsigned  vlen = bj_env[cnt].e_value.GetLength() + 1;
		if  (nlen + vlen + hwm > JOBSPACE)
			return  -1;
		elist[cnt].e_name = htons(hwm);
		strcpy(spacep, (const char *) bj_env[cnt].e_name);
		spacep += nlen;
		hwm += nlen;
		elist[cnt].e_value = htons(hwm);
		strcpy(spacep, (const char *) bj_env[cnt].e_value);
		spacep += vlen;
		hwm += vlen;
	}
	
	for  (cnt = 0;  cnt < bj_nredirs; cnt++)  {
		rlist[cnt].fd = bj_redirs[cnt].fd;
		rlist[cnt].action = bj_redirs[cnt].action;
		if  (rlist[cnt].action <= RD_ACT_PIPEI)  {
			unsigned  nlen = bj_redirs[cnt].buffer.GetLength() + 1;
			if  (nlen + hwm > JOBSPACE)
				return  -1;
			rlist[cnt].arg = htons(hwm);
			strcpy(spacep, (const char *) bj_redirs[cnt].buffer);
			spacep += nlen;
			hwm += nlen;
		}
		else
			rlist[cnt].arg = htons(bj_redirs[cnt].arg);
	}
	
	if  (bj_direct.IsEmpty())
		Jreq.ni_hdr.ni_direct = htons(-1);
	else  {
		unsigned  dlen = bj_direct.GetLength() + 1;
		if  (dlen + hwm > JOBSPACE)
			return  -1;
		Jreq.ni_hdr.ni_direct = htons(hwm);
		strcpy(spacep, (const char *) bj_direct);
		spacep += dlen;
		hwm += dlen;
	}
	
	if  (title.IsEmpty())
		Jreq.ni_hdr.ni_title = htons(-1);
	else  {
		unsigned  tlen = title.GetLength() + 1;
		if  (tlen + hwm > JOBSPACE)
			return  -1;
		Jreq.ni_hdr.ni_title = htons(hwm);
		strcpy(spacep, (const char *) title);
		spacep += tlen;
		hwm += tlen;
	}
    
    hwm += sizeof(nijobmsg) - JOBSPACE;
	return  hwm;      
}

long	xmit(char *, int);

//  Initialise job request buffer and send job details

BOOL	Btjob::Initjob(const CString &title) const
{   
	nijobmsg  jbuf;
	int	 hwm = packjob(jbuf, title);
	if  (hwm < 0)  {
		AfxMessageBox(IDP_TOOMANYSTRINGS, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}
	union  {	
		ni_jobhdr	jhdr;
		char		jhbuf[CL_SV_BUFFSIZE];
	};
    memset((void *) &jhdr, '\0', sizeof(jhdr));
	jhdr.code = CL_SV_STARTJOB;
	jhdr.joblength = htons((unsigned short) hwm);
	strncpy(jhdr.uname, ((CBtrwApp *)AfxGetApp())->m_sendowner, UIDSIZE);
	strncpy(jhdr.gname, ((CBtrwApp *)AfxGetApp())->m_sendgroup, UIDSIZE);
	
	//  Stuff a buffersworth of the job header in at a time
	
	char	*sbuf = (char *) &jbuf;
	const  int  xbytes = CL_SV_BUFFSIZE - sizeof(jhdr);
	
	while  (hwm > 0)  {
		int  sbytes = hwm > xbytes? xbytes: hwm;
		memcpy((void *) &jhbuf[sizeof(jhdr)], (void *) sbuf, sbytes);
		if  (!xmit(jhbuf, sbytes + sizeof(jhdr)))
			return  FALSE;
		hwm -= sbytes;
		sbuf += sbytes;
		jhdr.code = CL_SV_CONTJOB;
	}
	return  TRUE;
}

static  jobno_t	Endjob()
{
	ni_jobhdr	jhdr;
    memset((void *) &jhdr, '\0', sizeof(jhdr));
	jhdr.code = CL_SV_ENDJOB;
	jhdr.joblength = htons(sizeof(jhdr));
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	strncpy(jhdr.uname, ma.m_sendowner, UIDSIZE);
	strncpy(jhdr.gname, ma.m_sendgroup, UIDSIZE);
	return  jobno_t(xmit((char *) &jhdr, sizeof(jhdr)));
}

//	Send job data
	
static	BOOL	copyout(FILE  *inf)
{
	union  {	
		ni_jobhdr	jhdr;
		char		jhbuf[CL_SV_BUFFSIZE];
	};
    memset((void *) &jhdr, '\0', sizeof(jhdr));
	jhdr.code = CL_SV_JOBDATA;
	jhdr.joblength = htons(CL_SV_BUFFSIZE);
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	strncpy(jhdr.uname, ma.m_sendowner, UIDSIZE);
	strncpy(jhdr.gname, ma.m_sendgroup, UIDSIZE);

	int	nbytes, ch;
	nbytes = sizeof(jhdr);
	while  ((ch = getc(inf)) != EOF)  {
		jhbuf[nbytes++] = ch;
		if  (nbytes >= CL_SV_BUFFSIZE)  {
			if  (!xmit(jhbuf, CL_SV_BUFFSIZE))
				return  FALSE;
			nbytes = sizeof(jhdr);
		}
	}
	if  (nbytes > sizeof(jhdr)  &&  !xmit(jhbuf, nbytes))
		return  FALSE;
	return	TRUE;
}

BOOL	Btjob::submitjob(jobno_t &resnum, const CString &filename)
{
	CString	Fulltitle;
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());

//	Force on export flag if remote runnable, or turn off if non-export
//  flag set in program options.

	bj_jflags |= BJ_EXPORT;
	if  (ma.m_nonexport  &&  !(bj_jflags & BJ_REMRUNNABLE))
		bj_jflags &= ~BJ_EXPORT;
	
	if  (bj_title.Find(':') > 0  ||  ma.m_jobqueue.IsEmpty())
		Fulltitle = bj_title;
	else
		Fulltitle = ma.m_jobqueue + ':' + bj_title;

	FILE	*inf = fopen((const char *) filename, ma.m_binmode? "rb": "rt");
	if  (!inf)  {
		AfxMessageBox(IDP_CANTOPENJFILE, MB_ICONSTOP);
		return  FALSE;
	}
		
	if  (Initjob(Fulltitle) && copyout(inf))  {
		fclose(inf);
		resnum = Endjob();
		if  (resnum > 0)
			return  TRUE;
	}
	return  FALSE;
}
