/* btuconv.c -- dump out user permissions file as shell script

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include "defaults.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "files.h"
#include "ecodes.h"
#include "btmode.h"
#include "btuser.h"

#define ALLPRIVS_R4     0x003F                  /* Privs */

typedef struct  {
        time_t          btd_lastp;      /* Last read password file */
        unsigned  char  btd_minp,       /* Minimum priority  */
                        btd_maxp,       /* Maximum priority  */
                        btd_defp,       /* Default priority  */
                        btd_resvd1;     /* reserved */
        unsigned  short btd_maxll;      /* Max load level */
        unsigned  short btd_totll;      /* Max total load level */
        unsigned  short btd_spec_ll;    /* Non-std jobs load level */
        unsigned  short btd_priv;       /* Privileges */
        unsigned  short btd_jflags[3];  /* Flags for jobs */
        unsigned  short btd_vflags[3];  /* Flags for variables */
}  Btdef_r4;

typedef struct  {
        unsigned  char  btu_isvalid,    /* Valid user id */
                        btu_minp,       /* Minimum priority  */
                        btu_maxp,       /* Maximum priority  */
                        btu_defp;       /* Default priority  */
        int_ugid_t      btu_user;       /* User id */
        unsigned  short btu_maxll;      /* Max load level */
        unsigned  short btu_totll;      /* Max total load level */
        unsigned  short btu_spec_ll;    /* Non-std jobs load level */
        unsigned  short btu_priv;       /* Privileges */
        unsigned  short btu_jflags[3];  /* Flags for jobs */
        unsigned  short btu_vflags[3];  /* Flags for variables */
        double          btu_charge;     /* User's bill */
}  Btuser_r4;

extern  FILE    *Cfile;

struct  {
        char    *srcdir;        /* Directory we read from if not pwd */
        char    *outfile;       /* Output file */
        long    errtol;         /* Number of errors we'll take */
        long    errors;         /* Number we've had */
        short   ignsize;        /* Ignore file size */
        short   ignfmt;         /* Ignore file format errors */
}  popts;

extern char *expand_srcdir(char *);
extern char *make_absolute(char *);

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "btuconv:Mem alloc fault: %s line %d\n", fl, ln);
        exit(E_NOMEM);
}

static int  btu4fldsok(Btuser_r4 *od)
{
        return  !(od->btu_minp == 0  ||
                  od->btu_defp == 0  ||
                  od->btu_maxp == 0  ||
                  od->btu_maxll == 0  ||
                  od->btu_totll == 0  ||
                  od->btu_spec_ll == 0  ||
                  (od->btu_priv & ~ALLPRIVS_R4) != 0);
}

