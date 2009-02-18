#if !defined(AFX_LOGINHOST_H__50E2F401_778D_11D1_BA1D_00C0DF501B60__INCLUDED_)
#define AFX_LOGINHOST_H__50E2F401_778D_11D1_BA1D_00C0DF501B60__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LoginHost.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginHost dialog

class CLoginHost : public CDialog
{
// Construction
public:
	CLoginHost(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoginHost)
	enum { IDD = IDD_LOGINHOST };
	CString	m_unixhost;
	CString	m_clienthost;
	CString	m_username;
	CString	m_passwd;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginHost)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoginHost)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINHOST_H__50E2F401_778D_11D1_BA1D_00C0DF501B60__INCLUDED_)
