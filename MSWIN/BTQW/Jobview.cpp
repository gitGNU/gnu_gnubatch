#include "stdafx.h"
#include "jobdoc.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "jobview.h"
#include "mainfrm.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
                           
extern	int		calc_fmt_length(CString  CBtqwApp ::*);
extern	void	save_fmt_widths(CString  CBtqwApp ::*, const Formatrow *, const int);

IMPLEMENT_DYNCREATE(CJobView, CRowView)

/////////////////////////////////////////////////////////////////////////////
CJobView::CJobView()
{
}

void CJobView::OnUpdate(CView*v, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
	for  (int fcnt = 0;  fcnt < m_nformats;  fcnt++)
		m_formats[fcnt].f_maxwidth = 0;
}

void CJobView::GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight)
{
	TEXTMETRIC textm;
	pDC->GetTextMetrics(&textm);
	nRowWidth = textm.tmAveCharWidth * calc_fmt_length(&CBtqwApp::m_jfmt);
	nRowHeight = textm.tmHeight;
	m_avecharwidth = textm.tmAveCharWidth;
}

int CJobView::GetActiveRow()
{
	return  ((CJobdoc*)GetDocument())->jindex(curr_job);
}

unsigned CJobView::GetRowCount()
{
	return  ((CJobdoc*)GetDocument())->number();
}

void CJobView::ChangeSelectionToRow(int nRow)
{
	CJobdoc* pDoc = GetDocument();
	Btjob *j = (*pDoc)[nRow];
	if  (j)
		curr_job = jident(*j);
	else
		curr_job = jident();
}               

static	CString	result;
static	CJobdoc	*current_doc;

static	int	fmt_args(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	
	for  (unsigned ac = 0;  ac < jb.bj_nargs;  ac++)  {
		if  (ac != 0)
			result += ',';
		result += jb.bj_arg[ac];
	}
    
	return  result.GetLength();
}

static	int  fmt_avoid(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable || jb.bj_times.tc_istime == 0)
		return  0;
	
	for  (int nd = 0;  nd < TC_NDAYS;  nd++)
		if  (jb.bj_times.tc_nvaldays & (1 << nd))  {
			if  (!result.IsEmpty())
					result += ',';
			CString	dayname;
			dayname.LoadString(IDS_SUN + nd);
			result += dayname;
		}
    
	return  result.GetLength();
}

static	int  fmt_condfull(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	
	for  (int  uc = 0;  uc < MAXCVARS;  uc++)  {
		const Jcond	&jc = jb.bj_conds[uc];
		
		if  (jc.bjc_compar == C_UNUSED)
			break;
		
		Btvar	*vp = Vars()[jc.bjc_varind];
		
		if  (uc != 0)
			result += ',';
		if  (vp)  {
			result += look_host(vp->hostid);
			result += ':';
			result += vp->var_name;
		}
		else
			result += '?';
		
		CString	opn;
		opn.LoadString(IDS_CONDOP + jc.bjc_compar);
		
		result += opn;

		if  (jc.bjc_value.const_type == CON_STRING)
			result += jc.bjc_value.con_string;
		else  {
			char	thing[24];
			wsprintf(thing, "%ld", jc.bjc_value.con_long);
			result += thing;
		} 
	}

	return  result.GetLength();
}

static	int  fmt_cond(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	
	for  (int  uc = 0;  uc < MAXCVARS;  uc++)  {
		const Jcond	&jc = jb.bj_conds[uc];
		
		if  (jc.bjc_compar == C_UNUSED)
			break;
		
		Btvar	*vp = Vars()[jc.bjc_varind];
		
		if  (uc != 0)
			result += ',';
		if  (vp)
			result += vp->var_name;
		else
			result += '?';
	}

	if  (result.GetLength() > fwidth)
		result.LoadString(IDS_LOTSCONDS);

	return  result.GetLength();
}

