/* envprocess.c -- parse strings expending ${FOO-BAR} constructs

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
#include "defaults.h"
#include "files.h"
#include "errnums.h"
#include "ipcstuff.h"

static  char    Filename[] = __FILE__;

extern char *strread(FILE *, const char *);

#ifndef MALLINC
#define MALLINC 64
#endif

extern  char    **environ;
static  char    **ienviron;     /* Environment not to be passed on */
char            **xenviron;     /* Environment to pass to sub-processes */

/* This is for "multiple environment stuff */

char            *envselect_name;
int             envselect_value;

/* Getenv where we are looking for a non-null-terminated string */

static char *sgetenv(const char *sstart, const char *send)
{
        char    **envp;
        int     l = send - sstart;

        if  (ienviron)
                for  (envp = ienviron;  *envp;  envp++)
                        if  (strncmp(*envp, sstart, l) == 0  &&  (*envp)[l] == ':')
                                return  &(*envp)[l+1];
        for  (envp = environ;  *envp;  envp++)
                if  (strncmp(*envp, sstart, l) == 0  &&  (*envp)[l] == '=')
                        return  &(*envp)[l+1];
        return  (char *) 0;
}

static int  appenv(char ***envlist, const char *schar, const char *echar, const int freeit)
{
        char    **envp, **newenv, **nep;

        for  (envp = *envlist;  *envp;  envp++)
                if  (strncmp(*envp, schar, (echar - schar) + 1) == 0)
                        return  0;

        if  ((newenv = (char **) malloc((unsigned) ((envp - *envlist) + 2) * sizeof(char *))) == (char **) 0)
                ABORT_NOMEM;

        nep = newenv;
        envp = *envlist;

        /* Copy existing entries */

        while  (*envp)
                *nep++ = *envp++;

        /* Copy in new, stick null on end */

        *nep++ = stracpy(schar);
        *nep = (char *) 0;

        /* If allocated before free it.  Assign new environment */

        if  (freeit)
                free((char *) *envlist);
        *envlist = newenv;
        return  1;
}

/* Initialise environment variables not otherwise defined from Master Config file.
   Assignments with : do not go into real environment */

void  init_mcfile()
{
        FILE    *inf;
        char    *inl, *cp, *ep;
        static  char    alloc_env = 0;
        int     hadspdir = 0;

        /* Select alternate environment if specified as a variable.
           We need a numeric value for adding to IPC ids.
           We need a character value for adding to file names and service names.
           For consistency we recreate the name value in case it's given as something
           confusing. */

        if  ((cp = getenv(ENV_SELECT_VAR)) && isdigit(*cp))  {
                char    buff[20];
                envselect_value = atoi(cp);
                sprintf(buff, "%d", envselect_value);
                envselect_name = stracpy(buff);
                envselect_value++;
                envselect_value *= ENV_INC_MULT;
        }

        if  ((inf = fopen(MASTER_CONFIG, "r")) == (FILE *) 0)
                return;

        while  ((inl = strread(inf, "\n")))  {
                for  (cp = inl;  isspace(*cp);  cp++)
                        ;
                if  (*cp == '#' || !((ep = strchr(cp, '='))  ||  (ep = strchr(cp, ':'))))  {
                        free(inl);
                        continue;
                }

                /* For multiple environments, specifically pick out SPOOLDIR and tack the name on the end */

                if  (!hadspdir  &&  envselect_name  &&  strncmp(cp, "SPOOLDIR", ep-cp) == 0)  {
                        char    *new_inl = malloc((unsigned) (strlen(inl) + strlen(envselect_name) + 1));
                        if  (!new_inl)
                                ABORT_NOMEM;

                        /* Copy across and reset the pointers so rest of code works */

                        cp = new_inl + (cp - inl);
                        ep = new_inl + (ep - inl);
                        strcpy(new_inl, inl);
                        strcat(new_inl, envselect_name);
                        free(inl);
                        inl = new_inl;
                        hadspdir = 1;
                }

                if  (*ep == ':')  {

                        /* Internal environment not to be passed on */

                        if  (ienviron)
                                appenv(&ienviron, cp, ep, 1);
                        else  {
                                char    **nep;
                                if  ((ienviron = (char **) malloc(2 * sizeof(char *))) == (char **) 0)
                                        ABORT_NOMEM;
                                nep = ienviron;
                                *nep++ = stracpy(cp);
                                *nep = (char *) 0;
                        }
                }
                else  if  (appenv(&environ, cp, ep, alloc_env))
                        alloc_env++;
                free(inl);
        }

        fclose(inf);

        /* If we didn't have a SPOOLDIR set, we'll have to invent one from the default value */

        if  (!hadspdir  &&  envselect_name)  {
                char    *spdir = SPDIR_RAW;
                char    *buff = malloc((unsigned) (strlen(spdir) + strlen(envselect_name) + 12));
                if  (!buff)
                        ABORT_NOMEM;
                sprintf(buff, "SPOOLDIR:%s%s", spdir, envselect_name);
                appenv(&ienviron, buff, strchr(buff, ':'), 1);
                free(buff);
        }
}

/* Open the file name, prefixing /etc if not absolute */

static FILE *path_fopen(const char *name)
{
        char    *thing, *etc = "/etc/";
        FILE    *fp;

        if  (name[0] == '/')
                return  fopen(name, "r");
        if  (!(thing = malloc((unsigned) (strlen(name) + strlen(etc) + 1))))
                ABORT_NOMEM;
        strcpy(thing, etc);
        strcat(thing, name);
        fp = fopen(thing, "r");
        free(thing);
        return  fp;
}

