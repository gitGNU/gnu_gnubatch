/* jvuprocs.h -- For remembering where vars came from when sorting them.

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

struct  svent   {
        vhash_t place;
        struct  Ventry  *vep;
};

extern  BtjobRef        *jj_ptrs;
extern  struct  svent   *vv_ptrs;

extern  ULONG   Last_j_ser,
                Last_v_ser;

extern  BtmodeRef       Mode_arg;
extern  BtjobRef        JREQ;

#define visible(md)     mpermitted(md, BTM_SHOW, 0)

void  freexbuf(const ULONG);
#ifdef  USING_FLOCK
#define freexbuf_serv   freexbuf
#else
void  freexbuf_serv(const ULONG);
#endif
void  initxbuffer(const int);
void  jlock();
void  junlock();
void  vlock();
void  vunlock();
void  openjfile(const int, const int);
void  openvfile(const int, const int);
void  rjobfile(const int);
void  rvarfile(const int);
void  rvarlist(const int);

extern  vhash_t lookupvar(const char *, const netid_t, const unsigned, ULONG *);
ULONG  getxbuf();
#ifdef  USING_FLOCK
#define getxbuf_serv    getxbuf
#else
extern  int     Sem_chan;               /* Id of semaphore used for locking */
ULONG  getxbuf_serv();
#endif
#ifdef  USING_MMAP
void  sync_xfermmap();
#endif
