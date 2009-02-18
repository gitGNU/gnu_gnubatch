// deftimop.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"        
#include "deftimop.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Cdeftimopt dialog

Cdeftimopt::Cdeftimopt(CWnd* pParent /*=NULL*/)
	: CDialog(Cdeftimopt::IDD, pParent)
{
	//{{AFX_DATA_INIT(Cdeftimopt)
	m_nptype = -1;
	m_repeatopt = -1;
	m_avsun = FALSE;
	m_avmon = FALSE;
	m_avtue = FALSE;
	m_avwed = FALSE;
	m_avthu = FALSE;
	m_avfri = FALSE;
	m_avsat = FALSE;
	m_avhol = FALSE;
	//}}AFX_DATA_INIT
}

void Cdeftimopt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Cdeftimopt)
	DDX_Radio(pDX, IDC_NPSKIP, m_nptype);
	DDX_Radio(pDX, IDC_DELETE, m_repeatopt);
	DDX_Check(pDX, IDC_AVSUN, m_avsun);
	DDX_Check(pDX, IDC_AVMON, m_avmon);
	DDX_Check(pDX, IDC_AVTUE, m_avtue);
	DDX_Check(pDX, IDC_AVWED, m_avwed);
	DDX_Check(pDX, IDC_AVTHU, m_avthu);
	DDX_Check(pDX, IDC_AVFRI, m_avfri);
	DDX_Check(pDX, IDC_AVSAT, m_avsat);
	DDX_Check(pDX, IDC_AVHOLIDAY, m_avhol);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Cdeftimopt, CDialog)
	//{{AFX_MSG_MAP(Cdeftimopt)
	ON_BN_CLICKED(IDC_DELETE, OnClickedDelete)
	ON_BN_CLICKED(IDC_RETAIN, OnClickedRetain)
	ON_BN_CLICKED(IDC_REPMINS, OnClickedRepmins)
	ON_BN_CLICKED(IDC_REPHOURS, OnClickedRephours)
	ON_BN_CLICKED(IDC_REPDAYS, OnClickedRepdays)
	ON_BN_CLICKED(IDC_REPWEEKS, OnClickedRepweeks)
	ON_BN_CLICKED(IDC_REPMONTHSB, OnClickedRepmonthsb)
	ON_BN_CLICKED(IDC_REPMONTHSE, OnClickedRepmonthse)
	ON_BN_CLICKED(IDC_REPYEARS, OnClickedRepyears)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Cdeftimopt message handlers

BOOL Cdeftimopt::OnInitDialog()
{
	CDialog::OnInitDialog();
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_REPEAT))->SetRange(1, SHRT_MAX);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_MONTHDAY))->SetRange(1, 31);
	if  (m_repeatopt >= TC_MINUTES)  {
		SetDlgItemInt(IDC_REPEAT, int(m_tc_rate));
		if  (m_repeatopt == TC_MONTHSB || m_repeatopt == TC_MONTHSE)
			SetDlgItemInt(IDC_MONTHDAY, int(m_tc_mday));            
		else
			GetDlgItem(IDC_MONTHDAY)->EnableWindow(FALSE);
	}
	else  {
		GetDlgItem(IDC_REPEAT)->EnableWindow(FALSE);
		GetDlgItem(IDC_MONTHDAY)->EnableWindow(FALSE);
	}	
	return TRUE;
}

void Cdeftimopt::OnOK()
{
	if  (m_repeatopt >= TC_MINUTES)  {
		m_tc_rate = (unsigned long) GetDlgItemInt(IDC_REPEAT);
		if  (m_repeatopt == TC_MONTHSB || m_repeatopt == TC_MONTHSE)
        	m_tc_mday = (unsigned char) GetDlgItemInt(IDC_MONTHDAY);
    }
	CDialog::OnOK();
}

void Cdeftimopt::Enablerepunit(const BOOL on)
{
	if  (on)  {
		GetDlgItem(IDC_REPEAT)->EnableWindow(TRUE);
		if  (m_tc_rate == 0)
			m_tc_rate = 5;
		SetDlgItemInt(IDC_REPEAT, int(m_tc_rate));
	}
	else  {
		SetDlgItemText(IDC_REPEAT, "");
		GetDlgItem(IDC_REPEAT)->EnableWindow(FALSE);
	}
}
		
