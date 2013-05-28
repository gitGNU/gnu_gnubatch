/* xmfilemon.c -- main module for gbch-xmfilemon

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
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/ArrowB.h>
#include <Xm/DialogS.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#ifdef HAVE_XM_SPINB_H
#include <Xm/SpinB.h>
#endif
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include "defaults.h"
#include "filemon.h"
#include "ecodes.h"
#include "cfile.h"
#include "incl_unix.h"
#include "helpargs.h"
#include "errnums.h"
#include "files.h"

static  char    Filename[] = __FILE__;

char    *Curr_pwd;
char    Confvarname[] = "FILEMONCONF";

uid_t           Daemuid;

enum  wot_mode  wotact = WM_STOP_FOUND;
enum  wot_form  wotf = WF_APPEARS;
enum  wot_file  wotfl = WFL_SPEC_FILE;
enum  inc_exist wotexist = IE_IGNORE_EXIST;
int     isdaemon = 0,
        cmdscript = 0,
        recursive = 0,
        followlinks = 0;

char    *work_directory,        /* Wot we're looking at */
        *script_file,           /* Wot we want to do */
        *file_patt;             /* Pattern to match */

int     grow_time = DEF_GROW_TIME,
        poll_time = DEF_POLL_TIME;

static  XtAppContext    app;
#ifndef HAVE_XM_SPINB_H
static  int             arr_rtime, arr_rint;
static  XtIntervalId    arrow_timer;
#endif

static  Widget          dir_w,                  /* Directory to scan */
                        file_w,                 /* File to look for */
                        shell_w,                /* Command to execute */
                        grot_w,                 /* No grow etc time */
                        pollt_w,                /* Poll time */
                        inclexist_w,            /* Include existing */
                        recursive_w,            /* Recursive */
                        follow_w,               /* Follow links */
                        daemon_w,               /* Daemon process */
                        contfound_w,            /* Continue if found */
                        cmdnotsc_w;             /* Command not script */

typedef struct  {
        int             rtime, rint;
        int             grow_time, poll_time;
        Boolean         isdaemon;
        Boolean         contfound;
        Boolean         includeexist;
        Boolean         recursive;
        Boolean         followlinks;
        Boolean         scriptcmd;
        String          style;
        String          typematch;
        String          pattern;
        String          directory;
        String          script;
}  vrec_t;

static  XtResource      resources[] = {
        { "repeatTime", "RepeatTime", XtRInt, sizeof(int), XtOffsetOf(vrec_t, rtime), XtRImmediate, (XtPointer) 500 },
        { "repeatInt", "RepeatInt", XtRInt, sizeof(int), XtOffsetOf(vrec_t, rint), XtRImmediate, (XtPointer) 100 },
        { "pollFreq", "PollFreq", XtRInt, sizeof(int), XtOffsetOf(vrec_t, poll_time), XtRImmediate, (XtPointer) DEF_POLL_TIME },
        { "nomodTime", "NoModTime", XtRInt, sizeof(int), XtOffsetOf(vrec_t, grow_time), XtRImmediate, (XtPointer) DEF_GROW_TIME },
        { "daemon", "Daemon", XtRBoolean, sizeof(Boolean), XtOffsetOf(vrec_t, isdaemon), XtRImmediate, (XtPointer) False },
        { "contFound", "ContFound", XtRBoolean, sizeof(Boolean), XtOffsetOf(vrec_t, contfound), XtRImmediate, (XtPointer) False },
        { "includeExist", "IncludeExist", XtRBoolean, sizeof(Boolean), XtOffsetOf(vrec_t, includeexist), XtRImmediate, (XtPointer) False },
        { "recursive", "Recursive", XtRBoolean, sizeof(Boolean), XtOffsetOf(vrec_t, recursive), XtRImmediate, (XtPointer) False },
        { "followLinks", "FollowLinks", XtRBoolean, sizeof(Boolean), XtOffsetOf(vrec_t, followlinks), XtRImmediate, (XtPointer) False },
        { "scriptCmd", "ScriptCnd", XtRBoolean, sizeof(Boolean), XtOffsetOf(vrec_t, scriptcmd), XtRImmediate, (XtPointer) False },
        { "style", "Style", XtRString, sizeof(String), XtOffsetOf(vrec_t, style), XtRString, "appears" },
        { "typeMatch", "TypeMatch", XtRString, sizeof(String), XtOffsetOf(vrec_t, typematch), XtRString, "any" },
        { "pattern", "Pattern", XtRString, sizeof(String), XtOffsetOf(vrec_t, pattern), XtRString, "" },
        { "directory", "Directory", XtRString, sizeof(String), XtOffsetOf(vrec_t, directory), XtRString, "" },
        { "script", "Script", XtRString, sizeof(String), XtOffsetOf(vrec_t, script), XtRString, "" }
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

void  nomem(const char *fl, const int ln)
{
        fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        exit(E_NOMEM);
}

Widget  FindWidget(Widget w)
{
        while  (w && !XtIsWidget(w))
                w = XtParent(w);
        return  w;
}

static char *makebigvec(char **mat)
{
        unsigned  totlen = 0, len;
        char    **ep, *newstr, *pos;

        for  (ep = mat;  *ep;  ep++)
                totlen += strlen(*ep) + 1;

        newstr = malloc((unsigned) totlen);
        if  (!newstr)
                ABORT_NOMEM;
        pos = newstr;
        for  (ep = mat;  *ep;  ep++)  {
                len = strlen(*ep);
                strcpy(pos, *ep);
                free(*ep);
                pos += len;
                *pos++ = '\n';
        }
        pos[-1] = '\0';
        free((char *) mat);
        return  newstr;
}

void  dohelp(Widget wid, int helpcode)
{
        char    **evec = helpvec(helpcode, 'H'), *newstr;
        Widget          ew;
        if  (!evec[0])  {
                disp_arg[9] = helpcode;
                free((char *) evec);
                evec = helpvec($E{Missing help code}, 'E');
        }
        ew = XmCreateInformationDialog(FindWidget(wid), "help", NULL, 0);
        XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_CANCEL_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(ew, XmDIALOG_HELP_BUTTON));
        newstr = makebigvec(evec);
        XtVaSetValues(ew,
                      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
                      XtVaTypedArg, XmNmessageString, XmRString, newstr, strlen(newstr),
                      NULL);
        free(newstr);
        XtManageChild(ew);
        XtPopup(XtParent(ew), XtGrabNone);
}

