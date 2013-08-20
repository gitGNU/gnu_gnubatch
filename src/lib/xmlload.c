/* xmlload.c -- Load up jobs from XML files.

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
#ifdef  HAVE_LIBXML2
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "bjparam.h"
#include "btjob.h"
#include "btvar.h"
#include "errnums.h"
#include "ipcstuff.h"
#include "q_shm.h"
#include "files.h"
#include "jvuprocs.h"
#include "xmlldsv.h"

#ifdef  HAVE_LIBXML2

static  char    Filename[] = __FILE__;

#define  CCHARP  (const char *)
#define  CXMLCH  (const xmlChar *)

static  int     load_xml_bool(xmlNodePtr nd, const char *name)
{
        xmlNodePtr  cnode;
        for  (cnode = nd->children;  cnode;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, name) == 0)
                        return  1;
        return  0;
}

static  long    load_xml_int(xmlNodePtr nd)
{
        return  atol(CCHARP nd->children->content);
}

static  netid_t load_hostid(xmlNodePtr nd)
{
        netid_t  nid = look_int_hostname(CCHARP nd->children->content);
        if  (nid == -1)
                nid = 0;
        return  nid;
}

static  vhash_t load_varind(xmlNodePtr nd, const unsigned perm)
{
        netid_t  hid = 0;
        const  char  *vname = CCHARP 0;
        xmlNodePtr  cnode;
        ULONG   Saveseq;

        cnode = nd->children;
        if  (cnode->type == XML_TEXT_NODE)  {
                const  char  *vn = CCHARP cnode->content;
                const  char  *cp = strchr(vn, ':');
                if  (cp)  {
                        unsigned  l = cp - vn;
                        char    b[1024];
                        if  (l > 1023)
                                l = 1023;
                        strncpy(b, vn, l);
                        b[l+1] = '\0';
                        hid = look_int_hostname(b);
                        vname = cp+1;
                }
                else
                        vname = cp;
        }
        else  for  (;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;
                if  (strcmp(nname, "host") == 0)
                        hid = look_int_hostname(CCHARP cnode->children->content);
                else  if (strcmp(nname, "name") == 0)
                        vname = CCHARP cnode->children->content;
        }

        if  (!vname  ||  hid == -1)
                return  -1;
        return  lookupvar(vname, hid, perm, &Saveseq);
}

static  int     load_value(xmlNodePtr nd, BtconRef value)
{
        xmlNodePtr  cnode = nd->children;

        if  (!cnode)
                return  0;
        if  (strcmp(CCHARP cnode->name, "textval") == 0)  {
                strncpy(value->con_un.con_string, CCHARP cnode->children->content, BTC_VALUE);
                value->const_type = CON_STRING;
        }
        else  {
                value->con_un.con_long = load_xml_int(cnode);
                value->const_type = CON_LONG;
        }
        return  1;
}

static  void    load_jobnum(xmlNodePtr nd, BtjobhRef jh)
{
        xmlNodePtr cnode;

        for  (cnode = nd->children;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;
                if  (strcmp(nname, "host") == 0)
                        jh->bj_hostid = load_hostid(cnode);
                else  if  (strcmp(nname, "num"))
                        jh->bj_job = load_xml_int(cnode);
        }
}

static  void    load_mode(xmlNodePtr nd, BtmodeRef md)
{
        xmlNodePtr cnode;

        for  (cnode = nd->children;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;
                switch  (nname[0])  {
                case  'u':
                        if  (strcmp(nname, "uperm") == 0)
                                md->u_flags = load_xml_int(cnode);
                        else  if  (strcmp(nname, "user") == 0)  {
                                if  (cnode->children)
                                        strncpy(md->o_user, CCHARP cnode->children->content, UIDSIZE);
                        }
                        continue;
                case  'g':
                        if  (strcmp(nname, "gperm") == 0)
                                md->g_flags = load_xml_int(cnode);
                        else  if  (strcmp(nname, "group") == 0)  {
                                if  (cnode->children)
                                        strncpy(md->o_group, CCHARP cnode->children->content, UIDSIZE);
                        }
                        continue;
                case  'o':
                        if  (strcmp(nname, "operm") == 0)
                                md->o_flags = load_xml_int(cnode);
                        continue;
                }
        }
}

static  void    load_exits(xmlNodePtr nd, unsigned char *l, unsigned char *u)
{
        xmlNodePtr cnode;
        for  (cnode = nd->children;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;
                if  (strcmp(nname, "l") == 0)
                        *l = load_xml_int(cnode);
                else  if  (strcmp(nname, "u") == 0)
                        *u = load_xml_int(cnode);
        }
}

static  void    load_times(xmlNodePtr nd, TimeconRef tms)
{
        xmlNodePtr  cnode;
        const  char  *nname = CCHARP xmlGetProp(nd, CXMLCH "timeset");

        if  (nname && strcmp(nname, "y") == 0)
                tms->tc_istime = 1;
        for  (cnode = nd->children;  cnode;  cnode = cnode->next)  {
                nname = CCHARP cnode->name;
                switch  (nname[0])  {
                case  'n':
                        if  (strcmp(nname, "nexttime") == 0)
                                tms->tc_nexttime = load_xml_int(cnode);
                        else  if  (strcmp(nname, "nvaldays") == 0)
                                tms->tc_nvaldays = load_xml_int(cnode);
                        else  if  (strcmp(nname, "nposs") == 0)
                                tms->tc_nposs = load_xml_int(cnode);
                        continue;
                case  'm':
                        if  (strcmp(nname, "mday") == 0)
                                tms->tc_mday = load_xml_int(cnode);
                        continue;
                case  'r':
                        if  (strcmp(nname, "repeat") == 0)
                                tms->tc_repeat = load_xml_int(cnode);
                        else  if  (strcmp(nname, "rate") == 0)
                                tms->tc_rate = load_xml_int(cnode);
                        continue;
                }
        }
}

static  int     load_cond(xmlNodePtr nd, JcondRef cnd)
{
        xmlNodePtr  cnode;
        const  char     *ty = CCHARP xmlGetProp(nd, CXMLCH "type");
        int     tyn;

        if  (!ty)
                return  0;

        tyn = atoi(ty);
        if  (tyn <= 0)
                return  0;

        for  (cnode = nd->children;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;
                if  (strcmp(nname, "iscrit"))
                        cnd->bjc_iscrit = CCRIT_NORUN;
                else  if  (strcmp(nname, "vname") == 0)  {
                        vhash_t vh = load_varind(cnode, BTM_READ);
                        if  (vh < 0)
                                return  0;
                        cnd->bjc_varind = vh;
                }
                else  if  (strcmp(nname, "value")  == 0)
                        if  (!load_value(cnode, &cnd->bjc_value))
                                return  0;
        }
        cnd->bjc_compar = tyn;
        return  1;
}

static  int     load_conds(xmlNodePtr nd, JcondRef clist)
{
        xmlNodePtr  cnode;
        unsigned  ccount = 0;

        for  (cnode = nd->children;  cnode  &&  ccount < MAXCVARS;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, "cond") == 0)  {
                        if  (!load_cond(cnode, &clist[ccount]))
                                return  0;
                        ccount++;
                }
        return  1;
}

static  int     load_ass(xmlNodePtr nd, JassRef ass)
{
        xmlNodePtr  cnode;
        const  char     *ty = CCHARP xmlGetProp(nd, CXMLCH "type");
        int     tyn;

        if  (!ty)
                return  0;

        tyn = atoi(ty);
        if  (tyn <= 0)
                return  0;

        for  (cnode = nd->children;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;
                if  (strcmp(nname, "iscrit"))
                        ass->bja_iscrit = ACRIT_NORUN;
                else  if  (strcmp(nname, "vname") == 0)  {
                        vhash_t vh = load_varind(cnode, BTM_WRITE);
                        if  (vh < 0)
                                return  0;
                        ass->bja_varind = vh;
                }
                else  if  (strcmp(nname, "flags") == 0)
                        ass->bja_flags = load_xml_int(cnode);
                else  if  (strcmp(nname, "const")  == 0)
                        if  (!load_value(cnode, &ass->bja_con))
                                return  0;
        }
        ass->bja_op = tyn;
        return  1;
}

static  int     load_asses(xmlNodePtr nd, JassRef alist)
{
        xmlNodePtr  cnode;
        unsigned  acount = 0;

        for  (cnode = nd->children;  cnode  &&  acount < MAXSEVARS;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, "ass") == 0)  {
                        if  (!load_ass(cnode, &alist[acount]))
                                return  0;
                        acount++;
                }
        return  1;
}

static  const   char    **load_args(xmlNodePtr parent, unsigned *na)
{
        xmlNodePtr cnode;
        unsigned  cnt = 0, np = 0;
        const   char    **result;

        for  (cnode = parent->children;  cnode;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, "arg") == 0)
                        cnt++;

        if  (cnt == 0)
                return  (const char **) 0;

        /* Note this isn't a "deep" malloc we save pointers to bits of the XML tree */

        if  (!(result = malloc(cnt * sizeof(const char *))))
                ABORT_NOMEM;

        for  (cnode = parent->children;  cnode;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, "arg") == 0)
                        result[np++] = CCHARP cnode->children->content;

        *na = cnt;
        return  result;
}

