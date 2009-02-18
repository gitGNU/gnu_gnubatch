// maildlg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "maildlg.h"
#include "Btrw.hpp"

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

const DWORD a115HelpIDs[]=
{
	IDC_WRITE,	IDH_115_254,	// Mail/Write flags: "Write completion" (Button)
	IDC_MAIL,	IDH_115_253,	// Mail/Write flags: "Mail completion" (Button)
	0, 0
};

BOOL CMaildlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a115HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
