/* xmbr_jlist.c -- job list handling for gbch-xmr

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
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#ifdef  TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif   defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include "incl_unix.h"
#include "incl_sig.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"
#include "helpalt.h"
#include "files.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "q_shm.h"
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbr_ext.h"
#include "optflags.h"
#include "shutilmsg.h"
#include "stringvec.h"

static  char    Filename[] = __FILE__;

#define NAMESIZE        14      /* Padding for temp file */

extern  USHORT  Save_umask;

static  char    *tmpfl;

unsigned                pend_njobs, pend_max;
struct  pend_job        *pend_list;

HelpaltRef      daynames_full, monnames;
#ifdef HAVE_XM_SPINB_H
XmStringTable   timezerof, stdaynames, stmonnames;
#endif

char    *no_name;

int     longest_day,
        longest_mon;

ULONG   default_rate;
unsigned char   default_interval,
                default_nposs;
USHORT          defavoid, def_assflags;

#define ppermitted(flg) (mypriv->btu_priv & flg)

char            *ccritmark,
                *scritmark;

static  HelpaltRef      progresslist;
extern  HelpaltRef      assnames, actnames, stdinnames;
extern  int             redir_actlen, redir_stdinlen;

#ifndef PATH_MAX
#define PATH_MAX        2048
#endif

extern  char    *getscript_file(FILE *, unsigned *);

static  int     has_xml_suffix(const char *name)
{
        int  lng = strlen(name);

        return  lng >= sizeof(XMLJOBSUFFIX)  &&  ncstrcmp(name + lng - sizeof(XMLJOBSUFFIX) + 1, XMLJOBSUFFIX) == 0;
}

int  jlist_dirty()
{
        int     cnt;

        for  (cnt = 0;  cnt < pend_njobs;  cnt++)
                if  (pend_list[cnt].changes != 0)
                        return  1;
        return  0;
}

/* Generate list of queue names in ql */

void  gen_qlist_sv(struct stringvec *ql)
{
        int     jcnt;

        rjobfile(1);

        stringvec_init(ql);
        stringvec_append(ql, "");

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                const   char    *tit = title_of(jj_ptrs[jcnt]);
                const   char    *ip;
                char    *it;
                unsigned        lng;
                if  (!(ip = strchr(tit, ':')))
                        continue;
                lng = ip - tit + 1;
                if  (!(it = malloc(lng)))
                        ABORT_NOMEM;
                strncpy(it, tit, lng-1);
                it[lng-1] = '\0';
                stringvec_insert_unique(ql, it);
                free(it);
        }

        for  (jcnt = 0;  jcnt < pend_njobs;  jcnt++)
                if  (pend_list[jcnt].jobqueue)
                        stringvec_insert_unique(ql, pend_list[jcnt].jobqueue);
}

/* Allocate various messages.  */

static  struct  flag_ctrl       {
        int     defcode;                /* Code to look up */
        SHORT   defval;                 /* Default if unreadable */
        USHORT  flag;           /* Flag involved */
}  flagctrl[] = {
        { $N{Assignment start flag help}, 1, BJA_START },
        { $N{Assignment reverse flag help}, 1, BJA_REVERSE },
        { $N{Assignment exit flag help}, 1, BJA_OK },
        { $N{Assignment error flag help}, 1, BJA_ERROR },
        { $N{Assignment abort flag help}, 1, BJA_ABORT },
        { $N{Assignment cancel flag help}, 0, BJA_CANCEL }
};

void  initmoremsgs()
{
        int     cnt, n;

        if  (!(tmpfl = malloc((unsigned)(strlen(spdir) + 2 + NAMESIZE))))
                ABORT_NOMEM;

        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
                print_error($E{Missing alternative code});
        }
        if  (!(assnames = helprdalt($Q{Assignment names})))  {
                disp_arg[9] = $Q{Assignment names};
                print_error($E{Missing alternative code});
        }
        ccritmark = gprompt($P{Critical condition marker});
        scritmark = gprompt($P{Assignment critical});
        def_assflags = 0;
        for  (cnt = 0;  cnt < XtNumber(flagctrl);  cnt++)  {
                int     nn = helpnstate(flagctrl[cnt].defcode);
                if  ((nn < 0  &&  flagctrl[cnt].defval) ||  nn > 0)
                        def_assflags |= flagctrl[cnt].flag;
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        no_name = gprompt($P{xbtr no file name});
        if  (!(actnames = helprdalt($Q{Redirection type names})))  {
                disp_arg[9] = $Q{Redirection type names};
                print_error($E{Missing alternative code});
        }
        else
                redir_actlen = altlen(actnames);
        if  (!(stdinnames = helprdalt($Q{stdio file names})))  {
                disp_arg[9] = $Q{stdio file names};
                print_error($E{Missing alternative code});
        }
        else
                redir_stdinlen = altlen(stdinnames);
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                print_error($E{Missing alternative code});
        }
        if  ((daynames_full = helprdalt($Q{Weekdays full})))
                longest_day = altlen(daynames_full);
        if  ((monnames = helprdalt($Q{Months full})))
                longest_mon = altlen(monnames);
        if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
                disp_arg[9] = $Q{Repeat unit abbrev};
                print_error($E{Missing alternative code});
        }
        if  ((n = helpnstate($N{Default repeat alternative})) < 0  ||  n > TC_YEARS)
                n = TC_RETAIN;
        default_interval = (unsigned char) n;
        if  ((n = helpnstate($N{Default number of units})) <= 0)
                n = 10;
        default_rate = (ULONG) n;
        if  ((n = helpnstate($N{Default skip delay option})) < 0  ||  n > TC_CATCHUP)
                n = TC_WAIT1;
        default_nposs = (unsigned char) n;
        defavoid = 0;
        for  (n = 0;  n < TC_NDAYS;  n++)
                if  (helpnstate($N{Base for days to avoid}+n) > 0)
                        defavoid |= 1 << n;
