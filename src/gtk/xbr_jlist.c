/* xbr_jlist.c -- job list handling for gbch-xr

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
#include <gtk/gtk.h>
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
#include "xbr_ext.h"
#include "gtk_lib.h"
#include "stringvec.h"
#ifdef  HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include "optflags.h"

static  char    Filename[] = __FILE__;

#define NAMESIZE        14      /* Padding for temp file */

#define C_MASK          037

static  char    *tmpfl;

unsigned                pend_njobs, pend_max;
struct  pend_job        *pend_list;

extern  char            *Curr_pwd;

char            *execprog, *ldsvprog, *gtkprog;
HelpaltRef      daynames_full, monnames, ifnposs_names, repunit_full;
char            *delend_full, *retend_full;
char            *no_title, *no_name;
#ifdef  SUFFIXES
char            *cmd_prefix, *cmd_suffix, *job_prefix, *job_suffix;
#endif

ULONG           default_rate;
unsigned char   default_interval,
                default_nposs;
USHORT          defavoid, def_assflags;

#define ppermitted(flg) (mypriv->btu_priv & flg)

HelpaltRef      progresslist;
extern  HelpaltRef      assnames, actnames, stdinnames;

#ifdef  HAVE_SYS_RESOURCE_H
extern  int     Max_files;
#endif

extern  char    *getscript_file(FILE *, unsigned *);

static  int     has_xml_suffix(const char *name)
{
        int  lng = strlen(name);

        return  lng >= sizeof(XMLJOBSUFFIX)  &&  ncstrcmp(name + lng - sizeof(XMLJOBSUFFIX) + 1, XMLJOBSUFFIX) == 0;
}

/* First arg is r w d or x with x meaning write and make executable */

FILE    *ldsv_open(const char rw, const char *dirname, const char *filename)
{
        FILE    *result;
        GString  *path = g_string_new(NULL);
        char    popenmode[2];

        if  (dirname)
                g_string_printf(path, "%s -%c %s/%s", ldsvprog, rw, dirname, filename);
        else
                g_string_printf(path, "%s -%c %s", ldsvprog, rw, filename);
        popenmode[0] = rw == 'x'? 'w': rw;
        popenmode[1] = '\0';
        result = popen(path->str, popenmode);
        g_string_free(path, TRUE);
        return  result;
}

void    ldsv_del(struct pend_job *pj, char *filename, const int errnum)
{
        GString  *path;
        if  (!filename)
                return;
        path = g_string_new(NULL);
        g_string_printf(path, "%s -d %s/%s", ldsvprog, pj->directory, filename);
        if  (system(path->str) != 0)
                doerror(errnum);
        g_string_free(path, TRUE);
}

void    ldsv_unlink(char *fname)
{
        GString  *path = g_string_new(NULL);
        g_string_printf(path, "%s -d %s", ldsvprog, fname);
        if  (system(path->str) != 0)  {
                disp_str = fname;
                doerror($EH{xbtr could not delete});
        }
        g_string_free(path, TRUE);
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

void  gen_qlist(struct stringvec *ql)
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

void  initmoremsgs()
{
        int     n;
#ifdef  HAVE_SYS_RESOURCE_H
        struct  rlimit  cfiles;
        getrlimit(RLIMIT_NOFILE, &cfiles);
        Max_files = cfiles.rlim_cur;
#endif
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
        delend_full = gprompt($P{Delete at end full});
        retend_full = gprompt($P{Retain at end full});
        repunit_full = helprdalt($Q{Repeat unit full});
        ifnposs_names = helprdalt($Q{Wtime if not poss});
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        no_title = gprompt($P{xbtr no title});
        no_name = gprompt($P{xbtr no file name});
        if  (!(actnames = helprdalt($Q{Redirection type names})))  {
                disp_arg[9] = $Q{Redirection type names};
                print_error($E{Missing alternative code});
        }
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                print_error($E{Missing alternative code});
        }
        daynames_full = helprdalt($Q{Weekdays full});
        monnames = helprdalt($Q{Months full});
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

#ifdef  SUFFIXES
        job_prefix = helpprmpt($P{Default job file prefix});
        job_suffix = helpprmpt($P{Default job file suffix});
        cmd_prefix = helpprmpt($P{Default cmd file prefix});
        cmd_suffix = helpprmpt($P{Default cmd file suffix});
        if  (!job_prefix)
                job_prefix = "";
        if  (!job_suffix)
                job_suffix = "";
        if  (!cmd_prefix)
                cmd_prefix = "";
        if  (!cmd_prefix)
                cmd_suffix = "";
#endif

        ldsvprog = envprocess(GTKLDSAV);
        execprog = envprocess(EXECPROG);
        gtkprog = envprocess(GTKSAVE);
        Save_umask = umask(C_MASK);
}

/* Update job title display */

