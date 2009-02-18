#include "stdafx.h"
#include <ctype.h>
#include "xbwnetwk.h"
#include "btrw.h" 

Btjob::Btjob()
{
	bj_arg = NULL;
	bj_env = NULL;
	bj_redirs = NULL;
	bj_nargs = bj_nenv = bj_nredirs = 0;
}

Btjob::~Btjob()
{
	if  (bj_arg)  {
		delete [] bj_arg;
		bj_arg = NULL;
	}
	if  (bj_env)  {
		delete [] bj_env;
		bj_env = NULL;
	}
	if  (bj_redirs)  {
		delete [] bj_redirs;
		bj_redirs = NULL;
	}
}
		
Btjob &Btjob::operator = (const Btjob &src)
{   
	hostid = src.hostid;
	slotno = src.slotno;
	
	bj_job = src.bj_job;
	bj_time = src.bj_time;
	bj_stime = src.bj_stime;
	bj_etime = src.bj_etime;
	bj_runhostid = src.bj_runhostid;
    bj_progress = src.bj_progress;
    bj_pri = src.bj_pri;
    bj_jflags = src.bj_jflags;
    bj_wpri = src.bj_pri;
    
    bj_cmdinterp = src.bj_cmdinterp;
    
	bj_ll = src.bj_ll;
	bj_umask = src.bj_umask;
	bj_ulimit = src.bj_ulimit;
	bj_deltime = src.bj_deltime;
	bj_runtime = src.bj_runtime;
	bj_autoksig = src.bj_autoksig;
	bj_runon = src.bj_runon;
	
	bj_pid = src.bj_pid;
	bj_lastexit = src.bj_lastexit;
	
	bj_mode = src.bj_mode;
	
	for  (unsigned  cnt = 0;  cnt < MAXCVARS;  cnt++)
		bj_conds[cnt] = src.bj_conds[cnt];           
	for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)
		bj_asses[cnt] = src.bj_asses[cnt];
	bj_times = src.bj_times;
	bj_exits = src.bj_exits;
	
	//  Hairy bits now follow
	
	bj_title = src.bj_title;
	bj_direct = src.bj_direct;
	
	if  (bj_nredirs = src.bj_nredirs)  {
		bj_redirs = new Mredir[bj_nredirs];
		for  (cnt = 0;  cnt < bj_nredirs;  cnt++)
			bj_redirs[cnt] = src.bj_redirs[cnt];
	}
	else
		bj_redirs = NULL;
				
	if  (bj_nenv = src.bj_nenv)  {
		bj_env = new Menvir[bj_nenv];
		for  (cnt = 0;  cnt < bj_nenv;  cnt++)
			bj_env[cnt] = src.bj_env[cnt];
	}
	else
		bj_env = NULL;		
    
    if  (bj_nargs = src.bj_nargs)  {
    	bj_arg = new CString[bj_nargs];
    	for  (cnt = 0;  cnt < bj_nargs;  cnt++)
    		bj_arg[cnt] = src.bj_arg[cnt];
    }                   
    else
    	bj_arg = NULL;
    return  *this;
}

Btjob::Btjob(Btjob &src)
{
	*this = src;
}

void	Btjob::init_job()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	*this = ma.m_defaultjob;
	if  (bj_direct.GetLength() == 0)  {
		const  char	*tp;
		if  (!(tp = ma.m_envtable.unix_getenv("PWD"))  &&  !(tp = ma.m_envtable.unix_getenv("HOME")))
    			tp = "/tmp";
    	bj_direct = tp;
    }
	ma.m_sendowner = ma.m_username;
	ma.m_sendgroup = ma.m_groupname;
}

void	Btjob::append_env(const char *ev)
{
	while  (isspace(*ev))
		ev++;
	const  char  *ep = strchr(ev, '=');
	if  (!ep)
		return;
	Menvir *curr;
	if  (bj_nenv == 0)  {
		bj_env = new Menvir [1];
		bj_nenv = 1;
		curr = &bj_env[0];
	}
	else  {
		Menvir *newe = new Menvir [bj_nenv + 1];
		for  (unsigned cnt = 0;  cnt < bj_nenv;  cnt++)
			newe[cnt] = bj_env[cnt];
		delete [] bj_env;
		bj_env = newe;
		curr = &bj_env[bj_nenv];
		bj_nenv++;
	}
	curr->e_name = CString(ev, ep - ev);
	curr->e_value = ep + 1;
}

extern	char	FAR	month_days[];

static	BOOL	argerror(const UINT helpcode, const char FAR *arg)
{
	CString	msg;
	msg.LoadString(helpcode);
	msg += " \"";
	msg += arg;
	msg += '\"';
	AfxMessageBox(msg, MB_OK|MB_ICONSTOP, helpcode);
	return  FALSE;
}

