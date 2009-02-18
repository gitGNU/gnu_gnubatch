// othersig.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COthersig dialog

class COthersig : public CDialog
{
// Construction
public:
	COthersig(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(COthersig)
	enum { IDD = IDD_OTHERSIG };
	int		m_sigtype;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COthersig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COthersig)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
