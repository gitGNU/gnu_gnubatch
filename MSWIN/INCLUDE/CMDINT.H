/* cmdint.h -- Command interpreter structure

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

struct  Cmdint	{
	unsigned  short	 ci_ll;		//  Default load level
	unsigned  char	 ci_nice; 	//  Nice value
	unsigned  char	 ci_flags;	//  Nasty value
	char	ci_name[CI_MAXNAME+1]; 	//  Name (e.g. shell)
	char	ci_path[CI_MAXFPATH+1]; //  Path name
	char	ci_args[CI_MAXARGS+1]; 	//  Arg list
};

#define	CI_STDSHELL	0		// Standard shell

class  CIList	{
private:
	unsigned  cilnum, where;
	Cmdint	  *cilist;
public:
	CIList()	{ cilnum = where = 0; cilist = NULL; }
	~CIList()	{ if (cilist) { delete [] cilist; cilist = NULL; } }
	void		init_list() { where = 0; }
	unsigned	number()	{ return  cilnum; }
	Cmdint		*next()		{ return  where >= cilnum? NULL: &cilist[where++];	}
	void		setup(Cmdint *l, unsigned num) { cilnum = num; cilist = l; }
	void		unsetup()	{ if (cilist) { delete [] cilist; cilist = NULL; } cilnum = where = 0; }
	Cmdint		*def_cmdint()	{  return  cilnum? &cilist[CI_STDSHELL]: NULL;	}
	unsigned  short	cmdint_ll(const char *name) {
		for  (unsigned  cnt = 0;  cnt < cilnum;  cnt++)
			if  (strcmp(name, cilist[cnt].ci_name) == 0)
				return  cilist[cnt].ci_ll;
		return  cilist[CI_STDSHELL].ci_ll;
	}
};
