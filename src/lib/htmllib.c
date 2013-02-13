/* htmllib.c -- HTML library routines

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

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef  HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "defaults.h"
#include "files.h"
#include "errnums.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "xihtmllib.h"

static  char    Filename[] = __FILE__;

#define FILE_NAME_SIZE  255

FILE    *Htmlini;

static  char    *inidir;

LONG    sect_start = 0L,        /* Offset of section start just after [name] bit */
        sect_end = 0L,          /* Offset of end of section */
        dflt_end = 0L;

extern  const   char    *progname;

void  html_error(const char *msg)
{
        printf("Content-type: text/html\n\n<html>\n<head>\n<title>System error from %s</title>\n</head>\n", progname);
        printf("<body>\n<H1 align=center>Error from %s</H1>\n", progname);
        printf("%s\n<p>Please press BACK on your browser.\n</body></html>\n", msg);
}

int  html_getline(char *buf)
{
        int     ccnt = 0, ch;

        while  ((ch = getc(Htmlini)) != EOF)  {
                if  (ccnt <= 0)  {

                        /* Skip over blank lines and leading spaces at start of line */

                        if  (isspace(ch))
                                continue;

                        /* Handle comments with # or ; at start of line */

                        if  (ch == '#' || ch == ';')  {
                                do  ch = getc(Htmlini);
                                while  (ch != '\n' && ch != EOF);
                                continue;
                        }
                }

                /* Note end of line */

                if  (ch == '\n')  {
                        /* Trim trailing spaces */
                        while  (ccnt > 0  &&  isspace(buf[ccnt-1]))
                                ccnt--;
                        buf[ccnt] = '\0';
                        return  ccnt;
                }

                /* Otherwise accumulate characters */

                if  (ccnt < HINILWIDTH-1)
                        buf[ccnt++] = ch;
        }
        return  0;
}

void  html_openini()
{
        char    *fn = envprocess(XIHTMLINI), *cp;
        char    inbuf[HINILWIDTH];
        int     lng, pnl;

        if  (!(Htmlini = fopen(fn, "r")))  {
                html_error("Could not open html ini file\n");
                exit(E_SETUP);
        }
        if  ((cp = strrchr(fn, '/')))  {
                *++cp = '\0';
                inidir = stracpy(fn); /* Including the / */
        }
        else
                inidir = "/";
        free(fn);
        fcntl(fileno(Htmlini), F_SETFD, 1);

        /* Now scan fild for start and end of section and default
           first find end of default section */

        do  {
                dflt_end = ftell(Htmlini); /* Mark where we got to as end of defaults */

                if  ((lng = html_getline(inbuf)) <= 0)
                        return;

        }  while (inbuf[0] != '['  ||  inbuf[lng-1] != ']');

        /* We got the start of a section, carry on until we get ours */

        pnl = strlen(progname);

        while  (ncstrncmp(&inbuf[1], progname, pnl) != 0  ||  inbuf[pnl+1] != ']')  {
                do  if  ((lng = html_getline(inbuf)) <= 0)
                        return;
                while    (inbuf[0] != '['  ||  inbuf[lng-1] != ']');
        }

        sect_start = ftell(Htmlini);
        do  {
                sect_end = ftell(Htmlini); /* Mark where we got to as end of defaults */

                if  ((lng = html_getline(inbuf)) <= 0)
                        return;

        }  while (inbuf[0] != '['  ||  inbuf[lng-1] != ']');
}

void  html_closeini()
{
        if  (Htmlini)  {
                fclose(Htmlini);
                Htmlini = (FILE *) 0;
        }
}

static int  parfind(const off_t epos, char *rbuf, const char *parname)
{
        int     pnl = strlen(parname);
        char    inbuf[HINILWIDTH];

        do  {
                if  (html_getline(inbuf) <= 0)
                        return  0;

                if  (ncstrncmp(inbuf, parname, pnl) == 0)  {
                        char    *ep = &inbuf[pnl];
                        while  (isspace(*ep))
                                ep++;
                        if  (*ep == '=')  {
                                do  ep++;
                                while  (isspace(*ep));
                                while  (*ep)
                                        *rbuf++ = *ep++;
                                *rbuf = '\0';
                                return  1;
                        }
                }

        }  while  (ftell(Htmlini) < epos);

        return  0;
}

