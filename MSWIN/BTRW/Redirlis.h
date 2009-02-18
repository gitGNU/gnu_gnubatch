// redirlis.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRedirlist dialog

class CRedirlist : public CDialog
{
// Construction
public:
	CRedirlist(CWnd* pParent = NULL);	// standard constructor 
	~CRedirlist();

// Dialog Data
	//{{AFX_DATA(CRedirlist)
	enum { IDD = IDD_REDIRLIST };
	CDragListBox	m_dragredir;
	//}}AFX_DATA
	unsigned  m_nredirs;
	Mredir	  *m_redirs;
	unsigned  m_maxredirs;
	int		  m_changes;
	BOOL	  m_writeable;
	
private:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRedirlist)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRedirlist)
	afx_msg void OnClickedNewredir();
	afx_msg void OnClickedEditredir();
	afx_msg void OnClickedDelredir();
	afx_msg void OnDblclkRedirlist();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
