/* makevec.c -- turn environment variable into command args

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.


 *----------------------------------------------------------------------
 *      Turn the argument, delimited by spaces into a vector of strings like
 *      the second argument to main. This is used to process an environment
 *      variable looking like command arguments.
 */

#include "config.h"
#include "incl_unix.h"

static  char    Filename[] = __FILE__;

#define INITV   20
#define INCV    5

char **makevec(const char *arg)
{
        char    *resv, **res, **ores;
        int     count, tot, rbits = 0, quote;

        ores = res = (char **)malloc((unsigned)(INITV * sizeof(char *)));
        resv = (char *)malloc((unsigned)(strlen(arg) + 1));
        tot = INITV;
        count = INITV - 1;
        if  (res == (char **) 0 || resv == (char *) 0)
                ABORT_NOMEM;
        strcpy(resv, arg);
        *res++ = resv;
        rbits++;

        for  (;;)  {
                quote = 0;
                while  (*resv == ' ')
                        resv++;
                if  (*resv == '\0')  {
                        *res = (char *) 0;
                        return  ores;
                }
                if  (*resv == '\'' || *resv == '\"')
                        quote = *resv++;
                *res++ = resv;
                rbits++;
                while  (*resv != '\0'  &&  (*resv != ' ' || quote)  &&  *resv != quote)
                        resv++;
                if  (*resv)
                        *resv++ = '\0';
                if  (--count <= 0)  {
                        tot += INCV;
                        count = INCV;
                        ores = (char **)realloc((char *) ores, (unsigned) tot * sizeof(char *));
                        if  (ores == (char **) 0)
                                ABORT_NOMEM;
                        res = &ores[rbits];
                }
        }
}
