/* helpalt.h -- defines for alternative list

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

typedef struct  {
        SHORT   numalt;                 /*  Number of alternatives  */
        SHORT   def_alt;                /*  Default (or -1 if none) */
        SHORT   *alt_nums;              /*  Alternative numbers */
        char    **list;                 /*  Strings, + null term  */
}  Helpalt, *HelpaltRef;

typedef const   Helpalt *CHelpaltRef;

extern  void    freealts(HelpaltRef);
extern  int     altlen(CHelpaltRef);
extern  char    *disp_alt(int, HelpaltRef);
extern  int     lookup_alt(const char *, HelpaltRef);
extern  HelpaltRef      helprdalt(const int);
#ifdef  refresh                 /* Assumed to be #defined in curses.h */
extern  HelpaltRef      galts(WINDOW *, const int, const int);
#endif

/* Some standard ones */

extern  HelpaltRef      repunit, days_abbrev, ifnposses;
