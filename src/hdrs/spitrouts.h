/* spitrouts.h -- routines for dumping out stuff to file

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

extern const char * const condname[];
extern const char * const assname[];

void  spitbtrstr(const int, FILE *, const int);

void  dumpcon(FILE *, BtconRef);
void  dumpstr(FILE *, char *);
void  dumpnstr(FILE *, char *, int);
void  dumpmode(FILE *, char *, unsigned);
void  dumpemode(FILE *, const char, const char, const int, unsigned);
void  dumptime(FILE *, CTimeconRef);
void  dumpconds(FILE *, JcondRef);
void  dumpasses(FILE *, JassRef);
void  dumpecrun(FILE *, CBtjobRef);
void  dumpredirs(FILE *, CBtjobRef);
void  dumpargs(FILE *, CBtjobRef);

int  spit_msg(FILE *, CBtjobRef, int);
int  spit_time(FILE *, CTimeconRef, int, const ULONG, const int);
void  spit_redir(FILE *, const unsigned, const unsigned, const unsigned, const char *);
void  spit_pparm(FILE *, CBtjobhRef, const ULONG);
void  spit_cond(FILE *, const unsigned, const unsigned, const netid_t, char *, BtconRef);
void  spit_ass(FILE *, const unsigned, const unsigned, const unsigned, const netid_t, char *, BtconRef);
