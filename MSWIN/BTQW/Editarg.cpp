// editarg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "editarg.h"
#include "Btqw.hpp"

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

const DWORD a108HelpIDs[]=
{
	IDC_ARGVAL,	IDH_108_158,	// Edit Argument: "" (Edit)
	0, 0
};

BOOL CEditarg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a108HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
