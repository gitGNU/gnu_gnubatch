// defcond.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDefcond dialog

class CDefcond : public CDialog
{
// Construction
public:
	CDefcond(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDefcond)
	enum { IDD = IDD_DEFCOND };
	int		m_condop;
	int		m_condcrit;
	CString	m_constval;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDefcond)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDefcond)
	afx_msg void OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
