/* rvfile.c -- read variable list (options with and without sorting)

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
#include <sys/ipc.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  USING_MMAP
#include <sys/mman.h>
#else
#include <sys/shm.h>
#endif
#ifndef USING_FLOCK
#include <sys/sem.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "btmode.h"
#include "btconst.h"
#include "btvar.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "ecodes.h"
#include "jvuprocs.h"
#include "optflags.h"
#include "files.h"

static  char    Filename[] = __FILE__;

/* Define these externals here rather than in main mods.  */

struct  vshm_info       Var_seg;
struct  svent           *vv_ptrs;
ULONG                   Last_v_ser;

/* Initialise Var_seg pointers, we set offset of shm in Var_seg.dptr */

static  void    init_varseg()
{
        Var_seg.inf.seg = (char *) Var_seg.dptr;
        Var_seg.Nvars = Var_seg.dptr->vs_maxvars;
        Var_seg.vhash = (vhash_t *) (Var_seg.inf.seg + sizeof(struct vshm_hdr));
        Var_seg.vidhash = (vhash_t *) ((char *) Var_seg.vhash + VAR_HASHMOD * sizeof(vhash_t));
        Var_seg.vlist = (struct Ventry *) ((char *) Var_seg.vidhash + VAR_HASHMOD * sizeof(vhash_t));
}

#ifdef  USING_FLOCK

static void  setvhold(const int typ)
{
        struct  flock   lck;
        lck.l_type = typ;
        lck.l_whence = 0;       /* I.e. SEEK_SET */
        lck.l_start = 0;
        lck.l_len = 0;
        for  (;;)  {
#ifdef  USING_MMAP
                if  (fcntl(Var_seg.inf.mmfd, F_SETLKW, &lck) >= 0)
                        return;
#else
                if  (fcntl(Var_seg.inf.lockfd, F_SETLKW, &lck) >= 0)
                        return;
#endif
                if  (errno != EINTR)
                        ABORT_NOMEM;
        }
}

#else

struct  sembuf
vr[3] = {{      VQ_READING,     1,      SEM_UNDO        },
        {       VQ_FIDDLE,      -1,     0               },
        {       VQ_FIDDLE,      1,      0               }},
vu[1] = {{      VQ_READING,     -1,     SEM_UNDO        }};
#endif

void  vlock()
{
#ifdef  USING_FLOCK
        setvhold(F_RDLCK);
#else
        for  (;;)  {
                if  (semop(Sem_chan, &vr[0], 3) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_VARL);
        }
#endif
}

void  vunlock()
{
#ifdef  USING_FLOCK
        setvhold(F_UNLCK);
#else
        for  (;;)  {
                if  (semop(Sem_chan, &vu[0], 1) >= 0)
                        return;
                if  (errno == EINTR)
                        continue;
                print_error($E{Semaphore error probably undo});
                exit(E_VARL);
        }
#endif
}

/* Find the segment */

