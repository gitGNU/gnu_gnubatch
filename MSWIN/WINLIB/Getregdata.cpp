/* getregdata.cpp -- Get registry data

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
#include "GetRegData.h"

#ifdef	REGSTRING

CString	GetRegString(CString name)
{
	CString	result = "";
	int  wh = name.ReverseFind('\\');
	if  (wh < 0)
		return  result;
	HKEY	hk;
	if  (RegOpenKeyEx(HKEY_LOCAL_MACHINE, name.Left(wh), 0, KEY_READ, &hk) != ERROR_SUCCESS)
		return  result;
	DWORD  buffsize = 100;
	char	buff[100];
	if  (RegQueryValueEx(hk, name.Right(name.GetLength() - wh - 1), 0, NULL, (LPBYTE) buff, &buffsize) == ERROR_SUCCESS)
		result = buff;
	RegCloseKey(hk);
	return  result;
}

#else

void	GetUserAndComputerNames(CString &usern, CString &computn)
{
	char	buffr[100];
	DWORD	buffrs = sizeof(buffr);
	if  (GetUserName(buffr, &buffrs))
		usern = buffr;
	buffrs = sizeof(buffr);
	if  (GetComputerName(buffr, &buffrs))
		computn = buffr;

}
#endif
