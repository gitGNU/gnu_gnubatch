/* bgtksave.c -- GTK program to save data in user's home directory

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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include "defaults.h"
#include "incl_unix.h"
#include "files.h"
#include "ecodes.h"

#define BUFFSIZE        256

/* Field names are alternate arguments */

int  isfld(char *buff, char **argv)
{
        char    *ep = strchr(buff, '=');
        char    **ap;
        unsigned  lng = ep - buff;

        if  (!ep)
                return  0;

        for  (ap = argv + 1;  *ap;  ap += 2)  {
                if  (lng != strlen(*ap))
                        continue;
                if  (strncmp(buff, *ap, lng) == 0)
                        return  1;
        }
        return  0;
}

/*  Arguments are:
    1. Encoded options XBTQDISPOPT
    2. Encoded class code XBTQDISPCC
    3. Users to limit display to XBTQDISPUSER
    4. Printers to limit display to XBTQDISPPTR
    5. Job titles to limit display to XBTQDISPTIT
    6. Fields for job view XBTQJOBFLD
    7. Fields for job view XBTQPTRFLD */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *homed;
        int     oldumask, cnt;
        FILE    *xtfile;

        versionprint(argv, "$Revision: 1.9 $", 1);

        /* If we haven't got the right arguments then just quit.
           This is only meant to be run by xbtq.
           Maybe one day we'll have a more sophisticated routine. */

        if  ((argc & 1) == 0)
                return  E_USAGE;

        if  (!(homed = getenv("HOME")))  {
                struct  passwd  *pw = getpwuid(getuid());
                if  (!pw)
                        return  E_SETUP;
                homed = pw->pw_dir;
        }

        /* Set umask so anyone can read the file (home dir mush be at least 0111). */

        oldumask = umask(0);
        umask(oldumask & ~0444);

        if  (chdir(homed) < 0  ||  (chdir(HOME_CONFIG_DIR) < 0  &&  (mkdir(HOME_CONFIG_DIR, 0777) < 0 || chdir(HOME_CONFIG_DIR) < 0)))
                return  E_SETUP;

         if  ((xtfile = fopen(HOME_CONFIG_FILE, "r")))  {
                FILE  *tmpf = tmpfile();
                char    buffer[BUFFSIZE];

                while  (fgets(buffer, BUFFSIZE, xtfile))  {
                        if  (!isfld(buffer, argv))
                                fputs(buffer, tmpf);
                }
                rewind(tmpf);
                fclose(xtfile);
                if  (!(xtfile = fopen(HOME_CONFIG_FILE, "w")))
                        return  E_NOPRIV;
                while  (fgets(buffer, BUFFSIZE, tmpf))
                        fputs(buffer, xtfile);
        }
        else  if  (!(xtfile = fopen(HOME_CONFIG_FILE, "w")))
                return  E_NOPRIV;

        /* Now stick the new stuff on the end of the file */

        for  (cnt = 1;  cnt < argc;  cnt += 2)  {
                char    *fld = argv[cnt];
                char    *val = argv[cnt+1];
                if  (strcmp(val, "-") != 0)
                        fprintf(xtfile, "%s=%s\n", fld, val);
        }

        return  0;
}
