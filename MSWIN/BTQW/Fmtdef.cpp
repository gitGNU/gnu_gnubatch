// fmtdef.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "netwmsg.h"
#include "mainfrm.h"
#include "btqw.h"
#include "formatcode.h"
#include "fmtdef.h"
#include "editfmt.h"
#include "editsep.h"
#include <ctype.h>
#include "Btqw.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFmtdef dialog


CFmtdef::CFmtdef(CWnd* pParent /*=NULL*/)
	: CDialog(CFmtdef::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFmtdef)
	//}}AFX_DATA_INIT
	m_changes = 0;
	m_numformats = 0;
}

void CFmtdef::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFmtdef)
	DDX_Control(pDX, IDC_FMTLIST, m_dragformat);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFmtdef, CDialog)
	//{{AFX_MSG_MAP(CFmtdef)
	ON_LBN_DBLCLK(IDC_FMTLIST, OnDblclkFmtlist)
	ON_BN_CLICKED(IDC_NEWFMT, OnNewfmt)
	ON_BN_CLICKED(IDC_EDITFMT, OnEditfmt)
	ON_BN_CLICKED(IDC_DELFMT, OnDelfmt)
	ON_BN_CLICKED(IDC_NEWSEP, OnNewsep)
	ON_BN_CLICKED(IDC_RESETDEFLT, OnResetdeflt)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFmtdef message handlers

static	void	GenFmtString(const Formatrow &copy, CString &result)
{
	if  (copy.f_issep)  {
		result = '\"';
		result += copy.f_field;
		result += '\"';
	}
	else  {
		result = copy.f_ltab? '<': ' ';
		if  (copy.f_skipright)
			result += '>';
		char	ww[20];
		wsprintf(ww, "\t%u\t", copy.f_width);
		result += ww;
		result += copy.f_field;
	}
}		

void CFmtdef::Addformat(const Formatrow &copy)
{
	m_formats[m_numformats] = copy;
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_FMTLIST);
	int	 where = lb->GetCurSel();
	CString	res;
	GenFmtString(copy, res);
	where = lb->InsertString(where, res);
	lb->SetItemData(where, DWORD(m_numformats));
	m_numformats++;
	m_changes++;
}	


BOOL CFmtdef::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString	w4;
	if  (w4.LoadString(m_what4))  {
		CEdit  *msg = (CEdit *) GetDlgItem(IDC_WHAT4);
		msg->SetSel(0, -1);
		msg->ReplaceSel((const char *) w4);
		msg->SetReadOnly(TRUE);
	}
	((CListBox *) GetDlgItem(IDC_FMTLIST))->SetTabStops();
	Decodeformats();
	return TRUE;
}

void CFmtdef::OnOK()
{
	CListBox	*lb = (CListBox *) GetDlgItem(IDC_FMTLIST);
	int	nitems = lb->GetCount();

	CString	result;
	
	for  (int cnt = 0;  cnt < nitems;  cnt++)  {

		Formatrow	&fr = m_formats[int(lb->GetItemData(cnt))];
		
		if  (fr.f_issep)
			result += fr.f_field;
		else  {
			result += '%';
			if  (fr.f_ltab)
				result += '<';
			if  (fr.f_skipright)
				result += '>';
			char	ww[12];
			wsprintf(ww, "%u%c", fr.f_width, fr.f_char);
			result += ww;
		}
	}
	
	m_fmtstring = result;
	CDialog::OnOK();
}

void CFmtdef::OnDblclkFmtlist()
{
	if  (m_numformats != 0)
		OnEditfmt();
	else
		OnNewfmt();
}

void CFmtdef::OnNewfmt()
{
	if  (m_numformats >= MAXFORMATS)  {
		AfxMessageBox(IDP_TOOMANYFMTS, MB_OK|MB_ICONSTOP);
		return;
	}	
	CEditfmt	dlg;
	dlg.m_width = 10;
	dlg.m_uppercode = m_uppercode;
	dlg.m_lowercode = m_lowercode;
	if  (dlg.DoModal() == IDOK)  {
		Formatrow	fr;
		fr.f_char = dlg.m_existing;
		fr.f_issep = FALSE;
		fr.f_ltab = dlg.m_tableft;
		fr.f_skipright = dlg.m_skipright;
		fr.f_width = dlg.m_width;
		if  (isupper(fr.f_char))
			fr.f_code = m_uppercode + fr.f_char - 'A';
		else
			fr.f_code = m_lowercode + fr.f_char - 'a';
		fr.f_field.LoadString(fr.f_code);
		Addformat(fr);    
	}
}

