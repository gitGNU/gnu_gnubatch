/* xmlsave.c -- save jobs to XML files.

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
#include <libxml/xmlwriter.h>
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
#include "xmlldsv.h"

#ifdef  HAVE_LIBXML2

#define  CXMLCH  (const xmlChar *)

static  void    save_xml_intprop(xmlNodePtr node, const char *name, const long value)
{
        char    nbuf[30];
        sprintf(nbuf, "%ld", value);
        xmlSetProp(node, CXMLCH name, CXMLCH nbuf);
}

static  void    save_xml_int(xmlNodePtr parent, const char *name, const long value)
{
        char    nbuf[30];
        sprintf(nbuf, "%ld", value);
        xmlNewTextChild(parent, NULL, CXMLCH name, CXMLCH nbuf);
}

static  void    save_xml_bool(xmlNodePtr parent, const char *name, const int value)
{
        if  (value)
                xmlNewChild(parent, NULL, CXMLCH name, NULL);
}

static  void    save_xml_hostid(xmlNodePtr parent, const char *name, const netid_t host)
{
        if  (host)
                xmlNewTextChild(parent, NULL, CXMLCH name, CXMLCH look_host(host));
}

static void     save_xml_jobnum(xmlNodePtr parent, const char *name, CBtjobhRef jp)
{
        xmlNodePtr jnn = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        save_xml_hostid(jnn, "host", jp->bj_hostid);
        save_xml_int(jnn, "num", jp->bj_job);
}

static  void    save_xml_varname(xmlNodePtr parent, const char *name, CBtvarRef vp)
{
        xmlNodePtr  vnn = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        save_xml_hostid(vnn, "host", vp->var_id.hostid);
        xmlNewTextChild(vnn, NULL, CXMLCH "name", CXMLCH vp->var_name);
}

static  void    save_xml_varind(xmlNodePtr parent, const char *name, const vhash_t varind)
{
        save_xml_varname(parent, name, &Var_seg.vlist[varind].Vent);
}

static  void    save_xml_value(xmlNodePtr parent, const char *name, CBtconRef value)
{
        xmlNodePtr  vnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        if  (value->const_type == CON_STRING)
                xmlNewTextChild(vnode, NULL, CXMLCH "textval", CXMLCH value->con_un.con_string);
        else
                save_xml_int(vnode, "intval", value->con_un.con_long);
}

static  void    save_xml_times(xmlNodePtr parent, const char *name, CTimeconRef jtimes)
{
        xmlNodePtr  tnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);

        xmlSetProp(tnode, CXMLCH "timeset", jtimes->tc_istime? CXMLCH "y": CXMLCH "n");
        save_xml_int(tnode, "nexttime", jtimes->tc_nexttime);
        save_xml_int(tnode, "repeat", jtimes->tc_repeat);
        save_xml_int(tnode, "rate", jtimes->tc_rate);
        save_xml_int(tnode, "mday", jtimes->tc_mday);
        save_xml_int(tnode, "nvaldays", jtimes->tc_nvaldays);
        save_xml_int(tnode, "nposs", jtimes->tc_nposs);
}

static  void    save_xml_mode(xmlNodePtr parent, const char *name, CBtmodeRef modep)
{
        xmlNodePtr  mnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);

        save_xml_int(mnode, "uperm", modep->u_flags);
        save_xml_int(mnode, "gperm", modep->g_flags);
        save_xml_int(mnode, "operm", modep->o_flags);
        if  (modep->o_user[0])
                xmlNewTextChild(mnode, NULL, CXMLCH "user", CXMLCH modep->o_user);
        if  (modep->o_group[0])
                xmlNewTextChild(mnode, NULL, CXMLCH "group", CXMLCH modep->o_group);
}

static  void    save_xml_exits(xmlNodePtr parent, const char *name, const int lower, const int upper)
{
        xmlNodePtr  enode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        save_xml_int(enode, "l", lower);
        save_xml_int(enode, "u", upper);
}

static  void    save_xml_conds(xmlNodePtr parent, const char *name, CJcondRef clist)
{
        xmlNodePtr  cnode;
        int     nconds = 0, cnt;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                if  (clist[cnt].bjc_compar == C_UNUSED)
                        break;
                nconds++;
        }

        if  (nconds == 0)
                return;

        cnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                xmlNodePtr  cn;
                CJcondRef  jc = &clist[cnt];
                if  (jc->bjc_compar == C_UNUSED)
                        break;
                cn = xmlNewChild(cnode, NULL, CXMLCH "cond", NULL);
                save_xml_intprop(cn, "type", jc->bjc_compar);
                save_xml_bool(cn, "iscrit", jc->bjc_iscrit);
                save_xml_varind(cn, "vname", jc->bjc_varind);
                save_xml_value(cn, "value", &jc->bjc_value);
        }
}

static  void    save_xml_asses(xmlNodePtr parent, const char *name, CJassRef alist)
{
        xmlNodePtr  anode;
        int  nasses = 0, cnt;

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                if  (alist[cnt].bja_op == BJA_NONE)
                        break;
                nasses++;
        }

        if  (nasses == 0)
                return;

        anode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                xmlNodePtr  an;
                CJassRef  ja = &alist[cnt];
                if  (ja->bja_op == BJA_NONE)
                        break;
                an = xmlNewChild(anode, NULL, CXMLCH "ass", NULL);
                save_xml_intprop(an, "type", ja->bja_op);
                save_xml_bool(an, "iscrit", ja->bja_iscrit);
                save_xml_varind(an, "vname", ja->bja_varind);
                if  (ja->bja_op < BJA_SEXIT)  {
                        save_xml_value(an, "const", &ja->bja_con);
                        save_xml_int(an, "flags", ja->bja_flags);
                }
        }
}

static  void    save_xml_args(xmlNodePtr parent, const char *name, CBtjobRef job)
{
        xmlNodePtr  anode;
        unsigned  cnt;

        if  (job->h.bj_nargs == 0)
                return;

        anode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < job->h.bj_nargs;  cnt++)
                xmlNewTextChild(anode, NULL, CXMLCH "arg", CXMLCH ARG_OF(job, cnt));
}

static  void    save_xml_envs(xmlNodePtr parent, const char *name, CBtjobRef job)
{
        xmlNodePtr  enode;
        unsigned  cnt;

        if  (job->h.bj_nenv == 0)
                return;

        enode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < job->h.bj_nenv;  cnt++)  {
                char  *en, *ev;
                xmlNodePtr  nd;
                ENV_OF(job, cnt, en, ev);
                if  (strlen(en) == 0)
                        continue;
                nd = xmlNewChild(enode, NULL, CXMLCH "env", NULL);
                xmlNewTextChild(nd, NULL, CXMLCH "name", CXMLCH en);
                xmlNewTextChild(nd, NULL, CXMLCH "value", CXMLCH ev);
        }
}

static  void    save_xml_redirs(xmlNodePtr parent, const char *name, CBtjobRef job)
{
        xmlNodePtr  rnode;
        unsigned  cnt;

        if  (job->h.bj_nredirs == 0)
                return;

        rnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < job->h.bj_nredirs;  cnt++)  {
                RedirRef  rp = REDIR_OF(job, cnt);
                xmlNodePtr  nd = xmlNewChild(rnode, NULL, CXMLCH "redir", NULL);
                save_xml_intprop(nd, "type", rp->action);
                save_xml_int(nd, "fd", rp->fd);
                if  (rp->action < RD_ACT_CLOSE)
                        xmlNewTextChild(nd, NULL, CXMLCH "file", CXMLCH &job->bj_space[rp->arg]);
                else  if  (rp->action == RD_ACT_DUP)
                        save_xml_int(nd, "fd2", rp->arg);
        }
}

static  xmlDocPtr  save_job_xml_int(CBtjobRef job, const char *scriptstr, const unsigned scriptlen, const int progress, const int verbose)
{
        CBtjobhRef      jp = &job->h;
        xmlDocPtr       doc;
        xmlNodePtr      root, dnode;

        doc = xmlNewDoc(CXMLCH "1.0");
        root = xmlNewNode(NULL, CXMLCH "Job");
        xmlDocSetRootElement(doc, root);
        xmlNewNs(root, CXMLCH NSID, NULL);
        dnode = xmlNewChild(root, NULL, CXMLCH "jobdescr", NULL);
        /* Save job number for reference if it is defined */
        if  (jp->bj_job != 0)
                save_xml_jobnum(dnode, "jobnum", jp);
        save_xml_int(dnode, "progress", progress >= 0? progress: jp->bj_progress);
        save_xml_int(dnode, "pri", jp->bj_pri);
        save_xml_int(dnode, "ll", jp->bj_ll);
        save_xml_int(dnode, "umask", jp->bj_umask);
        if  (jp->bj_ulimit != 0)
                save_xml_int(dnode, "ulimit", jp->bj_ulimit);
        save_xml_int(dnode, "jflags", jp->bj_jflags);
        save_xml_int(dnode, "subtime", jp->bj_time);
        save_xml_hostid(dnode, "orighost", jp->bj_orighostid);
        if  (jp->bj_runtime != 0)  {
                save_xml_int(dnode, "runtime", jp->bj_runtime);
                if  (jp->bj_autoksig != 0  &&  jp->bj_runon != 0)  {
                        save_xml_int(dnode, "autoksig", jp->bj_autoksig);
                        save_xml_int(dnode, "runon", jp->bj_runon);
                }
        }
        if  (jp->bj_deltime != 0)
                save_xml_int(dnode, "deltime", jp->bj_deltime);
        xmlNewTextChild(dnode, NULL, CXMLCH "cmdinterp", CXMLCH jp->bj_cmdinterp);
        if  (jp->bj_title >= 0)
                xmlNewTextChild(dnode, NULL, CXMLCH "title", CXMLCH title_of(job));
        if  (jp->bj_direct >= 0)
                xmlNewTextChild(dnode, NULL, CXMLCH "direct", CXMLCH &job->bj_space[jp->bj_direct]);
        save_xml_times(dnode, "times", &jp->bj_times);
        save_xml_mode(dnode, "jmode", &jp->bj_mode);
        save_xml_exits(dnode, "nexit", jp->bj_exits.nlower, jp->bj_exits.nupper);
        save_xml_exits(dnode, "eexit", jp->bj_exits.elower, jp->bj_exits.eupper);
        save_xml_conds(dnode, "conds", jp->bj_conds);
        save_xml_asses(dnode, "asses", jp->bj_asses);
        save_xml_args(dnode, "args", job);
        save_xml_envs(dnode, "envs", job);
        save_xml_redirs(dnode, "redirs", job);

        xmlAddChild(xmlNewChild(root, NULL, CXMLCH "script", NULL), xmlNewCDataBlock(doc, CXMLCH scriptstr, scriptlen));

        if  (verbose)
                save_xml_bool(xmlNewChild(root, NULL, CXMLCH "qopts", NULL), "verbose", verbose);
        return  doc;
}

