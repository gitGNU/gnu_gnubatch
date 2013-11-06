/* getpwgrpents.c -- hand-written getpwent and getgrent routines

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


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "incl_unix.h"

static  char    Filename[] = __FILE__;

#define NPWFIELDS  7
#define NGRPFIELDS 4

#define INIT_BUFFER  200
#define INC_BUFFER 50

#define  INIT_NGROUPS   20
#define INC_NGROUPS 5

/* Buffer structure for analysing password and group files */

struct  pwbufdets  {
        FILE    *inf;
        char    *buffer;
        unsigned        clen;
        unsigned        buffmax;
        int     nflds;
        char    *fieldlist[NPWFIELDS];
};

static  struct  pwbufdets  pwbuff, grpbuff;

/* Open buffer structure if needed return 0 if failed */

static int      openbuf(struct pwbufdets *buf, const char *fname)
{
        if  (buf->inf)
                return  1;
        if  (!(buf->inf = fopen(fname, "r")))
                return  0;
        if  (!(buf->buffer = malloc(INIT_BUFFER+1)))
                ABORT_NOMEM;
        buf->buffmax = INIT_BUFFER;
        return  1;
}

/* Close buffer and deallocate */

static  void    closebuf(struct pwbufdets *buf)
{
        if  (!buf->inf)
                return;
        fclose(buf->inf);
        buf->inf = (FILE *) 0;
        free(buf->buffer);
        buf->buffer = (char *) 0;
}

/* Get the next line from the file and split it up into : separated fields.
   Return the number of fields read in. Skip to end if more than NPWFIELDS and return 0 */

static  int     fillbuf(struct pwbufdets *buf)
{
        int     ch;

        buf->nflds = 1;
        buf->fieldlist[0] = buf->buffer;
        buf->clen = 0;

        while  ((ch = getc(buf->inf)) != '\n')  {
                if  (ch == EOF)
                        return  0;

                /* Increase buffer size if needed */

                if  (buf->clen >= buf->buffmax)  {
                        int     fcnt;
                        char    *oldptr = buf->buffer;
                        buf->buffmax += INC_BUFFER;
                        buf->buffer = realloc(buf->buffer, buf->buffmax + 1);
                        if  (!buf->buffer)
                                ABORT_NOMEM;
                        for  (fcnt = 0;  fcnt < buf->nflds;  fcnt++)
                                buf->fieldlist[fcnt] = (buf->fieldlist[fcnt] - oldptr) + buf->buffer;
                }

                /* If a colon, make next field */

                if  (ch == ':')  {
                        if  (buf->nflds >= NPWFIELDS)  {
                                do  ch = getc(buf->inf);
                                while  (ch != '\n'  &&  ch != EOF);
                                return  0;
                        }
                        buf->fieldlist[buf->nflds] = &buf->buffer[buf->clen+1];
                        buf->nflds++;
                        ch = '\0';
                }
                buf->buffer[buf->clen] = ch;
                buf->clen++;
        }

        buf->buffer[buf->clen] = '\0';
        return  buf->nflds;
}

struct  passwd  *my_getpwent()
{
        static  struct  passwd  resbuf;

        if  (!openbuf(&pwbuff, "/etc/passwd"))
                return  (struct passwd *) 0;

        if  (fillbuf(&pwbuff) != NPWFIELDS)
                return  (struct passwd *) 0;

        resbuf.pw_name = pwbuff.fieldlist[0];
        resbuf.pw_uid = atol(pwbuff.fieldlist[2]);
        resbuf.pw_gid = atol(pwbuff.fieldlist[3]);
        resbuf.pw_dir = pwbuff.fieldlist[5];
        return  &resbuf;
}

void    my_endpwent()
{
        closebuf(&pwbuff);
}

struct  group  *my_getgrent()
{
        static  struct  group  resbuf;
        static  unsigned  nmembs = 0;
        char    *gmfld, *cp;
        unsigned  cfld;

        if  (!openbuf(&grpbuff, "/etc/group"))
                return  (struct group *) 0;
        if  (fillbuf(&grpbuff) != NGRPFIELDS)
                return  (struct group *) 0;
        resbuf.gr_name = grpbuff.fieldlist[0];
        resbuf.gr_gid = atol(grpbuff.fieldlist[2]);

        if  (!resbuf.gr_mem)  {
                resbuf.gr_mem = (char **) malloc((1 + INIT_NGROUPS) * sizeof(char *));
                if  (!resbuf.gr_mem)
                        ABORT_NOMEM;
                nmembs = INIT_NGROUPS;
        }
        gmfld = grpbuff.fieldlist[3];

        if  (!gmfld[0])  {      /* No members */
                resbuf.gr_mem[0] = (char *) 0;
                return  &resbuf;
        }

        /* Make the member list up */

        resbuf.gr_mem[0] = gmfld;
        cfld = 1;
        cp = gmfld;
        while  (*cp)  {
                if  (*cp == ',')  {
                        if  (cfld >= nmembs)  {
                                nmembs += INC_NGROUPS;
                                resbuf.gr_mem = (char **) realloc(resbuf.gr_mem, (nmembs+1) * sizeof(char *));
                                if  (!resbuf.gr_mem)
                                        ABORT_NOMEM;
                        }
                        resbuf.gr_mem[cfld] = cp+1;
                        cfld++;
                        *cp = '\0';
                }
                cp++;
        }
        /* Mark last one */
        resbuf.gr_mem[cfld] = (char *) 0;
        return  &resbuf;
}

void    my_endgrent()
{
        closebuf(&pwbuff);
}
