// Jobcolours.cpp: implementation of the CJobcolours class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "btqw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CJobcolours::CJobcolours()
{
	COLORREF dflt = ::GetSysColor(COLOR_WINDOWTEXT);
	for  (int i = 0;  i < 7;  i++)
		m_table[i] = dflt;
}

CJobcolours::~CJobcolours()
{

}

COLORREF	&CJobcolours::operator [] (const unsigned req)
{
	return  m_table[whichcol(req)];
}

const CJobcolours::jcl_type  CJobcolours::whichcol(const unsigned req)
{
	switch  (req)  {
	default:
	case BJP_NONE:		return  READY_CL;
	case BJP_DONE:		return  FINISHED_CL;
	case BJP_ERROR:		return  ERROR_CL;
	case BJP_ABORTED:	return  ABORT_CL;
	case BJP_CANCELLED:	return  CANC_CL;
	case BJP_STARTUP1:case  BJP_STARTUP2:	return  STARTUP_CL;
	case BJP_RUNNING: 	return  RUNNING_CL;
	case BJP_FINISHED:	return  FINISHED_CL;
	}
}

