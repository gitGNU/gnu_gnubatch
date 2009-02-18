/* dumpjob.cpp -- Output job to file

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

#include "stdafx.h"
#include <ctype.h>
#include <time.h>
#include <direct.h>
#ifdef	BTQW
#include "formatcode.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"                   
#endif
#ifdef	BTRW
#include "btrw.h"
#endif
#include "files.h"

static	CString	resultcmd;

inline	void	appendlong(const long item)
{
	char	thing[24];
	wsprintf(thing, "%ld", item);
	resultcmd += thing;
}

inline	void	appendoctal(const unsigned item)
{
	for  (int cnt = 9;  cnt >= 0;  cnt -= 3)
		resultcmd += char(((item >> cnt) & 7) + '0');
}

static	void	dumpmode(char *prefix, unsigned md)
{
	resultcmd += prefix;
	resultcmd += ':';
	if  (md & BTM_READ)
		resultcmd += 'R';
	if  (md & BTM_WRITE)
		resultcmd += 'W';
	if  (md & BTM_SHOW)
		resultcmd += 'S';
	if  (md & BTM_RDMODE)
		resultcmd += 'M';
	if  (md & BTM_WRMODE)
		resultcmd += 'P';
	if  (md & BTM_UGIVE)
		resultcmd += 'U';
	if  (md & BTM_UTAKE)
		resultcmd += 'V';
	if  (md & BTM_GGIVE)
		resultcmd += 'G';
	if  (md & BTM_GTAKE)
		resultcmd += 'H';
	if  (md & BTM_DELETE)
		resultcmd += 'D';
	if  (md & BTM_KILL)
		resultcmd += 'K';
}

static	void	dumpconds(const Btjob &jb)
{
	for  (int jn = 0;  jn < MAXCVARS;  jn++)  {
		const Jcond	&jc = jb.bj_conds[jn];
		
		if  (jc.bjc_compar == C_UNUSED)
			return;
                                      
		resultcmd += " -";                                      
		resultcmd += char(jc.bjc_iscrit? 'k': 'K');
		resultcmd += " -c";
        
#ifdef	BTQW		
		Btvar	&vp = *Vars()[jc.bjc_varind];
		resultcmd += look_host(vp.hostid);
		resultcmd += ':';
		resultcmd += vp.var_name;
#endif
#ifdef	BTRW
		resultcmd += jc.bjc_var;
#endif
		CString	opn;
		opn.LoadString(IDS_CONDOP + jc.bjc_compar);
		resultcmd += opn;
		
		if  (jc.bjc_value.const_type == CON_STRING)  {
			if  (isdigit(jc.bjc_value.con_string[0]))
				resultcmd += ':';
			resultcmd += jc.bjc_value.con_string;
		}
		else
			appendlong(jc.bjc_value.con_long);
	}
}

static	void	dumpasses(const Btjob &jb)
{
	for  (int jn = 0;  jn < MAXSEVARS;  jn++)  {
		const Jass	&ja = jb.bj_asses[jn];
		
		if  (ja.bja_op == BJA_NONE)
			return;

		resultcmd += " -";
		resultcmd += char(ja.bja_iscrit? 'b': 'B');
		if  (ja.bja_op < BJA_SEXIT  &&  ja.bja_flags != 0)  {
			resultcmd += " -f";	
			if  (ja.bja_flags & BJA_START)
				resultcmd += 'S';
			if  (ja.bja_flags & BJA_REVERSE)
				resultcmd += 'R';
			if  (ja.bja_flags & BJA_OK)
				resultcmd += 'N';
			if  (ja.bja_flags & BJA_ERROR)
				resultcmd += 'E';
			if  (ja.bja_flags & BJA_ABORT)
				resultcmd += 'A';
			if  (ja.bja_flags & BJA_CANCEL)
				resultcmd += 'C';
		}

		resultcmd += " -s";
#ifdef	BTQW
		Btvar	&vp = *Vars()[ja.bja_varind];
		resultcmd += look_host(vp.hostid);
		resultcmd += ':';
		resultcmd += vp.var_name;
#endif
#ifdef	BTRW
		resultcmd += ja.bja_var;
#endif
		
		if  (ja.bja_op >= BJA_SEXIT)  {
			CString	msg;
			msg.LoadString(ja.bja_op == BJA_SEXIT? IDS_AEXIT: IDS_ASIGNAL);
			resultcmd += '=' + msg;
		}
		else  {
			CString	opn;
			opn.LoadString(IDS_ASSOP + ja.bja_op);
			resultcmd += opn;
			if  (ja.bja_con.const_type == CON_STRING)  {
				if  (isdigit(ja.bja_con.con_string[0]))
					resultcmd += ':';
				resultcmd += ja.bja_con.con_string;
			}
			else
				appendlong(ja.bja_con.con_long);
		}
	}
}

void	dumptime(const Btjob &jb)
{
	if  (jb.bj_times.tc_istime == 0)
		return;

	resultcmd += " -T";
	char	thing[2+1+2+1+2+1+2+1+2+1+10/*for luck*/];
	
	tm	*t = localtime(&jb.bj_times.tc_nexttime);
	
	wsprintf(thing, "%.2d/%.2d/%.2d,%.2d:%.2d",
		       t->tm_year % 100,
		       t->tm_mon + 1,
		       t->tm_mday,
		       t->tm_hour,
		       t->tm_min);
	resultcmd += thing;
	
	resultcmd += " -A-";
	for  (int nd = 0;  nd < TC_NDAYS; nd++)
		if  (jb.bj_times.tc_nvaldays & (1 << nd))  {
			CString	dname;
			dname.LoadString(IDS_SUN + nd);
			resultcmd += ',' + dname;
		}

	if  (jb.bj_times.tc_repeat < TC_MINUTES)
		resultcmd += jb.bj_times.tc_repeat == TC_DELETE? " -d": " -o";
	else  {
		resultcmd += " -r";
		CString	runit;
		runit.LoadString(IDS_MINUTES + jb.bj_times.tc_repeat - TC_MINUTES);
		resultcmd += runit + ':';
		appendlong(jb.bj_times.tc_rate);
		if  (jb.bj_times.tc_repeat == TC_MONTHSB || jb.bj_times.tc_repeat == TC_MONTHSE)  {
			resultcmd += ':';
			if  (jb.bj_times.tc_repeat == TC_MONTHSB)
				appendlong(jb.bj_times.tc_mday);
			else  {
				extern  char  FAR	month_days[];
				month_days[1] = t->tm_year % 4 == 0? 29: 28;
				int	mday = month_days[t->tm_mon] - jb.bj_times.tc_mday;
				appendlong(mday <= 0? 1: mday);
			}
		}
	}
}