#ifdef HAVE_XM_SPINB_H

        /* Set up the string tables for time setting now
           Timezerof is done as strings to give us nice zero filled hh:mm etc */

        if  (!(timezerof = (XmStringTable) XtMalloc((unsigned) (60 * sizeof(XmString)))))
                ABORT_NOMEM;
        if  (!(stdaynames = (XmStringTable) XtMalloc((unsigned) (7 * sizeof(XmString)))))
                ABORT_NOMEM;
        if  (!(stmonnames = (XmStringTable) XtMalloc((unsigned) (12 * sizeof(XmString)))))
                ABORT_NOMEM;

        for  (n = 0;  n < 60;  n++)  {
                char    buf[3];
                sprintf(buf, "%.2d", n);
                timezerof[n] = XmStringCreateLocalized(buf);
        }
        for  (n = 0;  n < 7;  n++)
                stdaynames[n] = XmStringCreateLocalized(disp_alt(n, daynames_full));
        for  (n = 0;  n < 12;  n++)
                stmonnames[n] = XmStringCreateLocalized(disp_alt(n, monnames));
#endif /* HAVE_XM_SPINB_H */
}

/* Display a job from pending list */

XmString  jdisplay(struct pend_job *pj)
{
        int     bpos = 0;
        const   char    *ip;
        char    *cp, resbuf[PATH_MAX];

        /* Progress, then title, tabs between */
        cp = resbuf;
        ip = disp_alt(pj->job->h.bj_progress, progresslist);
        while  (*ip)  {
                *cp++ = *ip++;
                bpos++;
        }
        do  {
                *cp++ = ' ';
                bpos++;
        }  while  (bpos & 7);
        ip = title_of(pj->job);
        while  (*ip)  {
                *cp++ = *ip++;
                bpos++;
        }
        do  {
                *cp++ = ' ';
                bpos++;
        }  while  (bpos & 7);

        /* Now diverge a bit according to whether we're using single file or not.
           Render XML job files as without the suffix and legacy ones as cmdfile (jobfile) */

        if  ((ip = pj->xml_jobfile_name))  {
                int  lng = strlen((const char *) pj->xml_jobfile_name);
                if  (has_xml_suffix((const char *) pj->xml_jobfile_name))
                        lng -= sizeof(XMLJOBSUFFIX) - 1;
                while  (lng > 0)  {
                        *cp++ = *ip++;
                        bpos++;
                        lng--;
                }
        }
        else  if  ((ip = pj->cmdfile_name))  {          /* Have cmdfile name */
                while  (*ip)  {
                        *cp++ = *ip++;
                        bpos++;
                }
                *cp++ = ' ';
                *cp++ = '(';
                bpos += 2;
                if  (!(ip = pj->jobfile_name))
                        ip = no_name;
                while  (*ip)  {
                        *cp++ = *ip++;
                        bpos++;
                }
                *cp++ = ')';
                bpos++;
        }
        else  {                                 /* Just put in no name, we can't have just jobfile name any more */
                ip = no_name;
                while  (*ip)  {
                        *cp++ = *ip++;
                        bpos++;
                }
        }

        /* Space out and put in directory */

        do  {
                *cp++ = ' ';
                bpos++;
        }  while  (bpos & 7);
        ip = pj->directory;
        while  (*ip)  {
                *cp++ = *ip++;
                bpos++;
        }
        *cp = '\0';

        return  XmStringCreateSimple(resbuf);
}

int  getselectedjob(const int moan)
{
        int     *plist, pcnt;

        if  (XmListGetSelectedPos(jwid, &plist, &pcnt)  &&  pcnt > 0)  {
                int  result = plist[0] - 1;
                XtFree((char *) plist);
                return  result;
        }
        if  (moan)
                doerror(jwid, pend_njobs != 0? $EH{xmbtq no job selected}: $EH{xmbtq no jobs to select});
        return  -1;
}

struct pend_job *job_or_deflt(const int isjob)
{
        if  (isjob)  {
                int     indx;
                if  ((indx = getselectedjob(1)) < 0)
                        return  (struct pend_job *) 0;
                return  &pend_list[indx];
        }
        return  &default_pend;
}

#define INIT_PEND       20
#define INC_PEND        5

