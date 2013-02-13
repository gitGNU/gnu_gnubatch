/* qmatch.c -- glob-style pattern matching with , alternatives

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
#include "incl_unix.h"

static int  ematch(char *pattern, char *value)
{
        int     cnt, nott;
        char    *cp;

        for  (;;)  {

                switch  (*pattern)  {
                case  '\0':
                        if  (*value == '\0' || *value == ':')
                                return  1;
                        return  0;

                default:
                        if  (*pattern != *value)
                                return  0;
                        pattern++;
                        value++;
                        continue;

                case  '?':
                        if  (*value == '\0' ||  *value == ':')
                                return  0;
                        pattern++;
                        value++;
                        continue;

                case  '*':
                        pattern++;
                        cp = strchr(value, ':');
                        for  (cnt = cp? cp - value: strlen(value); cnt >= 0;  cnt--)
                                if  (ematch(pattern, value+cnt))
                                        return  1;
                        return  0;

                case  '[':
                        if  (*value == '\0' ||  *value == ':')
                                return  0;
                        nott = 0;
                        if  (*++pattern == '!')  {
                                nott = 1;
                                pattern++;
                        }

                        /* Safety in case pattern truncated */

                        if  (*pattern == '\0')
                                return  0;

                        do  {
                                int  lrange, hrange;

                                /* Initialise limits of range */

                                lrange = hrange = *pattern++;
                                if  (*pattern == '-')  {
                                        hrange = *++pattern;

                                        if  (hrange == 0) /* Safety in case trunacated */
                                                return  0;

                                        /* Be relaxed about backwards ranges */

                                        if  (hrange < lrange)  {
                                                int     tmp = hrange;
                                                hrange = lrange;
                                                lrange = tmp;
                                        }
                                        pattern++; /* Past rhs of range */
                                }

                                /* If value matches, and we are excluding range, then pattern
                                   doesn't and we quit. Otherwise we skip to the end.  */

                                if  (*value >= lrange  &&  *value <= hrange)  {
                                        if  (nott)
                                                return  0;
                                        while  (*pattern  &&  *pattern != ']')
                                                pattern++;
                                        if  (*pattern == '\0') /* Safety */
                                                return  0;
                                        pattern++;
                                        goto  endpat;
                                }

                        }  while  (*pattern  &&  *pattern != ']');

                        if  (*pattern == '\0') /* Safety */
                                return  0;

                        while  (*pattern++ != ']')
                                ;
                        if  (!nott)
                                return  0;
                endpat:
                        value++;
                        continue;
                }
        }
}

int  qmatch(char *pattern, char *value)
{
        int     res;
        char    *cp;

        do  {
                /* Allow for ,-separated alternatives.  There isn't a
                   potential bug here with [,.] because we only
                   allow names with alphanumerics and _ in.  */

                cp = strchr(pattern, ',');
                if  (cp)  {
                        *cp = '\0';
                        res = ematch(pattern, value);
                        *cp = ',';
                        pattern = cp + 1;
                }
                else
                        res = ematch(pattern, value);
                if  (res)
                        return  1;
        }  while  (cp);

        /* Not found...  */

        return  0;
}