static	int  fmt_dir(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	result = jb.bj_direct;
	return  result.GetLength();
}

static	int  fmt_env(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
		
	for  (unsigned ec = 0;  ec < jb.bj_nenv;  ec++)  {
		if  (ec != 0)
			result += ',';
		result += jb.bj_env[ec].e_name;
		result += '=';
		result += jb.bj_env[ec].e_value;
	}
	
	return  result.GetLength();
}

static	int  fmt_group(const Btjob &jb, const int isreadable, const int fwidth)
{
	result = jb.bj_mode.o_group;	
	return  result.GetLength();
}

static	int  fmt_title(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
		
	int	colp;
	
	if  (!current_doc->m_wrestrict.queuename.IsEmpty()  &&  (colp = jb.bj_title.Find(':')) >= 0)
		result = jb.bj_title.Right(jb.bj_title.GetLength() - colp - 1);
	else
		result = jb.bj_title;
	return  result.GetLength();
}

static	int  fmt_interp(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	result = jb.bj_cmdinterp;
	return  result.GetLength();
}

static	int  fmt_loadlev(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	char	fmt[10], thing[20];
	wsprintf(fmt, "%%%du", fwidth > 19? 19: fwidth);
	wsprintf(thing, fmt, jb.bj_ll);
	result = thing;
	return  result.GetLength();
}

static	void dumpmode(const char *prefix, const unsigned md)
{
	result += prefix;
	result += ':';
	if  (md & BTM_READ)
		result += 'R';
	if  (md & BTM_WRITE)
		result += 'W';
	if  (md & BTM_SHOW)
		result += 'S';
	if  (md & BTM_RDMODE)
		result += 'M';
	if  (md & BTM_WRMODE)
		result += 'P';
	if  (md & BTM_UGIVE)
		result += 'U';
	if  (md & BTM_UTAKE)
		result += 'V';
	if  (md & BTM_GGIVE)
		result += 'G';
	if  (md & BTM_GTAKE)
		result += 'H';
	if  (md & BTM_DELETE)
		result += 'D';
	if  (md & BTM_KILL)
		result += 'K';
}

static	int  fmt_mode(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!jb.bj_mode.mpermitted(BTM_RDMODE))
		return  0;
		
	dumpmode("U", jb.bj_mode.u_flags);
	dumpmode(",G", jb.bj_mode.g_flags);
	dumpmode(",O", jb.bj_mode.o_flags);
	return  result.GetLength();
}

static	int  fmt_umask(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	for  (int  shf = 6;  shf >= 0;  shf -= 3)
		result += char(((jb.bj_umask >> shf) & 7) + '0');
	return  3;
}

static	int  fmt_jobno(const Btjob &jb, const int isreadable, const int fwidth)
{
	result = look_host(jb.hostid);
	char  thing[20];
	wsprintf(thing, ":%ld", jb.bj_job);
    result += thing;
	return  result.GetLength();
}

static	int  fmt_progress(const Btjob &jb, const int isreadable, const int fwidth)
{
	result.LoadString(IDS_PROG_NONE + jb.bj_progress);
	if  (jb.bj_progress == BJP_RUNNING  &&  jb.bj_runhostid != jb.hostid)  {
		result += ':';
		result += look_host(jb.bj_runhostid);
	}
	return  result.GetLength();
}

static	int  fmt_prio(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	char	fmt[10], thing[20];
	wsprintf(fmt, "%%%du", fwidth > 19? 19: fwidth);
	wsprintf(thing, fmt, jb.bj_pri);
	result = thing;
	return  result.GetLength();
}

static	int  fmt_pid(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable || jb.bj_progress != BJP_RUNNING)
		return  0;
	char	fmt[10], thing[40];
	wsprintf(fmt, "%%%dld", fwidth > 39? 39: fwidth);
	wsprintf(thing, fmt, jb.bj_pid);
	result = thing;
	return  result.GetLength();
}

