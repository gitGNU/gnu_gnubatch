// netstat.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "btqw.h"
#include "netstat.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetstat dialog


CNetstat::CNetstat(CWnd* pParent /*=NULL*/)
	: CDialog(CNetstat::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNetstat)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CNetstat::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetstat)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNetstat, CDialog)
	//{{AFX_MSG_MAP(CNetstat)
	ON_CBN_SELCHANGE(IDC_HOST, OnSelchangeHost)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNetstat message handlers

BOOL CNetstat::OnInitDialog()
{
	CDialog::OnInitDialog();
	remote	*rp;
	CComboBox	*cb = (CComboBox *)GetDlgItem(IDC_HOST);	
	current_q.setfirst();
	pending_q.setfirst();
	while  (rp = current_q.next())
		cb->SetItemData(cb->AddString(rp->namefor()), DWORD(rp->hostid));
	while  (rp = pending_q.next())
		cb->SetItemData(cb->AddString(rp->namefor()), DWORD(rp->hostid));
	return TRUE;
}

void CNetstat::OnSelchangeHost()
{
	CComboBox	*cb = (CComboBox *)GetDlgItem(IDC_HOST);	
    int	sel = cb->GetCurSel();
   	((CButton *) GetDlgItem(IDC_NS_PROBESENT))->SetCheck(0);
   	((CButton *) GetDlgItem(IDC_NS_SYNCREQ))->SetCheck(0);
   	((CButton *) GetDlgItem(IDC_NS_SYNCDONE))->SetCheck(0);
    if  (sel < 0)
    	return;
    netid_t	hostid = netid_t(cb->GetItemData(sel));
    remote	*rp;
    if  (rp = pending_q.find(hostid))  {
		((CButton *) GetDlgItem(IDC_NS_PROBESENT))->SetCheck(1);
		return;
	}
	if  (!(rp = current_q.find(hostid)))
		return;
	((CButton *) GetDlgItem(IDC_NS_PROBESENT))->SetCheck(1);
	if  (rp->sockfd == INVALID_SOCKET)
		return;                                     
	((CButton *) GetDlgItem(IDC_CONNECTED))->SetCheck(1);
	if  (rp->is_sync >= NSYNC_REQ)  {
	   	((CButton *) GetDlgItem(IDC_NS_SYNCREQ))->SetCheck(1);
	   	if  (rp->is_sync >= NSYNC_OK)
	   		((CButton *) GetDlgItem(IDC_NS_SYNCDONE))->SetCheck(1);
	}
}

const DWORD a123HelpIDs[]=
{
	IDC_HOST,	IDH_123_272,	// Network Status: "" (ComboBox)
	IDC_NS_PROBESENT,	IDH_123_273,	// Network Status: "Probe sent (or no probe required)" (Button)
	IDC_CONNECTED,	IDH_123_274,	// Network Status: "TCP Connection made" (Button)
	IDC_NS_SYNCREQ,	IDH_123_275,	// Network Status: "Sync Requested" (Button)
	IDC_NS_SYNCDONE,	IDH_123_276,	// Network Status: "Sync Completed" (Button)
	0, 0
};

BOOL CNetstat::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a123HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
