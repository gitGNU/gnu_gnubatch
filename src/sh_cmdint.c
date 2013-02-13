/* sh_cmdint.c -- scheduler command interpreters

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
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "errnums.h"
#include "bjparam.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "btjob.h"
#include "btvar.h"
#include "network.h"
#include "cmdint.h"
#include "shreq.h"
#include "netmsg.h"
#include "btuser.h"
#include "q_shm.h"
#include "sh_ext.h"

/* Initialise command interpreter file if it doesn't exist */

void  initcifile()
{
        char    *cifile;
        BtuserRef       g;
        Cmdint          madeup;

        if  (!open_ci(O_RDONLY))
                return;

        cifile = envprocess(BTCI);
        disp_str = cifile;
        if  ((Ci_fd = open(cifile, O_RDWR|O_CREAT|O_TRUNC, CMODE)) < 0)
                panic($E{Panic cannot create CI file});

#ifdef  HAVE_FCHOWN
        if  (Daemuid != ROOTID)
                Ignored_error = fchown(Ci_fd, Daemuid, Daemgid);
#else
        if  (Daemuid != ROOTID)
                Ignored_error = chown(cifile, Daemuid, Daemgid);
#endif
        free(cifile);

        /* Initialise one entry, to be "/bin/sh" */

        strcpy(madeup.ci_name, DEF_CI_NAME);
        strcpy(madeup.ci_path, DEF_CI_PATH);
        strcpy(madeup.ci_args, DEF_CI_ARGS);
        madeup.ci_ll = (g = getbtuentry(Daemuid))? g->btu_spec_ll: U_DF_SPECLL;
        madeup.ci_nice = DEF_CI_NICE;
        madeup.ci_flags = 0;
        Ignored_error = write(Ci_fd, (char *) &madeup, sizeof(madeup));
        close(Ci_fd);
        if  (open_ci(O_RDONLY))
                panic($E{Panic cannot read CI file});
}