static int  isit_r4(const int fd, const struct stat *sb)
{
        int             uidn = 0;
        Btdef_r4        oh;
        Btuser_r4       od;

        if  ((sb->st_size - sizeof(Btdef_r4)) % sizeof(Btuser_r4) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(fd, 0L, 0);

        if  (read(fd, (char *) &oh, sizeof(oh)) != sizeof(oh))
                return  0;

        /* We'll have to give up if the header is knackered */

        if  (oh.btd_minp == 0  ||
             oh.btd_defp == 0  ||
             oh.btd_maxp == 0  ||
             oh.btd_maxll == 0  ||
             oh.btd_totll == 0  ||
             oh.btd_spec_ll == 0  ||
             (oh.btd_priv & ~ALLPRIVS_R4) != 0)
                return  0;

        while  (read(fd, (char *) &od, sizeof(od)) == sizeof(od))  {
                if  (!od.btu_isvalid)
                        continue;
                if  (btu4fldsok(&od))
                        uidn++;
                else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
                        return  0;
        }
        return  uidn > 0;
}

void  conv_r4(const int ifd)
{
        int     uidn;
        Btdef_r4        oh;
        Btuser_r4       od;
        unsigned  long          flags;

        printf("#! /bin/sh\n# Conversion from release 4\n");
        lseek(ifd, 0L, 0);

        if  (read(ifd, (char *) &oh, sizeof(oh)) != sizeof(oh))  {
                fprintf(stderr, "Bad header old file\n");
                exit(3);
        }

        flags = oh.btd_priv;
        if  (flags & BTM_WADMIN)
                flags |= BTM_ORP_UG | BTM_ORP_UO | BTM_ORP_GO;

        printf("# Converted from vn 4\n\ngbch-uchange -DA -l %d -d %d -m %d -M %u -T %u -S %u -p 0x%lx -J 0x%x,%x,%x -V 0x%x,%x,%x\n",
                      oh.btd_minp, oh.btd_defp, oh.btd_maxp,
                      oh.btd_maxll, oh.btd_totll, oh.btd_spec_ll,
                      flags,
                      oh.btd_jflags[0], oh.btd_jflags[1], oh.btd_jflags[2],
                      oh.btd_vflags[0], oh.btd_vflags[1], oh.btd_vflags[2]);

        uidn = 0;

        while  (read(ifd, (char *) &od, sizeof(od)) == sizeof(od))  {
                if  (!od.btu_isvalid  ||  !btu4fldsok(&od))
                        continue;
                if  (od.btu_minp == oh.btd_minp  &&  od.btu_defp == oh.btd_defp  &&
                     od.btu_maxp == oh.btd_maxp  &&  od.btu_totll == oh.btd_totll  &&
                     od.btu_maxll == oh.btd_maxll  &&  od.btu_spec_ll == oh.btd_spec_ll  &&
                     od.btu_priv == oh.btd_priv  &&
                     od.btu_jflags[0] == oh.btd_jflags[0]  &&
                     od.btu_jflags[1] == oh.btd_jflags[1]  &&
                     od.btu_jflags[2] == oh.btd_jflags[2]  &&
                     od.btu_vflags[0] == oh.btd_vflags[0]  &&
                     od.btu_vflags[1] == oh.btd_vflags[1]  &&
                     od.btu_vflags[2] == oh.btd_vflags[2])
                        continue;
                flags = od.btu_priv;
                if  (flags & BTM_WADMIN)
                        flags |= BTM_ORP_UG | BTM_ORP_UO | BTM_ORP_GO;
                printf("gbch-uchange -l %d -d %d -m %d -M %u -T %u -S %u -p 0x%lx -J 0x%x,%x,%x -V 0x%x,%x,%x %s\n",
                      od.btu_minp, od.btu_defp, od.btu_maxp,
                      od.btu_maxll, od.btu_totll, od.btu_spec_ll,
                      flags,
                      od.btu_jflags[0], od.btu_jflags[1], od.btu_jflags[2],
                      od.btu_vflags[0], od.btu_vflags[1], od.btu_vflags[2],
                      prin_uname((uid_t) od.btu_user));
                uidn++;
        }
}

static int  btu5fldsok(Btuser *od)
{
        return  !(od->btu_minp == 0  ||
                  od->btu_defp == 0  ||
                  od->btu_maxp == 0  ||
                  od->btu_maxll == 0  ||
                  od->btu_totll == 0  ||
                  od->btu_spec_ll == 0  ||
                  (od->btu_priv & ~ALLPRIVS) != 0);
}

static int  isit_r5(const int fd, const struct stat *sb)
{
        int     uidn = 0;
        Btdef   oh;
        Btuser  od;

        if  ((sb->st_size - sizeof(Btdef)) % sizeof(Btuser) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(fd, 0L, 0);

        if  (read(fd, (char *) &oh, sizeof(oh)) != sizeof(oh))
                return  0;

        /* We'll have to give up if the header is knackered */

        if  (oh.btd_minp == 0  ||
             oh.btd_defp == 0  ||
             oh.btd_maxp == 0  ||
             oh.btd_maxll == 0  ||
             oh.btd_totll == 0  ||
             oh.btd_spec_ll == 0  ||
             (oh.btd_priv & ~ALLPRIVS) != 0)
                return  0;

        while  (read(fd, (char *) &od, sizeof(od)) == sizeof(od))  {
                if  (!od.btu_isvalid)
                        continue;
                if  (btu5fldsok(&od))
                        uidn++;
                else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
                        return  0;
        }
        return  uidn > 0;
}

void  conv_r5(const int ifd)
{
        int     uidn = 0;
        Btdef   oh;
        Btuser  od;

        printf("#! /bin/sh\n# Conversion from release 5 up\n");
        lseek(ifd, 0L, 0);

        if  (read(ifd, (char *) &oh, sizeof(oh)) != sizeof(oh))  {
                fprintf(stderr, "Bad header old file\n");
                exit(3);
        }

        printf("# Converted from vn %d\n\ngbch-uchange -DA -l %d -d %d -m %d -M %u -T %u -S %u -p 0x%lx -J 0x%x,%x,%x -V 0x%x,%x,%x\n",
                      oh.btd_version,
                      oh.btd_minp, oh.btd_defp, oh.btd_maxp,
                      oh.btd_maxll, oh.btd_totll, oh.btd_spec_ll,
                      (unsigned long) oh.btd_priv,
                      oh.btd_jflags[0], oh.btd_jflags[1], oh.btd_jflags[2],
                      oh.btd_vflags[0], oh.btd_vflags[1], oh.btd_vflags[2]);

        while  (read(ifd, (char *) &od, sizeof(od)) == sizeof(od))  {
                if  (!od.btu_isvalid)
                        continue;
                if  (od.btu_minp == oh.btd_minp  &&  od.btu_defp == oh.btd_defp  &&
                     od.btu_maxp == oh.btd_maxp  &&  od.btu_totll == oh.btd_totll  &&
                     od.btu_maxll == oh.btd_maxll  &&  od.btu_spec_ll == oh.btd_spec_ll  &&
                     od.btu_priv == oh.btd_priv  &&
                     od.btu_jflags[0] == oh.btd_jflags[0]  &&
                     od.btu_jflags[1] == oh.btd_jflags[1]  &&
                     od.btu_jflags[2] == oh.btd_jflags[2]  &&
                     od.btu_vflags[0] == oh.btd_vflags[0]  &&
                     od.btu_vflags[1] == oh.btd_vflags[1]  &&
                     od.btu_vflags[2] == oh.btd_vflags[2])
                        continue;
                printf("gbch-uchange -l %d -d %d -m %d -M %u -T %u -S %u -p 0x%lx -J 0x%x,%x,%x -V 0x%x,%x,%x %s\n",
                      od.btu_minp, od.btu_defp, od.btu_maxp,
                      od.btu_maxll, od.btu_totll, od.btu_spec_ll,
                      (unsigned long) od.btu_priv,
                      od.btu_jflags[0], od.btu_jflags[1], od.btu_jflags[2],
                      od.btu_vflags[0], od.btu_vflags[1], od.btu_vflags[2],
                      prin_uname((uid_t) od.btu_user));
                uidn++;
        }
}

struct  flst  {
        int     (*isitfn)(const int, const struct stat *);
        void    (*convfn)(const int);
}  funclist[] =  { { isit_r4, conv_r4 },  { isit_r5, conv_r5 } };

#define N_CONVS ((sizeof(funclist)) / sizeof(struct flst))

/* Versions relate to the version of Xi-Batch on which this is based */

MAINFN_TYPE  main(int argc, char **argv)
{
        int             ifd, ch, forcevn = 0;
        struct  stat    sbuf;
        extern  int     optind;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);

        while  ((ch = getopt(argc, argv, "sfe:v:D:")) != EOF)
                switch  (ch)  {
                default:
                        goto  usage;
                case  'D':
                        popts.srcdir = optarg;
                        continue;
                case  's':
                        popts.ignsize++;
                        continue;
                case  'f':
                        popts.ignfmt++;
                        continue;
                case  'e':
                        popts.errtol = atol(optarg);
                        continue;
                case  'v':
                        forcevn = atoi(optarg);
                        if  (forcevn < 4  ||  forcevn > 6)  {
                                fprintf(stderr, "Sorry I don't know about version %d\n", forcevn);
                                return  101;
                        }
                        continue;
                }

        if  (argc - optind < 1  ||  argc - optind > 2)  {
        usage:
                fprintf(stderr, "Usage: %s [-D dir] [-s] [-f] [-e n] [-v n] vfile outfile\n", argv[0]);
                return  100;
        }

        if  (popts.srcdir)  {
                char    *newd = expand_srcdir(popts.srcdir);
                if  (!newd)  {
                        fprintf(stderr, "Invalid source directory %s\n", popts.srcdir);
                        return  10;
                }
                if  (stat(newd, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                        fprintf(stderr, "Source dir %s is not a directory\n", newd);
                        return  12;
                }
                popts.srcdir = newd;
        }

        if  (argc - optind > 1  &&  strcmp(argv[optind+1], "-") != 0)  {
                popts.outfile = argv[optind+1];
                if  (!freopen(popts.outfile, "w", stdout))  {
                        fprintf(stderr, "Sorry cannot create %s\n", popts.outfile);
                        return  3;
                }
        }

        /* Now change directory to the source directory if specified */

        if  (popts.srcdir)  {
                if  (popts.outfile)
                        popts.outfile = make_absolute(popts.outfile);
                if  (chdir(popts.srcdir) < 0)  {
                        fprintf(stderr, "Cannot open source directory %s\n", popts.srcdir);
                        if  (popts.outfile)
                                unlink(popts.outfile);
                        return  13;
                }
        }

        /* Open source user file */

        if  ((ifd = open(argv[optind], O_RDONLY)) < 0)  {
                fprintf(stderr, "Sorry cannot open %s\n", argv[optind]);
                if  (popts.outfile)
                        unlink(popts.outfile);
                return  2;
        }

        fstat(ifd, &sbuf);
        if  (forcevn != 0)  {
                forcevn -= 4;
                if  (forcevn >= N_CONVS)
                        forcevn = N_CONVS-1;
                if  ((*funclist[forcevn].isitfn)(ifd, &sbuf))  {
                        (*funclist[forcevn].convfn)(ifd);
                        goto  dun;
                }
                fprintf(stderr, "Too many errors in file - conversion failed\n");
                if  (popts.outfile)
                        unlink(popts.outfile);
                return  10;
        }
        else  {
                int     cnt;
                for  (cnt = N_CONVS-1;  cnt >= 0;  cnt--)
                        if  ((*funclist[cnt].isitfn)(ifd, &sbuf))  {
                                (*funclist[cnt].convfn)(ifd);
                                goto  dun;
                        }
                fprintf(stderr, "I am confused about the format of your user file (is it pre-r4?)\n");
                if  (popts.outfile)
                        unlink(popts.outfile);
                return  9;
        }

 dun:
        if  (popts.errors > 0)
                fprintf(stderr, "There were %ld error%s found\n", popts.errors, popts.errors > 1? "s": "");
        close(ifd);
#ifdef  HAVE_FCHMOD
        if  (popts.outfile)
                fchmod(fileno(stdout), 0755);
#else
        if  (popts.outfile)
                chmod(popts.outfile, 0755);
#endif
        fprintf(stderr, "Converted btufile ok\n");
        return  0;
}
