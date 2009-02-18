// btrwdoc.cpp : implementation of the CBtrwDoc class
//

#include "stdafx.h"
#include <ctype.h>
#include "xbwnetwk.h"
#include "btrw.h"
#include "btrwdoc.h"
#include "timedlg.h"
#include "titledlg.h"
#include "jperm.h"
#include "argdlg.h"
#include "envlist.h"
#include "maildlg.h"
#include "procpar.h"
#include "redirlis.h"
#include "jasslist.h"
#include "jcondlis.h"
#include "timelim.h"
#include "uperms.h"
#include "defcond.h"
#include "defass.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBtrwDoc

IMPLEMENT_DYNCREATE(CBtrwDoc, CDocument)

BEGIN_MESSAGE_MAP(CBtrwDoc, CDocument)
	//{{AFX_MSG_MAP(CBtrwDoc)
	ON_COMMAND(ID_ACTION_DELETEJOB, OnActionDeletejob)
	ON_COMMAND(ID_ACTION_SUBMITJOB, OnActionSubmitjob)
	ON_COMMAND(ID_JOBS_EDITJOB, OnJobsEditjob)
	ON_COMMAND(ID_ACTION_SETRUNNABLE, OnActionSetrunnable)
	ON_COMMAND(ID_ACTION_SETCANCELLED, OnActionSetcancelled)
	ON_COMMAND(ID_JOBS_TIME, OnJobsTime)
	ON_COMMAND(ID_ACTION_TITLEPRILOADLEV, OnActionTitlepriloadlev)
	ON_COMMAND(ID_JOBS_PROCPARAM, OnJobsProcparam)
	ON_COMMAND(ID_ACTION_PERMISSIONS, OnActionPermissions)
	ON_COMMAND(ID_JOBS_MAIL, OnJobsMail)
	ON_COMMAND(ID_JOBS_ARGUMENTS, OnJobsArguments)
	ON_COMMAND(ID_JOBS_ENVIRONMENT, OnJobsEnvironment)
	ON_COMMAND(ID_JOBS_REDIRECTIONS, OnJobsRedirections)
	ON_COMMAND(ID_CONDITIONS_JOBCONDITIONS, OnConditionsJobconditions)
	ON_COMMAND(ID_CONDITIONS_JOBASSIGNMENTS, OnConditionsJobassignments)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_ACTION_COPYJOBFILE, OnActionCopyjobfile)
	ON_COMMAND(ID_ENVIRONMENT_CLEARENVIRONMENT, OnEnvironmentClearenvironment)
	ON_COMMAND(ID_ENVIRONMENT_ENVIRONMENTDEFAULT, OnEnvironmentEnvironmentdefault)
	ON_COMMAND(ID_ENVIRONMENT_SQUEEZEENVIRONMENT, OnEnvironmentSqueezeenvironment)
	ON_COMMAND(ID_JOBS_TIMELIMITS, OnJobsTimelimits)
	ON_COMMAND(ID_FILE_USERPERMISSIONS, OnFileUserpermissions)
	ON_COMMAND(ID_ACTION_SETDONE, OnActionSetdone)
	ON_COMMAND(ID_DEFAULTS_TIME, OnDefaultsTime)
	ON_COMMAND(ID_DEFAULTS_TITLEPRILOADLEV, OnDefaultsTitlepriloadlev)
	ON_COMMAND(ID_DEFAULTS_MAIL, OnDefaultsMail)
	ON_COMMAND(ID_DEFAULTS_PROCPARAM, OnDefaultsProcparam)
	ON_COMMAND(ID_DEFAULTS_TIMELIMITS, OnDefaultsTimelimits)
	ON_COMMAND(ID_DEFAULTS_ARGUMENTS, OnDefaultsArguments)
	ON_COMMAND(ID_DEFAULTS_REDIRECTIONS, OnDefaultsRedirections)
	ON_COMMAND(ID_DEFAULTS_CONDITIONS, OnDefaultsConditions)
	ON_COMMAND(ID_DEFAULTS_ASSIGNMENTS, OnDefaultsAssignments)
	ON_COMMAND(ID_DEFAULTS_NEWCONDITIONS, OnDefaultsNewconditions)
	ON_COMMAND(ID_DEFAULTS_NEWASSIGNMENTS, OnDefaultsNewassignments)
	ON_COMMAND(ID_DEFAULTS_PERMISSIONS, OnDefaultsPermissions)
	ON_COMMAND(ID_DEFAULTS_SETRUNNABLE, OnDefaultsSetrunnable)
	ON_COMMAND(ID_DEFAULTS_SETCANCELLED, OnDefaultsSetcancelled)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtrwDoc construction/destruction

