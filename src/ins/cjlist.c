/* cjlist.c -- dump out job list as shell script

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
#ifdef  HAVE_LIMITS_H
#include <limits.h>
#endif
#include <pwd.h>
#include <grp.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
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
#include "jobsave.h"
#include "spitrouts.h"
#include "ecodes.h"

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

struct  {
        char    *srcdir;        /* Directory we read from if not pwd */
        char    *outdir;        /* Directory we write to */
        char    *outfile;       /* Output file */
        char    *delim;         /* Delimiter prefix */
        long    errtol;         /* Number of errors we'll take */
        long    errors;         /* Number we've had */
        short   ignsize;        /* Ignore file size */
        short   ignfmt;         /* Ignore file format errors */
        short   ignusers;       /* Ignore invalid users */
}  popts;

extern char *expand_srcdir(char *);
extern char *make_absolute(char *);

char    *progname;

void    nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static int unameok(const char *un, const int_ugid_t uid)
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

static int gnameok(const char *gn, const int_ugid_t gid)
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

static int jobok(const jobno_t jobnum)
{
        char    *nam;
        struct  stat    sbuf;

        if  (jobnum == 0)
                return  0;
        nam = mkspid(SPNAM, jobnum);
        if  (stat(nam, &sbuf) < 0)
                return  0;
        return  1;
}

static int validvar(const Vref *vr)
{
        const   char    *cp = vr->sv_name;

        if  (isdigit(*cp))
                return  0;
        do  {
                if  (!isalnum(*cp)  &&  *cp != '_')
                        return  0;
                cp++;
        }  while  (*cp);
        return  1;
}

static int jobfldsok(struct Jsave *old)
{
        int     cnt;

        if  (!jobok(old->sj_job))
                return  0;
        if  (old->sj_progress > BJP_FINISHED)
                return  0;
        if  (old->sj_pri == 0)
                return  0;
        if  (old->sj_ll == 0)
                return  0;
        if  (old->sj_umask > 0777)
                return  0;
        if  (old->sj_ulimit < 0)
                return  0;
        if  (!old->sj_cmdinterp[0])
                return  0;
        if  (!unameok(old->sj_mode.o_user, old->sj_mode.o_uid))
                return  0;
        if  (!unameok(old->sj_mode.c_user, old->sj_mode.c_uid))
                return  0;
        if  (!gnameok(old->sj_mode.o_group, old->sj_mode.o_gid))
                return  0;
        if  (!gnameok(old->sj_mode.c_group, old->sj_mode.c_gid))
                return  0;
        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                if  (old->sj_conds[cnt].sjc_compar == C_UNUSED)
                        break;
                if  (old->sj_conds[cnt].sjc_compar > C_GE)
                        return  0;
                if  (old->sj_conds[cnt].sjc_crit > CCRIT_NORUN)
                        return  0;
                if  (!validvar(&old->sj_conds[cnt].sjc_varind))
                        return  0;
                if  (old->sj_conds[cnt].sjc_value.const_type <= CON_NONE || old->sj_conds[cnt].sjc_value.const_type > CON_STRING)
                        return  0;
        }
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                if  (old->sj_asses[cnt].sja_op == BJA_NONE)
                        break;
                if  (old->sj_asses[cnt].sja_op > BJA_SSIG)
                        return  0;
                if  (old->sj_asses[cnt].sja_crit > ACRIT_NORUN)
                        return  0;
                if  ((old->sj_asses[cnt].sja_flags & (BJA_START|BJA_OK|BJA_ERROR|BJA_ABORT|BJA_CANCEL)) == 0)
                        return  0;
                if  ((old->sj_asses[cnt].sja_flags & ~(BJA_START|BJA_OK|BJA_ERROR|BJA_ABORT|BJA_CANCEL|BJA_REVERSE)) != 0)
                        return  0;
                if  (!validvar(&old->sj_asses[cnt].sja_varind))
                        return  0;
                if  (old->sj_asses[cnt].sja_con.const_type <= CON_NONE || old->sj_asses[cnt].sja_con.const_type > CON_STRING)
                        return  0;
        }
        return  1;
}

static const char *get_host(netid_t nid)
{
        struct  hostent *dbhost;
        if  ((dbhost = gethostbyaddr((char *) &nid, sizeof(nid), AF_INET)))
                return  dbhost->h_name;
        return  "unknown";
}

static  FILE    *openjob(const jobno_t jobnum)
{
        return  fopen(mkspid(SPNAM, jobnum), "r");
}

