/* xmljobdump.c -- internal utility to write out job files in XML format

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
#include "xmlldsv.h"

#ifdef HAVE_LIBXML2

#define IPC_MODE        0

int     Ctrl_chan;
extern  long  mymtype;

#endif

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

#ifdef HAVE_LIBXML2

static  void    parsejum(char *jobsp, jobno_t *jn, netid_t *nid)
{
        char    *colp = strchr(jobsp, ':');

        *nid = 0;
        if  (colp)  {
                netid_t netid;
                *colp = '\0';
                if  ((netid = look_int_hostname(jobsp)) == -1)
                        exit(E_USAGE);
                *nid = netid;
                jobsp = colp+1;
        }
        *jn = atol(jobsp);
}

static  BtjobRef  findjob(jobno_t jnum, netid_t nid)
{
        unsigned  jind = Job_seg.hashp_jno[jno_jhash(jnum)];

        while  (jind != JOBHASHEND)  {
                if  (Job_seg.jlist[jind].j.h.bj_job == jnum  &&  Job_seg.jlist[jind].j.h.bj_hostid == nid)
                        return  &Job_seg.jlist[jind].j;
                jind = Job_seg.jlist[jind].nxt_jno_hash;
        }
        exit(E_JDJNFND);
}

void  dumpjob(BtjobRef jp, char *dir, char *file, const int progress, const int verbose)
{
        char    *scriptstr;
        unsigned  scriptlen = 0;
        int     oldumask, ret;

        /* We are in the spool directory so this should work.
           We are also in with effuid=batch user
           Bomb if it's not there */

        if  (!(scriptstr = getscript(jp, &scriptlen)))
                exit(E_JDFNFND);

        init_xml();

        /* Flip to the invoking user before writing out the XML file */

#ifdef  HAVE_SETEUID
        seteuid(Realuid);
        if  (chdir(dir) < 0)
                exit(E_JDNOCHDIR);
        oldumask = umask(0);
        if  ((ret = save_job_xml(jp, scriptstr, scriptlen, file, progress, verbose)) == 0)
                Ignored_error = chmod(file, (int) (0666 & ~oldumask));
        seteuid(Daemuid);
#else  /* !HAVE_SETEUID */
#ifdef  ID_SWAP
#if     defined(NHONSUID) || defined(DEBUG)
        if  (Daemuid != ROOTID  &&  Realuid != ROOTID  &&  Effuid != ROOTID)  {
#else
        if  (Daemuid != ROOTID  &&  Realuid != ROOTID)  {
#endif
                setuid(Realuid);
                if  (chdir(dir) < 0)
                        exit(E_JDNOCHDIR);
                oldumask = umask(0);
                if  ((ret = save_job_xml(jp, scriptstr, scriptlen, file, progress, verbose)) == 0)
                        Ignored_error = chmod(file, (int) (0666 & ~oldumask));
                setuid(Daemuid);
        }
        else  {
#endif  /* ID_SWAP */
                if  (chdir(dir) < 0)
                        exit(E_JDNOCHDIR);
                oldumask = umask(0);
                if  ((ret = save_job_xml(jp, scriptstr, scriptlen, file, progress, verbose)) == 0)  {
                        Ignored_error = chmod(file, (int) (0666 & ~oldumask));
                        Ignored_error = chown(file, Realuid, Realgid);
                }
#ifdef  ID_SWAP
        }
#endif
#endif /* !HAVE_SETEUID */
        if  (ret != 0)
                exit(E_JDFNOCR);
}

void  deljob(BtjobRef jp)
{
        Shipc           Oreq;

        BLOCK_ZERO(&Oreq, sizeof(Oreq));
        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.uuid = Realuid;
        Oreq.sh_params.ugid = Realgid;
        mymtype = MTOFFSET + (Oreq.sh_params.upid = getpid());
        Oreq.sh_un.jobref.hostid = jp->h.bj_hostid;
        Oreq.sh_un.jobref.slotno = jp->h.bj_slotno;
        Oreq.sh_params.mcode = J_DELETE;
        msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(jident), 0);
        if  (readreply() != J_OK)
                exit(E_CANTDEL);
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     ch, verbose = 0, nodelete = 0, progress = -1;
        char    *spdir, *jobsp = (char *) 0, *Destdir = (char *) 0, *destfile = (char *) 0;
        FILE            *ignored;
        jobno_t         Jobnum;
        netid_t         netid;
        BtjobRef        jp;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        extern  int     optind;

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
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);

        /* This is only called internally, so don't bother with messages.
           Args are -v turn on verbose -C force cancelled -N force normal
           -j job number -d dest dir -f dest file */

        while  ((ch = getopt(argc, argv, "vnCNj:d:f:")) != EOF)
                switch  (ch)  {
                default:
                        return  E_USAGE;
                case  'v':
                        verbose = 1;
                        continue;
                case  'n':
                        nodelete = 1;
                        continue;
                case  'C':
                        progress = BJP_CANCELLED;
                        continue;
                case  'N':
                        progress = BJP_NONE;
                        continue;
                case  'j':
                        jobsp = optarg;
                        continue;
                case  'd':
                        Destdir = optarg;
                        continue;
                case  'f':
                        destfile = optarg;
                        continue;
                }
        if  (!(Destdir && destfile && jobsp))
                return  E_USAGE;

        parsejum(jobsp, &Jobnum, &netid);

        spdir = envprocess(SPDIR);
        if  (chdir(spdir) < 0)
                return  E_NOCHDIR;
        free(spdir);

        /* Get permissions of invoking user to check he/she is allowed to do this */

        mypriv = getbtuentry(Realuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)
                return  E_NOTRUN;

#ifndef USING_FLOCK
        /* Set up semaphores */

        if  ((Sem_chan = semget(SEMID+envselect_value, SEMNUMS + XBUFJOBS, IPC_MODE)) < 0)
                return  E_SETUP;
#endif

        openvfile(1, 0);
        openjfile(1, 0);
        rvarfile(0);
        rjobfile(0);            /* NB Leave locked */

        jp = findjob(Jobnum, netid);            /* Bombs if it can't find it */
        junlock();

        if  (!mpermitted(&jp->h.bj_mode, nodelete? BTM_READ: BTM_READ|BTM_DELETE, mypriv->btu_priv)  &&  !(mypriv->btu_priv & BTM_WADMIN))
                 return  E_CANTDEL;

        dumpjob(jp, Destdir, destfile, progress, verbose);
        if  (!nodelete)
                deljob(jp);
        return  0;
}
#else
MAINFN_TYPE  main(int argc, char **argv)
{
        versionprint(argv, "$Revision: 1.9 $", 1);
        return  E_NOTIMPL;
}
#endif
