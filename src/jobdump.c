/* jobdump.c -- internal utility to write out job files

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
#include <sys/stat.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btuser.h"
#include "ecodes.h"
#include "statenums.h"
#include "errnums.h"
#include "helpalt.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "shreq.h"
#include "netmsg.h"
#include "files.h"
#include "helpargs.h"
#include "cfile.h"
#include "jvuprocs.h"
#include "spitrouts.h"
#include "optflags.h"

#define JOBDUMP_INLINE

#define IPC_MODE        0

int     nodelete;

int     Ctrl_chan;

BtjobRef                Wj;
jobno_t         Jobnum;
netid_t         netid;
char            *Dirname;
char            *Xfile, *Jfile;

Shipc           Oreq;
extern          long  mymtype;

static  enum    { PD_DUMPJOB, ASCANC, ASRUNN } pardump = PD_DUMPJOB;
static  enum    { NOVERBOSE, VERBOSE } Verbose = NOVERBOSE;

FILE *net_feed(const int, const netid_t, const jobno_t, const int);

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

void  dumphdrs(BtjobRef jp, FILE *xfl)
{
        fputs(BTR_PROGRAM, xfl);
        spitbtrstr(jp->h.bj_progress == BJP_DONE? $A{btr arg done}:
                   jp->h.bj_progress < BJP_DONE? $A{btr arg norm}: $A{btr arg canc}, xfl, 1);
        spitbtrstr($A{btr arg pri}, xfl, 0);
        fprintf(xfl, "%d \\\n", jp->h.bj_pri);
        if  ((jp->h.bj_jflags & (BJ_WRT|BJ_MAIL)) != (BJ_WRT|BJ_MAIL))
                spitbtrstr($A{btr arg nomess}, xfl, 1);
        if  (jp->h.bj_jflags & BJ_WRT)
                spitbtrstr($A{btr arg write}, xfl, 1);
        if  (jp->h.bj_jflags & BJ_MAIL)
                spitbtrstr($A{btr arg mail}, xfl, 1);
        if  (jp->h.bj_jflags & BJ_REMRUNNABLE)
                spitbtrstr($A{btr arg fullexport}, xfl, 1);
        else  if  (jp->h.bj_jflags & BJ_EXPORT)
                spitbtrstr($A{btr arg export}, xfl, 1);
        else
                spitbtrstr($A{btr arg loco}, xfl, 1);
        if  (jp->h.bj_title >= 0)  {
                char    *title = &jp->bj_space[jp->h.bj_title];
                char    *colp;
                if  ((colp = strchr(title, ':')))  {
                        spitbtrstr($A{btr arg queue}, xfl, 0);
                        fprintf(xfl, "\'%.*s\' \\\n", (int) (colp - title), title);
                        title = colp + 1;
                }
                spitbtrstr($A{btr arg title}, xfl, 0);
                fprintf(xfl, "\'%s\' \\\n", title);
        }
        spitbtrstr($A{btr arg mode}, xfl, 0);
        dumpmode(xfl, "U", jp->h.bj_mode.u_flags);
        dumpmode(xfl, ",G", jp->h.bj_mode.g_flags);
        dumpmode(xfl, ",O", jp->h.bj_mode.o_flags);
        fputs(" \\\n", xfl);
        dumpconds(xfl, jp->h.bj_conds);
        dumpasses(xfl, jp->h.bj_asses);
        spitbtrstr($A{btr arg interp}, xfl, 0);
        fprintf(xfl, "%s \\\n", jp->h.bj_cmdinterp);
        spitbtrstr($A{btr arg ll}, xfl, 0);
        fprintf(xfl, "%d \\\n", jp->h.bj_ll);
        dumptime(xfl, &jp->h.bj_times);
}

void  dumpjob(BtjobRef jp)
{
        FILE    *ifl, *xfl, *jfl;
        unsigned        oldumask, cnt;
        int     ch;

        if  (netid)
                ifl = net_feed(FEED_JOB, netid, Jobnum, Job_seg.dptr->js_viewport);
        else
                ifl = fopen(mkspid(SPNAM, Jobnum), "r");

        if  (ifl == (FILE *) 0)
                exit(E_JDFNFND);

#ifdef  HAVE_SETEUID
        seteuid(Realuid);
        if  (chdir(Dirname) < 0)
                exit(E_JDNOCHDIR);
        oldumask = umask(0);
        if  ((xfl = fopen(Xfile, "w")) == (FILE *) 0)
                exit(E_JDFNOCR);
        if  ((jfl = fopen(Jfile, "w")) == (FILE *) 0)
                exit(E_JDFNOCR);
#ifdef  HAVE_FCHMOD
        Ignored_error = fchmod(fileno(xfl), (int) (0777 & ~oldumask));
        Ignored_error = fchmod(fileno(jfl), (int) (0666 & ~oldumask));
#else
        Ignored_error = chmod(Xfile, (int) (0777 & ~oldumask));
        Ignored_error = chmod(Jfile, (int) (0666 & ~oldumask));
#endif
        seteuid(Daemuid);
#else  /* !HAVE_SETEUID */
#ifdef  ID_SWAP
#if     defined(NHONSUID) || defined(DEBUG)
        if  (Daemuid != ROOTID  &&  Realuid != ROOTID  &&  Effuid != ROOTID)  {
#else
        if  (Daemuid != ROOTID  &&  Realuid != ROOTID)  {
#endif
                setuid(Realuid);
                if  (chdir(Dirname) < 0)
                        exit(E_JDNOCHDIR);
                oldumask = umask(0);
                if  ((xfl = fopen(Xfile, "w")) == (FILE *) 0)
                        exit(E_JDFNOCR);
                if  ((jfl = fopen(Jfile, "w")) == (FILE *) 0)
                        exit(E_JDFNOCR);
#ifdef  HAVE_FCHMOD
                Ignored_error = fchmod(fileno(xfl), (int) (0777 & ~oldumask));
                Ignored_error = fchmod(fileno(jfl), (int) (0666 & ~oldumask));
#else
                Ignored_error = chmod(Xfile, (int) (0777 & ~oldumask));
                Ignored_error = chmod(Jfile, (int) (0666 & ~oldumask));
#endif
                setuid(Daemuid);
        }
        else  {
#endif  /* ID_SWAP */
                if  (chdir(Dirname) < 0)
                        exit(E_JDNOCHDIR);
                oldumask = umask(0);
                if  ((xfl = fopen(Xfile, "w")) == (FILE *) 0)
                        exit(E_JDFNOCR);
                if  ((jfl = fopen(Jfile, "w")) == (FILE *) 0)
                        exit(E_JDFNOCR);
#ifdef  HAVE_FCHMOD
                Ignored_error = fchmod(fileno(xfl), (int) (0777 & ~oldumask));
                Ignored_error = fchmod(fileno(jfl), (int) (0666 & ~oldumask));
#else
                Ignored_error = chmod(Xfile, (int) (0777 & ~oldumask));
                Ignored_error = chmod(Jfile, (int) (0666 & ~oldumask));
#endif
#if     defined(HAVE_FCHOWN) && !defined(M88000)
                Ignored_error = fchown(fileno(xfl), Realuid, Realgid);
                Ignored_error = fchown(fileno(jfl), Realuid, Realgid);
#else
                Ignored_error = chown(Xfile, Realuid, Realgid);
                Ignored_error = chown(Jfile, Realuid, Realgid);
#endif
#ifdef  ID_SWAP
        }
#endif
#endif /* !HAVE_SETEUID */

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

        dumphdrs(jp, xfl);

        if  (jp->h.bj_mode.o_uid != Realuid)  {
                spitbtrstr($A{btr arg setu}, xfl, 0);
                fprintf(xfl, "%s \\\n", jp->h.bj_mode.o_user);
        }
        if  (jp->h.bj_mode.o_gid != Realgid)  {
                spitbtrstr($A{btr arg setg}, xfl, 0);
                fprintf(xfl, "%s \\\n", jp->h.bj_mode.o_group);
        }

        if  (jp->h.bj_direct >= 0)  {
                spitbtrstr($A{btr arg dir}, xfl, 0);
                fprintf(xfl, "%s \\\n", &jp->bj_space[jp->h.bj_direct]);
        }
        spitbtrstr(jp->h.bj_jflags & BJ_NOADVIFERR? $A{btr arg noadverr}: $A{btr arg adverr}, xfl, 1);

        dumpecrun(xfl, jp);
        dumpredirs(xfl, jp);
        dumpargs(xfl, jp);

        /* Fix for GTK so we can have job and command files in different places.
           If abs path name don't put directory in front */

        if  (Jfile[0] == '/')
                fprintf(xfl, "%s\n", Jfile);
        else
                fprintf(xfl, "%s/%s\n", Dirname, Jfile);
        fclose(xfl);
        while  ((ch = getc(ifl)) != EOF)
                putc(ch, jfl);
        fclose(ifl);
        fclose(jfl);
}

