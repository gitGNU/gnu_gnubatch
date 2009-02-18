// portnums.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPortnums dialog

class CPortnums : public CDialog
{
// Construction
public:
	CPortnums(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CPortnums)
	enum { IDD = IDD_PORTNUMS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPortnums)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:

	// Generated message map functions
	//{{AFX_MSG(CPortnums)
	virtual BOOL OnInitDialog();
	afx_msg void OnSavesettings();
	afx_msg void OnSetdefault();
	afx_msg void OnApplyinc();
	afx_msg void OnApplydec();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
