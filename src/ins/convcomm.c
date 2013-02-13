/* convcomm.c -- common routines for "dump" programs

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
#include <sys/types.h>
#ifdef  HAVE_LIMITS_H
#include <limits.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "files.h"

static  char    Filename[] = __FILE__;

/* Expand src directory name if it's something like
   SPOOLDIR (which we know about and can insert a default)
   or an environment variable name */

char *expand_srcdir(char *dir)
{
        if  (!strchr(dir, '/'))  {
                char    *rd;
                char    fpath[100];

                if  (strlen(dir) >= sizeof(fpath)-1)
                        return  (char *) 0;

                init_mcfile();

                /* Have to do it like this and not use getenv because for Batch
                   the master config file entries with NAME:VALUE (colon) don't
                   get put in the environment */

                if  (strcmp(dir, "SPOOLDIR") == 0)
                        rd = SPDIR;
                else  {
                        sprintf(fpath, "$%s", dir);
                        rd = fpath;
                }
                rd = envprocess(rd);
                return  rd && *rd == '/'? rd: (char *) 0;
        }
        return  dir;
}

/* Make a path name absolute before we change directory */

char *make_absolute(char *file)
{
        char    *curr, *res;

        if  (file[0] == '/')
                return  file;

        curr = runpwd();
        if  (!(res = malloc((unsigned) (strlen(curr) + strlen(file) + 2))))
                ABORT_NOMEM;

        sprintf(res, "%s/%s", curr, file);
        free(curr);
        return  res;
}
