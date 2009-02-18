// editredi.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditredir dialog

class CEditredir : public CDialog
{
// Construction
public:
	CEditredir(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEditredir)
	enum { IDD = IDD_EDITREDIR };
	//}}AFX_DATA
	Mredir	m_redir;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditredir)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	Enableitems();
	void	Enablefile(const BOOL);
	void	Enablefd2(const BOOL);

	// Generated message map functions
	//{{AFX_MSG(CEditredir)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedCfile();
	afx_msg void OnClickedPipeto();
	afx_msg void OnClickedPipefrom();
	afx_msg void OnClickedClosefile();
	afx_msg void OnClickedDupfrom();
	afx_msg void OnClickedFread();
	afx_msg void OnClickedFwrite();
	afx_msg void OnClickedFappend();
	virtual void OnOK();
	afx_msg void OnChangeFd();
	afx_msg void OnChangeFd2();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
