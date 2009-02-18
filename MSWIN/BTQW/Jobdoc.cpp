// jobdoc.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "jobview.h"
#include "jdatadoc.h"
#include "jsearch.h"
#include "winopts.h"
#include "othersig.h"
#include "jperm.h"
#include "titledlg.h"
#include "argdlg.h"
#include "envlist.h"
#include "maildlg.h"
#include "procpar.h"
#include "redirlis.h"
#include "timedlg.h"
#include "jasslist.h"
#include "jcondlis.h"
#include "cpyoptdl.h"
#include "timelim.h"
#include <string.h>

BOOL  Smstr(const CString &, const char *, const BOOL = FALSE);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#if	_MSC_VER == 700
#define UNIXTODOSTIME	((70UL * 365UL + 68/4 + 1) * 24UL * 3600UL)
#else
#define	UNIXTODOSTIME	0
#endif

/////////////////////////////////////////////////////////////////////////////
// CJobdoc

IMPLEMENT_DYNCREATE(CJobdoc, CDocument)

CJobdoc::CJobdoc()
{
	m_sjtitle = m_suser = TRUE;
	m_swraparound = FALSE;
	CBtqwApp	*mw = (CBtqwApp *)AfxGetApp();
	m_wrestrict = mw->m_restrict;
	m_jobcolours = mw->m_appcolours;
	revisejobs(mw->m_appjoblist);
}

CJobdoc::~CJobdoc()
{
}

