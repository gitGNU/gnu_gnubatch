// jdatadoc.cpp : implementation file
//

#include "stdafx.h"
#include "netmsg.h"
#include "xbwnetwk.h"
#include "btqw.h"                   
#include "jdatadoc.h"
#include <ctype.h>
                                                           
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJdatadoc

IMPLEMENT_DYNCREATE(CJdatadoc, CDocument)

SOCKET	net_feed(const Btjob &job)
{
	SOCKET	sock;
	sockaddr_in  sin;

	if  ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)  {
		AfxMessageBox(IDP_CCFEEDSOCKET, MB_OK|MB_ICONEXCLAMATION);
		return  INVALID_SOCKET;                                   
	}	

	//	Set up bits and pieces.

	sin.sin_family = AF_INET;
	sin.sin_port = Locparams.vportnum;
	memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));
	sin.sin_addr.s_addr = job.hostid;

	if  (connect(sock, (sockaddr FAR *) &sin, sizeof(sin)) != 0)  {
		closesocket(sock);
		AfxMessageBox(IDP_CCOFEEDSOCKET, MB_OK|MB_ICONEXCLAMATION);
		return  INVALID_SOCKET;
	}

	//	Send out initial packet saying what we want.

	feeder	fd(FEED_JOB, htonl(job.bj_job));
	if  (send(sock, (char FAR *) &fd, sizeof(fd), 0) != sizeof(fd))	{
		closesocket(sock);
		AfxMessageBox(IDP_CSFEEDSOCKET, MB_OK|MB_ICONEXCLAMATION);
		return  INVALID_SOCKET;
	}
	return  sock;
}
                              
//  Read and join up boundaries
                              
int	net_if_read(const SOCKET  sk, char FAR *buffer, const int length)
{
	int  nbytes, toread = length;
                           
	while  ((nbytes = recv(sk, buffer, toread, 0)) > 0)  {
		if  (nbytes == toread)
			return	length;
		buffer += nbytes;
		toread -= nbytes;
	}
	return	nbytes;
}	

static	UINT	charsize(char ch, UINT  chcount)
{
	if  (ch == '\t')
		return  4 - (chcount & 3);
	if  (ch == '\r')
		return  0;
	if  (ch & 0x80)  {
		ch &= ~0x80;
		return  isprint(ch)? 3: 4;
	}
	else
		return  isprint(ch)? 1: 2;
}
                                      
CJdatadoc::CJdatadoc()
{
	charwidth = charheight = 0;
	m_invalid = FALSE;
}

CJdatadoc::~CJdatadoc()
{
	if  (!tname.IsEmpty())  {
		jobfile.Close();
		remove(tname);
	}	
}
   
int		CJdatadoc::loaddoc()
{   
	SOCKET	sk = net_feed(jcopy);
	if  (sk == INVALID_SOCKET)
		return  0;

	char	title[80];
	if  (!jcopy.bj_title.IsEmpty())
		wsprintf(title, "Job %s:%ld:%s", (const char FAR *) look_host(jcopy.hostid), jcopy.bj_job, (const char FAR *) jcopy.bj_title);
	else
		wsprintf(title, "Job %s:%ld (untitled)", (const char FAR *) look_host(jcopy.hostid), jcopy.bj_job);
	SetTitle(title);

	char	*tn = _tempnam("C:\\XIBATCH", "JDAT");
	if  (!tn)  {
		closesocket(sk);
		AfxMessageBox(IDP_CCTEMPFILE, MB_OK|MB_ICONEXCLAMATION);
		return  0;      
	}	
	tname = tn;
	if  (!jobfile.Open(tname, CFile::modeCreate | CFile::modeReadWrite))  {
		closesocket(sk);
		AfxMessageBox(IDP_CCTEMPFILE, MB_OK|MB_ICONEXCLAMATION);
		return  0;      
	}	

	//  Slurp up file from network and write to file
		
	char	buffer[256];
	int		nbytes;
	
	TRY {
		while  ((nbytes = recv(sk, buffer, sizeof(buffer), 0)) > 0)
			jobfile.Write(buffer, UINT(nbytes));
	}
	CATCH(CException, e)
	{
		closesocket(sk);
		AfxMessageBox(IDP_CWTEMPFILE, MB_OK|MB_ICONEXCLAMATION);
		return  0;
	}
	END_CATCH
	// Finished with network
	closesocket(sk);
	// Work out length and width
	unsigned	cwidth = 0;
	jobfile.SeekToBegin();
	while  ((nbytes = jobfile.Read(buffer, sizeof(buffer))) > 0)
		for  (int  nc = 0;  nc < nbytes;  nc++)  {
			int	 ch = buffer[nc];
			if  (ch == '\n')  {
		   		charheight++;
		   		if  (charwidth < cwidth)
					charwidth = cwidth;
				cwidth = 0;
		    }
		    else
		    	cwidth += charsize(ch, cwidth);
		}

	if  (charheight == 0)
		charheight = 1;

	if  (charwidth == 0)
		charwidth = 80;
		          
	return  1;
}