void CFmtdef::OnNewsep()
{
	if  (m_numformats >= MAXFORMATS)  {
		AfxMessageBox(IDP_TOOMANYFMTS, MB_OK|MB_ICONSTOP);
		return;
	}	
	CEditsep	dlg;
	if  (dlg.DoModal() == IDOK)  {
		if  (dlg.m_sepvalue.IsEmpty())
			dlg.m_sepvalue = ' ';
		Formatrow	fr;
		fr.f_char = ' ';
		fr.f_issep = TRUE;
		fr.f_ltab = FALSE;
		fr.f_skipright = FALSE;
		fr.f_width = dlg.m_sepvalue.GetLength();
		fr.f_field = dlg.m_sepvalue;
		Addformat(fr);
	}
}

void CFmtdef::OnEditfmt()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_FMTLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;
	int	rowpos = int(lb->GetItemData(where));
	Formatrow	&fr = m_formats[rowpos];
	if  (fr.f_issep)  {
		CEditsep	dlg;
		dlg.m_sepvalue = fr.f_field;
		if  (dlg.DoModal() != IDOK)
			return;
		if  (dlg.m_sepvalue.IsEmpty())
			dlg.m_sepvalue = ' ';
		fr.f_width = dlg.m_sepvalue.GetLength();
		fr.f_field = dlg.m_sepvalue;
	}
	else  {
		CEditfmt	dlg;
		dlg.m_width = fr.f_width;
		dlg.m_uppercode = m_uppercode;
		dlg.m_lowercode = m_lowercode;
		dlg.m_existing = fr.f_char;
		dlg.m_tableft = fr.f_ltab;
		dlg.m_skipright = fr.f_skipright;
		if  (dlg.DoModal() != IDOK)
			return;
		fr.f_char = dlg.m_existing;
		fr.f_ltab = dlg.m_tableft;
		fr.f_skipright = dlg.m_skipright;
		fr.f_width = dlg.m_width;
		if  (isupper(fr.f_char))
			fr.f_code = m_uppercode + fr.f_char - 'A';
		else
			fr.f_code = m_lowercode + fr.f_char - 'a';
		fr.f_field.LoadString(fr.f_code);
	}
	lb->DeleteString(where);
	CString	rep;
	GenFmtString(fr, rep);
	where = lb->InsertString(where, rep);
	lb->SetItemData(where, DWORD(rowpos));
	m_changes++;
}

void CFmtdef::OnDelfmt()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_FMTLIST);
	int	 where = lb->GetCurSel();
	if  (where < 0)
		return;

	//  Find the index of the selected item in my list
	//  Squash up the list

	int	 rowpos = int(lb->GetItemData(where));
	int	 cnt;
	for  (cnt = rowpos + 1;  cnt < m_numformats;  cnt++)
		m_formats[cnt-1] = m_formats[cnt];

	//  Delete it from the list box and then change the
	//  indices of the items in the list box which refer
	//  to the items in my list to be one less

	lb->DeleteString(where);
	for  (cnt = lb->GetCount() - 1;  cnt >= 0;  cnt--)  {
		int	 rp = int(lb->GetItemData(cnt));
		if  (rp >= rowpos)
			lb->SetItemData(cnt, DWORD(rp-1));
	}
	m_numformats--;      
	m_changes++;
}

