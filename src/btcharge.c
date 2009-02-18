/* btcharge.c -- main program for gbch-charge

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

/*
 * btcharge:
 *	print/manipulate charging details
 *
 * With this program you can:
 *	print the charge units accrued for all or a specified group of users,
 *	and optionally zero it after printing
 *
 *	add a specified number of charge units to the stored charge
 *	for a given set of users
 *
 * Usage:
 *	btcharge [-p] [-z] [-c charge] [user ...]
 *						print out [and zero] charges
 *						for specified users,
 *						default is all users
 *
 * if no flags given, default is -p
 *
 * if no flags given, default is -p
 *
 *	-p
 *		print the charges for the given users (summary)
 *	-P
 *		print full details of charges
 *	-z
 *		zero the stored charges for the given users
 *	-c charge
 *		add the charge to the stored charges for the given users
 *	-C
 *		consolidate charges for given users
 *	-R
 *		reset file to minimum length
 *	-K
 *		cancel flags
 *
 *	in all cases, no list of users means all users
 *
 * N.B.:
 *	btcharge -p -z
 *		will print out the records BEFORE zeroing them
 *	btcharge -p -c charge
 *		will print out the records AFTER adding the charge to them!
 */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#ifdef	TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#elif	defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "incl_unix.h"
#include "defaults.h"
#include "incl_ugid.h"
#include "files.h"
#include "btmode.h"
#include "btuser.h"
#include "errnums.h"
#include "ecodes.h"
#include "cfile.h"
#include "helpargs.h"
#include "ipcstuff.h"
#include "optflags.h"

static	char	Filename[] = __FILE__;

FILE	*Cfile;					/* open message file */
char	*Curr_pwd;

int	nerrors	= 0;				/* # errors */

int	zero_usage = 0,
	print_usage = 0,
	consolidate = 0,
	reset_file = 0;

double	chargefee = 0.0;

char	*Restru, *Restrg, *jobqueue;

uid_t	Realuid, Effuid, Daemuid;
gid_t	Realgid, Effgid;

#ifdef	SHAREDLIBS
#include "helpalt.h"
#include "bjparam.h"
#include "cmdint.h"
#include "btconst.h"
#include "btvar.h"
#include "timecon.h"
#include "btjob.h"
#include "q_shm.h"

int		Ctrl_chan;
long		mymtype;
HelpaltRef	repunit, ifnposses, days_abbrev;
char		*Args[1], *exitcodename, *signalname;
BtuserRef	mypriv;
struct		jshm_info	Job_seg;
#endif

char	*file_name;

unsigned	num_users, hashed_users;
int_ugid_t	*user_list;

struct	huid	{
	struct	huid	*next;
	char		*uname;
	int_ugid_t	uid;
	int		wanted;
	double		charge;
};

#define	UG_HASHMOD	97
static	struct	huid	*uhash[UG_HASHMOD];

void  nomem(const char *fl, const int ln)
{
	fprintf(stderr, "%s:Mem alloc fault: %s line %d\n", progname, fl, ln);
	exit(E_NOMEM);
}

/* Generate list of user ids from argument list.  */

static void  translate_users(char **ulist)
{
	int_ugid_t	ru, *lp;
	char	**up;

	/* Count them...  */

	for  (up = ulist;  *up;  up++)
		num_users++;

	if  (num_users == 0)
		return;

	if  (!(user_list = (int_ugid_t *) malloc(num_users * sizeof(int_ugid_t))))
		ABORT_NOMEM;

	lp = user_list;

	for  (up = ulist;  *up;  up++)
		if  ((ru = lookup_uname(*up)) == UNKNOWN_UID)  {
			disp_str = *up;
			print_error($E{Btcharge unknown user});
			nerrors++;
			num_users--;
		}
		else
			*lp++ = ru;

	/* If they're all dead, stop */

	if  (num_users == 0)
		exit(E_NOUSER);
}

/* Open file as required */

