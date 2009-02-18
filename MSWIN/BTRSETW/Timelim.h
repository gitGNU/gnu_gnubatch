// timelim.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTimelim dialog

class CTimelim : public CDialog
{
// Construction
public:
	CTimelim(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTimelim)
	enum { IDD = IDD_TIMELIMS };
	int		m_autoksig;
	UINT	m_deltime;
	//}}AFX_DATA
	BOOL		m_writeable;
	unsigned  short	m_runon;
	unsigned  long	m_runtime;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimelim)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTimelim)
	virtual BOOL OnInitDialog();
	afx_msg void OnRtoff();
	afx_msg void OnDeltaposScrHour(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrSec(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrMin2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposScrSec2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP() 
public:
	void	fillinruntime();
	void	fillinrunontime();
	void	runtimebump(const long);
	void	runonbump(const int);
};