static	BOOL	parseavoid(Timecon &tc, char *arg)
{
   	unsigned  res = 0;
   	int		cnt;
   	char	*copyarg = arg;
    CString	Weekdays[8];
    if  (*arg == ',')  {
		res = tc.tc_nvaldays;
		arg++;
	}
	else  if  (*arg == '-')  {
		if  (*++arg == '\0')
			goto  done;
		if  (*arg != ',')
			return  argerror(IDP_SJINVALIDAVOID, copyarg);
		arg++;
	}
            
    for  (cnt = 0;  cnt < 8;  cnt++)
       	Weekdays[cnt].LoadString(cnt + IDS_SUN);
            	
	do  {
		int	np = 0;
		char	cbuf[30];

		if  (!isalpha(*arg))
			return  argerror(IDP_SJINVALIDAVOID, copyarg);

		do	if  (np < 29)
			cbuf[np++] = *arg++;
		while  (isalpha(*arg));
    	cbuf[np] = '\0';
		for  (cnt = 0;  cnt < 8;  cnt++)
			if  (Weekdays[cnt].CompareNoCase(cbuf) == 0)
				goto  foundday;
		return  argerror(IDP_SJINVALIDAVOID, cbuf);
	foundday:
		res |= 1 << cnt;
	}  while  (*arg++ == ',');

 done:
	if  ((res & TC_ALLWEEKDAYS) != TC_ALLWEEKDAYS)
		tc.tc_nvaldays = res;
	return  TRUE;
}

BOOL	parsecond(char *arg, Jcond &sc, const BOOL crit)
{
	int	 cp;         
	sc.bjc_var = arg;
	if  ((cp = sc.bjc_var.Find(':')) < 0)
		return  argerror(IDP_SJINVALIDCONDVAR, arg);
	do  cp++;
	while  (isalnum(sc.bjc_var[cp]));
	sc.bjc_var = sc.bjc_var.Left(cp);
	arg += cp;
    sc.bjc_iscrit = crit? 1: 0;
	unsigned  char	compar;

	//	Now look at condition

	char  *copyarg = arg;

	switch  (*arg++)  {
	default:
	badop:
		return  argerror(IDP_SJINVALIDCONDOP, copyarg);
	case  '!':
		if  (*arg++ != '=')
			goto  badop;
		compar = C_NE;
		break;
	case  '=':
		if  (*arg == '=')
			arg++;
		compar = C_EQ;
		break;
	case  '<':
		if  (*arg == '=')  {
			arg++;
			compar = C_LE;
		}
		else
			compar = C_LT;
		break;
	case  '>':
		if  (*arg == '=')  {
			arg++;
			compar = C_GE;
		}
		else
			compar = C_GT;
		break;
	}

	if  (!isdigit(*arg)  &&  *arg != '-')  {
		if  (*arg == ':') 	// Force to string
			arg++;
		int  np = 0;
		while  (*arg)  {
			if  (np >= BTC_VALUE)
				return  argerror(IDP_SJINVALIDCONDCONST, copyarg);
			sc.bjc_value.con_string[np++] = *arg++;
		}
		sc.bjc_value.con_string[np] = '\0';
		sc.bjc_value.const_type = CON_STRING;
	}
	else  {
		sc.bjc_value.con_long = atol(arg);
		sc.bjc_value.const_type = CON_LONG;
	}
    sc.bjc_compar = compar;
	return  TRUE;
}

BOOL	parseass(char *arg, Jass &sa, const BOOL Crit, const unsigned Flags)
{
	int	 cp;
	char	*copyarg = arg;
	
	sa.bja_var = arg;
	if  ((cp = sa.bja_var.Find(':')) < 0)  {
	badass:
		return  argerror(IDP_SJINVALIDASS, copyarg);
	}
	do  cp++;
	while  (isalnum(sa.bja_var[cp]));
	sa.bja_var = sa.bja_var.Left(cp);
	sa.bja_iscrit = Crit? 1: 0;
	sa.bja_flags = Flags;
	
	arg += cp;

	//	Now look at assignment
                 
    unsigned  char  op;
    
	switch  (*arg++)  {
	default:
		goto  badass;
	case  '=':
		op = BJA_ASSIGN;
		break;
	case  '+':
		op = BJA_INCR;
		goto  skipeq;
	case  '-':
		op = BJA_DECR;
		goto  skipeq;
	case  '*':
		op = BJA_MULT;
		goto  skipeq;
	case  '/':
		op = BJA_DIV;
		goto  skipeq;
	case  '%':
		op = BJA_MOD;
	skipeq:
		if  (*arg == '=')
			arg++;
		break;
	}

	if  (!isdigit(*arg) && *arg != '-')  {
		CString	ename;
		ename.LoadString(IDS_AEXIT);
		if  (*arg == '?' || ename.CompareNoCase(arg) == 0)  {
			if  (op != BJA_ASSIGN)
				goto  badass;
			op = BJA_SEXIT;
			sa.bja_con.con_long = 0;
			sa.bja_con.const_type = CON_LONG;
			goto  fin;
		}
		ename.LoadString(IDS_ASIGNAL);
		if  (*arg == '!' || ename.CompareNoCase(arg) == 0)  {
			if  (op != BJA_ASSIGN)
				goto  badass;
			op = BJA_SSIG;
			sa.bja_con.con_long = 0;
			sa.bja_con.const_type = CON_LONG;
			goto  fin;
		}
		if  (*arg == ':') 	// Force to string
			arg++;
		int  np = 0;
		while  (*arg)  {
			if  (np >= BTC_VALUE)
				goto  badass;
			sa.bja_con.con_string[np++] = *arg++;
		}
		sa.bja_con.con_string[np] = '\0';
		sa.bja_con.const_type = CON_STRING;
	}
	else  {
		sa.bja_con.con_long = atol(arg);
		sa.bja_con.const_type = CON_LONG;
	}
fin:
	sa.bja_op = op;
   	return  TRUE;
}

