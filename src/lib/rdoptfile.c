/* rdoptfile.c -- look in user's .gnubatch files

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include "incl_unix.h"
#include "defaults.h"
#include "errnums.h"
#include "files.h"

static  char    Filename[] = __FILE__;

#ifndef MALLINC
#define MALLINC 64
#endif

static  FILE    *fid;
static  char    *last_file;             /* Remember last file name */

void  close_optfile()
{
        if  (fid != (FILE *) 0)  {
                fclose(fid);
                fid = (FILE *) 0;
        }
        if  (last_file)  {
                free(last_file);
                last_file = (char *) 0;
        }
}

/* Read option file looking for a line beginning with <keyword>=
   Allocate memory for and return the result.  This memory SHOULD
   BE FREED IN THE CALLING ROUTINE when the result is no longer
   needed.  */

char *rdoptfile(const char *file, const char *keyword)
{
        const   char    *inp;
        char    *outp, *result;
        int     ch, outlen;

        /* If it was the last file we read, just rewind it */

        if  (fid  &&  last_file  &&  strcmp(file, last_file) == 0)
                rewind(fid);
        else  {
                close_optfile();
                if  (!(fid = fopen(file, "r")))
                        return  (char *)  0;
                fcntl(fileno(fid), F_SETFD, 1);         /* Close on exec */
                last_file = stracpy(file);
        }

        /* Now look for keyword */

        outlen = MALLINC;
        if  ((result = malloc((unsigned) MALLINC)) == (char *) 0)
                ABORT_NOMEM;

        for  (;;)  {
                ch = getc(fid);
                switch  (ch)  {
                case  EOF:
                        free(result);
                        return  (char *) 0;
                case  '\n':
                case  '\t':
                case  ' ':
                        continue;
                case  '#':              /*  Comment  */
                skipl:
                        while  (ch != '\n' && ch != EOF)
                                ch = getc(fid);
                        continue;
                default:
                        if  (ch != *keyword)
                                goto  skipl;

                        /* Match up rest of keyword */

                        for  (inp = keyword + 1; *inp;  inp++)  {
                                ch = getc(fid);
                                if  (ch != *inp)
                                        goto  skipl;
                        }

                        /* Skip white space after keyword Check for = */

                        do  ch = getc(fid);
                        while  (ch == ' ' || ch == '\t');
                        if  (ch != '=')
                                goto  skipl;

                        /* Accumulate chars of keyword, NOT skipping leading white space */

                        outp = result;
                        ch = getc(fid);         /* First char following = */

                        do  {
                                if  (outp - result >= outlen - 1) {
                                        int displ = outp - result;
                                        outlen += MALLINC;
                                        if  ((result = realloc(result, (unsigned) outlen))
                                                        == (char *) 0)
                                                ABORT_NOMEM;
                                        outp = result + displ;
                                }
                                *outp++ = (char) ch;
                                ch = getc(fid);
                        }  while  (ch != '\n'  &&  ch != EOF);

                        *outp = '\0';
                        return  result;
                }
        }
}
