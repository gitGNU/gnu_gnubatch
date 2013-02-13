/* btfmadefs.c -- arg defs for file monitor

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

const   Argdefault      Adefs[] = {
  {  '?', $A{btfilemon arg explain} },
  {  'd', $A{btfilemon arg daemon} },
  {  'n', $A{btfilemon arg nodaemon} },
  {  'D', $A{btfilemon arg directory} },
  {  'a', $A{btfilemon arg anyfile} },
  {  'p', $A{btfilemon arg pattern file} },
  {  's', $A{btfilemon arg given file} },
  {  'A', $A{btfilemon arg arrival} },
  {  'G', $A{btfilemon arg grow time} },
  {  'M', $A{btfilemon arg mod time} },
  {  'I', $A{btfilemon arg change time} },
  {  'u', $A{btfilemon arg access time} },
  {  'r', $A{btfilemon arg file removed} },
  {  'S', $A{btfilemon arg halt found} },
  {  'C', $A{btfilemon arg cont found} },
  {  'i', $A{btfilemon arg ignore existing} },
  {  'e', $A{btfilemon arg include existing} },
  {  'P', $A{btfilemon arg poll time} },
  {  'X', $A{btfilemon arg script file} },
  {  'c', $A{btfilemon arg command} },
  {  'z', $A{btfilemon arg nonrecursive}  },
  {  'R', $A{btfilemon arg recursive}  },
  {  'Z', $A{btfilemon arg no links}  },
  {  'L', $A{btfilemon arg follow links}  },
  {  'm', $A{btfilemon arg run monitor}  },
  {  'l', $A{btfilemon arg list monitor}  },
  {  'k', $A{btfilemon arg kill proc}  },
  {  'K', $A{btfilemon arg killall}  },
  { 0, 0 }
};
