// timedlg.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h" 
#include "mainfrm.h"
#include "netwmsg.h"
#include "btqw.h"
#include "timedlg.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

const	long	SECSPERDAY = 3600L * 24L;

const  unsigned  char  mdays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

/////////////////////////////////////////////////////////////////////////////
// Ctimedlg dialog

Ctimedlg::Ctimedlg(CWnd* pParent /*=NULL*/)
	: CDialog(Ctimedlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(Ctimedlg)
	//}}AFX_DATA_INIT
}

void Ctimedlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Ctimedlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Ctimedlg, CDialog)
	//{{AFX_MSG_MAP(Ctimedlg)
	ON_BN_CLICKED(IDC_DELETE, OnClickedDelete)
	ON_BN_CLICKED(IDC_RETAIN, OnClickedRetain)
	ON_BN_CLICKED(IDC_REPMINS, OnClickedRepmins)
	ON_BN_CLICKED(IDC_REPHOURS, OnClickedRephours)
	ON_BN_CLICKED(IDC_REPDAYS, OnClickedRepdays)
	ON_BN_CLICKED(IDC_REPWEEKS, OnClickedRepweeks)
	ON_BN_CLICKED(IDC_REPMONTHSB, OnClickedRepmonthsb)
	ON_BN_CLICKED(IDC_REPMONTHSE, OnClickedRepmonthse)
	ON_BN_CLICKED(IDC_REPYEARS, OnClickedRepyears)
	ON_BN_CLICKED(IDC_AVSUN, OnClickedAvsun)
	ON_BN_CLICKED(IDC_AVMON, OnClickedAvmon)
	ON_BN_CLICKED(IDC_AVTUE, OnClickedAvtue)
	ON_BN_CLICKED(IDC_AVWED, OnClickedAvwed)
	ON_BN_CLICKED(IDC_AVTHU, OnClickedAvthu)
	ON_BN_CLICKED(IDC_AVFRI, OnClickedAvfri)
	ON_BN_CLICKED(IDC_AVSAT, OnClickedAvsat)
	ON_BN_CLICKED(IDC_AVHOLIDAY, OnClickedAvholiday)
	ON_BN_CLICKED(IDC_TIMEISSET, OnClickedTimeisset)
	ON_EN_CHANGE(IDC_REPEAT, OnChangeRepeat)
	ON_BN_CLICKED(IDC_NPSKIP, OnClickedNpskip)
	ON_BN_CLICKED(IDC_NPDELCURR, OnClickedNpdelcurr)
	ON_BN_CLICKED(IDC_NPDELALL, OnClickedNpdelall)
	ON_BN_CLICKED(IDC_NPCATCHUP, OnNpcatchup)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_HOUR, OnDeltaposScrHour)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MIN, OnDeltaposScrMin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_WDAY, OnDeltaposScrWday)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MDAY, OnDeltaposScrMday)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MON, OnDeltaposScrMon)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_YEAR, OnDeltaposScrYear)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_MONTHDAY, OnDeltaposScrMonthday)
	ON_EN_CHANGE(IDC_MONTHDAY, OnChangeMonthday)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Ctimedlg message handlers

