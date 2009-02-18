// timedflt.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "timedflt.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimedflt dialog

CTimedflt::CTimedflt(CWnd* pParent /*=NULL*/)
	: CDialog(CTimedflt::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTimedflt)
	m_avsun = FALSE;
	m_avmon = FALSE;
	m_avtue = FALSE;
	m_avwed = FALSE;
	m_avthu = FALSE;
	m_avfri = FALSE;
	m_avsat = FALSE;
	m_avhol = FALSE;
	m_nptype = -1;
	m_repeatopt = -1;
	//}}AFX_DATA_INIT
}

void CTimedflt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTimedflt)
	DDX_Check(pDX, IDC_AVSUN, m_avsun);
	DDX_Check(pDX, IDC_AVMON, m_avmon);
	DDX_Check(pDX, IDC_AVTUE, m_avtue);
	DDX_Check(pDX, IDC_AVWED, m_avwed);
	DDX_Check(pDX, IDC_AVTHU, m_avthu);
	DDX_Check(pDX, IDC_AVFRI, m_avfri);
	DDX_Check(pDX, IDC_AVSAT, m_avsat);
	DDX_Check(pDX, IDC_AVHOLIDAY, m_avhol);
	DDX_Radio(pDX, IDC_NPSKIP, m_nptype);
	DDX_Radio(pDX, IDC_DELETE, m_repeatopt);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTimedflt, CDialog)
	//{{AFX_MSG_MAP(CTimedflt)
	ON_BN_CLICKED(IDC_REPMINS, OnClickedRepmins)
	ON_BN_CLICKED(IDC_REPHOURS, OnClickedRephours)
	ON_BN_CLICKED(IDC_REPDAYS, OnClickedRepdays)
	ON_BN_CLICKED(IDC_REPWEEKS, OnClickedRepweeks)
	ON_BN_CLICKED(IDC_REPMONTHSB, OnClickedRepmonthsb)
	ON_BN_CLICKED(IDC_REPMONTHSE, OnClickedRepmonthse)
	ON_BN_CLICKED(IDC_REPYEARS, OnClickedRepyears)
	ON_BN_CLICKED(IDC_DELETE, OnClickedDelete)
	ON_BN_CLICKED(IDC_RETAIN, OnClickedRetain)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimedflt message handlers

BOOL CTimedflt::OnInitDialog()
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

void CTimedflt::OnOK()
{
	if  (m_repeatopt >= TC_MINUTES)  {
		m_tc_rate = (unsigned long) GetDlgItemInt(IDC_REPEAT);
		if  (m_repeatopt == TC_MONTHSB || m_repeatopt == TC_MONTHSE)
        	m_tc_mday = (unsigned char) GetDlgItemInt(IDC_MONTHDAY);
    }
	CDialog::OnOK();
}

void CTimedflt::Enablerepunit(const BOOL on)
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
		
void CTimedflt::Enablerepmday(const BOOL on)
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

void CTimedflt::OnClickedRepmins()
{
	m_repeatopt = TC_MINUTES;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void CTimedflt::OnClickedRephours()
{
	m_repeatopt = TC_HOURS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void CTimedflt::OnClickedRepdays()
{
	m_repeatopt = TC_DAYS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void CTimedflt::OnClickedRepweeks()
{
	m_repeatopt = TC_WEEKS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void CTimedflt::OnClickedRepmonthsb()
{
	m_repeatopt = TC_MONTHSB;
	Enablerepunit(TRUE);
	Enablerepmday(TRUE);
}

void CTimedflt::OnClickedRepmonthse()
{
	m_repeatopt = TC_MONTHSE;
	Enablerepunit(TRUE);
	Enablerepmday(TRUE);
}

void CTimedflt::OnClickedRepyears()
{
	m_repeatopt = TC_YEARS;
	Enablerepunit(TRUE);
	Enablerepmday(FALSE);
}

void CTimedflt::OnClickedDelete()
{
	m_repeatopt = TC_DELETE;
	Enablerepunit(FALSE);
	Enablerepmday(FALSE);
}

void CTimedflt::OnClickedRetain()
{
	m_repeatopt = TC_RETAIN;
	Enablerepunit(FALSE);
	Enablerepmday(FALSE);
}

const DWORD a103HelpIDs[]=
{
	IDC_REPEAT,	IDH_103_186,	// Time defaults: "0" (Edit)
	IDC_SCR_REPEAT,	IDH_103_186,	// Time defaults: "Spin1" (msctls_updown32)
	IDC_MONTHDAY,	IDH_103_189,	// Time defaults: "0" (Edit)
	IDC_SCR_MONTHDAY,	IDH_103_189,	// Time defaults: "Spin2" (msctls_updown32)
	IDC_AVSUN,	IDH_103_192,	// Time defaults: "Sun" (Button)
	IDC_AVMON,	IDH_103_192,	// Time defaults: "Mon" (Button)
	IDC_AVTUE,	IDH_103_192,	// Time defaults: "Tue" (Button)
	IDC_AVWED,	IDH_103_192,	// Time defaults: "Wed" (Button)
	IDC_AVTHU,	IDH_103_192,	// Time defaults: "Thur" (Button)
	IDC_AVFRI,	IDH_103_192,	// Time defaults: "Fri" (Button)
	IDC_AVSAT,	IDH_103_192,	// Time defaults: "Sat" (Button)
	IDC_AVHOLIDAY,	IDH_103_192,	// Time defaults: "Holiday" (Button)
	IDC_DELETE,	IDH_103_176,	// Time defaults: "Run and delete" (Button)
	IDC_RETAIN,	IDH_103_177,	// Time defaults: "Run and retain" (Button)
	IDC_NPSKIP,	IDH_103_201,	// Time defaults: "Skip" (Button)
	IDC_REPMINS,	IDH_103_178,	// Time defaults: "Minutes" (Button)
	IDC_NPDELCURR,	IDH_103_202,	// Time defaults: "Delay Current" (Button)
	IDC_REPHOURS,	IDH_103_178,	// Time defaults: "Hours" (Button)
	IDC_NPDELALL,	IDH_103_203,	// Time defaults: "Delay all" (Button)
	IDC_REPDAYS,	IDH_103_178,	// Time defaults: "Days" (Button)
	IDC_NPCATCHUP,	IDH_103_204,	// Time defaults: "Catch up" (Button)
	IDC_REPWEEKS,	IDH_103_178,	// Time defaults: "Weeks" (Button)
	IDC_REPMONTHSB,	IDH_103_178,	// Time defaults: "Months (rel begin)" (Button)
	IDC_REPMONTHSE,	IDH_103_178,	// Time defaults: "Months (rel end)" (Button)
	IDC_REPYEARS,	IDH_103_178,	// Time defaults: "Years" (Button)
	0, 0
};

BOOL CTimedflt::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a103HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
