/* bq_jci.c -- Command Interpreter handling for gbch-q

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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <curses.h>
#include <ctype.h>
#include "incl_unix.h"
#include "defaults.h"
#include "incl_net.h"
#include "network.h"
#include "incl_ugid.h"
#include "btconst.h"
#include "timecon.h"
#include "btmode.h"
#include "bjparam.h"
#include "btjob.h"
#include "btuser.h"
#include "cmdint.h"
#include "magic_ch.h"
#include "sctrl.h"
#include "ecodes.h"
#include "errnums.h"
#include "statenums.h"
#include "files.h"
#include "helpalt.h"
#include "optflags.h"

#ifdef	OS_BSDI
#define	_begy	begy
#define	_begx	begx
#define	_maxy	maxy
#define	_maxx	maxx
#endif

#ifndef	getmaxyx
#define	getmaxyx(win,y,x)	((y) = (win)->_maxy, (x) = (win)->_maxx)
#endif
#ifndef	getbegyx
#define	getbegyx(win,y,x)	((y) = (win)->_begy, (x) = (win)->_begx)
#endif

#define	BOXWID	1
#define	LINES_P_CI	3

void	dochelp(WINDOW *, int);
void	doerror(WINDOW *, int);
void	endhe(WINDOW *, WINDOW **);
void	ws_fill(WINDOW *, const int, const struct sctrl *, const char *);
void	wn_fill(WINDOW *, const int, const struct sctrl *, const LONG);
extern	int	rdalts(WINDOW *, int, int, HelpaltRef, int, int);
extern	int	r_max(int, int);
extern	int	chk_okcidel(const int);
extern	LONG	wnum(WINDOW *, const int, struct sctrl *, const LONG);
extern	const	char *qtitle_of(CBtjobRef);
extern	char	*wgets(WINDOW *, const int, struct sctrl *, const char *);
void	mvwhdrstr(WINDOW *, const int, const int, const char *);

static	char	Filename[] = __FILE__;

extern	WINDOW	*escr,
		*hlpscr,
		*jscr,
		*hjscr,
		*tjscr,
		*Ew;

extern	char	*Curr_pwd,
		*spdir;

#define	NULLCP		(char *) 0
#define	HELPLESS	((char **(*)()) 0)

#define	CINAME_P	3
#define	CIARGS_P	19
#define	CIPATH_P	6
#define	CILL_P		52
#define	CINICE_P	62
#define	CIEXP_P		65
#define	CIARG0_P	69

static	struct	sctrl
 wss_name = { $H{btq ci unique name}, HELPLESS, CI_MAXNAME, 0, CINAME_P, MAG_P, 0L, 0L, NULLCP },
 wss_path = { $H{btq ci path name}, HELPLESS, CI_MAXFPATH, 0, CIPATH_P, MAG_OK, 0L, 0L, NULLCP },
 wss_args = { $H{btq ci args}, HELPLESS, CI_MAXARGS, 0, CIARGS_P, MAG_OK|MAG_NL, 0L, 0L, NULLCP },
 wns_ll = { $H{btq ci load lev}, HELPLESS, 5, 0, CILL_P, MAG_P, 1L, 0x7fffL, NULLCP },
 wns_nice = { $H{btq ci nice}, HELPLESS, 2, 0, CINICE_P, MAG_P, 0L, 39L, NULLCP },
 ud_dir = { $H{btq unqueue dir}, HELPLESS, 0, 0, 0, MAG_P|MAG_OK, 0L, 0L, NULLCP },
 ud_xf =  { $H{btq unqueue command file}, HELPLESS, CI_MAXNAME, 0, 0, MAG_P|MAG_OK, 0L, 0L, NULLCP },
 ud_jf =  { $H{btq unqueue job file}, HELPLESS, CI_MAXNAME, 0, 0, MAG_P|MAG_OK, 0L, 0L, NULLCP };

static	char	*ci_arg0flg,	/* Mark substitute title for arg 0 */
		*ci_expflag;	/* Mark do expansion of $n args */

int		Ci_hrows;	/* Rows for header */
char		**Ci_hdr;

#define	NULLENTRY(n)	(Ci_list[n].ci_name[0] == '\0')

