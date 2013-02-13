/* xmbq_view.c -- view scripts / log / holidays for gbch-xmq

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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
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
#include <Xm/ArrowB.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
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
#include "netmsg.h"
#include "statenums.h"
#include "errnums.h"
#include "ecodes.h"
#include "q_shm.h"
#include "ipcstuff.h"
#include "jvuprocs.h"
#include "xm_commlib.h"
#include "xmbq_ext.h"
#include "xmmenu.h"
#include "xmbtqdoc.bm"

static  char    Filename[] = __FILE__;

#define my_max(a,b) ((int)(a)>(int)(b)?(int)(a):(int)(b))
#define my_min(a,b) ((int)(a)<(int)(b)?(int)(a):(int)(b))

static  char    *buffer;

static  char    had_first, inprogress, matchcase, sbackward, wraparound;

static  unsigned        linecount,      /* Number of lines in document */
                        pagewidth;      /* In chars */

static  ULONG   linepixwidth;   /* In pixels */

static  Widget  scrolled_w,
                drawing_a,      /* Drawing area */
                vsb,            /* Scroll bars */
                hsb;

static  GC      mygc;           /* Graphics context for writes */

static  Pixmap          docbitmap;

static  XFontStruct     *font;

static  Dimension       view_width,
                        view_height;

static  unsigned        cell_width,
                        cell_height,
                        pix_hoffset,
                        pix_voffset;

static  unsigned        last_top,
                        last_left;

static  FILE            *infile;

static  unsigned        last_matchline;
static  char            *matchtext;

static void  cb_vsrchfor(Widget);
static void  cb_vrsrch(Widget, int);
static void  endview(Widget, int);
static void  clearlog(Widget);
FILE *net_feed(const int, const netid_t, const jobno_t, const int);

static  casc_button
job_casc[] = {
        {       ITEM,   "Exit",         endview,        0       }},
search_casc[] = {
        {       ITEM,   "Search",       cb_vsrchfor,    0       },
        {       ITEM,   "Searchforw",   cb_vrsrch,      0       },
        {       ITEM,   "Searchback",   cb_vrsrch,      1       }},
helpv_casc[] = {
        {       ITEM,   "Help",         dohelp,         $H{xmbtq view job help} },
        {       ITEM,   "Helpon",       cb_chelp,       0       }},
helpse_casc[] = {
        {       ITEM,   "Help",         dohelp,         $H{xmbtq view syserr help}      },
        {       ITEM,   "Helpon",       cb_chelp,       0       }},
file_casc[] = {
        {       ITEM,   "Clearlog",     clearlog,       0       },
        {       ITEM,   "Exit",         endview,        0       }};

static  pull_button
        job_button =
                {       "Job",          XtNumber(job_casc),     $H{xmbtq view job menu help},   job_casc        },
        srch_button =
                {       "Srch",         XtNumber(search_casc),  $H{xmbtq view search menu help},search_casc     },
        helpv_button =
                {       "Help",         XtNumber(helpv_casc),   $H{xmbtq view help menu help},  helpv_casc,     1},
        file_button =
                {       "File",         XtNumber(file_casc),    $H{xmbtq syse file menu help},  file_casc       },
        helpse_button =
                {       "Help",         XtNumber(helpse_casc),  $H{xmbtq syse help menu help},  helpse_casc,    1};

static  pull_button     *viewmenlist[] = { &job_button, &srch_button, &helpv_button, NULL },
                        *syserrmenlist[] = { &file_button, &helpse_button, NULL };


static void  setup_menus(Widget panew, pull_button **menlist)
{
        XtWidgetGeometry        size;
        Widget                  helpw, result;
        pull_button             **item;

        result = XmCreateMenuBar(panew, "viewmenu", NULL, 0);

        /* Get rid of resize button for menubar */

        size.request_mode = CWHeight;
        XtQueryGeometry(result, NULL, &size);
        XtVaSetValues(result, XmNpaneMaximum, size.height*2, XmNpaneMinimum, size.height*2, NULL);

        /* Use the BuildPulldown in xmbtq.c */

        for  (item = menlist;  *item;  item++)
                if  ((helpw = BuildPulldown(result, *item)))
                        XtVaSetValues(result, XmNmenuHelpWidget, helpw, NULL);
        XtManageChild(result);
}

/* Read file to find where all the pages start.  */

static void  scanfile(FILE *fp)
{
        int     curline, ch;

        pagewidth = 0;
        linecount = 0;
        curline = 0;

        while  ((ch = getc(fp)) != EOF)  {
                switch  (ch)  {
                default:
                        if  (ch & 0x80)  {
                                ch &= 0x7f;
                                curline += 2;
                        }
                        if  (ch < ' ')
                                curline++;
                        curline++;
                        break;
                case  '\n':
                        if  (curline > pagewidth)
                                pagewidth = curline;
                        curline = 0;
                        linecount++;
                        break;
                case  '\t':
                        curline = (curline + 8) & ~7;
                        break;
                }
        }
}

/* Get line number lnum
   I never said this was brilliantly efficient.  */

