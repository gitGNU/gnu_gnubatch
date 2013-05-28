/* btfilemon.c -- main module for gbch-filemon

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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

/* Deal with all permutations of time stuff...  */

#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <errno.h>
#include <setjmp.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "ecodes.h"
#include "helpargs.h"
#include "errnums.h"
#include "optflags.h"
#include "cfile.h"
#include "incl_dir.h"
#include "ipcstuff.h"
#include "helpalt.h"
#include "filemon.h"

static  char    Filename[] = __FILE__;

#if     (defined(OS_LINUX) || defined(OS_BSDI)) && !defined(_SEM_SEMUN_UNDEFINED)
#define my_semun        semun
#else

/* Define a union as defined in semctl(2) to pass the 4th argument of
   semctl. On most machines it is good enough to pass the value
   but on the SPARC and PYRAMID unions are passed differently
   even if all the possible values would fit in an int.  */

union   my_semun        {
        int     val;
        struct  semid_ds        *buf;
        ushort                  *array;
};
#endif

#ifndef HAVE_LONG_FILE_NAMES
#include <sys/dir.h>
#include <errno.h>
typedef struct  {
        int     dd_fd;
}       DIR;
struct dirent  {
        LONG    d_ino;
        char    d_name[1];
};

#define dirfd(d) ((d)->dd_fd)

/* Big enough to ensure that nulls on end */

static  union  {
        struct  dirent  result_d;
        char    result_b[sizeof(struct direct) + 2];
}  Result;

static  DIR     Res;