#ifndef HAVE_XM_SPINB_H
static void  time_adj(Widget w, XtIntervalId *id, const int adj, XtTimerCallbackProc tcb)
{
        int             n;
        char            *txt, nbuf[6];

        XtVaGetValues(w, XmNvalue, &txt, NULL);
        n = atoi(txt) + adj;
        XtFree(txt);
        if  (n == 0  ||  n > MAX_POLL_TIME)
                return;
        sprintf(nbuf, "%5d", n);
        XmTextSetString(w, nbuf);
        arrow_timer = XtAppAddTimeOut(app, id? arr_rint: arr_rtime, tcb, (XtPointer) w);
}

static void  time_up(Widget w, XtIntervalId *id)
{
        time_adj(w, id, 1, (XtTimerCallbackProc) time_up);
}

static void  time_dn(Widget w, XtIntervalId *id)
{
        time_adj(w, id, -1, (XtTimerCallbackProc) time_dn);
}

static void     time_cb(Widget tw, XmArrowButtonCallbackStruct *cbs, void (*fn)(Widget, XtIntervalId *))
{
        if  (cbs->reason == XmCR_ARM)
                (*fn)(tw, NULL);
        else  if  (arrow_timer)  {
                XtRemoveTimeOut(arrow_timer);
                arrow_timer = (XtIntervalId) 0;
        }
}

static void  timeup_cb(Widget w, Widget tw, XmArrowButtonCallbackStruct *cbs)
{
        time_cb(tw, cbs, time_up);
}

static void  timedn_cb(Widget w, Widget tw, XmArrowButtonCallbackStruct *cbs)
{
        time_cb(tw, cbs, time_dn);
}

Widget  CreateArrowPair(char *name, Widget formw, Widget topw, Widget leftw, XtCallbackProc upcall, XtCallbackProc dncall)
{
        Widget  uparrow, dnarrow;
        char    fullname[10];

        sprintf(fullname, "%sup", name);
        uparrow = XtVaCreateManagedWidget(fullname,
                                          xmArrowButtonWidgetClass,     formw,
                                          XmNtopAttachment,             XmATTACH_WIDGET,
                                          XmNtopWidget,                 topw,
                                          XmNleftAttachment,            XmATTACH_WIDGET,
                                          XmNleftWidget,                leftw,
                                          XmNarrowDirection,            XmARROW_UP,
                                          XmNborderWidth,               0,
                                          XmNrightOffset,               0,
                                          NULL);

        sprintf(fullname, "%sdn", name);

        dnarrow = XtVaCreateManagedWidget(fullname,
                                          xmArrowButtonWidgetClass,     formw,
                                          XmNtopAttachment,             XmATTACH_WIDGET,
                                          XmNtopWidget,                 topw,
                                          XmNleftAttachment,            XmATTACH_WIDGET,
                                          XmNleftWidget,                uparrow,
                                          XmNarrowDirection,            XmARROW_DOWN,
                                          XmNborderWidth,               0,
                                          XmNleftOffset,                0,
                                          NULL);

        XtAddCallback(uparrow, XmNarmCallback, upcall, (XtPointer) leftw);
        XtAddCallback(dnarrow, XmNarmCallback, dncall, (XtPointer) leftw);
        XtAddCallback(uparrow, XmNdisarmCallback, upcall, (XtPointer) 0);
        XtAddCallback(dnarrow, XmNdisarmCallback, dncall, (XtPointer) 0);
        return  dnarrow;
}
#endif /* ! HAVE_XM_SPINB_H */

