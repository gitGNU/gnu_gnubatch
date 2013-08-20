/* xmlinit.c -- Initialise XML library.

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
#ifdef  HAVE_LIBXML2
#include <libxml/tree.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "jvuprocs.h"
#include "xmlldsv.h"

#ifdef  HAVE_LIBXML2

static  void    err_rout(void *ctx, const char *msg, ...)
{
        return;
}

/* Don't quite understand why this has to be a pointer to a pointer but.... */

static  xmlGenericErrorFunc funcarray[] = { err_rout };
#endif

void    init_xml()
{
#ifdef  HAVE_LIBXML2
        initGenericErrorDefaultFunc(funcarray);
#endif
}
