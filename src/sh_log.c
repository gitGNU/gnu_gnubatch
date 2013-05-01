/* sh_log.c -- scheduler logging

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
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "btmode.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "helpalt.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"

static  char    *jlname,        /* File name for job logs */
                *vlname;        /* File name for var logs */

static  char    *unnamedj, *clustermsg, *exportmsg, *localmsg;
static  HelpaltRef      eventnames,
                        sourcenames;

static  FILE    *jlfile,        /* Job log file */
                *vlfile;        /* Var log file */

/* Close any existing log file */

static void  closelog(char **np, FILE **fp)
{
        if  (*fp)  {
                if  (np[0][0] == '|')
                        pclose(*fp);
                else
                        fclose(*fp);
                free(*np);
                *fp = (FILE *) 0;
                *np = (char *) 0;
        }
}

/* Open a new log file */

static  void  openlog(char *name, char **np, FILE **fp, BtmodeRef mp)
{
        FILE    *f;

        if  (name[0] == '\0')
                return;
        if  (name[0] == '|')  {
                f = popen(name+1, "w");
                if  (f == (FILE *) 0)
                        return;
        }
        else  {
                unsigned        mm = 0;

                f = fopen(name, "a");
                if  (f == (FILE *) 0)
                        return;
#if     defined(HAVE_FCHOWN) && !defined(M88000)
                Ignored_error = fchown(fileno(f), (uid_t) mp->o_uid, (gid_t) mp->o_gid);
#else
                Ignored_error = chown(name, (uid_t) mp->o_uid, (gid_t) mp->o_gid);
#endif
                if  (mp->u_flags & BTM_READ)
                        mm |= 0400;
                if  (mp->u_flags & BTM_WRITE)
                        mm |= 0200;
                if  (mp->g_flags & BTM_READ)
                        mm |= 040;
                if  (mp->g_flags & BTM_WRITE)
                        mm |= 020;
                if  (mp->o_flags & BTM_READ)
                        mm |= 04;
                if  (mp->o_flags & BTM_WRITE)
                        mm |= 02;
#ifdef  HAVE_FCHMOD
                fchmod(fileno(f), mm);
#else
                chmod(name, mm);
#endif
        }
        fcntl(fileno(f), F_SETFD, 1);
        *np = stracpy(name);
        *fp = f;
}

/* Flush the job and variable longs when we write out job and variable files.  */

void  flushlogs(const int final)
{
        if  (final)  {
                closelog(&jlname, &jlfile);
                closelog(&vlname, &vlfile);
        }
        else  {
                if  (jlname  &&  jlname[0] == '|'  &&  jlfile)  {
                        pclose(jlfile);
                        jlfile = popen(jlname + 1, "w");
                        if  (jlfile)
                                fcntl(fileno(jlfile), F_SETFD, 1);
                }
                if  (vlname  &&  vlname[0] == '|'  &&  vlfile)  {
                        pclose(vlfile);
                        vlfile = popen(vlname + 1, "w");
                        if  (vlfile)
                                fcntl(fileno(vlfile), F_SETFD, 1);
                }
        }
}

void  openjlog(char *name, BtmodeRef mp)
{
        closelog(&jlname, &jlfile);
        openlog(name, &jlname, &jlfile, mp);
}

void  openvlog(char *name, BtmodeRef mp)
{
        closelog(&vlname, &vlfile);
        openlog(name, &vlname, &vlfile, mp);
}

/* Log job event */

void  logjob(BtjobRef jp, unsigned event, netid_t hostid, int_ugid_t uid, int_ugid_t gid)
{
        time_t  now;
        struct  tm      *tp;
        int     mon, mday;
        const   char    *title;

        if  (!jlfile)
                return;

        now = time((time_t *) 0);
        tp = localtime(&now);
        mon = tp->tm_mon+1;
        mday = tp->tm_mday;
#ifdef  HAVE_TM_ZONE
        if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
        if  (timezone >= 4 * 60 * 60)  {
#endif
                mday = mon;
                mon = tp->tm_mday;
        }

        title = title_of(jp);

        fprintf(jlfile,
                       "%.2d/%.2d/%.4d|%.2d:%.2d:%.2d|%s|%s|",
                       mday, mon, tp->tm_year + 1900,
                       tp->tm_hour, tp->tm_min, tp->tm_sec,
                       JOB_NUMBER(jp),
                       title[0]? title: unnamedj);
        if  (hostid)
                fprintf(jlfile, "%s:", look_host(hostid));
        fprintf(jlfile,
                       "%s|%s|%s|%d|%d",
                       disp_alt((int) event, eventnames),
                       prin_uname((uid_t) uid),
                       prin_gname((gid_t) gid),
                       jp->h.bj_pri,
                       jp->h.bj_ll);
        putc('\n', jlfile);
        fflush(jlfile);
}

/* Log var event.  */

void  logvar(BtvarRef vp, unsigned event, unsigned source, netid_t hostid, int_ugid_t uid, int_ugid_t gid, BtjobRef jp)
{
        time_t  now;
        struct  tm      *tp;
        int     mon, mday;

        if  (!vlfile)
                return;

        now = time((time_t *) 0);
        tp = localtime(&now);
        mon = tp->tm_mon+1;
        mday = tp->tm_mday;
#ifdef  HAVE_TM_ZONE
        if  (tp->tm_gmtoff <= -4 * 60 * 60)  {
#else
        if  (timezone >= 4 * 60 * 60)  {
#endif
                mday = mon;
                mon = tp->tm_mday;
        }

        fprintf(vlfile,
                       "%.2d/%.2d/%.4d|%.2d:%.2d:%.2d|%s|",
                       mday, mon, tp->tm_year + 1900,
                       tp->tm_hour, tp->tm_min, tp->tm_sec,
                       vp->var_name);

        if  (hostid)
                fprintf(vlfile, "%s:", look_host(hostid));

        fprintf(vlfile,
                       "%s|%s|%s|%s|",
                       disp_alt((int) event, eventnames),
                       disp_alt((int) source, sourcenames),
                       prin_uname((uid_t) uid),
                       prin_gname((gid_t) gid));

        switch  (event)  {
        case  $S{log code chcomment}:
                fprintf(vlfile, "%s", vp->var_comment);
                break;
        case  $S{log code chflags}:
                fprintf(vlfile, "%s", vp->var_flags & VF_CLUSTER? clustermsg:
                               vp->var_flags & VF_EXPORT? exportmsg: localmsg);
                break;

        default:
                if  (vp->var_value.const_type == CON_LONG)
                        fprintf(vlfile, "%ld", (long) vp->var_value.con_un.con_long);
                else
                        fprintf(vlfile, "%s", vp->var_value.con_un.con_string);

        case  $S{log code delete}:
                break;
        }
        if  (jp)
                fprintf(vlfile, "|%s|%s", JOB_NUMBER(jp), title_of(jp));
        putc('\n', vlfile);
        fflush(vlfile);
}

/* Set up log strings */

void  initlog()
{
        if  (!(unnamedj = helpprmpt($P{Unnamed job source})))
                unnamedj = "<job>";

        if  (!(clustermsg = helpprmpt($P{Cluster var source})))
                clustermsg = "cluster";
        if  (!(exportmsg = helpprmpt($P{Exported job source})))
                exportmsg = "export";
        if  (!(localmsg = helpprmpt($P{Local only source})))
                localmsg = "local";

        eventnames = helprdalt($Q{Log event type});
        sourcenames = helprdalt($Q{Log source type});

        if  (!eventnames  ||  !sourcenames)  {
                print_error($E{Panic trouble with log in config file});
                exit(E_BADCFILE);
        }
}
