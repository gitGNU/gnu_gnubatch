/* magic_ch.h -- "magic" chars for curses clients

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

#define MAG_A   1                       /*  Alphabetic chars can be magic  */
#define MAG_P   2                       /*  Any printing chars can be magic */
#define MAG_R   4                       /*  Return value in retv  */
#define MAG_OK  8                       /*  Wgets - allow non-alpha chars  */
#define MAG_CRS 16                      /*  Cursor up/down */
#define MAG_NAME        32              /*  Force it to be a name */
#define MAG_OCTAL       64              /*  Octal numbers (yuk) */
#define MAG_NL  128                     /*  Treat null input as leave unch */

extern int  getkey(const unsigned);
void  endwinkeys();