static  MenvirRef       load_envs(xmlNodePtr parent, unsigned *ne)
{
        xmlNodePtr cnode;
        unsigned  cnt = 0, np = 0;
        MenvirRef       result;

        for  (cnode = parent->children;  cnode;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, "env") == 0)
                        cnt++;
        if  (cnt == 0)
                return  (MenvirRef) 0;

        if  (!(result = malloc(cnt * sizeof(Menvir))))
                ABORT_NOMEM;

        for  (cnode = parent->children;  cnode;  cnode = cnode->next)  {
                MenvirRef  ce = &result[np];
                xmlNodePtr  gnode;

                if  (strcmp(CCHARP cnode->name, "env") != 0)
                        continue;

                ce->e_name = ce->e_value = (char *) 0;

                for  (gnode = cnode->children;  gnode;  gnode = gnode->next)  {
                        const char *nname = CCHARP gnode->name;
                        if  (strcmp(nname, "name") == 0)  {
                                if  (gnode->children)
                                        ce->e_name = (char *) gnode->children->content;
                        }
                        else  if  (strcmp(nname, "value") == 0)  {
                                if  (gnode->children)
                                        ce->e_value = (char *)  gnode->children->content;
                                else
                                        ce->e_value = "";
                        }
                }
                if  (!ce->e_name  ||  !ce->e_value)  {
                        cnt--;  /* One less than we thought */
                        continue;
                }
                np++;
        }

        *ne = cnt;
        return  result;
}

