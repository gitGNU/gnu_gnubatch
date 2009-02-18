// defass.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "defass.h"
#include <ctype.h>
#include "Btrsetw.hpp"

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
	m_astart = FALSE;
	m_areverse = FALSE;
	m_anormal = FALSE;
	m_aerror = FALSE;
	m_aabort = FALSE;
	m_acancel = FALSE;
	m_asscrit = -1;
	m_asstype = -1;
	m_assval = "";
	//}}AFX_DATA_INIT
}

void Cdefass::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Cdefass)
	DDX_Check(pDX, IDC_ASTART, m_astart);
	DDX_Check(pDX, IDC_AREVERSE, m_areverse);
	DDX_Check(pDX, IDC_ANORMAL, m_anormal);
	DDX_Check(pDX, IDC_AERROR, m_aerror);
	DDX_Check(pDX, IDC_AABORT, m_aabort);
	DDX_Check(pDX, IDC_ACANCEL, m_acancel);
	DDX_Radio(pDX, IDC_ANOTCRIT, m_asscrit);
	DDX_Radio(pDX, IDC_ASET, m_asstype);
	DDX_Text(pDX, IDC_VALUE, m_assval);
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
	existing = atol(intext);
	if  (pNMUpDown->iDelta >= 0)
		existing++;
	else
		existing--;
	wsprintf(intext, "%ld", existing);
	SetDlgItemText(IDC_VALUE, intext);
ret:
	*pResult = 0;
}

const DWORD a101HelpIDs[]=
{
	IDC_ANORMAL,	IDH_101_162,	// Assignment defaults: "Normal exit" (Button)
	IDC_AERROR,	IDH_101_163,	// Assignment defaults: "Error exit" (Button)
	IDC_AABORT,	IDH_101_164,	// Assignment defaults: "Abort" (Button)
	IDC_ACANCEL,	IDH_101_165,	// Assignment defaults: "Cancel" (Button)
	IDC_ANOTCRIT,	IDH_101_166,	// Assignment defaults: "Ignore" (Button)
	IDC_ACRIT,	IDH_101_167,	// Assignment defaults: "Hold job" (Button)
	IDC_VALUE,	IDH_101_150,	// Assignment defaults: "" (Edit)
	IDC_SCR_VALUE,	IDH_101_150,	// Assignment defaults: "Spin1" (msctls_updown32)
	IDC_ASET,	IDH_101_152,	// Assignment defaults: "Set" (Button)
	IDC_AADD,	IDH_101_153,	// Assignment defaults: "+" (Button)
	IDC_ASUB,	IDH_101_154,	// Assignment defaults: "-" (Button)
	IDC_AMULT,	IDH_101_155,	// Assignment defaults: "X" (Button)
	IDC_ADIV,	IDH_101_156,	// Assignment defaults: "/" (Button)
	IDC_AMOD,	IDH_101_157,	// Assignment defaults: "Md" (Button)
	IDC_AEXIT,	IDH_101_158,	// Assignment defaults: "Exit" (Button)
	IDC_ASIGNAL,	IDH_101_159,	// Assignment defaults: "Sig" (Button)
	IDC_ASTART,	IDH_101_160,	// Assignment defaults: "On start" (Button)
	IDC_AREVERSE,	IDH_101_161,	// Assignment defaults: "REVERSE on exit" (Button)
	0, 0
};

BOOL Cdefass::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a101HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
