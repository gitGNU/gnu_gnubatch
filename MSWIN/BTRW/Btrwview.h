// btrwview.h : interface of the CBtrwView class
//
/////////////////////////////////////////////////////////////////////////////

class CBtrwView : public CView
{
protected: // create from serialization only
	CBtrwView();
	DECLARE_DYNCREATE(CBtrwView)

// Attributes
public:
	CBtrwDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtrwView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBtrwView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CBtrwView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in btrwview.cpp
inline CBtrwDoc* CBtrwView::GetDocument()
   { return (CBtrwDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