int  html_iniparam(const char *parname, char *buf)
{
        LONG    cpos = ftell(Htmlini);

        /* If the last op left us somewhere in the middle of a section, try to go
           on from there, saves reading through the file all the time */

        if  (cpos >= sect_start && cpos < sect_end  &&  parfind(sect_end, buf, parname))
                return  1;

        /* Rewind to start of section and try again */

        if  (sect_start > 0  &&  sect_start < sect_end)  {
                fseek(Htmlini, sect_start, 0);
                if  (parfind(sect_end, buf, parname))
                     return  1;
        }

        /* Try the default section */

        if  (dflt_end > 0)  {
                fseek(Htmlini, 0L, 0);
                if  (parfind(dflt_end, buf, parname))
                        return  1;
        }

        return  0;
}

int  html_inibool(const char *parname, const int deflt)
{
        char    inbuf[HINILWIDTH];

        if  (!html_iniparam(parname, inbuf))
                return  deflt;

        switch  (toupper(inbuf[0]))  {
        default:
                return  deflt;
        case  'Y':case  'T':
                return  1;
        case  'N':case  'F':
                return  0;
        }
}

long  html_iniint(const char *parname, const int deflt)
{
        long    result = 0, r2 = 0, r3 = 0;
        char    *cp;
        char    inbuf[HINILWIDTH];

        if  (!html_iniparam(parname, inbuf)  ||  !isdigit(inbuf[0]))
                return  deflt * 24L * 3600L;

        cp = inbuf;
        while  (isdigit(*cp))
                result = result*10 + *cp++ - '0';

        /* Old style, take as days */

        if  (*cp++ != ':' || !isdigit(*cp))
                return  result * 24L * 3600L;

        while  (isdigit(*cp))
                r2 = r2*10 + *cp++ - '0';

        /* Only one colon, take as hh:mm */

        if  (*cp++ != ':' || !isdigit(*cp))
                return  (result * 60 + r2) * 60;

        /* DD:HH:MM */

        while  (isdigit(*cp))
                r3 = r3*10 + *cp++ - '0';

        return  ((result * 24 + r2) * 60 + r3) * 60;
}

char *html_inistr(const char *parname, char *deflt)
{
        char    inbuf[HINILWIDTH];

        if  (!html_iniparam(parname, inbuf))
                return  deflt? stracpy(deflt): deflt;

        if  (inbuf[0] == '\"'  &&  inbuf[strlen(inbuf)-1] == '\"')  {
                int  cnt, lng = strlen(inbuf)-1;
                char    stringres[HINILWIDTH];
                char    *rp = stringres;
                for  (cnt = 1;  cnt < lng;  cnt++)  {
                        if  (inbuf[cnt] == '\"')
                                cnt++;
                        *rp++ = inbuf[cnt];
                }
                *rp = '\0';
                return  stracpy(stringres);
        }
        else
                return  stracpy(inbuf);
}

char *html_inifile(const char *parname, char *deflt)
{
        char    *res1 = html_inistr(parname, deflt);
        char    *nres;

        if  (!res1)
                return  res1;

        /* We always get a "malloc"ed version for consistency. */

        if  (*res1 == '~'  ||  strchr(res1, '$'))  {

                if  (*res1 == '~')  {
                        char    *res2 = unameproc(res1, "/", Realuid);
                        free(res1);
                        res1 = res2;
                }

                if  (strchr(res1, '$'))  {
                        int     count_recurse = RECURSE_MAX;
                        do  {
                                char  *res2 = envprocess(res1);
                                free(res1);
                                res1 = res2;
                        }  while  (strchr(res1, '$')  &&  --count_recurse > 0);
                }
        }

        /* Make an absolute path if needed by prepending ini dir */

        if  (*res1 == '/')
                return  res1;

        if  (!(nres = malloc((unsigned) (strlen(res1) + strlen(inidir) + 1))))
                ABORT_HTML_NOMEM;

        sprintf(nres, "%s%s", inidir, res1);
        free(res1);
        return  nres;
}

/* Common case of wanting cookie expiry time. */

int  html_cookexpiry()
{
        return  (html_iniint("cookexpiry", DEFLT_COOKEXP_DAYS) + 1800*24L) / (3600*24L);
}

