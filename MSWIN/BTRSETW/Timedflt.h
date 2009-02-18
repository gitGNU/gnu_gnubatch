// timedflt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTimedflt dialog

class CTimedflt : public CDialog
{
// Construction
public:
	CTimedflt(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTimedflt)
	enum { IDD = IDD_DEFTIMES };
	BOOL	m_avsun;
	BOOL	m_avmon;
	BOOL	m_avtue;
	BOOL	m_avwed;
	BOOL	m_avthu;
	BOOL	m_avfri;
	BOOL	m_avsat;
	BOOL	m_avhol;
	int		m_nptype;
	int		m_repeatopt;
	//}}AFX_DATA
	unsigned  char  m_tc_mday;	// Month day
	unsigned  long	m_tc_rate;	// Repeat interval

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimedflt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	Enablerepunit(const BOOL);
	void	Enablerepmday(const BOOL);

protected:

	// Generated message map functions
	//{{AFX_MSG(CTimedflt)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedRepmins();
	afx_msg void OnClickedRephours();
	afx_msg void OnClickedRepdays();
	afx_msg void OnClickedRepweeks();
	afx_msg void OnClickedRepmonthsb();
	afx_msg void OnClickedRepmonthse();
	afx_msg void OnClickedRepyears();
	afx_msg void OnClickedDelete();
	afx_msg void OnClickedRetain();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
