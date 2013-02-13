/* runpwd.c -- get current directory

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
#include <stdio.h>

extern char *strread(FILE *, const char *);

/* Yes I know there are standard alternatives to this but there
   weren't when this was first written */

char *runpwd()
{
        FILE    *fp;
        char    *result;

        if  ((fp = popen("/bin/pwd", "r")) == (FILE *) 0)
                return  (char *) 0;
        result = strread(fp, "\n");
        pclose(fp);
        return  result;
}