CBtrwDoc::CBtrwDoc()
{
	// TODO: add one-time construction code here
}

CBtrwDoc::~CBtrwDoc()
{
}

BOOL CBtrwDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	CheckChanges();
	m_currjob.init_job();
	((CBtrwApp *)AfxGetApp())->clean();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CBtrwDoc serialization

void CBtrwDoc::Serialize(CArchive& ar)
{
	if  (ar.IsLoading())  {
		CheckChanges();
		if  (!m_currjob.loadoptions(ar.GetFile(), m_jobfile))  {
			AfxMessageBox(IDP_NOTJOBFORMAT);
			m_currjob.init_job();
		}
	}
	else
		m_currjob.saveoptions(ar.GetFile(), m_jobfile);
	((CBtrwApp *)AfxGetApp())->clean();
}

/////////////////////////////////////////////////////////////////////////////
// CBtrwDoc diagnostics

#ifdef _DEBUG
void CBtrwDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBtrwDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBtrwDoc commands

void CBtrwDoc::OnActionDeletejob()
{
	if  (m_jobfile.IsEmpty() && GetPathName().IsEmpty())
		return;
	if  (AfxMessageBox(IDP_DELAREYOUSURE, MB_YESNO|MB_ICONQUESTION) == IDYES)  {
		TRY {
			if  (!m_jobfile.IsEmpty())
				CFile::Remove((const char *) m_jobfile);
			if  (!GetPathName().IsEmpty())
				CFile::Remove((const char *) GetPathName());
		}
		CATCH(CException, e)
		{
		}
		END_CATCH
		((CBtrwApp *)AfxGetApp())->clean();
		OnNewDocument();
	}			
}

void CBtrwDoc::OnActionSubmitjob()
{
	if  (GetPathName().IsEmpty())  {
		AfxMessageBox(IDP_NOBATFILE, MB_OK|MB_ICONSTOP);
		return;
	}
	if  (m_jobfile.IsEmpty())  {
		AfxMessageBox(IDP_NOJOBFILE, MB_OK|MB_ICONSTOP);
		return;
	}
	jobno_t	jobres;
	if  (m_currjob.submitjob(jobres, m_jobfile)  &&  ((CBtrwApp *)AfxGetApp())->m_verbose)  {
		CString	msg;
		msg.LoadString(IDP_SUBMITTEDOK);
		char  nbuf[20];
		wsprintf(nbuf, "%ld", jobres);
		msg += nbuf;
		AfxMessageBox(msg, MB_OK|MB_ICONINFORMATION, IDP_SUBMITTEDOK);
	}
}

void CBtrwDoc::OnJobsEditjob()
{
	const  CString	 &which = m_jobfile.IsEmpty()? GetPathName(): m_jobfile; 
	CString  cmd;
	cmd = ((CBtrwApp *)AfxGetApp())->m_jeditor + ' ' + which;
	WinExec((const char *)cmd, SW_SHOW);
}

void CBtrwDoc::OnDefaultsSetrunnable()
{
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	ma.m_defaultjob.bj_progress = BJP_NONE;
	ma.defdirty();
}

void CBtrwDoc::OnDefaultsSetcancelled()
{
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	ma.m_defaultjob.bj_progress = BJP_CANCELLED;
	ma.defdirty();
}

void CBtrwDoc::OnActionSetrunnable()
{
	m_currjob.bj_progress = BJP_NONE;
	UpdateAllViews(NULL);
	((CBtrwApp *)AfxGetApp())->dirty();
}

