// envlist.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "envlist.h"
#include "editenv.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnvlist dialog

CEnvlist::CEnvlist(CWnd* pParent /*=NULL*/)
	: CDialog(CEnvlist::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnvlist)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_nenvs = m_maxenvs = 0;
	m_changes = 0;
	m_envs = NULL;
}

CEnvlist::~CEnvlist()
{
	if  (m_envs)
		delete [] m_envs;
}		

void CEnvlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnvlist)
	DDX_Control(pDX, IDC_ENVLIST, m_dragenv);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEnvlist, CDialog)
	//{{AFX_MSG_MAP(CEnvlist)
	ON_BN_CLICKED(IDC_NEWENV, OnClickedNewenv)
	ON_BN_CLICKED(IDC_EDITENV, OnClickedEditenv)
	ON_BN_CLICKED(IDC_DELENV, OnClickedDelenv)
	ON_LBN_DBLCLK(IDC_ENVLIST, OnDblclkEnvlist)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnvlist message handlers

void CEnvlist::OnClickedNewenv()
{
	CEditenv	dlg;
	if  (dlg.DoModal() != IDOK)
		return;

	Menvir	nv;
	nv.e_name = dlg.m_ename;
	nv.e_value = dlg.m_evalue;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ENVLIST);
	int	 where = lb->GetCurSel();
	where = lb->InsertString(where, nv.e_name + "\t" + nv.e_value);

	if  (m_nenvs >= m_maxenvs)  {
		m_maxenvs += 10;
		Menvir  *news = new Menvir [m_maxenvs];
		for  (unsigned cnt = 0;  cnt < m_nenvs;  cnt++)
			news[cnt] = m_envs[cnt];
		if  (m_envs)
			delete [] m_envs;
		m_envs = news;
	}
	m_envs[m_nenvs] = nv;
	lb->SetItemData(where, DWORD(m_nenvs));
	m_nenvs++;
	m_changes++;
}

void CEnvlist::OnClickedEditenv()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ENVLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;
	unsigned  rowpos = lb->GetItemData(where);
	CEditenv	dlg;
	dlg.m_ename = m_envs[rowpos].e_name;
	dlg.m_evalue = m_envs[rowpos].e_value;
	if  (dlg.DoModal() == IDOK)  {
		m_envs[rowpos].e_name = dlg.m_ename;
		m_envs[rowpos].e_value = dlg.m_evalue;
		lb->DeleteString(where);
		lb->SetItemData(lb->InsertString(where, dlg.m_ename + "\t" + dlg.m_evalue), DWORD(rowpos));
		m_changes++;
	}	
}

void CEnvlist::OnClickedDelenv()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ENVLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;

	unsigned  rowpos = lb->GetItemData(where);
	lb->DeleteString(where);

	for  (unsigned  cnt = rowpos + 1;  cnt < m_nenvs;  cnt++)
		m_envs[cnt-1] = m_envs[cnt];
	m_nenvs--;

	for  (int bcnt = lb->GetCount()-1;  bcnt >= 0;  bcnt--)  {
		unsigned  rp = lb->GetItemData(bcnt);
		if  (rp >= rowpos)
			lb->SetItemData(bcnt, DWORD(rp-1));
	}
	m_changes++;
}

void CEnvlist::OnDblclkEnvlist()
{
	if  (!m_writeable)
		return;
	if  (m_nenvs != 0)
		OnClickedEditenv();
	else
		OnClickedNewenv();
}

BOOL CEnvlist::OnInitDialog()
{
	CDialog::OnInitDialog();	
	m_maxenvs = m_nenvs;

	CListBox  *lb = (CListBox *) GetDlgItem(IDC_ENVLIST);
	lb->SetTabStops();
	for  (unsigned  cnt = 0;  cnt < m_nenvs;  cnt++)  {
		int  where = lb->InsertString(-1, m_envs[cnt].e_name + "\t" + m_envs[cnt].e_value);
		lb->SetItemData(where, DWORD(cnt));
	}

	if  (!m_writeable)  {
		GetDlgItem(IDC_NEWENV)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDITENV)->EnableWindow(FALSE);
		GetDlgItem(IDC_DELENV)->EnableWindow(FALSE);
	}			
	return TRUE;
}

void CEnvlist::OnOK() 
{
	if  (m_writeable)  {
		if  (m_nenvs != 0)  {
			Menvir  *news = new Menvir [m_nenvs];
			CListBox  *lb = (CListBox *) GetDlgItem(IDC_ENVLIST);
			for  (unsigned cnt = 0; cnt < m_nenvs;  cnt++)  {
				unsigned  rp = lb->GetItemData(cnt);
				news[cnt] = m_envs[rp];
				if  (rp != cnt)
					m_changes++;
			}
			delete [] m_envs;
			m_envs = news;
		}
	}
	else
		m_changes = 0;
	CDialog::OnOK();
}

const DWORD a110HelpIDs[]=
{
	IDC_ENVLIST,	IDH_110_202,	// Job environment variables: "" (ListBox)
	IDC_NEWENV,	IDH_110_203,	// Job environment variables: "New" (Button)
	IDC_EDITENV,	IDH_110_204,	// Job environment variables: "Edit" (Button)
	IDC_DELENV,	IDH_110_205,	// Job environment variables: "Delete" (Button)
	0, 0
};

BOOL CEnvlist::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a110HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
