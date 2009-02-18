/* helpprpt.cpp -- get prompt strings

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
#include <ctype.h>
#include <iostream.h>
#include <fstream.h>

extern	ifstream	Cfile;

extern	int	helprdn(void);
char	*help_readl(int &);

char	*helpprmpt(const short current_state)
{
	int	perc = 0;
	streampos	lasttime = Cfile.tellg();

	for  (;;)  {
		int	ch = Cfile.get();
		if  (ch == EOF)  {
			//	Contortions as prompts frequently follow each other
			if  (lasttime <= streampos(0))
				return	(char *) 0;
			Cfile.clear();
			Cfile.seekg(0);
			lasttime = streampos(0);
			continue;
		}

		//	If  line doesnt start with a digit ignore it

		if  (!isdigit(ch)  &&  ch != '-')  {
	skipn:
			while  (ch != '\n'  &&  ch != EOF)
				ch = Cfile.get();
			continue;
		}

		//	Read leading state number
		//	If not current state forget it

		Cfile.putback(ch);
		if  (helprdn() != current_state)  {
			do  ch = Cfile.get();
			while  (ch != '\n' && ch != EOF);
			continue;
		}
		
		ch = Cfile.get();

		//	If its not a 'p' ignore it.

		if  (toupper(ch) != 'P')
			goto  skipn;

		if  ((ch = Cfile.get()) != ':')
			goto  skipn;

		return	help_readl(perc);
	}
}
