// NewGrpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "NewGrpDlg.h"
#include "ulist.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewGrpDlg dialog


CNewGrpDlg::CNewGrpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewGrpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewGrpDlg)
	m_groupname = _T("");
	//}}AFX_DATA_INIT
}


void CNewGrpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewGrpDlg)
	DDX_CBString(pDX, IDC_GROUP, m_groupname);
	DDV_MaxChars(pDX, m_groupname, 11);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewGrpDlg, CDialog)
	//{{AFX_MSG_MAP(CNewGrpDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewGrpDlg message handlers

BOOL CNewGrpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_GROUP);
	UGroupList *ug = new UGroupList;
	const  char  FAR  *n;
	while  (n = ug->next())
		uP->AddString(n);
	uP->SelectString(-1, m_groupname);
	delete  ug;
	return  TRUE;
}

const DWORD a108HelpIDs[]=
{
	IDC_GROUP,	IDH_108_244,	// Select New Group: "" (ComboBox)
	0, 0
};

BOOL CNewGrpDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a108HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
