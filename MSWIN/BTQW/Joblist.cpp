#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "btqw.h"

int		joblist::jindex(const jident &wi) const
{
	for  (unsigned cnt = 0;	cnt < njobs;  cnt++)
		if  (*list[cnt] == wi)
			return  cnt;
	return  -1;
}		

Btjob	*joblist::operator [] (const unsigned ind) const
{
	if  (ind >= njobs)
		return  NULL;
	return  list[ind];
}

Btjob	*joblist::operator [] (const jident &wi) const
{
	int	ret = jindex(wi);
	if  (ret < 0)
		return  NULL;
	return  list[ret];
}	

void	joblist::checkgrow()
{
	if  (njobs >= maxjobs)  {
		if  (list)  {
			Btjob  **oldlist = list;
			list = new Btjob *[maxjobs + INCJOBS];
			VERIFY(list != NULL);
			memcpy((void *)list, (void *) oldlist, maxjobs * sizeof(Btjob *));
			delete [] oldlist;
			maxjobs += INCJOBS;
		}
		else  {
			maxjobs = INITJOBS;
			list = new Btjob *[INITJOBS];
			VERIFY(list != NULL);
		}
    }
}    
	
void	joblist::append(Btjob *j)
{
	checkgrow();
	list[njobs++] = j;
}
			  
void	joblist::insert(Btjob *j, const unsigned where)
{
	checkgrow();
	if  (where >= njobs)
		list[njobs] = j;
	else  {
		for  (int cnt = njobs - 1; cnt >= int(where); cnt--)
			list[cnt+1] = list[cnt];                           
		list[where] = j;
	}   
	njobs++;
}	

void	joblist::remove(const unsigned where)
{
	njobs--;
	delete list[where];
	for  (unsigned  cnt = where;  cnt < njobs;  cnt++)
		list[cnt] = list[cnt+1];                         
	if  (njobs == 0)  {
		delete [] list;
		list = NULL;
		maxjobs = 0;
	}		
}                                                            

int	joblist::move(const unsigned from, const unsigned to)
{
	if  (from == to)
		return 0;  
	Btjob  *j = list[from];
	if  (from < to)
		for  (unsigned cnt = from; cnt < to;  cnt++)
			list[cnt] = list[cnt+1];                   
	else
		for  (unsigned cnt = from; cnt > to;  cnt--)
			list[cnt] = list[cnt-1];				   
	list[to] = j;
	return  1;
}

//  These routines are called in response to network operations

static	void	poke(const UINT msg, const unsigned indx = 0)
{
	CWnd  *maw = AfxGetApp()->m_pMainWnd;
	if  (maw)
		maw->SendMessageToDescendants(msg, indx);
}	
		
void	joblist::net_jclear(const netid_t hostid)
{
	unsigned  cnt = 0;
	int		  changes = 0;
	while  (cnt < njobs)
	    if  (list[cnt]->hostid == hostid)  {
	        remove(cnt);
	        changes++;                         
	    }               
	    else
	    	cnt++;
	if	(changes)
		poke(WM_NETMSG_JREVISED);
}

Btjob::Btjob()
{
	bj_arg = NULL;
	bj_env = NULL;
	bj_redirs = NULL;
	bj_nargs = bj_nenv = bj_nredirs = 0;
}

