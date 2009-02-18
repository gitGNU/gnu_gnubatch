#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#include "xbwnetwk.h"
#include "ulist.h"
#include "resource.h"
#include "listvar.h"
#include "btrw.h"

extern	sockaddr_in	serv_addr;

listvar::listvar(const unsigned mode)
{
	ua_venq	req;
	req.uav_code = CL_SV_VLIST;
	req.uav_perm = htons(mode);
	time(&Locparams.tlastop);
	if  (sendto(Locparams.uasocket, (char FAR *) &req, sizeof(req), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		nbytes = pos = 0;
	else  {		
		pos = 0;
		nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
	}	
}

char	*listvar::next()
{
	if  (Locparams.uasocket == INVALID_SOCKET  ||  nbytes <= 0)
		return  NULL;
	if  (pos >= nbytes)  {
		nbytes = recvfrom(Locparams.uasocket, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
		pos = 0;
		if  (nbytes <= 0)
			return  NULL;
	}                                                   
	char  *res = &buffer[pos];
	size_t	len = strlen(res);
	if  (len == 0)
		return  NULL;
	pos += len + 1;
	return  res;
}	

//  Slurp up parameters from host:
//  	Umask and Ulimit,	command interpreters,  environment table

#define	CI_INCBUF	10

BOOL	gethostparams()
{
	CBtrwApp  &ma = *((CBtrwApp *)AfxGetApp());
	char	abyte[1];
	
	//  Get umask and ulimit - piece of cake
	
	time(&Locparams.tlastop);
	abyte[0] = CL_SV_UMLPARS;
	if  (sendto(Locparams.uasocket, abyte, sizeof(abyte), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		return  FALSE;
	else  {
		ua_umlreply	rep;
		(void) recvfrom(Locparams.uasocket, (char *) &rep, sizeof(rep), 0, (sockaddr FAR *) 0, (int FAR *) 0);
	    ma.m_umask = ntohs(rep.ua_umask);
	    ma.m_ulimit = ntohl(rep.ua_ulimit);
	}
	
	//  Now get command interpreter list, possibly in several bits.
	
	unsigned  cinum = 0, ci_maxnum = CI_INCBUF;
	Cmdint	*citable = new Cmdint [CI_INCBUF];
	
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
			if  (nbytes < sizeof(Cmdint) || ((Cmdint *) &buffer[0])->ci_ll == 0)
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
	ma.m_cilist.setup(citable, cinum);
		
	//  Now for the environment list
	
	abyte[0] = CL_SV_ELIST;
	if  (sendto(Locparams.uasocket, abyte, sizeof(abyte), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
		return  FALSE;                                             
	
	//  Individual environment variables can be larger than the buffer, so we allocate a really big
	//  buffer to build them in. We still use the tiny buffer for our UDP transactions as this
	//  can be a limiting factor.
	
	char	*bigbuff = new char [1024];
	unsigned	bigsize = 1024;
	
	pos = nbytes = 0;
	for  (;;)  {
		if  (pos >= nbytes)  {
    		nbytes = recvfrom(Locparams.uasocket, bigbuff, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
    		if  (nbytes <= 0)
    			return  FALSE;
			pos = 0;
			if  (bigbuff[0] == '\0')
				break;
			while  (bigbuff[nbytes-1] != '\0')  {
				if  (nbytes + sizeof(buffer) >= bigsize)  {
					char	*oldbuff = bigbuff;
					bigbuff = new char [bigsize*2];
					memcpy(bigbuff, oldbuff, nbytes);
					bigsize *= 2;
					delete [] oldbuff;
				}
				int  newbytes = recvfrom(Locparams.uasocket, &bigbuff[nbytes], sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
				if  (newbytes <= 0)
					return  FALSE;
				nbytes += newbytes;
			}					
		}
		char	*thing = &bigbuff[pos];
		ma.m_envtable.add_unixenv(thing);
		pos += strlen(thing) + 1;
	}
	
	return  TRUE;
}		

long	xmit(char *msg, const int lng)
{
	client_if	reply;

	time(&Locparams.tlastop);

	if  (sendto(Locparams.uasocket, msg, lng, 0, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
		int	errnum = WSAGetLastError();
		AfxMessageBox(IDP_CANNOTSENDJOB, MB_OK|MB_ICONSTOP);
		return	0;
	}
	if  (recvfrom(Locparams.uasocket, (char *) &reply, sizeof(reply), 0, (sockaddr *) 0, (int *) 0) < 0)  {
		AfxMessageBox(IDP_CANNOTRECVJOB, MB_OK|MB_ICONSTOP);
		return	0;
	}
	if  (reply.code != SV_CL_ACK)  {
		UINT	mcode;
		if  (reply.code == XBNR_ERR)  {
			switch  (ntohl(reply.param))  {
			default:
				mcode = IDP_UNDEFSCHEDERR;
				break;
			case  J_VNEXIST:
				mcode = IDP_SCHERR_VNEXIST;
				break;
			case  J_NOPERM:
				mcode = IDP_SCHERR_JNOPERM;
				break;
			case  J_VNOPERM:
				mcode = IDP_SCHERR_VNOPERM;
				break;
			case  J_NOPRIV:
				mcode = IDP_SCHERR_NOPRIV;
				break;
			case  J_SYSVAR:
				mcode = IDP_SCHERR_SYSVAR;
				break;
			case  J_SYSVTYPE:
				mcode = IDP_SCHERR_SYSVTYPE;
				break;
			case  J_FULLUP:
				mcode = IDP_SCHERR_FULLUP;
				break;
			case  J_REMVINLOCJ:
				mcode = IDP_SCHERR_REMVINLOCJ;
				break;
			case  J_LOCVINEXPJ:
				mcode = IDP_SCHERR_LOCVINEXPJ;
				break;
			case  J_MINPRIV:
				mcode = IDP_SCHERR_MINPRIV;
				break;
			}
		}
		else
			mcode = IDP_CANNOTRECVJOB + reply.code;
		AfxMessageBox(mcode, MB_OK|MB_ICONSTOP);
		return	0;
	}
	long  retc = ntohl(reply.param);
	return  retc == 0? -1: retc;
}