void  update_title(struct pend_job *pj)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter, citer;

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                const  char  *ip = title_of(pj->job);
                if  (strlen(ip) == 0)
                        ip = no_title;
                gtk_tree_model_sort_convert_iter_to_child_iter(sorted_jlist_store, &citer, &iter);
                gtk_list_store_set(raw_jlist_store, &citer, JLIST_TITLE_COL, ip, -1);
        }
}

void  update_state(struct pend_job *pj)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter, citer;
        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                gtk_tree_model_sort_convert_iter_to_child_iter(sorted_jlist_store, &citer, &iter);
                gtk_list_store_set(raw_jlist_store, &citer, JLIST_PROGRESS_COL, disp_alt(pj->job->h.bj_progress, progresslist), -1);
        }
}

int  getselectedjob(const int moan)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter;

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(sorted_jlist_store), &iter, JLIST_SEQ_COL, &seq, -1);
                return  (int) seq;
        }
        if  (moan)
                doerror(pend_njobs != 0? $EH{xmbtq no job selected}: $EH{xmbtq no jobs to select});
        return  -1;
}

struct pend_job *job_or_deflt(GtkAction *action)
{
        const   char    *act = gtk_action_get_name(action);
        if  (act[strlen(act)-1] != 'd')  {
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

static  GString  *display_jobfile_name(struct pend_job *pj)
{
        GString  *jf_name = g_string_new(NULL);

        /* Legacy style - put as command file name (job script file name)
           XML style, put in the name removing the suffix if it matches */

        if  (pj->xml_jobfile_name)  {
                int  lng = strlen((const char *) pj->xml_jobfile_name);
                if  (has_xml_suffix((const char *) pj->xml_jobfile_name))
                        lng -= sizeof(XMLJOBSUFFIX) - 1;
                g_string_append_len(jf_name, pj->xml_jobfile_name, lng);
        }
        else  if  (pj->cmdfile_name)
                g_string_printf(jf_name, "%s (%s)", pj->cmdfile_name, !pj->jobfile_name || strlen(pj->jobfile_name) == 0? no_name: pj->jobfile_name);
        else
                g_string_assign(jf_name, no_name);
        return  jf_name;
}

static void  append_job(struct pend_job *pj)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter     iter, piter;
        const  char     *tit;
        GString  *jf_name;

        tit = title_of(pj->job);
        if  (strlen(tit) == 0)
                tit = no_title;

        jf_name = display_jobfile_name(pj);

        gtk_list_store_append(raw_jlist_store, &iter);
        gtk_list_store_set(raw_jlist_store, &iter,
                           JLIST_SEQ_COL,       pend_njobs,
                           JLIST_PROGRESS_COL,  disp_alt(pj->job->h.bj_progress, progresslist),
                           JLIST_TITLE_COL,     tit,
                           JLIST_JOBFILE_COL,   jf_name->str,
                           JLIST_DIRECT_COL,    pj->directory,
                           JLIST_UNSAVED_COL,   pj->changes != 0,
                           -1);

        g_string_free(jf_name, TRUE);

        gtk_tree_model_sort_convert_child_iter_to_iter(sorted_jlist_store, &piter, &iter);
        gtk_tree_selection_select_iter(sel, &piter);
}

void  cb_options()
{
        int     indx;
        struct  pend_job  *pj;
        GtkWidget *dlg, *verb;

        if  ((indx = getselectedjob(1)) < 0)
                return;

        pj = &pend_list[indx];

        dlg = gprompt_dialog(toplevel, $P{xbtr job opt dlgtit});
        verb = gprompt_checkbutton($P{xbtr verbose job});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), verb, FALSE, FALSE, DEF_DLG_VPAD);
        if  (pj->Verbose)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(verb), TRUE);
        gtk_widget_show_all(dlg);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                int newv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(verb))? 1: 0;
                if  (newv != pj->Verbose)  {
                        pj->Verbose = newv;
                        note_changes(pj);
                }
        }
        gtk_widget_destroy(dlg);
}

void  cb_jnew()
{
        struct  pend_job  *pj;

        chkalloc();
        pj = &pend_list[pend_njobs];
        job_initialise(pj, stracpy(Curr_pwd), (char *) 0);
        append_job(pj);
        pend_njobs++;
}

char *gen_path(char *dir, char *fil)
{
        char    *result;
        if  (!(result = malloc((unsigned) (strlen(dir) + strlen(fil) + 2))))
                ABORT_NOMEM;
        sprintf(result, "%s/%s", dir, fil);
        return  result;
}

static gboolean  isit_cmd(const GtkFileFilterInfo *filter_info, gpointer data)
{
        FILE    *fp;
        struct  stat    sbuf;
        char    Pbuf[300];

        if  (stat(filter_info->filename, &sbuf) < 0  ||  (sbuf.st_mode & S_IFMT) != S_IFREG  ||  sbuf.st_size == 0)
                return  FALSE;

        fp = ldsv_open('r', (const char *) 0, (const char *) filter_info->filename);
        if  (!fp)
             return  FALSE;

        while  (fgets(Pbuf, sizeof(Pbuf), fp))  {
                char    *lp;
                if  (Pbuf[0] == '#')
                        continue;
                if  ((lp = strchr(Pbuf, '\n')))
                        *lp = '\0';
                if  (strncmp(Pbuf, BTR_PROGRAM " ", sizeof(BTR_PROGRAM)) == 0)  {
                        pclose(fp);
                        return  TRUE;
                }
                if  (!strchr(Pbuf, '='))  {
                        pclose(fp);
                        return  FALSE;
                }
        }
        pclose(fp);
        return  FALSE;
}

