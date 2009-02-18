// Jobcolours.h: interface for the CJobcolours class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_JOBCOLOURS_H__F63C3805_45F6_11D4_9542_00E09872E940__INCLUDED_)
#define AFX_JOBCOLOURS_H__F63C3805_45F6_11D4_9542_00E09872E940__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CJobcolours  
{
private:
	COLORREF	m_table[7];

public:
	CJobcolours();
	~CJobcolours();

	enum jcl_type	{
		READY_CL = 0, STARTUP_CL = 1, RUNNING_CL = 2, FINISHED_CL = 3,
		ERROR_CL = 4, ABORT_CL = 5, CANC_CL = 6 };

	COLORREF & operator [] (const unsigned);

private:
	const jcl_type whichcol(const unsigned);
};

#endif // !defined(AFX_JOBCOLOURS_H__F63C3805_45F6_11D4_9542_00E09872E940__INCLUDED_)
