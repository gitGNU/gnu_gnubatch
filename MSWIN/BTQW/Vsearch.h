// vsearch.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVSearch dialog

class CVSearch : public CDialog
{
// Construction
public:
	CVSearch(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVSearch)
	enum { IDD = IDD_VSEARCH };
	CString	m_searchstr;
	int		m_sforward;
	BOOL	m_suser;
	BOOL	m_svname;
	BOOL	m_svcomment;
	BOOL	m_svvalue;
	BOOL	m_wraparound;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVSearch)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVSearch)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
