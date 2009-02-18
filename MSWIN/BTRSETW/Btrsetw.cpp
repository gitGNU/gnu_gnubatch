// btrsetw.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <direct.h>
#include "xbwnetwk.h"
#include "files.h"  
#include "btrsetw.h"
#include "mainfrm.h"
#include "btrsedoc.h"
#include "btrsevw.h"
#include "ulist.h"
#include <ctype.h>
#include <string.h>
#include "getregdata.h"
#include "loginhost.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwApp

BEGIN_MESSAGE_MAP(CBtrsetwApp, CWinApp)
	//{{AFX_MSG_MAP(CBtrsetwApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwApp construction

CBtrsetwApp::CBtrsetwApp()
{
	m_needsave = FALSE;
	m_isvalid = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CBtrsetwApp object

CBtrsetwApp NEAR theApp;

// We set them in mainfram.cpp

const  char poptname[] = "Program options";
const  char tdefname[] = "Time defaults";
const  char cdefname[] = "Cond defaults";
const  char adefname[] = "Ass defaults";
const  char	qdefname[] = "Queue defaults";

char  basedir[_MAX_PATH];

extern	const	int	getuml(UINT &, unsigned long &, CIList &);

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwApp initialization

BOOL CBtrsetwApp::InitInstance()
{
	_getcwd(basedir, sizeof(basedir));
	strcat(basedir, "\\");

	if  (!getenv("TZ"))
		_putenv(DEFAULT_TZ);
	_tzset();
	SetDialogBkColor(RGB(255,255,255));

	// Register the application's document templates.  Document templates
	// serve as the connection between documents, frame windows and views.

	AddDocTemplate(new CSingleDocTemplate(IDR_MAINFRAME,
			RUNTIME_CLASS(CBtrsetwDoc),
			RUNTIME_CLASS(CMainFrame),     // main SDI frame window
			RUNTIME_CLASS(CBtrsetwView)));

	//  We only allow one of these things to happen at once.

	if  (m_hPrevInstance != NULL)  {
		AfxMessageBox(IDP_PREVINSTANCE, MB_OK|MB_ICONSTOP);
		return  FALSE;
	}

	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, WININI);
	Locparams.uaportnum = htons(short(::GetPrivateProfileInt("UDP Ports", "Client", DEF_CLIENT_PORT, pfilepath)));

	int  ret = winsockstart();
    if  (ret != 0)  {
		if  (ret == IDP_ALIASCLASH)
			ret = IDP_NO_HOSTFILE;
		if  (ret == IDP_NO_HOSTFILE  ||  ret == IDP_HF_INVALIDHOST)  {
			if  (AfxMessageBox(ret, MB_ICONWARNING|MB_OKCANCEL) != IDOK)
				return  FALSE;
			m_isvalid = FALSE;
		}
		else  if  (ret == IDP_NO_SERVER)  {
    		AfxMessageBox(ret, MB_OK|MB_ICONWARNING);
			m_isvalid = FALSE;
		}
		else  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			return  FALSE;
		}
    }
	
	if  (m_isvalid  &&  (ret = initenqsocket(Locparams.servid)) != 0)  {
		AfxMessageBox(ret, MB_OK|MB_ICONWARNING);
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
	if  (!m_isvalid)
		goto  nogood;

	if  ((ret = xb_enquire(m_winuser, m_winmach, m_username)) != 0)  {
		if  (ret != IDP_XBENQ_PASSREQ)  {
	    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
			m_isvalid = FALSE;
			goto  madeit;            
		}
		CLoginHost	dlg;
		dlg.m_unixhost = look_host(Locparams.servid);
		dlg.m_clienthost = m_winmach;
		dlg.m_username = m_winuser;
		int	cnt = 0;
		for  (;;)  {
			if  (dlg.DoModal() != IDOK)  {
				m_isvalid = FALSE;
				goto  madeit;
			}
			if  ((ret = xb_login(dlg.m_username, m_winmach, (const char *) dlg.m_passwd, m_username)) == 0)
				break;
			if  (ret != IDP_XBENQ_BADPASSWD || cnt >= 2)  {
		    	AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
				m_isvalid = FALSE;
				goto  madeit;
			}
			if  (AfxMessageBox(ret, MB_RETRYCANCEL|MB_ICONQUESTION) == IDCANCEL)  {
				m_isvalid = FALSE;
				goto  madeit;
			}
			cnt++;
		}
	}

	//  And now do the business...

	if  ((ret = getbtuser(m_mypriv, m_username, m_groupname)) != 0)  {
		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
    	m_isvalid = FALSE;
		goto  madeit;
	}
	//m_progdefs.user = m_username;
	//m_progdefs.group = m_groupname;

    if  ((ret = getuml(m_myumask, m_myulimit, m_cilist)) != 0)  {
   		AfxMessageBox(ret, MB_OK|MB_ICONSTOP);
   		m_isvalid = FALSE;
    }

madeit:
    loadprogopts();
nogood:
	OnFileNew();
	return TRUE;
}