BOOL	parsearg(Btjob &job, char *arg)
{
 	CString  *newa = new CString [job.bj_nargs + 1];
 	for  (unsigned  cnt = 0;  cnt < job.bj_nargs;  cnt++)
 		newa[cnt] = job.bj_arg[cnt];
 	if  (job.bj_nargs != 0)
 		delete [] job.bj_arg;
 	job.bj_arg = newa;
 	job.bj_arg[job.bj_nargs] = arg;
 	job.bj_nargs++;
	return  TRUE;
}

BOOL	parseredir(Btjob &job, char *arg)
{
    Mredir	res;

	//	Any leading digit is a file descriptor
	
	int  whichfd = -1, action;
	char	*copyarg = arg;
	
	if  (isdigit(*arg))  {
		whichfd = 0;
		do	whichfd = whichfd * 10 + *arg++ - '0';
		while  (isdigit(*arg));

		if  (whichfd < 0 || whichfd >= _NFILE)
			return  argerror(IDP_SJINVALIDFILENUM, copyarg);
	}

    switch  (*arg++)  {
	default:
	badredir:
		return  argerror(IDP_SJINVALIDREDIR, copyarg);
		
		//	Some sort of input
	
	case  '<':

		if  (whichfd < 0)
			whichfd = 0;
		action = RD_ACT_RD;

		switch  (*arg)  {
		default:
			goto  rfname;
		case  '&':
			goto  amper;
		case  '|':
			arg++;
			action = RD_ACT_PIPEI;
			goto  rfname;
		case  '>':
			action = RD_ACT_RDWR;
			if  (*++arg == '>')  {
				arg++;
				action = RD_ACT_RDWRAPP;
			}
			if  (*arg == '&')
				goto  amper;
			goto  rfname;
		}

		//	Some sort of output
        
    case  '>':

		if  (whichfd < 0)
			whichfd = 1;

		action = RD_ACT_WRT;

		switch  (*arg)  {
		default:
			goto  rfname;

		case  '&':
			goto  amper;
		case  '|':
			arg++;
			action = RD_ACT_PIPEO;
			goto  rfname;
		case  '>':
			action = RD_ACT_APPEND;
			if  (*++arg == '<')  {
				arg++;
				action = RD_ACT_RDWRAPP;
			}
			if  (*arg == '&')
				goto  amper;
			goto  rfname;
		}

	case  '|':
		if  (whichfd < 0)
			whichfd = 1;
		action = RD_ACT_PIPEO;
		goto  rfname;
	}

 amper:
	if  (*++arg == '-')  {
		action = RD_ACT_CLOSE;
		arg++;
	}
	else  {
		int	dupfd = 0;
		action = RD_ACT_DUP;
		if  (!isdigit(*arg))
			goto  badredir;
		do  dupfd = dupfd * 10 + *arg++ - '0';
		while  (isdigit(*arg));
		if  (dupfd < 0 || dupfd >= _NFILE)
			return  argerror(IDP_SJINVALIDFILENUM, copyarg);
		res.arg = dupfd;
	}
	if  (*arg)
		goto  badredir;
	res.fd = whichfd;
	res.action = action;
	goto  retu;

 rfname:
	while  (isspace(*arg))
		arg++;
	if  (!*arg)
		goto  badredir;
	res.buffer = arg;
	res.fd = whichfd;
	res.action = action;
 retu:
 	Mredir  *newr = new Mredir [job.bj_nredirs + 1];
 	for  (unsigned  cnt = 0;  cnt < job.bj_nredirs;  cnt++)
 		newr[cnt] = job.bj_redirs[cnt];
 	if  (job.bj_nredirs != 0)
 		delete [] job.bj_redirs;
 	job.bj_redirs = newr;
 	job.bj_redirs[job.bj_nredirs] = res;
 	job.bj_nredirs++;
 	return  TRUE;
}

