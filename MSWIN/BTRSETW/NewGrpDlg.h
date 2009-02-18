#if !defined(AFX_NEWGRPDLG_H__44BE0EC3_76CB_11D1_BA1D_00C0DF501B60__INCLUDED_)
#define AFX_NEWGRPDLG_H__44BE0EC3_76CB_11D1_BA1D_00C0DF501B60__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NewGrpDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewGrpDlg dialog

class CNewGrpDlg : public CDialog
{
// Construction
public:
	CNewGrpDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewGrpDlg)
	enum { IDD = IDD_NEWGROUP };
	CString	m_groupname;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewGrpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewGrpDlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWGRPDLG_H__44BE0EC3_76CB_11D1_BA1D_00C0DF501B60__INCLUDED_)
