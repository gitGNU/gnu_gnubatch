/* ulist.h -- Obtain a list of Unix Users

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

#define	UIDSIZE	11

class	UUGList	{
private:
	int			pos, nbytes;
	unsigned	preflen;
	char		prefix[UIDSIZE+1];
	char		buffer[CL_SV_BUFFSIZE];

protected:
	void	init(const unsigned char, const char FAR *);
	
public:
	~UUGList();
	const  char  FAR  *next(void);
};

class  UUserList : public UUGList  {
public:
	UUserList(const char FAR *prefix = (const char FAR *) 0)
		{
			init(CL_SV_ULIST, prefix);
		}
};
	
class  UGroupList : public UUGList  {
public:
	UGroupList(const char FAR *prefix = (const char FAR *) 0)
		{
			init(CL_SV_GLIST, prefix);
		}
};
