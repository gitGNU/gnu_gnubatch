/* xmbr_files.c -- file handling for gbch-xmr

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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <Xm/FileSB.h>
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "statenums.h"
#include "errnums.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "cmdint.h"
#include "btuser.h"
#include "q_shm.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "incl_dir.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

#ifndef HAVE_LONG_FILE_NAMES
#include <sys/dir.h>

/* Stuff to simulate BSD-like directory ops on Sys V etc */

typedef struct  {
        int     dd_fd;
}       DIR;
struct dirent  {
        LONG    d_ino;
        char    d_name[1];
};

/* Big enough to ensure that nulls on end */

static  union  {
        struct  dirent  result_d;
        char    result_b[sizeof(struct direct) + 2];
}  Result;

static  DIR     Res;

DIR     *opendir(filename)
char    *filename;
{
        int     fd;
        struct  stat    sbuf;

        if  ((fd = open(filename, 0)) < 0)
                return  (DIR *) 0;
        if  (fstat(fd, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                errno = ENOTDIR;
                 close(fd);
                return  (DIR *) 0;
        }

        Res.dd_fd = fd;
        return  &Res;
}

struct  dirent  *readdir(dirp)
DIR     *dirp;
{
        struct  dirent  *dp;
        struct  direct  indir;

        while  (read(dirp->dd_fd, (char *)&indir, sizeof(indir)) > 0)  {
                if  (indir.d_ino == 0)
                        continue;
                Result.result_d.d_ino = indir.d_ino;
                strncpy(Result.result_d.d_name, indir.d_name, DIRSIZ);
                return  &Result.result_d;
        }
        return  (struct dirent *) 0;
}

void    seekdir(dirp, loc)
DIR     *dirp;
long    loc;
{
        lseek(dirp->dd_fd, loc, 0);
}

#define rewinddir(dirp) seekdir(dirp,0L)

int     closedir(dirp)
DIR     *dirp;
{
        return  close(dirp->dd_fd);
}
#endif  /*  !HAVE_LONG_FILE_NAMES  */

static int  checkit(char *dirname, char *fname)
{
        FILE    *fp;
        struct  stat    sbuf;
        char    Pbuf[PATH_MAX];

        sprintf(Pbuf, "%s/%s", dirname, fname);
        if  (stat(Pbuf, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFREG  ||  sbuf.st_size == 0)
                return  0;
        SWAP_TO(Realuid);
        fp = fopen(Pbuf, "r");
        SWAP_TO(Daemuid);
        if  (!fp)
             return  0;
        while  (fgets(Pbuf, sizeof(Pbuf), fp))  {
                char    *lp;
                if  (Pbuf[0] == '#')
                        continue;
                if  ((lp = strchr(Pbuf, '\n')))
                        *lp = '\0';
                if  (strncmp(Pbuf, BTR_PROGRAM " ", sizeof(BTR_PROGRAM)) == 0)  {
                        fclose(fp);
                        return  1;
                }
                if  (!strchr(Pbuf, '='))  {
                        fclose(fp);
                        return  0;
                }
        }
        fclose(fp);
        return  0;
}

/* Have a look at files in the given directory to see if they are
   tolerable as command files.  */

#define INIT_NAMES      20
#define INC_NAMES       10

void  isit_cmdfile(Widget w, XtPointer searchdata)
{
        XmFileSelectionBoxCallbackStruct *cbs = (XmFileSelectionBoxCallbackStruct *) searchdata;
        char    *dirname;
        DIR     *dfd;
        struct  dirent  *dp;
        XmString        *namelist = (XmString *) 0;
        unsigned        namecount = 0, namemax = 0;

        if  (!XmStringGetLtoR(cbs->dir, XmSTRING_DEFAULT_CHARSET, &dirname) || !dirname)
                return;

        if  (!(dfd = opendir(dirname)))
                return;

        while  ((dp = readdir(dfd)) != (struct dirent *) 0)  {
                if  (dp->d_name[0] == '.') /* Skip over .files (including .xibatch!) */
                        continue;

                if  (!checkit(dirname, dp->d_name))
                        continue;

                if  (namecount >= namemax)  {
                        if  (namemax == 0)  {
                                namemax = INIT_NAMES;
                                namelist = (XmString *) XtMalloc(INIT_NAMES * sizeof(XmString));
                        }
                        else  {
                                namemax += INC_NAMES;
                                namelist = (XmString *) XtRealloc((XtPointer) namelist, namemax * sizeof(XmString));
                        }
                }

                namelist[namecount] = XmStringCreateSimple(dp->d_name);
                namecount++;
        }

        closedir(dfd);

        XtFree(dirname);

        if  (namecount)  {
                XmString        Path;
                Path = XmStringConcat(cbs->dir, namelist[0]);
                XtVaSetValues(w,
                              XmNfileListItems,         namelist,
                              XmNdirSpec,               Path,
                              XmNfileListItemCount,     (int) namecount,
                              XmNlistUpdated,           True,
                              NULL);
                XmStringFree(Path);
                do  XmStringFree(namelist[--namecount]);
                while  (namecount);
                XtFree((XtPointer) namelist);
        }
        else
                XtVaSetValues(w,
                              XmNfileListItems,         NULL,
                              XmNfileListItemCount,     0,
                              XmNlistUpdated,           True,
                              NULL);
}

int  f_exists(const char *path)
{
        struct  stat    sbuf;

        return  stat(path, &sbuf) >= 0;
}
