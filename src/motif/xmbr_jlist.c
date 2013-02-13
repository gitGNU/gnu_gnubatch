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

int     longest_day,
        longest_mon;

ULONG   default_rate;
unsigned char   default_interval,
                default_nposs;
USHORT          defavoid, def_assflags;

#define ppermitted(flg) (mypriv->btu_priv & flg)

char            *ccritmark,
                *scritmark,
                *file_sep;

static  HelpaltRef      progresslist;
extern  HelpaltRef      assnames, actnames, stdinnames;
extern  int             redir_actlen, redir_stdinlen;

#define MALLINIT        10
#define MALLINC         5

char **gen_qlist(char *unused)
{
        int             jcnt, rt;
        unsigned        rcnt, rmax;
        char            **result;
        const   char    *ip;

        rjobfile(1);

        if  ((result = (char **) malloc((unsigned) ((MALLINIT + 1) * sizeof(char *)))) == (char **) 0)
                ABORT_NOMEM;

        result[0] = stracpy("");
        rcnt = 1;
        rmax = MALLINIT;

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                const   char    *tit = title_of(jj_ptrs[jcnt]);
                char    *it;
                unsigned        lng;
                if  (!(ip = strchr(tit, ':')))
                        continue;
                lng = ip - tit + 1;
                if  (!(it = malloc(lng)))
                        ABORT_NOMEM;
                strncpy(it, tit, lng-1);
                it[lng-1] = '\0';
                for  (rt = 0;  rt < rcnt;  rt++)
                        if  (strcmp(it, result[rt]) == 0)  {
                                free(it);
                                goto  clash;
                        }
                if  (rcnt >= rmax)  {
                        rmax += MALLINC;
                        if  ((result = (char **) realloc((char *) result, (rmax+1) * sizeof(char *))) == 0)
                                ABORT_NOMEM;
                }
                result[rcnt++] = it;
        clash:
                ;
        }
        result[rcnt] = (char *) 0;
        return  result;
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
        file_sep = gprompt($P{xmbtr file sep});
        def_assflags = 0;
        for  (cnt = 0;  cnt < XtNumber(flagctrl);  cnt++)  {
                int     nn = helpnstate(flagctrl[cnt].defcode);
                if  ((nn < 0  &&  flagctrl[cnt].defval) ||  nn > 0)
                        def_assflags |= flagctrl[cnt].flag;
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
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
        char    *cp, resbuf[256];

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
        if  ((ip = pj->cmdfile_name))  {
                while  (*ip)  {
                        *cp++ = *ip++;
                        bpos++;
                }
        }
        for  (ip = file_sep; *ip;  ip++)  {
                *cp++ = *ip;
                bpos++;
        }
        if  ((ip = pj->jobfile_name))  {
                while  (*ip)  {
                        *cp++ = *ip++;
                        bpos++;
                }
        }
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

static void  chkalloc()
{
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
}

void  cb_jnew(Widget w, int notused)
{
        int     indx;
        unsigned        cnt;
        XmString        str;

        chkalloc();

        if  ((indx = getselectedjob(0)) >= 0)
                for  (cnt = pend_njobs;  cnt > (unsigned) indx;  cnt--)
                        pend_list[cnt] = pend_list[cnt-1];
        else
                indx = pend_njobs;

        job_initialise(&pend_list[indx], stracpy(Curr_pwd), (char *) 0);

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

static int  get_dir_file(char **dname, char **fname, XmFileSelectionBoxCallbackStruct *cbs)
{
        char    *fl;

        if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &fl) || !fl)
                return  0;

        if  (fl[0] == '\0')  {
                XtFree(fl);
                return  0;
        }
        if  (fl[0] == '/')  {
                char    *sp = strrchr(fl, '/');
                if  (sp[1] == '\0')  { /* 'Twas a directory */
                        XtFree(fl);
                        return  0;
                }
                *fname = stracpy(&sp[1]);
                *sp = '\0';
                *dname = stracpy(fl);
        }
        else  {
                char    *dl;
                int     nn;
                if  (!XmStringGetLtoR(cbs->dir, XmSTRING_DEFAULT_CHARSET, &dl) || !dl)  {
                        XtFree(fl);
                        return  0;
                }
                if  (dl[0] != '/')  {
                        XtFree(dl);
                        XtFree(fl);
                        return  0;
                }
                nn = strlen(dl) - 1;
                if  (nn > 0  &&  dl[nn] == '/')
                        dl[nn] = '\0';
                *dname = stracpy(dl);
                XtFree(dl);
                *fname = stracpy(fl);
        }
        XtFree(fl);
        return  1;
}

char *gen_path(char *dir, char *fil)
{
        char    *result;
        if  (!(result = malloc((unsigned) (strlen(dir) + strlen(fil) + 2))))
                ABORT_NOMEM;
        sprintf(result, "%s/%s", dir, fil);
        return  result;
}

