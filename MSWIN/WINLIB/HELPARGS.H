/* helpargs.h -- For building up tables of arguments

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

#define	MAX_ANY_ARGS	50
#define	MAXARGNAME	30

#define	HELPLETTER	'A'			// Look for this letter in file
#define	ARG_STARTV	(' ' + 1)	// Possible argument starting char
#define	ARG_ENDV	126			// Possible argument ending char

//	This structure represents a chain of argument names starting from
//	keywords of the same letter (only identifier characters allowed).

struct	Helpargkey	{
	Helpargkey	*next;		// Next in chain
	char		*chars;		// Chars of keyword
	int			value;		// State value
};

//	This structure gives an element of the vector to look up to find the
//	arg value for a one-letter argument, or the start of a chain for
//	multi-letter ones.

struct	Helparg	{
	Helpargkey		*mult_chain;	// Chain for multi-keywords
	int				value;			// State value
	Helparg()
	{
		mult_chain = NULL;
		value = 0;
	}
	~Helparg();
};

//	Array for passing defaults in - terminated by null letter

struct	Argdefault	{
	char	letter;		// Letter in question
	int		value;		// Default value
};

struct	optv	{
	int		isplus;
	union  {
		int		letter;
		char	*string;
	};
};

extern	optv  FAR	optvec[];
extern	ifstream	Cfile;
extern	Helparg		*helpargs(const Argdefault *, const int, const int);
extern	void		makeoptvec(const Helparg *, const int, const int);
