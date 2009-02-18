#include "stdafx.h"
#include <direct.h>
#include "files.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "mainfrm.h"
#include "jdatadoc.h"
#include "jdatavie.h"
#include "btqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "jobdoc.h"
#include "jobview.h"
#include "vardoc.h"
#include "varview.h"
#include <direct.h>
#include "getregdata.h"
#include "loginhost.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern	BOOL	gethostparams();

/////////////////////////////////////////////////////////////////////////////
// CBtqwApp

BEGIN_MESSAGE_MAP(CBtqwApp, CWinApp)
	//{{AFX_MSG_MAP(CBtqwApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtqwApp construction

CBtqwApp::CBtqwApp()
{
	m_initok = FALSE;
	m_unsaved = FALSE;
	m_warnoffline = FALSE;
	m_polltime = 1000;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CBtqwApp object

CBtqwApp NEAR theApp;

const  char poptname[] = "Program options";
const  char tdefname[] = "Time defaults";
const  char cdefname[] = "Cond defaults";
const  char adefname[] = "Ass defaults";

char  FAR	basedir[_MAX_PATH];

/////////////////////////////////////////////////////////////////////////////
// CBtqwApp initialization

BOOL CBtqwApp::InitInstance()
{
	_getcwd(basedir, sizeof(basedir));
	strcat(basedir, "\\");

	if  (!getenv("TZ"))
		_putenv(DEFAULT_TZ);
	_tzset();

	loadprogopts();
	
	SetDialogBkColor(RGB(255,255,255));        // set dialog background color to white

	CMultiDocTemplate	*dtjob;
	CMultiDocTemplate	*dtvar;
	CMultiDocTemplate	*dtjdata;

	AddDocTemplate(dtjob = new CMultiDocTemplate(IDR_JOBS,
			RUNTIME_CLASS(CJobdoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CJobView)));
	AddDocTemplate(dtvar = new CMultiDocTemplate(IDR_VARS,
			RUNTIME_CLASS(CVardoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CVarView)));
	AddDocTemplate(dtjdata = new CMultiDocTemplate(IDR_VIEWJOB,
			RUNTIME_CLASS(CJdatadoc),
			RUNTIME_CLASS(CMDIChildWnd),        // standard MDI child frame
			RUNTIME_CLASS(CJdataview)));

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	pMainFrame->ShowWindow(SW_SHOWNORMAL);
	pMainFrame->UpdateWindow();
	pMainFrame->m_dtjob = dtjob;
	pMainFrame->m_dtvar = dtvar;
	pMainFrame->m_dtjdata = dtjdata;
	
	m_pMainWnd = pMainFrame;
	
	//  We only allow one of these things to happen at once
	
	if  (m_hPrevInstance != NULL)  {
		AfxMessageBox(IDP_PREVINSTANCE, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}
	UINT  eret = winsockstart();
    if  (eret != 0)  {
    	AfxMessageBox(eret, MB_OK|MB_ICONSTOP);
		return  FALSE;
    }
    if  ((eret = initenqsocket(Locparams.servid)) != 0)  {
    	AfxMessageBox(eret, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}
    
	//  New logic - first run enquiry.
	//  If that works, run getbtuser to get details etc.
	//  If enquiry doesn't work, then ask for password

#ifdef	REGSTRING
	m_winuser = GetRegString("Network\\Logon\\username");
	m_winmach = GetRegString("System\\CurrentControlSet\\Services\\VxD\\VNETSUP\\ComputerName");
#else
	GetUserAndComputerNames(m_winuser, m_winmach);
#endif

	int	ret;
	if  ((ret = xb_enquire(m_winuser, m_winmach, m_username)) != 0)  {
		if  (ret != IDP_XBENQ_PASSREQ)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			winsockend();
			return  FALSE;            
		}
		CLoginHost	dlg;
		dlg.m_unixhost = look_host(Locparams.servid);
		dlg.m_clienthost = m_winmach;
		dlg.m_username = m_winuser;
		int	cnt = 0;
		for  (;;)  {
			if  (dlg.DoModal() != IDOK)  {
				winsockend();
				return  FALSE;
			}
			if  ((ret = xb_login(dlg.m_username, m_winmach, (const char *) dlg.m_passwd, m_username)) == 0)
				break;
			if  (ret != IDP_XBENQ_BADPASSWD || cnt >= 2)  {
		    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				winsockend();
				return  FALSE;
			}
			if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)  {
				winsockend();
				return  FALSE;
			}
			cnt++;
		}
	}
	
    ret = getbtuser(m_mypriv, m_username, m_groupname);
    if  (ret != 0)  {
    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
    	winsockend();
    	return  FALSE;
    }
    if  (!gethostparams())  {
    	AfxMessageBox(IDP_NOCILIST, MB_OK|MB_ICONSTOP);
    	winsockend();
    	return  FALSE;
    }
	if  ((eret = initsockets()) != 0)  {
		AfxMessageBox(eret, MB_OK|MB_ICONSTOP);
		winsockend();
		return  FALSE;
	}
    m_initok = TRUE;
    pMainFrame->SetTimer(WM_NETMSG_TICKLE, Locparams.servtimeout*1000, NULL);
    attach_hosts();
	pMainFrame->InitWins();
	return TRUE;
}                   

int CBtqwApp::ExitInstance()
{
	if  (m_unsaved  &&  AfxMessageBox(IDP_SAVEPROPTS, MB_YESNO|MB_ICONQUESTION) == IDYES)
		saveprogopts();
	netshut();
	winsockend();
	return  m_initok? 0: 1;
}	

BOOL	CBtqwApp::OnIdle(long Count)
{
	if  (CWinApp::OnIdle(Count))
		return  TRUE;
	unsigned  long	todo;
	if  (ioctlsocket(Locparams.probesock, FIONREAD, &todo) != SOCKET_ERROR  &&  todo != 0)  {
		reply_probe();                                                                
		return  FALSE;
	}
	if  (Locparams.Netsync_req != 0)
		netsync();                  
	return  FALSE;
}

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CBtqwApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// Load/save program options.
// We set them in mainfram.cpp

static	short	gettcpport(const char *inifilename, const char *name, const short deflt)
{
	return  short(::GetPrivateProfileInt("TCP Ports", name, deflt, inifilename));
}

static	short	getudpport(const char *inifilename, const char *name, const short deflt)
{
	return  short(::GetPrivateProfileInt("UDP Ports", name, deflt, inifilename));
}

void  CBtqwApp::loadprogopts()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);    
    strcat(pfilepath, WININI);

	//  Initialise port numbers

	Locparams.lportnum = htons(gettcpport(pfilepath, "Listen", DEF_LISTEN_PORT));
	Locparams.vportnum = htons(gettcpport(pfilepath, "Feeder", DEF_FEEDER_PORT));
	Locparams.pportnum = htons(getudpport(pfilepath, "Probe", DEF_PROBE_PORT));
	Locparams.uaportnum = htons(getudpport(pfilepath, "Client", DEF_CLIENT_PORT));

	m_warnoffline = ::GetPrivateProfileInt(poptname, "Warnoffline", 0, pfilepath);
	m_polltime = ::GetPrivateProfileInt(poptname, "Polltime", 0, pfilepath);
	m_restrict.onlyl = ::GetPrivateProfileInt(poptname, "Localonly", 0, pfilepath) ? restrictdoc::YES: restrictdoc::NO;
	m_restrict.confd = ::GetPrivateProfileInt(poptname, "Confirmdel", 1, pfilepath)? restrictdoc::ALWAYS: restrictdoc::NEVER;
	m_sjbat = ::GetPrivateProfileInt(poptname, "Jobext", 0, pfilepath);
	char	jobq[200];
	if  (::GetPrivateProfileString(poptname, "Jobqueue", "", jobq, 200, pfilepath) > 0)
		m_restrict.queuename = jobq;
	m_restrict.incnull = ::GetPrivateProfileInt(poptname, "Incnull", 1, pfilepath);
	if  (::GetPrivateProfileString(poptname, "User", "", jobq, 200, pfilepath) > 0)
		m_restrict.user = jobq;
	if  (::GetPrivateProfileString(poptname, "Group", "", jobq, 200, pfilepath) > 0)
		m_restrict.group = jobq;
	if  (::GetPrivateProfileString(poptname, "Jfmt", "", jobq, 200, pfilepath) > 0)
		m_jfmt = jobq;
	else
		m_jfmt.LoadString(IDS_DEF_JFMT);
	if  (::GetPrivateProfileString(poptname, "Vfmt1", "", jobq, 200, pfilepath) > 0)
		m_v1fmt = jobq;
	else
		m_v1fmt.LoadString(IDS_DEF_V1FMT);
	for (unsigned cnt = BJP_NONE; cnt <= BJP_FINISHED;  cnt++)  {
		char  nbuf[12];
		wsprintf(nbuf, "jcolour%d", cnt);
		m_appcolours[cnt] = ::GetPrivateProfileInt(poptname, nbuf, 0, pfilepath);
	}

	m_timedefs.m_nptype = ::GetPrivateProfileInt(tdefname, "Notposs", TC_WAIT1, pfilepath);
	m_timedefs.m_repeatopt = ::GetPrivateProfileInt(tdefname, "Repeat", TC_RETAIN, pfilepath);
	m_timedefs.m_avsun = ::GetPrivateProfileInt(tdefname, "AvSun", 1, pfilepath);
	m_timedefs.m_avmon = ::GetPrivateProfileInt(tdefname, "AvMon", 0, pfilepath);
	m_timedefs.m_avtue = ::GetPrivateProfileInt(tdefname, "AvTue", 0, pfilepath);
	m_timedefs.m_avwed = ::GetPrivateProfileInt(tdefname, "AvWed", 0, pfilepath);
	m_timedefs.m_avthu = ::GetPrivateProfileInt(tdefname, "AvThu", 0, pfilepath);
	m_timedefs.m_avfri = ::GetPrivateProfileInt(tdefname, "AvFri", 0, pfilepath);
	m_timedefs.m_avsat = ::GetPrivateProfileInt(tdefname, "AvSat", 1, pfilepath);
	m_timedefs.m_avhol = ::GetPrivateProfileInt(tdefname, "AvHol", 0, pfilepath);
	m_timedefs.m_tc_mday = ::GetPrivateProfileInt(tdefname, "Monthday", 0, pfilepath);
	m_timedefs.m_tc_rate = ::GetPrivateProfileInt(tdefname, "Reprate", 5, pfilepath);
	if  (::GetPrivateProfileString(cdefname, "Value", "", jobq, 200, pfilepath) > 0)
		m_conddefs.m_constval = jobq;
	m_conddefs.m_condop = ::GetPrivateProfileInt(cdefname, "Op", C_EQ, pfilepath) - C_EQ;
	m_conddefs.m_condcrit = ::GetPrivateProfileInt(cdefname, "Crit", 0, pfilepath);
	if  (::GetPrivateProfileString(adefname, "Value", "", jobq, 200, pfilepath) > 0)
		m_assdefs.m_assvalue = jobq;
	m_assdefs.m_asstype = ::GetPrivateProfileInt(adefname, "Op", BJA_ASSIGN, pfilepath) - BJA_ASSIGN; 
	m_assdefs.m_asscrit = ::GetPrivateProfileInt(adefname, "Crit", 0, pfilepath);
	m_assdefs.m_astart = ::GetPrivateProfileInt(adefname, "Start", 1, pfilepath);
	m_assdefs.m_areverse = ::GetPrivateProfileInt(adefname, "Reverse", 1, pfilepath);
	m_assdefs.m_anormal = ::GetPrivateProfileInt(adefname, "Normal", 1, pfilepath);
	m_assdefs.m_aerror = ::GetPrivateProfileInt(adefname, "Error", 1, pfilepath);
	m_assdefs.m_aabort = ::GetPrivateProfileInt(adefname, "Abort", 1, pfilepath);
	m_assdefs.m_acancel = ::GetPrivateProfileInt(adefname, "Cancel", 0, pfilepath);
}

