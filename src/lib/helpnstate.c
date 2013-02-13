/* helpnstate.c -- get state numbers

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
#include <ctype.h>
#include "errnums.h"
#include "statenums.h"

int  helpnstate(const int current_state)
{
        int     ch, result, hadeof = 0;

        for  (;;)  {
                ch = getc(Cfile);
                if  (ch == EOF)  {
                        if  (hadeof)
                                return  0;      /* Invalid result */
                        hadeof++;
                        fseek(Cfile, 0L, 0);
                        continue;
                }

                /* If line doesn't start with a digit ignore it */

                if  (!isdigit(ch) && ch != '-')  {
                        while  (ch != '\n'  &&  ch != EOF)
                                ch = getc(Cfile);
                        continue;
                }

                /* Read leading state number.
                   If not current state forget it */

                ungetc(ch, Cfile);
                if  (helprdn() != current_state)  {
                        do  ch = getc(Cfile);
                        while  (ch != '\n' && ch != EOF);
                        continue;
                }
                ch = getc(Cfile);

                /* Only interested in 'N's */

                if  (ch == 'N'  ||  ch == 'n')  {
                        result = helprdn();
                        do  ch = getc(Cfile);
                        while  (ch != '\n'  &&  ch != EOF);
                        return  result;
                }
                while  (ch != '\n'  &&  ch != EOF)
                        ch = getc(Cfile);
        }
}
