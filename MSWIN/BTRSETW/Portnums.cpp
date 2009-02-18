// portnums.cpp : implementation file
//

#include "stdafx.h"
#include "files.h"
#include "xbwnetwk.h"
#include "btrsetw.h"
#include "portnums.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPortnums dialog


CPortnums::CPortnums(CWnd* pParent /*=NULL*/)
	: CDialog(CPortnums::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPortnums)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CPortnums::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPortnums)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPortnums, CDialog)
	//{{AFX_MSG_MAP(CPortnums)
	ON_BN_CLICKED(IDC_SAVESETTINGS, OnSavesettings)
	ON_BN_CLICKED(IDC_SETDEFAULT, OnSetdefault)
	ON_BN_CLICKED(IDC_APPLYINC, OnApplyinc)
	ON_BN_CLICKED(IDC_APPLYDEC, OnApplydec)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

char	TCP_sect[] = "TCP Ports";
char	UDP_sect[] = "UDP Ports";

struct	pstr	{
	char	*sectname;
	char	*entname;
	int		dlgid;
	int		scr_dlgid;
	int		default_val;
};

struct	pstr	pstr_list[] = {
	{	TCP_sect,	"Listen",	IDC_CONNPORT,	IDC_SCR_CONNPORT,	DEF_LISTEN_PORT	},
	{	TCP_sect,	"Feeder",	IDC_VIEWPORT,	IDC_SCR_VIEWPORT,	DEF_FEEDER_PORT	},
	{	UDP_sect,	"Probe",	IDC_PROBEPORT,	IDC_SCR_PROBEPORT,	DEF_PROBE_PORT },
	{	UDP_sect,	"Client",	IDC_CLIENTPORT,	IDC_SCR_CLIENTPORT,	DEF_CLIENT_PORT },
	{	TCP_sect,	"API",		IDC_APITCPPORT,	IDC_SCR_APITCPPORT,	DEF_APITCP_PORT },
	{	UDP_sect,	"API",		IDC_APIUDPPORT,	IDC_SCR_APIUDPPORT,	DEF_APIUDP_PORT }
};

extern	char	basedir[];

/////////////////////////////////////////////////////////////////////////////
// CPortnums message handlers

BOOL CPortnums::OnInitDialog()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, WININI);
	CDialog::OnInitDialog();
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		SetDlgItemInt(w.dlgid, ::GetPrivateProfileInt(w.sectname, w.entname, w.default_val, pfilepath));
		((CSpinButtonCtrl *) GetDlgItem(w.scr_dlgid))->SetRange(1, SHRT_MAX);
	}
	SetDlgItemInt(IDC_INCALLBY, 1000);
	SetDlgItemInt(IDC_CLIENTPORT, ntohs(Locparams.uaportnum));
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_INCALLBY))->SetRange(1, SHRT_MAX);
	return TRUE;
}

void CPortnums::OnSavesettings()
{
	char	pfilepath[_MAX_PATH];
	strcpy(pfilepath, basedir);
    strcat(pfilepath, WININI);
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		char	nbuf[20];
		wsprintf(nbuf, "%d", GetDlgItemInt(w.dlgid));
		::WritePrivateProfileString(w.sectname, w.entname, nbuf, pfilepath);
	}
}

void CPortnums::OnSetdefault()
{
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		SetDlgItemInt(w.dlgid, w.default_val);
	}
}

void CPortnums::OnApplyinc()
{
	int	incr = GetDlgItemInt(IDC_INCALLBY);
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		long	result = num + incr;
		if  (result > 32767L)
			return;
	}
	for  (cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		SetDlgItemInt(w.dlgid, num + incr);
	}
}

void CPortnums::OnApplydec()
{
	int	incr = GetDlgItemInt(IDC_INCALLBY);
	for  (int cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		long	result = num - incr;
		if  (result <= 0l)
			return;
	}
	for  (cnt = 0;  cnt < sizeof(pstr_list)/sizeof(struct pstr);  cnt++)  {
		pstr	&w = pstr_list[cnt];
		int		num = GetDlgItemInt(w.dlgid);
		SetDlgItemInt(w.dlgid, num - incr);
	}
}

void CPortnums::OnOK()
{
	Locparams.uaportnum = htons(GetDlgItemInt(IDC_CLIENTPORT));
	CDialog::OnOK();
}

const DWORD a109HelpIDs[]=
{
	IDC_INCALLBY,	IDH_109_254,	// Port Numbers: "0" (Edit)
	IDC_SCR_INCALLBY,	IDH_109_254,	// Port Numbers: "Spin1" (msctls_updown32)
	IDC_APPLYINC,	IDH_109_256,	// Port Numbers: "Incr" (Button)
	IDC_APPLYDEC,	IDH_109_257,	// Port Numbers: "Decr" (Button)
	IDC_CLIENTPORT,	IDH_109_258,	// Port Numbers: "0" (Edit)
	IDC_SCR_CLIENTPORT,	IDH_109_258,	// Port Numbers: "Spin2" (msctls_updown32)
	IDC_PROBEPORT,	IDH_109_260,	// Port Numbers: "0" (Edit)
	IDC_SCR_PROBEPORT,	IDH_109_260,	// Port Numbers: "Spin3" (msctls_updown32)
	IDC_CONNPORT,	IDH_109_262,	// Port Numbers: "0" (Edit)
	IDC_SCR_CONNPORT,	IDH_109_262,	// Port Numbers: "Spin4" (msctls_updown32)
	IDC_VIEWPORT,	IDH_109_264,	// Port Numbers: "0" (Edit)
	IDC_SCR_VIEWPORT,	IDH_109_264,	// Port Numbers: "Spin8" (msctls_updown32)
	IDC_APITCPPORT,	IDH_109_266,	// Port Numbers: "0" (Edit)
	IDC_SCR_APITCPPORT,	IDH_109_266,	// Port Numbers: "Spin6" (msctls_updown32)
	IDC_APIUDPPORT,	IDH_109_268,	// Port Numbers: "0" (Edit)
	IDC_SCR_APIUDPPORT,	IDH_109_268,	// Port Numbers: "Spin7" (msctls_updown32)
	IDC_SAVESETTINGS,	IDH_109_270,	// Port Numbers: "Save Settings to INI file" (Button)
	IDC_SETDEFAULT,	IDH_109_253,	// Port Numbers: "Set Default" (Button)
	0, 0
};

BOOL CPortnums::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a109HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
