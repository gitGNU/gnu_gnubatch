/* gbatch_strings.c -- API function for string handling

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

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include "gbatch.h"
#include "xbapi_int.h"
#include "incl_unix.h"
#include "incl_net.h"

extern struct api_fd *gbatch_look_fd(const int);

/* Get routines */

const char *gbatch_gettitle(const int fd, const apiBtjob *job)
{
        const   char            *result;
        unsigned                lng;
        struct  api_fd          *fdp;

        if  (job->h.bj_title < 0)
                return  "";
        result = &job->bj_space[job->h.bj_title];

        fdp = gbatch_look_fd(fd);
        if  (!fdp  ||  !fdp->queuename)
                return  result;

        lng = strlen(fdp->queuename);
        if  (strncmp(result, fdp->queuename, lng) == 0 && result[lng] == ':')
                return  &result[lng+1];
        return  result;
}

const char *gbatch_getdirect(const apiBtjob *job)
{
        return  job->h.bj_direct >= 0? &job->bj_space[job->h.bj_direct] : "/";
}

const char *gbatch_getarg(const apiBtjob *job, const unsigned cnt)
{
        return  cnt < job->h.bj_nargs?
                &job->bj_space[((USHORT *) &job->bj_space[job->h.bj_arg])[cnt]]:
                (const char *) 0;
}

int     gbatch_unpackenv(const apiBtjob *job, const unsigned cnt, const char **name, const char **value)
{
        Envir   *ep;

        if  (cnt >= job->h.bj_nenv)  {
                *name = *value = (const char *) 0;
                return  0;
        }

        ep = &((Envir *) &job->bj_space[job->h.bj_env])[cnt];
        *name = &job->bj_space[ep->e_name];
        *value = &job->bj_space[ep->e_value];
        return  1;
}

const char *gbatch_getenv(const apiBtjob *job, const char *name)
{
        unsigned        cnt;
        for  (cnt = 0;  cnt < job->h.bj_nenv;  cnt++)  {
                const  Envir  *ep = &((Envir *) &job->bj_space[job->h.bj_env])[cnt];
                if  (strcmp(&job->bj_space[ep->e_name], name) == 0)
                        return  &job->bj_space[ep->e_value];
        }
        return  (const char *) 0;
}

const char **gbatch_getenvlist(const apiBtjob *job)
{
        static  char            **eresult;
        static  unsigned        esize;
        unsigned        cnt, rsize = (1 + job->h.bj_nenv) * sizeof(char *);

        if  (rsize > esize)  {
                if  (eresult)
                        free((char *) eresult);
                if  (!(eresult = (char **) malloc(rsize)))
                        return  (const char **) 0;
                esize = rsize;
        }
        for  (cnt = 0;  cnt < job->h.bj_nenv;  cnt++)  {
                const   Envir   *ep = &((Envir *) &job->bj_space[job->h.bj_env])[cnt];
                const   char    *np = &job->bj_space[ep->e_name];
                const   char    *vp = &job->bj_space[ep->e_value];
                char    *res = malloc((unsigned) (strlen(np) + strlen(vp) + 2));
                if  (!res)
                        return  (const char **) 0;
                sprintf(res, "%s=%s", np, vp);
                eresult[cnt] = res;
        }
        eresult[cnt] = (char *) 0;
        return  (const char **) eresult;
}

const apiMredir *gbatch_getredir(const apiBtjob *job, const unsigned cnt)
{
        static  apiMredir       result;
        const   Redir           *rp;

        if  (cnt >= job->h.bj_nredirs)
                return  (const apiMredir *) 0;
        rp = &((const Redir *) &job->bj_space[job->h.bj_redirs])[cnt];

        result.fd = rp->fd;
        if  ((result.action = rp->action) >= RD_ACT_CLOSE)
                result.un.arg = rp->arg;
        else
                result.un.buffer = &job->bj_space[rp->arg];
        return  &result;
}

/* Copy things we aren't changing, returning next offset in buffer if
   all ok, otherwise -1 to say it won't fit.  */

