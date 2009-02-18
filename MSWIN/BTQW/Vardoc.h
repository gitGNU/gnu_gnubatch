// vardoc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVardoc document

class CVardoc : public CDocument
{
	DECLARE_DYNCREATE(CVardoc)
protected:
	CVardoc();			// protected constructor used by dynamic creation

// Var list we are supporting and are interested in

	varlist		m_varlist;		//  List of vars in window
	
public:
	restrictdoc	m_wrestrict;	//  Restricted view
	BOOL	m_suser, m_svname, m_svcomment, m_svvalue, m_swraparound;
	long	m_constvalue;

public:                        
	void	revisevars(varlist &);
	unsigned	number()	{  return  m_varlist.number();	}
	int		vindex(const vident &ind) { return m_varlist.vindex(ind); }
	int		vsortindex(const vident &ind) { return m_varlist.vsortindex(ind); }
	Btvar	*operator [] (const unsigned ind) { return m_varlist[ind]; }
	Btvar	*operator [] (const vident &ind) { return m_varlist[vindex(ind)];  }
	Btvar	*sorted(const unsigned ind) { return m_varlist.sorted(ind); }
	Btvar	*sorted(const vident &ind)	{ return m_varlist.sorted(vsortindex(ind));	}
 		
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVardoc)
	public:
	//}}AFX_VIRTUAL

// Implementation

protected:
	virtual ~CVardoc();
	Btvar	*GetSelectedVar(const unsigned = BTM_WRITE, const BOOL = TRUE);
	Btvar	*GetArithVar();
	void	DoSearch(const CString, const BOOL);
	BOOL	Smatches(const CString, const int);
// Attributes
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(CVardoc)
	afx_msg void OnActionPermissions();
	afx_msg void OnSearchSearchbackwards();
	afx_msg void OnSearchSearchfor();
	afx_msg void OnSearchSearchforward();
	afx_msg void OnVariablesAssign();
	afx_msg void OnVariablesDecrement();
	afx_msg void OnVariablesDivide();
	afx_msg void OnVariablesIncrement();
	afx_msg void OnVariablesMultiply();
	afx_msg void OnVariablesRemainder();
	afx_msg void OnVariablesSetcomment();
	afx_msg void OnVariablesSetconstant();
	afx_msg void OnWindowWindowoptions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