static  int     copyjob(FILE *ifd, const jobno_t jobnum)
{
        int     ch;
        char    *nam = mkspid(SPNAM, jobnum);
        FILE    *ofd;
        char    path[PATH_MAX];

        sprintf(path, "%s/%s", popts.outdir, nam);
        if  (!(ofd = fopen(path, "w")))
                return  0;
        while  ((ch = getc(ifd)) != EOF)
                putc(ch, ofd);
        fclose(ofd);
        return  1;
}

static  void    copyjob_stdout(FILE *ifd, const jobno_t jobnum)
{
        int     ch;
        printf("<<\'%s_%lu\'\n", popts.delim, (unsigned long) jobnum);
        while  ((ch = getc(ifd)) != EOF)
                putchar(ch);
        printf("%s_%lu\n", popts.delim, (unsigned long) jobnum);
}

static  void    conv_env(const unsigned nenv, const int envp, const char *spacep)
{
        unsigned        cnt;
        const   Envir   *el = (const Envir *) &spacep[envp];

        for  (cnt = 0;  cnt < nenv;  el++, cnt++)
                printf("%s=\'%s\' \\\n", &spacep[el->e_name], &spacep[el->e_value]);
}

static void conv_redirs(const unsigned nredirs, const int redirp, const char *spacep)
{
        const   Redir   *rp = (const Redir *) &spacep[redirp];
        unsigned        cnt;

        for  (cnt = 0;  cnt < nredirs;  rp++, cnt++)  {
                fputs(" -I \'", stdout);
                spit_redir(stdout, rp->fd, rp->action, rp->arg, &spacep[rp->arg]);
                putchar('\'');
        }
}

static void conv_args(const unsigned nargs, const int argp, const char *spacep)
{
        const Jarg  *ap = (const Jarg *) &spacep[argp];
        unsigned  cnt;

        for  (cnt = 0;  cnt < nargs;  ap++, cnt++)  {
                const   char    *cp = &spacep[*ap];
                fputs(" -a \'", stdout);
                while  (*cp)  {
                        if  (*cp == '\'')
                                fputs("\'\\\'", stdout);
                        putchar(*cp);
                        cp++;
                }
                putchar('\'');
        }
}

static void conv_conds(struct Sjcond *sjc)
{
        unsigned  cnt;
        Vref    *vr;

        for  (cnt = 0;  cnt < MAXCVARS;  sjc++, cnt++)  {
                if  (sjc->sjc_compar == C_UNUSED)
                        return;
                fputs(sjc->sjc_crit & CCRIT_NORUN? " -k": " -K", stdout);
                fputs(" -c ", stdout);
                vr = &sjc->sjc_varind;
                if  (vr->sv_hostid)
                        printf("\'%s:%s%s", (char *) get_host(vr->sv_hostid), vr->sv_name, condname[sjc->sjc_compar - C_EQ]);
                else
                        printf("\'%s%s", vr->sv_name, condname[sjc->sjc_compar - C_EQ]);
                if  (sjc->sjc_value.const_type == CON_LONG)
                        printf("%ld\'", (long) sjc->sjc_value.con_un.con_long);
                else  {
                        if  (isdigit(sjc->sjc_value.con_un.con_string[0]))
                                putchar(':');
                        printf("%s\'", sjc->sjc_value.con_un.con_string);
                }
        }
}

static void conv_asses(struct Sjass *sja)
{
        unsigned  cnt;
        Vref    *vr;

        for  (cnt = 0;  cnt < MAXSEVARS;  sja++, cnt++)  {
                if  (sja->sja_op == BJA_NONE)
                        return;
                fputs(" -f ", stdout);
                if  (sja->sja_flags)  {
                        if  (sja->sja_flags & BJA_START)
                                putchar('S');
                        if  (sja->sja_flags & BJA_REVERSE)
                                putchar('R');
                        if  (sja->sja_flags & BJA_OK)
                                putchar('N');
                        if  (sja->sja_flags & BJA_ERROR)
                                putchar('E');
                        if  (sja->sja_flags & BJA_ABORT)
                                putchar('A');
                        if  (sja->sja_flags & BJA_CANCEL)
                                putchar('C');
                }
                else
                        putchar('-');

                fputs(sja->sja_crit & ACRIT_NORUN? "  -b": " -B", stdout);
                fputs(" -s ", stdout);
                vr = &sja->sja_varind;
                if  (vr->sv_hostid)
                        printf("\'%s:%s", (char *) get_host(vr->sv_hostid), vr->sv_name);
                else
                        printf("\'%s", vr->sv_name);

                if  (sja->sja_op >= BJA_SEXIT)  {
                        putchar('=');
                        fputs(sja->sja_op == BJA_SEXIT? "exitcode": "signal", stdout);
                        putchar('\'');
                }
                else  {
                        printf("%s", assname[sja->sja_op-BJA_ASSIGN]);
                        if  (sja->sja_con.const_type == CON_LONG)
                                printf("%ld", (long) sja->sja_con.con_un.con_long);
                        else  {
                                if  (isdigit(sja->sja_con.con_un.con_string[0]))
                                        putchar(':');
                                fputs(sja->sja_con.con_un.con_string, stdout);
                        }
                        putchar('\'');
                }
        }
}

