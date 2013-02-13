/* incl_unix.h -- try to include standard UNIX library stuff

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

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef  HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef  HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef  HAVE_WAIT_H
#include <wait.h>
#endif

#define QSORTP1 (void *)
#define QSORTP4 (int (*)(const void *,const void *))

#ifdef  STDC_HEADERS
#include <string.h>
#else
#ifndef HAVE_STRCHR
#define strchr  index
#define strrchr rindex
#endif
#endif /* !STDC_HEADERS */

#ifdef  HAVE_MEMCPY
#define BLOCK_COPY(to, from, count)     memcpy((char *) (to), (char *) (from), (unsigned) (count))
#define BLOCK_ZERO(to, count)           memset((char *) (to), 0, (unsigned) (count))
#elif   defined(HAVE_BCOPY)
#define BLOCK_COPY(to, from, count)     bcopy((char *) (from), (char *) (to), (unsigned) (count))
#define BLOCK_ZERO(to, count)           bzero((char *) (to), (unsigned) (count))
#else
#define BLOCK_COPY(to, from, count)     undefined_block_copy_routine(to, from, count);
#define BLOCK_ZERO(to, count)           undefined_block_zero_routine(to, count);
#endif

/* These are our own string-ish things */

extern int  ncstrcmp(const char *, const char *);
extern int  ncstrncmp(const char *, const char *, int);

extern int  qmatch(char *, char *);

extern char *stracpy(const char *);
extern char *runpwd();

extern void  nomem(const char *, const int);
#define ABORT_NOMEM     nomem(Filename, __LINE__)
#define ABORT_NOMEMINL  nomem(__FILE__, __LINE__)

/* This is to shut compilers up about errors which don't matter */

extern  int     Ignored_error;