DIR     *opendir(filename)
char    *filename;
{
        int     fd;
        struct  stat    sbuf;

        if  ((fd = open(filename, 0)) < 0)
                return  (DIR *) 0;
        if  (fstat(fd, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                errno = ENOTDIR;
                close(fd);
                return  (DIR *) 0;
        }

        Res.dd_fd = fd;
        return  &Res;
}

struct  dirent  *readdir(dirp)
DIR     *dirp;
{
        struct  dirent  *dp, indir;

        while  (read(dirp->dd_fd, (char *)&indir, sizeof(indir)) > 0)  {
                if  (indir.d_ino == 0)
                        continue;
                Result.result_d.d_ino = indir.d_ino;
                strncpy(Result.result_d.d_name, indir.d_name, DIRSIZ);
                return  &Result.result_d;
        }
        return  (struct dirent *) 0;
}

void    seekdir(dirp, loc)
DIR     *dirp;
LONG    loc;
{
        lseek(dirp->dd_fd, (long) loc, 0);
}

#define rewinddir(dirp) seekdir(dirp,0)

int     closedir(dirp)
DIR     *dirp;
{
        return  close(dirp->dd_fd);
}
#endif /* !HAVE_LONG_FILE_NAMES */

enum  wot_prog          wotprog = WP_SETMONITOR;
enum  wot_mode          wotmode = WM_STOP_FOUND;
enum  wot_form          wotform = WF_APPEARS;
enum  wot_file          wotfile = WFL_ANY_FILE;
enum  inc_exist         wotexist = IE_IGNORE_EXIST;

char            isdaemon, recursive, followlinks;

unsigned        grow_time = DEF_GROW_TIME,
                poll_time = DEF_POLL_TIME;

char            *work_directory,                /* Directory we are looking at */
                *Curr_pwd,                      /* Original working directory */
                *file_patt,                     /* File to match */
                *script_file,                   /* File to execute */
                *script_cmd;                    /* Command to invoke as opposed to file */

enum    file_type  {
        FT_EXIST_NOMATCH,       /* Existed at start but doesn't match pattern or not bothering */
        FT_EXIST_MATCH,         /* Existed at start, matches pattern, watch for changes */
        FT_NEW                  /* New file, monitor in progress */
};

struct  recfile  {
        struct  recfile         *next;          /* Next on hash */
        struct  recfile         *prev;          /* Prev on hash */
        enum    file_type       type;           /* Type of file for monitoring */
        short                   notfile;        /* Not a regular file */
        size_t                  size;           /* Size of file at last look */
        time_t                  atime;          /* Access time of file last look */
        time_t                  mtime;          /* Mod time of file last look */
        time_t                  ctime;          /* Change time of file last look */
        time_t                  lastlook;       /* Time we last looked */
        unsigned  long          seq;            /* Have we met it on run */
        char                    name[1];        /* Malloced to appropriate size */
};

#define SCAN_HASHMOD    509
struct  recfile *hashtab[SCAN_HASHMOD];         /* Hash table */
unsigned        file_num,                       /* Number of known files */
                active_num;                     /* Number of active files */

DIR             *Dir_p;
unsigned        long            scan_seq;       /* Number of scans we've done */
time_t          last_mtime;                     /* Of directory we're looking at */

char    Confvarname[] = "FILEMONCONF";

int     sem_chan, shm_chan;
struct  fm_shm_entry    *shm_list,
                        *my_shm_slot;
int     kmcount;
char    **kmlist;

jmp_buf subd_jmp;

RETSIGTYPE  process_alarm(int);

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

struct  sembuf
jw[2] = {{      JQ_READING,     0,      0               },
        {       JQ_FIDDLE,      -1,     SEM_UNDO        }},
jwu[1] = {{     JQ_FIDDLE,      1,      SEM_UNDO        }},
jr[3] = {{      JQ_READING,     1,      SEM_UNDO        },
        {       JQ_FIDDLE,      -1,     0               },
        {       JQ_FIDDLE,      1,      0               }},
jru[1] = {{     JQ_READING,     -1,     SEM_UNDO        }};

void  jwlock()
{
        if  (semop(sem_chan, jw, 2) < 0)  {
                print_error($E{Cannot write lock fm semaphore});
                exit(E_SETUP);
        }
}

void  jwunlock()
{
        semop(sem_chan, jwu, 1);
}

void  jrlock()
{
        if  (semop(sem_chan, jr, 3) < 0)  {
                print_error($E{Cannot read lock fm semaphore});
                exit(E_SETUP);
        }
}

void  jrunlock()
{
        semop(sem_chan, jru, 1);
}

int  make_shmslot(char *dir)
{
        int     cnt, possslot = 30000;

        jwlock();

        for  (cnt = 0;  cnt < MAX_FM_PROCS;  cnt++)  {
                if  (shm_list[cnt].fm_pid == 0)  {
                        if  (cnt < possslot)
                                possslot = cnt;
                }
                else  if  (strcmp(shm_list[cnt].fm_path, dir) == 0)  {
                        disp_str = dir;
                        disp_arg[1] = shm_list[cnt].fm_pid;
                        jwunlock();
                        return  $E{FM Directory name clash};
                }
        }

        if  (possslot >= MAX_FM_PROCS)  {
                jwunlock();
                return  $E{Too many file monitors};
        }

        my_shm_slot = &shm_list[possslot];
        my_shm_slot->fm_pid = getpid();
        my_shm_slot->fm_uid = Realuid;
        my_shm_slot->fm_typ = wotform;
        strncpy(my_shm_slot->fm_path, dir, PATH_MAX-1);
        jwunlock();
        return  0;
}

int  init_shm(const int nocreate)
{
        char    *segp;

        if  ((shm_chan = shmget(FMSHMID, MAX_FM_PROCS*sizeof(struct fm_shm_entry), 0666)) < 0)  {
                union   my_semun        uun;
                ushort  semarray[2];

                if  (nocreate)
                        return  0;

                if  ((shm_chan = shmget(FMSHMID, MAX_FM_PROCS*sizeof(struct fm_shm_entry), 0666|IPC_CREAT|IPC_EXCL)) < 0)  {
                        print_error(errno == EEXIST? $E{Race to create fm shm segment}: $E{Cannot create fm shm segment});
                        exit(E_SETUP);
                }
                if  ((segp = shmat(shm_chan, (char *) 0, 0)) == (char *) -1)  {
                        print_error($E{Cannot attach fm shm segment});
                        exit(E_SETUP);
                }
                BLOCK_ZERO(segp, MAX_FM_PROCS*sizeof(struct fm_shm_entry));
                shm_list = (struct fm_shm_entry *) segp;
                if  ((sem_chan = semget(FMSEMID, 2, 0666|IPC_CREAT|IPC_EXCL)) < 0)  {
                        print_error($E{Cannot create fm semaphore});
                        exit(E_SETUP);
                }
                semarray[JQ_FIDDLE] = 1;
                semarray[JQ_READING] = 0;
                uun.array = semarray;
                if  (semctl(sem_chan, 0, SETALL, uun) < 0)  {
                        print_error($E{Could not reset fm semaphore});
                        exit(E_SETUP);
                }
        }
        else  {
                if  ((segp = shmat(shm_chan, (char *) 0, 0)) == (char *) -1)  {
                        print_error($E{Cannot attach fm shm segment});
                        exit(E_SETUP);
                }
                shm_list = (struct fm_shm_entry *) segp;
                if  ((sem_chan = semget(FMSEMID, 2, 0666)) < 0)  {
                        print_error($E{Cannot open fm semaphore});
                        exit(E_SETUP);
                }
        }
        return  1;
}

void  do_listmons()
{
        HelpaltRef      flist;
        int             flen;
        struct  fm_shm_entry    *fp, *fend;

        if  (!init_shm(1))
                return;

        if  (!(flist = helprdalt($Q{File monitor types})))  {
                disp_arg[9] = $Q{File monitor types};
                print_error($E{Missing alternative code});
                exit(E_SETUP);
        }
        flen = altlen(flist);

        jrlock();
        fend = &shm_list[MAX_FM_PROCS];
        for  (fp = shm_list;  fp < fend;  fp++)
                if  (fp->fm_pid  &&  (Realuid == ROOTID  ||  fp->fm_uid == Realuid))
                        printf("%.5ld %-*s %s\n", (long) fp->fm_pid, flen, disp_alt(fp->fm_typ, flist), fp->fm_path);
        jrunlock();
}

int  do_killmons()
{
        struct  fm_shm_entry    *fp, *fend;
        int     pidcnt = 0, cnt;
        PIDTYPE pidlist[MAX_FM_PROCS];

        if  (!init_shm(1))
                return  E_FALSE;

        jrlock();
        fend = &shm_list[MAX_FM_PROCS];
        for  (fp = shm_list;  fp < fend;  fp++)
                if  (fp->fm_pid  &&  (Realuid == ROOTID  ||  fp->fm_uid == Realuid))
                        for  (cnt = 0;  cnt < kmcount;  cnt++)
                                if  (strcmp(kmlist[cnt], fp->fm_path) == 0)  {
                                        pidlist[pidcnt++] = fp->fm_pid;
                                        break;
                                }
        jrunlock();
        if  (pidcnt <= 0)
                return  E_FALSE;
        for  (cnt = 0;  cnt < pidcnt;  cnt++)
                kill(pidlist[cnt], SIGTERM);
        return  E_TRUE;
}

int  do_killall()
{
        struct  fm_shm_entry    *fp, *fend;
        int     pidcnt = 0, nomatch = 0, cnt;
        PIDTYPE pidlist[MAX_FM_PROCS];

        if  (!init_shm(1))
                return  E_FALSE;

        jrlock();
        fend = &shm_list[MAX_FM_PROCS];
        for  (fp = shm_list;  fp < fend;  fp++)
                if  (fp->fm_pid)  {
                        if  (Realuid == ROOTID  ||  fp->fm_uid == Realuid)
                                pidlist[pidcnt++] = fp->fm_pid;
                        else
                                nomatch++;
                }
        jrunlock();
        for  (cnt = 0;  cnt < pidcnt;  cnt++)
                kill(pidlist[cnt], SIGTERM);
        if  (nomatch <= 0)  {
                union   my_semun        uun;
                uun.val = 0;
                semctl(sem_chan, 0, IPC_RMID, uun);
                shmctl(shm_chan, IPC_RMID, (struct shmid_ds *) 0);
        }
        return  pidcnt > 0? E_TRUE: E_FALSE;
}

/* Like it sez - do bizniz.  */

void  do_bizniz(struct recfile *rp)
{
        if  (fork() != 0)  {    /* Main process either stops if one shot or returns */
                if  (wotmode == WM_STOP_FOUND)
                        exit(0);
#ifdef  BUGGY_SIGCLD
                while  (wait((int *) 0) >= 0)
                        ;
#endif
                return;
        }

#ifdef  BUGGY_SIGCLD
        /* Gryations to avoid zombies... */

        if  (wotmode == WM_CONT_FOUND  &&  fork() != 0)
                _exit(0);
#endif
        if  (script_file)  {
                struct  tm      *tp;
                char    *argv[8], buf1[20], buf2[20], buf3[20];

                sprintf(buf1, "%ld", (long) rp->size);
                argv[0] = DEF_CI_NAME;
                argv[1] = script_file;
                argv[2] = rp->name;
                argv[3] = work_directory;
                argv[4] = buf1;
                argv[5] = argv[6] = argv[7] = (char *) 0;

                switch  (wotform)  {
                case  WF_STOPSUSE:
                        tp = localtime(&rp->atime);
                        goto  trest;

                case  WF_STOPSWRITE:
                        tp = localtime(&rp->mtime);
                        goto  trest;

                case  WF_STOPSCHANGE:
                        tp = localtime(&rp->ctime);
                trest:
                        sprintf(buf2, "%.4d/%.2d/%.2d", tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday);
                        sprintf(buf3, "%.2d:%.2d:%.2d", tp->tm_hour, tp->tm_min, tp->tm_sec);
                        argv[5] = buf2;
                        argv[6] = buf3;
                default:
                        break;
                }
                execv(DEF_CI_PATH, argv);
        }
        else  {                 /* Must be script_cmd */
                char    *argv[4];
                unsigned  ld = strlen(work_directory);
                unsigned  fd = strlen(rp->name);
                char    *sbp, *rbp;
                char    rbuf[PATH_MAX + 10];

                sbp = script_cmd;
                rbp = rbuf;

                while  (*sbp)  {
                        if  (*sbp == '%')  {
                                switch  (*++sbp)  {
                                case  '\0':
                                        goto  dun;
                                default:
                                        *rbp++ = '%';
                                        break;
                                case  'd':
                                        if  (rbp + ld > &rbuf[PATH_MAX])
                                                goto  dun;
                                        strcpy(rbp, work_directory);
                                        rbp += ld;
                                        sbp++;
                                        continue;
                                case  'f':
                                        if  (rbp + fd > &rbuf[PATH_MAX])
                                                goto  dun;
                                        strcpy(rbp, rp->name);
                                        rbp += fd;
                                        sbp++;
                                        continue;
                                }
                        }
                        *rbp++ = *sbp++;
                        if  (rbp > &rbuf[PATH_MAX])
                                break;
                }
        dun:
                *rbp = '\0';
                argv[0] = DEF_CI_NAME;
                argv[1] = "-c";
                argv[2] = rbuf;
                argv[3] = (char *) 0;
                execv(DEF_CI_PATH, argv);
        }
        exit(255);
}

/* Return 1 if the file interests us, otherwise 0 */

int  interested(struct recfile *rp)
{
        if  (rp->notfile)
                return  0;

        switch  (wotfile)  {
        default:
        case  WFL_ANY_FILE:
                return  1;

        case  WFL_SPEC_FILE:
                return  strcmp(rp->name, file_patt) == 0;

        case  WFL_PATT_FILE:
                return  qmatch(file_patt, rp->name);
        }
}

struct recfile *lookuphash(const char *name, unsigned *hvp)
{
        unsigned  hashval = 0;
        const   char    *np = name;
        struct  recfile  *rp;

        while  (*np)
                hashval = ((hashval << 1) | (hashval >> 31)) ^ *np++;

        hashval %= SCAN_HASHMOD;
        *hvp = hashval;

        for  (rp = hashtab[hashval];  rp;  rp = rp->next)
                if  (strcmp(name, rp->name) == 0)
                        return  rp;

        return  (struct recfile *) 0;
}

struct recfile *alloc_new(struct dirent *dp, struct stat *sbp, const unsigned hashval)
{
        struct  recfile  *rp = (struct recfile *) malloc((unsigned) (sizeof(struct recfile) + strlen(dp->d_name)));

        if  (!rp)
                ABORT_NOMEM;

        strcpy(rp->name, dp->d_name);
        rp->size = sbp->st_size;
        rp->atime = sbp->st_atime;
        rp->mtime = sbp->st_mtime;
        rp->ctime = sbp->st_ctime;
        rp->notfile = (sbp->st_mode & S_IFMT) != S_IFREG? 1: 0;
        rp->seq = scan_seq;
        file_num++;
        rp->prev = (struct recfile *) 0;
        if  ((rp->next = hashtab[hashval]))
                rp->next->prev = rp;
        hashtab[hashval] = rp;
        return  rp;
}

void  follow_subdir(char *dirname)
{
#ifdef  HAVE_SIGACTION
        sigset_t        sset;
#endif
        char    *newdir;
        struct  recfile **rpp;

        /* Allocate new process.
           If we can't rely on SIGCLD ignoring, then make it a grandchild
           process.  */

        if  (fork() != 0)
                return;
#ifdef  BUGGY_SIGCLD
        if  (fork() != 0)
                _exit(0);
#endif

        /*  Allocate the new working directory */

        if  (!(newdir = malloc((unsigned) (strlen(work_directory) + strlen(dirname) + 2))))
                ABORT_NOMEM;
        sprintf(newdir, "%s/%s", work_directory, dirname);
        free(work_directory);
        work_directory = newdir;
        closedir(Dir_p);        /* Did that last in case we lost dirname */

        /* Now clear hash table and start again */

        file_num = active_num = 0;
        scan_seq = 0;
        last_mtime = 0;

        for  (rpp = &hashtab[0];  rpp < &hashtab[SCAN_HASHMOD];  rpp++)  {
                struct  recfile *rp, *np = (struct recfile *) 0;
                for  (rp = *rpp;  rp;  rp = np)  {
                        np = rp->next;
                        free((char *) rp);
                }
                *rpp = (struct recfile *) 0;
        }

        /* Release blocked alarm signal if applicable */

#ifdef  HAVE_SIGACTION
        sigfillset(&sset);
        sigprocmask(SIG_UNBLOCK, &sset, (sigset_t *) 0);
#elif   defined(STRUCT_SIG)
        sigsetmask(0);
#elif   defined(HAVE_SIGSET)
        sigrelse(SIGALRM);
#else
        signal(SIGALRM, process_alarm);
#endif
        longjmp(subd_jmp, 1);
}

void  process_dir()
{
        struct  dirent  *dp;
        time_t  now = time((time_t *) 0);
        unsigned  known_files = 0;
        struct  stat    sbuf;

        scan_seq++;
        if  (wotform == WF_APPEARS  ||  wotform == WF_REMOVED)  {
#ifdef  HAVE_DIRFD
                fstat(dirfd(Dir_p), &sbuf);
#else
#ifdef  HAVE_DD_FD
                fstat(Dir_p->dd_fd, &sbuf);
#else
                stat(".", &sbuf);
#endif
#endif
                if  (sbuf.st_mtime == last_mtime)
                        return;
                last_mtime = sbuf.st_mtime;
        }

        rewinddir(Dir_p);

        while  ((dp = readdir(Dir_p)))  {

                unsigned        hashval;
                struct  recfile *rp;

                /* Ignore '.' and '..' */

                if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' ||  (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
                        continue;

                rp = lookuphash(dp->d_name, &hashval);

                if  (rp)  {     /* Known file */

                        rp->seq = scan_seq;
                        known_files++;

                        switch  (rp->type)  {
                        default:
                                continue;

                        case  FT_EXIST_MATCH:

                                if  (stat(dp->d_name, &sbuf) < 0)
                                        continue; /* huh?? */

                                switch  (wotform)  {
                                default:
                                        continue; /* Hub?? */

                                case  WF_STOPSGROW:
                                        if  (sbuf.st_size == rp->size)
                                                continue;
                                        /* If file has actually shrunk, reset size record */
                                        if  (sbuf.st_size < rp->size)  {
                                                rp->size = sbuf.st_size;
                                                continue;
                                        }
                                        break;

                                case  WF_STOPSWRITE:
                                        if  (sbuf.st_mtime == rp->mtime)
                                                continue;
                                        break;

                                case  WF_STOPSCHANGE:
                                        if  (sbuf.st_ctime == rp->ctime)
                                                continue;
                                        break;

                                case  WF_STOPSUSE:
                                        if  (sbuf.st_atime == rp->atime)
                                                continue;
                                        break;
                                }

                                /* Existing file which we were watching has changed, so
                                   we convert it to a "new" file */

                                rp->size = sbuf.st_size;
                                rp->atime = sbuf.st_atime;
                                rp->mtime = sbuf.st_mtime;
                                rp->ctime = sbuf.st_ctime;
                                rp->lastlook = now;
                                rp->type = FT_NEW;
                                active_num++;
                                continue;

                        case  FT_NEW:

                                if  (stat(dp->d_name, &sbuf) < 0)
                                        continue; /* huh?? */

                                switch  (wotform)  {
                                default:
                                        rp->lastlook = now;
                                        continue; /* hub??? */

                                case  WF_STOPSGROW:
                                        if  (rp->size != sbuf.st_size)  {
                                                rp->lastlook = now;
                                                goto  notyet;
                                        }
                                        if  (rp->lastlook + grow_time <= now)
                                                break;
                                        goto  notyet;

                                case  WF_STOPSWRITE:
                                        if  (rp->mtime != sbuf.st_mtime)  {
                                                rp->lastlook = now;
                                                goto  notyet;
                                        }
                                        if  (rp->mtime + grow_time <= now)
                                                break;
                                        goto  notyet;

                                case  WF_STOPSUSE:
                                        if  (rp->atime != sbuf.st_atime)  {
                                                rp->lastlook = now;
                                                goto  notyet;
                                        }
                                        if  (rp->atime + grow_time <= now)
                                                break;
                                        goto  notyet;

                                case  WF_STOPSCHANGE:
                                        if  (rp->ctime != sbuf.st_ctime)  {
                                                rp->lastlook = now;
                                                goto  notyet;
                                        }
                                        if  (rp->ctime + grow_time <= now)
                                                break;
                                notyet:
                                        rp->size = sbuf.st_size;
                                        rp->atime = sbuf.st_atime;
                                        rp->mtime = sbuf.st_mtime;
                                        rp->ctime = sbuf.st_ctime;
                                        continue;
                                }

                                /* Ready to roll.... */

                                rp->size = sbuf.st_size;
                                rp->atime = sbuf.st_atime;
                                rp->mtime = sbuf.st_mtime;
                                rp->ctime = sbuf.st_ctime;
                                rp->lastlook = now;
                                do_bizniz(rp);                  /* Doesn't return if we're not continuing */
                                rp->type = wotexist == IE_IGNORE_EXIST? FT_EXIST_NOMATCH: FT_EXIST_MATCH;
                                active_num--;
                                continue;
                        }
                }

                /* We have a gleaming new file..... */

#ifdef  HAVE_LSTAT
                if  (!followlinks  &&  lstat(dp->d_name, &sbuf) < 0)
                        continue;
                else
#endif
                if  (stat(dp->d_name, &sbuf) < 0)
                        continue; /* huh? */

                if  (recursive  &&  (sbuf.st_mode & S_IFMT) == S_IFDIR)
                        follow_subdir(dp->d_name);
                rp = alloc_new(dp, &sbuf, hashval);
                rp->lastlook = now;

                if  (interested(rp))  {

                        rp->type = FT_NEW;

                        /* If we just want to know when it appears, do the business now, and
                           then pretend it was an old file */

                        if  (wotform == WF_APPEARS)  {
                                do_bizniz(rp);
                                rp->type = wotexist == IE_IGNORE_EXIST? FT_EXIST_NOMATCH: FT_EXIST_MATCH;
                        }
                }
                else
                        rp->type = FT_EXIST_NOMATCH;

                known_files++;          /* Cous we've incremented file_num */
        }

        /* Delete records of files which have disappeared.  */

        if  (known_files < file_num)  {
                unsigned  hashval;
                for  (hashval = 0;  hashval < SCAN_HASHMOD;  hashval++)  {
                        struct  recfile *rp, **rpp;

                        for  (rpp = &hashtab[hashval];  (rp = *rpp);  )  {

                                if  (rp->seq < scan_seq)  {             /* Didn't hit it */
                                        if  (wotform == WF_REMOVED  &&  rp->type != FT_EXIST_NOMATCH)
                                                do_bizniz(rp);
                                        if  ((*rpp = rp->next))         /* Not at end of chain */
                                                rp->next->prev = rp->prev;
                                        file_num--;
                                        if  (rp->type == FT_NEW)
                                                active_num--;
                                        free((char *) rp);
                                }
                                else
                                        rpp = &rp->next;
                        }
                }
        }
}

void  init_dir()
{
        struct  dirent  *dp;
        time_t  now = time((time_t *) 0);

        if  (chdir(work_directory) < 0)  {
                disp_str = work_directory;
                print_error($E{filemon cannot select directory});
                exit(E_NOCHDIR);
        }

        if  (!(Dir_p = opendir(".")))  {
                disp_str = work_directory;
                print_error($E{filemon cannot read directory});
                exit(E_NOCHDIR);
        }

        if  (wotform == WF_APPEARS  ||  wotform == WF_REMOVED)  {
                struct  stat    sbuf;
#ifdef  HAVE_DIRFD
                fstat(dirfd(Dir_p), &sbuf);
#else
#ifdef  HAVE_DD_FD
                fstat(Dir_p->dd_fd, &sbuf);
#else
                stat(".", &sbuf);
#endif
#endif
                last_mtime = sbuf.st_mtime;
        }

        while  ((dp = readdir(Dir_p)))  {

                unsigned        hashval;
                struct  recfile *rp;
                struct  stat    sbuf;

                /* Ignore '.' and '..' */

                if  (dp->d_name[0] == '.'  &&  (dp->d_name[1] == '\0' ||  (dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
                        continue;

                if  (lookuphash(dp->d_name, &hashval))  {
                        disp_str = dp->d_name;
                        print_error($E{filemon duplicated name});
                        exit(E_SETUP);
                }

#ifdef  HAVE_LSTAT
                if  (!followlinks  &&  lstat(dp->d_name, &sbuf) < 0)
                        continue; /* huh? */
                else
#endif
                if  (stat(dp->d_name, &sbuf) < 0)
                        continue; /* huh? */

                if  (recursive  &&  (sbuf.st_mode & S_IFMT) == S_IFDIR)
                        follow_subdir(dp->d_name);
                rp = alloc_new(dp, &sbuf, hashval);
                rp->lastlook = now;
                rp->type = (wotexist == IE_INCL_EXIST  &&  interested(rp))? FT_EXIST_MATCH: FT_EXIST_NOMATCH;
        }
}

void  exit_cleanup()
{
        if  (my_shm_slot)
                my_shm_slot->fm_pid = 0;
}

RETSIGTYPE  process_quit(int signum)
{
#ifndef HAVE_ATEXIT
        exit_cleanup();
#endif
        exit(E_SIGNAL);
}

RETSIGTYPE  process_alarm(int signum)
{
#ifdef  UNSAFE_SIGNALS
        signal(signum, SIG_IGN);
#endif
        process_dir();
#ifdef  UNSAFE_SIGNALS
        signal(signum, process_alarm);
#endif
        alarm(active_num > 0  &&  grow_time < poll_time? grow_time: poll_time);
}

OPTION(o_explain)
{
        print_error($E{btfilemon explain});
        exit(0);
        return  0;              /* Silence compilers */
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

OPTION(o_daemon)
{
        isdaemon++;
        return  OPTRESULT_OK;
}

OPTION(o_nodaemon)
{
        isdaemon = 0;
        return  OPTRESULT_OK;
}

OPTION(o_directory)
{
        struct  stat    sbuf;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        /* We haven't changed directory yet */

        if  (stat(arg, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                arg_errnum = $E{Invalid monitor directory};
                return  OPTRESULT_ERROR;
        }

        if  (work_directory)
                free(work_directory);

        work_directory = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_anyfile)
{
        wotfile = WFL_ANY_FILE;
        return  OPTRESULT_OK;
}

OPTION(o_patternfile)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        if  (file_patt)
                free(file_patt);
        file_patt = stracpy(arg);

        wotfile = WFL_PATT_FILE;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_givenfile)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        if  (file_patt)
                free(file_patt);
        file_patt = stracpy(arg);

        wotfile = WFL_SPEC_FILE;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_arrival)
{
        wotform = WF_APPEARS;
        return  OPTRESULT_OK;
}

OPTION(o_removed)
{
        wotform = WF_REMOVED;
        return  OPTRESULT_OK;
}

OPTION(o_nogrow)
{
        int     n;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        n = atoi(arg);
        if  (n <= 0  ||  n > MAX_POLL_TIME)  {
                arg_errnum = $E{btfilemon invalid time field};
                return  OPTRESULT_ERROR;
        }
        grow_time = n;

        wotform = WF_STOPSGROW;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_nomod)
{
        int     n;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        n = atoi(arg);
        if  (n <= 0  ||  n > MAX_POLL_TIME)  {
                arg_errnum = $E{btfilemon invalid time field};
                return  OPTRESULT_ERROR;
        }
        grow_time = n;

        wotform = WF_STOPSWRITE;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_nochange)
{
        int     n;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        n = atoi(arg);
        if  (n <= 0  ||  n > MAX_POLL_TIME)  {
                arg_errnum = $E{btfilemon invalid time field};
                return  OPTRESULT_ERROR;
        }
        grow_time = n;

        wotform = WF_STOPSCHANGE;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_noaccess)
{
        int     n;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        n = atoi(arg);
        if  (n <= 0  ||  n > MAX_POLL_TIME)  {
                arg_errnum = $E{btfilemon invalid time field};
                return  OPTRESULT_ERROR;
        }
        grow_time = n;

        wotform = WF_STOPSUSE;
        return  OPTRESULT_ARG_OK;
}

OPTION(o_haltfound)
{
        wotmode = WM_STOP_FOUND;
        return  OPTRESULT_OK;
}

OPTION(o_contfound)
{
        wotmode = WM_CONT_FOUND;
        return  OPTRESULT_OK;
}

OPTION(o_ignoreexist)
{
        wotexist = IE_IGNORE_EXIST;
        return  OPTRESULT_OK;
}

OPTION(o_includeexist)
{
        wotexist = IE_INCL_EXIST;
        return  OPTRESULT_OK;
}

OPTION(o_polltime)
{
        int     n;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        n = atoi(arg);
        if  (n <= 0  ||  n > MAX_POLL_TIME)  {
                arg_errnum = $E{btfilemon invalid time field};
                return  OPTRESULT_ERROR;
        }
        poll_time = n;

        return  OPTRESULT_ARG_OK;
}

OPTION(o_scriptfile)
{
        struct  stat    sbuf;

        if  (!arg)
                return  OPTRESULT_MISSARG;

        /* We haven't changed directory yet */

        if  (stat(arg, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFREG)  {
                arg_errnum = $E{Invalid script file};
                return  OPTRESULT_ERROR;
        }

        if  (script_file)  {
                free(script_file);
                script_file = (char *) 0;
        }
        if  (script_cmd)  {
                free(script_cmd);
                script_cmd = (char *) 0;
        }

        script_file = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_scriptcmd)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        if  (script_file)  {
                free(script_file);
                script_file = (char *) 0;
        }
        if  (script_cmd)  {
                free(script_cmd);
                script_cmd = (char *) 0;
        }

        script_cmd = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_nonrecursive)
{
        recursive = 0;
        return  OPTRESULT_OK;
}

OPTION(o_recursive)
{
        recursive = 1;
        return  OPTRESULT_OK;
}

OPTION(o_nolinks)
{
        followlinks = 0;
        return  OPTRESULT_OK;
}

OPTION(o_links)
{
        followlinks = 1;
        return  OPTRESULT_OK;
}

OPTION(o_setmonitor)
{
        wotprog = WP_SETMONITOR;
        return  OPTRESULT_OK;
}

OPTION(o_listmons)
{
        wotprog = WP_LISTMONS;
        return  OPTRESULT_OK;
}

OPTION(o_killmon)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        if  (wotprog != WP_KILLMON)  {
                wotprog = WP_KILLMON;
                kmcount = 0;
                if  (!kmlist  &&  !(kmlist = (char **) malloc(MAX_FM_PROCS * sizeof(char *))))
                        ABORT_NOMEM;
        }
        if  (kmcount >= MAX_FM_PROCS)  {
                print_error($E{Too many fm kill args});
                exit(E_USAGE);
        }
        kmlist[kmcount++] = stracpy(arg);
        return  OPTRESULT_ARG_OK;
}

OPTION(o_killall)
{
        wotprog = WP_KILLALL;
        return  OPTRESULT_OK;
}

#include "inline/btfmadefs.c"

optparam        optprocs[] = {
o_explain,
o_daemon,       o_nodaemon,     o_directory,    o_scriptfile,
o_scriptcmd,    o_anyfile,      o_patternfile,  o_givenfile,
o_arrival,      o_nogrow,       o_nomod,        o_nochange,
o_noaccess,     o_removed,      o_haltfound,    o_contfound,
o_ignoreexist,  o_includeexist, o_polltime,     o_nonrecursive,
o_recursive,    o_nolinks,      o_links,        o_setmonitor,
o_listmons,     o_killmon,      o_killall,
o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        int     cancont = 0;

        fprintf(dest, "%s", name);

        cancont = spitoption(isdaemon? $A{btfilemon arg daemon}: $A{btfilemon arg nodaemon}, $A{btfilemon arg explain}, dest, '=', cancont);

        if  (wotprog != WP_KILLMON)
                cancont = spitoption(wotprog == WP_LISTMONS? $A{btfilemon arg list monitor}:
                                     wotprog == WP_KILLALL? $A{btfilemon arg killall} :
                                     $A{btfilemon arg run monitor},
                                     $A{btfilemon arg explain}, dest, ' ', cancont);

        cancont = spitoption(recursive?
                             $A{btfilemon arg recursive}: $A{btfilemon arg nonrecursive},
                             $A{btfilemon arg explain}, dest, ' ', cancont);
        cancont = spitoption(followlinks?
                             $A{btfilemon arg follow links}: $A{btfilemon arg no links},
                             $A{btfilemon arg explain}, dest, ' ', cancont);

        if  (wotfile == WFL_ANY_FILE)
                cancont = spitoption($A{btfilemon arg anyfile}, $A{btfilemon arg explain}, dest, ' ', cancont);

        cancont = spitoption(wotmode == WM_STOP_FOUND? $A{btfilemon arg halt found}: $A{btfilemon arg cont found}, $A{btfilemon arg explain}, dest, ' ', cancont);
        cancont = spitoption(wotexist == IE_INCL_EXIST? $A{btfilemon arg include existing}: $A{btfilemon arg ignore existing}, $A{btfilemon arg explain}, dest, ' ', cancont);

        switch  (wotform)  {
        default:
        case  WF_APPEARS:
                spitoption($A{btfilemon arg arrival}, $A{btfilemon arg explain}, dest, ' ', cancont);
                break;
        case  WF_REMOVED:
                spitoption($A{btfilemon arg file removed}, $A{btfilemon arg explain}, dest, ' ', cancont);
                break;

        case  WF_STOPSGROW:
                spitoption($A{btfilemon arg grow time}, $A{btfilemon arg explain}, dest, ' ', 0);
        grest:
                fprintf(dest, " %u", grow_time);
                break;

        case  WF_STOPSWRITE:
                spitoption($A{btfilemon arg mod time}, $A{btfilemon arg explain}, dest, ' ', 0);
                goto  grest;

        case  WF_STOPSCHANGE:
                spitoption($A{btfilemon arg change time}, $A{btfilemon arg explain}, dest, ' ', 0);
                goto  grest;

        case  WF_STOPSUSE:
                spitoption($A{btfilemon arg access time}, $A{btfilemon arg explain}, dest, ' ', 0);
                goto  grest;
        }

        if  (wotprog == WP_KILLMON)  {
                int     cnt;
                for  (cnt = 0;  cnt < kmcount;  cnt++)  {
                        spitoption($A{btfilemon arg kill proc}, $A{btfilemon arg explain}, dest, ' ', 0);
                        fprintf(dest, " \'%s\'", kmlist[cnt]);
                }
        }

        if  (wotfile != WFL_ANY_FILE)  {
                spitoption(wotfile == WFL_SPEC_FILE? $A{btfilemon arg given file}: $A{btfilemon arg pattern file}, $A{btfilemon arg explain}, dest, ' ', 0);
                fprintf(dest, " \'%s\'", file_patt);
        }

        spitoption($A{btfilemon arg poll time}, $A{btfilemon arg explain}, dest, ' ', 0);
        fprintf(dest, " %u", poll_time);

        if  (work_directory)  {
                spitoption($A{btfilemon arg directory}, $A{btfilemon arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", work_directory);
        }

        if  (script_file)  {
                spitoption($A{btfilemon arg script file}, $A{btfilemon arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", script_file);
        }
        else  if  (script_cmd)  {
                spitoption($A{btfilemon arg command}, $A{btfilemon arg explain}, dest, ' ', 0);
                fprintf(dest, " %s", script_cmd);
        }

        putc('\n', dest);
}

MAINFN_TYPE  main(int argc, char **argv)
{
        int     ret;
#ifdef  STRUCT_SIG
        struct  sigstruct_name  z;
#endif

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

        if  (!(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();
        Cfile = open_cfile(Confvarname, "filemon.help");
        argv = optprocess(argv, Adefs, optprocs, $A{btfilemon arg explain}, $A{btfilemon arg freeze home}, 0);

#define FREEZE_EXIT
#include "inline/freezecode.c"

        if  (argv[0])  {
                print_error($E{Extra arguments not expected});
                exit(E_USAGE);
        }
        if  (wotprog == WP_SETMONITOR  &&  !script_file  &&  !script_cmd)  {
                print_error($E{filemon no script file given});
                exit(E_USAGE);
        }

        if  (!(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        if  (work_directory)  {
                /*      Keep name the same if we're not changing directory */
                if  (script_file  &&  script_file[0] != '/'  &&  strcmp(work_directory, Curr_pwd) != 0)  {
                        char    *m = malloc((unsigned) (strlen(Curr_pwd) + strlen(script_file) + 2));
                        if  (!m)
                                ABORT_NOMEM;
                        sprintf(m, "%s/%s", Curr_pwd, script_file);
                        free(script_file);
                        script_file = m;
                }
                if  (work_directory[0] != '/')  {
                        char    *m = malloc((unsigned) (strlen(Curr_pwd) + strlen(work_directory) + 2));
                        if  (!m)
                                ABORT_NOMEM;
                        sprintf(m, "%s/%s", Curr_pwd, work_directory);
                        free(work_directory);
                        work_directory = m;
                }
        }
        else
                work_directory = stracpy(Curr_pwd); /* Copy in case deallocated later */

        switch  (wotprog)  {
        case  WP_LISTMONS:
                do_listmons();
                return  0;
        case  WP_KILLMON:
                return  do_killmons();
        case  WP_KILLALL:
                return  do_killall();
        default:
                break;
        }

        if  (isdaemon)  {
                if  (fork() != 0)
                        exit(0);
#ifdef  SETPGRP_VOID
                setpgrp();
#else
                setpgrp(0, getpid());
#endif
        }

#ifdef  STRUCT_SIG
        z.sighandler_el = process_alarm;
        sigmask_clear(z);
        z.sigflags_el = SIGVEC_INTFLAG;
        sigact_routine(SIGALRM, &z, (struct sigstruct_name *) 0);
        z.sighandler_el = process_quit;
        sigact_routine(SIGHUP, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGINT, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGQUIT, &z, (struct sigstruct_name *) 0);
        sigact_routine(SIGTERM, &z, (struct sigstruct_name *) 0);
#else
        signal(SIGALRM, process_alarm);
        signal(SIGHUP, process_quit);
        signal(SIGINT, process_quit);
        signal(SIGQUIT, process_quit);
        signal(SIGTERM, process_quit);
#endif
#ifndef BUGGY_SIGCLD
#ifdef  STRUCT_SIG
        z.sighandler_el = SIG_IGN;
#ifdef  SA_NOCLDWAIT
        z.sigflags_el |= SA_NOCLDWAIT;
#endif
        sigact_routine(SIGCLD, &z, (struct sigstruct_name *) 0);
#else
        signal(SIGCLD, SIG_IGN);
#endif
#endif /* Ok SIGCLD */

#ifdef  HAVE_ATEXIT
        atexit(exit_cleanup);
#endif
        init_shm(0);

        /* This is where recursively-invoked subprocesses jump to */
        setjmp(subd_jmp);

        if  ((ret = make_shmslot(work_directory)) != 0)  {
                print_error(ret);
                return  E_FM_TOOMANY;
        }

        init_dir();
        alarm(poll_time);
        for  (;;)
                pause();
}
