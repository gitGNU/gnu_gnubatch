/* getbtuser.c -- get user permission structure

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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "incl_sig.h"
#include "files.h"
#include "btmode.h"
#include "btuser.h"
#include "ecodes.h"
#include "errnums.h"

static	char	Filename[] = __FILE__;

#define	INITU	70
#define	INCU	10

extern	void	uloop_over(int, void (*)(int, char *, int_ugid_t), char *);
extern	int  	isvuser(const uid_t);

Btdef	Btuhdr;
static	int	btuf_fid = -1;
int	btu_needs_rebuild;
ULONG	Fileprivs;

static	unsigned  char	igsigs[]= { SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGALRM, SIGUSR1, SIGUSR2 };

#ifdef	UNSAFE_SIGNALS
static	RETSIGTYPE	(*oldsigs[sizeof(igsigs)])(int);
#endif

static void  savesigs(const int saving)
{
	int	cnt;
#ifdef	HAVE_SIGACTION
	sigset_t	nset;
	sigemptyset(&nset);
	for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
		sigaddset(&nset, igsigs[cnt]);
	sigprocmask(saving? SIG_BLOCK: SIG_UNBLOCK, &nset, (sigset_t *) 0);
#elif	defined(STRUCT_SIG)
	int	msk = 0;
	for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
		msk |= sigmask(igsigs[cnt]);
	if  (saving)
		sigsetmask(sigsetmask(~0) | msk);
	else
		sigsetmask(sigsetmask(~0) & ~msk);
#elif	defined(HAVE_SIGSET)
	if  (saving)
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			sighold(igsigs[cnt]);
	else
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			sigrelse(igsigs[cnt]);
#else
	if  (saving)
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			oldsigs[cnt] = signal((int) igsigs[cnt], SIG_IGN);
	else
		for  (cnt = 0;  cnt < sizeof(igsigs);  cnt++)
			signal((int) igsigs[cnt], oldsigs[cnt]);
#endif
}

static void  iu(int fid, char *arg, int_ugid_t uid)
{
	((BtuserRef) arg) ->btu_user = uid;
	insertu(fid, (BtuserRef) arg);
}

/* Create user control file from scratch.  Return 0 - failure, 1 - ok */

