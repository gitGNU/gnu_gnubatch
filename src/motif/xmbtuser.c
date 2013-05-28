/* xmbtuser.c -- main module for gbch-xmuser

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
static  char    rcsid1[] = "@(#) $Id: xmbtuser.c,v 1.6 2009/02/18 06:51:32 toadwarble Exp $";           /* We use these in the about message */
static  char    rcsid2[] = "@(#) $Revision: 1.9 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
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
#include <Xm/CascadeB.h>
#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/SeparatoGP.h>
#include <Xm/TextF.h>
#include "incl_sig.h"
#include "defaults.h"
#include "files.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "helpargs.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "btmode.h"
#include "btuser.h"
#include "xm_commlib.h"
#include "xmbtu_ext.h"
#include "xmmenu.h"

static  char    Filename[] = __FILE__;

char    *Curr_pwd;

unsigned        loadstep;

int     hchanges,       /* Had changes to default */
        uchanges;       /* Had changes to user(s) */

char            alphsort;
BtuserRef       ulist;
static  char    *defhdr;

static  char    *urestrict,     /* Restrict  */
                *grestrict;

struct  privabbrev      privnames[] =
        {{      BTM_RADMIN,     $P{Read adm abbr},              WORKW_RADMIN,   "radmin"        },
         {      BTM_WADMIN,     $P{Write adm abbr},             WORKW_WADMIN,   "wadmin"        },
         {      BTM_CREATE,     $P{Create entry abbr},          WORKW_CREATE,   "create"        },
         {      BTM_SPCREATE,   $P{Special create abbr},        WORKW_SPCREATE, "spcreate"      },
         {      BTM_SSTOP,      $P{Stop sched abbr},            WORKW_SSTOP,    "sstop"         },
         {      BTM_UMASK,      $P{Change default modes abbr},  WORKW_UMASK,    "cdeflt"        },
         {      BTM_ORP_UG,     $P{Combine user group abbr},    WORKW_ORUG,     "orug"          },
         {      BTM_ORP_UO,     $P{Combine user other abbr},    WORKW_ORUO,     "oruo"          },
         {      BTM_ORP_GO,     $P{Combine group other abbr},   WORKW_ORGO,     "orgo"          }};

#define USNAM_P         0
#define GRPNAM_P        8
#define DEFP_P          16
#define MINP_P          20
#define MAXP_P          24
#define MAXLL_P         28
#define TOTLL_P         34
#define SPECLL_P        40
#define P_P             46

/* X Stuff */

XtAppContext    app;
Display         *dpy;

Widget  toplevel,       /* Main window */
        dwid,           /* Default list */
        uwid;           /* User scroll list */

static  Widget  panedw,         /* Paned window to stick rest in */
                menubar;        /* Menu */

typedef struct  {
        Boolean tit_pres;
        Boolean footer_pres;
        String  onlyuser;
        String  onlygroup;
        int     loadstep;
        int     rtime, rint;
}  vrec_t;

static void  cb_about();
static void  cb_quit(Widget, int);
static void  cb_saveopts(Widget);

static  XtResource      resources[] = {
        { "titlePresent", "TitlePresent", XtRBoolean, sizeof(Boolean),
                  XtOffsetOf(vrec_t, tit_pres), XtRImmediate, False },
        { "footerPresent", "FooterPresent", XtRBoolean, sizeof(Boolean),
                  XtOffsetOf(vrec_t, footer_pres), XtRImmediate, False },
        { "loadStep", "LoadStep", XtRInt, sizeof(int),
                  XtOffsetOf(vrec_t, loadstep),  XtRImmediate, (XtPointer) 100 },
        { "onlyUser", "OnlyUser", XtRString, sizeof(String),
                  XtOffsetOf(vrec_t, onlyuser), XtRString, "" },
        { "onlyGroup", "OnlyGroup", XtRString, sizeof(String),
                  XtOffsetOf(vrec_t, onlygroup), XtRString, "" },
        { "repeatTime", "RepeatTime", XtRInt, sizeof(int),
                  XtOffsetOf(vrec_t, rtime), XtRImmediate, (XtPointer) 500 },
        { "repeatInt", "RepeatInt", XtRInt, sizeof(int),
                  XtOffsetOf(vrec_t, rint), XtRImmediate, (XtPointer) 100 }};

