/* optcopy.cpp -- copy options

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
#ifdef	BTRW
#include "xbwnetwk.h"
#include "btrw.h"
#endif
#ifdef	BTQW
#include "netmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"                   
#endif
#include "files.h"

extern	char	FAR	basedir[];

const  char poptname[] = "Program options";
const  char tdefname[] = "Time defaults";
const  char qdefname[] = "Queue defaults";

#ifdef	BTRW

const  char cdefname[] = "Cond defaults";
const  char adefname[] = "Ass defaults";

extern	BOOL	parsecond(char *, Jcond &, const BOOL);
extern	BOOL	parseass(char *, Jass &, const BOOL, const unsigned);
extern	BOOL	parsearg(Btjob &, char *);
extern	BOOL	parseredir(Btjob &, char *);

unsigned  long	GetPrivateProfileUlong(const char *section, const char *entry, const unsigned long deflt, const char *file)
{
	char	ibuf[100];
	if  (::GetPrivateProfileString(section, entry, "", ibuf, sizeof(ibuf), file) <= 0)
		return  deflt;
	return  strtoul(ibuf, (char **) 0, 0);
}
		
void	Btjob::optread()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, WININI);

	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	char	ibuf[256];
	if  (::GetPrivateProfileString(poptname, "Jobqueue", "", ibuf, sizeof(ibuf), pfilepath) > 0)
		ma.m_jobqueue = ibuf;
	::GetPrivateProfileString(qdefname, "Title", "", ibuf, sizeof(ibuf), pfilepath);
	bj_title = ibuf;
	bj_times.tc_nposs = ::GetPrivateProfileInt(tdefname, "Notposs", TC_WAIT1, pfilepath);
    bj_times.tc_repeat = ::GetPrivateProfileInt(tdefname, "Repeat", TC_HOURS, pfilepath);
    bj_times.tc_nvaldays = 0;
    if  (::GetPrivateProfileInt(tdefname, "AvSun", 1, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 0);
    if  (::GetPrivateProfileInt(tdefname, "AvMon", 0, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 1);
    if  (::GetPrivateProfileInt(tdefname, "AvTue", 0, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 2);
    if  (::GetPrivateProfileInt(tdefname, "AvWed", 0, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 3);
    if  (::GetPrivateProfileInt(tdefname, "AvThu", 0, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 4);
    if  (::GetPrivateProfileInt(tdefname, "AvFri", 0, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 5);
    if  (::GetPrivateProfileInt(tdefname, "AvSat", 1, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 6);
    if  (::GetPrivateProfileInt(tdefname, "AvHol", 0, pfilepath))
    	bj_times.tc_nvaldays |= (1 << 7);
    bj_times.tc_mday = ::GetPrivateProfileInt(tdefname, "Monthday", 0, pfilepath);
	bj_times.tc_rate = GetPrivateProfileUlong(tdefname, "Reprate", 5, pfilepath);

    //  Now for queueing options....
    
    bj_progress = BJP_NONE;
    if  (::GetPrivateProfileInt(qdefname, "Cancelled", 0, pfilepath))
    	bj_progress = BJP_CANCELLED;
    bj_pri = ::GetPrivateProfileInt(qdefname, "Priority", ma.m_mypriv.btu_defp, pfilepath);

    bj_jflags = (unsigned char) (ma.m_nonexport? 0: BJ_EXPORT);
    if  (::GetPrivateProfileInt(qdefname, "Export", 0, pfilepath))
	    bj_jflags |= BJ_EXPORT;
    if  (::GetPrivateProfileInt(qdefname, "RemRun", 0, pfilepath))
	    bj_jflags |= BJ_EXPORT|BJ_REMRUNNABLE;
	if  (::GetPrivateProfileInt(qdefname, "Noadv", 0, pfilepath))
		bj_jflags |= BJ_NOADVIFERR;

	::GetPrivateProfileString(qdefname, "Directory", "", ibuf, sizeof(ibuf), pfilepath);
	bj_direct = ibuf;

	::GetPrivateProfileString(qdefname, "Interp", ma.m_cilist.def_cmdint()->ci_name, ibuf, sizeof(ibuf), pfilepath);
	bj_cmdinterp = ibuf;
    bj_ll = ::GetPrivateProfileInt(qdefname, "Loadlev", ma.m_cilist.cmdint_ll(ibuf), pfilepath);
    bj_umask = (unsigned short) GetPrivateProfileUlong(qdefname, "Umask", ma.m_umask, pfilepath);
    bj_ulimit = GetPrivateProfileUlong(qdefname, "Ulimit", ma.m_ulimit, pfilepath);
    if  (bj_ulimit == 0)
    	bj_ulimit = 0x80000;
    bj_deltime = ::GetPrivateProfileInt(qdefname, "Deltime", 0, pfilepath);
    bj_runtime = GetPrivateProfileUlong(qdefname, "Runtime", 0, pfilepath);
    bj_autoksig = ::GetPrivateProfileInt(qdefname, "Whichsig", 0, pfilepath);
    bj_runon = ::GetPrivateProfileInt(qdefname, "Runon", 0, pfilepath);

	::GetPrivateProfileString(qdefname, "Normal", "0,0", ibuf, sizeof(ibuf), pfilepath);
	char	*cp = ibuf;
	int		n1 = 0, n2 = 0;
	while  (isdigit(*cp))
		n1 = n1 * 10 + *cp++ - '0';
	if  (*cp == ',')  {
		cp++;
		while  (isdigit(*cp))
			n2 = n2 * 10 + *cp++ - '0';
	}
	else
		n2 = n1;
	bj_exits.nlower = n1;
	bj_exits.nupper = n2;
	::GetPrivateProfileString(qdefname, "Error", "1,255", ibuf, sizeof(ibuf), pfilepath);
	cp = ibuf;
	n1 = n2 = 0;
	while  (isdigit(*cp))
		n1 = n1 * 10 + *cp++ - '0';
	if  (*cp == ',')  {
		cp++;
		while  (isdigit(*cp))
			n2 = n2 * 10 + *cp++ - '0';
	}
	else
		n2 = n1;
	bj_exits.elower = n1;
	bj_exits.eupper = n2;

	bj_mode.u_flags = (unsigned short) GetPrivateProfileUlong(qdefname, "JUmode", ma.m_mypriv.btu_jflags[0], pfilepath);
	bj_mode.g_flags = (unsigned short) GetPrivateProfileUlong(qdefname, "JGmode", ma.m_mypriv.btu_jflags[1], pfilepath);
	bj_mode.o_flags = (unsigned short) GetPrivateProfileUlong(qdefname, "JOmode", ma.m_mypriv.btu_jflags[2], pfilepath);

	for  (int jn = 0;  jn < MAXCVARS;  jn++)  {
		char	ebuf[20];
		wsprintf(ebuf, "Ccrit%d", jn);
		int	 Crit = ::GetPrivateProfileInt(qdefname, ebuf, 0, pfilepath);
		wsprintf(ebuf, "Cond%d", jn);
		if  (::GetPrivateProfileString(qdefname, ebuf, "", ibuf, sizeof(ibuf), pfilepath) == 0  ||
        		!parsecond(ibuf, bj_conds[jn], Crit))  {
        	bj_conds[jn].bjc_compar = C_UNUSED;     
        	break;
        }
	}

	for  (jn = 0;  jn < MAXSEVARS;  jn++)  {
		char	ebuf[20];
		wsprintf(ebuf, "Acrit%d", jn);
		int	 Crit = ::GetPrivateProfileInt(qdefname, ebuf, 0, pfilepath);
		wsprintf(ebuf, "Aflags%d", jn);
		unsigned  Flags = (unsigned short) GetPrivateProfileUlong(qdefname, ebuf, 0, pfilepath);
		wsprintf(ebuf, "Ass%d", jn);
		if  (::GetPrivateProfileString(qdefname, ebuf, "", ibuf, sizeof(ibuf), pfilepath) == 0  ||
				!parseass(ibuf, bj_asses[jn], Crit, Flags))  {
			bj_asses[jn].bja_op = BJA_NONE;
			break;                         
		}
	}
    
    bj_nargs = 0;
	for  (jn = 0;; jn++)  {
		char	ebuf[20];
		wsprintf(ebuf, "Arg%d", jn);
		if  (::GetPrivateProfileString(qdefname, ebuf, "", ibuf, sizeof(ibuf), pfilepath) == 0)
			break;
		if  (!parsearg(*this, ibuf))
			break;
	}
	bj_nredirs = 0;
	for  (jn = 0;; jn++)  {
		char	ebuf[20];
		wsprintf(ebuf, "Redir%d", jn);
		if  (::GetPrivateProfileString(qdefname, ebuf, "", ibuf, sizeof(ibuf), pfilepath) == 0)
			break;
		if  (!parseredir(*this, ibuf))
			break;
	}

	//  Defaults for new conditiona and assignments
	
	Jcond	&jc = ma.m_defcond;
	Btcon  &jcc = jc.bjc_value;
	
	if  (::GetPrivateProfileString(cdefname, "Value", "", ibuf, sizeof(ibuf), pfilepath) > 0  &&
			(isdigit(ibuf[0])  ||  ibuf[0] == '-'))  {
		jcc.const_type = CON_LONG;
		jcc.con_long = atol(ibuf);
	}
	else  {
		jcc.const_type = CON_STRING;
		strncpy(jcc.con_string, ibuf, BTC_VALUE);
		jcc.con_string[BTC_VALUE] = '\0';
	}
	jc.bjc_compar = ::GetPrivateProfileInt(cdefname, "Op", 0, pfilepath);
	jc.bjc_iscrit = ::GetPrivateProfileInt(cdefname, "Crit", 0, pfilepath);

	Jass  &ja = ma.m_defass;
	Btcon  &jac = ja.bja_con;
	if  (::GetPrivateProfileString(adefname, "Value", "", ibuf, sizeof(ibuf), pfilepath) > 0  &&
			(isdigit(ibuf[0])  ||  ibuf[0] == '-'))  {
		jac.const_type = CON_LONG;
		jac.con_long = atol(ibuf);
	}
	else  {
		jac.const_type = CON_STRING;
		strncpy(jac.con_string, ibuf, BTC_VALUE);
		jac.con_string[BTC_VALUE] = '\0';
	}
	ja.bja_op = ::GetPrivateProfileInt(adefname, "Op", 0, pfilepath); 
	ja.bja_iscrit = ::GetPrivateProfileInt(adefname, "Crit", 0, pfilepath);
	ja.bja_flags = 0;
	if  (::GetPrivateProfileInt(adefname, "Start", 1, pfilepath))
		ja.bja_flags |= BJA_START;
	if  (::GetPrivateProfileInt(adefname, "Reverse", 1, pfilepath))
		ja.bja_flags |= BJA_REVERSE;
	if  (::GetPrivateProfileInt(adefname, "Normal", 1, pfilepath))
		ja.bja_flags |= BJA_OK;
	if  (::GetPrivateProfileInt(adefname, "Error", 1, pfilepath))
		ja.bja_flags |= BJA_ERROR;
	if  (::GetPrivateProfileInt(adefname, "Abort", 1, pfilepath))
		ja.bja_flags |= BJA_ABORT;
	if  (::GetPrivateProfileInt(adefname, "Cancel", 0, pfilepath))
		ja.bja_flags |= BJA_CANCEL;
}	
#endif

inline	void	WritePrivateProfileInt(const char *section, const char *field, const long value, const char *file)
{
	char	obuf[24];
	
	wsprintf(obuf, "%ld", value);
	::WritePrivateProfileString(section, field, obuf, file);
}

inline	void	WritePrivateProfileHex(const char *section, const char *field, const unsigned long value, const char *file)
{
	char	obuf[12];
	
	wsprintf(obuf, "0x%lx", value);
	::WritePrivateProfileString(section, field, obuf, file);
}

inline	void	WritePrivateProfileBool(const char *section, const char *field, const int value, const char *file)
{
	WritePrivateProfileInt(section, field, value? 1: 0, file);
}

inline	void	appendlong(const long item, CString &resultcmd)
{
	char	thing[24];
	wsprintf(thing, "%ld", item);
	resultcmd += thing;
}

extern	void	dumpredir(const Mredir &, CString &);

void	Btjob::optcopy()
{
	char	pfilepath[_MAX_PATH];
    strcpy(pfilepath, basedir);
    strcat(pfilepath, WININI);

	if  (!bj_title.IsEmpty())  {
		int	 colp = bj_title.Find(':');
		if  (colp > 0)  {
		    ::WritePrivateProfileString(poptname, "Jobqueue", (const char *) bj_title.Left(colp-1), pfilepath);
			int  lng = bj_title.GetLength() - colp - 1;
			if  (lng > 0)
			    ::WritePrivateProfileString(qdefname, "Title", (const char *) bj_title.Right(lng), pfilepath);
		}
		else
		    ::WritePrivateProfileString(qdefname, "Title", (const char *) bj_title, pfilepath);
	}

	//  User and group???
	//  Time defaults
	
    WritePrivateProfileInt(tdefname, "Notposs", bj_times.tc_nposs, pfilepath);
    WritePrivateProfileInt(tdefname, "Repeat", bj_times.tc_repeat, pfilepath);
    WritePrivateProfileBool(tdefname, "AvSun", bj_times.tc_nvaldays & (1 << 0), pfilepath);
    WritePrivateProfileBool(tdefname, "AvMon", bj_times.tc_nvaldays & (1 << 1), pfilepath);
	WritePrivateProfileBool(tdefname, "AvTue", bj_times.tc_nvaldays & (1 << 2), pfilepath);
    WritePrivateProfileBool(tdefname, "AvWed", bj_times.tc_nvaldays & (1 << 3), pfilepath);
    WritePrivateProfileBool(tdefname, "AvThu", bj_times.tc_nvaldays & (1 << 4), pfilepath);
    WritePrivateProfileBool(tdefname, "AvFri", bj_times.tc_nvaldays & (1 << 5), pfilepath);
    WritePrivateProfileBool(tdefname, "AvSat", bj_times.tc_nvaldays & (1 << 6), pfilepath);
    WritePrivateProfileBool(tdefname, "AvHol", bj_times.tc_nvaldays & (1 << 7), pfilepath);
    WritePrivateProfileInt(tdefname, "Monthday", bj_times.tc_mday, pfilepath);
	WritePrivateProfileInt(tdefname, "Reprate", bj_times.tc_rate, pfilepath);

    //  Now for queueing options....
    
	WritePrivateProfileBool(qdefname, "Cancelled", bj_progress > BJP_DONE, pfilepath);
	WritePrivateProfileInt(qdefname, "Priority", bj_pri, pfilepath);
	WritePrivateProfileBool(qdefname, "RemRun", bj_jflags & BJ_REMRUNNABLE, pfilepath);
	WritePrivateProfileBool(qdefname, "Export", bj_jflags & BJ_EXPORT, pfilepath);

	if  (!bj_direct.IsEmpty())
    	::WritePrivateProfileString(qdefname, "Directory", (const char *) bj_direct, pfilepath);
    
    ::WritePrivateProfileString(qdefname, "Interp", bj_cmdinterp, pfilepath);
    WritePrivateProfileInt(qdefname, "Loadlev", bj_ll, pfilepath);
	WritePrivateProfileHex(qdefname, "Umask", bj_umask, pfilepath);
    WritePrivateProfileHex(qdefname, "Ulimit", bj_ulimit, pfilepath);
    WritePrivateProfileInt(qdefname, "Deltime", bj_deltime, pfilepath);
    WritePrivateProfileInt(qdefname, "Runtime", bj_runtime, pfilepath);
    WritePrivateProfileInt(qdefname, "Whichsig", bj_autoksig, pfilepath);
    WritePrivateProfileInt(qdefname, "Runon", bj_runon, pfilepath);

    char  ebuf[50];
    wsprintf(ebuf, "%d,%d", bj_exits.nlower, bj_exits.nupper);
    ::WritePrivateProfileString(qdefname, "Normal", ebuf, pfilepath);
    wsprintf(ebuf, "%d,%d", bj_exits.elower, bj_exits.eupper);
    ::WritePrivateProfileString(qdefname, "Error", ebuf, pfilepath);
    WritePrivateProfileBool(qdefname, "Noadv", bj_jflags & BJ_NOADVIFERR, pfilepath);
	WritePrivateProfileHex(qdefname, "JUmode", bj_mode.u_flags, pfilepath);
	WritePrivateProfileHex(qdefname, "JGmode", bj_mode.g_flags, pfilepath);
	WritePrivateProfileHex(qdefname, "JOmode", bj_mode.o_flags, pfilepath);    

	for  (int jn = 0;  jn < MAXCVARS;  jn++)  {
		const Jcond	&jc = bj_conds[jn];
		CString  resultcmd;
		wsprintf(ebuf, "Ccrit%d", jn);        
		
		if  (jc.bjc_compar == C_UNUSED)
	        WritePrivateProfileBool(qdefname, ebuf, FALSE, pfilepath);
	    else  {
    	    WritePrivateProfileBool(qdefname, ebuf, jc.bjc_iscrit != 0, pfilepath);

			CString	opn;
			opn.LoadString(IDS_CONDOP + jc.bjc_compar);

#ifdef	BTQW        
			Btvar	&vp = *Vars()[jc.bjc_varind];
			resultcmd = look_host(vp.hostid);
			resultcmd += ':';
			resultcmd += vp.var_name;
			resultcmd += opn;
#endif
#ifdef	BTRW
			resultcmd = jc.bjc_var + opn;
#endif
			if  (jc.bjc_value.const_type == CON_STRING)  {
				if  (isdigit(jc.bjc_value.con_string[0]))
					resultcmd += ':';
				resultcmd += jc.bjc_value.con_string;
			}
			else
				appendlong(jc.bjc_value.con_long, resultcmd);
	    }
		wsprintf(ebuf, "Cond%d", jn);
    	::WritePrivateProfileString(qdefname, ebuf, (const char *) resultcmd, pfilepath);
	}

	for  (jn = 0;  jn < MAXSEVARS;  jn++)  {
		const Jass	&ja = bj_asses[jn];
		CString  resultcmd;

		wsprintf(ebuf, "Acrit%d", jn);        
		if  (ja.bja_op == BJA_NONE)  {
        	WritePrivateProfileBool(qdefname, ebuf, FALSE, pfilepath);
			wsprintf(ebuf, "Aflags%d", jn);        
	    	WritePrivateProfileHex(qdefname, ebuf, 0, pfilepath);
        }
        else  {
			WritePrivateProfileBool(qdefname, ebuf, ja.bja_iscrit != 0, pfilepath);

			if  (ja.bja_op < BJA_SEXIT)  {
				wsprintf(ebuf, "Aflags%d", jn);        
 		    	WritePrivateProfileHex(qdefname, ebuf, ja.bja_flags, pfilepath);
			}

#ifdef	BTQW
			Btvar	&vp = *Vars()[ja.bja_varind];
			resultcmd = look_host(vp.hostid);
			resultcmd += ':';
			resultcmd += vp.var_name;
#endif
#ifdef	BTRW
			resultcmd = ja.bja_var;
#endif
			if  (ja.bja_op >= BJA_SEXIT)
				resultcmd += ja.bja_op == BJA_SEXIT? "=?": "=!";
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
					appendlong(ja.bja_con.con_long, resultcmd);
			}
		}
		wsprintf(ebuf, "Ass%d", jn);
	    ::WritePrivateProfileString(qdefname, ebuf, (const char *) resultcmd, pfilepath);
	}
	
	for  (unsigned  cnta = 0;  cnta < bj_nargs;  cnta++)  {
		wsprintf(ebuf, "Arg%d", cnta);
		::WritePrivateProfileString(qdefname, ebuf, bj_arg[cnta], pfilepath);
	}
	//  Final null one to mark the end
	wsprintf(ebuf, "Arg%d", cnta);
	::WritePrivateProfileString(qdefname, ebuf, "", pfilepath);
	
	for  (cnta = 0;  cnta < bj_nredirs;  cnta++)  {
		wsprintf(ebuf, "Redir%d", cnta);
		CString	rd;
		dumpredir(bj_redirs[cnta], rd);
		::WritePrivateProfileString(qdefname, ebuf, (const char *) rd, pfilepath);
	}
	//  Final null one to mark the end
	wsprintf(ebuf, "Redir%d", cnta);
	::WritePrivateProfileString(qdefname, ebuf, "", pfilepath);

#ifdef	BTRW
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	Jcond	&jc = ma.m_defcond;
	switch  (jc.bjc_value.const_type)  {
	default:
		WritePrivateProfileInt(cdefname, "Value", 0, pfilepath);
		break;
	case  CON_LONG:
		WritePrivateProfileInt(cdefname, "Value", jc.bjc_value.con_long, pfilepath);
		break;
	case  CON_STRING:
		::WritePrivateProfileString(cdefname, "Value", jc.bjc_value.con_string, pfilepath);
		break;
	}	
	WritePrivateProfileInt(cdefname, "Op", jc.bjc_compar, pfilepath);
	WritePrivateProfileBool(cdefname, "Crit", jc.bjc_iscrit, pfilepath);
    
    Jass  &ja = ma.m_defass;
	switch  (ja.bja_con.const_type)  {
	default:
		WritePrivateProfileInt(adefname, "Value", 0, pfilepath);
		break;
	case  CON_LONG:
		WritePrivateProfileInt(adefname, "Value", ja.bja_con.con_long, pfilepath);
		break;
	case  CON_STRING:
		::WritePrivateProfileString(adefname, "Value", ja.bja_con.con_string, pfilepath);
		break;
	}	
	WritePrivateProfileInt(adefname, "Op", ja.bja_op, pfilepath);
	WritePrivateProfileBool(adefname, "Crit", ja.bja_iscrit, pfilepath);
	WritePrivateProfileBool(adefname, "Start", ja.bja_flags & BJA_START, pfilepath);
	WritePrivateProfileBool(adefname, "Reverse", ja.bja_flags & BJA_REVERSE, pfilepath);
	WritePrivateProfileBool(adefname, "Normal", ja.bja_flags & BJA_OK, pfilepath);
	WritePrivateProfileBool(adefname, "Error", ja.bja_flags & BJA_ERROR, pfilepath);
	WritePrivateProfileBool(adefname, "Abort", ja.bja_flags & BJA_ABORT, pfilepath);
	WritePrivateProfileBool(adefname, "Cancel", ja.bja_flags & BJA_CANCEL, pfilepath);
#endif
}