BEGIN_MESSAGE_MAP(CJobdoc, CDocument)
	//{{AFX_MSG_MAP(CJobdoc)
	ON_COMMAND(ID_SEARCH_SEARCHBACKWARDS, OnSearchSearchbackwards)
	ON_COMMAND(ID_SEARCH_SEARCHFOR, OnSearchSearchfor)
	ON_COMMAND(ID_SEARCH_SEARCHFORWARD, OnSearchSearchforward)
	ON_COMMAND(ID_ACTION_DELETEJOB, OnActionDeletejob)
	ON_COMMAND(ID_ACTION_SETCANCELLED, OnActionSetcancelled)
	ON_COMMAND(ID_ACTION_FORCERUN, OnActionForcerun)
	ON_COMMAND(ID_ACTION_KILLJOB, OnActionKilljob)
	ON_COMMAND(ID_ACTION_OTHERSIGNAL, OnActionOthersignal)
	ON_COMMAND(ID_ACTION_PERMISSIONS, OnActionPermissions)
	ON_COMMAND(ID_ACTION_QUITJOB, OnActionQuitjob)
	ON_COMMAND(ID_ACTION_TITLEPRILOADLEV, OnActionTitlepriloadlev)
	ON_COMMAND(ID_JOBS_ARGUMENTS, OnJobsArguments)
	ON_COMMAND(ID_JOBS_ENVIRONMENT, OnJobsEnvironment)
	ON_COMMAND(ID_JOBS_MAIL, OnJobsMail)
	ON_COMMAND(ID_JOBS_PROCPARAM, OnJobsProcparam)
	ON_COMMAND(ID_JOBS_REDIRECTIONS, OnJobsRedirections)
	ON_COMMAND(ID_JOBS_TIME, OnJobsTime)
	ON_COMMAND(ID_JOBS_VIEWJOB, OnJobsViewjob)
	ON_COMMAND(ID_CONDITIONS_JOBASSIGNMENTS, OnConditionsJobassignments)
	ON_COMMAND(ID_CONDITIONS_JOBCONDITIONS, OnConditionsJobconditions)
	ON_COMMAND(ID_ACTION_GOJOB, OnActionGojob)
	ON_COMMAND(ID_ACTION_SETRUNNABLE, OnActionSetrunnable)
	ON_COMMAND(ID_WINDOW_WINDOWOPTIONS, OnWindowWindowoptions)
	ON_COMMAND(ID_JOBS_ADVANCETIME, OnJobsAdvancetime)
	ON_COMMAND(ID_JOBS_UNQUEUEJOB, OnJobsUnqueuejob)
	ON_COMMAND(ID_JOBS_COPYJOB, OnJobsCopyjob)
	ON_COMMAND(ID_JOBS_COPYOPTIONSINJOB, OnJobsCopyoptionsinjob)
	ON_COMMAND(ID_JOBS_INVOKEBTRW, OnJobsInvokebtrw)
	ON_COMMAND(ID_JOBS_TIMELIMITS, OnJobsTimelimits)
	ON_COMMAND(ID_WJCOLOUR_ABORT, OnWjcolourAbort)
	ON_COMMAND(ID_WJCOLOUR_CANCELLED, OnWjcolourCancelled)
	ON_COMMAND(ID_WJCOLOUR_ERROR, OnWjcolourError)
	ON_COMMAND(ID_WJCOLOUR_FINISHED, OnWjcolourFinished)
	ON_COMMAND(ID_WJCOLOUR_READY, OnWjcolourReady)
	ON_COMMAND(ID_WJCOLOUR_RUNNING, OnWjcolourRunning)
	ON_COMMAND(ID_WJCOLOUR_STARTUP, OnWjcolourStartup)
	ON_COMMAND(ID_FILE_JCOLOUR_COPYTOPROG, OnFileJcolourCopytoprog)
	ON_COMMAND(ID_FILE_JCOLOUR_COPYTOWIN, OnFileJcolourCopytowin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void	CJobdoc::revisejobs(joblist &src)
{
	m_joblist.clear();                  
	for  (unsigned cnt = 0; cnt < src.number(); cnt++)  {
		Btjob	*j = src[cnt];
		if  (m_wrestrict.visible(*j))
			m_joblist.append(j);
	}
	UpdateAllViews(NULL);
}
		 
Btjob	*CJobdoc::GetSelectedJob(const unsigned perm, const BOOL notifrun, const BOOL moan)
{                 
	POSITION	p = GetFirstViewPosition();
	CJobView	*aview = (CJobView *)GetNextView(p);
	if  (aview)  {
		int	cr = aview->GetActiveRow();
		if  (cr >= 0)  {
			Btjob	 *result = (*this)[cr];
			if  (!result->bj_mode.mpermitted(perm))  {
				if  (moan)
					AfxMessageBox(IDP_NOTPERM, MB_OK|MB_ICONSTOP);
				return  NULL;
			}
			if  (notifrun  &&  result->bj_progress >= BJP_STARTUP1)  {
				if  (moan)
					AfxMessageBox(IDP_JRUNNING, MB_OK|MB_ICONSTOP);
				return  NULL;                 
			}					
			return  result;
		}
	}
	if  (moan)
		AfxMessageBox(IDP_NOJOBSELECTED, MB_OK|MB_ICONEXCLAMATION);
	return  NULL;
}
	
BOOL CJobdoc::Smatches(const CString str, const int ind)
{
	Btjob  *jb = (*this)[ind];
	if  (m_sjtitle && Smstr(str, (const char *) jb->bj_title))
		return  TRUE;
	if  (m_suser && Smstr(str, jb->bj_mode.o_user, TRUE))
		return  TRUE;
	return FALSE;
}	

void CJobdoc::DoSearch(const CString str, const BOOL forward)
{             
	int	cnt;
	Btjob  *cj = GetSelectedJob(BTM_READ, FALSE, FALSE);
	if  (forward)  {
		int	cwhich = cj? jindex(*cj): -1;
		for  (cnt = cwhich + 1;  cnt < int(number());  cnt++)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = 0;  cnt < cwhich;  cnt++)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	else  {
		int  cwhich = cj? jindex(*cj): number();
		for  (cnt = cwhich - 1;  cnt >= 0;  cnt--)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = number() - 1;  cnt > cwhich;  cnt--)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	AfxMessageBox(IDP_NOTFOUND, MB_OK|MB_ICONEXCLAMATION);
	return;
	
gotit:							
	POSITION	p = GetFirstViewPosition();
	CJobView	*aview = (CJobView *)GetNextView(p);
	if  (aview)  {
		aview->ChangeSelectionToRow(cnt);		
		aview->UpdateRow(cnt);
	    UpdateAllViews(NULL);
	}    
}	
	
void CJobdoc::OnSearchSearchbackwards()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, FALSE);
}