static void  enddseld(Widget w, int data, XmFileSelectionBoxCallbackStruct *cbs)
{
        char    *dirname;

        if  (data)  {
                int     n;
                if  (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &dirname))
                        return;
                if  (!dirname  ||  dirname[0] == '\0')
                        return;
                if  (dirname[n = strlen(dirname) - 1] == '/')
                        dirname[n] = '\0';
                XmTextSetString(dir_w, dirname);
                XtFree(dirname);
        }
        XtDestroyWidget(w);
}

static void  selectdir(Widget w)
{
        Widget  dseld;
        XmString        str;
        Arg     args[4];

        str = XmStringCreateSimple(work_directory);
        XtSetArg(args[0], XmNdirectory, str);
        dseld = XmCreateFileSelectionDialog(FindWidget(w), "dselb", args, 1);
        XmStringFree(str);
        XtAddCallback(dseld, XmNcancelCallback, (XtCallbackProc) enddseld, (XtPointer) 0);
        XtAddCallback(dseld, XmNokCallback, (XtCallbackProc) enddseld, (XtPointer) 1);
        XtUnmanageChild(XtParent(XmFileSelectionBoxGetChild(dseld, XmDIALOG_LIST)));
        XtUnmanageChild(XmFileSelectionBoxGetChild(dseld, XmDIALOG_LIST_LABEL));
        XtAddCallback(dseld, XmNhelpCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmfilemon dir sel dialog});
        XtManageChild(dseld);
}

static void  cancrout(Widget w)
{
        exit(E_FALSE);
}

#include "inline/btfmadefs.c"

char *gen_option(const int arg)
{
        int     v = arg - $A{btfilemon arg explain};

        if  (optvec[v].isplus)  {
                char    *r = malloc((unsigned) (strlen(optvec[v].aun.string) + 2));
                if  (!r)
                        ABORT_NOMEM;
                sprintf(r, "+%s", optvec[v].aun.string);
                return  r;
        }
        else  if  (optvec[v].aun.letter == 0)
                return  "+???";
        else  {
                char    *r = malloc(3);
                if  (!r)
                        ABORT_NOMEM;
                sprintf(r, "-%c", optvec[v].aun.letter);
                return  r;
        }
}

#define TBG_STATE(W)    XmToggleButtonGadgetGetState(W)