static	BOOL	parsemode(Btmode &md, char *arg)
{
	unsigned	resu = md.u_flags,
				resg = md.g_flags,
				reso = md.o_flags;
	char  *copyarg = arg;
    			
	do  {
		int  isuser = 0, isgroup = 0, isoth = 0;
		int  isplus = 0, isminus = 0;
		int	 wmode = 0;
		char  *startit = arg;
		int	 ch;

		while  ((ch = toupper(*arg)) == 'U'  ||  ch == 'G'  ||  ch == 'O')  {
			if  (ch == 'U')
				isuser = 1;
			else  if  (ch == 'G')
				isgroup = 1;
			else
				isoth = 1;
			arg++;
		}
		if  (ch != ':')  {
			isuser = isgroup = isoth = 1;
			arg = startit;
		}
		else
			arg++;
		if  ((ch = *arg) == '+')  {
			isplus = 1;
			arg++;
		}
		else  if  (ch == '-')  {
			isminus = 1;
			arg++;
		}
		else  if  (ch == '=')
			arg++;
        
        while  (isalpha(ch = *arg))  {
			arg++;
			switch  (ch)  {
			default:
				continue;
			case  'R':
				if  (isminus)
					wmode |= BTM_READ|BTM_WRITE;
				else
					wmode |= BTM_SHOW|BTM_READ;
				break;
        	case  'W':
				if  (isminus)
					wmode |= BTM_WRITE;
				else
					wmode |= BTM_SHOW|BTM_READ|BTM_WRITE;
				break;
        	case  'S':
				if  (isminus)
					wmode |= BTM_SHOW|BTM_READ|BTM_WRITE;
				else
					wmode |= BTM_SHOW;
				break;
        	case  'M':
				if  (isminus)
					wmode |= BTM_RDMODE|BTM_WRMODE;
				else
					wmode |= BTM_RDMODE;
				break;

			case  'P':
				if  (isminus)
					wmode |= BTM_WRMODE;
				else
					wmode |= BTM_RDMODE|BTM_WRMODE;
				break;

			case  'U':	wmode |= BTM_UGIVE;	break;
			case  'V':	wmode |= BTM_UTAKE;	break;
			case  'G':	wmode |= BTM_GGIVE;	break;
			case  'H':	wmode |= BTM_GTAKE;	break;
			case  'D':	wmode |= BTM_DELETE;	break;
			case  'K':	wmode |= BTM_KILL;	break;
			}
		}

		if  (ch != '\0'  &&  ch != ',')
			return  argerror(IDP_SJINVALIDMODE, copyarg);

		if  (isplus)  {
			if  (isuser)
				resu |= wmode;
			if  (isgroup)
				resg |= wmode;
			if  (isoth)
				reso |= wmode;
		}
		else  if  (isminus)  {
			if  (isuser)
				resu &= ~wmode;
			if  (isgroup)
				resg &= ~wmode;
			if  (isoth)
				reso &= ~wmode;
		}
		else  {
			if  (isuser)
				resu = (unsigned) wmode;
			if  (isgroup)
				resg = (unsigned) wmode;
			if  (isoth)
				reso = (unsigned) wmode;
		}                 
    }  while  (*arg++ == ',');

    md.u_flags = resu;
    md.g_flags = resg;
	md.o_flags = reso;
	return  TRUE;
}

static	BOOL	parsetime(Timecon &tc, char *arg)
{
	time_t	now = time(NULL), testit, result;
	tm	*tn = localtime(&now);
	int  year = tn->tm_year,
 		 month = tn->tm_mon + 1,
 		 day = tn->tm_mday,
 		 hour = tn->tm_hour,
 		 min = tn->tm_min, i;
 	char  *copyarg = arg;

	if  (!isdigit(*arg))  {
badtime:
		return  argerror(IDP_SJINVALIDTIME, copyarg);
	}
			
	int  num = 0;
	do	num = num * 10 + *arg++ - '0';
	while  (isdigit(*arg));

	if  (*arg == '/')  {	// It's a date I think
		arg++;
		if  (!isdigit(*arg))
			goto  badtime;
		int  num2 = 0;
		do	num2 = num2 * 10 + *arg++ - '0';
		while  (isdigit(*arg));

		if  (*arg == '/')  { // First digits were year
			if  (num > 1900)
				year = num - 1900;
			else  if  (num > 110)
				goto  badtime;
			else  if  (num < 90)
				year = num + 100;
			else
				year = num;
			arg++;
			if  (!isdigit(*arg))
				goto  badtime;
			month = num2;
			day = 0;
			do	day = day * 10 + *arg++ - '0';
			while  (isdigit(*arg));
		}
		else  {		// Day/month or Month/day
				   	// Decide by which side of the Atlantic
			if  (_timezone > 4 * 60 * 60)  {
				month = num;
				day = num2;
			}
			else  {
				month = num2;
				day = num;
			}
			if  (month < tn->tm_mon + 1  ||  (month == tn->tm_mon + 1 && day < tn->tm_mday))
				year++;
		}
		if  (*arg == '\0')
			goto  finish;
		if  (*arg != ',')
			goto  badtime;
		arg++;
		if  (!isdigit(*arg))
			goto  badtime;
		hour = 0;
		do	hour = hour * 10 + *arg++ - '0';
		while  (isdigit(*arg));
		if  (*arg != ':')
			goto  badtime;
		arg++;
		if  (!isdigit(*arg))
			goto  badtime;
		min = 0;
		do	min = min * 10 + *arg++ - '0';
		while  (isdigit(*arg));
	}
	else  {
		// If tomorrow advance date
		hour = num;
		if  (*arg != ':')
			goto  badtime;
		arg++;
		if  (!isdigit(*arg))
			goto  badtime;
		min = 0;
		do	min = min * 10 + *arg++ - '0';
		while  (isdigit(*arg));
		if  (hour < tn->tm_hour  ||  (hour == tn->tm_hour && min <= tn->tm_min))  {
			day++;
			month_days[1] = year % 4 == 0? 29: 28;
			if  (day > month_days[month-1])  {
				day = 1;
				if  (++month > 12)  {
					month = 1;
					year++;
				}
			}
		}
	}

	if  (*arg != '\0')
		goto  badtime;
 finish:
	if  (month > 12  || hour > 23  || min > 59)
		goto  badtime;
	month_days[1] = year % 4 == 0? 29: 28;
	month--;
	year -= 70;
	if  (day > month_days[month])
		goto  badtime;
	result = year * 365;
	if  (year > 2)
		result += (year + 1) / 4;
	for  (i = 0;  i < month;  i++)
		result += month_days[i];
	result = (result + day - 1) * 24;

	//	Build it up once as at 12 noon and work out timezone
	//	shift from that

	testit = (result + 12) * 60 * 60;
	tn = localtime(&testit);
	result = ((result + hour + 12 - tn->tm_hour) * 60 + min) * 60;
	tc.tc_nexttime = result;
	tc.tc_istime = 1;
	return  TRUE;
}

