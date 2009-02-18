// jasslist.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "btqw.h"
#include "defass.h"
#include "jasslist.h"
#include "editass.h"
#include <ctype.h>
#include <stdlib.h>
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJasslist dialog

CJasslist::CJasslist(CWnd* pParent /*=NULL*/)
	: CDialog(CJasslist::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJasslist)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_changes = 0;
}

void CJasslist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJasslist)
	DDX_Control(pDX, IDC_ASSLIST, m_dragass);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJasslist, CDialog)
	//{{AFX_MSG_MAP(CJasslist)
	ON_BN_CLICKED(IDC_NEWASS, OnClickedNewass)
	ON_BN_CLICKED(IDC_EDITASS, OnClickedEditass)
	ON_BN_CLICKED(IDC_ASSDEL, OnClickedAssdel)
	ON_LBN_DBLCLK(IDC_ASSLIST, OnDblclkAsslist)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJasslist message handlers

static	void	GenAssFmt(const Jass &copy, CString &res)
{
	char	texts[100];

	CString	opn;
	opn.LoadString(IDS_ASSOP + copy.bja_op);
#ifdef	BTRW
	if  (copy.bja_op >= BJA_SEXIT)                          
		wsprintf(texts, "%s\t%s", (const char FAR *) copy.bja_var, (const char FAR *) opn);
	else  if  (copy.bja_con.const_type == CON_STRING)
		wsprintf(texts, "%s\t%s\t%s", (const char FAR *) copy.bja_var,
			(const char FAR *) opn, (const char FAR *) copy.bja_con.con_string);
	else
		wsprintf(texts, "%s\t%s\t%ld", (const char FAR *) copy.bja_var,
			(const char FAR *) opn, copy.bja_con.con_long);
#else
	Btvar	&vp = *Vars()[copy.bja_varind];
	
	if  (copy.bja_op >= BJA_SEXIT)                          
		wsprintf(texts, "%s:%s\t%s", (const char FAR *) look_host(vp.hostid), (const char FAR *) vp.var_name, (const char FAR *) opn);
	else  if  (copy.bja_con.const_type == CON_STRING)
		wsprintf(texts, "%s:%s\t%s\t%s", (const char FAR *) look_host(vp.hostid), (const char FAR *) vp.var_name,
			(const char FAR *) opn, (const char FAR *) copy.bja_con.con_string);
	else
		wsprintf(texts, "%s:%s\t%s\t%ld", (const char FAR *) look_host(vp.hostid), (const char FAR *) vp.var_name,
			(const char FAR *) opn, copy.bja_con.con_long);
#endif
	res = texts;
}		

BOOL CJasslist::OnInitDialog()
{
	CDialog::OnInitDialog();
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ASSLIST);
	lb->SetTabStops();
	for  (unsigned  cnt = 0;  cnt < m_nasses;  cnt++)  {
		CString	rfmt;
		GenAssFmt(m_jasses[cnt], rfmt);
		lb->SetItemData(lb->InsertString(-1, rfmt), DWORD(cnt));
	}
	if  (!m_writeable)  {
		GetDlgItem(IDC_NEWASS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITASS)->EnableWindow(FALSE);
		GetDlgItem(IDC_ASSDEL)->EnableWindow(FALSE);
	}			
	return TRUE;
}