static void  accrout(Widget w)
{
        HelpargRef      helpa;
        char            *dirv, *filv, *script;
#ifdef  HAVE_XM_SPINB_H
        int             gv, pv;
        char            grotv[16], polltv[16];
#ifdef  BROKEN_SPINBOX
        int             mini;
#endif
#else
        char            *grotv, *polltv;
#endif
        char            **ap;
        char            *argvec[32];

        XtVaGetValues(dir_w, XmNvalue, &dirv, NULL);
        XtVaGetValues(file_w, XmNvalue, &filv, NULL);
        XtVaGetValues(shell_w, XmNvalue, &script, NULL);
#ifdef  HAVE_XM_SPINB_H
#ifdef  BROKEN_SPINBOX
        XtVaGetValues(grot_w, XmNposition, &gv, XmNminimumValue, &mini, NULL);
        gv += mini;
        XtVaGetValues(pollt_w, XmNposition, &pv, XmNminimumValue, &mini, NULL);
        pv += mini;
#else
        XtVaGetValues(grot_w, XmNposition, &gv, NULL);
        XtVaGetValues(pollt_w, XmNposition, &pv, NULL);
#endif
        sprintf(grotv, "%d", gv);
        sprintf(polltv, "%d", pv);
#else
        XtVaGetValues(grot_w, XmNvalue, &grotv, NULL);
        XtVaGetValues(pollt_w, XmNvalue, &polltv, NULL);
        /* Trim leading spaces on values */
        while  (isspace(*polltv))
                polltv++;
        while  (isspace(*grotv))
                grotv++;
#endif
        helpa = helpargs(Adefs, $A{btfilemon arg explain}, $A{btfilemon arg freeze home});
        makeoptvec(helpa, $A{btfilemon arg explain}, $A{btfilemon arg freeze home});

        ap = argvec;
        *ap++ = "btfilemon";
        *ap++ = gen_option($A{btfilemon arg run monitor});
        *ap++ = gen_option(TBG_STATE(daemon_w)? $A{btfilemon arg daemon}: $A{btfilemon arg nodaemon});

        switch  (wotfl)  {
        case  WFL_ANY_FILE:
                *ap++ = gen_option($A{btfilemon arg anyfile});
                break;
        case  WFL_PATT_FILE:
                if  (isgraph(filv[0]))  {
                        *ap++ = gen_option($A{btfilemon arg pattern file});
                        *ap++ = filv;
                }
                break;
        case  WFL_SPEC_FILE:
                if  (isgraph(filv[0]))  {
                        *ap++ = gen_option($A{btfilemon arg given file});
                        *ap++ = filv;
                }
                break;
        }

        switch  (wotf)  {
        case  WF_APPEARS:
                *ap++ = gen_option($A{btfilemon arg arrival});
                break;
        case  WF_REMOVED:
                *ap++ = gen_option($A{btfilemon arg file removed});
                break;
        case  WF_STOPSGROW:
                if  (isgraph(grotv[0]))  {
                        *ap++ = gen_option($A{btfilemon arg grow time});
                        *ap++ = grotv;
                }
                break;
        case  WF_STOPSWRITE:
                if  (isgraph(grotv[0]))  {
                        *ap++ = gen_option($A{btfilemon arg mod time});
                        *ap++ = grotv;
                }
                break;
        case  WF_STOPSCHANGE:
                if  (isgraph(grotv[0]))  {
                        *ap++ = gen_option($A{btfilemon arg change time});
                        *ap++ = grotv;
                }
                break;
        case  WF_STOPSUSE:
                if  (isgraph(grotv[0]))  {
                        *ap++ = gen_option($A{btfilemon arg access time});
                        *ap++ = grotv;
                }
                break;
        }

        *ap++ = gen_option(TBG_STATE(contfound_w)? $A{btfilemon arg cont found}: $A{btfilemon arg halt found});
        *ap++ = gen_option(TBG_STATE(inclexist_w)? $A{btfilemon arg include existing}: $A{btfilemon arg ignore existing});
        *ap++ = gen_option(TBG_STATE(recursive_w)? $A{btfilemon arg recursive}: $A{btfilemon arg nonrecursive});
        *ap++ = gen_option(TBG_STATE(follow_w)? $A{btfilemon arg follow links}: $A{btfilemon arg no links});
        if  (isgraph(polltv[0]))  {
                *ap++ = gen_option($A{btfilemon arg poll time});
                *ap++ = polltv;
        }
        if  (isgraph(dirv[0]))  {
                *ap++ = gen_option($A{btfilemon arg directory});
                *ap++ = dirv;
        }
        if  (isgraph(script[0]))  {
                *ap++ = gen_option(TBG_STATE(cmdnotsc_w)? $A{btfilemon arg command}: $A{btfilemon arg script file});
                *ap++ = script;
        }
        *ap = (char *) 0;
        execvp("btfilemon", argvec);
        exit(E_SETUP);
}

static void  typemon_cb(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                wotf = (enum wot_form) n;
}

static void  typepatt_cb(Widget w, int n, XmToggleButtonCallbackStruct *cbs)
{
        if  (cbs->set)
                wotfl = (enum wot_file) n;
}

struct  {
        char                    *widname;
        enum    wot_form        typ;
}       montypelist[] =  {
{       "ifapp",                WF_APPEARS      },
{       "nogrow",               WF_STOPSGROW    },
{       "mtime",                WF_STOPSWRITE   },
{       "ctime",                WF_STOPSCHANGE  },
{       "atime",                WF_STOPSUSE     },
{       "ifrem",                WF_REMOVED      }
};

struct  {
        char                    *widname;
        enum    wot_file        typ;
}       typepattlist[] =  {
{       "anyf",                 WFL_ANY_FILE    },
{       "pattf",                WFL_PATT_FILE   },
{       "specf",                WFL_SPEC_FILE   }
};

