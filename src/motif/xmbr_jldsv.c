/* xmbr_jldsv.c -- load/save job files for gbch-xmr

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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "btrvar.h"
#include "btuser.h"
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "helpargs.h"
#include "q_shm.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "spitrouts.h"
#include "optflags.h"

static  char    Filename[] = __FILE__;

#define XMBTR_INLINE

#define C_MASK          0377    /* Umask value */

static  HelpargRef      btr_avec;
static  char    Varname[] = "GBCH_R";

char *rdoptfile(const char *, const char *);
void  checksetmode(const int, const ushort *, const ushort, USHORT *);

#ifdef DO_NOT_DEFINE
/* This is just to force the inclusion of help
   messages in the help file corresponding to
   error messages in the inline/library stuff */
$H{Invalid assignment}
$H{Invalid char in time}
$H{Unknown command interp}
$H{Bad condition}
$H{Condition string too long}
$H{Load level out of range}
$H{Bad mode string}
$H{Priority out of range}
$H{Bad redirection}
$H{Bad repeat}
$H{Bad time spec}
$H{Assignment max exceeded}
$H{No set flags given}
$H{Condition max exceeded}
$H{File descriptor out of range}
$H{Redirection max exceeded}
$H{Max var name size}
$H{String too long in set}
$H{Must specify priority}
$H{Cannot use GNUbatch}
$H{Invalid priority}
$H{No room for job}
$H{Unreadable variable}
$H{Unwritable variable}
$H{No special create}
$H{No create perm}
$H{Setting avoid all}
$H{Cannot read weekdays}
$H{Bad avoid arg}
$H{Bad umask}
$H{Bad ulimit}
$H{Bad exit code spec}
$H{Cannot find rep units}
$H{Unknown host name in var}
$H{Invalid variable name}
$H{Cannot respecify mode}
$H{Could repeat endlessly}
$H{Bad delete time}
$H{Bad run time}
$H{Bad signal number}
$H{Bad grace time}
$H{Could repeat endlessly}
#endif

OPTION(o_explain)
{
        return  OPTRESULT_OK;
}

OPTION(o_noverbose)
{
        cjob->Verbose = 0;
        return  OPTRESULT_OK;
}

OPTION(o_verbose)
{
        cjob->Verbose = 1;
        return  OPTRESULT_OK;
}

DEOPTION(o_condcrit);
DEOPTION(o_nocondcrit);
DEOPTION(o_asscrit);
DEOPTION(o_noasscrit);
DEOPTION(o_canccond);
DEOPTION(o_condition);
DEOPTION(o_cancset);
DEOPTION(o_flags);
DEOPTION(o_set);
DEOPTION(o_advterr);
DEOPTION(o_noadvterr);
DEOPTION(o_directory);
DEOPTION(o_exits);
DEOPTION(o_noexport);
DEOPTION(o_export);
DEOPTION(o_fullexport);
DEOPTION(o_interpreter);
DEOPTION(o_loadlev);
DEOPTION(o_mail);
DEOPTION(o_write);
DEOPTION(o_cancmailwrite);
DEOPTION(o_normal);
DEOPTION(o_ascanc);
DEOPTION(o_asdone);
DEOPTION(o_priority);
DEOPTION(o_deltime);
DEOPTION(o_runtime);
DEOPTION(o_whichsig);
DEOPTION(o_gracetime);
DEOPTION(o_umask);
DEOPTION(o_ulimit);
DEOPTION(o_title);
DEOPTION(o_cancargs);
DEOPTION(o_argument);
DEOPTION(o_cancio);
DEOPTION(o_io);
DEOPTION(o_mode_job);
DEOPTION(o_notime);
DEOPTION(o_time);
DEOPTION(o_norepeat);
DEOPTION(o_deleteatend);
DEOPTION(o_repeat);
DEOPTION(o_avoiding);
DEOPTION(o_skip);
DEOPTION(o_hold);
DEOPTION(o_resched);
DEOPTION(o_catchup);

OPTION(o_owner)
{
#ifdef  INCLUDE_LOAD_CHOWNS
        int_ugid_t      nuid;
#endif

        if  (!arg)
                return  OPTRESULT_MISSARG;

#ifdef  INCLUDE_LOAD_CHOWNS
        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_OWNER_CHANGE;

        if  ((nuid = lookup_uname(arg)) == UNKNOWN_UID)  {
                arg_errnum = $EH{Unknown owner};
                return  OPTRESULT_ERROR;
        }
        if  (cjob->userid != nuid && (mypriv->btu_priv & BTM_WADMIN) == 0)  {
                arg_errnum = $EH{Cannot set owner};
                return  OPTRESULT_ERROR;
        }
        cjob->userid = nuid;
#endif
        return  OPTRESULT_ARG_OK;
}

