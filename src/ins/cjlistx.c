/* cjlistx.c -- dump out job list as XML files

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
#include "ecodes.h"
#include "defaults.h"
#include "files.h"
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
#ifdef  HAVE_LIBXML2
#include <libxml/xmlwriter.h>
#endif
#include <sys/errno.h>
#include <ctype.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "incl_ugid.h"
#include "network.h"
#include "btmode.h"
#include "btconst.h"
#include "timecon.h"
#include "btvar.h"
#include "bjparam.h"
#include "btjob.h"
#include "btuser.h"
#include "jobsave.h"
#include "spitrouts.h"

FILE	*Cfile;				/* Keep linker at bay */
char    *progname;

#ifdef  HAVE_LIBXML2

static  char    Filename[] = __FILE__;

#ifndef PATH_MAX
#define PATH_MAX        1024
#endif

#define  MAXTITLECHARS  120

struct  {
        char    *srcdir;        /* Directory we read from if not pwd */
        char    *srcfile;       /* Source file */
        char    *outdir;        /* Directory we write to */
        char    *outfile;       /* Output file */
        long    errors;         /* Number we've had */
        char    ignusers;       /* Ignore invalid users */
        char    verbose;         /* Verbose output */
        unsigned  short  namefrtit;      /* Name from title - max number of chars */
}  popts;

extern char *expand_srcdir(char *);
extern char *make_absolute(char *);
extern char *getscript_file(FILE *, unsigned *);
#endif

void    nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

#ifdef  HAVE_LIBXML2
/* Check user name in job is valid (unless we have turned off checks */

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

/* Ditto group name */

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

/* Check that the job number is OK and gives a valid job file */

static int jobok(const jobno_t jobnum)
{
        char    *nam;
        struct  stat    sbuf;

        if  (jobnum == 0)
                return  0;

        /* May have to tack directory name in front */

        nam = mkspid(SPNAM, jobnum);
        if  (popts.srcdir)  {
                char    path[PATH_MAX];
                sprintf(path, "%s/%s", popts.srcdir, nam);
                if  (stat(path, &sbuf) < 0)
                        return  0;
        }
        else  if  (stat(nam, &sbuf) < 0)
                return  0;
        if  ((sbuf.st_mode & S_IFMT) != S_IFREG)
                return  0;
        return  1;
}

/* Check that variable name looks sensible */

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
        if  (!gnameok(old->sj_mode.o_group, old->sj_mode.o_gid))
                return  0;

        /* Check conditions look sensible */

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

        /* Ditto assignments */

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                if  (old->sj_asses[cnt].sja_op == BJA_NONE)
                        break;
                if  (old->sj_asses[cnt].sja_op > BJA_SSIG)
                        return  0;
                if  (old->sj_asses[cnt].sja_crit > ACRIT_NORUN)
                        return  0;
                if  ((old->sj_asses[cnt].sja_flags & ~(BJA_START|BJA_OK|BJA_ERROR|BJA_ABORT|BJA_CANCEL|BJA_REVERSE)) != 0)
                        return  0;
                if  (!validvar(&old->sj_asses[cnt].sja_varind))
                        return  0;
                if  (old->sj_asses[cnt].sja_con.const_type <= CON_NONE || old->sj_asses[cnt].sja_con.const_type > CON_STRING)
                        return  0;
        }

        /* Return OK */

        return  1;
}

/* Buffer to put output job file name in */

static  char    titlebuf[MAXTITLECHARS + 20 + sizeof(XMLJOBSUFFIX)];

/* Try to make job name out of title */