static  GtkFileFilter  *get_xml_filter()
{
        static  GtkFileFilter  *isit_xml_job_filt = 0;
        if  (!isit_xml_job_filt)  {
                char *pr = gprompt($P{xbtr poss xml job files});
                isit_xml_job_filt = gtk_file_filter_new();
                gtk_file_filter_set_name(isit_xml_job_filt, pr);
                free(pr);
                gtk_file_filter_add_pattern(isit_xml_job_filt, (const gchar *) "*" XMLJOBSUFFIX);
                /* Increase the ref count on the filter so it doesn't get deallocated between calls */
                g_object_ref(G_OBJECT(isit_xml_job_filt));
        }

        return  isit_xml_job_filt;
}

/* Open a job file in the new XML format */

void  cb_jopen()
{
#ifdef  HAVE_LIBXML2
        GtkWidget  *fsb;
        char    *pr;

        pr = gprompt($P{xbtr open job dlgtit});
        fsb = gtk_file_chooser_dialog_new(pr, GTK_WINDOW(toplevel),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);

        /* Set up file chooser to pick files with our job suffix */

        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsb), Curr_pwd);
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fsb), get_xml_filter());

    restart:
        if  (gtk_dialog_run(GTK_DIALOG(fsb)) == GTK_RESPONSE_ACCEPT)  {
                char  *pathname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsb));
                char  *sp, *dirname, *filename;
                int     cnt, ret;
                struct  pend_job  *pj;

                dirname = pathname;
                sp = strrchr(pathname, '/');

                if  (sp)  {
                        if  (sp == pathname)            /* I.e. in root directory */
                                dirname = "/";
                        else
                                *sp = '\0';
                        filename = sp + 1;
                }
                else  {                 /* Not abs path */
                        dirname = Curr_pwd;
                        filename = pathname;
                }

                for  (cnt = 0;  cnt < pend_njobs;  cnt++)  {
                        pj = &pend_list[cnt];
                        if  (pj->directory  &&  strcmp(dirname, pj->directory) == 0  &&
                             pj->xml_jobfile_name  &&  strcmp(filename, pj->xml_jobfile_name) == 0)  {
                                doerror($EH{xmbtr file already loaded});
                                g_free(pathname);
                                goto  restart;
                        }
                }

                chkalloc();
                pj = &pend_list[pend_njobs];

                /* Put details of job file in but leave blank the command file */

                job_initialise(pj, stracpy(dirname), (char *) 0);
                pj->xml_jobfile_name = stracpy(filename);

                /* Carry on even if we have an error.  We may as well include the thing.  */

                if  ((ret = xml_job_load(pj)))
                        doerror(ret);
                else
                        pj->changes = 0;

                append_job(pj);
                pend_njobs++;
        }
        gtk_widget_destroy(fsb);
#else
        doerror($EH{No XML library});
#endif
}

void  cb_jlegopen()
{
        GtkWidget  *fsb;
        char    *pr;
        static  GtkFileFilter  *isit_cmd_filt = 0;

        pr = gprompt($P{xbtr open legacy job dlgtit});
        fsb = gtk_file_chooser_dialog_new(pr, GTK_WINDOW(toplevel),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsb), Curr_pwd);

        if  (!isit_cmd_filt)  {
                pr = gprompt($P{xbtr poss job cmd files});
                isit_cmd_filt = gtk_file_filter_new();
                gtk_file_filter_set_name(isit_cmd_filt, pr);
                free(pr);
                gtk_file_filter_add_custom(isit_cmd_filt, GTK_FILE_FILTER_FILENAME, (GtkFileFilterFunc) isit_cmd, NULL, NULL);
        }

        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fsb), isit_cmd_filt);
        g_object_ref(G_OBJECT(isit_cmd_filt));

    restart:
        if  (gtk_dialog_run(GTK_DIALOG(fsb)) == GTK_RESPONSE_ACCEPT)  {
                char  *pathname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsb));
                char  *sp = strrchr(pathname, '/');
                char  *dirname = pathname;
                char  *filename = sp+1;
                int     cnt, ret;
                struct  pend_job  *pj;

                if  (sp)  {
                        if  (sp == pathname)
                                dirname = "/";
                        *sp = '\0';
                }
                else  {
                        dirname = Curr_pwd;
                        filename = pathname;
                }
                for  (cnt = 0;  cnt < pend_njobs;  cnt++)  {
                        pj = &pend_list[cnt];
                        if  (pj->directory  &&  strcmp(dirname, pj->directory) == 0  &&
                             pj->cmdfile_name  &&  strcmp(filename, pj->cmdfile_name) == 0)  {
                                doerror($EH{xmbtr file already loaded});
                                g_free(pathname);
                                goto  restart;
                        }
                }

                chkalloc();
                pj = &pend_list[pend_njobs];
                job_initialise(pj, stracpy(dirname), stracpy(filename));

                /* Carry on even if we have an error.  We may as well include the thing.  */

                if  ((ret = job_load(pj)))
                        doerror(ret);
                else
                        pj->changes = 0;

                append_job(pj);
                pend_njobs++;
        }
        gtk_widget_destroy(fsb);
}

