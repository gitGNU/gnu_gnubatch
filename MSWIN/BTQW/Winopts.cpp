// winopts.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "winopts.h"
#include "ulist.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinopts dialog

CWinopts::CWinopts(CWnd* pParent /*=NULL*/)
	: CDialog(CWinopts::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWinopts)
	m_jobqueue = "";
	m_localonly = -1;
	m_confdel = -1;
	m_username = "";
	m_groupname = "";
	m_incnull = FALSE;
	//}}AFX_DATA_INIT
	m_isjobs = TRUE;
}

void CWinopts::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinopts)
	DDX_CBString(pDX, IDC_JOBQUEUE, m_jobqueue);
	DDX_Radio(pDX, IDC_LOCALONLY, m_localonly);
	DDX_Radio(pDX, IDC_NEVER, m_confdel);
	DDX_CBString(pDX, IDC_USER, m_username);
	DDV_MaxChars(pDX, m_username, 11);
	DDX_CBString(pDX, IDC_GROUP, m_groupname);
	DDV_MaxChars(pDX, m_groupname, 11);
	DDX_Check(pDX, IDC_INCNULL, m_incnull);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWinopts, CDialog)
	//{{AFX_MSG_MAP(CWinopts)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinopts message handlers


BOOL CWinopts::OnInitDialog()
{
	CDialog::OnInitDialog();	
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_JOBQUEUE);
	if  (m_isjobs)  {
		CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_JOBQUEUE);
		jobqueuelist  jq;
		CString  *qs;
		while  (qs = jq.next())
			uP->AddString(*qs);
		uP->SelectString(-1, m_jobqueue);
	}
	else  {
		GetDlgItem(IDC_JOBQUEUE)->EnableWindow(FALSE);
		GetDlgItem(IDC_JOBQNAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_DELJOBSNAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_NEVER)->EnableWindow(FALSE);
		GetDlgItem(IDC_ALWAYS)->EnableWindow(FALSE);
	}		
	UUserList	unixusers;
	const  char  FAR *nu;
	CComboBox  *uPu = (CComboBox *)GetDlgItem(IDC_USER);
	while  (nu = unixusers.next())
		uPu->AddString(nu);
	uPu->SelectString(-1, m_username);
	UGroupList	unixgroups;
	uPu = (CComboBox *)GetDlgItem(IDC_GROUP);
	while  (nu = unixgroups.next())
		uPu->AddString(nu);
	uPu->SelectString(-1, m_groupname);
	return TRUE;
}

void CWinopts::OnOK()
{
	if  (!m_isjobs)  {
		GetDlgItem(IDC_JOBQUEUE)->EnableWindow(TRUE);
//		GetDlgItem(IDC_JOBQNAME)->EnableWindow(TRUE);
//		GetDlgItem(IDC_DELJOBSNAME)->EnableWindow(TRUE);
		GetDlgItem(IDC_NEVER)->EnableWindow(TRUE);
		GetDlgItem(IDC_ALWAYS)->EnableWindow(TRUE);
	}
	CDialog::OnOK();
}

const DWORD a138HelpIDs[]=
{
	IDC_JOBQUEUE,	IDH_138_306,	// Window Options: "" (ComboBox)
	IDC_INCNULL,	IDH_138_307,	// Window Options: "Including empty" (Button)
	IDC_NEVER,	IDH_138_309,	// Window Options: "Never" (Button)
	IDC_ALWAYS,	IDH_138_310,	// Window Options: "Always" (Button)
	IDC_USER,	IDH_138_264,	// Window Options: "" (ComboBox)
	IDC_GROUP,	IDH_138_265,	// Window Options: "" (ComboBox)
	IDC_LOCALONLY,	IDH_138_313,	// Window Options: "Display local host only" (Button)
	IDC_DISPALL,	IDH_138_314,	// Window Options: "Display all hosts" (Button)
	0, 0
};

BOOL CWinopts::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a138HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
