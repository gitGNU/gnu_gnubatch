// mainfrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mainfrm.h" 
#include "jobdoc.h"
#include "vardoc.h"
#include "jdatadoc.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"
#include "poptsdlg.h"
#include "deftimop.h"
#include "defcond.h"
#include "defass.h"
#include "formatcode.h"
#include "fmtdef.h"
#include "netstat.h"
#include "uperms.h"
#include "Jobcolours.h"
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_WINDOW_NEWJOBWINDOW, OnWindowNewjobwindow)
	ON_COMMAND(ID_FILE_PROGRAMOPTIONS, OnFileProgramoptions)
	ON_MESSAGE(WM_NETMSG_ARRIVED, OnNMArrived)
	ON_MESSAGE(WM_NETMSG_NEWCONN, OnNMNewconn)
	ON_MESSAGE(WM_NETMSG_PROBERCV, OnNMProbercv)
	ON_MESSAGE(WM_NETMSG_MESSAGE, OnNMMessrcv)
	ON_COMMAND(ID_FILE_SAVEFILE, OnFileSavefile)
	ON_COMMAND(ID_WINDOW_NEWVARWINDOW, OnWindowNewvarwindow)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEFILE, OnUpdateFileSavefile)
	ON_COMMAND(ID_FILE_TIMEDEFAULTS, OnFileTimedefaults)
	ON_COMMAND(ID_FILE_CONDITIONDEFAULTS, OnFileConditiondefaults)
	ON_COMMAND(ID_FILE_ASSIGNMENTDEFAULTS, OnFileAssignmentdefaults)
	ON_COMMAND(ID_WINDOW_JOBDISPLAYFORMAT, OnWindowJobdisplayformat)
	ON_COMMAND(ID_WINDOW_VARDISPLAYFMTLINE1, OnWindowVardisplayfmtline1)
	ON_COMMAND(ID_FILE_NETWORKSTATS, OnFileNetworkstats)
	ON_COMMAND(ID_FILE_USERPERMS, OnFileUserperms)
	ON_WM_TIMER()
	ON_COMMAND(ID_JCOLOUR_ABORT, OnJcolourAbort)
	ON_COMMAND(ID_JCOLOUR_CANCELLED, OnJcolourCancelled)
	ON_COMMAND(ID_JCOLOUR_ERROR, OnJcolourError)
	ON_COMMAND(ID_JCOLOUR_FINISHED, OnJcolourFinished)
	ON_COMMAND(ID_JCOLOUR_READY, OnJcolourReady)
	ON_COMMAND(ID_JCOLOUR_RUNNING, OnJcolourRunning)
	ON_COMMAND(ID_JCOLOUR_STARTUP, OnJcolourStartup)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_INDEX, CMDIFrameWnd::OnHelpIndex)
	ON_COMMAND(ID_HELP_USING, CMDIFrameWnd::OnHelpUsing)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpIndex)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

// toolbar buttons - IDs are command buttons

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE("Failed to create toolbar\n");
		return -1;		// fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))  {
		TRACE("Failed to create status bar\n");
		return -1;		// fail to create
	}

	return 0;
}

void	CMainFrame::InitWins()
{
	OnWindowNewjobwindow();
    MDITile(MDITILE_HORIZONTAL);
}    

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnWindowNewjobwindow()
{
	CJobdoc *doc = (CJobdoc *) m_dtjob->CreateNewDocument();
	static int count = 0;
	char	tbuf[20];
	wsprintf(tbuf, "Job List %d", ++count);
	doc->SetTitle(tbuf);
	doc->m_wrestrict = ((CBtqwApp *)AfxGetApp())->m_restrict;
	CFrameWnd *frm = m_dtjob->CreateNewFrame(doc, NULL);
	m_dtjob->InitialUpdateFrame(frm, doc);
}

void CMainFrame::OnWindowNewvarwindow()
{
	CVardoc *doc = (CVardoc *) m_dtvar->CreateNewDocument();
	static int count = 0;
	char	tbuf[26];
	wsprintf(tbuf, "Variables List %d", ++count);
	doc->SetTitle(tbuf);
	doc->m_wrestrict = ((CBtqwApp *)AfxGetApp())->m_restrict;
	CFrameWnd *frm = m_dtvar->CreateNewFrame(doc, NULL);
	m_dtvar->InitialUpdateFrame(frm, doc);
}

void CMainFrame::OnNewJDWin(Btjob *cj)
{
	CJdatadoc *doc = (CJdatadoc *) m_dtjdata->CreateNewDocument();
	doc->setjob(*cj);
	if  (!doc->loaddoc())
		doc->m_invalid = TRUE;
	CFrameWnd *frm = m_dtjdata->CreateNewFrame(doc, NULL);
	m_dtjdata->InitialUpdateFrame(frm, doc);	
}	