#define SORTRESOURCE    3       /* Offset of resource for sorting */

static  casc_button
opt_casc[] = {
        {       ITEM,   "Disporder",    cb_disporder,   0       },
        {       ITEM,   "Saveopts",     cb_saveopts,    0       },
        {       DSEP    },
        {       ITEM,   "Quit", cb_quit,        0       }},
def_casc[] = {
        {       ITEM,   "dpri",         cb_pris,        0       },
        {       ITEM,   "dloadl",       cb_loadlev,     0       },
        {       ITEM,   "dmode",        cb_mode,        0       },
        {       ITEM,   "dpriv",        cb_priv,        0       },
        {       SEP     },
        {       ITEM,   "defcpy",       cb_copydef,     0       }},
user_casc[] = {
        {       ITEM,   "upri",         cb_pris,        1       },
        {       ITEM,   "uloadl",       cb_loadlev,     1       },
        {       ITEM,   "umode",        cb_mode,        1       },
        {       ITEM,   "upriv",        cb_priv,        1       },
        {       SEP     },
        {       ITEM,   "ucpy",         cb_copydef,     1       }},
search_casc[] = {
        {       ITEM,   "Search",       cb_srchfor,     0       },
        {       ITEM,   "Searchforw",   cb_rsrch,       0       },
        {       ITEM,   "Searchback",   cb_rsrch,       1       }},
help_casc[] = {
        {       ITEM,   "Help",         dohelp,         $H{xmbtuser main menu}  },
        {       ITEM,   "Helpon",       cb_chelp,       0       },
        {       SEP     },
        {       ITEM,   "About",        cb_about,       0       }};

static  pull_button
        opt_button = {
                "Options",      XtNumber(opt_casc),     $H{xmbtuser options menu},      opt_casc        },
        def_button = {
                "Defaults",     XtNumber(def_casc),     $H{xmbtuser defaults menu},     def_casc        },
        user_button = {
                "Users",        XtNumber(user_casc),    $H{xmbtuser user menu},         user_casc       },
        srch_button = {
                "Search",       XtNumber(search_casc),  $H{xmbtuser search menu},       search_casc     },
        help_button = {
                "Help",         XtNumber(help_casc),    $H{xmbtuser help menu},         help_casc,      1       };

static  pull_button     *menlist[] = {
        &opt_button, &def_button, &user_button, &srch_button, &help_button
};

#if     defined(HAVE_MEMCPY) && !defined(HAVE_BCOPY)

/* Define our own bcopy and bzero because X uses these in places and
   we don't want to include some -libucb which pulls in funny
   sprintfs etc */

void  bcopy(void *from, void *to, unsigned count)
{
        memcpy(to, from, count);
}
void  bzero(void *to, unsigned count)
{
        memset(to, '\0', count);
}
#endif

/* Different sorts of sorts (of sorts) */

int  sort_u(BtuserRef a, BtuserRef b)
{
        return  strcmp(prin_uname((uid_t) a->btu_user), prin_uname((uid_t) b->btu_user));
}

int  sort_g(BtuserRef a, BtuserRef b)
{
        gid_t   ga, gb;
        char    *au, *bu;

        au = prin_uname((uid_t) a->btu_user);           /*  Sets lastgid */
        ga = lastgid;
        bu = prin_uname((uid_t) b->btu_user);
        gb = lastgid;
        if  (ga == gb)
                return  strcmp(au, bu);
        return  strcmp(prin_gname((gid_t) ga), prin_gname((gid_t) gb));
}

int  sort_id(BtuserRef a, BtuserRef b)
{
        return  (ULONG) a->btu_user > (ULONG) b->btu_user? 1: (ULONG) a->btu_user < (ULONG) b->btu_user? -1: 0;
}

