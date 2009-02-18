// procpar.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "mainfrm.h"
#include "btrw.h"
#include "procpar.h"
#include "Btrw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProcpar dialog

CProcpar::CProcpar(CWnd* pParent /*=NULL*/)
	: CDialog(CProcpar::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProcpar)
	m_directory = "";
	m_remrunnable = FALSE;
	m_advterr = -1;
	//}}AFX_DATA_INIT
}

void CProcpar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcpar)
	DDX_Text(pDX, IDC_DIRECTORY, m_directory);
	DDX_Check(pDX, IDC_REMRUNNABLE, m_remrunnable);
	DDX_Radio(pDX, IDC_ADVTERR, m_advterr);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProcpar, CDialog)
	//{{AFX_MSG_MAP(CProcpar)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcpar message handlers

BOOL CProcpar::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	SetDlgItemInt(IDC_NEXITL, m_exits.nlower);
	SetDlgItemInt(IDC_NEXITU, m_exits.nupper);
	SetDlgItemInt(IDC_EEXITL, m_exits.elower);
	SetDlgItemInt(IDC_EEXITU, m_exits.eupper);

	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_NEXITL))->SetRange(0, 255);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_NEXITU))->SetRange(0, 255);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_EEXITL))->SetRange(0, 255);
	((CSpinButtonCtrl *) GetDlgItem(IDC_SCR_EEXITU))->SetRange(0, 255);
		
	char	obuf[10];
	wsprintf(obuf, "0x%.6lx", m_ulimit);
	SetDlgItemText(IDC_ULIMIT, obuf);

	for  (unsigned  cnt = 0;  cnt < 9;  cnt++)
		((CButton *)GetDlgItem(IDC_OX - cnt))->SetCheck((m_umask & (1 << cnt))? 1: 0); 
	if  (!m_writeable)  {
		GetDlgItem(IDC_DIRECTORY)->EnableWindow(FALSE);
		GetDlgItem(IDC_REMRUNNABLE)->EnableWindow(FALSE);
		GetDlgItem(IDC_ADVTERR)->EnableWindow(FALSE);
		GetDlgItem(IDC_NADVTERR)->EnableWindow(FALSE);
		GetDlgItem(IDC_ULIMIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_ULIMIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_NEXITL)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_NEXITL)->EnableWindow(FALSE);
		GetDlgItem(IDC_NEXITU)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_NEXITU)->EnableWindow(FALSE);
		GetDlgItem(IDC_EEXITL)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_EEXITL)->EnableWindow(FALSE);
		GetDlgItem(IDC_EEXITU)->EnableWindow(FALSE);
		GetDlgItem(IDC_SCR_EEXITU)->EnableWindow(FALSE);
		for  (unsigned  cnt = 0;  cnt < 9;  cnt++)
			GetDlgItem(IDC_UR + cnt)->EnableWindow(FALSE); 
	}	
		
	return TRUE;
}

void CProcpar::OnOK()
{
	if  (m_writeable)  {
		m_exits.nlower = GetDlgItemInt(IDC_NEXITL);
		m_exits.nupper = GetDlgItemInt(IDC_NEXITU);
		m_exits.elower = GetDlgItemInt(IDC_EEXITL);
		m_exits.eupper = GetDlgItemInt(IDC_EEXITU);
		if  (m_exits.nlower > m_exits.nupper)  {
			if  (AfxMessageBox(IDP_NORMBADRANGE, MB_OKCANCEL|MB_ICONEXCLAMATION) == IDCANCEL)  {
				OnCancel();
				return;
			}
			CEdit	*ew = (CEdit *) GetDlgItem(IDC_NEXITU);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}		
		if  (m_exits.elower > m_exits.eupper)  {
			if  (AfxMessageBox(IDP_ERRBADRANGE, MB_OKCANCEL|MB_ICONEXCLAMATION) == IDCANCEL)  {
				OnCancel();
				return;
			}
			CEdit	*ew = (CEdit *) GetDlgItem(IDC_EEXITU);
			ew->SetSel(0, -1);
			ew->SetFocus();
			return;
		}
		m_umask = 0;		
		for  (unsigned  cnt = 0;  cnt < 9;  cnt++)
			if  (((CButton *)GetDlgItem(IDC_OX - cnt))->GetCheck())
				m_umask |= 1 << cnt;
	} 
	CDialog::OnOK();
}

const DWORD a117HelpIDs[]=
{
	IDC_REMRUNNABLE,	IDH_117_277,	// Process parameters: "Remote executable" (Button)
	IDC_DIRECTORY,	IDH_117_255,	// Process parameters: "" (Edit)
	IDC_UR,	IDH_117_256,	// Process parameters: "R" (Button)
	IDC_UW,	IDH_117_257,	// Process parameters: "W" (Button)
	IDC_UX,	IDH_117_258,	// Process parameters: "X" (Button)
	IDC_GR,	IDH_117_259,	// Process parameters: "R" (Button)
	IDC_GW,	IDH_117_260,	// Process parameters: "W" (Button)
	IDC_GX,	IDH_117_261,	// Process parameters: "X" (Button)
	IDC_OR,	IDH_117_262,	// Process parameters: "R" (Button)
	IDC_OW,	IDH_117_263,	// Process parameters: "W" (Button)
	IDC_OX,	IDH_117_264,	// Process parameters: "X" (Button)
	IDC_ULIMIT,	IDH_117_265,	// Process parameters: "" (Edit)
	IDC_SCR_ULIMIT,	IDH_117_265,	// Process parameters: "Spin1" (msctls_updown32)
	IDC_NEXITL,	IDH_117_267,	// Process parameters: "0" (Edit)
	IDC_SCR_NEXITL,	IDH_117_267,	// Process parameters: "Spin2" (msctls_updown32)
	IDC_NEXITU,	IDH_117_267,	// Process parameters: "0" (Edit)
	IDC_SCR_NEXITU,	IDH_117_267,	// Process parameters: "Spin3" (msctls_updown32)
	IDC_EEXITL,	IDH_117_271,	// Process parameters: "0" (Edit)
	IDC_SCR_EEXITL,	IDH_117_271,	// Process parameters: "Spin4" (msctls_updown32)
	IDC_EEXITU,	IDH_117_271,	// Process parameters: "0" (Edit)
	IDC_SCR_EEXITU,	IDH_117_271,	// Process parameters: "Spin5" (msctls_updown32)
	IDC_ADVTERR,	IDH_117_275,	// Process parameters: "Advance time" (Button)
	IDC_NADVTERR,	IDH_117_276,	// Process parameters: "Do not advance time" (Button)
	0, 0
};

BOOL CProcpar::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a117HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
