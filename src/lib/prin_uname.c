/* prin_uname.c -- Hash password list and generate user names.

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include <pwd.h>
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "errnums.h"
#include "files.h"

#ifndef NGROUPS_MAX
#ifdef  NGROUPS
#define NGROUPS_MAX NGROUPS
#else
#define NGROUPS_MAX 32
#endif
#endif

static  char    Filename[] = __FILE__;

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

/* Structure used to hash user ids.  */

struct  uhash   {
        struct  uhash   *pwhuid_next, *pwhuname_next;
        int_ugid_t      pwh_uid, pwh_dgid;
#ifdef  HAVE_GETGROUPS
        USHORT          pwh_nsuppgrps;
        gid_t           *pwh_suppglist;
#endif
        char    *pwh_homed;     /* Used for unameproc */
        char    pwh_name[1];
};

uid_t   Daemuid,
        Realuid,
        Effuid;

int_ugid_t      lastgid;        /* Group of last user */

#define UG_HASHMOD      503

static  int     doneit;
static  struct  uhash   *uhash[UG_HASHMOD];
static  struct  uhash   *unhash[UG_HASHMOD];

/* Number of unique userids in password file */

unsigned  Npwusers;

/* "unread" password file to order */

void  un_rpwfile()
{
        unsigned   cnt;
        struct  uhash   *hp, *np;

        if  (!doneit)           /* Nothing to undo */
                return;

        /* Do it by name not by uid as we might have duplicate uids which aren't hashed */

        for  (cnt = 0;  cnt < UG_HASHMOD;  cnt++)  {
                for  (hp = unhash[cnt];  hp;  hp = np)  {
                        np = hp->pwhuname_next;
#ifdef  HAVE_GETGROUPS
                        if  (hp->pwh_nsuppgrps != 0)  {
                                hp->pwh_nsuppgrps = 0;
                                free((char *) hp->pwh_suppglist);
                        }
#endif
                        if  (hp->pwh_homed)
                                free(hp->pwh_homed);
                        free((char *) hp);
                }
                uhash[cnt] = unhash[cnt] = (struct uhash *) 0;
        }
        doneit = 0;
}

/* Read password file to build up hash table of user ids.  */

void  rpwfile()
{
        struct  passwd  *upw;
        struct  uhash  *hp, **hpp, **hnpp;
        char    *pn;
        unsigned  sum;

        Npwusers = 0;                   /* This is the number of unique IDs */

        while  ((upw = getpwent()))  {
                int     haduid = 0;
                pn = upw->pw_name;
                sum = 0;
                while  (*pn)
                        sum += *pn++;

                /* Find END of collision chain for uid lookup.
                   This is so system ids with the same
                   uid come out with first name in the
                   password file.  */

                /* Avoid adding clashing names with the same id to the hash */

                for  (hpp = &uhash[(ULONG)upw->pw_uid % UG_HASHMOD]; (hp = *hpp); hpp = &hp->pwhuid_next)
                        if  (hp->pwh_uid == upw->pw_uid)
                                haduid = 1;

                hnpp = &unhash[sum % UG_HASHMOD];
                if  ((hp = (struct uhash *) malloc(sizeof(struct uhash) + strlen(upw->pw_name))) == (struct uhash *) 0)
                        ABORT_NOMEM;
                hp->pwh_uid = upw->pw_uid;
                hp->pwh_dgid = upw->pw_gid;
#ifdef  HAVE_GETGROUPS
                hp->pwh_nsuppgrps = 0;
                hp->pwh_suppglist = (gid_t *) 0;
#endif
                hp->pwh_homed = stracpy(upw->pw_dir);
                strcpy(hp->pwh_name, upw->pw_name);

                /* Add to user id hash if unique id and bump the number of users */
                if  (haduid)
                        hp->pwhuid_next = 0;
                else  {
                        hp->pwhuid_next = *hpp;
                        *hpp = hp;
                        Npwusers++;
                }
                /* Always add the name to the name hash to look up id */
                hp->pwhuname_next = *hnpp;
                *hnpp = hp;
        }
        endpwent();
        doneit = 1;
}