void  openvfile(const int inbdir, const int isserv)
{
        char    *buffer;
#ifdef  USING_MMAP
        int     Mfd;

        if  (inbdir)
                Mfd = open(VMMAP_FILE, O_RDONLY);
        else  {
                char    *fname = mkspdirfile(VMMAP_FILE);
                Mfd = open(fname, O_RDONLY);
                free(fname);
        }
        if  (Mfd < 0)  {
                print_error($E{Cannot open vshm});
                exit(E_VARL);
        }

        fcntl(Mfd, F_SETFD, 1);
        Var_seg.inf.mmfd = Mfd;
        Var_seg.inf.reqsize = Var_seg.inf.segsize = lseek(Mfd, 0L, 2);
        if  ((buffer = mmap(0, Var_seg.inf.segsize, PROT_READ, MAP_SHARED, Mfd, 0)) == MAP_FAILED)  {
                print_error($E{Cannot open vshm});
                exit(E_VARL);
        }
        Var_seg.dptr = (struct vshm_hdr *) buffer;
#else
        int     i;

#ifdef  USING_FLOCK
        if  (inbdir)
                i = open(VLOCK_FILE, O_RDONLY);
        else  {
                char    *fname = mkspdirfile(VLOCK_FILE);
                i = open(fname, O_RDONLY);
                free(fname);
        }
        if  (i < 0)
                goto  fail;
        fcntl(i, F_SETFD, 1);
        Var_seg.inf.lockfd = i;
#endif

        for  (i = 0;  i < MAXSHMS;  i += SHMINC)  {
                Var_seg.inf.base = SHMID + i + VSHMOFF + envselect_value;
        here:
                if  ((Var_seg.inf.chan = shmget((key_t) Var_seg.inf.base, 0, 0)) < 0)
                        continue;
                if  ((buffer = shmat(Var_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                        break;
                Var_seg.dptr = (struct vshm_hdr *) buffer;
                if  (Var_seg.dptr->vs_type != TY_ISVAR)  {
                        shmdt(buffer);
                        continue;
                }
                if  (Var_seg.dptr->vs_nxtid)  {
                        Var_seg.inf.base = Var_seg.dptr->vs_nxtid;
                        shmdt(buffer);
                        goto  here;
                }
                goto  found;
        }
#ifdef  USING_FLOCK
 fail:
#endif
        Var_seg.inf.chan = 0;
        print_error($E{Cannot open vshm});
        exit(E_VARL);

 found:
#endif
        Last_v_ser = 0;
        init_varseg();
        if  ((vv_ptrs = (struct svent *) malloc((Var_seg.Nvars + 1) * sizeof(struct svent))) == (struct svent *) 0)
                ABORT_NOMEM;

#ifndef USING_FLOCK
        /* Fix semaphore flag for servers */

        if  (isserv)
                vr[0].sem_flg = vu[0].sem_flg = 0;
        else
                vr[0].sem_flg = vu[0].sem_flg = SEM_UNDO;
#endif
}

/* Read var file.  */

void  rvarfile(const int andunlock)
{
        char    *buffer = Var_seg.inf.seg;

#ifdef  USING_MMAP

        /* With memory maps we can rely on the current pointer pointing to the
           var header even if the thing has grown, so vs_maxvars will give the
           current maximum. */

        vlock();

        if  (Var_seg.Nvars == Var_seg.dptr->vs_maxvars)  {
                if  (andunlock)
                        vunlock();
                return;
        }

        munmap(buffer, Var_seg.inf.segsize);

        Var_seg.inf.reqsize = Var_seg.inf.segsize = lseek(Var_seg.inf.mmfd, 0L, 2);
        if  ((buffer = mmap(0, Var_seg.inf.segsize, PROT_READ, MAP_SHARED, Var_seg.inf.mmfd, 0)) == MAP_FAILED)
                ABORT_NOMEM;
        Var_seg.dptr = (struct vshm_hdr *) buffer;
#else
        vlock();

        if  (!Var_seg.dptr->vs_nxtid)  {
                if  (andunlock)
                        vunlock();
                return;
        }

        /* This copes with case where the var segment has moved.  */

        do  {   /* Well it might move again (unlikely - but...) */
                Var_seg.inf.base = Var_seg.dptr->vs_nxtid;
                shmdt(buffer);  /*  Lose old one  */

                if  ((Var_seg.inf.chan = shmget((key_t) Var_seg.inf.base, 0, 0)) <= 0  ||
                     (buffer = shmat(Var_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                        ABORT_NOMEM;
                Var_seg.dptr = (struct vshm_hdr *) buffer;
                if  (Var_seg.dptr->vs_type != TY_ISVAR)
                        ABORT_NOMEM;
        }  while  (Var_seg.dptr->vs_nxtid);

        /* Reinitialise pointers */

#endif

        init_varseg();
        if  (andunlock)
                vunlock();
}

/* Look up named var in file */

vhash_t  lookupvar(const char *name, const netid_t hostid, const unsigned perm, ULONG *Sseqp)
{
        int     hadmatch = 0;
        vhash_t hp;
        struct  Ventry  *fp;

        vlock();
        for  (hp = Var_seg.vhash[calchash(name)];  hp >= 0;  hp = fp->Vnext)  {
                fp = &Var_seg.vlist[hp];
                if  (fp->Vent.var_id.hostid == hostid && strcmp(fp->Vent.var_name, name) == 0)  {
                        hadmatch++;
                        if  (mpermitted(&fp->Vent.var_mode, perm, 0))  {
                                *Sseqp = fp->Vent.var_sequence;
                                vunlock();
                                return  hp;
                        }
                }
                else  if  (hadmatch)
                        break;
        }
        vunlock();
        return  -1;
}

static int  sort_v(struct svent *a, struct svent *b)
{
        int     s;
        return  (s = strcmp(a->vep->Vent.var_name, b->vep->Vent.var_name)) != 0? s:
                a->vep->Vent.var_id.hostid < b->vep->Vent.var_id.hostid ? -1:
                a->vep->Vent.var_id.hostid == b->vep->Vent.var_id.hostid ? 0: 1;
}

/* Read var file and sort */

void  rvarlist(const int andunlock)
{
        struct  Ventry  *vp, *ve;

        vlock();

#ifdef USING_MMAP
        if  (Var_seg.Nvars == Var_seg.dptr->vs_maxvars)  {

                char    *buffer = Var_seg.inf.seg;

                munmap(buffer, Var_seg.inf.segsize);
                Var_seg.inf.reqsize = Var_seg.inf.segsize = lseek(Var_seg.inf.mmfd, 0L, 2);
                if  ((buffer = mmap(0, Var_seg.inf.segsize, PROT_READ, MAP_SHARED, Var_seg.inf.mmfd, 0)) == MAP_FAILED)
                        ABORT_NOMEM;
                Var_seg.dptr = (struct vshm_hdr *) buffer;
                init_varseg();
                free((char *) vv_ptrs);
                if  ((vv_ptrs = (struct svent *) malloc((Var_seg.Nvars + 1) * sizeof(struct svent))) == (struct svent *) 0)
                        ABORT_NOMEM;
                Last_v_ser = 0;
        }
#else
        if  (Var_seg.dptr->vs_nxtid)  {

                /* This copes with case where the var segment has moved.  */

                char    *buffer = Var_seg.inf.seg;

                do  {   /* Well it might move again (unlikely - but...) */
                        Var_seg.inf.base = Var_seg.dptr->vs_nxtid;
                        shmdt(buffer);  /*  Lose old one  */

                        if  ((Var_seg.inf.chan = shmget((key_t) Var_seg.inf.base, 0, 0)) <= 0  ||
                             (buffer = shmat(Var_seg.inf.chan, (char *) 0, SHM_RDONLY)) == (char *) -1)
                                ABORT_NOMEM;
                        Var_seg.dptr = (struct vshm_hdr *) buffer;
                        if  (Var_seg.dptr->vs_type != TY_ISVAR)
                                ABORT_NOMEM;
                }  while  (Var_seg.dptr->vs_nxtid);

                /* Reinitialise pointers */

                init_varseg();
                free((char *) vv_ptrs);
                if  ((vv_ptrs = (struct svent *) malloc((Var_seg.Nvars + 1) * sizeof(struct svent))) == (struct svent *) 0)
                        ABORT_NOMEM;
                Last_v_ser = 0;
        }
#endif

        /* Do nothing if no changes have taken place */

        if  (Var_seg.dptr->vs_serial == Last_v_ser)  {
                if  (andunlock)
                        vunlock();
                return;
        }
        Last_v_ser = Var_seg.dptr->vs_serial;

        Var_seg.nvars = 0;              /* Ones we're interested in */
        ve = &Var_seg.vlist[Var_seg.dptr->vs_maxvars];

        for  (vp = &Var_seg.vlist[0]; vp < ve;  vp++)  {
                if  (!vp->Vused)
                        continue;
                if  (!visible(&vp->Vent.var_mode))
                        continue;
                if  (Dispflags & DF_LOCALONLY && vp->Vent.var_id.hostid != 0)
                        continue;
                if  (Restru && !qmatch(Restru, vp->Vent.var_mode.o_user))
                        continue;
                if  (Restrg && !qmatch(Restrg, vp->Vent.var_mode.o_group))
                        continue;
                vv_ptrs[Var_seg.nvars].vep = vp;
                vv_ptrs[Var_seg.nvars].place = vp - Var_seg.vlist;
                Var_seg.nvars++;
        }
        qsort(QSORTP1 vv_ptrs, Var_seg.nvars, sizeof(struct svent), QSORTP4 sort_v);
        if  (andunlock)
                vunlock();
}