static void  getline(const unsigned lnum)
{
        static  unsigned        lastlin = 0x7fff;
        int             ch, lng;

        if  (lastlin > lnum)  {
                lastlin = 0;
                rewind(infile);
        }
        for  (;  lastlin < lnum;  lastlin++)  {
                while  ((ch = getc(infile)) != '\n')
                        if  (ch == EOF)
                                goto  endf;
        }

        lng = 0;
        lastlin++;
        while  ((ch = getc(infile)) != '\n')  {
                switch  (ch)  {
                case  EOF:
                        goto  endf;
                case  '\t':
                        do  buffer[lng++] = ' ';
                        while  ((lng & 7) != 0);
                        break;
                default:
                        if  (ch & 0x80)  {
                                ch &= 0x7f;
                                buffer[lng++] = 'M';
                                buffer[lng++] = '-';
                        }
                        if  (ch < ' ')  {
                                buffer[lng++] = '^';
                                ch += ' ';
                        }
                        buffer[lng++] = (char) ch;
                }
        }
        buffer[lng] = '\0';
        return;
 endf:
        buffer[0] = '\0';
}

static void  redraw(Boolean doclear)
{
        int     yloc, yindx, xindx, lng, endy;

        if  (!XtWindow(drawing_a))
                return;

        yindx = pix_voffset / cell_height;
        xindx = pix_hoffset / cell_width;

        yloc = 0;
        endy = view_height;

        if  (doclear)
                goto  do_all;
        if  (last_left != pix_hoffset)
                goto  do_all;

        if  (last_top == pix_voffset)
                return; /* Nothing to do */

        if  (last_top < pix_voffset)  { /* Moved down file - move up unchanged bit */
                unsigned  delta = pix_voffset - last_top;
                unsigned  newyloc;
                if  (delta > view_height)
                        goto  do_all;
                newyloc = view_height - delta;

                XCopyArea(dpy, XtWindow(drawing_a), XtWindow(drawing_a), mygc, 0, delta, view_width, newyloc, 0, 0);

                /* Adjust position of last line in case window height
                   was such it got only half-drawn */

                yloc = newyloc - newyloc % cell_height;
                yindx += yloc / cell_height;
        }
        else  {
                unsigned  delta = last_top - pix_voffset;
                if  (delta > view_height)
                        goto  do_all;
                XCopyArea(dpy, XtWindow(drawing_a), XtWindow(drawing_a), mygc, 0, 0, view_width, view_height - delta, 0, delta);
                endy = delta;
        }

 do_all:
        XClearArea(dpy, XtWindow(drawing_a), 0, yloc, view_width, endy, False);
        while  (yloc < endy  &&  yindx < linecount)  {
                yloc += font->ascent;
                getline(yindx++);
                lng = strlen(buffer);
                if  (lng > xindx)  {
                        lng -= xindx;
                        XDrawImageString(dpy, XtWindow(drawing_a), mygc, 0, yloc, buffer + xindx, lng);
                }
                yloc += font->descent;
        }
        last_top = pix_voffset;
        last_left = pix_hoffset;
}

/* React to scrolling actions.  Reset position of Scrollbars; call
   redraw() to do actual scrolling.  cbs->value is Scrollbar's
   new position.  */

static void  scrolled(Widget scrollbar, int orientation, XmScrollBarCallbackStruct *cbs)
{
        if  (orientation == XmVERTICAL)
                pix_voffset = cbs->value * cell_height;
        else
                pix_hoffset = cbs->value * cell_width;
        redraw(False);
}

static void  expose_resize(Widget w, XtPointer unused, XmDrawingAreaCallbackStruct *cbs)
{
        Dimension   new_width, new_height, oldw, oldh;
        int         do_clear = 0;

        if  (!had_first)  {
                int             value;
                XGCValues       gcv;

                had_first++;
                XtVaGetValues(drawing_a,
                              XmNwidth, &view_width,
                              XmNheight, &view_height,
                              XtNforeground, &gcv.foreground,
                              XtNbackground, &gcv.background, NULL);
                vsb = XtVaCreateManagedWidget("vsb",
                                              xmScrollBarWidgetClass,   scrolled_w,
                                              XmNorientation,           XmVERTICAL,
                                              XmNmaximum,               linecount,
                                              XmNsliderSize,            my_max(1, my_min(view_height/cell_height, linecount)),
                                              NULL);

                hsb = XtVaCreateManagedWidget("hsb",
                                              xmScrollBarWidgetClass,   scrolled_w,
                                              XmNorientation,           XmHORIZONTAL,
                                              XmNmaximum,               pagewidth,
                                              XmNsliderSize,            my_max(1, my_min(view_width / cell_width, pagewidth)),
                                              NULL);
                pix_hoffset = pix_voffset = last_top = last_left = 0;
                XtAddCallback(vsb, XmNvalueChangedCallback, (XtCallbackProc) scrolled, (XtPointer) XmVERTICAL);
                XtAddCallback(hsb, XmNvalueChangedCallback, (XtCallbackProc) scrolled, (XtPointer) XmHORIZONTAL);
                XtAddCallback(vsb, XmNdragCallback, (XtCallbackProc) scrolled, (XtPointer) XmVERTICAL);
                XtAddCallback(hsb, XmNdragCallback, (XtCallbackProc) scrolled, (XtPointer) XmHORIZONTAL);
                gcv.font = font->fid;
                mygc = XCreateGC(dpy, XtWindow(toplevel), GCFont | GCForeground | GCBackground, &gcv);
                XmScrolledWindowSetAreas(scrolled_w, hsb, vsb, drawing_a);
                value = linecount - my_min(view_height/cell_height, linecount);
                if  (value > 0)  {
                        XtVaSetValues(vsb, XtNvalue, value, NULL);
                        pix_voffset = value * cell_height;
                }
        }

        if  (cbs->reason == XmCR_EXPOSE)  {
                redraw(True);
                return;
        }

        oldw = view_width;
        oldh = view_height;
        XtVaGetValues(drawing_a, XmNwidth, &view_width, XmNheight, &view_height, NULL);
        new_width = view_width / cell_width;
        new_height = view_height / cell_height;
        if  ((int) new_height >= linecount) {
                pix_voffset = 0;
                do_clear = 1;
                new_height = (Dimension) linecount;
        }
        else  {
                pix_voffset = my_min(pix_voffset, (linecount-new_height) * cell_height);
                if  (oldh > linecount * cell_height)
                        do_clear = 1;
        }
        XtVaSetValues(vsb,
                      XmNsliderSize,    my_max(new_height, 1),
                      XmNvalue,         pix_voffset / cell_height,
                      NULL);

        /* Identical to vertical case above */

        if  ((int) new_width >= pagewidth)  {
                pix_hoffset = 0;
                do_clear = 1;
                new_width = (Dimension) pagewidth;
        }
        else  {
                pix_hoffset = my_min(pix_hoffset, (pagewidth-new_width)*cell_width);
                if  (oldw > linepixwidth)
                        do_clear = 1;
        }
        XtVaSetValues(hsb,
                      XmNsliderSize,    my_max(new_width, 1),
                      XmNvalue,         pix_hoffset / cell_width,
                      NULL);

        if  (do_clear)
                redraw(True);
}

