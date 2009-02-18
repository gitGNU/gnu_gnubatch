// editredi.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "editredi.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditredir dialog

CEditredir::CEditredir(CWnd* pParent /*=NULL*/)
	: CDialog(CEditredir::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditredir)
	//}}AFX_DATA_INIT
}

void CEditredir::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditredir)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditredir, CDialog)
	//{{AFX_MSG_MAP(CEditredir)
	ON_BN_CLICKED(IDC_CFILE, OnClickedCfile)
	ON_BN_CLICKED(IDC_PIPETO, OnClickedPipeto)
	ON_BN_CLICKED(IDC_PIPEFROM, OnClickedPipefrom)
	ON_BN_CLICKED(IDC_CLOSEFILE, OnClickedClosefile)
	ON_BN_CLICKED(IDC_DUPFROM, OnClickedDupfrom)
	ON_BN_CLICKED(IDC_FREAD, OnClickedFread)
	ON_BN_CLICKED(IDC_FWRITE, OnClickedFwrite)
	ON_BN_CLICKED(IDC_FAPPEND, OnClickedFappend)
	ON_EN_CHANGE(IDC_FD, OnChangeFd)
	ON_EN_CHANGE(IDC_FD2, OnChangeFd2)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditredir message handlers

BOOL CEditredir::OnInitDialog()
{
	CDialog::OnInitDialog();
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_FD))->SetRange(0, 63);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_FD2))->SetRange(0, 63);
	SetDlgItemInt(IDC_FD, m_redir.fd);
	Enableitems();
	return  TRUE;
}

void CEditredir::OnClickedCfile()
{
	if  (((CButton *)GetDlgItem(IDC_CFILE))->GetCheck())
		m_redir.action = RD_ACT_WRT;
	Enableitems();		
}

void CEditredir::OnClickedPipeto()
{
	if  (((CButton *)GetDlgItem(IDC_PIPETO))->GetCheck())
		m_redir.action = RD_ACT_PIPEO;
	Enableitems();		
}

void CEditredir::OnClickedPipefrom()
{
	if  (((CButton *)GetDlgItem(IDC_PIPEFROM))->GetCheck())
		m_redir.action = RD_ACT_PIPEI;
	Enableitems();		
}

void CEditredir::OnClickedClosefile()
{
	if  (((CButton *)GetDlgItem(IDC_CLOSEFILE))->GetCheck())
		m_redir.action = RD_ACT_CLOSE;
	Enableitems();		
}

void CEditredir::OnClickedDupfrom()
{
	if  (((CButton *)GetDlgItem(IDC_DUPFROM))->GetCheck())  {
		m_redir.action = RD_ACT_DUP;                           
		m_redir.arg = m_redir.fd == 1? 2: 1;
	}	
	Enableitems();		
}

void CEditredir::OnClickedFread()
{
	if  (((CButton *)GetDlgItem(IDC_FREAD))->GetCheck())  {
		if  (m_redir.action == RD_ACT_WRT)
			m_redir.action = RD_ACT_RDWR;
		else  if  (m_redir.action == RD_ACT_APPEND)
			m_redir.action = RD_ACT_RDWRAPP;
	}
	else  {
		if  (m_redir.action == RD_ACT_RDWR)
			m_redir.action = RD_ACT_WRT;
		else  if  (m_redir.action = RD_ACT_RDWRAPP)
			m_redir.action = RD_ACT_APPEND;
	}
	SetDlgItemInt(IDC_FD, m_redir.fd);
	Enableitems();
}

void CEditredir::OnClickedFwrite()
{
	if  (((CButton *)GetDlgItem(IDC_FWRITE))->GetCheck())  {
		if  (m_redir.action == RD_ACT_RD)
			m_redir.action = RD_ACT_RDWR;
	}
	else  {
		if  (m_redir.action == RD_ACT_RDWR)
			m_redir.action = RD_ACT_RD;
	}
	Enableitems();
}

