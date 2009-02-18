// cpyoptdl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCpyOptdlg dialog

class CCpyOptdlg : public CDialog
{
// Construction
public:
	CCpyOptdlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCpyOptdlg)
	enum { IDD = IDD_COPYJOPTS };
	int		m_ascanc;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCpyOptdlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCpyOptdlg)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
