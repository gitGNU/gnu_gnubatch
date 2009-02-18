// rowview.cpp : implementation file
// This has been brazenly stolen from the chequebook program

#include "stdafx.h"
#include "netmsg.h"
#include "btqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "netwmsg.h"
#include <limits.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRowView

IMPLEMENT_DYNAMIC(CRowView, CScrollView)

CRowView::CRowView()
{
	m_nSelectedRow = 0;
	m_nformats = 0;
	m_formathacked = FALSE;
}

CRowView::~CRowView()
{
}


BEGIN_MESSAGE_MAP(CRowView, CScrollView)
	//{{AFX_MSG_MAP(CRowView)
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_NETMSG_JADD, OnNMJadd)
	ON_MESSAGE(WM_NETMSG_JCHANGE, OnNMJchange)
	ON_MESSAGE(WM_NETMSG_JDEL, OnNMJdel)
	ON_MESSAGE(WM_NETMSG_JREVISED, OnNMJrevised)
	ON_MESSAGE(WM_NETMSG_VADD, OnNMVadd)
	ON_MESSAGE(WM_NETMSG_VCHANGE, OnNMVchange)
	ON_MESSAGE(WM_NETMSG_VDEL, OnNMVDel)
	ON_MESSAGE(WM_NETMSG_VREVISED, OnNMVrevised)
	ON_NOTIFY(HDN_ENDTRACK, 1023, OnEndTrack)
	ON_NOTIFY(HDN_DIVIDERDBLCLICK, 1023, OnBdrDblClk)
	ON_COMMAND(ID_WINDOW_SAVEWIDTHSASDEFAULT, OnWindowSavewidthsasdefault)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_SAVEWIDTHSASDEFAULT, OnUpdateWindowSavewidthsasdefault)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRowView drawing

void CRowView::OnInitialUpdate()
{
	m_nPrevRowCount = GetRowCount();
	m_nSelectedRow = GetActiveRow();
}

void CRowView::UpdateRow(int nInvalidRow)
{
	int nRowCount = GetRowCount();

	// If the number of rows has changed, then adjust the scrolling range.

	if (nRowCount != m_nPrevRowCount)  {
		UpdateScrollSizes();
		m_nPrevRowCount = nRowCount;
	}

	CClientDC dc(this);
	OnPrepareDC(&dc);

	int nFirstRow, nLastRow;
	CRect rectClient;
	GetClientRect(&rectClient);
	dc.DPtoLP(&rectClient);
	RectLPtoRowRange(rectClient, nFirstRow, nLastRow, FALSE);
	if  (m_header.m_hWnd)
		nInvalidRow--;

	POINT pt;
	pt.x = 0;
	BOOL bNeedToScroll = TRUE;
	if (nInvalidRow < nFirstRow)
		pt.y = RowToYPos(nInvalidRow);
	else if (nInvalidRow > nLastRow)
		pt.y = max(0, RowToYPos(nInvalidRow+1) - rectClient.Height());
	else
		bNeedToScroll = FALSE;

	if (bNeedToScroll)  {
		ScrollToDevicePosition(pt);
		OnPrepareDC(&dc);
	}

	CRect rectInvalid = RowToWndRect(&dc, nInvalidRow);
	InvalidateRect(&rectInvalid);
	int nNewSelectedRow = GetActiveRow();
	if (nNewSelectedRow != m_nSelectedRow)  {
		CRect rectOldSelection = RowToWndRect(&dc, m_nSelectedRow);
		InvalidateRect(&rectOldSelection);
		m_nSelectedRow = nNewSelectedRow;
	}
}

void CRowView::UpdateScrollSizes()
{
	CRect rectClient;
	GetClientRect(&rectClient);

	CClientDC dc(this);
	CalculateRowMetrics(&dc);

	CSize sizeTotal(m_nRowWidth, m_nRowHeight * (GetRowCount() + 1));
	CSize sizePage(m_nRowWidth/5, max(m_nRowHeight, ((rectClient.bottom/m_nRowHeight)-1)*m_nRowHeight));
	CSize sizeLine(m_nRowWidth/20, m_nRowHeight);
	SetScrollSizes(MM_TEXT, sizeTotal, sizePage, sizeLine);
	
	rectClient.SetRect(-GetScrollPosition().x, 0, m_nRowWidth, m_nRowHeight);

	if  (m_header.m_hWnd)
		m_header.MoveWindow(&rectClient);
	else
		m_header.Create(HDS_HORZ | WS_TABSTOP |
						WS_VISIBLE |
						CCS_TOP |
						WS_CHILD |
						WS_BORDER,
						rectClient, this, 1023);
}