void  cb_jclosedel(GtkAction *action)
{
        int             indx, isdel;
        unsigned        cnt;
        struct          pend_job        *pj;
        GtkTreeIter     iter;

        if  ((indx = getselectedjob(1)) < 0)
                return;

        /* indx should be the right place in raw_jlist_store */

        isdel = strcmp(gtk_action_get_name(action), "Delete") == 0;

        pj = &pend_list[indx];
        if  (pj->changes != 0  &&  !Confirm($PH{xmbtr confirm close}))
                return;

        if  (isdel)  {
                if  (!Confirm($PH{xmbtr confirm delete}))
                        return;
                ldsv_del(pj, pj->jobfile_name, $EH{xmbtr cannot delete job file});
                ldsv_del(pj, pj->cmdfile_name, $EH{xmbtr cannot delete cmd file});
                ldsv_del(pj, pj->xml_jobfile_name, $EH{xmbtr cannot delete job file});
        }

        if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, indx))
                gtk_list_store_remove(raw_jlist_store, &iter);

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

        /* Move up all the jobs below that and change the index */

        for  (cnt = indx;  cnt < pend_njobs;  cnt++)  {
                pend_list[cnt] = pend_list[cnt+1];
                if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, cnt))
                        gtk_list_store_set(raw_jlist_store, &iter, JLIST_SEQ_COL, cnt, -1);
        }
}

static  char    *getlegfiledlg(const int prnum, struct pend_job *pj, char *fn)
{
        GtkWidget       *fsb;
        char    *pr, *result = (char *) 0;

        pr = gprompt(prnum);
        fsb = gtk_file_chooser_dialog_new(pr, GTK_WINDOW(toplevel),
                        GTK_FILE_CHOOSER_ACTION_SAVE,
                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        NULL);
        free(pr);
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fsb), TRUE);
        if  (fn)  {
                char  *fpath = gen_path(pj->directory, fn);
                gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(fsb), fpath);
                free(fpath);
        }
        else
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsb), pj->directory);
        if  (gtk_dialog_run(GTK_DIALOG(fsb)) == GTK_RESPONSE_ACCEPT)
                result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsb));
        gtk_widget_destroy(fsb);
        return  result;
}

/* Set up legacy-style file name returning 1 if OK 0 if nothing done */

static  int     getlegfilenames(struct pend_job *pj)
{
        char    *cmdfilepath, *jobfilepath, *dir, *jobf, *cmdf;
        int     preflen = 1;

        cmdfilepath = getlegfiledlg($P{xbtr set cmd file dlgtit}, pj, pj->cmdfile_name);
        if  (!cmdfilepath)
                return  0;
        jobfilepath = getlegfiledlg($P{xbtr set job file dlgtit}, pj, pj->jobfile_name);
        if  (!jobfilepath)  {
                g_free(cmdfilepath);
                return  0;
        }
        if  (strcmp(cmdfilepath, jobfilepath) == 0)  {
                doerror($EH{xbtr job cmd file names same});
                g_free(cmdfilepath);
                g_free(jobfilepath);
                return  0;
        }

        /* Try to find the longest common prefix */

        for  (;;)  {
                char    *csp, *jsp;
                int     csl, jsl;
                if  (!(csp = strchr(cmdfilepath + preflen, '/')))
                        break;
                if  (!(jsp = strchr(jobfilepath + preflen, '/')))
                        break;
                csl = (csp - cmdfilepath) - preflen;
                jsl = (jsp - jobfilepath) - preflen;
                if  (csl != jsl)
                        break;
                if  (strncmp(cmdfilepath + preflen, jobfilepath + preflen, csl) != 0)
                        break;
                preflen += csl + 1;
        }

        if  (preflen == 1)
                dir = "/";
        else  {
                cmdfilepath[preflen-1] = '\0';
                dir = cmdfilepath;
        }
        jobf = jobfilepath+preflen;
        cmdf = cmdfilepath+preflen;
        if  (strcmp(pj->directory, dir) != 0)  {
                free(pj->directory);
                pj->directory = stracpy(dir);
        }
        if  (pj->jobfile_name)
                free(pj->jobfile_name);
        pj->jobfile_name = stracpy(jobf);
        if  (pj->cmdfile_name)
                free(pj->cmdfile_name);
        pj->cmdfile_name = stracpy(cmdf);
        g_free(cmdfilepath);
        g_free(jobfilepath);
        return  1;
}

