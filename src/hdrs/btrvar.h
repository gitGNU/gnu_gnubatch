/* btrvar.h -- Structures for remembering variables for conditions and assignments

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

#ifdef  __cplusplus
struct  vdescr  {
        unsigned  char  crit;   // Critical flag
        char    *var;           // Name to look up later
        netid_t hostid;         // Machine its on 0 if this one
};

struct  scond : public vdescr   {
        SHORT   compar;         // Comparison
        Btcon   value;          // Value to compare
};

struct  Sass : public vdescr    {
        USHORT  flags;
        USHORT  op;
        Btcon   con;
};

extern  scond   Condlist[];
extern  Sass    Asslist[];

#else
struct  vdescr  {
        unsigned  char  crit;   /* Critical flag */
        char    *var;           /* Name to look up later */
        netid_t hostid;         /* Machine it's on 0 if this one */
};

struct  scond   {
        SHORT   compar;         /* Comparison */
        struct  vdescr  vd;
        Btcon   value;          /* Value to compare */
};

struct  Sass    {
        USHORT  flags;
        USHORT  op;
        struct  vdescr  vd;
        Btcon   con;
};

extern  unsigned        sflags;
extern  struct  scond   Condlist[];
extern  struct  Sass    Asslist[];

#endif

extern  int     Argcnt, Redircnt, Condcnt, Asscnt;
