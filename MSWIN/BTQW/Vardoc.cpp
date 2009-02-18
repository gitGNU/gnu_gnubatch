// vardoc.cpp : implementation file
//

#include "stdafx.h"
#include "mainfrm.h"
#include "netmsg.h"
#include "btqw.h"
#include "formatcode.h"
#include "rowview.h"
#include "vardoc.h"
#include "varview.h"
#include "varperm.h"
#include "vsearch.h"
#include "assvar.h"
#include "varcomm.h"
#include "constval.h"
#include "winopts.h"

BOOL  Smstr(const CString &, const char *, const BOOL = FALSE);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVardoc

IMPLEMENT_DYNCREATE(CVardoc, CDocument)

CVardoc::CVardoc()
{
	m_suser = m_svname = m_svcomment = m_svvalue = TRUE;
	m_swraparound = FALSE;
	m_constvalue = 1;
	CBtqwApp	*mw = (CBtqwApp *)AfxGetApp();
	m_wrestrict = mw->m_restrict;
	revisevars(mw->m_appvarlist);
}

CVardoc::~CVardoc()
{
}


BEGIN_MESSAGE_MAP(CVardoc, CDocument)
	//{{AFX_MSG_MAP(CVardoc)
	ON_COMMAND(ID_ACTION_PERMISSIONS, OnActionPermissions)
	ON_COMMAND(ID_SEARCH_SEARCHBACKWARDS, OnSearchSearchbackwards)
	ON_COMMAND(ID_SEARCH_SEARCHFOR, OnSearchSearchfor)
	ON_COMMAND(ID_SEARCH_SEARCHFORWARD, OnSearchSearchforward)
	ON_COMMAND(ID_VARIABLES_ASSIGN, OnVariablesAssign)
	ON_COMMAND(ID_VARIABLES_DECREMENT, OnVariablesDecrement)
	ON_COMMAND(ID_VARIABLES_DIVIDE, OnVariablesDivide)
	ON_COMMAND(ID_VARIABLES_INCREMENT, OnVariablesIncrement)
	ON_COMMAND(ID_VARIABLES_MULTIPLY, OnVariablesMultiply)
	ON_COMMAND(ID_VARIABLES_REMAINDER, OnVariablesRemainder)
	ON_COMMAND(ID_VARIABLES_SETCOMMENT, OnVariablesSetcomment)
	ON_COMMAND(ID_VARIABLES_SETCONSTANT, OnVariablesSetconstant)
	ON_COMMAND(ID_WINDOW_WINDOWOPTIONS, OnWindowWindowoptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVardoc commands

void	CVardoc::revisevars(varlist &src)
{
	m_varlist.clear();                  
	for  (unsigned cnt = 0; cnt < src.number(); cnt++)  {
		Btvar	*v = src.sorted(cnt);
		if  (m_wrestrict.visible(*v))
			m_varlist.append(v);
	}
	UpdateAllViews(NULL);
}
		 
Btvar	*CVardoc::GetSelectedVar(const unsigned perm, const BOOL moan)
{                 
	POSITION	p = GetFirstViewPosition();
	CVarView	*aview = (CVarView *)GetNextView(p);
	if  (aview)  {
		int	cr = aview->GetActiveRow();
		if  (cr >= 0)  {
			Btvar	 *result = sorted(cr);
			if  (!result->var_mode.mpermitted(perm))  {
				if  (moan)
					AfxMessageBox(IDP_VNOTPERM, MB_OK|MB_ICONSTOP);
				return  NULL;
			}
			return  result;
		}
	}
	if  (moan)
		AfxMessageBox(IDP_NOVARSELECTED, MB_OK|MB_ICONEXCLAMATION);
	return  NULL;
}			

Btvar	*CVardoc::GetArithVar()
{
	Btvar	*result = GetSelectedVar();
	if  (result  &&  result->var_value.const_type != CON_LONG)  {
		AfxMessageBox(IDP_NOTARITHVAR, MB_OK|MB_ICONEXCLAMATION);
		return  NULL;
	}
	return  result;
}		
		
	
BOOL CVardoc::Smatches(const CString str, const int ind)
{
	Btvar  *v = sorted(ind);
	if  (m_suser && Smstr(str, v->var_mode.o_user, TRUE))
		return  TRUE;
	if  (m_svname && Smstr(str, v->var_name, TRUE))
		return  TRUE;
	if  (m_svcomment && Smstr(str, v->var_comment, FALSE))
		return  TRUE;
	if  (m_svvalue)  {
		if  (v->var_value.const_type == CON_STRING)  {
			if  (Smstr(str, v->var_value.con_string, FALSE))
				return  TRUE;                         
		}
		else  {
			char  buf[20];
			wsprintf(buf, "%ld", v->var_value.con_long);
			if  (Smstr(str, buf, TRUE))
				return  TRUE;
		}
	}			
	return FALSE;
}	

void CVardoc::DoSearch(const CString str, const BOOL forward)
{             
	int	cnt;
	Btvar  *cv = GetSelectedVar(BTM_READ, FALSE);
	if  (forward)  {
		int	cwhich = cv? vsortindex(*cv): -1;
		for  (cnt = cwhich + 1;  cnt < int(number());  cnt++)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = 0;  cnt < cwhich;  cnt++)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	else  {
		int  cwhich = cv? vsortindex(*cv): number();
		for  (cnt = cwhich - 1;  cnt >= 0;  cnt--)
			if  (Smatches(str, cnt))
				goto  gotit;
		if  (m_swraparound)
			for  (cnt = number() - 1;  cnt > cwhich;  cnt--)
				if  (Smatches(str, cnt))
					goto  gotit;
	}
	AfxMessageBox(IDP_VNOTFOUND, MB_OK|MB_ICONEXCLAMATION);
	return;
	
gotit:							
	POSITION	p = GetFirstViewPosition();
	CVarView	*aview = (CVarView *)GetNextView(p);
	if  (aview)  {
		aview->ChangeSelectionToRow(cnt);		
		aview->UpdateRow(cnt);
	    UpdateAllViews(NULL);
	}    
}	

void CVardoc::OnActionPermissions()
{
	Btvar	*cv = GetSelectedVar(BTM_RDMODE);
	if  (!cv)
		return;
	CVarperm  dlg;
	dlg.m_user = cv->var_mode.o_user;
	dlg.m_group = cv->var_mode.o_group;
	dlg.m_umode = cv->var_mode.u_flags;
	dlg.m_gmode = cv->var_mode.g_flags;
	dlg.m_omode = cv->var_mode.o_flags;
	dlg.m_writeable = cv->var_mode.mpermitted(BTM_WRMODE);
	if  (dlg.DoModal() == IDOK)  {
		Btvar	newv = *cv;
		if  (dlg.m_umode != cv->var_mode.u_flags  ||
			 dlg.m_gmode != cv->var_mode.g_flags  ||
			 dlg.m_omode != cv->var_mode.o_flags)  {
			newv.var_mode.u_flags = dlg.m_umode;
			newv.var_mode.g_flags = dlg.m_gmode;
			newv.var_mode.o_flags = dlg.m_omode;
			Vars().chmodvar(newv);
		}
		if  (!dlg.m_user.IsEmpty() && dlg.m_user != cv->var_mode.o_user)
			Vars().chogvar(newv, V_CHOWN, dlg.m_user);
		if  (!dlg.m_group.IsEmpty() && dlg.m_group != cv->var_mode.o_group)
			Vars().chogvar(newv, V_CHGRP, dlg.m_group);
	}		
}

void CVardoc::OnSearchSearchbackwards()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_vlastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, FALSE);
}

