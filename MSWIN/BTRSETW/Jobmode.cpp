// jobmode.cpp : implementation file
//

#include "stdafx.h"
#include "btrsetw.h"
#include "jobmode.h"
#include "ulist.h"
#include "Btrsetw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJobmode dialog

CJobmode::CJobmode(CWnd* pParent /*=NULL*/)
	: CDialog(CJobmode::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJobmode)
	m_user = "";
	m_group = "";
	//}}AFX_DATA_INIT
}

void CJobmode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJobmode)
	DDX_CBString(pDX, IDC_USER, m_user);
	DDV_MaxChars(pDX, m_user, 11);
	DDX_CBString(pDX, IDC_GROUP, m_group);
	DDV_MaxChars(pDX, m_group, 11);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CJobmode, CDialog)
	//{{AFX_MSG_MAP(CJobmode)
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
// CJobmode message handlers

BOOL CJobmode::OnInitDialog()
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

void CJobmode::OnOK()
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

void CJobmode::OnClickedJuread()
{
	if  (((CButton *)GetDlgItem(IDC_JUREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_JUREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_JUWRITE))->SetCheck(0);
}

void CJobmode::OnClickedJuwrite()
{
	if  (((CButton *)GetDlgItem(IDC_JUWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JUREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_JUREAD))->SetCheck(1);
	}
}

void CJobmode::OnClickedJureveal()
{
	if  (!((CButton *)GetDlgItem(IDC_JUREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JUREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_JUWRITE))->SetCheck(0);
	}
}

void CJobmode::OnClickedJudispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_JUDISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JUSETMODE))->SetCheck(0);
}

void CJobmode::OnClickedJusetmode()
{
	if  (((CButton *)GetDlgItem(IDC_JUSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JUDISPMODE))->SetCheck(1);
}

void CJobmode::OnClickedJgread()
{
	if  (((CButton *)GetDlgItem(IDC_JGREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_JGREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_JGWRITE))->SetCheck(0);
}

void CJobmode::OnClickedJgwrite()
{
	if  (((CButton *)GetDlgItem(IDC_JGWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JGREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_JGREAD))->SetCheck(1);
	}
}

void CJobmode::OnClickedJgreveal()
{
	if  (!((CButton *)GetDlgItem(IDC_JGREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JGREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_JGWRITE))->SetCheck(0);
	}
}

void CJobmode::OnClickedJgdispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_JGDISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JGSETMODE))->SetCheck(0);
}

void CJobmode::OnClickedJgsetmode()
{
	if  (((CButton *)GetDlgItem(IDC_JGSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JGDISPMODE))->SetCheck(1);
}

void CJobmode::OnClickedJoread()
{
	if  (((CButton *)GetDlgItem(IDC_JOREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_JOREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_JOWRITE))->SetCheck(0);
}

void CJobmode::OnClickedJowrite()
{
	if  (((CButton *)GetDlgItem(IDC_JOWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JOREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_JOREAD))->SetCheck(1);
	}
}

void CJobmode::OnClickedJoreveal()
{
	if  (!((CButton *)GetDlgItem(IDC_JOREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_JOREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_JOWRITE))->SetCheck(0);
	}
}

void CJobmode::OnClickedJodispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_JODISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JOSETMODE))->SetCheck(0);
}

void CJobmode::OnClickedJosetmode()
{
	if  (((CButton *)GetDlgItem(IDC_JOSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_JODISPMODE))->SetCheck(1);
}

const DWORD a105HelpIDs[]=
{
	IDC_JGKILL,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JOREAD,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JOWRITE,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUREAD,	IDH_105_210,	// Job permissions: "Read" (Button)
	IDC_JOREVEAL,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUWRITE,	IDH_105_210,	// Job permissions: "Write" (Button)
	IDC_JODISPMODE,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUREVEAL,	IDH_105_210,	// Job permissions: "Reveal" (Button)
	IDC_JOSETMODE,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUDISPMODE,	IDH_105_210,	// Job permissions: "Display modes" (Button)
	IDC_JOASSGOWN,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUSETMODE,	IDH_105_210,	// Job permissions: "Set modes" (Button)
	IDC_JOASSOWN,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUASSOWN,	IDH_105_210,	// Job permissions: "Assume ownership" (Button)
	IDC_JOGOWN,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUASSGOWN,	IDH_105_210,	// Job permissions: "Assume group ownership" (Button)
	IDC_JOGGROUP,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUGOWN,	IDH_105_210,	// Job permissions: "Give away ownership" (Button)
	IDC_JODELETE,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUGGROUP,	IDH_105_210,	// Job permissions: "Give away group" (Button)
	IDC_JOKILL,	IDH_105_235,	// Job permissions: "" (Button)
	IDC_JUDELETE,	IDH_105_210,	// Job permissions: "Delete" (Button)
	IDC_USER,	IDH_105_243,	// Job permissions: "" (ComboBox)
	IDC_JUKILL,	IDH_105_210,	// Job permissions: "Kill" (Button)
	IDC_GROUP,	IDH_105_244,	// Job permissions: "" (ComboBox)
	IDC_JGREAD,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGWRITE,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGREVEAL,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGDISPMODE,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGSETMODE,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGASSGOWN,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGASSOWN,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGGOWN,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGGGROUP,	IDH_105_221,	// Job permissions: "" (Button)
	IDC_JGDELETE,	IDH_105_221,	// Job permissions: "" (Button)
	0, 0
};

BOOL CJobmode::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a105HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
