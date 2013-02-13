/* getxbuf.c -- get transfer buffer index

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
#include <errno.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifndef USING_FLOCK
#include <sys/ipc.h>
#include <sys/sem.h>
#endif
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#include "defaults.h"
#include "incl_unix.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "ipcstuff.h"
#include "errnums.h"
#include "ecodes.h"
#include "files.h"

struct  Transportbuf    *Xbuffer;

#ifdef  USING_FLOCK
static  int     Xfd;

static void  setlck(const unsigned startl, const unsigned lng)
{
        struct  flock   lck;
        lck.l_type = F_WRLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = startl;
        lck.l_len = lng;
        for  (;;)  {
                if  (fcntl(Xfd, F_SETLKW, &lck) >= 0)
                        return;
                if  (errno != EINTR)  {
                        print_error($E{Lock error xfer});
                        exit(E_NOMEM);
                }
        }
}

static void  unsetlck(const unsigned startl, const unsigned lng)
{
        struct  flock   lck;
        lck.l_type = F_UNLCK;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = startl;
        lck.l_len = lng;
        for  (;;)  {
                if  (fcntl(Xfd, F_SETLKW, &lck) >= 0)
                        return;
                if  (errno != EINTR)  {
                        print_error($E{Unlock error});
                        exit(E_NOMEM);
                }
        }
}

#else

extern  int     Sem_chan;

static  struct  sembuf
lsem[1] =       {{TQ_INDEX, -1, SEM_UNDO        }},
ssem[2] =       {{TQ_INDEX,  1, SEM_UNDO        },
                 {       0, -1, SEM_UNDO        }},
ussem[1] =      {{       0,  1, SEM_UNDO        }};
#endif

ULONG  getxbuf()
{
        ULONG   result;

        /* Wait for activity to cease....  */

#ifdef  USING_FLOCK
        setlck(0, sizeof(ULONG));
#else
        while  (semop(Sem_chan, lsem, sizeof(lsem)/sizeof(struct sembuf)) < 0)  {
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_NOMEM);
        }
#endif

        /* Increment for next customer and perhaps wrap round */

        result = Xbuffer->Next;
        if  (++Xbuffer->Next >= XBUFJOBS)
                Xbuffer->Next = 0;
#ifdef  USING_MMAP
        msync((char *) Xbuffer, sizeof(ULONG), MS_ASYNC|MS_INVALIDATE);
#endif

        /* Free the count, and lock the one we've chosen.  Note that
           this may suspend and will also suspend the lock
           semaphore but this will only hold up consumers (we hope) */

#ifdef  USING_FLOCK
        unsetlck(0, sizeof(ULONG));
        setlck((char *) &Xbuffer->Ring[result] - (char *) Xbuffer, sizeof(Btjob));
#else
        ssem[1].sem_num = (short) result + TQ_INDEX + 1;
        while  (semop(Sem_chan, ssem, sizeof(ssem)/sizeof(struct sembuf)) < 0)  {
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_NOMEM);
        }
#endif
        return  result;
}

void  freexbuf(const ULONG n)
{
#ifdef  USING_FLOCK
        unsetlck((char *) &Xbuffer->Ring[n] - (char *) Xbuffer, sizeof(Btjob));
#else
        ussem[0].sem_num = (short) n + TQ_INDEX + 1;
        while  (semop(Sem_chan, ussem, sizeof(ussem)/sizeof(struct sembuf)) < 0)  {
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_NOMEM);
        }
#endif
}

#ifndef USING_FLOCK
ULONG  getxbuf_serv()
{
        ULONG   result;

        lsem[0].sem_flg = ssem[0].sem_flg = ssem[1].sem_flg = ussem[0].sem_flg = 0;

        /* Wait for activity to cease....  */

        while  (semop(Sem_chan, lsem, sizeof(lsem)/sizeof(struct sembuf)) < 0)
                if  (errno != EINTR)
                        exit(E_NOMEM);

        /* Increment for next customer and perhaps wrap round */

        result = Xbuffer->Next;
        if  (++Xbuffer->Next >= XBUFJOBS)
                Xbuffer->Next = 0;

        /* Free the count, and lock the one we've chosen.  Note that
           this may suspend and will also suspend the lock
           semaphore but this will only hold up consumers (we hope) */

        ssem[1].sem_num = (short) result + TQ_INDEX + 1;
        while  (semop(Sem_chan, ssem, sizeof(ssem)/sizeof(struct sembuf)) < 0)
                if  (errno != EINTR)
                        exit(E_NOMEM);
        return  result;
}

void  freexbuf_serv(const ULONG n)
{
        ussem[0].sem_num = (short) n + TQ_INDEX + 1;
        while  (semop(Sem_chan, ussem, sizeof(ussem)/sizeof(struct sembuf)) < 0)
                if  (errno != EINTR)
                        exit(E_NOMEM);
}
#endif

#ifdef  USING_MMAP

static  long    xbuf_size;

void  sync_xfermmap()
{
        msync((char *) Xbuffer, xbuf_size, MS_ASYNC|MS_INVALIDATE);
}
#endif

void  initxbuffer(const int inbdir)
{
        char    *buffer;
#ifdef  USING_MMAP
#ifndef USING_FLOCK
        int     Xfd;
#endif

        if  (inbdir)
                Xfd = open(XFMMAP_FILE, O_RDWR);
        else  {
                char    *fname = mkspdirfile(XFMMAP_FILE);
                Xfd = open(fname, O_RDWR);
                free(fname);
        }
        if  (Xfd < 0)  {
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }

        fcntl(Xfd, F_SETFD, 1);
        if  ((buffer = mmap(0, xbuf_size = lseek(Xfd, 0L, 2), PROT_READ|PROT_WRITE, MAP_SHARED, Xfd, 0)) == MAP_FAILED)  {
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }
#else
        int     Xchan;

#ifdef  USING_FLOCK
        if  (inbdir)
                Xfd = open(XLOCK_FILE, O_RDWR);
        else  {
                char    *fname = mkspdirfile(XLOCK_FILE);
                Xfd = open(fname, O_RDWR);
                free(fname);
        }
        if  (Xfd < 0)
                goto  fail;
        fcntl(Xfd, F_SETFD, 1);
#endif
        if  ((Xchan = shmget((key_t) TRANSHMID + envselect_value, 0, 0)) < 0)  {
#ifdef  USING_FLOCK
        fail:
#endif
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }
        if  ((buffer = shmat(Xchan, (char *) 0, 0)) == (char *) -1)  {
                print_error($E{Cannot open jshm});
                exit(E_JOBQ);
        }
#endif
        Xbuffer = (struct Transportbuf *) buffer;
}