static	int  grab_file(const int omode)
{
	int	ret;

	if  ((ret = open(file_name, omode)) < 0)  {
		print_error(errno != ENOENT? $E{Cannot open user control file}: $E{Btcharge not created yet});
		exit(E_SETUP);
	}
	return  ret;
}

/* Impose fee. If no arguments are given, it must be on everyone */

static	void  impose_fee()
{
	int	fd = grab_file(O_WRONLY|O_APPEND);
	struct	btcharge	btc;

	time(&btc.btch_when);
	btc.btch_host = 0;		/* Me */
	btc.btch_pri = 0;
	btc.btch_ll = 0;
	btc.btch_runtime = chargefee;

	if  (num_users == 0)  {
		btc.btch_user = -1;
		btc.btch_what = BTCH_FEEALL;
		write(fd, (char *) &btc, sizeof(btc));
	}
	else  {
		unsigned  uc;
		btc.btch_what = BTCH_FEE;
		for  (uc = 0;  uc < num_users;  uc++)  {
			btc.btch_user = user_list[uc];
			write(fd, (char *) &btc, sizeof(btc));
		}
	}
	close(fd);
}

/* Calculate hash and allocate hash chain item.  */

static	struct huid *rhash(const int_ugid_t uid, const int had)
{
	struct	huid	*rp, **rpp;

	for  (rpp = &uhash[(ULONG) uid % UG_HASHMOD]; (rp = *rpp);  rpp = &rp->next)
		if  (uid == rp->uid)  {
			rp->wanted |= had;
			return  rp;
		}
	if  (!(rp = (struct huid *) malloc(sizeof(struct huid))))
		ABORT_NOMEM;
	rp->next = (struct huid *) 0;
	rp->uid = uid;
	rp->uname = (char *) 0;
	rp->wanted = had;
	rp->charge = 0;
	*rpp = rp;
	hashed_users++;
	return  rp;
}

/* Read in charge file and hash it up by user id and calculate charge.  */

static	void  buffer_up(const int had)
{
	int	fd = grab_file(O_RDONLY);
	unsigned  hp;
	struct	huid	*up;
	double	curr_chargeall = 0;
	struct	btcharge	btc;

	while  (read(fd, (char *) &btc, sizeof(btc)) == sizeof(btc))  {
		double	res;

		switch  (btc.btch_what)  {
		case  BTCH_RECORD:	/* Record left by btshed */
			up = rhash(btc.btch_user, had);
			res = btc.btch_pri;
			res /= U_DF_DEFP;
			up->charge += res * res * (double) btc.btch_ll * btc.btch_runtime / 3600.0;
			break;

		case  BTCH_FEE:			/* Impose fee */
			up = rhash(btc.btch_user, had);
			up->charge += btc.btch_runtime;
			break;

		case  BTCH_FEEALL:		/* Impose fee to all*/
			curr_chargeall += btc.btch_runtime;
			break;

		case  BTCH_CONSOL:		/* Consolidation of previous charges */
			up = rhash(btc.btch_user, had);
			up->charge = btc.btch_runtime - curr_chargeall;
			break;

		case  BTCH_ZERO:		/* Zero record for given user */
			up = rhash(btc.btch_user, had);
			up->charge = -curr_chargeall; /* To cancel effect when added later */
			break;

		case  BTCH_ZEROALL:		/* Zero record for all users */
			curr_chargeall = 0;
			for  (hp = 0;  hp < UG_HASHMOD;  hp++)
				for  (up = uhash[hp];  up;  up = up->next)
					up->charge = 0;
			break;
		}
	}

	if  (curr_chargeall != 0.0)
		for  (hp = 0;  hp < UG_HASHMOD;  hp++)
			for  (up = uhash[hp];  up;  up = up->next)
				up->charge += curr_chargeall;
	close(fd);
}

/* A sort of sort routine for qsort */

static int  sort_u(struct huid **a, struct huid **b)
{
	return  strcmp((*a)->uname, (*b)->uname);
}

