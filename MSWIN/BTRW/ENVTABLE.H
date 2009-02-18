//  Copy of Environment table from Unix host

class	envtable	{
	unsigned	numenvs, maxenvs;
	unsigned	ptr;
	char		**namelist;
	char		**vallist;
public:
	envtable()	{ numenvs = 0; namelist = vallist = NULL; }
	void		add_unixenv(const char *);
	const  char	*unix_getenv(const char *);
	unsigned	number() { return numenvs; }
	void		init_envlist() { ptr = 0; }
	int			next(const char *&n, const char *&v) {
		if (ptr >= numenvs) return 0;
		n = namelist[ptr];
		v = vallist[ptr];
		ptr++;
		return  1;
	}
};

		