// fmtdef.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFmtdef dialog

class CFmtdef : public CDialog
{
// Construction
public:
	CFmtdef(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CFmtdef)
	enum { IDD = IDD_FORMATDEF };
	CDragListBox	m_dragformat;
	//}}AFX_DATA
	CString	m_fmtstring;
	UINT	m_defcode, m_what4, m_uppercode, m_lowercode;
	int		m_changes;
	int		m_numformats;
	Formatrow	m_formats[MAXFORMATS];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFmtdef)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void	Decodeformats();
	void	Addformat(const Formatrow &);
	
protected:

	// Generated message map functions
	//{{AFX_MSG(CFmtdef)
	afx_msg void OnDblclkFmtlist();
	afx_msg void OnNewfmt();
	afx_msg void OnEditfmt();
	afx_msg void OnDelfmt();
	afx_msg void OnNewsep();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnResetdeflt();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
