/* xbq_jvlist.c -- job and variable lists for gbch-xq

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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
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
#include "cmdint.h"
#include "btvar.h"
#include "btuser.h"
#include "shreq.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "gtk_lib.h"
#include "formats.h"
#include "optflags.h"
#include "xbq_ext.h"
#include "cfile.h"
#include "stringvec.h"
#ifdef  HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

static  char    Filename[] = __FILE__;

/* These are the default job format columns */

#define JNUM_COL        1
#define JQUEUE_COL      2
#define JQTIT_COL       3
#define JTITLE_COL      4
#define JPROGRESS_COL   5
#define JPID_COL        6
#define JUSER_COL       7
#define JGROUP_COL      8
#define JMODE_COL       9
#define JORIGHOST_COL   10
#define JEXPORT_COL     11
#define JREMRUN_COL     12
#define JINTERP_COL     13
#define JPRIO_COL       14
#define JLOAD_COL       15
#define JSTIME_COL      16
#define JFTIME_COL      17
#define JAVOID_COL      18
#define JHOLS_COL       19
#define JREPEAT_COL     20
#define JIFNP_COL       21
#define JSCOND_COL      22
#define JFCOND_COL      23
#define JSASS_COL       24
#define JFASS_COL       25
#define JUMASK_COL      26
#define JDIR_COL        27
#define JARGS_COL       28
#define JENV_COL        29
#define JREDIRS_COL     30
#define JEXITS_COL      31
#define JXIT_COL        32
#define JSIG_COL        33
#define JOTIME_COL      34
#define JITIME_COL      35
#define JSTTIME_COL     36
#define JENDTIME_COL    37
#define JDELTIME_COL    38
#define JRUNTIME_COL    39
#define JGRACETIME_COL  40
#define JAUTOKSIG_COL   41

/* These are the default var format columns */

#define VNAME_COL       1
#define VVAL_COL        2
#define VEXP_COL        3
#define VCLUST_COL      4
#define VCOMM_COL       5
#define VUSER_COL       6
#define VGROUP_COL      7
#define VMODE_COL       8

USHORT  jdefflds[] =  { SEQ_COL, JNUM_COL, JUSER_COL, JTITLE_COL, JINTERP_COL, JPRIO_COL, JLOAD_COL, JSTIME_COL, JSCOND_COL, JPROGRESS_COL };
USHORT  vdefflds[] =  { VNAME_COL, VVAL_COL, VEXP_COL, VCLUST_COL, VCOMM_COL, VUSER_COL, VGROUP_COL  };

#define NDEFFLDS     sizeof(jdefflds)/sizeof(USHORT)
#define NVDEFFLDS     sizeof(vdefflds)/sizeof(USHORT)

USHORT  *def_jobflds, *def_varflds;
int     ndef_jobflds, ndef_varflds;

struct  jvlist_elems    jlist_els[] =  {
        {       G_TYPE_UINT,    JVREND_TEXT,    -1,     $P{xbtq seq hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq jnum hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq queue hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq qtit hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq title hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq progress hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq pid hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq juname hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq jgname hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq jmode hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq orighost hdr}  },
        {       G_TYPE_BOOLEAN, JVREND_TOGGLE,  -1,     $P{xbtq jexport hdr}  },
        {       G_TYPE_BOOLEAN, JVREND_TOGGLE,  -1,     $P{xbtq jremrun hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq interp hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq prio hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq loadlev hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq time hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq fulltime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq avoid hdr}  },
        {       G_TYPE_BOOLEAN, JVREND_TOGGLE,  -1,     $P{xbtq hols hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq repeat hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq ifnp hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq scond hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq fcond hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq sass hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq fass hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq umask hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq dir hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq args hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq env hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq redirs hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq exits hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq exit hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq signal hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq otime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq itime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq sttime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq endtime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq deltime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq runtime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq gracetime hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq autoksig hdr}  },
};

struct  jvlist_elems    vlist_els[] =  {
        {       G_TYPE_UINT,    JVREND_TEXT,    -1,     $P{xbtq vseq hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq vname hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq vval hdr}  },
        {       G_TYPE_BOOLEAN, JVREND_TOGGLE,  -1,     $P{xbtq vexport hdr}  },
        {       G_TYPE_BOOLEAN, JVREND_TOGGLE,  -1,     $P{xbtq vclust hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq vcomment hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq vuname hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq vgname hdr}  },
        {       G_TYPE_STRING,  JVREND_TEXT,    -1,     $P{xbtq vmode hdr}  }
};

#define NUM_JOB_ELS     sizeof(jlist_els) / sizeof(struct jvlist_elems)
#define NUM_VAR_ELS     sizeof(vlist_els) / sizeof(struct jvlist_elems)

struct  fielddef  {
        GtkWidget               *view;
        struct  jvlist_elems    *list_els;
        unsigned                num_els;
};

struct  fielddef        jobflds = { NULL, jlist_els, NUM_JOB_ELS },
                        varflds = { NULL, vlist_els, NUM_VAR_ELS };

extern  Shipc   Oreq;

#define ppermitted(flg) (mypriv->btu_priv & flg)

static  char    *localrun;

static  HelpaltRef      progresslist;
char                    *rdeletemsg, *rretainmsg;
HelpaltRef              varexport_types;

extern const char * const condname[];
extern const char * const assname[];

extern  HelpaltRef      assnames, actnames;

#ifdef  HAVE_SYS_RESOURCE_H
extern  int     Max_files;
#endif

const char *qtitle_of(CBtjobRef jp)
{
        const   char    *title = title_of(jp);
        const   char    *colp;

        if  (!jobqueue  ||  !(colp = strchr(title, ':')))
                return  title;
        return  colp + 1;
}

/* Generate list of queue names in ql */

void  gen_qlist(struct stringvec *ql)
{
        int     reread = 0, jcnt;
        char    *saveru = (char *) 0, *saverg = (char *) 0, *savejq = (char *) 0;

        /* If we've got some restrictions in force, suspend them so we get everything */

        if  (Restru || Restrg || jobqueue)  {
                saveru = Restru;
                saverg = Restrg;
                savejq = jobqueue;
                Restru = (char *) 0;
                Restrg = (char *) 0;
                jobqueue = (char *) 0;
                Last_j_ser = 0;
                rjobfile(1);
                reread++;
        }

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

        /* Replace any restrictions we had in force */

        if  (reread)  {
                Restru = saveru;
                Restrg = saverg;
                jobqueue = savejq;
                Last_j_ser = 0;
                rjobfile(1);
        }
}

void  initmoremsgs()
{
#ifdef  HAVE_SYS_RESOURCE_H
        struct  rlimit  cfiles;
        getrlimit(RLIMIT_NOFILE, &cfiles);
        Max_files = cfiles.rlim_cur;
#endif
        if  (!(progresslist = helprdalt($Q{Job progress code})))  {
                disp_arg[9] = $Q{Job progress code};
                print_error($E{Missing alternative code});
        }

        rdeletemsg = gprompt($P{Delete at end abbrev});
        rretainmsg = gprompt($P{Retain at end abbrev});

        if  (!(repunit = helprdalt($Q{Repeat unit abbrev})))  {
                disp_arg[9] = $Q{Repeat unit abbrev};
                print_error($E{Missing alternative code});
        }
        if  (!(days_abbrev = helprdalt($Q{Weekdays})))  {
                disp_arg[9] = $Q{Weekdays};
                print_error($E{Missing alternative code});
        }
        if  (!(varexport_types = helprdalt($Q{xbtq var export types})))  {
                disp_arg[9] = $Q{xbtq var export types};
                print_error($E{Missing alternative code});
        }
        if  (!(ifnposses = helprdalt($Q{Ifnposses})))  {
                disp_arg[9] = $Q{Ifnposses};
                print_error($E{Missing alternative code});
        }
        exitcodename = gprompt($P{Assign exit code});
        signalname = gprompt($P{Assign signal});
        localrun = gprompt($P{Locally run});
        Const_val = helpnstate($N{Initial constant value});
        if  (!(assnames = helprdalt($Q{Assignment names})))  {
                disp_arg[9] = $Q{Assignment names};
                print_error($E{Missing alternative code});
        }
        if  (!(actnames = helprdalt($Q{Redirection type names})))  {
                disp_arg[9] = $Q{Redirection type names};
                print_error($E{Missing alternative code});
        }
}

