// winopts.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWinopts dialog

class CWinopts : public CDialog
{
// Construction
public:
	CWinopts(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CWinopts)
	enum { IDD = IDD_WINOPTS };
	CString	m_jobqueue;
	int		m_localonly;
	int		m_confdel;
	CString	m_username;
	CString	m_groupname;
	BOOL	m_incnull;
	//}}AFX_DATA
	BOOL	m_isjobs;	//  True if in jobs window

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinopts)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWinopts)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
