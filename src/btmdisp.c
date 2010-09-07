/* btmdisp.c -- message dispatcher

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
#include "incl_sig.h"
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "defaults.h"
#include "errnums.h"
#include "files.h"
#include "ecodes.h"
#include "incl_unix.h"
#include "incl_ugid.h"
#include "cfile.h"
#include "notify.h"

static	char	Filename[] = __FILE__;

char	configname[] = USER_CONFIG;
char	shellname[] = DEF_CI_PATH;

char	*writer, *doswriter, *mailer;
char	*jobtitle, *exec_host;
char	*batch_uname, *homedir;
jobno_t	jobnum;

int_ugid_t	spuid, batch_uid;

int	has_sofile = 0, has_sefile = 0, pass_thru = 0;
int	repl_mail = 0, repl_write = 0, repl_doswrite = 0;
int	msg_code = $PE{Job completed msg}, exit_stat;
char	*orig_host;

extern	char	*Helpfile_path;

void  nomem(const char *fl, const int ln)
{
	fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
	exit(E_NOMEM);
}

void  pushfile(FILE *ofl, const int ifd, const int mcode)
{
	FILE	*ifl;
	int	ch;

	if  ((ifl = fdopen(ifd, "r")))  {
		if  ((ch = getc(ifl)) != EOF)  {
			fprint_error(ofl, mcode);
			do	putc(ch, ofl);
			while  ((ch = getc(ifl)) != EOF);
		}
		fclose(ifl);
	}
}

/* Invoke mail or write command.  Don't use popen since there is a bug
   in it which makes funny things happen if file descriptors 0 or
   1 are closed.  */

void  rmsg(char *cmd)
{
	int	ncode;
	FILE	*po;
	int	pfds[2];
	PIDTYPE	pid;

	if  (pipe(pfds) < 0  || (pid = fork()) < 0)
		return;

	if  (pid == 0)  {		/*  Child process  */
		char	*cp, **ap, *arglist[5];

		close(pfds[1]);	/*  Write side  */
		if  (pfds[0] != 0)  {
			close(0);
			dup(pfds[0]);
			close(pfds[0]);
		}

		ap = arglist;
		if  ((cp = strrchr(cmd, '/')))
			cp++;
		else
			cp = cmd;
		*ap++ = cp;
		if  (orig_host)
			*ap++ = orig_host;
		*ap++ = batch_uname;
		*ap = (char *) 0;

		execv(cmd, arglist);
		if  (errno == ENOEXEC)  {
			ap = arglist;
			*ap++ = cp;
			*ap++ = cmd;
			if  (orig_host)
				*ap++ = orig_host;
			*ap++ = batch_uname;
			*ap = (char *) 0;
			execv(shellname, arglist);
		}
		exit(255);
	}

	close(pfds[0]);			/*  Read side  */
	if  ((po = fdopen(pfds[1], "w")) == (FILE *) 0)  {
		kill(pid, SIGKILL);
		return;
	}

	if  (pass_thru)  {
		int  ch;
		while  ((ch = getchar()) != EOF)
			putc(ch, po);
	}
	else  {
		disp_arg[0] = jobnum;
		disp_arg[1] = exit_stat & 127;
		disp_arg[2] = (exit_stat >> 8) & 255;
		disp_str = exec_host;
		disp_str2 = jobtitle;
		ncode = msg_code;
		if  (jobtitle[0] == '\0')
			ncode -= $S{No title offset};
		if  (exec_host[0])
			ncode += $S{Host name offset};
		fprint_error(po, ncode);
		if  (has_sofile)
			pushfile(po, SOSTREAM, $E{Job stdout was});
		if  (has_sefile)
			pushfile(po, SESTREAM, $E{Job stderr was});
	}
	fclose(po);
	exit(0);
}

