/* xbwnetwk.h -- network structures.

   Copyright 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

class	local_params	{
 public:
	SOCKET	listsock;		// Socket to listen for connections on
	SOCKET	probesock;		// Datagram socket to probe machines with
	SOCKET	uasocket;
		
// The following are all htons-ified (which means byte-swapped on DOS)
// Port numbers used for above

	unsigned  short	lportnum;	// Re-used for number of hosts
	unsigned  short	vportnum;
	unsigned  short	pportnum;
	unsigned  short uaportnum;	// User enquiry port number

	short	Netsync_req;	// Tells us if we need to sync something
	unsigned  short	servtimeout;	//  Timeout for server
	time_t	 tlastop;		// Time of last op on uasocket
	netid_t  myhostid;		// My internet address htonl-ified
	netid_t  servid;		// Server internet address htnol-ified

	//  Initialise all these to invalid on startup.

	local_params() {
		listsock = probesock = uasocket = INVALID_SOCKET;
	}
};

extern	local_params	Locparams;

#ifdef	BTRSETW
inline	unsigned  num_hosts()	{	return  Locparams.lportnum;	}
#endif

class	remote	{
private:
	remote	*hh_next;		// Hash table of host names
	remote	*ha_next;		// Hash table of alias names
	remote	*hn_next;		// Hash table of netids
	char	*h_name;		// Actual host name 
	char	*h_alias;		// Alias for within Xi-Batch
public:
	const netid_t  hostid;	// hton-ified host id
	SOCKET	sockfd;			// Socket fd to talk to it 
	unsigned char	is_sync;// sync flags 
#define	NSYNC_NONE	0		// Not done yet 
#define	NSYNC_REQ	1		// Requested but not complete 
#define	NSYNC_OK	2		// Completed 
	unsigned  char	ht_flags;	// Host-type flags 
#define	HT_ISCLIENT	(1 << 0)	// Set to indicate "I" am client 
#define	HT_PROBEFIRST	(1 << 1)	// Probe connection first 
#define	HT_MANUAL	(1 << 2)		// Manual connection only
#define	HT_DOS		(1 << 3)		// DOS client (only used at other end)
#define	HT_SERVER	(1 << 4)		// Unix server
#define	HT_NAMEALIAS	(1 << 5)	// Named by alias
	unsigned  short	ht_timeout;		// Timeout value (seconds)
	time_t	lastwrite;				// When last done
#ifdef	BTQW
	unsigned  short	bytesleft;		// Bytes left to read in current message
	unsigned  short byteoffset;		// Where we have got to
	char			buffer[sizeof(jobnetmsg)];	//  Believed to be the longest thing
#endif

// Access functions

	const  char	*hostname() const	{  return  h_name;	}
	const  char *aliasname() const	{  return  h_alias;	}
	const  char	*namefor()	{ return  h_alias?  h_alias: h_name;	}

	remote(const netid_t,const char FAR *,const char FAR * = NULL,const unsigned char = 0, const unsigned short = NETTICKLE);
	~remote()	{ if  (h_name) delete h_name; if  (h_alias) delete h_alias;	}
	void	addhost();
	void	delhost();

	friend void			savehostfile();    
    friend BOOL			clashcheck(const char FAR *);
    friend BOOL			clashcheck(const netid_t);                                                     
    friend remote		*get_nth(const unsigned);                                                     
	friend remote		*find_host(const netid_t);    
	friend remote		*find_host(const SOCKET);    
	friend const char 	*look_host(const netid_t);
	friend netid_t		look_hname(const char *);
#ifdef	BTQW
	void	probe_attach();
	void	conn_attach();
	void	rattach();
	friend	void	attach_hosts();
#endif
};

#define	HASHMOD	17				// Not very big prime number
#define INC_REMOTES 4

//  Lists of remotes we know and are probing for.  Do this
//  as a nice class.

class	remote_queue	{
 private:
	unsigned  max, lookingat;
	remote	  **list;
 public:
	unsigned	alloc(remote *);
	remote		*find(const netid_t);
#ifdef	BTQW
	remote		*find(const SOCKET);
#endif
	void		setfirst()	{	lookingat = 0;	}
	void		setlast()	{	lookingat = max;	}
	remote		*next()
	{   
		remote  *result = (remote *) 0;
		while  (lookingat < max  &&  !result)
			result = list[lookingat++];
		return  result;			
	}
	remote		*prev()
	{
		remote  *result = (remote *) 0;
		while  (lookingat != 0  &&  !result)
			result = list[--lookingat];
		return  result;
	}
	void	free(const remote * const);
	unsigned	index(const remote * const);
	remote	*operator [] (const unsigned);
};

extern	remote_queue	current_q, pending_q;

extern	UINT		loadhostfile();
extern	UINT		winsockstart();
extern	void		winsockend();
extern	UINT		initenqsocket(const netid_t);
extern	void		refreshconn();

