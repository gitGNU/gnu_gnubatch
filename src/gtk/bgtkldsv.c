/* bgtkldsv.c -- GTK program file manipulation

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
#include "incl_sig.h"
#include "files.h"
#include "ecodes.h"

#define BUFFSIZE        256

/* On signal just exit. We put this in because
   it may have been called from an environment which ignores some signals, notably SIGPIPE. */

RETSIGTYPE      catch_sig_read(int n)
{
        _exit(E_SIGNAL);
}

int  readfile(char *fl)
{
        FILE  *inf = fopen(fl, "r");
        size_t  nb;
        char    buf[1024];
#ifdef  STRUCT_SIG
        struct  sigstruct_name  za;

        za.sighandler_el = catch_sig_read;
        sigmask_clear(za);
        za.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGINT, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGHUP, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGPIPE, &za, (struct sigstruct_name *) 0);
#else
        signal(SIGINT, catch_sig);
        signal(SIGQUIT, catch_sig);
        signal(SIGTERM, catch_sig);
        signal(SIGHUP, catch_sig);
        signal(SIGPIPE, catch_sig);
#endif

        if  (!inf)
                return  E_NOJOB;

        while  ((nb = fread(buf, 1, sizeof(buf), inf))  !=  0)
                fwrite(buf, 1, nb, stdout);
        fclose(inf);
        return  0;
}

/* Fix up for written file to be deleted on premature termination */

static  char    *output_file;

RETSIGTYPE      catch_sig_write(int n)
{
        if  (output_file)
                unlink(output_file);
        _exit(E_SIGNAL);
}

int     writefile(char *fl, const int setexec)
{
        FILE  *outf;
        size_t  nb;
        char    buf[1024];
        int     um = umask(0), cmask = 0666;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  za;

        za.sighandler_el = catch_sig_write;
        sigmask_clear(za);
        za.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGINT, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGHUP, &za, (struct sigstruct_name *) 0);
        sigact_routine(SIGPIPE, &za, (struct sigstruct_name *) 0);
#else
        signal(SIGINT, catch_sig);
        signal(SIGQUIT, catch_sig);
        signal(SIGTERM, catch_sig);
        signal(SIGHUP, catch_sig);
        signal(SIGPIPE, catch_sig);
#endif

        output_file = fl;

        outf = fopen(fl, "w");
        umask(um);

        if  (!outf)
                return  E_NOJOB;
        while  ((nb = fread(buf, 1, sizeof(buf), stdin))  !=  0)
                fwrite(buf, 1, nb, outf);

        if  (setexec)
                cmask = 0777;
        cmask &= ~um;
#ifdef  HAVE_FCHMOD
        fchmod(fileno(outf), cmask);
#endif
        fclose(outf);
#ifndef HAVE_FCHMOD
        chmod(fl, cmask);
#endif
        return  0;
}

/*  Arguments are:
    -w create/open file for writing (we read from pipe) / -r open file for reading (we write to pipe)
    file name -d just delete.
    Run as root to switch to appropriate User ID */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     ch;
        enum    { ACT_NONE, ACT_READ, ACT_WRITE, ACT_EXECWRITE, ACT_DELETE } act = ACT_NONE;
        struct  passwd  *pw = getpwnam(BATCHUNAME);
        uid_t  duid = ROOTID, myuid = getuid(), mygid = getgid();
        char    *filename;
        extern  int     optind;

        versionprint(argv, "$Revision: 1.9 $", 1);

        /* Get batch uid */

        if  (pw)
                duid = pw->pw_uid;
        endpwent();

        /* If the real user id is "batch" this is either because we have invoked the
           original ui program as batch or because we have invoked a GTK program which
           switched the real uid to batch. In such cases we fish the uid out of the
           home directory and use that.

           Refuse to work if no home directory or it looks strange */

        if  (myuid == duid)  {
                char    *homed = getenv("HOME");
                struct  stat  sbuf;

                if  (!homed  ||  stat(homed, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)
                        exit(E_SETUP);

                myuid = sbuf.st_uid;
                mygid = sbuf.st_gid;
        }

        /* Switch completely to the user id and do the bizniz. */

        setgid(mygid);
        setuid(myuid);

        /* If we haven't got the right arguments then just quit.
            This is only meant to be run by xbtr. */

         while  ((ch = getopt(argc, argv, "rRwWdDxX")) != EOF)
                 switch  (ch)  {
                 default:
                         return  E_USAGE;
                 case  'r':case  'R':
                         act = ACT_READ;
                         break;
                 case  'w':case  'W':
                         act = ACT_WRITE;
                         break;
                 case  'd':case  'D':
                         act = ACT_DELETE;
                         break;
                 case  'x':case  'X':
                         act = ACT_EXECWRITE;
                         break;
                 }

         filename = argv[optind];
         if  (!filename)
                 return  E_USAGE;

         switch  (act)  {
         default:
                 return  E_USAGE;
         case  ACT_READ:
                 return  readfile(filename);
         case  ACT_WRITE:
                 return  writefile(filename, 0);
         case  ACT_DELETE:
                 return  unlink(filename) >= 0? 0: E_NOJOB;
         case  ACT_EXECWRITE:
                 return  writefile(filename, 1);
         }
}
