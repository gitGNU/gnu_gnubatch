// jcondlis.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJcondlist dialog

class CJcondlist : public CDialog
{
// Construction
public:
	CJcondlist(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CJcondlist)
	enum { IDD = IDD_JCOND };
	CDragListBox	m_dragcond;
	//}}AFX_DATA
	int		m_changes;
	BOOL	m_writeable;
	unsigned  short	m_nconds;
	Jcond	m_jconds[MAXCVARS];
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJcondlist)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJcondlist)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedNewcond();
	afx_msg void OnClickedEditcond();
	afx_msg void OnClickedConddel();
	afx_msg void OnDblclkCondlist();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
