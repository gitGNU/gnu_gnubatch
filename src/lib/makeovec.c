/* makeovec.c -- turn options to vector

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
#include "helpargs.h"

struct  optv    optvec[MAX_ANY_ARGS];

void  makeoptvec(const HelpargRef ha, const int firstarg, const int lastarg)
{
        int     i, v;
        HelpargkeyRef   hk;

        for  (i = 0;  i < ARG_ENDV - ARG_STARTV + 1;  i++)  {
                v = ha[i].value - firstarg;
                if  (v >= 0 && v <= lastarg - firstarg)  {
                        optvec[v].isplus = 0;
                        optvec[v].aun.letter = i + ARG_STARTV;
                }
                for  (hk = ha[i].mult_chain;  hk;  hk = hk->next)  {
                        v = hk->value - firstarg;
                        if  (v >= 0  &&  v <= lastarg - firstarg  &&  !optvec[v].isplus  &&  optvec[v].aun.letter == 0)  {
                                optvec[v].isplus = 1;
                                optvec[v].aun.string = hk->chars;
                        }
                }
        }
}
