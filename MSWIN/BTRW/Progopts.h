// progopts.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgopts dialog

class CProgopts : public CDialog
{
// Construction
public:
	CProgopts(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CProgopts)
	enum { IDD = IDD_PROGOPTS };
	CString	m_jeditor;
	BOOL	m_binmode;
	CString	m_jobqueue;
	BOOL	m_verbose;
	BOOL	m_nonexport;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgopts)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProgopts)
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
