// jdsearch.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJDSearch dialog

class CJDSearch : public CDialog
{
// Construction
public:
	CJDSearch(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CJDSearch)
	enum { IDD = IDD_SEARCHDATA };
	BOOL	m_ignorecase;
	CString	m_lookfor;
	BOOL	m_wrapround;
	int		m_sforward;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJDSearch)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJDSearch)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
