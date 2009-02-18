// btrsevw.h : interface of the CBtrsetwView class
//
/////////////////////////////////////////////////////////////////////////////

class CBtrsetwView : public CScrollView
{
protected: // create from serialization only
	CBtrsetwView();
	DECLARE_DYNCREATE(CBtrsetwView)

// Attributes
public:
	int	m_nCharWidth;			// width of characters
	int m_nRowWidth;            // width of row in current device units
	int m_nRowHeight;           // height of row in current device units
	int m_nSelectedRow;			// record of selected row
	CBtrsetwDoc* GetDocument();

// Operations
public:
	void 	OnDrawRow(CDC*, int, int, BOOL);
	int		GetActiveRow()	{	return  m_nSelectedRow;	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtrsetwView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void	OnUpdate(CView*, LPARAM = 0L, CObject* = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBtrsetwView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
      
private:
	void	UpdateScrollSizes();	

// Generated message map functions
protected:
	//{{AFX_MSG(CBtrsetwView)
	afx_msg void OnNetworkAddnewhost();
	afx_msg void OnNetworkChangehost();
	afx_msg void OnNetworkDeletehost();
	afx_msg void OnNetworkSetasserver();
	afx_msg void OnOptionsProgramdefaults();
	afx_msg void OnOptionsConditiondefaults();
	afx_msg void OnOptionsAssignmentdefaults();
	afx_msg void OnOptionsTimedefaults();
	afx_msg void OnOptionsQueuedefaults();
	afx_msg void OnOptionsProcessparameters();
	afx_msg void OnOptionsRestoredefaults();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnOptionsPermissions();
	afx_msg void OnOptionsTimelimits();
	afx_msg void OnProgramPortsettings();
	afx_msg void OnProgramLoginorlogout();
	afx_msg void OnProgramNewgroup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG	// debug version in btrsevw.cpp
inline CBtrsetwDoc* CBtrsetwView::GetDocument()
   { return (CBtrsetwDoc*) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