void	dumpredir(const Mredir &rd, CString &result)
{
	char	thing[24];
	switch  (rd.action)  {
	case  RD_ACT_RD:
		if  (rd.fd != 0)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}			
		result += '<';
		break;
	case  RD_ACT_WRT:
		if  (rd.fd != 1)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}
		result += '>';
		break;
	case  RD_ACT_APPEND:
		if  (rd.fd != 1)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}
		result += ">>";
		break;
	case  RD_ACT_RDWR:
		if  (rd.fd != 0)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}			
		result += "<>";
		break;
	case  RD_ACT_RDWRAPP:
		if  (rd.fd != 0)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}			
		result += "<>>";
		break;
	case  RD_ACT_PIPEO:
		if  (rd.fd != 1)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}
		result += '|';
		break;
	case  RD_ACT_PIPEI:
		if  (rd.fd != 0)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}			
		result += "<|";
		break;
	case  RD_ACT_CLOSE:
		if  (rd.fd != 1)  {
			wsprintf(thing, "%d", rd.fd);
			result += thing;
		}
		result += ">&-";
		return;
	case  RD_ACT_DUP:
		if  (rd.fd != 1)
			appendlong(rd.fd);
		result += ">&";
		wsprintf(thing, "%d", rd.arg);
		result += thing;
		return;
	}
	result += rd.buffer;
}

