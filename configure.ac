dnl   Copyright 2009 Free Software Foundation, Inc.
dnl
dnl  This program is free software: you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation, either version 3 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License
dnl  along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl  Process this file with autoconf to produce a configure script.

AC_INIT(src/btsched.c, 1.9)
AC_SUBST(MAJOR_PACKAGE_VERSION)

changequote(<<, >>)dnl
MAJOR_PACKAGE_VERSION=`expr $PACKAGE_VERSION : '\([0-9]*\)\.'`
changequote([, ])dnl

AC_MSG_RESULT(Configuring for Release $PACKAGE_VERSION)

define([directarg],
[AC_ARG_WITH($1,
[--with-$1=/new/dir	Default location of $2],
[$3="$withval"],
[$3=$4])

AC_SUBST($3)
case "$$3" in
[/*)]	: ;;
[*)]	$3=$4
	;;
esac
])

AC_ARG_VAR(BATCHUSER, [name of user files are owned by (gnubatch)])
directarg(spool-directory,[spool directory (can be large)],[spooldir],[$localstatedir/gnubatch])
directarg(internal-programs,[internal programs],[pkgexecdir],[$libexecdir/gnubatch])
directarg(pkgdatadir,[data files other than help],[pkgdatadir],[$datadir/gnubatch])
directarg(helpfiles,[help files],[sphelpdir],[$pkgdatadir/help])
directarg(docs-directory,[directory for documents],[docdir],[$pkgdatadir/doc])
directarg(pdfs-directory,[directory for pdf docs],[pdfdir],[$pkgdatadir/pdf])

if test "$BATCHUSER" = ""
then BATCHUSER=gnubatch
fi

AC_SUBST(netsubdirs)
AC_ARG_WITH(network,[--without-network	Exclude network code],
[ac_network="$withval"],
[ac_network=y])

case "$ac_network" in
[[nN]]*)	ac_network=n;;
*)	ac_network=y
	AC_DEFINE(NETWORK_VERSION)
	netsubdirs="apilibd remcgid"
	;;
esac

AC_ARG_ENABLE(nohonsuid,[--enable-nohnonsuid	Note kernel does not honour suid as super-user],
[ac_nhonsuid="$enableval"],
[ac_nhonsuid=n])

case "$ac_nhonsuid" in
[[yY]]*)	AC_DEFINE(NHONSUID)
	;;
*)	: ;;
esac

AC_ARG_ENABLE(run-as-root,[--enable-run-as-root	Scheduler must run as root (SysIII-style kill)],
[ac_run_as_root="$enableval"],
[ac_run_as_root=n])

case "$ac_run_as_root" in
[[yY]]*)	AC_DEFINE(RUN_AS_ROOT)
	;;
*)	: ;;
esac

AC_ARG_ENABLE(sco-security,[--enable-sco-secuirty	Turn on broken C2-security],
[ac_sco_security="$enableval"],
[ac_sco_security=n])

case "$ac_sco_security" in
[[yY]]*)	AC_DEFINE(SCO_SECURITY)
	;;
*)	: ;;
esac

AC_ARG_ENABLE(using-mmap,[--enable-using-mmap	Use memory-mapped files rather than shared memory],
[ac_using_mmap="$enableval"],
[ac_using_mmap=n])

case "$ac_using_mmap" in
[[yY]]*)	AC_DEFINE(USING_MMAP)
	;;
*)	: ;;
esac

AC_ARG_ENABLE(using-flock,[--enable-using-flock Use file locking rather than semaphores],
[ac_using_flock="$enableval"],
[ac_using_flock=y])

case "$ac_using_flock" in
[[yY]]*)	AC_DEFINE(USING_FLOCK)
	;;
*)	: ;;
esac

dnl Checks on host type
AC_CANONICAL_SYSTEM

dnl Checks for programs.
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
LT_INIT
AC_PATH_PROG(mailer, mail)
AC_PATH_PROG(shell, sh)
AC_PATH_PROG(perlbin, perl)
AC_EGREP_CPP(yes,
[#ifdef unix
 yes
#endif
],[],AC_DEFINE(unix))

AC_SUBST(funny_compiler_options)
AC_SUBST(xmlinclude)
AC_SUBST(xmllib)
AC_SUBST(gcc_useful_options)
funny_compiler_options=''
gcc_useful_options=''

case "$target" in
*linux*)
	AC_DEFINE(SOCKLEN_T, socklen_t)
	AC_DEFINE(OS_LINUX)
	AC_DEFINE(BUGGY_DELCURTERM)
	;;
*ia64-*hpux*)
	AC_DEFINE(SOCKLEN_T, socklen_t)
	AC_DEFINE(OS_HPUX)
	AC_DEFINE(OS_HPUX_IA64)
	;;
*hp*11*)
	AC_DEFINE(SOCKLEN_T, socklen_t)
	AC_DEFINE(OS_HPUX)
	if test "$GCC" != "yes"
	then	funny_compiler_options='+DAportable +DS1.0'
	fi
	;;
*hp*10*)
	AC_DEFINE(SOCKLEN_T, int)
	funny_compiler_options='+DAportable +DS1.0'
	;;
*solaris*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_SOLARIS)
	;;
*dpx*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_DPX_2)
	AC_DEFINE(BUGGY_DELCURTERM)
	;;
*aix5*)
	AC_DEFINE(OS_AIX)
	AC_DEFINE(OS_AIX_5_0)
	AC_DEFINE(BROKEN_TERM_READ)
	AC_DEFINE(SOCKLEN_T, socklen_t)
	CC=cc
	ac_cv_prog_CC=cc
	CFLAGS=-O
	funny_compiler_options='-qmaxmem=8192'
	LEX=lex
	LEXLIB=-ll
	export CC CFLAGS LEX LEXLIB
	;;
*aix4.3*)
	AC_DEFINE(OS_AIX)
	AC_DEFINE(OS_AIX_4_3)
	AC_DEFINE(BROKEN_TERM_READ)
	AC_DEFINE(SOCKLEN_T, socklen_t)
	CC=cc
	ac_cv_prog_CC=cc
	CFLAGS=-O
	funny_compiler_options='-qmaxmem=8192'
	LEX=lex
	LEXLIB=-ll
	export CC CFLAGS LEX LEXLIB
	;;
*aix4.2*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_AIX)
	AC_DEFINE(BROKEN_TERM_READ)
	CC=cc
	ac_cv_prog_CC=cc
	CFLAGS=-O
	LEX=lex
	LEXLIB=-ll
	export CC CFLAGS LEX LEXLIB
	;;
*aix*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_AIX)
	CC=cc
	ac_cv_prog_CC=cc
	CFLAGS=-O
	LEX=lex
	LEXLIB=-ll
	export CC CFLAGS LEX LEXLIB
	;;
*freebsd*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_FREEBSD)
	AC_DEFINE(BUGGY_DELCURTERM)
	;;
*dgux*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_DGUX)
	;;
*ptx*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_PTX)
	AC_DEFINE(BUGGY_SIGCLD)
	;;
*dynix*)
	AC_DEFINE(SOCKLEN_T, int)
	AC_DEFINE(OS_DYNIX)
	AC_DEFINE(BUGGY_SIGCLD)
	;;
*)
	AC_DEFINE(SOCKLEN_T, int)
	;;
esac

# If (still) using GCC turn on -g -Wall and -fno-stack-protector

if test "$GCC" = "yes"
then
	gcc_useful_options='-g -Wall -fno-stack-protector'
fi

# Check fairly early for socket libraries because we need these for the X stuff
# First look for socket in the standard C library.
# If that fails look in a -linet library if that exists.
# Otherwise look in -lsocket and -lnsl

AC_SUBST(SOCKLIBS)
AC_CHECK_FUNC(socket,
  [SOCKLIBS=]
  AC_MSG_RESULT(Socket stuff apparently in C library),
  AC_MSG_CHECKING(location of socket libraries)
  AC_CHECK_LIB(c_s,open)
  AC_CHECK_LIB(inet,socket,[SOCKLIBS=-linet])
  AC_CHECK_LIB(nsl_s,t_open,[SOCKLIBS="$SOCKLIBS -lnsl_s"])
  AC_CHECK_LIB(nsl,t_open,[SOCKLIBS="$SOCKLIBS -lnsl"])
  AC_CHECK_LIB(socket,socket,[SOCKLIBS="-lsocket $SOCKLIBS"],[],$SOCKLIBS))

dnl Checks for header files.
AC_SYS_LONG_FILE_NAMES
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h malloc.h stdlib.h termio.h termios.h ulimit.h unistd.h utime.h wait.h)
AC_CHECK_HEADERS(sys/ioctl.h sys/select.h sys/time.h sys/utime.h sys/mman.h sys/resource.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(unsigned)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(int *)
AC_TYPE_OFF_T
AC_TYPE_UID_T
AC_TYPE_GETGROUPS
AC_HEADER_TIME
AC_STRUCT_TIMEZONE
AC_TYPE_PID_T
AC_MSG_CHECKING(pid_t isnt unsigned)
AC_TRY_RUN([#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
main()
{
	pid_t x = 0;
	x--;
	if (x>0) exit(1);
	exit(0);
}
],AC_DEFINE(PIDTYPE,pid_t)
AC_MSG_RESULT(No it isnt),
AC_DEFINE(PIDTYPE,int)
AC_MSG_RESULT(Made it an int in the end))
AC_MSG_CHECKING(getgroups args are the same size as gid_t)
AC_TRY_RUN([#include <sys/types.h>
main()
{
	if (sizeof(GETGROUPS_T) != sizeof(gid_t)) exit(1);
	exit(0);
}
],
AC_MSG_RESULT(they are)
AC_DEFINE(GETGROUPS_SAME_SIZE),
AC_MSG_RESULT(they are not))

dnl Check for sprintf returning char* rather than int
AC_MSG_CHECKING(return type of sprintf)
AC_TRY_RUN(
changequote(<<, >>)dnl
<<#include <stdio.h>
main() {
	char buf[20];
	exit(sprintf(buf, "%d", 123) > 4? 1: 0);
}
>>
changequote([, ]),
AC_MSG_RESULT(Returns int),
AC_MSG_RESULT(Returns char*)
AC_DEFINE(CHARSPRINTF))

AC_MSG_CHECKING(sys_errlist defined in stdio.h)
AC_TRY_COMPILE([#include <stdio.h>],
[char **x = sys_errlist; y(x);],
AC_MSG_RESULT(it is)
AC_DEFINE(SYS_ERRLIST_IN_STDIO_H),
AC_MSG_RESULT(not defined))

dnl Check for these specific funcs because X libraries get them wrong
dnl on some systems (e.g. SysV4) by invoking bcopies we don't have.
AC_CHECK_FUNCS(memcpy bcopy strchr)

dnl Look for X stuff
AC_PATH_X
AC_SUBST(XWIN_LIBS)
ac_save="$LDFLAGS"
if test "$x_libraries" != ""
then LDFLAGS="-L$x_libraries $LDFLAGS"
fi
if test "$no_x" != yes
then
	XWIN_LIBS="-lXt -lX11"
	AC_CHECK_LIB(Xext, XextCreateExtension, [XWIN_LIBS="-lXext $XWIN_LIBS"],[],[$XWIN_LIBS $SOCKLIBS])
	AC_CHECK_LIB(XIM, XDefaultString, [XWIN_LIBS="-lXIM $XWIN_LIBS"],[],[$XWIN_LIBS $SOCKLIBS])
	AC_CHECK_LIB(gen, regex, [XWIN_LIBS="$XWIN_LIBS -lgen"])
	AC_CHECK_LIB(Xm, XmCreateMenuBar, [XWIN_LIBS="-lXm $XWIN_LIBS"],
		AC_CHECK_LIB(Xm, XmCreatePopupMenu, [XWIN_LIBS="-lXm -lXp $XWIN_LIBS"],
		AC_MSG_RESULT(No motif library found - not building X)
		[no_x='yes'
		XWIN_LIBS=''],
		[-lXp $XWIN_LIBS $SOCKLIBS]),
	[$XWIN_LIBS $SOCKLIBS])
	if test "$x_includes" != ""
	then
		SAVE_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -I$x_includes"
	fi
	AC_TRY_CPP([#include <Xm/Xm.h>],,
	AC_MSG_RESULT(No motif header found - not building X)
	[no_x='yes'
	XWIN_LIBS=''])
	if test "$x_includes" != ""
	then
		CPPFLAGS="$SAVE_CPPFLAGS"
	fi
	AC_CHECK_LIB(Xm, XmRenditionCreate, [AC_DEFINE(HAVE_XMRENDITION)],[$XWIN_LIBS $SOCKLIBS])
	AC_CHECK_HEADERS(Xm/SpinB.h Xm/ComboBox.h)
	AC_MSG_CHECKING(Checking to see if it is LessTif)
	if grep -i lesstif /usr/include/Xm/Xm.h >/dev/null 2>&1
	then
		AC_MSG_RESULT(Yes it is)
		AC_DEFINE(BROKEN_COMBOBOX)
		AC_DEFINE(BROKEN_RENDITION)
		AC_DEFINE(BROKEN_SPINBOX)
	else
		AC_MSG_RESULT(Apparently not)
	fi
	if test "$x_libraries" != ""
	then	XWIN_LIBS="-L$x_libraries $XWIN_LIBS"
	fi
fi
LDFLAGS="$ac_save"

dnl Fool around with curses libs and terminfo
AC_SUBST(LIBCURSES)
if test "$LIBCURSES" = ""
then
	AC_CHECK_LIB(curses, wrefresh, [LIBCURSES=-lcurses],
	[AC_CHECK_LIB(ncurses, wrefresh, [LIBCURSES=-lncurses],
	[AC_CHECK_LIB(curses, wrefresh, [LIBCURSES="-lcurses -ltermcap"],, [-ltermcap])])])
	AC_TRY_COMPILE([#include <curses.h>],[struct termio x;return sizeof(x);],,AC_DEFINE(NO_TERMIO_IN_CURSES))
	AC_CHECK_LIB(c, doupdate,
	AC_DEFINE(HAVE_TERMINFO)
	AC_CHECK_LIB(c, tigetstr, AC_DEFINE(HAVE_TIGETSTR),, "$LIBCURSES"),, [$LIBCURSES])
fi

dnl Check for curses overlap bug - I think that this finds it.
AC_MSG_CHECKING(curses overlap bug)
ac_save_libs="$LIBS"
LIBS="$LIBCURSES $LIBS"
AC_TRY_RUN([#include <curses.h>
main()
{
	int	ch, r, c;
	r = open("/dev/null", 1);
	dup2(r, 1);
	initscr();
	for  (r = 0;  r < LINES;  r++)
		for  (c = 0;  c < COLS;  c++)
			mvaddch(r, c, 'A');
	refresh();
	ch = mvwinch(curscr, LINES-2, 0);
	clear();
	refresh();
	endwin();
	exit(ch == 'A'? 0: 1);
}
],AC_MSG_RESULT(I think it is OK),
AC_DEFINE(CURSES_OVERLAP_BUG)
AC_MSG_RESULT(I think it fails))
LIBS="$ac_save_libs"

dnl Check for libxml2
xmlinclude=''
xmllib=''
AC_CHECK_HEADER(libxml2/libxml/xmlexports.h,
AC_DEFINE(HAVE_LIBXML2)
xmlinclude='-I/usr/include/libxml2'
xmllib='-lxml2')

dnl Find maximum shm size and convert for size of jobs/vars

AC_MSG_CHECKING(maximum shared memory size)
AC_TRY_RUN([#include <stdio.h>
#include <sys/types.h>
#ifdef	OS_HPUX_IA64
#undef _INCLUDE_HPUX_SOURCE
#endif
#include <sys/ipc.h>
#include <sys/shm.h>

int	sizeok(n)
int	n;
{
	int	kk= shmget(IPC_PRIVATE, n, 0666);
	if  (kk < 0)
		return  0;
	(void) shmctl(kk, IPC_RMID, 0);
	return  1;
}

main()
{
	int	mins = 4096, maxs = 0x8000000, middle;
	FILE	*f = fopen("conftestshm", "w");

	if  (sizeok(maxs))  {
		(void) fprintf(f, "%d\n", maxs);
		exit(0);
	}
	if  (!sizeok(mins))  {
		(void) fprintf(f, "imposs\n");
		exit(1);
	}
	while  (mins < maxs)  {
		middle = (mins + maxs) / 2;
		if  (sizeok(middle))  {
			if  (mins == middle)
				break;
			mins = middle;
		}
		else  {
			if  (maxs == middle)
				break;
			maxs = middle;
		}
	}
	(void) fprintf(f, "%d\n", mins);
	exit(0);
}
],
[ac_shmsize=`cat conftestshm`]
AC_MSG_RESULT($ac_shmsize),
AC_MSG_ERROR(No joy - maybe no shm?))

AC_MSG_CHECKING(maximum number of messages on message queue)
AC_TRY_RUN(
changequote(<<, >>)dnl
<<#include <stdio.h>
#include <sys/types.h>
#ifdef	OS_HPUX_IA64
#undef _INCLUDE_HPUX_SOURCE
#endif
#include <sys/ipc.h>
#include <sys/msg.h>

#ifndef	MSGSIZE
#define	MSGSIZE	40	/* Size is a coincidence */
#endif

struct	x	{
	long	a;
	char	stuff[MSGSIZE];
}  y;

main(argc, argv) int argc; char **argv;
{
	int	nmsgs = 40, cnt;
	int	msg_chan;
	FILE	*f = fopen("conftestmsg", "w");
	if  (argc > 1)
		nmsgs = atoi(argv[1]);
	if  ((msg_chan = msgget(0x12345678, 0666|IPC_CREAT)) < 0)
		exit(2);
	y.a = 999;
	for  (cnt = 0;  cnt < MSGSIZE;  cnt++)
		y.stuff[cnt] = 'T';
	cnt = 0;
	while  (msgsnd(msg_chan, (void *) &y, MSGSIZE, IPC_NOWAIT) >= 0)
		cnt++;
	msgctl(msg_chan, IPC_RMID, (struct msqid_ds *) 0);
	fprintf(f, "%d\n", cnt);
	fclose(f);
	return  0;
}
>>
changequote([, ]),
[ac_msgqsize=`cat conftestmsg`]
AC_MSG_RESULT($ac_msgqsize),
AC_MSG_ERROR(Trouble with message queues - giving up))

AC_MSG_CHECKING(maximum number of semaphores per id)
AC_TRY_RUN(
[#include <stdio.h>
#include <sys/types.h>
#ifdef	OS_HPUX_IA64
#undef _INCLUDE_HPUX_SOURCE
#endif
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

union	my_semun	{
	int			val;
	struct	semid_ds	*buf;
	ushort			*array;
};

int	main()
{
	int	top = 65536, bottom = 1, middle, sres;
	union	my_semun	uun;
	FILE	*f = fopen("conftestsem", "w");

	while  (top > bottom+1)  {
		middle = (top + bottom) / 2;
		if  ((sres = semget(0x12344321, middle, 0666|IPC_CREAT)) < 0)
			top = middle;
		else  {
			bottom = middle;
			uun.val = 0;
			semctl(sres, 0, IPC_RMID, uun);
		}
	}

	fprintf(f, "%d\n", bottom);
	return  0;
}
],
[ac_numsema4=`cat conftestsem`]
[AC_MSG_RESULT($ac_numsema4)
if test $ac_numsema4 -lt 10; then
AC_MSG_ERROR($ac_numsema4 is too few - giving up)
elif test $ac_numsema4 -gt 200; then
AC_MSG_RESULT(Cutting that back to 200)
ac_numsema4=200
fi
],
AC_MSG_ERROR(Trouble with semaphores - giving up))

AC_DEFINE_UNQUOTED(NUMSEMA4, $ac_numsema4)
AC_DEFINE_UNQUOTED(NUMXBUFS, $ac_msgqsize)

dnl These numbers probably want mangling if we change

AC_CHECK_PROG(ENV_PROG, [printenv], [printenv], [])
AC_CHECK_PROG(ENV_PROG, [env], [env], [])

if test "$ENV_PROG" != ""
then
  ac_envsize=`$ENV_PROG|wc -c`
  if test $ac_envsize -gt 2500
  then	AC_MSG_WARN(Environment is $ac_envsize bytes)
  else	ac_envsize=2500
  fi
else  ac_envsize=2500
fi

AC_DEFINE_UNQUOTED(ENVSIZE, $ac_envsize)
ac_jobsize=`expr 2000 + $ac_envsize`
ac_initjalloc=`expr $ac_shmsize - 1250`
ac_initjalloc=`expr $ac_shmsize / $ac_jobsize`
if test $ac_initjalloc -gt 20000
then ac_initjalloc=20000
elif test $ac_initjalloc -gt 10000
then ac_initjalloc=10000
elif test $ac_initjalloc -gt 5000
then ac_initjalloc=5000
elif test $ac_initjalloc -gt 2000
then ac_initjalloc=2000
elif test $ac_initjalloc -gt 1000
then ac_initjalloc=1000
else
	ac_initjalloc=`expr $ac_initjalloc / 50`
	ac_initjalloc=`expr $ac_initjalloc \* 50`
fi
AC_MSG_RESULT(Allocated initial $ac_initjalloc jobs)
AC_DEFINE_UNQUOTED(INITJALLOC, $ac_initjalloc)

AC_CHECK_FUNCS(atexit fchmod fchown fgetc ftruncate getgroups seteuid setreuid lstat setrlimit)

if test "$ac_no_id_swap" = "y"
then	AC_MSG_RESULT(I believe there is no id swap on this machine)
else	AC_DEFINE(ID_SWAP)
fi

AC_SUBST(CRYPTLIB)
AC_CHECK_FUNC(crypt,
  [CRYPTLIB=],
  AC_CHECK_LIB(crypt, crypt, [CRYPTLIB=-lcrypt], AC_MSG_WARN(Problems with crypt)))
AC_SUBST(LIBSHAD)
AC_CHECK_FUNC(getspent, AC_DEFINE(SHADOW_PW),
AC_CHECK_LIB(sec,getspent, AC_DEFINE(SHADOW_PW) [LIBSHAD=-lsec]))

dnl Checks for library functions.
AC_FUNC_GETPGRP
AC_FUNC_SETPGRP
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS(sigaction sigvector sigvec sigset waitpid)

if test "$ac_cv_func_sigaction" = "yes"; then
AC_TRY_COMPILE([#include <signal.h>],
[struct sigaction z;z.sa_sigaction = 0;],AC_MSG_RESULT(struct sigaction has sa_sigaction member),
AC_MSG_RESULT(no sa_sigaction member in struct sigaction)
AC_DEFINE(NO_SA_SIGACTION))
elif test "$ac_cv_func_sigset" = "yes"; then
AC_MSG_CHECKING(sigset clears held signals in forked-off processes)
AC_TRY_RUN([#include <signal.h>
int	ping = 0;
RETSIGTYPE	catch()
{
	ping++;
}
main()
{
	int	st;
	sigset(SIGUSR1, catch);
	sighold(SIGUSR1);
	kill(getpid(), SIGUSR1);
	if  (fork() == 0)  {
		sigrelse(SIGUSR1);
		sleep(1);
		exit(ping? 255: 0);
	}
	while  (wait(&st) < 0)
		;
	exit(st? 1: 0);
}
],
AC_MSG_RESULT(it does),
AC_MSG_RESULT(IT DOESNT)
AC_DEFINE(BUGGY_SIGSET_HOLDS))
fi

define([dir_includes],[
#include <sys/types.h>
#ifdef	HAVE_LONG_FILE_NAMES
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif
#else
#include <sys/dir.h>
#define dirfd(x) 0
#endif
])

AC_MSG_CHECKING(Can find f.d. of directory)
AC_TRY_LINK(dir_includes,
[DIR *foo;int x = dirfd(foo);return 0;],
AC_MSG_RESULT(dirfd seems OK)
AC_DEFINE(HAVE_DIRFD),
AC_MSG_CHECKING(No dirfd, checking for dd_fd in DIR structure)
AC_TRY_COMPILE([dir_includes],
[DIR *foo;int x = foo->dd_fd;return 0;],
AC_MSG_RESULT(That seems to work)
AC_DEFINE(HAVE_DD_FD),
AC_MSG_RESULT(No directory fd macro or structure element found)))

AC_CONFIG_HEADER(config.h)
AC_CONFIG_COMMANDS([src/hdrs/files.h],
[./Makefiles_h.sh prefix=$prefix exec_prefix=$exec_prefix datarootdir=$datarootdir
./Makegnubatch.conf.sh prefix=$prefix exec_prefix=$exec_prefix datarootdir=$datarootdir
], [prefix=$prefix exec_prefix=$exec_prefix datarootdir=$datarootdir])

AC_OUTPUT(Makefile ingnubatch.conf util/Makefile src/Makefile src/helpmsg/Makefile src/hdrs/infiles.h src/lib/Makefile src/ins/Makefile src/gtk/Makefile
src/motif/Makefile src/apilib/Makefile src/loccgi/Makefile src/remcgi/Makefile doc/Makefile Netinst Checkuserinst Userinst STARTUP/Startdirs)

AH_TEMPLATE([BROKEN_COMBOBOX], [Define if Motif combo boxes don't work])
AH_TEMPLATE([BROKEN_RENDITION], [Define if Motif renditions don't work])
AH_TEMPLATE([BROKEN_SPINBOX], [Define if Motif spin boxes don't work properly])
AH_TEMPLATE([BROKEN_TERM_READ], [Define if read from a raw terminal can return zero bytes])
AH_TEMPLATE([BUGGY_DELCURTERM], [Define if "delcurterm" is unreliable and causes core dumps])
AH_TEMPLATE([BUGGY_SIGCLD], [Define if signal(SIGCLD, SIG_IGN) cannot be relied upon])
AH_TEMPLATE([BUGGY_SIGSET_HOLDS], [Define if sigset routines do not delete held pending signals in child processes])
AH_TEMPLATE([CHARSPRINTF], [Define if "sprintf" returns a char* rather than an int])
AH_TEMPLATE([CURSES_OVERLAP_BUG], [Define if curses messes-up overlapping subwindows])
AH_TEMPLATE([ENVSIZE], [/Set to number of bytes in environment])
AH_TEMPLATE([GETGROUPS_SAME_SIZE], [/Set if arg to getgroups is same as gid_t])
AH_TEMPLATE([HAVE_DIRFD], [Define if directory-reading stuff has dirfd funct/macro])
AH_TEMPLATE([HAVE_DD_FD], [Define if directory-reading stuff has dd_fd in dir structure])
AH_TEMPLATE([HAVE_LIBXML2], [Define if XML library libxml2 is available])
AH_TEMPLATE([HAVE_SIGSET], [Define if you have the sigset function.])
AH_TEMPLATE([HAVE_TERMINFO], [Define if we are using the terminfo library not termcap])
AH_TEMPLATE([HAVE_TIGETSTR], [Define if terminfo supports the TIGETSTR call])
AH_TEMPLATE([HAVE_XMRENDITION], [Define if Motif version has renditions in])
AH_TEMPLATE([ID_SWAP], [Define if kernel permits setuid to shuffle between uid and euid])
AH_TEMPLATE([INITJALLOC], [Define to set initial job allocation])
AH_TEMPLATE([INITVALLOC], [Define to set initial variable allocation])
AH_TEMPLATE([MOTIF_VN2], [Define if motif vn 2])
AH_TEMPLATE([NETWORK_VERSION], [Define to compile for network version])
AH_TEMPLATE([NHONSUID], [Define if kernel doesn't honour suid bit for root])
AH_TEMPLATE([NO_DTEMPLATE], [Define if DialogTemplates aren't defined in motif libraries])
AH_TEMPLATE([NO_SA_SIGACTION], [Define if sigaction structure doesn't have sa_sigaction in])
AH_TEMPLATE([NO_TERMIO_IN_CURSES], [Define if curses.h doesn't include termio.h])
AH_TEMPLATE([NUMSEMA4], [Define to maximum number of semaphores at once])
AH_TEMPLATE([NUMXBUFS], [Define to size of message queue])
AH_TEMPLATE([OS_AIX], [Define if on AIX])
AH_TEMPLATE([OS_AIX_4_3], [Define if on AIX 4.3])
AH_TEMPLATE([OS_AIX_5_0], [Define if on AIX 5.0])
AH_TEMPLATE([OS_BSDI], [Define if on BSDI])
AH_TEMPLATE([OS_DGUX], [Define if on DGUX])
AH_TEMPLATE([OS_DPX_2], [Define if on DPX_2])
AH_TEMPLATE([OS_DYNIX], [Define if on Dynix (not ptx)])
AH_TEMPLATE([OS_FREEBSD], [Define if on FreeBSD])
AH_TEMPLATE([OS_HPUX], [Define if on HPUX])
AH_TEMPLATE([OS_HPUX_IA64], [Define if on HPUX Itanium])
AH_TEMPLATE([OS_LINUX], [Define if on GNU/Linux])
AH_TEMPLATE([OS_OSF1], [Define if on OSF1])
AH_TEMPLATE([OS_PTX], [Define if on PTX])
AH_TEMPLATE([OS_SOLARIS], [Define if on Solaris])
AH_TEMPLATE([OS_ULTRIX], [Define if on Ultrix])
AH_TEMPLATE([PIDTYPE], [Define type for process ids])
AH_TEMPLATE([RUN_AS_ROOT], [Indicate that scheduler must run as root - funny "kill" sematics])
AH_TEMPLATE([SCO_SECURITY], [Define to turn on SCO-style C2 security stuff])
AH_TEMPLATE([SHADOW_PW], [Indicate that we are using shadow password files])
AH_TEMPLATE([SOCKLEN_T], [Define type for final arg of accept etc thanq HP])
AH_TEMPLATE([SYS_ERRLIST_IN_STDIO_H], [Define if stdio.h defines sys_errlist])
AH_TEMPLATE([USING_FLOCK], [Define to use file locking instead of semaphores])
AH_TEMPLATE([USING_MMAP], [Define to use Memory-mapped files rather than shared memory])
AH_TEMPLATE([unix], [Define this if the C preprocessor doesn't])

AH_BOTTOM([#ifdef OS_HPUX
#ifdef	OS_HPUX_IA64
#ifndef	CID_T_DEFINED
#define	CID_T_DEFINED 1
typedef	int	cid_t;
#endif
#endif
#define _XOPEN_SOURCE_EXTENDED 1
#include <stdarg.h>
#endif

#if	SIZEOF_LONG != 4
#if	SIZEOF_INT != 4
error: no basic signed 4-byte
#else
#define	LONG	int
#endif
#else
#define	LONG	long
#endif

#if	SIZEOF_UNSIGNED_LONG != 4
#if	SIZEOF_UNSIGNED != 4
error: no basic unsigned 4-byte
#else
#define	ULONG	unsigned
#endif
#else
#define	ULONG	unsigned long
#endif

#if	SIZEOF_SHORT != 2 || SIZEOF_UNSIGNED_SHORT != 2
error: no basic 2-byte
#else
#define	SHORT	short
#define	USHORT	unsigned short
#endif

#if	SIZEOF_INT_P != SIZEOF_INT
#define	INT_TO_XTPOINTER(X)	(XtPointer)(long)(X)
#define	XTPOINTER_TO_INT(X)	(int)(long)(X)
#else
#define	INT_TO_XTPOINTER(X)	(XtPointer)(X)
#define	XTPOINTER_TO_INT(X)	(int)(X)
#endif

#define	ROOTID	0
#ifdef	__GNUC__
#define	NORETURN_FUNC	__attribute__ ((noreturn))
#define	MAINFN_TYPE	int
#else
#define	NORETURN_FUNC
#define	MAINFN_TYPE
#endif
])
