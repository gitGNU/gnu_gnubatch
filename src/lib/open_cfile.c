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

        if  (strchr(filename, '$'))  {
                int     count_recurse = RECURSE_MAX;
                resf = envprocess(filename);
                while  (strchr(resf, '$')  &&  --count_recurse > 0)  {
                        char    *tmp = envprocess(resf);
                        free(resf);
                        resf = tmp;
                }
        }
        else
                resf = stracpy(filename);               /* Must be malloced */

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
        const   char    *cfname = USER_CONFIG;
        char    *loclist = envprocess(HELPPATH), *nxt, *filename;
        FILE    *res;

        nxt = loclist;
        for  (;;)  {
                char    *colp, *dirname;

                if  ((colp = strchr(nxt, ':')))
                        *colp = '\0';

                if  (nxt[0] == '-'  &&  nxt[1] == '\0')         /* Ignore - used for config files */
                        goto  donxt;

                if  (*nxt == '\0'  ||  (nxt[0] == '!' && nxt[1] == '\0'))  {
                        if  ((filename = getenv(keyword)))  {
                                free(loclist);
                                return  getcfilefrom(filename, keyword, deft_file, ".");
                        }
                }
                else  {
                        char    cfilename[PATH_MAX];
                        if  (strchr(nxt, '~'))  {
                                if  (!(dirname = unameproc(nxt, ".", Realuid)))
                                        goto  donxt;
                                if  (strchr(dirname, '$'))  {
                                        char    *tmp = envprocess(dirname);
                                        free(dirname);
                                        dirname = tmp;
                                }
                        }
                        else  if  (strchr(nxt, '$'))
                                dirname = envprocess(nxt);
                        else
                                dirname = stracpy(nxt);
                        sprintf(cfilename, "%s/%s", dirname, cfname);
                        if  ((filename = rdoptfile(cfilename, keyword)))  {
                                res = getcfilefrom(filename, keyword, deft_file, dirname);
                                free(dirname);
                                free(filename);
                                free(loclist);
                                return  res;
                        }
                        free(dirname);
                }
        donxt:
                if  (!colp)
                        break;
                *colp++ = ':';
                nxt = colp;
        }

        /* All that failed, so try the standard place.  */

        free(loclist);
        loclist = envprocess(CFILEDIR);                 /* Has a / on the end of it */
        filename = malloc((unsigned) (strlen(loclist) + strlen(deft_file) + 1));
        if  (!filename)
                ABORT_NOMEM;
        strcpy(filename, loclist);
        strcat(filename, deft_file);
        free(loclist);
        if  ((res = fopen(filename, "r")))
                fcntl(fileno(res), F_SETFD, 1);
        Helpfile_path = filename;
        return  res;
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
