// jperm.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "ulist.h"
#include "jperm.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJperm dialog

CJperm::CJperm(CWnd* pParent /*=NULL*/)
	: CDialog(CJperm::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJperm)
	m_user = "";
	m_group = "";
	//}}AFX_DATA_INIT
}

void CJperm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJperm)
	DDX_CBString(pDX, IDC_USER, m_user);
	DDV_MaxChars(pDX, m_user, 11);
	DDX_CBString(pDX, IDC_GROUP, m_group);
	DDV_MaxChars(pDX, m_group, 11);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJperm, CDialog)
	//{{AFX_MSG_MAP(CJperm)
	ON_BN_CLICKED(IDC_JUREAD, OnClickedJuread)
	ON_BN_CLICKED(IDC_JUWRITE, OnClickedJuwrite)
	ON_BN_CLICKED(IDC_JUREVEAL, OnClickedJureveal)
	ON_BN_CLICKED(IDC_JUDISPMODE, OnClickedJudispmode)
	ON_BN_CLICKED(IDC_JUSETMODE, OnClickedJusetmode)
	ON_BN_CLICKED(IDC_JGREAD, OnClickedJgread)
	ON_BN_CLICKED(IDC_JGWRITE, OnClickedJgwrite)
	ON_BN_CLICKED(IDC_JGREVEAL, OnClickedJgreveal)
	ON_BN_CLICKED(IDC_JGDISPMODE, OnClickedJgdispmode)
	ON_BN_CLICKED(IDC_JGSETMODE, OnClickedJgsetmode)
	ON_BN_CLICKED(IDC_JOREAD, OnClickedJoread)
	ON_BN_CLICKED(IDC_JOWRITE, OnClickedJowrite)
	ON_BN_CLICKED(IDC_JOREVEAL, OnClickedJoreveal)
	ON_BN_CLICKED(IDC_JODISPMODE, OnClickedJodispmode)
	ON_BN_CLICKED(IDC_JOSETMODE, OnClickedJosetmode)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJperm message handlers

BOOL CJperm::OnInitDialog()
{
	CDialog::OnInitDialog();
	CComboBox  *uP = (CComboBox *)GetDlgItem(IDC_USER);
	UUserList  *ul = new UUserList;
	const  char  FAR  *n;
	while  (n = ul->next())
		uP->AddString(n);
	uP->SelectString(-1, m_user);
	delete  ul;
	uP = (CComboBox *)GetDlgItem(IDC_GROUP);
	UGroupList *ug = new UGroupList;
	while  (n = ug->next())
		uP->AddString(n);
	uP->SelectString(-1, m_group);
	delete  ug;
	for  (unsigned  cnt = 0; cnt <= IDC_JUKILL-IDC_JUREAD; cnt++)
		((CButton *)GetDlgItem(IDC_JUREAD+cnt))->SetCheck(m_umode & (1 << cnt)? 1: 0);
	for  (cnt = 0; cnt <= IDC_JGKILL-IDC_JGREAD; cnt++)
		((CButton *)GetDlgItem(IDC_JGREAD+cnt))->SetCheck(m_gmode & (1 << cnt)? 1: 0);
	for  (cnt = 0; cnt <= IDC_JOKILL-IDC_JOREAD; cnt++)
		((CButton *)GetDlgItem(IDC_JOREAD+cnt))->SetCheck(m_omode & (1 << cnt)? 1: 0);
	if  (!m_writeable)  {
		for  (cnt = IDC_JUREAD;  cnt <= IDC_JUKILL;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		for  (cnt = IDC_JGREAD;  cnt <= IDC_JGKILL;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		for  (cnt = IDC_JOREAD;  cnt <= IDC_JOKILL;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
	}		
	return TRUE;
}

void CJperm::OnOK()
{
	if  (m_writeable)  {
		m_umode = m_gmode = m_omode = 0;
		for  (unsigned  cnt = 0; cnt <= IDC_JUKILL-IDC_JUREAD; cnt++)
			if  (((CButton *)GetDlgItem(IDC_JUREAD+cnt))->GetCheck())
				m_umode |= 1 << cnt;
		for  (cnt = 0; cnt <= IDC_JGKILL-IDC_JGREAD; cnt++)
			if  (((CButton *)GetDlgItem(IDC_JGREAD+cnt))->GetCheck())
				m_gmode |= 1 << cnt;
		for  (cnt = 0; cnt <= IDC_JOKILL-IDC_JOREAD; cnt++)
			if  (((CButton *)GetDlgItem(IDC_JOREAD+cnt))->GetCheck())
				m_omode |= 1 << cnt;	
		if  (!checkminmode(m_umode, m_gmode, m_omode))  {
			AfxMessageBox(IDP_JLTMINMODE, MB_OK|MB_ICONEXCLAMATION);
			GetDlgItem(IDC_JUDELETE)->SetFocus();
			return;
		}	
	}	
	CDialog::OnOK();
}

void CJperm::OnClickedJuread()
{
	if  (((CButton *)GetDlgItem(IDC_JUREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_JUREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_JUWRITE))->SetCheck(0);
}

void CJperm::OnClickedJuwrite()
{
	if  (((CButton *)GetDlgItem(IDC_JUWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JUREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_JUREAD))->SetCheck(1);
	}
}

void CJperm::OnClickedJureveal()
{
	if  (!((CButton *)GetDlgItem(IDC_JUREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JUREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_JUWRITE))->SetCheck(0);
	}
}

void CJperm::OnClickedJudispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_JUDISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JUSETMODE))->SetCheck(0);
}

void CJperm::OnClickedJusetmode()
{
	if  (((CButton *)GetDlgItem(IDC_JUSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JUDISPMODE))->SetCheck(1);
}

void CJperm::OnClickedJgread()
{
	if  (((CButton *)GetDlgItem(IDC_JGREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_JGREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_JGWRITE))->SetCheck(0);
}

void CJperm::OnClickedJgwrite()
{
	if  (((CButton *)GetDlgItem(IDC_JGWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JGREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_JGREAD))->SetCheck(1);
	}
}

void CJperm::OnClickedJgreveal()
{
	if  (!((CButton *)GetDlgItem(IDC_JGREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JGREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_JGWRITE))->SetCheck(0);
	}
}

void CJperm::OnClickedJgdispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_JGDISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JGSETMODE))->SetCheck(0);
}