void	CFmtdef::Decodeformats()
{
	CListBox  *lb = (CListBox *) GetDlgItem(IDC_FMTLIST);

	const char *fmt = (const char *) m_fmtstring;
	
	while  (*fmt)  {
		if  (m_numformats >= MAXFORMATS)
			return;
			
		Formatrow	&fr = m_formats[m_numformats];

		if  (*fmt != '%')  {
			fr.f_char = ' ';
			fr.f_code = 0;
			fr.f_issep = TRUE;
			fr.f_ltab = FALSE;
			fr.f_skipright = FALSE;
			fr.f_width = 1;
			fr.f_field = *fmt++;
			while  (*fmt &&  *fmt != '%')  {
				fr.f_width++;
				fr.f_field += *fmt++;
			}
		}
		else  {
			fr.f_issep = FALSE;
			fr.f_ltab = FALSE;
			fr.f_skipright = FALSE;
			if  (*++fmt == '<')  {
				fmt++;
				fr.f_ltab = TRUE;
			}
			if  (*fmt == '>')  {
				fmt++;
				fr.f_skipright = TRUE;
			}
			fr.f_width = 0;
			do  fr.f_width = fr.f_width * 10 + *fmt++ - '0';
			while  (isdigit(*fmt));
            fr.f_char = *fmt++;
            if  (isupper(fr.f_char) && m_uppercode != 0)
            	fr.f_code = m_uppercode + fr.f_char - 'A';
            else  if  (islower(fr.f_char) && m_lowercode != 0)
            	fr.f_code = m_lowercode + fr.f_char - 'a';
            else
            	continue;
            if  (!fr.f_field.LoadString(fr.f_code))
            	continue;
		}
		CString	resp;
		GenFmtString(fr, resp);
		int  where = lb->InsertString(-1, resp);
		lb->SetItemData(where, DWORD(m_numformats));
		m_numformats++;
	}
}

int	calc_fmt_length(CString  CBtqwApp ::* whichp)
{
	const  char	 *fmt = ((CBtqwApp *) AfxGetApp())->*whichp;
	int	lng = 0;
	
	while  (*fmt)  {
		if  (*fmt++ != '%')  {
			lng++;
			continue;
		}
		if  (*fmt == '<')
			fmt++;
		if  (*fmt == '>')
			fmt++;
		int  nn = 0;
		while  (isdigit(*fmt))
			nn = nn*10 + *fmt++ - '0';
		lng += nn;
		if  (isalpha(*fmt))
			fmt++;
	}
	return  lng;
}

void CFmtdef::OnResetdeflt()
{
	m_fmtstring.LoadString(m_defcode);
	m_numformats = 0;
	((CListBox *) GetDlgItem(IDC_FMTLIST))->ResetContent();
	Decodeformats();
	m_changes++;
}

void	save_fmt_widths(CString  CBtqwApp ::*ps, const Formatrow *fl, const int nf)
{
	CString	result;
	for  (int cnt = 0;  cnt < nf;  cnt++)  {

		const  Formatrow	&fr = fl[cnt];
		
		if  (fr.f_issep)
			result += fr.f_field;
		else  {
			result += '%';
			if  (fr.f_ltab)
				result += '<';
			if  (fr.f_skipright)
				result += '>';
			char	ww[12];
			wsprintf(ww, "%u%c", fr.f_width, fr.f_char);
			result += ww;
		}
	}

	CBtqwApp *ma = (CBtqwApp *) AfxGetApp();
	ma->*ps = result;
	ma->dirty();
}

const DWORD a116HelpIDs[]=
{
	IDC_WHAT4,	IDH_116_216,	// Set up formats: "" (Edit)
	IDC_FMTLIST,	IDH_116_217,	// Set up formats: "" (ListBox)
	IDC_NEWFMT,	IDH_116_218,	// Set up formats: "New" (Button)
	IDC_NEWSEP,	IDH_116_219,	// Set up formats: "New sep" (Button)
	IDC_EDITFMT,	IDH_116_220,	// Set up formats: "Edit" (Button)
	IDC_DELFMT,	IDH_116_221,	// Set up formats: "Delete" (Button)
	IDC_RESETDEFLT,	IDH_116_222,	// Set up formats: "Reset Default format" (Button)
	0, 0
};

BOOL CFmtdef::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return  do_contexthelp(pHelpInfo->iCtrlId, a116HelpIDs) ||
			CDialog::OnHelpInfo(pHelpInfo);
}
