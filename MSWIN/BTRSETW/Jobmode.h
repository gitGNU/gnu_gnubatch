// jobmode.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJobmode dialog

class CJobmode : public CDialog
{
// Construction
public:
	CJobmode(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CJobmode)
	enum { IDD = IDD_JOBMODE };
	CString	m_user;
	CString	m_group;
	//}}AFX_DATA
	unsigned short m_umode, m_gmode, m_omode;
	BOOL	m_writeable;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJobmode)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJobmode)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickedJuread();
	afx_msg void OnClickedJuwrite();
	afx_msg void OnClickedJureveal();
	afx_msg void OnClickedJudispmode();
	afx_msg void OnClickedJusetmode();
	afx_msg void OnClickedJgread();
	afx_msg void OnClickedJgwrite();
	afx_msg void OnClickedJgreveal();
	afx_msg void OnClickedJgdispmode();
	afx_msg void OnClickedJgsetmode();
	afx_msg void OnClickedJoread();
	afx_msg void OnClickedJowrite();
	afx_msg void OnClickedJoreveal();
	afx_msg void OnClickedJodispmode();
	afx_msg void OnClickedJosetmode();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