static	void	dumphdrs(const Btjob &jb)
{
#ifdef	BTQW
	resultcmd += ((CBtqwApp *)AfxGetApp())->m_binunq? " -O": " -W";
#endif
#ifdef	BTRW
	resultcmd += ((CBtrwApp *)AfxGetApp())->m_binmode? " -O": " -W";
#endif
	//  Clear all cumulative args, redirections, conds, assignments
	resultcmd += "Zeyz";

	resultcmd += jb.bj_progress == BJP_DONE? '.': jb.bj_progress < BJP_DONE? 'N': 'C';
	if  ((jb.bj_jflags & (BJ_WRT|BJ_MAIL)) != (BJ_WRT|BJ_MAIL))
		resultcmd += 'x';
	if  (jb.bj_jflags & BJ_WRT)
		resultcmd += 'w';
	if  (jb.bj_jflags & BJ_MAIL)
		resultcmd += 'm';

	resultcmd += jb.bj_jflags & BJ_REMRUNNABLE? 'G': jb.bj_jflags & BJ_EXPORT? 'F' : 'n';
	resultcmd += jb.bj_jflags & BJ_NOADVIFERR? 'j': 'J';
	resultcmd += jb.bj_times.tc_nposs > TC_CATCHUP? 'S' : "SHR9"[jb.bj_times.tc_nposs];
	resultcmd += " -p";
	appendlong(long(jb.bj_pri));

	dumptime(jb);

	if  (!jb.bj_title.IsEmpty())  {
		int	 colp = jb.bj_title.Find(':');
		if  (colp > 0)  {
			resultcmd += " -q";
			resultcmd += jb.bj_title.Left(colp-1);
			int  lng = jb.bj_title.GetLength() - colp - 1;
			if  (lng > 0)
				resultcmd += " -h\'" + jb.bj_title.Right(lng) + '\'';
		}
		else
			resultcmd += " -h\'" + jb.bj_title + '\'';
	}

	if  (!jb.bj_direct.IsEmpty())
		resultcmd += " -D" + jb.bj_direct;
		
	resultcmd += " -M";
	dumpmode("U", jb.bj_mode.u_flags);
	dumpmode(",G", jb.bj_mode.g_flags);
	dumpmode(",O", jb.bj_mode.o_flags);
	dumpconds(jb);
	dumpasses(jb);

	resultcmd += " -i" + jb.bj_cmdinterp;
	
	resultcmd += " -l";
	appendlong(long(jb.bj_ll));
	
	resultcmd += " -P";
	appendoctal(jb.bj_umask);
	resultcmd += " -L";
	appendlong(jb.bj_ulimit);
	resultcmd += " -t";
	appendlong(jb.bj_deltime);
	resultcmd += " -Y";
	appendlong(jb.bj_runtime);
	resultcmd += " -E";
	appendlong(jb.bj_autoksig);
	resultcmd += " -2";
	appendlong(jb.bj_runon);

	char	bits[24];
	wsprintf(bits, " -XN%d:%d", jb.bj_exits.nlower, jb.bj_exits.nupper);
	resultcmd += bits;
	wsprintf(bits, " -XE%d:%d", jb.bj_exits.elower, jb.bj_exits.eupper);
	resultcmd += bits;

	//  Now for redirections.

	for  (unsigned cnt = 0;  cnt < jb.bj_nredirs;  cnt++)  {
		resultcmd += " -I\'";
		dumpredir(jb.bj_redirs[cnt], resultcmd);
		resultcmd += '\'';
	}

	//  Now for arguments
	
	for  (cnt = 0;  cnt < jb.bj_nargs;  cnt++)
		resultcmd += " -a\'" + jb.bj_arg[cnt] + '\'';
}

BOOL	dumpjob(CFile &bfile, const CString &jobname, const Btjob &jb)
{
	TRY {
	    for  (unsigned  cnt = 0;  cnt < jb.bj_nenv;  cnt++)  {
	    	resultcmd = "set " + jb.bj_env[cnt].e_name + '=' + jb.bj_env[cnt].e_value + "\r\n";
			bfile.Write((const char *) resultcmd, resultcmd.GetLength());
	    }	
		resultcmd = "btr";
		dumphdrs(jb);
		resultcmd += ' ' + jobname + "\r\n\x1A";
		bfile.Write((const char *) resultcmd, resultcmd.GetLength());
	}
	CATCH(CException, e)
	{
		AfxMessageBox(IDP_CWBATFILE, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}
	END_CATCH
	resultcmd.Empty();
	return  TRUE;
}

