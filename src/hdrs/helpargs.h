/* helpargs.h -- helpfile argument defs

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

#define MAXARGNAME      30

#define HELPLETTER      'A'             /* Look for this letter in file */
#define ARG_STARTV      (' ' + 1)       /* Possible argument starting char */
#define ARG_ENDV        126             /* Possible argument ending char */

/* This structure represents a chain of argument names starting from
   keywords of the same letter (only identifier characters allowed). */

typedef struct  _hs     {
        struct  _hs     *next;          /* Next in chain */
        char            *chars;         /* Chars of keyword */
        int             value;          /* State value */
}  Helpargkey, *HelpargkeyRef;

typedef const   Helpargkey      *CHelpargkeyRef;

/* This structure gives an element of the vector to look up to find the
   arg value for a one-letter argument, or the start of a chain for
   multi-letter ones. */

typedef struct  {
        HelpargkeyRef   mult_chain;     /* Chain for multi-keywords */
        int             value;          /* State value */
}  Helparg, *HelpargRef;

typedef const   Helparg *CHelpargRef;

/* Array for passing defaults in - terminated by null letter */

typedef struct  {
        int             letter;         /* Letter in question */
        int             value;          /* Default value */
}  Argdefault;

struct  optv    {
        int     isplus;
        union   {
                int     letter;
                char    *string;
        }  aun;
};

extern  USHORT  Save_umask;
extern  struct  optv    optvec[];

/* This gives the maximum number of any kind of arguments */

#define MAX_ANY_ARGS            60

#define OPTION(name)    static int name(const char *arg)
#define EOPTION(name)   int name(const char *arg)
#define DEOPTION(name)  extern int name(const char *)
typedef int     (*optparam)(const char *);

#define OPTRESULT_OK            0       /* Result OK and no arg */
#define OPTRESULT_ARG_OK        1       /* Result OK - arg eaten */
#define OPTRESULT_LAST_ARG_OK   2       /* Result OK - arg eaten - last arg */
#define OPTRESULT_MISSARG       (-1)    /* Error result - missing arg */
#define OPTRESULT_ERROR         (-2)    /* Error result - code in arg_errnum */

extern  int     arg_errnum;

extern  void    doenv(char *, HelpargRef, optparam * const, const int);
extern  void    freehelpargs(HelpargRef);
extern  void    makeoptvec(const HelpargRef, const int, const int);
extern  int     spitoption(const int, const int, FILE *, const int, const int);
extern  int     proc_save_opts(const char *, const char *, void (*)(FILE *, const char *));

extern  char    **doopts(char **, HelpargRef, optparam * const, const int);
extern  char    **makevec(const char *);
extern  char    **optprocess(char **, const Argdefault *, optparam * const, const int, const int, const int);
extern  char    *make_varname();

extern  HelpargRef      helpargs(const Argdefault *, const int, const int);
