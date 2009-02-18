// btrsevw.cpp : implementation of the CBtrsetwView class
//

#include "stdafx.h"
#include "xbwnetwk.h"
#include "btrsedoc.h"
#include "btrsetw.h"
#include "btrsevw.h"
#include "progopts.h"
#include "procpar.h"
#include "queuedef.h"
#include "defcond.h"
#include "defass.h"
#include "timedflt.h"
#include "hostdlg.h"
#include "portnums.h"
#include "jobmode.h"
#include "shreq.h"
#include "timelim.h"
#include <ctype.h>
#include "loginhost.h"
#include "loginout.h"
#include "NewGrpDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define	HOST_COL	0
#define	HOST_LEN	HOSTNSIZE
#define	ALIAS_COL	(HOST_COL+HOST_LEN+1)
#define	ALIAS_LEN	HOSTNSIZE
#define	INET_COL	(ALIAS_COL+ALIAS_LEN+1)
#define	INET_LEN	15
#define	PROBE_COL	(INET_COL+INET_LEN+1)
#define	PROBE_LEN	7
#define	SERVER_COL	(PROBE_COL+PROBE_LEN+1)
#define	SERVER_LEN	8
#define	TIMEOUT_COL	(SERVER_COL+SERVER_LEN)
#define	TIMEOUT_LEN	5
#define	ROW_WIDTH	(TIMEOUT_COL+TIMEOUT_LEN)

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwView

IMPLEMENT_DYNCREATE(CBtrsetwView, CScrollView)