void  set_tview_col(struct fielddef *, const int);

void  men_toggled(GtkMenuItem *item, struct fielddef *fd)
{
        unsigned  cnt;

        for  (cnt = 0;  cnt <  fd->num_els;  cnt++)
                if  (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(fd->list_els[cnt].menitem)))  {
                        if  (fd->list_els[cnt].colnum < 0)  {
                                set_tview_col(fd, cnt);
                                gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(fd->view), TRUE);
                                Dirty++;
                        }
                }
                else  if  (fd->list_els[cnt].colnum >= 0)  {
                        int     cn = fd->list_els[cnt].colnum;
                        unsigned   cnt2;
                        gtk_tree_view_remove_column(GTK_TREE_VIEW(fd->view), gtk_tree_view_get_column(GTK_TREE_VIEW(fd->view), cn));
                        fd->list_els[cnt].colnum = -1;
                        for  (cnt2 = 0;  cnt2 < fd->num_els;  cnt2++)
                                if  (fd->list_els[cnt2].colnum > cn)
                                        fd->list_els[cnt2].colnum--;
                        Dirty++;
                }
}

gboolean  hdr_clicked(GtkWidget *treeview, GdkEventButton *event, struct fielddef *fd)
{
        GtkWidget       *menu;
        unsigned        cnt;

        if  (event->type != GDK_BUTTON_PRESS  ||  event->button != 3)
                return  FALSE;

        menu = gtk_menu_new();

        for  (cnt = 0;  cnt <  fd->num_els;  cnt++)  {
                GtkWidget  *item = gtk_check_menu_item_new_with_label(fd->list_els[cnt].descr);
                if  (fd->list_els[cnt].colnum >= 0)
                        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
                g_signal_connect(item, "toggled", (GCallback) men_toggled, fd);
                fd->list_els[cnt].menitem = item;
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
                g_object_unref(G_OBJECT(item));
        }

        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
        return  TRUE;
}

void  set_tview_col(struct fielddef *fd, const int typenum)
{
        struct jvlist_elems  *lp = &fd->list_els[typenum];
        GtkCellRenderer  *renderer;
        GtkTreeViewColumn  *col;
        GtkWidget       *lab;
        int  colnum = 0;

        switch  (lp->rendtype)  {
        case  JVREND_TEXT:
                renderer = gtk_cell_renderer_text_new();
                colnum = gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fd->view), -1, lp->msgtext, renderer, "text", typenum, NULL);
                break;

        case  JVREND_PROGRESS:
                renderer = gtk_cell_renderer_progress_new();
                colnum = gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fd->view), -1, lp->msgtext, renderer, "value", typenum, NULL);
                break;

        case  JVREND_TOGGLE:
                renderer = gtk_cell_renderer_toggle_new();
                colnum = gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fd->view), -1, lp->msgtext, renderer, "active", typenum, NULL);
                break;
        }

        col = gtk_tree_view_get_column(GTK_TREE_VIEW(fd->view), colnum-1);
        gtk_tree_view_column_set_resizable(col, TRUE);
        lab = gtk_label_new(lp->msgtext);
        gtk_tree_view_column_set_widget(col, lab);
        gtk_widget_show(lab);
        g_signal_connect(gtk_widget_get_ancestor(lab, GTK_TYPE_BUTTON), "button-press-event", G_CALLBACK(hdr_clicked), fd);
        gtk_tree_view_column_set_alignment(col, 0.0);
        lp->colnum = colnum-1;
}

void  set_tview_attribs(struct fielddef *fd, USHORT *flds, unsigned nflds)
{
        unsigned  cnt;
        for  (cnt = 0; cnt < nflds;  cnt++)
                set_tview_col(fd, flds[cnt]);
}

/* Create job list window.
   The job list is returned in the global jwid.
   We also set up the global jlist_store for the Liststore. */

void  init_jlist_win()
{
        unsigned  cnt;
        GtkTreeSelection *sel;
        GType   coltypes[NUM_JOB_ELS];

        /* First get all the prompt messages */

        for  (cnt = 0;  cnt < NUM_JOB_ELS;  cnt++)  {
                jlist_els[cnt].msgtext = gprompt(jlist_els[cnt].msgcode);
                coltypes[cnt] = jlist_els[cnt].type;
        }

        for  (cnt = 0;  cnt < NUM_JOB_ELS;  cnt++)
                jlist_els[cnt].descr = gprompt(jlist_els[cnt].msgcode + $P{xbtq seq full descr} - $P{xbtq seq hdr});

        /*  Create job display treeview */

        jwid = gtk_tree_view_new();
        jobflds.view = jwid;
        jlist_store = gtk_list_store_newv(NUM_JOB_ELS, coltypes);
        gtk_tree_view_set_model(GTK_TREE_VIEW(jwid), GTK_TREE_MODEL(jlist_store));
        if  (def_jobflds)
                set_tview_attribs(&jobflds, def_jobflds, ndef_jobflds);
        else
                set_tview_attribs(&jobflds, jdefflds, NDEFFLDS);
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(jwid), TRUE);
}

/* Create variable list window.
   The var list is returned in the global pwid.
   We also set up the global plist_store for the Liststore. */

void  init_vlist_win()
{
        unsigned  cnt;
        GtkTreeSelection *sel;
        GType   coltypes[NUM_VAR_ELS];

        /* First get all the prompt messages */

        for  (cnt = 0;  cnt < NUM_VAR_ELS;  cnt++)  {
                vlist_els[cnt].msgtext = gprompt(vlist_els[cnt].msgcode);
                coltypes[cnt] = vlist_els[cnt].type;
        }

        for  (cnt = 0;  cnt < NUM_VAR_ELS;  cnt++)
                vlist_els[cnt].descr = gprompt(vlist_els[cnt].msgcode + $P{xbtq vseq full descr} - $P{xbtq vseq hdr});

        /*  Create var display treeview */

        vwid = gtk_tree_view_new();
        varflds.view = vwid;
        vlist_store = gtk_list_store_newv(NUM_VAR_ELS, coltypes);
        gtk_tree_view_set_model(GTK_TREE_VIEW(vwid), GTK_TREE_MODEL(vlist_store));

        /* Set up initial fields either from our default or as read in from config file */

        if  (def_varflds)       /* Read in */
                set_tview_attribs(&varflds, def_varflds, ndef_varflds);
        else                    /* Set from our default */
                set_tview_attribs(&varflds, vdefflds, NVDEFFLDS);

        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(vwid));
        gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
        gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(vwid), TRUE);
}

/* Common stuff on job list entry where we don't worry about whether job is readable */

