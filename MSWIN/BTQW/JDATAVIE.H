// jdatavie.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJdataview view

class CJdataview : public CScrollView
{
	DECLARE_DYNCREATE(CJdataview)
protected:
	CJdataview();			// protected constructor used by dynamic creation

// Attributes
public:     
	int  m_nRowHeight, m_nRowWidth, m_nCharWidth;
	char	*m_buffer;		// Buffer for current row
	CString	m_lastlook;
	BOOL	m_ignorecase;
	BOOL	m_wrapround;
    
// Operations
public:	                                              
	char  *GetRow(const int row) { return ((CJdatadoc *)GetDocument())->GetRow(row, m_buffer); }

// Implementation

private:
	unsigned	GetNumRows();
	unsigned	GetNumCols();
	void	UpdateScrollSizes();
	int	 	matches(char *, CString, BOOL);
	int		lookback(const int, const int, int &);
	int		lookforw(const int, const int, int &);
	void	ExecuteSearch(const int = 0);

protected:
	virtual ~CJdataview();
	virtual	void OnDraw(CDC* pDC);		// overridden to draw this view
	virtual	void OnInitialUpdate();		// first time after construct
	virtual void OnUpdate(CView *, LPARAM, CObject *);

	// Generated message map functions
	//{{AFX_MSG(CJdataview)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSearchSearchforward();
	afx_msg void OnSearchSearchbackwards();
	afx_msg void OnSearchSearchfor();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