BOOL Ctimedlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_REPEAT))->SetRange(1, SHRT_MAX);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_MONTHDAY))->SetRange(1, 31);
    
    if  (m_tc.tc_istime)  {
    	((CButton *)GetDlgItem(IDC_TIMEISSET))->SetCheck(1);
    	Enablehtime(TRUE);
    }
    else  {
    	((CButton *)GetDlgItem(IDC_TIMEISSET))->SetCheck(0);
    	Enablehtime(FALSE);                               
    }
    if  (!m_writeable)  {
		for  (int cnt = IDC_MIN;  cnt <= IDC_YEAR;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);        
		for  (cnt = IDC_SCR_MIN;  cnt <= IDC_COLON;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		GetDlgItem(IDC_REPEAT)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_REPEAT)->EnableWindow(FALSE);
		for  (cnt = IDC_MESS_AVOIDING; cnt <= IDC_AVHOLIDAY; cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		for  (cnt = IDC_MESS_NOTPOSS; cnt <= IDC_NPDELALL; cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		GetDlgItem(IDC_REPRESULT)->EnableWindow(FALSE);
		GetDlgItem(IDC_MESS_COMESTO)->EnableWindow(FALSE);
		for  (cnt = IDC_MONDAYNAME;  cnt <= IDC_SCR_MONTHDAY;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
	}    
	return TRUE;
}

void Ctimedlg::Enablehtime(const BOOL enab)
{
	if  (enab)  {
		for  (int cnt = IDC_MIN;  cnt <= IDC_COLON;  cnt++)
			GetDlgItem(cnt)->EnableWindow(TRUE);
		Enablerepopt(TRUE);
		fillintime();
	}
	else  {
		Enablerepopt(FALSE);
		for  (int cnt = IDC_MIN;  cnt <= IDC_YEAR;  cnt++)  {
			SetDlgItemText(cnt, "");
			GetDlgItem(cnt)->EnableWindow(FALSE);        
		}
		for  (cnt = IDC_SCR_MIN;  cnt <= IDC_COLON;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
	}		
}

void	Ctimedlg::Enablerepopt(const BOOL enab)
{
	if  (enab)  {
		for  (int cnt = 0;  cnt <= TC_YEARS;  cnt++)  {
			CButton  *it = (CButton *) GetDlgItem(cnt + IDC_DELETE);
			it->EnableWindow(TRUE);                                 
			it->SetCheck(cnt == m_tc.tc_repeat? 1: 0);
		}
		Enablerepeat(m_tc.tc_repeat >= TC_MINUTES);
	}
	else  {
		Enablerepeat(FALSE);
		for  (int cnt = 0;  cnt <= TC_YEARS;  cnt++)  {
			CButton  *it = (CButton *) GetDlgItem(cnt + IDC_DELETE);
			it->EnableWindow(FALSE);                                 
			it->SetCheck(0);
		}
	}		
}
		
void	Ctimedlg::Enablerepeat(const BOOL enab)
{
	if  (enab)  {
		GetDlgItem(IDC_REPEAT)->EnableWindow(TRUE);
		GetDlgItem(IDC_SCR_REPEAT)->EnableWindow(TRUE);
		SetDlgItemInt(IDC_REPEAT, UINT(m_tc.tc_rate));
		Enablerepdays(TRUE);
		Enablenposs(TRUE);
		GetDlgItem(IDC_REPRESULT)->EnableWindow(TRUE);
		GetDlgItem(IDC_MESS_COMESTO)->EnableWindow(TRUE);
		fillincomes();
		Enablermday(m_tc.tc_repeat == TC_MONTHSB || m_tc.tc_repeat == TC_MONTHSE);
	}
	else  {
		SetDlgItemText(IDC_REPEAT, "");
		Enablermday(FALSE);
		GetDlgItem(IDC_REPEAT)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_REPEAT)->EnableWindow(FALSE);
		Enablerepdays(FALSE);
		Enablenposs(FALSE);
		SetDlgItemText(IDC_REPRESULT, "");
		GetDlgItem(IDC_REPRESULT)->EnableWindow(FALSE);
		GetDlgItem(IDC_MESS_COMESTO)->EnableWindow(FALSE);
	}
}

void	Ctimedlg::Enablerepdays(const BOOL enab)
{
	if  (enab)  {
		GetDlgItem(IDC_MESS_AVOIDING)->EnableWindow(TRUE);
		for  (int cnt = IDC_AVSUN; cnt <= IDC_AVHOLIDAY; cnt++)  {
			CButton  *it = (CButton *) GetDlgItem(cnt);
			it->EnableWindow(TRUE);
			it->SetCheck(m_tc.tc_nvaldays & (1 << (cnt - IDC_AVSUN))? 1: 0);
		}		
	}
	else  {
		for  (int cnt = IDC_AVSUN; cnt <= IDC_AVHOLIDAY; cnt++)  {
			CButton  *it = (CButton *) GetDlgItem(cnt);
			it->SetCheck(0);
			it->EnableWindow(FALSE);
		}		
		GetDlgItem(IDC_MESS_AVOIDING)->EnableWindow(FALSE);
	}
}

void	Ctimedlg::Enablenposs(const BOOL enab)
{
	if  (enab)  {
		GetDlgItem(IDC_MESS_NOTPOSS)->EnableWindow(TRUE);
		for  (int  cnt = IDC_NPSKIP; cnt <= IDC_NPCATCHUP; cnt++)  {
			CButton *it = (CButton *) GetDlgItem(cnt);
			it->EnableWindow(TRUE);
			it->SetCheck(m_tc.tc_nposs == cnt -  IDC_NPSKIP? 1: 0);
		}
	}
	else  {
		for  (int  cnt = IDC_NPSKIP; cnt <= IDC_NPCATCHUP; cnt++)  {
			CButton *it = (CButton *) GetDlgItem(cnt);
			it->SetCheck(0);
			it->EnableWindow(FALSE);
		}
		GetDlgItem(IDC_MESS_NOTPOSS)->EnableWindow(FALSE);
	}	
}
		
void	Ctimedlg::Enablermday(const BOOL enab)
{   
	if  (enab)  {
		for  (int cnt = IDC_MONDAYNAME;  cnt <= IDC_SCR_MONTHDAY;  cnt++)
			GetDlgItem(cnt)->EnableWindow(TRUE);
		int	 mday = m_tc.tc_mday;
		if  (m_tc.tc_repeat == TC_MONTHSE)  {
		    tm	*tp = localtime(&m_tc.tc_nexttime);       
			int	ndays = mdays[tp->tm_mon];
			if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
				ndays++;
            mday = ndays - mday;
        }    
		SetDlgItemInt(IDC_MONTHDAY, mday); 
	}
	else  {		
	    SetDlgItemText(IDC_MONTHDAY, "");
		for  (int cnt = IDC_MONDAYNAME;  cnt <= IDC_SCR_MONTHDAY;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
	}
}			
		
void	Ctimedlg::fillintime()
{
	tm	*tp = localtime(&m_tc.tc_nexttime);
	char	tdigs[4];
	wsprintf(tdigs, "%.2d", tp->tm_hour);
	SetDlgItemText(IDC_HOUR, tdigs);
	wsprintf(tdigs, "%.2d", tp->tm_min);
	SetDlgItemText(IDC_MIN, tdigs);
	CString	wday;
	wday.LoadString(IDS_SUNDAY + tp->tm_wday);
	SetDlgItemText(IDC_WDAY, wday);
	SetDlgItemInt(IDC_MDAY, tp->tm_mday);
	CString mon;
	mon.LoadString(IDS_JANUARY + tp->tm_mon);
	SetDlgItemText(IDC_MON, mon);
	SetDlgItemInt(IDC_YEAR, tp->tm_year + 1900);
}	

void	Ctimedlg::fillincomes()
{
	time_t	nexttime = m_tc.advtime();
	tm	*tp = localtime(&nexttime);
	char	tstring[50];
	CString	wday, mon;
	wday.LoadString(IDS_SUNDAY + tp->tm_wday);
	mon.LoadString(IDS_JANUARY + tp->tm_mon);
	wsprintf(tstring, "%.2d:%.2d %s %.2d %s %d",
		tp->tm_hour, tp->tm_min, (const char FAR *) wday,
		tp->tm_mday, (const char FAR *) mon, tp->tm_year + 1900);
	SetDlgItemText(IDC_REPRESULT, tstring);
}		

void Ctimedlg::OnClickedDelete()
{
	m_tc.tc_repeat = TC_DELETE;
	Enablerepeat(FALSE);
}

void Ctimedlg::OnClickedRetain()
{
	m_tc.tc_repeat = TC_RETAIN;
	Enablerepeat(FALSE);
}

void Ctimedlg::OnClickedRepmins()
{
	m_tc.tc_repeat = TC_MINUTES;
	Enablerepeat(TRUE);
}

void Ctimedlg::OnClickedRephours()
{
	m_tc.tc_repeat = TC_HOURS;
	Enablerepeat(TRUE);
}

void Ctimedlg::OnClickedRepdays()
{
	m_tc.tc_repeat = TC_DAYS;
	Enablerepeat(TRUE);
}

void Ctimedlg::OnClickedRepweeks()
{
	m_tc.tc_repeat = TC_WEEKS;
	Enablerepeat(TRUE);
}

void Ctimedlg::OnClickedRepmonthsb()
{
	m_tc.tc_repeat = TC_MONTHSB;
	tm  *tp = localtime(&m_tc.tc_nexttime);
	m_tc.tc_mday = tp->tm_mday;
	Enablerepeat(TRUE);
}

void Ctimedlg::OnClickedRepmonthse()
{
	m_tc.tc_repeat = TC_MONTHSE;
	tm  *tp = localtime(&m_tc.tc_nexttime);
	int	ndays = mdays[tp->tm_mon];
	if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
		ndays++;
	m_tc.tc_mday = ndays - tp->tm_mday;
	Enablerepeat(TRUE);
}

void Ctimedlg::OnClickedRepyears()
{
	m_tc.tc_repeat = TC_YEARS;
	Enablerepeat(TRUE);
}

void	Ctimedlg::checkday(const int daynum)
{
	if  (((CButton *)GetDlgItem(daynum))->GetCheck() == 1)  {
		unsigned  prev = m_tc.tc_nvaldays;
		m_tc.tc_nvaldays |= 1 << (daynum - IDC_AVSUN);
		if  ((m_tc.tc_nvaldays & TC_ALLWEEKDAYS) == TC_ALLWEEKDAYS)  {
			AfxMessageBox(IDP_NOTALLDAYS, MB_OK|MB_ICONEXCLAMATION);
			m_tc.tc_nvaldays = prev;
		}	             
	}
	else
		m_tc.tc_nvaldays &= ~(1 << (daynum - IDC_AVSUN));
	fillincomes();
}
	
void Ctimedlg::OnClickedAvsun()
{
	checkday(IDC_AVSUN);
}

void Ctimedlg::OnClickedAvmon()
{
	checkday(IDC_AVMON);
}

void Ctimedlg::OnClickedAvtue()
{
	checkday(IDC_AVTUE);
}

void Ctimedlg::OnClickedAvwed()
{
	checkday(IDC_AVWED);
}

void Ctimedlg::OnClickedAvthu()
{
	checkday(IDC_AVTHU);
}

void Ctimedlg::OnClickedAvfri()
{
	checkday(IDC_AVFRI);
}

void Ctimedlg::OnClickedAvsat()
{
	checkday(IDC_AVSAT);
}

void Ctimedlg::OnClickedAvholiday()
{
	checkday(IDC_AVHOLIDAY);
}

void Ctimedlg::OnClickedTimeisset()
{
	if  (((CButton *)GetDlgItem(IDC_TIMEISSET))->GetCheck() == 1)  {
		m_tc.tc_nexttime = time(NULL);
		m_tc.tc_istime = 1;
#ifdef	BTRW
		m_tc.tc_nvaldays = (1 << 0) | (1 << 6);	// Sun/sat
		m_tc.tc_rate = 5;
		m_tc.tc_nposs = TC_WAIT1;
		m_tc.tc_repeat = TC_RETAIN;
#endif
#ifdef	BTQW
		timedefs	&td = ((CBtqwApp *)AfxGetApp())->m_timedefs;
		m_tc.tc_repeat = td.m_repeatopt;
		m_tc.tc_rate = td.m_tc_rate;
		m_tc.tc_nposs = td.m_nptype;
		m_tc.tc_nvaldays = 0;
		if  (td.m_avsun)	m_tc.tc_nvaldays |= 1 << 0;
		if  (td.m_avmon)	m_tc.tc_nvaldays |= 1 << 1;
		if  (td.m_avtue)	m_tc.tc_nvaldays |= 1 << 2;
		if  (td.m_avwed)	m_tc.tc_nvaldays |= 1 << 3;
		if  (td.m_avthu)	m_tc.tc_nvaldays |= 1 << 4;
		if  (td.m_avfri)	m_tc.tc_nvaldays |= 1 << 5;
		if  (td.m_avsat)	m_tc.tc_nvaldays |= 1 << 6;
		if  (td.m_avhol)	m_tc.tc_nvaldays |= 1 << 7;
#endif
		Enablehtime(TRUE);
	}
	else  {
		m_tc.tc_istime = 0;
		Enablehtime(FALSE);
	}	
}

void Ctimedlg::OnChangeRepeat()
{
	int  newrate = GetDlgItemInt(IDC_REPEAT);
	if  (newrate > 0)  {
		m_tc.tc_rate = newrate;
		fillincomes();
	}	
}

void Ctimedlg::OnClickedNpskip()
{
	if  (((CButton *)GetDlgItem(IDC_NPSKIP))->GetCheck())
		m_tc.tc_nposs =	TC_SKIP;
}

void Ctimedlg::OnClickedNpdelcurr()
{
	if  (((CButton *)GetDlgItem(IDC_NPDELCURR))->GetCheck())
		m_tc.tc_nposs =	TC_WAIT1;
}

void Ctimedlg::OnClickedNpdelall()
{
	if  (((CButton *)GetDlgItem(IDC_NPDELALL))->GetCheck())
		m_tc.tc_nposs =	TC_WAITALL;
}

void Ctimedlg::OnNpcatchup()
{
	if  (((CButton *)GetDlgItem(IDC_NPCATCHUP))->GetCheck())
		m_tc.tc_nposs =	TC_CATCHUP;
}

void Ctimedlg::OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_tc.tc_nexttime + pNMUpDown->iDelta * 60L * 60L;
	if  (newtime <= time(NULL))
		MessageBeep(MB_ICONASTERISK);
	else  {
		m_tc.tc_nexttime = newtime;
		fillintime();
		if  (m_tc.tc_repeat >= TC_MINUTES)
			fillincomes();
	}
	*pResult = 0;
}

void Ctimedlg::OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_tc.tc_nexttime + pNMUpDown->iDelta * 60L;
	if  (newtime <= time(NULL))
		MessageBeep(MB_ICONASTERISK);
	else  {
		m_tc.tc_nexttime = newtime;
		fillintime();
		if  (m_tc.tc_repeat >= TC_MINUTES)
			fillincomes();
	}
	*pResult = 0;
}

void Ctimedlg::OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_tc.tc_nexttime + pNMUpDown->iDelta * 60L * 60L * 24L;
	if  (newtime <= time(NULL))
		MessageBeep(MB_ICONASTERISK);
	else  {
		m_tc.tc_nexttime = newtime;
		fillintime();
		if  (m_tc.tc_repeat >= TC_MINUTES)
			fillincomes();
	}
	*pResult = 0;
}

