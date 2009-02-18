// editenv.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditenv dialog

class CEditenv : public CDialog
{
// Construction
public:
	CEditenv(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEditenv)
	enum { IDD = IDD_EDITENV };
	CString	m_ename;
	CString	m_evalue;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditenv)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditenv)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
