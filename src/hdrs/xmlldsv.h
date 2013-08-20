/* xmlldsv.h -- load and save jobs as XML files.

   Copyright 2013 Free Software Foundation, Inc.

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


extern  void    init_xml();
extern  int     save_job_xml(CBtjobRef, const char *, const unsigned, const char *, const int, const int);
extern  int     save_job_xml_file(CBtjobRef, const char *, const unsigned, FILE *, const int, const int);
extern  int     load_job_xml(const char *, BtjobRef, char **, int *);
extern  int     load_job_xml_fd(const int, BtjobRef, char **, int *);

#define         XML_INVALID_FORMAT_FILE -1
#define         XML_INVALID_CONDS       -2
#define         XML_INVALID_ASSES       -3
#define         XML_TOOMANYSTRINGS      -4
#define         XML_NOTIMPL             -5
