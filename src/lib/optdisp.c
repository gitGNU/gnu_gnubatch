/* optdisp.c -- option handling related to display options

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
#include "incl_unix.h"
#include "helpargs.h"
#include "optflags.h"

char    *Restru,                                /* Define these here */
        *Restrg;

EOPTION(o_incnull)
{
        Dispflags &= ~DF_SUPPNULL;
        return  OPTRESULT_OK;
}

EOPTION(o_noincnull)
{
        Dispflags |= DF_SUPPNULL;
        return  OPTRESULT_OK;
}

EOPTION(o_localonly)
{
        Dispflags |= DF_LOCALONLY;
        return  OPTRESULT_OK;
}

EOPTION(o_nolocalonly)
{
        Dispflags &= ~DF_LOCALONLY;
        return  OPTRESULT_OK;
}

EOPTION(o_scrkeep)
{
        Dispflags |= DF_SCRKEEP;
        return  OPTRESULT_OK;
}

EOPTION(o_noscrkeep)
{
        Dispflags &= ~DF_SCRKEEP;
        return  OPTRESULT_OK;
}

EOPTION(o_confabort)
{
        Dispflags |= DF_CONFABORT;
        return  OPTRESULT_OK;
}

EOPTION(o_noconfabort)
{
        Dispflags &= ~DF_CONFABORT;
        return  OPTRESULT_OK;
}

EOPTION(o_justu)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (Restru)
                free(Restru);
        Restru = (arg[0] && (arg[0] != '-' || arg[1]))? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_justg)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (Restrg)
                free(Restrg);
        Restrg = (arg[0] && (arg[0] != '-' || arg[1]))? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_jobqueue)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        Anychanges |= OF_ANY_DOING_SOMETHING | OF_ANY_DOING_CHANGE;
        Procparchanges |= OF_JOBQUEUE_CHANGES;
        if  (jobqueue)
                free(jobqueue);
        jobqueue = (arg[0] && (arg[0] != '-' || arg[1])) ? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}

EOPTION(o_helpclr)
{
        Dispflags |= DF_HELPCLR;
        return  OPTRESULT_OK;
}

EOPTION(o_nohelpclr)
{
        Dispflags &= ~DF_HELPCLR;
        return  OPTRESULT_OK;
}

EOPTION(o_helpbox)
{
        Dispflags |= DF_HELPBOX;
        return  OPTRESULT_OK;
}

EOPTION(o_nohelpbox)
{
        Dispflags &= ~DF_HELPBOX;
        return  OPTRESULT_OK;
}

EOPTION(o_errbox)
{
        Dispflags |= DF_ERRBOX;
        return  OPTRESULT_OK;
}

EOPTION(o_noerrbox)
{
        Dispflags &= ~DF_ERRBOX;
        return  OPTRESULT_OK;
}

EOPTION(o_header)
{
        Dispflags |= DF_HAS_HDR;
        return  OPTRESULT_OK;
}

EOPTION(o_noheader)
{
        Dispflags &= ~DF_HAS_HDR;
        return  OPTRESULT_OK;
}

EOPTION(o_freezecd)
{
        Anychanges |= OF_ANY_FREEZE_WANTED | OF_ANY_FREEZE_CURRENT;
        return  OPTRESULT_OK;
}

EOPTION(o_freezehd)
{
        Anychanges |= OF_ANY_FREEZE_WANTED | OF_ANY_FREEZE_HOME;
        return  OPTRESULT_OK;
}
