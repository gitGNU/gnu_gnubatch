/* cmdint.h -- format of command interpreter list

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
        USHORT          ci_ll;          /*  Default load level */
        unsigned  char  ci_nice;        /*  Nice value */
        unsigned  char  ci_flags;       /*  Flags for cmd int */
        char    ci_name[CI_MAXNAME+1];  /*  Name (e.g. shell) */
        char    ci_path[CI_MAXFPATH+1]; /*  Path name */
        char    ci_args[CI_MAXARGS+1];  /*  Arg list */
}  Cmdint, *CmdintRef;

typedef const   Cmdint  *CCmdintRef;

#define CI_STDSHELL     0               /* Standard shell */

#define CIF_SETARG0     (1 << 0)        /* Flag to set arg0 from job title */
#define CIF_INTERPARGS  (1 << 1)        /* Flag to set to interpolate $1 etc args ourselves */

extern  CmdintRef       Ci_list;
extern  int             Ci_num, Ci_fd;

extern int  open_ci(const int);
extern void  rereadcif();
extern int  validate_ci(const char *);
