// editcond.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditcond dialog

class CEditcond : public CDialog
{
// Construction
public:
	CEditcond(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEditcond)
	enum { IDD = IDD_EDITCOND };
	//}}AFX_DATA
	Jcond	m_cond;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditcond)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditcond)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
