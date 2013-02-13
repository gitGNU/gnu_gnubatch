/* helprdalt.c -- read alternatives from help file

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
#include "helpalt.h"
#include "errnums.h"
#include "statenums.h"

static  char    Filename[] = __FILE__;

#define INITALTS        10
#define INCALTS         5

void  freealts(HelpaltRef a)
{
        int     i;

        for  (i = 0;  i < a->numalt;  i++)
                free(a->list[i]);
        free((char *) a->alt_nums);
        free((char *) a->list);
        free((char *) a);
}

/* Read a list of alternatives from the configuration file.
   These look like:

        <state number>A[D]<alt number>:Text

   The order they come in is the order they will be put out.
   The D signifies that the specified alternative is the default. */

HelpaltRef  helprdalt(const int current_state)
{
        int     ch, perc = 0, maxalts, nres, lastnum = -1;
        HelpaltRef      result;

        if  ((result = (HelpaltRef) malloc(sizeof(Helpalt))) == (HelpaltRef) 0)
                ABORT_NOMEM;

        result->numalt = 0;
        result->def_alt = -1;
        if  ((result->alt_nums = (SHORT *) malloc(sizeof(SHORT) * INITALTS)) == (SHORT *) 0)
                ABORT_NOMEM;
        if  ((result->list = (char **) malloc(sizeof(char *) * INITALTS)) == (char **) 0)
                ABORT_NOMEM;

        maxalts = INITALTS;

        fseek(Cfile, 0L, 0);

        for  (;;)  {
                ch = getc(Cfile);
                if  (ch == EOF)
                        break;

                /* If line doesn't start with a number ignore it */

                if  ((ch < '0' || ch > '9') && ch != '-')  {
skipn:                  while  (ch != '\n')
                                ch = getc(Cfile);
                        continue;
                }

                /* Read leading state number.
                   If not current state forget it */

                ungetc(ch, Cfile);
                if  (helprdn() != current_state)  {
skipr:                  do  ch = getc(Cfile);
                        while  (ch != '\n' && ch != EOF);
                        continue;
                }
                ch = getc(Cfile);

                /* Is it alternative spec - if not forget it */

                if  (ch != 'a' && ch != 'A')
                        goto  skipr;

                if  (result->numalt >= maxalts)  {
                        maxalts += INCALTS;
                        if  ((result->alt_nums = (SHORT *) realloc((char *) result->alt_nums, (unsigned) (sizeof(SHORT) * maxalts))) == (SHORT *) 0)
                                ABORT_NOMEM;
                        if  ((result->list = (char **) realloc((char *) result->list, (unsigned) (sizeof(char *) * maxalts))) == (char **) 0)
                                ABORT_NOMEM;
                }

                nres = result->numalt;
                ch = getc(Cfile);

                /* Mark as default?  */

                if  (ch == 'd' || ch == 'D')  {
                        result->def_alt = (SHORT) nres;
                        ch = getc(Cfile);
                }

                /* If not followed by number use last number + 1 */

                ungetc(ch, Cfile);
                if  ((ch < '0' || ch > '9') && ch != '-')
                        lastnum++;
                else
                        lastnum = helprdn();

                result->alt_nums[nres] = (SHORT) lastnum;

                if  ((ch = getc(Cfile)) != ':')  {
                        result->def_alt = -1;   /*  might have been set */
                        goto  skipn;
                }

                /* Read actual line.
                   Ignore percent flags. */

                result->list[nres] = help_readl(&perc);
                result->numalt++;
        }

        /* If nothing read, zap result.  */

        if  (result->numalt <= 0)  {
                freealts(result);
                return  (HelpaltRef) 0;
        }
        return  result;
}
