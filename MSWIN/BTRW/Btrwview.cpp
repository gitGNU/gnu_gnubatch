// btrwview.cpp : implementation of the CBtrwView class
//

#include "stdafx.h"
#include "xbwnetwk.h"
#include "btrw.h"
#include "btrwdoc.h"
#include "btrwview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBtrwView

IMPLEMENT_DYNCREATE(CBtrwView, CView)

BEGIN_MESSAGE_MAP(CBtrwView, CView)
	//{{AFX_MSG_MAP(CBtrwView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtrwView construction/destruction

CBtrwView::CBtrwView()
{
	// TODO: add construction code here
}

CBtrwView::~CBtrwView()
{
}

/////////////////////////////////////////////////////////////////////////////
// CBtrwView drawing

void CBtrwView::OnDraw(CDC* pDC)
{
	pDC->SetBkMode(TRANSPARENT);
	CBtrwDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	TEXTMETRIC textm;
	pDC->GetTextMetrics(&textm);
	int nRowHeight = textm.tmHeight;
	CString	msg, msg2;
	msg.LoadString(IDS_JOBFNAME);
	msg += ": ";
	msg += pDoc->m_jobfile;
	pDC->TextOut(0, 0, msg);
	CBtrwApp &ma = *((CBtrwApp *)AfxGetApp());
	Btjob  &jj = pDoc->m_currjob;
	int  ep;
	msg.LoadString(IDS_QUEUENAME);
	msg += ": ";
	if  ((ep = jj.bj_title.Find(':')) > 0)  {
		msg += jj.bj_title.Left(ep);
		msg += "  ";
		msg2.LoadString(IDS_HEADER);
		msg += msg2;
		msg += ": ";
		msg += jj.bj_title.Right(jj.bj_title.GetLength() - ep - 1);
	}
	else  {
		msg += ma.m_jobqueue;
		msg += "  ";
		msg2.LoadString(IDS_HEADER);
		msg += msg2;
		msg += ": ";
		msg += jj.bj_title;
	}
	pDC->TextOut(0, 3*nRowHeight, msg);
	msg.LoadString(IDS_PRINAME);
	char  nbuf[20];
	wsprintf(nbuf, ": %u  ", unsigned(jj.bj_pri));
	msg += nbuf;
	msg2.LoadString(IDS_CINAME);
	msg += msg2;
	msg += ": ";
	msg += jj.bj_cmdinterp;
	msg += "  ";
	msg2.LoadString(IDS_LOADLNAME);
	msg += msg2;
	wsprintf(nbuf, ": %u", unsigned(jj.bj_ll));
	msg += nbuf;
	pDC->TextOut(0, 6*nRowHeight, msg);
	if  (jj.bj_progress == BJP_NONE)
		msg.LoadString(IDS_RUNSTATE);
	else  if  (jj.bj_progress == BJP_DONE)
		msg.LoadString(IDS_DONESTATE);
	else
		msg.LoadString(IDS_CANCSTATE);
	pDC->TextOut(0, 9*nRowHeight, msg);
}

/////////////////////////////////////////////////////////////////////////////
// CBtrwView diagnostics

#ifdef _DEBUG
void CBtrwView::AssertValid() const
{
	CView::AssertValid();
}

void CBtrwView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CBtrwDoc* CBtrwView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBtrwDoc)));
	return (CBtrwDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBtrwView message handlers
