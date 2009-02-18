#include "stdafx.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "vardoc.h"
#include "varview.h"
#include <stdlib.h>
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CVarView, CRowView)

extern	int	calc_fmt_length(CString  CBtqwApp ::*);                           ;
extern	void	save_fmt_widths(CString  CBtqwApp ::*, const Formatrow *, const int);

/////////////////////////////////////////////////////////////////////////////
CVarView::CVarView()
{
}

void CVarView::OnUpdate(CView*, LPARAM lHint, CObject* pHint)
{   
	Invalidate();
	for  (int fcnt = 0;  fcnt < m_nformats;  fcnt++)
		m_formats[fcnt].f_maxwidth = 0;
}

void CVarView::GetRowWidthHeight(CDC* pDC, int& nRowWidth, int& nRowHeight)
{
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	nRowWidth = tm.tmAveCharWidth * calc_fmt_length(&CBtqwApp::m_v1fmt);
	nRowHeight = tm.tmHeight;
	m_avecharwidth = tm.tmAveCharWidth;
}

int CVarView::GetActiveRow()
{
	return  ((CVardoc*)GetDocument())->vsortindex(curr_var);
}

unsigned CVarView::GetRowCount()
{
	return  ((CVardoc*)GetDocument())->number();
}

void CVarView::ChangeSelectionToRow(int nRow)
{
	CVardoc* pDoc = GetDocument();
	Btvar  *p = pDoc->sorted(nRow);
	if  (p)
		curr_var = vident(*p);
	else
		curr_var = vident();
}               

CString	result;

static	int  fmt_comment(const Btvar &vp, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	result = vp.var_comment;
	return  result.GetLength();
}

static	int  fmt_group(const Btvar &vp, const int isreadable, const int fwidth)
{
	result = vp.var_mode.o_group;
	return  result.GetLength();
}

static	int  fmt_export(const Btvar &vp, const int isreadable, const int fwidth)
{
	if  (vp.var_flags & VF_EXPORT)
		result.LoadString(IDS_EXPORT);
	return  result.GetLength();
}

static	int  fmt_cluster(const Btvar &vp, const int isreadable, const int fwidth)
{
	if  (vp.var_flags & VF_CLUSTER)
		result.LoadString(IDS_CLUSTER);
	return  result.GetLength();
}

static	void dumpmode(const char *prefix, const unsigned md)
{
	result += prefix;
	result += ':';
	if  (md & BTM_READ)
		result += 'R';
	if  (md & BTM_WRITE)
		result += 'W';
	if  (md & BTM_SHOW)
		result += 'S';
	if  (md & BTM_RDMODE)
		result += 'M';
	if  (md & BTM_WRMODE)
		result += 'P';
	if  (md & BTM_UGIVE)
		result += 'U';
	if  (md & BTM_UTAKE)
		result += 'V';
	if  (md & BTM_GGIVE)
		result += 'G';
	if  (md & BTM_GTAKE)
		result += 'H';
	if  (md & BTM_DELETE)
		result += 'D';
}

static	int  fmt_mode(const Btvar &vp, const int isreadable, const int fwidth)
{
	if  (!vp.var_mode.mpermitted(BTM_RDMODE))
		return  0;
	dumpmode("U", vp.var_mode.u_flags);
	dumpmode(",G", vp.var_mode.g_flags);
	dumpmode(",O", vp.var_mode.o_flags);
	return  result.GetLength();
}

static	int  fmt_name(const Btvar &vp, const int isreadable, const int fwidth)
{
	result = look_host(vp.hostid);
	result += ':';
	result += vp.var_name;
	return  result.GetLength();
}

static	int  fmt_user(const Btvar &vp, const int isreadable, const int fwidth)
{
	result = vp.var_mode.o_user;
	return  result.GetLength();
}

static	int  fmt_value(const Btvar &vp, const int isreadable, const int fwidth)
{
	if  (!isreadable)
		return  0;
	const  Btcon	&cp = vp.var_value;
	if  (cp.const_type == CON_STRING)
		result = cp.con_string;
	else  {
		char	thing[20];
		wsprintf(thing, "%ld", cp.con_long);
		result = thing;
	}
	return  result.GetLength();
}

typedef	int	(*fmt_fn)(const Btvar &, const int, const int);