static void  set_jlist_common(GtkTreeIter *iter, const int jnum)
{
        CBtjobhRef  jh = &jj_ptrs[jnum]->h;
        char    outbuffer[HOSTNSIZE+30];

        /* Fill in the things which don't depend on whether we can read it */

        gtk_list_store_set(jlist_store, iter,
                           SEQ_COL,             (guint) jnum,
                           JNUM_COL,            JOBH_NUMBER(jh),
                           JUSER_COL,           jh->bj_mode.o_user,
                           JGROUP_COL,          jh->bj_mode.o_group,
                           JORIGHOST_COL,       look_int_host(jh->bj_orighostid),
                           JEXPORT_COL,         jh->bj_jflags & BJ_EXPORT,
                           JREMRUN_COL,         jh->bj_jflags & BJ_REMRUNNABLE,
                           -1);

        /* Progress - show host if relevant */

        if  (jh->bj_progress == BJP_RUNNING  &&  jh->bj_runhostid != jh->bj_hostid)  {
                if  (jh->bj_runhostid == 0)
                        gtk_list_store_set(jlist_store, iter, JPROGRESS_COL, localrun, -1);
                else  {
                        sprintf(outbuffer, "%s:%s", disp_alt((int) jh->bj_progress, progresslist), look_host(jh->bj_runhostid));
                        gtk_list_store_set(jlist_store, iter, JPROGRESS_COL, outbuffer, -1);
                }
        }
        else
                gtk_list_store_set(jlist_store, iter, JPROGRESS_COL, disp_alt((int) jh->bj_progress, progresslist), -1);
}

static void  set_jlist_title(GtkTreeIter *iter, CBtjobRef jp)
{
        const  char     *tit = title_of(jp);
        const  char     *colp = strchr(tit, ':');

        if  (colp)  {
                GString *str = g_string_new_len(tit, colp-tit);
                if  (jobqueue)
                        tit = colp+1;
                gtk_list_store_set(jlist_store, iter,
                                   JQUEUE_COL,  str->str,
                                   JQTIT_COL,   colp+1,
                                   JTITLE_COL,  tit,
                                   -1);
                g_string_free(str, TRUE);
        }
        else
                gtk_list_store_set(jlist_store, iter,
                                   JQUEUE_COL,  "",
                                   JQTIT_COL,   tit,
                                   JTITLE_COL,  tit,
                                   -1);
}

static void  set_jlist_prillum(GtkTreeIter *iter, CBtjobRef jp)
{
        char    outbuffer[30];

        if  (jp->h.bj_pid != 0)
                sprintf(outbuffer, "%ld", (long) jp->h.bj_pid);
        else
                outbuffer[0] = '\0';
        gtk_list_store_set(jlist_store, iter,
                           JPID_COL,    outbuffer,
                           JINTERP_COL, jp->h.bj_cmdinterp,
                           -1);
        sprintf(outbuffer, "%u", jp->h.bj_pri);
        gtk_list_store_set(jlist_store, iter, JPRIO_COL, outbuffer, -1);
        sprintf(outbuffer, "%u", jp->h.bj_ll);
        gtk_list_store_set(jlist_store, iter, JLOAD_COL, outbuffer, -1);
        sprintf(outbuffer, "%.3o", jp->h.bj_umask);
        gtk_list_store_set(jlist_store, iter, JUMASK_COL, outbuffer, -1);
        /*  forgotten ulimit */
}

static void  set_jlist_timerep(GtkTreeIter *iter, CBtjobRef jp)
{
        CBtjobhRef  jh = &jp->h;
        char    outbuffer[40];

        if  (jh->bj_times.tc_istime)  {
                time_t  w = jh->bj_times.tc_nexttime;
                time_t  now = time((time_t *) 0);
                struct  tm  *t = localtime(&w);
                int     day = t->tm_mday, mon = t->tm_mon+1;
                int     hour = t->tm_hour, min = t->tm_min;
                int     sep = ':';
#ifdef  HAVE_TM_ZONE
                if  (t->tm_gmtoff <= -4 * 60 * 60)
#else
                if  (timezone >= 4 * 60 * 60)
#endif
                {
                        day = mon;
                        mon = t->tm_mday;
                }

                sprintf(outbuffer, "%.2d/%.2d/%.4d %.2d:%.2d", day, mon, t->tm_year+1900, hour, min);
                gtk_list_store_set(jlist_store, iter, JFTIME_COL, outbuffer, -1);

                if  (w < now  ||  w - now > 24*60*60)  {
                        hour = day;
                        min = mon;
                        sep = '/';
                }

                sprintf(outbuffer, "%.2d%c%.2d", hour, sep, min);
                gtk_list_store_set(jlist_store, iter,
                                   JSTIME_COL,  outbuffer,
                                   JIFNP_COL,   disp_alt(jh->bj_times.tc_nposs, ifnposses),
                                   JHOLS_COL,   jh->bj_times.tc_nvaldays & TC_HOLIDAYBIT,
                                   -1);

                /* Repeat unit */

                if  (jh->bj_times.tc_repeat < TC_MINUTES)
                        gtk_list_store_set(jlist_store, iter,
                                           JREPEAT_COL, jh->bj_times.tc_repeat == TC_DELETE? rdeletemsg: rretainmsg,
                                           JAVOID_COL,  "",
                                           -1);
                else  {
                        GString  *avstr;
                        int     nd, had = 0;

                        if  (jh->bj_times.tc_repeat == TC_MONTHSB  ||  jh->bj_times.tc_repeat == TC_MONTHSE)  {
                                int  mday = jh->bj_times.tc_mday;
                                if  (jh->bj_times.tc_repeat == TC_MONTHSE)  {
                                        static  char    month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
                                        month_days[1] = t->tm_year % 4 == 0? 29: 28;
                                        mday = month_days[t->tm_mon] - jh->bj_times.tc_mday;
                                        if  (mday <= 0)
                                                mday = 1;
                                }
                                sprintf(outbuffer, "%s:%ld:%d", disp_alt((int) (jh->bj_times.tc_repeat - TC_MINUTES), repunit),
                                        (long) jh->bj_times.tc_rate, mday);
                        }
                        else
                                sprintf(outbuffer, "%s:%ld", disp_alt((int) (jh->bj_times.tc_repeat - TC_MINUTES), repunit),
                                        (long) jh->bj_times.tc_rate);

                        /* Avoiding days (only if repeat) .... */

                        avstr = g_string_new(NULL);
                        for  (nd = 0;  nd < TC_NDAYS;  nd++)  {
                                if  (jh->bj_times.tc_nvaldays & (1 << nd))  {
                                        if  (had)
                                                g_string_append_c(avstr, ',');
                                        had++;
                                        g_string_append(avstr, disp_alt(nd, days_abbrev));
                                }
                        }

                        gtk_list_store_set(jlist_store, iter, JREPEAT_COL, outbuffer, JAVOID_COL, avstr->str, -1);
                        g_string_free(avstr, TRUE);
                }
        }
        else  {
                outbuffer[0] = '\0';
                gtk_list_store_set(jlist_store, iter,
                                   JSTIME_COL,  outbuffer,
                                   JFTIME_COL,  outbuffer,
                                   JREPEAT_COL, outbuffer,
                                   JAVOID_COL,  outbuffer,
                                   JIFNP_COL,   outbuffer,
                                   JHOLS_COL,   FALSE,
                                   -1);
        }
}

static void  set_jlist_cond(GtkTreeIter *iter, CBtjobRef jp)
{
        int     cnt;
        const   Btvar   *vp;
        const   Jcond   *cp;

        GString *scond = g_string_new(NULL), *fcond = g_string_new(NULL);

        for  (cnt = 0;  cnt < MAXCVARS;  cnt++)  {
                cp = &jp->h.bj_conds[cnt];
                if  (cp->bjc_compar == C_UNUSED)
                        break;
                if  (cnt != 0)  {
                        g_string_append_c(scond, ',');
                        g_string_append_c(fcond, ',');
                }

                vp = &Var_seg.vlist[cp->bjc_varind].Vent;
                if  (vp->var_id.hostid)  {
                        const  char  *hn = look_host(vp->var_id.hostid);
                        g_string_append(scond, hn);
                        g_string_append(fcond, hn);
                        g_string_append_c(scond, ':');
                        g_string_append_c(fcond, ':');
                }
                g_string_append(scond, vp->var_name);
                g_string_append(fcond, vp->var_name);
                g_string_append(fcond, condname[cp->bjc_compar-1]);
                if  (cp->bjc_value.const_type == CON_STRING)
                        g_string_append_printf(fcond, "\"%s\"", cp->bjc_value.con_un.con_string);
                else
                        g_string_append_printf(fcond, "%ld", (long) cp->bjc_value.con_un.con_long);
        }

        gtk_list_store_set(jlist_store, iter, JSCOND_COL, scond->str, JFCOND_COL, fcond->str, -1);
        g_string_free(scond, TRUE);
        g_string_free(fcond, TRUE);
}

