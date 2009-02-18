// btrsedoc.h : interface of the CBtrsetwDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CBtrsetwDoc : public CDocument
{
protected: // create from serialization only
	CBtrsetwDoc();
	DECLARE_DYNCREATE(CBtrsetwDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtrsetwDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBtrsetwDoc();
#ifdef _DEBUG
	virtual	void AssertValid() const;
	virtual	void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CBtrsetwDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
