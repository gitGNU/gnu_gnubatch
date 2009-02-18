/* helpargs.cc -- argument handling

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
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include "helpargs.h"
#include "resource.h"

extern	int			helprdn(void);

inline	char	*stracpy(const char *orig)
{
	char	*result = new char [strlen(orig) + 1];
	strcpy(result, orig);
	return  result;
}

Helparg::~Helparg()
{
	Helpargkey  *fp, *np;
	
	for  (np = mult_chain;  np;  np = fp)  {
		fp = np->next;
		delete [] np->chars;
		delete np;
	}
}
	
Helparg	*helpargs(const Argdefault *defaults, const int mins, const int maxs)
{
	Helparg  *result = new Helparg[ARG_ENDV - ARG_STARTV + 1];
	if  (!result)
		return  NULL;

	//	We only use defaults if we have to.

	Cfile.clear();
	Cfile.seekg(0);          
	
	int	someset = 0, errs = 0;

	for  (;;)  {
		int  ch = Cfile.get();
		if  (ch != HELPLETTER  &&  ch != (HELPLETTER - 'A' + 'a'))  {
		skipn:
			while  (ch != '\n'  &&  ch != EOF)
				ch = Cfile.get();
			if  (ch == EOF)
				break;
			continue;
		}

		//	Had initial 'P' (or 'p')
		//	Expecting numeric to follow.

		ch = Cfile.get();
		if  (!isdigit(ch)  &&  ch != '-')
			goto  skipn;

		//	Put back character and read the number

		Cfile.putback(char(ch));
		int  staten = helprdn();
		if  (staten < mins  ||  staten > maxs)
			goto  skipn;

		//	If we don't find a ":", discard it

		if  ((ch = Cfile.get()) != ':')
			goto  skipn;

		//	If it could be part of a name, then explore that

		do	{

			ch = Cfile.get();

 			//	Must be valid starting char

			if  (ch < ARG_STARTV || ch > ARG_ENDV || ch == ',')
				goto  skipn;

			if  (isalnum(ch))  {
				int	pos = 0, startchar = ch;
				char	inbuf[MAXARGNAME];

				do  {
					if  (pos < MAXARGNAME - 1)
						inbuf[pos++] = tolower(ch);
					ch = Cfile.get();
				}  while  (isalnum(ch) || ch == '_' || ch == '-');

				//	Check valid termination, ignore rest of line if not.

				if  (!isspace(ch) && ch != ',' && ch != '#')
					goto  skipn;

				inbuf[pos] = '\0';

				//	If only one char, go on to single char case

				if  (pos <= 1)  {
					Cfile.putback(ch);
					ch = startchar;
					goto  singchar;
				}

				//	Search result chain for keyword ignore duplicates but complain
				//	about different state codes.

				Helpargkey  **rpp = &result[startchar - ARG_STARTV].mult_chain;
				Helpargkey  *rp;
				for  (;  rp = *rpp;  rpp = &rp->next)  {
					if  (strcmp(rp->chars, inbuf) == 0)  {
						if  (staten != rp->value)
							errs++;
						goto  skip_alloc;
					}
				}

				if  (!(rp = new Helpargkey))  {
					errs++;
					goto  skip_alloc;
				}

				//	Put on end of chain

				rp->chars = stracpy(inbuf);
				rp->value = staten;
				rp->next = NULL;

				*rpp = rp;
				someset++;
			}
			else  {
				//	Manoeuvre to escape a ,

				if  (ch == '\\')  {
					ch = Cfile.get();
					if  (ch < ARG_STARTV || ch > ARG_ENDV)
						goto  skipn;
				}
			singchar:
				int  whatv = result[ch - ARG_STARTV].value;

				if  (whatv != 0  &&  whatv != staten)
					errs++;
				result[ch - ARG_STARTV].value = staten;
				ch = Cfile.get();
				someset++;
			}

		skip_alloc:
			;
		}  while  (ch == ',');

		//	Skip white space

		while  (ch == ' '  ||  ch == '\t')
			ch = Cfile.get();

		//	Allow comments and skip

		if  (ch == '#')  {
			do  ch = Cfile.get();
			while  (ch != '\n'  &&  ch != EOF);
		}
	}

	//	Return if we had errors.
	//	We should really deallocate everything but we are about
	//	to exit so why bother

	if  (errs > 0)  {
		delete [] result;
		AfxMessageBox(IDP_ARGDEFERRS, MB_OK|MB_ICONSTOP);
		return  NULL;
	}

	//	If nothing got set use the default vector

	if  (someset <= 0)  {
		const  Argdefault  *defs = defaults;
		while  (defs->letter)  {
			result[defs->letter - ARG_STARTV].value = defs->value;
			defs++;
		}
	}
 	return  result;
}