static int  init_file(char *fname)
{
	int		fid;
	int_ugid_t	uu;
	Btuser		Spec;
	struct	stat	pwbuf;

	if  ((fid = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)
		return  0;

	if  ((uu = lookup_uname(BATCHUNAME)) != UNKNOWN_UID)
#if	defined(HAVE_FCHOWN) && !defined(M88000)
		fchown(fid, (uid_t) uu, getegid());
#else
		chown(fname, (uid_t) uu, getegid());
#endif
	savesigs(1);

	stat("/etc/passwd", &pwbuf);
	Btuhdr.btd_version = GNU_BATCH_MAJOR_VERSION;
	Btuhdr.btd_lastp = pwbuf.st_mtime;
	Btuhdr.btd_minp = U_DF_MINP;
	Btuhdr.btd_maxp = U_DF_MAXP;
	Btuhdr.btd_defp = U_DF_DEFP;
	Btuhdr.btd_maxll = U_DF_MAXLL;
	Btuhdr.btd_totll = U_DF_TOTLL;
	Btuhdr.btd_spec_ll = U_DF_SPECLL;
	Btuhdr.btd_priv = U_DF_PRIV;
	Btuhdr.btd_jflags[0] = U_DF_UJ;
	Btuhdr.btd_jflags[1] = U_DF_GJ;
	Btuhdr.btd_jflags[2] = U_DF_OJ;
	Btuhdr.btd_vflags[0] = U_DF_UV;
	Btuhdr.btd_vflags[1] = U_DF_GV;
	Btuhdr.btd_vflags[2] = U_DF_OV;
	write(fid, (char *) &Btuhdr, sizeof(Btuhdr));

	Spec.btu_isvalid = 1;
	Spec.btu_minp = U_DF_MINP;
	Spec.btu_maxp = U_DF_MAXP;
	Spec.btu_defp = U_DF_DEFP;
	Spec.btu_maxll = U_DF_MAXLL;
	Spec.btu_totll = U_DF_TOTLL;
	Spec.btu_spec_ll = U_DF_SPECLL;
	Spec.btu_priv = U_DF_PRIV;
	Spec.btu_jflags[0] = U_DF_UJ;
	Spec.btu_jflags[1] = U_DF_GJ;
	Spec.btu_jflags[2] = U_DF_OJ;
	Spec.btu_vflags[0] = U_DF_UV;
	Spec.btu_vflags[1] = U_DF_GV;
	Spec.btu_vflags[2] = U_DF_OV;

	uloop_over(fid, iu, (char *) &Spec);

	/* Set "root" and "batch" to be super-people.  */

	Spec.btu_priv = ALLPRIVS;
	Spec.btu_user = ROOTID;
	insertu(fid, &Spec);

	if  (uu > 0)  {
		gid_t	lastgid = getegid();
		Spec.btu_user = uu;
		insertu(fid, &Spec);
#if	defined(HAVE_FCHOWN) && !defined(M88000)
		fchown(fid, uu, lastgid);
#else
		chown(fname, uu, lastgid);
#endif
	}
	close(fid);
	savesigs(0);
	return  1;
}

/* Lock the whole caboodle */

static void  lockit(const int fid, const int type)
{
	struct	flock	lk;

	lk.l_type = (SHORT) type;
	lk.l_whence = 0;
	lk.l_start = 0;
	lk.l_len = 0;
	lk.l_pid = 0;
	if  (fcntl(fid, F_SETLKW, &lk) < 0)  {
		print_error($E{Cannot lock user ctrl file});
		exit(E_SETUP);
	}
}

/* Unlock the whole caboodle */

static void  unlockit(const int fid)
{
	struct	flock	lk;

	lk.l_type = F_UNLCK;
	lk.l_whence = 0;
	lk.l_start = 0;
	lk.l_len = 0;
	lk.l_pid = 0;
	if  (fcntl(fid, F_SETLKW, &lk) < 0)  {
		print_error($E{Cannot unlock user ctrl file});
		exit(E_SETUP);
	}
}

/* Didn't find user, or he wasn't valid, so make new thing.  */

static void  init_defaults(BtuserRef res, int_ugid_t uid)
{
	res->btu_isvalid = 1;
	res->btu_user = uid;
	res->btu_minp = Btuhdr.btd_minp;
	res->btu_maxp = Btuhdr.btd_maxp;
	res->btu_defp = Btuhdr.btd_defp;
	res->btu_maxll = Btuhdr.btd_maxll;
	res->btu_totll = Btuhdr.btd_totll;
	res->btu_spec_ll = Btuhdr.btd_spec_ll;
	res->btu_priv = Btuhdr.btd_priv;
	res->btu_jflags[0] = Btuhdr.btd_jflags[0];
	res->btu_jflags[1] = Btuhdr.btd_jflags[1];
	res->btu_jflags[2] = Btuhdr.btd_jflags[2];
	res->btu_vflags[0] = Btuhdr.btd_vflags[0];
	res->btu_vflags[1] = Btuhdr.btd_vflags[1];
	res->btu_vflags[2] = Btuhdr.btd_vflags[2];
}

/* Routine called by uloop_over to check for new users */

void  chk_nuser(int fid, char *arg, int_ugid_t uid)
{
	Btuser	uu;

	if  (!readu(fid, uid, &uu))  {
		init_defaults(&uu, uid);
		insertu(fid, &uu);
	}
}

void  rebuild_btufile()
{
	int		needsquash;
	long		posn;
	char		*fname = envprocess(BTUFILE);
	Btuser		bu;
	struct	stat	pwbuf;

	/* First lock the file (it must be open to get here).  This
	   might take a long time because some other guy got in
	   first and locked the file.  If he did then we probably
	   don't need to do the regen.  */

	savesigs(1);
	lockit(btuf_fid, F_WRLCK);
	lseek(btuf_fid, 0L, 0);
	read(btuf_fid, (char *)&Btuhdr, sizeof(Btuhdr));
	stat("/etc/passwd", &pwbuf);
	if  (Btuhdr.btd_lastp >= pwbuf.st_mtime)
		goto  forgetit;

	/* Go through the users in the password file and see if we
	   know about them.  */

	uloop_over(btuf_fid, chk_nuser, (char *) 0);

	/* We now remove users who no longer belong.  'needsquash'
	   records users over the direct seek limit who have
	   disappeared. In this case we mark them as invalid and
	   rewrite the file in the next loop eliminating them.  */

	needsquash = 0;
	posn = sizeof(Btuhdr);
	lseek(btuf_fid, posn, 0);

	for  (; read(btuf_fid, (char *) &bu, sizeof(Btuser)) == sizeof(Btuser);  posn += sizeof(Btuser))  {

		if  (!bu.btu_isvalid)  {
			/* The expression in the next statement could
			   be replaced by bu.btu_user, but there
			   is always the possibility that the
			   file has been mangled which this will
			   tend to eliminate.  */
			if  ((posn - sizeof(Btuhdr)) / sizeof(Btuser) >= SMAXUID)
				needsquash++;
			continue;
		}

		if  (isvuser((uid_t) bu.btu_user))	/* Still in pw file */
			continue;

		/* Mark no longer valid.  If it's beyond the magic
		   limit we will need to squash the file.  */

		bu.btu_isvalid = 0;
		lseek(btuf_fid, -(long) sizeof(Btuser), 1);
		write(btuf_fid, &bu, sizeof(Btuser));
		if  ((ULONG) bu.btu_user >= SMAXUID)
			needsquash++;
	}

	/* Copy password file mod time into header.  We do this rather
	   than the current time (a) to reduce the chances of
	   missing a further change (b) to cope with `time moving
	   backwards'.  */

	Btuhdr.btd_lastp = pwbuf.st_mtime;
	lseek(btuf_fid, 0L, 0);
	write(btuf_fid, (char *) &Btuhdr, sizeof(Btuhdr));

	/* Ok now for squashes. We copy out to a temporary file and
	   copy back in case anyone else is looking at the file
	   it saves messing around with locks.  */

	if  (needsquash)  {
		char	*tfname = envprocess(BTUTMP);
		int	outfd, uc;

		if  ((outfd = open(tfname, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0)  {
			disp_str = tfname;
			print_error($E{Cannot create temp user file});
			free(tfname);
			goto  forgetit;
		}

		/* Copy over the direct seek bits.  First the
		   header. Note that we should be at just the
		   right place in the original file.  */

		write(outfd, (char *) &Btuhdr, sizeof(Btuhdr));
		for  (uc = 0;  uc < SMAXUID;  uc++)  {
			read(btuf_fid, (char *) &bu, sizeof(bu));
			write(outfd, (char *) &bu, sizeof(bu));
		}

		/* And now for the rest */

		while  (read(btuf_fid, (char *) &bu, sizeof(bu)) == sizeof(bu))
			if  (bu.btu_isvalid)
				write(outfd, (char *) &bu, sizeof(bu));

		/* Now truncate the original file.  If we have some
		   sort of `truncate' system call use it here
		   instead of this mangling.  */

		lseek(btuf_fid, 0L, 0);
		lseek(outfd, (long) sizeof(Btuhdr), 0);
#ifdef	HAVE_FTRUNCATE
		ftruncate(btuf_fid, 0L);
#else
		close(open(fname, O_RDWR|O_TRUNC));
#endif

		write(btuf_fid, (char *) &Btuhdr, sizeof(Btuhdr));
		while  (read(outfd, (char *) &bu, sizeof(bu)) == sizeof(bu))
			write(btuf_fid, (char *) &bu, sizeof(bu));

		close(outfd);
		unlink(tfname);
		free(tfname);
	}

 forgetit:
	savesigs(0);
	free(fname);
	unlockit(btuf_fid);
	btu_needs_rebuild = 0;
}

/* See if we need to regenerate user file because of new users added recently.  */

static void  open_file(int mode)
{
	char	*fname = envprocess(BTUFILE);
	struct	stat	pwbuf;

	if  ((btuf_fid = open(fname, mode)) < 0)  {
		if  (errno == EACCES)  {
			print_error($E{Check file setup});
			exit(E_SETUP);
		}
		if  (errno == ENOENT)
			init_file(fname);
		btuf_fid = open(fname, mode);
	}

	if  (btuf_fid < 0)  {
		print_error($E{Cannot create user control file});
		exit(E_SETUP);
	}

	fcntl(btuf_fid, F_SETFD, 1);
	lockit(btuf_fid, F_RDLCK);
	read(btuf_fid, (char *)&Btuhdr, sizeof(Btuhdr));

	/* Check version number and print warning message if funny.  */

	if  (Btuhdr.btd_version != GNU_BATCH_MAJOR_VERSION)  {
		disp_arg[0] = Btuhdr.btd_version;
		disp_arg[1] = GNU_BATCH_MAJOR_VERSION;
		print_error($E{Wrong version of product});
	}

	if  (stat("/etc/passwd", &pwbuf) < 0)  {
		free(fname);
		return;
	}

	unlockit(btuf_fid);
	free(fname);

	if  (Btuhdr.btd_lastp >= pwbuf.st_mtime)  {
		if  (Btuhdr.btd_lastp > pwbuf.st_mtime)
			print_error($E{Funny times passwd file});
		btu_needs_rebuild = 0;
		return;
	}
	btu_needs_rebuild = 1;
}

/* Get info about specific user.  If we haven't met the guy before return null.  */

static BtuserRef  gpriv(const int_ugid_t uid)
{
	static	Btuser	result;
	int	ret;

	lockit(btuf_fid, F_RDLCK);
	ret = readu(btuf_fid, uid, &result);
	unlockit(btuf_fid);
	return  ret? &result: (BtuserRef) 0;
}

/* Get list of all users known.  */

static BtuserRef  gallpriv(unsigned *Np)
{
	BtuserRef	result, rp;
	unsigned  maxu = INITU, count = 0;

	if  ((result = (BtuserRef) malloc(INITU*sizeof(Btuser))) == (BtuserRef) 0)
		ABORT_NOMEM;

	/* NB assume that the last thing we did was read the header.
	   (or seek to it!).  */

	rp = result;
	while  (read(btuf_fid, (char *) rp, sizeof(Btuser)) == sizeof(Btuser))  {
		if  (rp->btu_isvalid)  {
			rp++;
			count++;
			if  (count >= maxu)  {
				maxu += INCU;
				if  ((result = (BtuserRef) realloc((char *) result, maxu * sizeof(Btuser))) == (BtuserRef) 0)
					ABORT_NOMEM;
				rp = &result[count];
			}
		}
	}

	*Np = count;
	return  result;
}

/* Routine to access privilege/mode file.  This is now the basic
   routine for user programs and does not return if there's a problem.  */

BtuserRef  getbtuser(const uid_t uid)
{
	BtuserRef	result;

	open_file(O_RDONLY);
	result = gpriv(uid);
	close(btuf_fid);
	btuf_fid = -1;

	if  (!result)  {
		print_error($E{Not registered yet});
		exit(E_UNOTSETUP);
	}
	Fileprivs = result->btu_priv;
	return  result;
}

/* Get entry in user file, possibly for update Only done for utility routines.  */

BtuserRef  getbtuentry(const uid_t uid)
{
	if  (btuf_fid < 0)
		open_file(O_RDWR);
	return  gpriv(uid);
}

/* Update details for given user only.  */

void  putbtuentry(BtuserRef item)
{
	lockit(btuf_fid, F_WRLCK);
	insertu(btuf_fid, item);
	unlockit(btuf_fid);
}

BtuserRef  getbtulist(unsigned *Nu)
{
	BtuserRef	result;
	if  (btuf_fid < 0)
		open_file(O_RDWR);
	else
		lseek(btuf_fid, (long) sizeof(Btuhdr), 0);
	lockit(btuf_fid, F_RDLCK);
	result = gallpriv(Nu);
	unlockit(btuf_fid);
	return  result;
}

/* Save list.  */

void  putbtulist(BtuserRef list, unsigned num, int hchanges)
{
	lockit(btuf_fid, F_WRLCK);
	if  (hchanges)  {
		lseek(btuf_fid, 0L, 0);
		write(btuf_fid, (char *) &Btuhdr, sizeof(Btuhdr));
	}
	else
		lseek(btuf_fid, (long) sizeof(Btuhdr), 0);

	if  (list)  {
		BtuserRef	up;
		for  (up = list;  up < &list[num];  up++)
			insertu(btuf_fid, up);
	}
	unlockit(btuf_fid);
}
