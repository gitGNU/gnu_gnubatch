/* optkeyword.c -- fetch value of named keyword from config files

   Copyright 2013 Free Software Foundation, Inc.

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
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include "defaults.h"
#include "helpargs.h"
#include "errnums.h"
#include "stringvec.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "files.h"

static  char    Filename[] = __FILE__;

/* Look in config files for the given keyword and return the last one found */

char *optkeyword(const char *Varname)
{
        char    *loclist, *cfilename, *dirname, *result, *newres;
        int     part;
        struct  stringvec  cpath;

        /* Break the path into components */

        loclist = envprocess(CONFIGPATH);
        stringvec_split(&cpath, loclist, ':');
        free(loclist);

        /* Kick off with nothing */

        result = (char *) 0;

        for  (part = 0;  part < stringvec_count(cpath);  part++)  {
                const  char  *pathseg = stringvec_nth(cpath, part);
                unsigned  lng  = strlen(pathseg);

                /* Treat null segments as reference to environment */

                if  (lng == 0)  {
                        if  ((newres = getenv(Varname)))  {
                                if  (result)
                                        free(result);
                                result = stracpy(newres);
                        }
                        continue;
                }

                /* Treat '-' as reference to command args but ignore it, '@' as new home directory config files,
                   '!' as reference to environment */

                if  (lng == 1)  {

                        /* Arg list */

                        if  (pathseg[0] == '-')
                                continue;

                        /* Environment */

                        if  (pathseg[0] == '!')  {
                                if  ((newres = getenv(Varname)))  {
                                        if  (result)
                                                free(result);
                                        result = stracpy(newres);
                                }
                                continue;
                        }

                        /* New style config file */

                        if  (pathseg[0] == '@')  {
                                cfilename = recursive_unameproc(HOME_CONFIG, ".", Realuid);
                                newres = rdoptfile(cfilename, Varname);
                                free(cfilename);
                                if  (newres)  {
                                        if  (result)
                                                free(result);
                                        result = newres;
                                }
                                continue;
                        }
                }

                /* Something else, treat as name of directory */

                dirname = recursive_unameproc(pathseg, ".", Realuid);
                cfilename = malloc((unsigned) (strlen(dirname) + sizeof(USER_CONFIG) + 1));
                if  (!cfilename)
                        ABORT_NOMEM;
                strcpy(cfilename, dirname);
                strcat(cfilename, "/" USER_CONFIG);
                free(dirname);
                if  ((newres = rdoptfile(cfilename, Varname)))  {
                        if  (result)
                                free(result);
                        result = newres;
                }
                free(cfilename);
        }

        close_optfile();
        stringvec_free(&cpath);
        return  result;                 /* Might be null of course */
}