static  int     movedown_select()
{
        int     indx, cnt;

        if  (pend_njobs >= pend_max)  {
                if  (pend_max == 0)  {
                        pend_max = INIT_PEND;
                        pend_list = (struct pend_job *) malloc(INIT_PEND * sizeof(struct pend_job));
                }
                else  {
                        pend_max += INC_PEND;
                        pend_list = (struct pend_job *) realloc((char *) pend_list, INC_PEND * sizeof(struct pend_job));
                }
                if  (!pend_list)
                        ABORT_NOMEM;
        }


        if  ((indx = getselectedjob(0)) < 0)
                return  pend_njobs;

        for  (cnt = pend_njobs;  cnt > (unsigned) indx;  cnt--)
                pend_list[cnt] = pend_list[cnt-1];

        return  indx;
}

static  void    redisp_onopen(const int indx)
{
        int     cnt;
        XmString  str;

        for  (cnt = indx;  cnt < pend_njobs;  cnt++)  {
                str = jdisplay(&pend_list[cnt]);
                XmListReplaceItemsPos(jwid, &str, 1, cnt+1);
                XmStringFree(str);
        }
        str = jdisplay(&pend_list[pend_njobs]);
        XmListAddItem(jwid, str, 0);
        XmStringFree(str);
        XmListSelectPos(jwid, indx+1, False);
        pend_njobs++;
}

void  cb_jnew(Widget w, int notused)
{
        int     indx = movedown_select();
        job_initialise(&pend_list[indx], stracpy(Curr_pwd), (char *) 0);
        redisp_onopen(indx);
}

static int  get_dir_file(char **dname, char **fname, XmFileSelectionBoxCallbackStruct *cbs)
{
        char    *filepath, *dirpath = (char *) 0, *dirname, *filename, *sp;

        if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &filepath) || !filepath)
                return  0;

        if  (filepath[0] == '\0')  {
                XtFree(filepath);
                return  0;
        }

        sp = strrchr(filepath, '/');
        dirname = filepath;

        if  (sp)  {                             /* Not just a filename */
                if  (sp == filepath)            /* I.e. in root directory */
                        dirname = "/";
                else
                        *sp = '\0';
                filename = sp + 1;
                if  (filename[0] == '\0')  {    /* Just a directory, no file */
                        XtFree(filepath);
                        return  0;
                }
        }
        else  {
                int     nn;

                /* No slash, need to get directory separately */

                filename = filepath;
                if  (!XmStringGetLtoR(cbs->dir, XmSTRING_DEFAULT_CHARSET, &dirpath) || !dirpath)  {
                        XtFree(filepath);
                        return  0;
                }
                if  (dirpath[0] != '/')  {
                        XtFree(dirpath);
                        XtFree(filepath);
                        return  0;
                }

                /* Trim off trailing / unless it's root dir */

                nn = strlen(dirpath) - 1;
                if  (nn > 0  &&  dirpath[nn] == '/')
                        dirpath[nn] = '\0';

                dirname = dirpath;
        }

        *dname = stracpy(dirname);
        *fname = stracpy(filename);
        if  (dirpath)
                XtFree(dirpath);
        XtFree(filepath);
        return  1;
}

char *gen_path(char *dir, char *fil)
{
        char    *result;
        int     ld = strlen(dir);
        if  (!(result = malloc((unsigned) (ld + strlen(fil) + 2))))
                ABORT_NOMEM;
        if  (dir[ld-1] == '/')          /* NB assume at least one char */
                sprintf(result, "%s%s", dir, fil);
        else
                sprintf(result, "%s/%s", dir, fil);
        return  result;
}

static void  endjopen(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        char    *dirname, *filename;

restart:

        if  (data  &&  get_dir_file(&dirname, &filename, cbs))  {
                int     cnt, ret, indx;
                struct  pend_job  *pj;

                for  (cnt = 0;  cnt < pend_njobs;  cnt++)  {
                        pj = &pend_list[cnt];
                        if  (pj->directory  &&  strcmp(dirname, pj->directory) == 0  &&
                             pj->xml_jobfile_name  &&  strcmp(filename, pj->xml_jobfile_name) == 0)  {
                                doerror(jwid, $EH{xmbtr file already loaded});
                                free(dirname);
                                free(filename);
                                goto  restart;
                        }
                }

                indx = movedown_select();
                pj = &pend_list[indx];
                job_initialise(pj, dirname, (char *) 0);
                pj->xml_jobfile_name = filename;

                /* Carry on even if we have an error.  We may as well include the thing.  */

                if  ((ret = xml_job_load(pj)))
                        doerror(jwid, ret);

                redisp_onopen(indx);
        }
        XtDestroyWidget(w);
}

void  cb_jopen(Widget w, int notused)
{
        Widget  fsb;
        XmString        str;
        str = XmStringCreateSimple(Curr_pwd);
        fsb = XmCreateFileSelectionDialog(FindWidget(w), "openj", NULL, 0);
        XtVaSetValues(fsb,
                      XmNfileSearchProc,        isit_xmlfile,
                      XmNdirectory,             str,
                      NULL);
        XmStringFree(str);
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) endjopen, (XtPointer) 0);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endjopen, (XtPointer) 1);
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtr open dialog help});
        XtManageChild(fsb);
}

