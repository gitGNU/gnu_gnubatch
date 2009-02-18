// editfmt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditfmt dialog

class CEditfmt : public CDialog
{
// Construction
public:
	CEditfmt(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEditfmt)
	enum { IDD = IDD_EDITFMT };
	BOOL	m_tableft;
	UINT	m_width;
	BOOL	m_skipright;
	//}}AFX_DATA
	char	m_existing;
	int		m_uppercode, m_lowercode;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditfmt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditfmt)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