static void  endjopen(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        if  (data)  {
                char    *fname, *dname;
                int     indx, ret;
                unsigned        cnt;
                XmString        str;

                if  (!get_dir_file(&dname, &fname, cbs))
                        return;

                for  (cnt = 0;  cnt < pend_njobs;  cnt++)
                        if  (pend_list[cnt].directory  &&  strcmp(dname, pend_list[cnt].directory) == 0  &&
                             pend_list[cnt].cmdfile_name  &&  strcmp(fname, pend_list[cnt].cmdfile_name) == 0)  {
                                doerror(jwid, $EH{xmbtr file already loaded});
                                free(dname);
                                free(fname);
                                return;
                        }

                chkalloc();

                if  ((indx = getselectedjob(0)) >= 0)
                        for  (cnt = pend_njobs;  cnt > (unsigned) indx;  cnt--)
                                pend_list[cnt] = pend_list[cnt-1];
                else
                        indx = pend_njobs;
                job_initialise(&pend_list[indx], dname, fname);

                /* Carry on even if we have an error.  We may as well include the thing.  */

                if  ((ret = job_load(&pend_list[indx])))
                        doerror(jwid, ret);

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
        XtDestroyWidget(w);
}

void  cb_jopen(Widget w, int notused)
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
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) endjopen, (XtPointer) 0);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endjopen, (XtPointer) 1);
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
        if  (pj->changes > 0  &&  !Confirm(w, isdel? $PH{xmbtr confirm delete}: $PH{xmbtr confirm close}))
                return;

        if  (isdel)  {
                if  (pj->jobfile_name)  {
                        char    *path = gen_path(pj->directory, pj->jobfile_name);
                        if  (unlink(path) < 0  &&  errno != ENOENT)
                                doerror(w, $EH{xmbtr cannot delete job file});
                        free(path);
                }
                if  (pj->cmdfile_name)  {
                        char    *path = gen_path(pj->directory, pj->cmdfile_name);
                        if  (unlink(path) < 0  &&  errno != ENOENT)
                                doerror(w, $EH{xmbtr cannot delete cmd file});
                        free(path);
                }
        }

        XmListDeletePos(jwid, indx+1);

        free((char *) pj->job);
        free(pj->directory);
        if  (pj->jobfile_name)
                free(pj->jobfile_name);
        if  (pj->cmdfile_name)
                free(pj->cmdfile_name);
        pend_njobs--;
        for  (cnt = indx;  cnt < pend_njobs;  cnt++)
                pend_list[cnt] = pend_list[cnt+1];
}

static void  endwfile(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        if  (data)  {
                char            *fname, *dname, **wot;
                int             indx;
                XmString        str;
                struct  pend_job        *pj;

                if  ((indx = getselectedjob(0)) < 0)
                        return;
                pj = &pend_list[indx];

                if  (!get_dir_file(&dname, &fname, cbs))
                        return;

                if  (strcmp(pj->directory, dname) != 0)  {
                        free(pj->directory);
                        pj->directory = stracpy(dname);
                }
                wot = data == JCMDFILE_JOB? &pj->jobfile_name: &pj->cmdfile_name;
                if  (!*wot  ||  strcmp(*wot, fname) != 0)  {
                        char    *path = gen_path(dname, fname);
                        int     fexists;
                        disp_str = path;
                        fexists = f_exists(path);
                        if  (!fexists || Confirm(w, $PH{xmbtr file exists confirm}))  {
                                if  (*wot)
                                        free(*wot);
                                *wot = stracpy(fname);
                                if  (data == JCMDFILE_JOB  &&  !fexists)  {
                                        int     fd;
                                        SWAP_TO(Realuid);
                                        fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0666);
                                        if  (fd >= 0)  {
#ifdef  HAVE_FCHMOD
                                                fchmod(fd, (int) (0666 & ~Save_umask));
                                                close(fd);
#else
                                                close(fd);
                                                chmod(path, (int) (0666 & ~Save_umask));
#endif
                                        }
                                        SWAP_TO(Daemuid);
                                }
                        }
                        free(path);
                }
                free(dname);
                free(fname);
                str = jdisplay(pj);
                XmListReplaceItemsPos(jwid, &str, 1, indx+1);
                XmStringFree(str);
                XmListSelectPos(jwid, indx+1, False);
        }
        XtDestroyWidget(w);
}

void  cb_jcmdfile(Widget w, int which)
{
        int     indx;
        Widget  fsb;
        XmString        str;

        if  ((indx = getselectedjob(1)) < 0)
                return;

        str = XmStringCreateSimple(pend_list[indx].directory);
        fsb = XmCreateFileSelectionDialog(FindWidget(w), which == JCMDFILE_JOB? "jobfile": "cmdfile", NULL, 0);
        XtVaSetValues(fsb,
                      XmNdirectory,             str,
                      NULL);
        XmStringFree(str);
        XtAddCallback(fsb, XmNcancelCallback, (XtCallbackProc) endwfile, (XtPointer) 0);
        XtAddCallback(fsb, XmNokCallback, (XtCallbackProc) endwfile, INT_TO_XTPOINTER(which));
        XtAddCallback(fsb, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER((which == JCMDFILE_JOB? $H{xmbtr job file dialog help}: $H{xmbtr cmd file dialog help})));
        XtManageChild(fsb);
}