static void  endjlegopen(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        char    *dname, *fname;

        if  (data  &&  get_dir_file(&dname, &fname, cbs))  {
                int     cnt, ret, indx;

                for  (cnt = 0;  cnt < pend_njobs;  cnt++)  {
                        struct  pend_job  *pj = &pend_list[cnt];

                        if  (pj->directory  &&  strcmp(dname, pj->directory) == 0  &&
                             pj->cmdfile_name  &&  strcmp(fname, pj->cmdfile_name) == 0)  {
                                doerror(jwid, $EH{xmbtr file already loaded});
                                free(dname);
                                free(fname);
                                return;
                        }
                }

                indx = movedown_select();
                job_initialise(&pend_list[indx], dname, fname);

                /* Carry on even if we have an error.  We may as well include the thing.  */

                if  ((ret = job_load(&pend_list[indx])))
                        doerror(jwid, ret);

                redisp_onopen(indx);
        }
        XtDestroyWidget(w);
}

void  cb_jlegopen(Widget w, int notused)
{
        Widget  fsb;
        XmString        str;
        str = XmStringCreateSimple(Curr_pwd);
        fsb = XmCreateFileSelectionDialog(FindWidget(w), "openj", NULL, 0);
        XtVaSetValues(fsb,
                      XmNfileSearchProc,        isit_cmdfile,
                      XmNdirectory,             str,
                      NULL);
        XmStringFree(str);
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) endjlegopen, (XtPointer) 0);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endjlegopen, (XtPointer) 1);
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtr open dialog help});
        XtManageChild(fsb);
}

void  cb_jclosedel(Widget w, int isdel)
{
        int     indx;
        unsigned        cnt;
        struct  pend_job        *pj;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];
        if  (pj->changes > 0  &&  !Confirm(w, $PH{xmbtr confirm close}))
                return;

        if  (isdel)  {
                if  (!Confirm(w, $PH{xmbtr confirm delete}))
                        return;
                jobfile_delete(pj, pj->jobfile_name, $EH{xmbtr cannot delete job file});
                jobfile_delete(pj, pj->cmdfile_name, $EH{xmbtr cannot delete cmd file});
                jobfile_delete(pj, pj->xml_jobfile_name, $EH{xmbtr cannot delete job file});
        }

        XmListDeletePos(jwid, indx+1);

        free((char *) pj->job);
        free(pj->directory);
        if  (pj->jobfile_name)
                free(pj->jobfile_name);
        if  (pj->cmdfile_name)
                free(pj->cmdfile_name);
        if  (pj->xml_jobfile_name)
                free(pj->xml_jobfile_name);
        if  (pj->jobscript)
                free(pj->jobscript);
        if  (pj->jobqueue)
                free(pj->jobqueue);
        pend_njobs--;
        for  (cnt = indx;  cnt < pend_njobs;  cnt++)
                pend_list[cnt] = pend_list[cnt+1];
}

struct  legfileres  {
        int     result_found;
        char    *jobf_dir, *jobf_file;
        char    *cmdf_dir, *cmdf_file;
};

static  void    fileselcanccb(Widget w, struct legfileres *data, XmFileSelectionBoxCallbackStruct *cbs)
{
        data->result_found = 0;
        XtDestroyWidget(w);
}

static void  endgetlegjobfile(Widget w, struct legfileres *data, XmFileSelectionBoxCallbackStruct *cbs)
{
        data->result_found = get_dir_file(&data->jobf_dir, &data->jobf_file, cbs);
        XtDestroyWidget(w);
}

static void  endgetlegcmdfile(Widget w, struct legfileres *data, XmFileSelectionBoxCallbackStruct *cbs)
{
        data->result_found = get_dir_file(&data->cmdf_dir, &data->cmdf_file, cbs);
        XtDestroyWidget(w);
}

static  void    initfsbdirname(Widget fsb, struct pend_job *pj, char *fname)
{
        XmString  str;
        if  (fname)  {
                char  *fpath = gen_path(pj->directory, fname);
                str = XmStringCreateLocalized(fpath);
                free(fpath);
                XtVaSetValues(fsb, XmNdirSpec, str, NULL);
        }
        else  {
                char    *d = pj->directory;
                if  (!d)
                        d = Curr_pwd;
                str = XmStringCreateLocalized(d);
                XtVaSetValues(fsb, XmNdirectory, str, NULL);
        }
        XmStringFree(str);
}

/* Set up legacy-style file name returning 1 if OK 0 if nothing done */