static	BOOL	parserepeat(Timecon &tc, char *arg)
{
	unsigned  char	res = TC_HOURS;
	char	*copyarg = arg;
			
	if  (isalpha(*arg))  {
		char	cbuf[30];
		int	np = 0;
		do	if  (np < 29)
			cbuf[np++] = *arg++;
		while  (isalpha(*arg));
        cbuf[np] = '\0';
		if  (*arg++ != ':')
			goto  badrep;
		for  (int a = 0;  a < TC_YEARS - TC_MINUTES + 1;  a++)  {
			CString  Unit;
			Unit.LoadString(a + IDS_MINUTES);
			if  (Unit.CompareNoCase(cbuf) == 0)
				goto  foundunit;
		}
		goto  badrep;
	foundunit:
		res = a + TC_MINUTES;
	}
	if  (!isdigit(*arg))  {
	badrep:
		return  argerror(IDP_SJINVALIDREP, copyarg);
	}
	unsigned  long  num = 0;
	do	num = num*10 + *arg++ - '0';
	while  (isdigit(*arg));
	static  unsigned  long  limrep[] = { 527040, 8784, 1000, 520, 50, 50, 10 };
	if  (num > limrep[res - TC_MINUTES])
		goto  badrep;
    unsigned  char	mday = 0;
	if  (*arg == ':')  {
		arg++;
		int  numi = 0;
		do	numi = numi*10 + *arg++ - '0';
		while  (isdigit(*arg));
		if  (numi <= 0  || numi > 31)
			goto  badrep;
		mday = numi;
	}
	if  (*arg)
		goto  badrep;
	tc.tc_repeat = res;
	tc.tc_istime = 1;
	tc.tc_rate = num;
	tc.tc_mday = mday;
	return  TRUE;
}

BOOL	Btjob::ParseBtrArgs(const char FAR *cmd, CString &jobfile)
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	BOOL	Crit_cond = FALSE, Crit_ass = FALSE;
	unsigned  Sflags = 0;
	
