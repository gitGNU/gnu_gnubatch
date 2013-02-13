/* helpprmpt.c -- Get prompt for given state from help file

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
#include "errnums.h"
#include "statenums.h"

char *helpprmpt(const int current_state)
{
        int     ch, perc;
        long    lasttime = ftell(Cfile);

        for  (;;)  {
                ch = getc(Cfile);
                if  (ch == EOF)  {
                        /* Contortions as prompts frequently follow each other */
                        if  (lasttime <= 0L)
                                return  (char *) 0;
                        fseek(Cfile, 0L, 0);
                        lasttime = 0L;
                        continue;
                }

                /* If line doesn't start with a digit ignore it */

                if  ((ch < '0' || ch > '9') && ch != '-')  {
skipn:                  while  (ch != '\n' && ch != EOF)
                                ch = getc(Cfile);
                        continue;
                }

                /* Read leading state number
                   If not current state forget it */

                ungetc(ch, Cfile);
                if  (helprdn() != current_state)  {
                        do  ch = getc(Cfile);
                        while  (ch != '\n' && ch != EOF);
                        continue;
                }
                ch = getc(Cfile);

                /* If it's not a 'p' ignore it.  */

                if  (ch != 'p' && ch != 'P')
                        goto  skipn;

                if  ((ch = getc(Cfile)) != ':')
                        goto  skipn;

                return  help_readl(&perc);
        }
}