void CRowView::OnDraw(CDC* pDC)
{
	if (GetRowCount() == 0)
		return;

	// The window has been invalidated and needs to be repainted.
	// First, determine the range of rows that need to be displayed.

	int nFirstRow, nLastRow;
	CRect rectClip;
	pDC->GetClipBox(&rectClip); // Get the invalidated region.
	RectLPtoRowRange(rectClip, nFirstRow, nLastRow, TRUE);

	// Draw each row in the invalidated region of the window.
	
	int nActiveRow = GetActiveRow();
	int nRow, y, yoffset = 0;
	int nLastViewableRow = LastViewableRow();
	if  (m_header.m_hWnd)  {
		if  (rectClip.PtInRect(CPoint(m_nRowWidth/2,m_nRowHeight/2)))
			SetUpHdr();
		yoffset = m_nRowHeight;
	}
	for (nRow = nFirstRow, y = m_nRowHeight * nFirstRow + yoffset; nRow <= nLastRow; nRow++, y += m_nRowHeight) {
		if (nRow > nLastViewableRow)  {
			CString strWarning;
			strWarning.LoadString(IDP_TOO_MANY_ROWS);
			pDC->TextOut(0, y, strWarning);
			break;
		}
		OnDrawRow(pDC, nRow, y, nRow == nActiveRow);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Implementation

int CRowView::RowToYPos(int nRow)
{
	return nRow * m_nRowHeight;
}

CRect CRowView::RowToWndRect(CDC* pDC, int nRow)
{
	int nHorzRes = pDC->GetDeviceCaps(HORZRES);
	if  (m_header.m_hWnd)
		nRow++;
	CRect rect(0, nRow * m_nRowHeight, nHorzRes, (nRow + 1) * m_nRowHeight);
	pDC->LPtoDP(&rect);
	return rect;
}

int CRowView::LastViewableRow()
{
	return  INT_MAX / m_nRowHeight - 1;
}

void CRowView::RectLPtoRowRange(const CRect& rect, int& nFirstRow, int& nLastRow, BOOL bIncludePartiallyShownRows)
{
	int nRounding = bIncludePartiallyShownRows? 0 : (m_nRowHeight - 1);
	nFirstRow = (rect.top + nRounding) / m_nRowHeight;
	if  (m_header.m_hWnd)  {
		if  (nFirstRow > 0)
			nFirstRow--;
		nLastRow = min(unsigned((rect.bottom - nRounding) / m_nRowHeight) - 1, GetRowCount() - 1);
	}
	else
		nLastRow = min(unsigned((rect.bottom - nRounding) / m_nRowHeight), GetRowCount() - 1);
}

void CRowView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CScrollView::OnPrepareDC(pDC, pInfo);
	CalculateRowMetrics(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// CRowView commands

void CRowView::OnSize(UINT nType, int cx, int cy)
{
	UpdateScrollSizes();
	CScrollView::OnSize(nType, cx, cy);
}

void CRowView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{                  
	int	nact = GetActiveRow();

	switch (nChar)
	{
		case VK_HOME:
			ChangeSelectionToRow(0);
			break;
		case VK_END:
			ChangeSelectionToRow(GetRowCount() - 1);
			break;
		case VK_UP:              
			if  (nact > 0)
			    ChangeSelectionToRow(nact - 1);
			break;
		case VK_DOWN:
			if  (unsigned(nact) < GetRowCount() - 1)
				ChangeSelectionToRow(nact + 1);
			break;
		case VK_PRIOR:
			OnVScroll(SB_PAGEUP,
				0,  // nPos not used for PAGEUP
				GetScrollBarCtrl(SB_VERT));
			break;
		case VK_NEXT:
			OnVScroll(SB_PAGEDOWN,
				0,  // nPos not used for PAGEDOWN
				GetScrollBarCtrl(SB_VERT));
			break;
		default:
			CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
	}
}

void CRowView::OnLButtonDown(UINT, CPoint point)
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	dc.DPtoLP(&point);
	CRect rect(point, CSize(1,1));
	int nFirstRow, nLastRow;
	RectLPtoRowRange(rect, nFirstRow, nLastRow, TRUE);
	if (unsigned(nFirstRow) <= (GetRowCount() - 1))  {
		ChangeSelectionToRow(nFirstRow);              
		GetDocument()->UpdateAllViews(NULL);
	}	
}

// Null routines for jobs when we're looking at the var list and
// vice versa.

void	CRowView::JobAllChange(const BOOL) {}
void	CRowView::JobChgJob(const unsigned) {}
void	CRowView::VarAllChange(const BOOL) {}
void	CRowView::VarChgVar(const unsigned) {}

LRESULT CRowView::OnNMJadd(WPARAM wParam, LPARAM lParam)
{
	JobAllChange();
	return  0;
}

LRESULT CRowView::OnNMJchange(WPARAM wParam, LPARAM lParam)
{
	JobChgJob(wParam);
	return  0;
}

LRESULT CRowView::OnNMJdel(WPARAM wParam, LPARAM lParam)
{
	JobAllChange();
	return  0;
}

LRESULT CRowView::OnNMJrevised(WPARAM wParam, LPARAM lParam)
{
	JobAllChange(lParam != 0);
	return  0;
}

LRESULT CRowView::OnNMVadd(WPARAM wParam, LPARAM lParam)
{
	VarAllChange();
	return  0;
}

LRESULT CRowView::OnNMVchange(WPARAM wParam, LPARAM lParam)
{
	VarChgVar(wParam);
	return  0;
}

LRESULT CRowView::OnNMVDel(WPARAM wParam, LPARAM lParam)
{
	VarAllChange();
	return  0;
}

LRESULT CRowView::OnNMVrevised(WPARAM wParam, LPARAM lParam)
{
	VarAllChange(lParam != 0);
	return  0;
}

void	CRowView::RedrawRow(const int row)
{
	CClientDC dc(this);
	OnPrepareDC(&dc);
	CRect rectClip;
	dc.GetClipBox(&rectClip); // Get the invalidated region.
	int nFirstRow, nLastRow, yoffset = 0;
	RectLPtoRowRange(rectClip, nFirstRow, nLastRow, TRUE);
	if  (row >= nFirstRow  &&  row <= nLastRow)  {
		if  (m_header.m_hWnd)
			yoffset = m_nRowHeight;
		OnDrawRow(&dc, row, yoffset + m_nRowHeight * row, row == GetActiveRow());
	}
}		 

int CRowView::GetFmtIndex(const int butt, int &xwid)
{
	int	bcnt = 0, fcnt = 0;
	Formatrow  *fl = m_formats;

	//  Find first non-separator

	while  (fl->f_issep)  {
		fl++;
		if  (++fcnt >= m_nformats)
			return  -1;
	}

	//  Find required button

	while  (bcnt < butt)  {
		bcnt++;
		do  {
			fl++;
			if  (++fcnt >= m_nformats)
				return  -1;
		}  while  (fl->f_issep);
	}

	//  Measure trailing width of sep chars

	xwid = 0;
	fl++;
	for  (int  scnt = fcnt + 1; scnt < m_nformats  &&  fl->f_issep; fl++, scnt++)
		xwid += fl->f_width;

	return  fcnt;
}

void CRowView::OnEndTrack(NMHDR* pNMHDR, LRESULT* pResult)
{
	int	 twid, butt = ((HD_NOTIFY *)pNMHDR)->iItem;
	HD_ITEM	*pn = ((HD_NOTIFY *)pNMHDR)->pitem; 
	int  fmtindx = GetFmtIndex(butt, twid);
	if  (fmtindx < 0)
		return;
	m_formats[fmtindx].f_width = pn->cxy/m_avecharwidth - twid;
	HD_ITEM	hd;
	hd.mask = HDI_WIDTH;
	hd.cxy = (m_formats[fmtindx].f_width + twid) * m_avecharwidth;
	m_header.SetItem(butt, &hd);
	UpdateScrollSizes();
	Invalidate();
	m_formathacked = TRUE;
}

void CRowView::OnBdrDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
	int	 twid, butt = ((HD_NOTIFY *)pNMHDR)->iItem;
	int  fmtindx = GetFmtIndex(butt, twid);
	Formatrow  &fp = m_formats[fmtindx];
	if  (fmtindx < 0  || fp.f_maxwidth == 0 || fp.f_maxwidth == fp.f_width)
		return;
	HD_ITEM	hd;
	hd.mask = HDI_WIDTH;
	fp.f_width = fp.f_maxwidth;
	hd.cxy = (fp.f_width + twid) * m_avecharwidth;
	m_header.SetItem(butt, &hd);
	UpdateScrollSizes();
	Invalidate();
	m_formathacked = TRUE;
}

BOOL CRowView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	Initformats();
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void CRowView::Initformats(CString CBtqwApp::* fstr, int (*const*ut)(...), int (*const*lt)(...), const UINT ucode, const UINT lcode, const char *rj)
{
	Formatrow	*fl = m_formats;
	
	for  (int cnt = 0;  cnt < m_nformats;  cnt++)
		fl[cnt].f_field.Empty();
	m_nformats = 0;

	const char	*cp = ((CBtqwApp *)AfxGetApp())->*fstr;

	while  (*cp  &&  m_nformats < MAXFORMATS)  {

		fl->f_issep = FALSE;
		fl->f_ltab = FALSE;
		fl->f_skipright = FALSE;
		fl->f_rjust = FALSE;
		fl->f_fmtfn = NULL;
		fl->f_code = 0;
		fl->f_width = 0;
		fl->f_maxwidth = 0;
		fl->f_char = ' ';

		if  (*cp == '%'  &&  cp[1] != '%')  {
			cp++;
			//  Interpret flag chars
			if  (*cp == '<')  {
				cp++;
				fl->f_ltab = TRUE;
			}
			if  (*cp == '>')  {
				cp++;
				fl->f_skipright = TRUE;
			}

			//  Init width

			while  (isdigit(*cp))
				fl->f_width = fl->f_width * 10 + *cp++ - '0';

			//  Interpret format char

			int	fmtchar = *cp++;
			fl->f_char = fmtchar;

			if  (ut  &&  isupper(fmtchar))  {
				fl->f_fmtfn = ut[fmtchar-'A'];
				fl->f_code = ucode + fmtchar - 'A';
			}
			else  if  (lt  &&  islower(fmtchar))  {
				fl->f_fmtfn = lt[fmtchar-'a'];
				fl->f_code = lcode + fmtchar - 'a';
			}
			else
				continue;

			if  (!fl->f_fmtfn)
				continue;

			if  (strchr(rj, fmtchar))
				fl->f_rjust = TRUE;

			if  (!fl->f_field.LoadString(fl->f_code))  {
				char  nbuf[2];
				nbuf[0] = fmtchar;
				nbuf[1] = '\0';
				fl->f_field = nbuf;
			}
			m_nformats++;
			fl++;
			continue;
		}

		//  Separator case

		fl->f_issep = TRUE;
		if  (*cp == '%')
			cp++;
		do  {
			fl->f_field += *cp++;
			fl->f_width++;
		}  while  (*cp && *cp != '%');

		m_nformats++;
		fl++;
	}
}

void CRowView::SetUpHdr()
{
	Formatrow	*fl = m_formats;
	int	nIndex = 0;
	int	fcnt = 0;

	while  (m_header.GetItemCount() > 0)
		m_header.DeleteItem(0);

	while  (fcnt < m_nformats)  {
		if  (!fl->f_issep)  {
			HD_ITEM	hdi;
			UINT	wid = fl->f_width;
			int	fc2;
			for  (fc2 = fcnt+1;  fc2 < m_nformats && m_formats[fc2].f_issep;  fc2++)
				wid += m_formats[fc2].f_width;
			hdi.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH;
			hdi.cxy = m_avecharwidth * wid;
			hdi.fmt = fl->f_rjust? HDF_RIGHT|HDF_STRING: HDF_LEFT|HDF_STRING;
			hdi.cchTextMax = fl->f_field.GetLength();
			hdi.pszText = (char *) (const char *) fl->f_field;
			m_header.InsertItem(nIndex, &hdi);
			nIndex++;
		}
		fl++;
		fcnt++;
	}
}

void CRowView::OnWindowSavewidthsasdefault() 
{
	SaveWidthSettings();
	m_formathacked = FALSE;
}

void CRowView::OnUpdateWindowSavewidthsasdefault(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_formathacked);	
}

void CRowView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	OnLButtonDown(nFlags, point);
	CRect  rect;
	GetWindowRect(&rect);
	Dopopupmenu(point + CPoint(rect.left, rect.top));
}