static  int     getlegfilenames(Widget w, struct pend_job *pj)
{
        Widget  fsb;
        struct  legfileres  answers;
        int     preflen = 1;
        char    *dir;

        fsb = XmCreateFileSelectionDialog(FindWidget(w), "jobfile", NULL, 0);
        initfsbdirname(fsb, pj, pj->jobfile_name);

        answers.result_found = -1;
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) fileselcanccb, (XtPointer) &answers);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endgetlegjobfile, (XtPointer) &answers);
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER($H{xmbtr job file dialog help}));
        XtManageChild(fsb);
        while  (answers.result_found < 0)
                XtAppProcessEvent(app, XtIMAll);
        if  (answers.result_found == 0)
                return  0;

        /* Repeat all that for the command file */

        fsb = XmCreateFileSelectionDialog(FindWidget(w), "cmdfile", NULL, 0);
        initfsbdirname(fsb, pj, pj->cmdfile_name);
        answers.result_found = -1;
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) fileselcanccb, (XtPointer) &answers);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endgetlegcmdfile, (XtPointer) &answers);
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER($H{xmbtr cmd file dialog help}));
        XtManageChild(fsb);
        while  (answers.result_found < 0)
                XtAppProcessEvent(app, XtIMAll);

        /* If that failed, undo what we did */

        if  (answers.result_found == 0)  {
                free(answers.jobf_dir);
                free(answers.jobf_file);
                return  0;
        }

        /* Check we haven't got the same file */

        if  (strcmp(answers.jobf_dir, answers.cmdf_dir) == 0  &&  strcmp(answers.jobf_file, answers.cmdf_file) == 0)  {
                free(answers.jobf_dir);
                free(answers.jobf_file);
                free(answers.cmdf_dir);
                free(answers.cmdf_file);
                doerror(w, $EH{xbtr job cmd file names same});
                return  0;
        }

        /* Free up existing ones */
        free(pj->directory);
        if  (pj->jobfile_name)
                free(pj->jobfile_name);
        if  (pj->cmdfile_name)
                free(pj->cmdfile_name);

        /* If it's the same directory for each, all is easy */

        if  (strcmp(answers.jobf_dir, answers.cmdf_dir) == 0)  {
                pj->directory = answers.jobf_dir;
                free(answers.cmdf_dir);
                pj->jobfile_name = answers.jobf_file;
                pj->cmdfile_name = answers.cmdf_file;
                return  1;
        }

        /* Try to find the longest common prefix (it can't be the whole lot we just checked) */

        for  (;;)  {
                char    *csp, *jsp;
                int     csl, jsl;
                if  (!(csp = strchr(answers.cmdf_dir + preflen, '/')))
                        break;
                if  (!(jsp = strchr(answers.jobf_dir + preflen, '/')))
                        break;
                csl = (csp - answers.cmdf_dir) - preflen;
                jsl = (jsp - answers.jobf_dir) - preflen;
                if  (csl != jsl)
                        break;
                if  (strncmp(answers.cmdf_dir + preflen, answers.jobf_dir + preflen, csl) != 0)
                        break;
                preflen += csl + 1;
        }

        if  (preflen == 1)
                dir = "/";
        else  {
                answers.cmdf_dir[preflen-1] = '\0';
                dir = answers.cmdf_dir;
        }

        pj->jobfile_name = gen_path(answers.jobf_dir+preflen, answers.jobf_file);
        pj->cmdfile_name = gen_path(answers.cmdf_dir+preflen, answers.cmdf_file);
        pj->directory = stracpy(dir);
        free(answers.jobf_dir);
        free(answers.jobf_file);
        free(answers.cmdf_dir);
        free(answers.cmdf_file);
        return  1;
}

static void  endgetxmlfile(Widget w, struct     legfileres *data, XmFileSelectionBoxCallbackStruct *cbs)
{
        data->result_found = get_dir_file(&data->jobf_dir, &data->jobf_file, cbs);
        XtDestroyWidget(w);
        if  (!has_xml_suffix(data->jobf_file))  {
                char  *rfile = malloc((unsigned) (strlen(data->jobf_file) + sizeof(XMLJOBSUFFIX)));
                if  (!rfile)
                        ABORT_NOMEM;
                strcpy(rfile, data->jobf_file);
                strcat(rfile, XMLJOBSUFFIX);
                free(data->jobf_file);
                data->jobf_file = rfile;
        }
}

/* Get save file name for XML files, returning 1 if OK 0 if unchanged */

static  int     getxmlfilename(Widget w, struct pend_job *pj)
{
        Widget  fsb;
        XmString  str;
        struct  legfileres  answers;

        fsb = XmCreateFileSelectionDialog(FindWidget(w), "jobfile", NULL, 0);
        initfsbdirname(fsb, pj, pj->xml_jobfile_name);
        str = XmStringCreateLocalized("*" XMLJOBSUFFIX);
        XtVaSetValues(fsb, XmNpattern, str, NULL);
        XmStringFree(str);
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) fileselcanccb, (XtPointer) &answers);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endgetxmlfile, (XtPointer) &answers);
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER($H{xmbtr job file dialog help}));
        XtManageChild(fsb);
        answers.result_found = -1;
        while  (answers.result_found < 0)
                XtAppProcessEvent(app, XtIMAll);
        if  (answers.result_found == 0)
                return  0;
        free(pj->directory);
        if  (pj->xml_jobfile_name)
                free(pj->xml_jobfile_name);
        pj->directory = answers.jobf_dir;
        pj->xml_jobfile_name = answers.jobf_file;
        return  1;
}

/* Check to see if we're converting from legacy format job files
   to XML and do the conversions required. */

