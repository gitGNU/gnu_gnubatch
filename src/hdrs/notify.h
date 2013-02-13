/* notify.h -- process notify

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

typedef enum { NOTIFY_MAIL, NOTIFY_WRITE, NOTIFY_DOSWRITE } cmd_type;

/* The following f.d. numbers are used to pass standard output
   and standard error as input to the btmdisp prog. */

#define SOSTREAM        18
#define SESTREAM        19
