// editenv.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "editenv.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditenv dialog

CEditenv::CEditenv(CWnd* pParent /*=NULL*/)
	: CDialog(CEditenv::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditenv)
	m_ename = "";
	m_evalue = "";
	//}}AFX_DATA_INIT
}

void CEditenv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditenv)
	DDX_Text(pDX, IDC_ENAME, m_ename);
	DDX_Text(pDX, IDC_EVALUE, m_evalue);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditenv, CDialog)
	//{{AFX_MSG_MAP(CEditenv)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditenv message handlers

const DWORD a108HelpIDs[]=
{
	IDC_ENAME,	IDH_108_183,	// Edit environment variable: "" (Edit)
	IDC_EVALUE,	IDH_108_184,	// Edit environment variable: "" (Edit)
	0, 0
};

BOOL CEditenv::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a108HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