/* Another sort of sort routine for qsort */

static	int  sort_uid(struct huid **a, struct huid **b)
{
	int_ugid_t	au = (*a)->uid, bu = (*b)->uid;
	return  au < bu? -1: au == bu? 0: 1;
}

/* Print out stuff.  */

static	void  do_print()
{
	unsigned  uc, wanted_users = 0;
	struct	huid	*hp, **sp, **sorted_list;

	if  (num_users == 0)
		buffer_up(1);
	else  {
		buffer_up(0);
		for  (uc = 0;  uc < num_users;  uc++)
			rhash(user_list[uc], 1);
	}

	/* Skip if file completely empty.  */

	if  (hashed_users == 0)
		return;

	/* Maybe we'll make this "wanted_users" rather than
	   "hashed_users" if this overflows, but we'll leave it
	   for now.  */

	if  (!(sorted_list = (struct huid **) malloc(hashed_users * sizeof(struct huid))))
		ABORT_NOMEM;

	sp = sorted_list;

	for  (uc = 0;  uc < UG_HASHMOD;  uc++)
		for  (hp = uhash[uc];  hp;  hp = hp->next)
			if  (hp->wanted)  {
				wanted_users++;
				*sp++ = hp;
				hp->uname = prin_uname(hp->uid);
			}

	qsort(QSORTP1 sorted_list, wanted_users, sizeof(struct huid *), QSORTP4 sort_u);

	/* Print the summary.  */

	for  (uc = 0;  uc < wanted_users;  uc++)  {
		disp_str = sorted_list[uc]->uname;
		disp_float = sorted_list[uc]->charge;
		fprint_error(stdout, $E{Btcharge output format});
	}
	free((char *) sorted_list);
}

static	void  do_printall()
{
	int	fd = grab_file(O_RDONLY);
	struct	btcharge	btc;

	while  (read(fd, (char *) &btc, sizeof(btc)) == sizeof(btc))  {
		disp_arg[0] = btc.btch_user;
		disp_arg[1] = btc.btch_pri;
		disp_arg[2] = btc.btch_ll;
		disp_arg[4] = btc.btch_when;
		disp_float = btc.btch_runtime;

		switch  (btc.btch_what)  {
		default:
			disp_arg[0] = btc.btch_what;
			print_error($E{Unexpected charges record});
			break;

		case  BTCH_RECORD:	/* Record left by btshed */
			if  (btc.btch_host)  {
				disp_arg[5] = btc.btch_host;
				fprint_error(stdout, $E{Btcharge record other host});
			}
			else
				fprint_error(stdout, $E{Btcharge record this host});
			break;

		case  BTCH_FEE:			/* Impose fee */
			fprint_error(stdout, $E{Btcharge fee imposed});
			break;

		case  BTCH_FEEALL:		/* Impose fee to all*/
			fprint_error(stdout, $E{Btcharge fee all});
			break;

		case  BTCH_CONSOL:		/* Consolidation of previous charges */
			fprint_error(stdout, $E{Btcharge user consolidated});
			break;

		case  BTCH_ZERO:		/* Zero record for given user */
			fprint_error(stdout, $E{Btcharge zero user});
			break;

		case  BTCH_ZEROALL:		/* Zero record for all users */
			fprint_error(stdout, $E{Btcharge zero all});
			break;
		}
	}
	close(fd);
}

static	struct huid **get_sorted()
{
	unsigned	uc;
	struct	huid	*hp, **sp, **sorted_list;

	if  (hashed_users == 0)  {
		buffer_up(0);
		if  (hashed_users == 0)
			return  (struct huid **) 0;
	}
	if  (!(sorted_list = (struct huid **) malloc(hashed_users * sizeof(struct huid))))
		ABORT_NOMEM;
	sp = sorted_list;
	for  (uc = 0;  uc < UG_HASHMOD;  uc++)
		for  (hp = uhash[uc];  hp;  hp = hp->next)
			*sp++ = hp;
	qsort(QSORTP1 sorted_list, hashed_users, sizeof(struct huid *), QSORTP4 sort_uid);
	return  sorted_list;
}

