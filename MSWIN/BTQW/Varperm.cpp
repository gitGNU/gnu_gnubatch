// varperm.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "ulist.h"
#include "varperm.h"
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVarperm dialog

CVarperm::CVarperm(CWnd* pParent /*=NULL*/)
	: CDialog(CVarperm::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVarperm)
	m_user = "";
	m_group = "";
	//}}AFX_DATA_INIT
}

void CVarperm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVarperm)
	DDX_CBString(pDX, IDC_USER, m_user);
	DDV_MaxChars(pDX, m_user, 11);
	DDX_CBString(pDX, IDC_GROUP, m_group);
	DDV_MaxChars(pDX, m_group, 11);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVarperm, CDialog)
	//{{AFX_MSG_MAP(CVarperm)
	ON_BN_CLICKED(IDC_VUREAD, OnClickedVuread)
	ON_BN_CLICKED(IDC_VUWRITE, OnClickedVuwrite)
	ON_BN_CLICKED(IDC_VUREVEAL, OnClickedVureveal)
	ON_BN_CLICKED(IDC_VUDISPMODE, OnClickedVudispmode)
	ON_BN_CLICKED(IDC_VUSETMODE, OnClickedVusetmode)
	ON_BN_CLICKED(IDC_VGREAD, OnClickedVgread)
	ON_BN_CLICKED(IDC_VGWRITE, OnClickedVgwrite)
	ON_BN_CLICKED(IDC_VGREVEAL, OnClickedVgreveal)
	ON_BN_CLICKED(IDC_VGDISPMODE, OnClickedVgdispmode)
	ON_BN_CLICKED(IDC_VGSETMODE, OnClickedVgsetmode)
	ON_BN_CLICKED(IDC_VOREAD, OnClickedVoread)
	ON_BN_CLICKED(IDC_VOREVEAL, OnClickedVoreveal)
	ON_BN_CLICKED(IDC_VODISPMODE, OnClickedVodispmode)
	ON_BN_CLICKED(IDC_VOSETMODE, OnClickedVosetmode)
	ON_BN_CLICKED(IDC_VOWRITE, OnClickedVowrite)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVarperm message handlers

BOOL CVarperm::OnInitDialog()
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
	for  (unsigned  cnt = 0; cnt <= IDC_VUDELETE-IDC_VUREAD; cnt++)
		((CButton *)GetDlgItem(IDC_VUREAD+cnt))->SetCheck(m_umode & (1 << cnt)? 1: 0);
	for  (cnt = 0; cnt <= IDC_VGDELETE-IDC_VGREAD; cnt++)
		((CButton *)GetDlgItem(IDC_VGREAD+cnt))->SetCheck(m_gmode & (1 << cnt)? 1: 0);
	for  (cnt = 0; cnt <= IDC_VODELETE-IDC_VOREAD; cnt++)
		((CButton *)GetDlgItem(IDC_VOREAD+cnt))->SetCheck(m_omode & (1 << cnt)? 1: 0);
	if  (!m_writeable)  {
		for  (cnt = IDC_VUREAD;  cnt <= IDC_VUDELETE;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		for  (cnt = IDC_VGREAD;  cnt <= IDC_VGDELETE;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
		for  (cnt = IDC_VOREAD;  cnt <= IDC_VODELETE;  cnt++)
			GetDlgItem(cnt)->EnableWindow(FALSE);
	}		
	return TRUE;
}

void CVarperm::OnOK()
{
	if  (m_writeable)  {
		m_umode = m_gmode = m_omode = 0;
		for  (unsigned  cnt = 0; cnt <= IDC_VUDELETE-IDC_VUREAD; cnt++)
			if  (((CButton *)GetDlgItem(IDC_VUREAD+cnt))->GetCheck())
				m_umode |= 1 << cnt;
		for  (cnt = 0; cnt <= IDC_VGDELETE-IDC_VGREAD; cnt++)
			if  (((CButton *)GetDlgItem(IDC_VGREAD+cnt))->GetCheck())
				m_gmode |= 1 << cnt;
		for  (cnt = 0; cnt <= IDC_VODELETE-IDC_VOREAD; cnt++)
			if  (((CButton *)GetDlgItem(IDC_VOREAD+cnt))->GetCheck())
				m_omode |= 1 << cnt;
		if  (!checkminmode(m_umode, m_gmode, m_omode))  {
			AfxMessageBox(IDP_VLTMINMODE, MB_OK|MB_ICONEXCLAMATION);
			GetDlgItem(IDC_VUDELETE)->SetFocus();
			return;
		}	
	}	
	CDialog::OnOK();
}

void CVarperm::OnClickedVuread()
{
	if  (((CButton *)GetDlgItem(IDC_VUREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_VUREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_VUWRITE))->SetCheck(0);
}

void CVarperm::OnClickedVuwrite()
{
	if  (((CButton *)GetDlgItem(IDC_VUWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_VUREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_VUREAD))->SetCheck(1);
	}
}

void CVarperm::OnClickedVureveal()
{
	if  (!((CButton *)GetDlgItem(IDC_VUREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_VUREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_VUWRITE))->SetCheck(0);
	}
}

void CVarperm::OnClickedVudispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_VUDISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_VUSETMODE))->SetCheck(0);
}

