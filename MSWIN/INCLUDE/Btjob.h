/* btjob.h -- job structure

   Copyright 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

struct	Envir  { 		//  One wot gets passed across
	unsigned  short	e_name;			// Offset of name
	unsigned  short	e_value;		// Offset of value
};

struct	Menvir  {
	CString		e_name;
	CString		e_value;
	Menvir  &operator = (const Menvir &c) { e_name = c.e_name; e_value = c.e_value; return *this; }
	int operator != (const Menvir &t)
		{
			return e_name != t.e_name ||  e_value != t.e_value;
		}
};

#define	RD_ACT_RD		1		// Open for reading
#define	RD_ACT_WRT		2		// Open/create for writing
#define	RD_ACT_APPEND	3		// Open/create and append write only
#define	RD_ACT_RDWR		4		// Open/create for read/write
#define	RD_ACT_RDWRAPP	5		// Open/create/read/write/append
#define	RD_ACT_PIPEO	6		// Pipe out
#define	RD_ACT_PIPEI	7		// Pipe in
#define	RD_ACT_CLOSE	8		// Close it (no file)
#define	RD_ACT_DUP		9		// Duplicate file descriptor given

struct	Redir  {
	unsigned  char	fd;
	unsigned  char	action;
	unsigned  short	arg;
};

struct	Mredir  {
	unsigned  char	fd;
	unsigned  char	action;
	unsigned  short	arg;
	CString			buffer;
	Mredir	&operator = (const Mredir &t)
	{
		fd = t.fd;
		action = t.action;
		arg = t.arg;
		buffer = t.buffer;
		return  *this;
	}	
	int operator != (const Mredir &t)
	{
		return fd != t.fd ||
			   action != t.action ||
			   (action < RD_ACT_CLOSE && buffer != t.buffer) ||
			   (action == RD_ACT_DUP && arg != t.arg);
	}
};

class  jident	{
public:
	netid_t		hostid;	// Originating host id - never zero in Win vn
	slotno_t	slotno;	// SHM slot number on machine
	int operator == (const jident &oth) { return hostid == oth.hostid && slotno == oth.slotno;  }
	jident()	{ hostid = slotno = 0L; }
};

struct	Jcond  {
	unsigned char	bjc_compar;
	unsigned char	bjc_iscrit;
#ifdef	BTRW
	CString			bjc_var;
#else
	unsigned		bjc_varind;
#endif
	Btcon			bjc_value;
};

struct	Jass  {
	unsigned  short	bja_flags;
	unsigned  char	bja_op;
	unsigned  char	bja_iscrit;
#ifdef	BTRW
	CString			bja_var;
#else
	unsigned		bja_varind;
#endif
	Btcon			bja_con;
};

class  Btjob :  public  jident {
public:
	jobno_t		bj_job;			//  Job number
	time_t		bj_time;		//  When originally submitted
	time_t		bj_stime;		//  When originally submitted
	time_t		bj_etime;		//  When originally submitted
	long		bj_pid;			//  Process id
	netid_t		bj_orighostid;	//  Where it came from
	netid_t		bj_runhostid; 	//  Host id job running on

	unsigned  char	bj_progress;
	unsigned  char  bj_pri;		//  Priority
	short		bj_wpri;		//  Working priority
	unsigned short	bj_ll;		//  Load level
	unsigned short	bj_umask;
	unsigned short	bj_nredirs,	//  Number of redirections
			bj_nargs,	//  Number of arguments
			bj_nenv;	//  Environment var number
		
	unsigned  char	bj_jflags;	//  Now a byte
	unsigned  char  bj_jrunflags;	//  Only for BJ_SKELHOLD

	CString		bj_title;	//  Name of job constructed
	CString		bj_direct;	//  Directory constructed

	unsigned long	bj_runtime;	//  Run limit (secs)
	unsigned short	bj_autoksig;	//  Auto kill sig before size 9s applied
	unsigned short	bj_runon;	//  Grace period (secs)
	unsigned short	bj_deltime;	//  Delete job automatically
	unsigned short	bj_lastexit;	//  Last exit status (as returned by wait)

	CString		bj_cmdinterp;	//  Command interpreter
	
	Btmode		bj_mode;
	Jcond		bj_conds[MAXCVARS];
	Jass		bj_asses[MAXSEVARS];
	Timecon		bj_times;

	long		bj_ulimit;
	Exits		bj_exits;

	Mredir			*bj_redirs;	// Redirections
	Menvir			*bj_env;	// Environment
	CString			*bj_arg;	// Max args
	
	Btjob();
	~Btjob();
	Btjob(Btjob &);
	Btjob	&operator = (const Btjob &);
#ifdef	BTQW
	void	unqueue(const BOOL = TRUE);
	void	optcopy();
#endif
#ifdef	BTRW
	void	init_job();
	BOOL	loadoptions(CFile *, CString &);
	BOOL	ParseBtrArgs(const char FAR *, CString &);
	BOOL	saveoptions(CFile *, const CString &);
	BOOL	submitjob(jobno_t &, const CString &);
	void	optcopy();
	void	optread();
private:
	int		packjob(nijobmsg &, const CString &) const;
	BOOL	Initjob(const CString &) const;
	void	append_env(const char *);
#endif
};
