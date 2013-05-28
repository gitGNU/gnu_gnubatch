/* btwrite.c -- write messages to user

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  OS_DYNIX
#include "/usr/.include/utmp.h"
#else
#include <utmp.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "errnums.h"
#include "ecodes.h"
#include "files.h"
#include "cfile.h"

extern char *strread(FILE *, const char *);

#ifdef  UTMP_FILE
#define ACTUAL_UTMP_FILE        UTMP_FILE
#else
#define ACTUAL_UTMP_FILE        "/etc/utmp"
#endif

#define INCFILES        10

FILE    *Cfile;                                 /* Need to define this here as not using client lib */
char    *dispatch;

struct  ostr    {
        FILE    *fp;
        int     ismail;
}  *ufiles;

int     fcnt, initfiles;
int     utmp = -1;

/* Satisfy sharedlibs dependencies */
#include "btmode.h"
#include "bjparam.h"
#include "cmdint.h"
#include "btconst.h"
#include "btvar.h"
#include "timecon.h"
#include "btjob.h"
#include "q_shm.h"
struct  jshm_info       Job_seg;
/* End of shared libs dependencies */

static  char    Filename[] = __FILE__;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

/* Add file descriptor to list.  */

void  addu(FILE *fp, int isp)
{
        if  (fcnt >= initfiles)  {
                initfiles += INCFILES;
                if  ((ufiles = (struct ostr *) realloc((char *) ufiles, (unsigned) (initfiles * sizeof(struct ostr)))) == (struct ostr *) 0)
                        ABORT_NOMEM;
        }
        ufiles[fcnt].fp = fp;
        ufiles[fcnt].ismail = isp;
        fcnt++;
}

/* Read through the utmp file to find every instance of the given user
   who is logged in, and try to open the corresponding terminal.  */

void  opuser(char *user)
{
        struct  utmp    up;
        int     fid, hadu = 0;
        FILE    *fp;
        char    *cmd;
        char    dnbuf[5+1+sizeof(up.ut_line)];

        /* If the file no opened yet, try to open it, otherwise seek
           back to the beginning.  */

        if  (utmp < 0)  {
                if  ((utmp = open(ACTUAL_UTMP_FILE, O_RDONLY)) < 0)
                        exit(1);
        }
        else
                lseek(utmp, 0L, 0);

        /* For each entry in the file, try to open the corresponding
           device.  If all goes ok, create the file decriptor.  */

        while  (read(utmp, (char *)&up, sizeof(up)) == sizeof(up))  {
                if  (
#ifdef  USER_PROCESS
                     up.ut_type != USER_PROCESS  ||
#endif
                     strncmp(up.ut_name, user, sizeof(up.ut_name)) != 0)
                        continue;
                sprintf(dnbuf, "/dev/%.*s", (int) sizeof(up.ut_line), up.ut_line);
                if  ((fid = open(dnbuf, O_WRONLY)) < 0)
                        continue;
                if  ((fp = fdopen(fid, "w")) == (FILE *) 0)
                        continue;
                addu(fp, 0);
                hadu++;
        }
        if  (hadu)
                return;
        if  (!dispatch)
                dispatch = envprocess(MSGDISPATCH);
        if  ((cmd = malloc((unsigned) (strlen(dispatch) + 20 + strlen(user) + 1))) == (char *) 0)
                ABORT_NOMEM;
        sprintf(cmd, "%s -mx %s", dispatch, user);
        if  ((fp = popen(cmd, "w")))
                addu(fp, 1);
        free(cmd);
}

void  bleepall()
{
        int     i;
        char    **spmsg = (char **) 0, **sp;

        for  (i = 0;  i < fcnt;  i++)
                if  (ufiles[i].ismail)  {
                        if  (!spmsg)
                                spmsg = helpvec($E{btwrite message diverted}, 'E');
                        for  (sp = spmsg;  *sp;  sp++)
                                fprintf(ufiles[i].fp, "%s\n", *sp);
                }
                else
                        putc('\007', ufiles[i].fp);

        if  (spmsg)
                freehelp(spmsg);
}

/* Send string in buf to every user we noted.  */

void  writeall(char *buf)
{
        int     i;

        for  (i = 0;  i < fcnt;  i++)  {
                fprintf(ufiles[i].fp, "%s\n", buf);
                fflush(ufiles[i].fp);
        }
}

/* Ye olde main routine.
   Arguments expected are a list of users to be splatted at. */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     i;
        char    **hv, *inb;

        versionprint(argv, "$Revision: 1.9 $", 1);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();

        Realuid = getuid();
        Realgid = getgid();

        if  ((Cfile = open_icfile()) == (FILE *) 0)
                exit(E_NOCONFIG);

        initfiles = argc * 2;

        if  ((ufiles = (struct ostr *) malloc((unsigned) (initfiles * sizeof(struct ostr)))) == (struct ostr *) 0)
                ABORT_NOMEM;

        for  (i = 1;  i < argc;  i++)
                opuser(argv[i]);

        if  (fcnt <= 0)
                exit(1);

        bleepall();

        for  (hv = helpvec($E{btwrite message from user}, 'E');  *hv;  hv++)
                writeall(*hv);

        for  (;;)  {
                if  ((inb = strread(stdin, "\n")) == (char *) 0)  {
                        if  (feof(stdin))
                                break;
                        writeall("");
                }
                else  {
                        writeall(inb);
                        free(inb);
                }
        }

        for  (i = 0;  i < fcnt;  i++)
                if  (ufiles[i].ismail)
                        pclose(ufiles[i].fp);
                else
                        fclose(ufiles[i].fp);

        return  0;
}
