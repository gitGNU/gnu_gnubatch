// constval.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "constval.h"
#include <stdlib.h>
#include <ctype.h>
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Cconstval dialog

Cconstval::Cconstval(CWnd* pParent /*=NULL*/)
	: CDialog(Cconstval::IDD, pParent)
{
	//{{AFX_DATA_INIT(Cconstval)
	m_constval = 0;
	//}}AFX_DATA_INIT
}

void Cconstval::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Cconstval)
	DDX_Text(pDX, IDC_CONSTVALUE, m_constval);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Cconstval, CDialog)
	//{{AFX_MSG_MAP(Cconstval)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_CONSTVALUE, OnDeltaposScrConstvalue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Cconstval message handlers

void Cconstval::OnDeltaposScrConstvalue(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int  sizeb, cnt;
	long  existing;
	char	intext[255];
	if  ((sizeb = GetDlgItemText(IDC_CONSTVALUE, intext, sizeof(intext))) <= 0)
		goto  beep;
	for  (cnt = 0;  cnt < sizeb;  cnt++)
		if  (!isdigit(intext[cnt]) && (cnt > 0 || intext[cnt] != '-'))
			goto  beep;
	existing = atol(intext) + pNMUpDown->iDelta;
	wsprintf(intext, "%ld", existing);
	SetDlgItemText(IDC_CONSTVALUE, intext);
	*pResult = 0;
	return;
beep:
	MessageBeep(MB_ICONASTERISK);
	*pResult = 0;
}

const DWORD a103HelpIDs[]=
{
	IDC_CONSTVALUE,	IDH_103_154,	// Constant for Arithmetic ops: "" (Edit)
	IDC_SCR_CONSTVALUE,	IDH_103_154,	// Constant for Arithmetic ops: "Spin1" (msctls_updown32)
	0, 0
};

BOOL Cconstval::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a103HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
