// varcomm.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "varcomm.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVarcomm dialog

CVarcomm::CVarcomm(CWnd* pParent /*=NULL*/)
	: CDialog(CVarcomm::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVarcomm)
	m_varcomment = "";
	m_varname = "";
	//}}AFX_DATA_INIT
}

void CVarcomm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVarcomm)
	DDX_Text(pDX, IDC_AVARCOMMENT, m_varcomment);
	DDV_MaxChars(pDX, m_varcomment, 41);
	DDX_Text(pDX, IDC_DVARNAME, m_varname);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVarcomm, CDialog)
	//{{AFX_MSG_MAP(CVarcomm)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVarcomm message handlers

const DWORD a135HelpIDs[]=
{
	IDC_AVARCOMMENT,	IDH_135_417,	// Variable Comment: "" (Edit)
	IDC_DVARNAME,	IDH_135_413,	// Variable Comment: "" (Edit)
	0, 0
};

BOOL CVarcomm::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a135HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
