/* helprdl.cpp -- read line from help file

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

#include "stdafx.h"
#include <iostream.h>
#include <fstream.h>
	
extern	ifstream	Cfile;

char	*help_readl(int &percentflag)
{
	streampos	w = Cfile.tellg();
	unsigned	l = 1;		//  Final null
	int	ch;

	while  ((ch = Cfile.get()) != '\n'  &&  ch != '\r'  &&  ch != EOF)
		l++;
	if  (ch == EOF)  {
		Cfile.clear();
		char	*result = new char[1];
		result[0] = '\0';
		return  result;
	}
	Cfile.seekg(w);
	char	*result = new char[l];
	char  *rp = result;

	while  ((ch = Cfile.get()) != '\n'  &&  ch != '\r'  &&  ch != EOF)
		if  ((*rp++ = ch) == '%')
			percentflag |= 1;
	*rp = '\0';
	return  result;
}
