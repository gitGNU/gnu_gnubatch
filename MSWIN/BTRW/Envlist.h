// envlist.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEnvlist dialog

class CEnvlist : public CDialog
{
// Construction
public:
	CEnvlist(CWnd* pParent = NULL);	// standard constructor
	~CEnvlist();

// Dialog Data
	//{{AFX_DATA(CEnvlist)
	enum { IDD = IDD_ENVLIST };
	CDragListBox	m_dragenv;
	//}}AFX_DATA
	unsigned  m_nenvs;
	Menvir	  *m_envs;
	unsigned  m_maxenvs;
	int		  m_changes;
	BOOL	m_writeable;
	
private:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnvlist)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEnvlist)
	afx_msg void OnClickedNewenv();
	afx_msg void OnClickedEditenv();
	afx_msg void OnClickedDelenv();
	afx_msg void OnDblclkEnvlist();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