Btjob::~Btjob()
{
	if  (bj_arg)
		delete [] bj_arg;
	if  (bj_env)
		delete [] bj_env;
	if  (bj_redirs)
		delete [] bj_redirs;
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
	bj_orighostid = src.bj_orighostid;
    bj_progress = src.bj_progress;
    bj_pri = src.bj_pri;
    bj_jflags = src.bj_jflags;
	bj_jrunflags = src.bj_jrunflags;
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

void	joblist::createdjob(const Btjob &jq)
{
	Btjob  *newj = new Btjob;
	VERIFY(newj != NULL);

	*newj = jq;

	//  Find home in our version of the job queue
	
	for  (int  where = njobs - 1;  where >= 0;  where--)  {
		if  (int(list[where]->bj_pri) >= newj->bj_wpri)
			break;
		if  (list[where]->bj_time < newj->bj_time)
			newj->bj_wpri--;
	}                                 
	insert(newj, unsigned(where + 1));
	poke(WM_NETMSG_JADD);
}

void	joblist::procjchange(Btjob &jq, const int jnum, const unsigned oldpri)
{
	int	 pdiff = int(jq.bj_pri) - int(oldpri);

	if  (pdiff == 0)  {
		poke(WM_NETMSG_JCHANGE, unsigned(jnum));
		return;                                  
	}
		
	jq.bj_wpri += pdiff;
	int  changes;
	if  (pdiff > 0)  {	//  Going up........
		for  (int  where = jnum - 1;  where >= 0;  where--)  {
			if  (int(list[where]->bj_pri) >= jq.bj_wpri)
				break;
			jq.bj_wpri--;
		}
		changes = move(unsigned(jnum), unsigned(where + 1));
	}
	else  {		//  Going down....
		for  (unsigned	where = jnum + 1;  where < njobs;  where++)  {
			if  (int(list[where]->bj_pri) <= jq.bj_wpri)
				break;
			jq.bj_wpri++;
		}
		changes = move(unsigned(jnum), where - 1);
	}
	if  (pdiff != 0)
		poke(changes? WM_NETMSG_JREVISED: WM_NETMSG_JCHANGE, unsigned(jnum));
}

void	joblist::changedjobhdr(const Btjob &jq)
{
	int	 jnum = jindex(jq);
	if  (jnum < 0)
		return;
	Btjob  *jp = list[jnum];
    unsigned  oldpri = jp->bj_pri;

	jp->hostid = jq.hostid;
	jp->slotno = jq.slotno;
	
	jp->bj_job = jq.bj_job;
	jp->bj_time = jq.bj_time;
	jp->bj_stime = jq.bj_stime;
	jp->bj_etime = jq.bj_etime;
	jp->bj_runhostid = jq.bj_runhostid;
    jp->bj_progress = jq.bj_progress;
    jp->bj_pri = jq.bj_pri;
    jp->bj_jflags = jq.bj_jflags;
    
    jp->bj_cmdinterp = jq.bj_cmdinterp;
    jp->bj_pid = jq.bj_pid;
    jp->bj_lastexit = jq.bj_lastexit;
    
	jp->bj_ll = jq.bj_ll;
	jp->bj_umask = jq.bj_umask;
	jp->bj_ulimit = jq.bj_ulimit;
	jp->bj_deltime = jq.bj_deltime;
	jp->bj_runtime = jq.bj_runtime;
	jp->bj_autoksig = jq.bj_autoksig;
	jp->bj_runon = jq.bj_runon;
	
	jp->bj_mode = jq.bj_mode;
	
	for  (unsigned  cnt = 0;  cnt < MAXCVARS;  cnt++)
		jp->bj_conds[cnt] = jq.bj_conds[cnt];           
	for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)
		jp->bj_asses[cnt] = jq.bj_asses[cnt];
	jp->bj_times = jq.bj_times;
	jp->bj_exits = jq.bj_exits;
	//	Note deliberate omission of "hairy bits"
    procjchange(*jp, jnum, oldpri);
}
	
void	joblist::changedjob(const Btjob &jq)
{
	int	 jnum = jindex(jq);
	if  (jnum < 0)
		return;
	Btjob  *jp = list[jnum];
	unsigned  oldpri = jp->bj_pri;
	int	 oldwpri = jp->bj_wpri;
	*jp = jq;
	jp->bj_wpri = oldwpri;
	procjchange(*jp, jnum, oldpri);
}
                           
void	joblist::boqjob(const jident &ji)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	int  savepri = list[where]->bj_pri;
	move(where, njobs-1);
	for  (int  cnt = njobs - 2;  cnt >= 0;  cnt--)   {
		if  (int(list[cnt]->bj_pri) >= savepri)
			break;                                    
		savepri--;
	}
	cnt++;
	move(njobs - 1, cnt);
	list[cnt]->bj_wpri = savepri;
	if  (cnt != where)
		poke(WM_NETMSG_JREVISED);
}

//  Force job actually this doesn't end up doing anything on this
//  machine

void	joblist::forced(const jident &j, const int adv)
{
	int	 where = jindex(j);
	if  (where < 0)
		return;
	list[where]->bj_jflags |= BJ_FORCE;
	if  (!adv)
		list[where]->bj_jflags |= BJ_FORCENA;
}

void	joblist::deletedjob(const jident &ji)
{
	int  where = jindex(ji);
	if  (where < 0)
		return;
	remove(unsigned(where));
	poke(WM_NETMSG_JDEL);
}

void	joblist::deskel_jobs(const netid_t nid)
{
	for  (unsigned cnt = 0;	cnt < njobs;  cnt++)  {
		Btjob	&jb = *list[cnt];
		if  (!(jb.bj_jrunflags & BJ_SKELHOLD))
			continue;
		if  (jb.hostid == nid)
			continue;
		sync_single(jb);
	}
}

void	joblist::chmogedjob(const Btjob &jq)
{
	int	 where = jindex(jq);
	if  (where < 0)
		return;
	list[where]->bj_mode = jq.bj_mode;
	poke(WM_NETMSG_JCHANGE, unsigned(where));
}	
	
void	joblist::statchjob(const jident &ji, const netid_t runhost, const long nexttime, const long lastpid, const unsigned short lastexit, const unsigned char prog)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	Btjob  *jp = list[where];
	jp->bj_progress = prog;
	jp->bj_runhostid = runhost;
	jp->bj_times.tc_nexttime = nexttime;
	jp->bj_pid = lastpid;
	jp->bj_lastexit = lastexit;
	jp->bj_jflags &= ~(BJ_FORCE|BJ_FORCENA);
	poke(WM_NETMSG_JCHANGE, unsigned(where));
}

