// unqueue.cpp : implementation file
//

#include "stdafx.h"
#include <ctype.h>
#include <time.h>
#include <direct.h>
#include "netmsg.h"
#include "btqw.h"                   
#include "files.h"

extern	BOOL	dumpjob(CFile &, const CString &, const Btjob &);

// Defined in JDatadoc.cpp

extern	SOCKET	net_feed(const Btjob &);
extern	int		net_if_read(const SOCKET, char FAR *, const int);

void	Btjob::unqueue(const BOOL and_delete)
{
	CFileDialog	jfn(FALSE,
					"xbj",
					NULL,
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,
					"Xibatch job files (*.xbj) | *.xbj | All Files (*.*) | *.* ||");
	
	CBtqwApp	&ma = *((CBtqwApp *)AfxGetApp());
	
	CFileDialog	qfn(FALSE,
					ma.m_sjbat? "bat" : "xbc",
					NULL,
					OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,
					ma.m_sjbat? "DOS-Style Batch files (*.bat) | *.bat | All Files (*.*) | *.* ||":
					"Xi-Batch control files (*.xbc) | *.xbc | All Files (*.*) | *.* ||");
	
	jfn.SetHelpID(IDD_JOBFILE);
	
	if  (jfn.DoModal() != IDOK)
		return;
		
	CString	outfile = jfn.GetPathName();

	//  Obtain file over network
		
	SOCKET	sk = net_feed(*this);
	if  (sk == INVALID_SOCKET)
		return;
		
	CFile	jobfile;

	if  (!jobfile.Open((const char *) outfile, CFile::modeCreate | CFile::modeWrite))  {
		closesocket(sk);
		AfxMessageBox(IDP_CCJOBQFILE, MB_OK|MB_ICONEXCLAMATION);
		return;      
	}	

	//  Slurp up file from network and write to file
		
	char	buffer[256];
	int		nbytes;
	
	TRY {
		if  (ma.m_binunq)  {
			while  ((nbytes = recv(sk, buffer, sizeof(buffer), 0)) > 0)
				jobfile.Write(buffer, UINT(nbytes));
		}
		else  {
			char  obuff[512];
			while  ((nbytes = recv(sk, buffer, sizeof(buffer), 0)) > 0)  {
				int	 ip, obytes;
			    for  (ip = obytes = 0;  ip < nbytes;  ip++)  {
			    	if  (buffer[ip] == '\n')
			    		obuff[obytes++] = '\r';
			    	obuff[obytes++] = buffer[ip];
			    }
			    jobfile.Write(obuff, UINT(obytes));
			}
			obuff[0] = '\x1A';
			jobfile.Write(obuff, 1);
		}
	}
	CATCH(CException, e)
	{
		closesocket(sk);
		AfxMessageBox(IDP_CWJOBQFILE, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	END_CATCH
	// Finished with network
	closesocket(sk);
	jobfile.Close();
	
	qfn.SetHelpID(IDD_BATFILE);
	if  (qfn.DoModal() != IDOK)  {
	zapit:
		TRY {
			CFile::Remove((const char *) outfile);
		}
		CATCH(CException, e)
		{
			AfxMessageBox(IDP_CDJOBQFILE, MB_OK|MB_ICONSTOP);
		}
		END_CATCH
		return;                        
	}

	CString	batfile = qfn.GetPathName();

	TRY {
		CFile bfile(batfile, CFile::modeCreate | CFile::modeWrite);
		if  (!dumpjob(bfile, outfile, *this))
			goto  zapit;
	}
	CATCH(CException, e)
	{
		AfxMessageBox(IDP_CWBATFILE, MB_OK|MB_ICONSTOP);
		goto  zapit;
	}
	END_CATCH

	if  (and_delete)
		Jobs().deletejob(*this);
}
						