void CVarperm::OnClickedVusetmode()
{
	if  (((CButton *)GetDlgItem(IDC_VUSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_VUDISPMODE))->SetCheck(1);
}

void CVarperm::OnClickedVgread()
{
	if  (((CButton *)GetDlgItem(IDC_VGREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_VGREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_VGWRITE))->SetCheck(0);
}

void CVarperm::OnClickedVgwrite()
{
	if  (((CButton *)GetDlgItem(IDC_VGWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_VGREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_VGREAD))->SetCheck(1);
	}
}

void CVarperm::OnClickedVgreveal()
{
	if  (!((CButton *)GetDlgItem(IDC_VGREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_VGREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_VGWRITE))->SetCheck(0);
	}
}

void CVarperm::OnClickedVgdispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_VGDISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_VGSETMODE))->SetCheck(0);
}

void CVarperm::OnClickedVgsetmode()
{
	if  (((CButton *)GetDlgItem(IDC_VGSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_VGDISPMODE))->SetCheck(1);
}

void CVarperm::OnClickedVoread()
{
	if  (((CButton *)GetDlgItem(IDC_VOREAD))->GetCheck())
		((CButton *)GetDlgItem(IDC_VOREVEAL))->SetCheck(1);
	else				
		((CButton *)GetDlgItem(IDC_VOWRITE))->SetCheck(0);
}

void CVarperm::OnClickedVowrite()
{
	if  (((CButton *)GetDlgItem(IDC_VOWRITE))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_VOREVEAL))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_VOREAD))->SetCheck(1);
	}
}

void CVarperm::OnClickedVoreveal()
{
	if  (!((CButton *)GetDlgItem(IDC_VOREVEAL))->GetCheck())  {
		((CButton *)GetDlgItem(IDC_VOREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_VOWRITE))->SetCheck(0);
	}
}

void CVarperm::OnClickedVodispmode()
{
	if  (!((CButton *)GetDlgItem(IDC_VODISPMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_VOSETMODE))->SetCheck(0);
}

void CVarperm::OnClickedVosetmode()
{
	if  (((CButton *)GetDlgItem(IDC_VOSETMODE))->GetCheck())
		((CButton *)GetDlgItem(IDC_VODISPMODE))->SetCheck(1);
}

const DWORD a136HelpIDs[]=
{
	IDC_VOREAD,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VOWRITE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VOREVEAL,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VODISPMODE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUREAD,	IDH_136_418,	// Variable permissions / user /group: "Read" (Button)
	IDC_VOSETMODE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUWRITE,	IDH_136_418,	// Variable permissions / user /group: "Write" (Button)
	IDC_VOASSOWN,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUREVEAL,	IDH_136_418,	// Variable permissions / user /group: "Reveal" (Button)
	IDC_VOASSGOWN,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUDISPMODE,	IDH_136_418,	// Variable permissions / user /group: "Display modes" (Button)
	IDC_VOGOWN,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUSETMODE,	IDH_136_418,	// Variable permissions / user /group: "Set modes" (Button)
	IDC_VOGGROUP,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUASSOWN,	IDH_136_418,	// Variable permissions / user /group: "Assume ownership" (Button)
	IDC_VODELETE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VUASSGOWN,	IDH_136_418,	// Variable permissions / user /group: "Assume group ownership" (Button)
	IDC_VUGOWN,	IDH_136_418,	// Variable permissions / user /group: "Give away ownership" (Button)
	IDC_USER,	IDH_136_264,	// Variable permissions / user /group: "" (ComboBox)
	IDC_VUGGROUP,	IDH_136_418,	// Variable permissions / user /group: "Give away group" (Button)
	IDC_GROUP,	IDH_136_265,	// Variable permissions / user /group: "" (ComboBox)
	IDC_VUDELETE,	IDH_136_418,	// Variable permissions / user /group: "Delete" (Button)
	IDC_VGREAD,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGWRITE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGREVEAL,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGDISPMODE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGSETMODE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGASSOWN,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGASSGOWN,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGGOWN,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGGGROUP,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	IDC_VGDELETE,	IDH_136_418,	// Variable permissions / user /group: "" (Button)
	0, 0
};

BOOL CVarperm::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a136HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
