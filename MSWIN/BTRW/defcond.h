// defcond.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Cdefcond dialog

class Cdefcond : public CDialog
{
// Construction
public:
	Cdefcond(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(Cdefcond)
	enum { IDD = IDD_DEFCONDOPT };
	CString	m_constval;
	int		m_condop;
	int		m_condcrit;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Cdefcond)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Cdefcond)
	afx_msg void OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
