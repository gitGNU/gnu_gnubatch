// othersig.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "othersig.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COthersig dialog

COthersig::COthersig(CWnd* pParent /*=NULL*/)
	: CDialog(COthersig::IDD, pParent)
{
	//{{AFX_DATA_INIT(COthersig)
	m_sigtype = -1;
	//}}AFX_DATA_INIT
}

void COthersig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COthersig)
	DDX_Radio(pDX, IDC_SIG_HANGUP, m_sigtype);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COthersig, CDialog)
	//{{AFX_MSG_MAP(COthersig)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COthersig message handlers

const DWORD a124HelpIDs[]=
{
	IDC_SIG_HANGUP,	IDH_124_277,	// Send kill signal: "Hangup" (Button)
	IDC_SIG_INTERRUPT,	IDH_124_278,	// Send kill signal: "Interrupt" (Button)
	IDC_SIG_QUIT,	IDH_124_279,	// Send kill signal: "Quit" (Button)
	IDC_SIG_TERMINATE,	IDH_124_280,	// Send kill signal: "Terminate" (Button)
	IDC_SIG_KILL,	IDH_124_281,	// Send kill signal: "KILL" (Button)
	0, 0
};

BOOL COthersig::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a124HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
