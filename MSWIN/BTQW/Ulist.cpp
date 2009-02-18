#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#include "netmsg.h"
#include "netwmsg.h"
#include "xbwnetwk.h"
#include "ulist.h"
#include "mainfrm.h"
#include "btqw.h"

extern	sockaddr_in	serv_addr;

//  Slurp up parameters from host:
//  Command interpreters

#define	CI_INCBUF	10

BOOL	gethostparams()
{
	unsigned  cinum = 0, ci_maxnum = CI_INCBUF;
	Cmdint	*citable = new Cmdint [CI_INCBUF];
	
	char	abyte[1];
	abyte[0] = CL_SV_CILIST;
	time(&Locparams.tlastop);

	if  (sendto(Locparams.uasocket, abyte, sizeof(abyte), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		return  FALSE;                                             
	
	char	buffer[CL_SV_BUFFSIZE];
    int	 nbytes = 0, pos = 0;
    for  (;;)  {
    	if  (pos >= nbytes)  {
    		nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
			pos = 0;
			if  (nbytes < sizeof(Cmdint) || ((Cmdint *)&buffer[0])->ci_ll == 0)
				break;
		}
		if  (cinum >= ci_maxnum)  {
			Cmdint	*oldtab = citable;
			unsigned  oldnum = ci_maxnum;
			ci_maxnum += CI_INCBUF;
			citable = new Cmdint [ci_maxnum];
			memcpy((char *) citable, oldtab, oldnum * sizeof(Cmdint));
			delete [] oldtab;
		}
		Cmdint	&res = citable[cinum++];
		Cmdint	*rbuf = (Cmdint *) &buffer[pos];
		res.ci_ll = ntohs(rbuf->ci_ll);
		res.ci_nice = rbuf->ci_nice;
		res.ci_flags = rbuf->ci_flags;
		strcpy(res.ci_name, rbuf->ci_name);
		strcpy(res.ci_path, rbuf->ci_path);
		strcpy(res.ci_args, rbuf->ci_args);
    	pos += sizeof(Cmdint);
    }

	if  (cinum == 0)  {
		delete [] citable;
		return  FALSE;    
	}
	((CBtqwApp *)AfxGetApp())->m_cilist.setup(citable, cinum);
	return  TRUE;
}		
	