static  char    *make_fname_from_title(struct Jsave *jb)
{
        int     ch, cnt;
        char    *tch;
        char    tchars[MAXTITLECHARS + 1];

        /* If not doing them from title or no title, forget it */

        if  (popts.namefrtit == 0  ||  jb->sj_title < 0)
                return  (char *) 0;

        cnt = 0;
        tch = &jb->sj_space[jb->sj_title];

        do  {
                ch = *tch++;
                if  (ch == 0)
                        break;
                if  (!isalnum(ch)  &&  ch != '.'  &&  ch != '_')  {
                        if  (!isspace(ch))
                                continue;
                        ch = '_';
                }
                tchars[cnt++] = ch;
        }  while  (cnt < (int) popts.namefrtit);

        /* If we didn't manage to make anything, drop out */

        if  (cnt <= 0)
                return  (char *) 0;

        tchars[cnt] = '\0';

        /* First try without any suffixes */

        strcpy(titlebuf, tchars);
        strcat(titlebuf, XMLJOBSUFFIX);
        if  (access(titlebuf, F_OK) < 0)
                return  titlebuf;

        /* Keep on increasing suffix until we find one that doesn't exist */

        cnt = 1;
        for  (;;)  {
                sprintf(titlebuf, "%s_%.3d" XMLJOBSUFFIX, tchars, cnt);
                if  (access(titlebuf, F_OK) < 0)
                        return  titlebuf;
                cnt++;
        }
}

/* Case where we make a file name from job number */

static  char    *make_fname_from_jnum(struct Jsave *jb)
{
        int     cnt;

        sprintf(titlebuf, "J%.6ld" XMLJOBSUFFIX, (long) jb->sj_job);
        if  (access(titlebuf, F_OK) < 0)
                return  titlebuf;
        cnt = 1;
        for  (;;)  {
                sprintf(titlebuf, "J%.6ld_%.3d" XMLJOBSUFFIX, (long) jb->sj_job, cnt);
                if  (access(titlebuf, F_OK) < 0)
                        return  titlebuf;
                cnt++;
        }
}

/* Generate a file name from the title or failing that, the job number */

static  char    *make_outfname(struct Jsave *jb)
{
        char    *ret = make_fname_from_title(jb);
        if  (ret)
                return  ret;
        return  make_fname_from_jnum(jb);
}

/* Cut down version of host lookup */

static const char *get_host(netid_t nid)
{
        struct  hostent *dbhost;
        if  ((dbhost = gethostbyaddr((char *) &nid, sizeof(nid), AF_INET)))
                return  dbhost->h_name;
        return  "unknown";
}

/* XML routines */

#define  CXMLCH  (const xmlChar *)

/* Save integer as attribute on node */

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
                xmlNewTextChild(parent, NULL, CXMLCH name, CXMLCH get_host(host));
}

/* This is a cut down version of saving a job number as we only do it for local files */

static void     save_xml_jobnum(xmlNodePtr parent, const char *name,  struct Jsave *jp)
{
        xmlNodePtr jnn = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        save_xml_int(jnn, "num", jp->sj_job);
}

static  void    save_xml_varname(xmlNodePtr parent, const char *name, Vref *vp)
{
        xmlNodePtr  vnn = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        save_xml_hostid(vnn, "host", vp->sv_hostid);
        xmlNewTextChild(vnn, NULL, CXMLCH "name", CXMLCH vp->sv_name);
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

static  void    save_xml_conds(xmlNodePtr parent, const char *name, struct Sjcond *clist)
{
        xmlNodePtr  cnode;
        int     nconds = 0, cnt;

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                if  (clist[cnt].sjc_compar == C_UNUSED)
                        break;
                nconds++;
        }

        if  (nconds == 0)
                return;

        cnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                xmlNodePtr  cn;
                struct Sjcond *jc = &clist[cnt];
                if  (jc->sjc_compar == C_UNUSED)
                        break;
                cn = xmlNewChild(cnode, NULL, CXMLCH "cond", NULL);
                save_xml_intprop(cn, "type", jc->sjc_compar);
                save_xml_bool(cn, "iscrit", jc->sjc_crit);
                save_xml_varname(cn, "vname", &jc->sjc_varind);
                save_xml_value(cn, "value", &jc->sjc_value);
        }
}

