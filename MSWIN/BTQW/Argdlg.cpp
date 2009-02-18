// argdlg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "argdlg.h"
#include "editarg.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CArgdlg dialog

CArgdlg::CArgdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CArgdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CArgdlg)
	//}}AFX_DATA_INIT
	m_nargs = m_maxargs = 0;    
	m_changes = 0;
	m_args = NULL;
}

CArgdlg::~CArgdlg()
{
	if  (m_args)
		delete [] m_args;
}
		
void CArgdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArgdlg)
	DDX_Control(pDX, IDC_ARGLIST, m_dragarg);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CArgdlg, CDialog)
	//{{AFX_MSG_MAP(CArgdlg)
	ON_BN_CLICKED(IDC_NEWARG, OnClickedNewarg)
	ON_BN_CLICKED(IDC_EDITARG, OnClickedEditarg)
	ON_BN_CLICKED(IDC_DELARG, OnClickedDelarg)
	ON_LBN_DBLCLK(IDC_ARGLIST, OnDblclkArglist)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArgdlg message handlers

BOOL CArgdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_maxargs = m_nargs;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ARGLIST);
	for  (unsigned  cnt = 0;  cnt < m_nargs;  cnt++)  {
		int  where = lb->InsertString(-1, m_args[cnt]);
		lb->SetItemData(where, DWORD(cnt));
	}

	if  (!m_writeable)  {
		GetDlgItem(IDC_NEWARG)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITARG)->EnableWindow(FALSE);
		GetDlgItem(IDC_DELARG)->EnableWindow(FALSE);
	}			
	return TRUE;
}

void CArgdlg::OnClickedNewarg()
{
	CEditarg	dlg;
	if  (dlg.DoModal() != IDOK)
		return;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ARGLIST);
	int	 where = lb->GetCurSel();
	where = lb->InsertString(where, dlg.m_argval);

	if  (m_nargs >= m_maxargs)  {
		m_maxargs += 10;
		CString  *news = new CString [m_maxargs];
		for  (unsigned cnt = 0;  cnt < m_nargs;  cnt++)
			news[cnt] = m_args[cnt];
		if  (m_args)
			delete [] m_args;
		m_args = news;
	}
	m_args[m_nargs] = dlg.m_argval;
	lb->SetItemData(where, DWORD(m_nargs));
	m_nargs++;
	m_changes++;
}

void CArgdlg::OnClickedEditarg()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ARGLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;
	unsigned  rowpos = lb->GetItemData(where);
	CEditarg	dlg;
	dlg.m_argval = m_args[rowpos];
	if  (dlg.DoModal() == IDOK)  {
		m_args[rowpos] = dlg.m_argval;
		lb->DeleteString(where);
		lb->SetItemData(lb->InsertString(where, dlg.m_argval), DWORD(rowpos));
		m_changes++;
	}	
}

void CArgdlg::OnClickedDelarg()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ARGLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;

	unsigned  rowpos = lb->GetItemData(where);
	lb->DeleteString(where);

	for  (unsigned  cnt = rowpos + 1;  cnt < m_nargs;  cnt++)
		m_args[cnt-1] = m_args[cnt];
	m_nargs--;

	for  (int bcnt = lb->GetCount()-1;  bcnt >= 0;  bcnt--)  {
		unsigned  rp = lb->GetItemData(bcnt);
		if  (rp >= rowpos)
			lb->SetItemData(bcnt, DWORD(rp-1));
	}
	m_changes++;
}

void CArgdlg::OnDblclkArglist()
{
	if  (!m_writeable)
		return;
	if  (m_nargs != 0)
		OnClickedEditarg();
	else
		OnClickedNewarg();
}

void CArgdlg::OnOK() 
{
	if  (m_writeable)  {
		if  (m_nargs != 0)  {
			CString  *news = new CString [m_nargs];
			CListBox  *lb = (CListBox *) GetDlgItem(IDC_ARGLIST);
			for  (unsigned cnt = 0; cnt < m_nargs;  cnt++)  {
				unsigned  rp = lb->GetItemData(cnt);
				news[cnt] = m_args[rp];
				if  (rp != cnt)
					m_changes++;
			}
			delete [] m_args;
			m_args = news;
		}
	}
	else
		m_changes = 0;
	CDialog::OnOK();
}

const DWORD a101HelpIDs[]=
{
	IDC_ARGLIST,	IDH_101_150,	// Job Argument List: "" (ListBox)
	IDC_NEWARG,	IDH_101_151,	// Job Argument List: "New" (Button)
	IDC_EDITARG,	IDH_101_152,	// Job Argument List: "Edit" (Button)
	IDC_DELARG,	IDH_101_153,	// Job Argument List: "Delete" (Button)
	0, 0
};

BOOL CArgdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a101HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