BEGIN_MESSAGE_MAP(CBtrsetwView, CScrollView)
	//{{AFX_MSG_MAP(CBtrsetwView)
	ON_COMMAND(ID_NETWORK_ADDNEWHOST, OnNetworkAddnewhost)
	ON_COMMAND(ID_NETWORK_CHANGEHOST, OnNetworkChangehost)
	ON_COMMAND(ID_NETWORK_DELETEHOST, OnNetworkDeletehost)
	ON_COMMAND(ID_NETWORK_SETASSERVER, OnNetworkSetasserver)
	ON_COMMAND(ID_OPTIONS_PROGRAMDEFAULTS, OnOptionsProgramdefaults)
	ON_COMMAND(ID_OPTIONS_CONDITIONDEFAULTS, OnOptionsConditiondefaults)
	ON_COMMAND(ID_OPTIONS_ASSIGNMENTDEFAULTS, OnOptionsAssignmentdefaults)
	ON_COMMAND(ID_OPTIONS_TIMEDEFAULTS, OnOptionsTimedefaults)
	ON_COMMAND(ID_OPTIONS_QUEUEDEFAULTS, OnOptionsQueuedefaults)
	ON_COMMAND(ID_OPTIONS_PROCESSPARAMETERS, OnOptionsProcessparameters)
	ON_COMMAND(ID_OPTIONS_RESTOREDEFAULTS, OnOptionsRestoredefaults)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SIZE()
	ON_COMMAND(ID_OPTIONS_PERMISSIONS, OnOptionsPermissions)
	ON_COMMAND(ID_OPTIONS_TIMELIMITS, OnOptionsTimelimits)
	ON_COMMAND(ID_PROGRAM_PORTSETTINGS, OnProgramPortsettings)
	ON_COMMAND(ID_PROGRAM_LOGINORLOGOUT, OnProgramLoginorlogout)
	ON_COMMAND(ID_PROGRAM_NEWGROUP, OnProgramNewgroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwView construction/destruction

CBtrsetwView::CBtrsetwView()
{
	m_nSelectedRow = -1;
}

CBtrsetwView::~CBtrsetwView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwView drawing

void CBtrsetwView::UpdateScrollSizes()
{        
	CClientDC dc(this);
	TEXTMETRIC tm;
	CRect rectClient;
	GetClientRect(&rectClient);
	dc.GetTextMetrics(&tm);
	m_nRowHeight = tm.tmHeight;
	m_nCharWidth = tm.tmAveCharWidth + 1;
	m_nRowWidth = ROW_WIDTH * m_nCharWidth;
	CSize sizePage(m_nRowWidth/5, max(m_nRowHeight, ((rectClient.bottom/m_nRowHeight)-1)*m_nRowHeight));
	CSize sizeLine(m_nRowWidth/20, m_nRowHeight);
	SetScrollSizes(MM_TEXT, CSize(m_nRowWidth, int(num_hosts()) * m_nRowHeight), sizePage, sizeLine);
}	

void CBtrsetwView::OnUpdate(CView*v, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
}

void CBtrsetwView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
{
	CBrush brushBackground;
	COLORREF crOldText = 0;
	COLORREF crOldBackground = 0;

	if  (bSelected) {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		crOldBackground = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		crOldText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else  {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
	}
	
	CRect rectSelection;
	pDC->GetClipBox(&rectSelection);
	rectSelection.top = y;
	rectSelection.bottom = y + m_nRowHeight;
	pDC->FillRect(&rectSelection, &brushBackground);

	remote	*ch = get_nth(unsigned(nRow));

	if  (ch)  {	
		TEXTMETRIC tm;
		pDC->GetTextMetrics(&tm);                                 
		const  char	*name = ch->hostname();
		pDC->TextOut(HOST_COL*tm.tmAveCharWidth, y, name, strlen(name));
		if  ((name = ch->aliasname()) && *name)
			pDC->TextOut(ALIAS_COL*tm.tmAveCharWidth, y, name, strlen(name));
		in_addr sin;
		sin.s_addr = ch->hostid;
		char  FAR *na = inet_ntoa(sin);
		CString  outs;
		if  (na)
			outs = na;
		else
			outs.LoadString(IDS_UNKNOWN);
		pDC->TextOut(INET_COL*tm.tmAveCharWidth, y, outs);
		char	timeb[TIMEOUT_LEN + 4];
		wsprintf(timeb, "%5u", ch->ht_timeout);
		pDC->TextOut(TIMEOUT_COL*tm.tmAveCharWidth, y, timeb, strlen(timeb));
		if  (ch->ht_flags & HT_PROBEFIRST)  {
			outs.LoadString(IDS_PROBE);
			pDC->TextOut(PROBE_COL*tm.tmAveCharWidth, y, outs);
		}	
		if  (ch->ht_flags & HT_SERVER)  {
			outs.LoadString(IDS_SERVER);
			pDC->TextOut(SERVER_COL*tm.tmAveCharWidth, y, outs);
		}	
	}   

	// Restore the DC.
	if (bSelected)	{
		pDC->SetBkColor(crOldBackground);
		pDC->SetTextColor(crOldText);
	}
}

void CBtrsetwView::OnDraw(CDC* pDC)
{
	if (num_hosts() == 0)
		return;
	int nFirstRow, nLastRow;
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	nFirstRow = rectClip.top / m_nRowHeight;
	nLastRow = min(unsigned(rectClip.bottom / m_nRowHeight) + 1, num_hosts());
	int nRow, y;
	for (nRow = nFirstRow, y = m_nRowHeight * nFirstRow; nRow < nLastRow; nRow++, y += m_nRowHeight)
		OnDrawRow(pDC, nRow, y, nRow == m_nSelectedRow);
}

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwView diagnostics

#ifdef _DEBUG
void CBtrsetwView::AssertValid() const
{
	CView::AssertValid();
}

void CBtrsetwView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CBtrsetwDoc* CBtrsetwView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBtrsetwDoc)));
	return (CBtrsetwDoc*) m_pDocument;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwView message handlers

void CBtrsetwView::OnNetworkAddnewhost()
{
	CHostdlg	hdlg;
	hdlg.m_probefirst = TRUE;
	hdlg.m_timeout = NETTICKLE;
	if  (hdlg.DoModal() == IDOK)  {
		remote	*rp = new remote(hdlg.m_hid, hdlg.m_hostname, hdlg.m_aliasname, hdlg.m_probefirst? HT_PROBEFIRST: 0, hdlg.m_timeout);
		rp->addhost();
		Invalidate();
		((CBtrsetwApp *) AfxGetApp())->dirty();
	}	
}

