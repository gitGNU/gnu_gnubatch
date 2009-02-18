// btrw.h : main header file for the BTRW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBtrwApp:
// See btrw.cpp for the implementation of this class
//

class CBtrwApp : public CWinApp
{
public:
	CBtrwApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBtrwApp)
	public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	CString	m_jeditor;
	CString	m_sendowner;
	CString m_sendgroup;
	CString m_jobqueue;
	BOOL	m_promptfirst;
	BOOL	m_verbose;
	BOOL	m_binmode;
	BOOL	m_nonexport;
	Btuser	m_mypriv;
	Btjob	m_defaultjob;
	Jcond	m_defcond;
	Jass	m_defass;
	unsigned short	m_umask;
	long			m_ulimit;
	CIList	m_cilist;
	envtable	m_envtable;
	CString		m_winuser, m_winmach;	// Windows params
    CString	m_username;			// User name from server
	CString	m_groupname;		// Group name from server
	BOOL	m_initok;
private:
	BOOL	m_changes;
	BOOL	m_defchanges;
public:
	void	FileOpen();
	void	dirty() { m_changes = TRUE; }
	void	clean() { m_changes = FALSE; }
	BOOL	isdirty() { return m_changes; }
	void	defdirty() { m_defchanges = TRUE; }
	void	defclean() { m_defchanges = FALSE; }
	BOOL	isdefdirty() { return m_defchanges; }
	void	loaddefault();
	
	//{{AFX_MSG(CBtrwApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileProgramoptions();
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnFileSavedefaults();
	afx_msg void OnUpdateFileSavedefaults(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
