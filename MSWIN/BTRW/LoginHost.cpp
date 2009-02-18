// LoginHost.cpp : implementation file
//

#include "stdafx.h"
#include "btrw.h"
#include "LoginHost.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginHost dialog


CLoginHost::CLoginHost(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginHost::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginHost)
	m_unixhost = _T("");
	m_clienthost = _T("");
	m_username = _T("");
	m_passwd = _T("");
	//}}AFX_DATA_INIT
}


void CLoginHost::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginHost)
	DDX_Text(pDX, IDC_UNIXHOST, m_unixhost);
	DDX_Text(pDX, IDC_CLIENTHOST, m_clienthost);
	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDV_MaxChars(pDX, m_username, 11);
	DDX_Text(pDX, IDC_PASSWD, m_passwd);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginHost, CDialog)
	//{{AFX_MSG_MAP(CLoginHost)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginHost message handlers

const DWORD a114HelpIDs[]=
{
	IDC_UNIXHOST,	IDH_114_249,	// Please log in to Unix Host....: "" (Edit)
	IDC_CLIENTHOST,	IDH_114_250,	// Please log in to Unix Host....: "" (Edit)
	IDC_USERNAME,	IDH_114_251,	// Please log in to Unix Host....: "" (Edit)
	IDC_PASSWD,	IDH_114_252,	// Please log in to Unix Host....: "" (Edit)
	0, 0
};

BOOL CLoginHost::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a114HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
