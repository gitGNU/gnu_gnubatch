//	Restrict view of jobs and vars in particular windows
//	Write to allow room for expansion...

class	restrictdoc	{
public:
	enum  ELocalonly { NO, YES };
	enum  EConfdel	 { NEVER, ALWAYS };	//  Not restriction but fits
public:
	CString		user;
	CString		group;
	CString		queuename;
	BOOL		incnull;
	ELocalonly	onlyl;
	EConfdel	confd;
	restrictdoc();
	int		visible(const Btjob &);
	int		visible(const Btvar &);
};

//  Time default options

class  timedefs	{
public:
	int		m_nptype;
	int		m_repeatopt;
	BOOL	m_avsun;
	BOOL	m_avmon;
	BOOL	m_avtue;
	BOOL	m_avwed;
	BOOL	m_avthu;
	BOOL	m_avfri;
	BOOL	m_avsat;
	BOOL	m_avhol;
	unsigned  char  m_tc_mday;	// Month day
	unsigned  long	m_tc_rate;	// Repeat interval
};		

class  conddefs	{
public:
	CString	m_constval;
	int		m_condop;
	int		m_condcrit;
};

class  assdefs {
public:
	CString	m_assvalue;
	int		m_asstype;
	int		m_asscrit;
	BOOL	m_astart;
	BOOL	m_areverse;
	BOOL	m_anormal;
	BOOL	m_aerror;
	BOOL	m_aabort;
	BOOL	m_acancel;
};
	