static void  set_jlist_ass(GtkTreeIter *iter, CBtjobRef jp)
{
        int     cnt;
        const   Btvar   *vp;
        const   Jass    *ap;
        GString *sass = g_string_new(NULL), *fass = g_string_new(NULL);

        for  (cnt = 0;  cnt < MAXSEVARS;  cnt++)  {
                ap = &jp->h.bj_asses[cnt];
                if  (ap->bja_op == BJA_NONE)
                        break;
                if  (cnt != 0)  {
                        g_string_append_c(sass, ',');
                        g_string_append_c(fass, ',');
                }

                if  (ap->bja_op < BJA_SEXIT)  {
                        if  (ap->bja_flags & BJA_START)
                                g_string_append_c(fass, 'S');
                        if  (ap->bja_flags & BJA_REVERSE)
                                g_string_append_c(fass, 'R');
                        if  (ap->bja_flags & BJA_OK)
                                g_string_append_c(fass, 'N');
                        if  (ap->bja_flags & BJA_ERROR)
                                g_string_append_c(fass, 'E');
                        if  (ap->bja_flags & BJA_ABORT)
                                g_string_append_c(fass, 'A');
                        if  (ap->bja_flags & BJA_CANCEL)
                                g_string_append_c(fass, 'C');
                        g_string_append_c(fass, ':');
                }

                vp = &Var_seg.vlist[ap->bja_varind].Vent;

                if  (vp->var_id.hostid)  {
                        const  char  *hn = look_host(vp->var_id.hostid);
                        g_string_append(sass, hn);
                        g_string_append(fass, hn);
                        g_string_append_c(sass, ':');
                        g_string_append_c(fass, ':');
                }

                g_string_append(sass, vp->var_name);
                g_string_append(fass, vp->var_name);
                g_string_append(fass, assname[ap->bja_op-1]);

                if  (ap->bja_op >= BJA_SEXIT)
                        g_string_append(fass, ap->bja_op == BJA_SEXIT? exitcodename: signalname);
                else  if  (ap->bja_con.const_type == CON_STRING)
                        g_string_append_printf(fass, "\"%s\"", ap->bja_con.con_un.con_string);
                else
                        g_string_append_printf(fass, "%ld", (long) ap->bja_con.con_un.con_long);
        }

        gtk_list_store_set(jlist_store, iter, JSASS_COL, sass->str, JFASS_COL, fass->str, -1);
        g_string_free(sass, TRUE);
        g_string_free(fass, TRUE);
}

static void  set_jlist_dir(GtkTreeIter *iter, CBtjobRef jp)
{
        if  (jp->h.bj_direct >= 0)
                gtk_list_store_set(jlist_store, iter, JDIR_COL, &jp->bj_space[jp->h.bj_direct], -1);
        else
                gtk_list_store_set(jlist_store, iter, JDIR_COL, "", -1);
}

static void  set_jlist_args(GtkTreeIter *iter, CBtjobRef jp)
{
        unsigned        ac;
        GString         *argb = g_string_new(NULL);

        for  (ac = 0;  ac < jp->h.bj_nargs;  ac++)  {
                if  (ac != 0)
                        g_string_append_c(argb, ',');
                g_string_append(argb, ARG_OF(jp, ac));
        }
        gtk_list_store_set(jlist_store, iter, JARGS_COL, argb->str, -1);
        g_string_free(argb, TRUE);
}

static void  set_jlist_env(GtkTreeIter *iter, CBtjobRef jp)
{
        unsigned        ec;
        GString         *envb = g_string_new(NULL);

        for  (ec = 0;  ec < jp->h.bj_nenv;  ec++)  {
                char    *name, *value;
                ENV_OF(jp, ec, name, value);
                if  (ec != 0)
                        g_string_append_c(envb, ',');
                g_string_append(envb, name);
        }
        gtk_list_store_set(jlist_store, iter, JENV_COL, envb->str, -1);
        g_string_free(envb, TRUE);
}

static void  set_jlist_redirs(GtkTreeIter *iter, CBtjobRef jp)
{
        unsigned        rc;
        GString         *redb = g_string_new(NULL);

        for  (rc = 0;  rc < jp->h.bj_nredirs;  rc++)  {
                CRedirRef       rp = REDIR_OF(jp, rc);
                if  (rc != 0)
                        g_string_append_c(redb, ',');
                switch  (rp->action)  {
                case  RD_ACT_RD:
                        if  (rp->fd != 0)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append_c(redb, '<');
                        break;
                case  RD_ACT_WRT:
                        if  (rp->fd != 1)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append_c(redb, '>');
                        break;
                case  RD_ACT_APPEND:
                        if  (rp->fd != 1)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append(redb, ">>");
                        break;
                case  RD_ACT_RDWR:
                        if  (rp->fd != 0)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append(redb, "<>");
                        break;
                case  RD_ACT_RDWRAPP:
                        if  (rp->fd != 0)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append(redb, "<>>");
                        break;
                case  RD_ACT_PIPEO:
                        if  (rp->fd != 1)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append_c(redb, '|');
                        break;
                case  RD_ACT_PIPEI:
                        if  (rp->fd != 0)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append(redb, "<|");
                        break;
                case  RD_ACT_CLOSE:
                        if  (rp->fd != 1)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append(redb, ">&-");
                        continue;
                case  RD_ACT_DUP:
                        if  (rp->fd != 1)
                                g_string_append_printf(redb, "%d", rp->fd);
                        g_string_append_printf(redb, ">&%d", rp->arg);
                        continue;
                }
                g_string_append(redb, &jp->bj_space[rp->arg]);
        }

        gtk_list_store_set(jlist_store, iter, JREDIRS_COL, redb->str, -1);
        g_string_free(redb, TRUE);
}

static void  set_jlist_exits(GtkTreeIter *iter, CBtjobRef jp)
{
        CBtjobhRef  jh = &jp->h;
        char    outbuffer[30];

        sprintf(outbuffer, "N(%d,%d),E(%d,%d)", jh->bj_exits.nlower, jh->bj_exits.nupper, jh->bj_exits.elower, jh->bj_exits.eupper);
        gtk_list_store_set(jlist_store, iter, JEXITS_COL, outbuffer, -1);
        sprintf(outbuffer, "%u", jh->bj_lastexit >> 8);
        gtk_list_store_set(jlist_store, iter, JXIT_COL, outbuffer, -1);
        sprintf(outbuffer, "%u", jh->bj_lastexit & 255);
        gtk_list_store_set(jlist_store, iter, JSIG_COL, outbuffer, -1);
}

void  ptimes(GtkTreeIter *iter, const time_t now, const time_t w, const int col)
{
        struct  tm  *t = localtime(&w);
        int     t1 = t->tm_hour, t2 = t->tm_min;
        int     sep = ':';
        char    outbuf[8];

        if  (w == 0)  {
                gtk_list_store_set(jlist_store, iter, col, "", -1);
                return;
        }

        if  (w < now || w - now > 24 * 60 * 60)  {
                t1 = t->tm_mday;
                t2 = t->tm_mon+1;
#ifdef  HAVE_TM_ZONE
                if  (t->tm_gmtoff <= -4 * 60 * 60)
#else
                if  (timezone >= 4 * 60 * 60)
#endif
                {
                        t1 = t2;
                        t2 = t->tm_mday;
                }
                sep = '/';
        }
        sprintf(outbuf, "%.2d%c%.2d", t1, sep, t2);
        gtk_list_store_set(jlist_store, iter, col, outbuf, -1);
}

