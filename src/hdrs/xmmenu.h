/* xmmenu.h -- menu stuff for Motif progs

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

enum  casc_type { SEP, DSEP, ITEM };

typedef struct  {
        enum    casc_type       type;
        char                    *name;
        void                    (*callback)();
        int                     callback_data;
}  casc_button;

typedef struct  {
        char            *pull_name;     /* Name of pulldown item */
        int             nitems;         /* Number of items */
        int             helpnum;
        casc_button     *items; /* List of items */
        char            ishelp;         /* Mark as help */
}  pull_button;

extern Widget  BuildPulldown(Widget, pull_button *);