static void  endview(Widget w, int data)
{
        if  (buffer)  {
                free(buffer);
                buffer = NULL;
        }
        if  (infile)  {
                fclose(infile);
                infile = NULL;
        }
        if  (mygc)  {
                XFreeGC(dpy, mygc);
                mygc = NULL;
        }
        if  (matchtext)  {
                XtFree(matchtext);
                matchtext = NULL;
        }
        if  (data >= 0)
                XtDestroyWidget(GetTopShell(w));
        inprogress = 0;
}

static int  foundmatch()
{
        int     bufl = strlen(buffer);
        int     matchl = strlen(matchtext);
        int     endl = bufl - matchl;
        int     bpos, mpos;

        for  (bpos = 0;  bpos <= endl;  bpos++)  {
                int     boffset = bpos;
                for  (mpos = 0;  mpos < matchl;  mpos++)  {
                        int     mch = matchtext[mpos];
                        int     bch = buffer[mpos+boffset];
                        if  (!matchcase)  {
                                mch = toupper(mch);
                                bch = toupper(bch);
                        }
                        if  (!(mch == bch  ||  mch == '.'))
                                goto  nomatch;

                        /* Make one space in pattern match any number
                           of spaces in buffer */

                        if  (bch == ' ')
                                while  (buffer[mpos+boffset+1] == ' ')
                                        boffset++;
                }

                /* Success...  */

                return  bpos;
        nomatch:
                ;
        }

        return  -1;
}

static void  execute_search()
{
        int     mpos, redraw_needed = 0;
        unsigned   cline = last_matchline, view_cells;

        if  (sbackward)  {
                while  (cline != 0)  {
                        getline(--cline);
                        if  ((mpos = foundmatch()) >= 0)
                                goto  foundit;
                }
                if  (wraparound)  {
                        cline = linecount - 1;
                        while  (cline > last_matchline)  {
                                getline(cline);
                                if  ((mpos = foundmatch()) >= 0)
                                        goto  foundit;
                                cline--;
                        }
                }
                doerror(drawing_a, $EH{xmbtq string not found backw});
        }
        else  {
                while  (++cline < linecount)  {
                        getline(cline);
                        if  ((mpos = foundmatch()) >= 0)
                                goto  foundit;
                }
                if  (wraparound)  {
                        cline = 0;
                        while  (cline < last_matchline)  {
                                getline(cline);
                                if  ((mpos = foundmatch()) >= 0)
                                        goto  foundit;
                                cline++;
                        }
                }
                doerror(drawing_a, $EH{xmbtq string not found forw});
        }
        return;                 /* Drawn a blank */

 foundit:

        /* Found it.
           Now scroll the picture so that matched line is at the top */

        last_matchline = cline;
        view_cells = view_height / cell_height;

        if  (view_cells < linecount)  {
                if  (cline > linecount - view_cells)
                        cline = linecount - view_cells;
                pix_voffset = cline * cell_height;
                XtVaSetValues(vsb, XmNvalue, cline, NULL);
                redraw_needed++; /* Why cant the set values routine invoke the callback???? */
        }

        view_cells = view_width / cell_width;
        if  (view_cells < pagewidth)  {
                if  (mpos > pagewidth - view_cells)
                        mpos = pagewidth - view_cells;
                pix_hoffset = mpos * cell_width;
                XtVaSetValues(hsb, XmNvalue, mpos, NULL);
                redraw_needed++;
        }
        if  (redraw_needed)
                redraw(False);
}

static void  cb_vrsrch(Widget w, int data)
{
        if  (!matchtext  ||  matchtext[0] == '\0')  {
                doerror(w, $EH{xmbtq no search string yet});
                return;
        }
        sbackward = (char) data;
        execute_search();
}

