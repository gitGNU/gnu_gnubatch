/* packjstring.c -- pack strings into job structure

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
#include "incl_unix.h"
#include "defaults.h"
#include "btmode.h"
#include "timecon.h"
#include "btconst.h"
#include "bjparam.h"
#include "btjob.h"

/* Pack up job strings into space in job structure.  Return 1 if ok 0
   if we run out of space.  The numbers of strings are assumed to
   be correct in "jp" */

int  packjstring(BtjobRef jp, const char *dp, const char *tp, MredirRef rp, MenvirRef ep, char **ap)
{
        unsigned  offset = 0;
        char    *next = &jp->bj_space[0];
        unsigned  cnt;

        /* Arguments */

        if  (jp->h.bj_nargs != 0)  {
                JargRef argblk;
                unsigned  lng = jp->h.bj_nargs * sizeof(Jarg);

                if  (offset + lng >= JOBSPACE)
                        return  0;
                jp->h.bj_arg = (SHORT) offset;
                argblk = (JargRef) next;
                next += lng;
                offset += lng;
                for  (cnt = 0;  cnt < jp->h.bj_nargs;  cnt++)  {
                        const  char     *arg = *ap++;
                        lng = strlen(arg) + 1;
                        if  (offset + lng >= JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, arg, lng);
                        argblk[cnt] = (Jarg) offset;
                        next += lng;
                        offset += lng;
                }
        }

        /* Environment */

        if  (jp->h.bj_nenv != 0)  {
                EnvirRef        envblk;
                unsigned  lng   = jp->h.bj_nenv * sizeof(Envir);
                unsigned  roundit = sizeof(LONG) - (offset & (sizeof(LONG) - 1));

                offset += roundit;
                next += roundit;
                if  (offset + lng >= JOBSPACE)
                        return  0;
                jp->h.bj_env = (SHORT) offset;
                envblk = (EnvirRef) next;
                next += lng;
                offset += lng;

                for  (cnt = 0;  cnt < jp->h.bj_nenv;  cnt++)  {
                        CMenvirRef  me = ep++;
                        unsigned  lngn = strlen(me->e_name) + 1;
                        unsigned  lngv = strlen(me->e_value) + 1;

                        if  (offset + lngn + lngv >= JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, me->e_name, lngn);
                        envblk[cnt].e_name = (USHORT) offset;
                        next += lngn;
                        offset += lngn;
                        BLOCK_COPY(next, me->e_value, lngv);
                        envblk[cnt].e_value = (USHORT) offset;
                        next += lngv;
                        offset += lngv;
                }
        }

        /* Redirections */

        if  (jp->h.bj_nredirs != 0)  {
                RedirRef        redirblk;
                unsigned  lng   = jp->h.bj_nredirs * sizeof(Redir);
                unsigned  roundit = sizeof(LONG) - (offset & (sizeof(LONG) - 1));

                offset += roundit;
                next += roundit;
                if  (offset + lng >= JOBSPACE)
                        return  0;
                jp->h.bj_redirs = (SHORT) offset;
                redirblk = (RedirRef) next;
                next += lng;
                offset += lng;
                for  (cnt = 0;  cnt < jp->h.bj_nredirs;  cnt++)  {
                        CMredirRef  mr = rp++;
                        RedirRef  res = &redirblk[cnt];

                        if  (mr->action < RD_ACT_CLOSE)  {
                                lng = strlen(mr->un.buffer) + 1;
                                if  (offset + lng >= JOBSPACE)
                                        return  0;
                                BLOCK_COPY(next, mr->un.buffer, lng);
                                res->arg = (USHORT) offset;
                                next += lng;
                                offset += lng;
                        }
                        else
                                res->arg = mr->un.arg;
                        res->action = mr->action;
                        res->fd = mr->fd;
                }
        }

        if  (dp)  {
                unsigned  lng = strlen(dp) + 1;
                if  (offset + lng >= JOBSPACE)
                        return  0;
                jp->h.bj_direct = (SHORT) offset;
                BLOCK_COPY(next, dp, lng);
                next += lng;
                offset += lng;
        }
        else
                jp->h.bj_direct = -1;

        if  (tp)  {
                unsigned  lng = strlen(tp) + 1;
                if  (offset + lng >= JOBSPACE)
                        return  0;
                jp->h.bj_title = (SHORT) offset;
                BLOCK_COPY(next, tp, lng);
                next += lng;
                offset += lng;
        }
        else
                jp->h.bj_title = -1;

        return  1;
}

/* Repack changed bits of a job */

int  repackjob(BtjobRef new, CBtjobRef old, const char *dp, const char *tp, const unsigned nr, const unsigned ne, const unsigned na, MredirRef rp, MenvirRef ep, char **ap)
{
        Mredir  reds[MAXJREDIRS];
        Menvir  envs[MAXJENVIR];
        char    *args[MAXJARGS];

        if  (!dp && old->h.bj_direct >= 0)
                dp = &old->bj_space[old->h.bj_direct];
        if  (!tp && old->h.bj_title >= 0)
                tp = &old->bj_space[old->h.bj_title];
        if  (rp)
                new->h.bj_nredirs = (USHORT) nr;
        else  {
                unsigned  cnt;
                for  (cnt = 0;  cnt < old->h.bj_nredirs;  cnt++)  {
                        CRedirRef       orp = REDIR_OF(old, cnt);
                        reds[cnt].fd = orp->fd;
                        if  ((reds[cnt].action = orp->action) >= RD_ACT_CLOSE)
                                reds[cnt].un.arg = orp->arg;
                        else
                                reds[cnt].un.buffer = (char *) &old->bj_space[orp->arg];
                }
                rp = reds;
                new->h.bj_nredirs = old->h.bj_nredirs;
        }
        if  (ep)
                new->h.bj_nenv = (USHORT) ne;
        else   {
                unsigned  cnt;
                for  (cnt = 0;  cnt < old->h.bj_nenv;  cnt++)
                        ENV_OF(old, cnt, envs[cnt].e_name, envs[cnt].e_value);
                ep = envs;
                new->h.bj_nenv = old->h.bj_nenv;
        }
        if  (ap)
                new->h.bj_nargs = (USHORT) na;
        else  {
                unsigned  cnt;
                for  (cnt = 0;  cnt < old->h.bj_nargs;  cnt++)
                        args[cnt] = (char *) ARG_OF(old, cnt);
                ap = args;
                new->h.bj_nargs = old->h.bj_nargs;
        }

        return  packjstring(new, dp, tp, rp, ep, ap);
}

const char *title_of(CBtjobRef jp)
{
        return  jp->h.bj_title < 0? "": &jp->bj_space[jp->h.bj_title];
}

void  unpackenv(BtjobRef jp, unsigned cnt, char **namep, char **valp)
{
        EnvirRef  ep = &((EnvirRef) &jp->bj_space[jp->h.bj_env])[cnt];
        *namep = &jp->bj_space[ep->e_name];
        *valp = &jp->bj_space[ep->e_value];
}
