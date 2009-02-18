// maildlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMaildlg dialog

class CMaildlg : public CDialog
{
// Construction
public:
	CMaildlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMaildlg)
	enum { IDD = IDD_MAILWRITE };
	BOOL	m_mail;
	BOOL	m_write;
	//}}AFX_DATA
	BOOL	m_writeable;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMaildlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMaildlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