void  initcifile()
{
	int	ret;

	if  ((ret = open_ci(O_RDWR)) != 0)  {
		print_error(ret);
		exit(E_SETUP);
	}
	Ci_hdr = helphdr('S');
	count_hv(Ci_hdr, &Ci_hrows, (int *) 0);
}

static void  rewcil(int ind)
{
	lseek(Ci_fd, (long) (ind * sizeof(Cmdint)), 0);
	Ignored_error = write(Ci_fd, (char *) &Ci_list[ind], sizeof(Cmdint));
}

static void  cidisplay(int start)
{
	char	**hv;
	int	rr;

	rereadcif();
#ifdef	OS_DYNIX
	clear();
#else
	erase();
#endif
	for  (rr = 0, hv = Ci_hdr;  *hv;  rr++, hv++)
		mvwhdrstr(stdscr, rr, 0, *hv);

	for  (;  start < Ci_num  &&  rr + LINES_P_CI <= LINES;  start++)  {
		if  (NULLENTRY(start))
			continue;
		ws_fill(stdscr, rr, &wss_name, Ci_list[start].ci_name);
		ws_fill(stdscr, rr, &wss_args, Ci_list[start].ci_args);
		wn_fill(stdscr, rr, &wns_ll, (LONG) Ci_list[start].ci_ll);
		wn_fill(stdscr, rr, &wns_nice, (LONG) Ci_list[start].ci_nice);
		if  (Ci_list[start].ci_flags & CIF_INTERPARGS)
			mvaddstr(rr, CIEXP_P, ci_expflag);
		if  (Ci_list[start].ci_flags & CIF_SETARG0)
			mvaddstr(rr, CIARG0_P, ci_arg0flg);
		ws_fill(stdscr, rr + 1, &wss_path, Ci_list[start].ci_path);

		rr += LINES_P_CI;
	}
#ifdef	CURSES_OVERLAP_BUG
	touchwin(stdscr);
#endif
}

/* See if the path specified is something like a sensible executable */

static int  pok(char *name)
{
	struct	stat	sbuf;

	if  (name[0] != '/'  ||
	     stat(name, &sbuf) < 0  ||
	     (sbuf.st_mode & S_IFMT) != S_IFREG  ||
#if	defined(S_IXUSR) && defined(S_IXGRP) && defined(S_IXOTH)
	     (sbuf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0)
#else
	     (sbuf.st_mode & 0111) == 0)
#endif
		return  0;
	return  1;
}

/* View/edit command interpreters */