OPTION(o_group)
{
#ifdef  INCLUDE_LOAD_CHOWNS
        int_ugid_t      ngid;
#endif

        if  (!arg)
                return  OPTRESULT_MISSARG;

#ifdef  INCLUDE_LOAD_CHOWNS
        Anychanges |= OF_ANY_DOING_SOMETHING;
        Procparchanges |= OF_GROUP_CHANGE;

        if  ((ngid = lookup_gname(arg)) == UNKNOWN_GID)  {
                arg_errnum = $EH{Unknown group};
                return  OPTRESULT_ERROR;
        }
        if  (cjob->grpid != ngid  &&  (mypriv->btu_priv & BTM_WADMIN) == 0)  {
                arg_errnum = $EH{Cannot set group};
                return  OPTRESULT_ERROR;
        }
        cjob->grpid = ngid;
#endif
        return  OPTRESULT_ARG_OK;
}

OPTION(o_freezecd)
{
        return  OPTRESULT_OK;
}

OPTION(o_freezehd)
{
        return  OPTRESULT_OK;
}

OPTION(o_jobqueue)              /* Home-grown version */
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        if  (cjob->jobqueue)
                free(cjob->jobqueue);
        cjob->jobqueue = (arg[0] && (arg[0] != '-' || arg[1])) ? stracpy(arg): (char *) 0;
        return  OPTRESULT_ARG_OK;
}

/* Dummy version as we don't currently support remote queueing (maybe
   we should?).  */

OPTION(o_queuehost)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;
        return  OPTRESULT_ARG_OK;
}


OPTION(o_localenv)
{
        return  OPTRESULT_OK;
}

OPTION(o_outsideenv)
{
        return  OPTRESULT_OK;
}

#include "inline/btradefs.c"
#include "inline/btroptp.c"

/* This is a version of doopts from the library which doesn't print errors.  */

static  char    *dooptsarg;     /* Need to save it in case it gets deallocated */

static char **my_doopts(char **argv)
{
        char    *arg;
        int             ad, rc;
        HelpargkeyRef   ap;

        if  (dooptsarg)  {
                free(dooptsarg);
                dooptsarg = (char *) 0;
        }
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

                        do      {
                                ad = btr_avec[*arg - ARG_STARTV].value;
                                if  (ad < $A{btr arg explain})  {
                                        disp_str = dooptsarg = stracpy(*argv);
                                        arg_errnum = $EH{program arg error};
                                        return  (char **) 0;
                                }

                                /*      Each function returns:
                                        1 (OPTRESULT_ARG_OK)
                                                if it eats the argument and it's OK
                                        2 (OPTRESULT_LAST_ARG_OK)
                                                ditto but the argument must be last
                                        0 (OPTRESULT_OK)
                                                if it ignores the argument.
                                        -1 (OPTRESULT_MISSARG) if no arg and one reqd
                                        -2 (OPTRESULT_ERROR) if something is wrong
                                                error code in arg_errnum. */

                                if  (!*++arg)  {
                                        disp_str = argv[1];
                                        if  ((rc = (optprocs[ad - $A{btr arg explain}])(argv[1])) < OPTRESULT_OK)  {
                                                if  (rc == OPTRESULT_MISSARG)  {
                                                        arg_errnum = $EH{program opt expects arg};
                                                        disp_str = dooptsarg = stracpy(*argv);
                                                }
                                                else
                                                        disp_str = dooptsarg = stracpy(disp_str);
                                                return  (char **) 0;
                                        }
                                        if  (rc > OPTRESULT_OK)  { /* Eaten the next arg */
                                                if  (rc > OPTRESULT_ARG_OK)
                                                        return  argv;
                                                argv++;
                                        }
                                        goto  nexta;
                                }

                                /* Trailing stuff after arg letter, we incremented to it */

                                disp_str = arg;
                                if  ((rc = (optprocs[ad - $A{btr arg explain}])(arg)) > OPTRESULT_OK)  { /* Eaten */
                                        if  (rc > OPTRESULT_ARG_OK)             /* Last of its kind */
                                                return  argv;
                                        goto  nexta;
                                }
                        }  while  (*arg);
                        continue;
                }

                arg++;          /* Increment past '+' */

        keyw_arg:
                for  (ap = btr_avec[tolower(*arg) - ARG_STARTV].mult_chain;  ap;  ap = ap->next)
                        if  (ncstrcmp(arg, ap->chars) == 0)
                                goto  found;

                disp_str = dooptsarg = stracpy(arg);
                arg_errnum = $EH{program arg bad string};
                return  (char **) 0;

        found:

                disp_str = argv[1];
                if  ((rc = (optprocs[ap->value - $A{btr arg explain}])(argv[1])) < OPTRESULT_OK)  {
                        if  (rc == OPTRESULT_MISSARG)  {
                                disp_str = dooptsarg = stracpy(arg);
                                arg_errnum = $EH{program opt expects arg};
                        }
                        else
                                disp_str = dooptsarg = stracpy(disp_str);
                        return  (char **) 0;
                }

                if  (rc > OPTRESULT_OK)  {              /* Eaten */
                        if  (rc > OPTRESULT_ARG_OK)     /* The end */
                                return  argv;
                        argv++;
                }
        }
}

