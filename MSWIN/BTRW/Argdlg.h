// argdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CArgdlg dialog

class CArgdlg : public CDialog
{
// Construction
public:
	CArgdlg(CWnd* pParent = NULL);	// standard constructor
	~CArgdlg();

// Dialog Data
	//{{AFX_DATA(CArgdlg)
	enum { IDD = IDD_ARGLIST };
	CDragListBox	m_dragarg;
	//}}AFX_DATA
	unsigned  m_nargs;
	CString	  *m_args;
	unsigned  m_maxargs;
	int		  m_changes;
	BOOL	  m_writeable;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArgdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CArgdlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedNewarg();
	afx_msg void OnClickedEditarg();
	afx_msg void OnClickedDelarg();
	afx_msg void OnDblclkArglist();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
