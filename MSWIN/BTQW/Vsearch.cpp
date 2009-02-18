// vsearch.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "vsearch.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVSearch dialog

CVSearch::CVSearch(CWnd* pParent /*=NULL*/)
	: CDialog(CVSearch::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVSearch)
	m_searchstr = "";
	m_sforward = -1;
	m_suser = FALSE;
	m_svname = FALSE;
	m_svcomment = FALSE;
	m_svvalue = FALSE;
	m_wraparound = FALSE;
	//}}AFX_DATA_INIT
}

void CVSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVSearch)
	DDX_Text(pDX, IDC_LOOKFOR, m_searchstr);
	DDX_Radio(pDX, IDC_SEARCH_FORWARD, m_sforward);
	DDX_Check(pDX, IDC_SRCH_USER, m_suser);
	DDX_Check(pDX, IDC_SRCH_VNAME, m_svname);
	DDX_Check(pDX, IDC_VCOMMENT, m_svcomment);
	DDX_Check(pDX, IDC_VVALUE, m_svvalue);
	DDX_Check(pDX, IDC_WRAPROUND, m_wraparound);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVSearch, CDialog)
	//{{AFX_MSG_MAP(CVSearch)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVSearch message handlers

const DWORD a137HelpIDs[]=
{
	IDC_LOOKFOR,	IDH_137_324,	// Search for variable: "" (Edit)
	IDC_SEARCH_FORWARD,	IDH_137_325,	// Search for variable: "Forward" (Button)
	IDC_SEARCH_BACK,	IDH_137_326,	// Search for variable: "Backward" (Button)
	IDC_WRAPROUND,	IDH_137_327,	// Search for variable: "Wrap Around" (Button)
	IDC_SRCH_USER,	IDH_137_329,	// Search for variable: "User" (Button)
	IDC_SRCH_VNAME,	IDH_137_448,	// Search for variable: "Variable name" (Button)
	IDC_VVALUE,	IDH_137_449,	// Search for variable: "Value" (Button)
	IDC_VCOMMENT,	IDH_137_450,	// Search for variable: "Comment" (Button)
	0, 0
};

BOOL CVSearch::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a137HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