static int  fixcsvars()
{
        int     nn;
        ULONG   Saveseq;

        /* Don't worry if none are specified */

        if  (Condcnt <= 0  &&  Asscnt <= 0)
                return  0;

        rvarfile(1);

        for  (nn = 0;  nn < Condcnt;  nn++)  {
                struct  scond   *sp = &Condlist[nn];
                JcondRef        jc = &JREQ->h.bj_conds[nn];
                vhash_t vp = lookupvar(sp->vd.var, sp->vd.hostid, BTM_READ, &Saveseq);

                if  (vp < 0)  {
                        disp_str = sp->vd.var;
                        return  $EH{Unreadable variable};
                }
                jc->bjc_compar = sp->compar;
                jc->bjc_varind = vp;
                jc->bjc_value = sp->value;
                jc->bjc_iscrit = sp->vd.hostid? sp->vd.crit: 0;
        }

        for  (nn = 0;  nn < Asscnt;  nn++)  {
                struct  Sass    *sp = &Asslist[nn];
                JassRef ja = &JREQ->h.bj_asses[nn];
                vhash_t vp = lookupvar(sp->vd.var, sp->vd.hostid, (unsigned)(sp->op == BJA_ASSIGN? BTM_WRITE: BTM_READ|BTM_WRITE), &Saveseq);

                if  (vp < 0)  {
                        disp_str = sp->vd.var;
                        return  $EH{Unwritable variable};
                }

                ja->bja_flags = sp->flags;
                ja->bja_op = sp->op;
                ja->bja_varind = vp;
                ja->bja_con = sp->con;
                ja->bja_iscrit = sp->vd.hostid? sp->vd.crit: 0;
        }

        return  0;
}

/* Read environment.
   In fact we "squash out" everything the same as in the static environment table.  */

static int  read_envir(char **envlist)
{
        char    **ep;
        int     envcount = 0;

        for  (ep = envlist;  *ep;  ep++)  {
                char    *eqp, *ncopy;
                if  (!(eqp = strchr(*ep, '='))) /* Don't understand no = */
                        continue;
                if  (envcount < MAXJENVIR)  {
                        unsigned  lng = eqp - *ep;
                        if  ((ncopy = malloc(lng + 1)) == (char *) 0)
                                ABORT_NOMEM;
                        BLOCK_COPY(ncopy, *ep, lng);
                        ncopy[lng] = '\0';
                        Envs[envcount].e_name = ncopy;
                        Envs[envcount].e_value = stracpy(eqp + 1);
                }
                envcount++;
        }
        if  (envcount > MAXJENVIR)  {
                disp_arg[0] = envcount;
                disp_arg[1] = MAXJENVIR;
                JREQ->h.bj_nenv = MAXJENVIR;
                return  $EH{Too large environment};
        }
        else
                JREQ->h.bj_nenv = (USHORT) envcount;
        return  0;
}