void  various_times(GtkTreeIter *iter, CBtjobRef jp)
{
        CBtjobhRef  jh = &jp->h;
        time_t  now = time((time_t *) 0);
        time_t  adjnow = (now / (24L * 60L * 60L)) * 24L * 60L * 60L;

        ptimes(iter, adjnow, jh->bj_time, JOTIME_COL);
        ptimes(iter, adjnow, jh->bj_stime, JSTTIME_COL);
        ptimes(iter, adjnow, jh->bj_etime, JENDTIME_COL);
        switch  (jh->bj_progress)  {
        default:
        case  BJP_FINISHED:
        case  BJP_ERROR:
        case  BJP_ABORTED:
                ptimes(iter, adjnow, jh->bj_etime, JITIME_COL);
                return;
        case  BJP_STARTUP1:
        case  BJP_STARTUP2:
        case  BJP_RUNNING:
                ptimes(iter, adjnow, jh->bj_stime, JITIME_COL);
                return;
        case  BJP_DONE:
                if  (jh->bj_etime)  {
                        ptimes(iter, adjnow, jh->bj_etime, JITIME_COL);
                        return;
                }
        case  BJP_CANCELLED:
                ptimes(iter, adjnow, jh->bj_time, JITIME_COL);
                return;
        case  BJP_NONE:
                if  (!jh->bj_times.tc_istime  &&  jh->bj_times.tc_nexttime < now)  {
                        ptimes(iter, adjnow, jh->bj_etime, JITIME_COL);
                        return;
                }
                ptimes(iter, adjnow, jh->bj_time, JITIME_COL);
                return;
        }
}

void  set_jlist_runt(GtkTreeIter *iter, CBtjobRef jp)
{
        CBtjobhRef  jh = &jp->h;
        char    outbuffer[50];

        if  (jh->bj_deltime != 0)  {
                sprintf(outbuffer, "%u", jh->bj_deltime);
                gtk_list_store_set(jlist_store, iter, JDELTIME_COL, outbuffer, -1);
        }
        else
                gtk_list_store_set(jlist_store, iter, JDELTIME_COL, "", -1);

        if  (jh->bj_runtime != 0)  {
                unsigned  long  hrs, mns, secs;
                hrs = jh->bj_runtime / 3600L;
                mns = jh->bj_runtime % 3600L;
                secs = mns % 60L;
                mns /= 60L;
                if  (hrs != 0)
                        sprintf(outbuffer, "%lu:%.2u:%.2u", hrs, (unsigned) mns, (unsigned) secs);
                else  if  (mns != 0)
                        sprintf(outbuffer, "%lu:%.2u", mns, (unsigned) secs);
                else
                        sprintf(outbuffer, "%lu", secs);
                gtk_list_store_set(jlist_store, iter, JRUNTIME_COL, outbuffer, -1);
                sprintf(outbuffer, "%u", jh->bj_autoksig);
                gtk_list_store_set(jlist_store, iter, JAUTOKSIG_COL, outbuffer, -1);
        }
        else
                gtk_list_store_set(jlist_store, iter, JRUNTIME_COL, "", JAUTOKSIG_COL, "", -1);

        if  (jh->bj_runtime != 0  &&  jh->bj_runon != 0)  {
                unsigned  mns, secs;
                mns = jh->bj_runon / 60;
                secs = jh->bj_runon % 60;
                if  (mns != 0)
                        sprintf(outbuffer, "%u:%.2u", mns, secs);
                else
                        sprintf(outbuffer, "%u", secs);
        }
        else
                outbuffer[0] = '\0';

        gtk_list_store_set(jlist_store, iter, JGRACETIME_COL, outbuffer, -1);
}

static unsigned  mfmtmode(char *buff, unsigned lng, const char *prefix, const unsigned md)
{
#ifdef  CHARSPRINTF
        int     cnt;
        sprintf(&buff[lng], "%s:", prefix);
        cnt = strlen(&buff[lng]);
        lng += cnt;
#else
        lng += sprintf(&buff[lng], "%s:", prefix);
#endif
        if  (md & BTM_READ)
                buff[lng++] = 'R';
        if  (md & BTM_WRITE)
                buff[lng++] = 'W';
        if  (md & BTM_SHOW)
                buff[lng++] = 'S';
        if  (md & BTM_RDMODE)
                buff[lng++] = 'M';
        if  (md & BTM_WRMODE)
                buff[lng++] = 'P';
        if  (md & BTM_UGIVE)
                buff[lng++] = 'U';
        if  (md & BTM_UTAKE)
                buff[lng++] = 'V';
        if  (md & BTM_GGIVE)
                buff[lng++] = 'G';
        if  (md & BTM_GTAKE)
                buff[lng++] = 'H';
        if  (md & BTM_DELETE)
                buff[lng++] = 'D';
        if  (md & BTM_KILL)
                buff[lng++] = 'K';
        buff[lng] = '\0';
        return  lng;
}

static void  set_mode_col(GtkListStore *store, GtkTreeIter *iter, const int col, CBtmodeRef md)
{
        char    outbuffer[50];

        outbuffer[0] = '\0';

        if  (mpermitted(md, BTM_RDMODE, mypriv->btu_priv))  {
                unsigned  lng = mfmtmode(outbuffer, 0, "U", md->u_flags);
                lng = mfmtmode(outbuffer, lng, ",G", md->g_flags);
                mfmtmode(outbuffer, lng, ",O", md->o_flags);
        }
        gtk_list_store_set(store, iter, col, outbuffer, -1);
}

void  set_jlist_store(const int jnum, GtkTreeIter *iter)
{
        CBtjobRef  jp = jj_ptrs[jnum];

        set_jlist_common(iter, jnum);

        if  (mpermitted(&jp->h.bj_mode, BTM_READ, mypriv->btu_priv))  {

                set_jlist_title(iter, jp);
                set_jlist_prillum(iter, jp);
                set_jlist_timerep(iter, jp);
                set_jlist_cond(iter, jp);
                set_jlist_ass(iter, jp);
                set_jlist_dir(iter, jp);
                set_jlist_args(iter, jp);
                set_jlist_env(iter, jp);
                set_jlist_redirs(iter, jp);
                set_jlist_exits(iter, jp);
                various_times(iter, jp);
                set_jlist_runt(iter, jp);
        }
        else  {
                /* Just fill everything with spaces */

                const  char     *outbuffer = "";
                gtk_list_store_set(jlist_store, iter,
                                   JQUEUE_COL,  outbuffer,
                                   JQTIT_COL,   outbuffer,
                                   JTITLE_COL,  outbuffer,
                                   JPID_COL,    outbuffer,
                                   JINTERP_COL, outbuffer,
                                   JPRIO_COL,   outbuffer,
                                   JLOAD_COL,   outbuffer,
                                   JSTIME_COL,  outbuffer,
                                   JFTIME_COL,  outbuffer,
                                   JAVOID_COL,  outbuffer,
                                   JHOLS_COL,   FALSE,
                                   JREPEAT_COL, outbuffer,
                                   JIFNP_COL,   outbuffer,
                                   JSCOND_COL,  outbuffer,
                                   JFCOND_COL,  outbuffer,
                                   JSASS_COL,   outbuffer,
                                   JFASS_COL,   outbuffer,
                                   JUMASK_COL,  outbuffer,
                                   JDIR_COL,    outbuffer,
                                   JARGS_COL,   outbuffer,
                                   JENV_COL,    outbuffer,
                                   JREDIRS_COL, outbuffer,
                                   JEXITS_COL,  outbuffer,
                                   JXIT_COL,    outbuffer,
                                   JSIG_COL,    outbuffer,
                                   JOTIME_COL,  outbuffer,
                                   JITIME_COL,  outbuffer,
                                   JSTTIME_COL, outbuffer,
                                   JENDTIME_COL,outbuffer,
                                   JDELTIME_COL,outbuffer,
                                   JRUNTIME_COL,outbuffer,
                                   JGRACETIME_COL,outbuffer,
                                   JAUTOKSIG_COL,outbuffer,
                                   -1);
        }

        set_mode_col(jlist_store, iter, JMODE_COL, &jp->h.bj_mode);
}

