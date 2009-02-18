// editarg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "editarg.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditarg dialog

CEditarg::CEditarg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditarg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditarg)
	m_argval = "";
	//}}AFX_DATA_INIT
}

void CEditarg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditarg)
	DDX_Text(pDX, IDC_ARGVAL, m_argval);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditarg, CDialog)
	//{{AFX_MSG_MAP(CEditarg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditarg message handlers

const DWORD a105HelpIDs[]=
{
	IDC_ARGVAL,	IDH_105_180,	// Edit Argument: "" (Edit)
	0, 0
};

BOOL CEditarg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a105HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
