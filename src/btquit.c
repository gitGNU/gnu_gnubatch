/* btquit.c -- main program for gbch-quit

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
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "files.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "cmdint.h"
#include "btvar.h"
#include "shreq.h"
#include "statenums.h"
#include "cfile.h"
#include "ecodes.h"
#include "errnums.h"
#include "ipcstuff.h"

extern  int     Ctrl_chan;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

/* Ye olde main routine. No arguments are expected.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        int     countdown = MSGQ_BLOCKS;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        char    string[80];
        Shipc   oreq;
        Repmess rep;

        versionprint(argv, "$Revision: 1.9 $", 0);

        if  ((progname = strrchr(argv[0], '/')))
                progname++;
        else
                progname = argv[0];

        init_mcfile();

        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        SWAP_TO(Daemuid);

        if  ((Ctrl_chan = msgget(MSGID+envselect_value, 0)) < 0)  {
                print_error($E{Scheduler not running});
                exit(E_NOTRUN);
        }

        /* Obtain confirmation message.  */

        if  (argc > 1)  {
                if  (argc != 2 || argv[1][0] != '-'  ||  (argv[1][1] != 'y'  && argv[1][1] != 'Y') || argv[1][2] != '\0')  {
                        print_error($E{Btquit usage});
                        exit(E_USAGE);
                }
        }
        else  {
                char    *prompt = gprompt($P{Btquit stopped ok});
                char    *ignored;
                fputs(prompt, stdout);
                fflush(stdout);
                ignored = fgets(string, sizeof(string), stdin);
                if  (string[0] != 'y' && string[0] != 'Y')  {
                        print_error($E{Btquit not stopped});
                        exit(0);
                }
        }

        oreq.sh_mtype = TO_SCHED;
        oreq.sh_params.mcode = O_STOP;
        oreq.sh_params.uuid = Realuid;
        oreq.sh_params.ugid = Realgid;
        oreq.sh_params.upid = getpid();
        oreq.sh_params.param = $S{Scheduler normal exit};

        while  (msgsnd(Ctrl_chan, (struct msgbuf *) &oreq, sizeof(Shreq), IPC_NOWAIT) < 0)  {
                switch  (errno)  {
                default:
                        print_error($E{Btquit IPC error});
                        exit(E_SETUP);

                case  EAGAIN:
                        print_error($E{IPC msg q error});
                        if  (--countdown < 0)  {
                                print_error($E{Btquit giving up});
                                exit(E_SETUP);
                        }
                        print_error($E{Waiting as shutting down});
                        sleep(MSGQ_BLOCKWAIT);
                        continue;

                case  EACCES:
                        print_error($E{Check file setup});
                        exit(E_SETUP);
                }
        }

        /* Now wait for reply */

        if  (msgrcv(Ctrl_chan, (struct msgbuf *) &rep, sizeof(Shreq), (long) (MTOFFSET + getpid()), 0) < 0)  {
                switch  (errno)  {
                default:
                        print_error($E{Btquit receive error});
                        exit(E_SETUP);

                case  EINVAL:
                case  EIDRM:
                        print_error($E{Btquit stopped ok});
                        exit(0);

                case  EACCES:
                        print_error($E{Check file setup});
                        exit(E_SETUP);
                }
        }

        switch  (rep.outmsg.mcode)  {
        case  O_OK:
                /* I don't believe this, but for completeness...  */
                print_error($E{Btquit stopped ok});
                exit(0);

        case  O_NOPERM:
                print_error($E{Btquit no perm});
                exit(E_PERM);

        default:
                disp_arg[0] = rep.outmsg.mcode;
                print_error($E{Unexpected sched message});
                exit(E_SHEDERR);
        }
}