void  job_initialise(struct pend_job *pj, char *dname, char *fname)
{
        *pj = default_pend;
        if  (!(pj->job = (BtjobRef) malloc(sizeof(Btjob))))
                ABORT_NOMEM;
        *pj->job = default_job;
        pj->changes = 1;
        pj->nosubmit = 1;
        pj->directory = dname;  /* Already stracpyed */
        if  (pj->jobqueue)
                pj->jobqueue = stracpy(pj->jobqueue);   /* Lets have a unique copy. */
        pj->jobfile_name = (char *) 0;
        pj->cmdfile_name = fname; /* Already stracpyed */
}

static void  cleanupspace(char **envlist)
{
        unsigned        cnt;

        if  (envlist)  {
                char    **ep;
                for  (ep = envlist; *ep;  ep++)
                        free(*ep);
                free((char *) envlist);
        }
        if  (job_title)  {
                free(job_title);
                job_title = (char *) 0;
        }
        if  (job_cwd)  {
                free(job_cwd);
                job_cwd = (char *) 0;
        }
        for  (cnt = 0;  cnt < JREQ->h.bj_nargs;  cnt++)
                free(Args[cnt]);
        for  (cnt = 0;  cnt < JREQ->h.bj_nenv;  cnt++)  {
                free(Envs[cnt].e_name);
                free(Envs[cnt].e_value);
        }
        for  (cnt = 0;  cnt < JREQ->h.bj_nredirs;  cnt++)
                if  (Redirs[cnt].action < RD_ACT_CLOSE)  {
                        free(Redirs[cnt].un.buffer);
                        Redirs[cnt].un.buffer = (char *) 0;
                }
        Argcnt = Redircnt = 0;
        o_canccond((char *) 0);
        o_cancset((char *) 0);
}

/* Load up a job from file */

#define INIT_JENV       50
#define INC_JENV        20

