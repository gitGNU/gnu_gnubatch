// LoginOut.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "LoginOut.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginOut dialog


CLoginOut::CLoginOut(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginOut::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginOut)
	m_unixuser = _T("");
	m_winuser = _T("");
	//}}AFX_DATA_INIT
}


void CLoginOut::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginOut)
	DDX_Text(pDX, IDC_UNIXUSER, m_unixuser);
	DDX_Text(pDX, IDC_WINUSER, m_winuser);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginOut, CDialog)
	//{{AFX_MSG_MAP(CLoginOut)
	ON_BN_CLICKED(IDC_LOGIN, OnLogin)
	ON_BN_CLICKED(IDC_LOGOUT, OnLogout)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginOut message handlers

void CLoginOut::OnLogin() 
{
	EndDialog(IDC_LOGIN);
}

void CLoginOut::OnLogout() 
{
	EndDialog(IDC_LOGOUT);
}

const DWORD a107HelpIDs[]=
{
	IDC_UNIXUSER,	IDH_107_249,	// Login as new user or log out: "" (Edit)
	IDC_WINUSER,	IDH_107_250,	// Login as new user or log out: "" (Edit)
	IDC_LOGIN,	IDH_107_251,	// Login as new user or log out: "Login as new user" (Button)
	IDC_LOGOUT,	IDH_107_252,	// Login as new user or log out: "Log out" (Button)
	0, 0
};

BOOL CLoginOut::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a107HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
