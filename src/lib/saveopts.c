/* saveopts.c -- save program options

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
#include <sys/stat.h>
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "files.h"
#include "helpargs.h"
#include "errnums.h"

USHORT  Save_umask = 0xFFFF;            /* Invalid value indicating not set */

int  spitoption(const int arg, const int firstarg, FILE *xfl, const int pch, const int cc)
{
        int     v = arg - firstarg;

        if  (optvec[v].isplus)
                fprintf(xfl, "%c+%s", pch, optvec[v].aun.string);
        else  if  (optvec[v].aun.letter == 0)
                fprintf(xfl, "%c+missing-arg-code-%d", pch, arg);
        else  if  (cc)  {
                fprintf(xfl, "%c", optvec[v].aun.letter);
                return  1;
        }
        else  {
                fprintf(xfl, "%c-%c", pch, optvec[v].aun.letter);
                return  1;
        }
        return  0;
}

static void  copyout(FILE *src, FILE *dest, const char *name)
{
        int     ch, match, nn;

        while  ((ch = getc(src)) != EOF)  {

                /* Trim off leading spaces rdoptfile ignores them but they shouldn't be there.  */

                if  (ch == ' ' || ch == '\t')
                        continue;

                if  (ch != name[0])  {
                putrest:
                        while  (ch != '\n' && ch != EOF)  {
                                putc(ch, dest);
                                ch = getc(src);
                        }
                        putc('\n', dest);
                        continue;
                }
                for  (match = 1;  name[match];  match++)  {
                        ch = getc(src);
                        if  (ch != name[match])
                                goto  nogood;
                }

                /* Zap any spaces after keyword */

                do  ch = getc(src);
                while  (ch == ' ' || ch == '\t');

                if  (ch == '=')  {              /* Got it! */
                        do  ch = getc(src);
                        while  (ch != '\n' && ch != EOF);
                        continue;
                }
        nogood:
                /* Didn't find it - copy the characters we did match */
                for  (nn = 0;  nn < match;  nn++)
                        putc(name[nn], dest);
                goto  putrest;
        }
}

static void  copyin(FILE *src, FILE *dest)
{
        int  ch;

        rewind(src);
        while  ((ch = getc(src)) != EOF)
                putc(ch, dest);
}

int     proc_save_opts(const char *direc, const char *varname, void (*fn)(FILE *, const char *))
{
        char    *fname;
        FILE    *ifl, *ofl;
        PIDTYPE pid;

        /* Gyrations to do everything with the right UID In some cases
           we could do things with chown but there are so many
           combinations we'll take the easy way out with a fork...  */

        if  ((pid = fork()) != 0)  {
                int     status;
                if  (pid < 0)
                        return  $E{saveopts cannot fork};
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                if  (status == 0)
                        return  0;
                if  (status & 0xff)  {
                        disp_arg[0] = status;
                        return  $E{saveopts crashed};
                }
                return  (status >> 8) + $E{saveopts file error};
        }

        /* This is what we wanted to achieve in the first place...  */

        setuid(Realuid);

        /* If we set the umask before, set it back */

        if  (Save_umask != 0xFFFF)
                umask(Save_umask);

        /* We now set direc to null to use the home directory version, which is moved
           to a .subdirectory so we aren't confused if we run from the home directory
           and end up reading the same file twice.
           Note that this code cheats by only allowing for one level of subdirectory */

        if  (direc)  {
                if  (!(fname = malloc((unsigned) (strlen(direc) + sizeof(USER_CONFIG) + 1))))
                        _exit($S{saveopts nomem});
                strcpy(fname, direc);
                strcat(fname, "/" USER_CONFIG);
        }
        else  {
                char    *dir = unameproc("~", ".", Realuid);
                if  (chdir(dir) < 0  ||  /* Change to home directory */
                     (chdir(HOME_CONFIG_DIR) < 0  &&  (mkdir(HOME_CONFIG_DIR, 0777) < 0 || chdir(HOME_CONFIG_DIR) < 0)))
                        _exit($S{saveopts cannot create});
                fname = HOME_CONFIG_FILE;
        }

        if  ((ifl = fopen(fname, "r")))  {
                struct  stat    sbuf;
                fstat(fileno(ifl), &sbuf);
                ofl = tmpfile();
                copyout(ifl, ofl, varname);
                fclose(ifl);
                if  (unlink(fname) < 0)
                        _exit($S{saveopts nodel});
                if  (!(ifl = fopen(fname, "w")))
                        _exit($S{saveopts no init and del});
                copyin(ofl, ifl);
#ifdef  HAVE_FCHMOD
                fchmod(fileno(ifl), sbuf.st_mode & ~S_IFMT);
#else
                chmod(fname, sbuf.st_mode & ~S_IFMT);
#endif
        }
        else  {

                if  (!(ifl = fopen(fname, "w")))
                        _exit($S{saveopts cannot create});
        }
        (*fn)(ifl, varname);
        fclose(ifl);
        _exit(0);
}
