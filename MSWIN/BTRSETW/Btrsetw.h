// btrsetw.h : main header file for the BTRSETW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CBtrsetwApp:
// See btrsetw.cpp for the implementation of this class
//

class CBtrsetwApp : public CWinApp
{
public:
	CBtrsetwApp();

	BOOL		m_initok;
	BOOL		m_needsave;  			// True if options unsaved
	BOOL		m_isvalid;				// Valid server
	CString		m_winuser, m_winmach;
	progdefs	m_progdefs;
	timedefs	m_timedefs;
	conddefs	m_conddefs;
	assdefs		m_assdefs;
	queuedefs	m_queuedefs;
	Btuser		m_mypriv;
	CIList		m_cilist;
	UINT		m_myumask;
	unsigned  long	m_myulimit;
	CString		m_username;		// User name from server
	CString		m_groupname;	// Group name from server

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtrsetwApp)
	public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	void		loadprogopts();
	void		saveprogopts();
	void		dirty() { m_needsave = TRUE; }
	BOOL		isdirty() const	{  return  m_needsave;  }
	

	//{{AFX_MSG(CBtrsetwApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
