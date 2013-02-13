/* xbtu_ext.h -- external defs for gbch-xuser

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

extern  unsigned        loadstep;       /* Rounding for load level */

extern  int     hchanges,       /* Had changes to default */
                uchanges;       /* Had changes to user(s) */

extern  char            alphsort;
#define SRT_NONE        0       /* Sort by numeric uid (default) */
#define SRT_USER        1       /* Sort by user name */
#define SRT_GROUP       2       /* Sort by group name */
extern  Btdef           Btuhdr;
extern  BtuserRef       ulist;

struct  privabbrev      {
        ULONG           priv_flag;
        int             priv_mcode;
        char            *priv_name;
        char            *priv_abbrev;
        GtkWidget       *priv_widget;
};

extern  struct  privabbrev      privnames[];

extern  GtkWidget       *toplevel,
                        *dwid,          /* Default list */
                        *uwid;          /* User scroll list */

extern  GtkListStore            *raw_ulist_store;
extern  GtkTreeModelSort        *ulist_store;

extern void  cb_pris(GtkAction *);
extern void  cb_loadlev(GtkAction *);
extern void  cb_jmode(GtkAction *);
extern void  cb_vmode(GtkAction *);
extern void  cb_priv(GtkAction *);
extern void  cb_copyall();
extern void  cb_copydef();
extern void  defdisplay();
extern void  update_all_users();
extern void  update_selected_users();

#define INDEX_COL       0
#define UID_COL         1
#define GID_COL         2
#define USNAM_COL       3
#define GRPNAM_COL      4
#define DEFP_COL        5
#define MINP_COL        6
#define MAXP_COL        7
#define MAXLL_COL       8
#define TOTLL_COL       9
#define SPECLL_COL      10
#define P_COL           11
