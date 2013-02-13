/* cgiuser.c -- CGI user handling

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
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "defaults.h"
#include "files.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "incl_net.h"
#include "network.h"
#include "errnums.h"
#include "xihtmllib.h"
#include "cgiuser.h"
#include "ecodes.h"

static  FILE    *user_file;
static  char    *login_name, *login_pass;
char            *dest_hostname;
netid_t         dest_hostid;

netid_t  my_look_hostname(const char *name)
{
        netid_t res = look_hostname(name);
        struct  hostent *hp;

        if  (res)
                return  res;

        if  (!(hp = gethostbyname(name)))  {
#ifdef  DGAVIION
                struct  in_addr ina_str;
                ina_str = inet_addr(name);
                res = ina_str.s_addr;
#else
                res = inet_addr(name);
#endif
                if  (res == -1L)
                        return  0;
        }
        else
                res = *(netid_t *) hp->h_addr;

        return  int2ext_netid_t(ext2int_netid_t(res));
}

static void  cgifileopen()
{
        if  (user_file)
                fseek(user_file, 0L, 0);
        else  {
                char    *fn = html_inifile(UFILEN_PARAM, HTML_UFILE);
                int     fd = open(fn, O_RDWR);

                if  (fd < 0  &&  errno == ENOENT)  {
                        int     oldumask = umask(007);
                        fd = open(fn, O_CREAT|O_RDWR, 0660);
                        umask(oldumask);
                        if  (fd >= 0  &&  Effuid != Daemuid) /* Shouldn't matter if not init'ed */
#ifdef  HAVE_FCHOWN
                                Ignored_error = fchown(fd, Daemuid, Effgid);
#else
                                Ignored_error = chown(fn, Daemuid, Effgid);
#endif
                }
                if  (fd < 0  ||  !(user_file = fdopen(fd, "r+")))  {
                        html_error("Cannot open user file");
                        exit(E_SETUP);
                }
                fcntl(fd, F_SETFD, 1);
                free(fn);
        }
}

static ULONG  alloc_key()
{
        static  time_t  seeded = 0;

        if  (!seeded)
                srand(time(&seeded));

        return  (rand() << 16) | (rand() & 0xffff);
}

ULONG  cgi_useralloc(const int_ugid_t uid, const netid_t nid)
{
        time_t  now = time((time_t *) 0);
        long    timeout = html_iniint(DEFLT_TMOPARAM, DEFLT_TIMEOUT);
        int     refr = html_inibool(TOREF_PARAM, 0);
        off_t   firstfree = -1L, current = 0L;
        struct  cgiuser entry;

        cgifileopen();

        while  (fread((char *) &entry, sizeof(entry), 1, user_file) > 0)  {
                if  (entry.alloc + timeout >= now)  {
                        if  (entry.uid == uid)  {
                                if  (refr  ||  entry.desthost != nid)  {
                                        entry.alloc = now;
                                        entry.desthost = nid;
                                        fseek(user_file, current, 0);
                                        fwrite((char *) &entry, sizeof(entry), 1, user_file);
                                }
                                return  entry.key;
                        }
                }
                else  if  (firstfree < 0  ||  current < firstfree)
                        firstfree = current;
                current += sizeof(entry);
        }

        if  (firstfree >= 0)
                fseek(user_file, firstfree, 0);
        BLOCK_ZERO(&entry, sizeof(entry));
        entry.uid = uid;
        entry.alloc = now;
        entry.key = alloc_key();
        entry.desthost = nid;
        fwrite((char *) &entry, sizeof(entry), 1, user_file);
        return  entry.key;
}

int_ugid_t  cgi_uidbykey(const ULONG key)
{
        time_t  now = time((time_t *) 0);
        long    timeout = html_iniint(DEFLT_TMOPARAM, DEFLT_TIMEOUT);
        int     refr = html_inibool(TOREF_PARAM, 0);
        off_t   current = 0L;
        struct  cgiuser entry;

        cgifileopen();

        while  (fread((char *) &entry, sizeof(entry), 1, user_file) > 0)  {
                if  (entry.key == key)  {
                        if  (entry.alloc + timeout < now)
                                break;
                        if  (refr)  {
                                entry.alloc = now;
                                fseek(user_file, current, 0);
                                fwrite((char *) &entry, sizeof(entry), 1, user_file);
                        }
                        if  ((dest_hostid = entry.desthost) == 0  ||  ext2int_netid_t(dest_hostid) == 0)  {
                                dest_hostid = myhostid;
                                dest_hostname = "localhost";
                        }
                        else
                                dest_hostname = look_host(dest_hostid);
                        return  entry.uid;
                }
                current += sizeof(entry);
        }

        return  UNKNOWN_UID;
}

netid_t  cgi_deflthost()
{
        if  (!(dest_hostname = html_inistr(DEFLT_HOSTPARAM, (char *) 0)))
                return  0;
        return  dest_hostid = my_look_hostname(dest_hostname);
}

int_ugid_t  cgi_defltuser(const int subsid)
{
        char    *un;
        int_ugid_t      res;

        if  ((subsid & CGI_AI_REMHOST  &&  !cgi_deflthost())  ||
             !(un = html_inistr(DEFLT_UPARAM, (char *) 0)))
                return  UNKNOWN_UID;

        if  (isdigit(un[0]))  {
                res = atol(un);
                free(un);
                return  isvuser(res)? res: UNKNOWN_UID;
        }
        else
                res = lookup_uname(un);
        free(un);
        return  res;
}

/* NB - This wants generalising!! */

