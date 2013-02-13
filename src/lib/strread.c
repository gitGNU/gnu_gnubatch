/* strread.c -- read string into allocated memory.

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
#include "incl_unix.h"

static  char    Filename[] = __FILE__;

#define INITSIZE        80
#define INCSIZE         40

char    *strread(FILE *fp, const char *delim)
{
        int     ch, ccnt;
        unsigned  bufsize;
        char    *resbuf, initbuf[INITSIZE];

        ccnt = 0;
        do  {
                if  ((ch = getc(fp)) == EOF)
                        return  (char *) 0;
                if  (strchr(delim, ch) != (char *) 0)  {
                        initbuf[ccnt] = '\0';
                        return  stracpy(initbuf);
                }
                initbuf[ccnt] = (char) ch;
        }  while  (++ccnt < INITSIZE-1);

        /* Our fixed-size buffer wasn't enough.  Allocate another one.  */

        initbuf[ccnt] = '\0';
        bufsize = INITSIZE+INCSIZE;
        if  ((resbuf = malloc(bufsize)) == (char *) 0)
                ABORT_NOMEM;
        strcpy(resbuf, initbuf);

        for  (;;)  {
                if  ((ch = getc(fp)) == EOF)  {
                        free(resbuf);
                        return  (char *) 0;
                }
                if  (strchr(delim, ch) != (char *) 0)  {
                        resbuf[ccnt] = '\0';
                        return  resbuf;
                }
                resbuf[ccnt] = (char) ch;
                if  (++ccnt >= bufsize)  {
                        bufsize += INCSIZE;
                        if  ((resbuf = realloc(resbuf, bufsize)) == 0)
                                ABORT_NOMEM;
                }
        }
}