/* Get save file name for XML files, returning 1 if OK 0 if unchanged */

static  int     getxmlfilename(struct pend_job *pj)
{
        GtkWidget       *fsb;
        char    *pr;
        int     ret = 0;

        pr = gprompt($P{xbtr set job file dlgtit});
        fsb = gtk_file_chooser_dialog_new(pr, GTK_WINDOW(toplevel),
                        GTK_FILE_CHOOSER_ACTION_SAVE,
                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        NULL);
        free(pr);

        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(fsb), TRUE);
        if  (pj->xml_jobfile_name)  {
                char  *fpath = gen_path(pj->directory, pj->xml_jobfile_name);
                gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(fsb), fpath);
                free(fpath);
        }
        else
                gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsb), pj->directory);
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fsb), get_xml_filter());
        if  (gtk_dialog_run(GTK_DIALOG(fsb)) == GTK_RESPONSE_ACCEPT)  {
                char  *pathname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsb));
                char  *sp, *dirname, *filename;

                if  (!has_xml_suffix(pathname))  {
                        GString  *npath = g_string_new(pathname);
                        g_string_append(npath, XMLJOBSUFFIX);
                        g_free(pathname);
                        pathname = npath->str;
                        g_string_free(npath, FALSE);    /* False because we're taking over the string */
                }

                dirname = pathname;
                sp = strrchr(pathname, '/');

                if  (sp)  {
                        if  (sp == pathname)            /* I.e. in root directory */
                                dirname = "/";
                        else
                                *sp = '\0';
                        filename = sp + 1;
                }
                else  {                 /* Not abs path */
                        dirname = Curr_pwd;
                        filename = pathname;
                }

                if  (strcmp(pj->directory, dirname) != 0)  {
                        free(pj->directory);
                        pj->directory = stracpy(dirname);
                }

                if  (pj->xml_jobfile_name)
                        free(pj->xml_jobfile_name);
                pj->xml_jobfile_name = stracpy(filename);
                g_free(pathname);
                ret = 1;
        }
        gtk_widget_destroy(fsb);
        return  ret;
}

/* Check to see if we're converting from legacy format job files
   to XML and do the conversions required. */

static  int     check_to_xml(struct pend_job *pj)
{
        FILE    *inf;
        unsigned  fsize;

        /* If we've got a script, there is nothing to do */

        if  (pj->jobscript)
                return  1;

        /* We shouldn't get in here without a jobfile name so we assume it's there */

        if  (!(inf = ldsv_open('r', pj->directory, pj->jobfile_name)))  {
                doerror($EH{xmbtr cannot open job file});
                return  0;
        }

        /* Load up the job script file. */

        pj->jobscript = getscript_file(inf, &fsize);
        if  (pclose(inf) < 0)  {
                doerror($EH{xmbtr cannot open job file});
                return  0;
        }

        /* If we didn't read anything from the file but didn't actually have an error, allocate a null string */

        if  (!pj->jobscript)
                pj->jobscript = stracpy("");

        /* Ask about deleting the original job files */

        if  (Confirm($PH{xbtr delete legacy job files}))  {
                ldsv_del(pj, pj->jobfile_name, $EH{xmbtr cannot delete job file});
                ldsv_del(pj, pj->cmdfile_name, $EH{xmbtr cannot delete cmd file});
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

static  int     check_from_xml(struct pend_job *pj)
{
        FILE    *outf;
        unsigned  len, nb;
        int     ret;

        /* If we didn't actually have a script, we are all OK */

        if  (!pj->jobscript)
                return  1;

        outf = ldsv_open('w', pj->directory, pj->jobfile_name);
        if  (!outf)  {
                doerror($EH{xbq cannot write job file});
                return  0;
        }

        /* Write out script to job file.
           Script might be zero length. */

        nb = len = strlen(pj->jobscript);
        if  (len > 0)
                nb = fwrite(pj->jobscript, sizeof(char), len, outf);

        /* Might detect error writing or on closing */

        ret = pclose(outf);
        if  (len != nb  ||  ret < 0)  {
                doerror($EH{xbq cannot write job file});
                return  0;
        }

        /* Possibly delete XML file, but deallocate name and pointer */

        if  (pj->xml_jobfile_name)  {
                if  (Confirm($PH{xbtr delete XML job file}))
                        ldsv_del(pj, pj->xml_jobfile_name, $EH{xmbtr cannot delete job file});
                free(pj->xml_jobfile_name);
                pj->xml_jobfile_name = (char *) 0;
        }
        free(pj->jobscript);
        pj->jobscript = (char *) 0;
        pj->scriptinmem = 0;
        return  1;
}

void    cb_jsaveas()
{
        int     indx, ret;
        struct  pend_job  *pj;
        GtkTreeIter     iter;

        if  ((indx = getselectedjob(1)) < 0)
                return;

        pj = &pend_list[indx];
        if  (pj->changes == 0  &&  !Confirm($PH{xmbtr file save confirm}))
                return;

        /* Need a script before we can save */

        if  (!pj->jobscript  &&  !pj->jobfile_name)  {
                doerror($EH{xbtr no script});
                return;
        }

        if  (xml_format)
                ret = getxmlfilename(pj);
        else
                ret = getlegfilenames(pj);

        if  (ret == 0)
                return;

        if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, indx))  {
                GString  *jfname = display_jobfile_name(pj);
                gtk_list_store_set(raw_jlist_store, &iter, JLIST_DIRECT_COL, pj->directory, JLIST_JOBFILE_COL, jfname->str, -1);
                g_string_free(jfname, TRUE);
        }

        if  (xml_format)  {
                if  (!check_to_xml(pj))
                        return;
                ret = job_save_xml(pj);
        }
        else  {
                if  (!check_from_xml(pj))
                        return;
                ret = job_save(pj);
        }

        if  (ret)
                doerror(ret);
        else  {
                pj->changes = 0;
                if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, indx))
                        gtk_list_store_set(raw_jlist_store, &iter, JLIST_UNSAVED_COL, FALSE, -1);
        }
}