void  set_vlist_store(const int vnum, GtkTreeIter *iter)
{
        CBtvarRef vp = &vv_ptrs[vnum].vep->Vent;
        char    vbuff[BTV_COMMENT+BTV_NAME+BTC_VALUE+HOSTNSIZE];

        gtk_list_store_set(vlist_store, iter,
                           SEQ_COL,             (guint) vnum,
                           VUSER_COL,           vp->var_mode.o_user,
                           VGROUP_COL,          vp->var_mode.o_group,
                           -1);

        gtk_list_store_set(vlist_store, iter, VNAME_COL, VAR_NAME(vp), -1);

        if  (mpermitted(&vp->var_mode, BTM_READ, mypriv->btu_priv))  {
                CBtconRef       cp = &vp->var_value;
                if  (cp->const_type == CON_STRING)
                        sprintf(vbuff, "\"%s\"", cp->con_un.con_string);
                else
                        sprintf(vbuff, "%ld", (long) cp->con_un.con_long);
                gtk_list_store_set(vlist_store, iter, VVAL_COL, vbuff, VCOMM_COL, vp->var_comment, -1);
        }
        else
                gtk_list_store_set(vlist_store, iter, VVAL_COL, "", VCOMM_COL, "", -1);

        gtk_list_store_set(vlist_store, iter,
                           VEXP_COL,    vp->var_flags & VF_EXPORT,
                           VCLUST_COL,  vp->var_flags & VF_CLUSTER,
                           -1);

        set_mode_col(vlist_store, iter, VMODE_COL, &vp->var_mode);
}

void  init_jdisplay()
{
        unsigned  jcnt;

        rjobfile(1);

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                GtkTreeIter   iter;
                gtk_list_store_append(jlist_store, &iter);
                set_jlist_store(jcnt, &iter);
        }
}

void  init_vdisplay()
{
        unsigned  vcnt;

        rvarlist(1);

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                GtkTreeIter   iter;
                gtk_list_store_append(vlist_store, &iter);
                set_vlist_store(vcnt, &iter);
        }
}

void  job_redisplay()
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter;
        unsigned        jcnt;
        jobno_t         Cjobno = -1;
        netid_t         Chostno = 0;

        /*      Get currently selected job if any  */

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;
                CBtjobhRef  jp;
                gtk_tree_model_get(GTK_TREE_MODEL(jlist_store), &iter, SEQ_COL, &seq, -1);
                jp = &jj_ptrs[seq]->h;
                Cjobno = jp->bj_job;
                Chostno = jp->bj_hostid;
        }

        gtk_list_store_clear(jlist_store);
        rjobfile(1);

        for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                gtk_list_store_append(jlist_store, &iter);
                set_jlist_store(jcnt, &iter);
        }

        /* Re-find and select selected job if we had one */

        if  (Cjobno > 0)  {
                for  (jcnt = 0;  jcnt < Job_seg.njobs;  jcnt++)  {
                        CBtjobhRef  jp = &jj_ptrs[jcnt]->h;
                        if  (jp->bj_job == Cjobno  &&  jp->bj_hostid == Chostno)  {
                                if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(jlist_store), &iter, NULL, jcnt))
                                        gtk_tree_selection_select_iter(sel, &iter);
                                break;
                        }
                }
        }
}

void  var_redisplay()
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(vwid));
        GtkTreeIter     iter;
        unsigned        vcnt;
        char            Cvarname[BTV_NAME+1];
        netid_t         Chostno = 0;
        int             hadsel = 0;

        /*      Get currently selected job if any  */

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;
                CBtvarRef  vp;
                gtk_tree_model_get(GTK_TREE_MODEL(vlist_store), &iter, SEQ_COL, &seq, -1);
                vp = &vv_ptrs[seq].vep->Vent;
                strncpy(Cvarname, vp->var_name, BTV_NAME);
                Cvarname[BTV_NAME] = '\0';
                Chostno = vp->var_id.hostid;
                hadsel++;
        }

        gtk_list_store_clear(vlist_store);
        rvarlist(1);

        for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                gtk_list_store_append(vlist_store, &iter);
                set_vlist_store(vcnt, &iter);
        }

        if  (hadsel)  {
                for  (vcnt = 0;  vcnt < Var_seg.nvars;  vcnt++)  {
                        CBtvarRef  vp = &vv_ptrs[vcnt].vep->Vent;
                        if  (vp->var_id.hostid == Chostno  &&  strcmp(Cvarname, vp->var_name) == 0)  {
                                if  (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(vlist_store), &iter, NULL, vcnt))
                                        gtk_tree_selection_select_iter(sel, &iter);
                                break;
                        }
                }
        }
}

/* Structure to help us sort the columns into order from the fields */

struct  sortcols  {
        unsigned  colnum;       /* Column number that the view recognises */
        unsigned  fldnum;       /* Field number */
};

int  compare_cols(struct sortcols *a, struct sortcols *b)
{
        return  a->colnum < b->colnum? -1: a->colnum > b->colnum? 1: 0;
}

char *gen_colorder(struct jvlist_elems *lst, const int num)
{
        struct  sortcols        scl[40];                /* This is the most we currently need */
        struct  sortcols        *sp = scl, *fp;
        struct  jvlist_elems    *lp;
        char    rbuf[40*3], dbuf[4];

        for  (lp = lst;  lp < &lst[num];  lp++)
                if  (lp->colnum >= 0)  {
                        sp->colnum = lp->colnum;
                        sp->fldnum = lp - lst;
                        sp++;
                }

        qsort(QSORTP1 scl, sp - scl, sizeof(struct sortcols), QSORTP4 compare_cols);

        rbuf[0] = '\0';

        for  (fp = scl;  fp < sp;  fp++)  {
                sprintf(dbuf, ",%u", fp->fldnum);
                strcat(rbuf, dbuf);
        }
        return  stracpy(&rbuf[1]);
}

char *gen_jfmts()
{
        return  gen_colorder(jlist_els, NUM_JOB_ELS);
}

char *gen_vfmts()
{
        return  gen_colorder(vlist_els, NUM_VAR_ELS);
}

BtjobRef  getselectedjob(unsigned perm)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter;

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                BtjobRef  result;
                guint  seq;
                gtk_tree_model_get(GTK_TREE_MODEL(jlist_store), &iter, SEQ_COL, &seq, -1);
                result = jj_ptrs[seq];
                if  (perm == 0  ||  mpermitted(&result->h.bj_mode, perm, mypriv->btu_priv))
                        return  result;

                disp_arg[0] = result->h.bj_job;
                disp_str = qtitle_of(result);
                disp_str2 = result->h.bj_mode.o_user;
                doerror($EH{xmbtq job op not permitted});
                return  NULL;
        }

        if  (perm)
                doerror(Job_seg.njobs != 0? $EH{xmbtq no job selected}: $EH{xmbtq no jobs to select});
        return  NULL;
}

/* Check ok to delete command interpreter (not referred to in any job).  */