void CJobdoc::OnSearchSearchfor()
{
	CJsearch	sdlg;
	sdlg.m_searchstring = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch;
	sdlg.m_stitle = m_sjtitle;
	sdlg.m_suser = m_suser;
	sdlg.m_sforward = 0;
	sdlg.m_swraparound = m_swraparound;
	if  (sdlg.DoModal() != IDOK)
		return;
	((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch = sdlg.m_searchstring;
	m_sjtitle = sdlg.m_stitle;
	m_suser = sdlg.m_suser;
	m_swraparound = sdlg.m_swraparound;
	DoSearch(sdlg.m_searchstring, sdlg.m_sforward == 0);
}

void CJobdoc::OnSearchSearchforward()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_jlastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, TRUE);		
}

void CJobdoc::OnWindowWindowoptions()
{
	CWinopts	dlg;
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	dlg.m_confdel = m_wrestrict.confd == restrictdoc::ALWAYS ? 1: 0;
	dlg.m_jobqueue = m_wrestrict.queuename;
	dlg.m_incnull = m_wrestrict.incnull;
	dlg.m_username = m_wrestrict.user;
	dlg.m_groupname = m_wrestrict.group;
	dlg.m_localonly = m_wrestrict.onlyl == restrictdoc::YES? 0: 1; 
	dlg.m_isjobs = TRUE;
	if  (dlg.DoModal() == IDOK)  {
		if  (dlg.m_confdel > 0)
			m_wrestrict.confd = restrictdoc::ALWAYS;
		else
			m_wrestrict.confd = restrictdoc::NEVER;
		m_wrestrict.user = dlg.m_username;
		m_wrestrict.group = dlg.m_groupname;
		m_wrestrict.queuename = dlg.m_jobqueue;
		m_wrestrict.incnull = dlg.m_incnull;
		m_wrestrict.onlyl = dlg.m_localonly > 0? restrictdoc::NO: restrictdoc::YES;
		revisejobs(Jobs());
	}
}

void CJobdoc::OnJobsViewjob()
{
	Btjob	*cj = GetSelectedJob();
	if  (!cj)
		return;
	((CMainFrame *)(AfxGetApp())->m_pMainWnd)->OnNewJDWin(cj);
}

void CJobdoc::OnActionDeletejob()
{
	Btjob	*cj = GetSelectedJob(BTM_DELETE);
	if  (!cj)
		return;
	if  (m_wrestrict.confd == restrictdoc::NEVER  ||
		 AfxMessageBox(IDP_CONFDELJ, MB_OKCANCEL|MB_ICONQUESTION) == IDOK)
		Jobs().deletejob(*cj);
}

void CJobdoc::OnActionSetrunnable()
{
	Btjob	*cj = GetSelectedJob(BTM_WRITE);
	if  (!cj  ||  cj->bj_progress == BJP_NONE  ||  cj->bj_progress >= BJP_STARTUP1)
		return;
	Btjob	newj = *cj;
	newj.bj_progress = BJP_NONE;
	Jobs().changejob(newj);
}

void CJobdoc::OnActionSetcancelled()
{
	Btjob	*cj = GetSelectedJob(BTM_WRITE);
	if  (!cj  ||  cj->bj_progress == BJP_CANCELLED  ||  cj->bj_progress >= BJP_STARTUP1)
		return;
	Btjob	newj = *cj;
	newj.bj_progress = BJP_CANCELLED;
	Jobs().changejob(newj);
}

void CJobdoc::OnActionForcerun()
{
	Btjob	*cj = GetSelectedJob(BTM_WRITE|BTM_KILL);
	if  (!cj  ||  cj->bj_progress >= BJP_STARTUP1)
		return;
	Jobs().forcejob(*cj, 1);
}