void CVardoc::OnSearchSearchfor()
{
	CVSearch	sdlg;
	sdlg.m_searchstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_vlastsearch;
	sdlg.m_suser = m_suser;
	sdlg.m_svname = m_svname;
	sdlg.m_svcomment = m_svcomment;
	sdlg.m_svvalue = m_svvalue;
	sdlg.m_sforward = 0;
	sdlg.m_wraparound = m_swraparound;
	if  (sdlg.DoModal() != IDOK)
		return;
	((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_vlastsearch = sdlg.m_searchstr;
	m_suser = sdlg.m_suser;
	m_svname = sdlg.m_svname;
	m_svcomment = sdlg.m_svcomment;
	m_svvalue = sdlg.m_svvalue;
	m_swraparound = sdlg.m_wraparound;
	DoSearch(sdlg.m_searchstr, sdlg.m_sforward == 0);
}

void CVardoc::OnSearchSearchforward()
{
	CString  sstr = ((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_vlastsearch;
	if  (sstr.IsEmpty())  {
		AfxMessageBox(IDP_NOSEARCHSTR, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	DoSearch(sstr, TRUE);		
}

void CVardoc::OnVariablesAssign()
{
	Btvar	*cv = GetSelectedVar(BTM_WRITE);
	if  (!cv)
		return;
	CAssvar dlg;
	dlg.m_varname = cv->var_name;
	dlg.m_varcomment = cv->var_comment;
	dlg.m_value = cv->var_value;
    if  (dlg.DoModal() == IDOK)  {
		Btvar	newv = *cv;
    	newv.var_value = dlg.m_value;
    	Vars().assignvar(newv);
    }
}

void CVardoc::OnVariablesDecrement()
{
	Btvar	*cv = GetArithVar();
	if  (!cv)
		return;
	Btvar	newv = *cv;
	newv.var_value.con_long -= m_constvalue;
	Vars().assignvar(newv);
}

void CVardoc::OnVariablesDivide()
{
	if  (m_constvalue == 0)  {
		AfxMessageBox(IDP_CONSTDIVZERO, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	Btvar	*cv = GetArithVar();
	if  (!cv)
		return;
	Btvar	newv = *cv;
	newv.var_value.con_long /= m_constvalue;
	Vars().assignvar(newv);
}

void CVardoc::OnVariablesIncrement()
{
	Btvar	*cv = GetArithVar();
	if  (!cv)
		return;
	Btvar	newv = *cv;
	newv.var_value.con_long += m_constvalue;
	Vars().assignvar(newv);
}

void CVardoc::OnVariablesMultiply()
{
	Btvar	*cv = GetArithVar();
	if  (!cv)
		return;
	Btvar	newv = *cv;
	newv.var_value.con_long *= m_constvalue;
	Vars().assignvar(newv);
}

void CVardoc::OnVariablesRemainder()
{
	if  (m_constvalue == 0)  {
		AfxMessageBox(IDP_CONSTDIVZERO, MB_OK|MB_ICONEXCLAMATION);
		return;
	}	
	Btvar	*cv = GetArithVar();
	if  (!cv)
		return;
	Btvar	newv = *cv;
	newv.var_value.con_long %= m_constvalue;
	Vars().assignvar(newv);
}

void CVardoc::OnVariablesSetcomment()
{
	Btvar	*cv = GetSelectedVar(BTM_WRITE);
	if  (!cv)
		return;
	CVarcomm  dlg;
	dlg.m_varname = cv->var_name;
	dlg.m_varcomment = cv->var_comment;
    if  (dlg.DoModal() == IDOK)  {
		Btvar	newv = *cv;
		strcpy(newv.var_comment, (const char *) dlg.m_varcomment);
    	Vars().chcommvar(newv);
    }
}

void CVardoc::OnVariablesSetconstant()
{
	Cconstval	dlg;
	dlg.m_constval = m_constvalue;
	if  (dlg.DoModal() == IDOK)
		m_constvalue = dlg.m_constval;
}

void CVardoc::OnWindowWindowoptions()
{
	CWinopts	dlg;
	CBtqwApp	&app = *((CBtqwApp *) AfxGetApp());
	dlg.m_username = m_wrestrict.user;
	dlg.m_groupname = m_wrestrict.group;
	dlg.m_localonly = m_wrestrict.onlyl == restrictdoc::YES? 0: 1; 
	dlg.m_isjobs = FALSE;
	if  (dlg.DoModal() == IDOK)  {
		m_wrestrict.user = dlg.m_username;
		m_wrestrict.group = dlg.m_groupname;
		m_wrestrict.onlyl = dlg.m_localonly > 0? restrictdoc::NO: restrictdoc::YES;
		revisevars(Vars());
	}
}
