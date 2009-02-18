// procpar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProcpar dialog

class CProcpar : public CDialog
{
// Construction
public:
	CProcpar(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CProcpar)
	enum { IDD = IDD_PROCPAR };
	CString	m_directory;
	int		m_local;
	int		m_advterr;
	//}}AFX_DATA
	unsigned  short m_umask;
	unsigned  long	m_ulimit;         
	Exits	m_exits;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProcpar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProcpar)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDeltaposScrUlimit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