/* Given a user id, return a user name.  */

static struct uhash *luid_lookup(const uid_t uid)
{
        struct  uhash  *hp;

        if  (!doneit)
                rpwfile();

        hp = uhash[(ULONG) uid % UG_HASHMOD];

        while  (hp)  {
                if  (uid == hp->pwh_uid)
                        return  hp;
                hp = hp->pwhuid_next;
        }
        return  (struct uhash *) 0;
}

/* Do the opposite */

static struct uhash *luname_lookup(const char *name)
{
        const  char  *cp;
        unsigned  sum = 0;
        struct  uhash  *hp;

        if  (!doneit)
                rpwfile();
        cp = name;
        while  (*cp)
                sum += *cp++;
        hp = unhash[sum % UG_HASHMOD];
        while  (hp)  {
                if  (strcmp(name, hp->pwh_name) == 0)
                        return  hp;
                hp = hp->pwhuname_next;
        }
        return  (struct uhash *) 0;
}

char *prin_uname(const uid_t uid)
{
        struct  uhash  *hp = luid_lookup(uid);
        if  (hp)  {
                lastgid = hp->pwh_dgid;
                return  hp->pwh_name;
        }
        else  {
                static  char    nbuf[10];
                lastgid = 1;            /* Better put something */
                sprintf(nbuf, "u%ld", (long) uid);
                return  nbuf;
        }
}

/* Validate user id */

int  isvuser(const uid_t uid)
{
        struct  uhash  *hp = luid_lookup(uid);
        if  (hp)  {
                lastgid = hp->pwh_dgid;
                return  1;
        }
        return  0;
}

int_ugid_t  lookup_uname(const char *name)
{
        const   struct  uhash  *hp = luname_lookup(name);
        if  (hp)  {
                lastgid = hp->pwh_dgid;
                return  hp->pwh_uid;
        }
        else  {
                lastgid = 1;
                return  UNKNOWN_UID;
        }
}

char *homedof(const int_ugid_t uid)
{
        struct  uhash   *hp = luid_lookup((uid_t) uid);
        if  (hp)
                return  hp->pwh_homed;
        else
                return  "/";
}

/* Loop over all known user ids, calling the supplied function with
   argument arg and uid.  */

void    uloop_over(void (*fn)(char *, int_ugid_t), char *arg)
{
        struct  uhash  *hp;
        unsigned        hi;

        if  (!doneit)
                rpwfile();
        for  (hi = 0;  hi < UG_HASHMOD;  hi++)
                for  (hp = uhash[hi];  hp;  hp = hp->pwhuid_next)
                        (*fn)(arg, hp->pwh_uid);
}

/* Generate a matrix for use when prompting for user names */

#define ULINIT  40
#define ULINCR  10

