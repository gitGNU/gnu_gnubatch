// editass.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "listvar.h"
#include "btrw.h"
#include "editass.h"
#include <ctype.h>
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditass dialog

CEditass::CEditass(CWnd* pParent /*=NULL*/)
	: CDialog(CEditass::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditass)
	//}}AFX_DATA_INIT
}

void CEditass::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditass)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditass, CDialog)
	//{{AFX_MSG_MAP(CEditass)
	ON_BN_CLICKED(IDC_ASET, OnClickedAset)
	ON_BN_CLICKED(IDC_AADD, OnClickedAadd)
	ON_BN_CLICKED(IDC_ASUB, OnClickedAsub)
	ON_BN_CLICKED(IDC_AMULT, OnClickedAmult)
	ON_BN_CLICKED(IDC_ADIV, OnClickedAdiv)
	ON_BN_CLICKED(IDC_AMOD, OnClickedAmod)
	ON_BN_CLICKED(IDC_AEXIT, OnClickedAexit)
	ON_BN_CLICKED(IDC_ASIGNAL, OnClickedAsignal)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_VALUE, OnDeltaposScrValue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditass message handlers

BOOL CEditass::OnInitDialog()
{
	CDialog::OnInitDialog();
	listvar	vl(BTM_READ|BTM_WRITE);
	CComboBox	*lb = (CComboBox *) GetDlgItem(IDC_ASSVAR);
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
	if  (m_ass.bja_op != BJA_NONE)  {
		((CButton *)GetDlgItem(IDC_ASET+m_ass.bja_op-BJA_ASSIGN))->SetCheck(1);
		if  (m_ass.bja_iscrit)
			((CButton *)GetDlgItem(IDC_ACRIT))->SetCheck(1);
		else
			((CButton *)GetDlgItem(IDC_ANOTCRIT))->SetCheck(1);
#ifdef	BTRW
		lb->SelectString(-1, m_ass.bja_var);
#else
		Btvar	*vp = Vars()[m_ass.bja_varind];
		if  (vp)  {
			char	wstring[UIDSIZE+BTV_NAME+2];
			wsprintf(wstring, "%s:%s", (const char FAR *) look_host(vp->hostid), (const char FAR *) vp->var_name);
			lb->SelectString(-1, wstring);
		}
#endif
		char	lbuf[BTC_VALUE+3];
		if  (m_ass.bja_con.const_type == CON_LONG)
			wsprintf(lbuf, "%ld", m_ass.bja_con.con_long);
		else  if  (isdigit(m_ass.bja_con.con_string[0]))
			wsprintf(lbuf, "\"%s\"", (const char FAR *) m_ass.bja_con.con_string);
		else                         
			wsprintf(lbuf, "%s", (const char FAR *) m_ass.bja_con.con_string);  
		SetDlgItemText(IDC_VALUE, lbuf);
	}
	else  {
		((CButton *)GetDlgItem(IDC_ANOTCRIT))->SetCheck(1);
		m_ass.bja_flags = BJA_START|BJA_REVERSE|BJA_OK|BJA_ERROR|BJA_ABORT;
	}	
	Enablemarks();
	return TRUE;
}

void	CEditass::Enablemarks()
{
	if  (m_ass.bja_op >= BJA_SEXIT)  {
		for  (unsigned  cnt = IDC_ASTART;  cnt <= IDC_ACANCEL;  cnt++)  {
			CButton  *we = (CButton *) GetDlgItem(cnt);
			we->SetCheck(0);
			we->EnableWindow(FALSE);
		}
	}
	else  {
		for  (unsigned  cnt = IDC_ASTART;  cnt <= IDC_ACANCEL;  cnt++)  {
			CButton  *we = (CButton *) GetDlgItem(cnt);
			we->SetCheck(0);
			we->EnableWindow(TRUE);
		}
		if  (m_ass.bja_flags & BJA_START)
			((CButton *)GetDlgItem(IDC_ASTART))->SetCheck(1);
		if  (m_ass.bja_flags & BJA_REVERSE)
			((CButton *)GetDlgItem(IDC_AREVERSE))->SetCheck(1);
		if  (m_ass.bja_flags & BJA_OK)
			((CButton *)GetDlgItem(IDC_ANORMAL))->SetCheck(1);
		if  (m_ass.bja_flags & BJA_ERROR)
			((CButton *)GetDlgItem(IDC_AERROR))->SetCheck(1);
		if  (m_ass.bja_flags & BJA_ABORT)
			((CButton *)GetDlgItem(IDC_AABORT))->SetCheck(1);
		if  (m_ass.bja_flags & BJA_CANCEL)
			((CButton *)GetDlgItem(IDC_ACANCEL))->SetCheck(1);
	}
}
		
