// editcond.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "listvar.h"
#include "btrw.h"
#include "editcond.h"
#include <ctype.h>
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditcond dialog

CEditcond::CEditcond(CWnd* pParent /*=NULL*/)
	: CDialog(CEditcond::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditcond)
	//}}AFX_DATA_INIT
}

void CEditcond::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditcond)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditcond, CDialog)
	//{{AFX_MSG_MAP(CEditcond)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_VALUE, OnDeltaposScrValue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditcond message handlers

BOOL CEditcond::OnInitDialog()
{
	CDialog::OnInitDialog();
	listvar	vl(BTM_READ);
	CComboBox	*lb = (CComboBox *) GetDlgItem(IDC_CONDVAR);
#ifdef	BTQW
	int	 rv;
	Btvar	*vp;
	while  ((rv = vl.next()) >= 0)  {
		vp = Vars()[rv];
		char	wstring[UIDSIZE+BTV_NAME+2];
		wsprintf(wstring, "%s:%s", (const char FAR *) look_host(vp->hostid), (const char FAR *) vp->var_name);
		int  indx = lb->AddString(wstring);
		lb->SetItemData(indx, DWORD(rv));
	}	
#endif
#ifdef	BTRW
	char	*namev;
	int		rv = 0;
	while  (namev = vl.next())  {
		int  indx = lb->AddString(namev);
		lb->SetItemData(indx, DWORD(rv));
		rv++;
	}
#endif
	if  (m_cond.bjc_compar != C_UNUSED)  {
		((CButton *)GetDlgItem(IDC_CEQ+m_cond.bjc_compar-C_EQ))->SetCheck(1);
		if  (m_cond.bjc_iscrit)
			((CButton *)GetDlgItem(IDC_CCRIT))->SetCheck(1);
		else
			((CButton *)GetDlgItem(IDC_CNOTCRIT))->SetCheck(1);
#ifdef	BTRW
		lb->SelectString(-1, m_cond.bjc_var);
#else
		Btvar	*vp = Vars()[m_cond.bjc_varind];
		if  (vp)  {
			char	wstring[UIDSIZE+BTV_NAME+2];
			wsprintf(wstring, "%s:%s", (const char FAR *) look_host(vp->hostid), (const char FAR *) vp->var_name);
			lb->SelectString(-1, wstring);
		}
#endif
		char	lbuf[BTC_VALUE+3];
		if  (m_cond.bjc_value.const_type == CON_LONG)
			wsprintf(lbuf, "%ld", m_cond.bjc_value.con_long);
		else  if  (isdigit(m_cond.bjc_value.con_string[0]))
			wsprintf(lbuf, "\"%s\"", (const char FAR *) m_cond.bjc_value.con_string);
		else
			wsprintf(lbuf, "%s", (const char FAR *) m_cond.bjc_value.con_string);  
		SetDlgItemText(IDC_VALUE, lbuf);
	}
	else
		((CButton *)GetDlgItem(IDC_CNOTCRIT))->SetCheck(1);
	return TRUE;
}

void CEditcond::OnOK()
{                                                    
	m_cond.bjc_compar = C_UNUSED;
	for  (unsigned  cnt = IDC_CEQ;  cnt <= IDC_CGE;  cnt++)
		if  (((CButton *)GetDlgItem(cnt))->GetCheck())  {
			m_cond.bjc_compar = cnt - IDC_CEQ + C_EQ;    
			break;
		}	
	if  (m_cond.bjc_compar == C_UNUSED)  {
		AfxMessageBox(IDP_NOCVARSEL, MB_OK|MB_ICONEXCLAMATION);
		GetDlgItem(IDC_CEQ)->SetFocus();
		return;
	}
	if  (((CButton *)GetDlgItem(IDC_CCRIT))->GetCheck())
		m_cond.bjc_iscrit = 1;
	else
		m_cond.bjc_iscrit = 0;
	CComboBox	*lb = (CComboBox *) GetDlgItem(IDC_CONDVAR);
	int	 which = lb->GetCurSel();
	if  (which < 0)  {
		AfxMessageBox(IDP_NOCVARSEL, MB_OK|MB_ICONEXCLAMATION);
		lb->SetFocus();
		return;
	}
#ifdef	BTRW
	lb->GetLBText(which, m_cond.bjc_var);
	if  (m_cond.bjc_var.Find(':') < 0)  {
		AfxMessageBox(IDP_NOAVARSEL, MB_OK|MB_ICONEXCLAMATION);
		lb->SetFocus();
		return;
	}
#else
	m_cond.bjc_varind = unsigned(lb->GetItemData(which));
#endif
	char	inb[BTC_VALUE+3];
	int	 sizeb;
	if  ((sizeb = GetDlgItemText(IDC_VALUE, inb, sizeof(inb))) <= 0)  {
		CEdit	*av = (CEdit *)GetDlgItem(IDC_VALUE); 
		AfxMessageBox(IDP_NOCVALUE, MB_OK|MB_ICONEXCLAMATION);
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
		m_cond.bjc_value.const_type = CON_STRING;
		strcpy(m_cond.bjc_value.con_string, bp);
	}
	else  {
		m_cond.bjc_value.const_type = CON_LONG;
		m_cond.bjc_value.con_long = atol(inb);
	}		
	CDialog::OnOK();
}

void CEditcond::OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult) 
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

const DWORD a107HelpIDs[]=
{
	IDC_CEQ,	IDH_107_172,	// Edit condition: "=" (Button)
	IDC_CNE,	IDH_107_172,	// Edit condition: "!=" (Button)
	IDC_CLT,	IDH_107_172,	// Edit condition: "<" (Button)
	IDC_CLE,	IDH_107_172,	// Edit condition: "<=" (Button)
	IDC_CGT,	IDH_107_172,	// Edit condition: ">" (Button)
	IDC_CGE,	IDH_107_172,	// Edit condition: ">=" (Button)
	IDC_VALUE,	IDH_107_154,	// Edit condition: "" (Edit)
	IDC_CNOTCRIT,	IDH_107_178,	// Edit condition: "Ignore" (Button)
	IDC_SCR_VALUE,	IDH_107_154,	// Edit condition: "Spin1" (msctls_updown32)
	IDC_CCRIT,	IDH_107_179,	// Edit condition: "Hold job" (Button)
	IDC_CONDVAR,	IDH_107_182,	// Edit condition: "" (ComboBox)
	0, 0
};

BOOL CEditcond::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a107HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