int  job_load(struct pend_job *pj)
{
        FILE    *fp;
        char    **envlist = (char **) 0, *ep, *envp, *ntit, *fulltit;
        int     lng = 0, ret;
        unsigned        envcnt = 0, envmax = 0;
        time_t  now;
        char    ebuf[2048];

        /* Set current pending job up for all the option routines.
           Initialise system defaults.  */

        cjob = pj;
        JREQ = cjob->job;
        Dispflags = 0;
        sflags = def_assflags;
        Mode_arg = &JREQ->h.bj_mode;
        BLOCK_ZERO(JREQ, sizeof(Btjob));
        BLOCK_ZERO(Mode_set, 3); /* KLUDGE!!!!!!! */
        JREQ->h.bj_pri = mypriv->btu_defp;
        strcpy(JREQ->h.bj_cmdinterp, Ci_list[CI_STDSHELL].ci_name);
        JREQ->h.bj_ll = Ci_list[CI_STDSHELL].ci_ll;
        /*JREQ->h.bj_times.tc_istime = 0;        No time spec */
        JREQ->h.bj_times.tc_repeat = TC_DELETE;
        JREQ->h.bj_times.tc_nposs = TC_WAIT1;
        JREQ->h.bj_times.tc_nvaldays = (USHORT) defavoid;
        JREQ->h.bj_exits.elower = 1;
        JREQ->h.bj_exits.eupper = 255;
        /*JREQ->h.bj_jflags = 0;                The default case */

        /* Open the job file and parse */

        sprintf(ebuf, "%s/%s", pj->directory, pj->cmdfile_name);

        SWAP_TO(Realuid);
        fp = fopen(ebuf, "r");
        SWAP_TO(Daemuid);
        if  (!fp)
                return  $EH{xmbtr cannot open cmd file};

        while  (fgets(ebuf, sizeof(ebuf), fp))  {
                int     namel, vall;

                lng = strlen(ebuf);

                if  (ebuf[0] == '#')
                        continue;

                /* If we didn't get the newline on the end, we have
                   too big a line to understand.  Trim off
                   trailing whitespace \ and \n */

                if  (lng < 4  ||  ebuf[--lng] != '\n'  ||  ebuf[--lng] != '\\')
                        goto  badfmt;

                while  (isspace(ebuf[lng-1]))
                        lng--;

                ebuf[lng] = '\0';

                /* If it's the start of the argument list, go to next stage.  */

                if  (strncmp(ebuf, BTR_PROGRAM " ", sizeof(BTR_PROGRAM)) == 0)
                        break;

                /* If it's not an environment variable, give up */

                if  (!(ep = strchr(ebuf, '=')))
                        goto  badfmt;

                /* Kill the = to give us a name */

                if  ((namel = ep - ebuf) <= 0)
                        goto  badfmt;
                *ep++ = 0;

                /* If environment var has quotes round it, strip them.  */

                if  (*ep == '\'' || *ep == '\"')  {
                        if  (ebuf[lng-1] != *ep)
                                goto  badfmt;
                        ebuf[--lng] = '\0';
                        ep++;
                }

                vall = lng - (ep - ebuf);

                if  (!(envp = malloc((unsigned) (namel + vall + 2))))
                        ABORT_NOMEM;

                sprintf(envp, "%s=%s", ebuf, ep);

                if  (envcnt >= envmax)  {
                        if  (envmax == 0)  {
                                envmax = INIT_JENV;
                                envlist = (char **) malloc((INIT_JENV + 1) * sizeof(char *));
                        }
                        else  {
                                envmax += INC_JENV;
                                envlist = (char **) realloc((char *) envlist, (unsigned)((envmax+1) * sizeof(char *)));
                        }
                        if  (!envlist)
                                ABORT_NOMEM;
                }
                envlist[envcnt++] = envp;
        }


        if  (envlist)
                envlist[envcnt] = (char *) 0;

        /* Done environment variables, now for arguments */

        ep = &ebuf[sizeof(BTR_PROGRAM)];                /* Past 'btr ' */
        for  (;;)  {
                char    **argvec, **reta;
                int     lng;

                while  (isspace(*ep))
                        ep++;

                /* Arguments start with - or + */

                if  (*ep != '-' && *ep != '+')
                        goto  badfmt;

                /* Build up pseudo-vector and parse
                   Errors get put in arg_errnum.  */

                argvec = makevec(ep);
                reta = my_doopts(argvec);
                checksetmode(0, (const USHORT *) 0, mypriv->btu_jflags[0], &JREQ->h.bj_mode.u_flags);
                checksetmode(1, (const USHORT *) 0, mypriv->btu_jflags[1], &JREQ->h.bj_mode.g_flags);
                checksetmode(2, (const USHORT *) 0, mypriv->btu_jflags[2], &JREQ->h.bj_mode.o_flags);
                free(argvec[0]);
                free((char *) argvec);
                if  (!reta)
                        goto  reterr;

                /* Read next argument line.
                   Final line ends with a file name.  */

                do  if  (!fgets(ebuf, sizeof(ebuf), fp))
                        goto  badfmt;
                while  (ebuf[0] == '#');

                /* Trim off trailing \ns and \s */

                lng = strlen(ebuf);
                if  (lng < 2  ||  ebuf[--lng] != '\n')
                        goto  badfmt;
                ebuf[lng] = '\0';

                if  (ebuf[0] == '/')            /* Final line */
                        break;
                if  (ebuf[--lng] != '\\')
                        goto  badfmt;
                ebuf[lng] = '\0';
                ep = ebuf;
        }

        if  ((ret = repmnthfix(&JREQ->h.bj_times)) != 0)  {
                arg_errnum = ret;
                goto  reterr;
        }

        /* And now we have the job file.
           We assume that they go in pairs.  */

        while  (lng > 0  &&  isspace(ebuf[lng-1]))
                lng--;

        if  (pj->jobfile_name)
                free(pj->jobfile_name);

        pj->jobfile_name = (ep = strrchr(ebuf, '/')) ? stracpy(ep+1): stracpy(ebuf);

        /* Set up variables in job.  */

        if  ((arg_errnum = fixcsvars()))
                goto  reterr;
        if  (envlist  &&  (arg_errnum = read_envir(envlist)))
                goto  reterr;

        if  (!(ntit = job_title))
                ntit = stracpy(pj->jobfile_name);
        if  (strchr(ntit, ':')  ||  !pj->jobqueue)
                fulltit = stracpy(ntit);
        else  {
                if  (!(fulltit = malloc((unsigned) (strlen(pj->jobqueue) + strlen(ntit) + 2))))
                        ABORT_NOMEM;
                sprintf(fulltit, "%s:%s", pj->jobqueue, ntit);
        }
        pj->job->h.bj_nargs = Argcnt;
        pj->job->h.bj_nredirs = Redircnt;
        ret = packjstring(pj->job, job_cwd? job_cwd: Curr_pwd, fulltit, Redirs, Envs, Args);
        free(fulltit);
        if  (!ret)  {
                arg_errnum = $EH{Too many job strings};
                /* Turn off everything that might be half-baked.  */
                pj->job->h.bj_title = pj->job->h.bj_direct = -1;
                pj->job->h.bj_nargs = pj->job->h.bj_nenv = pj->job->h.bj_nredirs = 0;
                goto  reterr;
        }

        fclose(fp);
        cleanupspace(envlist);
        if  (pj->job->h.bj_times.tc_istime  &&  pj->job->h.bj_times.tc_nexttime < time((time_t *) 0))
                doinfo(jwid, $E{xmbtr loaded job not future});
        return  0;

 badfmt:
        arg_errnum = $EH{xmbtr bad fmt cmd file};
 reterr:
        fclose(fp);
        cleanupspace(envlist);
        /* Clear these out so we don't get left with something half-baked.  */
        BLOCK_ZERO(pj->job->h.bj_conds, sizeof(pj->job->h.bj_conds));
        BLOCK_ZERO(pj->job->h.bj_asses, sizeof(pj->job->h.bj_asses));
        time(&now);
        if  (pj->job->h.bj_times.tc_istime  &&  pj->job->h.bj_times.tc_nexttime < now)
                pj->job->h.bj_times.tc_nexttime = now + 60L;
        return  arg_errnum;
}

