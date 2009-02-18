// btrw.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <direct.h>
#include "xbwnetwk.h"
#include "btrw.h"
#include "mainfrm.h"
#include "btrwdoc.h"
#include "btrwview.h"
#include "progopts.h"
#include "files.h"
#include "getregdata.h"
#include "loginhost.h"

extern	BOOL		gethostparams();
extern	void		parsecmdline(const char FAR *);
extern	void		parsecmdfile(const char FAR *);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBtrwApp

BEGIN_MESSAGE_MAP(CBtrwApp, CWinApp)
	//{{AFX_MSG_MAP(CBtrwApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_PROGRAMOPTIONS, OnFileProgramoptions)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_SAVEDEFAULTS, OnFileSavedefaults)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEDEFAULTS, OnUpdateFileSavedefaults)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtrwApp construction

CBtrwApp::CBtrwApp()
{
	m_initok = FALSE;
	m_binmode = FALSE;
	m_verbose = FALSE;
	m_nonexport = FALSE;
	m_promptfirst = FALSE;
	m_jobqueue = "";
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CBtrwApp object

CBtrwApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// CBtrwApp initialization

char  FAR	basedir[_MAX_PATH];

inline	short	getudpport(const char *inifilename, const char *name, const short deflt)
{
	return  short(::GetPrivateProfileInt("UDP Ports", name, deflt, inifilename));
}

BOOL CBtrwApp::InitInstance()
{
	_getcwd(basedir, sizeof(basedir));
	strcat(basedir, "\\");

	if  (!getenv("TZ"))
		_putenv(DEFAULT_TZ);
	_tzset();
	SetDialogBkColor(RGB(255,255,255));
	LoadStdProfileSettings();

    m_jeditor = GetProfileString("OPTIONS", "JEDITOR", "NOTEPAD");
	m_verbose =	GetProfileInt("OPTIONS", "Verbose", 0) != 0;
	m_nonexport = GetProfileInt("OPTIONS", "NonExport", 0) != 0;

	char	pfilepath[_MAX_PATH];
    strcpy(pfilepath, basedir);
    strcat(pfilepath, WININI);
	Locparams.uaportnum = htons(getudpport(pfilepath, "Client", DEF_CLIENT_PORT));

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CBtrwDoc),
		RUNTIME_CLASS(CMainFrame),     // main SDI frame window
		RUNTIME_CLASS(CBtrwView));
	AddDocTemplate(pDocTemplate);

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
	//  If that works, run getspuser to get details etc.
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
   	m_initok = TRUE;
	loaddefault();

	if (m_lpCmdLine[0] == '\0')  {
		OnFileNew();
	    return TRUE;
	}
	else  {
		char  FAR  *sp = strrchr(m_lpCmdLine, '.');
		if  (sp)  {
			sp++;
			if  (stricmp(sp, "xbc") == 0  ||  stricmp(sp, "bat") == 0)  {
				parsecmdfile(m_lpCmdLine);                                   
				return  FALSE;
			}
		}
		parsecmdline(m_lpCmdLine);
		return FALSE;
	}
}

int CBtrwApp::ExitInstance()
{
	winsockend();
	if  (isdefdirty()  &&  AfxMessageBox(IDP_SAVEDEFAULTSQ, MB_YESNO|MB_ICONQUESTION) == IDYES)
		OnFileSavedefaults();		
	CWinApp::ExitInstance();
	return  m_initok? 0: 1;
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
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
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
void CBtrwApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CBtrwApp commands

void CBtrwApp::OnFileProgramoptions()
{
	CProgopts	dlg;
	
	dlg.m_jeditor = m_jeditor;
	dlg.m_verbose = m_verbose;
	dlg.m_binmode = m_binmode;
	dlg.m_jobqueue = m_jobqueue;
	dlg.m_nonexport = m_nonexport;
	if  (dlg.DoModal() == IDOK)  {
		m_jeditor = dlg.m_jeditor;
		m_verbose = dlg.m_verbose;
		m_binmode = dlg.m_binmode;
		m_jobqueue = dlg.m_jobqueue;
		m_nonexport = dlg.m_nonexport;
		WriteProfileString("OPTIONS", "JEDITOR", m_jeditor);
		WriteProfileInt("OPTIONS", "Verbose", m_verbose? 1: 0);
		WriteProfileInt("OPTIONS", "NonExport", m_nonexport? 1: 0);
	}
}

void	CBtrwApp::FileOpen()
{
	CWinApp::OnFileOpen();
}

void CBtrwApp::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(isdirty());
}

void	CBtrwApp::loaddefault()
{
	m_defaultjob.optread();
}

void CBtrwApp::OnFileSavedefaults()
{
	m_defaultjob.optcopy();
	defclean();
}

void CBtrwApp::OnUpdateFileSavedefaults(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(isdefdirty());
}