const	fmt_fn	uppertab[] = {
	NULL,			//	A
	NULL,			//  B
	fmt_comment,    //  C
	NULL,           //  D
	fmt_export,		//  E
	NULL,			//  F
	fmt_group,		//  G
	NULL, NULL, NULL,   //  H,I,J
	fmt_cluster,	//  K
	NULL,	        //  L
	fmt_mode,		//  M
	fmt_name,		//  N
	NULL, NULL, NULL, NULL, NULL, NULL,	//  O,P,Q,R,S,T
	fmt_user,		//  U
	fmt_value,		//  V
	NULL, NULL, NULL, NULL			//  W, X, Y, Z
};

void	CVarView::Initformats()
{
	CRowView::Initformats(&CBtqwApp::m_v1fmt, (int (*const*)(...))uppertab, NULL, IDS_SVFORMAT_C-2, 0, "");
}

void CVarView::OnDrawRow(CDC* pDC, int nRow, int y, BOOL bSelected)
{
	CBrush brushBackground;
	COLORREF crOldText = 0;
	COLORREF crOldBackground = 0;

	if  (bSelected) {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		crOldBackground = pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		crOldText = pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else  {
		brushBackground.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		pDC->SetBkMode(TRANSPARENT);
	}
	
	CRect rectSelection;
	pDC->GetClipBox(&rectSelection);
	rectSelection.top = y;
	rectSelection.bottom = y + m_nRowHeight;
	pDC->FillRect(&rectSelection, &brushBackground);
	
	Btvar	*cv = ((CVardoc *)GetDocument())->sorted(nRow);
	
	if  (cv)  {
	
		Formatrow	*fl = m_formats;
		int	 currplace = -1, nominal_width = 0, fcnt = 0;

		BOOL	isreadable = cv->var_mode.mpermitted(BTM_READ);

		while  (fcnt < m_nformats)  {

			if  (fl->f_issep)  {
				CString	&fld = fl->f_field;
				int	lng = fld.GetLength();
				int	wc;
				for  (wc = 0;  wc < lng;  wc++)
					if  (fld[wc] != ' ')
						break;
				if  (wc < lng)
					pDC->TextOut((nominal_width + wc) * m_avecharwidth, y, fld.Right(lng-wc));
				nominal_width += fl->f_width;
				fl++;
				fcnt++;
				continue;
			}

			int  lastplace = -1;
			int	 nn = fl->f_width;
			if  (fl->f_ltab)
				lastplace = currplace;
			currplace = nominal_width;
			result.Empty();
			int  inlen = (*(fmt_fn)fl->f_fmtfn)(*cv, isreadable, nn);

			if  (inlen > nn  &&  lastplace >= 0)  {
				nn += currplace - lastplace;
				currplace = lastplace;
			}

			if  (inlen > nn)  {
				if  (fl->f_skipright)  {
					if  (UINT(inlen) > fl->f_maxwidth)
						fl->f_maxwidth = inlen;
					pDC->TextOut(currplace * m_avecharwidth, y, result);
					break;
				}
				result = result.Left(nn);
			}
			
			if  (inlen > 0  &&  result[0] == ' ')  {
				for  (int  wc = 1;  wc < inlen  &&  result[wc] == ' ';  wc++)
					;  
				inlen -= wc;
				pDC->TextOut((currplace + nn - inlen) * m_avecharwidth, y, result.Right(inlen));
			}
			else	
				pDC->TextOut(currplace * m_avecharwidth, y, result);
			if  (UINT(inlen) > fl->f_maxwidth)
				fl->f_maxwidth = inlen;
			nominal_width += fl->f_width;
			fl++;
			fcnt++;
		}
	}

	// Restore the DC.
	if (bSelected)	{
		pDC->SetBkColor(crOldBackground);
		pDC->SetTextColor(crOldText);
	}
}

void	CVarView::VarAllChange(const BOOL plushdr)
{
	if  (plushdr)  {
		Initformats();
		SetUpHdr();
	}
	((CVardoc *)GetDocument())->revisevars(Vars());
	UpdateScrollSizes();
}

void	CVarView::VarChgVar(const unsigned ind)
{
	Btvar	*vt = Vars()[ind];
	if  (vt)  {
		int  myind = ((CVardoc *)GetDocument())->vsortindex(*vt);
		if  (myind >= 0)
			RedrawRow(myind);
	}		
}

void	CVarView::SaveWidthSettings()
{
	save_fmt_widths(&CBtqwApp ::m_v1fmt, m_formats, m_nformats);
}

void	CVarView::Dopopupmenu(CPoint point)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_VARFLOAT));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}