void CEditass::OnOK()
{
	if  (m_ass.bja_op < BJA_SEXIT)  {
		m_ass.bja_flags = 0;
		if  (((CButton *)GetDlgItem(IDC_ASTART))->GetCheck())
			m_ass.bja_flags |= BJA_START;
		if  (((CButton *)GetDlgItem(IDC_ANORMAL))->GetCheck())
			m_ass.bja_flags |= BJA_OK;
		if  (((CButton *)GetDlgItem(IDC_AERROR))->GetCheck())
			m_ass.bja_flags |= BJA_ERROR;
		if  (((CButton *)GetDlgItem(IDC_AABORT))->GetCheck())
			m_ass.bja_flags |= BJA_ABORT;
		if  (((CButton *)GetDlgItem(IDC_ACANCEL))->GetCheck())
			m_ass.bja_flags |= BJA_CANCEL;
		if  (m_ass.bja_flags == 0)  {
			AfxMessageBox(IDP_NOAFLAGSSET, MB_OK|MB_ICONEXCLAMATION);
			GetDlgItem(IDC_ASTART)->SetFocus();
			return;
		}
		if  (((CButton *)GetDlgItem(IDC_AREVERSE))->GetCheck())
			m_ass.bja_flags |= BJA_REVERSE;
	}		
	if  (m_ass.bja_op == BJA_NONE)  {
		AfxMessageBox(IDP_NOAVARSEL, MB_OK|MB_ICONEXCLAMATION);
		GetDlgItem(IDC_ASET)->SetFocus();
		return;
	}
	if  (((CButton *)GetDlgItem(IDC_ACRIT))->GetCheck())
		m_ass.bja_iscrit = 1;
	else
		m_ass.bja_iscrit = 0;
	CComboBox	*lb = (CComboBox *) GetDlgItem(IDC_ASSVAR);
	int	 which = lb->GetCurSel();
	if  (which < 0)  {
		AfxMessageBox(IDP_NOAVARSEL, MB_OK|MB_ICONEXCLAMATION);
		lb->SetFocus();
		return;
	}
#ifdef	BTRW
	lb->GetLBText(which, m_ass.bja_var);
	if  (m_ass.bja_var.Find(':') < 0)  {
		AfxMessageBox(IDP_NOAVARSEL, MB_OK|MB_ICONEXCLAMATION);
		lb->SetFocus();
		return;
	}
#else
	m_ass.bja_varind = unsigned(lb->GetItemData(which));
#endif
	char	inb[BTC_VALUE+3];
	int	 sizeb;
	if  ((sizeb = GetDlgItemText(IDC_VALUE, inb, sizeof(inb))) <= 0)  {
		CEdit	*av = (CEdit *)GetDlgItem(IDC_VALUE); 
		AfxMessageBox(IDP_NOAVALUE, MB_OK|MB_ICONEXCLAMATION);
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
		m_ass.bja_con.const_type = CON_STRING;
		strcpy(m_ass.bja_con.con_string, bp);
	}
	else  {
		m_ass.bja_con.const_type = CON_LONG;
		m_ass.bja_con.con_long = atol(inb);
	}		
	
	CDialog::OnOK();
}

void CEditass::OnClickedAset()
{
	m_ass.bja_op = BJA_ASSIGN;
	Enablemarks();
}

void CEditass::OnClickedAadd()
{
	m_ass.bja_op = BJA_INCR;
	Enablemarks();
}

void CEditass::OnClickedAsub()
{
	m_ass.bja_op = BJA_DECR;
	Enablemarks();
}

void CEditass::OnClickedAmult()
{
	m_ass.bja_op = BJA_MULT;
	Enablemarks();
}

void CEditass::OnClickedAdiv()
{
	m_ass.bja_op = BJA_DIV;
	Enablemarks();
}

void CEditass::OnClickedAmod()
{
	m_ass.bja_op = BJA_MOD;
	Enablemarks();
}

void CEditass::OnClickedAexit()
{
	m_ass.bja_op = BJA_SEXIT;
	Enablemarks();
}

void CEditass::OnClickedAsignal()
{
	m_ass.bja_op = BJA_SSIG;
	Enablemarks();
}

void CEditass::OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult) 
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
	IDC_AEXIT,	IDH_106_162,	// Edit Assignment: "Exit" (Button)
	IDC_ASIGNAL,	IDH_106_163,	// Edit Assignment: "Sig" (Button)
	IDC_ASTART,	IDH_106_164,	// Edit Assignment: "On start" (Button)
	IDC_AREVERSE,	IDH_106_165,	// Edit Assignment: "REVERSE on exit" (Button)
	IDC_ANORMAL,	IDH_106_166,	// Edit Assignment: "Normal exit" (Button)
	IDC_AERROR,	IDH_106_167,	// Edit Assignment: "Error exit" (Button)
	IDC_AABORT,	IDH_106_168,	// Edit Assignment: "Abort" (Button)
	IDC_ACANCEL,	IDH_106_169,	// Edit Assignment: "Cancel" (Button)
	IDC_ANOTCRIT,	IDH_106_170,	// Edit Assignment: "Ignore" (Button)
	IDC_ACRIT,	IDH_106_171,	// Edit Assignment: "Hold job" (Button)
	IDC_VALUE,	IDH_106_154,	// Edit Assignment: "" (Edit)
	IDC_SCR_VALUE,	IDH_106_154,	// Edit Assignment: "Spin1" (msctls_updown32)
	IDC_ASET,	IDH_106_156,	// Edit Assignment: "Set" (Button)
	IDC_AADD,	IDH_106_157,	// Edit Assignment: "+" (Button)
	IDC_ASSVAR,	IDH_106_181,	// Edit Assignment: "" (ComboBox)
	IDC_ASUB,	IDH_106_158,	// Edit Assignment: "-" (Button)
	IDC_AMULT,	IDH_106_159,	// Edit Assignment: "X" (Button)
	IDC_ADIV,	IDH_106_160,	// Edit Assignment: "/" (Button)
	IDC_AMOD,	IDH_106_161,	// Edit Assignment: "Md" (Button)
	0, 0
};

BOOL CEditass::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a106HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
