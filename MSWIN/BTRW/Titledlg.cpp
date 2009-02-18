// titledlg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#define	APPCLASS	CBtrwApp
#include "titledlg.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTitledlg dialog

CTitledlg::CTitledlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTitledlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTitledlg)
	m_title = "";
	m_cmdinterp = "";
	m_priority = 0;
	m_loadlev = 0;
	//}}AFX_DATA_INIT
}

void CTitledlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTitledlg)
	DDX_Text(pDX, IDC_TITLE, m_title);
	DDX_CBString(pDX, IDC_CMDINTERP, m_cmdinterp);
	DDV_MaxChars(pDX, m_cmdinterp, 15);
	DDX_Text(pDX, IDC_PRIORITY, m_priority);
	DDV_MinMaxUInt(pDX, m_priority, 1, 255);
	DDX_Text(pDX, IDC_LOADLEV, m_loadlev);
	DDV_MinMaxUInt(pDX, m_loadlev, 1, 65535);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTitledlg, CDialog)
	//{{AFX_MSG_MAP(CTitledlg)
	ON_CBN_SELCHANGE(IDC_CMDINTERP, OnSelchangeCmdinterp)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTitledlg message handlers

BOOL CTitledlg::OnInitDialog()
{
	CDialog::OnInitDialog();	
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_CMDINTERP);
	CIList	&cil = ((APPCLASS *)AfxGetApp())->m_cilist;
	cil.init_list();
	const  Cmdint	*n;
	while  (n = cil.next())
		uP->SetItemData(uP->AddString(n->ci_name), DWORD(n->ci_ll));
	uP->SelectString(-1, m_cmdinterp);

	if  (m_writeable)  {   
		Btuser	&mypriv = ((APPCLASS *)AfxGetApp())->m_mypriv;
		((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_PRIORITY))->SetRange(mypriv.btu_minp, mypriv.btu_maxp);
		((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_LOADLEV))->SetRange(1, mypriv.btu_maxll);
	}
	else  {
		GetDlgItem(IDC_PRIORITY)->EnableWindow(FALSE);
		GetDlgItem(IDC_LOADLEV)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_PRIORITY)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_LOADLEV)->EnableWindow(FALSE);
		GetDlgItem(IDC_TITLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_CMDINTERP)->EnableWindow(FALSE);
	}	
	return TRUE;
}


void CTitledlg::OnSelchangeCmdinterp()
{
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_CMDINTERP);
	SetDlgItemInt(IDC_LOADLEV, UINT(uP->GetItemData(uP->GetCurSel())), FALSE);
}

const DWORD a122HelpIDs[]=
{
	IDC_TITLE,	IDH_122_351,	// Title Command Int Priority Load level: "" (Edit)
	IDC_CMDINTERP,	IDH_122_352,	// Title Command Int Priority Load level: "" (ComboBox)
	IDC_PRIORITY,	IDH_122_353,	// Title Command Int Priority Load level: "0" (Edit)
	IDC_SCR_PRIORITY,	IDH_122_353,	// Title Command Int Priority Load level: "Spin1" (msctls_updown32)
	IDC_LOADLEV,	IDH_122_355,	// Title Command Int Priority Load level: "0" (Edit)
	IDC_SCR_LOADLEV,	IDH_122_355,	// Title Command Int Priority Load level: "Spin2" (msctls_updown32)
	0, 0
};

BOOL CTitledlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a122HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
