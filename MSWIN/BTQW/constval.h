// constval.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Cconstval dialog

class Cconstval : public CDialog
{
// Construction
public:
	Cconstval(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(Cconstval)
	enum { IDD = IDD_CONSTVAL };
	long	m_constval;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Cconstval)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Cconstval)
	afx_msg void OnDeltaposScrConstvalue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