static void  endsdlg(Widget w, int data)
{
        if  (data)  {
                if  (matchtext) /* Last time round */
                        XtFree(matchtext);
                XtVaGetValues(workw[WORKW_STXTW], XmNvalue, &matchtext, NULL);
                if  (matchtext[0] == '\0')  {
                        doerror(w, $EH{xmbtq null search string});
                        return;
                }
                sbackward = XmToggleButtonGadgetGetState(workw[WORKW_FORWW])? 0: 1;
                matchcase = XmToggleButtonGadgetGetState(workw[WORKW_MATCHW])? 1: 0;
                wraparound = XmToggleButtonGadgetGetState(workw[WORKW_WRAPW])? 1: 0;
                XtDestroyWidget(GetTopShell(w));
                last_matchline = pix_voffset / cell_height;
                execute_search();
        }
        else
                XtDestroyWidget(GetTopShell(w));
}

static void  cb_vsrchfor(Widget parent)
{
        Widget  s_shell, panew, formw;

        InitsearchDlg(parent, &s_shell, &panew, &formw, matchtext);
        InitsearchOpts(formw, workw[WORKW_STXTW], sbackward, matchcase, wraparound);
        CreateActionEndDlg(s_shell, panew, (XtCallbackProc) endsdlg, $H{xmbtq jsearch dialog});
}

static void  initview(Widget *shw, Widget *pw, char *appname)
{
        Widget  v_shell;

        if  (!font)  {
                font = XLoadQueryFont(dpy, "fixed");
                cell_width = font->max_bounds.width;
                cell_height = font->ascent + font->descent;
        }

        linepixwidth = cell_width * pagewidth;

        *shw = v_shell = XtVaAppCreateShell(appname, NULL, topLevelShellWidgetClass, dpy, NULL);

        if  (!docbitmap)
                docbitmap = XCreatePixmapFromBitmapData(dpy,
                                                        RootWindowOfScreen(XtScreen(toplevel)),
                                                        xmbtqdoc_bits,
                                                        xmbtqdoc_width,
                                                        xmbtqdoc_height,
                                                        1, 0, 1);

        XtVaSetValues(v_shell, XmNiconPixmap, docbitmap, NULL);

        *pw = XtVaCreateWidget("pane",
                               xmPanedWindowWidgetClass,        v_shell,
                               XmNsashWidth,                    1,
                               XmNsashHeight,                   1,
                               NULL);
}

static void  setup_draw()
{
        drawing_a = XtVaCreateManagedWidget("vdraw",
                                            xmDrawingAreaWidgetClass,           scrolled_w,
                                            NULL);
        XtAddCallback(drawing_a, XmNexposeCallback, (XtCallbackProc) expose_resize, (XtPointer) 0);
        XtAddCallback(drawing_a, XmNresizeCallback, (XtCallbackProc) expose_resize, (XtPointer) 0);
}

