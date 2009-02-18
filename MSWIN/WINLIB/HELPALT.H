/* helpalt.h -- Alternative list

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

class  Helpalt	{
private:
	short	upperonly;		//  Set as upper case
	short	statecode;		//  State code of alternatives
	short	numalt;			//  Number of alternatives
	short	def_alt;		//  Default (or -1 if none)
	short	*alt_nums;		//  Alternative numbers
	char	**list;			//  Strings, + null term
	void	helprdalt();
public:
	Helpalt(const short code, const short upp = 0) :
		statecode(code), upperonly(upp) { numalt = -1; }
	~Helpalt();
	int		number();
	int		altlen();
	const  char	*disp_alt(const int);
	const  short look_alt(const char *);
	const  short gdef_alt() { if (numalt < 0) helprdalt(); return def_alt; }
};
