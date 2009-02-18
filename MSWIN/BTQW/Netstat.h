// netstat.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNetstat dialog

class CNetstat : public CDialog
{
// Construction
public:
	CNetstat(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CNetstat)
	enum { IDD = IDD_NETSTATS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetstat)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNetstat)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeHost();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