static void  EndSetupViewDlg(Widget shelldlg, Widget panew, XtCallbackProc endrout, int helpcode)
{
        Widget  actform, okw, cancw, helpw;
        Dimension       h;

        actform = XtVaCreateWidget("actform",
                                   xmFormWidgetClass,           panew,
                                   XmNfractionBase,             7,
                                   NULL);

        okw = XtVaCreateManagedWidget("Ok",
                                      xmPushButtonGadgetClass,  actform,
                                      XmNtopAttachment,         XmATTACH_FORM,
                                      XmNbottomAttachment,      XmATTACH_FORM,
                                      XmNleftAttachment,        XmATTACH_POSITION,
                                      XmNleftPosition,          1,
                                      XmNrightAttachment,       XmATTACH_POSITION,
                                      XmNrightPosition,         2,
                                      XmNshowAsDefault,         True,
                                      XmNdefaultButtonShadowThickness,  1,
                                      XmNtopOffset,             0,
                                      XmNbottomOffset,          0,
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

        XtAddCallback(okw, XmNactivateCallback, endrout, INT_TO_XTPOINTER(1));
        XtAddCallback(cancw, XmNactivateCallback, endrout, (XtPointer) 0);
        XtAddCallback(helpw, XmNactivateCallback, (XtCallbackProc) dohelp, INT_TO_XTPOINTER(helpcode));
        XtManageChild(actform);
        XtVaGetValues(cancw, XmNheight, &h, NULL);
        XtVaSetValues(actform, XmNpaneMaximum, h, XmNpaneMinimum, h, NULL);
        XtManageChild(panew);
        XtPopup(shelldlg, XtGrabNone);
}

void  cb_view()
{
        Widget  v_shell, panew, formw, previous;
        BtjobRef        cj;

        if  (inprogress)  {
                doerror(jwid, $EH{xmbtq view in progress});
                return;
        }
        inprogress++;
        had_first = 0;
        cj = getselectedjob(BTM_READ);
        if  (!cj)  {
                inprogress = 0;
                return;
        }

        /* If it's a remote file slurp it up into a temporary file */

        if  (cj->h.bj_hostid)  {
                FILE    *ifl;
                int     kch;
                if  (!(ifl = net_feed(FEED_JOB, cj->h.bj_hostid, cj->h.bj_job, Job_seg.dptr->js_viewport)))  {
                        disp_arg[0] = cj->h.bj_job;
                        disp_str = qtitle_of(cj);
                        disp_str2 = look_host(cj->h.bj_hostid);
                        doerror(jwid, $EH{xmbtq cannot open net file});
                        inprogress = 0;
                        return;
                }
                infile = tmpfile();
                while  ((kch = getc(ifl)) != EOF)
                        putc(kch, infile);
                fclose(ifl);
                rewind(infile);
        }
        else  if  (!(infile = fopen(mkspid(SPNAM, cj->h.bj_job), "r")))  {
                disp_arg[0] = cj->h.bj_job;
                doerror(jwid, $EH{xmbtq cannot open job file});
                inprogress = 0;
                return;
        }

        scanfile(infile);
        if  (linecount == 0 || pagewidth == 0)  {
                doerror(jwid, $EH{xmbtq empty job file});
                fclose(infile);
                inprogress = 0;
                return;
        }
        if  (!(buffer = malloc(pagewidth+1)))
                ABORT_NOMEM;

        initview(&v_shell, &panew, "xmbtqview");
        setup_menus(panew, viewmenlist);

        formw = XtVaCreateWidget("form",
                                 xmFormWidgetClass,             panew,
                                 XmNfractionBase,               3,
                                 NULL);

        /* Build Jobno, title, user left to right */

        previous = CreateJtitle(formw, cj);
        previous = place_label_top(formw, previous, "Header");
        previous = XtVaCreateManagedWidget("hdr",
                                           xmTextFieldWidgetClass,      formw,
                                           XmNcursorPositionVisible,    False,
                                           XmNeditable,                 False,
                                           XmNtopAttachment,            XmATTACH_FORM,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               previous,
                                           NULL);

        XmTextSetString(previous, (char *) qtitle_of(cj));

        previous = place_label_top(formw, previous, "User");
        previous = XtVaCreateManagedWidget("user",
                                           xmTextFieldWidgetClass,      formw,
                                           XmNcolumns,                  UIDSIZE,
                                           XmNmaxWidth,                 UIDSIZE,
                                           XmNcursorPositionVisible,    False,
                                           XmNeditable,                 False,
                                           XmNtopAttachment,            XmATTACH_FORM,
                                           XmNleftAttachment,           XmATTACH_WIDGET,
                                           XmNleftWidget,               previous,
                                           NULL);

        XmTextSetString(previous, cj->h.bj_mode.o_user);

        scrolled_w = XtVaCreateManagedWidget("vscroll",
                                             xmScrolledWindowWidgetClass,       formw,
                                             XmNscrollingPolicy,                XmAPPLICATION_DEFINED,
                                             XmNvisualPolicy,                   XmVARIABLE,
                                             XmNtopAttachment,                  XmATTACH_WIDGET,
                                             XmNtopWidget,                      previous,
                                             XmNbottomAttachment,               XmATTACH_FORM,
                                             XmNleftAttachment,                 XmATTACH_FORM,
                                             XmNrightAttachment,                XmATTACH_FORM,
                                             NULL);

        setup_draw();
        XtManageChild(formw);
        EndSetupViewDlg(v_shell, panew, (XtCallbackProc) endview, $H{xmbtq view job help});
        XtAddCallback(v_shell, XmNpopdownCallback, (XtCallbackProc) endview, (XtPointer) 0);
        XtAddCallback(v_shell, XmNdestroyCallback, (XtCallbackProc) endview, (XtPointer) -1);
}

static void  clearlog(Widget w)
{
        if  (!Confirm(w, $PH{xmbtq confirm delete log file}))
                return;
        unlink(REPFILE);
        endview(w, 1);
}

void  cb_syserr()
{
        Widget  v_shell, panew;

        if  (!(infile = fopen(REPFILE, "r")))  {
                doerror(jwid, $EH{xmbtq cannot open syslog});
                return;
        }
        if  (inprogress)  {
                doerror(jwid, $EH{xmbtq syse view in progress});
                return;
        }
        inprogress++;
        had_first = 0;
        scanfile(infile);

        if  (linecount == 0 || pagewidth == 0)  {
                doerror(jwid, $EH{xmbtq cannot open syslog});
                fclose(infile);
                inprogress = 0;
                return;
        }

        if  (!(buffer = malloc(pagewidth+1)))
                ABORT_NOMEM;
        initview(&v_shell, &panew, "xmbtqverr");
        setup_menus(panew, syserrmenlist);
        scrolled_w = XtVaCreateManagedWidget("vscroll",
                                             xmScrolledWindowWidgetClass,       panew,
                                             XmNscrollingPolicy,                XmAPPLICATION_DEFINED,
                                             XmNvisualPolicy,                   XmVARIABLE,
                                             NULL);
        setup_draw();
        EndSetupViewDlg(v_shell, panew, (XtCallbackProc) endview, $H{xmbtq view syserr help});
        XtAddCallback(v_shell, XmNpopdownCallback, (XtCallbackProc) endview, (XtPointer) 0);
        XtAddCallback(v_shell, XmNdestroyCallback, (XtCallbackProc) endview, (XtPointer) -1);
}

static  int     hfid = -1, year, changes = 0;
static  HelpaltRef Monthnames;
static  unsigned  char  *yearmap;
static  unsigned  char  ***monthd;
static  Widget  yearw, calw;

#define ppermitted(flg) (mypriv->btu_priv & flg)

#define DAYSET_FLAG     0x80
#define ISHOL(map, day)         map[(day) >> 3] & (1 << (day & 7))
#define SETHOL(map, day)        map[(day) >> 3] |= (1 << (day & 7))

static void  fillyear()
{
        int     startday = (1 + (year - 90) + (year - 89) / 4) % 7;
        int     month, week, yday = 0, mday, wday, daysinmon;
        month_days[1] = year % 4 == 0? 29: 28;

        for  (month = 0;  month < 12;  month++)  {
                daysinmon = month_days[month];
                BLOCK_ZERO(monthd[month][0], 7);
                BLOCK_ZERO(monthd[month][4], 7);
                mday = 1;
                for  (wday = startday;  wday < 7;  wday++)  {
                        monthd[month][0][wday] = mday++;
                        if  (ISHOL(yearmap, yday))
                                monthd[month][0][wday] |= DAYSET_FLAG;
                        yday++;
                }
                for  (week = 1;  week < 5;  week++)
                        for  (wday = 0;  wday < 7;  wday++)  {
                                monthd[month][week][wday] = mday++;
                                if  (ISHOL(yearmap, yday))
                                        monthd[month][week][wday] |= DAYSET_FLAG;
                                yday++;
                                if  (mday > daysinmon)
                                        goto  dun;
                        }
                for  (wday = 0;  wday < 7;  wday++)  {
                        monthd[month][0][wday] = mday++;
                        if  (ISHOL(yearmap, yday))
                                monthd[month][0][wday] |= DAYSET_FLAG;
                        yday++;
                        if  (mday > daysinmon)
                                break;
                }
        dun:
                startday = (startday + daysinmon) % 7;
        }
}

static void  readyear()
{
        int     startday = (1 + (year - 90) + (year - 89) / 4) % 7;
        int     month, week, yday = 0, mday, wday, daysinmon;
        month_days[1] = year % 4 == 0? 29: 28;

        BLOCK_ZERO(yearmap, YVECSIZE);
        for  (month = 0;  month < 12;  month++)  {
                daysinmon = month_days[month];
                mday = 1;
                for  (wday = startday;  wday < 7;  wday++)  {
                        if  (monthd[month][0][wday] & DAYSET_FLAG)
                                SETHOL(yearmap, yday);
                        mday++;
                        yday++;
                }
                for  (week = 1;  week < 5;  week++)
                        for  (wday = 0;  wday < 7;  wday++)  {
                                if  (monthd[month][week][wday] & DAYSET_FLAG)
                                        SETHOL(yearmap, yday);
                                yday++;
                                mday++;
                                if  (mday > daysinmon)
                                        goto  dun;
                        }
                for  (wday = 0;  wday < 7;  wday++)  {
                        if  (monthd[month][0][wday] & DAYSET_FLAG)
                                SETHOL(yearmap, yday);
                        yday++;
                        mday++;
                        if  (mday > daysinmon)
                                break;
                }
        dun:
                startday = (startday + daysinmon) % 7;
        }
}

#define MONTH_WID               (7*3 - 1)
#define INTERMONTH_GAP          4
#define INTERQUARTER_GAP        1
#define ROW_WID                 (3*MONTH_WID + 2*INTERMONTH_GAP + 1)
#define MONTH_HEIGHT            6
#define QUARTER_SIZE            (6 * ROW_WID + INTERQUARTER_GAP)

static int  space(int pos, int n, char *buff)
{
        while  (n > 0)  {
                buff[pos++] = ' ';
                n--;
        }
        return  pos;
}

static int  copyin(int pos, char *buff, char *from)
{
        while  (*from)
                buff[pos++] = *from++;
        return  pos;
}

static void  displayyear()
{
        int     quarter, monq, month, wday, week, lng;
        int     w_pos;
        char    nbuf[5];
        char    yearbuf[QUARTER_SIZE*4];

        sprintf(nbuf, "%d", year+1900);
        XmTextSetString(yearw, nbuf);
        w_pos = 0;

        for  (quarter = 0;  quarter < 12;  quarter += 3)  {

                /* Insert centred month names */

                for  (monq = 0;  monq < 3;  monq++)  {
                        char    *mnam;
                        int     startpos = w_pos;
                        month = quarter + monq;
                        mnam = disp_alt(month, Monthnames);
                        lng = strlen(mnam);
                        w_pos = space(w_pos, (MONTH_WID - lng) / 2, yearbuf);
                        w_pos = copyin(w_pos, yearbuf, mnam);
                        w_pos = space(w_pos, MONTH_WID - (w_pos - startpos), yearbuf);
                        if  (monq < 2)
                                w_pos = space(w_pos, INTERMONTH_GAP, yearbuf);
                }
                yearbuf[w_pos++] = '\n';

                for  (week = 0;  week < 5;  week++)  {
                        for  (monq = 0;  monq < 3;  monq++)  {
                                month = quarter + monq;
                                for  (wday = 0;  wday < 7;  wday++)  {
                                        int     dayno = monthd[month][week][wday], mday;
                                        if  ((mday = dayno & ~DAYSET_FLAG) != 0)  {
                                                sprintf(nbuf, "%2d", mday);
                                                w_pos = copyin(w_pos, yearbuf, nbuf);
                                        }
                                        else
                                                w_pos = space(w_pos, 2, yearbuf);
                                        if  (wday < 6)
                                                w_pos = space(w_pos, 1, yearbuf);
                                }
                                if  (monq < 2)
                                        w_pos = space(w_pos, INTERMONTH_GAP, yearbuf);
                        }
                        yearbuf[w_pos++] = '\n';
                }
                if  (quarter < 9)
                        for  (monq = 0;  monq < INTERQUARTER_GAP;  monq++)
                                yearbuf[w_pos++] = '\n';
        }
        yearbuf[w_pos] = '\0';
        XtVaSetValues(calw, XmNvalue, yearbuf, NULL);
        XmTextSetHighlight(calw, 0, w_pos, XmHIGHLIGHT_NORMAL);

        /* Set up highlighting */

        w_pos = 0;

        for  (quarter = 0;  quarter < 12;  quarter += 3)  {
                w_pos += ROW_WID;
                for  (week = 0;  week < 5;  week++)  {
                        for  (monq = 0;  monq < 3;  monq++)  {
                                month = quarter + monq;
                                for  (wday = 0;  wday < 7;  wday++)  {
                                        int     dayno = monthd[month][week][wday];
                                        if  (dayno & DAYSET_FLAG)
                                                XmTextSetHighlight(calw, w_pos, w_pos+2, XmHIGHLIGHT_SELECTED);
                                        w_pos += 2;
                                        if  (wday < 6)
                                                w_pos++;
                                }
                                if  (monq < 2)
                                        w_pos += INTERMONTH_GAP;
                        }
                        w_pos++;
                }
                w_pos += INTERQUARTER_GAP;
        }
}

static void  selectit(Widget w, int toset)
{
        XmTextPosition  where = XmTextGetCursorPosition(calw);
        int     quarter, qq, week, column, monq, day, month;

        quarter = where / QUARTER_SIZE;
        qq = where % QUARTER_SIZE;
        week = qq / ROW_WID;
        column = qq % ROW_WID;

        if  (week <= 0)         /* On title */
                return;
        week--;
        monq = column / (MONTH_WID + INTERMONTH_GAP);
        column %= MONTH_WID + INTERMONTH_GAP;
        day = column / 3;
        if  (column % 3 == 0 || day >= 7)       /* On space */
                return;

        month = quarter * 3 + monq;
        if  (monthd[month][week][day] == 0)     /* No day */
                return;
        if  (monthd[month][week][day] & DAYSET_FLAG)  {
                if  (toset)
                        return;
                monthd[month][week][day] &= ~DAYSET_FLAG;
        }
        else  {
                if  (!toset)
                        return;
                monthd[month][week][day] |= DAYSET_FLAG;
        }
        changes++;
        if  (column % 3 == 2)   /* 2nd digit */
                where--;
        XmTextSetHighlight(calw, where-1, where+1, toset? XmHIGHLIGHT_SELECTED: XmHIGHLIGHT_NORMAL);
}

void  do_caltog(Widget wid, XEvent *xev, String *args, Cardinal *nargs)
{
        XButtonPressedEvent  *bpe = (XButtonPressedEvent *) xev;
        XmTextPosition  where = XmTextXYToPos(calw, bpe->x, bpe->y);
        int     quarter, qq, week, column, monq, day, month, toset;

        quarter = where / QUARTER_SIZE;
        qq = where % QUARTER_SIZE;
        week = qq / ROW_WID;
        column = qq % ROW_WID;

        if  (week <= 0)         /* On title */
                return;
        week--;
        monq = column / (MONTH_WID + INTERMONTH_GAP);
        column %= MONTH_WID + INTERMONTH_GAP;
        day = column / 3;
        if  (column % 3 == 0 || day >= 7)       /* On space */
                return;

        month = quarter * 3 + monq;
        if  (monthd[month][week][day] == 0)     /* No day */
                return;
        monthd[month][week][day] ^= DAYSET_FLAG;
        toset = monthd[month][week][day] & DAYSET_FLAG;
        changes++;
        if  (column % 3 == 2)   /* 2nd digit */
                where--;
        XmTextSetHighlight(calw, where-1, where+1, toset? XmHIGHLIGHT_SELECTED: XmHIGHLIGHT_NORMAL);
}

static void  chngyear(Widget w, int incdec)
{
        int     newyear = year + incdec;

        /* Ignore years before 1990 and after 2037 as funny things
           happen to Unix time in 2038 as it ceases to fit into a
           +ve signed long.  (To be precise at 3:13am on Tuesday
           19th January 2038).  */

        if  (newyear < 90  ||  newyear >= 138)
                return;

        if  (changes)  {
                readyear();
                lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
                write(hfid, (char *) yearmap, YVECSIZE);
                changes = 0;
        }

        year = newyear;
        lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
        if  (read(hfid, (char *) yearmap, YVECSIZE) != YVECSIZE)
                BLOCK_ZERO(yearmap, YVECSIZE);
        fillyear();
        displayyear();
}

static void  endh(Widget w, int data)
{
        if  (data  &&  changes)  {
                readyear();
                lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
                write(hfid, (char *) yearmap, YVECSIZE);
                changes = 0;
        }

        if  (hfid > 0)  {
                close(hfid);
                hfid = -1;
        }

        /* Free the vectors we allocated */

        if  (yearmap)  {
                free((char *) yearmap);
                yearmap = (unsigned char *) 0;
        }

        if  (monthd)  {
                int     month, week;
                for  (month = 0;  month < 12;  month++)  {
                        for  (week = 0;  week < 5;  week++)
                                free((char *) monthd[month][week]);
                        free((char *) monthd[month]);
                }
                free((char *) monthd);
                monthd = (unsigned char ***) 0;
        }
        XtDestroyWidget(GetTopShell(w));
}

void  cb_hols(Widget parent)
{
        int     readonly, month;
        char    *fname = envprocess(HOLFILE);
        time_t  now = time((time_t *) 0);
        struct  tm  *tp = localtime(&now);
        Widget  h_shell, panew, formw, prevy, nexty, selb, uselb;

        year = tp->tm_year;

        if  ((readonly = !ppermitted(BTM_WADMIN)))  {
                hfid = open(fname, O_RDONLY);
                free(fname);
                if  (hfid < 0)  {
                        doerror(jwid, $EH{xmbtq cannot open hols});
                        return;
                }
        }
        else  {
                USHORT  oldumask = umask(0);
                hfid = open(fname, O_RDWR|O_CREAT, 0644);
#ifndef HAVE_FCHOWN
                if  (Realuid == ROOTID)
                        chown(fname, Daemuid, Realgid);
#endif
                umask(oldumask);
                free(fname);
                if  (hfid < 0)  {
                        doerror(jwid, $EH{xmbtq cannot create hols});
                        return;
                }
#ifdef  HAVE_FCHOWN
                if  (Realuid == ROOTID)
                        fchown(hfid, Daemuid, Realgid);
#endif
        }

        /* Set up monthd and year map vectors */

        if  (!Monthnames)
                Monthnames = helprdalt($Q{Months full});

        if  (!(yearmap = (unsigned char *) malloc(YVECSIZE * sizeof(unsigned char))))
                ABORT_NOMEM;

        if  (!(monthd = (unsigned char ***) malloc(12 * sizeof(unsigned char **))))
                ABORT_NOMEM;

        for  (month = 0;  month < 12;  month++)  {
                int     week;
                if  (!(monthd[month] = (unsigned char **) malloc(5 * sizeof(unsigned char *))))
                        ABORT_NOMEM;
                for  (week = 0;  week < 5;  week++)
                        if  (!(monthd[month][week] = (unsigned char *) malloc(7 * sizeof(unsigned char))))
                                ABORT_NOMEM;
        }

        /* Read existing holiday vector from file (or make one up with
           no holidays) */

        lseek(hfid, (long) ((year - 90) * YVECSIZE), 0);
        if  (read(hfid, (char *) yearmap, YVECSIZE) != YVECSIZE)
                BLOCK_ZERO(yearmap, YVECSIZE);

        /* Set up our windows */

        CreateEditDlg(parent, "holidays", &h_shell, &panew, &formw, 3);

        /* Create title */

        prevy = XtVaCreateManagedWidget("prevy",
                                        xmArrowButtonWidgetClass,       formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_FORM,
                                        XmNarrowDirection,              XmARROW_LEFT,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        selb = XtVaCreateManagedWidget("selh",
                                       xmPushButtonWidgetClass,         formw,
                                       XmNtopAttachment,                XmATTACH_FORM,
                                       XmNleftAttachment,               XmATTACH_WIDGET,
                                       XmNleftWidget,                   prevy,
                                       XmNtopOffset,                    0,
                                       XmNbottomOffset,                 0,
                                       NULL);

        yearw = XtVaCreateManagedWidget("year",
                                        xmTextFieldWidgetClass,         formw,
                                        XmNcolumns,                     4,
                                        XmNcursorPositionVisible,       False,
                                        XmNeditable,                    False,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  selb,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        uselb = XtVaCreateManagedWidget("uselh",
                                        xmPushButtonWidgetClass,        formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  yearw,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        nexty = XtVaCreateManagedWidget("nexty",
                                        xmArrowButtonWidgetClass,       formw,
                                        XmNtopAttachment,               XmATTACH_FORM,
                                        XmNleftAttachment,              XmATTACH_WIDGET,
                                        XmNleftWidget,                  uselb,
                                        XmNarrowDirection,              XmARROW_RIGHT,
                                        XmNtopOffset,                   0,
                                        XmNbottomOffset,                0,
                                        NULL);

        XtAddCallback(prevy, XmNactivateCallback, (XtCallbackProc) chngyear, (XtPointer) -1);
        XtAddCallback(nexty, XmNactivateCallback, (XtCallbackProc) chngyear, (XtPointer) 1);
        changes = 0;
        if  (readonly)  {
                XtSetSensitive(selb, False);
                XtSetSensitive(uselb, False);
        }
        else  {
                XtAddCallback(selb, XmNactivateCallback, (XtCallbackProc) selectit, (XtPointer) 1);
                XtAddCallback(uselb, XmNactivateCallback, (XtCallbackProc) selectit, (XtPointer) 0);
        }

        calw = XtVaCreateManagedWidget("cal",
                                       xmTextWidgetClass,               formw,
                                       XmNeditable,                     False,
                                       XmNtopAttachment,                XmATTACH_WIDGET,
                                       XmNtopWidget,                    yearw,
                                       XmNleftAttachment,               XmATTACH_FORM,
                                       XmNrightAttachment,              XmATTACH_FORM,
                                       XmNrows,                         MONTH_HEIGHT * 4 + INTERQUARTER_GAP*3,
                                       XmNcolumns,                      ROW_WID,
                                       XmNeditMode,                     XmMULTI_LINE_EDIT,
                                       NULL);
        fillyear();
        displayyear();
        XtManageChild(formw);
        CreateActionEndDlg(h_shell, panew, (XtCallbackProc) endh, $H{xmbtq holidays dialog});
}
