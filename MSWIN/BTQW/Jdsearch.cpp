// jdsearch.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "netmsg.h"
#include "btqw.h"
#include "jdsearch.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJDSearch dialog

CJDSearch::CJDSearch(CWnd* pParent /*=NULL*/)
	: CDialog(CJDSearch::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJDSearch)
	m_ignorecase = FALSE;
	m_lookfor = "";
	m_wrapround = FALSE;
	m_sforward = -1;
	//}}AFX_DATA_INIT
}

void CJDSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJDSearch)
	DDX_Check(pDX, IDC_IGNORECASE, m_ignorecase);
	DDX_Text(pDX, IDC_LOOKFOR, m_lookfor);
	DDX_Check(pDX, IDC_WRAPROUND, m_wrapround);
	DDX_Radio(pDX, IDC_SEARCH_FORWARD, m_sforward);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJDSearch, CDialog)
	//{{AFX_MSG_MAP(CJDSearch)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJDSearch message handlers

const DWORD a128HelpIDs[]=
{
	IDC_LOOKFOR,	IDH_128_324,	// Search job data: "" (Edit)
	IDC_SEARCH_FORWARD,	IDH_128_325,	// Search job data: "Forward" (Button)
	IDC_SEARCH_BACK,	IDH_128_326,	// Search job data: "Backward" (Button)
	IDC_WRAPROUND,	IDH_128_327,	// Search job data: "Wrap Around" (Button)
	IDC_IGNORECASE,	IDH_128_328,	// Search job data: "Ignore Case" (Button)
	0, 0
};

BOOL CJDSearch::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a128HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
