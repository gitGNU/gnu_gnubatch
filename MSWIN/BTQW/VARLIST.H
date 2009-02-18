#define	INITVARS	20		// Initial space to reserve
#define	INCVARS		5		// Space for more

class	varlist	{
 private:
	unsigned	nvars;		// Number currently known
	unsigned	maxvars;	// Number allocated
	Btvar		**sortlist;	// Vector of pointers to structures
	Btvar		**list;		// Historical vector
	void		checkgrow();
 public:
	varlist()	{	nvars = maxvars = 0;  list = sortlist = NULL;	}
	~varlist()	{	if (list)	{ delete [] sortlist; delete [] list;	}}
	unsigned	number()  const	{	return  nvars;	}
	int			vindex(const vident &)  const;
	int			vsortindex(const vident &)  const;
	Btvar		*operator [] (const unsigned)  const;
	Btvar		*sorted(const unsigned) const;
	void		append(Btvar *);
	void		remove(const unsigned);
	void		clear()	{	nvars = 0;	}
// Network operations
	void		net_vclear(const netid_t);
	void		createdvar(const Btvar &);
	void		deletedvar(const vident &);
	void		assignedvar(const Btvar &);
	void		chmogedvar(const Btvar &);
	void		renamedvar(const vident &, const char *);
// Transmit ops
	void		assignvar(const Btvar &);
	void		chcommvar(const Btvar &);
	void		chogvar(const vident &, const unsigned, const CString &);
	void		chmodvar(const Btvar &);
};

//  Declare this here I suppose....

void	jopvar(const unsigned, const Btcon &, const unsigned);

class	listvar	{
private:
	unsigned  short	perm;
	unsigned		posn;
public:
	listvar(const unsigned short p) : perm(p) { posn = 0; }
	int	next();
};
		