void Ctimedlg::OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_tc.tc_nexttime + pNMUpDown->iDelta * 60L * 60L * 24L;
	if  (newtime <= time(NULL))
		MessageBeep(MB_ICONASTERISK);
	else  {
		m_tc.tc_nexttime = newtime;
		fillintime();
		if  (m_tc.tc_repeat >= TC_MINUTES)
			fillincomes();
	}
	*pResult = 0;
}

void Ctimedlg::OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_tc.tc_nexttime;
	tm	*lt = localtime(&newtime);
	if  (pNMUpDown->iDelta >= 0)  {
		unsigned  ndays = mdays[lt->tm_mon];
		if  (lt->tm_mon == 1  &&  lt->tm_year % 4 == 0)
			ndays++;
		newtime += ndays * 60L * 60L * 24L;
	}	
	else  {
		int	mon = lt->tm_mon - 1;
		if  (mon < 0)
			mon = 11;
		unsigned  ndays = mdays[mon];
		if  (mon == 1  &&  lt->tm_year % 4 == 0)
			ndays++;
		newtime -= ndays * 60L * 60L * 24L;
	}	
	if  (newtime <= time(NULL))
		MessageBeep(MB_ICONASTERISK);
	else  {
		m_tc.tc_nexttime = newtime;
		fillintime();
		if  (m_tc.tc_repeat >= TC_MINUTES)
			fillincomes();
	}
	*pResult = 0;
}

