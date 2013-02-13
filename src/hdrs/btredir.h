/* btredir.h -- Redirection structures

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

#define MAXREDIRPATH    79

typedef struct  {
        unsigned  char  fd;             /* Stream or -ve no more */
        unsigned  char  action;         /* Action */
#define RD_ACT_RD       1               /* Open for reading */
#define RD_ACT_WRT      2               /* Open/create for writing */
#define RD_ACT_APPEND   3               /* Open/create and append write only */
#define RD_ACT_RDWR     4               /* Open/create for read/write */
#define RD_ACT_RDWRAPP  5               /* Open/create/read/write/append */
#define RD_ACT_PIPEO    6               /* Pipe out */
#define RD_ACT_PIPEI    7               /* Pipe in */
#define RD_ACT_CLOSE    8               /* Close it (no file) */
#define RD_ACT_DUP      9               /* Duplicate file descriptor given */

        USHORT          arg;            /* Offset or file to dup from */
}  Redir, *RedirRef;

typedef const   Redir   *CRedirRef;

/* For when we are saving it up */

typedef struct  {
        unsigned  char  fd;
        unsigned  char  action;
        union   {
                USHORT  arg;            /* File to dup from */
                char    *buffer;
        }  un;
}  Mredir, *MredirRef;

typedef const   Mredir  *CMredirRef;

/* Extract redirection from offset */

#define REDIR_OF(jp, cnt)       &((RedirRef) &jp->bj_space[jp->h.bj_redirs])[cnt]

/* Only used by clients but define here */

extern  Mredir  Redirs[];
