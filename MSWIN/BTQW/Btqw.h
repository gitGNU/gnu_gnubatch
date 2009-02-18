// btqw.h : main header file for the BTQW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CBtqwApp:
// See btqw.cpp for the implementation of this class
//

class CBtqwApp : public CWinApp
{
public:
	CBtqwApp();

	BOOL		m_initok;
	BOOL		m_unsaved;  			// True if options unsaved
	BOOL		m_warnoffline;
	BOOL		m_binunq;
	int			m_sjbat;
	unsigned 	m_polltime;				// Poll time (ms)	
	restrictdoc	m_restrict;
	timedefs	m_timedefs;
	conddefs	m_conddefs;
	assdefs		m_assdefs;
	joblist		m_appjoblist;
	varlist		m_appvarlist;
	Btuser		m_mypriv;
	CIList		m_cilist;
	CString		m_winuser, m_winmach;	// Windows params
	CString		m_username;				// User name from server
	CString		m_groupname;			// Group name from server
	CString		m_jfmt, m_v1fmt;		// Display formats
	CJobcolours	m_appcolours;
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtqwApp)
	public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	virtual BOOL OnIdle(long);
	//}}AFX_VIRTUAL

// Implementation

	void	loadprogopts();
	void	saveprogopts();
	void	dirty() { m_unsaved = TRUE; }
	BOOL	isdirty() const	{  return  m_unsaved;  }

	//{{AFX_MSG(CBtqwApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
	
inline	joblist	&Jobs()
{
	return  ((CBtqwApp *)AfxGetApp())->m_appjoblist;
}

inline	varlist	&Vars()
{
	return  ((CBtqwApp *)AfxGetApp())->m_appvarlist;
}			