/* Job save - we support legacy job saving as well as XML */

void  cb_jsave()
{
        int     indx, ret;
        struct  pend_job  *pj;
        GtkTreeIter     iter;

        if  ((indx = getselectedjob(1)) < 0)
                return;

        pj = &pend_list[indx];

        /* Jump into save as if we haven't got the relevant file names.
           This also does the check we've got the script */

        if  (xml_format)  {
                if  (!pj->xml_jobfile_name)  {
                        cb_jsaveas();
                        return;
                }
        }
        else  if  (!pj->cmdfile_name  ||  !pj->jobfile_name)  {
                cb_jsaveas();
                return;
        }

        if  (pj->changes == 0  &&  !Confirm($PH{xmbtr file save confirm}))
                return;

        if  (xml_format)  {
                if  (!(check_to_xml(pj)))
                        return;
                ret = job_save_xml(pj);
        }
        else  {
                if  (!(check_from_xml(pj)))
                        return;
                ret = job_save(pj);
        }

        if  (ret)
                doerror(ret);
        else  {
                pj->changes = 0;
                if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, indx))
                        gtk_list_store_set(raw_jlist_store, &iter, JLIST_UNSAVED_COL, FALSE, -1);
        }
}

void  note_changes(struct pend_job *pj)
{
        GtkTreeIter     iter;

        pj->changes = 1;
        pj->nosubmit = 1;
        if  (pj == &default_pend)
                return;
        if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(raw_jlist_store), &iter, NULL, pj - pend_list))
                gtk_list_store_set(raw_jlist_store, &iter, JLIST_UNSAVED_COL, TRUE, -1);
}

static  GString  *script_edit_title(struct pend_job *pj)
{
        GString  *result = g_string_new(NULL);
        const  char  *tit = title_of(pj->job);

        if  (tit[0])
                g_string_printf(result, "\"%s\"", tit);
        else
                g_string_printf(result, "(%s)", no_name);

        if  (pj->xml_jobfile_name)
                g_string_append_printf(result, " - %s", pj->xml_jobfile_name);
        else  if  (pj->cmdfile_name)
                g_string_append_printf(result, " %s", pj->cmdfile_name);
        if  (pj->jobfile_name)
                g_string_append_printf(result, " [%s]", pj->jobfile_name);
        return  result;
}

/* Edit job script in memory.
   Return replacement script or */

static  char    *script_edit(GString *title, char *orig)
{
        GtkWidget       *dlg, *textb, *scroll;
        GtkTextBuffer *buff;
        PangoFontDescription *font_desc;
        char    *ret = (char *) 0;
        static  gint    wwidth = 500, wheight = 600;

        dlg = gprompt_dialog(toplevel, $P{xbtr job script edit});
        gtk_window_set_default_size(GTK_WINDOW(dlg), wwidth, wheight);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), gtk_label_new(title->str), FALSE, FALSE, DEF_DLG_VPAD);
        g_string_free(title, TRUE);
        textb = gtk_text_view_new();
        buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textb));

        /* If there was an existing script, copy it in */
        if  (orig)
                gtk_text_buffer_set_text(buff, orig, -1);

        if  ((font_desc = pango_font_description_from_string("fixed")))  {
                gtk_widget_modify_font(textb, font_desc);
                pango_font_description_free(font_desc);
        }

        scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_set_border_width(GTK_CONTAINER(scroll), 5);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), textb);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), scroll, TRUE, TRUE, 0);
        gtk_widget_show_all(dlg);

        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                gchar *res;
                GtkTextIter  st, nd;
                gtk_text_buffer_get_bounds(buff, &st, &nd);
                res = gtk_text_buffer_get_text(buff, &st, &nd, FALSE);
                ret = stracpy(res);
                g_free(res);
                gtk_window_get_size(GTK_WINDOW(dlg), &wwidth, &wheight);
        }
        gtk_widget_destroy(dlg);
        return  ret;
}