void CBtrwDoc::OnActionSetcancelled()
{
	m_currjob.bj_progress = BJP_CANCELLED;
	UpdateAllViews(NULL);
	((CBtrwApp *)AfxGetApp())->dirty();
}

void CBtrwDoc::OnActionSetdone()
{
	m_currjob.bj_progress = BJP_DONE;
	UpdateAllViews(NULL);
	((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoTime(Btjob &job)
{
	Ctimedlg  dlg;
	dlg.m_tc = job.bj_times;
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK)  {
		job.bj_times = dlg.m_tc;
		return  TRUE;
	}
	return  FALSE;
}

void CBtrwDoc::OnDefaultsTime()
{
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	if  (DoTime(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnJobsTime()
{
	if  (DoTime(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoTitlepriloadlev(Btjob &job)
{
	CTitledlg  dlg;
	CBtrwApp &ma = *((CBtrwApp *)AfxGetApp());
	int	ep;
	if  ((ep = job.bj_title.Find(':')) > 0  &&  job.bj_title.Left(ep) == ma.m_jobqueue)
		dlg.m_title = job.bj_title.Right(job.bj_title.GetLength() - ep - 1);
	else
		dlg.m_title = job.bj_title;
	dlg.m_cmdinterp = job.bj_cmdinterp;
	dlg.m_priority = job.bj_pri;
	dlg.m_loadlev = job.bj_ll;           
	dlg.m_writeable = TRUE;
	dlg.m_hostid = Locparams.servid;
	if  (dlg.DoModal() == IDOK)  {
		if  (!ma.m_jobqueue.IsEmpty() && dlg.m_title.Find(':') < 0)
			job.bj_title = ma.m_jobqueue + ':' + dlg.m_title;
		else
			job.bj_title = dlg.m_title;
		job.bj_cmdinterp = dlg.m_cmdinterp;
		job.bj_pri = dlg.m_priority;
		job.bj_ll = dlg.m_loadlev;
		return  TRUE;
	}	
	return  FALSE;
}

void CBtrwDoc::OnDefaultsTitlepriloadlev()
{
	CBtrwApp &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoTitlepriloadlev(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnActionTitlepriloadlev()
{
	if  (DoTitlepriloadlev(m_currjob))  {
		UpdateAllViews(NULL);
		((CBtrwApp *)AfxGetApp())->dirty();
	}	
}

static	BOOL	DoProcParam(Btjob &job)
{
	CProcpar  dlg;
	dlg.m_umask = job.bj_umask;
	dlg.m_ulimit = job.bj_ulimit;
	dlg.m_directory = job.bj_direct;
	dlg.m_exits = job.bj_exits;
	dlg.m_remrunnable = (job.bj_jflags & BJ_REMRUNNABLE) != 0;
	dlg.m_advterr = job.bj_jflags & BJ_NOADVIFERR? 1: 0;
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK)  {
		job.bj_umask = dlg.m_umask;
		job.bj_ulimit = dlg.m_ulimit;     
		if  (!dlg.m_directory.IsEmpty())
			job.bj_direct = dlg.m_directory;
		job.bj_exits = dlg.m_exits;
		if  (dlg.m_remrunnable)
			job.bj_jflags |= BJ_EXPORT|BJ_REMRUNNABLE;
		else  {
			job.bj_jflags &= ~BJ_REMRUNNABLE;
			if  (((CBtrwApp *)AfxGetApp())->m_nonexport)
				job.bj_jflags &= ~BJ_EXPORT;	
		}
		if  (dlg.m_advterr > 0)
			job.bj_jflags |= BJ_NOADVIFERR;
		else
			job.bj_jflags &= ~BJ_NOADVIFERR;
		return  TRUE;
	}		         
	return  FALSE;
}

void CBtrwDoc::OnDefaultsProcparam()
{
	CBtrwApp &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoProcParam(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnJobsProcparam()
{
	if  (DoProcParam(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoPermissions(Btjob &job)
{
	CJperm  dlg;
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	dlg.m_user = ma.m_sendowner;
	dlg.m_group = ma.m_sendgroup;
	dlg.m_umode = job.bj_mode.u_flags;
	dlg.m_gmode = job.bj_mode.g_flags;
	dlg.m_omode = job.bj_mode.o_flags;
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK)  {
		ma.m_sendowner = dlg.m_user;
		ma.m_sendgroup = dlg.m_group;
		job.bj_mode.u_flags = dlg.m_umode;
	 	job.bj_mode.g_flags = dlg.m_gmode;
		job.bj_mode.o_flags = dlg.m_omode;
		return  TRUE;
	}		         
	return  FALSE;
}

void CBtrwDoc::OnDefaultsPermissions()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoPermissions(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnActionPermissions()
{
	if  (DoPermissions(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoMail(Btjob &job)
{
	CMaildlg	dlg;
	dlg.m_mail = (job.bj_jflags & BJ_MAIL) != 0;
	dlg.m_write = (job.bj_jflags & BJ_WRT) != 0;
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK)  {
		if  (dlg.m_mail)
			job.bj_jflags |= BJ_MAIL;
		else
			job.bj_jflags &= ~BJ_MAIL;
		if  (dlg.m_write)
			job.bj_jflags |= BJ_WRT;
		else
			job.bj_jflags &= ~BJ_WRT;
		return  TRUE;
	}		
	return  FALSE;
}

void CBtrwDoc::OnDefaultsMail()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	if	(DoMail(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnJobsMail()
{
	if  (DoMail(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoArguments(Btjob &job)
{
	CArgdlg  dlg;
	dlg.m_nargs = job.bj_nargs;
	if  (dlg.m_nargs != 0)
		dlg.m_args = new CString [dlg.m_nargs];
	for  (unsigned  cnt = 0;  cnt < job.bj_nargs;  cnt++)
		dlg.m_args[cnt] = job.bj_arg[cnt];
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		job.bj_nargs = dlg.m_nargs;
		if  (job.bj_nargs != 0)  {
			job.bj_arg = new CString [job.bj_nargs];
			for  (cnt = 0;  cnt < job.bj_nargs;  cnt++)
				job.bj_arg[cnt] = dlg.m_args[cnt];
		}
		else
			job.bj_arg = NULL;
		return  TRUE;
	}
	return  FALSE;
}

void CBtrwDoc::OnDefaultsArguments()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoArguments(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnJobsArguments()
{
	if  (DoArguments(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

void CBtrwDoc::OnJobsEnvironment()
{
	CEnvlist  dlg;
	dlg.m_nenvs = m_currjob.bj_nenv;
	if  (dlg.m_nenvs != 0)
		dlg.m_envs = new Menvir [dlg.m_nenvs];
	for  (unsigned  cnt = 0;  cnt < m_currjob.bj_nenv;  cnt++)
		dlg.m_envs[cnt] = m_currjob.bj_env[cnt];
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		m_currjob.bj_nenv = dlg.m_nenvs;
		if  (m_currjob.bj_nenv != 0)  {
			m_currjob.bj_env = new Menvir [m_currjob.bj_nenv];
			for  (cnt = 0;  cnt < m_currjob.bj_nenv;  cnt++)
				m_currjob.bj_env[cnt] = dlg.m_envs[cnt];
		}
		else
			m_currjob.bj_env = NULL;
		((CBtrwApp *)AfxGetApp())->dirty();
	}
}

static	BOOL	DoRedirections(Btjob &job)
{
	CRedirlist  dlg;
	dlg.m_nredirs = job.bj_nredirs;
	if  (dlg.m_nredirs != 0)
		dlg.m_redirs = new Mredir [dlg.m_nredirs];
	for  (unsigned  cnt = 0;  cnt < job.bj_nredirs;  cnt++)
		dlg.m_redirs[cnt] = job.bj_redirs[cnt];
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		job.bj_nredirs = dlg.m_nredirs;
		if  (job.bj_nredirs != 0)  {
			job.bj_redirs = new Mredir [job.bj_nredirs];
			for  (cnt = 0;  cnt < job.bj_nredirs;  cnt++)
				job.bj_redirs[cnt] = dlg.m_redirs[cnt];
		}
		else
			job.bj_redirs = NULL;
		return  TRUE;
	}
	return  FALSE;
}

void CBtrwDoc::OnDefaultsRedirections()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoRedirections(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnJobsRedirections()
{
	if  (DoRedirections(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoConditions(Btjob &job)
{
	CJcondlist  dlg;
	dlg.m_nconds = 0;
	for  (unsigned  cnt = 0; cnt < MAXCVARS; cnt++)
		if  (job.bj_conds[cnt].bjc_compar != C_UNUSED)
			dlg.m_jconds[dlg.m_nconds++] = job.bj_conds[cnt];
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		for  (cnt = 0;  cnt < dlg.m_nconds;  cnt++)
			job.bj_conds[cnt] = dlg.m_jconds[cnt];
		for  (;  cnt < MAXCVARS;  cnt++)
			job.bj_conds[cnt].bjc_compar = C_UNUSED;
		return  TRUE;
	}
	return  FALSE;
}

void CBtrwDoc::OnDefaultsConditions()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoConditions(ma.m_defaultjob))
		ma.defdirty();	
}

void CBtrwDoc::OnConditionsJobconditions()
{
	if  (DoConditions(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

static	BOOL	DoAssignments(Btjob &job)
{
	CJasslist  dlg;
	dlg.m_nasses = 0;
	for  (unsigned  cnt = 0; cnt < MAXSEVARS; cnt++)
		if  (job.bj_asses[cnt].bja_op != BJA_NONE)
			dlg.m_jasses[dlg.m_nasses++] = job.bj_asses[cnt];
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK && dlg.m_changes)  {
		for  (cnt = 0;  cnt < dlg.m_nasses;  cnt++)
			job.bj_asses[cnt] = dlg.m_jasses[cnt];
		for  (;  cnt < MAXSEVARS;  cnt++)
			job.bj_asses[cnt].bja_op = BJA_NONE;
		return  TRUE;
	}
	return  FALSE;
}

void CBtrwDoc::OnDefaultsAssignments()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	if  (DoAssignments(ma.m_defaultjob))
		ma.defdirty();	
}

void CBtrwDoc::OnConditionsJobassignments()
{
	if  (DoAssignments(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}

void CBtrwDoc::CheckChanges()
{
	if  (((CBtrwApp *)AfxGetApp())->isdirty()  &&  !m_jobfile.IsEmpty()  &&
		AfxMessageBox(IDP_SAVEFIRST, MB_ICONQUESTION|MB_YESNO) == IDYES)  {
		CFile	ff;
		if  (ff.Open(GetPathName(), CFile::modeWrite))
			m_currjob.saveoptions(&ff, m_jobfile);
		((CBtrwApp *)AfxGetApp())->clean();
	}
}
		
void CBtrwDoc::OnFileOpen()
{
	CheckChanges();
	((CBtrwApp *)AfxGetApp())->FileOpen();
}

void CBtrwDoc::OnActionCopyjobfile()
{
	CFileDialog	jfn(FALSE,
					"xbj",
					NULL,
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,
					"Xibatch job files (*.xbj) | *.xbj | All Files (*.*) | *.* ||");
					
	if  (m_jobfile.IsEmpty())  {
		jfn.SetHelpID(IDD_NEWJOBFILE);
	    if  (jfn.DoModal() != IDOK)
			return;
		m_jobfile = jfn.GetPathName();
		CFile	ff;
		if  (ff.Open(m_jobfile, CFile::modeCreate | CFile::modeWrite))
			ff.Close();
	}
	else  {
		jfn.SetHelpID(IDD_COPYJOBFILE);
		if  (jfn.DoModal() != IDOK)
			return;
		CFile	fi, fo;
		if  (fi.Open(m_jobfile, CFile::modeRead) && fo.Open(jfn.GetPathName(), CFile::modeCreate|CFile::modeWrite))  {
			UINT	nbytes;
			char	buff[512];
			TRY {
				while  ((nbytes = fi.Read(buff, sizeof(buff))) > 0)
					fo.Write(buff, nbytes);
			}
			CATCH(CException, e)
			{
				return;
			}
			END_CATCH
		}
		m_jobfile = jfn.GetPathName();
	}
	((CBtrwApp *)AfxGetApp())->dirty();
	UpdateAllViews(NULL);
}

void CBtrwDoc::OnEnvironmentClearenvironment()
{
	Btjob	&jj = m_currjob;
	if  (jj.bj_env)  {
		delete [] jj.bj_env;
		jj.bj_env = NULL;
	}
	jj.bj_nenv = 0;
	((CBtrwApp *)AfxGetApp())->dirty();
}

void CBtrwDoc::OnEnvironmentEnvironmentdefault()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	Btjob	&jj = m_currjob;
	if  (jj.bj_nenv != 0)  {
		delete [] jj.bj_env;
		jj.bj_env = NULL;   
	}
	jj.bj_nenv = ma.m_envtable.number();
	if  (jj.bj_nenv != 0)  {
		jj.bj_env = new Menvir [jj.bj_nenv];
		ma.m_envtable.init_envlist();
		for  (unsigned cnt = 0;  cnt < jj.bj_nenv;  cnt++)  {
			const char *n, *v;
			ma.m_envtable.next(n, v);
			jj.bj_env[cnt].e_name = n;
			jj.bj_env[cnt].e_value = v;
		}
	} 
	ma.dirty();
}

void CBtrwDoc::OnEnvironmentSqueezeenvironment()
{
	Btjob	&jj = m_currjob;

	if  (jj.bj_nenv == 0)	//  Nothing to do
		return;
	
	Menvir	*nenv = new Menvir [jj.bj_nenv];
	Menvir  *np = nenv, *ep = &jj.bj_env[jj.bj_nenv];
	unsigned  ncount = 0;
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	
	for  (Menvir  *rp = jj.bj_env;  rp < ep;  rp++)  {
		ma.m_envtable.init_envlist();
		const char *n, *v;
        while  (ma.m_envtable.next(n, v))
			if  (rp->e_name == n  &&  rp->e_value == v)
				goto  gotit;
		*np++ = *rp;
		ncount++;		 
	gotit:
		;
	}
	if  (ncount == jj.bj_nenv)  {	//  No changes
		delete [] nenv;
		return;
	}
	delete [] jj.bj_env;
	jj.bj_nenv = ncount;
	if  (ncount == 0)  {
		delete [] nenv;
		jj.bj_env = NULL;
	}
	else
		jj.bj_env = nenv;
	ma.dirty();
}

static	BOOL	DoTimelimits(Btjob &job)
{
	CTimelim  dlg;

	dlg.m_deltime = job.bj_deltime;
	dlg.m_runtime = job.bj_runtime;
	dlg.m_runon = job.bj_runon;
	static  unsigned  char	siglist[8] = { UNIXSIGTERM, UNIXSIGKILL, UNIXSIGHUP, UNIXSIGINT,
										   UNIXSIGQUIT, UNIXSIGALRM, UNIXSIGBUS, UNIXSIGSEGV };
	for  (int signum = 0;  signum < 8;  signum++)
		if  (job.bj_autoksig == siglist[signum])  {
			dlg.m_autoksig = signum;
			goto  dun1;
		}
	dlg.m_autoksig = 1;		//  SIGKILL
dun1:
	dlg.m_writeable = TRUE;
	if  (dlg.DoModal() == IDOK && dlg.m_writeable)  {
		job.bj_deltime = dlg.m_deltime;
		job.bj_runtime = dlg.m_runtime;
		job.bj_runon = dlg.m_runon;
		job.bj_autoksig = siglist[dlg.m_autoksig];
		return  TRUE;
	}               
	return  FALSE;
}

void CBtrwDoc::OnDefaultsTimelimits()
{
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	if  (DoTimelimits(ma.m_defaultjob))
		ma.defdirty();
}

void CBtrwDoc::OnJobsTimelimits()
{
	if  (DoTimelimits(m_currjob))
		((CBtrwApp *)AfxGetApp())->dirty();
}
		
void CBtrwDoc::OnFileUserpermissions()
{
	CUperms		dlg;
	CBtrwApp	&ma = *((CBtrwApp *)AfxGetApp());
	dlg.m_priv = &ma.m_mypriv;
	dlg.m_user = ma.m_username;
	dlg.m_group = ma.m_groupname;
	dlg.DoModal();	
}

static	void	extractcval(Btcon &con, CString &str)
{
	int	lng = str.GetLength();
	if  (lng > 0  &&  (isdigit(str[0]) || str[0] == '-'))  {
			con.const_type = CON_LONG;
			con.con_long = atol(str);
	}
	else  {
		if  (lng > 1  &&  str[0] == '"')  {
			str = str.Right(--lng);
			if  (lng > 1  &&  str[lng-1] == '"')
				str = str.Left(--lng);
		}
		con.const_type = CON_STRING;
		strncpy(con.con_string, str, lng > BTC_VALUE? BTC_VALUE: lng);
		con.con_string[BTC_VALUE] = '\0';
	}
}

void CBtrwDoc::OnDefaultsNewconditions()
{
	Cdefcond	dlg;
	CBtrwApp	&ma = *((CBtrwApp *) AfxGetApp());
	Jcond	&cd = ma.m_defcond;
	dlg.m_condop = cd.bjc_compar - C_EQ;
	dlg.m_condcrit = cd.bjc_iscrit;
	switch  (cd.bjc_value.const_type)  {
	default:
		dlg.m_constval = "0";
		break;
	case  CON_LONG:
		char  cbuf[20];
		wsprintf(cbuf, "%ld", cd.bjc_value.con_long);
		dlg.m_constval = cbuf;
		break;
	case  CON_STRING:
		dlg.m_constval = '"';
		dlg.m_constval += cd.bjc_value.con_string;
		dlg.m_constval += '"';
		break;
	}
	if  (dlg.DoModal() == IDOK)  {
		extractcval(cd.bjc_value, dlg.m_constval);
		cd.bjc_compar = dlg.m_condop + C_EQ;
		cd.bjc_iscrit = dlg.m_condcrit;
		ma.defdirty();
	}
}

void CBtrwDoc::OnDefaultsNewassignments()
{
	Cdefass	dlg;
	CBtrwApp	&ma = *((CBtrwApp *) AfxGetApp());
	Jass	&ad = ma.m_defass;
	switch  (ad.bja_con.const_type)  {
	default:
		dlg.m_assvalue = "0";
		break;
	case  CON_LONG:
		char  cbuf[20];
		wsprintf(cbuf, "%ld", ad.bja_con.con_long);
		dlg.m_assvalue = cbuf;
		break;
	case  CON_STRING:
		dlg.m_assvalue = '"';
		dlg.m_assvalue += ad.bja_con.con_string;
		dlg.m_assvalue += '"';
		break;
	}

	dlg.m_asstype = ad.bja_op - BJA_ASSIGN;
	dlg.m_asscrit = ad.bja_iscrit;
	dlg.m_astart = (ad.bja_flags & BJA_START) != 0;
	dlg.m_areverse = (ad.bja_flags & BJA_REVERSE) != 0;
	dlg.m_anormal = (ad.bja_flags & BJA_OK) != 0;
	dlg.m_aerror = (ad.bja_flags & BJA_ERROR) != 0;
	dlg.m_aabort = (ad.bja_flags & BJA_ABORT) != 0;
	dlg.m_acancel = (ad.bja_flags & BJA_CANCEL) != 0;

	if  (dlg.DoModal() == IDOK)  {
		extractcval(ad.bja_con, dlg.m_assvalue);
		ad.bja_op = dlg.m_asstype + BJA_ASSIGN;
		ad.bja_iscrit = dlg.m_asscrit;
		ad.bja_flags = 0;
		if  (dlg.m_astart)
			 ad.bja_flags |= BJA_START;
		if  (dlg.m_areverse)
			 ad.bja_flags |= BJA_REVERSE;
		if  (dlg.m_anormal)
			 ad.bja_flags |= BJA_OK;
		if  (dlg.m_aerror)
			 ad.bja_flags |= BJA_ERROR;
		if  (dlg.m_aabort)
			 ad.bja_flags |= BJA_ABORT;
		if  (dlg.m_acancel)
			 ad.bja_flags |= BJA_CANCEL;
		ma.defdirty();
	}
}
