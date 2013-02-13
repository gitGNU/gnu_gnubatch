/* errnums.h -- error number handline

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

/* These are all parameters which programs can plug values into
   for displaying error etc messages with */

extern  LONG    disp_arg[];
extern  double  disp_float;
extern  const   char    *disp_str,
                        *disp_str2,
                        *progname;
extern  FILE    *Cfile;

extern  void    count_hv(char **, int *, int *);
extern  void    freehelp(char **);
extern  void    print_error(int);
extern  void    fprint_error(FILE *, int);

/* These really want replacing with "stringvecs" as arrays of char arrays
   are messy. */

extern  char    **helpvec(const int, const char);
extern  char    **helphdr(const int);
extern  char    **mmangle(char **);