void CJobdoc::OnActionKilljob()
{
	Btjob	*cj = GetSelectedJob(BTM_KILL);
	if  (!cj  ||  cj->bj_progress != BJP_RUNNING)
		return;
	Jobs().killjob(*cj, UNIXSIGINT);
}

void CJobdoc::OnActionQuitjob()
{
	Btjob	*cj = GetSelectedJob(BTM_KILL);
	if  (!cj  ||  cj->bj_progress >= BJP_RUNNING)
		return;
	Jobs().killjob(*cj, UNIXSIGQUIT);
}

void CJobdoc::OnActionOthersignal()
{
	Btjob	*cj = GetSelectedJob(BTM_KILL);
	if  (!cj  ||  cj->bj_progress != BJP_RUNNING)
		return;
	const  int  sigs[] = { UNIXSIGHUP, UNIXSIGINT, UNIXSIGQUIT,
						   UNIXSIGTERM, UNIXSIGKILL };
	COthersig  dlg;
	if  (dlg.DoModal() == IDOK  &&  dlg.m_sigtype >= 0  &&  dlg.m_sigtype < sizeof(sigs) / sizeof(int))
		Jobs().killjob(*cj, sigs[dlg.m_sigtype]);	
}

void CJobdoc::OnActionPermissions()
{
	Btjob	*cj = GetSelectedJob(BTM_RDMODE);
	if  (!cj)
		return;
	CJperm  dlg;
	dlg.m_user = cj->bj_mode.o_user;
	dlg.m_group = cj->bj_mode.o_group;
	dlg.m_umode = cj->bj_mode.u_flags;
	dlg.m_gmode = cj->bj_mode.g_flags;
	dlg.m_omode = cj->bj_mode.o_flags;
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRMODE);
	if  (dlg.DoModal() == IDOK)  {
		Btjob	newj = *cj;
		if  (dlg.m_umode != cj->bj_mode.u_flags  ||
			 dlg.m_gmode != cj->bj_mode.g_flags  ||
			 dlg.m_omode != cj->bj_mode.o_flags)  {
			newj.bj_mode.u_flags = dlg.m_umode;
			newj.bj_mode.g_flags = dlg.m_gmode;
			newj.bj_mode.o_flags = dlg.m_omode;
			Jobs().chmodjob(newj);
		}
		if  (!dlg.m_user.IsEmpty() && dlg.m_user != cj->bj_mode.o_user)
			Jobs().chogjob(newj, J_CHOWN, dlg.m_user);
		if  (!dlg.m_group.IsEmpty() && dlg.m_group != cj->bj_mode.o_group)
			Jobs().chogjob(newj, J_CHGRP, dlg.m_group);
	}		
}

void CJobdoc::OnActionTitlepriloadlev()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CTitledlg  dlg;
	dlg.m_title = cj->bj_title;
	dlg.m_cmdinterp = cj->bj_cmdinterp;
	dlg.m_priority = cj->bj_pri;
	dlg.m_loadlev = cj->bj_ll;           
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	dlg.m_hostid = cj->hostid;
	if  (dlg.DoModal() == IDOK  &&  dlg.m_writeable)  {
		Btjob	newj = *cj;
		newj.bj_title = dlg.m_title;
		newj.bj_cmdinterp = dlg.m_cmdinterp;
		newj.bj_pri = dlg.m_priority;
		newj.bj_ll = dlg.m_loadlev;
		if  (newj.bj_title != cj->bj_title  ||
			 newj.bj_cmdinterp != cj->bj_cmdinterp  ||
			 newj.bj_pri != cj->bj_pri ||
			 newj.bj_ll != cj->bj_ll)
			Jobs().changejob(newj);    
	}	
}