void  init_xenv()
{
        char    *flist = envprocess(ENVIR_FILE);
        char    *cp;

        if  (!flist)  {
                xenviron = environ;
                return;
        }

        do  {
                FILE    *fp = (FILE *) 0;
                cp = strchr(flist, ',');
                if  (cp)  {
                        *cp = '\0';
                        fp = path_fopen(flist);
                        *cp++ = ',';
                        flist = cp;
                }
                else
                        fp = path_fopen(flist);
                if  (fp)  {     /* No error message if unopenable */
                        char    *inl, *lp, *ep;
                        while  ((inl = strread(fp, "\n")))  {
                                for  (lp = inl;  isspace(*lp);  lp++)
                                        ;
                                if  (*lp == '#' || !(ep = strchr(lp, '=')))  {
                                        free(inl);
                                        continue;
                                }
                                if  (xenviron)
                                        appenv(&xenviron, lp, ep, 1);
                                else  {
                                        char    **nep;
                                        if  ((xenviron = (char **) malloc(2 * sizeof(char *))) == (char **) 0)
                                                ABORT_NOMEM;
                                        nep = xenviron;
                                        *nep++ = stracpy(lp);
                                        *nep = (char *) 0;
                                }
                                free(inl);
                        }
                        fclose(fp);
                }
        }  while  (cp);

        if  (!xenviron)
                xenviron = environ;
}

char **squash_envir(char **senv, char **denv)
{
        unsigned  ecount = 1;
        char    **ep, **xp, **result, **rp;

        /* If we didn't have an environment file, don't do anything.
           This is so that btr etc doesn't compare its own
           environment with the proposed one and deduce that
           nothing has changed when in fact it is wildly
           different from the btsched one.  */

        if  (!senv  ||  senv == denv)
                return  denv;

        for  (ep = denv;  *ep;  ep++)
                ecount++;

        if  (!(result = (char **) malloc(ecount * sizeof(char *))))
                ABORT_NOMEM;

        rp = result;

        for  (ep = denv;  *ep;  ep++)  {
                for  (xp = senv;  *xp;  xp++)
                        if  (strcmp(*ep, *xp) == 0)
                                goto  gotit;
                *rp++ = *ep;
        gotit:
                ;
        }
        *rp = (char *) 0;
        return  result;
}

char    *envprocess(const char *inp_string)
{
        const   char    *inp, *es, *rs, *ee;
        char    *outp, *result, *val;
        int     i, outlen;

        inp = inp_string;
        outlen = strlen(inp_string) + MALLINC;
        if  ((outp = result = (char *) malloc((unsigned) outlen)) == (char *) 0)
                ABORT_NOMEM;
        while  (*inp != '\0')  {
                if  (*inp == '$')  {

                        /* Environment variable.  */

                        if  (*++inp == '$')
                                goto  is_ch;

                        es = inp;
                        rs = (const char *) 0;

                        if  (*inp == '{')  {
                                inp++;
                                es++;
                                while  (*inp != '-' && *inp != '}' && *inp != '\0')
                                        inp++;
                                ee = inp;
                                if  (*inp == '-')  {
                                        rs = ++inp;
                                        while (*inp != '}' && *inp != '\0')
                                                inp++;
                                }
                        }
                        else  {
                                while (isalnum(*inp) || *inp == '_')
                                        inp++;
                                ee = inp;
                        }
                        if  (ee <= es)  {       /* Invalid var */
                                free((char *) result);
                                return  (char *) 0;
                        }

                        /* If no such var, ignore or insert replacement */

                        if  ((val = sgetenv(es, ee)))  {
                                int     lng = strlen(val);

                                i = outp - result;
                                if  (i + lng >= outlen) {
                                        do  outlen += MALLINC;
                                        while  (outlen <= i + lng);
                                        if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
                                                ABORT_NOMEM;
                                        outp = result + i;
                                }
                                strcpy(outp, val);
                                outp += lng;
                        }
                        else  if  (rs)  {
                                int     lng = inp - rs;
                                i = outp - result;
                                if  (lng > 0)  {
                                        if  (i + lng >= outlen)  {
                                                do  outlen += MALLINC;
                                                while  (outlen <= i + lng);
                                                if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
                                                        ABORT_NOMEM;
                                                outp = result + i;
                                        }
                                        strncpy(outp, rs, lng);
                                        outp += lng;
                                }
                        }
                        else  if  (ee == es + 1  &&  *es == '0')  {
                                int     lng = strlen(progname);
                                i = outp - result;
                                if  (i + lng >= outlen) {
                                        do  outlen += MALLINC;
                                        while  (outlen <= i + lng);
                                        if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
                                                ABORT_NOMEM;
                                        outp = result + i;
                                }
                                strcpy(outp, progname);
                                outp += lng;
                        }

                        if  (*inp == '}')
                                inp++;
                        continue;
                }
is_ch:
                *outp++ = *inp++;
                i = outp - result;
                if  (i >= outlen)  {
                        do  outlen += MALLINC;
                        while  (i >= outlen);
                        if  ((result = (char *) realloc(result, (unsigned) outlen)) == (char *) 0)
                                ABORT_NOMEM;
                        outp = result + i;
                }
        }

        *outp = '\0';
        return  result;
}

/* Get an absolute path name to the given file in the batch spool directory */

extern char *mkspdirfile(const char *bfile)
{
        char    *spdir = envprocess(SPDIR);
        char    *res = malloc((unsigned) (strlen(spdir) + strlen(bfile) + 2));
        if  (!res)
                ABORT_NOMEM;
        sprintf(res, "%s/%s", spdir, bfile);
        free(spdir);
        return  res;
}
