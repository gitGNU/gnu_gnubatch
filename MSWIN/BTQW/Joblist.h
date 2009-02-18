#define	INITJOBS		10		// Initial space to reserve
#define	INCJOBS			5		// Space for more

class	joblist	{
private:
	unsigned  njobs;
	unsigned  maxjobs;
	Btjob	  **list;
	void	  checkgrow();
	void	  procjchange(Btjob &, const int, const unsigned);
public:
	joblist()	{ njobs = maxjobs = 0; list = NULL;	}
	~joblist()	{ if  (list)  delete [] list; }
	unsigned	number()	const  {  return  njobs;	}
	int		jindex(const jident &)  const;
	Btjob	*operator [] (const unsigned)  const;
	Btjob	*operator [] (const jident &)  const;
	void	append(Btjob *);
	void	insert(Btjob *, const unsigned);
	void	remove(const unsigned);
	int		move(const unsigned, const unsigned);
	void	clear() { njobs = 0; }
// Network ops
	void	net_jclear(const netid_t);
	void	createdjob(const Btjob &);
	void	changedjobhdr(const Btjob &);	//  None of the strings
	void	changedjob(const Btjob &);                             
	void	boqjob(const jident &);
	void	forced(const jident &, const int);
	void	deletedjob(const jident &);
	void	deskel_jobs(const netid_t);
	void	chmogedjob(const Btjob &);
	void	remassjob(const jident &, const unsigned, const unsigned);
	void	statchjob(const jident &, const netid_t, const long, const long, const unsigned short, const unsigned char);
// "Transmit" sort of network ops
	void	changejob(const Btjob &);
	void	chogjob(const jident &, const unsigned, const CString &);
	void	chmodjob(const Btjob &);
	void	killjob(const jident &, const int);
	void	deletejob(const jident &);
	void	forcejob(const jident &, const int);
};

class  jobqueuelist	{
private:
	unsigned  index, number;
	CString	 *list;
public:
	jobqueuelist();
	~jobqueuelist();
	CString	*next();
};

		