/* vfmt_export.c -- display export for variable display

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

VFORMAT(fmt_export)
{
#ifdef  BTVLIST_INLINE
        static  char    *exportmark;
        if  (!exportmark)
                exportmark = gprompt($P{Variable exported flag});
#endif
        return  vp->var_flags & VF_EXPORT? (fmt_t) strlen(strcpy(bigbuff, exportmark)): 0;
}
VFORMAT(fmt_cluster)
{
#ifdef  BTVLIST_INLINE
        static  char    *clustermark;
        if  (!clustermark)
                clustermark = gprompt($P{Variable clustered flag});
#endif
        return  vp->var_flags & VF_CLUSTER? (fmt_t) strlen(strcpy(bigbuff, clustermark)): 0;
}
