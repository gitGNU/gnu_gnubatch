/* open_ci.c -- open/load file of command interpreters

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"
#include "bjparam.h"
#include "cmdint.h"

static  char    Filename[] = __FILE__;

int     Ci_num,
        Ci_fd;

static  time_t  Ci_time;
CmdintRef       Ci_list;

int  open_ci(const int omode)
{
        char    *cifile = envprocess(BTCI);
        struct  stat    sbuf;

        if  ((Ci_fd = open(cifile, omode)) < 0)  {
                free(cifile);
                return  $E{Cannot open ci file};
        }
        free(cifile);

        fstat(Ci_fd, &sbuf);
        if  (sbuf.st_size == 0  ||  sbuf.st_size % sizeof(Cmdint) != 0)  {
                close(Ci_fd);
                return  $E{Cannot read ci file};
        }
        Ci_num = sbuf.st_size / sizeof(Cmdint);
        if  ((Ci_list = (CmdintRef) malloc((unsigned) sbuf.st_size)) == (CmdintRef) 0)
                ABORT_NOMEM;
        if  (read(Ci_fd, (char *) Ci_list, (unsigned) sbuf.st_size) != sbuf.st_size)  {
                close(Ci_fd);
                return  $E{Cannot read ci file};
        }

        fcntl(Ci_fd, F_SETFD, 1);
        Ci_time = sbuf.st_mtime;
        return  0;
}

void  rereadcif()
{
        struct  stat    sbuf;
        fstat(Ci_fd, &sbuf);
        if  (Ci_time == sbuf.st_mtime)
                return;
        free(Ci_list);
        Ci_num = sbuf.st_size / sizeof(Cmdint);
        if  ((Ci_list = (CmdintRef) malloc((unsigned) sbuf.st_size)) == (CmdintRef) 0)
                ABORT_NOMEM;
        lseek(Ci_fd, 0L, 0);
        Ignored_error = read(Ci_fd, (char *) Ci_list, (unsigned) sbuf.st_size);
        Ci_time = sbuf.st_mtime;
}

int  validate_ci(const char *name)
{
        int     n;
        rereadcif();

        for  (n = 0;  n < Ci_num;  n++)
                if  (strcmp(name, Ci_list[n].ci_name) == 0)
                        return  n;
        return  -1;
}