void  dumphdrs(BtjobRef jp, FILE *xfl)
{
        spitbtrstr($A{btr arg nomess}, xfl, 1);
        if  (jp->h.bj_jflags & BJ_WRT)
                spitbtrstr($A{btr arg write}, xfl, 1);
        if  (jp->h.bj_jflags & BJ_MAIL)
                spitbtrstr($A{btr arg mail}, xfl, 1);

        spitbtrstr($A{btr arg interp}, xfl, 0);
        fprintf(xfl, "%s \\\n", jp->h.bj_cmdinterp);
        spitbtrstr($A{btr arg ll}, xfl, 0);
        fprintf(xfl, "%d \\\n", jp->h.bj_ll);
        spitbtrstr($A{btr arg pri}, xfl, 0);
        fprintf(xfl, "%d \\\n", jp->h.bj_pri);

        spitbtrstr(jp->h.bj_progress < BJP_DONE? $A{btr arg norm}:
                   jp->h.bj_progress == BJP_DONE? $A{btr arg done}: $A{btr arg canc}, xfl, 1);

        dumptime(xfl, &jp->h.bj_times);

        if  (jp->h.bj_jflags & BJ_REMRUNNABLE)
                spitbtrstr($A{btr arg fullexport}, xfl, 1);
        else  if  (jp->h.bj_jflags & BJ_EXPORT)
                spitbtrstr($A{btr arg export}, xfl, 1);
        else
                spitbtrstr($A{btr arg loco}, xfl, 1);
        spitbtrstr($A{btr arg mode}, xfl, 0);
        dumpmode(xfl, "U", jp->h.bj_mode.u_flags);
        dumpmode(xfl, ",G", jp->h.bj_mode.g_flags);
        dumpmode(xfl, ",O", jp->h.bj_mode.o_flags);
        fputs(" \\\n", xfl);
        rvarfile(1);
        dumpconds(xfl, jp->h.bj_conds);
        dumpasses(xfl, jp->h.bj_asses);
}

