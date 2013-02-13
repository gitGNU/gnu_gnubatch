/* xihtmllib.h -- HTML routine library defs

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

#define RECURSE_MAX     10
#define HINILWIDTH      2048

#define HTML_TMPNAME    "tempfiles"

#define DEFLT_COOKEXP_DAYS      10              /* Default time for cookie expiry */

struct  posttab  {
        char    *postname;      /* Name in pair */
        void    (*post_fn)(char *);
        char    **postfile;     /* For file name controls - temp file name inserted */
};

extern void  html_error(const char *);
extern void  html_nomem(const char *, const int);
#define ABORT_HTML_NOMEM        html_nomem(Filename, __LINE__)
extern int      html_getline(char *);
extern void     html_openini();
extern void     html_closeini();
extern int      html_iniparam(const char *, char *);
extern int      html_inibool(const char *, const int);
extern long     html_iniint(const char *, const int);
extern char     *html_inistr(const char *, char *);
extern char     *html_inifile(const char *, char *);
extern int      html_cookexpiry();
extern int      html_output_file(const char *, const int);
extern void     html_out_or_err(const char *, const int);
extern int      html_out_param_file(const char *, const int, const ULONG, const ULONG);
extern int      html_out_cparam_file(const char *, const int, const char *);
extern void     html_disperror(const int);
extern void     html_fldprint(const char *);
extern void     html_pre_putchar(const int);
extern int      html_getpostline(char *);
extern void     html_convert(const char *, char *);
extern char     **html_getvalues(const char *);
extern void     html_postvalues(struct posttab *);
