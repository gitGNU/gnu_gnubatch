// titledlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTitledlg dialog

class CTitledlg : public CDialog
{
// Construction
public:
	CTitledlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTitledlg)
	enum { IDD = IDD_TITLEPRIO };
	CString	m_title;
	CString	m_cmdinterp;
	UINT	m_priority;
	UINT	m_loadlev;
	//}}AFX_DATA
	BOOL	m_writeable;
	netid_t	m_hostid;			//  Hostid needed for CI list

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTitledlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTitledlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCmdinterp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
