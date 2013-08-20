/* xbq_ext.h -- External symbols for xbtq

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

extern  char    *spdir;         /* Spool directory, typically /usr/spool/batch */

extern  int     Ctrl_chan;
extern  Shipc   Oreq;

extern  int             Const_val;
extern  char   xml_format;      /* Use single XML files for unqueue */
extern  char   Dirty;           /* Unsaved option changes */

/* X stuff */

extern  GtkWidget       *toplevel,      /* Main window */
                        *jwid,          /* Job scroll list */
                        *vwid;          /* Variable scroll list */

extern  GtkListStore    *jlist_store,
                        *vlist_store;

enum    jvrend_t  { JVREND_TEXT, JVREND_PROGRESS, JVREND_TOGGLE  };

struct  jvlist_elems  {
        GType   type;                   /* Type of element for settling up list store */
        enum    jvrend_t     rendtype;  /* Renderer type for treeview */
        int     colnum;                 /* Column number in treeview */
        int     msgcode;                /* Message prompt number */
        int     sortid;                 /* Sort id where applicable */
        char    *msgtext;               /* Message text */
        char    *descr;                 /* Full description for menu */
        GtkWidget  *menitem;            /* Menu item */
};

#define DEF_DLG_HPAD    5
#define DEF_DLG_VPAD    5
#define DEF_BUTTON_PAD  3

/* This is the column in the list stores we use to remember the sequence number */

#define SEQ_COL         0

#define MAXMACS         10

struct  macromenitem  {
        char    *cmd;
        char    *descr;
        unsigned  mergeid;
};

extern int  add_macro_to_list(const char *, const char, struct macromenitem *);

extern void  msg_error();
extern void  qwjimsg(const unsigned, CBtjobRef);
extern void  wjmsg(const unsigned, const ULONG);

extern void  qdojerror(unsigned, BtjobRef);
extern int  chk_okcidel(const int);
extern const char *qtitle_of(CBtjobRef);

extern BtjobRef  getselectedjob(unsigned);
extern BtvarRef  getselectedvar(unsigned);

extern GtkWidget *start_jobdlg(CBtjobRef, const int, const int);