int CBtrsetwApp::ExitInstance()
{
	if  (m_needsave  &&  AfxMessageBox(IDP_SAVEPROPTS, MB_YESNO|MB_ICONQUESTION) == IDYES)  {
		if  (m_isvalid)
			saveprogopts();
		savehostfile();
	}
	winsockend();
	return  m_isvalid? 0: 1;
}	

void	CBtrsetwApp::loadprogopts()
{
    char	cbuf[256];
    char	pfilepath[_MAX_PATH];

	strcpy(pfilepath, basedir);    
    strcat(pfilepath, WININI);

	//  Program Options

	m_progdefs.verbose = ::GetPrivateProfileInt(poptname, "Verbose", 0, pfilepath);
	m_progdefs.binmode = ::GetPrivateProfileInt(poptname, "Binmode", 0, pfilepath);
	if  (::GetPrivateProfileString(poptname, "Jobqueue", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_progdefs.queuename = cbuf;
	if  (::GetPrivateProfileString(poptname, "User", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_progdefs.user = cbuf;
	if  (::GetPrivateProfileString(poptname, "Group", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_progdefs.group = cbuf;
	
	//  Time defaults
	
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

	//  Condition defaults
	
	if  (::GetPrivateProfileString(cdefname, "Value", "", cbuf, 80, pfilepath) > 0)
		m_conddefs.m_constval = cbuf;
	m_conddefs.m_condop = ::GetPrivateProfileInt(cdefname, "Op", 0, pfilepath);
	m_conddefs.m_condcrit = ::GetPrivateProfileInt(cdefname, "Crit", 0, pfilepath);
	
	//  Assignment defaults
	
	if  (::GetPrivateProfileString(adefname, "Value", "", cbuf, 80, pfilepath) > 0)
		m_assdefs.m_assvalue = cbuf;
	m_assdefs.m_asstype = ::GetPrivateProfileInt(adefname, "Op", 0, pfilepath); 
	m_assdefs.m_asscrit = ::GetPrivateProfileInt(adefname, "Crit", 0, pfilepath);
	m_assdefs.m_astart = ::GetPrivateProfileInt(adefname, "Start", 1, pfilepath);
	m_assdefs.m_areverse = ::GetPrivateProfileInt(adefname, "Reverse", 1, pfilepath);
	m_assdefs.m_anormal = ::GetPrivateProfileInt(adefname, "Normal", 1, pfilepath);
	m_assdefs.m_aerror = ::GetPrivateProfileInt(adefname, "Error", 1, pfilepath);
	m_assdefs.m_aabort = ::GetPrivateProfileInt(adefname, "Abort", 1, pfilepath);
	m_assdefs.m_acancel = ::GetPrivateProfileInt(adefname, "Cancel", 0, pfilepath);
    
    //  Now for queueing options....
    
    m_queuedefs.m_progress = ::GetPrivateProfileInt(qdefname, "Cancelled", 0, pfilepath) ? queuedefs::CANCELLED: queuedefs::RUNNABLE;
	m_queuedefs.m_priority = unsigned(::GetPrivateProfileInt(qdefname, "Priority", m_mypriv.btu_defp, pfilepath));
    m_queuedefs.m_export = ::GetPrivateProfileInt(qdefname, "RemRun", 0, pfilepath) ? queuedefs::REMRUN :
                           ::GetPrivateProfileInt(qdefname, "Export", 0, pfilepath) ? queuedefs::EXPORTF :
                           queuedefs::LOCAL;

	if  (::GetPrivateProfileString(qdefname, "Directory", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_queuedefs.m_directory = cbuf;
	if  (::GetPrivateProfileString(qdefname, "Title", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_queuedefs.m_title = cbuf;
	if  (::GetPrivateProfileString(qdefname, "Interp", "", cbuf, sizeof(cbuf), pfilepath) > 0)
		m_queuedefs.m_cmdinterp = cbuf;
		
    m_queuedefs.m_loadlev = unsigned(::GetPrivateProfileInt(qdefname, "Loadlev", 0, pfilepath));                      
	m_queuedefs.m_umask = unsigned(::GetPrivateProfileInt(qdefname, "Umask", m_myumask, pfilepath));
	if  (::GetPrivateProfileString(qdefname, "Ulimit", "", cbuf, sizeof(cbuf), pfilepath) > 0  &&  isdigit(cbuf[0]))
		m_queuedefs.m_ulimit = strtoul(cbuf, (char **) 0, 0);
	else
		m_queuedefs.m_ulimit = m_myulimit? m_myulimit: 0x80000;
	m_queuedefs.m_deltime = unsigned(::GetPrivateProfileInt(qdefname, "Deltime", 0, pfilepath));
	m_queuedefs.m_autoksig = unsigned(::GetPrivateProfileInt(qdefname, "Whichsig", 0, pfilepath));
	m_queuedefs.m_runon = unsigned(::GetPrivateProfileInt(qdefname, "Runon", 0, pfilepath));
	if  (::GetPrivateProfileString(qdefname, "Runtime", "0", cbuf, sizeof(cbuf), pfilepath) < 0)
		m_queuedefs.m_runtime = 0;
	else
		m_queuedefs.m_runtime = strtoul(cbuf, (char **) 0, 0);
	if  (::GetPrivateProfileString(qdefname, "Normal", "0,0", cbuf, sizeof(cbuf), pfilepath) > 0)  {
	    m_queuedefs.m_exits.nlower = isdigit(cbuf[0]) ? atoi(cbuf): 0;
	    char  *cp;
	    if  ((cp = strchr(cbuf, ',')) && isdigit(cp[1]))
	    	m_queuedefs.m_exits.nupper = atoi(cp+1);
	    else
    		m_queuedefs.m_exits.nupper = m_queuedefs.m_exits.nlower;
    }
	if  (::GetPrivateProfileString(qdefname, "Error", "1,255", cbuf, sizeof(cbuf), pfilepath) > 0)  {
	    m_queuedefs.m_exits.elower = isdigit(cbuf[0]) ? atoi(cbuf): 1;
	    char  *cp;
	    if  ((cp = strchr(cbuf, ',')) && isdigit(cp[1]))
	    	m_queuedefs.m_exits.eupper = atoi(cp+1);
	    else
    		m_queuedefs.m_exits.eupper = 255;
    }

    m_queuedefs.m_advterr = ::GetPrivateProfileInt(qdefname, "Noadv", 0, pfilepath) ? queuedefs::NOADV: queuedefs::ADV;

	m_queuedefs.m_mode.u_flags = unsigned(::GetPrivateProfileInt(qdefname, "JUmode", m_mypriv.btu_jflags[0], pfilepath));
	m_queuedefs.m_mode.g_flags = unsigned(::GetPrivateProfileInt(qdefname, "JGmode", m_mypriv.btu_jflags[1], pfilepath));
	m_queuedefs.m_mode.o_flags = unsigned(::GetPrivateProfileInt(qdefname, "JOmode", m_mypriv.btu_jflags[2], pfilepath));    
}

static	void	WritePrivateProfileInt(const char *section,
								 	   const char *field,
								 	   const long item,
								 	   const char *pfilename)
{
	char	nbuf[20];
	wsprintf(nbuf, "%ld", item);
	WritePrivateProfileString(section, field, nbuf, pfilename);
}

inline	void	WritePrivateProfileBool(const char *section,
								 	    const char *field,
								 	    const BOOL item,
								 	    const char *pfilename)
{
	WritePrivateProfileInt(section, field, item? 1: 0, pfilename);
}
									 	    
void	CBtrsetwApp::saveprogopts()
{
    char	pfilepath[_MAX_PATH];
    
	strcpy(pfilepath, basedir);    
    strcat(pfilepath, WININI);

	//  Program Options

	WritePrivateProfileBool(poptname, "Verbose", m_progdefs.verbose, pfilepath);
	WritePrivateProfileBool(poptname, "Binmode", m_progdefs.binmode, pfilepath);
	WritePrivateProfileString(poptname, "Jobqueue", m_progdefs.queuename, pfilepath);
	WritePrivateProfileString(poptname, "User", m_progdefs.user, pfilepath);
	WritePrivateProfileString(poptname, "Group", m_progdefs.group, pfilepath);
	
	//  Time defaults
	
	WritePrivateProfileInt(tdefname, "Notposs", m_timedefs.m_nptype, pfilepath);
	WritePrivateProfileInt(tdefname, "Repeat", m_timedefs.m_repeatopt, pfilepath);
	WritePrivateProfileBool(tdefname, "AvSun", m_timedefs.m_avsun, pfilepath);
	WritePrivateProfileBool(tdefname, "AvMon", m_timedefs.m_avmon, pfilepath);
	WritePrivateProfileBool(tdefname, "AvTue", m_timedefs.m_avtue, pfilepath);
	WritePrivateProfileBool(tdefname, "AvWed", m_timedefs.m_avwed, pfilepath);
	WritePrivateProfileBool(tdefname, "AvThu", m_timedefs.m_avthu, pfilepath);
	WritePrivateProfileBool(tdefname, "AvFri", m_timedefs.m_avfri, pfilepath);
	WritePrivateProfileBool(tdefname, "AvSat", m_timedefs.m_avsat, pfilepath);
	WritePrivateProfileBool(tdefname, "AvHol", m_timedefs.m_avhol, pfilepath);
	WritePrivateProfileInt(tdefname, "Monthday", m_timedefs.m_tc_mday, pfilepath);
	WritePrivateProfileInt(tdefname, "Reprate", m_timedefs.m_tc_rate, pfilepath);

	//  Condition defaults
	
	WritePrivateProfileString(cdefname, "Value", m_conddefs.m_constval, pfilepath);
	WritePrivateProfileInt(cdefname, "Op", m_conddefs.m_condop, pfilepath);
	WritePrivateProfileInt(cdefname, "Crit", m_conddefs.m_condcrit, pfilepath);
	
	//  Assignment defaults
	
	WritePrivateProfileString(adefname, "Value", m_assdefs.m_assvalue, pfilepath);
	WritePrivateProfileInt(adefname, "Op", m_assdefs.m_asstype, pfilepath);
	WritePrivateProfileInt(adefname, "Crit", m_assdefs.m_asscrit, pfilepath);
	WritePrivateProfileBool(adefname, "Start", m_assdefs.m_astart, pfilepath);
	WritePrivateProfileBool(adefname, "Reverse", m_assdefs.m_areverse, pfilepath);
	WritePrivateProfileBool(adefname, "Normal", m_assdefs.m_anormal, pfilepath);
	WritePrivateProfileBool(adefname, "Error", m_assdefs.m_aerror, pfilepath);
	WritePrivateProfileBool(adefname, "Abort", m_assdefs.m_aabort, pfilepath);
	WritePrivateProfileBool(adefname, "Cancel", m_assdefs.m_acancel, pfilepath);
    
    //  Now for queueing options....
    
    switch  (m_queuedefs.m_progress)  {
    default:
    	WritePrivateProfileInt(qdefname, "Cancelled", 1, pfilepath);
    	break;
    case  queuedefs::RUNNABLE:
    	WritePrivateProfileInt(qdefname, "Cancelled", 0, pfilepath);
    	break;
    }
    
    WritePrivateProfileInt(qdefname, "Priority", m_queuedefs.m_priority, pfilepath);
    
    switch  (m_queuedefs.m_export)  {
    default:
    case  queuedefs::LOCAL:
	    WritePrivateProfileInt(qdefname, "Export", 0, pfilepath);
	    WritePrivateProfileInt(qdefname, "RemRun", 0, pfilepath);
        break;
    case  queuedefs::EXPORTF:
	    WritePrivateProfileInt(qdefname, "Export", 1, pfilepath);
	    WritePrivateProfileInt(qdefname, "RemRun", 0, pfilepath);
        break;
    case  queuedefs::REMRUN:
	    WritePrivateProfileInt(qdefname, "Export", 1, pfilepath);
	    WritePrivateProfileInt(qdefname, "RemRun", 1, pfilepath);
        break;
    }

	WritePrivateProfileString(qdefname, "Directory", m_queuedefs.m_directory, pfilepath);
	WritePrivateProfileString(qdefname, "Title", m_queuedefs.m_title, pfilepath);
	WritePrivateProfileString(qdefname, "Interp", m_queuedefs.m_cmdinterp, pfilepath);
    WritePrivateProfileInt(qdefname, "Loadlev", m_queuedefs.m_loadlev, pfilepath);
    WritePrivateProfileInt(qdefname, "Umask", m_queuedefs.m_umask, pfilepath);
    WritePrivateProfileInt(qdefname, "Ulimit", m_queuedefs.m_ulimit, pfilepath);
    WritePrivateProfileInt(qdefname, "Deltime", m_queuedefs.m_deltime, pfilepath);
    WritePrivateProfileInt(qdefname, "Runtime", m_queuedefs.m_runtime, pfilepath);
    WritePrivateProfileInt(qdefname, "Whichsig", m_queuedefs.m_autoksig, pfilepath);
    WritePrivateProfileInt(qdefname, "Runon", m_queuedefs.m_runon, pfilepath);
	char	nbuf[20];
	wsprintf(nbuf, "%d,%d", m_queuedefs.m_exits.nlower, m_queuedefs.m_exits.nupper);
	WritePrivateProfileString(qdefname, "Normal", nbuf, pfilepath);
	wsprintf(nbuf, "%d,%d", m_queuedefs.m_exits.elower, m_queuedefs.m_exits.eupper);
	WritePrivateProfileString(qdefname, "Error", nbuf, pfilepath);
    switch  (m_queuedefs.m_advterr)  {
    default:
    	WritePrivateProfileInt(qdefname, "Noadv", 0, pfilepath);
    	break;
    case  queuedefs::NOADV:
    	WritePrivateProfileInt(qdefname, "Noadv", 1, pfilepath);
    	break;
    }
    WritePrivateProfileInt(qdefname, "JUmode", m_queuedefs.m_mode.u_flags, pfilepath);
    WritePrivateProfileInt(qdefname, "JGmode", m_queuedefs.m_mode.g_flags, pfilepath);
    WritePrivateProfileInt(qdefname, "JOmode", m_queuedefs.m_mode.o_flags, pfilepath);
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

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
void CBtrsetwApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwApp commands