int  ciprocess()
{
	int	err_no, ch, nsel, start, select, srow, currow, tilines, cnt, incr, creatrow, readwrite;
	char	*str;
	Cmdint	Newentry;

	if  (!ci_arg0flg)
		ci_arg0flg = gprompt($P{Ci set arg0});
	if  (!ci_expflag)
		ci_expflag = gprompt($P{Ci expand args});
	rereadcif();

	readwrite = mypriv->btu_priv & BTM_SPCREATE;

	start = 0;
	srow = 0;
	tilines = ((LINES - Ci_hrows) / LINES_P_CI) * LINES_P_CI;

 Cireset:
	Ew = stdscr;
	select_state($S{btq edit command ints});
 Cidisp:
	cidisplay(start);
 Cimove:
	currow = (srow - start) * LINES_P_CI;
	wmove(stdscr, currow+Ci_hrows, 0);
	cnt = srow;
	for  (select = 0;  select < Ci_num;  select++)  {
		if  (NULLENTRY(select))
		     continue;
		if  (--cnt < 0)
			goto  Cirefresh;
	}
	if  (srow == 0)  {
		if  (hjscr)  {
			touchwin(hjscr);
#ifdef	HAVE_TERMINFO
			wnoutrefresh(hjscr);
#else
			wrefresh(hjscr);
#endif
		}
		if  (tjscr)  {
			touchwin(tjscr);
#ifdef	HAVE_TERMINFO
			wnoutrefresh(tjscr);
#else
			wrefresh(tjscr);
#endif
		}
		doerror(jscr, $E{btq no command interps});
#ifdef	CURSES_MEGA_BUG
		clear();
		refresh();
#endif
		return  -1;
	}
	start = 0;
	srow = 0;
	goto  Cimove;

 Cirefresh:

	refresh();

 nextin:
	do  ch = getkey(MAG_A|MAG_P);
	while  (ch == EOF  &&  (hlpscr || escr));

	if  (hlpscr)  {
		endhe(stdscr, &hlpscr);
		if  (Dispflags & DF_HELPCLR)
			goto  nextin;
	}
	if  (escr)
		endhe(stdscr, &escr);

	switch  (ch)  {
	case  EOF:
		goto  nextin;
	default:
		err_no = $E{btq ci unknown command};
	err:
		doerror(stdscr, err_no);
		goto  nextin;

	case  $K{key help}:
		dochelp(stdscr, $H{btq edit command ints});
		goto  nextin;

	case  $K{key refresh}:
		wrefresh(curscr);
		goto  Cirefresh;

	/* Move up or down.  */

	case  $K{key cursor down}:
		for  (nsel = select + 1;  nsel < Ci_num  &&  NULLENTRY(nsel); nsel++)
			;
		if  (nsel >= Ci_num)  {
ev:			err_no = $E{btq ci off end};
			goto  err;
		}
		select = nsel;
		srow++;
		if  ((currow += LINES_P_CI) < tilines)
			goto  Cimove;
		start++;
		goto  Cidisp;

	case  $K{key cursor up}:
		for  (nsel = select - 1;  nsel >= 0  &&  NULLENTRY(nsel);  nsel--)
			;
		if  (nsel < 0)  {
bv:			err_no = $E{btq ci off beginning};
			goto  err;
		}
		select = nsel;
		srow--;
		if  (srow >= start)
			goto  Cimove;
		start = srow;
		goto  Cidisp;

	/* Half/Full screen up/down */

	case  $K{key screen down}:
		cnt = tilines / LINES_P_CI;
	countdown:
		incr = cnt;
		for  (nsel = select + 1;  nsel < Ci_num;  nsel++)
			if  (!NULLENTRY(nsel)  &&  --cnt <= 0)
				goto  gotitd;
		goto  ev;
	gotitd:
		start += incr;
		srow += incr;
		goto  Cidisp;

	case  $K{key half screen down}:
		cnt = tilines / LINES_P_CI / 2;
		goto  countdown;

	case  $K{key half screen up}:
		cnt = tilines / LINES_P_CI / 2;
		goto  countup;

	case  $K{key screen up}:
		cnt = tilines / LINES_P_CI;
	countup:
		incr = cnt;
		for  (nsel = select - 1;  nsel >= 0;  nsel--)
			if  (!NULLENTRY(nsel)  &&  --cnt <= 0)
				goto  gotit;
		goto  bv;
	gotit:
		start -= incr;
		srow -= incr;
		goto  Cidisp;

		/* Go home.  */

	case  $K{key halt}:

		/* May need to restore clobbered heading */

#ifdef	CURSES_OVERLAP_BUG
		clear();
		refresh();
#endif
		if  (hjscr)  {
			touchwin(hjscr);
#ifdef	HAVE_TERMINFO
			wnoutrefresh(hjscr);
#else
			wrefresh(hjscr);
#endif
		}
		if  (tjscr)  {
			touchwin(tjscr);
#ifdef	HAVE_TERMINFO
			wnoutrefresh(tjscr);
#else
			wrefresh(tjscr);
#endif
		}
		return  -1;

	case  $K{btq key add ci}:
		if  (!readwrite)  {
		noperm:
			err_no = $E{btq ci no permission};
			goto  err;
		}

		/* Find an empty place on the screen if there is one,
		   otherwise clear the current line.  */

		creatrow = currow;
		nsel = select;

		while  ((creatrow + LINES_P_CI) < tilines)  {
			for  (nsel++;  nsel < Ci_num  &&  NULLENTRY(nsel);  nsel++)
				;
			creatrow += LINES_P_CI;
			if  (nsel >= Ci_num)
				goto  foundc;
		}
		creatrow = currow;
		clrtoeol();
		move(Ci_hrows+currow+1, 0);
		clrtoeol();
	foundc:
		str = wgets(stdscr, Ci_hrows+creatrow, &wss_name, "");
		if  (str == (char *) 0  || str[0] == '\0')
			goto  Cidisp;

		for  (nsel = 0;  nsel < Ci_num;  nsel++)
			if  (strcmp(Ci_list[nsel].ci_name, str) == 0)  {
				disp_str = str;
				cidisplay(start);
				err_no = $E{btq ci unique name};
				move(Ci_hrows+creatrow, 0);
				goto  err;
			}
		strncpy(Newentry.ci_name, str, CI_MAXNAME);
		Newentry.ci_name[CI_MAXNAME] = '\0';
		Newentry.ci_args[0] = '\0';

		/* Get path name.  */

		for  (;;)  {
			wss_path.msg = Newentry.ci_name;
			str = wgets(stdscr, Ci_hrows + creatrow + 1, &wss_path, "");
			if  (str == (char *) 0  || str[0] == '\0')
				goto  Cidisp;
			if  (pok(str))  {
				strncpy(Newentry.ci_path, str, CI_MAXFPATH);
				Newentry.ci_path[CI_MAXFPATH] = '\0';
				break;
			}
			ws_fill(stdscr, Ci_hrows + creatrow + 1, &wss_path, "");
			disp_str = str;
			doerror(stdscr, $E{btq ci path name});
		}

		Newentry.ci_ll = mypriv->btu_spec_ll;
		Newentry.ci_nice = DEF_CI_NICE;
		Newentry.ci_flags = 0;
		for  (nsel = 0;  nsel < Ci_num;  nsel++)
			if  (NULLENTRY(nsel))  {
				lseek(Ci_fd, (long) (nsel * sizeof(Cmdint)), 0);
				Ignored_error = write(Ci_fd, (char *) &Newentry, sizeof(Cmdint));
				goto  Cidisp;
			}
		lseek(Ci_fd, 0L, 2);
		Ignored_error = write(Ci_fd, (char *) &Newentry, sizeof(Cmdint));
		goto  Cidisp;

	case  $K{btq key delete ci}:

		if  (!readwrite)
			goto  noperm;

		/* Don't let anyone delete the shell.  */

		if  (select == CI_STDSHELL)  {
			disp_str = Ci_list[0].ci_name;
			disp_str2 = Ci_list[0].ci_path;
			err_no = $E{btq ci deleting shell};
			goto  err;
		}

		/* Don't delete names in use */

		if  (!chk_okcidel(select))  {
			disp_str = Ci_list[select].ci_name;
			disp_str2 = Ci_list[select].ci_path;
			err_no = $E{btq command interp in use};
			goto  err;
		}

		if  (Dispflags & DF_CONFABORT)  {
			char	*ca = gprompt($P{btq confirm del ci});

			clrtoeol();
			printw(ca, Ci_list[select].ci_name, Ci_list[select].ci_path);
			refresh();
			select_state($S{btq confirm del ci});

			for  (;;)  {
				do  ch = getkey(MAG_A|MAG_P);
				while  (ch == EOF  &&  (hlpscr || escr));

				if  (hlpscr)  {
					endhe(stdscr, &hlpscr);
					if  (Dispflags & DF_HELPCLR)
						continue;
				}
				if  (escr)
					endhe(stdscr, &escr);
				if  (ch == $K{key help})  {
					dochelp(stdscr, $H{btq confirm del ci});
					continue;
				}
				if  (ch == $K{key refresh})  {
					wrefresh(curscr);
					refresh();
					continue;
				}
				if  (ch == $K{Confirm delete OK}  ||  ch == $K{Confirm delete cancel})
					break;
				doerror(stdscr, $E{btq confirm del ci});
			}
			if  (ch == $K{Confirm delete cancel})
				goto  Cireset;
		}
		Ci_list[select].ci_name[0] = '\0';
		rewcil(select);
		goto  Cireset;

	case  $K{btq key ci name}:
		if  (!readwrite)
			goto  noperm;

		str = wgets(stdscr, Ci_hrows + currow, &wss_name, Ci_list[select].ci_name);
		if  (str == (char *) 0)
			goto  Cimove;

		/* Ignore null strings.  */

		if  (str[0] == '\0')  {
			ws_fill(stdscr, Ci_hrows + currow, &wss_name, Ci_list[select].ci_name);
			goto  Cimove;
		}

		for  (nsel = 0;  nsel < Ci_num;  nsel++)
			if  (nsel != select  &&  strcmp(Ci_list[nsel].ci_name, str) == 0)  {
				disp_str = str;
				err_no = $E{btq ci unique name};
				ws_fill(stdscr, Ci_hrows + currow, &wss_name, Ci_list[select].ci_name);
				move(Ci_hrows+currow, 0);
				goto  err;
			}
		strncpy(Ci_list[select].ci_name, str, CI_MAXNAME);
		Ci_list[select].ci_name[CI_MAXNAME] = '\0';
		rewcil(select);
		goto  Cimove;

	case  $K{btq key ci args}:
		if  (!readwrite)
			goto  noperm;
		wss_args.msg = Ci_list[select].ci_name;
		str = wgets(stdscr, Ci_hrows + currow, &wss_args, Ci_list[select].ci_args);
		if  (str)  {
			strncpy(Ci_list[select].ci_args, str, CI_MAXARGS);
			Ci_list[select].ci_path[CI_MAXFPATH] = '\0';
			rewcil(select);
		}
		goto  Cimove;

	case  $K{btq key ci path}:
		if  (!readwrite)
			goto  noperm;
		wss_path.msg = Ci_list[select].ci_name;
		str = wgets(stdscr, Ci_hrows + currow + 1, &wss_path, Ci_list[select].ci_path);
		if  (str == (char *) 0)
			goto  Cimove;
		if  (pok(str))  {
			strncpy(Ci_list[select].ci_path, str, CI_MAXFPATH);
			Ci_list[select].ci_path[CI_MAXFPATH] = '\0';
			rewcil(select);
			goto  Cimove;
		}
		disp_str = str;
		err_no = $E{btq ci path name};
		ws_fill(stdscr, Ci_hrows + currow + 1, &wss_path, Ci_list[select].ci_path);
		move(Ci_hrows+currow, 0);
		goto  err;

	case  $K{btq key ci ll}:
		if  (!readwrite)
			goto  noperm;
		wns_ll.msg = Ci_list[select].ci_name;
		incr = wnum(stdscr, Ci_hrows + currow, &wns_ll, (LONG) Ci_list[select].ci_ll);
		if  (incr < 0)
			goto  Cimove;
		Ci_list[select].ci_ll = (USHORT) incr;
		rewcil(select);
		goto  Cimove;

	case  $K{btq key ci nice}:
		if  (!readwrite)
			goto  noperm;
		wns_nice.msg = Ci_list[select].ci_name;
		incr = wnum(stdscr, Ci_hrows + currow, &wns_nice, (LONG) Ci_list[select].ci_nice);
		if  (incr < 0)
			goto  Cimove;
		Ci_list[select].ci_nice = (unsigned char) incr;
		rewcil(select);
		goto  Cimove;

	case  $K{btq key ci arg0}:
		if  (!readwrite)
			goto  noperm;
		Ci_list[select].ci_flags ^= CIF_SETARG0;
		rewcil(select);
		goto  Cidisp;

	case  $K{btq key ci expand}:
		if  (!readwrite)
			goto  noperm;
		Ci_list[select].ci_flags ^= CIF_INTERPARGS;
		rewcil(select);
		goto  Cidisp;
	}
}