static  MredirRef  load_redirs(xmlNodePtr parent, unsigned *nr)
{
        xmlNodePtr cnode;
        unsigned  cnt = 0, np = 0;
        MredirRef       result;

        for  (cnode = parent->children;  cnode;  cnode = cnode->next)
                if  (strcmp(CCHARP cnode->name, "redir") == 0)
                        cnt++;
        if  (cnt == 0)
                return  (MredirRef) 0;

        if  (!(result = malloc(cnt * sizeof(Mredir))))
                ABORT_NOMEM;

        for  (cnode = parent->children;  cnode;  cnode = cnode->next)  {
                MredirRef  cr = &result[np];
                xmlNodePtr  gnode;
                const  char  *ty;
                int     tyn;

                if  (strcmp(CCHARP cnode->name, "redir") != 0)
                        continue;

                ty = CCHARP xmlGetProp(cnode, CXMLCH "type");
                if  (!ty  ||  (tyn = atoi(ty)) <= 0)  {
                        cnt--;
                        continue;
                }

                cr->action = tyn;

                for  (gnode = cnode->children;  gnode;  gnode = gnode->next)  {
                        const char *nname = CCHARP gnode->name;
                        if  (strcmp(nname, "fd") == 0)
                                cr->fd = load_xml_int(gnode);
                        else  if  (strcmp(nname, "file") == 0)
                                cr->un.buffer = (char *) gnode->children->content;
                        else  if  (strcmp(nname, "fd2") == 0)
                                cr->un.arg = load_xml_int(gnode);
                }
                np++;
        }

        *nr = cnt;
        return  result;
}