int  chk_okcidel(const int n)
{
        unsigned  jind = Job_seg.dptr->js_q_head;
        const   char    *ciname = Ci_list[n].ci_name;

        while  (jind != JOBHASHEND)  {
                if  (strcmp(Job_seg.jlist[jind].j.h.bj_cmdinterp, ciname) == 0)
                        return  0;
                jind = Job_seg.jlist[jind].q_nxt;
        }
        return  1;
}

BtvarRef  getselectedvar(unsigned perm)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(vwid));
        GtkTreeIter  iter;

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;
                BtvarRef  vp;

                gtk_tree_model_get(GTK_TREE_MODEL(vlist_store), &iter, SEQ_COL, &seq, -1);
                vp = &vv_ptrs[seq].vep->Vent;
                if  (perm == 0  ||  mpermitted(&vp->var_mode, perm, mypriv->btu_priv))
                        return  vp;

                disp_str = vp->var_name;
                disp_str2 = vp->var_mode.o_user;
                doerror($EH{xmbtq no var access perm});
                return  NULL;
        }
        if  (perm)
                doerror(Var_seg.nvars != 0? $EH{xmbtq no var selected}: $EH{xmbtq no vars to select});
        return  NULL;
}

/* For job conditions and assignments.  Check that specified variable
   name is valid, and if so return the strchr or -1 if not.  */

int  val_var(const char *name, const unsigned modeflag)
{
        int     first = 0, last = Var_seg.nvars, middle, s;
        const   char    *colp;
        BtvarRef        vp;
        netid_t hostid = 0;

        if  (!name)
                return  -1;

        if  ((colp = strchr(name, ':')) != (char *) 0)  {
                char    hname[HOSTNSIZE+1];
                s = colp - name;
                if  (s > HOSTNSIZE)
                        s = HOSTNSIZE;
                strncpy(hname, name, s);
                hname[s] = '\0';
                if  ((hostid = look_int_hostname(hname)) == -1)
                        return  -1;
                colp++;
        }
        else
                colp = name;

        while  (first < last)  {
                middle = (first + last) / 2;
                vp = &vv_ptrs[middle].vep->Vent;
                if  ((s = strcmp(colp, vp->var_name)) == 0)  {
                        if  (vp->var_id.hostid == hostid)  {
                                if  (mpermitted(&vp->var_mode, modeflag, mypriv->btu_priv))
                                        return  middle;
                                return  -1;
                        }
                        if  (vp->var_id.hostid < hostid)
                                first = middle + 1;
                        else
                                last = middle;
                }
                else  if  (s > 0)
                        first = middle + 1;
                else
                        last = middle;
        }
        return  -1;
}

/*
 ********************************************************************************
 *
 *                        Search stuff .....
 *
 ********************************************************************************
 */

static  char    *laststring;
static  char    last_wrap,
                last_isvar,
                last_match,
                searchtit = 1,
                searchuser = 1,
                searchgroup = 1,
                searchnam = 1,
                searchcomm = 1,
                searchval = 1;

struct  sparts  {
        GtkWidget  *stit, *suser, *sgroup, *sname, *scomm, *sval;
};

static int  smatch_in(const char *field)
{
        const   char    *tp, *mp;

        while  (*field)  {
                tp = field;
                mp = laststring;
                while  (*mp)  {
                        char    mch = *mp, fch = *tp;
                        if  (!last_match)  {
                                mch = toupper(mch);
                                fch = toupper(fch);
                        }
                        if  (*mp != '.'  &&  mch != fch)
                                goto  ng;
                        mp++;
                        tp++;
                }
                return  1;
        ng:
                field++;
        }
        return  0;
}

static int  foundin_job(const int n)
{
        BtjobRef        jp = jj_ptrs[n];

        if  (searchtit  &&  smatch_in(qtitle_of(jp)))
                return  1;
        if  (searchuser  &&  smatch_in(jp->h.bj_mode.o_user))
                return  1;
        if  (searchgroup  &&  smatch_in(jp->h.bj_mode.o_group))
                return  1;
        return  0;
}

static int  foundin_var(const int n)
{
        BtvarRef        vv = &vv_ptrs[n].vep->Vent;

        if  (searchuser  &&  smatch_in(vv->var_mode.o_user))
                return  1;
        if  (searchgroup  &&  smatch_in(vv->var_mode.o_group))
                return  1;
        if  (searchnam  &&  smatch_in(vv->var_name))
                return  1;
        if  (searchcomm  &&  smatch_in(vv->var_comment))
                return  1;
        if  (searchval)  {
                if  (vv->var_value.const_type == CON_LONG)  {
                        char    nbuf[20];
                        sprintf(nbuf, "%ld", (long) vv->var_value.con_un.con_long);
                        if  (smatch_in(nbuf))
                                return  1;
                }
                else  if  (smatch_in(vv->var_value.con_un.con_string))
                        return  1;
        }
        return  0;
}

static void  jsearch_result(const int jnum)
{
        GtkTreeIter  iter;
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreePath  *path;

        gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(jlist_store), &iter, NULL, jnum);
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(jlist_store), &iter);
        gtk_tree_selection_select_path(sel, path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(jwid), path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(path);
}

static void  execute_jsearch(const int isback)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(jwid));
        GtkTreeIter  iter;
        int  jnum;

        /* Save messing about if no jobs there */

        if  (Job_seg.njobs == 0)  {
                doerror($EH{xmbtq no jobs to select});
                return;
        }

        /* If we have a selected job start from just before or just after that one */

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;

                /* Get which one from the sequence number */

                gtk_tree_model_get(GTK_TREE_MODEL(jlist_store), &iter, SEQ_COL, &seq, -1);
                jnum = seq;     /* Easier working from signed */

                if  (isback)  {
                        for  (jnum--;  jnum >= 0;  jnum--)  {
                                if  (foundin_job(jnum))  {
                                        jsearch_result(jnum);
                                        return;
                                }
                        }

                        if  (last_wrap)  {
                                for  (jnum = Job_seg.njobs-1;  jnum >= 0;  jnum--)  {
                                        if  (foundin_job(jnum))  {
                                                jsearch_result(jnum);
                                                return;
                                        }
                                }
                        }
                }
                else  {
                        for  (jnum++;  (unsigned) jnum < Job_seg.njobs;  jnum++)  {
                                if  (foundin_job(jnum))  {
                                        jsearch_result(jnum);
                                        return;
                                }
                        }
                        if  (last_wrap)  {
                                for  (jnum = 0;  (unsigned) jnum < Job_seg.njobs;  jnum++)  {
                                        if  (foundin_job(jnum))  {
                                                jsearch_result(jnum);
                                                return;
                                        }
                                }
                        }
                }
        }
        else  if  (isback)  {
                for  (jnum = Job_seg.njobs-1;  jnum >= 0;  jnum--)  {
                        if  (foundin_job(jnum))  {
                                jsearch_result(jnum);
                                return;
                        }
                }
        }
        else  {
                for  (jnum = 0;  (unsigned) jnum < Job_seg.njobs;  jnum++)  {
                        if  (foundin_job(jnum))  {
                                jsearch_result(jnum);
                                return;
                        }
                }
        }

        doerror(isback? $EH{xmbtq job not found backw}: $EH{xmbtq job not found forw});
}

static void  vsearch_result(const int vnum)
{
        GtkTreeIter  iter;
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(vwid));
        GtkTreePath  *path;

        gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(jlist_store), &iter, NULL, vnum);
        path = gtk_tree_model_get_path(GTK_TREE_MODEL(jlist_store), &iter);
        gtk_tree_selection_select_path(sel, path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(vwid), path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(path);
}

