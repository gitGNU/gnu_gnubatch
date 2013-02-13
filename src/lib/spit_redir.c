/* spit_redir.c -- output redirections

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
#include "defaults.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"

void  spit_redir(FILE *dest, const unsigned fd, const unsigned action, const unsigned arg, const char *buff)
{
        putc(' ', dest);
        switch  (action)  {
        case  RD_ACT_RD:
                if  (fd != 0)
                        fprintf(dest, "%u", fd);
                putc('<', dest);
                break;
        case  RD_ACT_WRT:
                if  (fd != 1)
                        fprintf(dest, "%u", fd);
                putc('>', dest);
                break;
        case  RD_ACT_APPEND:
                if  (fd != 1)
                        fprintf(dest, "%u", fd);
                fputs(">>", dest);
                break;
        case  RD_ACT_RDWR:
                if  (fd != 0)
                        fprintf(dest, "%u", fd);
                fputs("<>", dest);
                break;
        case  RD_ACT_RDWRAPP:
                if  (fd != 0)
                        fprintf(dest, "%u", fd);
                fputs("<>>", dest);
                break;
        case  RD_ACT_PIPEO:
                if  (fd != 1)
                        fprintf(dest, "%u", fd);
                putc('|', dest);
                break;
        case  RD_ACT_PIPEI:
                if  (fd != 0)
                        fprintf(dest, "%u", fd);
                fputs("<|", dest);
                break;
        case  RD_ACT_CLOSE:
                if  (fd != 1)
                        fprintf(dest, "%u", fd);
                fputs(">&-", dest);
                return;
        case  RD_ACT_DUP:
                if  (fd != 1)
                        fprintf(dest, "%u", fd);
                fprintf(dest, ">&%u", arg);
                return;
        }
        fprintf(dest, "%s", buff);
}