void CMainFrame::OnFileProgramoptions()
{
	CPOptsdlg	dlg;
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	dlg.m_confabort = app.m_restrict.confd == restrictdoc::ALWAYS;
	dlg.m_probewarn = app.m_warnoffline ? 1: 0; 
	dlg.m_jobqueue = app.m_restrict.queuename;
	dlg.m_incnull = app.m_restrict.incnull;
	dlg.m_localonly = app.m_restrict.onlyl == restrictdoc::YES? 0: 1;
	dlg.m_polltime = app.m_polltime;
	dlg.m_user = app.m_restrict.user;
	dlg.m_group = app.m_restrict.group;
	dlg.m_binunq = app.m_binunq;
	dlg.m_sjext = app.m_sjbat;
	if  (dlg.DoModal() == IDOK)  {
		if  (dlg.m_confabort > 0)
			app.m_restrict.confd = restrictdoc::ALWAYS;
		else
			app.m_restrict.confd = restrictdoc::NEVER;
		app.m_warnoffline = dlg.m_probewarn != 0;
		app.m_restrict.queuename = dlg.m_jobqueue;
		app.m_restrict.incnull = dlg.m_incnull;
		app.m_restrict.onlyl = dlg.m_localonly == 0? restrictdoc::YES: restrictdoc::NO;
		app.m_polltime = dlg.m_polltime;
		app.m_restrict.user = dlg.m_user;
		app.m_restrict.group = dlg.m_group;
		app.m_binunq = dlg.m_binunq;
		app.m_sjbat = dlg.m_sjext;
		app.dirty();
	}
}		

// Accept stuff arriving from network
// Some don't work properly with FTP TCP/IP (does anything???)

LRESULT CMainFrame::OnNMArrived(WPARAM wParam, LPARAM lParam)
{
	net_recvmsg(wParam, lParam);
	return  0;
}

LRESULT CMainFrame::OnNMNewconn(WPARAM wParam, LPARAM lParam)
{
	return  0;
}

LRESULT CMainFrame::OnNMProbercv(WPARAM wParam, LPARAM lParam)
{             
	net_recvprobe(wParam, lParam);
	return  0;
}                                            

LRESULT CMainFrame::OnNMMessrcv(WPARAM wParam, LPARAM lParam)
{            
	unsigned  mcode = 0;
	
	switch  (wParam & REQ_TYPE)  {
	case  SYS_REPLY:
		if  (wParam == O_NOPERM)
			mcode = IDP_SYSM_NOPERM;
		break;
	case  JOB_REPLY:                                
		if  (wParam != J_OK)
			mcode = wParam - J_OK + IDP_JOBM_OK;
		break;
	case  VAR_REPLY:
		if  (wParam != V_OK)
			mcode = wParam - V_OK + IDP_VARM_OK;
		break;
	case  NET_REPLY:
		if  (wParam != N_CONNOK)
			mcode = wParam - N_CONNOK + IDP_NETM_OK;
		break;
	}                
	if  (mcode != 0)
		AfxMessageBox(mcode, MB_OK|MB_ICONEXCLAMATION);
	return  0;
}		

void CMainFrame::OnFileSavefile()
{                                
	((CBtqwApp *) AfxGetApp())->saveprogopts();
}

void CMainFrame::OnUpdateFileSavefile(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(((CBtqwApp *) AfxGetApp())->isdirty());	
}

void CMainFrame::OnFileTimedefaults()
{
	Cdeftimopt	dlg;
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	timedefs	&td = app.m_timedefs;
	dlg.m_nptype = td.m_nptype;
	dlg.m_repeatopt = td.m_repeatopt;
	dlg.m_avsun = td.m_avsun;
	dlg.m_avmon = td.m_avmon;
	dlg.m_avtue = td.m_avtue;
	dlg.m_avwed = td.m_avwed;
	dlg.m_avthu = td.m_avthu;
	dlg.m_avfri = td.m_avfri;
	dlg.m_avsat = td.m_avsat;
	dlg.m_avhol = td.m_avhol;
	dlg.m_tc_mday = td.m_tc_mday;
	dlg.m_tc_rate = td.m_tc_rate;
	if  (dlg.DoModal() == IDOK)  {
		td.m_nptype = dlg.m_nptype;
		td.m_repeatopt = dlg.m_repeatopt;
		td.m_avsun = dlg.m_avsun;
		td.m_avmon = dlg.m_avmon;
		td.m_avtue = dlg.m_avtue;
		td.m_avwed = dlg.m_avwed;
		td.m_avthu = dlg.m_avthu;
		td.m_avfri = dlg.m_avfri;
		td.m_avsat = dlg.m_avsat;
		td.m_avhol = dlg.m_avhol;
		td.m_tc_mday = dlg.m_tc_mday;
		td.m_tc_rate = dlg.m_tc_rate;
		app.dirty();
	}
}