static void conv_time(TimeconRef tc)
{
        unsigned        nd;
        struct  tm      *t;
        static  char    *days_abbrev[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Hday" };
        static  char    *timnames[] = { "Minutes", "Hours", "Days", "Weeks", "Monthsb", "Monthse", "Years" };
        if  (!tc->tc_istime)
                return;

        t = localtime(&tc->tc_nexttime);
        printf(" -T %.2d/%.2d/%.2d,%.2d:%.2d",
                       t->tm_year % 100,
                       t->tm_mon + 1,
                       t->tm_mday,
                       t->tm_hour,
                       t->tm_min);

        if  (tc->tc_repeat < TC_MINUTES)
                printf(" -%c", tc->tc_repeat == TC_DELETE? 'd': 'o');
        else  {
                printf(" -r %s:%ld", timnames[tc->tc_repeat - TC_MINUTES], (long) tc->tc_rate);
                if  (tc->tc_repeat == TC_MONTHSB)
                        printf(":%d", tc->tc_mday);
                else  {
                        int     mday;
                        static  char    month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
                        month_days[1] = t->tm_year % 4 == 0? 29: 28;
                        mday = month_days[t->tm_mon] - tc->tc_mday;
                        if  (mday <= 0)
                                mday = 1;
                        printf(":%d", mday);
                }
        }
        fputs(" -A -", stdout);
        for  (nd = 0;  nd < TC_NDAYS;  nd++)
                if  (tc->tc_nvaldays & (1 << nd))
                        printf(",%s", days_abbrev[nd]);
        printf(" -%c", "SHR9"[tc->tc_nposs]);
}

int isit_r5(const int ifd, const struct stat *sb)
{
        int     okjobs = 0;
        struct  Jsave   old;

        if  (sb->st_size  ==  0)
                return  1;

        if  ((sb->st_size % sizeof(struct Jsave)) != 0  &&  (!popts.ignsize || ++popts.errors > popts.errtol))
                return  0;

        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))  {
                if  (jobfldsok(&old))
                        okjobs++;
                else  if  (!popts.ignfmt || ++popts.errors > popts.errtol)
                        return  0;
        }
        return  okjobs > 0;
}

void conv_r5(const int ifd)
{
        FILE    *jfile;
        struct  Jsave   old;

        printf("#! /bin/sh\n# Conversion from release 5/6\n");
        lseek(ifd, 0L, 0);

        while  (read(ifd, (char *) &old, sizeof(old)) == sizeof(old))  {
                if  (!jobfldsok(&old)  ||  !(jfile = openjob(old.sj_job)))
                        continue;
                if  (popts.outdir  &&  !copyjob(jfile, old.sj_job))  {
                        fclose(jfile);
                        continue;
                }
                printf("\n# Conversion of job number %ld\n\n", (long) old.sj_job);
                conv_env(old.sj_nenv, old.sj_env, old.sj_space);
                printf(BTR_PROGRAM " -%c -%c -%c -p %d -i %s -l %u -P 0%.3o -L %ld -t %u -Y %ld -2 %u -W %d",
                              old.sj_progress == BJP_NONE? 'N': old.sj_progress == BJP_DONE? '.': 'C',
                              old.sj_jflags & BJ_REMRUNNABLE? 'G': old.sj_jflags & BJ_EXPORT? 'F': 'n',
                              old.sj_jflags & BJ_NOADVIFERR? 'J': 'j',
                              old.sj_pri, old.sj_cmdinterp, old.sj_ll,
                              old.sj_umask, (long) old.sj_ulimit,
                              old.sj_deltime, (long) old.sj_runtime, old.sj_runon, old.sj_autoksig);
                if  (old.sj_title >= 0)
                        printf(" -h \'%s\'", &old.sj_space[old.sj_title]);
                if  (old.sj_direct >= 0)
                        printf(" -D \'%s\'", &old.sj_space[old.sj_direct]);
                if  (old.sj_jflags & BJ_WRT)
                        fputs(" -w", stdout);
                if  (old.sj_jflags & BJ_MAIL)
                        fputs(" -m", stdout);
                printf(" -X N%d:%d -X E%d:%d",
                              old.sj_exits.nlower, old.sj_exits.nupper,
                              old.sj_exits.elower, old.sj_exits.eupper);
                fputs(" -M ", stdout);
                dumpmode(stdout, "U", old.sj_mode.u_flags);
                dumpmode(stdout, ",G", old.sj_mode.g_flags);
                dumpmode(stdout, ",O", old.sj_mode.o_flags);
                printf(" -u %s -g %s", old.sj_mode.o_user, old.sj_mode.o_group);
                conv_redirs(old.sj_nredirs, old.sj_redirs, old.sj_space);
                conv_args(old.sj_nargs, old.sj_arg, old.sj_space);
                conv_conds(old.sj_conds);
                conv_asses(old.sj_asses);
                conv_time(&old.sj_times);
                putchar(' ');
                if  (popts.outdir)
                        printf("%s/%s\n", popts.outdir, mkspid(SPNAM, old.sj_job));
                else
                        copyjob_stdout(jfile, old.sj_job);
                fclose(jfile);
        }
}