#define	INCSILIST	3

char	**listcis(char *prefix)
{
	unsigned	cicnt;
	char	**result;
	unsigned  maxr, countr;
	int	sfl = 0;

	if  (prefix)  {
		sfl = strlen(prefix);
		maxr = (Ci_num + 1) / 2;
	}
	else
		maxr = Ci_num;

	if  ((result = (char **) malloc(maxr * sizeof(char *))) == (char **) 0)
		ABORT_NOMEM;

	countr = 0;

	for  (cicnt = 0;  cicnt < Ci_num;  cicnt++)  {

		if  (NULLENTRY(cicnt))
			continue;

		/* Skip ones which don't match the prefix.  */

		if  (strncmp(Ci_list[cicnt].ci_name, prefix, (unsigned) sfl) != 0)
			continue;

		if  (countr + 1 >= maxr)  {
			maxr += INCSILIST;
			if  ((result = (char**) realloc((char *) result, maxr * sizeof(char *))) == (char **) 0)
				ABORT_NOMEM;
		}
		result[countr++] = stracpy(Ci_list[cicnt].ci_name);
	}

	if  (countr == 0)  {
		free((char *) result);
		return  (char **) 0;
	}

	result[countr] = (char *) 0;
	return  result;
}

/* Stuff to implement the unqueue command */