static  void    save_xml_asses(xmlNodePtr parent, const char *name, struct  Sjass *alist)
{
        xmlNodePtr  anode;
        int  nasses = 0, cnt;

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                if  (alist[cnt].sja_op == BJA_NONE)
                        break;
                nasses++;
        }

        if  (nasses == 0)
                return;

        anode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                xmlNodePtr  an;
                struct  Sjass  *ja = &alist[cnt];
                if  (ja->sja_op == BJA_NONE)
                        break;
                an = xmlNewChild(anode, NULL, CXMLCH "ass", NULL);
                save_xml_intprop(an, "type", ja->sja_op);
                save_xml_bool(an, "iscrit", ja->sja_crit);
                save_xml_varname(an, "vname", &ja->sja_varind);
                if  (ja->sja_op < BJA_SEXIT)  {
                        save_xml_value(an, "const", &ja->sja_con);
                        save_xml_int(an, "flags", ja->sja_flags);
                }
        }
}

static  void    save_xml_args(xmlNodePtr parent, const char *name, struct Jsave *job)
{
        xmlNodePtr  anode;
        unsigned  cnt;
        const Jarg  *ap;

        if  (job->sj_nargs == 0)
                return;

        ap = (const Jarg *) &job->sj_space[job->sj_arg];
        anode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < job->sj_nargs;  cnt++)
                xmlNewTextChild(anode, NULL, CXMLCH "arg", CXMLCH &job->sj_space[ap[cnt]]);
}

static  void    save_xml_envs(xmlNodePtr parent, const char *name, struct Jsave *job)
{
        xmlNodePtr  enode;
        unsigned  cnt;
        const   Envir   *el;

        if  (job->sj_nenv == 0)
                return;

        el = (const Envir *) &job->sj_space[job->sj_env];
        enode = xmlNewChild(parent, NULL, CXMLCH name, NULL);
        for  (cnt = 0;  cnt < job->sj_nenv;  cnt++, el++)  {
                char  *en = &job->sj_space[el->e_name];
                char  *ev = &job->sj_space[el->e_value];
                xmlNodePtr  nd;
                if  (strlen(en) == 0)
                        continue;
                nd = xmlNewChild(enode, NULL, CXMLCH "env", NULL);
                xmlNewTextChild(nd, NULL, CXMLCH "name", CXMLCH en);
                xmlNewTextChild(nd, NULL, CXMLCH "value", CXMLCH ev);
        }
}

static  void    save_xml_redirs(xmlNodePtr parent, const char *name, struct Jsave *job)
{
        xmlNodePtr  rnode;
        unsigned  cnt;
        const   Redir   *rp;

        if  (job->sj_nredirs == 0)
                return;

        rp = (const Redir *) &job->sj_space[job->sj_redirs];
        rnode = xmlNewChild(parent, NULL, CXMLCH name, NULL);

        for  (cnt = 0;  cnt < job->sj_nredirs;  cnt++, rp++)  {
                xmlNodePtr  nd = xmlNewChild(rnode, NULL, CXMLCH "redir", NULL);
                save_xml_intprop(nd, "type", rp->action);
                save_xml_int(nd, "fd", rp->fd);
                if  (rp->action < RD_ACT_CLOSE)
                        xmlNewTextChild(nd, NULL, CXMLCH "file", CXMLCH &job->sj_space[rp->arg]);
                else  if  (rp->action == RD_ACT_DUP)
                        save_xml_int(nd, "fd2", rp->arg);
        }
}

