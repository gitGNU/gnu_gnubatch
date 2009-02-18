// defcond.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "defcond.h"
#include <ctype.h>
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Cdefcond dialog

Cdefcond::Cdefcond(CWnd* pParent /*=NULL*/)
	: CDialog(Cdefcond::IDD, pParent)
{
	//{{AFX_DATA_INIT(Cdefcond)
	m_constval = "";
	m_condop = -1;
	m_condcrit = -1;
	//}}AFX_DATA_INIT
}

void Cdefcond::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Cdefcond)
	DDX_Text(pDX, IDC_VALUE, m_constval);
	DDX_Radio(pDX, IDC_CEQ, m_condop);
	DDX_Radio(pDX, IDC_CNOTCRIT, m_condcrit);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Cdefcond, CDialog)
	//{{AFX_MSG_MAP(Cdefcond)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_VALUE, OnDeltaposScrValue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Cdefcond message handlers

void Cdefcond::OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int  sizeb, cnt;
	long	existing;
	char	intext[255];
	if  ((sizeb = GetDlgItemText(IDC_VALUE, intext, sizeof(intext))) <= 0)
		goto  ret;
	for  (cnt = 0;  cnt < sizeb;  cnt++)
		if  (!isdigit(intext[cnt]) && (cnt > 0 || intext[cnt] != '-'))	{
			MessageBeep(MB_ICONASTERISK);
			goto  ret;
		}
	existing = atol(intext) + pNMUpDown->iDelta;
	wsprintf(intext, "%ld", existing);
	SetDlgItemText(IDC_VALUE, intext);
ret:
	*pResult = 0;
}

const DWORD a106HelpIDs[]=
{
	IDC_CNOTCRIT,	IDH_106_185,	// Condition defaults: "Ignore" (Button)
	IDC_CCRIT,	IDH_106_185,	// Condition defaults: "Hold job" (Button)
	IDC_CEQ,	IDH_106_176,	// Condition defaults: "=" (Button)
	IDC_CNE,	IDH_106_176,	// Condition defaults: "!=" (Button)
	IDC_CLT,	IDH_106_176,	// Condition defaults: "<" (Button)
	IDC_CLE,	IDH_106_176,	// Condition defaults: "<=" (Button)
	IDC_CGT,	IDH_106_176,	// Condition defaults: ">" (Button)
	IDC_CGE,	IDH_106_176,	// Condition defaults: ">=" (Button)
	IDC_VALUE,	IDH_106_160,	// Condition defaults: "" (Edit)
	IDC_SCR_VALUE,	IDH_106_160,	// Condition defaults: "Spin1" (msctls_updown32)
	0, 0
};

BOOL Cdefcond::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a106HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
