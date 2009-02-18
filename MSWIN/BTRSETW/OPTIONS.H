//  Program default options

class  progdefs	{
public:
	CString		user;
	CString		group;
	CString		queuename;
	BOOL		verbose;
	BOOL		binmode;
};

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

class  queuedefs {
public:
	enum  Advterr { ADV, NOADV };
	enum  Exportflg { LOCAL, EXPORTF, REMRUN };
	enum  Progress { RUNNABLE, CANCELLED };
	CString	m_title;
	CString	m_cmdinterp;
	CString	m_directory;
	UINT	m_priority;
	UINT	m_loadlev;
	UINT	m_umask;
	UINT	m_deltime;
	UINT	m_autoksig;
	UINT	m_runon;
	unsigned  long	m_ulimit;
	unsigned  long	m_runtime;
	Exits	m_exits;
	Exportflg	m_export;
	Advterr	m_advterr;                     
	Progress m_progress;
	Btmode	m_mode;
};	
	