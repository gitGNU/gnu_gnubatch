/* open_cfile.c -- open message file

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "cfile.h"
#include "ecodes.h"
#include "stringvec.h"

static  char    Filename[] = __FILE__;

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

#define RECURSE_MAX     10      /* Maximum number of nested $ENV constructs */

extern  char    *Helpfile_path;

/* Define this here */

FILE    *Cfile;

FILE  *getcfilefrom(char *filename, const char *keyword, const char *deft_file, const char *defdir)
{
        char    *resf;
        FILE    *res;

        /* Expand out $ and ~ constructs in filename */

        resf = recursive_unameproc(filename, ".", Realuid);

        if  (resf[0] != '/')  {
                char  *fullp = malloc((unsigned) (strlen(defdir) + strlen(resf) + 2));
                if  (!fullp)
                        ABORT_NOMEM;
                sprintf(fullp, "%s/%s", defdir, resf);
                free(resf);
                resf = fullp;
        }

        /* If not opened, try again with default file name substituted
           for last part of path name */

        if  (!(res = fopen(resf, "r")))  {
                char    *deff, *slp;

                if  ((deff = (char *) malloc((unsigned) (strlen(resf) + strlen(deft_file)))) == (char *) 0)
                        ABORT_NOMEM;

                strcpy(deff, resf);
                if  ((slp = strrchr(deff, '/')))
                        slp++;
                else
                        slp = deff;
                strcpy(slp, deft_file);
                if  (!(res = fopen(deff, "r")))
                        fprintf(stderr,
                                       "Help cannot open `%s'\n(filename obtained from %s=%s)\n",
                                       resf,
                                       keyword,
                                       filename);
                free(resf);
                resf = deff;
        }
        Helpfile_path = resf;
        if  (res)
                fcntl(fileno(res), F_SETFD, 1);
        return  res;
}

static  FILE    *open_cfile_int(const char *keyword, const char *deft_file)
{
        char    *loclist, *dirname, *cfilename, *filename;
        int     part;
        FILE    *result;
        struct  stringvec  hpath;

        /* Split path up into bits */

        loclist = envprocess(HELPPATH);
        stringvec_split(&hpath, loclist, ':');
        free(loclist);

        for  (part = 0;  part < stringvec_count(hpath);  part++)  {
                const  char  *pathseg = stringvec_nth(hpath, part);
                unsigned  lng  = strlen(pathseg);

                /* Treat null segments as reference to environment */

                if  (lng == 0)  {
                        filename = getenv(keyword);
                        if  (!filename)
                                continue;
                        stringvec_free(&hpath);
                        return  getcfilefrom(filename, keyword, deft_file, ".");
                }

                /* Treat '@' as new home directory config files, '!' as reference to environment */

                if  (lng == 1)  {

                        /* Environment */

                        if  (pathseg[0] == '!')  {
                                filename = getenv(keyword);             /* This hasn't been malloced */
                                if  (!filename)
                                        continue;
                                stringvec_free(&hpath);
                                return  getcfilefrom(filename, keyword, deft_file, ".");
                        }

                        /* New style config file */

                        if  (pathseg[0] == '@')  {
                                cfilename = recursive_unameproc(HOME_CONFIG, ".", Realuid);
                                filename = rdoptfile(cfilename, keyword);
                                free(cfilename);
                                if  (!filename)
                                        continue;
                                result = getcfilefrom(filename, keyword, deft_file, ".");
                                free(filename);
                                stringvec_free(&hpath);
                                return  result;
                        }
                }

                /* Something else, treat as name of directory which we add the .file to */

                dirname = recursive_unameproc(pathseg, ".", Realuid);
                cfilename = malloc((unsigned) (strlen(dirname) + sizeof(USER_CONFIG) + 1));
                if  (!cfilename)
                        ABORT_NOMEM;
                strcpy(cfilename, dirname);
                strcat(cfilename, "/" USER_CONFIG);
                free(dirname);

                /* Now read the config file */

                filename = rdoptfile(cfilename, keyword);
                free(cfilename);
                if  (!filename)
                        continue;

                /* Got file it's an error if it's not there */

                result = getcfilefrom(filename, keyword, deft_file, ".");
                free(filename);
                stringvec_free(&hpath);
                return  result;
        }

        /* All that failed, so try the standard place.  */

        stringvec_free(&hpath);
        dirname = envprocess(CFILEDIR);                 /* Has a / on the end of it */
        filename = malloc((unsigned) (strlen(dirname) + strlen(deft_file) + 1));
        if  (!filename)
                ABORT_NOMEM;
        strcpy(filename, dirname);
        strcat(filename, deft_file);
        free(dirname);
        if  ((result = fopen(filename, "r")))
                fcntl(fileno(result), F_SETFD, 1);
        Helpfile_path = filename;
        return  result;
}

FILE    *open_cfile(const char *keyword, const char *deft_file)
{
        FILE    *res = open_cfile_int(keyword, deft_file);
        if  (!res)  {
                const  char  *sp = strrchr(Helpfile_path, '/');
                if  (sp)
                        sp++;
                else
                        sp = Helpfile_path;
                fprintf(stderr, "Cannot find help message file %s\n", Helpfile_path);
                if  (strcmp(sp, deft_file) != 0)
                        fprintf(stderr, "(Default is %s but %s was assigned to)\n", deft_file, keyword);
                fprintf(stderr, "Maybe installation was not complete?\n");
                exit(E_NOCONFIG);
        }
        return  res;
}
