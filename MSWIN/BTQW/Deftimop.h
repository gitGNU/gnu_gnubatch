// deftimop.h : header file
//

///////////////////////////////////////////////////////////////////////////
// Cdeftimopt dialog

class Cdeftimopt : public CDialog
{
// Construction
public:
	Cdeftimopt(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(Cdeftimopt)
	enum { IDD = IDD_DEFTIMOPT };
	int		m_nptype;
	int		m_repeatopt;
	BOOL	m_avsun;
	BOOL	m_avmon;
	BOOL	m_avtue;
	BOOL	m_avwed;
	BOOL	m_avthu;
	BOOL	m_avfri;
	BOOL	m_avsat;
	BOOL	m_avhol;
	//}}AFX_DATA
	unsigned  char  m_tc_mday;	// Month day
	unsigned  long	m_tc_rate;	// Repeat interval

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Cdeftimopt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	Enablerepunit(const BOOL);
	void	Enablerepmday(const BOOL);

protected:

	// Generated message map functions
	//{{AFX_MSG(Cdeftimopt)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedDelete();
	afx_msg void OnClickedRetain();
	afx_msg void OnClickedRepmins();
	afx_msg void OnClickedRephours();
	afx_msg void OnClickedRepdays();
	afx_msg void OnClickedRepweeks();
	afx_msg void OnClickedRepmonthsb();
	afx_msg void OnClickedRepmonthse();
	afx_msg void OnClickedRepyears();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