void	joblist::remassjob(const jident &ji, const unsigned flag, const unsigned status)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	Btjob  *jp = list[where];
	Jass	*ja = jp->bj_asses;
	for  (int  cnt = MAXSEVARS-1;  cnt >= 0;  ja++, cnt--)	{
		unsigned  op = ja->bja_op;
		if  (op == BJA_NONE)
			return;
		unsigned  jflags = op >= BJA_SEXIT? BJA_OK|BJA_ERROR|BJA_ABORT: ja->bja_flags;
		if  ((jflags & flag) == 0)
			continue;
		Btcon	*con = &ja->bja_con, zer;
		if  (jflags & BJA_REVERSE  &&  (flag & BJA_START) == 0)  {
			switch  (op)  {            
			case  BJA_ASSIGN:
				if  ((zer.const_type = con->const_type) == CON_LONG)
					zer.con_long = 0;
				else
					zer.con_string[0] = '\0';
				con = &zer;
				break;
			case  BJA_INCR:
				op = BJA_DECR;
				break;
			case  BJA_DECR:
				op = BJA_INCR;
				break;
			case  BJA_MULT:
				op = BJA_DIV;
				break;
			case  BJA_DIV:
				op = BJA_MULT;
				break;
			}
		}	
		switch  (op)  {
		case  BJA_ASSIGN:
		case  BJA_INCR:          
		case  BJA_DECR:
		case  BJA_MULT:
		case  BJA_DIV:
		case  BJA_MOD:
			jopvar(ja->bja_varind, *con, op);
			break;
		case  BJA_SEXIT:
			zer.const_type = CON_LONG;
			zer.con_long = (status >> 8) & 0xFF;
			jopvar(ja->bja_varind, zer, BJA_ASSIGN);
			break;
		case  BJA_SSIG:
			zer.const_type = CON_LONG;
			zer.con_long = status & 0xFF;
			jopvar(ja->bja_varind, zer, BJA_ASSIGN);
			break;
		}
	}		
}

static	int	 stringsdiff(const Btjob &one, const Btjob &two)
{
	if  (one.bj_title != two.bj_title  ||
		 one.bj_direct != two.bj_direct ||
		 one.bj_nredirs != two.bj_nredirs ||
		 one.bj_nenv != two.bj_nenv ||
		 one.bj_nargs != two.bj_nargs)
		 return  1;
	
	for  (unsigned cnt = 0;  cnt < one.bj_nargs;  cnt++)
		if  (one.bj_arg[cnt] != two.bj_arg[cnt])
			return  1;
	
	for  (cnt = 0;  cnt < one.bj_nenv;  cnt++)
		if  (one.bj_env[cnt] != two.bj_env[cnt])
			return  1;
	 
	for  (cnt = 0;  cnt < one.bj_nredirs;  cnt++)
		if  (one.bj_redirs[cnt] != two.bj_redirs[cnt])
			return  1;
	
	return  0;
}

// "Transmit" sort of network ops

void	joblist::changejob(const Btjob &jq)
{
	int	 where = jindex(jq);
	if  (where < 0)
		return;
	Btjob	&jp = *list[where];
	job_sendupdate(jq, stringsdiff(jp, jq)? J_CHANGED: J_HCHANGED);
}

void	joblist::chogjob(const jident &ji, const unsigned op, const CString &newog)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	job_sendugupdate(*list[where], op, newog);
}

void	joblist::chmodjob(const Btjob &jq)
{
	int	 where = jindex(jq);
	if  (where < 0)
		return;
	job_sendmdupdate(*list[where], jq);
}

void	joblist::killjob(const jident &ji, const int sigtype)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	job_message(*list[where], J_KILL, (unsigned long) sigtype);
}

void	joblist::deletejob(const jident &ji)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	job_message(*list[where], J_DELETE);
}

void	joblist::forcejob(const jident &ji, const int noadv)
{
	int	 where = jindex(ji);
	if  (where < 0)
		return;
	job_message(*list[where], noadv? J_FORCENA: J_FORCE);
}

jobqueuelist::jobqueuelist()
{
	index = 0;
	number = 1;
	unsigned lim = Jobs().number();
	list = new CString [lim+1];	// Zeroth is null string
	for  (unsigned cnt = 0;  cnt < lim;  cnt++)  {
		Btjob	*j = Jobs()[cnt];
		if  (j->bj_mode.mpermitted(BTM_READ))  {
			CString  &title = j->bj_title;
			int	 pos = title.Find(':');
			if  (pos < 0)
				continue;
			CString prefix = title.Left(pos);
			for  (unsigned chk = 0;  chk < number;  chk++)
				if  (list[chk] == prefix)
					goto  hadit;
			list[number++] = prefix;
		hadit:
			;
		}
	}
}

jobqueuelist::~jobqueuelist()
{
	delete [] list;
}

CString	*jobqueuelist::next()
{
	if  (index >= number)
		return  NULL;
	return  &list[index++];
}
	

	