void  deljob(BtjobRef jp)
{
        if  (nodelete)
                return;
        Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
        Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
        Oreq.sh_params.mcode = J_DELETE;
        msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), 0);
        if  (readreply() == J_OK)
                return;
        exit(E_CANTDEL);
}

#include "inline/btradefs.c"
#include "inline/spopts_jd.c"

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char            *spdir, *colp;
        unsigned        jind;
        BtjobRef        jp;
        HelpargRef      helpa;
        FILE            *ignored;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif

        versionprint(argv, "$Revision: 1.9 $", 1);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();
        ignored = freopen("/dev/null", "w", stderr);

        Realuid = getuid();
        Effuid = geteuid();
        Realgid = getgid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        helpa = helpargs(Adefs, $A{btr arg explain}, $S{Largest btr arg});
        makeoptvec(helpa, $A{btr arg explain}, $S{Largest btr arg});

        repunit = helprdalt($Q{Repeat unit abbrev});
        days_abbrev = helprdalt($Q{Weekdays});
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        if  (!repunit || !days_abbrev)
                exit(E_SETUP);

        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);

        /* This is only called internally, so don't bother with messages.  */

        Dirname = argv[2];              /* Check validity and replace ~ in 6-arg case */

        if  (argc == 4)  {
                char    *arg = argv[1];
                if  (arg[0] != '-'  ||  arg[2] != '\0')
                        exit(E_USAGE);
                switch  (arg[1])  {
                default:
                        exit(E_USAGE);
                case  'c':
                        pardump = ASCANC;
                        Verbose = NOVERBOSE;
                        nodelete++;
                        break;
                case  'C':
                        pardump = ASCANC;
                        Verbose = VERBOSE;
                        nodelete++;
                        break;
                case  'n':
                        pardump = ASRUNN;
                        Verbose = NOVERBOSE;
                        nodelete++;
                        break;
                case  'N':
                        pardump = ASRUNN;
                        Verbose = VERBOSE;
                        nodelete++;
                        break;
                }
                if  ((colp = strchr(argv[3], ':')))  {
                        *colp = '\0';
                        if  ((netid = look_int_hostname(argv[3])) == -1)
                                exit(E_USAGE);
                        Jobnum = atol(colp+1);
                }
                else
                        Jobnum = atol(argv[3]);

                /* In this case only, replace ~ directory name with null to
                   select the home directory version */

                if  (strcmp(Dirname, "~") == 0)
                        Dirname = (char *) 0;
        }
        else  {
                if  (argc == 6)  {
                        if  (strcmp(argv[1], "-n") == 0)
                                nodelete++;
                        argv++;
                        Dirname = argv[2];			/* Reposition */
                }
                else  if  (argc != 5)
                        exit(E_USAGE);

                if  ((colp = strchr(argv[1], ':')))  {
                        *colp = '\0';
                        if  ((netid = look_int_hostname(argv[1])) == -1)
                                exit(E_USAGE);
                        Jobnum = atol(colp+1);
                }
                else
                        Jobnum = atol(argv[1]);
                Xfile = argv[3];
                Jfile = argv[4];
        }

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)
                exit(E_NOCHDIR);
        free(spdir);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)
                exit(E_NOTRUN);

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)
                exit(E_SETUP);
