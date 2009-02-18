#include "stdafx.h"
#include <string.h>
#include <sys/types.h>
#include "xbwnetwk.h"
#include "ulist.h"
#include "resource.h"

#define	UDP_TRIES	3

extern	sockaddr_in	serv_addr;
extern	const  int	udp_sendrecv(char FAR *, char FAR *, const int, const int);

#define	INITCI	5
#define	INCCI	5

int		checkminmode(const unsigned um, const unsigned gm, const unsigned om)
{
	if  (um & BTM_SHOW  &&  um & (BTM_DELETE|BTM_WRMODE))
		return  1;
	if  (gm & BTM_SHOW  &&  gm & (BTM_DELETE|BTM_WRMODE))
		return  1;
	if  (om & BTM_SHOW  &&  om & (BTM_DELETE|BTM_WRMODE))
		return  1;           
	return  0;
}

#define	CI_INCBUF	10

const  int	getuml(UINT &um, unsigned long &ul, CIList &cil)
{
	ni_jobhdr	enq;
	enq.code = CL_SV_UMLPARS;
	enq.joblength = htons(sizeof(enq));
	ua_umlreply	resp;
	int		ret = udp_sendrecv((char FAR *) &enq, (char FAR *) &resp, sizeof(enq), sizeof(resp));
	if  (ret == 0)  {
		um = ntohs(resp.ua_umask);
		ul = ntohl(resp.ua_ulimit);

		//  Now get command interpreter list,
		//  possibly in several bits.
	
		unsigned  cinum = 0, ci_maxnum = CI_INCBUF;
		Cmdint	*citable = new Cmdint [CI_INCBUF];
		char	abyte[1];
	    abyte[0] = CL_SV_CILIST;
		SOCKET	&sockfd = Locparams.uasocket;
		if  (sendto(sockfd, abyte, sizeof(abyte), 0, (sockaddr FAR *) &serv_addr, sizeof(serv_addr)) < 0)
			return  IDP_GBTU_NOSEND;                                             

		char	buffer[CL_SV_BUFFSIZE];
    	int	 nbytes = 0, pos = 0;

    	for  (;;)  {
    		if  (pos >= nbytes)  {
    			nbytes = recvfrom(sockfd, buffer, sizeof(buffer), 0, (sockaddr FAR *) 0, (int FAR *) 0);
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
			return  IDP_NOCILIST;
		}
		cil.unsetup();
		cil.setup(citable, cinum);
	}
	return  ret;
}

