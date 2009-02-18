/* conthelp.cpp -- Generate context help

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

BOOL	do_contexthelp(const UINT cid, const DWORD *hlist)
{
	for  (int cnt = 0;  hlist[cnt] != 0;  cnt += 2)
	if  (hlist[cnt] == DWORD(cid))  {
		AfxGetApp()->WinHelp(hlist[cnt+1], HELP_CONTEXTPOPUP);
		return  TRUE;
	}
	return  FALSE;
}


