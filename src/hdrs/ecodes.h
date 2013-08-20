/* ecodes.h -- exit codes

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

#define E_TRUE          0       /* Return true */
#define E_FALSE         1       /* Return false */
#define E_USAGE         2       /* Bad arguments to program */
#define E_PERM          3       /* Invalid permissions */
#define E_SYNCFAIL      4       /* Syncronisation failure */
#define E_NOCHDIR       5
#define E_NOTRUN        6
#define E_NOHOST        7
#define E_NETERR        8
#define E_SHUTD         11
#define E_NOJOB         13
#define E_CLASHES       14
#define E_NOPRIV        16
#define E_FNOTFND       19
#define E_VNOTFND       20
#define E_UNOTSETUP     30
#define E_NOUSER        31
#define E_RUNNING       32
#define E_DFULL         50
#define E_BADCFILE      100
#define E_INPUTIO       101
#define E_JDFNFND       150
#define E_JDNOCHDIR     151
#define E_JDFNOCR       152
#define E_JDJNFND       153
#define E_CANTDEL       154
#define E_CANTSAVEO     155
#define E_FM_DCLASH     160
#define E_FM_TOOMANY    161
#define E_SIGNAL        200
#define E_NOPIPE        201
#define E_NOFORK        202
#define E_JOBQ          203
#define E_VARL          204
#define E_NOTIMPL       230
#define E_SHEDERR       240
#define E_BTEXEC1       244
#define E_BTEXEC2       245
#define E_EXENODIR      246
#define E_EXENOJOB      247
#define E_EXENOCI       248
#define E_EXENOOPEN     249
#define E_SETUP         250
#define E_EXENOPIPE     251
#define E_EXENOFORK     252
#define E_LIMIT         253
#define E_NOMEM         254
#define E_NOCONFIG      255
