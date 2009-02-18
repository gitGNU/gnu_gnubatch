// poptsdlg.cpp : implementation file
//

#include "stdafx.h"
#include "ulist.h"
#include "netmsg.h"
#include "btqw.h"    
#include "poptsdlg.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPOptsdlg dialog

CPOptsdlg::CPOptsdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPOptsdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPOptsdlg)
	m_confabort = -1;
	m_probewarn = -1;
	m_jobqueue = "";
	m_localonly = -1;
	m_polltime = 0;
	m_user = "";
	m_group = "";
	m_incnull = FALSE;
	m_binunq = FALSE;
	m_sjext = -1;
	//}}AFX_DATA_INIT
}

void CPOptsdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPOptsdlg)
	DDX_Radio(pDX, IDC_NEVER, m_confabort);
	DDX_Radio(pDX, IDC_PROBE_IGNORE, m_probewarn);
	DDX_CBString(pDX, IDC_JOBQUEUE, m_jobqueue);
	DDX_Radio(pDX, IDC_LOCALONLY, m_localonly);
	DDX_Text(pDX, IDC_POLLTIME, m_polltime);
	DDX_CBString(pDX, IDC_USER, m_user);
	DDV_MaxChars(pDX, m_user, 11);
	DDX_CBString(pDX, IDC_GROUP, m_group);
	DDV_MaxChars(pDX, m_group, 11);
	DDX_Check(pDX, IDC_INCNULL, m_incnull);
	DDX_Check(pDX, IDC_BINUNQ, m_binunq);
	DDX_Radio(pDX, IDC_SJXBC, m_sjext);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPOptsdlg, CDialog)
	//{{AFX_MSG_MAP(CPOptsdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

BOOL CPOptsdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_POLLTIME))->SetRange(1, SHRT_MAX);
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_JOBQUEUE);
	jobqueuelist  jq;
	CString  *qs;
	while  (qs = jq.next())
		uP->AddString(*qs);
	uP->SelectString(-1, m_jobqueue);
	uP = (CComboBox *)GetDlgItem(IDC_USER);
	UUserList	unixusers;
	const  char  FAR *nu;
	while  (nu = unixusers.next())
		uP->AddString(nu);
	uP->SelectString(-1, m_user);
	UGroupList	unixgroups;
	uP = (CComboBox *)GetDlgItem(IDC_GROUP);
	while  (nu = unixgroups.next())
		uP->AddString(nu);
	uP->SelectString(-1, m_group);
	return TRUE;
}

const DWORD a126HelpIDs[]=
{
	IDC_JOBQUEUE,	IDH_126_306,	// Program options: "" (ComboBox)
	IDC_INCNULL,	IDH_126_307,	// Program options: "Including empty" (Button)
	IDC_NEVER,	IDH_126_309,	// Program options: "Never" (Button)
	IDC_ALWAYS,	IDH_126_310,	// Program options: "Always" (Button)
	IDC_USER,	IDH_126_264,	// Program options: "" (ComboBox)
	IDC_PROBE_IGNORE,	IDH_126_311,	// Program options: "Ignore" (Button)
	IDC_GROUP,	IDH_126_265,	// Program options: "" (ComboBox)
	IDC_PROBE_WARN,	IDH_126_312,	// Program options: "Warn" (Button)
	IDC_LOCALONLY,	IDH_126_313,	// Program options: "Display local host only" (Button)
	IDC_DISPALL,	IDH_126_314,	// Program options: "Display all hosts" (Button)
	IDC_POLLTIME,	IDH_126_315,	// Program options: "0" (Edit)
	IDC_SCR_POLLTIME,	IDH_126_315,	// Program options: "Spin1" (msctls_updown32)
	IDC_BINUNQ,	IDH_126_317,	// Program options: "Unqueue as binary" (Button)
	IDC_SJXBC,	IDH_126_318,	// Program options: "XBC" (Button)
	IDC_SJBAT,	IDH_126_319,	// Program options: "BAT" (Button)
	0, 0
};

BOOL CPOptsdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a126HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
