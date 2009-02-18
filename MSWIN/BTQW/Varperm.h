// varperm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVarperm dialog

class CVarperm : public CDialog
{
// Construction
public:
	CVarperm(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVarperm)
	enum { IDD = IDD_VARMODE };
	CString	m_user;
	CString	m_group;
	//}}AFX_DATA
	unsigned short m_umode, m_gmode, m_omode;
	BOOL	m_writeable;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVarperm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVarperm)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedVuread();
	afx_msg void OnClickedVuwrite();
	afx_msg void OnClickedVureveal();
	afx_msg void OnClickedVudispmode();
	afx_msg void OnClickedVusetmode();
	afx_msg void OnClickedVgread();
	afx_msg void OnClickedVgwrite();
	afx_msg void OnClickedVgreveal();
	afx_msg void OnClickedVgdispmode();
	afx_msg void OnClickedVgsetmode();
	afx_msg void OnClickedVoread();
	afx_msg void OnClickedVoreveal();
	afx_msg void OnClickedVodispmode();
	afx_msg void OnClickedVosetmode();
	afx_msg void OnClickedVowrite();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
