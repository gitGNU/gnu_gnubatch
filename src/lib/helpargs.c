/* helpargs.c -- Dynamic specification of command arguments.

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
#include "incl_unix.h"
#include "helpargs.h"
#include "errnums.h"
#include "statenums.h"
#include "ecodes.h"

static  char    Filename[] = __FILE__;

HelpargRef  helpargs(const Argdefault *defaults, const int mins, const int maxs)
{
        HelpargRef      result;
        HelpargkeyRef   rp, *rpp;
        int     ch, staten, errs = 0, someset = 0, whatv;

        if  ((result = (HelpargRef) malloc(sizeof(Helparg) * (ARG_ENDV - ARG_STARTV + 1))) == (HelpargRef) 0)
                ABORT_NOMEM;

        /* Initialise to null */

        for  (ch = 0;  ch < ARG_ENDV - ARG_STARTV + 1;  ch++)  {
                result[ch].mult_chain = (HelpargkeyRef) 0;
                result[ch].value = 0;
        }

        /* We only use defaults if we have to.  */

        fseek(Cfile, 0L, 0);

        for  (;;)  {
                ch = getc(Cfile);

                if  (ch != HELPLETTER  &&  ch != (HELPLETTER - 'A' + 'a'))  {
                skipn:
                        while  (ch != '\n'  &&  ch != EOF)
                                ch = getc(Cfile);
                        if  (ch == EOF)
                                break;
                        continue;
                }

                /* Had initial 'P' (or 'p')
                   Expecting numeric to follow.  */

                ch = getc(Cfile);
                if  (!isdigit(ch)  &&  ch != '-')
                        goto  skipn;

                /* Put back character and read the number */

                ungetc(ch, Cfile);
                staten = helprdn();
                if  (staten < mins  ||  staten > maxs)
                        goto  skipn;

                /* If we don't find a ":", discard it */

                ch = getc(Cfile);
                if  (ch != ':')
                        goto  skipn;

                /* If it could be part of a name, then explore that */

                do      {

                        ch = getc(Cfile);

                        /* Must be valid starting char */

                        if  (ch < ARG_STARTV || ch > ARG_ENDV || ch == ',')
                                goto  skipn;

                        if  (isalnum(ch))  {
                                char    inbuf[MAXARGNAME];
                                int     pos = 0, startchar;

                                startchar = ch;
                                do  {
                                        if  (pos < MAXARGNAME - 1)
                                                inbuf[pos++] = tolower(ch);
                                        ch = getc(Cfile);
                                }  while  (isalnum(ch) || ch == '_' || ch == '-');

                                /* Check valid termination, ignore
                                   rest of line if not.  */

                                if  (!isspace(ch) && ch != ',' && ch != '#')
                                        goto  skipn;

                                inbuf[pos] = '\0';

                                /* If only one char, go on to single
                                   char case */

                                if  (pos <= 1)  {
                                        ungetc(ch, Cfile);
                                        ch = startchar;
                                        goto  singchar;
                                }

                                /* Search result chain for keyword
                                   Ignore duplicates but complain
                                   about different state codes.  */

                                rpp = &result[startchar - ARG_STARTV].mult_chain;
                                for  (;  (rp = *rpp);  rpp = &rp->next)  {
                                        if  (strcmp(rp->chars, inbuf) == 0)  {
                                                if  (staten != rp->value)  {
                                                        disp_str = rp->chars;
                                                        disp_arg[0] = rp->value;
                                                        disp_arg[1] = staten;
                                                        print_error($E{argument keyword error});
                                                        errs++;
                                                }
                                                goto  skip_alloc;
                                        }
                                }

                                if  ((rp = (HelpargkeyRef) malloc(sizeof(Helpargkey))) == (HelpargkeyRef) 0)
                                        ABORT_NOMEM;

                                /* Put on end of chain */

                                rp->chars = stracpy(inbuf);
                                rp->value = staten;
                                rp->next = (HelpargkeyRef) 0;

                                *rpp = rp;
                                someset++;
                        }
                        else  {
                                /* Manoeuvre to escape a , */

                                if  (ch == '\\')  {
                                        ch = getc(Cfile);
                                        if  (ch < ARG_STARTV || ch > ARG_ENDV)
                                                goto  skipn;
                                }
                        singchar:
                                whatv = result[ch - ARG_STARTV].value;

                                if  (whatv != 0  &&  whatv != staten)  {
                                        disp_arg[0] = whatv;
                                        disp_arg[1] = staten;
                                        disp_arg[2] = ch;
                                        print_error($E{argument option error});
                                        errs++;
                                }
                                result[ch - ARG_STARTV].value = staten;
                                ch = getc(Cfile);
                                someset++;
                        }

                skip_alloc:
                        ;
                }  while  (ch == ',');

                /* Skip white space */

                while  (ch == ' '  ||  ch == '\t')
                        ch = getc(Cfile);

                /* Allow comments and skip */

                if  (ch == '#')  {
                        do  ch = getc(Cfile);
                        while  (ch != '\n'  && ch != EOF);
                }
        }

        /* Return if we had errors.  We should really deallocate
           everything but we are about to exit so why bother */

        if  (errs > 0)  {
                disp_arg[0] = errs;
                print_error($E{arg conflict abort});
                exit(E_BADCFILE);
        }

        /* If nothing got set use the default vector */

        if  (someset <= 0)
                while  (defaults->letter)  {
                        result[defaults->letter - ARG_STARTV].value = defaults->value;
                        defaults++;
                }

        return  result;
}

void  freehelpargs(HelpargRef he)
{
        int     ch;
        HelpargkeyRef   kv, nkv;

        for  (ch = 0;  ch < ARG_ENDV - ARG_STARTV + 1;  ch++)
                if  ((kv = he[ch].mult_chain))
                        do  {
                                nkv = kv->next;
                                free((char *) kv);
                        }  while  ((kv = nkv));
        free((char *) he);
}
