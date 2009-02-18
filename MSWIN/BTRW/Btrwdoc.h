// btrwdoc.h : interface of the CBtrwDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CBtrwDoc : public CDocument
{
protected: // create from serialization only
	CBtrwDoc();
	DECLARE_DYNCREATE(CBtrwDoc)

// Attributes
public:
	Btjob	m_currjob;
	CString	m_jobfile;

// Operations
public:
	void	CheckChanges();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtrwDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBtrwDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CBtrwDoc)
	afx_msg void OnActionDeletejob();
	afx_msg void OnActionSubmitjob();
	afx_msg void OnJobsEditjob();
	afx_msg void OnActionSetrunnable();
	afx_msg void OnActionSetcancelled();
	afx_msg void OnJobsTime();
	afx_msg void OnActionTitlepriloadlev();
	afx_msg void OnJobsProcparam();
	afx_msg void OnActionPermissions();
	afx_msg void OnJobsMail();
	afx_msg void OnJobsArguments();
	afx_msg void OnJobsEnvironment();
	afx_msg void OnJobsRedirections();
	afx_msg void OnConditionsJobconditions();
	afx_msg void OnConditionsJobassignments();
	afx_msg void OnFileOpen();
	afx_msg void OnActionCopyjobfile();
	afx_msg void OnEnvironmentClearenvironment();
	afx_msg void OnEnvironmentEnvironmentdefault();
	afx_msg void OnEnvironmentSqueezeenvironment();
	afx_msg void OnJobsTimelimits();
	afx_msg void OnFileUserpermissions();
	afx_msg void OnActionSetdone();
	afx_msg void OnDefaultsTime();
	afx_msg void OnDefaultsTitlepriloadlev();
	afx_msg void OnDefaultsMail();
	afx_msg void OnDefaultsProcparam();
	afx_msg void OnDefaultsTimelimits();
	afx_msg void OnDefaultsArguments();
	afx_msg void OnDefaultsRedirections();
	afx_msg void OnDefaultsConditions();
	afx_msg void OnDefaultsAssignments();
	afx_msg void OnDefaultsNewconditions();
	afx_msg void OnDefaultsNewassignments();
	afx_msg void OnDefaultsPermissions();
	afx_msg void OnDefaultsSetrunnable();
	afx_msg void OnDefaultsSetcancelled();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