static	void  write_consol(struct huid **sorted_list, const int omode)
{
	int		fd;
	unsigned	uc;
	struct  btcharge	orec;

	time(&orec.btch_when);
	orec.btch_host = 0;
	orec.btch_pri = 0;
	orec.btch_what = BTCH_CONSOL;
	orec.btch_ll = 0;

	fd = grab_file(omode);
	for  (uc = 0;  uc < hashed_users;  uc++)  {
		if  ((orec.btch_runtime = sorted_list[uc]->charge) == 0)
			continue;
		orec.btch_user = sorted_list[uc]->uid;
		write(fd, (char *) &orec, sizeof(orec));
	}
	close(fd);
	free((char *) sorted_list);
}

static void  do_consol()
{
	struct	huid	**sorted_list = get_sorted();
	if  (!sorted_list)
		return;
	write_consol(sorted_list, O_WRONLY|O_APPEND);
}

static	void  do_zero()
{
	int	fd = grab_file(O_WRONLY|O_APPEND);
	struct	btcharge	btc;

	time(&btc.btch_when);
	btc.btch_host = 0;		/* Me */
	btc.btch_pri = 0;
	btc.btch_ll = 0;
	btc.btch_runtime = 0;

	/* Any previous thing is invalid in case we do reset.  */

	if  (hashed_users != 0)  {
		unsigned  uc;
		struct	huid  *rp, *np;
		for  (uc = 0;  uc < UG_HASHMOD;  uc++)  {
			for  (rp = uhash[uc];  rp;  rp = np)  {
				np = rp->next;
				free((char *) rp);
			}
			uhash[uc] = (struct huid *) 0;
		}
		hashed_users = 0;
	}

	if  (num_users == 0)  {
		btc.btch_user = -1;
		btc.btch_what = BTCH_ZEROALL;
		write(fd, (char *) &btc, sizeof(btc));
	}
	else  {
		unsigned  uc;
		btc.btch_what = BTCH_ZERO;
		for  (uc = 0;  uc < num_users;  uc++)  {
			btc.btch_user = user_list[uc];
			write(fd, (char *) &btc, sizeof(btc));
		}
	}
	close(fd);
}

static	void  do_reset()
{
	struct	huid	**sorted_list;
	if  (msgget(MSGID+envselect_value, 0) >= 0)  {
		print_error($E{Btcharge batch running});
		exit(E_RUNNING);
	}
	sorted_list = get_sorted();
	if  (!sorted_list)
		return;
	write_consol(sorted_list, O_WRONLY|O_TRUNC|O_APPEND);
}

OPTION(o_explain)
{
	print_error($E{btcharge explain});
	exit(0);
	return  OPTRESULT_OK;
}

OPTION(o_print)
{
	print_usage = 1;
	return  OPTRESULT_OK;
}

OPTION(o_printfull)
{
	print_usage = 2;
	return  OPTRESULT_OK;
}

OPTION(o_zero)
{
	zero_usage = 1;
	return  OPTRESULT_OK;
}

OPTION(o_charge)
{
	if  (!arg)
		return  OPTRESULT_MISSARG;
	if  ((chargefee = atof(arg)) <= 0.0)  {
		arg_errnum = $E{Bad number for charge amt};
		return  OPTRESULT_ERROR;
	}
	return  OPTRESULT_ARG_OK;
}

OPTION(o_consolidate)
{
	consolidate = 1;
	return  OPTRESULT_OK;
}

OPTION(o_resetfile)
{
	reset_file = 1;
	return  OPTRESULT_OK;
}

OPTION(o_cancelflags)
{
	zero_usage = 0;
	print_usage = 0;
	consolidate = 0;
	reset_file = 0;
	return  OPTRESULT_OK;
}

