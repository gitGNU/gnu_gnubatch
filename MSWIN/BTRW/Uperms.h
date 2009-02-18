// uperms.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUperms dialog

class CUperms : public CDialog
{
// Construction
public:
	CUperms(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CUperms)
	enum { IDD = IDD_USERPERM };
	CString	m_user;
	CString	m_group;
	//}}AFX_DATA
	Btuser	*m_priv;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUperms)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUperms)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
