// timelim.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "timelim.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimelim dialog


CTimelim::CTimelim(CWnd* pParent /*=NULL*/)
	: CDialog(CTimelim::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTimelim)
	m_autoksig = -1;
	m_deltime = 0;
	//}}AFX_DATA_INIT
}

void CTimelim::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTimelim)
	DDX_Radio(pDX, IDC_KW_TERM, m_autoksig);
	DDX_Text(pDX, IDC_DELTIME, m_deltime);
	DDV_MinMaxUInt(pDX, m_deltime, 0, 32767);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTimelim, CDialog)
	//{{AFX_MSG_MAP(CTimelim)
	ON_BN_CLICKED(IDC_RTOFF, OnRtoff)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_HOUR, OnDeltaposScrHour)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MIN, OnDeltaposScrMin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_SEC, OnDeltaposScrSec)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MIN2, OnDeltaposScrMin2)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_SEC2, OnDeltaposScrSec2)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTimelim message handlers

BOOL CTimelim::OnInitDialog()
{
	CDialog::OnInitDialog();

	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_DELTIME))->SetRange(0, SHRT_MAX);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_HOUR))->SetRange(0, 168);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_MIN))->SetRange(0, 60);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_SEC))->SetRange(0, 60);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_MIN2))->SetRange(0, 32767/60);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_SEC2))->SetRange(0, 60);
	fillinruntime();
	fillinrunontime();
	if  (!m_writeable)  {
		GetDlgItem(IDC_DELTIME)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_DELTIME)->EnableWindow(FALSE);
		GetDlgItem(IDC_HOUR)->EnableWindow(FALSE);
		GetDlgItem(IDC_MIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_SEC)->EnableWindow(FALSE);
		GetDlgItem(IDC_MIN2)->EnableWindow(FALSE);
		GetDlgItem(IDC_SEC2)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_HOUR)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_MIN)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_SEC)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_MIN2)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_SEC2)->EnableWindow(FALSE);
	}
	return TRUE;
}

void CTimelim::OnRtoff()
{
	m_runtime = 0;
	fillinruntime();
}

void  CTimelim::runtimebump(const long delta)
{                                
	unsigned	long	newtime = m_runtime + delta;
	if  (delta >= 0)  {
		if  (newtime < m_runtime)  {
			MessageBeep(MB_ICONASTERISK);
			return;
		}
	}
	else  if  (newtime > m_runtime)  {
		MessageBeep(MB_ICONASTERISK);
		return;
	}
	m_runtime = newtime;
	fillinruntime();
}

void  CTimelim::runonbump(const int delta)
{                                
	unsigned	short	newtime = m_runon + delta;
	if  (delta >= 0)  {
		if  (newtime < m_runon)  {
			MessageBeep(MB_ICONASTERISK);
			return;
		}
	}
	else  if  (newtime > m_runon)  {
		MessageBeep(MB_ICONASTERISK);
		return;
	}
	m_runon = newtime;
	fillinrunontime();
}

void	CTimelim::fillinruntime()
{
	char	tdigs[10];
	wsprintf(tdigs, "%ld", m_runtime / 3600L);
	SetDlgItemText(IDC_HOUR, tdigs);
	wsprintf(tdigs, "%.2ld", (m_runtime % 3600L) / 60L);
	SetDlgItemText(IDC_MIN, tdigs);
	wsprintf(tdigs, "%.2ld", m_runtime % 60L);
	SetDlgItemText(IDC_SEC, tdigs);
}

void	CTimelim::fillinrunontime()
{
	char	tdigs[10];
	wsprintf(tdigs, "%.2d", m_runon / 60);
	SetDlgItemText(IDC_MIN2, tdigs);
	wsprintf(tdigs, "%.2d", m_runon % 60);
	SetDlgItemText(IDC_SEC2, tdigs);
}

void CTimelim::OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	runtimebump(pNMUpDown->iDelta * 3600L);
	*pResult = 0;
}

void CTimelim::OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	runtimebump(pNMUpDown->iDelta * 60L);
	*pResult = 0;
}

void CTimelim::OnDeltaposScrSec(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	runtimebump(pNMUpDown->iDelta);
	*pResult = 0;
}

void CTimelim::OnDeltaposScrMin2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	runonbump(pNMUpDown->iDelta * 60);
	*pResult = 0;
}

void CTimelim::OnDeltaposScrSec2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	runonbump(pNMUpDown->iDelta);
	*pResult = 0;
}

const DWORD a130HelpIDs[]=
{
	IDC_SCR_MIN2,	IDH_130_391,	// Job time limits: "Spin5" (msctls_updown32)
	IDC_SEC2,	IDH_130_391,	// Job time limits: "" (Edit)
	IDC_SCR_SEC2,	IDH_130_391,	// Job time limits: "Spin6" (msctls_updown32)
	IDC_DELTIME,	IDH_130_376,	// Job time limits: "0" (Edit)
	IDC_SCR_DELTIME,	IDH_130_376,	// Job time limits: "Spin1" (msctls_updown32)
	IDC_MIN,	IDH_130_333,	// Job time limits: "" (Edit)
	IDC_HOUR,	IDH_130_333,	// Job time limits: "" (Edit)
	IDC_SEC,	IDH_130_333,	// Job time limits: "" (Edit)
	IDC_SCR_SEC,	IDH_130_333,	// Job time limits: "Spin4" (msctls_updown32)
	IDC_RTOFF,	IDH_130_382,	// Job time limits: "OFF" (Button)
	IDC_KW_TERM,	IDH_130_383,	// Job time limits: "Sigterm" (Button)
	IDC_KW_KILL,	IDH_130_383,	// Job time limits: "Sigkill" (Button)
	IDC_SCR_MIN,	IDH_130_333,	// Job time limits: "Spin3" (msctls_updown32)
	IDC_KW_HUP,	IDH_130_383,	// Job time limits: "Sighup" (Button)
	IDC_SCR_HOUR,	IDH_130_333,	// Job time limits: "Spin2" (msctls_updown32)
	IDC_KW_INT,	IDH_130_383,	// Job time limits: "Sigint" (Button)
	IDC_KW_QUIT,	IDH_130_383,	// Job time limits: "Sigquit" (Button)
	IDC_KW_ARLM,	IDH_130_383,	// Job time limits: "Sigalrm" (Button)
	IDC_KW_BUS,	IDH_130_383,	// Job time limits: "Sigbus" (Button)
	IDC_KW_SEGV,	IDH_130_383,	// Job time limits: "Sigsegv" (Button)
	IDC_MIN2,	IDH_130_391,	// Job time limits: "" (Edit)
	0, 0
};

BOOL CTimelim::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a130HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
