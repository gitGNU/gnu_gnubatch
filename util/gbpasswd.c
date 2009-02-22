/* gbpasswd.c -- Edit passwords for web/MS interfaces

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
#include <pwd.h>
#include <sys/types.h>
#include "incl_unix.h"
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/stat.h>

unsigned  vcount = 0, vmax = 0;
char	**vlist;

/* I don't know where to find this routine in the general case but I
   hope that this will work instead of groping around zillions of
   includes */

extern char *crypt(const char *, const char *);

#ifdef	OS_FREEBSD
/* Not found in BSDs */

char *l64a(long v)
{
	static  char  result[7];
	int	cnt;
	for  (cnt = 0;  cnt < 6;  cnt++)  {
		result[cnt] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[v & 63];
		v >>= 6;
	}
	result[6] = '\0';
	return  result;
}
#endif

void	addvec(char *s)
{
	if  (vcount >= vmax)  {
		if  (vmax == 0)
			vlist = (char **) malloc(10 * sizeof(char *));
		else
			vlist = (char **) realloc((char *) vlist, (vmax + 10) * sizeof(char *));
		if  (!vlist)  {
			fprintf(stderr, "Sorry out of memory\n");
			exit(255);
		}
		vmax += 10;
	}
	vlist[vcount] = s;
	vcount++;
}

int	main(int argc, char **argv)
{
	int	ch, force = 0, errors = 0, delpw = 0;
	unsigned  cnt;
	char	*user = (char *) 0, *passwd = (char *) 0;
	struct	passwd	*pwd;
	extern	char	*optarg;

	while  ((ch = getopt(argc, argv, "F:fu:dp:")) != EOF)
		switch  (ch)  {
		case  '?':
			fprintf(stderr, "Usage: %s [-d] [-f] [-u user] [-p passwd ] [-F file]...\n", argv[0]);
			return  1;
		case  'f':
			force++;
			continue;
		case  'F':
			addvec(optarg);
			continue;
		case  'p':
			passwd = optarg;
			continue;
		case  'u':
			user = optarg;
			continue;
		case  'd':
			delpw++;
			continue;
		}

	if  (vcount == 0)
		addvec("/usr/local/share/gbpwfile");

	if  (user)  {
		if  (!(pwd = getpwnam(user)))  {
			fprintf(stderr, "Unknown user %s\n", user);
			return  2;
		}
		if  (pwd->pw_uid != getuid()  &&  getuid() != ROOTID)  {
			fprintf(stderr, "Only root can set passwords for %s\n", user);
			return  3;
		}
	}
	else  {
		if  (!(pwd = getpwuid(getuid())))  {
			fprintf(stderr, "Unable to find current uid %d in pw file\n", getuid());
			return  4;
		}
		user = pwd->pw_name;
	}

	if  (!delpw)  {
		if  (pwd->pw_uid == ROOTID  &&  !force)  {
			fprintf(stderr, "Please specify -f option to set %s password\n", user);
			return  5;
		}
		if  (!passwd)  {
			fprintf(stderr, "Setting password for %s\n", user);
			passwd = getpass("New password: ");
		}
	}

	srand(time(0));
	umask(022);
	for  (cnt = 0;  cnt < vcount;  cnt++)  {
		char  *salt, *enc = (char *) 0;
		FILE  *pwf = fopen(vlist[cnt], "r");

		if  (!delpw)  {
			salt = l64a(rand() + 4096);
			enc = crypt(passwd, salt);
		}

		if  (pwf)  {
			int	had = 0;
			FILE  *tf = tmpfile();
			char  inbuf[120];
			while  (fgets(inbuf, sizeof(inbuf), pwf))  {
				int   lng = strlen(inbuf) - 1;
				char  *cp;
				if  (lng >= 0  &&  inbuf[lng] == '\n')
					inbuf[lng] = '\0';
				if  (!(cp = strchr(inbuf, ':')))
					continue;
				*cp = '\0';
				if  (strcmp(inbuf, user) == 0)  {
					if  (!delpw)
						fprintf(tf, "%s:%s\n", user, enc);
					had++;
				}
				else  {
					*cp = ':';
					fprintf(tf, "%s\n", inbuf);
				}
			}
			fclose(pwf);
			if  (!had)  {
				if  (delpw)  {
					fclose(tf);
					continue;
				}
				fprintf(tf, "%s:%s\n", user, enc);
			}
			if  (!(pwf = fopen(vlist[cnt], "w")))  {
				fprintf(stderr, "Cannot write %s\n", vlist[cnt]);
				errors++;
				fclose(tf);
				continue;
			}
			rewind(tf);
			while  (fgets(inbuf, sizeof(inbuf), tf))
				fputs(inbuf, pwf);

			fclose(tf);
			fclose(pwf);
		}
		else  {
			if  (delpw)
				continue;
			if  (!(pwf = fopen(vlist[cnt], "w")))  {
				fprintf(stderr, "Cannot write %s\n", vlist[cnt]);
				errors++;
				continue;
			}
			fprintf(pwf, "%s:%s\n", user, enc);
		}
	}

	return  errors > 0? 10: 0;
}
