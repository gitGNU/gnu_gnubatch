// uperms.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#define	APPCLASS	CBtrwApp
#include "uperms.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUperms dialog


CUperms::CUperms(CWnd* pParent /*=NULL*/)
	: CDialog(CUperms::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUperms)
	m_user = "";
	m_group = "";
	//}}AFX_DATA_INIT
}

void CUperms::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUperms)
	DDX_Text(pDX, IDC_USER, m_user);
	DDX_Text(pDX, IDC_GROUP, m_group);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CUperms, CDialog)
	//{{AFX_MSG_MAP(CUperms)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CUperms message handlers

BOOL CUperms::OnInitDialog()
{
	CDialog::OnInitDialog();
	if  (m_priv->btu_priv & BTM_RADMIN)
		((CButton *)GetDlgItem(IDC_RADMIN))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_WADMIN)
		((CButton *)GetDlgItem(IDC_WADMIN))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_CREATE)
		((CButton *)GetDlgItem(IDC_CREATE))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_SPCREATE)
		((CButton *)GetDlgItem(IDC_SPCREATE))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_SSTOP)
		((CButton *)GetDlgItem(IDC_SSTOP))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_UMASK)
		((CButton *)GetDlgItem(IDC_UMASK))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_ORP_UG)
		((CButton *)GetDlgItem(IDC_ORUG))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_ORP_UO)
		((CButton *)GetDlgItem(IDC_ORUO))->SetCheck(1);
	if  (m_priv->btu_priv & BTM_ORP_GO)
		((CButton *)GetDlgItem(IDC_ORGO))->SetCheck(1);
	char	nbuf[10];
	wsprintf(nbuf, "%u", m_priv->btu_maxll);
	SetDlgItemText(IDC_MAXLL, nbuf);
	wsprintf(nbuf, "%u", m_priv->btu_totll);
	SetDlgItemText(IDC_TOTLL, nbuf);
	wsprintf(nbuf, "%u", m_priv->btu_spec_ll);
	SetDlgItemText(IDC_SPECLL, nbuf);
	return TRUE;
}

const DWORD a123HelpIDs[]=
{
	IDC_RADMIN,	IDH_123_357,	// User Permissions: "Read admin" (Button)
	IDC_WADMIN,	IDH_123_357,	// User Permissions: "Write admin" (Button)
	IDC_CREATE,	IDH_123_357,	// User Permissions: "Create entry" (Button)
	IDC_SPCREATE,	IDH_123_357,	// User Permissions: "Special Create" (Button)
	IDC_SSTOP,	IDH_123_357,	// User Permissions: "Stop Scheduler" (Button)
	IDC_UMASK,	IDH_123_357,	// User Permissions: "Override def perms" (Button)
	IDC_USER,	IDH_123_247,	// User Permissions: "" (Edit)
	IDC_ORUG,	IDH_123_357,	// User Permissions: "OR user/group" (Button)
	IDC_GROUP,	IDH_123_248,	// User Permissions: "" (Edit)
	IDC_ORUO,	IDH_123_357,	// User Permissions: "OR user/other" (Button)
	IDC_ORGO,	IDH_123_357,	// User Permissions: "OR group/other" (Button)
	IDC_MAXLL,	IDH_123_366,	// User Permissions: "" (Edit)
	IDC_TOTLL,	IDH_123_367,	// User Permissions: "" (Edit)
	IDC_SPECLL,	IDH_123_368,	// User Permissions: "" (Edit)
	0, 0
};

BOOL CUperms::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a123HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