static	int  fmt_queue(const Btjob &jb, const int isreadable, const int fwidth)
{
	int	 colp;
	if  (!isreadable || (colp = jb.bj_title.Find(':')) <= 0)
		return  0;
	result = jb.bj_title.Left(colp);
	return  colp;
}

static	int  fmt_qtit(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	int	 colp;
	if  ((colp = jb.bj_title.Find(':')) >= 0)  {
		int	 lng = jb.bj_title.GetLength() - colp - 1;
		result = jb.bj_title.Right(lng);
		return  lng;
	}
	result = jb.bj_title;
	return  result.GetLength();
}

static	int  fmt_redirs(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;

	for  (unsigned  rc = 0;  rc < jb.bj_nredirs;  rc++)  {
		Mredir	&rd = jb.bj_redirs[rc];

		if  (rc != 0)
			result += ',';

		char	thing[10];
		wsprintf(thing, "%d", rd.fd);
		
		switch  (rd.action)  {
		case  RD_ACT_RD:
			if  (rd.fd != 0)
				result += thing;
			result += '<';
			break;
		case  RD_ACT_WRT:
			if  (rd.fd != 1)
				result += thing;
			result += '>';
			break;
		case  RD_ACT_APPEND:
			if  (rd.fd != 1)
				result += thing;
			result += ">>";
			break;
		case  RD_ACT_RDWR:
			if  (rd.fd != 0)
				result += thing;
			result += "<>";
			break;
		case  RD_ACT_RDWRAPP:
			if  (rd.fd != 0)
				result += thing;
			result += "<>>";
			break;
		case  RD_ACT_PIPEO:
			if  (rd.fd != 1)
				result += thing;
			result += '|';
			break;
		case  RD_ACT_PIPEI:
			if  (rd.fd != 0)
				result += thing;
			result += "<|";
			break;
		case  RD_ACT_CLOSE:
			if  (rd.fd != 1)
				result += thing;
			result += ">&-";
			continue;
		case  RD_ACT_DUP:
			if  (rd.fd != 1)
				result += thing;
			wsprintf(thing, ">&%d", rd.arg);
			result += thing;
			continue;
		}
		result += rd.buffer;
	}
	return  result.GetLength();
}

static	int  fmt_repeat(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable || jb.bj_times.tc_istime == 0)
		return  0;
	                                            
	if  (jb.bj_times.tc_repeat < TC_MINUTES)
		result.LoadString(IDS_DELETE + jb.bj_times.tc_repeat - TC_DELETE);
	else  {
		result.LoadString(IDS_MINUTES + jb.bj_times.tc_repeat - TC_MINUTES);
		char	thing[24];
		wsprintf(thing, ":%ld", jb.bj_times.tc_rate);
		result += thing;
		if  (jb.bj_times.tc_repeat == TC_MONTHSB  ||  jb.bj_times.tc_repeat == TC_MONTHSE)  {
			wsprintf(thing, ":%d", jb.bj_times.tc_mday);
			result += thing;
		}
	}
	return  result.GetLength();
}

static	int  fmt_assfull(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	
	for  (int  uc = 0;  uc < MAXSEVARS;  uc++)  {
		const Jass	&ja = jb.bj_asses[uc];
		
		if  (ja.bja_op == BJA_NONE)
			break;
		
		if  (uc != 0)
			result += ',';

		if  (ja.bja_op < BJA_SEXIT)  {
			if  (ja.bja_flags & BJA_START)
				result += 'S';
			if  (ja.bja_flags & BJA_REVERSE)
				result += 'R';
			if  (ja.bja_flags & BJA_OK)
				result += 'N';
			if  (ja.bja_flags & BJA_ERROR)
				result += 'E';
			if  (ja.bja_flags & BJA_ABORT)
				result += 'A';
			if  (ja.bja_flags & BJA_CANCEL)
				result += 'C';
			result += ':';
		}

		Btvar	*vp = Vars()[ja.bja_varind];
			
		if  (vp)  {
			result += look_host(vp->hostid);
			result += ':';
			result += vp->var_name;
		}
		else
			result += '?';
		
		CString	opn;
		opn.LoadString(IDS_ASSOP + ja.bja_op);
		if  (ja.bja_op >= BJA_SEXIT)  {
			result += '=';
			result += opn;
		}
		else  {
			result += opn;
			if  (ja.bja_con.const_type == CON_STRING)
				result += ja.bja_con.con_string;
			else  {
				char	thing[20];
				wsprintf(thing, "%ld", ja.bja_con.con_long);
				result += thing;
			}
		}
	}
	return  result.GetLength();
}

