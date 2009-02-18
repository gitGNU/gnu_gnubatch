// defass.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Cdefass dialog

class Cdefass : public CDialog
{
// Construction
public:
	Cdefass(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(Cdefass)
	enum { IDD = IDD_DEFASS };
	BOOL	m_astart;
	BOOL	m_areverse;
	BOOL	m_anormal;
	BOOL	m_aerror;
	BOOL	m_aabort;
	BOOL	m_acancel;
	int		m_asscrit;
	int		m_asstype;
	CString	m_assval;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Cdefass)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Cdefass)
	afx_msg void OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
