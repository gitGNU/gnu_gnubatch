/* prin_gname.c -- group name handling

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
#include <grp.h>
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"

static  char    Filename[] = __FILE__;

#ifdef  HAVE_GETGROUPS

/* Kludgy global option to set if program needs to worry about
   supplementary groups. */
char    Requires_suppgrps;

extern void  add_suppgrp(const char *, const gid_t);
#endif

/* Structure used to hash group ids.  */

struct  ghash   {
        struct  ghash   *grph_next, *grpu_next;
        int_ugid_t      grph_gid;
        char    grph_name[1];
};

#define UG_HASHMOD      37

static  int     doneit;
static  struct  ghash   *ghash[UG_HASHMOD];
static  struct  ghash   *gnhash[UG_HASHMOD];

/* Define these here */

gid_t   Realgid, Effgid, Daemgid;

/* Read group file to build up hash table of group ids.  This is
   usually done once only at the start of the program, however if
   done again it must be to re-read supp groups only.  */

void  rgrpfile()
{
        struct  group  *ugrp;
        struct  ghash  *hp, **hpp, **hnpp;
        char    *pn;
#ifdef  HAVE_GETGROUPS
        char    **gp;
#endif
        unsigned  sum;

        if  (doneit)  {
#ifdef  HAVE_GETGROUPS
                if  (Requires_suppgrps)  {
                        while  ((ugrp = getgrent()) != (struct group *) 0)
                                for  (gp = ugrp->gr_mem;  *gp;  gp++)
                                        add_suppgrp(*gp, ugrp->gr_gid);
                        endgrent();
                }
#endif
                return;
        }
        while  ((ugrp = getgrent()) != (struct group *) 0)  {
                pn = ugrp->gr_name;
                sum = 0;
                while  (*pn)
                        sum += *pn++;

                for  (hpp = &ghash[(ULONG) ugrp->gr_gid % UG_HASHMOD]; (hp = *hpp); hpp = &hp->grph_next)
                        ;

                hnpp = &gnhash[sum % UG_HASHMOD];
                if  ((hp = (struct ghash *) malloc(sizeof(struct ghash) + strlen(ugrp->gr_name))) == (struct ghash *) 0)
                        ABORT_NOMEM;
                hp->grph_gid = ugrp->gr_gid;
                strcpy(hp->grph_name, ugrp->gr_name);
                hp->grph_next = *hpp;
                hp->grpu_next = *hnpp;
                *hpp = hp;
                *hnpp = hp;
#ifdef  HAVE_GETGROUPS
                if  (Requires_suppgrps)
                        for  (gp = ugrp->gr_mem;  *gp;  gp++)
                                add_suppgrp(*gp, ugrp->gr_gid);
#endif
        }
        endgrent();
        doneit = 1;
}

/* Given a group id, return a group name.  */

char *prin_gname(const gid_t gid)
{
        struct  ghash  *hp;
        static  char    nbuf[10];

        if  (!doneit)
                rgrpfile();

        hp = ghash[((ULONG) gid) % UG_HASHMOD];

        while  (hp)  {
                if  (gid == hp->grph_gid)
                        return  hp->grph_name;
                hp = hp->grph_next;
        }
        sprintf(nbuf, "g%ld", (long) gid);
        return  nbuf;
}

/* Do the opposite - return -1 if invalid Declare as long in case
   gid_t isn't signed (thanq Amdahl) */

int_ugid_t  lookup_gname(const char *name)
{
        const  char     *cp;
        unsigned  sum = 0;
        struct  ghash  *hp;

        if  (!doneit)
                rgrpfile();
        cp = name;
        while  (*cp)
                sum += *cp++;
        hp = gnhash[sum % UG_HASHMOD];
        while  (hp)  {
                if  (strcmp(name, hp->grph_name) == 0)
                        return  hp->grph_gid;
                hp = hp->grpu_next;
        }
        return  UNKNOWN_GID;
}

/* Generate a matrix for use when prompting for group names */

#define GLINIT  10
#define GLINCR  5

char **gen_glist(const char *prefix)
{
        struct  ghash  *hp;
        unsigned        hi;
        char    **result;
        unsigned  maxr, countr;
        int     sfl = 0;

        if  (!doneit)
                rgrpfile();

        if  ((result = (char **) malloc(GLINIT * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        maxr = GLINIT;
        countr = 0;
        if  (prefix)
                sfl = strlen(prefix);

        for  (hi = 0;  hi < UG_HASHMOD;  hi++)
                for  (hp = ghash[hi];  hp;  hp = hp->grph_next)  {

                        /* Skip ones which don't match the prefix */

                        if  (strncmp(hp->grph_name, prefix, sfl) != 0)
                                continue;

                        if  (countr + 1 >= maxr)  {
                                maxr += GLINCR;
                                if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
                                        ABORT_NOMEM;
                        }

                        result[countr++] = stracpy(hp->grph_name);
                }

        if  (countr == 0)  {
                free((char *) result);
                return  (char **) 0;
        }

        result[countr] = (char *) 0;
        return  result;
}