void Cdeftimopt::Enablerepmday(const BOOL on)
{
	if  (on)  {
		GetDlgItem(IDC_MONTHDAY)->EnableWindow(TRUE);
		if  (m_tc_mday == 0)
			m_tc_mday = m_repeatopt == TC_MONTHSB? 1: 31;
		SetDlgItemInt(IDC_MONTHDAY, int(m_tc_mday));
	}
	else  {
		SetDlgItemText(IDC_MONTHDAY, "");
		GetDlgItem(IDC_MONTHDAY)->EnableWindow(FALSE);
	}
}

void Cdeftimopt::OnClickedDelete()
{
	m_repeatopt = TC_DELETE;
	Enablerepunit(FALSE);
	Enablerepmday(FALSE);
}

void Cdeftimopt::OnClickedRetain()
{
	m_repeatopt = TC_RETAIN;
	Enablerepunit(FALSE);
	Enablerepmday(FALSE);
}

void Cdeftimopt::OnClickedRepmins()
{
	m_repeatopt = TC_MINUTES;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void Cdeftimopt::OnClickedRephours()
{
	m_repeatopt = TC_HOURS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void Cdeftimopt::OnClickedRepdays()
{
	m_repeatopt = TC_DAYS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void Cdeftimopt::OnClickedRepweeks()
{
	m_repeatopt = TC_WEEKS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void Cdeftimopt::OnClickedRepmonthsb()
{
	m_repeatopt = TC_MONTHSB;
	Enablerepunit(TRUE);
	Enablerepmday(TRUE);
}

void Cdeftimopt::OnClickedRepmonthse()
{
	m_repeatopt = TC_MONTHSE;
	Enablerepunit(TRUE);
	Enablerepmday(TRUE);
}

void Cdeftimopt::OnClickedRepyears()
{
	m_repeatopt = TC_YEARS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

const DWORD a107HelpIDs[]=
{
	IDC_RETAIN,	IDH_107_345,	// Time defaults: "Run and retain" (Button)
	IDC_NPSKIP,	IDH_107_370,	// Time defaults: "Skip" (Button)
	IDC_REPMINS,	IDH_107_345,	// Time defaults: "Minutes" (Button)
	IDC_NPDELCURR,	IDH_107_370,	// Time defaults: "Delay Current" (Button)
	IDC_REPHOURS,	IDH_107_345,	// Time defaults: "Hours" (Button)
	IDC_NPDELALL,	IDH_107_370,	// Time defaults: "Delay all" (Button)
	IDC_REPDAYS,	IDH_107_345,	// Time defaults: "Days" (Button)
	IDC_NPCATCHUP,	IDH_107_370,	// Time defaults: "Catch up" (Button)
	IDC_REPWEEKS,	IDH_107_345,	// Time defaults: "Weeks" (Button)
	IDC_REPMONTHSB,	IDH_107_345,	// Time defaults: "Months (rel begin)" (Button)
	IDC_REPMONTHSE,	IDH_107_345,	// Time defaults: "Months (rel end)" (Button)
	IDC_REPYEARS,	IDH_107_345,	// Time defaults: "Years" (Button)
	IDC_REPEAT,	IDH_107_355,	// Time defaults: "0" (Edit)
	IDC_SCR_REPEAT,	IDH_107_355,	// Time defaults: "Spin1" (msctls_updown32)
	IDC_MONTHDAY,	IDH_107_358,	// Time defaults: "0" (Edit)
	IDC_SCR_MONTHDAY,	IDH_107_358,	// Time defaults: "Spin2" (msctls_updown32)
	IDC_AVSUN,	IDH_107_361,	// Time defaults: "Sun" (Button)
	IDC_AVMON,	IDH_107_361,	// Time defaults: "Mon" (Button)
	IDC_AVTUE,	IDH_107_361,	// Time defaults: "Tue" (Button)
	IDC_AVWED,	IDH_107_361,	// Time defaults: "Wed" (Button)
	IDC_AVTHU,	IDH_107_361,	// Time defaults: "Thur" (Button)
	IDC_AVFRI,	IDH_107_361,	// Time defaults: "Fri" (Button)
	IDC_AVSAT,	IDH_107_361,	// Time defaults: "Sat" (Button)
	IDC_AVHOLIDAY,	IDH_107_361,	// Time defaults: "Holiday" (Button)
	IDC_DELETE,	IDH_107_345,	// Time defaults: "Run and delete" (Button)
	0, 0
};

BOOL Cdeftimopt::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a107HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