//  Locate the given row

long	CJdatadoc::Locrow(const unsigned row)
{
	if  (row < cline)  {
		jobfile.Seek(0L, CFile::begin);
		cline = 0;
		offset = 0;
	}
	else
		jobfile.Seek(offset, CFile::begin);

	while  (cline < row)  {
		char  buffer[100];
		UINT  nbytes;
		nbytes = jobfile.Read(buffer, sizeof(buffer));
		if  (nbytes == 0)
			return  -1L;
		for  (UINT nc = 0;  nc < nbytes;  nc++)  {
			offset++;
			if  (buffer[nc] == '\n')  {
				cline++;
				if  (cline >= row)
					break;
			}
		}
	}
	return  offset;
}
			
UINT	CJdatadoc::RowLength(const long splace)
{
	jobfile.Seek(splace, CFile::begin);
	UINT	chcount = 0;
	for  (;;)  {
		char	buffer[100];
		int  nbytes = jobfile.Read(buffer, sizeof(buffer));
		for  (int  nc = 0;  nc < nbytes;  nc++)
			if  (buffer[nc] == '\n')
				return  chcount + 1;
			else
				chcount += charsize(buffer[nc], chcount);
	}
}	

char	*CJdatadoc::FindRow(const int row)
{
	long	splace = Locrow(row);
	if  (splace < 0)
		return  NULL;	
	UINT	chcount = RowLength(splace);
	char  *result = new char [chcount];
	if  (!result)
		return  NULL;
	jobfile.Seek(splace, CFile::begin);
	UINT  pos = 0;
	for  (;;)  {         
		char  buffer[100];
		int  nbytes = jobfile.Read(buffer, sizeof(buffer));
		for  (int  nc = 0;  nc < nbytes;  nc++)
			if  (buffer[nc] == '\n')  {
				result[pos] = '\0';
				return  result;
			}
			else  if  (buffer[nc] != '\r')
				result[pos++] = buffer[nc];					               
	}	
}

char	*CJdatadoc::GetRow(const int row, char *result)
{
	long	splace = Locrow(row);
	if  (splace < 0)
		return  NULL;	
	jobfile.Seek(splace, CFile::begin);
	UINT  pos = 0;
	for  (;;)  {
		char  buffer[100];
		int  nbytes = jobfile.Read(buffer, sizeof(buffer));
		if  (nbytes <= 0)  {
			result[pos] = '\0';
			return  result;
		}
		for  (int  nc = 0;  nc < nbytes;  nc++)  {
			int  ch = buffer[nc];			
			if  (ch == '\n')  {
				result[pos] = '\0';
				return  result;
			}
			else  if  (ch == '\t')
				do  result[pos] = ' ';
				while  (++pos & 3);
			else  if  (ch == '\r')
				continue;
			else  {
				if  (ch & 0x80)  {
					ch &= 0x7f;
					result[pos++] = 'M';
					result[pos++] = '-';
				}
				if  (!isprint(ch))  {
					result[pos++] = '^';
					if  (ch < ' ')
						result[pos++] = char(ch + '@');
					else
						result[pos++] = '?';
				}
				else
					result[pos++] = ch;
			}
		}	
	}			
}					

BEGIN_MESSAGE_MAP(CJdatadoc, CDocument)
	//{{AFX_MSG_MAP(CJdatadoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

