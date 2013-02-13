/* sctrl.h -- curses routine wget* options

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

struct  sctrl   {
        int     helpcode;               /*  Help code (see help file)  */
        char    **(*helpfn)();          /*  Routine which will return block
                                            of specific possibilities  */
        USHORT  size;                   /*  Size of field  */
        SHORT   retv;                   /*  Return value for MAG_R  */
        SHORT   col;                    /*  Position on screen or -1 */
        unsigned  char  magic_p;        /*  Magic printing chars  */
        LONG    min, vmax;              /*  Min/max values  */
        char    *msg;                   /*  Special description (if any) */
};

/* Mapping of format characters (assumed A-Z a-z) and format routines */

struct  formatdef  {
        SHORT   statecode;      /* Code number for heading if applicable */
        SHORT   sugg_width;     /* Suggested width */
        char    *msg;           /* Heading */
        char    *explain;       /* More detailed explanation */
        int     (*fmt_fn)();
};