static  int     load_jobdescr(xmlNodePtr dnode, BtjobRef jp)
{
        xmlNodePtr  cnode;
        BtjobhRef jh = &jp->h;
        const  char     *direct = CCHARP 0, *title = CCHARP 0;
        const   char    **arglist = (const char **) 0;
        MenvirRef envlist = (MenvirRef) 0;
        MredirRef redirlist = (MredirRef) 0;
        unsigned        nargs = 0, nenvs = 0, nredirs = 0;
        int     ret;

        for  (cnode = dnode->children;  cnode;  cnode = cnode->next)  {
                const  char  *nname = CCHARP cnode->name;

                switch  (nname[0])  {
                case  'j':
                        if  (strcmp(nname, "jobnum") == 0)
                                load_jobnum(cnode, jh);
                        else  if  (strcmp(nname, "jflags") == 0)
                                jh->bj_jflags = load_xml_int(cnode);
                        else  if  (strcmp(nname, "jmode") == 0)
                                load_mode(cnode, &jh->bj_mode);
                        continue;
                case  'p':
                        if  (strcmp(nname, "progress") == 0)
                                jh->bj_progress = load_xml_int(cnode);
                        else  if  (strcmp(nname, "pri") == 0)
                                jh->bj_pri = load_xml_int(cnode);
                        continue;
                case  'l':
                        if  (strcmp(nname, "ll") == 0)
                                jh->bj_ll = load_xml_int(cnode);
                        continue;
                case  'u':
                        if  (strcmp(nname, "umask") == 0)
                                jh->bj_umask = load_xml_int(cnode);
                        else  if  (strcmp(nname, "ulimit") == 0)
                                jh->bj_ulimit = load_xml_int(cnode);
                        continue;
                case  's':
                        if  (strcmp(nname, "subtime") == 0)
                                jh->bj_time = load_xml_int(cnode);
                        continue;
                case  'o':
                        if  (strcmp(nname, "orighost") == 0)
                                jh->bj_orighostid = load_hostid(cnode);
                        continue;
                case  'r':
                        if  (strcmp(nname, "redirs") == 0)
                                redirlist = load_redirs(cnode, &nredirs);
                        else  if  (strcmp(nname, "runtime") == 0)
                                jh->bj_runtime = load_xml_int(cnode);
                        else  if  (strcmp(nname, "runon") == 0)
                                jh->bj_runon = load_xml_int(cnode);
                        continue;
                case  'a':
                        if  (strcmp(nname, "asses") == 0)  {
                                if  (!load_asses(cnode, jh->bj_asses))
                                        return  XML_INVALID_ASSES;
                        }
                        else  if  (strcmp(nname, "args") == 0)
                                arglist = load_args(cnode, &nargs);
                        else  if  (strcmp(nname, "autoksig") == 0)
                                jh->bj_autoksig = load_xml_int(cnode);
                        continue;
                case  'd':
                        if  (strcmp(nname, "direct") == 0)
                                direct = CCHARP cnode->children->content;
                        else  if  (strcmp(nname, "deltime") == 0)
                                jh->bj_deltime = load_xml_int(cnode);
                        continue;
                case  'c':
                        if  (strcmp(nname, "cmdinterp") == 0)
                                strncpy(jh->bj_cmdinterp, CCHARP cnode->children->content, CI_MAXNAME);
                        else  if  (strcmp(nname, "conds") == 0)  {
                                if  (!load_conds(cnode, jh->bj_conds))
                                        return  XML_INVALID_CONDS;
                        }
                        continue;
                case  't':
                        if  (strcmp(nname, "times") == 0)
                                load_times(cnode, &jh->bj_times);
                        else  if  (strcmp(nname, "title") == 0)
                                title = CCHARP cnode->children->content;
                        continue;
                case  'n':
                        if  (strcmp(nname, "nexit") == 0)
                                load_exits(cnode, &jh->bj_exits.nlower, &jh->bj_exits.nupper);
                        continue;
                case  'e':
                        if  (strcmp(nname, "envs") == 0)
                                envlist = load_envs(cnode, &nenvs);
                        else  if  (strcmp(nname, "eexit") == 0)
                                load_exits(cnode, &jh->bj_exits.elower, &jh->bj_exits.eupper);
                        continue;
                }
        }

        /* Set up numbers of args, envs, redirs */

        jh->bj_nargs = nargs;
        jh->bj_nenv = nenvs;
        jh->bj_nredirs = nredirs;

        ret = packjstring(jp, direct, title, redirlist, envlist, (char **) arglist);
        if  (redirlist)
                free((char *) redirlist);
        if  (envlist)
                free((char *) envlist);
        if  (arglist)
                free((char *) arglist);
        return  ret == 0? XML_TOOMANYSTRINGS: 0;
}