void CMainFrame::OnFileConditiondefaults()
{
	Cdefcond	dlg;
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	conddefs	&cd = app.m_conddefs;
	dlg.m_constval = cd.m_constval.IsEmpty()? "0": cd.m_constval;
	dlg.m_condop = cd.m_condop;
	dlg.m_condcrit = cd.m_condcrit;
	if  (dlg.DoModal() == IDOK)  {
		cd.m_constval = dlg.m_constval.GetLength() > 0 &&
						 (isdigit(dlg.m_constval[0]) || dlg.m_constval[0] == '-' || dlg.m_constval[0] == '"')?
						dlg.m_constval:
						'"' + dlg.m_constval + '"';
		cd.m_condop = dlg.m_condop;
		cd.m_condcrit = dlg.m_condcrit;
		app.dirty();
	}
}

void CMainFrame::OnFileAssignmentdefaults()
{
	Cdefass	dlg;
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	assdefs	&cd = app.m_assdefs;
	dlg.m_assvalue = cd.m_assvalue.IsEmpty()? "0": cd.m_assvalue;
	dlg.m_asstype = cd.m_asstype;
	dlg.m_asscrit = cd.m_asscrit;
	dlg.m_astart = cd.m_astart;
	dlg.m_areverse = cd.m_areverse;
	dlg.m_anormal = cd.m_anormal;
	dlg.m_aerror = cd.m_aerror;
	dlg.m_aabort = cd.m_aabort;
	dlg.m_acancel = cd.m_acancel;
	if  (dlg.DoModal() == IDOK)  {
		cd.m_assvalue = dlg.m_assvalue.GetLength() > 0 &&
						 (isdigit(dlg.m_assvalue[0]) || dlg.m_assvalue[0] == '-' || dlg.m_assvalue[0] == '"')?
						dlg.m_assvalue:
						'"' + dlg.m_assvalue + '"';
		cd.m_asstype = dlg.m_asstype;
		cd.m_asscrit = dlg.m_asscrit;
		cd.m_astart = dlg.m_astart;
		cd.m_areverse = dlg.m_areverse;
		cd.m_anormal = dlg.m_anormal;
		cd.m_aerror = dlg.m_aerror;
		cd.m_aabort = dlg.m_aabort;
		cd.m_acancel = dlg.m_acancel;
		app.dirty();
	}
}

void CMainFrame::OnWindowJobdisplayformat()
{
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	CFmtdef	dlg;
	dlg.m_fmtstring = app.m_jfmt;
	dlg.m_defcode = IDS_DEF_JFMT;
	dlg.m_what4 = IDS_EDITINGJFMT;
	dlg.m_uppercode = IDS_JFORMAT_A;
	dlg.m_lowercode = IDS_JFORMAT_aa;
	if  (dlg.DoModal() == IDOK)  {
		app.m_jfmt = dlg.m_fmtstring;
		app.dirty();
		app.m_pMainWnd->SendMessageToDescendants(WM_NETMSG_JREVISED, 0, 1);
	}
}

void CMainFrame::OnWindowVardisplayfmtline1()
{
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	CFmtdef	dlg;
	dlg.m_fmtstring = app.m_v1fmt;
	dlg.m_defcode = IDS_DEF_V1FMT;
	dlg.m_what4 = IDS_EDITINGV1FMT;
	dlg.m_uppercode = IDS_VFORMAT_C - 2;
	dlg.m_lowercode = 0;
	if  (dlg.DoModal() == IDOK)  {
		app.m_v1fmt = dlg.m_fmtstring;
		app.dirty();
		app.m_pMainWnd->SendMessageToDescendants(WM_NETMSG_VREVISED, 0, 1);
	}
}

void CMainFrame::OnFileNetworkstats()
{
	CNetstat	dlg;
	dlg.DoModal();
}

void CMainFrame::OnFileUserperms()
{
	CUperms		dlg;
	CBtqwApp	&ma = *((CBtqwApp *)AfxGetApp());
	dlg.m_priv = &ma.m_mypriv;
	dlg.m_user = ma.m_username;
	dlg.m_group = ma.m_groupname;
	dlg.DoModal();
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	refreshconn();
}

void CMainFrame::OnJcolourAbort() 
{
	RunColourDlg(BJP_ABORTED);
}

void CMainFrame::OnJcolourCancelled() 
{
	RunColourDlg(BJP_CANCELLED);
}

void CMainFrame::OnJcolourError() 
{
	RunColourDlg(BJP_ERROR);
}

void CMainFrame::OnJcolourFinished() 
{
	RunColourDlg(BJP_DONE);			
}

void CMainFrame::OnJcolourReady() 
{
	RunColourDlg(BJP_NONE);
}

void CMainFrame::OnJcolourRunning() 
{
	RunColourDlg(BJP_RUNNING);
}

void CMainFrame::OnJcolourStartup() 
{
	RunColourDlg(BJP_STARTUP1);		
}

void CMainFrame::RunColourDlg(const unsigned n)
{
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	COLORREF	&mc = app.m_appcolours[n];
	CColorDialog	cdlg;
	cdlg.m_cc.Flags |= CC_RGBINIT|CC_SOLIDCOLOR;
	cdlg.m_cc.rgbResult = mc;
	if  (cdlg.DoModal() == IDOK)  {
		mc = cdlg.m_cc.rgbResult;
		app.dirty();
	}
}
