/* btcilist.c -- main module for gbch-cilist

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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "bjparam.h"
#include "cmdint.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpargs.h"
#include "cfile.h"
#include "files.h"
#include "helpalt.h"
#include "btconst.h"
#include "btmode.h"
#include "btvar.h"
#include "btuser.h"
#include "xbnetq.h"
#include "optflags.h"
#include "services.h"

#define BTCILIST_INLINE

static  char    Filename[] = __FILE__;

extern  netid_t Out_host;

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

OPTION(o_explain)
{
        print_error($E{btcilist explain});
        exit(0);
        return  0;              /* Silence compilers */
}

OPTION(o_disphost)
{
        if  (!arg)
                return  OPTRESULT_MISSARG;

        if  (strcmp(arg, "-") == 0)  {
                Out_host = 0L;
                return  OPTRESULT_ARG_OK;
        }
        if  ((Out_host = look_int_hostname(arg)) == -1)  {
                print_error($E{Unknown queue host});
                exit(E_USAGE);
        }
        return  OPTRESULT_ARG_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

const   char    Sname[] = GBNETSERV_PORT;

static  struct  sockaddr_in     serv_addr, cli_addr;

/* Defaults and proc table for arg interp.  */

const   Argdefault      Adefs[] = {
  {  '?', $A{btcilist arg explain} },
  {  'Q', $A{btcilist arg host} },
  { 0, 0 }
};

optparam        optprocs[] = {
o_explain,      o_disphost,     o_freezecd,     o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
        fprintf(dest, "%s", name);
        spitoption($A{btcilist arg host}, $A{btcilist arg explain}, dest, '=', 0);
        fprintf(dest, " %s\n", Out_host? look_host(Out_host): "-");
}

static int  initsock(const netid_t hostid)
{
        int     sockfd, portnum;
        struct  servent *sp;

        /* Get port number for this caper */

        if  (!(sp = env_getserv(Sname, IPPROTO_UDP)))  {
                print_error($E{No xbnetserv UDP service});
                endservent();
                exit(E_NETERR);
        }
        portnum = sp->s_port;
        endservent();

        BLOCK_ZERO(&serv_addr, sizeof(serv_addr));
        BLOCK_ZERO(&cli_addr, sizeof(cli_addr));
        serv_addr.sin_family = cli_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = hostid;
        cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = portnum;
        cli_addr.sin_port = 0;

        /* Save now in case of error.  */

        disp_arg[0] = ntohs(portnum);
        disp_arg[1] = hostid;

        if  ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)  {
                print_error($E{Cannot create UDP access socket});
                exit(E_NETERR);
        }
        if  (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)  {
                print_error($E{Cannot bind UDP access socket});
                close(sockfd);
                exit(E_NETERR);
        }
        return  sockfd;
}