/* Make ourselves a temporary file for editing a script with an external editor in.
   We may not actually have a script yet so we just create the file. */

static  char    *tmpscript_file(struct pend_job *pj)
{
        GString *path = g_string_new(NULL);
        static  int     tsind = 0;
        const  char     *tdir;
        char    *ret;
        FILE    *outf;
        int     len;

        if  (!(tdir = getenv("TMPDIR")) || access(tdir, F_OK) < 0)  {
#ifdef  P_tmpdir
                tdir = P_tmpdir;
                if  (access(tdir, F_OK) < 0)
#endif
                        tdir = "/tmp";
        }
        do  {
                tsind++;
                g_string_printf(path, "%s/bscedit.%d.%d", tdir, getpid(), tsind);
        }  while  (access(path->str, F_OK) >= 0);

        /* Copy using stracpy in case we can't free a string */

        ret = stracpy(path->str);
        g_string_free(path, TRUE);

        /* Create file using ext program to get right userid, if we have a script copy it out */

        outf = ldsv_open('w', (const char *) 0, ret);
        if  (pj->jobscript  &&  (len = strlen(pj->jobscript)) > 0)
                fwrite(pj->jobscript, sizeof(char), len, outf);
        pclose(outf);
        return  ret;
}

static  int     run_ext_editor(char *filename)
{
        PIDTYPE pid;

        if  ((pid = fork()) != 0)  {
                int     status;
                if  (pid < 0)
                        return  $EH{xmbtr cannot fork for editor};
#ifdef  HAVE_WAITPID
                while  (waitpid(pid, &status, 0) < 0)
                        ;
#else
                while  (wait(&status) != pid)
                        ;
#endif
                if  (status != 0)  {
                        disp_str = editor_name;
                        return  $EH{xmbtr cannot execute editor};
                }
                return  0;
        }

        umask((int) Save_umask);
        Ignored_error = chdir(Curr_pwd);
        if  (xterm_edit)  {
                char    *termname = envprocess("${XTERM:-xterm}");
                execl(execprog, termname, "-e", editor_name, filename, (char *) 0);
        }
        else
                execl(execprog, editor_name, filename, (char *) 0);
        exit(255);
        return  0;
}

static  void    inmem_edit_internal_editor(struct pend_job *pj)
{
        char    *newsc = script_edit(script_edit_title(pj), pj->jobscript);
        if  (newsc)  {
                if  (pj->jobscript)
                        free(pj->jobscript);
                pj->jobscript = newsc;
                note_changes(pj);
        }
}

static  void    inmem_edit_external_editor(struct pend_job *pj)
{
        char    *tmpf = tmpscript_file(pj);
        int     ret = run_ext_editor(tmpf);
        unsigned  fsize;
        char    *newsc;
        FILE    *inf;

        if  (ret != 0)  {               /* Had error */
                doerror(ret);
                ldsv_unlink(tmpf);
                free(tmpf);
                return;
        }

        if  (!(inf = ldsv_open('r', (const char *) 0, tmpf)))  {
                doerror($EH{xmbtr cannot open job file});
                ldsv_unlink(tmpf);
                free(tmpf);
                return;
        }

        newsc = getscript_file(inf, &fsize);
        if  (pclose(inf) < 0)  {        /* Actual error */
                doerror($EH{xmbtr cannot open job file});
                ldsv_unlink(tmpf);
                free(tmpf);
                return;
        }

        if  (pj->jobscript)
                free(pj->jobscript);

        if  (!newsc)  /* Didn't actually read anything */
                newsc = stracpy("");
        pj->jobscript = newsc;
        note_changes(pj);
        ldsv_unlink(tmpf);
        free(tmpf);
}

static  void    file_edit_internal_editor(struct pend_job *pj)
{
        FILE  *inf = ldsv_open('r', pj->directory, pj->jobfile_name);
        char    *scr, *newscr;
        unsigned  fsize;

        if  (!inf)  {
                doerror($EH{xmbtr cannot open job file});
                return;
        }

        scr = getscript_file(inf, &fsize);

        if  (pclose(inf) < 0)  {
                doerror($EH{xmbtr cannot open job file});
                return;
        }

        newscr = script_edit(script_edit_title(pj), scr);
        if  (newscr)  {
                pj->jobscript = newscr;         /* No mem leak this is only done if it's null */
                note_changes(pj);
        }
        if  (scr)
                free(scr);
}

static  void    file_edit_external_editor(struct pend_job *pj)
{
        char    *path = gen_path(pj->directory, pj->jobfile_name);
        int     ret = run_ext_editor(path);
        free(path);
        if  (ret)
                doerror(ret);
        else
                note_changes(pj);
}

/* Edit job script case. */

