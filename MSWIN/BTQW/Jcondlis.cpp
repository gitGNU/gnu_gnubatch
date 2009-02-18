// jcondlis.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "btqw.h"
#include "jcondlis.h"
#include "editcond.h"
#include "defcond.h"
#include <ctype.h>
#include <stdlib.h>
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJcondlist dialog

CJcondlist::CJcondlist(CWnd* pParent /*=NULL*/)
	: CDialog(CJcondlist::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJcondlist)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_changes = 0;
}

void CJcondlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJcondlist)
	DDX_Control(pDX, IDC_CONDLIST, m_dragcond);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJcondlist, CDialog)
	//{{AFX_MSG_MAP(CJcondlist)
	ON_BN_CLICKED(IDC_NEWCOND, OnClickedNewcond)
	ON_BN_CLICKED(IDC_EDITCOND, OnClickedEditcond)
	ON_BN_CLICKED(IDC_CONDDEL, OnClickedConddel)
	ON_LBN_DBLCLK(IDC_CONDLIST, OnDblclkCondlist)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJcondlist message handlers

static	void	GenCondFmt(const Jcond &copy, CString &res)
{
	char	texts[100];
	CString	opn;
	opn.LoadString(IDS_CONDOP + copy.bjc_compar);
#ifdef	BTRW
	if  (copy.bjc_value.const_type == CON_STRING)
		wsprintf(texts, "%s\t%s\t%s", (const char FAR *) copy.bjc_var,
			(const char FAR *) opn, (const char FAR *) copy.bjc_value.con_string);
	else
		wsprintf(texts, "%s\t%s\t%ld", (const char FAR *) copy.bjc_var,
			(const char FAR *) opn, copy.bjc_value.con_long);					
#else
	Btvar	&vp = *Vars()[copy.bjc_varind];
	                          
	if  (copy.bjc_value.const_type == CON_STRING)
		wsprintf(texts, "%s:%s\t%s\t%s", (const char FAR *) look_host(vp.hostid), (const char FAR *) vp.var_name,
			(const char FAR *) opn, (const char FAR *) copy.bjc_value.con_string);
	else
		wsprintf(texts, "%s:%s\t%s\t%ld", (const char FAR *) look_host(vp.hostid), (const char FAR *) vp.var_name,
			(const char FAR *) opn, copy.bjc_value.con_long);					
#endif
	res = texts;
}		

BOOL CJcondlist::OnInitDialog()
{
	CDialog::OnInitDialog();
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_CONDLIST);
	lb->SetTabStops();
	for  (unsigned  cnt = 0;  cnt < m_nconds;  cnt++)  {
		CString	rfmt;
		GenCondFmt(m_jconds[cnt], rfmt);
		lb->SetItemData(lb->InsertString(-1, rfmt), DWORD(cnt));
	}
	if  (!m_writeable)  {
		GetDlgItem(IDC_NEWCOND)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITCOND)->EnableWindow(FALSE);
		GetDlgItem(IDC_CONDDEL)->EnableWindow(FALSE);
	}			
	return TRUE;
}

void CJcondlist::OnClickedNewcond()
{
	if  (m_nconds >= MAXCVARS)  {
		AfxMessageBox(IDP_TOOMANYCONDS, MB_OK|MB_ICONSTOP);
		return;
	}	
	CEditcond	dlg;
#ifdef	BTQW
	conddefs	cd = ((CBtqwApp *)AfxGetApp())->m_conddefs;
	dlg.m_cond.bjc_compar = cd.m_condop + C_EQ;
	dlg.m_cond.bjc_iscrit = cd.m_condcrit;
	if  (!cd.m_constval.IsEmpty() && (isdigit(cd.m_constval[0]) || cd.m_constval[0] == '-'))  {
		dlg.m_cond.bjc_value.const_type = CON_LONG;
		dlg.m_cond.bjc_value.con_long = atol((const char *) cd.m_constval);
	}
	else  {
		dlg.m_cond.bjc_value.const_type = CON_STRING;
		strncpy(dlg.m_cond.bjc_value.con_string, cd.m_constval, BTC_VALUE);
		dlg.m_cond.bjc_value.con_string[BTC_VALUE] = '\0';
	}	
#endif
#ifdef	BTRW
	dlg.m_cond = ((CBtrwApp *)AfxGetApp())->m_defcond;
#endif

	if  (dlg.DoModal() != IDOK)
		return;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_CONDLIST);
	int	 where = lb->GetCurSel();
	CString	rfmt;
	GenCondFmt(dlg.m_cond, rfmt);

	where = lb->InsertString(where, rfmt);
	m_jconds[m_nconds] = dlg.m_cond;
	lb->SetItemData(where, DWORD(m_nconds));
	m_nconds++;
	m_changes++;
}

void CJcondlist::OnClickedEditcond()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_CONDLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;
	unsigned  rowpos = lb->GetItemData(where);
	CEditcond	dlg;
	dlg.m_cond = m_jconds[rowpos];
	if  (dlg.DoModal() == IDOK)  {
		m_jconds[rowpos] = dlg.m_cond;
		lb->DeleteString(where);
		CString	rfmt;
		GenCondFmt(dlg.m_cond, rfmt);
		lb->SetItemData(lb->InsertString(where, rfmt), DWORD(rowpos));
		m_changes++;
	}	
}

void CJcondlist::OnClickedConddel()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_CONDLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;

	unsigned  rowpos = lb->GetItemData(where);
	lb->DeleteString(where);

	for  (unsigned  cnt = rowpos + 1;  cnt < m_nconds;  cnt++)
		m_jconds[cnt-1] = m_jconds[cnt];
	m_nconds--;

	for  (int bcnt = lb->GetCount()-1;  bcnt >= 0;  bcnt--)  {
		unsigned  rp = lb->GetItemData(bcnt);
		if  (rp >= rowpos)
			lb->SetItemData(bcnt, DWORD(rp-1));
	}
	m_changes++;
}

void CJcondlist::OnDblclkCondlist()
{
	if  (!m_writeable)
		return;
	if  (m_nconds != 0)
		OnClickedEditcond();
	else
		OnClickedNewcond();
}

void CJcondlist::OnOK() 
{
	if  (m_writeable)  {
		if  (m_nconds != 0)  {
			Jcond	news[MAXCVARS];
			CListBox  *lb = (CListBox *) GetDlgItem(IDC_CONDLIST);
			for  (int cnt = 0; cnt < m_nconds;  cnt++)  {
				int  rp = lb->GetItemData(cnt);
				news[cnt] = m_jconds[rp];
				if  (rp != cnt)
					m_changes++;
			}
			for  (cnt = 0;  cnt < m_nconds;  cnt++)
				m_jconds[cnt] = news[cnt];
		}
	}
	else
		m_changes = 0;
	CDialog::OnOK();
}

const DWORD a118HelpIDs[]=
{
	IDC_CONDLIST,	IDH_118_227,	// Job conditions: "" (ListBox)
	IDC_NEWCOND,	IDH_118_228,	// Job conditions: "New" (Button)
	IDC_EDITCOND,	IDH_118_229,	// Job conditions: "Edit" (Button)
	IDC_CONDDEL,	IDH_118_230,	// Job conditions: "Delete" (Button)
	0, 0
};

BOOL CJcondlist::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a118HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
