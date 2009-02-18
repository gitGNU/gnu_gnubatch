/* opencfle.cpp -- open help message file

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
#include <iostream.h>
#include <fstream.h>
#include <direct.h>
#include "files.h"

extern	ifstream	Cfile;

int  open_cfile(const char *deft_file)
{
	char	namebuf[_MAX_PATH];
    _getdcwd(0, namebuf, _MAX_PATH);
    strcat(namebuf, "\\");
    strcat(namebuf, deft_file);
	Cfile.open(namebuf, ios::nocreate | ios::binary);
	return  Cfile.good();
}
