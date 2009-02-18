// hostdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHostdlg dialog

class CHostdlg : public CDialog
{
// Construction
public:
	CHostdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CHostdlg)
	enum { IDD = IDD_HOST };
	CString	m_hostname;
	CString	m_aliasname;
	BOOL	m_probefirst;
	UINT	m_timeout;
	//}}AFX_DATA
	netid_t	m_hid;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHostdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	Refocus(const int);

protected:

	// Generated message map functions
	//{{AFX_MSG(CHostdlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
