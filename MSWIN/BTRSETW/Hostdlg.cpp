// hostdlg.cpp : implementation file
//

#include "stdafx.h"
#include "xbwnetwk.h"
#include "btrsetw.h"
#include "hostdlg.h"
#include <limits.h>
#include <ctype.h>
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHostdlg dialog

CHostdlg::CHostdlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHostdlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHostdlg)
	m_hostname = "";
	m_aliasname = "";
	m_probefirst = FALSE;
	m_timeout = 0;
	//}}AFX_DATA_INIT
}

void CHostdlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHostdlg)
	DDX_Text(pDX, IDC_HOSTNAME, m_hostname);
	DDV_MaxChars(pDX, m_hostname, 14);
	DDX_Text(pDX, IDC_ALIASNAME, m_aliasname);
	DDV_MaxChars(pDX, m_aliasname, 14);
	DDX_Check(pDX, IDC_PROBEFIRST, m_probefirst);
	DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHostdlg, CDialog)
	//{{AFX_MSG_MAP(CHostdlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHostdlg message handlers

BOOL CHostdlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_TIMEOUT))->SetRange(1, SHRT_MAX);
	return TRUE;
}

void CHostdlg::Refocus(const int nCtrl)
{                                      
	CEdit	*ew = (CEdit *) GetDlgItem(nCtrl);
	ew->SetSel(0, -1);
	ew->SetFocus();
}	
	
void CHostdlg::OnOK()
{
	char	hostname[HOSTNSIZE + 10], aliasname[HOSTNSIZE+10];
	
	GetDlgItemText(IDC_HOSTNAME, hostname, sizeof(hostname)-1);
	GetDlgItemText(IDC_ALIASNAME, aliasname, sizeof(aliasname)-1);
	hostname[sizeof(hostname)-1] = aliasname[sizeof(aliasname)-1] = '\0';

	if  (hostname[0] == '\0')  {
		AfxMessageBox(IDP_NOHOSTNAME, MB_OK|MB_ICONEXCLAMATION);
		Refocus(IDC_HOSTNAME);
		return;
	}	
	if  (isdigit(hostname[0])  &&  (aliasname[0] == '\0' || !isalpha(aliasname[0])))  {
		AfxMessageBox(IDP_NOALIASNAME, MB_OK|MB_ICONEXCLAMATION);
		Refocus(IDC_ALIASNAME);
		return;
	}	
	if  (strcmp(hostname, aliasname) == 0)  {
		AfxMessageBox(IDP_ALIASSAMEASHOST, MB_OK|MB_ICONEXCLAMATION);
		Refocus(IDC_ALIASNAME);
		return;
	}
	if  (isdigit(hostname[0]))  {
		netid_t	hid = inet_addr(hostname);
		if  (hid == INADDR_NONE)  {
			AfxMessageBox(IDP_BADINETADDR, MB_OK|MB_ICONEXCLAMATION); 
			Refocus(IDC_HOSTNAME);
			return;
		}
		if  (clashcheck(hid)) {
			AfxMessageBox(IDP_HIDCLASH, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_HOSTNAME);
			return;
		}	
		if  (hid == Locparams.myhostid)  {
			AfxMessageBox(IDP_HIDCLASHMINE, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_HOSTNAME);
			return;
		}	
		if  (clashcheck(aliasname))  {
			AfxMessageBox(IDP_ALIASCLASH, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_ALIASNAME);
			return;
		}	        
		m_hid = hid;
	}
	else  {
		if  (clashcheck(hostname))  {
			AfxMessageBox(IDP_HOSTCLASH, MB_OK|MB_ICONEXCLAMATION);	
			Refocus(IDC_HOSTNAME);
			return;
		}
		if  (aliasname[0]  &&  clashcheck(aliasname))  {
			AfxMessageBox(IDP_ALIASCLASH, MB_OK|MB_ICONEXCLAMATION);
			Refocus(IDC_ALIASNAME);
			return;
		}
		hostent	FAR *hp = gethostbyname(hostname);
		netid_t	nid;
		if  (!hp  ||  (nid = *(netid_t FAR *) hp->h_addr) == 0L  ||  nid == -1L)  {
			AfxMessageBox(IDP_UNKNOWNHOSTNAME, MB_OK|MB_ICONEXCLAMATION);	
			Refocus(IDC_HOSTNAME);
			return;
		}
		if  (nid == Locparams.myhostid)  {
			AfxMessageBox(IDP_MYHOSTNAME, MB_OK|MB_ICONEXCLAMATION);	
			Refocus(IDC_HOSTNAME);
			return;
		}
		m_hid = nid;
	}
		
	CDialog::OnOK();
}

const DWORD a104HelpIDs[]=
{
	IDC_SCR_TIMEOUT,	IDH_104_207,	// New Host Name: "Spin1" (msctls_updown32)
	IDC_PROBEFIRST,	IDH_104_209,	// New Host Name: "Probe first" (Button)
	IDC_HOSTNAME,	IDH_104_205,	// New Host Name: "" (Edit)
	IDC_ALIASNAME,	IDH_104_206,	// New Host Name: "" (Edit)
	IDC_TIMEOUT,	IDH_104_207,	// New Host Name: "0" (Edit)
	0, 0
};

BOOL CHostdlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a104HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