/* Don't put exit as a callback or we'll get some weird exit code
   based on a Widget pointer.  */

static void  cb_quit(Widget w, int n)
{
        if  (uchanges || hchanges)  {
                if  (alphsort != SRT_NONE)
                        qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_id);
                putbtulist(ulist);
        }
        exit(n);
}

static  char  *confline_arg;

static  void  save_confline_opt(FILE *fp, const char *vname)
{
        fprintf(fp, "%s=%s\n", vname, confline_arg);
}

static void  cb_saveopts(Widget w)
{
        int     items;
        Dimension  wid;
        char    digbuf[20];

        if  (!Confirm(w, $PH{Confirm write options}))
                return;

        disp_str = "(Home)";
        confline_arg = digbuf;
        digbuf[0] = alphsort == SORT_USER? 'U': alphsort == SORT_GROUP? 'G' : 'N';
        digbuf[1] = '\0';
        proc_save_opts((const char *) 0, "XMBTUSORT", save_confline_opt);
        XtVaGetValues(uwid, XmNwidth, &wid, XmNvisibleItemCount, &items, NULL);
        sprintf(digbuf, "%d", wid);
        confline_arg = digbuf;
        proc_save_opts((const char *) 0, "XMBTUWIDTH", save_confline_opt);
        sprintf(digbuf, "%d", items);
        proc_save_opts((const char *) 0, "XMBTUITEMS", save_confline_opt);
}

/* For when we run out of memory.....  */

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

static void  cb_about()
{
        Widget          dlg;
        char    buf[sizeof(rcsid1) + sizeof(rcsid2) + 2];
        sprintf(buf, "%s\n%s", rcsid1, rcsid2);
        dlg = XmCreateInformationDialog(uwid, "about", NULL, 0);
        XtVaSetValues(dlg,
                      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
                      XtVaTypedArg, XmNmessageString, XmRString, buf, strlen(buf),
                      NULL);
        XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_CANCEL_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));
        XtManageChild(dlg);
        XtPopup(XtParent(dlg), XtGrabNone);
}

Widget  BuildPulldown(Widget menub, pull_button *item)
{
        int     cnt;
        Widget  pulldown, cascade, button;

        pulldown = XmCreatePulldownMenu(menub, "pulldown", NULL, 0);
        cascade = XtVaCreateManagedWidget(item->pull_name, xmCascadeButtonWidgetClass, menub,
                                          XmNsubMenuId, pulldown, NULL);

        if  (item->helpnum != 0)
                XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(item->helpnum));

        for  (cnt = 0;  cnt < item->nitems;  cnt++)  {
                char    sname[20];
                casc_button     *cb = &item->items[cnt];
                switch  (cb->type)  {
                case  SEP:
                        sprintf(sname, "separator%d", cnt);
                        button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown, NULL);
                        continue;
                case  DSEP:
                        sprintf(sname, "separator%d", cnt);
                        button = XtVaCreateManagedWidget(sname, xmSeparatorGadgetClass, pulldown,
                                                         XmNseparatorType, XmDOUBLE_LINE, NULL);
                        continue;
                case  ITEM:
                        button = XtVaCreateManagedWidget(cb->name, xmPushButtonGadgetClass, pulldown, NULL);
                        if  (cb->callback)
                                XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) cb->callback, INT_TO_XTPOINTER(cb->callback_data));
                        continue;
                }
        }
        if  (item->ishelp)
                return  cascade;
        return  NULL;
}