nexta:
    while  (isspace(*cmd))
    	cmd++;
    
    if  (*cmd != '-' && *cmd != '/')  {
	    jobfile = cmd;
	    return  TRUE;
	}

	//  Default saved command for error messages
	    
    const char  FAR *savecmd = cmd++;         

	while  (*cmd)  {
		switch  (*cmd++)  {
		default:
			return  argerror(IDP_SJINVALIDARG, savecmd);
		case  ' ':
		case  '\t':
			goto  nexta;
		case  'Q':
			ma.m_promptfirst = TRUE;
			continue;             
		case  'V':
			ma.m_verbose = FALSE;
		    continue;
		case  'v':
			ma.m_verbose = TRUE;
		    continue;
		case  'm':
			bj_jflags |= BJ_MAIL;
			continue;
		case  'w':
			bj_jflags |= BJ_WRT;
			continue;
		case  'x':
			bj_jflags &= ~(BJ_WRT|BJ_MAIL);
			continue;
		case  'N':
			bj_progress = BJP_NONE;
			continue;
		case  'C':
		    bj_progress = BJP_CANCELLED;
		    continue;
		case  '.':
			bj_progress = BJP_DONE;
			continue;
		case  'S':
			bj_times.tc_nposs = TC_SKIP;
			continue;
		case  'H':
			bj_times.tc_nposs = TC_WAIT1;
			continue;
		case  'R':
			bj_times.tc_nposs = TC_WAITALL;
			continue;
		case  '9':
			bj_times.tc_nposs = TC_CATCHUP;
			continue;
		case  'U':
			bj_times.tc_istime = 0;
			continue;
		case  'o':
			bj_times.tc_repeat = TC_RETAIN;
			bj_times.tc_istime = 1;
			continue;
		case  'd':
			bj_times.tc_repeat = TC_DELETE;
			bj_times.tc_istime = 1;
			continue;
		case  'e':
			bj_nargs = 0;
			if  (bj_arg)  {
				delete [] bj_arg;
				bj_arg = NULL;
			}
			continue;
		case  'y':
		{
    		for  (int  cnt = 0;  cnt < MAXCVARS;  cnt++)
    			bj_conds[cnt].bjc_compar = C_UNUSED;
			continue;
		}
		case  'z':
		{
    		for  (int  cnt = 0;  cnt < MAXSEVARS;  cnt++)
    			bj_asses[cnt].bja_op = BJA_NONE;
    		continue;
    	}
		case  'j':
			bj_jflags &= ~BJ_NOADVIFERR;
			continue;
		case  'J':
			bj_jflags |= BJ_NOADVIFERR;
			continue;
		case  'n':
			bj_jflags &= ~(BJ_EXPORT|BJ_REMRUNNABLE);
			continue;
		case  'F':
			bj_jflags &= ~BJ_REMRUNNABLE;
			bj_jflags |= BJ_EXPORT;
			continue;
		case  'G':
			bj_jflags |= BJ_EXPORT|BJ_REMRUNNABLE;
			continue;
		case  'k':
			Crit_cond = TRUE;
			continue;
		case  'K':
			Crit_cond = FALSE;
			continue;
		case  'b':
			Crit_ass = TRUE;
			continue;
		case  'B':
			Crit_ass = FALSE;
		case  'Z':
			bj_nredirs = 0;
			if  (bj_redirs)  {
				delete [] bj_redirs;
				bj_redirs = NULL;
			}
			continue;
		case  'O':
			ma.m_binmode = TRUE;
			continue;
		case  'W':
			ma.m_binmode = FALSE;
			continue;

		case  'I': 
		case  'h': 
		case  'i': 
		case  'p': 
		case  'l': 
		case  'T': 
		case  'r': 
		case  'D': 
		case  'a': 
		case  'c': 
		case  'f': 
		case  's': 
		case  'A': 
		case  'M': 
		case  'P': 
		case  'L': 
		case  'X': 
		case  'u': 
		case  'g': 
		case  'q': 
		case  't': 
		case  'Y': 
		case  'E': 
		case  '2':
			break;
		}
		
		//  We have a command which takes an arg.
		//  The arg may follow immediately or after spaces
		
		int	 cmdlet = cmd[-1];
		char	copyarg[256];
		int  ccnt = 0;
		
		if  (!*cmd)
			return  argerror(IDP_SJMISSARG, savecmd);

		while  (isspace(*cmd))
			cmd++;

		while  (*cmd  &&  !isspace(*cmd))  {
			if  (*cmd == '\'' || *cmd == '\"')  {
				int	 quote = *cmd++;
				while  (*cmd != quote)  {
					if  (!*cmd)
						return  argerror(IDP_SJINVALIDARG, copyarg);
					copyarg[ccnt++] = *cmd++;
				}
				cmd++;
			}
			else
				copyarg[ccnt++] = *cmd++;
		}
		copyarg[ccnt] = '\0';

		char  *arg = copyarg;

		switch  (cmdlet)  {
		default:
			return  argerror(IDP_SJINVALIDARG, savecmd);
		
		case  'A':
			if  (parseavoid(bj_times, arg))
				goto  nexta;
			return  FALSE;
		case  'a':
			if  (parsearg(*this, arg))
				goto  nexta;
			return  FALSE; 
		case  'c': 
		{
			for  (int cnt = 0;  cnt < MAXCVARS;  cnt++)
				if  (bj_conds[cnt].bjc_compar == C_UNUSED)  {
					if  (parsecond(arg, bj_conds[cnt], Crit_cond))
						goto  nexta;
					return  FALSE;
				}
			return  argerror(IDP_SJTOOMANYCONDS, copyarg);
		}
		case  'D': 
			bj_direct = arg;
			goto  nexta;
		case  'E': 
		{
			unsigned	 num;

			if  (!isdigit(*arg))
				return  argerror(IDP_SJINVALIDSIGNUM, copyarg);
			num = atoi(arg);
			if  (num >= 32)
				return  argerror(IDP_SJINVALIDSIGNUM, copyarg);
			bj_autoksig = (unsigned short) num;
			goto  nexta;
		}
		case  'f': 
		{
		    unsigned  Nflags = 0;

			while  (*arg)
			switch  (*arg++)  {
			case  'S':
			case  's':
				Nflags |= BJA_START;
				break;
			case  'N':
			case  'n':
				Nflags |= BJA_OK;
				break;
			case  'E':
			case  'e':
				Nflags |= BJA_ERROR;
				break;
			case  'A':
			case  'a':
				Nflags |= BJA_ABORT;
				break;
			case  'C':
			case  'c':
				Nflags |= BJA_CANCEL;
				break;
			case  'R':
			case  'r':
				Nflags |= BJA_REVERSE;
				break;
			}

			if  (Nflags != 0)
				Sflags = Nflags;
			goto  nexta;
		}
		case  'g': 
			ma.m_sendgroup = arg;
			goto  nexta;
		case  'h': 
			bj_title = arg;
			goto  nexta;
		case  'I':
			if  (parseredir(*this, arg))
				goto  nexta;
			return  FALSE; 
    	case  'i': 
		{
    		CIList	&cil = ((CBtrwApp *)AfxGetApp())->m_cilist;
    		cil.init_list();
    		Cmdint	*ci;
    		while  (ci = cil.next())
    			if  (strcmp(ci->ci_name, arg) == 0)  {
    				bj_cmdinterp = arg;
    				bj_ll = ci->ci_ll;
    				goto  nexta;
    			}
			return  argerror(IDP_SJUNKNOWNCI, copyarg);
		}
		case  'L': 
		{
			long  result = atol(arg);
			if  (result > 0)
				bj_ulimit = result;
			goto  nexta;
		}
		case  'l': 
		{
			unsigned  num = atoi(arg);
			if  (num != 0  &&  num <= 0x7fff)
				bj_ll = num;
			goto  nexta;
		}
		case  'M':
			if  (parsemode(bj_mode, arg))
				goto  nexta;
			return  FALSE; 
		case  'P': 
		{
			unsigned  result = 0;
			while  (*arg)  {
				if  (*arg < '0' || *arg > '7')
					return  argerror(IDP_SJINVALIDUMASK, copyarg);
				result = (result << 3) + *arg++ - '0';
			}
			if  (result < 0777)
				bj_umask = result;
			goto  nexta;
		}
		case  'p': 
		{
			int  num = atoi(arg);
			if  (num < 0 && num >= 255)
				bj_pri = (unsigned char) num;
			goto  nexta;
		}
		case  'q': 
			ma.m_jobqueue = (arg[0] && (arg[0] != '-' || arg[1])) ? arg: "";
			goto  nexta;
		case  'r':
			if  (parserepeat(bj_times, arg))
				goto  nexta;
			return  FALSE;	 
		case  's': 
		{
			for  (int cnt = 0;  cnt < MAXSEVARS;  cnt++)
				if  (bj_asses[cnt].bja_op == BJA_NONE)  {
					if  (parseass(arg, bj_asses[cnt], Crit_ass, Sflags))
						goto  nexta;
					return  FALSE;
				} 
			return  argerror(IDP_SJTOOMANYASSES, copyarg);
		}
		case  'T':
			if  (parsetime(bj_times, arg))
				goto  nexta;
			return  FALSE;
		case  't': 
		{
			unsigned  long num;

			if  (!isdigit(*arg))  {
		baddel:
				return  argerror(IDP_SJINVALIDDELTIME, copyarg);
			}
			num = atoi(arg);
			if  (num > 65535)
				goto  baddel;
			bj_deltime = (unsigned short) num;
			goto  nexta;
		}
		case  'u': 
			ma.m_sendowner = arg;
		case  'X': 
		{
			int  which = 0, n1 = 0, n2;

			switch  (*arg)  {
			default:
				break;
			case  'n':case  'N':
				/* which = 0; */
				arg++;
				break;
			case  'e':case  'E':
				which = 1;
				arg++;
				break;
			}
			if  (!isdigit(*arg))  {
		badexit:
				return  argerror(IDP_SJINVALIDEXIT, copyarg);
			}
			do  n1 = n1 * 10 + *arg++ - '0';
			while  (isdigit(*arg));
			if  (*arg == ':')  {
				arg++;
				if  (!isdigit(*arg))
					goto  badexit;
				n2 = 0;
				do  n2 = n2 * 10 + *arg++ - '0';
				while  (isdigit(*arg));
				if  (n1 > n2)  {
					int	tmp = n1;
					n1 = n2;
					n2 = tmp;
				}
			}
			else
				n2 = n1;
			if  (*arg)
				goto  badexit;
			if  (n2 > 255)
				goto  badexit;
			if  (which)  {
				bj_exits.elower = n1;
				bj_exits.eupper = n2;
			}
			else  {
				bj_exits.nlower = n1;
				bj_exits.nupper = n2;
			}
			goto  nexta;
		}
		case  'Y': 
		{
			int	hr = 0, mn = 0, sc = 0;

			for  (int  bits = 0;  bits < 3;  bits++)  {
				if  (!isdigit(*arg))
					goto  badrunt;
				do  sc = sc*10 + *arg++ - '0';
				while  (isdigit(*arg));
				if  (!*arg)
					goto  setres;
				if  (*arg++ != ':')
					goto  badrunt;
				hr = mn;
				mn = sc;
				sc = 0;
			}
		badrunt:
			return  argerror(IDP_SJINVALIDRUNT, copyarg);
		setres:
			if  (mn < 0 || mn > 59 || sc < 0 || sc > 59)
				goto  badrunt;
			bj_runtime = (hr * 60L + mn) * 60L + sc;
			goto  nexta;
		}
		case  '2':
		{
			unsigned  long	mn = 0, sc = 0;
			if  (!isdigit(*arg))
				goto  badrunon;
			do  sc = sc*10 + *arg++ - '0';
			while  (isdigit(*arg));
			if  (!*arg)
				goto  setres2;
			if  (*arg++ != ':')
				goto  badrunon;
			mn = sc;
			sc = 0;
			do  sc = sc*10 + *arg++ - '0';
			while  (isdigit(*arg));
			if  (*arg)  {
	badrunon:
				return  argerror(IDP_SJINVALIDRUNON, copyarg);
		    }
 	setres2:
			if  (sc > 59)
				goto  badrunon;
			sc += mn * 60;
			if  (sc > 65535)
				goto  badrunon;
			bj_runon = (unsigned short) sc;
			goto  nexta;
		}
	}
	}
	AfxMessageBox(IDP_SJNOJOBFILE, MB_OK|MB_ICONSTOP);
	return  FALSE;
}