static  int     check_to_xml(Widget w, struct pend_job *pj)
{
        char    *fpath;
        FILE    *inf;
        unsigned  fsize;

        /* If we've got a script, there is nothing to do */

        if  (pj->jobscript)
                return  1;

        /* We shouldn't get in here without a jobfile name so we assume it's there */

        fpath = gen_path(pj->directory, pj->jobfile_name);
        SWAP_TO(Realuid);
        inf = fopen(fpath, "r");
        SWAP_TO(Daemuid);
        free(fpath);

        if  (!inf)  {
                doerror(w, $EH{xmbtr cannot open job file});
                return  0;
        }

        /* Load up the job script file. */

        pj->jobscript = getscript_file(inf, &fsize);
        fclose(inf);

        /* If we didn't read anything from the file but didn't actually have an error, allocate a null string */

        if  (!pj->jobscript)
                pj->jobscript = stracpy("");

        /* Ask about deleting the original job files */

        if  (Confirm(w, $PH{xbtr delete legacy job files}))  {
                jobfile_delete(pj, pj->jobfile_name, $EH{xmbtr cannot delete job file});
                jobfile_delete(pj, pj->cmdfile_name, $EH{xmbtr cannot delete cmd file});
        }
        free(pj->jobfile_name);
        pj->jobfile_name = (char *) 0;
        if  (pj->cmdfile_name)  {
                free(pj->cmdfile_name);
                pj->cmdfile_name = (char *) 0;
        }
        pj->scriptinmem = 1;
        return  1;
}

/* Check to see if we're converting from XML format to legacy format */

static  int     check_from_xml(Widget w, struct pend_job *pj)
{
        char    *fpath;
        FILE    *outf;
        unsigned  len, nb;
        int     ret;

        /* If we didn't actually have a script, we are all OK */

        if  (!pj->jobscript)
                return  1;

        fpath = gen_path(pj->directory, pj->jobfile_name);
        SWAP_TO(Realuid);
        outf = fopen(fpath, "w");
        SWAP_TO(Daemuid);
        free(fpath);
        if  (!outf)  {
                doerror(w, $EH{xbq cannot write job file});
                return  0;
        }

        /* Write out script to job file.
           Script might be zero length. */

        nb = len = strlen(pj->jobscript);
        if  (len > 0)
                nb = fwrite(pj->jobscript, sizeof(char), len, outf);

        /* Might detect error writing or on closing */

        ret = fclose(outf);
        if  (len != nb  ||  ret < 0)  {
                doerror(w, $EH{xbq cannot write job file});
                return  0;
        }

        /* Possibly delete XML file, but deallocate name and pointer */

        if  (pj->xml_jobfile_name)  {
                if  (Confirm(w, $PH{xbtr delete XML job file}))
                        jobfile_delete(pj, pj->xml_jobfile_name, $EH{xmbtr cannot delete job file});
                free(pj->xml_jobfile_name);
                pj->xml_jobfile_name = (char *) 0;
        }
        free(pj->jobscript);
        pj->jobscript = (char *) 0;
        pj->scriptinmem = 0;
        return  1;
}

void    cb_jsaveas(Widget w, int notused)
{
        int     indx, ret;
        struct  pend_job  *pj;
        XmString        str;

        if  ((indx = getselectedjob(1)) < 0)
                return;

        pj = &pend_list[indx];

        if  (pj->changes == 0  &&  !Confirm(w, $PH{xmbtr file save confirm}))
                return;

        /* Need a script before we can save */

        if  (!pj->jobscript  &&  !pj->jobfile_name)  {
                 doerror(w, $EH{xbtr no script});
                 return;
        }

        /* Get the file name or names to be used */

        if  (xml_format)
                ret = getxmlfilename(w, pj);
        else
                ret = getlegfilenames(w, pj);

        if  (ret == 0)
                return;

        str = jdisplay(pj);
        XmListReplaceItemsPos(jwid, &str, 1, indx+1);
        XmStringFree(str);
        XmListSelectPos(jwid, indx+1, False);

        if  (xml_format)  {
                if  (!(check_to_xml(w, pj)))
                        return;
                ret = job_save_xml(pj);
        }
        else  {
                if  (!(check_from_xml(w, pj)))
                        return;
                ret = job_save(pj);
        }

        if  (ret)
                doerror(w, ret);
        else
                pj->changes = 0;

        str = jdisplay(pj);
        XmListReplaceItemsPos(jwid, &str, 1, indx+1);
        XmStringFree(str);
        XmListSelectPos(jwid, indx+1, False);
}

void  cb_jsave(Widget w, int notused)
{
        int     indx, ret;
        struct  pend_job        *pj;
        XmString        str;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];

        /* Jump into save as if we haven't got the relevant file names.
           This also does the check we've got the script. */

        if  (xml_format)  {
                if  (!pj->xml_jobfile_name)  {
                        cb_jsaveas(w, notused);
                        return;
                }
        }
        else  if  (!pj->cmdfile_name  ||  !pj->jobfile_name)  {
                cb_jsaveas(w, notused);
                return;
        }

        /* Query if not changed since last save */

        if  (pj->changes <= 0  &&  !Confirm(w, $PH{xmbtr file save confirm}))
                return;

        if  (xml_format)  {
                if  (!(check_to_xml(w, pj)))
                        return;
                ret = job_save_xml(pj);
        }
        else  {
                if  (!(check_from_xml(w, pj)))
                        return;
                ret = job_save(pj);
        }

        if  (ret)
                doerror(w, ret);
        else
                pj->changes = 0;

        str = jdisplay(pj);
        XmListReplaceItemsPos(jwid, &str, 1, indx+1);
        XmStringFree(str);
        XmListSelectPos(jwid, indx+1, False);
}

void  displaybusy(const int on)
{
        static  Cursor  cursor;
        XSetWindowAttributes    attrs;
        if  (!cursor)
                cursor = XCreateFontCursor(dpy, XC_watch);
        attrs.cursor = on? cursor: None;
        XChangeWindowAttributes(dpy, XtWindow(toplevel), CWCursor, &attrs);
        XFlush(dpy);
}

