/* mmangle.c -- Mangle error message vector to incorporate standard strings.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *----------------------------------------------------------------------
 *	Mangle error message vector to incorporate standard strings.
 *	These are as follows:
 *
 *	%P - program name (from progname / argv[0])
 *	%p - process id as decimal integer
 *	%E - `errno' interpreted from sys_errlist
 *	%U - user name corresponding to effective uid
 *	%R - user name corresponding to real uid
 *	%u[0-9] - arg 0 to 9 interpreted as uid
 *	%g[0-9] - arg 0 to 9 interpreted as gid
 *	%G - group name corresponding to effective gid
 *	%H - group name corresponding to real gid
 *	%d[0-9] - insert decimal value of arg 0 to 9
 *	%x[0-9] - insert hex value of arg 0 to 9
 *	%o[0-9] - insert octal value of arg 0 to 9
 *	%c[0-9] - insert character (or \xnn) value of arg 0 to 9
 *	%N[0-9] - host name
 *	%s - insert value of disp_str
 *	%t - insert value of disp_str2
 *	%f - interpret and insert float value
 *	%F - file name of help file
 */

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "incl_net.h"
#include "defaults.h"
#include "network.h"
#include "incl_ugid.h"

static	char	Filename[] = __FILE__;

extern	int	save_errno;
const	char	*progname,
		*disp_str,
		*disp_str2;
char		*Helpfile_path;
LONG	disp_arg[10];
double	disp_float;

static char *concat(char *first, const char *insert, char *rmb, char *rme)
{
	int	firstbit = rmb - first;
	int	inlng = strlen(insert);
	char	*result;

	if  ((result = malloc((unsigned) (firstbit + inlng + strlen(rme) + 1))) == (char *) 0)
		ABORT_NOMEM;
	strncpy(result, first, (unsigned) firstbit);
	strncpy(result+firstbit, insert, (unsigned) inlng);
	strcpy(result+firstbit+inlng, rme);
	return  result;
}

char **mmangle(char **mvec)
{
	char	**mp;
	int	n;
	char	*line, *cp, *newline;
	const	char	*fmt;
	char	numb[30];

	for  (mp = mvec;  *mp;  mp++)  {
	restart:
		for  (line = *mp;  (cp = strchr(line, '%'));  line = cp + 1)  {
			switch  (cp[1])  {
			default:
				continue;

			case  '%':
				newline = concat(*mp, "%", cp, cp + 2);
				n = cp - *mp;
				free(*mp);
				*mp = newline;
				cp = newline + n;
				continue;

			case  'E':
				n = save_errno;
				newline = concat(*mp, strerror(n), cp, cp + 2);
				break;

			case  'P':
				newline = concat(*mp, progname, cp, cp + 2);
				break;

			case  'F':
				newline = concat(*mp, Helpfile_path, cp, cp + 2);
				break;

			case  'U':
				newline = concat(*mp, prin_uname(geteuid()), cp, cp + 2);
				break;

			case  'R':
				newline = concat(*mp, prin_uname(getuid()), cp, cp + 2);
				break;

			case  'G':
				newline = concat(*mp, prin_gname(getegid()), cp, cp + 2);
				break;

			case  'H':
				newline = concat(*mp, prin_gname(getgid()), cp, cp + 2);
				break;

			case  'p':
				sprintf(numb, "%ld", (long) getpid());
				newline = concat(*mp, numb, cp, cp + 2);
				break;

			case  's':
				if  (!(fmt = disp_str))
					disp_str = "<null>";
				newline = concat(*mp, disp_str, cp, cp+2);
				break;

			case  't':
				if  (!(fmt = disp_str2))
					disp_str2 = "<null>";
				newline = concat(*mp, disp_str2, cp, cp+2);
				break;

			case  'N':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				newline = concat(*mp, look_host((netid_t) disp_arg[n]), cp, cp+3);
				break;

			case  'g':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				newline = concat(*mp, prin_gname((gid_t) disp_arg[n]), cp, cp+3);
				break;

			case  'u':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				newline = concat(*mp, prin_uname((uid_t) disp_arg[n]), cp, cp+3);
				break;

			case  'D':
			{
				struct	tm	*tp;
				int	day, mon;
				time_t	w;
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				w = disp_arg[n];
				tp = localtime(&w);
				day = tp->tm_mday;
				mon = tp->tm_mon+1;
#ifdef	HAVE_TM_ZONE
				if  (tp->tm_gmtoff <= -4 * 60 * 60)
#else
				if  (timezone >= 4 * 60 * 60)
#endif
				{ /* Dyslexic pirates at you-know-where */
					day = mon;
					mon = tp->tm_mday;
				}
				sprintf(numb, "%.2d/%.2d/%.4d", day, mon, tp->tm_year + 1900);
				newline = concat(*mp, numb, cp, cp+3);
				break;
			}

			case  'T':
			{
				struct	tm	*tp;
				time_t	w;
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				w = disp_arg[n];
				tp = localtime(&w);
				sprintf(numb, "%.2d:%.2d:%.2d", tp->tm_hour, tp->tm_min, tp->tm_sec);
				newline = concat(*mp, numb, cp, cp+3);
				break;
			}


			case  'x':
				fmt = "%x";
				goto  fcont;
			case  'o':
				fmt = "%o";
				goto  fcont;
			case  'd':
				fmt = "%d";
			fcont:
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
			fcont2:
				sprintf(numb, fmt, disp_arg[n]);
				newline = concat(*mp, numb, cp, cp + 3);
				break;
			case  'c':
				n = cp[2] - '0';
				if  (n < 0 || n > 9)
					continue;
				if  (disp_arg[n] < ' ' || disp_arg[n] > '~')
					fmt = "\\x%.2lx";
				else
					fmt = "%lc";
				goto  fcont2;
			case  'f':
				sprintf(numb, "%.2f", disp_float);
				newline = concat(*mp, numb, cp, cp + 2);
				break;
			}
			free(*mp);
			*mp = newline;
			goto  restart;
		}
	}
	return  mvec;
}
