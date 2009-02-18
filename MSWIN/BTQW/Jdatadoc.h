#define	JD_LHINT_DOC	0L

class CJdatadoc : public CDocument
{
	DECLARE_DYNCREATE(CJdatadoc)
protected:
	CJdatadoc();			// protected constructor used by dynamic creation

// Attributes
	Btjob	jcopy;				// copy of job (make this a pointer???)
	CFile	jobfile;			// temporary file for job
	CString	tname;				// temporary file name
	unsigned	charwidth, charheight;	//  Size of job in chars
	long	offset;   			// Offset in file
	unsigned	cline;			// Line number 0 based
public:
	BOOL	m_invalid;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJdatadoc)
	public:
	//}}AFX_VIRTUAL

// Implementation

// Operations
public:
	void  setjob(const Btjob &j) { jcopy = j;	}
	int	  loaddoc();                                       
	unsigned  jdwidth()		const	{	return  charwidth;	}
	unsigned  jdheight()	const	{	return  charheight;	}
	UINT  RowLength(const long);
	long  Locrow(const unsigned);
	char  *FindRow(const int);
	char  *GetRow(const int, char *);

// Implementation

public:
	virtual ~CJdatadoc();

	// Generated message map functions
protected:
	//{{AFX_MSG(CJdatadoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