static int  arg_copy(apiBtjob *job, const char *savebuffer, int hwm)
{
        char    *next = &job->bj_space[hwm];
        USHORT  *oldarglist, *arglist;
        unsigned        cnt;

        oldarglist = (USHORT *) &savebuffer[job->h.bj_arg];
        arglist = (USHORT *) next;
        job->h.bj_arg = (SHORT) hwm;
        hwm += job->h.bj_nargs * sizeof(USHORT);
        if  (hwm > JOBSPACE)
                return  -1;
        next = &job->bj_space[hwm];
        for  (cnt = 0;  cnt < job->h.bj_nargs;  cnt++)  {
                const   char    *oldarg = &savebuffer[oldarglist[cnt]];
                unsigned  lng   = strlen(oldarg) + 1;
                arglist[cnt] = (USHORT) hwm;
                hwm += lng;
                if  (hwm > JOBSPACE)
                        return  -1;
                BLOCK_COPY(next, oldarg, lng);
                next += lng;
        }
        return  (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);
}

static int  env_copy(apiBtjob *job, const char *savebuffer, int hwm)
{
        char    *next = &job->bj_space[hwm];
        Envir   *elist, *oldelist, *ep, *nep, *endep;

        elist = (Envir *) next;
        oldelist = (Envir *) &savebuffer[job->h.bj_env];
        endep = &oldelist[job->h.bj_nenv];
        job->h.bj_env = (SHORT) hwm;
        hwm += job->h.bj_nenv * sizeof(Envir);
        if  (hwm > JOBSPACE)
                return  -1;
        next = &job->bj_space[hwm];

        nep = elist;

        for  (ep = oldelist;  ep < endep;  ep++)  {
                const   char    *np = &savebuffer[ep->e_name];
                const   char    *vp = &savebuffer[ep->e_value];
                unsigned  lngn = strlen(np) + 1;
                unsigned  lngv = strlen(vp) + 1;
                if  (hwm + lngn + lngv > JOBSPACE)
                        return  0;
                nep->e_name = (USHORT) hwm;
                BLOCK_COPY(next, np, lngn);
                hwm += lngn;
                next += lngn;
                nep->e_value = (USHORT) hwm;
                BLOCK_COPY(next, vp, lngv);
                hwm += lngv;
                next += lngv;
                nep++;
        }
        return  (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);
}

static int  redir_copy(apiBtjob *job, const char *savebuffer, int hwm)
{
        char    *next = &job->bj_space[hwm];
        Redir   *rlist, *oldrlist;
        unsigned        cnt;

        oldrlist = (Redir *) &savebuffer[job->h.bj_redirs];
        rlist = (Redir *) next;
        job->h.bj_redirs = (SHORT) hwm;
        hwm += job->h.bj_nredirs * sizeof(Redir);
        if  (hwm > JOBSPACE)
                return  -1;
        next = &job->bj_space[hwm];
        for  (cnt = 0;  cnt < job->h.bj_nredirs;  cnt++)  {
                const  Redir *oldr = &oldrlist[cnt];
                Redir   *newr = &rlist[cnt];
                *newr = *oldr;
                if  (oldr->action < RD_ACT_CLOSE)  {
                        const   char  *ap = &savebuffer[oldr->arg];
                        unsigned  lng = strlen(ap) + 1;
                        if  (hwm + lng > JOBSPACE)
                                return  -1;
                        BLOCK_COPY(next, ap, lng);
                        newr->arg = (USHORT) hwm;
                        next += lng;
                        hwm += lng;
                }
        }
        return  (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);
}

static int  direc_copy(apiBtjob *job, const char *savebuffer, int hwm)
{
        const   char    *olddirect;
        int     lng;
        char    *next;

        if  (job->h.bj_direct < 0)
                return  hwm;
        olddirect = &savebuffer[job->h.bj_direct];
        next = &job->bj_space[hwm];
        lng = strlen(olddirect) + 1;
        if  (hwm + lng > JOBSPACE)
                return  -1;
        job->h.bj_direct = (SHORT) hwm;
        BLOCK_COPY(next, olddirect, lng);
        return  hwm + lng;
}

static int  title_copy(apiBtjob *job, const char *savebuffer, int hwm)
{
        const   char    *oldtitle;
        int     lng;
        char    *next;

        if  (job->h.bj_title < 0)
                return  hwm;
        oldtitle = &savebuffer[job->h.bj_title];
        next = &job->bj_space[hwm];
        lng = strlen(oldtitle) + 1;
        if  (hwm + lng > JOBSPACE)
                return  -1;
        job->h.bj_title = (SHORT) hwm;
        BLOCK_COPY(next, oldtitle, lng);
        return  hwm + lng;
}

/* Put routines */