void CJobdoc::OnJobsArguments()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CArgdlg  dlg;
	dlg.m_nargs = cj->bj_nargs;
	if  (dlg.m_nargs != 0)
		dlg.m_args = new CString [dlg.m_nargs];
	for  (unsigned  cnt = 0;  cnt < cj->bj_nargs;  cnt++)
		dlg.m_args[cnt] = cj->bj_arg[cnt];
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		Btjob	newj = *cj;
		newj.bj_nargs = dlg.m_nargs;
		if  (newj.bj_nargs != 0)  {
			newj.bj_arg = new CString [newj.bj_nargs];
			for  (cnt = 0;  cnt < newj.bj_nargs;  cnt++)
				newj.bj_arg[cnt] = dlg.m_args[cnt];
		}
		else
			newj.bj_arg = NULL;
		Jobs().changejob(newj);
	}
}		

void CJobdoc::OnJobsEnvironment()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CEnvlist  dlg;
	dlg.m_nenvs = cj->bj_nenv;
	if  (dlg.m_nenvs != 0)
		dlg.m_envs = new Menvir [dlg.m_nenvs];
	for  (unsigned  cnt = 0;  cnt < cj->bj_nenv;  cnt++)
		dlg.m_envs[cnt] = cj->bj_env[cnt];
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		Btjob	newj = *cj;
		newj.bj_nenv = dlg.m_nenvs;
		if  (newj.bj_nenv != 0)  {
			newj.bj_env = new Menvir [newj.bj_nenv];
			for  (cnt = 0;  cnt < newj.bj_nenv;  cnt++)
				newj.bj_env[cnt] = dlg.m_envs[cnt];
		}
		else
			newj.bj_env = NULL;
		Jobs().changejob(newj);
	}
}

void CJobdoc::OnJobsMail()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CMaildlg	dlg;
	dlg.m_mail = (cj->bj_jflags & BJ_MAIL) != 0;
	dlg.m_write = (cj->bj_jflags & BJ_WRT) != 0;
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK  &&  dlg.m_writeable)  {
		Btjob	newj = *cj;
		if  (dlg.m_mail)
			newj.bj_jflags |= BJ_MAIL;
		else
			newj.bj_jflags &= ~BJ_MAIL;
		if  (dlg.m_write)
			newj.bj_jflags |= BJ_WRT;
		else
			newj.bj_jflags &= ~BJ_WRT;
		if  (newj.bj_jflags != cj->bj_jflags)
			Jobs().changejob(newj);    
	}		
}

void CJobdoc::OnJobsProcparam()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CProcpar  dlg;
	dlg.m_umask = cj->bj_umask;
	dlg.m_ulimit = cj->bj_ulimit;
	dlg.m_directory = cj->bj_direct;
	dlg.m_exits = cj->bj_exits;
	dlg.m_remrunnable = (cj->bj_jflags & BJ_REMRUNNABLE) != 0;
	dlg.m_advterr = cj->bj_jflags & BJ_NOADVIFERR? 1: 0;
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK  &&  dlg.m_writeable)  {
		Btjob	newj = *cj;
		newj.bj_umask = dlg.m_umask;
		newj.bj_ulimit = dlg.m_ulimit;     
		if  (!dlg.m_directory.IsEmpty())
			newj.bj_direct = dlg.m_directory;
		newj.bj_exits = dlg.m_exits;
		if  (dlg.m_remrunnable)
			newj.bj_jflags |= BJ_REMRUNNABLE;
		else
			newj.bj_jflags &= ~BJ_REMRUNNABLE;
		if  (dlg.m_advterr > 0)
			newj.bj_jflags |= BJ_NOADVIFERR;
		else
			newj.bj_jflags &= ~BJ_NOADVIFERR;
		if  (newj.bj_umask != cj->bj_umask  ||
			 newj.bj_ulimit != cj->bj_ulimit ||
			 newj.bj_direct != cj->bj_direct ||
			 newj.bj_jflags != cj->bj_jflags ||
		     newj.bj_exits.nlower != cj->bj_exits.nlower ||
		     newj.bj_exits.nupper != cj->bj_exits.nupper ||
		     newj.bj_exits.elower != cj->bj_exits.elower ||
		     newj.bj_exits.eupper != cj->bj_exits.eupper)
			Jobs().changejob(newj);    
	}		
}