static int  checkpw(const char *name, const char *passwd)
{
        static  char    *btpwnam;
        int             ipfd[2], opfd[2];
        char            rbuf[1];
        PIDTYPE         pid;

        if  (!btpwnam)
                btpwnam = envprocess(BTPWPROG);

        /* Don't bother with error messages, just say no.  */

        if  (pipe(ipfd) < 0)
                return  0;
        if  (pipe(opfd) < 0)  {
                close(ipfd[0]);
                close(ipfd[1]);
                return  0;
        }

        if  ((pid = fork()) == 0)  {
                close(opfd[1]);
                close(ipfd[0]);
                if  (opfd[0] != 0)  {
                        close(0);
                        Ignored_error = dup(opfd[0]);
                        close(opfd[0]);
                }
                if  (ipfd[1] != 1)  {
                        close(1);
                        Ignored_error = dup(ipfd[1]);
                        close(ipfd[1]);
                }
                execl(btpwnam, btpwnam, name, (char *) 0);
                exit(255);
        }
        close(opfd[0]);
        close(ipfd[1]);
        if  (pid < 0)  {
                close(ipfd[0]);
                close(opfd[1]);
                return  0;
        }
        Ignored_error = write(opfd[1], passwd, strlen(passwd));
        rbuf[0] = '\n';
        Ignored_error = write(opfd[1], rbuf, sizeof(rbuf));
        close(opfd[1]);
        if  (read(ipfd[0], rbuf, sizeof(rbuf)) != sizeof(rbuf))  {
                close(ipfd[0]);
                return  0;
        }
        close(ipfd[0]);
        return  rbuf[0] == '0'? 1: 0;
}

static void  log_host(char *arg)
{
        dest_hostname = stracpy(arg);
}

static void  log_name(char *arg)
{
        login_name = stracpy(arg);
}

static void  log_pass(char *arg)
{
        login_pass = stracpy(arg);
}

static void  gotlogin(const int_ugid_t uid, const netid_t nid, const int isdeflt)
{
        ULONG  key = cgi_useralloc(uid, nid);
        if  (html_out_param_file(isdeflt? "defltlogin": "gotlogin", 1, key, html_cookexpiry()))
                exit(0);
        html_error("No got login file");
        exit(E_SETUP);
}

struct  posttab logintab[] =  {
        { "login", log_name  },
        { "passwd", log_pass  },
        { "desthost", log_host  },
        { (char *) 0  }
};

char **cgi_arginterp(const int ac, char **av, const int subsid)
{
        char            *qs, **resargs;
        int_ugid_t      uid;
        ULONG           key;
#ifdef  DEBUG
        FILE            *foo = fopen("/tmp/foo", "a");
        extern  const   char    *progname;
#endif

        /* If nothing given, invoke no argument routine */

        if  (ac <= 1)  {
                if  (subsid & CGI_AI_SUBSID)
                        goto  badargs;
                if  ((uid = cgi_defltuser(subsid)) != UNKNOWN_UID)
                        gotlogin(uid, dest_hostid, 1);
                html_out_or_err("nologin", 1);
                exit(0);
        }

        /* Otherwise convert argument(s)
           Prefer QUERY_STRING to arguments as server software
           may mangle it */

        qs = getenv("QUERY_STRING");
#ifdef  DEBUG
        fprintf(foo, "%s: Query string is %s av[1] is %s\n", progname, qs? qs: "None", av[1]);
        fflush(foo);
#endif
        resargs = html_getvalues(qs? qs: av[1]);

        if  (ncstrcmp(resargs[0], "login") == 0)  {
                char    *pp;
                if  (subsid & CGI_AI_SUBSID)
                        goto  badargs;
                html_postvalues(logintab);
                if  (subsid & CGI_AI_REMHOST)  {
                        if  (!dest_hostname  || dest_hostname[0] == '\0')  {
                                if  (cgi_deflthost() == 0)  {
                                        html_out_or_err("invhost", 1);
                                        exit(0);
                                }
                        }
                        else  if  (!(dest_hostid = my_look_hostname(dest_hostname)))  {
                                html_out_or_err("invhost", 1);
                                exit(0);
                        }
                }

                if  (!(login_name  &&  login_pass  &&
                       (uid = lookup_uname(login_name)) != UNKNOWN_UID  &&  checkpw(login_name, login_pass)))  {
                        html_out_or_err("logfailed", 1);
                        exit(0);
                }

                /* Obliterate traces of password */

                for  (pp = login_pass;  *pp;  pp++)
                        *pp = '*';
                free(login_pass);

                /* Allocate new user slot and proceed with default format */

                gotlogin(uid, dest_hostid, 0);
        }

        /* Normal transactions start with key.
           Check validity and treat invalid as stale */

        if  (isdigit(resargs[0][0]))  {
                key = strtoul(resargs[0], (char **) 0, 0);
                if  ((uid = cgi_uidbykey(key)) == UNKNOWN_UID)  {
                        freehelp(resargs);
                        if  (html_out_param_file(subsid & CGI_AI_SUBSID? "stalesubs": "stalelogin",
                                                 1,
                                                 cgi_defltuser(subsid) != UNKNOWN_UID? 1: 0,
                                                 html_iniint(DEFLT_TMOPARAM, DEFLT_TIMEOUT)/(3600*24L)))
                                exit(0);
                        html_error("No stale login file");
                        exit(E_SETUP);
                }
                Realuid = uid;
                return  resargs + 1;
        }
        else  {
        badargs:
                html_out_or_err("badargs", 1);
                exit(0);
        }
        return  (char **) 0;
}