int     save_job_xml(CBtjobRef job, const char *scriptstr, const unsigned scriptlen, const char *outfile, const int progress, const int verbose)
{
        xmlDocPtr  doc = save_job_xml_int(job, scriptstr, scriptlen, progress, verbose);
        int     ret = xmlSaveFormatFile(outfile, doc, 1);
        int     serror = errno;
        xmlFreeDoc(doc);
        if  (ret >= 0)
                return  0;
        return  serror;
}

int     save_job_xml_file(CBtjobRef job, const char *scriptstr, const unsigned scriptlen, FILE *outfile, const int progress, const int verbose)
{
        xmlDocPtr  doc = save_job_xml_int(job, scriptstr, scriptlen, progress, verbose);
        xmlChar *outc;
        int     len, ret, serror;
        xmlDocDumpFormatMemory(doc, &outc, &len, 1);
        xmlFreeDoc(doc);
        ret = fwrite(outc, sizeof(char), len, outfile);
        serror = errno;
        xmlFree(outc);
        if  (ret == len)
                return  0;
        return  serror;
}
#else
int     save_job_xml(CBtjobRef job, const char *scriptstr, const unsigned scriptlen, const char *outfile, const int progress, const int verbose)
{
        return  XML_NOTIMPL;
}

int     save_job_xml_file(CBtjobRef job, const char *scriptstr, const unsigned scriptlen, FILE *outfile, const int progress, const int verbose)
{
        return  XML_NOTIMPL;
}
#endif