int  html_output_file(const char *name, const int cont_type)
{
        char    *fname = html_inifile(name, (char *) 0);
        FILE    *fp;
        int     ch;

        if  (!fname)
                return  0;
        fp = fopen(fname, "r");
        free(fname);
        if  (!fp)
                return  0;
        if  (cont_type)
                fputs("Content-type: text/html\n\n", stdout);
        while  ((ch = getc(fp)) != EOF)
                putchar(ch);
        fclose(fp);
        return  1;
}

void  html_out_or_err(const char *name, const int cont_type)
{
        if  (!(html_output_file(name, cont_type)))  {
                char    outbuf[HINILWIDTH];
                sprintf(outbuf, "Missing output file on server: %s", name);
                html_error(outbuf);
                exit(E_SETUP);
        }
}

/* Same but with two optional parameters */

int  html_out_param_file(const char *name, const int cont_type, const ULONG par1, const ULONG par2)
{
        char    *fname = html_inifile(name, (char *) 0);
        FILE    *fp;
        int     ch;

        if  (!fname)
                return  0;
        fp = fopen(fname, "r");
        free(fname);
        if  (!fp)
                return  0;
        if  (cont_type)
                fputs("Content-type: text/html\n\n", stdout);
        while  ((ch = getc(fp)) != EOF)  {
                if  (ch == '$')  {
                        if  ((ch = getc(fp)) == EOF)
                                break;
                        if  (ch == 'Y')  {
                                printf("%lu", (unsigned long) par1);
                                continue;
                        }
                        else  if  (ch == 'Z')  {
                                printf("%lu", (unsigned long) par2);
                                continue;
                        }
                        else
                                putchar('$');
                }
                putchar(ch);
        }
        fclose(fp);
        return  1;
}

int  html_out_cparam_file(const char *name, const int cont_type, const char *par1)
{
        char    *fname = html_inifile(name, (char *) 0);
        FILE    *fp;
        int     ch;

        if  (!fname)
                return  0;
        fp = fopen(fname, "r");
        free(fname);
        if  (!fp)
                return  0;
        if  (cont_type)
                fputs("Content-type: text/html\n\n", stdout);
        while  ((ch = getc(fp)) != EOF)  {
                if  (ch == '$')  {
                        if  ((ch = getc(fp)) == EOF)
                                break;
                        if  (ch == 'Y')  {
                                fputs(par1, stdout);
                                continue;
                        }
                        else
                                putchar('$');
                }
                putchar(ch);
        }
        fclose(fp);
        return  1;
}

