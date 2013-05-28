/* ciconv.c -- dump out command interpreter file as shell script

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
#include "files.h"
#include "ecodes.h"
#include "bjparam.h"
#include "cmdint.h"

typedef struct  {
        unsigned  short  ci_ll;
        unsigned  short  ci_nice;
        char    ci_name[CI_MAXNAME+1];
        char    ci_path[CI_MAXFPATH+1];
        char    ci_args[CI_MAXARGS+1];
}  Cmdint_r4;

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

static int  printable(const char *str)
{
        while  (*str)  {
                if  (!isprint(*str))
                        return  0;
                str++;
        }
        return  1;
}

static int  graphic(const char *str)
{
        while  (*str)  {
                if  (!isgraph(*str))
                        return  0;
                str++;
        }
        return  1;
}

static int  ci4fldsok(Cmdint_r4 *oci)
{
        if  (oci->ci_ll == 0  ||  oci->ci_nice > 39)
                return  0;
        if  (oci->ci_path[0] != '/')
                return  0;
        if  (!graphic(oci->ci_name) || !graphic(oci->ci_path) || !printable(oci->ci_args))
                return  0;
        return  1;
}

static int  isit_r4(const int fd, const struct stat *sb)
{
        int             okcis = 0;
        Cmdint_r4       oci;

        if  (sb->st_size % sizeof(Cmdint_r4) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(fd, 0L, 0);

        while  (read(fd, (char *) &oci, sizeof(oci)) == sizeof(oci))  {
                if  (oci.ci_name[0] == '\0')
                        continue;
                if  (ci4fldsok(&oci))
                        okcis++;
                else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
                        return  0;
        }
        return  okcis > 0;
}

void  conv_r4(const int ifd)
{
        int             cnt = 0;
        Cmdint_r4       oci;

        printf("#! /bin/sh\n# Conversion from release 4\n");
        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &oci, sizeof(oci)) == sizeof(oci))  {
                if  (oci.ci_name[0] == '\0'  ||  !ci4fldsok(&oci))
                        continue;
                if  (oci.ci_name[0] != '\0')  {
                        if  (cnt == CI_STDSHELL)
                                printf("gbch-cichange -i -n %s -N %u -L %u -p %s -a \'%s\' %s\n",
                                       oci.ci_name,
                                       oci.ci_nice, oci.ci_ll,
                                       oci.ci_path, oci.ci_args[0]? oci.ci_args: ":",
                                       DEF_CI_NAME);
                        else
                                printf("gbch-cichange -Ai -N %u -L %u -p %s -a \'%s\' %s\n",
                                       oci.ci_nice, oci.ci_ll,
                                       oci.ci_path, oci.ci_args[0]? oci.ci_args: ":",
                                       oci.ci_name);
                }
                cnt++;
        }
}

static int  ci5fldsok(Cmdint *oci)
{
        if  (oci->ci_ll == 0  ||  oci->ci_nice > 39)
                return  0;
        if  ((oci->ci_flags & ~(CIF_SETARG0|CIF_INTERPARGS)) != 0)
                return  0;
        if  (oci->ci_path[0] != '/')
                return  0;
        if  (!graphic(oci->ci_name) || !graphic(oci->ci_path) || !printable(oci->ci_args))
                return  0;
        return  1;
}

static int  isit_r5(const int fd, const struct stat *sb)
{
        int     okcis = 0;
        Cmdint  oci;

        if  (sb->st_size % sizeof(Cmdint) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(fd, 0L, 0);

        while  (read(fd, (char *) &oci, sizeof(oci)) == sizeof(oci))  {
                if  (oci.ci_name[0] == '\0')
                        continue;
                if  (ci5fldsok(&oci))
                        okcis++;
                else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
                        return  0;

        }
        return  okcis > 0;
}

void  conv_r5(const int ifd)
{
        int             cnt = 0;
        Cmdint          oci;

        printf("#! /bin/sh\n# Conversion from release 5 up\n");
        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &oci, sizeof(oci)) == sizeof(oci))  {
                if  (oci.ci_name[0] == '\0' || !ci5fldsok(&oci))
                        continue;
                if  (oci.ci_name[0] != '\0')  {
                        if  (cnt == CI_STDSHELL)
                                printf("gbch-cichange -n %s -%c%c -N %u -L %u -p %s -a \'%s\' %s\n",
                                       oci.ci_name,
                                       oci.ci_flags & CIF_SETARG0? 't': 'i',
                                       oci.ci_flags & CIF_INTERPARGS? 'e': 'u',
                                       oci.ci_nice, oci.ci_ll,
                                       oci.ci_path, oci.ci_args[0]? oci.ci_args: ":",
                                       DEF_CI_NAME);
                        else
                                printf("gbch-cichange -A%c%c -N %u -L %u -p %s -a \'%s\' %s\n",
                                       oci.ci_flags & CIF_SETARG0? 't': 'i',
                                       oci.ci_flags & CIF_INTERPARGS? 'e': 'u',
                                       oci.ci_nice, oci.ci_ll,
                                       oci.ci_path, oci.ci_args[0]? oci.ci_args: ":",
                                       oci.ci_name);
                }
                cnt++;
        }
}

struct  flst  {
        int     (*isitfn)(const int, const struct stat *);
        void    (*convfn)(const int);
}  funclist[] =  { { isit_r4, conv_r4 },  { isit_r5, conv_r5 } };

#define N_CONVS ((sizeof(funclist)) / sizeof(struct flst))

MAINFN_TYPE  main(int argc, char **argv)
{
        int             ifd, ch, forcevn = 0;
        struct  stat    sbuf;
        extern  int     optind;
        extern  char    *optarg;

        progname = argv[0];

        versionprint(argv, "$Revision: 1.9 $", 0);

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

        /* Open source ci file */

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
                fprintf(stderr, "I am confused about the format of your cmd int file (is it pre-r4?)\n");
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
        fprintf(stderr, "Converted cifile ok\n");
        return  0;
}
