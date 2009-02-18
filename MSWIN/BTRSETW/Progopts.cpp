// progopts.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "progopts.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Cprogopts dialog

Cprogopts::Cprogopts(CWnd* pParent /*=NULL*/)
	: CDialog(Cprogopts::IDD, pParent)
{
	//{{AFX_DATA_INIT(Cprogopts)
	m_jobqueue = "";
	m_verbose = FALSE;
	m_binmode = FALSE;
	//}}AFX_DATA_INIT
}

void Cprogopts::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Cprogopts)
	DDX_Text(pDX, IDC_JOBQUEUE, m_jobqueue);
	DDX_Check(pDX, IDC_VERBOSE, m_verbose);
	DDX_Check(pDX, IDC_BINMODE, m_binmode);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Cprogopts, CDialog)
	//{{AFX_MSG_MAP(Cprogopts)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Cprogopts message handlers

const DWORD a111HelpIDs[]=
{
	IDC_JOBQUEUE,	IDH_111_328,	// Program Options: "" (Edit)
	IDC_VERBOSE,	IDH_111_296,	// Program Options: "Verbose" (Button)
	IDC_BINMODE,	IDH_111_297,	// Program Options: "Binary mode (no CR conversion)" (Button)
	0, 0
};

BOOL Cprogopts::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a111HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