void CJperm::OnClickedJgsetmode()
{
	if  (((CButton *)GetDlgItem(IDC_JGSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JGDISPMODE))->SetCheck(1);
}

void CJperm::OnClickedJoread()
{
	if  (((CButton *)GetDlgItem(IDC_JOREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_JOREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_JOWRITE))->SetCheck(0);
}

void CJperm::OnClickedJowrite()
{
	if  (((CButton *)GetDlgItem(IDC_JOWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JOREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_JOREAD))->SetCheck(1);
	}
}

void CJperm::OnClickedJoreveal()
{
	if  (!((CButton *)GetDlgItem(IDC_JOREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JOREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_JOWRITE))->SetCheck(0);
	}
}

void CJperm::OnClickedJodispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_JODISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JOSETMODE))->SetCheck(0);
}

void CJperm::OnClickedJosetmode()
{
	if  (((CButton *)GetDlgItem(IDC_JOSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JODISPMODE))->SetCheck(1);
}

const DWORD a120HelpIDs[]=
{
	IDC_JOWRITE,	IDH_120_254,	// Job permissions / user / group: "" (Button)
	IDC_JUREAD,	IDH_120_231,	// Job permissions / user / group: "Read" (Button)
	IDC_JOREVEAL,	IDH_120_255,	// Job permissions / user / group: "" (Button)
	IDC_JUWRITE,	IDH_120_232,	// Job permissions / user / group: "Write" (Button)
	IDC_JODISPMODE,	IDH_120_256,	// Job permissions / user / group: "" (Button)
	IDC_JUREVEAL,	IDH_120_233,	// Job permissions / user / group: "Reveal" (Button)
	IDC_JOSETMODE,	IDH_120_257,	// Job permissions / user / group: "" (Button)
	IDC_JUDISPMODE,	IDH_120_234,	// Job permissions / user / group: "Display modes" (Button)
	IDC_JOASSGOWN,	IDH_120_258,	// Job permissions / user / group: "" (Button)
	IDC_JUSETMODE,	IDH_120_235,	// Job permissions / user / group: "Set modes" (Button)
	IDC_JOASSOWN,	IDH_120_259,	// Job permissions / user / group: "" (Button)
	IDC_JUASSOWN,	IDH_120_236,	// Job permissions / user / group: "Assume ownership" (Button)
	IDC_JOGOWN,	IDH_120_260,	// Job permissions / user / group: "" (Button)
	IDC_JUASSGOWN,	IDH_120_237,	// Job permissions / user / group: "Assume group ownership" (Button)
	IDC_JOGGROUP,	IDH_120_261,	// Job permissions / user / group: "" (Button)
	IDC_JUGOWN,	IDH_120_238,	// Job permissions / user / group: "Give away ownership" (Button)
	IDC_JODELETE,	IDH_120_262,	// Job permissions / user / group: "" (Button)
	IDC_JUGGROUP,	IDH_120_239,	// Job permissions / user / group: "Give away group" (Button)
	IDC_JOKILL,	IDH_120_263,	// Job permissions / user / group: "" (Button)
	IDC_JUDELETE,	IDH_120_240,	// Job permissions / user / group: "Delete" (Button)
	IDC_USER,	IDH_120_264,	// Job permissions / user / group: "" (ComboBox)
	IDC_JUKILL,	IDH_120_241,	// Job permissions / user / group: "Kill" (Button)
	IDC_GROUP,	IDH_120_265,	// Job permissions / user / group: "" (ComboBox)
	IDC_JGREAD,	IDH_120_242,	// Job permissions / user / group: "" (Button)
	IDC_JGWRITE,	IDH_120_243,	// Job permissions / user / group: "" (Button)
	IDC_JGREVEAL,	IDH_120_244,	// Job permissions / user / group: "" (Button)
	IDC_JGDISPMODE,	IDH_120_245,	// Job permissions / user / group: "" (Button)
	IDC_JGSETMODE,	IDH_120_246,	// Job permissions / user / group: "" (Button)
	IDC_JGASSGOWN,	IDH_120_247,	// Job permissions / user / group: "" (Button)
	IDC_JGASSOWN,	IDH_120_248,	// Job permissions / user / group: "" (Button)
	IDC_JGGOWN,	IDH_120_249,	// Job permissions / user / group: "" (Button)
	IDC_JGGGROUP,	IDH_120_250,	// Job permissions / user / group: "" (Button)
	IDC_JGDELETE,	IDH_120_251,	// Job permissions / user / group: "" (Button)
	IDC_JGKILL,	IDH_120_252,	// Job permissions / user / group: "" (Button)
	IDC_JOREAD,	IDH_120_253,	// Job permissions / user / group: "" (Button)
	0, 0
};

BOOL CJperm::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a120HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