static	int  fmt_ass(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	
	for  (int  uc = 0;  uc < MAXSEVARS;  uc++)  {
		const Jass	&ja = jb.bj_asses[uc];
		if  (ja.bja_op == BJA_NONE)
			break;
		if  (uc != 0)
			result += ',';
		Btvar	*vp = Vars()[ja.bja_varind];
		if  (vp)
			result += vp->var_name;
		else
			result += '?';
	}
	if  (result.GetLength() > fwidth)
		result.LoadString(IDS_LOTSASSES);
	return  result.GetLength();
}

static	int  fmt_timefull(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable || jb.bj_times.tc_istime == 0)
		return  0;
	                                            
	time_t	w = jb.bj_times.tc_nexttime;
	tm  *t = localtime(&w);
	
	int	day = t->tm_mday, mon = t->tm_mon+1;
	if  (timezone >= 4 * 60 * 60)  {
		day = mon;
		mon = t->tm_mday;
	}
	
	char	thing[20];
	wsprintf(thing, "%.2d/%.2d/%.2d %.2d:%.2d", day, mon, t->tm_year % 100, t->tm_hour, t->tm_min);
	result = thing;
	return  result.GetLength();
}

static	int  fmt_time(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable || jb.bj_times.tc_istime == 0)
		return  0;
	                                            
	time_t	w = jb.bj_times.tc_nexttime, now = time(NULL);
	tm  *t = localtime(&w);
	if  (!t)
		return  0;
	int	t1 = t->tm_hour, t2 = t->tm_min;
	char  sep = ':';
    if  (w < now || w - now > 24L * 60L * 60L)  {
		t1 = t->tm_mday;
		t2 = t->tm_mon+1;
		if  (timezone >= 4 * 60 * 60)  {
			t1 = t2;
			t2 = t->tm_mday;
		}
		sep = '/';
	}
	char	thing[8];
	wsprintf(thing, "%.2d%c%.2d", t1, sep, t2);
	result = thing;
	return  result.GetLength();
}

static	int  fmt_user(const Btjob &jb, const int isreadable, const int fwidth)
{
	result = jb.bj_mode.o_user;	
	return  result.GetLength();
}

static	int  fmt_ulimit(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	char	fmt[10], thing[40];
	wsprintf(fmt, "%%%dlx", fwidth > 39? 39: fwidth);
	wsprintf(thing, fmt, jb.bj_ulimit);
	result = thing;
	return  result.GetLength();
}

static	int  fmt_exits(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	char	thing[40];
	wsprintf(thing, "N(%d,%d),E(%d,%d)",
			jb.bj_exits.nlower, jb.bj_exits.nupper,
			jb.bj_exits.elower, jb.bj_exits.eupper);
	result = thing;
	return  result.GetLength();
}

static	int  fmt_orighost(const Btjob &jb, const int isreadable, const int fwidth)
{
	result = look_host(jb.bj_orighostid);
	return  result.GetLength();
}

