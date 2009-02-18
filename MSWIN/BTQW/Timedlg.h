// timedlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Ctimedlg dialog

class Ctimedlg : public CDialog
{
// Construction
public:
	Ctimedlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(Ctimedlg)
	enum { IDD = IDD_TIMER };
	//}}AFX_DATA
	BOOL		m_writeable;
	Timecon		m_tc;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Ctimedlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Ctimedlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedDelete();
	afx_msg void OnClickedRetain();
	afx_msg void OnClickedRepmins();
	afx_msg void OnClickedRephours();
	afx_msg void OnClickedRepdays();
	afx_msg void OnClickedRepweeks();
	afx_msg void OnClickedRepmonthsb();
	afx_msg void OnClickedRepmonthse();
	afx_msg void OnClickedRepyears();
	afx_msg void OnClickedAvsun();
	afx_msg void OnClickedAvmon();
	afx_msg void OnClickedAvtue();
	afx_msg void OnClickedAvwed();
	afx_msg void OnClickedAvthu();
	afx_msg void OnClickedAvfri();
	afx_msg void OnClickedAvsat();
	afx_msg void OnClickedAvholiday();
	afx_msg void OnClickedTimeisset();
	afx_msg void OnChangeRepeat();
	afx_msg void OnClickedNpskip();
	afx_msg void OnClickedNpdelcurr();
	afx_msg void OnClickedNpdelall();
	afx_msg void OnNpcatchup();
	afx_msg void OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrWday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMon(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrYear(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMonthday(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeMonthday();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	void	Enablehtime(const BOOL);
	void	Enablerepopt(const BOOL);
	void	Enablerepeat(const BOOL);
	void	Enablerepdays(const BOOL);
	void	Enablenposs(const BOOL);
	void	Enablermday(const BOOL);
	void	fillintime();
	void	fillincomes();
	
private:
	void	checkday(const int);
};