const	char	Booltrue[] = "1";
const	char	Boolfalse[] = "0";

void  CBtqwApp::saveprogopts()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);    
    strcat(pfilepath, WININI);

	char	obuf[80];
	wsprintf(obuf, "%d", m_warnoffline); 
	::WritePrivateProfileString(poptname, "Warnoffline", obuf, pfilepath);
	wsprintf(obuf, "%d", m_polltime);
	::WritePrivateProfileString(poptname, "Polltime", obuf, pfilepath);
	::WritePrivateProfileString(poptname, "Localonly", m_restrict.onlyl == restrictdoc::YES? Booltrue: Boolfalse, pfilepath);
	wsprintf(obuf, "%d", m_restrict.confd == restrictdoc::ALWAYS? 1: 0);
	::WritePrivateProfileString(poptname, "Confirmdel", obuf, pfilepath);
	wsprintf(obuf, "%d", m_sjbat);
	::WritePrivateProfileString(poptname, "Jobext", obuf, pfilepath);
	::WritePrivateProfileString(poptname, "Jobqueue", (const char *) m_restrict.queuename, pfilepath);
	::WritePrivateProfileString(poptname, "Incnull", m_restrict.incnull? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(poptname, "User", (const char *) m_restrict.user, pfilepath);
	::WritePrivateProfileString(poptname, "Group", (const char *) m_restrict.group, pfilepath);
	::WritePrivateProfileString(poptname, "Jfmt", (const char *) ('\"' + m_jfmt + '\"'), pfilepath);
	::WritePrivateProfileString(poptname, "Vfmt1", (const char *) ('\"' + m_v1fmt + '\"'), pfilepath);
	for (unsigned cnt = BJP_NONE; cnt <= BJP_FINISHED;  cnt++)  {
		char  nbuf[12];
		wsprintf(nbuf, "jcolour%d", cnt);
		wsprintf(obuf, "%lu", m_appcolours[cnt]);
		::WritePrivateProfileString(poptname, nbuf, obuf, pfilepath);
	}
	
	wsprintf(obuf, "%d", m_timedefs.m_nptype);
	::WritePrivateProfileString(tdefname, "Notposs", obuf, pfilepath);
	wsprintf(obuf, "%d", m_timedefs.m_repeatopt);
	::WritePrivateProfileString(tdefname, "Repeat", obuf, pfilepath);
	::WritePrivateProfileString(tdefname, "AvSun", m_timedefs.m_avsun? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvMon", m_timedefs.m_avmon? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvTue", m_timedefs.m_avtue? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvWed", m_timedefs.m_avwed? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvThu", m_timedefs.m_avthu? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvFri", m_timedefs.m_avfri? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvSat", m_timedefs.m_avsat? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(tdefname, "AvHol", m_timedefs.m_avhol? Booltrue: Boolfalse, pfilepath);
	wsprintf(obuf, "%d", m_timedefs.m_tc_mday);
	::WritePrivateProfileString(tdefname, "Monthday", obuf, pfilepath);
	wsprintf(obuf, "%ld", m_timedefs.m_tc_rate);
	::WritePrivateProfileString(tdefname, "Reprate", obuf, pfilepath);
	::WritePrivateProfileString(cdefname, "Value", (const char *)m_conddefs.m_constval, pfilepath);
	wsprintf(obuf, "%d", m_conddefs.m_condop + C_EQ);
	::WritePrivateProfileString(cdefname, "Op", obuf, pfilepath);
	::WritePrivateProfileString(cdefname, "Crit", m_conddefs.m_condcrit? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Value", (const char *)m_assdefs.m_assvalue, pfilepath);
	wsprintf(obuf, "%d", m_assdefs.m_asstype + BJA_ASSIGN);
	::WritePrivateProfileString(adefname, "Op", obuf, pfilepath);
	::WritePrivateProfileString(adefname, "Crit", m_assdefs.m_asscrit? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Start", m_assdefs.m_astart? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Reverse", m_assdefs.m_areverse? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Normal", m_assdefs.m_anormal? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Error", m_assdefs.m_aerror? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Abort", m_assdefs.m_aabort? Booltrue: Boolfalse, pfilepath);
	::WritePrivateProfileString(adefname, "Cancel", m_assdefs.m_acancel? Booltrue: Boolfalse, pfilepath);
		
	m_unsaved = FALSE;
}
