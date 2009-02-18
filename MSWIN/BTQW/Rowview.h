/////////////////////////////////////////////////////////////////////////////
// CRowView view

class CRowView : public CScrollView
{
	DECLARE_DYNAMIC(CRowView)
protected:
	CRowView();			// protected constructor used by dynamic creation

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRowView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Attributes
public:
	int m_nRowWidth;            // width of row in current device units
	int m_nRowHeight;           // height of row in current device units
	int m_nPrevRowCount;		// record of number of rows
	int m_nSelectedRow;			// record of selected row
	int	m_avecharwidth;			// average char width
	int	m_nformats;				// number of formats
	BOOL	m_formathacked;		// formats hacked

	CHeaderCtrl	m_header;
	Formatrow	m_formats[MAXFORMATS];

// Operations
public:
	virtual void UpdateRow(int nInvalidRow);    // called by derived class's

// Overridables
protected:
	virtual void     GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight) = 0;
	virtual int      GetActiveRow() = 0;
	virtual unsigned GetRowCount() = 0;
	virtual void     OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected) = 0;
	virtual void     ChangeSelectionToRow(int nRow) = 0;

	virtual void	JobAllChange(const BOOL = FALSE);
	virtual void	JobChgJob(const unsigned);
	virtual void	VarAllChange(const BOOL = FALSE);
	virtual	void	VarChgVar(const unsigned);

	virtual void	Initformats() = 0;
	virtual	void	SaveWidthSettings() = 0;
	virtual void	Dopopupmenu(CPoint pos) = 0;

// Implementation
protected:
	virtual ~CRowView();
	virtual	void OnDraw(CDC* pDC);		// overridden to draw this view
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual	void OnInitialUpdate();		// first time after construct
	virtual void CalculateRowMetrics(CDC* pDC)
		{ GetRowWidthHeight(pDC, m_nRowWidth, m_nRowHeight); }
	virtual void UpdateScrollSizes();
	virtual CRect RowToWndRect(CDC* pDC, int nRow);
	virtual int RowToYPos(int nRow);
	virtual void RectLPtoRowRange(const CRect& rectLP, 
			int& nFirstRow, int& nLastRow, BOOL bIncludePartiallyShownRows);
	virtual int LastViewableRow();
	void	RedrawRow(const int);
	void	Initformats(CString CBtqwApp::*, int (*const*)(...), int (*const*)(...), const UINT, const UINT, const char *);
	void	SetUpHdr();
	int		GetFmtIndex(const int, int &);

	// Generated message map functions
	//{{AFX_MSG(CRowView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnNMJadd(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMJchange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMJdel(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMJrevised(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMVadd(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMVchange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMVDel(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNMVrevised(WPARAM wParam, LPARAM lParam);
    afx_msg void OnEndTrack(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBdrDblClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnWindowSavewidthsasdefault();
	afx_msg void OnUpdateWindowSavewidthsasdefault(CCmdUI* pCmdUI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
