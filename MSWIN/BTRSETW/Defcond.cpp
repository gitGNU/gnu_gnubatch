// defcond.cpp : implementation file
//

#include "stdafx.h"
#include "xbwnetwk.h"
#include "btrsetw.h"
#include "defcond.h"
#include <ctype.h>
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefcond dialog

CDefcond::CDefcond(CWnd* pParent /*=NULL*/)
	: CDialog(CDefcond::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDefcond)
	m_condop = -1;
	m_condcrit = -1;
	m_constval = "";
	//}}AFX_DATA_INIT
}

void CDefcond::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefcond)
	DDX_Radio(pDX, IDC_CEQ, m_condop);
	DDX_Radio(pDX, IDC_CNOTCRIT, m_condcrit);
	DDX_Text(pDX, IDC_VALUE, m_constval);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDefcond, CDialog)
	//{{AFX_MSG_MAP(CDefcond)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_VALUE, OnDeltaposScrValue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefcond message handlers

void CDefcond::OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult) 
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
	existing = atol(intext);
	if  (pNMUpDown->iDelta >= 0)
		existing++;
	else
		existing--;
	wsprintf(intext, "%ld", existing);
	SetDlgItemText(IDC_VALUE, intext);
ret:
	*pResult = 0;
}

const DWORD a102HelpIDs[]=
{
	IDC_CEQ,	IDH_102_168,	// Condition defaults: "=" (Button)
	IDC_CNE,	IDH_102_168,	// Condition defaults: "!=" (Button)
	IDC_CLT,	IDH_102_168,	// Condition defaults: "<" (Button)
	IDC_CLE,	IDH_102_168,	// Condition defaults: "<=" (Button)
	IDC_CGT,	IDH_102_168,	// Condition defaults: ">" (Button)
	IDC_CGE,	IDH_102_168,	// Condition defaults: ">=" (Button)
	IDC_VALUE,	IDH_102_150,	// Condition defaults: "" (Edit)
	IDC_CNOTCRIT,	IDH_102_174,	// Condition defaults: "Ignore" (Button)
	IDC_SCR_VALUE,	IDH_102_150,	// Condition defaults: "Spin1" (msctls_updown32)
	IDC_CCRIT,	IDH_102_175,	// Condition defaults: "Hold job" (Button)
	0, 0
};

BOOL CDefcond::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a102HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