void CEditredir::OnClickedFappend()
{
	if  (((CButton *)GetDlgItem(IDC_FAPPEND))->GetCheck())  {
		if  (m_redir.action == RD_ACT_WRT)
			m_redir.action = RD_ACT_APPEND;
		else  if  (m_redir.action == RD_ACT_RDWR)
			m_redir.action = RD_ACT_RDWRAPP;
	}
	else  {
		if  (m_redir.action == RD_ACT_APPEND)
			m_redir.action = RD_ACT_WRT;
		else  if  (m_redir.action = RD_ACT_RDWRAPP)
			m_redir.action = RD_ACT_RDWR;
	}
	Enableitems();
}

void CEditredir::Enableitems()
{
	switch  (m_redir.action)  {
	case  RD_ACT_RD:
		((CButton *)GetDlgItem(IDC_CFILE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(0);
		GetDlgItem(IDC_FREAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(TRUE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(FALSE);
		Enablefile(TRUE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_WRT:
		((CButton *)GetDlgItem(IDC_CFILE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(0);
		GetDlgItem(IDC_FREAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(TRUE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(TRUE);
		Enablefile(TRUE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_APPEND:
		((CButton *)GetDlgItem(IDC_CFILE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(1);
		GetDlgItem(IDC_FREAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(TRUE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(TRUE);
		Enablefile(TRUE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_RDWR:
		((CButton *)GetDlgItem(IDC_CFILE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(0);
		GetDlgItem(IDC_FREAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(TRUE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(TRUE);
		Enablefile(TRUE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_RDWRAPP:
		((CButton *)GetDlgItem(IDC_CFILE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(1);
		GetDlgItem(IDC_FREAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(TRUE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(TRUE);
		Enablefile(TRUE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_PIPEO:
	case  RD_ACT_PIPEI:
		((CButton *)GetDlgItem(m_redir.action == RD_ACT_PIPEI? IDC_PIPEFROM: IDC_PIPETO))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(0);
		GetDlgItem(IDC_FREAD)->EnableWindow(FALSE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(FALSE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(FALSE);
		Enablefile(TRUE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_CLOSE:
		((CButton *)GetDlgItem(IDC_CLOSEFILE))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(0);
		GetDlgItem(IDC_FREAD)->EnableWindow(FALSE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(FALSE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(FALSE);
		Enablefile(FALSE);
		Enablefd2(FALSE);
		return;
	case  RD_ACT_DUP:
		((CButton *)GetDlgItem(IDC_DUPFROM))->SetCheck(1);
		((CButton *)GetDlgItem(IDC_FREAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FWRITE))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_FAPPEND))->SetCheck(0);
		GetDlgItem(IDC_FREAD)->EnableWindow(FALSE);
		GetDlgItem(IDC_FWRITE)->EnableWindow(FALSE);
		GetDlgItem(IDC_FAPPEND)->EnableWindow(FALSE);
		Enablefile(FALSE);
		Enablefd2(TRUE);
		return;
	}
}

void	CEditredir::Enablefile(const BOOL on)
{
	if  (on)  {
		GetDlgItem(IDC_FILENAME)->EnableWindow(TRUE);
		SetDlgItemText(IDC_FILENAME, m_redir.buffer);
	}
	else  {
		SetDlgItemText(IDC_FILENAME, "");
		GetDlgItem(IDC_FILENAME)->EnableWindow(FALSE);
	}
}

void	CEditredir::Enablefd2(const BOOL on)
{
	if  (on)  {
		GetDlgItem(IDC_FD2)->EnableWindow(TRUE);
		GetDlgItem(IDC_SCR_FD2)->EnableWindow(TRUE);
		GetDlgItem(IDC_BRAC)->EnableWindow(TRUE);
		GetDlgItem(IDC_STDFILE2)->EnableWindow(TRUE);
		GetDlgItem(IDC_KET)->EnableWindow(TRUE);
		SetDlgItemInt(IDC_FD2, m_redir.arg);
	}	
	else  {
		SetDlgItemText(IDC_FD2, "");
		GetDlgItem(IDC_FD2)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_FD2)->EnableWindow(FALSE);
		GetDlgItem(IDC_BRAC)->EnableWindow(FALSE);
		GetDlgItem(IDC_STDFILE2)->EnableWindow(FALSE);
		GetDlgItem(IDC_KET)->EnableWindow(FALSE);
	}
}		
				
void CEditredir::OnOK()
{
	m_redir.fd = (unsigned char) GetDlgItemInt(IDC_FD);

	switch  (m_redir.action)  {
	case  RD_ACT_RD:
	case  RD_ACT_WRT:
	case  RD_ACT_APPEND:
	case  RD_ACT_RDWR:
	case  RD_ACT_RDWRAPP:
	case  RD_ACT_PIPEO:
	case  RD_ACT_PIPEI:
		{
			char	buff[256];
			if  (GetDlgItemText(IDC_FILENAME, buff, sizeof(buff)) <= 0)  {
				AfxMessageBox(IDP_NOFILENAME, MB_OK|MB_ICONEXCLAMATION);
				CEdit	*ew = (CEdit *) GetDlgItem(IDC_FILENAME);
				ew->SetSel(0, -1);
				ew->SetFocus();
				return;
			}
			m_redir.buffer = buff;
		}
		break;
	case  RD_ACT_CLOSE:
		break;
	case  RD_ACT_DUP:
		m_redir.arg = GetDlgItemInt(IDC_FD2);
		break;
	}

	CDialog::OnOK();
}

void CEditredir::OnChangeFd()
{
	UINT  fd = GetDlgItemInt(IDC_FD);
	if  (fd < 3)  {
		CString  msg;
		msg.LoadString(IDS_STDIN_NAME + fd);
		SetDlgItemText(IDC_STDFILE, msg);
	}
	else
		SetDlgItemText(IDC_STDFILE, "");	
}

void CEditredir::OnChangeFd2()
{
	UINT  fd = GetDlgItemInt(IDC_FD2);
	if  (fd < 3)  {
		CString  msg;
		msg.LoadString(IDS_STDIN_NAME + fd);
		SetDlgItemText(IDC_STDFILE2, msg);
	}
	else
		SetDlgItemText(IDC_STDFILE2, "");	
}

const DWORD a109HelpIDs[]=
{
	IDC_FD,	IDH_109_185,	// Edit Job Redirection: "0" (Edit)
	IDC_SCR_FD,	IDH_109_185,	// Edit Job Redirection: "Spin1" (msctls_updown32)
	IDC_STDFILE,	IDH_109_187,	// Edit Job Redirection: "" (Edit)
	IDC_CFILE,	IDH_109_188,	// Edit Job Redirection: "File" (Button)
	IDC_PIPETO,	IDH_109_189,	// Edit Job Redirection: "Pipe out to" (Button)
	IDC_PIPEFROM,	IDH_109_190,	// Edit Job Redirection: "Pipe in from" (Button)
	IDC_CLOSEFILE,	IDH_109_191,	// Edit Job Redirection: "Close" (Button)
	IDC_DUPFROM,	IDH_109_192,	// Edit Job Redirection: "Dup f.d." (Button)
	IDC_FD2,	IDH_109_193,	// Edit Job Redirection: "0" (Edit)
	IDC_SCR_FD2,	IDH_109_193,	// Edit Job Redirection: "Spin2" (msctls_updown32)
	IDC_STDFILE2,	IDH_109_195,	// Edit Job Redirection: "" (Edit)
	IDC_FILENAME,	IDH_109_196,	// Edit Job Redirection: "" (Edit)
	IDC_FREAD,	IDH_109_199,	// Edit Job Redirection: "Read" (Button)
	IDC_FWRITE,	IDH_109_200,	// Edit Job Redirection: "Write" (Button)
	IDC_FAPPEND,	IDH_109_201,	// Edit Job Redirection: "Append" (Button)
	0, 0
};

BOOL CEditredir::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a109HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