void CJobdoc::OnJobsRedirections()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CRedirlist  dlg;
	dlg.m_nredirs = cj->bj_nredirs;
	if  (dlg.m_nredirs != 0)
		dlg.m_redirs = new Mredir [dlg.m_nredirs];
	for  (unsigned  cnt = 0;  cnt < cj->bj_nredirs;  cnt++)
		dlg.m_redirs[cnt] = cj->bj_redirs[cnt];
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		Btjob	newj = *cj;
		newj.bj_nredirs = dlg.m_nredirs;
		if  (newj.bj_nredirs != 0)  {
			newj.bj_redirs = new Mredir [newj.bj_nredirs];
			for  (cnt = 0;  cnt < newj.bj_nredirs;  cnt++)
				newj.bj_redirs[cnt] = dlg.m_redirs[cnt];
		}
		else
			newj.bj_redirs = NULL;
		Jobs().changejob(newj);
	}
}

void CJobdoc::OnJobsTime()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	Ctimedlg  dlg;
	dlg.m_tc = cj->bj_times;
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK  &&  dlg.m_writeable)  {
		Btjob	newj = *cj;
		newj.bj_times = dlg.m_tc;
		Jobs().changejob(newj);
	}	
}

void CJobdoc::OnConditionsJobassignments()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CJasslist  dlg;
	dlg.m_nasses = 0;
	for  (unsigned  cnt = 0; cnt < MAXSEVARS; cnt++)
		if  (cj->bj_asses[cnt].bja_op != BJA_NONE)
			dlg.m_jasses[dlg.m_nasses++] = cj->bj_asses[cnt];
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		Btjob	newj = *cj;
		for  (cnt = 0;  cnt < dlg.m_nasses;  cnt++)
			newj.bj_asses[cnt] = dlg.m_jasses[cnt];
		for  (;  cnt < MAXSEVARS;  cnt++)
			newj.bj_asses[cnt].bja_op = BJA_NONE;
		Jobs().changejob(newj);
	}
}

void CJobdoc::OnConditionsJobconditions()
{
	Btjob	*cj = GetSelectedJob(BTM_READ, TRUE);
	if  (!cj)
		return;
	CJcondlist  dlg;
	dlg.m_nconds = 0;
	for  (unsigned  cnt = 0; cnt < MAXCVARS; cnt++)
		if  (cj->bj_conds[cnt].bjc_compar != C_UNUSED)
			dlg.m_jconds[dlg.m_nconds++] = cj->bj_conds[cnt];
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		Btjob	newj = *cj;
		for  (cnt = 0;  cnt < dlg.m_nconds;  cnt++)
			newj.bj_conds[cnt] = dlg.m_jconds[cnt];
		for  (;  cnt < MAXCVARS;  cnt++)
			newj.bj_conds[cnt].bjc_compar = C_UNUSED;
		Jobs().changejob(newj);
	}
}

void CJobdoc::OnActionGojob()
{
	Btjob	*cj = GetSelectedJob(BTM_WRITE|BTM_KILL, TRUE);
	if  (!cj)
		return;
	Jobs().forcejob(*cj, 0);
}

