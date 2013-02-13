/* xmbtu_ext.h -- External symbols for xmbtuser

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
        int             priv_workw;
        char            *buttname;
        char            *priv_abbrev;
};

extern  struct  privabbrev      privnames[];

/* X stuff */

extern  Widget  dwid,           /* Default list */
                uwid;           /* User scroll list */

#define WORKW_SORTU     0
#define WORKW_SORTG     1
#define WORKW_LOADLS    2
#define WORKW_MINPW     0
#define WORKW_DEFPW     1
#define WORKW_MAXPW     2
#define WORKW_MAXLLPW   0
#define WORKW_TOTLLPW   1
#define WORKW_SPECLLPW  2
#define WORKW_RADMIN    0
#define WORKW_WADMIN    1
#define WORKW_CREATE    2
#define WORKW_SPCREATE  3
#define WORKW_SSTOP     4
#define WORKW_UMASK     5
#define WORKW_ORUG      6
#define WORKW_ORUO      7
#define WORKW_ORGO      8
#define WORKW_IMPW      0

extern void  cb_cdisplay(Widget, int);
extern void  cb_disporder(Widget);
extern void  cb_pris(Widget, int);
extern void  cb_loadlev(Widget, int);
extern void  cb_macrou(Widget, int);
extern void  cb_mode(Widget, int);
extern void  cb_priv(Widget, int);
extern void  cb_copydef(Widget, int);
extern void  defdisplay();
extern void  displaybusy(const int);
extern void  cb_srchfor(Widget);
extern void  cb_rsrch(Widget, int);
extern void  udisplay(int, int *);
extern int  sort_id(BtuserRef, BtuserRef);
extern int  sort_u(BtuserRef, BtuserRef);
extern int  sort_g(BtuserRef, BtuserRef);

#define SORT_NONE       0       /* Sort by numeric uid (default) */
#define SORT_USER       1       /* Sort by user name */
#define SORT_GROUP      2       /* Sort by group name */