static  void    load_jobscript(xmlNodePtr snode, char **scriptp)
{
        *scriptp = stracpy(CCHARP snode->children->content);
}

static  void    load_options(xmlNodePtr onode, int *verb)
{
        *verb = load_xml_bool(onode, "verbose");
}

static  int     parserest(xmlDocPtr doc, BtjobRef jp, char **scriptp, int *verb)
{
        xmlNodePtr      root, nnode;
        int     ret;
        *verb = 0;
        BLOCK_ZERO(jp, sizeof(Btjob));
        jp->h.bj_slotno = -1;

        root = xmlDocGetRootElement(doc);
        for  (nnode = root->children;  nnode;  nnode = nnode->next)  {
                const char *nname = CCHARP nnode->name;
                if  (strcmp(nname, "jobdescr") == 0)  {
                        if  ((ret = load_jobdescr(nnode, jp)) != 0)
                                return  ret;
                }
                else  if  (strcmp(nname, "script") == 0)
                        load_jobscript(nnode, scriptp);
                else  if  (strcmp(nname, "qopts") == 0)
                        load_options(nnode, verb);
        }
        xmlFreeDoc(doc);
        return  0;
}

int     load_job_xml(const char *filename, BtjobRef jp, char **scriptp, int *verb)
{
        xmlDocPtr       doc = xmlReadFile(filename, NULL, XML_PARSE_NOBLANKS);
        if  (!doc)
                return  XML_INVALID_FORMAT_FILE;
        return  parserest(doc, jp, scriptp, verb);
}

int     load_job_xml_fd(const int fd, BtjobRef jp, char **scriptp, int *verb)
{
        xmlDocPtr       doc = xmlReadFd(fd, "jobspec.xml", NULL, XML_PARSE_NOBLANKS);
        if  (!doc)
                return  XML_INVALID_FORMAT_FILE;
        return  parserest(doc, jp, scriptp, verb);
}
#else
int     load_job_xml(const char *filename, BtjobRef jp, char **scriptp, int *verb)
{
        return  XML_NOTIMPL;
}

int     load_job_xml_fd(const int fd, BtjobRef jp, char **scriptp, int *verb)
{
        return  XML_NOTIMPL;
}
#endif
