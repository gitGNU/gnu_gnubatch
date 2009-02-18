class CJobView : public CRowView
{
	DECLARE_DYNCREATE(CJobView)
public:
	CJobView();
                         
private:
	jident	curr_job;
                         
// Attributes
public:
	CJobdoc* GetDocument()  {
		ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CJobdoc)));
		return (CJobdoc*) m_pDocument;
	}

// Overrides of CView
	void OnUpdate(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);

// Overrides of CRowView
	void       GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight);
	int        GetActiveRow();
	unsigned   GetRowCount();
	void       OnDrawRow(CDC* pDC, int nRowNo, int y, BOOL bSelected);
	void       ChangeSelectionToRow(int nRow);
    void	   JobAllChange(const BOOL);
	void	   JobChgJob(const unsigned);
	void	   Initformats();
	void	   SaveWidthSettings();
	void	   Dopopupmenu(CPoint pos);

// Implementation
protected:
	virtual ~CJobView() {}
};
