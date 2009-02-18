/* btrargs.ini -- Defaults and proc table for arg interp.

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
	
const	Argdefault  Adefs[] = {
	{  '?', ARG_BTR_EXPLAIN },
	{  'V', ARG_BTR_NOVERBOSE },
	{  'v', ARG_BTR_VERBOSE },
	{  'm', ARG_BTR_MAIL },
	{  'w', ARG_BTR_WRITE },
	{  'h', ARG_BTR_TITLE },
	{  'x', ARG_BTR_CANCMAILWRITE },
	{  'i', ARG_BTR_INTERPRETER },
	{  'p', ARG_BTR_PRIORITY },
	{  'l', ARG_BTR_LOADLEV },
	{  'N', ARG_BTR_NORMAL },
	{  'C', ARG_BTR_ASCANC },
	{  'S', ARG_BTR_SKIP },
	{  'H', ARG_BTR_HOLD },
	{  'R', ARG_BTR_RESCHED },
	{  'U', ARG_BTR_NOTIME },
	{  'T', ARG_BTR_TIME },
	{  'Z', ARG_BTR_CANCIO },
	{  'I', ARG_BTR_IO },
	{  'o', ARG_BTR_NOREPEAT },
	{  'r', ARG_BTR_REPEAT },
	{  'd', ARG_BTR_DELETEATEND },
	{  'D', ARG_BTR_DIRECTORY },
	{  'e', ARG_BTR_CANCARGS },
	{  'a', ARG_BTR_ARGUMENT },
	{  'y', ARG_BTR_CANCCOND },
	{  'c', ARG_BTR_CONDITION },
	{  'z', ARG_BTR_CANCSET },
	{  'f', ARG_BTR_FLAGS },
	{  's', ARG_BTR_SET },
	{  'A', ARG_BTR_AVOIDING },
	{  'M', ARG_BTR_MODE },
	{  'P', ARG_BTR_UMASK },
	{  'L', ARG_BTR_ULIMIT },
	{  'X', ARG_BTR_EXITS },
	{  'j',	ARG_BTR_AVTERR },
	{  'J',	ARG_BTR_NOAVTERR },
	{  'u', ARG_BTR_OWNER },
	{  'g', ARG_BTR_GROUP },
	{  'n', ARG_BTR_NOEXPORT },
	{  'F', ARG_BTR_EXPORT	},
	{  'G', ARG_BTR_FULLEXPORT },
	{  'k', ARG_BTR_CONDCRIT },
	{  'K', ARG_BTR_NOCONDCRIT },
	{  'b', ARG_BTR_ASSCRIT },
	{  'B', ARG_BTR_NOASSCRIT },
	{  'q', ARG_BTR_JOBQUEUE },
	{  't', ARG_BTR_DELTIME },
	{  'Y', ARG_BTR_RUNTIME },
	{  'E', ARG_BTR_WHICHSIG },
	{  '2', ARG_BTR_GRACETIME },
	{  'O', ARG_BTR_BINMODE },
	{  'W', ARG_BTR_TEXTMODE },
	{ 0, 0 }
};
