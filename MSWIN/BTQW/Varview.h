class CVarView : public CRowView
{
	DECLARE_DYNCREATE(CVarView)
public:
	CVarView();
                         
private:
	vident	curr_var;
                         
// Attributes
public:
	CVardoc* GetDocument()  {
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVardoc)));
		return (CVardoc*) m_pDocument;
	}

// Overrides of CView
	void OnUpdate(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);

// Overrides of CRowView
	void       GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight);
	int        GetActiveRow();
	unsigned   GetRowCount();
	void       OnDrawRow(CDC* pDC, int nRowNo, int y, BOOL bSelected);
	void       ChangeSelectionToRow(int nRow);
	void	   VarAllChange(const BOOL);
	void	   VarChgVar(const unsigned);
	void	   Initformats();
	void	   SaveWidthSettings();
	void	   Dopopupmenu(CPoint pos);

// Implementation
protected:
	virtual ~CVarView() {}
};