static  int     save_as_xml(char *outfname, struct Jsave *jb, char *scriptstring, const unsigned scriptlen)
{
        xmlDocPtr       doc;
        xmlNodePtr      root, dnode;
        int     ret, serror;

        doc = xmlNewDoc(CXMLCH "1.0");
        root = xmlNewNode(NULL, CXMLCH "Job");
        xmlDocSetRootElement(doc, root);
        xmlNewNs(root, CXMLCH NSID, NULL);
        dnode = xmlNewChild(root, NULL, CXMLCH "jobdescr", NULL);
        save_xml_jobnum(dnode, "jobnum", jb);
        save_xml_int(dnode, "progress", jb->sj_progress == BJP_NONE || jb->sj_progress == BJP_CANCELLED? jb->sj_progress: BJP_ABORTED);
        save_xml_int(dnode, "pri", jb->sj_pri);
        save_xml_int(dnode, "ll", jb->sj_ll);
        save_xml_int(dnode, "umask", jb->sj_umask);
        if  (jb->sj_ulimit != 0)
                save_xml_int(dnode, "ulimit", jb->sj_ulimit);
        save_xml_int(dnode, "jflags", jb->sj_jflags);
        save_xml_int(dnode, "subtime", jb->sj_time);
        /* Error in no orig host in saved job */
        if  (jb->sj_runtime != 0)  {
                save_xml_int(dnode, "runtime", jb->sj_runtime);
                if  (jb->sj_autoksig != 0  &&  jb->sj_runon != 0)  {
                        save_xml_int(dnode, "autoksig", jb->sj_autoksig);
                        save_xml_int(dnode, "runon", jb->sj_runon);
                }
        }
        if  (jb->sj_deltime != 0)
                save_xml_int(dnode, "deltime", jb->sj_deltime);
        xmlNewTextChild(dnode, NULL, CXMLCH "cmdinterp", CXMLCH jb->sj_cmdinterp);
        if  (jb->sj_title >= 0)
                xmlNewTextChild(dnode, NULL, CXMLCH "title", CXMLCH &jb->sj_space[jb->sj_title]);
        if  (jb->sj_direct >= 0)
                xmlNewTextChild(dnode, NULL, CXMLCH "direct", CXMLCH &jb->sj_space[jb->sj_direct]);
        save_xml_times(dnode, "times", &jb->sj_times);
        save_xml_mode(dnode, "jmode", &jb->sj_mode);
        save_xml_exits(dnode, "nexit", jb->sj_exits.nlower, jb->sj_exits.nupper);
        save_xml_exits(dnode, "eexit", jb->sj_exits.elower, jb->sj_exits.eupper);
        save_xml_conds(dnode, "conds", jb->sj_conds);
        save_xml_asses(dnode, "asses", jb->sj_asses);
        save_xml_args(dnode, "args", jb);
        save_xml_envs(dnode, "envs", jb);
        save_xml_redirs(dnode, "redirs", jb);

        if  (scriptstring)
                xmlAddChild(xmlNewChild(root, NULL, CXMLCH "script", NULL), xmlNewCDataBlock(doc, CXMLCH scriptstring, scriptlen));

        save_xml_bool(xmlNewChild(root, NULL, CXMLCH "qopts", NULL), "verbose", popts.verbose);
        ret = xmlSaveFormatFile(outfname, doc, 1);
        serror = errno;
        xmlFreeDoc(doc);
        if  (ret >= 0)
                return  0;
        return  serror;
}

void  convert_file(const int ifd)
{
        for  (;;)  {
                int     nb, ret;
                FILE    *jfile;
                char    *scriptstring;
                char    *outfname;
                unsigned        scriptlen;
                struct  Jsave   old;
                char    path[PATH_MAX];

                nb = read(ifd, (char *) &old, sizeof(old));
                if  (nb != sizeof(old))  {
                        if  (nb <= 0)
                                return;
                        fprintf(stderr, "Truncated job file expected %lu bytes read %d bytes\n", (unsigned long) sizeof(old), nb);
                        popts.errors++;
                        return;
                }

                if  (!jobfldsok(&old))  {
                        fprintf(stderr, "Damaged format of job number %ld\n", (long) old.sj_job);
                        popts.errors++;
                        continue;
                }

                /* Get spool job script file */

                if  (popts.srcdir)  {
                        sprintf(path, "%s/%s", popts.srcdir, mkspid(SPNAM, old.sj_job));
                        jfile = fopen(path, "r");
                }
                else
                        jfile = fopen(mkspid(SPNAM, old.sj_job), "r");

                if  (!jfile)  {
                        fprintf(stderr, "Cannot find saved job script file for job %ld\n", (long) old.sj_job);
                        popts.errors++;
                        continue;
                }

                scriptstring = getscript_file(jfile, &scriptlen);
                fclose(jfile);

                outfname = make_outfname(&old);
                if  ((ret = save_as_xml(outfname, &old, scriptstring, scriptlen)) != 0)  {
                        fprintf(stderr, "Could not save job %ld to %s\n", (long) old.sj_job, outfname);
                        popts.errors++;
                        continue;
                }
                else  if  (popts.verbose)
                        fprintf(stderr, "Saved job %ld to %s\n", (long) old.sj_job, outfname);

                if  (scriptstring)
                        free(scriptstring);
        }
}

