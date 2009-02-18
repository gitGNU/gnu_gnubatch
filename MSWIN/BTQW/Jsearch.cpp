// jsearch.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "jsearch.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJsearch dialog

CJsearch::CJsearch(CWnd* pParent /*=NULL*/)
	: CDialog(CJsearch::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJsearch)
	m_searchstring = "";
	m_sforward = -1;
	m_stitle = FALSE;
	m_suser = FALSE;
	m_swraparound = FALSE;
	//}}AFX_DATA_INIT
}

void CJsearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJsearch)
	DDX_Text(pDX, IDC_LOOKFOR, m_searchstring);
	DDX_Radio(pDX, IDC_SEARCH_FORWARD, m_sforward);
	DDX_Check(pDX, IDC_SRCH_JTITLE, m_stitle);
	DDX_Check(pDX, IDC_SRCH_USER, m_suser);
	DDX_Check(pDX, IDC_WRAPROUND, m_swraparound);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJsearch, CDialog)
	//{{AFX_MSG_MAP(CJsearch)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJsearch message handlers

const DWORD a129HelpIDs[]=
{
	IDC_LOOKFOR,	IDH_129_324,	// Search for job strings: "" (Edit)
	IDC_SEARCH_FORWARD,	IDH_129_325,	// Search for job strings: "Forward" (Button)
	IDC_SEARCH_BACK,	IDH_129_326,	// Search for job strings: "Backward" (Button)
	IDC_WRAPROUND,	IDH_129_327,	// Search for job strings: "Wrap Around" (Button)
	IDC_SRCH_USER,	IDH_129_329,	// Search for job strings: "User" (Button)
	IDC_SRCH_JTITLE,	IDH_129_330,	// Search for job strings: "Job title" (Button)
	0, 0
};

BOOL CJsearch::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a129HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
