// varcomm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVarcomm dialog

class CVarcomm : public CDialog
{
// Construction
public:
	CVarcomm(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVarcomm)
	enum { IDD = IDD_VARCOMM };
	CString	m_varcomment;
	CString	m_varname;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVarcomm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVarcomm)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
