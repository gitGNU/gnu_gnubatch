// cpyoptdl.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "cpyoptdl.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCpyOptdlg dialog


CCpyOptdlg::CCpyOptdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCpyOptdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCpyOptdlg)
	m_ascanc = -1;
	//}}AFX_DATA_INIT
}

void CCpyOptdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCpyOptdlg)
	DDX_Radio(pDX, IDC_ASCANC, m_ascanc);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCpyOptdlg, CDialog)
	//{{AFX_MSG_MAP(CCpyOptdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CCpyOptdlg message handlers

const DWORD a104HelpIDs[]=
{
	IDC_ASCANC,	IDH_104_156,	// Copy job options: "As cancelled" (Button)
	IDC_ASREADY,	IDH_104_157,	// Copy job options: "As ready to run" (Button)
	0, 0
};

BOOL CCpyOptdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a104HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
