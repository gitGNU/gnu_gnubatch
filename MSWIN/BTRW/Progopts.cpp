// progopts.cpp : implementation file
//

#include "stdafx.h"
#include "btrw.h"
#include "progopts.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgopts dialog


CProgopts::CProgopts(CWnd* pParent /*=NULL*/)
	: CDialog(CProgopts::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgopts)
	m_jeditor = "";
	m_binmode = FALSE;
	m_jobqueue = "";
	m_verbose = FALSE;
	m_nonexport = FALSE;
	//}}AFX_DATA_INIT
}

void CProgopts::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgopts)
	DDX_Text(pDX, IDC_JEDITOR, m_jeditor);
	DDV_MaxChars(pDX, m_jeditor, 8);
	DDX_Check(pDX, IDC_BINMODE, m_binmode);
	DDX_Text(pDX, IDC_JOBQUEUE, m_jobqueue);
	DDX_Check(pDX, IDC_VERBOSE, m_verbose);
	DDX_Check(pDX, IDC_NONEXPORT, m_nonexport);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProgopts, CDialog)
	//{{AFX_MSG_MAP(CProgopts)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProgopts message handlers

void CProgopts::OnOK()
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

const DWORD a118HelpIDs[]=
{
	IDC_JEDITOR,	IDH_118_278,	// Program options: "" (Edit)
	IDC_BINMODE,	IDH_118_279,	// Program options: "Binary job files" (Button)
	IDC_VERBOSE,	IDH_118_280,	// Program options: "Verbose (display result)" (Button)
	IDC_NONEXPORT,	IDH_118_281,	// Program options: "Created jobs NOT exported" (Button)
	IDC_JOBQUEUE,	IDH_118_282,	// Program options: "" (Edit)
	0, 0
};

BOOL CProgopts::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a118HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
