// queuedef.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "queuedef.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQueuedef dialog

CQueuedef::CQueuedef(CWnd* pParent /*=NULL*/)
	: CDialog(CQueuedef::IDD, pParent)
{
	//{{AFX_DATA_INIT(CQueuedef)
	m_title = "";
	m_cmdint = "";
	m_priority = 0;
	m_loadlev = 0;
	//}}AFX_DATA_INIT
}

void CQueuedef::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQueuedef)
	DDX_Text(pDX, IDC_TITLE, m_title);
	DDX_CBString(pDX, IDC_CMDINTERP, m_cmdint);
	DDX_Text(pDX, IDC_PRIORITY, m_priority);
	DDV_MinMaxUInt(pDX, m_priority, 1, 255);
	DDX_Text(pDX, IDC_LOADLEV, m_loadlev);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CQueuedef, CDialog)
	//{{AFX_MSG_MAP(CQueuedef)
	ON_CBN_SELCHANGE(IDC_CMDINTERP, OnSelchangeCmdinterp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueuedef message handlers

BOOL CQueuedef::OnInitDialog()
{
	CDialog::OnInitDialog();	
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_CMDINTERP);
	CIList	&cil = ((CBtrsetwApp *)AfxGetApp())->m_cilist;
	const  Cmdint	*n;
	while  (n = cil.next())
		uP->SetItemData(uP->AddString(n->ci_name), DWORD(n->ci_ll));
	uP->SelectString(-1, m_cmdint);
	Btuser	&mypriv = ((CBtrsetwApp *)AfxGetApp())->m_mypriv;
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_PRIORITY))->SetRange(mypriv.btu_minp, mypriv.btu_maxp);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_LOADLEV))->SetRange(1, mypriv.btu_maxll);
	if  (!(((CBtrsetwApp *)AfxGetApp())->m_mypriv.btu_priv & BTM_SPCREATE))  {
		GetDlgItem(IDC_LOADLEV)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_LOADLEV)->EnableWindow(FALSE);
	}	
	return TRUE;
}

void CQueuedef::OnSelchangeCmdinterp()
{
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_CMDINTERP);
	SetDlgItemInt(IDC_LOADLEV, UINT(uP->GetItemData(uP->GetCurSel())), FALSE);
}

const DWORD a112HelpIDs[]=
{
	IDC_PRIORITY,	IDH_112_300,	// Queue defaults: "0" (Edit)
	IDC_SCR_PRIORITY,	IDH_112_300,	// Queue defaults: "Spin1" (msctls_updown32)
	IDC_LOADLEV,	IDH_112_302,	// Queue defaults: "0" (Edit)
	IDC_SCR_LOADLEV,	IDH_112_302,	// Queue defaults: "Spin2" (msctls_updown32)
	IDC_TITLE,	IDH_112_298,	// Queue defaults: "" (Edit)
	IDC_CMDINTERP,	IDH_112_299,	// Queue defaults: "" (ComboBox)
	0, 0
};

BOOL CQueuedef::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a112HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
