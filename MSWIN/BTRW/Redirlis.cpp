// redirlis.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "redirlis.h"
#include "editredi.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRedirlist dialog

CRedirlist::CRedirlist(CWnd* pParent /*=NULL*/)
	: CDialog(CRedirlist::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRedirlist)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT                            
	m_changes = 0;
	m_nredirs = m_maxredirs = 0;
	m_redirs = NULL;
}

CRedirlist::~CRedirlist()
{
	if  (m_redirs)
		delete [] m_redirs;
}		

void CRedirlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRedirlist)
	DDX_Control(pDX, IDC_REDIRLIST, m_dragredir);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRedirlist, CDialog)
	//{{AFX_MSG_MAP(CRedirlist)
	ON_BN_CLICKED(IDC_NEWREDIR, OnClickedNewredir)
	ON_BN_CLICKED(IDC_EDITREDIR, OnClickedEditredir)
	ON_BN_CLICKED(IDC_DELREDIR, OnClickedDelredir)
	ON_LBN_DBLCLK(IDC_REDIRLIST, OnDblclkRedirlist)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRedirlist message handlers

static	void	GenRdFmt(const Mredir &copy, CString &result)
{
	char	texts[30];
	wsprintf(texts, "%d\t", copy.fd);
	result = texts;
	if  (copy.action < RD_ACT_CLOSE)
		result += copy.buffer;
	else  {
		if  (copy.action == RD_ACT_DUP)
			wsprintf(texts, "%d = %d", copy.fd, copy.arg);
		result += texts;
	}
}		
		
void CRedirlist::OnClickedNewredir()
{
	CEditredir	dlg;
	dlg.m_redir.fd = 1;
	dlg.m_redir.action = RD_ACT_WRT;
	if  (dlg.DoModal() != IDOK)
		return;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_REDIRLIST);
	int	 where = lb->GetCurSel();
	CString	rfmt;
	GenRdFmt(dlg.m_redir, rfmt);

	where = lb->InsertString(where, rfmt);

	if  (m_nredirs >= m_maxredirs)  {
		m_maxredirs += 10;
		Mredir  *news = new Mredir [m_maxredirs];
		for  (unsigned cnt = 0;  cnt < m_nredirs;  cnt++)
			news[cnt] = m_redirs[cnt];
		if  (m_redirs)
			delete [] m_redirs;
		m_redirs = news;
	}
	m_redirs[m_nredirs] = dlg.m_redir;
	lb->SetItemData(where, DWORD(m_nredirs));
	m_nredirs++;
	m_changes++;
}

void CRedirlist::OnClickedEditredir()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_REDIRLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;
	unsigned  rowpos = lb->GetItemData(where);
	CEditredir	dlg;
	dlg.m_redir = m_redirs[rowpos];
	if  (dlg.DoModal() == IDOK)  {
		m_redirs[rowpos] = dlg.m_redir;
		lb->DeleteString(where);
		CString	rfmt;
		GenRdFmt(dlg.m_redir, rfmt);
		lb->SetItemData(lb->InsertString(where, rfmt), DWORD(rowpos));
		m_changes++;
	}	
}

void CRedirlist::OnClickedDelredir()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_REDIRLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;

	unsigned  rowpos = lb->GetItemData(where);
	lb->DeleteString(where);

	for  (unsigned  cnt = rowpos + 1;  cnt < m_nredirs;  cnt++)
		m_redirs[cnt-1] = m_redirs[cnt];
	m_nredirs--;

	for  (int bcnt = lb->GetCount()-1;  bcnt >= 0;  bcnt--)  {
		unsigned  rp = lb->GetItemData(bcnt);
		if  (rp >= rowpos)
			lb->SetItemData(bcnt, DWORD(rp-1));
	}
	m_changes++;
}

void CRedirlist::OnDblclkRedirlist()
{
	if  (!m_writeable)
		return;
	if  (m_nredirs != 0)
		OnClickedEditredir();
	else
		OnClickedNewredir();
}

BOOL CRedirlist::OnInitDialog()
{
	CDialog::OnInitDialog();	
	m_maxredirs = m_nredirs;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_REDIRLIST);
	lb->SetTabStops();

	for  (unsigned  cnt = 0;  cnt < m_nredirs;  cnt++)  {
		CString	rfmt;
		GenRdFmt(m_redirs[cnt], rfmt);
		int  where = lb->InsertString(-1, rfmt);
		lb->SetItemData(where, DWORD(cnt));
	}

	if  (!m_writeable)  {
		GetDlgItem(IDC_NEWREDIR)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITREDIR)->EnableWindow(FALSE);
		GetDlgItem(IDC_DELREDIR)->EnableWindow(FALSE);
	}			
	return TRUE;
}

void CRedirlist::OnOK() 
{
	if  (m_writeable)  {
		if  (m_nredirs != 0)  {
			Mredir  *news = new Mredir [m_nredirs];
			CListBox  *lb = (CListBox *) GetDlgItem(IDC_REDIRLIST);
			for  (unsigned cnt = 0; cnt < m_nredirs;  cnt++)  {
				unsigned  rp = lb->GetItemData(cnt);
				news[cnt] = m_redirs[rp];
				if  (rp != cnt)
					m_changes++;
			}
			delete [] m_redirs;
			m_redirs = news;
		}
	}
	else
		m_changes = 0;
	CDialog::OnOK();
}

const DWORD a119HelpIDs[]=
{
	IDC_REDIRLIST,	IDH_119_283,	// Job redirections: "" (ListBox)
	IDC_NEWREDIR,	IDH_119_284,	// Job redirections: "New" (Button)
	IDC_EDITREDIR,	IDH_119_285,	// Job redirections: "Edit" (Button)
	IDC_DELREDIR,	IDH_119_286,	// Job redirections: "Delete" (Button)
	0, 0
};

BOOL CRedirlist::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a119HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
