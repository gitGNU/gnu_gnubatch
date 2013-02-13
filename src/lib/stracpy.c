/* stracpy.c -- Allocate a buffer for and copy a string.

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

#include "config.h"
#include "incl_unix.h"

/* Include this here as it's used virtually everywhere */

int     Ignored_error;

static  char    Filename[] = __FILE__;

/* Yes I have heard of strdup but UNIXes hadn't when I wrote this.
   Also I want to abort if no space */

char *stracpy(const char *s)
{
        unsigned  l = strlen(s) + 1;
        char    *r;
        if  ((r = malloc(l)) == (char *) 0)
                ABORT_NOMEM;
        return  strcpy(r, s);
}
