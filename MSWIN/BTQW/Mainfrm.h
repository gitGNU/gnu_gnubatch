// mainfrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	CMultiDocTemplate	*m_dtjob;
	CMultiDocTemplate	*m_dtvar;
	CMultiDocTemplate	*m_dtjdata;
	CString				m_jlastsearch;
	CString				m_vlastsearch;

// Operations

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

protected:	// control bar embedded members
	CStatusBar	m_wndStatusBar;
	CToolBar	m_wndToolBar;
                     
public:                   
	void InitWins();
	void OnNewJDWin(Btjob *);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowNewjobwindow();
	afx_msg void OnFileProgramoptions();
	afx_msg LRESULT OnNMArrived(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMNewconn(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMProbercv(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMMessrcv(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFileSavefile();
	afx_msg void OnWindowNewvarwindow();
	afx_msg void OnUpdateFileSavefile(CCmdUI* pCmdUI);
	afx_msg void OnFileTimedefaults();
	afx_msg void OnFileConditiondefaults();
	afx_msg void OnFileAssignmentdefaults();
	afx_msg void OnWindowJobdisplayformat();
	afx_msg void OnWindowVardisplayfmtline1();
	afx_msg void OnFileNetworkstats();
	afx_msg void OnFileUserperms();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnJcolourAbort();
	afx_msg void OnJcolourCancelled();
	afx_msg void OnJcolourError();
	afx_msg void OnJcolourFinished();
	afx_msg void OnJcolourReady();
	afx_msg void OnJcolourRunning();
	afx_msg void OnJcolourStartup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void RunColourDlg(const unsigned);
};

/////////////////////////////////////////////////////////////////////////////