/* Run external editor.
   fname is the file to be edited. */

static  int     run_external_editor(Widget w, char *fname)
{
        PIDTYPE pid;

        if  ((pid = fork()) != 0)  {            /* Main path */
                int     status;

                if  (pid < 0)  {
                        doerror(w, $EH{xmbtr cannot fork for editor});
                        return  0;
                }

                displaybusy(1);         /* Try to display busy cursor sometimes works */

#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                displaybusy(0);         /* Undisplay cursor */

                if  (status != 0)  {
                        disp_str = editor_name;
                        doerror(w, $EH{xmbtr cannot execute editor});
                        return  0;
                }

                return  1;
        }

        /* Go back to invoking environment */

        umask((int) Save_umask);
        setuid(Realuid);

        if  (xterm_edit)  {
                char    *termname = envprocess("${XTERM:-xterm}");
                execlp(termname, editor_name, "-e", editor_name, fname, (char *) 0);
        }
        else
                execlp(editor_name, editor_name, fname, (char *) 0);
        _exit(255);
}

static  void    inmem_edit_external_editor(Widget w, struct pend_job *pj)
{
        char    *tmp_file = tempnam((const char *) 0, "XMBTR");
        FILE    *tfl;
        int     um = umask(0);

        /* Open temp file for writing - should be world writable */

        tfl = fopen(tmp_file, "w+");
        umask(um);

        if  (!tfl)  {
                doerror(w, $EH{xmbtr cannot create temp file});
                free(tempnam);
                return;
        }

        /* If we have an existing script, write it out */

        if  (pj->jobscript)  {
                int  len = strlen(pj->jobscript);
                if  (len > 0)  {
                        fwrite(pj->jobscript, sizeof(char), len, tfl);
                        fflush(tfl);
                }
        }

        if  (run_external_editor(w, tmp_file))  {       /* If OK replace script */
                char    *newsc;
                unsigned        fsize = 0;
                rewind(tfl);
                newsc = getscript_file(tfl, &fsize);
                if  (pj->jobscript)
                        free(pj->jobscript);
                if  (!newsc)  /* Didn't actually read anything */
                        newsc = stracpy("");
                pj->jobscript = newsc;
        }

        fclose(tfl);
        unlink(tmp_file);
        free(tmp_file);
}

static  void    file_edit_external_editor(Widget w, struct pend_job *pj)
{
        char    *path = gen_path(pj->directory, pj->jobfile_name);

        if  (!f_exists(path))  {
                free(path);
                doerror(w, $EH{xmbtr no such file});
                return;
        }

        run_external_editor(w, path);
        free(path);
}

/* Editing - note that we don't bother with Internal editing in the Motif case, we prefer
   to use an external editor.
   It is possible to do it but I'm not sure if it is worth the trouble as we move to toolkits
   other than Motif. */

void  cb_edit(Widget w, int notused)
{
        int     indx;
        struct  pend_job        *pj;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];

        if  (pj->jobscript || !pj->jobfile_name)
                /* Already got in-memory script or no script. */
                inmem_edit_external_editor(w, pj);
        else  {
                if  (!pj->directory)  {
                        doerror(w, $EH{xmbtr no job file name});
                        return;
                }
                file_edit_external_editor(w, pj);
        }
 }

static void  enddsel(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        if  (data)  {
                char    *dirname;
                int     nn;
                if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &dirname) || !dirname)
                        return;
                if  (dirname[0] == '\0')  {
                        XtFree(dirname);
                        return;
                }
                nn = strlen(dirname) - 1;
                if  (dirname[0] != '/'  ||  dirname[nn] != '/')  {
                        disp_str = dirname;
                        doerror(w, $EH{xmbtr invalid directory});
                        XtFree(dirname);
                        return;
                }
                if  (nn > 0  &&  dirname[nn] == '/')
                        dirname[nn] = '\0';
                if  (strcmp(dirname, Curr_pwd) != 0)  {
                        free(Curr_pwd);
                        Curr_pwd = stracpy(dirname);
                }
                XtFree(dirname);
        }
        XtDestroyWidget(w);
}

void  cb_direct(Widget w, int notused)
{
        Widget  fsb;
        XmString        str;

        fsb = XmCreateFileSelectionDialog(FindWidget(w), "seldir", NULL, 0);
        str = XmStringCreateSimple(Curr_pwd);
        XtVaSetValues(fsb, XmNdirectory, str, NULL);
        XmStringFree(str);
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) enddsel, (XtPointer) 0);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) enddsel, (XtPointer) 1);
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtr select dir dialog help});
        XtManageChild(fsb);
}

/* Generate output file name */

static FILE *goutfile(jobno_t *jn)
{
        FILE    *res;
        int     fid;

        for  (;;)  {
                sprintf(tmpfl, "%s/%s", spdir, mkspid(SPNAM, *jn));
                if  ((fid = open(tmpfl, O_WRONLY|O_CREAT|O_EXCL, 0666)) >= 0)
                        break;
                *jn += JN_INC;
        }
        if  ((res = fdopen(fid, "w")) == (FILE *) 0)  {
                unlink(tmpfl);
                ABORT_NOMEM;
        }
        return  res;
}

