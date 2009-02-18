// maildlg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "maildlg.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMaildlg dialog

CMaildlg::CMaildlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMaildlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMaildlg)
	m_mail = FALSE;
	m_write = FALSE;
	//}}AFX_DATA_INIT
}

void CMaildlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMaildlg)
	DDX_Check(pDX, IDC_MAIL, m_mail);
	DDX_Check(pDX, IDC_WRITE, m_write);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMaildlg, CDialog)
	//{{AFX_MSG_MAP(CMaildlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMaildlg message handlers

BOOL CMaildlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if  (!m_writeable)  {
		GetDlgItem(IDC_MAIL)->EnableWindow(FALSE);
		GetDlgItem(IDC_WRITE)->EnableWindow(FALSE);
	}	
	return TRUE;
}

const DWORD a122HelpIDs[]=
{
	IDC_MAIL,	IDH_122_270,	// Mail/Write flags: "Mail completion" (Button)
	IDC_WRITE,	IDH_122_271,	// Mail/Write flags: "Write completion" (Button)
	0, 0
};

BOOL CMaildlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a122HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
