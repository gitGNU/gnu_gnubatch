// assvar.cpp : implementation file
//

#include "stdafx.h"
#include "jobdoc.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "assvar.h"
#include <stdlib.h>
#include <ctype.h>
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAssvar dialog

CAssvar::CAssvar(CWnd* pParent /*=NULL*/)
	: CDialog(CAssvar::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAssvar)
	m_varname = "";
	m_varcomment = "";
	//}}AFX_DATA_INIT
}

void CAssvar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAssvar)
	DDX_Text(pDX, IDC_DVARNAME, m_varname);
	DDX_Text(pDX, IDC_DVARCOMMENT, m_varcomment);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAssvar, CDialog)
	//{{AFX_MSG_MAP(CAssvar)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_AVARVALUE, OnDeltaposScrAvarvalue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAssvar message handlers

BOOL CAssvar::OnInitDialog()
{
	CDialog::OnInitDialog();
	char	lbuf[BTC_VALUE+3];
	if  (m_value.const_type == CON_LONG)
		wsprintf(lbuf, "%ld", m_value.con_long);
	else  if  (isdigit(m_value.con_string[0]))
		wsprintf(lbuf, "\"%s\"", (const char FAR *) m_value.con_string);
	else
		wsprintf(lbuf, "%s", (const char FAR *) m_value.con_string);  
	SetDlgItemText(IDC_AVARVALUE, lbuf);
	return TRUE;
}

void CAssvar::OnOK()
{
	char	inb[BTC_VALUE+3];
	int	 sizeb;
	if  ((sizeb = GetDlgItemText(IDC_AVARVALUE, inb, sizeof(inb))) <= 0)  {
		CEdit	*av = (CEdit *) GetDlgItem(IDC_AVARVALUE); 
		AfxMessageBox(IDP_NOVVALUE, MB_OK|MB_ICONEXCLAMATION);
		av->SetSel(0, -1);
		av->SetFocus();
		return;
	}
	if  (!(inb[0] == '-' || isdigit(inb[0])))  {
		char	*bp = inb;
		if  (*bp == '\"')  {
			if  (inb[sizeb-1] == '\"')
				inb[sizeb-1] = '\0';
		  	bp++;
		}
		m_value.const_type = CON_STRING;
		strcpy(m_value.con_string, bp);
	}
	else  {
		m_value.const_type = CON_LONG;
		m_value.con_long = atol(inb);
	}		
	CDialog::OnOK();
}

void CAssvar::OnDeltaposScrAvarvalue(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int  sizeb, cnt;
	long	existing;
	char	intext[255];
	if  ((sizeb = GetDlgItemText(IDC_AVARVALUE, intext, sizeof(intext))) <= 0)
		goto  beep;
	for  (cnt = 0;  cnt < sizeb;  cnt++)
		if  (!isdigit(intext[cnt]) && (cnt > 0 || intext[cnt] != '-'))
			goto  beep;
	existing = atol(intext) + pNMUpDown->iDelta;
	wsprintf(intext, "%ld", existing);
	SetDlgItemText(IDC_AVARVALUE, intext);
	*pResult = 0;
	return;
beep:
	MessageBeep(MB_ICONASTERISK);
	*pResult = 0;
}

const DWORD a134HelpIDs[]=
{
	IDC_AVARVALUE,	IDH_134_415,	// Assign Variable: "" (Edit)
	IDC_SCR_AVARVALUE,	IDH_134_415,	// Assign Variable: "Spin1" (msctls_updown32)
	IDC_DVARNAME,	IDH_134_413,	// Assign Variable: "" (Edit)
	IDC_DVARCOMMENT,	IDH_134_414,	// Assign Variable: "" (Edit)
	0, 0
};

BOOL CAssvar::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a134HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