/* Use a static location for the job number which we increment each time */

static  jobno_t         jn;

static  int     submit_core(Widget w, struct pend_job *pj)
{
        BtjobRef                jreq;
        ULONG                   jindx;
        Shipc                   Oreq;
        Repmess                 rr;
        extern  long            mymtype;

        jreq = &Xbuffer->Ring[jindx = getxbuf()];
        BLOCK_COPY(jreq, pj->job, sizeof(Btjob));
        jreq->h.bj_slotno = -1;
        jreq->h.bj_job = jn;
        time(&jreq->h.bj_time);

        Oreq.sh_mtype = TO_SCHED;
        Oreq.sh_params.mcode = J_CREATE;
        Oreq.sh_params.uuid = pj->userid;
        Oreq.sh_params.ugid = pj->grpid;
        Oreq.sh_params.hostid = 0;
        Oreq.sh_params.param = 0;
        Oreq.sh_un.sh_jobindex = jindx;
        Oreq.sh_params.upid = getpid();
#ifdef  USING_MMAP
        sync_xfermmap();
#endif
        if  (msgsnd(Ctrl_chan, (struct msgbuf *) &Oreq, sizeof(Shreq) + sizeof(ULONG), 0) < 0)  {
                int     savee = errno;
                unlink(tmpfl);
                freexbuf(jindx);
                errno = savee;
                doerror(w, savee == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
                return  0;
        }

        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
                if  (errno == EINTR)
                        continue;
                freexbuf(jindx);
                doerror(w, $E{Error on IPC});
                return  0;
        }

        freexbuf(jindx);

        if  (rr.outmsg.mcode != J_OK) {
                unlink(tmpfl);
                if  ((rr.outmsg.mcode & REQ_TYPE) != JOB_REPLY)  { /* Not expecting net errors */
                        disp_arg[0] = rr.outmsg.mcode;
                        doerror(w, $EH{Unexpected sched message});
                }
                else  {
                        disp_str = title_of(pj->job);
                        doerror(w, (int) ((rr.outmsg.mcode & ~REQ_TYPE) + $EH{Base for scheduler job errors}));
                }
                return  0;
        }

        return  1;
}

/* Check currently-selected job OK to submit */

struct  pend_job        *sub_check(Widget w)
{
        int     indx;
        struct  pend_job  *pj;

        if  ((indx = getselectedjob(1)) < 0)
                return  (struct pend_job *) 0;

        /* Protest if job unchanged since last time */

        pj = &pend_list[indx];
        if  (pj->nosubmit == 0  &&  !Confirm(w, $PH{xmbtr job unchanged confirm}))
                return  (struct pend_job *) 0;

        /* Check in future */

        if  (pj->job->h.bj_times.tc_istime  &&  pj->job->h.bj_times.tc_nexttime < time((time_t *) 0))  {
                doerror(w, $EH{xmbtr cannot submit not future});
                return  (struct pend_job *) 0;
        }

        if  (!pj->jobscript  &&  !pj->jobfile_name)  {
                doerror(w, $EH{xmbtr no job file name});
                return  (struct pend_job *) 0;
        }

        return  pj;
}

void  cb_submit(Widget w, int notused)
{
        struct  pend_job        *pj;
        FILE                    *outf;

        if  (!(pj = sub_check(w)))
                return;

        jn = jn? jn+1: getpid();

        if  (pj->jobscript)  {
                outf = goutfile(&jn);
                unsigned  len = strlen(pj->jobscript);
                unsigned  nb = 0;
                if  (len != 0)
                        nb = fwrite(pj->jobscript, sizeof(char), len, outf);
                if  (nb != len)  {
                        fclose(outf);
                        unlink(tmpfl);
                        doerror(w, $EH{No room for job});
                        return;
                }
        }
        else  {
                FILE    *inf;
                char    *path;
                unsigned  inlng, outlng;
                char    inbuf[1024];

                /* We checked jobfile_name was set in sub_check */

                path = gen_path(pj->directory, pj->jobfile_name);
                SWAP_TO(Realuid);
                inf = fopen(path, "r");
                SWAP_TO(Daemuid);
                free(path);

                if  (!inf)  {
                        doerror(w, $EH{xmbtr cannot open job file});
                        return;
                }

                outf = goutfile(&jn);

                while  ((inlng = fread(inbuf, sizeof(char), sizeof(inbuf), inf)) != 0)  {
                        outlng = fwrite(inbuf, sizeof(char), inlng, outf);
                        if  (outlng != inlng)  {
                                fclose(inf);
                                fclose(outf);
                                unlink(tmpfl);
                                doerror(w, $EH{No room for job});
                                return;
                        }
                }

                fclose(inf);
        }

        if  (fclose(outf) != 0)  {
                unlink(tmpfl);
                doerror(w, $EH{No room for job});
                return;
        }

        if  (!submit_core(w, pj))
                 return;

        if  (pj->Verbose)  {
                 disp_arg[0] = jn;
                 disp_str = title_of(pj->job);
                 doinfo(w, disp_str[0]? $E{xmbtr job created ok title}: $E{xmbtr job created ok no title});
        }

        pj->nosubmit = 0;
}