void CJasslist::OnClickedNewass()
{
	if  (m_nasses >= MAXSEVARS)  {
		AfxMessageBox(IDP_TOOMANYASSES, MB_OK|MB_ICONSTOP);
		return;
	}	
	CEditass	dlg;
#ifdef	BTQW
	assdefs	cd = ((CBtqwApp *)AfxGetApp())->m_assdefs;
	dlg.m_ass.bja_op = cd.m_asstype + BJA_ASSIGN;
	dlg.m_ass.bja_iscrit = cd.m_asscrit;
	dlg.m_ass.bja_flags = 0;
	if  (cd.m_astart)  dlg.m_ass.bja_flags |= BJA_START;
	if  (cd.m_areverse)  dlg.m_ass.bja_flags |= BJA_REVERSE;
	if  (cd.m_anormal)  dlg.m_ass.bja_flags |= BJA_OK;
	if  (cd.m_aerror)  dlg.m_ass.bja_flags |= BJA_ERROR;
	if  (cd.m_aabort)  dlg.m_ass.bja_flags |= BJA_ABORT;
	if  (cd.m_acancel)  dlg.m_ass.bja_flags |= BJA_CANCEL;
	if  (!cd.m_assvalue.IsEmpty() && (isdigit(cd.m_assvalue[0]) || cd.m_assvalue[0] == '-'))  {
		dlg.m_ass.bja_con.const_type = CON_LONG;
		dlg.m_ass.bja_con.con_long = atol((const char *) cd.m_assvalue);
	}
	else  {
		dlg.m_ass.bja_con.const_type = CON_STRING;
		strncpy(dlg.m_ass.bja_con.con_string, cd.m_assvalue, BTC_VALUE);
		dlg.m_ass.bja_con.con_string[BTC_VALUE] = '\0';
	}
#endif	
#ifdef	BTRW
	dlg.m_ass = ((CBtrwApp *)AfxGetApp())->m_defass;
#endif

	if  (dlg.DoModal() != IDOK)
		return;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ASSLIST);
	int	 where = lb->GetCurSel();
	CString	rfmt;
	GenAssFmt(dlg.m_ass, rfmt);

	where = lb->InsertString(where, rfmt);
	m_jasses[m_nasses] = dlg.m_ass;
	lb->SetItemData(where, DWORD(m_nasses));
	m_nasses++;
	m_changes++;
}

void CJasslist::OnClickedEditass()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ASSLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;
	unsigned  rowpos = lb->GetItemData(where);
	CEditass	dlg;
	dlg.m_ass = m_jasses[rowpos];
	if  (dlg.DoModal() == IDOK)  {
		m_jasses[rowpos] = dlg.m_ass;
		lb->DeleteString(where);
		CString	rfmt;
		GenAssFmt(dlg.m_ass, rfmt);
		lb->SetItemData(lb->InsertString(where, rfmt), DWORD(rowpos));
		m_changes++;
	}	
}

void CJasslist::OnClickedAssdel()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ASSLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;

	unsigned  rowpos = lb->GetItemData(where);
	lb->DeleteString(where);

	for  (unsigned  cnt = rowpos + 1;  cnt < m_nasses;  cnt++)
		m_jasses[cnt-1] = m_jasses[cnt];
	m_nasses--;

	for  (int bcnt = lb->GetCount()-1;  bcnt >= 0;  bcnt--)  {
		unsigned  rp = lb->GetItemData(bcnt);
		if  (rp >= rowpos)
			lb->SetItemData(bcnt, DWORD(rp-1));
	}
	m_changes++;
}


void CJasslist::OnDblclkAsslist()
{
	if  (!m_writeable)
		return;
	if  (m_nasses != 0)
		OnClickedEditass();
	else
		OnClickedNewass();
}

void CJasslist::OnOK() 
{
	if  (m_writeable)  {
		if  (m_nasses != 0)  {
			Jass	news[MAXSEVARS];
			CListBox  *lb = (CListBox *) GetDlgItem(IDC_ASSLIST);
			for  (int cnt = 0; cnt < m_nasses;  cnt++)  {
				int  rp = lb->GetItemData(cnt);
				news[cnt] = m_jasses[rp];
				if  (rp != cnt)
					m_changes++;
			}
			for  (cnt = 0;  cnt < m_nasses;  cnt++)
				m_jasses[cnt] = news[cnt];
		}
	}
	else
		m_changes = 0;
	CDialog::OnOK();
}

const DWORD a117HelpIDs[]=
{
	IDC_ASSLIST,	IDH_117_223,	// Job Assignments: "" (ListBox)
	IDC_NEWASS,	IDH_117_224,	// Job Assignments: "New" (Button)
	IDC_EDITASS,	IDH_117_225,	// Job Assignments: "Edit" (Button)
	IDC_ASSDEL,	IDH_117_226,	// Job Assignments: "Delete" (Button)
	0, 0
};

BOOL CJasslist::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a117HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