static void  execute_vsearch(const int isback)
{
        GtkTreeSelection  *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(vwid));
        GtkTreeIter  iter;
        int  vnum;

        /* Save messing about if no vars there
           This "cannot happen"  */

        if  (Var_seg.nvars == 0)  {
                doerror($EH{xmbtq no vars to select});
                return;
        }

        /* If we have a selected var start from just before or just after that one */

        if  (gtk_tree_selection_get_selected(sel, NULL, &iter))  {
                guint  seq;

                /* Get which one from the sequence number */

                gtk_tree_model_get(GTK_TREE_MODEL(vlist_store), &iter, SEQ_COL, &seq, -1);
                vnum = seq;     /* Easier working from signed */

                if  (isback)  {
                        for  (vnum--;  vnum >= 0;  vnum--)  {
                                if  (foundin_var(vnum))  {
                                        vsearch_result(vnum);
                                        return;
                                }
                        }

                        if  (last_wrap)  {
                                for  (vnum = Var_seg.nvars-1;  vnum >= 0;  vnum--)  {
                                        if  (foundin_var(vnum))  {
                                                vsearch_result(vnum);
                                                return;
                                        }
                                }
                        }
                }
                else  {
                        for  (vnum++;  (unsigned) vnum < Var_seg.nvars;  vnum++)  {
                                if  (foundin_var(vnum))  {
                                        vsearch_result(vnum);
                                        return;
                                }
                        }
                        if  (last_wrap)  {
                                for  (vnum = 0;  (unsigned) vnum < Var_seg.nvars;  vnum++)  {
                                        if  (foundin_var(vnum))  {
                                                vsearch_result(vnum);
                                                return;
                                        }
                                }
                        }
                }
        }
        else  if  (isback)  {
                for  (vnum = Var_seg.nvars-1;  vnum >= 0;  vnum--)  {
                        if  (foundin_var(vnum))  {
                                vsearch_result(vnum);
                                return;
                        }
                }
        }
        else  {
                for  (vnum = 0;  (unsigned) vnum < Var_seg.nvars;  vnum++)  {
                        if  (foundin_var(vnum))  {
                                vsearch_result(vnum);
                                return;
                        }
                }
        }

        doerror(isback? $EH{xmbtq var not found backw}: $EH{xmbtq var not found forw});
}

static void  execute_search(const int isback)
{
        if  (last_isvar)
                execute_vsearch(isback);
        else
                execute_jsearch(isback);
}

static void  jorv_flip(GtkWidget *jorv, struct sparts *sp)
{
        if  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jorv)))  {
                gtk_widget_set_sensitive(sp->stit, FALSE);
                gtk_widget_set_sensitive(sp->sname, TRUE);
                gtk_widget_set_sensitive(sp->scomm, TRUE);
                gtk_widget_set_sensitive(sp->sval, TRUE);
        }
        else  {
                gtk_widget_set_sensitive(sp->stit, TRUE);
                gtk_widget_set_sensitive(sp->sname, FALSE);
                gtk_widget_set_sensitive(sp->scomm, FALSE);
                gtk_widget_set_sensitive(sp->sval, FALSE);
        }
}

void  cb_search()
{
        GtkWidget  *dlg, *hbox, *sstringw, *jorv, *backw, *matchw, *wrapw;
        char    *pr;
        struct  sparts  sp;

        pr = gprompt($P{xbtq search jvlist});
        dlg = gtk_dialog_new_with_buttons(pr,
                                          GTK_WINDOW(toplevel),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL, NULL);
        free(pr);

        /* String to search for */

        hbox = gtk_hbox_new(FALSE, DEF_DLG_HPAD);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, DEF_DLG_VPAD);
        gtk_box_pack_start(GTK_BOX(hbox), gprompt_label($P{xbtq search str}), FALSE, FALSE, DEF_DLG_HPAD);
        sstringw = gtk_entry_new();
        if  (laststring)
                gtk_entry_set_text(GTK_ENTRY(sstringw), laststring);
        gtk_box_pack_start(GTK_BOX(hbox), sstringw, FALSE, FALSE, DEF_DLG_HPAD);

        jorv = gprompt_checkbutton($P{xbtq search jorv});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), jorv, FALSE, FALSE, DEF_DLG_VPAD);

        sp.stit = gprompt_checkbutton($P{xbtq search title});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.stit, FALSE, FALSE, DEF_DLG_VPAD);

        sp.suser = gprompt_checkbutton($P{xbtq search user});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.suser, FALSE, FALSE, DEF_DLG_VPAD);

        sp.sgroup = gprompt_checkbutton($P{xbtq search group});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.sgroup, FALSE, FALSE, DEF_DLG_VPAD);

        sp.sname = gprompt_checkbutton($P{xbtq search vname});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.sname, FALSE, FALSE, DEF_DLG_VPAD);

        sp.scomm = gprompt_checkbutton($P{xbtq search vcomment});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.scomm, FALSE, FALSE, DEF_DLG_VPAD);

        sp.sval = gprompt_checkbutton($P{xbtq search vval});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), sp.sval, FALSE, FALSE, DEF_DLG_VPAD);

        backw = gprompt_checkbutton($P{xbtq search backw});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), backw, FALSE, FALSE, DEF_DLG_VPAD);

        wrapw = gprompt_checkbutton($P{xbtq search wrap});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), wrapw, FALSE, FALSE, DEF_DLG_VPAD);

        matchw = gprompt_checkbutton($P{xbtq search match});
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), matchw, FALSE, FALSE, DEF_DLG_VPAD);

        if  (last_isvar)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(jorv), TRUE);
        if  (last_wrap)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrapw), TRUE);
        if  (last_match)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(matchw), TRUE);
        if  (searchtit)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.stit), TRUE);
        if  (searchuser)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.suser), TRUE);
        if  (searchgroup)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.sgroup), TRUE);
        if  (searchnam)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.sname), TRUE);
        if  (searchcomm)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.scomm), TRUE);
        if  (searchval)
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sp.sval), TRUE);

        /* Turn off inappropriate ones according to whether job or printer selected */

        if  (last_isvar)
                gtk_widget_set_sensitive(sp.stit, FALSE);
        else  {
                gtk_widget_set_sensitive(sp.sname, FALSE);
                gtk_widget_set_sensitive(sp.scomm, FALSE);
                gtk_widget_set_sensitive(sp.sval, FALSE);
        }

        g_signal_connect(G_OBJECT(jorv), "toggled", G_CALLBACK(jorv_flip), (gpointer) &sp);

        gtk_widget_show_all(dlg);

        while  (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)  {
                const  char  *news = gtk_entry_get_text(GTK_ENTRY(sstringw));
                gboolean  isvar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(jorv));
                gboolean  isback = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(backw));

                if  (strlen(news) == 0)  {
                        doerror($EH{xmbtq null search string});
                        continue;
                }
                if  (laststring)
                        free(laststring);
                laststring = stracpy(news);
                searchtit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.stit))? 1: 0;
                searchuser = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.suser))? 1: 0;
                searchgroup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.sgroup))? 1: 0;
                searchnam = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.sname))? 1: 0;
                searchcomm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.scomm))? 1: 0;
                searchval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sp.sval))? 1: 0;
                last_isvar = isvar? 1: 0;
                last_wrap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrapw))? 1: 0;
                last_match = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(matchw))? 1: 0;
                if  (isvar)  {
                        if  (!(searchuser || searchgroup || searchnam || searchcomm || searchval))  {
                                doerror($EH{xmbtq no var search atts});
                                continue;
                        }
                }
                else  if  (!(searchtit || searchuser || searchgroup))  {
                        doerror($EH{xmbtq no job search atts});
                        continue;
                }
                execute_search(isback);
                break;
        }
        gtk_widget_destroy(dlg);
}

void  cb_rsearch(GtkAction *action)
{
        const   char    *act = gtk_action_get_name(action);

        if  (!laststring)  {
                doerror($EH{xmbtq no search string yet});
                return;
        }

        execute_search(act[0] == 'R');
}
