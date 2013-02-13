/* btradefs.c -- arg defs for gbch-r and friends

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

const   Argdefault  Adefs[] = {
        {  '?', $A{btr arg explain} },
        {  'V', $A{btr arg noverb} },
        {  'v', $A{btr arg verb} },
        {  'm', $A{btr arg mail} },
        {  'w', $A{btr arg write} },
        {  'h', $A{btr arg title} },
        {  'x', $A{btr arg nomess} },
        {  'i', $A{btr arg interp} },
        {  'p', $A{btr arg pri} },
        {  'l', $A{btr arg ll} },
        {  'N', $A{btr arg norm} },
        {  'C', $A{btr arg canc} },
        {  'S', $A{btr arg skip} },
        {  'H', $A{btr arg hold} },
        {  'R', $A{btr arg resched} },
        {  '9', $A{btr arg catchup} },
        {  'U', $A{btr arg notime} },
        {  'T', $A{btr arg time} },
        {  'Z', $A{btr arg cancio} },
        {  'I', $A{btr arg io} },
        {  'o', $A{btr arg norep} },
        {  'r', $A{btr arg repeat} },
        {  'd', $A{btr arg delete} },
        {  'D', $A{btr arg dir} },
        {  'e', $A{btr arg cancarg} },
        {  'a', $A{btr arg argument} },
        {  'y', $A{btr arg canccond} },
        {  'c', $A{btr arg cond} },
        {  'z', $A{btr arg cancset} },
        {  'f', $A{btr arg setflags} },
        {  's', $A{btr arg set} },
        {  'A', $A{btr arg avoid} },
        {  'M', $A{btr arg mode} },
        {  'P', $A{btr arg umask} },
        {  'L', $A{btr arg ulimit} },
        {  'X', $A{btr arg exits} },
        {  'j', $A{btr arg adverr} },
        {  'J', $A{btr arg noadverr} },
        {  'u', $A{btr arg setu} },
        {  'g', $A{btr arg setg} },
        {  'n', $A{btr arg loco} },
        {  'F', $A{btr arg export}      },
        {  'G', $A{btr arg fullexport} },
        {  'k', $A{btr arg condcrit} },
        {  'K', $A{btr arg nocondcrit} },
        {  'b', $A{btr arg asscrit} },
        {  'B', $A{btr arg noasscrit} },
        {  'q', $A{btr arg queue} },
#ifndef JOBDUMP_INLINE
        {  'Q', $A{btr arg host} },
        {  'E', $A{btr arg locenv} },
        {  'O', $A{btr arg remenv} },
#endif
        {  't', $A{btr arg deltime} },
        {  'Y', $A{btr arg runtime} },
        {  'W', $A{btr arg wsig} },
        {  '2', $A{btr arg grace} },
        {  '.', $A{btr arg done} },
        { 0, 0 }
};