void  cb_jsave(Widget w, int notused)
{
        int     indx, ret;
        struct  pend_job        *pj;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];
        if  (pj->changes <= 0  &&  !Confirm(w, $PH{xmbtr file save confirm}))
                return;
        if  (!pj->jobfile_name)  {
                doerror(w, $EH{xmbtr no job file name});
                return;
        }
        if  (!pj->cmdfile_name)  {
                doerror(w, $EH{xmbtr no cmd file name});
                return;
        }
        if  ((ret = job_save(pj)))
                doerror(w, ret);
        else
                pj->changes = 0;
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

void  cb_edit(Widget w, int notused)
{
        int     indx;
        struct  pend_job        *pj;
        char    *path;
        PIDTYPE pid;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];
        if  (!pj->directory || !pj->jobfile_name)  {
                doerror(w, $EH{xmbtr no job file name});
                return;
        }
        path = gen_path(pj->directory, pj->jobfile_name);
        if  (!f_exists(path))  {
                free(path);
                doerror(w, $EH{xmbtr no such file});
                return;
        }

        if  ((pid = fork()) != 0)  {
                int     status;
                free(path);
                if  (pid < 0)  {
                        doerror(w, $EH{xmbtr cannot fork for editor});
                        return;
                }
                displaybusy(1);
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                if  (status != 0)  {
                        disp_str = editor_name;
                        doerror(w, $EH{xmbtr cannot execute editor});
                }
                displaybusy(0);
                return;
        }

        umask((int) Save_umask);
        setuid(Realuid);
        chdir(pj->directory);
        if  (xterm_edit)  {
                char    *termname = envprocess("${XTERM:-xterm}");
                execlp(termname, editor_name, "-e", editor_name, path, (char *) 0);
        }
        else
                execlp(editor_name, editor_name, path, (char *) 0);
        exit(255);
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

void  cb_submit(Widget w, int notused)
{
        int                     indx, ch;
        ULONG                   jindx;
        struct  pend_job        *pj;
        BtjobRef                jreq;
        char                    *path;
        FILE                    *outf, *inf;
        static  jobno_t         jn;
        Shipc                   Oreq;
        Repmess                 rr;

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];
        if  (pj->nosubmit == 0  &&  !Confirm(w, $PH{xmbtr job unchanged confirm}))
                return;
        if  (pj->job->h.bj_times.tc_istime  &&  pj->job->h.bj_times.tc_nexttime < time((time_t *) 0))  {
                doerror(jwid, $EH{xmbtr cannot submit not future});
                return;
        }
        if  (!pj->jobfile_name)  {
                doerror(w, $EH{xmbtr no job file name});
                return;
        }
        path = gen_path(pj->directory, pj->jobfile_name);
        if  (!f_exists(path))  {
                free(path);
                doerror(w, $EH{xmbtr no such file});
                return;
        }

        if  (!(inf = fopen(path, "r")))  {
                free(path);
                doerror(w, $EH{xmbtr cannot open job file});
                return;
        }
        free(path);

        jn = jn? jn+1: getpid();
        outf = goutfile(&jn);

        while  ((ch = getc(inf)) != EOF)  {
                if  (putc(ch, outf) == EOF)  {
                        unlink(tmpfl);
                        fclose(inf);
                        fclose(outf);
                        doerror(w, $EH{No room for job});
                        return;
                }
        }

        fclose(inf);
        fclose(outf);

#ifdef  NHONSUID
        if  (Daemuid != ROOTID  && (Realuid == ROOTID || Effuid == ROOTID))
                chown(tmpfl, Daemuid, Realgid);
#elif   !defined(HAVE_SETEUID)  &&  defined(ID_SWAP)
        if  (Daemuid != ROOTID  &&  Realuid == ROOTID)
                chown(tmpfl, Daemuid, Realgid);
#endif

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
                doerror(jwid, savee == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
                return;
        }

        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
                if  (errno == EINTR)
                        continue;
                freexbuf(jindx);
                doerror(jwid, $E{Error on IPC});
                return;
        }

        freexbuf(jindx);

        if  (rr.outmsg.mcode != J_OK) {
                unlink(tmpfl);
                if  ((rr.outmsg.mcode & REQ_TYPE) != JOB_REPLY)  { /* Not expecting net errors */
                        disp_arg[0] = rr.outmsg.mcode;
                        doerror(jwid, $EH{Unexpected sched message});
                }
                else  {
                        disp_str = title_of(pj->job);
                        doerror(jwid, (int) ((rr.outmsg.mcode & ~REQ_TYPE) + $EH{Base for scheduler job errors}));
                }
                return;
        }

        if  (pj->Verbose)  {
                disp_arg[0] = jn;
                disp_str = title_of(pj->job);
                doinfo(jwid, disp_str[0]? $E{xmbtr job created ok title}: $E{xmbtr job created ok no title});
        }
        pj->nosubmit = 0;
}