void  html_nomem(const char *fl, const int ln)
{
        char    msgbuf[100];
        sprintf(msgbuf, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
        html_error(msgbuf);
        exit(E_NOMEM);
}

void  html_disperror(const int errnum)
{
        char    **emess = helpvec(errnum, 'E'), **ep;

        html_output_file("error_preamble", 1);
        for  (ep = emess;  *ep;  ep++)  {
                printf("%s\n", *ep);
                free(*ep);
        }
        free((char *) emess);
        html_output_file("error_postamble", 0);
}

void  html_fldprint(const char *fld)
{
        while  (*fld)  {
                char    *msg;
                switch  (*fld)  {
                default:
                        putchar(*fld);
                        fld++;
                        continue;
                case  ' ':
                        msg = "nbsp";
                        break;
                case  '<':
                        msg = "lt";
                        break;
                case  '>':
                        msg = "gt";
                        break;
                case  '&':
                        msg = "amp";
                        break;
                case  '\"':
                        msg = "quot";
                        break;
                }
                printf("&%s;", msg);
                fld++;
        }
        putchar('\n');
}

void  html_pre_putchar(const int ch)
{
        const  char  *msg;

        switch  (ch)  {
        default:
                if  (!isprint(ch))
                        return;
        case  '\t':
        case  '\n':
        case  ' ':
                putchar(ch);
                return;
        case  '<':
                msg = "lt";
                break;
        case  '>':
                msg = "gt";
                break;
        case  '&':
                msg = "amp";
                break;
        }
        printf("&%s;", msg);
}

int  html_getpostline(char *buf)
{
        int     ccnt = 0, ch;

        while  ((ch = getchar()) != '\n'  &&  ch != EOF)  {

                /* Also treat '&' as end of line. "Real" & chars are
                   escaped. */

                if  (ch == '&')  {
                        buf[ccnt] = '\0';
                        return  ccnt;
                }

                /* Otherwise accumulate characters */

                if  (ccnt < HINILWIDTH-1)
                        buf[ccnt++] = ch;
        }

        /* We might get EOF in the middle of a line */

        if  (ccnt > 0)  {
                /* Trim trailing spaces */
                while  (ccnt > 0  &&  isspace(buf[ccnt-1]))
                        ccnt--;
                buf[ccnt] = '\0';
                return  ccnt;
        }
        return  0;
}

void  html_convert(const char *inbuf, char *outbuf)
{
        int     ch, chleft = HINILWIDTH;

        while  (*inbuf)  {
                switch  (*inbuf)  {
                case  '\\':
                        inbuf++;
                        if  (!inbuf)
                                continue;
                default:
                        *outbuf = *inbuf++;
                        break;
                case  '+':
                        *outbuf = ' ';
                        inbuf++;
                        break;
                case  '%':
                        switch  (*++inbuf)  {
                        default:
                                continue;
                        case '0':case '1':case '2':case '3':case '4':
                        case '5':case '6':case '7':case '8':case '9':
                                ch = *inbuf++ - '0';
                                break;
                        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                                ch = *inbuf++ - 'a' + 10;
                                break;
                        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                                ch = *inbuf++ - 'A' + 10;
                                break;
                        }
                        ch <<= 4;
                        switch  (*inbuf)  {
                        default:
                                continue;
                        case '0':case '1':case '2':case '3':case '4':
                        case '5':case '6':case '7':case '8':case '9':
                                ch |= *inbuf++ - '0';
                                break;
                        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':
                                ch |= *inbuf++ - 'a' + 10;
                                break;
                        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':
                                ch |= *inbuf++ - 'A' + 10;
                                break;
                        }
                        if  (ch == '\r')
                                continue;
                        *outbuf = ch;
                        break;
                }

                /* Defend against dodgy arguments... */

                if  (--chleft > 0)
                        outbuf++;
        }
        *outbuf = '\0';
}

char **html_getvalues(const char *arg)
{
        const  char  *ap, *np;
        char    **result, **rp;
        unsigned  cnta = 2;     /* We must have at least one plus the null on end */
        unsigned  lng;
        char    outbuf[HINILWIDTH];

        for  (ap = arg;  (np = strchr(ap, '&')) || (np = strchr(ap, ';'));  ap = np+1)
                cnta++;

        if  (!(result = (char **) malloc(sizeof(char *) * cnta)))
                ABORT_HTML_NOMEM;

        rp = result;

        for  (ap = arg;  (np = strchr(ap, '&')) || (np = strchr(ap, ';'));  ap = np+1)  {
                char    inbuf[HINILWIDTH];

                lng = np - ap;
                if  (lng >= HINILWIDTH)
                        lng = HINILWIDTH-1;
                strncpy(inbuf, ap, lng);
                inbuf[lng] = '\0';
                html_convert(inbuf, outbuf);
                *rp++ = stracpy(outbuf);
        }

        html_convert(ap, outbuf);
        *rp++ = stracpy(outbuf);
        *rp = (char *) 0;
        return  result;
}

/* Roll our own temporary files as there are such a plethora of mktemp functions etc. */

static char *gen_tempfile()
{
        char    *tname = html_inifile(HTML_TMPNAME, HTML_TMPFILE), *newname;
        static  int     tmpfcnt = 0;

        if  (!(newname = malloc((unsigned) (strlen(tname) + 50))))
                ABORT_HTML_NOMEM;
        sprintf(newname, tname, (long) getpid(), ++tmpfcnt);
        free(tname);
        tname = stracpy(newname);
        free(newname);
        return  tname;
}

static int  html_getmpline(char *buf, const int nocr)
{
        int     result = 0, ch;

        while  ((ch = getchar()) != EOF)  {
                if  (ch == '\r'  &&  nocr)
                        continue;
                buf[result] = ch;
                if  (++result >= HINILWIDTH-1)
                        break;
                if  (ch == '\n')
                        break;
        }
        buf[result] = '\0';
        return  result;
}

/* Extract name possibly surrounded by ' or "s from line of multipart stuff.
   Return next char. str is initially char BEFORE the construct */

static char *mp_name(char *str, char *nbuf)
{
        int  lng = 0;

        if  (*++str == '\'' || *str == '\"')  {
                int     quot = *str++;
                while  (*str  &&  *str != quot)  {
                        if  (lng < FILE_NAME_SIZE)
                                nbuf[lng++] = *str;
                        str++;
                }
                if  (*str)
                        str++;  /* Past quote */
        }
        else  {
                while  (*str && !isspace(*str))  {
                        if  (lng < FILE_NAME_SIZE)
                                nbuf[lng++] = *str;
                        str++;
                }
        }
        nbuf[lng] = '\0';
        return  str;
}

static struct posttab *find_posttab(const char *nam, struct posttab *pt)
{
        struct posttab *result;

        for  (result = pt;  result->postname;  result++)
                if  (ncstrcmp(nam, result->postname) == 0)
                        return  result;
        return  (struct posttab *) 0;
}

static int  is_boundary(char *buff, char *boundary, const int buffb, const int boundb)
{
        if  (buffb < boundb + 3)        /* Two extra minuses and \n */
                return  0;
        if  (buff[0] != '-'  ||  buff[1] != '-'  ||  strncmp(&buff[2], boundary, boundb) != 0)
                return  0;
        return  1;
}

static int  find_startbound(char *boundary)
{
        int     szb = strlen(boundary);

        for  (;;)  {
                int     nbytes;
                char    inbuf[HINILWIDTH];

                if  ((nbytes = html_getmpline(inbuf, 1)) <= 0)
                        return  0;
                if  (!is_boundary(inbuf, boundary, nbytes, szb))
                        continue;
                if  (nbytes < szb + 5  || inbuf[szb+2] != '-' || inbuf[szb+3] != '-')
                        return  1;
                return  0;
        }
}

#define RES_INIT        50
#define RES_INC         30

static int  html_getmpencsect(struct posttab *pt, char *boundary)
{
        int     szb = strlen(boundary), nbytes;
        char    *cp;
        struct  posttab *wht;
        char    inbuf[HINILWIDTH], namebuf[FILE_NAME_SIZE+1];
        static  char    ct[] = "content";

        /* Found start of section, get description, expecting something like

           Content-Disposition: form-data; name="something" or
           Content-Disposition: form-data; name="filename"; filename="gggg"

           possibly followed by

           Content-Type: text/plain

           but in either case by a blank line before the start of the data.

           If the latter, we create a temporary file name to put it all in */

        if  ((nbytes = html_getmpline(inbuf, 1)) <= 0)
                return  0;

        if  (ncstrncmp(inbuf, ct, sizeof(ct)-1) != 0) /* Confused */
                return  0;

        /* Look for the "=" of "name=" */
        for  (cp = inbuf + sizeof(ct);  *cp && *cp != '=';  cp++)
                ;
        if  (!*cp)
                return  0;

        cp = mp_name(cp, namebuf);

        /* If it doesn't interest us, find next boundary and return if end */

        if  (!(wht = find_posttab(namebuf, pt)))  {
                while  ((nbytes = html_getmpline(inbuf, 1)) > 0)  {
                        if  (!is_boundary(inbuf, boundary, nbytes, szb))
                                continue;
                        if  (inbuf[szb+2] == '-' && inbuf[szb+3] == '-')
                                return  0;
                        return  1;      /* Something follows */
                }
                return  0;
        }

        /* See if we've got a file name. */

        while  (*cp  &&  isspace(*cp))
                cp++;

        if  (*cp == ';')  {

                char    *tempfname = gen_tempfile();
                FILE    *outf;
                int     hadnl = 0;

                /* Assume it's a file name */

                do  cp++;
                while  (*cp  &&  *cp != '=');

                mp_name(cp, namebuf);

                /* The name of the file will be the argument.
                   We plant the name of the temporary file in
                   the "->postfile" if it exists, otherwise we
                   just delete it. */

                if  (!(outf = fopen(tempfname, "w")))  {
                        free(tempfname);
                        return  0;
                }

                /* Read next line and skip over a possible Content-type line.
                   Read until we have read a blank line (1 char) */

                do  {
                        if  ((nbytes = html_getmpline(inbuf, 1)) <= 0)
                                return  0;
                }  while  (nbytes > 1);

                /* Now read the file which is terminated by a \n and
                   then the boundary. (The file might not have a \n at the end). */

                while  ((nbytes = html_getmpline(inbuf, 0)) > 0)  {
                        if  (hadnl)  {
                                if  (is_boundary(inbuf, boundary, nbytes, szb))
                                        break;
                                putc('\n', outf);
                                hadnl = 0;
                        }
                        if  (inbuf[nbytes-1] == '\n')  {
                                nbytes--;
                                hadnl = 1;
                        }
                        if  (nbytes > 0)
                                fwrite(inbuf, sizeof(char), nbytes, outf);
                }
                fclose(outf);

                /* If the application uses this, it should delete
                   it or whatever */

                if  (wht->postfile)
                        *wht->postfile = tempfname;
                else  {
                        unlink(tempfname);
                        free(tempfname);
                }
                (wht->post_fn)(namebuf);
        }
        else  {
                char    *result;
                int     hadnl = 0;
                unsigned  rsize, rmax;

                /* Otherwise we have an ordinary sort of thing.

                   Read next line and skip over a possible Content-type line.
                   Read until we have read a blank line (1 char) */

                do  {
                        if  ((nbytes = html_getmpline(inbuf, 1)) <= 0)
                                return  0;
                }  while  (nbytes > 1);

                if  (!(result = malloc(RES_INIT+1)))
                        ABORT_HTML_NOMEM;

                result[0] = '\0';
                rsize = 0;
                rmax = RES_INIT;

                while  ((nbytes = html_getmpline(inbuf, 1)) > 0)  {
                        if  (hadnl)  {
                                if  (is_boundary(inbuf, boundary, nbytes, szb))
                                        break;
                                if  (rsize >= rmax)  {
                                        rmax += RES_INC;
                                        if  (!(result = realloc(result, rmax+1)))
                                                ABORT_HTML_NOMEM;
                                }
                                result[rsize++] = '\n';
                                hadnl = 0;
                        }
                        if  (inbuf[nbytes-1] == '\n')  {
                                nbytes--;
                                hadnl = 1;
                        }
                        if  (nbytes > 0)  {
                                if  (rsize + nbytes >= rmax)  {
                                        rmax += ((nbytes + RES_INC - 1) / RES_INC) * RES_INC;
                                        if  (!(result = realloc(result, rmax+1)))
                                                ABORT_HTML_NOMEM;
                                }
                                BLOCK_COPY(&result[rsize], inbuf, nbytes);
                                rsize += nbytes;
                        }
                }
                result[rsize] = '\0';
                (wht->post_fn)(result);
                free(result);
        }
        if  (nbytes <= 0)
                return  0;
        if  (nbytes < szb + 5  || inbuf[szb+2] != '-' || inbuf[szb+3] != '-')
                return  1;
        return  0;
}

void  html_postvalues(struct posttab *pt)
{
        char    *ep;
        struct  posttab *pp;
        static  char    mup[] = "multipart/";
        static  char    bnd[] = "boundary=";

        if  (!(ep = getenv("REQUEST_METHOD"))  || ncstrcmp(ep, "post") != 0)
                fprintf(stderr, "Offline - please enter \"post\" data\n");

        /* Handle multi-part encoding separately */

        if  ((ep = getenv("CONTENT_TYPE"))  &&  ncstrncmp(ep, mup, sizeof(mup)-1) == 0)  {
                char    *boundary = ep + sizeof(mup);

                /* Find "boundary=" marker and advance past it.  */
                do  {
                        boundary++;
                        if  (!*boundary)        /* Huh??? */
                                return;
                }  while  (tolower(*boundary) != 'b' || ncstrncmp(boundary, bnd, sizeof(bnd)-1) != 0);
                boundary += sizeof(bnd)-1;
                if    (find_startbound(boundary))
                        while  (html_getmpencsect(pt, boundary))
                                ;
        }
        else  {
                char    inbuf[HINILWIDTH];

                while  (html_getpostline(inbuf))  {
                        if  (!(ep = strchr(inbuf, '=')))
                                continue;
                        *ep = '\0';
                        if  ((pp = find_posttab(inbuf, pt)))  {
                                char    argbuf[HINILWIDTH];
                                html_convert(ep+1, argbuf);
                                (pp->post_fn)(argbuf);
                        }
                }
        }
}