static  int     delimok(char *arg)
{
        if  (!isalpha(*arg))
                return  0;
        while  (*++arg)
                if  (!isalnum(*arg) && *arg != '_')
                        return  0;
        return  1;
}

MAINFN_TYPE main(int argc, char **argv)
{
        int             ifd, ch, forcevn = 0;
        struct  stat    sbuf;
        struct  flock   rlock;
        extern  int     optind;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);
        progname = argv[0];

        while  ((ch = getopt(argc, argv, "usfe:v:D:I:")) != EOF)
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
                        if  (forcevn < 5  ||  forcevn > 6)  {
                                fprintf(stderr, "Sorry I don't know about version %d\n", forcevn);
                                return  101;
                        }
                        continue;
                case  'I':
                        if  (!delimok(optarg))  {
                                fprintf(stderr, "Invalid format delimiter %s - should be name\n", optarg);
                                return  102;
                        }
                        popts.delim = optarg;
                        continue;
                }

        if  ((argc - optind != 3 && !popts.delim) || (argc - optind != 2 && popts.delim))  {
        usage:
                fprintf(stderr, "Usage: %s [-D dir] [-u] [-s] [-f] [-e n] [-v n] [-I delim] jfile outfile [workdir]\n", argv[0]);
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

        if  (!popts.delim)  {

                /* Get out directory for saved jobs before we mess around with
                   output files and suchwhat. */

                popts.outdir = argv[optind+2];
                if  (stat(popts.outdir, &sbuf) < 0)  {
                        fprintf(stderr, "Cannot find directory %s\n", popts.outdir);
                        return  4;
                }
                if  ((sbuf.st_mode & S_IFMT) != S_IFDIR)  {
                        fprintf(stderr, "%s is not a directory\n", popts.outdir);
                        return  5;
                }
        }

        /* Create output file, remembering to unlink it later
           if something goes wrong */

        popts.outfile = argv[optind+1];
        if  (!freopen(popts.outfile, "w", stdout))  {
                fprintf(stderr, "Sorry cannot create %s\n", popts.outfile);
                return  3;
        }

        /* Now change directory to the source directory if specified */

        if  (popts.srcdir)  {
                popts.outfile = make_absolute(popts.outfile);
                if  (popts.outdir)
                        popts.outdir = make_absolute(popts.outdir);
                if  (chdir(popts.srcdir) < 0)  {
                        fprintf(stderr, "Cannot open source directory %s\n", popts.srcdir);
                        unlink(popts.outfile);
                        return  13;
                }
        }

        /* Open source job file */

        if  ((ifd = open(argv[optind], O_RDONLY)) < 0)  {
                fprintf(stderr, "Sorry cannot open %s\n", argv[optind]);
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
                if  (isit_r5(ifd, &sbuf))  {
                        conv_r5(ifd);
                        goto  dun;
                }
                fprintf(stderr, "Too many errors in file - conversion failed\n");
                unlink(popts.outfile);
                return  10;
        }
        else  if  (isit_r5(ifd, &sbuf))
                conv_r5(ifd);
        else  {
                fprintf(stderr, "I am confused about the format of your job file (is it pre-r5?)\n");
                unlink(popts.outfile);
                return  9;
        }

 dun:
        if  (popts.errors > 0)
                fprintf(stderr, "There were %ld error%s found\n", popts.errors, popts.errors > 1? "s": "");
        close(ifd);
#ifdef  HAVE_FCHMOD
        fchmod(fileno(stdout), 0755);
#else
        chmod(popts.outfile, 0755);
#endif
        fprintf(stderr, "Finished outputting job file\n");
        return  0;
}
