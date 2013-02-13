/* doopts.c -- option argument processing

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
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include "incl_unix.h"
#include "helpargs.h"
#include "errnums.h"
#include "ecodes.h"

/* Put this here to resolve references in shared libraries.
   Some of these ought to be moved in due course.  */

ULONG   Anychanges, Procparchanges, Condasschanges, Timechanges, Dispflags;
int     arg_errnum;

char **doopts(char **argv, HelpargRef Adesc, optparam *const optlist, const int minstate)
{
        char    *arg;
        int             ad, rc;
        HelpargkeyRef   ap;

 nexta:
        for  (;;)  {
                arg = *++argv;
                if  (arg == (char *) 0 || (*arg != '-' && *arg != '+'))
                        return  argv;

                if  (*arg == '-')  {

                        /* Treat -- as alternative to + to start keywords
                           or -- on its own as end of arguments */

                        if  (*++arg == '-')  {
                                if  (*++arg)
                                        goto  keyw_arg;
                                return  ++argv;
                        }

                        /* Past initial '-', argv still on whole argument */

                        while  (*arg >= ARG_STARTV)  {
                                ad = Adesc[*arg - ARG_STARTV].value;
                                if  (ad == 0  ||  ad < minstate)  {
                                        disp_str = *argv;
                                        print_error($E{program arg error});
                                        exit(E_USAGE);
                                }

                                /* Each function returns:
                                   1 (OPTRESULT_ARG_OK)
                                        if it eats the argument and it's OK
                                   2 (OPTRESULT_LAST_ARG_OK)
                                        ditto but the argument must be last
                                   0 (OPTRESULT_OK)
                                        if it ignores the argument.
                                  -1 (OPTRESULT_MISSARG) if no arg and one reqd
                                  -2 (OPTRESULT_ERROR) if something is wrong
                                        error code in arg_errnum. */

                                if  (!*++arg)  { /* No trailing stuff after arg letter */
                                        disp_str = argv[1];     /* Saves doing it later */

                                        if  ((rc = (optlist[ad - minstate])(argv[1])) < OPTRESULT_OK)  {
                                                if  (rc == OPTRESULT_MISSARG)  {
                                                        disp_str = *argv;
                                                        print_error($E{program opt expects arg});
                                                }
                                                else
                                                        print_error(arg_errnum);
                                                exit(E_USAGE);
                                        }
                                        if  (rc > OPTRESULT_OK)  { /* Eaten the next arg */
                                                if  (rc > OPTRESULT_ARG_OK) /* Last stop at once */
                                                        return  argv;
                                                argv++;
                                        }
                                        goto  nexta;
                                }

                                /* Trailing stuff after arg letter, we incremented to it */

                                disp_str = arg;
                                if  ((rc = (optlist[ad - minstate])(arg)) > OPTRESULT_OK)  { /* Eaten */
                                        if  (rc > OPTRESULT_ARG_OK)     /* Last of its kind */
                                                return  argv;           /* Point to "this" */
                                        goto  nexta;
                                }

                        }
                        continue;
                }

                arg++;          /* Increment past '+' */

        keyw_arg:

                for  (ap = Adesc[tolower(*arg) - ARG_STARTV].mult_chain;  ap;  ap = ap->next)
                        if  (ncstrcmp(arg, ap->chars) == 0)
                                goto  found;

                disp_str = arg;
                print_error($E{program arg bad string});
                exit(E_USAGE);

        found:

                disp_str = argv[1]; /* Saves doing it later */
                if  ((rc = (optlist[ap->value - minstate])(argv[1])) < OPTRESULT_OK)  {
                        if  (rc == OPTRESULT_MISSARG)  {
                                disp_str = arg;
                                print_error($E{program opt expects arg});
                        }
                        else
                                print_error(arg_errnum); /* Routine set up disp_str etc */
                        exit(E_USAGE);
                }

                if  (rc > OPTRESULT_OK)  {                      /* Eaten */
                        if  (rc > OPTRESULT_ARG_OK)             /* The end */
                                return  argv;
                        argv++;
                }
        }
}
