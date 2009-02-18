#if !defined(AFX_LOGINOUT_H__44BE0EC2_76CB_11D1_BA1D_00C0DF501B60__INCLUDED_)
#define AFX_LOGINOUT_H__44BE0EC2_76CB_11D1_BA1D_00C0DF501B60__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LoginOut.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginOut dialog

class CLoginOut : public CDialog
{
// Construction
public:
	CLoginOut(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoginOut)
	enum { IDD = IDD_LOGINOUT };
	CString	m_unixuser;
	CString	m_winuser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginOut)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLoginOut)
	afx_msg void OnLogin();
	afx_msg void OnLogout();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINOUT_H__44BE0EC2_76CB_11D1_BA1D_00C0DF501B60__INCLUDED_)