void Ctimedlg::OnDeltaposScrYear(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	time_t	newtime = m_tc.tc_nexttime;
	tm	*tp = localtime(&newtime);
	if  (pNMUpDown->iDelta >= 0)  {
		if  ((tp->tm_year % 4 == 0  &&  tp->tm_mon <= 1) ||
			 (tp->tm_year % 4 == 3  &&  tp->tm_mon > 1))
			newtime += SECSPERDAY;
		newtime += 365 * SECSPERDAY;
	}
	else  {
		if  ((tp->tm_year % 4 == 1  &&  tp->tm_mon <= 1) ||
			 (tp->tm_year % 4 == 0  &&  tp->tm_mon > 1))
			newtime -= SECSPERDAY;
		newtime -= 365 * SECSPERDAY;
	}	
	if  (newtime <= time(NULL))
		MessageBeep(MB_ICONASTERISK);
	else  {
		m_tc.tc_nexttime = newtime;
		fillintime();
		if  (m_tc.tc_repeat >= TC_MINUTES)
			fillincomes();
	}
	*pResult = 0;
}

void Ctimedlg::OnDeltaposScrMonthday(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int  existing = GetDlgItemInt(IDC_MONTHDAY);
	Timecon		&tc = m_tc;
	tm	*tp = localtime(&tc.tc_nexttime);       
	int	ndays = mdays[tp->tm_mon];
	if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
		ndays++;
	if  (pNMUpDown->iDelta >= 0)  {
		if  (existing >= ndays)
			goto  beep;
		existing++;
	}
	else  if  (existing <= 1)
		goto  beep;
	else
		existing--;
	tc.tc_mday = tc.tc_repeat == TC_MONTHSE? ndays - existing: existing;
	SetDlgItemInt(IDC_MONTHDAY, existing);
	fillincomes();	
	*pResult = 0;
	return;
beep:
	MessageBeep(MB_ICONASTERISK);
	*pResult = 0;
}