int  gbatch_puttitle(const int fd, apiBtjob *job, const char *title)
{
        int     hwm = 0;
        char    savebuffer[JOBSPACE];

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  (title  &&  title[0])  {
                int     lng, alloc = 0;
                char    *rtitle = (char *) title;
                struct  api_fd  *fdp;

                if  (!strchr(title, ':')  &&  (fdp = gbatch_look_fd(fd))  &&  fdp->queuename)  {
                        if  ((rtitle = malloc((unsigned) (strlen(fdp->queuename) + strlen(title) + 2))))  {
                                sprintf(rtitle, "%s:%s", fdp->queuename, title);
                                alloc = 1;
                        }
                        else
                                rtitle = (char *) title;
                }
                lng = strlen(rtitle) + 1;
                if  (lng + hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(&job->bj_space[hwm], rtitle, lng);
                if  (alloc)
                        free(rtitle);
                job->h.bj_title = (SHORT) hwm;
                hwm += lng;
        }
        else
                job->h.bj_title = -1;
        return  1;
}

int  gbatch_putdirect(apiBtjob *job, const char *direct)
{
        int     hwm = 0;
        char    savebuffer[JOBSPACE];

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  (direct  &&  direct[0])  {
                int     lng = strlen(direct) + 1;
                if  (lng + hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(&job->bj_space[hwm], direct, lng);
                job->h.bj_direct = (SHORT) hwm;
                hwm += lng;
        }
        else
                job->h.bj_direct = -1;

        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_putarg(apiBtjob *job, const unsigned cnt, const char *arg)
{
        int             hwm = 0, lng;
        unsigned        cc, oldnargs = job->h.bj_nargs;
        char            *next;
        USHORT          *oldarglist, *arglist;
        char            savebuffer[JOBSPACE];

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        next = job->bj_space;

        if  (cnt >= oldnargs)
                job->h.bj_nargs = (USHORT) (cnt + 1);
        oldarglist = (USHORT *) &savebuffer[job->h.bj_arg];
        arglist = (USHORT *) next;
        job->h.bj_arg = (SHORT) hwm;
        hwm += job->h.bj_nargs * sizeof(USHORT);
        if  (hwm > JOBSPACE)
                return  0;
        next = &job->bj_space[hwm];

        /* Copy up to arg */

        for  (cc = 0;  cc < cnt;  cc++)  {
                char    *oldarg = &savebuffer[oldarglist[cc]];
                lng = strlen(oldarg) + 1;
                arglist[cc] = (USHORT) hwm;
                hwm += lng;
                if  (hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(next, oldarg, lng);
                next += lng;
        }

        /* Copy arg */

        lng = strlen(arg) + 1;
        arglist[cnt] = (USHORT) hwm;
        hwm += lng;
        if  (hwm > JOBSPACE)
                return  0;
        BLOCK_COPY(next, arg, lng);
        next += lng;

        /* Copy past arg */

        for  (cc++;  cc < job->h.bj_nargs;  cc++)  {
                char    *oldarg = &savebuffer[oldarglist[cc-1]];
                lng = strlen(oldarg) + 1;
                arglist[cc] = (USHORT) hwm;
                hwm += lng;
                if  (hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(next, oldarg, lng);
                next += lng;
        }
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_delarg(apiBtjob *job, const unsigned cnt)
{
        int             hwm = 0, lng;
        unsigned        cc;
        char            *next;
        USHORT          *oldarglist, *arglist;
        char            savebuffer[JOBSPACE];

        if  (cnt >= job->h.bj_nargs)
                return  0;

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        next = job->bj_space;

        oldarglist = (USHORT *) &savebuffer[job->h.bj_arg];
        arglist = (USHORT *) next;
        job->h.bj_arg = (SHORT) hwm;
        hwm += (job->h.bj_nargs - 1) * sizeof(USHORT);
        if  (hwm > JOBSPACE)
                return  0;
        next = &job->bj_space[hwm];

        /* Copy up to arg */

        for  (cc = 0;  cc < cnt;  cc++)  {
                char    *oldarg = &savebuffer[oldarglist[cc]];
                lng = strlen(oldarg) + 1;
                arglist[cc] = (USHORT) hwm;
                hwm += lng;
                if  (hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(next, oldarg, lng);
                next += lng;
        }

        /* Copy past arg */

        for  (cc++;  cc < job->h.bj_nargs;  cc++)  {
                char    *oldarg = &savebuffer[oldarglist[cc]];
                lng = strlen(oldarg) + 1;
                arglist[cc-1] = (USHORT) hwm;
                hwm += lng;
                if  (hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(next, oldarg, lng);
                next += lng;
        }
        job->h.bj_nargs--;
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_putarglist(apiBtjob *job, const char **argl)
{
        int             hwm = 0, lng;
        unsigned        cc, cnt;
        char            *next;
        USHORT          *arglist;
        char            savebuffer[JOBSPACE];

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        next = job->bj_space;

        /* Count arguments */

        cnt = 0;
        if  (argl)  for  (;  argl[cnt];  cnt++)
                ;

        job->h.bj_nargs = (USHORT) cnt;
        arglist = (USHORT *) next;
        job->h.bj_arg = (SHORT) hwm;
        hwm += cnt * sizeof(USHORT);
        if  (hwm > JOBSPACE)
                return  0;
        next = &job->bj_space[hwm];

        for  (cc = 0;  cc < cnt;  cc++)  {
                lng = strlen(argl[cc]) + 1;
                arglist[cc] = (USHORT) hwm;
                hwm += lng;
                if  (hwm > JOBSPACE)
                        return  0;
                BLOCK_COPY(next, argl[cc], lng);
                next += lng;
        }
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_putenv(apiBtjob *job, const char *enval)
{
        int             hwm = 0, adding = 1;
        unsigned        namel;
        char            *eqp = strchr(enval, '=');
        char            *next;
        Envir           *elist, *oldelist, *ep, *endep, *nep;
        char            savebuffer[JOBSPACE];

        /* Reject if format is wrong.  */

        if  (!eqp  ||  (namel = eqp - enval) == 0)
                return  0;

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;

        next = &job->bj_space[hwm];
        elist = (Envir *) next;
        oldelist = (Envir *) &savebuffer[job->h.bj_env];
        endep = &oldelist[job->h.bj_nenv];
        job->h.bj_env = (SHORT) hwm;

        /* Find out if we are replacing an existing one, otherwise we
           need to add to the end and increase the vector size */

        for  (ep = oldelist;  ep < endep;  ep++)  {
                const   char    *ename = &savebuffer[ep->e_name];
                if  (strncmp(ename, enval, namel) == 0  &&  ename[namel] == '\0')  {
                        adding = 0;
                        break;
                }
        }

        hwm += (adding + job->h.bj_nenv) * sizeof(Envir);
        next += (adding + job->h.bj_nenv) * sizeof(Envir);

        if  (hwm > JOBSPACE)
                return  0;

        nep = elist;

        for  (ep = oldelist;  ep < endep;  ep++)  {
                const   char    *np = &savebuffer[ep->e_name];
                const   char    *vp = &savebuffer[ep->e_value];
                unsigned  lngn = strlen(np), lngv;
                if  (strncmp(np, enval, namel) == 0  &&  lngn == namel)
                        vp = eqp + 1;
                lngv = strlen(vp) + 1;
                lngn++;
                if  (hwm + lngn + lngv > JOBSPACE)
                        return  0;
                nep->e_name = (USHORT) hwm;
                BLOCK_COPY(next, np, lngn);
                hwm += lngn;
                next += lngn;
                nep->e_value = (USHORT) hwm;
                BLOCK_COPY(next, vp, lngv);
                hwm += lngv;
                next += lngv;
                nep++;
        }
        if  (adding)  {
                unsigned  lngv = strlen(eqp);   /* On the = so includes + 1 */
                job->h.bj_nenv++;
                if  (hwm + namel + 1 + lngv > JOBSPACE)
                        return  0;
                nep->e_name = (USHORT) hwm;
                BLOCK_COPY(next, enval, namel);
                next[namel] = '\0';
                hwm += namel + 1;
                next += namel + 1;
                nep->e_value = (USHORT) hwm;
                BLOCK_COPY(next, eqp+1, lngv);
                hwm += lngv;
                next += lngv;
                nep++;
        }
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_delenv(apiBtjob *job, const char *name)
{
        int     hwm = 0;
        char    *next;
        Envir   *elist, *oldelist, *ep, *nep, *endep;
        char    savebuffer[JOBSPACE];

        if  (!gbatch_getenv(job, name)) /* Ignore it */
                return  1;

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;

        next = &job->bj_space[hwm];
        elist = (Envir *) next;
        oldelist = (Envir *) &savebuffer[job->h.bj_env];
        endep = &oldelist[job->h.bj_nenv];

        job->h.bj_env = (SHORT) hwm;
        job->h.bj_nenv--;
        hwm += job->h.bj_nenv * sizeof(Envir);
        next = &job->bj_space[hwm];

        if  (hwm > JOBSPACE) /* Not very likely if it fitted before but... */
                return  0;

        nep = elist;

        for  (ep = oldelist;  ep < endep;  ep++)  {
                const   char    *np = &savebuffer[ep->e_name];
                const   char    *vp = &savebuffer[ep->e_value];
                unsigned  lngn, lngv;
                if  (strcmp(np, name) == 0)
                        continue;
                lngn = strlen(np) + 1;
                lngv = strlen(vp) + 1;
                if  (hwm + lngn + lngv > JOBSPACE)
                        return  0;
                nep->e_name = (USHORT) hwm;
                BLOCK_COPY(next, np, lngn);
                hwm += lngn;
                next += lngn;
                nep->e_value = (USHORT) hwm;
                BLOCK_COPY(next, vp, lngv);
                hwm += lngv;
                next += lngv;
                nep++;
        }
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_putelist(apiBtjob *job, const char **els)
{
        int     hwm = 0;
        unsigned        cnt, nenvs = 0;
        char    *next;
        Envir   *elist, *ep;
        char    savebuffer[JOBSPACE];

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);
        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;

        /* Count the things.
           Just throw away environment vars without =s in them */

        if  (els)  for  (cnt = 0; els[cnt];  cnt++)
                if  (strchr(els[cnt], '='))
                     nenvs++;

        next = &job->bj_space[hwm];
        elist = (Envir *) next;
        job->h.bj_nenv = (USHORT) nenvs;
        job->h.bj_env = (SHORT) hwm;

        hwm += nenvs * sizeof(Envir);
        next = &job->bj_space[hwm];

        if  (hwm > JOBSPACE)
                return  0;

        ep = elist;

        for  (cnt = 0;  els[cnt];  cnt++)  {
                char    *eqp = strchr(els[cnt], '=');
                unsigned  lngn, lngv;

                if  (!eqp)
                        continue;
                lngn = eqp - els[cnt];
                lngv = strlen(eqp);
                if  (hwm + lngn + 1 + lngv > JOBSPACE)
                        return  0;
                ep->e_name = (USHORT) hwm;
                BLOCK_COPY(next, els[cnt], lngn);
                next[lngn] = '\0';
                hwm += lngn + 1;
                next += lngn + 1;
                ep->e_value = (USHORT) hwm;
                BLOCK_COPY(next, eqp+1, lngv);
                hwm += lngv;
                next += lngv;
                ep++;
        }
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = redir_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_putredir(apiBtjob *job, const unsigned cnt, const apiMredir *rd)
{
        int     hwm = 0;
        unsigned        nredirs = job->h.bj_nredirs, cc;
        char    *next;
        Redir   *rlist, *oldrlist;
        char    savebuffer[JOBSPACE];

        /* We only add on the end, we don't invent new ones in between.  */

        if  (cnt > nredirs)
                return  0;

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);

        if  (cnt == nredirs)            /* New one on the end */
                job->h.bj_nredirs++;

        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;

        next = &job->bj_space[hwm];

        oldrlist = (Redir *) &savebuffer[job->h.bj_redirs];
        rlist = (Redir *) next;
        job->h.bj_redirs = (SHORT) hwm;
        hwm += job->h.bj_nredirs * sizeof(Redir);
        if  (hwm > JOBSPACE)
                return  0;
        next = &job->bj_space[hwm];

        /* Copy up to redir */

        for  (cc = 0;  cc < cnt;  cc++)  {
                const  Redir *oldr = &oldrlist[cc];
                rlist[cc] = *oldr;
                if  (oldr->action < RD_ACT_CLOSE)  {
                        const   char  *ap = &savebuffer[oldr->arg];
                        unsigned  lng = strlen(ap) + 1;
                        if  (hwm + lng > JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, ap, lng);
                        rlist[cc].arg = (USHORT) hwm;
                        next += lng;
                        hwm += lng;
                }
        }

        /* Copy redirection itself */

        rlist[cnt].fd = rd->fd;
        if  ((rlist[cnt].action = rd->action) < RD_ACT_CLOSE)  {
                unsigned  lng = strlen(rd->un.buffer) + 1;
                if  (hwm + lng > JOBSPACE)
                        return  0;
                BLOCK_COPY(next, rd->un.buffer, lng);
                rlist[cnt].arg = (USHORT) hwm;
                next += lng;
                hwm += lng;
        }
        else
                rlist[cnt].arg = rd->un.arg;

        /* Copy past redir */

        for  (cc++;  cc < nredirs;  cc++)  {
                const  Redir *oldr = &oldrlist[cc];
                rlist[cc] = *oldr;
                if  (oldr->action < RD_ACT_CLOSE)  {
                        const   char  *ap = &savebuffer[oldr->arg];
                        unsigned  lng = strlen(ap) + 1;
                        if  (hwm + lng > JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, ap, lng);
                        rlist[cc].arg = (USHORT) hwm;
                        next += lng;
                        hwm += lng;
                }
        }

        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_delredir(apiBtjob *job, const unsigned cnt)
{
        int             hwm = 0;
        unsigned        nredirs = job->h.bj_nredirs, cc;
        char            *next;
        Redir           *rlist, *oldrlist;
        char            savebuffer[JOBSPACE];

        if  (cnt >= nredirs)
                return  0;

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);

        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;

        next = &job->bj_space[hwm];

        oldrlist = (Redir *) &savebuffer[job->h.bj_redirs];
        rlist = (Redir *) next;
        job->h.bj_redirs = (SHORT) hwm;
        job->h.bj_nredirs--;
        hwm += job->h.bj_nredirs * sizeof(Redir);
        if  (hwm > JOBSPACE)
                return  0;
        next = &job->bj_space[hwm];

        /* Copy up to redir */

        for  (cc = 0;  cc < cnt;  cc++)  {
                const  Redir *oldr = &oldrlist[cc];
                rlist[cc] = *oldr;
                if  (oldr->action < RD_ACT_CLOSE)  {
                        const   char  *ap = &savebuffer[oldr->arg];
                        unsigned  lng = strlen(ap) + 1;
                        if  (hwm + lng > JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, ap, lng);
                        rlist[cc].arg = (USHORT) hwm;
                        next += lng;
                        hwm += lng;
                }
        }

        /* Copy past redir */

        for  (cc++;  cc < nredirs;  cc++)  {
                const  Redir *oldr = &oldrlist[cc];
                rlist[cc-1] = *oldr;
                if  (oldr->action < RD_ACT_CLOSE)  {
                        const   char  *ap = &savebuffer[oldr->arg];
                        unsigned  lng = strlen(ap) + 1;
                        if  (hwm + lng > JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, ap, lng);
                        rlist[cc-1].arg = (USHORT) hwm;
                        next += lng;
                        hwm += lng;
                }
        }

        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}

int  gbatch_putredirlist(apiBtjob *job, const apiMredir *rdlist, const unsigned num)
{
        int             hwm = 0;
        unsigned        cnt;
        char            *next;
        Redir           *rlist;
        char            savebuffer[JOBSPACE];

        BLOCK_COPY(savebuffer, job->bj_space, JOBSPACE);

        if  ((hwm = arg_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = env_copy(job, savebuffer, hwm)) < 0)
                return  0;

        next = &job->bj_space[hwm];

        rlist = (Redir *) next;
        job->h.bj_redirs = (SHORT) hwm;
        job->h.bj_nredirs = (USHORT) num;
        hwm += num * sizeof(Redir);
        if  (hwm > JOBSPACE)
                return  0;
        next = &job->bj_space[hwm];

        for  (cnt = 0;  cnt < num;  cnt++)  {
                Redir   *newr = &rlist[cnt];
                const   apiMredir  *oldr = &rdlist[cnt];
                newr->fd = oldr->fd;
                if  ((newr->action = oldr->action) < RD_ACT_CLOSE)  {
                        unsigned  lng = strlen(oldr->un.buffer) + 1;
                        if  (hwm + lng > JOBSPACE)
                                return  0;
                        BLOCK_COPY(next, oldr->un.buffer, lng);
                        newr->arg = (USHORT) hwm;
                        next += lng;
                        hwm += lng;
                }
                else
                        newr->arg = oldr->un.arg;
        }
        hwm = (hwm + sizeof(LONG) - 1) & ~(sizeof(LONG) - 1);

        if  ((hwm = direc_copy(job, savebuffer, hwm)) < 0)
                return  0;
        if  ((hwm = title_copy(job, savebuffer, hwm)) < 0)
                return  0;
        return  1;
}
