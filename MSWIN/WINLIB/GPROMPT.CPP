/* gprompt.cpp -- Cut down version of gprompt for windoze

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
#include <string.h>
#include "gprompt.h"

char	*helpprmpt(const short);

inline	char	*stracpy(const char *orig)
{
	char	*result = new char [strlen(orig) + 1];
	strcpy(result, orig);
	return  result;
}

gprompt::gprompt(const int statecode)
{
	statenum = short(statecode);
	string = (char *) 0;
}

gprompt::~gprompt()
{
	if  (string)
		delete [] string;
}

gprompt::operator char *()
{
	if  (!string  &&  !(string = helpprmpt(statenum)))  {
		char	thing[50];
		wsprintf(thing, "Missing prompt number %d", statenum);
		string = stracpy(thing);
	}
	return  string;
}