MAINFN_TYPE main(int argc, char **argv)
{
        int             ifd, ch;
        struct  stat    sbuf;
        struct  flock   rlock;
        extern  int     optind;
        extern  char    *optarg;

        versionprint(argv, "$Revision: 1.9 $", 0);
        progname = argv[0];

        while  ((ch = getopt(argc, argv, "vut:D:o:O:f:j:")) != EOF)
                switch  (ch)  {
                default:
                        fprintf(stderr, "Usage: %s [-v] [-u]  [-t n] [-D srcdir] [-o outdir] [-f jobfile]\n", argv[0]);
                        return  100;
                case  'D':
                        popts.srcdir = optarg;
                        continue;
                case  'u':
                        popts.ignusers = 1;
                        continue;
                case  't':
                        popts.namefrtit = atoi(optarg);
                        if  (popts.namefrtit > MAXTITLECHARS)
                                popts.namefrtit = MAXTITLECHARS;
                        continue;
                case  'v':
                        popts.verbose = 1;
                        continue;
                case  'o':
                case  'O':
                        popts.outdir = optarg;
                        continue;
                case  'f':
                case  'j':
                        popts.srcfile = optarg;
                        continue;
                }

        /* Set up source file, using standard name if not specified */

        if  (!popts.srcfile)
                popts.srcfile = JFILE;

        /* If source file was not an absolute path, get directory for it,
           using default if not specified */

        if  (popts.srcfile[0] != '/')  {
                char    *newd;
                if  (!popts.srcdir)
                        popts.srcdir = "SPOOLDIR";
                newd = expand_srcdir(popts.srcdir);
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
        else  if  (!popts.srcdir)  {
                /* If no directory given assume it's the directory given in the path */
                char    *sp = strrchr(popts.srcfile, '/');
                if  (sp)  {
                        *sp = '\0';
                        popts.srcdir = popts.srcfile;
                        popts.srcfile = sp+1;
                }
                else  if  (!popts.outdir)  {
                        fprintf(stderr, "No output directory given and working from current dir\n");
                        return 20;
                }
        }

        /* Change to dest directory if it's specified */

        if  (popts.outdir  &&  chdir(popts.outdir) < 0)  {
                fprintf(stderr, "Cannot select output directory %s\n", popts.outdir);
                return  13;
        }

        if  (popts.srcfile[0] == '/')
                ifd = open(popts.srcfile, O_RDONLY);
        else  {
                char  *path = malloc((unsigned) (strlen(popts.srcdir) + strlen(popts.srcfile) + 2));
                if  (!path)
                        ABORT_NOMEM;
                sprintf(path, "%s/%s", popts.srcdir, popts.srcfile);
                ifd = open(path, O_RDONLY);
                free(path);
        }

        /* Open and lock source job file */

        if  (ifd < 0)  {
                fprintf(stderr, "Sorry cannot open %s\n", popts.srcfile);
                return  2;
        }

        rlock.l_type = F_RDLCK;
        rlock.l_whence = 0;
        rlock.l_start = 0L;
        rlock.l_len = 0L;
        if  (fcntl(ifd, F_SETLKW, &rlock) < 0)  {
                fprintf(stderr, "Sorry could not lock %s\n", popts.srcfile);
                return  3;
        }

        convert_file(ifd);

        if  (popts.errors > 0)
                fprintf(stderr, "There were %ld error%s found\n", popts.errors, popts.errors > 1? "s": "");

        close(ifd);
        fprintf(stderr, "Finished outputting job files\n");
        return  0;
}
#else
MAINFN_TYPE main(int argc, char **argv)
{
	versionprint(argv, "$Revision: 1.9 $", 0);
        fprintf(stderr, "This is not implemented as no XML library was available on build\n");
        return  E_NOTIMPL;
}
#endif