static void  setup_macros(Widget menub, const int helpcode, const int helpbase, char *pullname, XtCallbackProc macroproc)
{
        int     cnt, had = 0;
        Widget  pulldown, cascade, button;
        char    *macroprmpt[10];

        for  (cnt = 0;  cnt < 10;  cnt++)
                if  ((macroprmpt[cnt] = helpprmpt(helpbase+cnt)))
                        had++;

        if  (had <= 0)
                return;

        pulldown = XmCreatePulldownMenu(menub, pullname, NULL, 0);
        cascade = XtVaCreateManagedWidget(pullname, xmCascadeButtonWidgetClass, menub, XmNsubMenuId, pulldown, NULL);
        XtAddCallback(cascade, XmNhelpCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        for  (cnt = 0;  cnt < 10;  cnt++)  {
                char    sname[20];
                if  (!macroprmpt[cnt])
                        continue;
                free(macroprmpt[cnt]);
                sprintf(sname, "macro%d", cnt);
                button = XtVaCreateManagedWidget(sname, xmPushButtonGadgetClass, pulldown, NULL);
                XtAddCallback(button, XmNactivateCallback, macroproc, INT_TO_XTPOINTER(cnt));
        }
}

static void  setup_menus()
{
        int                     cnt;
        XtWidgetGeometry        size;
        Widget                  helpw;

        menubar = XmCreateMenuBar(panedw, "menubar", NULL, 0);

        /* Get rid of resize button for menubar */

        size.request_mode = CWHeight;
        XtQueryGeometry(menubar, NULL, &size);
        XtVaSetValues(menubar, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

        for  (cnt = 0;  cnt < XtNumber(menlist);  cnt++)
                if  ((helpw = BuildPulldown(menubar, menlist[cnt])))
                        XtVaSetValues(menubar, XmNmenuHelpWidget, helpw, NULL);

        setup_macros(menubar,
                     $H{xmbtuser user macro menu},
                     $PH{Job or User macro},
                     "usermacro", (XtCallbackProc) cb_macrou);

        XtManageChild(menubar);
}

static void  maketitle(char *tname)
{
        Widget                  labv;
        XtWidgetGeometry        size;

        labv = XtVaCreateManagedWidget(tname, xmLabelWidgetClass, panedw, NULL);
        size.request_mode = CWHeight;
        XtQueryGeometry(labv, NULL, &size);
        XtVaSetValues(labv, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
}

#include "xmbtuser.bm"

static void  wstart(int argc, char **argv)
{
        int     cnt, uwidth = -1, uitems = -1;
        char    *arg;
        vrec_t  vrec;
        Pixmap  bitmap;
        XtWidgetGeometry        size;

        toplevel = XtVaAppInitialize(&app, "GBATCH", NULL, 0, &argc, argv, NULL, NULL);
        if  (argc > 1)  {
                if  (strcmp(argv[1], "*") != 0)
                        urestrict = stracpy(argv[1]);
                if  (argc > 2  &&  strcmp(argv[2], "*") != 0)
                        grestrict = stracpy(argv[2]);
        }
        XtGetApplicationResources(toplevel, &vrec, resources, XtNumber(resources), NULL, 0);
        bitmap = XCreatePixmapFromBitmapData(dpy = XtDisplay(toplevel),
                                             RootWindowOfScreen(XtScreen(toplevel)),
                                             xmbtuser_bits, xmbtuser_width, xmbtuser_height, 1, 0, 1);
        XtVaSetValues(toplevel, XmNiconPixmap, bitmap, NULL);

        /* Set up parameters from resources */

        if  ((arg = optkeyword("XMBTUSORT")))  {
                switch  (arg[0])  {
                default:
                        alphsort = SORT_NONE;   break;
                case  'u':case 'U':
                        alphsort = SORT_USER;   break;
                case  'g':case 'G':
                        alphsort = SORT_GROUP;  break;
                }
                free(arg);
        }
        if  ((arg = optkeyword("XMBTUWIDTH")))  {
                uwidth = atoi(arg);
                free(arg);
        }
        if  ((arg = optkeyword("XMBTUITEMS")))  {
                uitems = atoi(arg);
                free(arg);
        }

        arr_rtime = vrec.rtime;
        arr_rint = vrec.rint;
        loadstep = vrec.loadstep;
        if  (loadstep == 0)
                loadstep = 1;

        /* Now to create all the bits of the application */

        panedw = XtVaCreateWidget("layout", xmPanedWindowWidgetClass, toplevel, NULL);

        setup_menus();

        dwid = XtVaCreateManagedWidget("dlist",
                                       xmListWidgetClass,       panedw,
                                       XmNvisibleItemCount,     1,
                                       XmNselectionPolicy,      XmSINGLE_SELECT,
                                       NULL);
        size.request_mode = CWHeight;
        XtQueryGeometry(dwid, NULL, &size);
        XtVaSetValues(dwid, XmNpaneMaximum, size.height, XmNpaneMinimum, size.height, NULL);
        XtAddCallback(dwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtuser deflist help});

        if  (vrec.tit_pres)
                maketitle("utitle");

        uwid = XmCreateScrolledList(panedw, "ulist", NULL, 0);
        XtVaSetValues(uwid, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
        if  (uwidth > 0)
                XtVaSetValues(uwid, XmNwidth, uwidth, NULL);
        if  (uitems > 0)
                XtVaSetValues(uwid, XmNvisibleItemCount, uitems, NULL);
        XtAddCallback(uwid, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmbtuser ulist help});
        XtManageChild(uwid);

        if  (vrec.footer_pres)
                maketitle("footer");

        XtManageChild(panedw);
        XtRealizeWidget(toplevel);
        defhdr = gprompt($P{Btulist default name});
        for  (cnt = 0;  cnt < XtNumber(privnames);  cnt++)
                privnames[cnt].priv_abbrev = gprompt(privnames[cnt].priv_mcode);
}

/* Copy but avoid copying trailing null */

#define movein(to, from)        BLOCK_COPY(to, from, strlen(from))
#ifdef  HAVE_MEMCPY
#define BLOCK_SET(to, n, ch)    memset((void *) to, ch, (unsigned) n)
#else
static void  BLOCK_SET(char *to, unsigned n, const char ch)
{
        while  (n != 0)  {
                *to++ = ch;
                n--;
        }
}
#endif

void  defdisplay()
{
        int     outl;
        XmString        str;
        char    obuf[100], nbuf[16];

        BLOCK_SET(obuf, sizeof(obuf), ' ');
        movein(&obuf[USNAM_P], defhdr);
        sprintf(nbuf, "%3d", (int) Btuhdr.btd_defp);
        movein(&obuf[DEFP_P], nbuf);
        sprintf(nbuf, "%3d", (int) Btuhdr.btd_minp);
        movein(&obuf[MINP_P], nbuf);
        sprintf(nbuf, "%3d", (int) Btuhdr.btd_maxp);
        movein(&obuf[MAXP_P], nbuf);
        sprintf(nbuf, "%5u", Btuhdr.btd_maxll);
        movein(&obuf[MAXLL_P], nbuf);
        sprintf(nbuf, "%5u", Btuhdr.btd_totll);
        movein(&obuf[TOTLL_P], nbuf);
        sprintf(nbuf, "%5u", Btuhdr.btd_spec_ll);
        movein(&obuf[SPECLL_P], nbuf);
        if  (Btuhdr.btd_priv)  {
                char    *np = &obuf[P_P];
                unsigned  cnt;
                int     had = 0;

                for  (cnt = 0;  cnt < XtNumber(privnames);  cnt++)
                        if  (Btuhdr.btd_priv & privnames[cnt].priv_flag)  {
                                unsigned  lng = strlen(privnames[cnt].priv_abbrev);
                                if  (had)
                                        *np++ = '|';
                                strncpy(np, privnames[cnt].priv_abbrev, lng);
                                np += lng;
                                had++;
                        }
        }

        /* Trim trailing spaces */

        for  (outl = sizeof(obuf) - 1;  outl >= 0  &&  obuf[outl] == ' ';  outl--)
                ;
        obuf[outl+1] = '\0';
        str = XmStringCreateSimple(obuf);
        XmListDeleteAllItems(dwid);
        XmListAddItem(dwid, str, 0);
        XmStringFree(str);
}

static XmString  fillbuffer(char *buff, unsigned buffsize, BtuserRef uitem)
{
        int     outl;
        char    nbuf[20];

        BLOCK_SET(buff, buffsize, ' ');
        movein(&buff[USNAM_P], prin_uname((uid_t) uitem->btu_user));
        movein(&buff[GRPNAM_P], prin_gname((gid_t) lastgid));
        sprintf(nbuf, "%3d", (int) uitem->btu_defp);
        movein(&buff[DEFP_P], nbuf);
        sprintf(nbuf, "%3d", (int) uitem->btu_minp);
        movein(&buff[MINP_P], nbuf);
        sprintf(nbuf, "%3d", (int) uitem->btu_maxp);
        movein(&buff[MAXP_P], nbuf);
        sprintf(nbuf, "%5u", uitem->btu_maxll);
        movein(&buff[MAXLL_P], nbuf);
        sprintf(nbuf, "%5u", uitem->btu_totll);
        movein(&buff[TOTLL_P], nbuf);
        sprintf(nbuf, "%5u", uitem->btu_spec_ll);
        movein(&buff[SPECLL_P], nbuf);
        if  (uitem->btu_priv)  {
                char    *np = &buff[P_P];
                unsigned  cnt;
                int     had = 0;

                for  (cnt = 0;  cnt < XtNumber(privnames);  cnt++)
                        if  (uitem->btu_priv & privnames[cnt].priv_flag)  {
                                unsigned  lng = strlen(privnames[cnt].priv_abbrev);
                                if  (had)
                                        *np++ = '|';
                                strncpy(np, privnames[cnt].priv_abbrev, lng);
                                np += lng;
                                had++;
                        }
        }
        for  (outl = buffsize - 1;  outl >= 0  &&  buff[outl] == ' ';  outl--)
                ;
        buff[outl+1] = '\0';
        return  XmStringCreateSimple(buff);
}

void  udisplay(int nu, int *posns)
{
        int             ucnt;
        XmString        str;
        char            obuf[100];

        if  (nu <= 0)  {
                XmListDeleteAllItems(uwid);
                for  (ucnt = 0;  ucnt < (int) Npwusers;  ucnt++)  {
                        str = fillbuffer(obuf, sizeof(obuf), &ulist[ucnt]);
                        XmListAddItem(uwid, str, 0);
                        XmStringFree(str);
                }
        }
        else  {
                if  (nu > 1)
                        XtVaSetValues(uwid, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
                for  (ucnt = 0;  ucnt < nu;  ucnt++)  {
                        str = fillbuffer(obuf, sizeof(obuf), &ulist[posns[ucnt]-1]);
                        XmListReplaceItemsPos(uwid, &str, 1, posns[ucnt]);
                        XmStringFree(str);
                        XmListSelectPos(uwid, posns[ucnt], False);
                }
                if  (nu > 1)
                        XtVaSetValues(uwid, XmNselectionPolicy, XmEXTENDED_SELECT, NULL);
        }
}

/* Ye olde main routine.  */

MAINFN_TYPE  main(int argc, char **argv)
{
#if     defined(NHONSUID) || defined(DEBUG)
        int_ugid_t      chk_uid;
#endif
        BtuserRef       mypriv;

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
        Cfile = open_cfile("XMBTUSERCONF", "xmbtuser.help");
        SCRAMBLID_CHECK

        /* If we haven't got a directory, use the current */

        if  (!Curr_pwd  &&  !(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        SWAP_TO(Daemuid);
        wstart(argc, argv);
        mypriv = getbtuentry(Realuid);

        if  (!(mypriv->btu_priv & BTM_WADMIN))  {
                doerror(toplevel, $EH{No write admin file priv});
                exit(E_NOPRIV);
        }
        ulist = getbtulist();

        switch  (alphsort)  {
        case  SRT_USER:
                qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_u);
                break;
        case  SRT_GROUP:
                qsort(QSORTP1 ulist, Npwusers, sizeof(Btuser), QSORTP4 sort_g);
                break;
        }
        defdisplay();
        udisplay(0, (int *) 0);
        XtAppMainLoop(app);
        return  0;              /* Shut up compilers moaning */
}
