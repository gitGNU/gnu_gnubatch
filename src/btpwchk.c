/* btpwchk.c -- verify passwords

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
#ifdef  SHADOW_PW
#include <shadow.h>
#endif
#include <pwd.h>
#include "defaults.h"
#include "incl_unix.h"
#include "files.h"

FILE    *Cfile;                                 /* Need definition of this here */

/* I don't know where to find this routine in the general case but I
   hope that this will work instead of groping around zillions of
   includes */

char *crypt(const char *, const char *);

void  nomem(const char *a, const int b)
{
        exit(255);
}

FILE *open_pwfile()
{
        char    *fname = envprocess(XIPWFILE);
        FILE    *pwf = fopen(fname, "r");
        free(fname);
        return  pwf;
}

char *get_pwfile(FILE *pwf, const char *nam)
{
        char    buf[120];

        while  (fgets(buf, sizeof(buf), pwf))  {
                int     lng = strlen(buf) - 1;
                char    *cp;
                if  (buf[lng] == '\n')
                        buf[lng] = '\0';
                cp = strchr(buf, ':');
                if  (!cp)
                        continue;
                *cp = '\0';
                if  (ncstrcmp(nam, buf) == 0)
                        return  stracpy(cp+1);
        }
        return  (char *) 0;
}

/* Take a user name as 1st argument and a password as standard input
   (not 2nd argument!!)  If OK return '0' on std output.  If
   anything wrong at all return non zero.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *username = argv[1], *pw;
        FILE    *xipwf;
        int     lng;
        char    inbuf[80];
#ifdef  SHADOW_PW
        struct  spwd    *pwe;
#else
        struct  passwd  *pwe;
#endif

        versionprint(argv, "$Revision: 1.9 $", 1);

        init_mcfile();

        if  (argc != 2)  {
                putchar('2');
                return  2;
        }

        if  (!fgets(inbuf, sizeof(inbuf), stdin))  {
                putchar('3');
                return  3;
        }

        lng = strlen(inbuf) - 1;
        if  (inbuf[lng] == '\n')
                inbuf[lng] = '\0';

        if  ((xipwf = open_pwfile()))  {
                pw = get_pwfile(xipwf, username);
                fclose(xipwf);
                if  (!pw)  {
                        putchar('5');
                        return  5;
                }
        }
        else  {
#ifdef  SHADOW_PW
                if  (!(pwe = getspnam(username)))  {
                        putchar('4');
                        return  4;
                }
                pw = pwe->sp_pwdp;
#else
                if  (!(pwe = getpwnam(username)))  {
                        putchar('4');
                        return  4;
                }
                pw = pwe->pw_passwd;
#endif
        }

        if  (strcmp(pw, crypt(inbuf, pw)) == 0)  {
                putchar('0');
                return  0;
        }
        else  {
                putchar('1');
                return  1;
        }
}