#endif

        openvfile(1, 0);
        openjfile(1, 0);
        rvarfile(0);
        rjobfile(0);            /* NB Leave locked */

        /* Set up scheduler request main parameters.  Note that
           uid/gid etc should still be that of the real uid who
           invoked btq.  */

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());

        jind = Job_seg.hashp_jno[jno_jhash(Jobnum)];
        while  (jind != JOBHASHEND)  {
                if  (Job_seg.jlist[jind].j.h.bj_job == Jobnum  &&  Job_seg.jlist[jind].j.h.bj_hostid == netid)
                        goto  gotit;
                jind = Job_seg.jlist[jind].nxt_jno_hash;
        }
        exit(E_JDJNFND);

 gotit:
        jp = &Job_seg.jlist[jind].j;
        junlock();
        mypriv = getbtuentry(Realuid);
        if  (!mpermitted(&jp->h.bj_mode, nodelete? BTM_READ: BTM_READ|BTM_DELETE, mypriv->btu_priv)  &&  !(mypriv->btu_priv & BTM_WADMIN))
                return  E_CANTDEL;

        switch  (pardump)  {
        default:
                /* Dump out job case, Dirname should be something sensible */
                dumpjob(jp);
                deljob(jp);
                break;

        case  ASCANC:
        case  ASRUNN:
                /* Dirname may be null if home directory is intended */
                Wj = jp;
                /* FIXME sometime we don't want to be called BTR */
                if  (proc_save_opts(Dirname, "BTR", spit_options) != 0)
                        exit(E_CANTSAVEO);
                break;
        }
        return  0;
}