BOOL	Btjob::loadoptions(CFile *inf, CString &jobfile)
{
	init_job();

	if  (inf->GetLength() < 5)
		return  FALSE;
	
	char	inbuf[1024];
	UINT	inl;
	long	offset = 0;
	
	TRY {
	
		//  Read in a bufferfull and find the end of the line
		
		while ((inl = inf->Read(inbuf, sizeof(inbuf)-1)) != 0)  {
			inbuf[inl] = '\0';
			int  dp = strcspn(inbuf, "\r\n\x1A");
			if  (dp < 0)
				return  FALSE;
			if  (dp > 4)  {
				inbuf[dp] = '\0';
				if  (_strnicmp(inbuf, "set ", 4) == 0)
					append_env(inbuf+4);
				else  if  (_strnicmp(inbuf, "btr ", 4) == 0)
					return  ParseBtrArgs(&inbuf[4], jobfile);
			}
			
			// Skip over any other \r \n ^zs
			
			do  dp++;
			while  (strchr("\r\n\x1A", inbuf[dp]));
			
			//  Find the beginning of the next line
			
			offset += dp;
			inf->Seek(offset, CFile::begin);
		}
		
		//  Didn't find a btr command, must have been something else
		
		return  FALSE;
	}
	CATCH(CException, e)
	{
		return  FALSE;
	}
	END_CATCH
	
	return  FALSE;
}

