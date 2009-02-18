// assvar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAssvar dialog

class CAssvar : public CDialog
{
// Construction
public:
	CAssvar(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAssvar)
	enum { IDD = IDD_VARASS };
	CString	m_varname;
	CString	m_varcomment;
	//}}AFX_DATA
	Btcon	m_value;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAssvar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAssvar)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDeltaposScrAvarvalue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
