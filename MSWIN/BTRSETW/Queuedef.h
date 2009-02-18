// queuedef.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueuedef dialog

class CQueuedef : public CDialog
{
// Construction
public:
	CQueuedef(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CQueuedef)
	enum { IDD = IDD_QUEUEDEFS };
	CString	m_title;
	CString	m_cmdint;
	UINT	m_priority;
	UINT	m_loadlev;
	//}}AFX_DATA
	netid_t	m_hostid;			//  Hostid needed for CI list

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueuedef)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CQueuedef)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCmdinterp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
