/* helprdalt.cpp -- Read alternative list from help file

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
#include <memory.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include "helpalt.h"

extern	ifstream	Cfile;

extern	int		helprdn(void);
extern	char	*help_readl(int &);

/*
 *	Read a list of alternatives from the configuration file.
 *	These look like:
 *
 *		<state number>A[D]<alt number>:Text
 *
 *	The order they come in is the order they will be put out.
 *	The D signifies that the specified alternative is the default.
 */

#define	INITALTS	10
#define	INCALTS		3

void	Helpalt::helprdalt()
{
	int	 lastnum = -1, perc = 0;
	
	numalt = 0;
	def_alt = -1;
	alt_nums = new short [INITALTS];
	list = new char * [INITALTS];
	memset((void *) list, '\0', INITALTS * sizeof(char *));
	int  maxalts = INITALTS;
	Cfile.clear();
	Cfile.seekg(0);

	for  (;;)  {
		int  ch = Cfile.get();
		if  (ch == EOF)
			break;

		//	If  line doesn't start with a number ignore it

		if  (!isdigit(ch)  &&  ch != '-')  {
skipn:			while  (ch != '\n'  &&  ch != EOF)
					ch = Cfile.get();
			continue;
		}

		//	Read leading state number
		//	If not current state forget it
		
		Cfile.putback(ch);
		if  (helprdn() != statecode)  {
skipr:			do  ch = Cfile.get();
			while  (ch != '\n' && ch != EOF);
			continue;
		}
		ch = Cfile.get();

		//	Is it alternative spec - if not forget it

		if  (toupper(ch) != 'A')
			goto  skipr;

		if  (numalt >= maxalts)  {
			int  newmaxalts = maxalts + INCALTS;
			short  *rans = new short [newmaxalts];
			char  **rlist = new char *[newmaxalts];
			memset((void *) rlist, '\0', newmaxalts*sizeof(char *));
			memcpy((void *) rans, (void *) alt_nums, maxalts * sizeof(short));
			memcpy((void *) rlist, (void *) list, maxalts * sizeof(char *));
			delete [] alt_nums;
			for  (int  cnt = 0;  cnt < maxalts;  cnt++)
				delete [] list[cnt];
			delete [] list;
			alt_nums = rans;
			list = rlist;
			maxalts = newmaxalts;
		}

		int  nres = numalt;
		ch = Cfile.get();

		//	Mark as default?
		
		if  (toupper(ch) == 'D')  {
			def_alt = nres;
			ch = Cfile.get();
		}

		//	If not followed by number use last number + 1
		
		Cfile.putback(ch);
		
		if  (!isdigit(ch)  &&  ch != '-')
			lastnum++;
		else
			lastnum = helprdn();

		alt_nums[nres] = lastnum;

		if  ((ch = Cfile.get()) != ':')  {
			def_alt = -1;		//  might have been set
			goto  skipn;
		}

		//	Read actual line.
		//	Ignore percent flags.
		
		list[nres] = help_readl(perc);
		if  (upperonly)  {
			for  (char  *cp = list[nres];  *cp;  cp++)
				*cp = toupper(*cp);
		}
		numalt++;
	}
}

Helpalt::~Helpalt()
{
	if  (numalt > 0)  {
		delete [] alt_nums;
		for  (int  cnt = 0;  cnt < numalt;  cnt++)
			delete [] list[cnt];
		delete [] list;
		numalt = 0;
	}
}

int		Helpalt::number()
{
	if  (numalt < 0)
		helprdalt();
	return  numalt;
}
	
int		Helpalt::altlen()
{
	if  (numalt < 0)
		helprdalt();
	for  (int  cnt = 0, maxl = 0;  cnt < numalt;  cnt++)  {
		int	 lng;
		if  ((lng = strlen(list[cnt])) > maxl)
			maxl = lng;
	}
	return  maxl;
}

const  char	*Helpalt::disp_alt(const int which)
{
	if  (numalt < 0)
		helprdalt();
	for  (int  cnt = 0;  cnt < numalt;  cnt++)
		if  (alt_nums[cnt] == which)
			return  list[cnt];		
	return  "???";
}
        
const  short  Helpalt::look_alt(const char *str)
{
	if  (numalt < 0)
		helprdalt();
	if  (upperonly)  {
		for  (int  cnt = 0;  cnt < numalt;  cnt++)
			if  (_stricmp(str, list[cnt]) == 0)
				return  alt_nums[cnt];
	}
	else
		for  (int  cnt = 0;  cnt < numalt;  cnt++)
			if  (strcmp(str, list[cnt]) == 0)
				return  alt_nums[cnt];
	return  def_alt;
}
