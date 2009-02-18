// btrsedoc.cpp : implementation of the CBtrsetwDoc class
//

#include "stdafx.h"
#include "btrsetw.h"
#include "btrsedoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwDoc

IMPLEMENT_DYNCREATE(CBtrsetwDoc, CDocument)

BEGIN_MESSAGE_MAP(CBtrsetwDoc, CDocument)
	//{{AFX_MSG_MAP(CBtrsetwDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwDoc construction/destruction

CBtrsetwDoc::CBtrsetwDoc()
{
	// TODO: add one-time construction code here
}

CBtrsetwDoc::~CBtrsetwDoc()
{
}

BOOL CBtrsetwDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwDoc serialization

void CBtrsetwDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


/////////////////////////////////////////////////////////////////////////////
// CBtrsetwDoc diagnostics

#ifdef _DEBUG
void CBtrsetwDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBtrsetwDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwDoc commands