void Ctimedlg::OnChangeMonthday() 
{
	Timecon		&tc = m_tc;
	int  existing = GetDlgItemInt(IDC_MONTHDAY);
	tm	*tp = localtime(&tc.tc_nexttime);       
	int	ndays = mdays[tp->tm_mon];
	if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
		ndays++;
	if  (existing > ndays)
		SetDlgItemInt(IDC_MONTHDAY, ndays);
}

void Ctimedlg::OnOK() 
{
	Timecon		&tc = m_tc;
	if  (tc.tc_repeat == TC_MONTHSB  ||  tc.tc_repeat == TC_MONTHSE)  {
		int  existing = GetDlgItemInt(IDC_MONTHDAY);
		if  (tc.tc_repeat == TC_MONTHSE)  {
			tm	*tp = localtime(&tc.tc_nexttime);       
			int	ndays = mdays[tp->tm_mon];
			if  (tp->tm_mon == 1  &&  tp->tm_year % 4 == 0)
				ndays++;
			tc.tc_mday = ndays - existing;
		}
		else
			tc.tc_mday = existing;
	}

	CDialog::OnOK();
}

const DWORD a131HelpIDs[]=
{
	IDC_RETAIN,	IDH_131_346,	// Set job time: "Run and retain" (Button)
	IDC_NPSKIP,	IDH_131_370,	// Set job time: "Skip" (Button)
	IDC_REPMINS,	IDH_131_347,	// Set job time: "Minutes" (Button)
	IDC_NPDELCURR,	IDH_131_371,	// Set job time: "Delay Current" (Button)
	IDC_REPHOURS,	IDH_131_348,	// Set job time: "Hours" (Button)
	IDC_NPDELALL,	IDH_131_372,	// Set job time: "Delay all" (Button)
	IDC_REPDAYS,	IDH_131_349,	// Set job time: "Days" (Button)
	IDC_NPCATCHUP,	IDH_131_373,	// Set job time: "Catch up" (Button)
	IDC_REPWEEKS,	IDH_131_350,	// Set job time: "Weeks" (Button)
	IDC_REPMONTHSB,	IDH_131_351,	// Set job time: "Months (rel begin)" (Button)
	IDC_REPRESULT,	IDH_131_375,	// Set job time: "" (Edit)
	IDC_REPMONTHSE,	IDH_131_352,	// Set job time: "Months (rel end)" (Button)
	IDC_REPYEARS,	IDH_131_353,	// Set job time: "Years" (Button)
	IDC_TIMEISSET,	IDH_131_331,	// Set job time: "Set time for job" (Button)
	IDC_REPEAT,	IDH_131_355,	// Set job time: "0" (Edit)
	IDC_MIN,	IDH_131_333,	// Set job time: "" (Edit)
	IDC_SCR_REPEAT,	IDH_131_355,	// Set job time: "Spin1" (msctls_updown32)
	IDC_HOUR,	IDH_131_333,	// Set job time: "" (Edit)
	IDC_WDAY,	IDH_131_333,	// Set job time: "" (Edit)
	IDC_MONTHDAY,	IDH_131_358,	// Set job time: "" (Edit)
	IDC_MDAY,	IDH_131_333,	// Set job time: "" (Edit)
	IDC_SCR_MONTHDAY,	IDH_131_358,	// Set job time: "Spin2" (msctls_updown32)
	IDC_MON,	IDH_131_333,	// Set job time: "" (Edit)
	IDC_YEAR,	IDH_131_333,	// Set job time: "" (Edit)
	IDC_AVSUN,	IDH_131_361,	// Set job time: "Sun" (Button)
	IDC_SCR_MIN,	IDH_131_333,	// Set job time: "Spin2" (msctls_updown32)
	IDC_AVMON,	IDH_131_361,	// Set job time: "Mon" (Button)
	IDC_SCR_HOUR,	IDH_131_333,	// Set job time: "Spin1" (msctls_updown32)
	IDC_AVTUE,	IDH_131_361,	// Set job time: "Tue" (Button)
	IDC_SCR_WDAY,	IDH_131_333,	// Set job time: "Spin7" (msctls_updown32)
	IDC_AVWED,	IDH_131_361,	// Set job time: "Wed" (Button)
	IDC_SCR_MDAY,	IDH_131_333,	// Set job time: "Spin8" (msctls_updown32)
	IDC_AVTHU,	IDH_131_361,	// Set job time: "Thur" (Button)
	IDC_SCR_MON,	IDH_131_333,	// Set job time: "Spin9" (msctls_updown32)
	IDC_AVFRI,	IDH_131_361,	// Set job time: "Fri" (Button)
	IDC_SCR_YEAR,	IDH_131_333,	// Set job time: "Spin10" (msctls_updown32)
	IDC_AVSAT,	IDH_131_361,	// Set job time: "Sat" (Button)
	IDC_AVHOLIDAY,	IDH_131_361,	// Set job time: "Holiday" (Button)
	IDC_DELETE,	IDH_131_345,	// Set job time: "Run and delete" (Button)
	0, 0
};

BOOL Ctimedlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a131HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