void CJobdoc::OnJobsAdvancetime()
{
	Btjob	*cj = GetSelectedJob(BTM_WRITE);
	if  (!cj)
		return;
	if  (!cj->bj_times.tc_istime)  {
		AfxMessageBox(IDP_NOTIMETOADV, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	Btjob	newj = *cj;
	newj.bj_times.tc_nexttime = newj.bj_times.advtime();
	Jobs().changejob(newj);
}

void CJobdoc::OnJobsUnqueuejob()
{
	Btjob	*cj = GetSelectedJob(BTM_READ|BTM_RDMODE|BTM_DELETE, TRUE);
	if  (!cj)
		return;
	Btjob	jcopy = *cj;
	jcopy.unqueue();
}

void CJobdoc::OnJobsCopyjob()
{
	Btjob	*cj = GetSelectedJob(BTM_READ|BTM_RDMODE);
	if  (!cj)
		return;   
	Btjob	jcopy = *cj;
	jcopy.unqueue(FALSE);
}

void CJobdoc::OnJobsCopyoptionsinjob()
{
	Btjob	*cj = GetSelectedJob(BTM_READ|BTM_RDMODE);
	if  (!cj)
		return;        
	CCpyOptdlg	dlg;
	dlg.m_ascanc = cj->bj_progress == BJP_NONE? 1: 0;
	if  (dlg.DoModal() == IDOK)  {
		Btjob	jcopy = *cj;
		jcopy.bj_progress = dlg.m_ascanc == 1? BJP_NONE: BJP_CANCELLED;
		jcopy.optcopy();
	}
}

void CJobdoc::OnJobsInvokebtrw()
{
	WinExec("BTRW", SW_SHOW);
}

void CJobdoc::OnJobsTimelimits()
{
	Btjob	*cj = GetSelectedJob(BTM_READ);
	if  (!cj)
		return;
	CTimelim  dlg;
	dlg.m_deltime = cj->bj_deltime;
	dlg.m_runtime = cj->bj_runtime;
	dlg.m_runon = cj->bj_runon;
	static  unsigned  char	siglist[8] = { UNIXSIGTERM, UNIXSIGKILL, UNIXSIGHUP, UNIXSIGINT,
										   UNIXSIGQUIT, UNIXSIGALRM, UNIXSIGBUS, UNIXSIGSEGV };
	for  (int signum = 0;  signum < 8;  signum++)
		if  (cj->bj_autoksig == siglist[signum])  {
			dlg.m_autoksig = signum;
			goto  dun1;
		}
	dlg.m_autoksig = 1;		//  SIGKILL
dun1:
	dlg.m_writeable = cj->bj_mode.mpermitted(BTM_WRITE);
	if  (dlg.DoModal() == IDOK && dlg.m_writeable)  {
		Btjob	newj = *cj;
		newj.bj_deltime = dlg.m_deltime;
		newj.bj_runtime = dlg.m_runtime;
		newj.bj_runon = dlg.m_runon;
		newj.bj_autoksig = siglist[dlg.m_autoksig];
		Jobs().changejob(newj);
	}
}

void CJobdoc::OnWjcolourAbort() 
{
	RunColourDlg(BJP_ABORTED);
}

void CJobdoc::OnWjcolourCancelled() 
{
	RunColourDlg(BJP_CANCELLED);
}

void CJobdoc::OnWjcolourError() 
{
	RunColourDlg(BJP_ERROR);
}

void CJobdoc::OnWjcolourFinished() 
{
	RunColourDlg(BJP_DONE);			
}

void CJobdoc::OnWjcolourReady() 
{
	RunColourDlg(BJP_NONE);
}

void CJobdoc::OnWjcolourRunning() 
{
	RunColourDlg(BJP_RUNNING);
}

void CJobdoc::OnWjcolourStartup() 
{
	RunColourDlg(BJP_STARTUP1);		
}

void CJobdoc::RunColourDlg(const unsigned int n)
{
	COLORREF	&mc = m_jobcolours[n];
	CColorDialog	cdlg;
	cdlg.m_cc.Flags |= CC_RGBINIT|CC_SOLIDCOLOR;
	cdlg.m_cc.rgbResult = mc;
	if  (cdlg.DoModal() == IDOK)  {
		mc = cdlg.m_cc.rgbResult;
		UpdateAllViews(NULL);
	}
}

void CJobdoc::OnFileJcolourCopytoprog() 
{
	CBtqwApp	*mw = (CBtqwApp *)AfxGetApp();
	mw->m_appcolours = m_jobcolours;
	mw->dirty();
}

void CJobdoc::OnFileJcolourCopytowin() 
{
	m_jobcolours = ((CBtqwApp *)AfxGetApp())->m_appcolours;
	UpdateAllViews(NULL);
}
