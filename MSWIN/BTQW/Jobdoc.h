class CJobdoc : public CDocument
{
	DECLARE_DYNCREATE(CJobdoc)
protected:
	CJobdoc();			// protected constructor used by dynamic creation

// Job list we are supporting and are interested in

	joblist		m_joblist;		//  List of jobs in window
public:
	restrictdoc	m_wrestrict;	//  Restricted view
	BOOL	m_sjtitle, m_suser, m_swraparound;
	CJobcolours	m_jobcolours;

public:                        
	void	revisejobs(joblist &);
	unsigned	number()	{  return  m_joblist.number();	}
	int		jindex(const jident &ind) { return m_joblist.jindex(ind); }
	Btjob	*operator [] (const unsigned ind) { return m_joblist[ind]; }
	Btjob	*operator [] (const jident &ind) { return m_joblist[ind];  }
 		
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJobdoc)
	public:
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CJobdoc();
	Btjob	*GetSelectedJob(const unsigned = BTM_READ, const BOOL notifrun = FALSE, const BOOL = TRUE);
	void	DoSearch(const CString, const BOOL);
	BOOL	Smatches(const CString, const int);

	// Generated message map functions
protected:
	//{{AFX_MSG(CJobdoc)
	afx_msg void OnSearchSearchbackwards();
	afx_msg void OnSearchSearchfor();
	afx_msg void OnSearchSearchforward();
	afx_msg void OnActionDeletejob();
	afx_msg void OnActionSetcancelled();
	afx_msg void OnActionForcerun();
	afx_msg void OnActionKilljob();
	afx_msg void OnActionOthersignal();
	afx_msg void OnActionPermissions();
	afx_msg void OnActionQuitjob();
	afx_msg void OnActionTitlepriloadlev();
	afx_msg void OnJobsArguments();
	afx_msg void OnJobsEnvironment();
	afx_msg void OnJobsMail();
	afx_msg void OnJobsProcparam();
	afx_msg void OnJobsRedirections();
	afx_msg void OnJobsTime();
	afx_msg void OnJobsViewjob();
	afx_msg void OnConditionsJobassignments();
	afx_msg void OnConditionsJobconditions();
	afx_msg void OnActionGojob();
	afx_msg void OnActionSetrunnable();
	afx_msg void OnWindowWindowoptions();
	afx_msg void OnJobsAdvancetime();
	afx_msg void OnJobsUnqueuejob();
	afx_msg void OnJobsCopyjob();
	afx_msg void OnJobsCopyoptionsinjob();
	afx_msg void OnJobsInvokebtrw();
	afx_msg void OnJobsTimelimits();
	afx_msg void OnWjcolourAbort();
	afx_msg void OnWjcolourCancelled();
	afx_msg void OnWjcolourError();
	afx_msg void OnWjcolourFinished();
	afx_msg void OnWjcolourReady();
	afx_msg void OnWjcolourRunning();
	afx_msg void OnWjcolourStartup();
	afx_msg void OnFileJcolourCopytoprog();
	afx_msg void OnFileJcolourCopytowin();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void RunColourDlg(const unsigned n);
};