static	int	fmt_seq(const Btjob &jb, const int isreadable, const int fwidth)
{
	char	fmt[10], thing[20];
	wsprintf(fmt, "%%%du", fwidth > 19? 19: fwidth);
	wsprintf(thing, fmt, current_doc->jindex(jb));
	result += thing;
	return  result.GetLength();
}

static	int	fmt_export(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (jb.bj_jflags & BJ_REMRUNNABLE)
		result.LoadString(IDS_REMRUNNABLE);
	else	if  (jb.bj_jflags & BJ_EXPORT)
		result.LoadString(IDS_EXPORT);
	return  result.GetLength();
}

static	int	ptimes(const time_t w)
{
	if  (w == 0)
		return  0;
		
	tm  *t = localtime(&w);
	
	time_t	now = time(NULL);
	now = (now / (24L*60L*60L)) * (24L*60L*60L);
	int	t1 = t->tm_hour, t2 = t->tm_min;
	char	sep = ':';
	if  (w < now  ||  w > now + 24L*60L*60L)  {
		t1 = t->tm_mday;
		t2 = t->tm_mon+1;
		if  (timezone >= 4 * 60 * 60)  {
			t2 = t1;
			t1 = t->tm_mday;
		}
		sep = '/';
	}
	char	thing[20];
	wsprintf(thing, "%.2d%c%.2d", t1, sep, t2);
	result = thing;
	return  result.GetLength();
}

static	int	fmt_stime(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable)
		return  ptimes(jb.bj_stime);
	return  0;
}

static	int	fmt_etime(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable)
		return  ptimes(jb.bj_etime);
	return  0;
}

static	int	fmt_otime(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable)
		return  ptimes(jb.bj_time);
	return  0;
}

static	int	fmt_itime(const Btjob &jb, const int isreadable, const int fwidth)
{
	switch  (jb.bj_progress)  {
	default:
		return  0;
	case  BJP_FINISHED:
	case  BJP_ERROR:
	case  BJP_ABORTED:
		return  fmt_etime(jb, isreadable, fwidth);
	case  BJP_STARTUP1:
	case  BJP_STARTUP2:
	case  BJP_RUNNING:
		return  fmt_stime(jb, isreadable, fwidth);
	case  BJP_DONE:
		if  (jb.bj_etime)
			return  fmt_etime(jb, isreadable, fwidth);
	case  BJP_CANCELLED:
		return  fmt_time(jb, isreadable, fwidth);
	case  BJP_NONE:
		if  (!jb.bj_times.tc_istime  &&  jb.bj_times.tc_nexttime < time(NULL))
			return  fmt_etime(jb, isreadable, fwidth);
		return  fmt_time(jb, isreadable, fwidth);
	}
}

static	int	fmt_xit(const Btjob &jb, const int isreadable, const int fwidth)
{
	char	format[10], thing[40];
	wsprintf(format, "%%.%dd", fwidth > 39? 39: fwidth);
	wsprintf(thing, format, jb.bj_lastexit >> 8);
	result = thing;
	return  result.GetLength();
}	

static	int	fmt_sig(const Btjob &jb, const int isreadable, const int fwidth)
{
	char	format[10], thing[40];
	wsprintf(format, "%%.%dd", fwidth > 39? 39: fwidth);
	wsprintf(thing, format, jb.bj_lastexit & 255);
	result = thing;
	return  result.GetLength();
}	

static	int	fmt_deltime(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable  &&  jb.bj_deltime != 0)  {
		char	format[10], thing[40];
		wsprintf(format, "%%%du", fwidth > 39? 39: fwidth);
		wsprintf(thing, format, jb.bj_deltime);
		result = thing;
		return  result.GetLength();
	}
	return  0;
}