void  cb_edit()
{
        int     indx;
        struct  pend_job        *pj;

        /* Get the job in question */

        if  ((indx = getselectedjob(1)) < 0)
                return;
        pj = &pend_list[indx];

        /* We try to end up with a job script in memory these days, even if
           we end up saving it in legacy format.
           So recognise 4 cases:
           1. Already in memory, internal editor (or starting from new)
           2. Already in memory, external editor (or starting from new)
           3. In job file, internal editor.
           4. In job file, external editor.

           Only in the last case do we end up without the script in memory.*/

        if  (pj->jobscript || !pj->jobfile_name)  {
                /* Already got in-memory script or no script. */
                if  (internal_edit)
                        inmem_edit_internal_editor(pj);
                else
                        inmem_edit_external_editor(pj);
        }
        else  {
                if  (!pj->directory)  {
                        doerror($EH{xmbtr no job file name});
                        return;
                }
                if  (internal_edit)
                        file_edit_internal_editor(pj);
                else
                        file_edit_external_editor(pj);
        }
}

void  cb_direct()
{
        GtkWidget       *dlg;
        char            *pr;

        pr = gprompt($P{xbtr select working dir});
        dlg = gtk_file_chooser_dialog_new(pr,
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          NULL);
        free(pr);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), Curr_pwd);
        if  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                gchar  *selected = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
                free(Curr_pwd);
                Curr_pwd = stracpy(selected);
                g_free(selected);
        }
        gtk_widget_destroy(dlg);
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

static  int     submit_core(struct pend_job *pj)
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
                doerror(savee == EAGAIN? $EH{IPC msg q full}: $EH{IPC msg q error});
                return  0;
        }

        while  (msgrcv(Ctrl_chan, (struct msgbuf *) &rr, sizeof(Shreq), mymtype, 0) < 0)  {
                if  (errno == EINTR)
                        continue;
                freexbuf(jindx);
                doerror($E{Error on IPC});
                return  0;
        }

        freexbuf(jindx);

        if  (rr.outmsg.mcode != J_OK) {
                unlink(tmpfl);
                if  ((rr.outmsg.mcode & REQ_TYPE) != JOB_REPLY)  { /* Not expecting net errors */
                        disp_arg[0] = rr.outmsg.mcode;
                        doerror($EH{Unexpected sched message});
                }
                else  {
                        disp_str = title_of(pj->job);
                        doerror((int) ((rr.outmsg.mcode & ~REQ_TYPE) + $EH{Base for scheduler job errors}));
                }
                return  0;
        }

        return  1;
}

/* Check currently-selected job OK to submit */

struct  pend_job        *sub_check()
{
        int     indx;
        struct  pend_job  *pj;

        if  ((indx = getselectedjob(1)) < 0)
                return  (struct pend_job *) 0;

        /* Protest if job unchanged since last time */

        pj = &pend_list[indx];
        if  (pj->nosubmit == 0  &&  !Confirm($PH{xmbtr job unchanged confirm}))
                return  (struct pend_job *) 0;

        /* Check in future */

        if  (pj->job->h.bj_times.tc_istime  &&  pj->job->h.bj_times.tc_nexttime < time((time_t *) 0))  {
                doerror($EH{xmbtr cannot submit not future});
                return  (struct pend_job *) 0;
        }

        if  (!pj->jobscript  &&  !pj->jobfile_name)  {
                doerror($EH{xmbtr no job file name});
                return  (struct pend_job *) 0;
        }

        return  pj;
}

void  cb_submit()
{
        struct  pend_job        *pj;
        FILE                    *outf;

        if  (!(pj = sub_check()))
                return;

        /* This is where we diverge according to whether we've got an in-memory script or not */

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
                        doerror($EH{No room for job});
                        return;
                }
        }
        else  {
                FILE  *inf;
                unsigned  inlng, outlng;
                char    inbuf[1024];

                /* We checked jobfile_name was set in sub_check */

                if  (!(inf = ldsv_open('r', pj->directory, pj->jobfile_name)))  {
                        doerror($EH{xmbtr cannot open job file});
                        return;
                }

                outf = goutfile(&jn);

                while  ((inlng = fread(inbuf, sizeof(char), sizeof(inbuf), inf)) != 0)  {
                        outlng = fwrite(inbuf, sizeof(char), inlng, outf);
                        if  (outlng != inlng)  {
                                pclose(inf);
                                fclose(outf);
                                unlink(tmpfl);
                                doerror($EH{No room for job});
                                return;
                        }
                }
                if  (pclose(inf) < 0)  {
                        fclose(outf);
                        unlink(tmpfl);
                        doerror($EH{xmbtr cannot open job file});
                        return;
                }
        }

        if  (fclose(outf) != 0)  {
                unlink(tmpfl);
                doerror($EH{No room for job});
                return;
        }

        if  (!submit_core(pj))
                return;

        if  (pj->Verbose)  {
                disp_arg[0] = jn;
                disp_str = title_of(pj->job);
                doinfo(disp_str[0]? $E{xmbtr job created ok title}: $E{xmbtr job created ok no title});
        }
        pj->nosubmit = 0;
}
