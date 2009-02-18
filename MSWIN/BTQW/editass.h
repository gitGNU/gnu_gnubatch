// editass.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditass dialog

class CEditass : public CDialog
{
// Construction
public:
	CEditass(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEditass)
	enum { IDD = IDD_EDITASS };
	//}}AFX_DATA
	Jass	m_ass;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditass)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	Enablemarks();

	// Generated message map functions
	//{{AFX_MSG(CEditass)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedAset();
	afx_msg void OnClickedAadd();
	afx_msg void OnClickedAsub();
	afx_msg void OnClickedAmult();
	afx_msg void OnClickedAdiv();
	afx_msg void OnClickedAmod();
	afx_msg void OnClickedAexit();
	afx_msg void OnClickedAsignal();
	afx_msg void OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