FILE *getmsgfile()
{
	FILE		*res;
	char		*homedf, *sysf, *repl;
	unsigned	hdlng, lng;

	/* If I don't know the user, just return the standard file.  */

	if  ((spuid = lookup_uname(batch_uname)) == UNKNOWN_UID)  {
		homedir = "/";
		return  open_icfile();
	}

	/* Get user's home directory */

	homedir = unameproc("~", "/", (uid_t) spuid);
	hdlng = strlen(homedir) + 1;
	lng = hdlng + sizeof(configname);

	/* Get hold of .xibatch file in home directory for user.  */

	if  (!(homedf = malloc(lng)))
		ABORT_NOMEM;
	strcpy(homedf, homedir);
	strcat(homedf, "/");
	strcat(homedf, configname);

	/* Whilst we're there, grab alternate programs if
	   specified. Also reset uid to the relevant user.  */

	if  ((repl = rdoptfile(homedf, "MAILER")))  {
		mailer = repl;
		repl_mail++;
	}
	if  ((repl = rdoptfile(homedf, "WRITER")))  {
		writer = repl;
		repl_write++;
	}
	if  ((repl = rdoptfile(homedf, "DOSWRITER")))  {
		doswriter = repl;
		repl_doswrite++;
	}
	if  (!(sysf = rdoptfile(homedf, "SYSMESG")))  {
		free(homedf);
		return  open_icfile();
	}

	/* If not absolute, bring it down from the home directory.  */

	if  (sysf[0] != '/')  {
		char	*abssysf;
		lng = hdlng + strlen(sysf) + 1;
		if  (!(abssysf = malloc(lng)))
			ABORT_NOMEM;
		strcpy(abssysf, homedir);
		strcat(abssysf, "/");
		strcat(abssysf, sysf);
		free(sysf);	/* Not needed any more */
		sysf = abssysf;
	}

	free(homedf);

	if  (!(res = fopen(sysf, "r")))  {
		free(sysf);
		return  open_icfile();
	}

	Helpfile_path = sysf;
	fcntl(fileno(res), F_SETFD, 1);
	return  res;
}

MAINFN_TYPE  main(int argc, char **argv)
{
	cmd_type	cmd = NOTIFY_MAIL;
	char		**ep, **lep;
	extern	char	**environ;
	static	char	lnam[] = "LOGNAME=";

	versionprint(argv, "$Revision: 1.1 $", 1);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();

	/* Initialise default message despatch */

	writer = envprocess(WRITER);
	doswriter = envprocess(DOSWRITER);
	mailer = envprocess(MAILER);

	if  ((batch_uid = lookup_uname(BATCHUNAME)) == UNKNOWN_UID)
		batch_uid = ROOTID;

	/* Now decode arguments.
	   -m	Mail style
	   -w	Write style
	   -d	Dos write
	   -o	Standard output file sent on f.d. SOSTREAM
	   -e	Standard error file sent on f.d. SESTREAM
	   -x	Pass through (no message code)
	   Follow by user id of job.
	   Follow by message code (integer) and possible host name.
	   Any other arguments are read from environment.
	   We don't check the arguments very well. */

	while  (argv[1])  {
		char	*arg = argv[1];
		if  (*arg++ != '-')
			break;
		argv++;
		while  (*arg)
			switch  (*arg++)  {
			default:
				continue;
			case  'm':case  'M':
				cmd = NOTIFY_MAIL;
				continue;
			case  'w':case  'W':
				cmd = NOTIFY_WRITE;
				continue;
			case  'd':case  'D':
				cmd = NOTIFY_DOSWRITE;
				continue;
			case  'o':case  'O':
				has_sofile = 1;
				continue;
			case  'e':case  'E':
				has_sefile = 1;
				continue;
			case  'x':case  'X':
				pass_thru = 1;
				continue;
			}
	}

	argv++;
	if  (!*argv)
		exit(E_USAGE);
	if  (!pass_thru)  {
		jobnum = atol(*argv++);
		jobtitle = *argv++;
		exec_host = *argv++;
		if  (!jobtitle  ||  !exec_host)
			exit(E_USAGE);
	}
	batch_uname = *argv++;
	if  (!pass_thru)  {
		if  (!*argv)
			exit(E_USAGE);
		exit_stat = atoi(*argv++);
		if  (!*argv)
			exit(E_USAGE);
		msg_code = atoi(*argv++);
		if  (*argv)	/* Optional */
			orig_host = *argv++;
	}

	/* Get message file.  */

	if  (!(Cfile = getmsgfile()))
		exit(E_SETUP);

	/* And now a whole load of nonsense to delete LOGNAME from the
	   environment. For some idiotic reason mail uses this
	   rather than the uid, and as a result will pick up some
	   fossil LOGNAME which happens to be lying around.  */

	for  (ep = environ;  *ep;  ep++)  {
		if  (strncmp(*ep, lnam, sizeof(lnam) - 1) == 0)  {
			for  (lep = ep + 1;  *lep;  lep++)
				;
			*ep = *--lep;
			*lep = (char *) 0;
			break;
		}
	}

	switch  (cmd)  {
	case  NOTIFY_MAIL:
		setuid((uid_t) repl_mail? spuid: batch_uid);
		rmsg(mailer);
		break;
	case  NOTIFY_WRITE:
		setuid((uid_t) repl_write? spuid: batch_uid);
		rmsg(writer);
		break;
	case  NOTIFY_DOSWRITE:
		setuid((uid_t) repl_doswrite? spuid: batch_uid);
		rmsg(doswriter);
		break;
	}

	return  E_SETUP;		/* Actually only reached if buggy*/
}