extern	BOOL	dumpjob(CFile &, const CString &, const Btjob &);

BOOL	Btjob::saveoptions(CFile *outf, const CString &jobfile)
{
	return  dumpjob(*outf, jobfile, *this);
}

static	void	promptsubmit(Btjob &job, CString &jobfile)
{
	if  (((CBtrwApp *)AfxGetApp())->m_promptfirst)  {
		CString	pf;
		pf.LoadString(IDP_OKTOSUBMIT);
		pf += ' ';
		pf += jobfile;
		if  (AfxMessageBox(pf, MB_YESNO|MB_ICONQUESTION, IDP_OKTOSUBMIT) != IDYES)
			return;
	}
	jobno_t	jobres;
	if  (job.submitjob(jobres, jobfile) && ((CBtrwApp *)AfxGetApp())->m_verbose)  {
		CString	msg;
		msg.LoadString(IDP_SUBMITTEDOK);
		char  nbuf[20];
		wsprintf(nbuf, "%ld", jobres);
		msg += nbuf;
		AfxMessageBox(msg, MB_OK|MB_ICONINFORMATION, IDP_SUBMITTEDOK);
	}
}

void	parsecmdline(const char FAR * cmd)
{
	Btjob	Cjob;
	Cjob.init_job();
	CString	jobfile;
	Cjob.ParseBtrArgs(cmd, jobfile);
	promptsubmit(Cjob, jobfile);
}

void	parsecmdfile(const char FAR *fname)
{
	//  Take -Q or /Q before file name to mean prompt first
	
	if  ((*fname == '-' || *fname == '/') && fname[1] == 'Q')  {
		((CBtrwApp *)AfxGetApp())->m_promptfirst = TRUE;
		fname += 2;
		while  (isspace(*fname))
			fname++;
	}
	CFile	rf;
	CString	Cmdfile = fname;
	if  (!rf.Open(Cmdfile, CFile::modeRead))  {
		CString	co;
		co.LoadString(IDP_CANTOPENCMDFILE);
		co += " \"";
		co += fname;
		co += '"';
		AfxMessageBox(co, MB_OK|MB_ICONSTOP, IDP_CANTOPENCMDFILE);
		return;
	}
	CString	jobfile;
	Btjob	rjob;
	if  (rjob.loadoptions(&rf, jobfile))
		promptsubmit(rjob, jobfile);
}	

void	DroppedFile(const char FAR *path)
{
	const  char	FAR *sp = strrchr(path, '.');
	if  (sp)  {
		sp++;
		if  (stricmp(sp, "xbc") == 0  ||  stricmp(sp, "bat") == 0)  {
			parsecmdfile(path);                                                               
			return;
		}
	}
    parsecmdline(path);
}