static	int	fmt_runtime(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable  &&  jb.bj_runtime != 0)  {
		unsigned  long	hrs, mns, secs;
		hrs = jb.bj_runtime / 3600L;
		mns = jb.bj_runtime % 3600L;
		secs = mns % 60L;
		mns /= 60L;
		char	format[20], thing[40];
		if  (hrs != 0)  {
			int	resw = fwidth - 6;
			wsprintf(format, "%%%dlu:%%.2u:%%.2u", resw < 0? 0: resw > 39? 39: resw);
			wsprintf(thing, format, hrs, unsigned(mns), unsigned(secs));
		}
		else  if  (mns != 0)  {
			int	resw = fwidth - 3;
			wsprintf(format, "%%%dlu:%%.2u", resw < 0? 0: resw > 39? 39: resw);
			wsprintf(thing, format, mns, unsigned(secs));
		}
		else  {
			wsprintf(format, "%%%dlu", fwidth > 39? 39: fwidth);
			wsprintf(thing, format, secs);
		}
		result = thing;
		return  result.GetLength();
	}
	return  0;
}

static	int	fmt_autoksig(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable  &&  jb.bj_runtime != 0)  {
		char	format[10], thing[40];
		wsprintf(format, "%%%du", fwidth > 39? 39: fwidth);
		wsprintf(thing, format, jb.bj_autoksig);
		result = thing;
		return  result.GetLength();
	}
	return  0;
}

static	int	fmt_gracetime(const Btjob &jb, const int isreadable, const int fwidth)
{
	if  (isreadable  &&  jb.bj_runtime != 0  &&  jb.bj_runon != 0)  {
		unsigned  mns, secs;
		mns = jb.bj_runon / 60;
		secs = jb.bj_runon % 60;
		char	format[20], thing[40];
		if  (mns != 0)  {
			int	resw = fwidth - 3;
			wsprintf(format, "%%%du:%%.2u", resw < 0? 0: resw > 39? 39: resw);
			wsprintf(thing, format, mns, secs);
		}
		else  {
			wsprintf(format, "%%%du", fwidth > 39? 39: fwidth);
			wsprintf(thing, format, secs);
		}
		result = thing;
		return  result.GetLength();
	}
	return  0;
}

typedef	int	(*fmt_fn)(const Btjob &, const int, const int);

static	fmt_fn	uppertab[] = {
	fmt_args,		//  A
	NULL,			//  B	
	fmt_condfull,	//  C
	fmt_dir,		//  D
	fmt_env,		//  E
	NULL,			//  F
	fmt_group,		//  G
	fmt_title,		//  H
	fmt_interp,		//  I
	NULL,			//	J
	NULL,			//	K
	fmt_loadlev,	//  L
	fmt_mode,		//  M
	fmt_jobno,		//  N
	fmt_orighost,	//  O
	fmt_progress,	//  P
	NULL,			//  Q
	fmt_redirs,		//  R
	fmt_assfull,	//  S
	fmt_timefull,	//  T
	fmt_user,		//  U
	NULL,			//  V
	fmt_itime,		//  W
	fmt_exits,		//  X
	NULL,			//  Y
	NULL			//  Z
},
	lowertab[] = {
	fmt_avoid,		//  a
	fmt_stime,		//  b
	fmt_cond,		//  c
	fmt_deltime,	//  d
	fmt_export,		//  e
	fmt_etime,		//  f
	fmt_gracetime,	//  g
	fmt_qtit,		//  h
	fmt_pid,		//  i
	NULL,			//  j
	fmt_autoksig,	//  k
	fmt_runtime,	//  l
	fmt_umask,		//  m
	fmt_seq,		//  n
	fmt_otime,		//  o
	fmt_prio,		//  p
	fmt_queue,		//  q
	fmt_repeat,		//  r
	fmt_ass,		//  s
	fmt_time,		//  t
	fmt_ulimit,		//  u
	NULL,			//  v
	NULL,			//  w
	fmt_xit,		//  x
	fmt_sig,		//  y
	NULL			//  z
};

void	CJobView::Initformats()
{
	CRowView::Initformats(&CBtqwApp::m_jfmt, (int (*const*)(...))uppertab, (int (*const*)(...))lowertab, IDS_SJFORMAT_A, IDS_SJFORMAT_aa, "Ldgiklnpuxy");
}

