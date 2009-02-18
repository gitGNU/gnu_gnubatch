// editsep.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "editsep.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditsep dialog


CEditsep::CEditsep(CWnd* pParent /*=NULL*/)
	: CDialog(CEditsep::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditsep)
	m_sepvalue = "";
	//}}AFX_DATA_INIT
}

void CEditsep::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditsep)
	DDX_Text(pDX, IDC_SEPVALUE, m_sepvalue);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditsep, CDialog)
	//{{AFX_MSG_MAP(CEditsep)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CEditsep message handlers

const DWORD a114HelpIDs[]=
{
	IDC_SEPVALUE,	IDH_114_211,	// New separator string: "" (Edit)
	0, 0
};

BOOL CEditsep::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a114HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
