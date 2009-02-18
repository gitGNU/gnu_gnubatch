// progopts.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Cprogopts dialog

class Cprogopts : public CDialog
{
// Construction
public:
	Cprogopts(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(Cprogopts)
	enum { IDD = IDD_PROGOPTS };
	CString	m_jobqueue;
	BOOL	m_verbose;
	BOOL	m_binmode;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Cprogopts)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Cprogopts)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