MAINFN_TYPE  main(int argc, char **argv)
{
        int     cnt;
        Widget  toplevel, dshell, panedw, formw, prevleft, typemonrc, existrc, typepattrc, actform, acceptw, cancw, helpw;
#ifdef HAVE_XM_SPINB_H
        Widget  spinb;
#else
        char    nbuf[6];
#endif
        Dimension       h;
        vrec_t          vrec;

        versionprint(argv, "$Revision: 1.9 $", 0);
        init_mcfile();
        Realuid = getuid();
        Realgid = getgid();
        Effuid = geteuid();
        Effgid = getegid();
        Cfile = open_cfile(Confvarname, "filemon.help");

        if  (!(Curr_pwd = getenv("PWD")))
                Curr_pwd = runpwd();

        toplevel = XtVaAppInitialize(&app, "GBATCH", NULL, 0, &argc, argv, NULL, NULL);
        XtGetApplicationResources(toplevel, &vrec, resources, XtNumber(resources), NULL, 0);
#ifndef HAVE_XM_SPINB_H
        arr_rtime = vrec.rtime;
        arr_rint = vrec.rint;
#endif
        grow_time = vrec.grow_time;
        poll_time = vrec.poll_time;
        isdaemon = vrec.isdaemon? 1: 0;
        cmdscript = vrec.scriptcmd? 1: 0;
        wotact = vrec.contfound? WM_CONT_FOUND: WM_STOP_FOUND;
        wotexist = vrec.includeexist? IE_INCL_EXIST: IE_IGNORE_EXIST;
        recursive = vrec.recursive? 1: 0;
        followlinks = vrec.followlinks? 1: 0;

        switch  (vrec.style[0])  {
        case  'a':
        case  'A':
                if  (ncstrcmp(vrec.style, "appears") == 0)  {
                        wotf = WF_APPEARS;
                        break;
                }
        case  'd':
                if  (ncstrcmp(vrec.style, "deleted") == 0)  {
                        wotf = WF_REMOVED;
                        break;
                }
        case  'r':
                if  (ncstrcmp(vrec.style, "removed") == 0)  {
                        wotf = WF_REMOVED;
                        break;
                }
        case  'n':
        case  'N':
                if  (ncstrcmp(vrec.style, "nogrow") == 0)  {
                        wotf = WF_STOPSGROW;
                        break;
                }
                if  (ncstrcmp(vrec.style, "nomod") == 0)  {
                        wotf = WF_STOPSWRITE;
                        break;
                }
                if  (ncstrcmp(vrec.style, "nochange") == 0)  {
                        wotf = WF_STOPSCHANGE;
                        break;
                }
                if  (ncstrcmp(vrec.style, "noaccess") == 0)  {
                        wotf = WF_STOPSUSE;
                        break;
                }
        default:
                wotf = WF_APPEARS;
                break;
        }

        switch  (vrec.typematch[0])  {
        case  'a':
        case  'A':
                if  (ncstrcmp(vrec.style, "any") == 0)  {
                        wotfl = WFL_ANY_FILE;
                        break;
                }
        case  'p':
        case  'P':
                if  (ncstrcmp(vrec.style, "pattern") == 0)  {
                        wotfl = WFL_PATT_FILE;
                        break;
                }
        case  's':
        case  'S':
                if  (ncstrcmp(vrec.style, "specific") == 0)  {
                        wotfl = WFL_SPEC_FILE;
                        break;
                }
        default:
                wotfl = WFL_ANY_FILE;
                break;
        }
        file_patt = vrec.pattern;

        work_directory = vrec.directory;
        if  (!work_directory  ||  work_directory[0] == '\0')
                work_directory = Curr_pwd;
        script_file = vrec.script;

        /*      Finished with arguments, now do the bizniz  */

        dshell = XtVaCreatePopupShell("fsdlg",
                                     xmDialogShellWidgetClass,  toplevel,
                                     XmNdialogStyle,            XmDIALOG_FULL_APPLICATION_MODAL,
                                     XmNdeleteResponse,         XmDESTROY,
                                     NULL);

        panedw = XtVaCreateWidget("layout", xmPanedWindowWidgetClass, dshell,
                                  XmNsashWidth,                 1,
                                  XmNsashHeight,                1,
                                  NULL);

        formw = XtVaCreateWidget("form",
                                 xmFormWidgetClass,             panedw,
                                 XmNfractionBase,               24,
                                 NULL);

        /*  First row is directory box */

        prevleft = XtVaCreateManagedWidget("dirname",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_FORM,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           NULL);

        dir_w = XtVaCreateManagedWidget("dir",
                                        xmTextFieldWidgetClass, formw,
                                        XmNcolumns,             60,
                                        XmNmaxWidth,            60,
                                        XmNtopAttachment,       XmATTACH_FORM,
                                        XmNleftAttachment,      XmATTACH_WIDGET,
                                        XmNleftWidget,          prevleft,
                                        NULL);

        XmTextSetString(dir_w, work_directory);

        prevleft = XtVaCreateManagedWidget("selectd",
                                           xmPushButtonGadgetClass,             formw,
                                           XmNtopAttachment,                    XmATTACH_FORM,
                                           XmNleftAttachment,                   XmATTACH_WIDGET,
                                           XmNleftWidget,                       dir_w,
                                           XmNdefaultButtonShadowThickness,     1,
                                           NULL);

        XtAddCallback(prevleft, XmNactivateCallback, (XtCallbackProc) selectdir, (XtPointer) 0);

        /* Next row is how we monitor */

        typemonrc = XtVaCreateManagedWidget("typem",
                                            xmRowColumnWidgetClass,     formw,
                                            XmNtopAttachment,           XmATTACH_WIDGET,
                                            XmNtopWidget,               dir_w,
                                            XmNleftAttachment,          XmATTACH_FORM,
                                            XmNpacking,                 XmPACK_COLUMN,
                                            XmNnumColumns,              1,
                                            XmNisHomogeneous,           True,
                                            XmNentryClass,              xmToggleButtonGadgetClass,
                                            XmNradioBehavior,           True,
                                            XmNborderWidth,             0,
                                            NULL);

        for  (cnt = 0;  cnt < XtNumber(montypelist);  cnt++)  {
                Widget  butt = XtVaCreateManagedWidget(montypelist[cnt].widname,
                                        xmToggleButtonGadgetClass,      typemonrc,
                                        XmNborderWidth,                 0,
                                        NULL);

                if  (wotf == montypelist[cnt].typ)
                        XmToggleButtonGadgetSetState(butt, True, False);

                XtAddCallback(butt, XmNvalueChangedCallback, (XtCallbackProc) typemon_cb, (XtPointer) montypelist[cnt].typ);
        }

        existrc = XtVaCreateManagedWidget("existf",
                                          xmRowColumnWidgetClass,       formw,
                                          XmNtopAttachment,             XmATTACH_WIDGET,
                                          XmNtopWidget,                 dir_w,
                                          XmNleftAttachment,            XmATTACH_WIDGET,
                                          XmNleftWidget,                typemonrc,
                                          XmNpacking,                   XmPACK_COLUMN,
                                          XmNnumColumns,                1,
                                          XmNisHomogeneous,             True,
                                          XmNentryClass,                xmToggleButtonGadgetClass,
                                          XmNborderWidth,               0,
                                          NULL);

        inclexist_w = XtVaCreateManagedWidget("include",
                                              xmToggleButtonGadgetClass,        existrc,
                                              XmNborderWidth,                   0,
                                              NULL);

        if  (wotexist == IE_INCL_EXIST)
                XmToggleButtonGadgetSetState(inclexist_w, True, False);

        recursive_w = XtVaCreateManagedWidget("recursive",
                                              xmToggleButtonGadgetClass,        existrc,
                                              XmNborderWidth,                   0,
                                              NULL);

        if  (recursive)
                XmToggleButtonGadgetSetState(recursive_w, True, False);

        follow_w = XtVaCreateManagedWidget("followlinks",
                                           xmToggleButtonGadgetClass,   existrc,
                                           XmNborderWidth,              0,
                                           NULL);

        if  (followlinks)
                XmToggleButtonGadgetSetState(follow_w, True, False);


        prevleft = XtVaCreateManagedWidget("nogrowtime",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        typemonrc,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           NULL);

#ifdef HAVE_XM_SPINB_H
        spinb = XtVaCreateManagedWidget("growsp",
                                        xmSpinBoxWidgetClass,           formw,
                                        XmNtopAttachment,               XmATTACH_WIDGET,
                                        XmNtopWidget,                   typemonrc,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  prevleft,
                                        NULL);

        grot_w = XtVaCreateManagedWidget("growth_t",
                                         xmTextFieldWidgetClass,        spinb,
                                         XmNmaximumValue,               99999,
                                         XmNminimumValue,               1,
                                         XmNspinBoxChildType,           XmNUMERIC,
#ifdef  BROKEN_SPINBOX
                                         XmNpositionType,               XmPOSITION_INDEX,
                                         XmNposition,                   grow_time-1,
#else
                                         XmNposition,                   grow_time,
#endif
                                         XmNcolumns,                    5,
                                         XmNeditable,                   False,
                                         XmNcursorPositionVisible,      False,
                                         NULL);
#else
        grot_w = XtVaCreateManagedWidget("growth_t",
                                         xmTextFieldWidgetClass,        formw,
                                         XmNcolumns,                    5,
                                         XmNmaxWidth,                   5,
                                         XmNtopAttachment,              XmATTACH_WIDGET,
                                         XmNtopWidget,                  typemonrc,
                                         XmNleftAttachment,             XmATTACH_WIDGET,
                                         XmNleftWidget,                 prevleft,
                                         XmNeditable,                   False,
                                         XmNcursorPositionVisible,      False,
                                         NULL);

        sprintf(nbuf, "%5d", grow_time);
        XmTextSetString(grot_w, nbuf);
        prevleft = CreateArrowPair("grows", formw, typemonrc, grot_w, (XtCallbackProc) timeup_cb, (XtCallbackProc) timedn_cb);
#endif

        typepattrc = XtVaCreateManagedWidget("typep",
                                             xmRowColumnWidgetClass,    formw,
                                             XmNtopAttachment,          XmATTACH_WIDGET,
#ifdef HAVE_XM_SPINB_H
                                             XmNtopWidget,              spinb,
#else
                                             XmNtopWidget,              grot_w,
#endif
                                             XmNleftAttachment,         XmATTACH_FORM,
                                             XmNpacking,                XmPACK_COLUMN,
                                             XmNnumColumns,             1,
                                             XmNisHomogeneous,          True,
                                             XmNentryClass,             xmToggleButtonGadgetClass,
                                             XmNradioBehavior,          True,
                                             XmNborderWidth,            0,
                                             NULL);

        for  (cnt = 0;  cnt < XtNumber(typepattlist);  cnt++)  {
                Widget  butt = XtVaCreateManagedWidget(typepattlist[cnt].widname,
                                        xmToggleButtonGadgetClass,      typepattrc,
                                        XmNborderWidth,                 0,
                                        NULL);

                if  (wotfl == typepattlist[cnt].typ)
                        XmToggleButtonGadgetSetState(butt, True, False);

                XtAddCallback(butt, XmNvalueChangedCallback, (XtCallbackProc) typepatt_cb, (XtPointer) typepattlist[cnt].typ);
        }

        prevleft = XtVaCreateManagedWidget("filename",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        typepattrc,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           NULL);

        file_w = XtVaCreateManagedWidget("file",
                                         xmTextFieldWidgetClass,formw,
                                         XmNcolumns,            60,
                                         XmNmaxWidth,           60,
                                         XmNtopAttachment,      XmATTACH_WIDGET,
                                         XmNtopWidget,          typepattrc,
                                         XmNleftAttachment,     XmATTACH_WIDGET,
                                         XmNleftWidget,         prevleft,
                                         NULL);

        if  (file_patt)
                XmTextSetString(file_w, file_patt);

        contfound_w = XtVaCreateManagedWidget("cont",
                                              xmToggleButtonGadgetClass,        formw,
                                              XmNborderWidth,                   0,
                                              XmNtopAttachment,                 XmATTACH_WIDGET,
                                              XmNtopWidget,                     file_w,
                                              XmNleftAttachment,                XmATTACH_FORM,
                                              NULL);

        if  (wotact == WM_CONT_FOUND)
                XmToggleButtonGadgetSetState(contfound_w, True, False);


        daemon_w = XtVaCreateManagedWidget("daem",
                                           xmToggleButtonGadgetClass,   formw,
                                           XmNborderWidth,              0,
                                           XmNtopAttachment,            XmATTACH_WIDGET,
                                           XmNtopWidget,                contfound_w,
                                           XmNleftAttachment,           XmATTACH_FORM,
                                           NULL);

        if  (isdaemon)
                XmToggleButtonGadgetSetState(daemon_w, True, False);


        prevleft = XtVaCreateManagedWidget("polltime",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
                                           XmNtopWidget,        daemon_w,
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           NULL);

#ifdef HAVE_XM_SPINB_H
        spinb = XtVaCreateManagedWidget("pollsp",
                                        xmSpinBoxWidgetClass,   formw,
                                        XmNtopAttachment,       XmATTACH_WIDGET,
                                        XmNtopWidget,           daemon_w,
                                        XmNleftAttachment,      XmATTACH_WIDGET,
                                        XmNleftWidget,          prevleft,
                                        NULL);

        pollt_w = XtVaCreateManagedWidget("pollt",
                                          xmTextFieldWidgetClass,       spinb,
                                          XmNmaximumValue,              99999,
                                          XmNminimumValue,              1,
                                          XmNspinBoxChildType,          XmNUMERIC,
#ifdef  BROKEN_SPINBOX
                                          XmNpositionType,              XmPOSITION_INDEX,
                                          XmNposition,                  poll_time-1,
#else
                                          XmNposition,                  poll_time,
#endif
                                          XmNcolumns,                   5,
                                          XmNcursorPositionVisible,     False,
                                          XmNeditable,                  False,
                                          NULL);

#else
        pollt_w = XtVaCreateManagedWidget("pollt",
                                          xmTextFieldWidgetClass,       formw,
                                          XmNcolumns,                   5,
                                          XmNmaxWidth,                  5,
                                          XmNcursorPositionVisible,     False,
                                          XmNeditable,                  False,
                                          XmNtopAttachment,             XmATTACH_WIDGET,
                                          XmNtopWidget,                 daemon_w,
                                          XmNleftAttachment,            XmATTACH_WIDGET,
                                          XmNleftWidget,                prevleft,
                                          NULL);

        sprintf(nbuf, "%5d", poll_time);
        XmTextSetString(pollt_w, nbuf);
        CreateArrowPair("poll", formw, daemon_w, pollt_w, (XtCallbackProc) timeup_cb, (XtCallbackProc) timedn_cb);
#endif
        prevleft = XtVaCreateManagedWidget("command",
                                           xmLabelGadgetClass,  formw,
                                           XmNtopAttachment,    XmATTACH_WIDGET,
#ifdef HAVE_XM_SPINB_H
                                           XmNtopWidget,        spinb,
#else
                                           XmNtopWidget,        pollt_w,
#endif
                                           XmNleftAttachment,   XmATTACH_FORM,
                                           NULL);

        shell_w = XtVaCreateManagedWidget("cmd",
                                          xmTextFieldWidgetClass,       formw,
                                          XmNcolumns,                   40,
                                          XmNmaxWidth,                  40,
                                          XmNtopAttachment,             XmATTACH_WIDGET,
#ifdef HAVE_XM_SPINB_H
                                          XmNtopWidget,                 spinb,
#else
                                          XmNtopWidget,                 pollt_w,
#endif
                                          XmNleftAttachment,            XmATTACH_WIDGET,
                                          XmNleftWidget,                prevleft,
                                          XmNcursorPositionVisible,     False,
                                          NULL);

        XmTextSetString(shell_w, script_file);

        cmdnotsc_w = XtVaCreateManagedWidget("cscmd",
                                             xmToggleButtonGadgetClass, formw,
                                             XmNborderWidth,            0,
                                             XmNtopAttachment,          XmATTACH_WIDGET,
                                             XmNtopWidget,              shell_w,
                                             XmNleftAttachment,         XmATTACH_FORM,
                                             NULL);

        if  (cmdscript)
                XmToggleButtonGadgetSetState(cmdnotsc_w, True, False);

        XtManageChild(formw);

        actform = XtVaCreateWidget("actform",
                                   xmFormWidgetClass,           panedw,
                                   XmNfractionBase,             7,
                                   NULL);

        acceptw = XtVaCreateManagedWidget("Accept",
                                          xmPushButtonGadgetClass,      actform,
                                          XmNtopAttachment,             XmATTACH_FORM,
                                          XmNbottomAttachment,          XmATTACH_FORM,
                                          XmNleftAttachment,            XmATTACH_POSITION,
                                          XmNleftPosition,              1,
                                          XmNrightAttachment,           XmATTACH_POSITION,
                                          XmNrightPosition,             2,
                                          XmNshowAsDefault,             True,
                                          XmNdefaultButtonShadowThickness,1,
                                          XmNtopOffset,                 0,
                                          XmNbottomOffset,              0,
                                          NULL);

        cancw = XtVaCreateManagedWidget("Cancel",
                                        xmPushButtonGadgetClass,        actform,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNbottomAttachment,            XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                3,
                                        XmNrightAttachment,             XmATTACH_POSITION,
                                        XmNrightPosition,               4,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        helpw = XtVaCreateManagedWidget("Help",
                                        xmPushButtonGadgetClass,        actform,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNbottomAttachment,            XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_POSITION,
                                        XmNleftPosition,                5,
                                        XmNrightAttachment,             XmATTACH_POSITION,
                                        XmNrightPosition,               6,
                                        XmNshowAsDefault,               False,
                                        XmNdefaultButtonShadowThickness,        1,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        XtAddCallback(acceptw, XmNactivateCallback, (XtCallbackProc) accrout, (XtPointer) 0);
        XtAddCallback(cancw, XmNactivateCallback, (XtCallbackProc) cancrout, (XtPointer) 0);
        XtAddCallback(helpw, XmNactivateCallback, (XtCallbackProc) dohelp, (XtPointer) $H{xmfilemon main help});
        XtManageChild(actform);
        XtVaGetValues(acceptw, XmNheight, &h, NULL);
        XtVaSetValues(actform, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
        XtManageChild(panedw);
        XtPopup(dshell, XtGrabNone);
        XtAppMainLoop(app);
        return  0;              /* Shut up compiler */
}
