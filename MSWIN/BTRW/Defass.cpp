// defass.cpp : implementation file
//

#include "stdafx.h"
#include "xbwnetwk.h"
#include "btrw.h"
#include "defass.h"
#include <ctype.h>
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Cdefass dialog

Cdefass::Cdefass(CWnd* pParent /*=NULL*/)
	: CDialog(Cdefass::IDD, pParent)
{
	//{{AFX_DATA_INIT(Cdefass)
	m_assvalue = "";
	m_asstype = -1;
	m_asscrit = -1;
	m_astart = FALSE;
	m_areverse = FALSE;
	m_anormal = FALSE;
	m_aerror = FALSE;
	m_aabort = FALSE;
	m_acancel = FALSE;
	//}}AFX_DATA_INIT
}

void Cdefass::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Cdefass)
	DDX_Text(pDX, IDC_VALUE, m_assvalue);
	DDX_Radio(pDX, IDC_ASET, m_asstype);
	DDX_Radio(pDX, IDC_ANOTCRIT, m_asscrit);
	DDX_Check(pDX, IDC_ASTART, m_astart);
	DDX_Check(pDX, IDC_AREVERSE, m_areverse);
	DDX_Check(pDX, IDC_ANORMAL, m_anormal);
	DDX_Check(pDX, IDC_AERROR, m_aerror);
	DDX_Check(pDX, IDC_AABORT, m_aabort);
	DDX_Check(pDX, IDC_ACANCEL, m_acancel);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(Cdefass, CDialog)
	//{{AFX_MSG_MAP(Cdefass)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SCR_VALUE, OnDeltaposScrValue)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Cdefass message handlers

void Cdefass::OnDeltaposScrValue(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int  sizeb, cnt;
	char	intext[255];
	long	existing;
	if  ((sizeb = GetDlgItemText(IDC_VALUE, intext, sizeof(intext))) <= 0)
		goto  ret;
	for  (cnt = 0;  cnt < sizeb;  cnt++)
		if  (!isdigit(intext[cnt]) && (cnt > 0 || intext[cnt] != '-'))	{
			MessageBeep(MB_ICONASTERISK);
			goto  ret;
		}
	existing = atol(intext) + pNMUpDown->iDelta;
	wsprintf(intext, "%ld", existing);
	SetDlgItemText(IDC_VALUE, intext);
ret:
	*pResult = 0;
}

const DWORD a103HelpIDs[]=
{
	IDC_AEXIT,	IDH_103_156,	// Assignment defaults: "Exit" (Button)
	IDC_ASIGNAL,	IDH_103_156,	// Assignment defaults: "Sig" (Button)
	IDC_ASTART,	IDH_103_164,	// Assignment defaults: "On start" (Button)
	IDC_AREVERSE,	IDH_103_164,	// Assignment defaults: "REVERSE on exit" (Button)
	IDC_ANORMAL,	IDH_103_164,	// Assignment defaults: "Normal exit" (Button)
	IDC_AERROR,	IDH_103_164,	// Assignment defaults: "Error exit" (Button)
	IDC_AABORT,	IDH_103_164,	// Assignment defaults: "Abort" (Button)
	IDC_ACANCEL,	IDH_103_164,	// Assignment defaults: "Cancel" (Button)
	IDC_ANOTCRIT,	IDH_103_170,	// Assignment defaults: "Ignore" (Button)
	IDC_ACRIT,	IDH_103_170,	// Assignment defaults: "Hold job" (Button)
	IDC_VALUE,	IDH_103_154,	// Assignment defaults: "" (Edit)
	IDC_SCR_VALUE,	IDH_103_154,	// Assignment defaults: "Spin1" (msctls_updown32)
	IDC_ASET,	IDH_103_156,	// Assignment defaults: "Set" (Button)
	IDC_AADD,	IDH_103_156,	// Assignment defaults: "+" (Button)
	IDC_ASUB,	IDH_103_156,	// Assignment defaults: "-" (Button)
	IDC_AMULT,	IDH_103_156,	// Assignment defaults: "X" (Button)
	IDC_ADIV,	IDH_103_156,	// Assignment defaults: "/" (Button)
	IDC_AMOD,	IDH_103_156,	// Assignment defaults: "Md" (Button)
	0, 0
};

BOOL Cdefass::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a103HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
