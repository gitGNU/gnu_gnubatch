/* optprocess.c -- process options to shell commands

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
#include <ctype.h>
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "optflags.h"
#include "errnums.h"
#include "stringvec.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

static  char    Filename[] = __FILE__;

/* Construct environment variable name from program name */

char    *make_varname()
{
        char  *varname = malloc((unsigned) strlen(progname) + 1), *vp;
        const  char  *pp;

        if  (!varname)
                ABORT_NOMEM;

        pp = progname;
        vp = varname;

        while  (*pp)  {
                int  ch = *pp++;
                if  (!isalpha(ch))
                        *vp++ = '_';
                else
                        *vp++ = toupper(ch);
        }
        *vp = '\0';
        return  varname;
}

char   **optprocess(char **argv, const Argdefault *defaultopts, optparam *const optlist, const int minstate, const int maxstate, const int keepargs)
{
	char    *Varname = make_varname();
        HelpargRef      avec = helpargs(defaultopts, minstate, maxstate);
        char    *loclist, *cfilename, *dirname, *name;
        int     hadargv = 0, part;
        struct  stringvec  cpath;

        /* Break the path into components */

        loclist = envprocess(CONFIGPATH);
        stringvec_split(&cpath, loclist, ':');
        free(loclist);

        for  (part = 0;  part < stringvec_count(cpath);  part++)  {
                const  char  *pathseg = stringvec_nth(cpath, part);
                unsigned  lng  = strlen(pathseg);

                /* Treat null segments as reference to environment */

                if  (lng == 0)  {
                        doenv(getenv(Varname), avec, optlist, minstate);
                        continue;
                }

                /* Treat '-' as reference to command args, '@' as new home directory config files,
                   '!' as reference to environment */

                if  (lng == 1)  {

                        /* Arg list */

                        if  (pathseg[0] == '-')  {
                                if  (!hadargv)          /* Only once */
                                        argv = doopts(&argv[0], avec, optlist, minstate);
                                hadargv++;
                                continue;
                        }

                        /* Environment */

                        if  (pathseg[0] == '!')  {
                                doenv(getenv(Varname), avec, optlist, minstate);
                                continue;
                        }

                        /* New style config file */

                        if  (pathseg[0] == '@')  {
                                cfilename = recursive_unameproc(HOME_CONFIG, ".", Realuid);
                                name = rdoptfile(cfilename, Varname);
                                free(cfilename);
                                if  (name)  {
                                        doenv(name, avec, optlist, minstate);
                                        free(name);
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
                if  ((name = rdoptfile(cfilename, Varname)))  {
                        doenv(name, avec, optlist, minstate);
                        free(name);
                }
                free(cfilename);
        }

        close_optfile();
        stringvec_free(&cpath);

        if  (keepargs || Anychanges & OF_ANY_FREEZE_WANTED)
                makeoptvec(avec, minstate, maxstate);
        freehelpargs(avec);
        if  (!hadargv)
                argv++;
        free(Varname);
        return  argv;
}
