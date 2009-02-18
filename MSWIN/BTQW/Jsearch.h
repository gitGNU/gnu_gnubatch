// jsearch.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJsearch dialog

class CJsearch : public CDialog
{
// Construction
public:
	CJsearch(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CJsearch)
	enum { IDD = IDD_SEARCHJ };
	CString	m_searchstring;
	int		m_sforward;
	BOOL	m_stitle;
	BOOL	m_suser;
	BOOL	m_swraparound;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJsearch)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJsearch)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