void CJobView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
{
	CBrush brushBackground;
	COLORREF crOldText = 0;
	COLORREF crOldBackground = 0;

	if  (bSelected) {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		crOldBackground = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		crOldText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else  {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
	}
	
	CRect rectSelection;
	pDC->GetClipBox(&rectSelection);
	rectSelection.top = y;
	rectSelection.bottom = y + m_nRowHeight;
	pDC->FillRect(&rectSelection, &brushBackground);
	
	current_doc = ((CJobdoc *)GetDocument());
	Btjob *cj = (*current_doc)[nRow];
	
	if  (cj)  {

		if  (!bSelected)
			pDC->SetTextColor(current_doc->m_jobcolours[cj->bj_progress]);
	
		Formatrow	*fl = m_formats;
		int	 currplace = -1, nominal_width = 0, fcnt = 0;

		BOOL	isreadable = cj->bj_mode.mpermitted(BTM_READ);

		while  (fcnt < m_nformats)  {

			if  (fl->f_issep)  {
				CString	&fld = fl->f_field;
				int	lng = fld.GetLength();
				int	wc;
				for  (wc = 0;  wc < lng;  wc++)
					if  (fld[wc] != ' ')
						break;
				if  (wc < lng)
					pDC->TextOut((nominal_width + wc) * m_avecharwidth, y, fld.Right(lng-wc));
				nominal_width += fl->f_width;
				fl++;
				fcnt++;
				continue;
			}

			int  lastplace = -1;
			int	 nn = fl->f_width;
			if  (fl->f_ltab)
				lastplace = currplace;
			currplace = nominal_width;
			result.Empty();
			int  inlen = (*(fmt_fn)fl->f_fmtfn)(*cj, isreadable, nn);

			if  (inlen > nn  &&  lastplace >= 0)  {
				nn += currplace - lastplace;
				currplace = lastplace;
			}
			if  (inlen > nn)  {
				if  (fl->f_skipright)  {
					if  (UINT(inlen) > fl->f_maxwidth)
						fl->f_maxwidth = inlen;
					pDC->TextOut(currplace * m_avecharwidth, y, result);
					break;
				}
				result = result.Left(nn);
			}
			if  (inlen > 0  &&  result[0] == ' ')  {
				for  (int  wc = 1;  wc < inlen  &&  result[wc] == ' ';  wc++)
					;
				inlen -= wc;
				pDC->TextOut((currplace + nn - inlen) * m_avecharwidth, y, result.Right(inlen));
			}
			else
				pDC->TextOut(currplace * m_avecharwidth, y, result);
			if  (UINT(inlen) > fl->f_maxwidth)
					fl->f_maxwidth = inlen;
			nominal_width += fl->f_width;
			fl++;
			fcnt++;
		}
	}

	// Restore the DC.
	if (bSelected)	{
		pDC->SetBkColor(crOldBackground);
		pDC->SetTextColor(crOldText);
	}
}

void	CJobView::JobAllChange(const BOOL plushdr)
{
	if  (plushdr)  {
		Initformats();
		SetUpHdr();
	}
	((CJobdoc *)GetDocument())->revisejobs(Jobs());
	UpdateScrollSizes();
}

void	CJobView::JobChgJob(const unsigned ind)
{
	Btjob	*jb = Jobs()[ind];
	if  (jb)  {
		int  myind = ((CJobdoc *)GetDocument())->jindex(*jb);
		if  (myind >= 0)
			RedrawRow(myind);
	}		
} 

void	CJobView::SaveWidthSettings()
{
	save_fmt_widths(&CBtqwApp ::m_jfmt, m_formats, m_nformats);
}

void	CJobView::Dopopupmenu(CPoint point)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_JOBFLOAT));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}