char **gen_ulist(const char *prefix)
{
        struct  uhash  *hp;
        unsigned        hi;
        char    **result;
        unsigned  maxr, countr;
        int     sfl = 0;

        if  (!doneit)
                rpwfile();

        if  ((result = (char **) malloc(ULINIT * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        maxr = ULINIT;
        countr = 0;
        if  (prefix)
                sfl = strlen(prefix);

        for  (hi = 0;  hi < UG_HASHMOD;  hi++)
                for  (hp = uhash[hi];  hp;  hp = hp->pwhuid_next)  {

                        /* Skip ones which don't match the prefix */

                        if  (strncmp(hp->pwh_name, prefix, sfl) != 0)
                                continue;

                        if  (countr + 1 >= maxr)  {
                                maxr += ULINCR;
                                if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
                                        ABORT_NOMEM;
                        }

                        result[countr++] = stracpy(hp->pwh_name);
                }

        if  (countr == 0)  {
                free((char *) result);
                return  (char **) 0;
        }

        result[countr] = (char *) 0;
        return  result;
}

/* Process string to extract ~username constructs or ~ for $HOME ~+
   for $PWD if it exists ~- for $OLDPWD if it exists */

char  *unameproc(char *str, const char *currdir, const uid_t realuid)
{
        char    *ep, *cp, *newstr;
        const   char    *ins;
        int     l1, l2, l3, alloc = 0;

        /* It's probably unlikely that there will be more than one ~
           in the string, but let's not be taken by surprise if
           this happens.  */

        while  ((cp = strchr(str, '~')) != (char *) 0)  {
                ep = cp;
                if  (*++ep == '+')  {
                        ep++;
                        ins = currdir;
                }
                else  if  (*ep == '-')  {
                        if  ((ins = getenv("OLDPWD")) == (char *) 0)  {
                                disp_str = "~- (OLDPWD)";
                                if  (alloc)
                                        free(str);
                                return  stracpy("/");
                        }
                        ep++;
                }
                else  if  (!isalpha(*ep))  {
                        if  ((ins = getenv("HOME")) == (char *) 0)  {
                                struct  uhash   *hp = luid_lookup(realuid);
                                if  (!hp)  {
                                        disp_str = "HOME";
                                        if  (alloc)
                                                free(str);
                                        return  stracpy("/");
                                }
                                ins = hp->pwh_homed;
                        }
                }
                else  {
                        int     ls = 0;
                        static  char    cv[12];
                        const   struct  uhash   *hp;

                        do  if  (ls < 11)
                                cv[ls++] = *ep++;
                        while  (isalnum(*ep));
                        cv[ls] = '\0';
                        if  (!(hp = luname_lookup(cv)))  {
                                disp_str = cv;
                                if  (alloc)
                                        free(str);
                                return  stracpy("/");
                        }
                        ins = hp->pwh_homed;
                }

                l1 = cp - str;
                l2 = strlen(ins);
                l3 = strlen(ep);

                if  ((newstr = malloc((unsigned) (l1 + l2 + l3 + 1))) == (char *) 0)
                        ABORT_NOMEM;

                strncpy(newstr, str, l1);
                strcpy(newstr + l1, ins);
                strcpy(newstr + l1 + l2, ep);
                if  (alloc)
                        free(str);
                str = newstr;
                alloc++;
        }

        /* Insist on a "malloced" version */

        return  alloc? str: stracpy(str);
}

/* Recursive version of above which also does envprocess-ing as well */

char  *recursive_unameproc(const char *str, const char *currdir, const uid_t realuid)
{
        char    *newstr = unameproc((char *) str, currdir, realuid);
        int     number = RECURSE_MAX;

        while  (--number > 0)  {
                char    *redone;
                if  (strchr(newstr, '~'))  {
                        redone = unameproc(newstr, currdir, realuid);
                        free(newstr);
                        newstr = redone;
                        continue;
                }
                if  (!strchr(newstr, '$'))
                        return  newstr;
                redone = envprocess(newstr);
                free(newstr);
                newstr = redone;
        }
        return  newstr;
}

#ifdef  HAVE_GETGROUPS
void  add_suppgrp(const char *unam, const gid_t gid)
{
        struct  uhash  *hp = luname_lookup(unam);
        unsigned        cnt;

        if  (!hp)
                return;
#ifdef  NOT_DEFGROUP
        if  ((int_ugid_t) gid == hp->pwh_dgid)
                return;
#endif
        for  (cnt = 0;  cnt < hp->pwh_nsuppgrps;  cnt++)
                if  (gid == hp->pwh_suppglist[cnt])
                        return;
        if  (hp->pwh_nsuppgrps == 0)
                hp->pwh_suppglist = (gid_t *) malloc(sizeof(gid_t));
        else
                hp->pwh_suppglist = (gid_t *) realloc((char *) hp->pwh_suppglist,
                                                        (unsigned) ((hp->pwh_nsuppgrps+1) * sizeof(gid_t)));
        if  (!hp->pwh_suppglist)
                ABORT_NOMEM;
        hp->pwh_suppglist[hp->pwh_nsuppgrps] = gid;
        hp->pwh_nsuppgrps++;
}

int  get_suppgrps(const gid_t uid, gid_t **res)
{
        const   struct  uhash  *hp = luid_lookup(uid);
        if  (hp)  {
                *res = hp->pwh_suppglist;
                return  (int) hp->pwh_nsuppgrps;
        }
        return  0;
}
#endif