int  dounqueue(BtjobRef jp)
{
	static	char	*udprog, *udprompt, *copyprompt, *dirprompt, *xfilep, *jfilep;
	static	char	*udcverb, *udccanc;
	static	HelpaltRef	udopts, verbopts, cancopts;
	static	int	maxp;
	int	jsy, jmsy, dummy, whichopt;
	PIDTYPE	pid;
	WINDOW	*cw;
	char	*hd;
	char	*Exist, *Direc, *Expdir;
	const	char	*title;

	/* Can the geyser do it?  */

	disp_str = title = qtitle_of(jp);
	disp_arg[0] = jp->h.bj_job;

	if  (!mpermitted(&jp->h.bj_mode, BTM_READ|BTM_WRITE|BTM_DELETE, mypriv->btu_priv))  {
		doerror(jscr, $E{btq unq no permission});
		return  0;
	}
	if  (jp->h.bj_progress >= BJP_STARTUP1)  {
		doerror(jscr, $E{btq unq runnable job});
		return  0;
	}

	/* First time round, read prompts */

	if  (!udprompt)  {
		udprog = envprocess(DUMPJOB);
		dirprompt = gprompt($P{btq unqueue dir});
		xfilep = gprompt($P{btq unqueue command file});
		jfilep = gprompt($P{btq unqueue job file});
		udprompt = gprompt($P{btq unqueue hdr prompt});
		udopts = galts(jscr, $Q{btq unqueue options}, 4);
		copyprompt = gprompt($P{btq copy options prompt});
		verbopts = galts(jscr, $Q{btq unq verbose}, 2);
		udcverb = gprompt($P{btq unq verbose});
		cancopts = galts(jscr, $Q{btq unq jobstate}, 2);
		udccanc = gprompt($P{btq unq jobstate});
		maxp = r_max(strlen(dirprompt), r_max(strlen(xfilep), r_max(strlen(jfilep), r_max(strlen(udcverb), strlen(udccanc)))));
		ud_xf.col = ud_jf.col = ud_dir.col = maxp + BOXWID + 1;
		ud_dir.size = COLS - maxp - BOXWID - 2;
	}

	if  (!(udopts && verbopts && cancopts))
		return  -1;

	getyx(jscr, jsy, dummy);
	whichopt = rdalts(jscr, jsy, 0, udopts, -1, $H{btq unqueue options});
	if  (whichopt < 0)
		return  -1;

	/* Create sub-window to input variable creation details */

	getbegyx(jscr, jsy, dummy);
	getmaxyx(jscr, jmsy, dummy);
	if  (jmsy < 4+2*BOXWID)  {
		doerror(jscr, $E{btq jlist no space unq win});
		return  0;
	}

	if  ((cw = newwin(5+2*BOXWID, 0, jsy + (jmsy - 5-2*BOXWID) / 2, 0)) == (WINDOW *) 0)  {
		doerror(jscr, $E{btq jlist cannot create unq win});
		return  0;
	}

#ifdef	HAVE_TERMINFO
	box(cw, 0, 0);
#else
	box(cw, '|', '-');
#endif
	mvwprintw(cw, BOXWID, BOXWID, whichopt > 1? copyprompt: udprompt, title, jp->h.bj_job);
	mvwaddstr(cw, 2+BOXWID, BOXWID, dirprompt);
	hd = Curr_pwd;
	if  (whichopt == 2)
		hd = envprocess("$HOME");
	ws_fill(cw, 2+BOXWID, &ud_dir, hd);
	if  (whichopt < 2)  {
		mvwaddstr(cw, 3+BOXWID, BOXWID, xfilep);
		mvwaddstr(cw, 4+BOXWID, BOXWID, jfilep);
	}

	/* Read directory field */

	reset_state();
	Ew = cw;

	Exist = hd;
	for  (;;)  {
		if  (!(Direc = wgets(cw, 2+BOXWID, &ud_dir, Exist)))  {
#ifdef	CURSES_MEGA_BUG
			refresh();
#endif
			delwin(cw);
			return  -1;
		}
		if  (Direc[0] == '\0')
			Expdir = stracpy(Exist);
		else  {
			Expdir = envprocess(Direc);
			if  (strchr(Expdir, '~'))  {
				char	*nd = unameproc(Expdir, hd, Realuid);
				free(Expdir);
				Expdir = nd;
			}
		}

		disp_str = Expdir;
		Exist = Direc[0]? Direc: hd;

		if  (Expdir[0] != '/')  {
			doerror(cw, $E{btq unq not absolute path});
			free(Expdir);
			continue;
		}
		if  (chdir(Expdir) >= 0)
			break;
		doerror(cw, $E{btq unq not a directory});
		free(Expdir);
	}

	if  (whichopt < 2)  {
		char	*Xfname, *Jfname, *arg0;
		struct	stat	sbuf;
		char	**ap, *arglist[7];
		char	jobnobuf[HOSTNSIZE+12];

		Exist = "";
		for  (;;)  {
			if  (!(Xfname = wgets(cw, 3+BOXWID, &ud_xf, Exist)))
				goto  aborted;
			disp_str = Xfname;
			if  (strchr(Xfname, '/'))  {
				doerror(cw, $E{btq unq not a file name});
				continue;
			}

			if  (stat(Xfname, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) == S_IFREG)
				break;
			doerror(cw, $E{btq unq not a regular file});
			Exist = Xfname;
		}

		Xfname = stracpy(Xfname);
		Exist = "";
		for  (;;)  {
			if  (!(Jfname = wgets(cw, 4+BOXWID, &ud_jf, Exist)))  {
				free(Xfname);
				goto  aborted;
			}
			disp_str = Jfname;
			if  (strchr(Jfname, '/'))  {
				doerror(cw, $E{btq unq not a file name});
				continue;
			}

			if  (stat(Jfname, &sbuf) < 0  || (sbuf.st_mode & S_IFMT) == S_IFREG)
				break;
			doerror(cw, $E{btq unq not a regular file});
			Exist = Jfname;
		}

		/* Ok now do the business */

#ifdef	CURSES_MEGA_BUG
		refresh();
#endif
		delwin(cw);
		Ignored_error = chdir(spdir);

		if  ((pid = fork()))  {
			int	status;

			free(Expdir);
			free(Xfname);

			if  (pid < 0)  {
				doerror(jscr, $E{btq unq cannot fork});
				return  -1;
			}
#ifdef	HAVE_WAITPID
			while  (waitpid(pid, &status, 0) < 0)
				;
#else
			while  (wait(&status) != pid)
				;
#endif
			if  (status == 0)	/* All ok */
				return  1;
			if  (status & 0xff)  {
				doerror(jscr, $E{btq unq program fault});
				return  -1;
			}
			status = (status >> 8) & 0xff;
			disp_arg[0] = jp->h.bj_job;
			disp_str = title;
			switch  (status)  {
			default:
				disp_arg[1] = status;
				doerror(jscr, $E{btq unq misc error});
				return  -1;
			case  E_JDFNFND:
				doerror(jscr, $E{btq unq file not found});
				return  -1;
			case  E_JDNOCHDIR:
				doerror(jscr, $E{btq unq dir not found});
				return  -1;
			case  E_JDFNOCR:
				doerror(jscr, $E{btq unq cannot create directory});
				return  -1;
			case  E_JDJNFND:
				doerror(jscr, $E{btq unq unknown job});
				return  -1;
			case  E_CANTDEL:
				doerror(jscr, $E{btq unq cannot delete});
				return  -1;
			}
		}

		setuid(Realuid);
		Ignored_error = chdir(Curr_pwd);	/* So that it picks up config file correctly */
		if  (jp->h.bj_hostid)
			sprintf(jobnobuf, "%s:%ld", look_host(jp->h.bj_hostid), (long) jp->h.bj_job);
		else
			sprintf(jobnobuf, "%ld", (long) jp->h.bj_job);
		if  (!(arg0 = strrchr(udprog, '/')))
			arg0 = udprog;
		else
			arg0++;
		ap = arglist;
		*ap++ = arg0;
		if  (whichopt > 0)		/* No delete */
			*ap++ = "-n";
		*ap++ = jobnobuf;
		*ap++ = Expdir;
		*ap++ = Xfname;
		*ap++ = Jfname;
		*ap = (char *) 0;
		execv(udprog, arglist);
		exit(E_SETUP);
	}
	else  {
		int	isverb, iscanc;
		char	*arg0, **ap, *arglist[5], abuf[3];
		char	jobnobuf[HOSTNSIZE+12];

		if  (whichopt == 2)		/* Don't need home directory any more */
			free(hd);
		mvwaddstr(cw, 3+BOXWID, BOXWID, udcverb);
		wrefresh(cw);
		isverb = rdalts(cw, 3+BOXWID, maxp+BOXWID+1, verbopts, -1, $H{btq unq verbose});
		if  (isverb < 0)
			goto  aborted;
		mvwaddstr(cw, 3+BOXWID, maxp+BOXWID+1, disp_alt(isverb, verbopts));
		mvwaddstr(cw, 4+BOXWID, BOXWID, udccanc);
		wrefresh(cw);
		iscanc = rdalts(cw, 4+BOXWID, maxp+BOXWID+1, cancopts, jp->h.bj_progress != BJP_NONE? 1: 0, $H{btq unq jobstate});
		if  (iscanc < 0)
			goto  aborted;
		mvwaddstr(cw, 4+BOXWID, maxp+BOXWID+1, disp_alt(iscanc, cancopts));

		/* Ok now do the business */

#ifdef	CURSES_MEGA_BUG
		refresh();
#endif
		delwin(cw);
		Ignored_error = chdir(spdir);

		if  ((pid = fork()))  {
			int	status;

			free(Expdir);

			if  (pid < 0)  {
				doerror(jscr, $E{btq unq cannot fork});
				return  -1;
			}
#ifdef	HAVE_WAITPID
			while  (waitpid(pid, &status, 0) < 0)
				;
#else
			while  (wait(&status) != pid)
				;
#endif
			if  (status == 0)	/* All ok */
				return  1;
			if  (status & 0xff)  {
				doerror(jscr, $E{btq unq program fault});
				return  -1;
			}
			status = (status >> 8) & 0xff;
			disp_arg[0] = jp->h.bj_job;
			disp_str = title;
			switch  (status)  {
			default:
				disp_arg[1] = status;
				doerror(jscr, $E{btq unq misc error});
				return  -1;
			case  E_JDNOCHDIR:
				doerror(jscr, $E{btq unq dir not found});
				return  -1;
			case  E_JDJNFND:
				doerror(jscr, $E{btq unq unknown job});
				return  -1;
			case  E_CANTSAVEO:
				doerror(jscr, $E{btq unq cannot save options file});
				return  -1;
			}
		}

		setuid(Realuid);
		Ignored_error = chdir(Curr_pwd);	/* So that it picks up config file correctly */
		if  (jp->h.bj_hostid)
			sprintf(jobnobuf, "%s:%ld", look_host(jp->h.bj_hostid), (long) jp->h.bj_job);
		else
			sprintf(jobnobuf, "%ld", (long) jp->h.bj_job);
		if  (!(arg0 = strrchr(udprog, '/')))
			arg0 = udprog;
		else
			arg0++;
		ap = arglist;
		abuf[0] = '-';
		abuf[1] = iscanc? 'C': 'N';
		if  (!isverb)
			abuf[1] = tolower(abuf[1]);
		abuf[2] = '\0';
		*ap++ = arg0;
		*ap++ = abuf;
		*ap++ = Expdir;
		*ap++ = jobnobuf;
		*ap = (char *) 0;
		execv(udprog, arglist);
		exit(E_SETUP);
	}

 aborted:
	free(Expdir);
#ifdef	CURSES_MEGA_BUG
	refresh();
#endif
	delwin(cw);
	Ignored_error = chdir(spdir);
	return  -1;
}