DEOPTION(o_freezecd);
DEOPTION(o_freezehd);

/* Defaults and proc table for arg interp.  */

const	Argdefault  Adefs[] = {
	{  '?', $A{btcharge arg explain} },
	{  'p', $A{btcharge arg print} },
	{  'P', $A{btcharge arg full} },
	{  'z', $A{btcharge arg zero} },
	{  'c', $A{btcharge arg add} },
	{  'C', $A{btcharge arg consol} },
	{  'R', $A{btcharge arg reset} },
	{  'K', $A{btcharge arg cancel} },
	{ 0, 0 }
};

optparam	optprocs[] = {
o_explain,	o_print,	o_printfull,	o_zero,
o_charge,	o_consolidate,	o_resetfile,	o_cancelflags,
o_freezecd,	o_freezehd
};

void  spit_options(FILE *dest, const char *name)
{
	int	cancont;
	fprintf(dest, "%s", name);
	cancont = spitoption($A{btcharge arg cancel}, $A{btcharge arg explain}, dest, '=', 0);
	if  (print_usage)
		cancont = spitoption(print_usage > 1? $A{btcharge arg full}: $A{btcharge arg print}, $A{btcharge arg explain}, dest, ' ', cancont);
	if  (zero_usage)
		cancont = spitoption($A{btcharge arg zero}, $A{btcharge arg explain}, dest, ' ', cancont);
	if  (reset_file)
		cancont = spitoption($A{btcharge arg reset}, $A{btcharge arg explain}, dest, ' ', cancont);
	if  (consolidate)
		cancont = spitoption($A{btcharge arg consol}, $A{btcharge arg explain}, dest, ' ', cancont);
	if  (chargefee)  {
		spitoption($A{btcharge arg add}, $A{btcharge arg explain}, dest, ' ', 0);
		fprintf(dest, " %g", chargefee);
	}
	putc('\n', dest);
}

MAINFN_TYPE  main(int argc, char **argv)
{
#if	defined(NHONSUID) || defined(DEBUG)
	int_ugid_t	chk_uid;
#endif
	BtuserRef	mypriv;

	versionprint(argv, "$Revision: 1.1 $", 0);

	if  ((progname = strrchr(argv[0], '/')))
		progname++;
	else
		progname = argv[0];

	init_mcfile();
	file_name = envprocess(CHFILE);

	Realuid = getuid();
	Realgid = getgid();
	Effuid = geteuid();
	Effgid = getegid();
	INIT_DAEMUID
	Cfile = open_cfile(MISC_UCONFIG, "btrest.help");
	SCRAMBLID_CHECK
	argv = optprocess(argv, Adefs, optprocs, $A{btcharge arg explain}, $A{btcharge arg freeze home}, 0);

	/* No options defaults to -p */

	if  (chargefee == 0.0  &&  zero_usage + print_usage + consolidate + reset_file == 0)
		print_usage = 1;

	SWAP_TO(Daemuid);

#define	FREEZE_EXIT
#include "inline/freezecode.c"

	mypriv = getbtuser(Realuid);
	if  (chargefee != 0.0  ||  zero_usage + consolidate + reset_file != 0)  {
		if  (!(mypriv->btu_priv & BTM_WADMIN))  {
			print_error($E{No write admin file priv});
			exit(E_NOPRIV);
		}
	}
	else  if  (!(mypriv->btu_priv & BTM_RADMIN))  {
		print_error($E{No read admin file priv});
		exit(E_NOPRIV);
	}

	/* First impose any charges
	   Then do any printing.
	   Then do any consolidation.
	   Then reset file. */

	translate_users(&argv[0]);

	if  (chargefee)
		impose_fee();
	if  (print_usage)  {
		if  (print_usage > 1)
			do_printall();
		else
			do_print();
	}
	if  (consolidate)
		do_consol();
	if  (zero_usage)
		do_zero();
	if  (reset_file)
		do_reset();
	exit(nerrors);
}
