/* cvlist.c -- dump out variable list as shell script

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
#include <pwd.h>
#include <grp.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "btuser.h"
#include "files.h"
#include "spitrouts.h"
#include "ecodes.h"

struct  {
        char    *srcdir;        /* Directory we read from if not pwd */
        char    *outfile;       /* Output file */
        long    errtol;         /* Number of errors we'll take */
        long    errors;         /* Number we've had */
        short   ignsize;        /* Ignore file size */
        short   ignfmt;         /* Ignore file format errors */
        short   ignusers;       /* Ignore invalid users */
}  popts;

extern char *expand_srcdir(char *);
extern char *make_absolute(char *);

char    *progname;

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static int  unameok(const char *un, const int_ugid_t uid)
{
        struct  passwd  *pw;

        if  (strlen(un) > UIDSIZE)
                return  0;
        if  (!popts.ignusers)  {
                if  (!(pw = getpwnam(un)))
                        return  0;
                if  (pw->pw_uid != uid)
                        return  0;
        }
        return  1;
}

static int  gnameok(const char *gn, const int_ugid_t gid)
{
        struct  group   *gw;

        if  (strlen(gn) > UIDSIZE)
                return  0;
        if  (!popts.ignusers)  {
                if  (!(gw = getgrnam(gn)))
                        return  0;
                if  (gw->gr_gid != gid)
                        return  0;
        }
        return  1;
}

static int  nameok(BtvarRef vr)
{
        const   char    *cp = vr->var_name;

        if  (isdigit(*cp))
                return  0;
        do  {
                if  (!isalnum(*cp)  &&  *cp != '_')
                        return  0;
                cp++;
        }  while  (*cp);
        return  1;
}

static int  varfldsok(Btvar *old)
{
        if  (!nameok(old))
                return  0;
        if  (!unameok(old->var_mode.o_user, old->var_mode.o_uid))
                return  0;
        if  (!unameok(old->var_mode.c_user, old->var_mode.c_uid))
                return  0;
        if  (!gnameok(old->var_mode.o_group, old->var_mode.o_gid))
                return  0;
        if  (!gnameok(old->var_mode.c_group, old->var_mode.c_gid))
                return  0;
        if  (old->var_type > VT_STARTWAIT)
                return  0;
        if  (old->var_value.const_type <= CON_NONE || old->var_value.const_type > CON_STRING)
                return  0;
        return  1;
}

int  isit_r4(const int ifd, const struct stat *sb)
{
        int     okvars = 0;
        Btvar   old;

        if  ((sb->st_size % sizeof(Btvar)) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))  {
                if  (varfldsok(&old))
                        okvars++;
                else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
                        return  0;
        }
        return  okvars > 0;
}

void  conv_r4(const int ifd)
{
        Btvar   old;

        printf("#! /bin/sh\n# Conversion from release 4 up\n");
        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))  {
                if  (old.var_type  ||  !varfldsok(&old))
                        continue;
                printf("gbch-var -C -%c -%c -c \'%s\' -U %s -G %s -M ",
                              old.var_flags & VF_CLUSTER? 'K': 'k',
                              old.var_flags & VF_EXPORT? 'E': 'L',
                              old.var_comment,
                              old.var_mode.o_user, old.var_mode.o_group);
                dumpmode(stdout, "U", old.var_mode.u_flags);
                dumpmode(stdout, ",G", old.var_mode.g_flags);
                dumpmode(stdout, ",O", old.var_mode.o_flags);
                if  (old.var_value.const_type == CON_LONG)
                        printf(" -s %ld", (long) old.var_value.con_un.con_long);
                else  {
                        if  (isdigit(old.var_value.con_un.con_string[0]))
                                fputs(" -S", stdout);
                        printf(" -s \'%s\'", old.var_value.con_un.con_string);
                }
                printf(" %s\n", old.var_name);
        }
}

MAINFN_TYPE  main(int argc, char **argv)
{
        int             ifd, ch, forcevn = 0;
        struct  stat    sbuf;
        struct  flock   rlock;
        extern  int     optind;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);
        progname = argv[0];

        while  ((ch = getopt(argc, argv, "usfe:v:D:")) != EOF)
                switch  (ch)  {
                default:
                        goto  usage;
                case  'D':
                        popts.srcdir = optarg;
                        continue;
                case  'u':
                        popts.ignusers++;
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
                fprintf(stderr, "Usage: %s [-D dir] [-u] [-s] [-f] [-e n] [-v n] vfile outfile\n", argv[0]);
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

        /* Open source var file */

        if  ((ifd = open(argv[optind], O_RDONLY)) < 0)  {
                fprintf(stderr, "Sorry cannot open %s\n", argv[optind]);
                if  (popts.outfile)
                        unlink(popts.outfile);
                return  2;
        }

        rlock.l_type = F_RDLCK;
        rlock.l_whence = 0;
        rlock.l_start = 0L;
        rlock.l_len = 0L;
        if  (fcntl(ifd, F_SETLKW, &rlock) < 0)  {
                fprintf(stderr, "Sorry could not lock %s\n", argv[optind]);
                return  3;
        }

        fstat(ifd, &sbuf);
        if  (forcevn != 0)  {
                if  (isit_r4(ifd, &sbuf))  {
                        conv_r4(ifd);
                        goto  dun;
                }
                fprintf(stderr, "Too many errors in file - conversion failed\n");
                if  (popts.outfile)
                        unlink(popts.outfile);
                return  10;
        }
        else  if  (isit_r4(ifd, &sbuf))
                conv_r4(ifd);
        else  {
                fprintf(stderr, "I am confused about the format of your variable file (is it pre-r4?)\n");
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
        fprintf(stderr, "Finished outputting variable file\n");
        return  0;
}
