// jasslist.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJasslist dialog

class CJasslist : public CDialog
{
// Construction
public:
	CJasslist(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CJasslist)
	enum { IDD = IDD_JASS };
	CDragListBox	m_dragass;
	//}}AFX_DATA
	int		m_changes;
	BOOL	m_writeable;
	unsigned  short	m_nasses;
	Jass	m_jasses[MAXSEVARS];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJasslist)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJasslist)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedNewass();
	afx_msg void OnClickedEditass();
	afx_msg void OnClickedAssdel();
	afx_msg void OnDblclkAsslist();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