int  job_save(struct pend_job *pj)
{
        BtjobRef        jp = pj->job;
        FILE            *xfl;
        unsigned        cnt;
        char            *path;

        if  (!(path = malloc((unsigned) (strlen(pj->directory) + strlen(pj->cmdfile_name) + 2))))
                ABORT_NOMEM;
        sprintf(path, "%s/%s", pj->directory, pj->cmdfile_name);

        SWAP_TO(Realuid);
        xfl = fopen(path, "w");
        if  (!xfl)  {
                SWAP_TO(Daemuid);
                free(path);
                return  $EH{xmbtr cannot write cmd file};
        }

#ifdef  HAVE_FCHMOD
        fchmod(fileno(xfl), (int) (0777 &~Save_umask));
#else
        chmod(path, (int) (0777 &~Save_umask));
#endif
        SWAP_TO(Daemuid);
        free(path);

        /* Put a comment to trigger shell things in.  */

        fputs("#! /bin/sh\n", xfl);

        /* Handle environment
           We insert this in front of the command to look like this
           ENV1='VAL1' \
           ENV2='VAL2' \
           ....
           ENVn='VALn' command */

        for  (cnt = 0;  cnt < jp->h.bj_nenv;  cnt++)  {
                char    *name, *value, *vp, *qp;

                ENV_OF(jp, cnt, name, value);

                /* Ignore assignments to the variable PWD because we
                   (think we've) already taken care of that and
                   because many shells break if we try to assign
                   to it.  */

                if  (strcmp(name, "PWD") == 0)
                        continue;

                /* Put quotes round the value, in case they've got
                   funny chars in. This won't quite handle env
                   vars with \ns in but since most other software
                   (including most shells) can't either we're in
                   great company.  */

                fprintf(xfl, "%s=\'", name);

                /* The following loop is in case we have one or more
                   single quotes in the string. We replace the
                   quote with '\'' (i.e. ending ' an escaped '
                   restart ') vp start of (rest of) string, qp
                   quote posn.  */

                for  (vp = value;  (qp = strchr(vp, '\''));  vp = qp + 1)  {
                        while  (vp < qp)  {
                                putc(*vp, xfl);
                                vp++;
                        }
                        fputs("\'\\\'\'", xfl);
                }
                fprintf(xfl, "%s\' \\\n", vp);
        }

        /* Finished with environment, now for btr command & args.  */

        fputs(BTR_PROGRAM, xfl);
        spitbtrstr(pj->Verbose? $A{btr arg verb}: $A{btr arg noverb}, xfl, 1);
        if  (jp->h.bj_title >= 0)  {
                char    *title = &jp->bj_space[jp->h.bj_title];
                char    *colp;
                if  ((colp = strchr(title, ':')))  {
                        spitbtrstr($A{btr arg queue}, xfl, 0);
                        fprintf(xfl, "\'%.*s\' \\\n", (int) (colp - title), title);
                        title = colp + 1;
                }
                else  if  (pj->jobqueue)  {
                        spitbtrstr($A{btr arg queue}, xfl, 0);
                        fprintf(xfl, "\'%s\' \\\n", pj->jobqueue);
                }
                spitbtrstr($A{btr arg title}, xfl, 0);
                fprintf(xfl, "\'%s\' \\\n", title);
        }
        else  if  (pj->jobqueue)  {
                spitbtrstr($A{btr arg queue}, xfl, 0);
                fprintf(xfl, "\'%s\' \\\n", pj->jobqueue);
        }

        dumphdrs(jp, xfl);

        if  (pj->userid != Realuid)  {
                spitbtrstr($A{btr arg setu}, xfl, 0);
                fprintf(xfl, "%s \\\n", jp->h.bj_mode.o_user);
        }
        if  (pj->grpid != Realgid)  {
                spitbtrstr($A{btr arg setg}, xfl, 0);
                fprintf(xfl, "%s \\\n", jp->h.bj_mode.o_group);
        }

        dumpecrun(xfl, jp);
        spitbtrstr($A{btr arg dir}, xfl, 0);
        fprintf(xfl, "%s \\\n", jp->h.bj_direct >= 0? &jp->bj_space[jp->h.bj_direct]: pj->directory);
        spitbtrstr(jp->h.bj_jflags & BJ_NOADVIFERR? $A{btr arg noadverr}: $A{btr arg adverr}, xfl, 1);

        dumpredirs(xfl, jp);
        dumpargs(xfl, jp);

        fprintf(xfl, "%s/%s\n", pj->directory, pj->jobfile_name);
        fclose(xfl);
        return  0;
}