static void  udp_enquire(const int udpsock, char *outmsg, const int outlen)
{
        if  (sendto(udpsock, outmsg, outlen, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  {
                disp_arg[1] = serv_addr.sin_addr.s_addr;
                print_error($E{Cannot send UDP packet});
                exit(E_NETERR);
        }
}

static int  udp_get(const int udpsock, char *inmsg, const int inlen)
{
        int     inbytes;
        SOCKLEN_T               repl = sizeof(struct sockaddr_in);
        struct  sockaddr_in     reply_addr;
        if  ((inbytes = recvfrom(udpsock, inmsg, inlen, 0, (struct sockaddr *) &reply_addr, &repl)) <= 0)  {
                disp_arg[1] = serv_addr.sin_addr.s_addr;
                print_error($E{Cannot receive UDP packet});
                exit(E_NETERR);
        }
        return  inbytes;
}

#define INIT_CIS        10
#define INC_CIS         5

static void  get_remcilist(const netid_t host)
{
        int     fd = initsock(host), inbytes;
        unsigned  Ci_max = 0;
        char    obuf[1];
        char    resbuf[CL_SV_BUFFSIZE];

        obuf[0] = CL_SV_CILIST;
        udp_enquire(fd, obuf, sizeof(obuf));

        /* The following code assumes that xbnetserv only sends
           complete Cmdints (see xbnet_ua.c).  */

        while  ((inbytes = udp_get(fd, resbuf, sizeof(resbuf))) >= sizeof(Cmdint))  {
                char    *bp = resbuf;
                int     bytesleft;
                CmdintRef       cp, icp;
                for  (bytesleft = inbytes;  bytesleft >= sizeof(Cmdint);  bytesleft -= sizeof(Cmdint))  {
                        if  (!*bp)
                                return;
                        if  (Ci_num >= Ci_max)  {
                                if  (Ci_max == 0)  {
                                        Ci_list = (CmdintRef) malloc(INIT_CIS * sizeof(Cmdint));
                                        Ci_max = INIT_CIS;
                                }
                                else  {
                                        Ci_max += INC_CIS;
                                        Ci_list = (CmdintRef) realloc((char *) Ci_list, Ci_max * sizeof(Cmdint));
                                }
                                if  (!Ci_list)
                                        ABORT_NOMEM;
                        }
                        icp = (CmdintRef) bp;
                        cp = &Ci_list[Ci_num];
                        cp->ci_ll = ntohs(icp->ci_ll);
                        cp->ci_nice = icp->ci_nice;
                        cp->ci_flags = icp->ci_flags;
                        strcpy(cp->ci_name, icp->ci_name);
                        strcpy(cp->ci_path, icp->ci_path);
                        strcpy(cp->ci_args, icp->ci_args);
                        bp += sizeof(Cmdint);
                        Ci_num++;
                }
        }
}

static void  cidisplay()
{
        unsigned        cnt;
        int     namel = 0, argl = 0, pathl = 0, lng;
        char    *sa0, *expa;

        sa0 = gprompt($P{Ci set arg0});
        expa = gprompt($P{Ci expand args});

        for  (cnt = 0;  cnt < Ci_num;  cnt++)  {
                CmdintRef       cp = &Ci_list[cnt];
                if  (cp->ci_name[0] == '\0')
                        continue;
                lng = strlen(cp->ci_name);
                if  (lng > namel)
                        namel = lng;
                lng = strlen(cp->ci_path);
                if  (lng > pathl)
                        pathl = lng;
                lng = strlen(cp->ci_args);
                if  (lng > argl)
                        argl = lng;
        }

        for  (cnt = 0;  cnt < Ci_num;  cnt++)  {
                CmdintRef       cp = &Ci_list[cnt];
                if  (cp->ci_name[0] == '\0')
                        continue;
                printf("%-*.*s %-*.*s %5u%3u %-*.*s",
                       namel, namel, cp->ci_name,
                       pathl, pathl, cp->ci_path,
                       cp->ci_ll, (unsigned) cp->ci_nice,
                       argl, argl, cp->ci_args);
                if  (cp->ci_flags & CIF_INTERPARGS)
                        printf(" %s", expa);
                if  (cp->ci_flags & CIF_SETARG0)
                        printf(" %s", sa0);
                putchar('\n');
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
        char    *Curr_pwd = (char *) 0;
        int     ret;
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
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
        INIT_DAEMUID
        Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
        SCRAMBLID_CHECK
        argv = optprocess(argv, Adefs, optprocs, $A{btcilist arg explain}, $A{btcilist arg freeze home}, 0);

#include "inline/freezecode.c"

        if  (argv[0])  {
                disp_str = argv[0];
                print_error($E{Btcilist usage});
                return  E_USAGE;
        }

        if  (Anychanges & OF_ANY_FREEZE_WANTED)
                exit(0);

        /* Now we want to be Daemuid throughout if possible.  */

        setuid(Daemuid);

        if  (Out_host)
                get_remcilist(Out_host);
        else  if  ((ret = open_ci(O_RDONLY)) != 0)  {
                print_error(ret);
                exit(E_SETUP);
        }
        cidisplay();
        return  0;
}