void CBtrsetwView::OnNetworkChangehost()
{
	if  (num_hosts() == 0)  {
		AfxMessageBox(IDP_NOHOSTSYET, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	if  (GetActiveRow() < 0)  {
		AfxMessageBox(IDP_NOTSELECTED, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	remote	*rp = get_nth(unsigned(GetActiveRow()));
	rp->delhost();
	CHostdlg	hdlg;
	unsigned  char	is_serv = (unsigned char) (rp->ht_flags & HT_SERVER);
	hdlg.m_probefirst = rp->ht_flags & HT_PROBEFIRST? TRUE: FALSE;
	hdlg.m_timeout = rp->ht_timeout;
	hdlg.m_hostname = rp->hostname();
	hdlg.m_aliasname = rp->aliasname()? rp->aliasname(): "";
	if  (hdlg.DoModal() == IDOK)  {
		remote	*nrp = new remote(hdlg.m_hid, hdlg.m_hostname, hdlg.m_aliasname, hdlg.m_probefirst? HT_PROBEFIRST|is_serv: is_serv, hdlg.m_timeout);
		nrp->addhost();
		delete  rp;
		((CBtrsetwApp *) AfxGetApp())->dirty();
	}
	else
		rp->addhost();
	Invalidate();
}

void CBtrsetwView::OnNetworkDeletehost()
{
	if  (num_hosts() == 0)  {
		AfxMessageBox(IDP_NOHOSTSYET, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	if  (GetActiveRow() < 0)  {
		AfxMessageBox(IDP_NOTSELECTED, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
		
	if  (AfxMessageBox(IDP_SUREDEL, MB_YESNO|MB_ICONQUESTION) == IDYES)  {
		remote	*rp = get_nth(unsigned(GetActiveRow()));
		rp->delhost();
		delete  rp;
		m_nSelectedRow = -1;
		Invalidate();
		((CBtrsetwApp *) AfxGetApp())->dirty();
	}
}

static	BOOL	dologin(CString unixh, const BOOL enqfirst)
{
	CBtrsetwApp  &ma = *((CBtrsetwApp *) AfxGetApp());
	int	ret;
	CString	newname;

	if  (enqfirst) {
		if  ((ret = xb_enquire(ma.m_winuser, ma.m_winmach, newname)) == 0)
			return  TRUE;
		if  (ret != IDP_XBENQ_PASSREQ)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			return  FALSE;
		}
	}
	
	CLoginHost	dlg;
	dlg.m_unixhost = unixh;
	dlg.m_clienthost = ma.m_winmach;
	dlg.m_username = ma.m_winuser;
	int	cnt = 0;
	for  (;;)  {
		if  (dlg.DoModal() != IDOK)
			return  FALSE;
		if  ((ret = xb_login(dlg.m_username, ma.m_winmach, (const char *) dlg.m_passwd, newname)) == 0)
			break;
		if  (ret != IDP_XBENQ_BADPASSWD || cnt >= 2)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			return  FALSE;
		}
		if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)
			return  FALSE;
		cnt++;
	}
	
	return  TRUE;
}

void CBtrsetwView::OnNetworkSetasserver()
{
	if  (num_hosts() == 0)  {
		AfxMessageBox(IDP_NOHOSTSYET, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	                                               
	if  (GetActiveRow() < 0)  {
		AfxMessageBox(IDP_NOTSELECTED, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	netid_t	oldserver = Locparams.servid;
	remote	*newserv = get_nth(unsigned(GetActiveRow()));
	Locparams.servid = newserv->hostid;
    if  (initenqsocket(Locparams.servid) != 0)  {
    	AfxMessageBox(IDP_NOTALIVE, MB_OK|MB_ICONSTOP);
giveup:
		Locparams.servid = oldserver;
		if  (Locparams.servid != 0)
			initenqsocket(Locparams.servid);
		return;
	}
	int	ret;
	Btuser	newbtu;
	CString	newuser, newgroup;
	CBtrsetwApp  &app = *(CBtrsetwApp *) AfxGetApp();

	if  (!dologin(newserv->hostname(), TRUE))
		goto  giveup;

	if  ((ret = getbtuser(newbtu, newuser, newgroup)) != 0)  {
		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		goto  giveup;
	}

	app.m_mypriv = newbtu;
	app.m_username = newuser;
	app.m_groupname = newgroup;
	
	//  Initialise queue default modes to that from server
	
	app.m_queuedefs.m_mode.u_flags = newbtu.btu_jflags[0];
	app.m_queuedefs.m_mode.g_flags = newbtu.btu_jflags[1];
	app.m_queuedefs.m_mode.o_flags = newbtu.btu_jflags[2];
	
	//  Check default priority against range
    
    if  (app.m_queuedefs.m_priority < newbtu.btu_minp || app.m_queuedefs.m_priority > newbtu.btu_maxp)
		app.m_queuedefs.m_priority = newbtu.btu_defp;

	unsigned  cnt = 0;
	remote	*rp;
	while  (rp = get_nth(cnt))  {
		rp->ht_flags &= ~HT_SERVER;
		cnt++;
	}		                                               
	newserv->ht_flags |= HT_SERVER;
	Invalidate();
	app.dirty();
	app.m_isvalid = TRUE;
}

void CBtrsetwView::OnOptionsProgramdefaults()
{
	Cprogopts	dlg;
	CBtrsetwApp  &app = *(CBtrsetwApp *) AfxGetApp();
	progdefs &pd = app.m_progdefs;
	dlg.m_jobqueue = pd.queuename;
	dlg.m_verbose = pd.verbose;
	dlg.m_binmode = pd.binmode;
	if  (dlg.DoModal() == IDOK)  {
		pd.queuename = dlg.m_jobqueue;
		pd.verbose = dlg.m_verbose;
		pd.binmode = dlg.m_binmode;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsConditiondefaults()
{
	CDefcond	dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	conddefs	&cd = app.m_conddefs;
	dlg.m_constval = cd.m_constval.IsEmpty()? "0": cd.m_constval;
	dlg.m_condop = cd.m_condop - C_EQ;
	dlg.m_condcrit = cd.m_condcrit;
	if  (dlg.DoModal() == IDOK)  {
		cd.m_constval = dlg.m_constval.GetLength() > 0 &&
						 (isdigit(dlg.m_constval[0]) || dlg.m_constval[0] == '-' || dlg.m_constval[0] == '"')?
						dlg.m_constval:
						'"' + dlg.m_constval + '"';
		cd.m_condop = dlg.m_condop + C_EQ;
		cd.m_condcrit = dlg.m_condcrit;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsAssignmentdefaults()
{
	Cdefass	dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	assdefs	&cd = app.m_assdefs;
	dlg.m_assval = cd.m_assvalue.IsEmpty()? "0": cd.m_assvalue;
	dlg.m_asstype = cd.m_asstype - BJA_ASSIGN;
	dlg.m_asscrit = cd.m_asscrit;
	dlg.m_astart = cd.m_astart;
	dlg.m_areverse = cd.m_areverse;
	dlg.m_anormal = cd.m_anormal;
	dlg.m_aerror = cd.m_aerror;
	dlg.m_aabort = cd.m_aabort;
	dlg.m_acancel = cd.m_acancel;
	if  (dlg.DoModal() == IDOK)  {
		cd.m_assvalue = dlg.m_assval.GetLength() > 0 &&
						 (isdigit(dlg.m_assval[0]) || dlg.m_assval[0] == '-' || dlg.m_assval[0] == '"')?
						dlg.m_assval:
						'"' + dlg.m_assval + '"';
		cd.m_asstype = dlg.m_asstype + BJA_ASSIGN;
		cd.m_asscrit = dlg.m_asscrit;
		cd.m_astart = dlg.m_astart;
		cd.m_areverse = dlg.m_areverse;
		cd.m_anormal = dlg.m_anormal;
		cd.m_aerror = dlg.m_aerror;
		cd.m_aabort = dlg.m_aabort;
		cd.m_acancel = dlg.m_acancel;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsTimedefaults()
{
	CTimedflt	dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	timedefs	&td = app.m_timedefs;
	dlg.m_nptype = td.m_nptype;
	dlg.m_repeatopt = td.m_repeatopt;
	dlg.m_avsun = td.m_avsun;
	dlg.m_avmon = td.m_avmon;
	dlg.m_avtue = td.m_avtue;
	dlg.m_avwed = td.m_avwed;
	dlg.m_avthu = td.m_avthu;
	dlg.m_avfri = td.m_avfri;
	dlg.m_avsat = td.m_avsat;
	dlg.m_avhol = td.m_avhol;
	dlg.m_tc_mday = td.m_tc_mday;
	dlg.m_tc_rate = td.m_tc_rate;
	if  (dlg.DoModal() == IDOK)  {
		td.m_nptype = dlg.m_nptype;
		td.m_repeatopt = dlg.m_repeatopt;
		td.m_avsun = dlg.m_avsun;
		td.m_avmon = dlg.m_avmon;
		td.m_avtue = dlg.m_avtue;
		td.m_avwed = dlg.m_avwed;
		td.m_avthu = dlg.m_avthu;
		td.m_avfri = dlg.m_avfri;
		td.m_avsat = dlg.m_avsat;
		td.m_avhol = dlg.m_avhol;
		td.m_tc_mday = dlg.m_tc_mday;
		td.m_tc_rate = dlg.m_tc_rate;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsQueuedefaults()
{
	CQueuedef	dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	queuedefs	&qd = app.m_queuedefs;
	
	dlg.m_title = qd.m_title;
	dlg.m_cmdint = qd.m_cmdinterp;
	dlg.m_priority = qd.m_priority;
	dlg.m_loadlev = qd.m_loadlev;
	dlg.m_hostid = Locparams.servid;
    if  (dlg.DoModal() == IDOK)  {
		qd.m_title = dlg.m_title;
		qd.m_cmdinterp = dlg.m_cmdint;
		qd.m_priority = dlg.m_priority;
		qd.m_loadlev = dlg.m_loadlev;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsProcessparameters()
{
	CProcpar  dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	queuedefs	&qd = app.m_queuedefs;
	dlg.m_directory = qd.m_directory;
	dlg.m_local = int(qd.m_export);
	dlg.m_advterr = int(qd.m_advterr);
	dlg.m_umask = qd.m_umask;
	dlg.m_ulimit = qd.m_ulimit;
	dlg.m_exits = qd.m_exits;
	if  (dlg.DoModal() == IDOK)  {         
		qd.m_directory = dlg.m_directory;
		qd.m_export = dlg.m_local > 1? queuedefs::REMRUN: dlg.m_local > 0 ? queuedefs::EXPORTF: queuedefs::LOCAL;
		qd.m_advterr = dlg.m_advterr > 0? queuedefs::NOADV: queuedefs::ADV;
		qd.m_umask = dlg.m_umask;
		qd.m_ulimit = dlg.m_ulimit;
		qd.m_exits = dlg.m_exits;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsRestoredefaults()
{
	if  (AfxMessageBox(IDP_SURERESTORE, MB_ICONQUESTION|MB_YESNO) != IDYES)
		return;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	progdefs &pd = app.m_progdefs;
	timedefs	&td = app.m_timedefs;
	conddefs	&cd = app.m_conddefs;
	assdefs	&ad = app.m_assdefs;
	queuedefs	&qd = app.m_queuedefs;
	pd.queuename = "";
	pd.verbose = FALSE;
	pd.binmode = FALSE;
	td.m_nptype = TC_WAIT1;
	td.m_repeatopt = TC_RETAIN;
	td.m_avsun = TRUE;
	td.m_avmon = FALSE;
	td.m_avtue = FALSE;
	td.m_avwed = FALSE;
	td.m_avthu = FALSE;
	td.m_avfri = FALSE;
	td.m_avsat = TRUE;
	td.m_avhol = FALSE;
	td.m_tc_mday = 0;
	td.m_tc_rate = 5;
	cd.m_constval = "";
	cd.m_condop = C_EQ;
	cd.m_condcrit = CCRIT_IGNORE;
	ad.m_assvalue = "1";
	ad.m_asstype = BJA_ASSIGN;
	ad.m_asscrit = ACRIT_IGNORE;
	ad.m_astart = TRUE;
	ad.m_areverse = TRUE;
	ad.m_anormal = TRUE;
	ad.m_aerror = TRUE;
	ad.m_aabort = TRUE;
	ad.m_acancel = FALSE;
	qd.m_title = "";
	qd.m_cmdinterp = "";
	qd.m_priority = app.m_mypriv.btu_defp;
	qd.m_loadlev = 0;
	qd.m_directory = "";
	qd.m_export = queuedefs::LOCAL;
	qd.m_advterr = queuedefs::ADV;
	qd.m_umask = app.m_myumask;
	qd.m_ulimit = app.m_myulimit;
	qd.m_exits.nlower = qd.m_exits.nupper = 0;
	qd.m_exits.elower = 1;
	qd.m_exits.eupper = 255;
	app.dirty();	
}
   
void CBtrsetwView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);
	CRect rect(point, CSize(1,1));
	int nFirstRow;
	nFirstRow = rect.top / m_nRowHeight;
	if (unsigned(nFirstRow) < num_hosts())  {
		m_nSelectedRow = nFirstRow;              
		GetDocument()->UpdateAllViews(NULL);
	}	
}

void CBtrsetwView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);
	OnNetworkChangehost();
}

void CBtrsetwView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);
	UpdateScrollSizes();
}

void CBtrsetwView::OnOptionsPermissions()
{
	CJobmode  dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	Btuser  &mypriv = app.m_mypriv;
	Btmode  &mmode = app.m_queuedefs.m_mode;
	dlg.m_umode = mmode.u_flags;
	dlg.m_gmode = mmode.g_flags;
	dlg.m_omode = mmode.o_flags;
	dlg.m_writeable = (mypriv.btu_priv & BTM_UMASK) != 0;
	dlg.m_user = app.m_progdefs.user.IsEmpty()? app.m_username: app.m_progdefs.user;
	dlg.m_group = app.m_progdefs.group.IsEmpty()? app.m_groupname: app.m_progdefs.group;
	if  (dlg.DoModal() == IDOK)  {
		mmode.u_flags = dlg.m_umode;
		mmode.g_flags = dlg.m_gmode;
		mmode.o_flags = dlg.m_omode;
//		app.m_progdefs.user = dlg.m_user;
//		app.m_progdefs.group = dlg.m_group;
		app.dirty();
	}
}

void CBtrsetwView::OnOptionsTimelimits()
{
	CTimelim  dlg;
	CBtrsetwApp	&app = *((CBtrsetwApp *) AfxGetApp());
	queuedefs	&qd = app.m_queuedefs;
	dlg.m_deltime = qd.m_deltime;
	dlg.m_runtime = qd.m_runtime;
	dlg.m_runon = qd.m_runon;
	dlg.m_writeable = TRUE;
	static  unsigned  char	siglist[8] = { UNIXSIGTERM, UNIXSIGKILL, UNIXSIGHUP, UNIXSIGINT,
										   UNIXSIGQUIT, UNIXSIGALRM, UNIXSIGBUS, UNIXSIGSEGV };
	for  (int signum = 0;  signum < 8;  signum++)
		if  (qd.m_autoksig == siglist[signum])  {
			dlg.m_autoksig = signum;
			goto  dun1;
		}
	dlg.m_autoksig = 1;		//  SIGKILL
dun1:
	if  (dlg.DoModal() == IDOK)  {         
		qd.m_deltime = dlg.m_deltime;
		qd.m_runtime = dlg.m_runtime;
		qd.m_runon = dlg.m_runon;
		qd.m_autoksig = siglist[dlg.m_autoksig];
		app.dirty();
	}
}

void CBtrsetwView::OnProgramPortsettings()
{
	CPortnums	dlg;
	dlg.DoModal();
}

void CBtrsetwView::OnProgramLoginorlogout() 
{
	CLoginOut	lodlg;
	CBtrsetwApp	&ma = *((CBtrsetwApp *) AfxGetApp());
	lodlg.m_winuser = ma.m_winuser;

	if  (ma.m_isvalid)
		lodlg.m_unixuser = ma.m_username;
	else
		lodlg.m_unixuser.LoadString(IDS_NOTLOGGED);

	int	ret = lodlg.DoModal();

	if  (ret == IDC_LOGOUT)  {
		if  ((ret = xb_logout()) != 0)
			AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		ma.m_isvalid = FALSE;
		return;
	}

	if  (ret != IDC_LOGIN)
		return;

	if  (dologin(look_host(Locparams.servid), FALSE))  {
		Btuser	newbtu;
		CString	newuser, newgroup;
	   	if  ((ret = getbtuser(newbtu, newuser, newgroup)) != 0)  {
			AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			ma.m_isvalid = FALSE;
			return;
		}
		ma.m_mypriv = newbtu;
		ma.m_username = newuser;
		ma.m_groupname = newgroup;
	
		//  Initialise queue default modes to that from server
	
		ma.m_queuedefs.m_mode.u_flags = newbtu.btu_jflags[0];
		ma.m_queuedefs.m_mode.g_flags = newbtu.btu_jflags[1];
		ma.m_queuedefs.m_mode.o_flags = newbtu.btu_jflags[2];
	
		//  Check default priority against range
    
		if  (ma.m_queuedefs.m_priority < newbtu.btu_minp || ma.m_queuedefs.m_priority > newbtu.btu_maxp)
			ma.m_queuedefs.m_priority = newbtu.btu_defp;
		
		ma.m_mypriv = newbtu;
		ma.m_isvalid = TRUE;
	}
	else
		ma.m_isvalid = FALSE;
}

void CBtrsetwView::OnProgramNewgroup() 
{
	CNewGrpDlg  dlg;
	CBtrsetwApp	&ma = *((CBtrsetwApp *) AfxGetApp());

	dlg.m_groupname = ma.m_groupname;
	if  (dlg.DoModal() == IDOK  &&  dlg.m_groupname != ma.m_groupname)  {
		int	ret = xb_newgrp(dlg.m_groupname);
		if  (ret != 0)
			AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
		else  {
			if  (ma.m_progdefs.group == ma.m_groupname)  {
				ma.m_progdefs.group = dlg.m_groupname;
				ma.dirty();
			}
			ma.m_groupname = dlg.m_groupname;
		}
	}
}