void  cb_loaddefs(Widget w, int ishomed)
{
        char    *filename, *name, **ret = (char **) 0;
        int             had = 0;

        if  (ishomed)
                filename = recursive_unameproc(HOME_CONFIG, Curr_pwd, Realuid);
        else  {
                if  (!(filename = malloc((unsigned) (strlen(Curr_pwd) + sizeof(USER_CONFIG) + 1))))
                        ABORT_NOMEM;
                strcpy(filename, Curr_pwd);
                strcat(filename, "/" USER_CONFIG);
        }

        if  ((name = rdoptfile(filename, Varname)))  {
                char    **evec = makevec(name);
                init_defaults();
                cjob = &default_pend;
                JREQ = default_pend.job;
                Mode_arg = &default_pend.job->h.bj_mode;
                ret = my_doopts(evec);
                free(evec[0]);
                free((char *) evec);
                free(name);
                had++;
        }

        free(filename);

        if  (!had)
                return;

        checksetmode(0, (const USHORT *) 0, mypriv->btu_jflags[0], &default_job.h.bj_mode.u_flags);
        checksetmode(1, (const USHORT *) 0, mypriv->btu_jflags[1], &default_job.h.bj_mode.g_flags);
        checksetmode(2, (const USHORT *) 0, mypriv->btu_jflags[2], &default_job.h.bj_mode.o_flags);
        fixcsvars();
        default_job.h.bj_nargs = Argcnt;
        default_job.h.bj_nredirs = Redircnt;
        if  (!packjstring(&default_job, Curr_pwd, job_title, Redirs, Envs, Args))  {
                ret = (char **) 0;
                arg_errnum = $EH{Too many job strings defaults};
        }
        cleanupspace((char **) 0);
        repmnthfix(&default_job.h.bj_times);

        if  (!ret)  {
                time_t          now;

                if  (w)
                        doerror(jwid, arg_errnum);
                else
                        print_error(arg_errnum);

                /* Clear these out so we don't get left with something
                   half-baked. Silently adjust the time.  */

                default_job.h.bj_title = default_job.h.bj_direct = -1;
                default_job.h.bj_nargs = default_job.h.bj_nenv = default_job.h.bj_nredirs = 0;
                BLOCK_ZERO(default_job.h.bj_conds, sizeof(default_job.h.bj_conds));
                BLOCK_ZERO(default_job.h.bj_asses, sizeof(default_job.h.bj_asses));
                time(&now);
                if  (default_job.h.bj_times.tc_istime  &&  default_job.h.bj_times.tc_nexttime < now)
                        default_job.h.bj_times.tc_nexttime = now + 60L;
        }
        if  (w  &&  default_job.h.bj_times.tc_istime  &&  default_job.h.bj_times.tc_nexttime < time((time_t *) 0))
                doinfo(w, $E{xmbtr default not future});
}

void  init_defaults()
{
        int     ret;
        char    **sq_env;
        extern  char    **environ, **xenviron;

        default_pend.job = &default_job;
        default_pend.userid = Realuid;
        default_pend.grpid = Realgid;
        default_pend.directory = stracpy(Curr_pwd);
        default_job.h.bj_slotno = -1;
        default_job.h.bj_umask = Save_umask = umask(C_MASK);
        default_job.h.bj_ulimit = 0L;
        default_job.h.bj_ll = Ci_list[CI_STDSHELL].ci_ll;
        strcpy(default_job.h.bj_cmdinterp, Ci_list[CI_STDSHELL].ci_name);
        default_job.h.bj_times.tc_repeat = TC_DELETE;
        default_job.h.bj_times.tc_nposs = TC_WAIT1;
        default_job.h.bj_times.tc_nvaldays = (USHORT) defavoid;
        default_job.h.bj_exits.elower = 1;
        default_job.h.bj_exits.eupper = 255;
        default_job.h.bj_pri = mypriv->btu_defp;
        default_job.h.bj_mode.u_flags = mypriv->btu_jflags[0];
        default_job.h.bj_mode.g_flags = mypriv->btu_jflags[1];
        default_job.h.bj_mode.o_flags = mypriv->btu_jflags[2];
        default_job.h.bj_nargs = 0;
        default_job.h.bj_nredirs = 0;
        cjob = &default_pend;
        JREQ = default_pend.job;
        sq_env = squash_envir(xenviron, environ);
        ret = read_envir(sq_env);
        if  (sq_env != environ)
                free((char *) sq_env);
        if  (ret)  {
                print_error(ret);
                exit(E_LIMIT);
        }
        packjstring(&default_job, Curr_pwd, (char *) 0, (Mredir *) 0, Envs, (char **) 0);
}

void  load_options()
{
        btr_avec = helpargs(Adefs, $A{btr arg explain}, $S{Largest btr arg});
        makeoptvec(btr_avec, $A{btr arg explain}, $S{Largest btr arg});
        cb_loaddefs((Widget) 0, 1);
        cb_loaddefs((Widget) 0, 0);
}

#include "inline/spopts_jd.c"

void  cb_savedefs(Widget w, int ishomed)
{
        int     ret;

        if  (ishomed)  {
                ret = proc_save_opts((const char *) 0, Varname, spit_options);
                disp_str = "(Home)";
        }
        else  {
                ret = proc_save_opts(Curr_pwd, Varname, spit_options);
                disp_str = Curr_pwd;
        }

        if  (ret)
                doerror(jwid, ret);
}
