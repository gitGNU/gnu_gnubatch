#
# File for "helpparse" to generate message files from
#
#   Copyright 2009 Free Software Foundation, Inc.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

######################################################################
######################################################################
##  Macros for library modules

DEFINE	LIB0 (lib) :
		advtime.c  repmnthfix.c;

DEFINE	LIB1 (lib) :
		calchash.c getbtuser.c
		getxbuf.c look_host.c mpermitted.c net_feed.c
		open_ci.c packjstring.c readreply.c
		produser.c rjfile.c rvfile.c;

DEFINE	LIB2 (lib) :
		altlen.c disp_alt.c disperror.c doenv.c doopts.c galts.c
		gprompt.c help_readl.c helpargs.c helphdr.c helpnstate.c
		helpprmpt.c helprdalt.c helprdn.c helpvec.c makeovec.c makevec.c
		mmangle.c saveopts.c op_icf.c open_cfile.c optprocess.c rdoptfile.c;

DEFINE	LIB3 (lib) :
		envprocess.c ncstrncmp.c prin_gname.c prin_uname.c qmatch.c
		runpwd.c spath.c stracpy.c strread.c mkspid.c;

DEFINE  REMSUB (lib) :  remsubops.c;

DEFINE	DUMPLIB (lib) :
		dumpargs.c dumpcon.c dumpconds.c dumpecrun.c dumpemode.c dumpmode.c dumpnstr.c
		dumpredirs.c dumpstr.c dumptime.c spit_cond.c spit_msg.c spit_pparm.c spit_redir.c
		spit_time.c spitbtrstr.c chkfuture.c;

DEFINE	SCREENLIB (lib) :
		getkey.c make_km.c setkey.c wgets.c whdrstr.c whexnum.c wnum.c;

DEFINE	JFMT_INLINE (inline) :
		fmtmode.c
		jfmt_args.c	jfmt_ass.c	jfmt_assfull.c	jfmt_avoid.c
		jfmt_cond.c	jfmt_dir.c	jfmt_env.c	jfmt_exits.c
		jfmt_export.c	jfmt_fcond.c	jfmt_group.c	jfmt_interp.c
		jfmt_jobno.c	jfmt_loadlev.c	jfmt_mode.c	jfmt_orighost.c
		jfmt_pid.c	jfmt_prio.c	jfmt_progress.c	jfmt_qtit.c
		jfmt_queue.c	jfmt_redirs.c	jfmt_repeat.c	jfmt_time.c
		jfmt_timefull.c jfmt_times.c	jfmt_title.c	jfmt_ulimit.c
		jfmt_umask.c	jfmt_user.c	jfmt_xit.c ;

DEFINE	BTQ_INLINE (inline) :		jfmt_seq.c ;

DEFINE	VFMT_INLINE (inline) :
		fmtmode.c
		vfmt_comment.c	vfmt_export.c	vfmt_group.c	vfmt_mode.c
		vfmt_name.c	vfmt_user.c	vfmt_value.c ;

DEFINE	JOPTS_LIB (lib) :		optpparm.c opttime.c optvars.c;

DEFINE	DISPOPT_LIB (lib) :		optdisp.c;

DEFINE	FREEZE_INLINE (inline) :	freezecode.c ;

DEFINE	BTR_INLINE (inline) :		btradefs.c	btroptp.c	spopts_btr.c ;

DEFINE	JOBDUMP_INLINE (inline) :	btradefs.c	spopts_jd.c	;

DEFINE	LIBSHLL (lib) :			cgifndjb.c 	cgiutil.c	shutilmsg.c;

DEFINE	XMBTQ_INLINE (inline) :
		jfmt_seq.c
		vfmt_comment.c	vfmt_export.c	vfmt_group.c	vfmt_mode.c
		vfmt_name.c	vfmt_user.c	vfmt_value.c ;

DEFINE	XMBTR_INLINE (inline) :		btroptp.c	spopts_jd.c ;

DEFINE	BTFILEMON_INLINE (inline) :	btfmadefs.c	;

######################################################################
######################################################################
##  Which programs helps are used by

btint-config (helpmsg) :
	dosbtwrite btmdisp btexec btsched btwrite xbnetserv;

btrest.help (helpmsg) :
	btcharge	btcichange	btcilist	btjchange
	btjdel		btjlist		btjstat		btquit
	btr		btstart		btuchange	btulist
	btvar		btvlist		jobdump		rbtr
	bts
	btjcgi		btjvcgi		btjdcgi		btjccgi
	btjcrcgi	btvcgi		btvccgi
	rbtjcgi		rbtjccgi	rbtjdcgi	rbtjvcgi
	rbtjcrcgi	rbtvcgi		rbtvccgi	bthols;

btq.help (helpmsg) :
	btq;

btuser.help (helpmsg) :
	btuser;

xmbtq.help (helpmsg) :
	xmbtq xbtq;

xmbtr.help (helpmsg) :
	xmbtr xbtr;

xmbtuser.help  (helpmsg) :
	xmbtuser xbtuser;

filemon.help (helpmsg) :
	btfilemon	xmfilemon	xfilemon;

######################################################################
######################################################################
##  Which modules make up which programs

MODULES dosbtwrite =
	dosbtwrite.c LIB1 LIB2 LIB3 ;

MODULES btexec	=
	btexec.c LIB2 LIB3 ;

MODULES btmdisp =
	btmdisp.c LIB2 LIB3 ;

MODULES btsched	=
	btsched.c sh_jlist.c sh_vlist.c sh_cmdint.c sh_exec.c
	sh_misc.c sh_oper.c sh_log.c sh_network.c sh_netlock.c
	sh_netfeed.c sh_pack.c LIB0 LIB1 LIB2 LIB3;

MODULES btwrite =
	btwrite.c LIB2 LIB3 ;

MODULES xbnetserv =
	xbnetserv.c xbnet_ua.c xbnet_api.c LIB1 LIB2 LIB3;

MODULES btcharge =
	btcharge.c LIB1 LIB2 LIB3 FREEZE_INLINE ;

MODULES btcichange =
	btcichange.c LIB1 LIB2 LIB3 FREEZE_INLINE ;

MODULES btcilist =
	btcilist.c LIB1 LIB2 LIB3 FREEZE_INLINE ;

MODULES btjchange =
	btjchange.c rbt_access.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB JOPTS_LIB FREEZE_INLINE LIBSHLL ;

MODULES btjdel =
	btjdel.c LIB0 LIB1 LIB2 LIB3 FREEZE_INLINE LIBSHLL ;

MODULES btjlist =
	btjlist.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB JFMT_INLINE FREEZE_INLINE LIBSHLL ;

MODULES btjstat =
	btjstat.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB FREEZE_INLINE LIBSHLL ;

MODULES bthols =
	bthols.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB FREEZE_INLINE LIBSHLL ;

MODULES btquit =
	btquit.c LIB1 LIB2 LIB3 ;

MODULES btr =
	btr.c LIB0 LIB1 LIB2 LIB3 lib/chkfuture.c BTR_INLINE DISPOPT_LIB JOPTS_LIB FREEZE_INLINE ;

MODULES bts =
        bts.c LIB1 LIB2 LIB3 REMSUB; 

MODULES btstart =
	btstart.c LIB0 LIB1 LIB2 LIB3 ;

MODULES btuchange =
	btuchange.c LIB1 LIB2 LIB3 FREEZE_INLINE ;

MODULES btulist =
	btulist.c LIB1 LIB2 LIB3 FREEZE_INLINE ;

MODULES btvar =
	btvar.c LIB1 LIB2 LIB3 DISPOPT_LIB FREEZE_INLINE ;

MODULES btvlist =
	btvlist.c LIB1 LIB2 LIB3 DISPOPT_LIB VFMT_INLINE FREEZE_INLINE ;

MODULES jobdump =
	jobdump.c LIB0 LIB1 LIB2 LIB3 JOBDUMP_INLINE ;

MODULES rbtr =
	rbtr.c rbt_access.c rbt_job.c lib/chkfuture.c LIB0 LIB1 LIB2 LIB3 REMSUB BTR_INLINE JOPTS_LIB FREEZE_INLINE ;

MODULES btq =
	btq.c		bq_jlist.c	bq_jobops.c	bq_viewj.c
	bq_jci.c	bq_paredit.c	bq_vlist.c	bq_varops.c
	bq_miscscr.c	wtimes.c	bq_propts.c	bq_hols.c
	LIB0		LIB1		LIB2		LIB3		SCREENLIB
	DISPOPT_LIB	JFMT_INLINE	VFMT_INLINE	BTQ_INLINE ;

MODULES btuser =
	btuser.c btu_propts.c LIB1 LIB2 LIB3 DISPOPT_LIB SCREENLIB ;

MODULES btjcgi (loccgi) =
	btjcgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB JFMT_INLINE LIBSHLL;

MODULES btjvcgi (loccgi) =
	btjvcgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES btjdcgi (loccgi) =
	btjdcgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES btjccgi (loccgi) =
	btjccgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES btjcrcgi (loccgi) =
	btjcrcgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES btvcgi (loccgi) =
	btvcgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB VFMT_INLINE ;

MODULES btvccgi (loccgi) =
	btvccgi.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES xmbtq (motif) =
	xmbtq.c		xmbq_cbs.c	xmbq_jlist.c	xmbq_jcall.c
	xmbq_vlist.c	xmbq_vcall.c	xmbq_cmdint.c	xmbq_jobpar.c
	xmbq_view.c	xm_commlib.c	bqr_common.c
	LIB0 LIB1 LIB2 LIB3	DISPOPT_LIB	XMBTQ_INLINE	JFMT_INLINE	VFMT_INLINE ;

MODULES xmbtuser (motif) =
	xmbtuser.c xmbtu_cbs.c 	xm_commlib.c LIB1 LIB2 LIB3 DISPOPT_LIB;

MODULES xbtuser (gtk) =
	xbtuser.c xbtu_cbs.c gtk_lib.c gtk_modebits.c LIB1 LIB2 LIB3;

MODULES xbtq (gtk) =
	xbtq.c xbq_jvlist.c xbq_cbs.c xbq_jcall.c xbq_jobpar.c xbq_vcall.c xbq_cmdint.c xbq_view.c gtk_lib.c gtk_modebits.c
	LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB;

MODULES xmbtr (motif) =
	xmbtr.c xmbr_jlist.c xmbr_cbs.c xmbr_jcall.c xmbr_jobpar.c xmbr_remsub.c
	xmbr_vlist.c xmbr_files.c xmbr_jldsv.c xm_commlib.c bqr_common.c LIB0 LIB1 LIB2 LIB3 XMBTR_INLINE REMSUB DISPOPT_LIB JOPTS_LIB JOBDUMP_INLINE;

MODULES xbtr (gtk) =
	xbtr.c xbr_jlist.c xbr_cbs.c xbq_jcall.c xbq_jobpar.c xbr_remsub.c xbr_jldsv.c gtk_lib.c
	gtk_modebits.c LIB0 LIB1 LIB2 LIB3 XMBTR_INLINE REMSUB DISPOPT_LIB JOPTS_LIB JOBDUMP_INLINE;

MODULES	btfilemon =
	btfilemon.c LIB2 LIB3 BTFILEMON_INLINE;

MODULES	xmfilemon (motif) =
	xmfilemon.c LIB2 LIB3 BTFILEMON_INLINE;

MODULES	xfilemon (gtk) =
	xfilemon.c LIB2 LIB3 BTFILEMON_INLINE gtk_lib.c;

MODULES rbtjcgi (remcgi) =
	rbtjcgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB JFMT_INLINE LIBSHLL;

MODULES rbtjccgi (remcgi) =
	rbtjccgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES rbtjdcgi (remcgi) =
	rbtjdcgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES rbtjvcgi (remcgi) =
	rbtjvcgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES rbtjcrcgi (remcgi) =
	rbtjcrcgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB LIBSHLL;

MODULES rbtvcgi (remcgi) =
	rbtvcgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB VFMT_INLINE LIBSHLL;

MODULES rbtvccgi (remcgi) =
	rbtvccgi.c rcgilib.c LIB0 LIB1 LIB2 LIB3 DISPOPT_LIB VFMT_INLINE LIBSHLL;

######################################################################
######################################################################
##  Mostly names (important they come early)
##

[
Default names and comments for system variables
Names should be consecutive odd numbers, comments even numbers
]

P{Sysvarname load level} = 1		"LOADLEVEL"
P{Sysvarname load level} + 1		"Maximum value of load level"
P{Sysvarname cload} ++			"CLOAD"
P{Sysvarname cload} + 1			"Current value of load level"
P{Sysvarname log jobs} ++		"LOGJOBS"
P{Sysvarname log jobs} + 1		"File to save job record in"
P{Sysvarname log vars} ++		"LOGVARS"
P{Sysvarname log vars} + 1		"File to save variable record in"
P{Sysvarname machine} ++		"MACHINE"
P{Sysvarname machine} + 1		"Name of current host"
P{Sysvarname startlim} ++		"STARTLIM"
P{Sysvarname startlim} + 1		"Number of jobs to start at once"
P{Sysvarname startwait} ++		"STARTWAIT"
P{Sysvarname startwait} +1		"Wait time in seconds for job start"

[
Job Progress codes.
]

(0)	R	{Job progress code} = 100	""
(1)	Q	{Job progress code}	 	"Done"
(2)	Q	{Job progress code}		"Err"
(3)	Q	{Job progress code}		"Abrt"
(4)	Q	{Job progress code}		"Canc"
(5)	Q	{Job progress code}		"Init"
(6)	Q	{Job progress code}		"Strt"
(7)	Q	{Job progress code}		"Run"
(8)	Q	{Job progress code}		"Fin"

[
Colours for job progress
]

(0)	R	{Job progress clr} = 7000	""
(1)	Q	{Job progress clr}	 	"darkblue"
(2)	Q	{Job progress clr}		"darkred"
(3)	Q	{Job progress clr}		"darkred"
(4)	Q	{Job progress clr}		"purple"
(5)	Q	{Job progress clr}		"darkgreen"
(6)	Q	{Job progress clr}		"darkgreen"
(7)	Q	{Job progress clr}		"darkgreen"
(8)	Q	{Job progress clr}		"darkblue"

[
Subset of job progress codes which we can set to
]

(0)	R	{Set job progress code} = 101 		"Nil"
(1)	Q	{Set job progress code}	"Done"
(2)	Q	{Set job progress code}	"Err"
(3)	Q	{Set job progress code}	"Abrt"
(4)	Q	{Set job progress code}	"Canc"

H{Set job progress code}
"Please specify the new progress code you want to set this to.
Setting to nil or `Done' will allow it to run."

[
Flags to put on jobs/vars for display purposes (vars first)
]
P{Variable exported flag} ++			"Export"
P{Exportable flag} ++				"Export"
P{Remote runnable flag} ++			"Rem-runnable"
P{Locally run} ++				"Local Exec"
P{Variable clustered flag} ++			"Cluster"

[
Prompts and bits of times
]

P{Set time for} = ROUND(@, 10)	"Set time for: "

[
Days of the week
]

(0)Q{Weekdays} ++		"Sun"
(1)Q{Weekdays} COPY		"Mon"
(2)Q{Weekdays} COPY		"Tue"
(3)Q{Weekdays} COPY		"Wed"
(4)Q{Weekdays} COPY		"Thu"
(5)Q{Weekdays} COPY		"Fri"
(6)Q{Weekdays} COPY		"Sat"
(7)Q{Weekdays} COPY		"Hday"

N{Base for days to avoid} = {Weekdays} + 10
{Boolean true} = 1

[
Define which days are default "avoid"
Sunday and Saturday
]

({Base for days to avoid})	N	{Boolean true}
({Base for days to avoid}+6)	N	{Boolean true}

(0)Q{Weekdays full} = {Weekdays} + 1	"Sunday"
(1)Q{Weekdays full} COPY		"Monday"
(2)Q{Weekdays full} COPY		"Tuesday"
(3)Q{Weekdays full} COPY		"Wednesday"
(4)Q{Weekdays full} COPY		"Thursday"
(5)Q{Weekdays full} COPY		"Friday"
(6)Q{Weekdays full} COPY		"Saturday"
(7)Q{Weekdays full} COPY		"Holiday"

[
If not possible
]
(0)Q{Ifnposses} ++		"Skip"
(1)Q{Ifnposses} COPY		"Delay"
(2)Q{Ifnposses} COPY		"Delall"
(3)Q{Ifnposses} COPY		"Ctchup"

[
Define month names
]

(0)Q{Months} = ROUND(@, 20)	"Jan"
(1)Q{Months} COPY		"Feb"
(2)Q{Months} COPY		"Mar"
(3)Q{Months} COPY		"Apr"
(4)Q{Months} COPY		"May"
(5)Q{Months} COPY		"Jun"
(6)Q{Months} COPY		"Jul"
(7)Q{Months} COPY		"Aug"
(8)Q{Months} COPY		"Sep"
(9)Q{Months} COPY		"Oct"
(10)Q{Months} COPY		"Nov"
(11)Q{Months} COPY		"Dec"

[
Month names in full
]

(0)Q{Months full} = ROUND(@, 10)	"January"
(1)Q{Months full} COPY		"February"
(2)Q{Months full} COPY		"March"
(3)Q{Months full} COPY		"April"
(4)Q{Months full} COPY		"May"
(5)Q{Months full} COPY		"June"
(6)Q{Months full} COPY		"July"
(7)Q{Months full} COPY		"August"
(8)Q{Months full} COPY		"September"
(9)Q{Months full} COPY		"October"
(10)Q{Months full} COPY		"November"
(11)Q{Months full} COPY		"December"

[
Indicate delete at end/retain as done
]

P{Delete at end abbrev} ++	"Delete"
P{Retain at end abbrev} ++	"Retain"

[
Abbreviated repeat units
]

(0)Q{Repeat unit abbrev} ++	"Minutes"
(1)R{Repeat unit abbrev} COPY	"Hours"
(2)Q{Repeat unit abbrev} COPY	"Days"
(3)Q{Repeat unit abbrev} COPY	"Weeks"
(4)Q{Repeat unit abbrev} COPY	"Monthsb"
(5)Q{Repeat unit abbrev} COPY	"Monthse"
(6)Q{Repeat unit abbrev} COPY	"Years"

[
Repeat units in full
]

(0)Q,N{Repeat unit full} ++	"Minutes"
(1)R{Repeat unit full} COPY	"Hours"
(2)Q{Repeat unit full} COPY	"Days"
(3)Q{Repeat unit full} COPY	"Weeks"
(4)Q{Repeat unit full} COPY	"Months (rel beg)"
(5)Q{Repeat unit full} COPY	"Months (rel end)"
(6)Q{Repeat unit full} COPY	"Years"

H{Repeat unit full}
"Please enter the number of %s between
repetitions (ESC to abort)."

P{Delete at end full} ++	"Delete at end"
P{Retain at end full} ++	"Retain as done"

N{Default number of units} = {Repeat unit full}
N{Default repeat alternative} ++
N{Default skip delay option} ++

[
Define default number of units here
]

({Default number of units})	N	5

[
Default Repeat alternative
0 - delete, 1 - retain 2 to 8 mins,hours,days,weeks,monthsb,monthse,years
]

({Default repeat alternative})	N	1

[
Default "not possible" alternative 0 - skip 1 - delay 2 - delay all 3 - catch up
]

({Default skip delay option})	N	1

[
Assignment strings exit and signal
]

P{Assign exit code} = ROUND({Repeat unit full}, 20)	"exitcode"
P{Assign signal} ++		"signal"
P{xmbtr file sep} ++		"->"
P{xmbtr default job} ++		"(default job)"
P{xmbtr none host} ++		"(none)"

[
Default prefixes for job and command files
]

P{Default job file prefix} = ROUND(@, 10)	"J"
P{Default cmd file prefix} ++			"C"
P{Default job file suffix} ++			".gbj"
P{Default cmd file suffix} ++			".sh"

[
Display marker for btcilist etc set arg 0 and expand
]

P{Ci set arg0} = ROUND(@, 10)		"Set a0"
P{Ci expand args} ++			"$N"

[
Header portions for restricted views
]

P{btjcgi restr start} = ROUND(@, 20)	"<u>"
P{btjcgi restr view} ++			"View restricted"
P{btjcgi restr queue} ++		"For queue(s):"
P{btjcgi restr null} ++			"(plus null)"
P{btjcgi restr users} ++		"User(s):"
P{btjcgi restr groups} ++		"Group(s):"
P{btjcgi restr loco} ++			"Local only"
P{btjcgi restr plusrr} ++		"Local and Remote Run"
P{btjcgi restr end} ++			"</u>"

[
Flags for default/all
]

P{Btulist default name} = 397			"DEFAULT"
P{Btulist all name} ++				"ALL"

N{Privs initial row} = 399
S{Privs abbrev} = 10

[
Names for permissions
]

P,N{Read adm full} = 400		"Read admin file"
P,N{Write adm full} ++ COPY		"Write admin file"
P,N{Create entry full} ++ COPY		"Create entry"
P,N{Special create full} ++ COPY	"Special create"
P,N{Stop sched full} ++ COPY		"Stop scheduler"
P,N{Change default modes full} ++ COPY	"Change default modes"
P,N{Combine user group full} ++	COPY	"Combine user/group perms"
P,N{Combine user other full} ++ COPY	"Combine user/other perms"
P,N{Combine group other full} ++ COPY	"Combine group/other perms"

[
Define order of rows displayed
]
({Privs initial row})		N	{Read adm full}
({Read adm full})		N	{Write adm full}
({Write adm full})		N	{Create entry full}
({Create entry full})		N	{Special create full}
({Special create full})		N	{Stop sched full}
({Stop sched full})		N	{Change default modes full}
({Change default modes full})	N	{Combine user group full}
({Combine user group full})	N	{Combine user other full}
({Combine user other full})	N	{Combine group other full}
({Combine group other full})	N	-1

[
Abbreviations for priv names
]

P{Read adm abbr} = {Read adm full} + {Privs abbrev}	"RA"
P{Write adm abbr} ++			"WA"
P{Create entry abbr} ++			"CR"
P{Special create abbr} ++		"SPC"
P{Stop sched abbr} ++			"ST"
P{Change default modes abbr} ++		"Cdft"
P{Combine user group abbr} ++		"UG"
P{Combine user other abbr} ++		"UO"
P{Combine group other abbr} ++		"GO"

N{Modes initial row} = 449

[
Names for modes
]

P,N{Read mode name} = 450		"Read"
P,N{Write mode name} ++ COPY		"Write"
P,N{Reveal mode name} ++ COPY		"Reveal"
P,N{Display mode name} ++ COPY		"Display mode"
P,N{Set mode name} ++ COPY		"Set mode"
P,N{Assume owner mode name} ++ COPY	"Assume ownership"
P,N{Assume group mode name} ++ COPY	"Assume group ownership"
P,N{Give owner mode name} ++ COPY	"Give away owner"
P,N{Give group mode name} ++ COPY	"Give away group"
P,N{Delete mode name} ++ COPY		"Delete"
P,N{Kill mode name} ++ COPY		"Kill (jobs only)"

[
Define order in which mode names appear
]

({Modes initial row})	N	{Read mode name}
({Read mode name})	N	{Write mode name}
({Write mode name})	N	{Reveal mode name}
({Reveal mode name})	N	{Display mode name}
({Display mode name})	N	{Set mode name}
({Set mode name})	N	{Assume owner mode name}
({Assume owner mode name})	N	{Assume group mode name}
({Assume group mode name})	N	{Give owner mode name}
({Give owner mode name})	N	{Give group mode name}
({Give group mode name})	N	{Delete mode name}
({Delete mode name})	N	{Kill mode name}
({Kill mode name})	N	-1

######################################################################
######################################################################
##  Mostly btr messages (important they come early)
##

[
Messages and prompts for shell commands
]

E,H{Invalid assignment} = 50
"%P: Bad assignment `%s'",
"Your saved job contains an invalid assignment '%s'"

E,H{Invalid char in time} ++
"%P: Bad char `%c0' in time spec `%s'",
"There is an incorrect format of the time specification in the
saved job."

E,H{Unknown command interp} ++
"%P: Unknown command interp `%s'",
"The command interpreter specified in the job is not known"

E,H{Bad condition} ++
"%P: Bad condition `%s'",
"Invalid condition in saved job."

E,H{Condition string too long} ++
"%P: String too long in condition `%s'",
"The string is too long in the condition in the saved job."

E,H{Load level out of range} ++
"%P: Load level `%d0' out of range",
"The load level argument specified in the job is incorrect"

E,H{Bad mode string} ++
"%P: Bad mode string `%s'",
"The mode string specified in the job is incorrect."

E,H{Priority out of range} ++
"%P: Priority `%d0' out of range",
"The priority argument specified in the job is incorrect."

E,H{Bad redirection} ++
"%P: Bad redirection statement `%s'",
"Invalid redirection statement in saved job."

E,H{Bad repeat} ++
"%P: Bad repeat statement `%s'",
"The repeat specification given in the saved job is incorrect"

E,H{Bad time spec} ++
"%P: Bad time specification `%s'",
"The time given in the saved job is incorrect."

# Don't mean an H here as it is a warning only in xmbtr

E{Time not in future} ++		"%P: Time given is not in future"

E,H{Assignment max exceeded} ++
"%P: Sorry, you cannot have that many assignments.",
"The maximum level of assignments has been reached."

E,H{No set flags given} ++
"%P: Sorry, you must set some flags",
"All the assignment flags in the saved job are unset."

E,H{Condition max exceeded} ++
"%P: Sorry, you cannot have that many conditions",
"Conditions limit exceeded in saved job."

E,H{File descriptor out of range} ++
"%P: File descriptor %d0 out of range (this is a Unix limit)",
"File descriptor out of range in saved file."

E,H{Redirection max exceeded} ++
"%P: Sorry, you cannot have that many redirections",
"Sorry but you have specified too many redirections in
the saved file."

E,H{Max var name size} ++
"%P: Sorry, variable name `%s' is too long",
"The variable name given in the saved job is too long."

E,H{String too long in set} ++
"%P: Sorry, string too long `%s'",
"The string is too long in the assignment in the saved job."

E,H{Cannot open file argument} ++
"%P: Cannot open file argument `%s'",
"The saved job references a file argument %s
which cannot be opened"

E,H{Must specify priority} ++
"%P: Sorry, no default priority, you must specify one [ -p n ]",
"Your default priority has been set to be outside the ranges
of the maximum and minimum priorities.
Any job (including this saved job) must explicitly specify the
priority"

E,H{Cannot use GNUbatch} ++
"%P: Sorry, you cannot use GNUbatch",
"Your ranges of minimum and maximum priorities are set so
that you cannot submit a job with a valid priority"

E,H{Invalid priority} ++
"%P: Invalid priority %d0, must be in range %d1 to %d2",
"The saved job contains a priority %d0 outside your
minimum of %d1 and your maximum of %d2."

E,H{No room for job} ++
"%P: Sorry, no room on disc for job",
"The disc space in the batch spool area is full up"

E,H{Unreadable variable} ++
"%P: Unknown or unreadable variable %s",
"Saved job references unknown or unreadable variable %s"

E,H{Unwritable variable} ++
"%P: Unknown or unwritable variable %s",
"Saved job assigns to unknown or unwritable variable %s"

E,H{No special create} ++
"%P: Sorry, but you need `special create permission' to respecify
load levels.",
"As shown, you need the 'special create permission' to respecify
load levels from the default, as specified in the job."

E,H{No create perm} ++
"%P: Sorry, but you need `create permission' to create jobs.",
"Your account does not have permission to create new jobs.
Please have this changed if necessary. NB Do not confuse this
with 'special create permission'"

E{Avoiding all in defaults} ++
"%P: Sorry but your default ``avoid'' days (in this file) includes
every day of the week."

# Extra stuff here

E,H{Setting avoid all} ++
"%P: Sorry but you cannot avoid every day of the week.",
"Sorry but you have specified every day of the week to avoid.
Please unset some."

E,H{Cannot read weekdays} ++
"%P: Cannot find weekdays list (in %F)",
"Sorry but the help file %F lacks the list of weekdays."

E,H{Bad avoid arg} ++
"%P: Bad -A (avoid) argument.
Enter as -A Mon,Wed. Leading comma includes defaults,
Leading - as in -A -,Wed clears existing.",
"Bad -A (avoid) argument.
Invalid avoid days argument in saved job."

E{Invalid load level} ++
"%P: Invalid load level %d0, your maximum is %d1."

E,H{Bad umask} ++
"%P: Invalid umask %s, must be 3 OCTAL digits",
"Invalid umask in saved file."

E,H{Bad ulimit} ++
"%P: Invalid ulimit %s, value %s must be a long > 0 or 0",
"Invalid ulimit in saved file."

E,H{Bad exit code spec} ++
"%P: Bad exit code format %s. Format is -XN1:10 or -XE11:100",
"Invalid exit code format in saved file."

E,H{Cannot find rep units} ++
"%P: Fatal error - cannot find Unit (months/days etc)
specification in help file %F",
"Sorry, but there is no repeat unit given in help file %F"

E{Job created with name} ++		"%P: Job `%s' created successfully from %t - job number %d0"
E{Job created without name} ++		"%P: Job number %d0 created successfully from %t"

P,E{Expecting terminal input} ++
"Standard input",
"Warning: Expecting input from terminal"

E{Past time incremented} ++
"%P:  Warning: The specified time was not in the future.
Incremented to: %s"

E{Btdst usage} ++
"Usage: %P [-R] start-time end-time adj
adj is time to adjust in seconds -3600 will bring time forward
(change TO summer time) 3600 puts back (winter time)."

E{Bad dst adjustment} ++		"%P: Bad DST adjustment start time must precede end time."
E{No perm for DST} ++			"%P: No permission for DST adjustment"

E,H{Cannot set owner} ++
"%P: Sorry but you need 'write admin' permission to queue jobs
with different owners.",
"You are trying to select a different owner, but you require
""write admin"" permission to do this."

E,H{Cannot set group} ++
"%P: Sorry but you need 'write admin' permission to queue jobs
with different groups.",
"You are trying to select a different group, but you require
""write admin"" permission to do this."

E,H{Unknown owner} ++
"%P: Unknown user `%s'",
"The proposed user name %s is not known."

E,H{Unknown group} ++
"%P: Unknown group `%s'",
"The proposed group name %s is not known."

E,H{Unknown host name in var} ++
"Unknown host name in variable reference '%s'",
"The host name given in the variable reference is not known."

E,H{Invalid variable name} ++
"Invalid variable name in '%s'.",
"The variable name given in the saved job is invalid."

E,H{Cannot respecify mode} ++
"%P: You are not allowed to vary from your default mode.",
"You are not allowed to vary your default mode from that given
in your user profile (as given in the saved job)."

E,H{Could repeat endlessly} ++
"%P: The effective ""target day"" of your months-relative-to-end
parameter could take you back past the beginning of the
month on the next repeat. Specify the desired target day using
a day as ""30"" in -r Monthse:1:30",
"Strange target day in saved file."

E,H{Bad delete time} ++
"Invalid delete time %s.
Please specify as a number of hours",
"Invalid delete time in saved file."

E,H{Bad run time} ++
"Invalid run time %s.
Please specify seconds, mm:ss or hh:mm:ss",
"Invalid run time in saved file."

E,H{Bad signal number} ++
"Invalid signal number %s",
"Invalid signal number in saved file."

E,H{Bad grace time} ++
"Invalid run-on time %s.
Please specify as seconds or mm:ss",
"Invalid ""grace period"" in saved file."

E{No jobs specified} ++
"%P: You didn't give any jobs to work on.
Please specify some."

E{Unknown host name} ++			"%P: Unknown host name %s"
E{Not numeric argument} ++		"%P: Job argument %s is not numeric"
E{No valid args to process} ++		"%P: No valid arguments to process - aborting"
E{Cannot find job} ++			"%P: Cannot find job %s"
E{btjdel job running} ++		"%P: Job %s is running - use ""-y"" argument to force kill."
E{btjdel job still running} ++		"%P: Job %s ""%t"" did not die in %d0 seconds - try -K9"
E{btjchange no changes} ++		"%P: You didn't specify any changes to the jobs"
E{Unqueue process aborted} ++		"Unqueue process aborted job %d0 `%s'"
E{Undef err unq} ++			"Undefined error unqueue process job %d0 `%s'"
E{Unq file not found} ++		"Unqueue process file not found for job %d0 `%s'"
E{Unq dir not found} ++			"Unqueue process directory not found job %d0 `%s'"
E{Unq cannot create file} ++		"Unqueue process cannot create file for job %d0 `%s'"
E{Unq job not found} ++			"Unqueue process cannot find job %d0 `%s'"
E{Unq cannot del} ++			"Unqueue process cannot delete job %d0 `%s'"

E{Bad kill type} ++
"%P invalid kill argument %s
Please specify signal number in range %d0 to %d1"

E{btjdel cannot fork} ++
"%P: Cannot fork - %E
(Trying to unqueue)"

E{Btuchange inv pri} ++			"Invalid priority arg `%s' - should be numeric"
E{Btuchange inv pri range} ++		"Invalid priority `%s' - should be 1 to 255"
E{Btuchange inv ll} ++			"Invalid load level arg `%s' - should be numeric"
E{Btuchange inv ll range} ++		"Invalid load level `%s' - should be 1 to 65535"
E{Btuchange inv priv} ++		"Invalid privilege flag `%s'"
E{Btuchange unexp arg} ++		"Unexpected arguments follow set of defaults"
E{Btuchange unexp user} ++		"Unexpected user name `%s' - ignored"
E{Btuchange inv nice} ++		"Invalid nice arg `%s' - should be numeric"
E{Btuchange nice range} ++		"Invalid nice `%s' - should be 0 to 39"

E{No upd ci perm} ++
"You need ""special create"" permission to update command
interpreters - sorry."

E{Btcilist usage} ++
"%P: Usage - no arguments are expected apart from
-Q hostname"

E{Btcichange missing args} ++		"%P - expecting further arguments."
E{Btcichange unknown ci} ++		"%P - unknown command interpreter %s"
E{Btcichange name clash} ++		"%P - new name %t for %s clashes with existing name"
E{Btcichange del def} ++		"%P - you cannot delete the default shell name."
E{Btcichange new name clash} ++		"%P - new command interpreter %s clashes with existing."

E{Btcichange no path name} ++
"%P - no path name set for new command interpreter.
Please try again with option ""-p""."

E{Btquit no perm} ++
"Sorry - you (%R) are not allowed to stop the GNUbatch scheduler
Please ask someone who is.
If you meant to just stop a single job, this is the wrong
command anyway - use `btq' (etc) instead."

E{Btquit usage} ++
"Usage: %P
  or   %P -y     to suppress confirmation message and proceed"

E{Btquit not stopped} ++		"GNUbatch not stopped."

E{Btquit IPC error} ++
"Error trying to send message to scheduler
Error was: %E
Please investigate!"

E{Btquit receive error} ++
"Error trying to receive message from scheduler
Error was: %E
Please investigate!"

P,E{Btquit stopped ok} ++
"GNUbatch scheduler is running - please confirm quit: ",
"GNUbatch stopped satisfactorily."

E{Cannot find rbtr} ++
"%P:Cannot locate the program ""rbtr"" to run remote queue operation.
Are you sure that this has been installed?"

E{Cannot run rbtr} ++
"%P:Cannot run the program ""rbtr"" to run remote queue operation.
Are you sure that this has been installed?"

E,H{Cannot create UDP access socket} ++
"Cannot create UDP socket %d0 to access remote %N1",
"Trouble trying to open the UDP socket for obtaining remote parameters."

E,H{Cannot bind UDP access socket} ++
"Cannot bind UDP socket %d0 to access remote %N1",
"Trouble trying to bind UDP socket for obtaining remote parameters."

E{Unknown queue host} ++		"Unknown queue host %s"

E{Is my host} ++
"The name you gave to send to is this machine, or another name
for it. We don't loop back to btr as it is possible for btr
to reinvoke %P and we could go on for ever."

E{No remote host specified} ++		"No remote host specified - please use btr for submitting local jobs"
E,H{No listening port} ++
"No TCP listening port found",
"Some problem with your services database - no port for listening"

E{btjdel file name too long} ++         "File name too long in %P argument"

E,H{No xbnetserv TCP service} = 155
"Cannot find TCP service name. Maybe you need to set up
the TCP service port gnubatch-netserv in /etc/services.",
"No TCP service port gnubatch-netserv in /etc/services.
This needs to be set up to submit jobs remotely."

E,H{No xbnetserv UDP service} ++
"Cannot find UDP service name. Maybe you need to set up
the UDP service port gnubatch-netserv in /etc/services",
"No UDP service port gnubatch-netserv in /etc/services.
This needs to be set up to submit jobs remotely."

E,H{Netconn cannot open apisock} ++
"Cannot open socket for API",
"Unable to open the socked for API connections"

E,H{Cannot send UDP packet} ++
"Cannot send UDP packet.
Is xbnetserv running on %N1?",
"The attempt to send a message to the remote host to obtain
system parameters failed. Possibly this is because there is
some fault on the network, or xbnetserv is not running on
the remote host."

E,H{Cannot receive UDP packet} ++
"Cannot receive UDP packet. Is xbnetserv running on %N1?",
"The attempt to receive a message from the remote host to obtain
system parameters failed. Possibly this is because there is
some fault on the network, or xbnetserv is not running on
the remote host."

E,H{No such user on remote} ++
"No such valid user on remote %N1",
"The user %R, as whom you are logged in, does not exist on the remote host.
You need to set it up first - sorry."

E,H{Trouble with job header} ++
"Trouble sending job header - %E",
"There has been a Network failure sending header of job to remote."

E,H{Trouble with job} ++
"Trouble sending job - %E",
"There has been a Network failure sending the job to the remote system."

E,H{Cant read status result} ++
"Can't read status result - sorry I don't know what
happened.",
"Cannot read response from server - probable network error"

E,H{Bad condition result} ++
"Bad Condition result - your job was rejected by host %N1
due to an invalid condition but I cannot detect which one.",
"Something is wrong with one of the condition variables in your
job, however we cannot discover which one - sorry. Maybe something
got changed in mid-flight."

E,H{Bad assignment result} ++
"Bad assignment result - your job was rejected by host %N1
due to an invalid condition but I cannot detect which one.",
"Something is wrong with one of the assignment variables in your
job, however we cannot discover which one - sorry. Maybe something
got changed in mid-flight."

E,H{Unknown queue result message} ++
"Unknown queue result message %d0",
"Some unexpected message %d0 returned from remote host."

E,H{No XML library} ++
"Built without XML library",
"Cannot complete this operation as this version was built without the XML library"

[
Btstart
]

E{Restarting scheduler} = 180	"%P - restarting GNUBatch scheduler process"
E{Btstart fork looping} ++		"%P - waiting to fork scheduler process (got error %E)"
E{Btstart giving up} ++			"Could not start scheduler - giving up."
E{Btstart still couldnt open} ++	"Still could not open control file err is %E"
E{Btconn unknown host} ++		"%P - Unknown host name `%s'"

E{Dos client as host} ++
"%s is a client (probably DOS) system it must connect to this
machine, not vice versa - sorry."

E{Btconn no conn priv} ++
"Sorry but you (%R) do not have privileges to connect or
disconnect hosts. You need to have ""stop scheduler"" privilege"

E{Btconn no machine arg} ++		"%P - No machine name given"
E{Btconn multi machine} ++		"%P - sorry only one machine at a time may be given"
E{Btconn connection fail} ++		"%P - connection to %s failed"

E{Btstart explain} ++
"Usage:
   %P [-l init ll] [-j initial jsize] [-v initial vsize] [-t] [-T]

Arguments are ineffective if scheduler is running - quit first."

E{Btvar usage error} = ROUND(@, 20)
"Usage:
   %P [-SDCXELkK] [-u value ] [-s value ] [+eq|+ne|+lt|+le|+gt|+ge value ] Var"

E{Btvar bad variable name} ++		"%P: Bad variable name %s"
E{Btvar set and delete} ++		"%P: Cannot have set and delete at once"
E{Btvar notfnd print} ++		"%P: Unknown variable %s"
E{Btvar notfnd compare} ++		"%P: Cannot find variable %s to compare"
E{Btvar notfnd set} ++			"%P: Cannot find variable %s to set"
E{Btvar notfnd del} ++			"%P: Cannot find variable %s to delete"
E{Btvar notfnd mode set} ++		"%P: Cannot find variable %s to change mode"
E{Btvar mode set and delete} ++		"%P: Cannot have mode set and delete at once"
E{Btvar unknown host name} ++ 		"%P: Unknown host name in '%s'."
E{Btvar notfnd set user} ++		"%P: Cannot find variable %s to change owner"
E{Btvar notfnd set group} ++		"%P: Cannot find variable %s to change group"

E{btjlist cannot view local job} ++	"Cannot open job file for local job %d0"
E{btjlist cannot view remote job} ++	"Cannot open job file for remote job %s:%d0"
E{btjlist no perm view local} ++	"No permission to view local job %d0"
E{btjlist no perm view remote} ++	"No permission to view remote job %s:%d0"

E{btjstat usage} ++			"Usage: %P [-d] [-s state] jobnumber"
E{btjstat job not found} ++		"Job %s not found"

E{btulist nothing to do} ++		"Both header and user lines suppressed - nothing to do"

E{btvcgi var exists} = ROUND(@,50)	"Variable %s already exists"
E{btvcgi var non numeric} ++		"Variable %s not numeric - cannot do arithmetic"
E{btvcgi wrong number args} ++		"Arg %s - expected %d0 args got %d1"
E{btvcgi op local host only} ++		"Operation %s is only available on the local host"

E{btjccrcgi no data} = ROUND(@, 10)	"No job data (script for job) supplied"
E{btjcrcrgi cant reopen temp file} ++	"Cannot reopen temporary file for create from file"

E{btjcgi no mode perm} = ROUND(@, 10)	"You do not have permission to change modes of job %s"

E{Bts no files} = 280                   "%P: no files to work on"
E{Bts cannot open file} ++              "%P: Cannot open argument file %s"
E{Bts no permission on file}++          "%P: No permission to open file %s"
E{Bts other error file} ++              "%P: Other error (%E) opening %s"
E{Bts aborting due to arg errors} ++    "%P: Aborting had %d0 error(s) in args"
E{Bts invalid format file} ++           "%P: Invalid format XML file %s"
E{Bts invalid conds} ++                 "%P: Invalid conditions in file %s"
E{Bts invalid asses} ++                 "%P: Invalid assignments in file %s"
E{Bts too many strings} ++
"%P: Sorry job in %s has too many strings to pack into currently-compiled version"
E{Bts unknown error} ++                 "%P: Unknown XML error parsing file %s"
E{Bts invalid priority} ++              "%P: Invalid priority %d0 in job file %s, should be between %d1 and %d2"
E{Bts invalid load level} ++            "%P: Load level %d0 in saved job %s is invalid max for user is %d1"
E{Bts invalid directory} ++             "%P: No directory given for job file %s"
E{Bts cannot set mode} ++               "%P: Modes in job file %s differ from default for user %R"
E{Bts write script fail} ++
"%P: Cannot write script for job file %s to file.
Error code was %E"

[
API error codes
]

S{Base for API errors} = 26100

E{Base for API errors} -1 COPY
"Remote CGI program error - invalid API fd
(Probably a bug in %P)"

E{Base for API errors} -2 COPY
"Remote CGI error - run out of memory in xbnetserv executing %P
(Probably a bug in %P or xbnetserv)."

E{Base for API errors} -3 COPY
"Remote CGI program error - invalid hostname"

E{Base for API errors} -4 COPY
"Remote CGI program error - invalid service"

E{Base for API errors} -5 COPY
"Remote CGI program error - no default service.
(Expecting xbapi service to be specified)."

E{Base for API errors} -6 COPY
"Remote CGI program error in %P - cannot create socket for API"

E{Base for API errors} -7 COPY
"Remote CGI program error in %P - cannot bind socket for API"

E{Base for API errors} -8 COPY
"Remote CGI program error in %P - cannot connect API socket"

E{Base for API errors} -9 COPY
"Remote CGI program error in %P - Read fail"

E{Base for API errors} -10 COPY
"Remote CGI program error in %P - Write fail"

E{Base for API errors} -11 COPY
"Remote CGI program error - run out of processes"

E{Base for API errors} -23 COPY
"Remote CGI program error in %P - invalid user"

E{Base for API errors} -24 COPY
"Remote CGI program error in %P - invalid command interpreter"

E{Base for API errors} -25 COPY
"Remote CGI program error in %P - invalid condition variable(s)
(Probably not readable by user)."

E{Base for API errors} -26 COPY
"Remote CGI program error in %P - invalid assignment variable(s)
(Probably not writeable by user)."

E{Base for API errors} -28 COPY
"Remote CGI program error in %P - no memory for queue file.
Unable to allocate space in xbnetserv to hold user details."

E{Base for API errors} -29 COPY
"Remote CGI program error in %P - no create permission"

E{Base for API errors} -30 COPY
"Remote CGI program error in %P - invalid priority"

E{Base for API errors} -31 COPY
"Remote CGI program error in %P - invalid load level"

E{Base for API errors} -32 COPY
"Remote CGI program error in %P - invalid user name"

E{Base for API errors} -33 COPY
"Remote CGI program error in %P - out of file space.
Probably batch spool directory has run out of space."

E{Base for API errors} -34 COPY
"Remote CGI program error in %P - message queue is full.
The host system message queue is full or filling up.
This may be due to another application using all available
message queue space."

E{Base for API errors} -35 COPY
"Remote CGI program error in %P - invalid job data"

E{Base for API errors} -36 COPY
"Remote CGI program error in %P - unknown user"

E{Base for API errors} -37 COPY
"Remote CGI program error in %P - unknown group"

E{Base for API errors} -38 COPY
"Remote CGI program error in %P - unknown error"

E{Base for API errors} -39 COPY
"Remote CGI program error in %P - need read
admin permission for operation."

E{Base for API errors} -40 COPY
"Remote CGI program error in %P - need change defaults
permission."

E{Base for API errors} -41 COPY
"Remote CGI program error in %P - unknown command.
(Probably a bug)."

E{Base for API errors} -42 COPY
"Remote CGI program error in %P - sequence error.
The job or variable has changed. Try again."

E{Base for API errors} -43 COPY
"Remote CGI program error in %P - unknown job"

E{Base for API errors} -44 COPY
"Remote CGI program error in %P - unknown variable"

E{Base for API errors} -45 COPY
"Remote CGI program error in %P - no permission for operation"

E{Base for API errors} -46 COPY
"Remote CGI program error in %P - invalid year."

E{Base for API errors} -47 COPY
"Remote CGI program error in %P - job is running.
Halt the job(s) before trying this."

E{Base for API errors} -48 COPY
"Remote CGI program error in %P - no time to advance.
The job does not have a time to advance."

E{Base for API errors} -49 COPY
"Remote CGI program error in %P - invalid variable name."

E{Base for API errors} -50 COPY
"Remote CGI program error in %P - variable error."

E{Base for API errors} -51 COPY
"Remote CGI program error in %P - invalid slot number."

E{Base for API errors} -52 COPY
"Remote CGI program error in %P - job is not running."

E{Base for API errors} -53 COPY
"Remote CGI program error in %P - no memory for queue op."

E{Base for API errors} -54 COPY
"Remote CGI program error in %P - no permission to access variable."

E{Base for API errors} -55 COPY
"Remote CGI program error in %P - remote variable
in local-only job."

E{Base for API errors} -56 COPY
"Remote CGI program error in %P - local variable
in remote-accessed job."

E{Base for API errors} -57 COPY
"Remote CGI program error in %P - too few
permissions on job or variable."

E{Base for API errors} -58 COPY
"Remote CGI program error in %P -
Invalid use of system variable."

E{Base for API errors} -59 COPY
"Remote CGI program error in %P -
Invalid type for system variable."

E{Base for API errors} -60 COPY
"Remote CGI program error in %P -
Variable exists."

E{Base for API errors} -61 COPY
"Remote CGI program error in %P -
Attempting to delete system variable."

E{Base for API errors} -62 COPY
"Remote CGI program error in %P -
Attempting to delete variable in use."

E{Base for API errors} -63 COPY
"Remote CGI program error in %P -
Attempting to delete remote variable."

E{Base for API errors} -64 COPY
"Remote CGI program error in %P -
No password specified."

E{Base for API errors} -65 COPY
"Remote CGI program error in %P -
Invalid password specified."

E{Base for API errors} -66 COPY
"Remote CGI program error in %P -
Invalid group specified."

S{Base for scheduler job errors} = 1000

[
Errors from scheduler about jobs
]

E,H{Base for scheduler job errors} + 1 COPY
"The job %d0 does not exist. Perhaps someone else just deleted it?",
"The job %d0 does not exist or has vanished.
Perhaps someone else just deleted it?"

E,H{Base for scheduler job errors} + 2 COPY
"One or more of the variables referred to in the job %s
(%d0) do not exist. Please try again.",
"One or more of the variables referred to in the job %s
(%d0) do not exist. Please try again."

E,H{Base for scheduler job errors} + 3 COPY
"You are not permitted to perform the requested operation
on job %s (%d0).",
"You are not permitted to perform the requested operation
on job %s (%d0)."

E,H{Base for scheduler job errors} + 4 COPY
"One or more of the variables referred to in the job %s
(%d0) does not have the correct permissions.",
"One or more of the variables referred to in the job %s
(%d0) does not have the correct permissions."

E,H{Base for scheduler job errors} + 5 COPY
"You do not have the right privileges to perform
this operation.",
"You do not have the right privileges to perform
this operation."

E,H{Base for scheduler job errors} + 6 COPY
"The job would set a read-only system variable.",
"The job would set a read-only system variable."

E,H{Base for scheduler job errors} + 7 COPY
"The job would set a system variable to the wrong type.",
"The job would set a system variable to the wrong type."

E,H{Base for scheduler job errors} + 8 COPY
"Cannot allocate any more jobs",
"Cannot allocate any more jobs"

E,H{Base for scheduler job errors} + 9 COPY
"Job is running",
"Job is running"

E,H{Base for scheduler job errors} + 10 COPY
"Remote variable(s) found in local job",
"Your job is local (not exported) but contains
remotely-visible variables. Set the job to be
exported first before you allocate any variables."

E,H{Base for scheduler job errors} + 11 COPY
"Exported job refers to local variable(s)
In other words you must export the variables before you
can include them in jobs to export.",
"Exported job refers to local variable(s)
Your job is exported but contains purely local
variables. Delete the variables or set the job
to be local first."

E,H{Base for scheduler job errors} + 12 COPY
"There are too few modes permitted on this job for anyone
to do anything with it (check your defaults with btuser?).",
"There are too few modes permitted on this job for anyone
to do anything with it (check your defaults with btuser?)."

E,H{Base for scheduler job errors} + 13 COPY
"Incomplete remote job",
"This remote job refers to variables and conditions on other
machines which are not connected yet. Changes have been
suppressed to avoid conflicts. Please connect as required."

E,S{Base for scheduler var errors} = ROUND(@, 50)
""
[
Errors from scheduler about vars
]

E,H{Base for scheduler var errors} + 1 COPY
"Variable %s exists",
"Your variable already exists - sorry I missed it."

E,H{Base for scheduler var errors} + 2 COPY
"Variable %s does not exist",
"The variable does not exist - maybe it just got deleted."

E,H{Base for scheduler var errors} + 3 COPY
"Variable %s clashes with similar named variable
belonging to another user.",
"Variable %s clashes with similarly-named variable
belonging to another user."

E,H{Base for scheduler var errors} + 4 COPY
"You are not permitted to perform the requested operation
on variable %s.",
"You are not permitted to perform the requested operation
on variable %s."

E,H{Base for scheduler var errors} + 5 COPY
"You need a special privilege to perform this.",
"You need a special privilege to perform this variable operation."

E,H{Base for scheduler var errors} + 6 COPY
"Some other user attempted to update variable %s
at the same time. Please check and perhaps try again.",
"Some other user attempted to update variable %s
at the same time. Please check and perhaps try again."

E,H{Base for scheduler var errors} + 7 COPY
"Attempting to reset a read-only system variable %s.",
"Attempting to reset a read-only system variable %s."

E,H{Base for scheduler var errors} + 8 COPY
"Attempting to set system variable %s to wrong type.",
"Attempting to set system variable %s to wrong type."

E,H{Base for scheduler var errors} + 9 COPY
"No more space for variables.",
"I cannot allocate any more shared memory space to hold
new variables - sorry. Please check your kernel configuration."

E,H{Base for scheduler var errors} + 10 COPY
"Attempting to delete system var %s",
"You cannot delete a system variable such as %s"

E,H{Base for scheduler var errors} + 11 COPY
"Variable %s in use in jobs.",
"One or more jobs refer to this variable either in conditions
or assignments."

E,H{Base for scheduler var errors} + 12 COPY
"There are too few modes permitted on this variable for anyone
to do anything with it (check your defaults with btuser?).",
"There are too few modes permitted on this variable for anyone
to do anything with it (check your defaults with btuser?)."

E,H{Base for scheduler var errors} + 13 COPY
"Invalid Delete of remote variable attempted",
"You cannot delete a remote variable.
Log in to the relevant machine first."

E,H{Base for scheduler var errors} + 14 COPY
"Unknown remote user for variable",
"The user name is not recognised system-wide."

E,H{Base for scheduler var errors} + 15 COPY
"Unknown remote group for variable",
"The group name is not recognised system-wide."

E,H{Base for scheduler var errors} + 16 COPY
"Cannot rename variable %s as %t
as it clashes with an existing %t",
"Cannot rename variable %s as %t
as it clashes with an existing %t"

E,H{Base for scheduler var errors} + 17 COPY
"Cannot set %s local and clustered",
"The variable %s must be set exported before it
can be set clustered unset clustered before it
is set local."

E,H{Base for scheduler var errors} + 18 COPY
"Cannot rename clustered variable %s.",
"You are not allowed to rename a clustered variable
such as %s. You should reset the clustered
marker first. (This may entail removing references
to it from job conditions or assignments)."

S{Base for scheduler net errors} = ROUND(@, 50)

[
Errors from scheduler about networks
]
E,H{Base for scheduler net errors} + 4 COPY
"Unable to fork to satisfy remote request",
"The remote request involves a fork which cannot
be done because there are too many processes
at one or other end."

E,H{Base for scheduler net errors} + 5 COPY
"Bad message queue on remote request
(Problems at other end).",
"There is a problem with the message queue on
the remote machine associated with the last
request you did. Maybe the scheduler process
just crashed?"

E,H{Base for scheduler net errors} + 6 COPY
"Network timeout on remote request",
"An internal timeout has failed at the remote end.
Maybe the network or a host has just died?"

E,H{Base for scheduler net errors} + 7 COPY
"Specified host is off line",
"A host is off line or has just died."

E,H{Base for scheduler net errors} + 8 COPY
"Specified host died",
"I am pretty sure that one of your remote hosts
just died."

E{Base for scheduler net errors} + 9 COPY	"Connection failed"

E{Base for rbtr return errors} = ROUND(@, 50)	"Conns"

[
Errors within rbtr
]

E,H{Base for rbtr return errors} + 1	COPY
"This machine is unknown to the host machine %N1
Please ensure that this machine name is placed in the
system hosts file on %N1, possibly
with the ""manual"" flag.",
"Please set up the system hosts file as described"

E,H{Base for rbtr return errors} + 2	COPY
"Not a DOS client. You should not have got this error.",
"This error - er - cannot happen. It is a figment of your
fevered imagination."

E,H{Base for rbtr return errors} + 3	COPY
"Unknown user at remote machine %N1",
"Sorry but the user name given is not known at the remote
host %N1."

E,H{Base for rbtr return errors} + 4	COPY
"Unknown command interpreter %s",
"The command interpreter specified in your job is not known
at the remote host %N1. You need to set it up there."

E,H{Base for rbtr return errors} + 5	COPY
"Permission denied to condition variable %s by host %N1",
"No read permission to variable %s by host %N1."

E,H{Base for rbtr return errors} + 6	COPY
"Permission denied to assignment variable %s by host %N1",
"No assign permission to variable %s by host %N1."

E,H{Base for rbtr return errors} + 8	COPY
"No memory for queue file on host %N1 - sorry",
"No memory/disk space for queue file on the host."

E,H{Base for rbtr return errors} + 9	COPY
"No create permission for new jobs on host %N1",
"You do not have permission to create jobs on the host."

E,H{Base for rbtr return errors} + 10	COPY
"Invalid priority specified",
"Priority is outside acceptable range on remote host."

E,H{Base for rbtr return errors} + 11	COPY
"Invalid load level specified",
"Load level is outside acceptable range on remote host."

E,H{Base for rbtr return errors} + 12	COPY
"Bad user name - you do not have write admin file permission
which you must have to respecify users.",
"You cannot respecify user names on the remote host %N1."

E,H{Base for rbtr return errors} + 13	COPY
"Remote file system (on %N1) is full - cannot accept job.",
"The file system is too full on host %N1 to spool any new jobs."

E,H{Base for rbtr return errors} + 14	COPY
"Remote IPC message queue (on %N1) is full - cannot accept job.",
"The IPC message queue for GNUbatch is full on the remote machine and
new jobs cannot be accepted."

E,H{Base for rbtr return errors} + 15	COPY
"Invalid job data - protocol error with %N1",
"There is a protocol error between %P and xbnetserv on the remote."

E,H{Base for rbtr return errors} + 16	COPY
"Unknown user on machine %N1 specified",
"Unknown user name specified on host %N1."

E,H{Base for rbtr return errors} + 17	COPY
"Unknown group on machine %N1 specified",
"Unknown group name specified on host %N1."

E,H{Base for rbtr return errors} + 18	COPY
"Other error",
"Unknown other error."

E,H{Base for rbtr return errors} + 19	COPY
"No read admin file permission on host %N1",
"No permission to read admin files on host."

E,H{Base for rbtr return errors} + 20	COPY
"No permission to vary modes from defaults on host %N1",
"You do not have permission to vary modes from the defaults on
the remote server %N1."

E,H{No hosts found} = ROUND(@, 20)
"No hosts found to send to",
"There are no remote hosts set up to send to.
You need to set them up in the system hosts file before you
can use this program to send jobs remotely"

P,H{OK to use only host} ++
"Only one host - OK to use it",
"You only have the host %s to use - is that OK?
I need confirmation before continuing."

E,H{Invalid host name} ++
"Invalid host name",
"The host name you have given is not one which has been set up
in the system hosts file. You need to do this before you can use
a new host name."

E,H{Cannot open socket for remote job send} ++
"Cannot open remote send socket",
"Unable to open socket to connect to remote server for TCP
send of job data."

E,H{Cannot connect to remote} ++
"Cannot connect to remote",
"Unable to open TCP socket to connect to remote server for TCP
send of job data. Possibly xbnetserv has crashed?"

{job fmt title} = 2000 - 'A'

[
Display title abbreviations for jobs (btjlist)
]

P{job fmt title}+'A'		"Arguments"
P{job fmt title}+'C'		"Conditions"
P{job fmt title}+'D'		"Directory"
P{job fmt title}+'E'		"Environment"
P{job fmt title}+'G'		"Group"
P{job fmt title}+'H'		"Title"
P{job fmt title}+'I'		"Cmd Interp"
P{job fmt title}+'L'		"Loadlev"
P{job fmt title}+'M'		"Mode"
P{job fmt title}+'N'		"Jobno"
P{job fmt title}+'O'		"Orighost"
P{job fmt title}+'P'		"Progress"
P{job fmt title}+'R'		"Redirections"
P{job fmt title}+'S'		"Assignments"
P{job fmt title}+'T'		"Date/time"
P{job fmt title}+'U'		"User"
P{job fmt title}+'W'		"Ltime"
P{job fmt title}+'X'		"Exit codes"
P{job fmt title}+'Y'		"Holidays"
P{job fmt title}+'a'		"Avoiding"
P{job fmt title}+'b'		"Stime"
P{job fmt title}+'c'		"Cond"
P{job fmt title}+'d'		"Delt"
P{job fmt title}+'e'		"Export"
P{job fmt title}+'f'		"Etime"
P{job fmt title}+'g'		"Grace"
P{job fmt title}+'h'		"Title"
P{job fmt title}+'i'		"Pid"
P{job fmt title}+'k'		"Ksig"
P{job fmt title}+'l'		"Runtime"
P{job fmt title}+'m'		"Umask"
P{job fmt title}+'o'		"Otime"
P{job fmt title}+'p'		"Pri"
P{job fmt title}+'q'		"Queue"
P{job fmt title}+'r'		"Repeat"
P{job fmt title}+'s'		"Ass vars"
P{job fmt title}+'t'		"Time"
P{job fmt title}+'u'		"Ulimit"
P{job fmt title}+'w'		"Ifnp"
P{job fmt title}+'x'		"Xit"
P{job fmt title}+'y'		"Sig"

[
Display titles more abbreviated for jobs in btq
]

P{job fmt title}+200+'A'	"Args"
P{job fmt title}+200+'C'	"Cond"
P{job fmt title}+200+'D'	"Dir"
P{job fmt title}+200+'E'	"Env"
P{job fmt title}+200+'G'	"Group"
P{job fmt title}+200+'H'	"Title"
P{job fmt title}+200+'I'	"Shell"
P{job fmt title}+200+'L'	" Load"
P{job fmt title}+200+'M'	"Mode"
P{job fmt title}+200+'N'	"Jobno"
P{job fmt title}+200+'O'	"Origh"
P{job fmt title}+200+'P'	"Prog"
P{job fmt title}+200+'R'	"Redirs"
P{job fmt title}+200+'S'	"Assigns"
P{job fmt title}+200+'T'	"Date/Time"
P{job fmt title}+200+'U'	"User"
P{job fmt title}+200+'W'	"Ltime"
P{job fmt title}+200+'X'	"Exits"
P{job fmt title}+200+'Y'	"Hols"
P{job fmt title}+200+'a'	"Avoid"
P{job fmt title}+200+'b'	"Stime"
P{job fmt title}+200+'c'	"Cond"
P{job fmt title}+200+'d'	"Delt"
P{job fmt title}+200+'e'	"Exp"
P{job fmt title}+200+'f'	"Etime"
P{job fmt title}+200+'g'	"Grace"
P{job fmt title}+200+'h'	"Title"
P{job fmt title}+200+'i'	"Pid"
P{job fmt title}+200+'k'	"Ksig"
P{job fmt title}+200+'l'	"Runtime"
P{job fmt title}+200+'m'	"Umask"
P{job fmt title}+200+'n'	"Seq"
P{job fmt title}+200+'o'	"Otime"
P{job fmt title}+200+'p'	"Pri"
P{job fmt title}+200+'q'	"Queue"
P{job fmt title}+200+'r'	"Repeat"
P{job fmt title}+200+'s'	"Ass"
P{job fmt title}+200+'t'	"Time"
P{job fmt title}+200+'u'	"Ulimit"
P{job fmt title}+200+'w'	"Ifnp"
P{job fmt title}+200+'x'	"Xit"
P{job fmt title}+200+'y'	"Sig"

[
Display titles in full for jobs
]

P{job fmt title}+400+'A'	"Arguments"
P{job fmt title}+400+'C'	"Conditions (in full)"
P{job fmt title}+400+'D'	"Directory"
P{job fmt title}+400+'E'	"Environment"
P{job fmt title}+400+'G'	"Group"
P{job fmt title}+400+'H'	"Title (in full)"
P{job fmt title}+400+'I'	"Command Interpreter"
P{job fmt title}+400+'L'	"Load Level"
P{job fmt title}+400+'M'	"Mode"
P{job fmt title}+400+'N'	"Job number"
P{job fmt title}+400+'O'	"Originating host"
P{job fmt title}+400+'P'	"Progress"
P{job fmt title}+400+'R'	"Redirections"
P{job fmt title}+400+'S'	"Assignments (in full)"
P{job fmt title}+400+'T'	"Date and time"
P{job fmt title}+400+'U'	"User"
P{job fmt title}+400+'W'	"Last / Next time"
P{job fmt title}+400+'X'	"Exit codes"
P{job fmt title}+400+'Y'	"Holidays"
P{job fmt title}+400+'a'	"Avoiding"
P{job fmt title}+400+'b'	"Start time"
P{job fmt title}+400+'c'	"Conditions (abbreviated)"
P{job fmt title}+400+'d'	"Delete time"
P{job fmt title}+400+'e'	"Export / Remote runnable"
P{job fmt title}+400+'f'	"End time"
P{job fmt title}+400+'g'	"Grace period"
P{job fmt title}+400+'h'	"Title (no queue name)"
P{job fmt title}+400+'i'	"Process id"
P{job fmt title}+400+'k'	"Signal to kill after run time"
P{job fmt title}+400+'l'	"Maximum run time"
P{job fmt title}+400+'m'	"Umask"
P{job fmt title}+400+'n'	"Sequence"
P{job fmt title}+400+'o'	"Time submitted"
P{job fmt title}+400+'p'	"Priority"
P{job fmt title}+400+'q'	"Queue name"
P{job fmt title}+400+'r'	"Repeat requirement"
P{job fmt title}+400+'s'	"Assignments (abbreviated)"
P{job fmt title}+400+'t'	"Time or date"
P{job fmt title}+400+'u'	"Ulimit"
P{job fmt title}+400+'w'	"If not possible"
P{job fmt title}+400+'x'	"Exit code last run"
P{job fmt title}+400+'y'	"Signal num last run"

{var fmt title} = 2600 - 'A'

[
Display titles abbreviated for variables
]

P{var fmt title}+'C'	"Comment"
P{var fmt title}+'E'	"Exp/Loc"
P{var fmt title}+'G'	"Group"
P{var fmt title}+'K'	"Clust"
P{var fmt title}+'M'	"Mode"
P{var fmt title}+'N'	"Name"
P{var fmt title}+'U'	"User"
P{var fmt title}+'V'	"Value"

[
Display titles in full for variables
]

P{var fmt title}+200+'C'	"Comment"
P{var fmt title}+200+'E'	"Export or Local"
P{var fmt title}+200+'G'	"Group Owner"
P{var fmt title}+200+'K'	"Clustered marker"
P{var fmt title}+200+'M'	"Mode"
P{var fmt title}+200+'N'	"Name of variable"
P{var fmt title}+200+'U'	"User Owner"
P{var fmt title}+200+'V'	"Value of variable"

{Btulist title} = 3400 - 'a'

[
Display titles for btulist
]
P{Btulist title}+'d'-1		"Defp"
P{Btulist title}+'g'-1		"Group"
P{Btulist title}+'i'-1		"Uid"
P{Btulist title}+'j'-1		"Jmode"
P{Btulist title}+'l'-1		"Minp"
P{Btulist title}+'m'-1		"Maxp"
P{Btulist title}+'p'-1		"Privs"
P{Btulist title}+'s'-1		"Specll"
P{Btulist title}+'t'-1		"Totll"
P{Btulist title}+'u'-1		"User"
P{Btulist title}+'v'-1		"Vmode"
P{Btulist title}+'x'-1		"Maxll"

[
Argument syntax for btr
]
A{btr arg explain} = 100		"?,explain"
A{btr arg noverb} ++ COPY		"V,no-verbose"
A{btr arg verb} ++ COPY			"v,verbose"
A{btr arg mail} ++ COPY			"m,mail-message"
A{btr arg write} ++ COPY		"w,write-message"
A{btr arg title} ++ COPY		"h,title"
A{btr arg nomess} ++ COPY		"x,no-message"
A{btr arg interp} ++ COPY		"i,interpreter"
A{btr arg pri} ++ COPY			"p,priority"
A{btr arg ll} ++ COPY			"l,loadlev"
A{btr arg mode} ++ COPY			"M,mode"
A{btr arg norm} ++ COPY			"N,normal"
A{btr arg canc} ++ COPY			"C,cancelled"
A{btr arg notime} ++ COPY		"U,no-time"
A{btr arg time} ++ COPY			"T,time"
A{btr arg norep} ++ COPY		"o,no-repeat"
A{btr arg delete} ++ COPY		"d,delete-at-end"
A{btr arg repeat} ++ COPY		"r,repeat"
A{btr arg avoid} ++ COPY		"A,avoiding-days"
A{btr arg skip} ++ COPY			"S,skip-if-held"
A{btr arg hold} ++ COPY			"H,hold-current"
A{btr arg resched} ++ COPY		"R,reschedule-all"
A{btr arg catchup} ++ COPY		"9,catch-up"
A{btr arg cancio} ++ COPY		"Z,cancel-io"
A{btr arg io} ++ COPY			"I,input-output"
A{btr arg dir} ++ COPY			"D,directory"
A{btr arg cancarg} ++ COPY		"e,cancel-arguments"
A{btr arg argument} ++ COPY		"a,argument"
A{btr arg canccond} ++ COPY		"y,cancel-condition"
A{btr arg cond} ++ COPY			"c,condition"
A{btr arg cancset} ++ COPY		"z,cancel-set"
A{btr arg setflags} ++ COPY		"f,flags-for-set"
A{btr arg set} ++ COPY			"s,set"
A{btr arg umask} ++ COPY		"P,umask"
A{btr arg ulimit} ++ COPY		"L,ulimit"
A{btr arg exits} ++ COPY		"X,exit-code"
A{btr arg adverr} ++ COPY		"j,advance-time-error"
A{btr arg noadverr} ++ COPY		"J,no-advance-time-error"
A{btr arg setu} ++ COPY			"u,set-owner"
A{btr arg setg} ++ COPY			"g,set-group"
A{btr arg loco} ++ COPY			"n,local-only"
A{btr arg export} ++ COPY		"F,export"
A{btr arg fullexport} ++ COPY		"G,full-export"
A{btr arg condcrit} ++ COPY		"k,condition-critical"
A{btr arg nocondcrit} ++ COPY		"K,condition-not-critical"
A{btr arg asscrit} ++ COPY		"b,assignment-critical"
A{btr arg noasscrit} ++ COPY		"B,assignment-not-critical"
A{btr arg queue} ++ COPY		"q,job-queue"
A{btr arg host} ++ COPY			"Q,host"
A{btr arg locenv} ++ COPY		"E,local-environment"
A{btr arg remenv} ++ COPY		"O,remote-environment"
A{btr arg deltime} ++ COPY		"t,delete-time"
A{btr arg runtime} ++ COPY		"Y,run-time"
A{btr arg wsig} ++ COPY			"W,which-signal"
A{btr arg grace} ++ COPY		"2,grace-time"
A{btr arg done} ++ COPY			".,done"
A{btr arg freeze current} ++ COPY	"freeze-current"
A{btr arg freeze home} ++ COPY		"freeze-home"

# Largest argument not including freezes.

S{Largest btr arg} = {btr arg done}

[
Argument syntax for bts
]

A{bts arg explain} = 480                "?,explain"
A{bts arg verbose} ++ COPY              "v,verbose"
A{bts arg quiet} ++ COPY                "q,quiet"
A{bts arg verbose as file} ++ COPY      "f,verbose-as-file"
A{bts arg queuehost} ++ COPY            "Q,host"
A{bts arg host as file} ++ COPY         "F,host-as-file"
A{bts arg cancelled} ++ COPY            "C,cancelled"
A{bts arg normal} ++ COPY               "N,normal"
A{bts arg state as file} ++ COPY        "S,state-as-file"
A{bts arg freeze current} ++ COPY       "freeze-current"
A{bts arg freeze home} ++ COPY          "freeze-home"

[
Argument syntax for btjchange
]

A{btjchange arg explain} = 160	"?,explain"
A{btjchange arg mail} ++ COPY		"m,mail-message"
A{btjchange arg write} ++ COPY		"w,write-message"
A{btjchange arg title} ++ COPY		"h,title"
A{btjchange arg nomess} ++ COPY		"x,no-message"
A{btjchange arg interp} ++ COPY		"i,interpreter"
A{btjchange arg pri} ++ COPY		"p,priority"
A{btjchange arg ll} ++ COPY		"l,loadlev"
A{btjchange arg mode} ++ COPY		"M,mode"
A{btjchange arg norm} ++ COPY		"N,normal"
A{btjchange arg canc} ++ COPY		"C,cancelled"
A{btjchange arg notime} ++ COPY		"U,no-time"
A{btjchange arg time} ++ COPY		"T,time"
A{btjchange arg norep} ++ COPY		"o,no-repeat"
A{btjchange arg delete} ++ COPY		"d,delete-at-end"
A{btjchange arg repeat} ++ COPY		"r,repeat"
A{btjchange arg avoid} ++ COPY		"A,avoiding-days"
A{btjchange arg skip} ++ COPY		"S,skip-if-held"
A{btjchange arg hold} ++ COPY		"H,hold-current"
A{btjchange arg resched} ++ COPY	"R,reschedule-all"
A{btjchange arg catchup} ++ COPY	"9,catch-up"
A{btjchange arg canccond} ++ COPY	"y,cancel-condition"
A{btjchange arg cond} ++ COPY		"c,condition"
A{btjchange arg cancset} ++ COPY	"z,cancel-set"
A{btjchange arg setflags} ++ COPY	"f,flags-for-set"
A{btjchange arg set} ++ COPY		"s,set"
A{btjchange arg setu} ++ COPY		"u,set-owner"
A{btjchange arg setg} ++ COPY		"g,set-group"
A{btjchange arg cancio} ++ COPY		"Z,cancel-io"
A{btjchange arg io} ++ COPY		"I,input-output"
A{btjchange arg dir} ++ COPY		"D,directory"
A{btjchange arg cancarg} ++ COPY	"e,cancel-arguments"
A{btjchange arg argument} ++ COPY	"a,argument"
A{btjchange arg resetenv} ++ COPY	"E,reset-environment"
A{btjchange arg umask} ++ COPY		"P,umask"
A{btjchange arg ulimit} ++ COPY		"L,ulimit"
A{btjchange arg exits} ++ COPY		"X,exit-code"
A{btjchange arg adverr} ++ COPY		"j,advance-time-error"
A{btjchange arg noadverr} ++ COPY	"J,no-advance-time-error"
A{btjchange arg loco} ++ COPY		"n,local-only"
A{btjchange arg export} ++ COPY		"F,export"
A{btjchange arg fullexport} ++ COPY	"G,full-export"
A{btjchange arg condcrit} ++ COPY	"k,condition-critical"
A{btjchange arg nocondcrit} ++ COPY	"K,condition-not-critical"
A{btjchange arg asscrit} ++ COPY	"b,assignment-critical"
A{btjchange arg noasscrit} ++ COPY	"B,assignment-not-critical"
A{btjchange arg queue} ++ COPY		"q,job-queue"
A{btjchange arg deltime} ++ COPY	"t,delete-time"
A{btjchange arg runtime} ++ COPY	"Y,run-time"
A{btjchange arg wsig} ++ COPY		"W,which-signal"
A{btjchange arg grace} ++ COPY		"2,grace-time"
A{btjchange arg freeze current} ++ COPY	"freeze-current"
A{btjchange arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btvar
]

A{btvar arg explain} = 220	"?,explain"
A{btvar arg cancel} ++ COPY		"X,cancel"
A{btvar arg fstring} ++ COPY		"S,force-string"
A{btvar arg create} ++ COPY		"C,create"
A{btvar arg delete} ++ COPY		"D,delete"
A{btvar arg set} ++ COPY		"s,set-value"
A{btvar arg undefval} ++ COPY		"u,undefined-value"
A{btvar arg comment} ++ COPY		"c,comment"
A{btvar arg resetexp} ++ COPY		"N,reset-export"
A{btvar arg loco} ++ COPY		"L,set-local"
A{btvar arg export} ++ COPY		"E,set-export"
A{btvar arg resetclust} ++ COPY		"o,reset-cluster"
A{btvar arg nocluster} ++ COPY		"k,no-cluster"
A{btvar arg cluster} ++ COPY		"K,cluster"
A{btvar arg mode} ++ COPY		"M,set-mode"
A{btvar arg setu} ++ COPY		"U,set-owner"
A{btvar arg setg} ++ COPY		"G,set-group"
A{btvar arg eq} ++ COPY			"e,eq"
A{btvar arg ne} ++ COPY			"n,ne"
A{btvar arg gt} ++ COPY			"g,gt"
A{btvar arg lt} ++ COPY			"l,lt"
A{btvar arg ge} ++ COPY			"ge"
A{btvar arg le} ++ COPY			"le"
A{btvar arg freeze current} ++ COPY	"freeze-current"
A{btvar arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btjdel
]

A{btjdel arg explain} = 260	"?,explain"
A{btjdel arg noforce} ++ COPY		"n,N,no-force"
A{btjdel arg force} ++ COPY		"y,Y,force"
A{btjdel arg nodel} ++ COPY		"k,do-not-delete"
A{btjdel arg del} ++ COPY		"d,delete"
A{btjdel arg wsig} ++ COPY		"K,signal-number"
A{btjdel arg sleep} ++ COPY		"S,sleep-time"
A{btjdel arg unqueue} ++ COPY		"u,unqueue"
A{btjdel arg nounqueue} ++ COPY		"e,do-not-unqueue"
A{btjdel arg jobpref} ++ COPY		"J,job-prefix"
A{btjdel arg cmdpref} ++ COPY		"C,command-prefix"
A{btjdel arg dir} ++ COPY		"D,directory"
A{btjdel arg xml unq} ++ COPY           "X,xml-unqueue"
A{btjdel arg freeze current} ++ COPY	"freeze-current"
A{btjdel arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btjlist
]

A{btjlist arg explain} = 280	"?,explain"
A{btjlist arg loco} ++ COPY		"L,local-only"
A{btjlist arg execrems} ++ COPY		"r,include-exec-remotes"
A{btjlist arg allrems} ++ COPY		"R,include-all-remotes"
A{btjlist arg queue} ++ COPY		"q,job-queue"
A{btjlist arg nosort} ++ COPY		"n,no-sort"
A{btjlist arg sort} ++ COPY		"s,sort"
A{btjlist arg stimes} ++ COPY		"S,short-times"
A{btjlist arg fulltimes} ++ COPY	"T,full-times"
A{btjlist arg format} ++ COPY		"F,format"
A{btjlist arg deffmt} ++ COPY		"D,default-format"
A{btjlist arg hdr} ++ COPY		"H,header"
A{btjlist arg nohdr} ++ COPY		"N,no-header"
A{btjlist arg justu} ++ COPY		"u,just-user"
A{btjlist arg justg} ++ COPY		"g,just-group"
A{btjlist arg nullq} ++ COPY		"z,null-queues"
A{btjlist arg nonullq} ++ COPY		"Z,no-null-queues"
A{btjlist arg bypassm} ++ COPY		"B,bypass-modes"
A{btjlist arg view} ++ COPY		"V,view-jobs"
A{btjlist arg no view} ++ COPY		"l,no-view-jobs"
A{btjlist arg freeze current} ++ COPY	"freeze-current"
A{btjlist arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btvlist
]

A{btvlist arg explain} = 320	"?,explain"
A{btvlist arg loco} ++ COPY		"L,local-only"
A{btvlist arg inclrems} ++ COPY		"R,include-remotes"
A{btvlist arg format} ++ COPY		"F,format"
A{btvlist arg deffmt} ++ COPY		"D,default-format"
A{btvlist arg hdr} ++ COPY		"H,header"
A{btvlist arg nohdr} ++ COPY		"N,no-header"
A{btvlist arg justu} ++ COPY		"u,just-user"
A{btvlist arg justg} ++ COPY		"g,just-group"
A{btvlist arg bypassm} ++ COPY		"B,bypass-modes"
A{btvlist arg freeze current} ++ COPY	"freeze-current"
A{btvlist arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btulist
]

A{btulist arg explain} = 340	"?,explain"
A{btulist arg sort user} ++ COPY	"u,user-name-sort"
A{btulist arg sort group} ++ COPY	"g,group-name-sort"
A{btulist arg sort uid} ++ COPY		"n,numeric-user-sort"
A{btulist default line} ++ COPY		"d,default-line"
A{btulist no default line} ++ COPY	"s,no-default-line"
A{btulist user lines} ++ COPY		"U,user-lines"
A{btulist no user lines} ++ COPY	"S,no-user-lines"
A{btulist format} ++ COPY		"F,format"
A{btulist default format} ++ COPY	"D,default-format"
A{btulist header} ++ COPY		"H,header"
A{btulist no header} ++ COPY		"N,no-header"
A{btulist arg freeze current} ++ COPY	"freeze-current"
A{btulist arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btuchange
]

A{btuchange arg explain} = 360		"?,explain"
A{btuchange arg setdefs} ++ COPY		"D,set-defaults"
A{btuchange arg setusers} ++ COPY		"u,set-users"
A{btuchange arg copydefs} ++ COPY		"A,copy-defaults"
A{btuchange arg nocopydefs} ++ COPY		"s,no-copy-defaults"
A{btuchange arg minpri} ++ COPY			"l,min-priority"
A{btuchange arg defpri} ++ COPY			"d,default-priority"
A{btuchange arg maxpri} ++ COPY			"m,max-priority"
A{btuchange arg maxll} ++ COPY			"M,max-load-level"
A{btuchange arg totll} ++ COPY			"T,total-load-level"
A{btuchange arg specll} ++ COPY			"S,special-load-level"
A{btuchange arg privs} ++ COPY			"p,privileges"
A{btuchange arg jobmode} ++ COPY		"J,job-mode"
A{btuchange arg varmode} ++ COPY		"V,var-mode"
A{btuchange arg rebuild} ++ COPY		"R,rebuild-file"
A{btuchange arg norebuild} ++ COPY		"N,no-rebuild"
A{btuchange dump pw} ++ COPY			"X,dump-passwd"
A{btuchange aswas pw} ++ COPY			"Y,default-passwd"
A{btuchange kill pw} ++ COPY			"Z,kill-dump-passwd"
A{btuchange arg freeze current} ++ COPY		"freeze-current"
A{btuchange arg freeze home} ++ COPY		"freeze-home"

[
Argument syntax for btcilist
]

A{btcilist arg explain} = 390		"?,explain"
A{btcilist arg host} ++ COPY			"Q,host"
A{btcilist arg freeze current} ++ COPY		"freeze-current"
A{btcilist arg freeze home} ++ COPY		"freeze-home"

[
Argument syntax for btcichange
]

A{btcichange arg explain} = 400         	"?,explain"
A{btcichange arg add} ++ COPY			"A,add"
A{btcichange arg delete} ++ COPY		"D,delete"
A{btcichange arg update} ++ COPY		"U,update"
A{btcichange arg nice} ++ COPY			"N,nice"
A{btcichange arg ll} ++ COPY			"L,load-level"
A{btcichange arg newname} ++ COPY		"n,new-name"
A{btcichange arg path} ++ COPY			"p,path"
A{btcichange arg args} ++ COPY			"a,args"
A{btcichange arg a0title} ++ COPY		"t,set-arg0-title"
A{btcichange arg a0name} ++ COPY		"i,set-arg0-name"
A{btcichange arg noexpand} ++ COPY		"u,no-expand-args"
A{btcichange arg expand} ++ COPY		"e,expand-args"
A{btcichange arg freeze current} ++ COPY	"freeze-current"
A{btcichange arg freeze home} ++ COPY		"freeze-home"

[
Argument defs for btjstat
]
A{btjstat arg explain} = 440	"?,explain"
A{btjstat arg defaults} ++ COPY		"d,default-states"
A{btjstat arg statcodes} ++ COPY	"s,state"
A{btjstat arg freeze current} ++ COPY	"freeze-current"
A{btjstat arg freeze home} ++ COPY	"freeze-home"

[
Argument defs for btstart
]

A{btstart arg explain} = 460	"?,explain"
A{btstart arg initll} ++ COPY		"l,initial-load-level"
A{btstart arg jsize} ++ COPY		"j,initial-job-size"
A{btstart arg vsize} ++ COPY		"v,initial-var-size"
A{btstart arg decpri} ++ COPY		"d,decrement-pri"
A{btstart arg nosetpgrp} ++ COPY	"t,no-setpgrp"
A{btstart arg setpgrp} ++ COPY		"T,setpgrp"
A{btstart arg freeze current} ++ COPY	"freeze-current"
A{btstart arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for bthols
]

A{bthols arg explain} = 470     	"?,explain"
A{bthols arg clear} ++ COPY		"C,clear"
A{bthols arg noclear} ++ COPY		"r,no-clear"
A{bthols arg display} ++ COPY		"d,display"
A{bthols arg set} ++ COPY		"s,set"
A{bthols arg freeze current} ++ COPY	"freeze-current"
A{bthols arg freeze home} ++ COPY	"freeze-home"

[
Argument syntax for btq
]

A{btq arg explain} = 200		"?,explain"
A{btq arg confdel} ++			"a,confirm-delete"
A{btq arg noconfdel} ++			"A,no-confirm-delete"
A{btq arg cursor keep} ++		"s,keep-cursor"
A{btq arg cursor follow} ++		"N,follow-job"
A{btq arg jobs screen} ++		"j,jobs-screen"
A{btq arg vars screen} ++		"v,vars-screen"
A{btq arg keepchar} ++			"H,keep-char-help"
A{btq arg losechar} ++			"h,lose-char-help"
A{btq arg help box} ++			"b,help-box"
A{btq arg no help hox} ++		"B,no-help-hox"
A{btq arg error box} ++			"e,error-box"
A{btq arg no error box} ++		"E,no-error-box"
A{btq arg local} ++			"l,local-only"
A{btq arg network} ++			"r,network-wide"
A{btq arg jobqueue} ++			"q,job-queue"
A{btq arg justu} ++			"u,just-user"
A{btq arg justg} ++			"g,just-group"
A{btq arg nullqs} ++			"z,null-queues"
A{btq arg no nullqs} ++			"Z,no-null-queues"
A{btq arg XML fmt} ++                   "X,xml-format"
A{btq arg no XML fmt} ++                "D,no-xml-format"

[
Argument defs for btuser
]
A{btuser arg explain} = 300		"?,explain"
A{btuser arg display} ++ COPY		"d,display-only"
A{btuser arg setdef mode} ++ COPY	"m,set-default-modes"
A{btuser arg view users} ++ COPY	"v,view-users"
A{btuser arg update users} ++ COPY	"i,update-users"
A{btuser arg sort user} ++ COPY		"u,user-name-sort"
A{btuser arg sort group} ++ COPY	"g,group-name-sort"
A{btuser arg sort uid} ++ COPY		"n,numeric-user-sort"
A{btuser arg losechar} ++ COPY		"h,lose-char-help"
A{btuser arg keepchar} ++ COPY		"H,keep-char-help"
A{btuser arg help box} ++ COPY		"b,help-box"
A{btuser arg no help box} ++ COPY	"B,no-help-box"
A{btuser arg error box} ++ COPY		"e,error-box"
A{btuser arg no error box} ++ COPY	"E,no-error-box"

[
Argument defs for btfilemon
]

A{btfilemon arg explain} = 100		"?,explain"
A{btfilemon arg daemon} ++ COPY		"d,daemon-process"
A{btfilemon arg nodaemon} ++ COPY	"n,not-daemon"
A{btfilemon arg directory} ++ COPY	"D,directory"
A{btfilemon arg script file} ++ COPY	"X,script-file"
A{btfilemon arg command} ++ COPY	"c,script-command"
A{btfilemon arg anyfile} ++ COPY	"a,any-file"
A{btfilemon arg pattern file} ++ COPY	"p,pattern-file"
A{btfilemon arg given file} ++ COPY	"s,specific-file"
A{btfilemon arg arrival} ++ COPY	"A,file-arrives"
A{btfilemon arg grow time} ++ COPY	"G,file-stops-growing"
A{btfilemon arg mod time} ++ COPY	"M,file-stops-writing"
A{btfilemon arg change time} ++ COPY	"I,file-stops-changing"
A{btfilemon arg access time} ++ COPY	"u,file-stops-use"
A{btfilemon arg file removed} ++ COPY	"r,file-deleted"
A{btfilemon arg halt found} ++ COPY	"S,halt-when-found"
A{btfilemon arg cont found} ++ COPY	"C,continue-running"
A{btfilemon arg ignore existing} ++ COPY	"i,ignore-existing"
A{btfilemon arg include existing} ++ COPY	"e,include-existing"
A{btfilemon arg poll time} ++ COPY	"P,poll-time"
A{btfilemon arg nonrecursive} ++ COPY	"z,non-recursive"
A{btfilemon arg recursive} ++ COPY	"R,recursive"
A{btfilemon arg no links} ++ COPY	"Z,dont-follow-links"
A{btfilemon arg follow links} ++ COPY	"L,follow-links"
A{btfilemon arg run monitor} ++ COPY	"m,run-monitor"
A{btfilemon arg list monitor} ++ COPY	"l,list-processes"
A{btfilemon arg kill proc} ++ COPY	"k,kill-processes"
A{btfilemon arg killall} ++ COPY	"K,kill-all"
A{btfilemon arg freeze current} ++ COPY	"freeze-current"
A{btfilemon arg freeze home} ++ COPY	"freeze-home"

[
Usage options - btr
]
E{btr explain} = 1
"%P        [ -BCFGHJKNRSUVZbdkjmnovwxyz ]
          [ -a <Argument> ]
          [ -c <Condition spec> ]
          [ -f <Flags for set> ]
          [ -g group ] [ -h <Heading> ]
          [ -I <I/O spec> ]
          [ -i <Interpreter> ]
          [ -l <Load level> ] [ -L ulimit ]
          [ -M <Mode spec> ]
          [ -P umask ] [ -p <Priority> ]
          [ -r <Repeat specification> ]
          [ -s <Set variables> ]
          [ -T <Time spec> ]
          [ -u owner ]"

[
Usage options - btjchange
]
E{btjchange explain} ++
"%P        [ -BCFGHJKNRSUZbdejkmnoqwvxyz ]
          [ -A avoiding ]          [ -M <Mode spec> ]
          [ -T <Time spec> ]       [ -c <Condition spec> ]
          [ -f <Flags for set> ]   [ -h <Heading> ]
          [ -i <Interpreter> ]     [ -l <Load level> ]
          [ -p <Priority> ]        [ -r <Repeat specification> ]
          [ -s <Set variables> ]   [ -u <Owner> ]
          [ -g <Group> ]           [ -D <Directory> ]
          [ -a <Argument> ]        [ -I <I/O Specification ]
          [ -L <Filesize> ]        [ -P <Umask> ]
          [ -X <Exitcode> ]"

[
Usage options - btjdel
]
E{btjdel explain} ++
"%P [args] job [job...]
Args are:
     -n or -N      do not ""force""
     -y or -Y      ""force"" i.e. try to kill jobs
           -k      kill, but do not delete jobs
           -d      cancel -k
           -K n    specify signal number to kill with.
                   (default is 15, i.e. SIGTERM)
           -S n    sleep for n seconds waiting for jobs to die
           -u      Unqueue job to current directory
                   (do not delete if -k given)
           -X      Unqueue as single XML file
           -e      Cancel unqueue
      -J f -C f    Reset job/file prefixes (suffix job number)
           -K n    Send a kill to the job
           -D d    specify the working directory"

[
Usage options - btjlist
]
E{btjlist explain} ++
"%P options are:
      -L   limit display to local jobs only
      -r   include remote jobs which can be run locally
      -R   include all remote jobs
      -n   do not sort
      -s   sort jobs into time order
      -q name      limit display to job queue name
      -q -         remove limit on display
      -z -Z  include/exclude null queue names
      -S   display times of current days jobs, otherwise dates
      -T   display full date and time of each job
      -F   specify format string
      -D   restore default format
      -H   provide header to output
      -N   suppress header
      -u user -g group limit display to user/group
      -V -l display/not job files rather than parameters"

[
Usage options - btvlist
]
E{btvlist explain} ++
"%P options are:
      -L   limit display to local variables only
      -R   include all remote variables
      -F   specify format string
      -D   restore default format
      -H   provide header to output
      -N   suppress header
      -u user -g group limit display to user/group"

[
Usage options - btvar
]
E{btvar explain} ++
"%P options are:
   -S        force to string
   -D        delete variable
   -C        create variable
   -X        clear other arguments
   -M flags  set mode
   -u value  treat undefined as having given value
   -s value  assign value
   -c comment  assign comment
   -N        reset export
   -L        set local
   -E        set export
   +eq/+ne/+lt/+le/+gt/+ge value  test against value"

[
Usage options - btulist
]
E{btulist explain} ++
"%P options are
   -u      sort by user
   -g      sort by group
   -n      sort by user id."

[
Usage options - btuchange
]
E{btuchange explain} ++
"%P options
    -D      Set defaults
    -u      Set users (all or named)
    -A      copy defaults to all users
    -s      Do not copy defaults
    -l n -d n -m n Set min, default, max priority
    -M n -T n -S n Set min, total, special loadlevs
    -p nn   Set privileges
    -J nn -V nn  Set modes job/var
    -R      Rebuild file if needed
Follow by named users or none to set all users"

[
Usage options - btcilist
]
E{btcilist explain} ++
"    -Q hostname  Display options for specified host"

[
Usage option - btcichange
]
E{btcichange explain} ++
"%P options
    -A -D -U       add delete update
    -N n           nice
    -L n           load level
    -p /path/name  path
    -n name        new name
    -a string      args (: to reset)
    -t -i          set job arg0 to title/ci name
    -e -u          expand numeric args internally/in c.i."

[
Usage options - btcharge
]
E{btcharge explain} ++ "%P has been deprecated as charges are not supported any more"

[
Usage options - btjstat
]
E{btjstat explain} ++
"   %P [-d] [-s states] jobnumber
    return exit code of 0 if job is in given state(s), 1 otherwise.
    default state is 'run'."

[
Usage options - bts
]

E{bts explain} = 19
"%P [options] files:

-v verbose (display job numbers)
-q quiet (do not display)
-f display job numbers if set in file
-Q host set host name to queue to - for local host
-F host as set in file
-C cancelled
-N normal
-S state of job as in file"

[
Usage options - bthols
]
E{bthols explain} = 20
"%P [options] year [file]:

-C (setting only) Clear existing holidays
-r Retain existing holidays, add new
-d Display holidays (default)
-s Set holidays"

[
Usage options - btfilemon
]

E{btfilemon explain} = 100
"%P options:

-m/-l/-k/-K normal, list, kill, kill all
-d/-n   Run/do not run as detached process
-a      Detect any file
-p pattern      Detect file of given pattern (use quotes)
-s file         Detect file of given name
-A      Detect arrival only
-r	Detect deletion only
-G/-M/-I/-u secs   Detect file not grown/written/changed/used for given time
-S/-C   Halt/do not halt on first file found
-i/-e   Ignore/do not ignore existing files
-R/-z   Do/do not Recursively apply to subdirectories
-L/-Z   Do/do not follow symbolic links
-D dir  Specify directory
-X file Specify shell script
-c cmd  Specify command
-P secs Specify poll time"

E{btfilemon invalid time field} ++	"Invalid time field %s. Field must be 1 to 3600"
E{filemon cannot select directory} ++	"Cannot select directory %s - aborting"
E{filemon cannot read directory} ++	"Cannot read directory %s - aborting"
E{Invalid monitor directory} ++		"Invalid monitor directory %s"
E{Invalid script file} ++		"Invalid script file %s"
E{Extra arguments not expected} ++	"%P does not take additional arguments"
E{filemon no script file given} ++	"Script file/cmd not given to %P. Use -X or -c arg."
E{filemon duplicated name} ++		"Confused by duplicated name in file monitor"

[
Messages for xmfilemon
]

H{xmfilemon dir sel dialog} = ROUND(@, 10)
"To select a new directory, select in the display (or use the .. entry
to go back) and press Filter. Repeat as necessary and press OK"

H{xmfilemon main help} ++
"Select appropriate items to select files for monitoring
and the type of monitoring required."

[
Additional messages for btfilemon
]

E{Race to create fm shm segment} = ROUND(@, 10)
"Cannot create shared memory segment for file monitor.
This is probably a temporary condition which may be
resolved if the program is re-run."

E{Cannot create fm shm segment} ++
"Cannot create shared memory segment for file monitor.
Error reported is %E."

E{Cannot attach fm shm segment} ++
"Cannot attach file monitor shm segment.
Error reported is %E."

E{Cannot create fm semaphore} ++
"Cannot create semaphore to lock shared memory segment
for file monitor. Error reported is %E."

E{Could not reset fm semaphore} ++
"Unable to initialise semaphore for file monitor.
Error reported is %E."

E{Cannot open fm semaphore} ++
"Cannot open semaphore for shared memory lock.
Error reported was %E."

E{Cannot read lock fm semaphore} ++
"Cannot lock file monitor shm for reading.
Error reported was %E."

E{Cannot write lock fm semaphore} ++
"Cannot lock file monitor shm for writing.
Error reported was %E."

E{FM Directory name clash} ++
"Directory %s is already being monitored by
another file monitor process pid %d1."

E{Too many file monitors} ++
"There are too many file monitor processes
already running on this machine.
Please kill some off first."

E{Too many fm kill args} ++
"Too many arguments for process kill."

(0)R{File monitor types} = ROUND(@, 10)	"App"
(1)Q{File monitor types} COPY		"Grow"
(2)Q{File monitor types} COPY		"Write"
(3)Q{File monitor types} COPY		"Change"
(4)Q{File monitor types} COPY		"Use"
(5)Q{File monitor types} COPY		"Rem"

[
Bthols messages
]

E{Bthols missing args} = 390 "Argument(s) missing - need year"
E{Bthols bad arg} ++ "Bad argument expecting year"
E{Bthols bad year} ++ "Invalid year %d0"
E{Bthols cannot read} ++ "Cannot open %s for reading"
E{Bthols cannot write} ++ "Cannot open %s for writing"
E{Bthols format error} ++ "Input file format error, expecting month: days"
E{Bthols invalid month} ++  "Unknown month %s"
E{Bthols invalid day in month} ++ "Invalid day number %d0 in month %s"
E{Bthols write error} ++ "Holiday write file error %E" 

######################################################################
######################################################################
##  Mostly btq messages + common stuff
##

# Global keys

[
Keys global to all states in screen-based progs
]

K{key help} = 400		"?"
K{key refresh} ++		"^L"
K{key abort} ++			"\e"
K{key guess} ++			"\s"
K{key eol} ++			"\r"
K{key halt} ++			"Q,q"
K{key cursor up} ++		"k,\kUP"
K{key cursor down} ++		"j,\kDOWN"
K{key screen up} ++		"^B"
K{key screen down} ++		"^F"
K{key half screen up} ++	"^U"
K{key half screen down} ++	"^D"
K{key next field} ++		"\t"
K{key previous field} ++	"\e\t,\e[Z"
K{key top} ++			"B,\kHOME"
K{key bottom} ++		"e"
K{key erase} = 420		"\kERASE"
K{key kill} ++			"\kKILL"
K{key save opts} = 430		"$"
K{key search forward} = 440	"^^"
K{key search backward} ++	"\\"
K{key stop} = 490		"^Z"

[
Keys for displaying job list
]

H,S{btq job list state} = 1
"?            Help (this screen)
^L           Refresh
q            Quit
V X          Variables, command interpreter list
k j B e      (Or arrow keys) up/down beginning/end
^B ^F ^U ^D  Screen up/down, half screen up/down
D            Delete job
M O G        Change mode owner group
\"            Change title
p l x        Change priority, load level command interp
P            Reset progress code
r z          Set runnable, cancelled
g f          Run if possible ignoring time adv time/no adv time
I A E R u    View job, edit args, env, I/O, process params
t a          Time settings / adv to next
C S F        Condition variables, assignments, Mail/write flags
K            Kill job
U            Unqueue job
H            Set/view holidays
^ \          Forward Reverse search job title
,            Alter display settings"

({btq job list state})	K	{btq jlist key delete} = 500	"D"
({btq job list state})	K	{btq jlist key chmod} ++	"M"
({btq job list state})	K	{btq jlist key chown} ++	"O"
({btq job list state})	K	{btq jlist key chgrp} ++	"G"
({btq job list state})	K	{btq jlist key title} ++	"\""
({btq job list state})	K	{btq jlist key prio} ++		"p"
({btq job list state})	K	{btq jlist key ll} ++		"l"
({btq job list state})	K	{btq jlist key time} ++		"t"
({btq job list state})	K	{btq jlist key view} ++		"I"
({btq job list state})	K	{btq jlist key cvars} ++	"C"
({btq job list state})	K	{btq jlist key svars} ++	"S"
({btq job list state})	K	{btq jlist key setcmdint} ++	"x"
({btq job list state})	K	{btq jlist key cmdint} ++	"X"
({btq job list state})	K	{btq jlist key kill} ++		"K"
({btq job list state})	K	{btq jlist key setstatus} ++	"P"
({btq job list state})	K	{btq jlist key vars} ++		"V"
({btq job list state})	K	{btq jlist key mwflags} ++	"F"
({btq job list state})	K	{btq jlist key unqueue} ++	"U"
({btq job list state})	K	{btq jlist key setrunn} ++	"r"
({btq job list state})	K	{btq jlist key setcanc} ++	"z"
({btq job list state})	K	{btq jlist key goj} ++		"g"
({btq job list state})	K	{btq jlist key edargs} ++	"A"
({btq job list state})	K	{btq jlist key edenv} ++	"E"
({btq job list state})	K	{btq jlist key edredir} ++	"R"
({btq job list state})	K	{btq jlist key procenv} ++	"u"
({btq job list state})	K	{btq jlist key holidays} ++	"H"
({btq job list state})	K	{btq jlist key gojna} ++	"f"
({btq job list state})	K	{btq jlist key advnext} ++	"a"
({btq job list state})	K	{btq jlist key setformat} ++	"\,"

({btq job list state})	K	{key exec} = 700		"@"

[
Keys for displaying var list
]

H,S{btq var list state} = 2
"?            Help (this screen)
^L           Refresh
q            Quit
J            Jobs
k j          (Or arrow keys) up/down
B e          Beginning/end
^B ^F        Screen up/down
^U ^D        Half screen up/down
C D A        Create/delete/assign var
M O G        Change mode owner group
\"            Change comment
R            Rename
=            Set arith constant
             (currently %d7)
+ - * / %%    Arithmetic with %d7
E L ~        Set export/local toggle
U K &        Set cluster/local toggle
^ \          Forward/reverse search name comment
,            Alter display settings"

({btq var list state})	K	{btq vlist key create} = 500	"C"
({btq var list state})	K	{btq vlist key delete} ++	"D"
({btq var list state})	K	{btq vlist key assign} ++	"A"
({btq var list state})	K	{btq vlist key chmod} ++	"M"
({btq var list state})	K	{btq vlist key chown} ++	"O"
({btq var list state})	K	{btq vlist key chgrp} ++	"G"
({btq var list state})	K	{btq vlist key chcomm} ++	"\""
({btq var list state})	K	{btq vlist key rename} ++	"R"
({btq var list state})	K	{btq vlist key setconst} ++	"="
({btq var list state})	K	{btq vlist key add} ++		"+"
({btq var list state})	K	{btq vlist key sub} ++		"-"
({btq var list state})	K	{btq vlist key mult} ++		"*"
({btq var list state})	K	{btq vlist key div} ++		"/"
({btq var list state})	K	{btq vlist key mod} ++		"%"
({btq var list state})	K	{btq vlist key sexport} ++	"E"
({btq var list state})	K	{btq vlist key usexport} ++	"L"
({btq var list state})	K	{btq vlist key togexport} ++	"~,!"
({btq var list state})	K	{btq vlist key jobs} ++		"J"
({btq var list state})	K	{btq vlist key holidays} ++	"H"
({btq var list state})	K	{btq vlist key fmt1} ++		"\,"
({btq var list state})	K	{btq vlist key fmt2} ++		";"
({btq var list state})	K	{btq vlist key scluster} ++	"U"
({btq var list state})	K	{btq vlist key uscluster} ++	"K"
({btq var list state})	K	{btq vlist key togcluster} ++	"&"

[
Initial value for constant
]

N{Initial constant value} = 900

({Initial constant value})	N	1

[
Keys relating to job and var deletion
]

P,H,E,S{btq job del state} = 3
"Confirm delete job %s: ",
"Please type Y if you want to delete this
Job `%s' (jobno %d0), otherwise N.",
"Unknown command, please type Y to delete the job or N to leave it"

({btq job del state})	K	{Confirm delete OK} = 500	"y,Y,t,T"
({btq job del state})	K	{Confirm delete cancel} ++	"n,N,f,F"

P,H,E,S{btq var del state} = 4
"Are you sure you want to delete variable %s? ",
"Please type Y if you want to delete this
Variable `%s', otherwise N to leave it",
"Unknown command, please type Y to delete the variable from the
job or N to leave it"

({btq var del state})	K	{Confirm delete OK}		"y,Y,t,T"
({btq var del state})	K	{Confirm delete cancel}		"n,N,f,F"

[
Set job and variable modes
]

H,S{btq job modes state} = 5
"Job modes - move the cursor up/down
left/right with cursor keys h/j/k/l
and type Y T to set N F to unset
! or ~ to invert"

({btq job modes state})	K	{btq key mode left} = 560	"h,\kLEFT"
({btq job modes state})	K	{btq key mode right} ++		"l,\kRIGHT"
({btq job modes state})	K	{btq key mode set} ++		"y,Y,t,T"
({btq job modes state})	K	{btq key mode unset} ++		"n,N,f,F"
({btq job modes state})	K	{btq key mode toggle} ++	"~,!"

H,S{btq var modes state} = 6
"Var modes - move the cursor up/down
left/right with cursor keys h/j/k/l
and type Y T to set N F to unset
! or ~ to invert"

({btq var modes state})	K	{btq key mode left}		"h,\kLEFT"
({btq var modes state})	K	{btq key mode right}		"l,\kRIGHT"
({btq var modes state})	K	{btq key mode set}		"y,Y,t,T"
({btq var modes state})	K	{btq key mode unset}		"n,N,f,F"
({btq var modes state})	K	{btq key mode toggle}		"~,!"

[
Interrogate jobs
]
H,S{btq view jobs state} = 7
"Interrogate move window left or right
with cursors. B or E to select beginning
or end < and > for lh and rh margin."

({btq view jobs state})	K	{btq key view left} = 550	"h,\kLEFT"
({btq view jobs state})	K	{btq key view right} ++		"l,\kRIGHT"
({btq view jobs state})	K	{btq key view lmarg} ++		"<"
({btq view jobs state})	K	{btq key view rmarg} ++		">"
# ({btq view jobs state})	K	{btq key top} = {key top}	"B"
# ({btq view jobs state})	K	{btq key bottom} = {key bottom}	"E"

[
Delete conditions and assignments
]
P,H,E,S{btq delete cond state} = 8
"Confirm delete condition with %s: ",
"Please indicate (Y or N) that you
want to delete this condition.",
"Invalid response, please confirm (Y) or deny (N) that you want
to delete this condition."

({btq delete cond state}) K	{btq delete job var OK} = 550	"y,Y,t,T"
({btq delete cond state}) K	{btq delete job var cancel} ++	"n,N,f,F"

P,H,E,S{btq delete ass state} = 9
"Confirm delete assign with %s: ",
"Please indicate (Y or N) that you
want to delete this assignment.",
"Invalid response, please confirm (Y) or deny (N) that you want
to delete this assignment."

({btq delete ass state}) K	{btq delete job var OK} 	"y,Y,t,T"
({btq delete ass state}) K	{btq delete job var cancel}	"n,N,f,F"

[
Add conditions and assignments
]

H,S{btq add cond state} = 10
"a   Add new condition
D   Delete condition
V   Change variable
C = < > ! Change condition
N   Change constant to compare with
c   Toggle ""critical"" mark for remotes"

({btq add cond state})	K	{btq key cond add} = 500	"a"
({btq add cond state})	K	{btq key cond delete} ++	"D"
({btq add cond state})	K	{btq key cond chg var} ++	"V"
({btq add cond state})	K	{btq key cond chg comp} ++	"C,=,<,>,!"
({btq add cond state})	K	{btq key cond chg const} ++	"N"
({btq add cond state})	K	{btq key cond tog crit} ++	"c"

H,S{btq add ass state} = 11
"Variables to be set by start or end of job

?           Help (this screen)
^L          Refresh
k j         Or arrow keys, move up/down
b           Beginning
e           End
q           Quit back to job screen
a D         Add, delete new variable
V = N       Set variable, operation, value
S O E A C   Adjust Start Ok Error Abort Canc
R           Adjust reverse
            flags
c   Toggle ""critical"" mark for remotes"

({btq add ass state})	K	{btq key ass add} = 500		"a"
({btq add ass state})	K	{btq key ass delete} ++		"D"
({btq add ass state})	K	{btq key ass chg var} ++	"V"
({btq add ass state})	K	{btq key ass chg ass} ++	"="
({btq add ass state})	K	{btq key ass chg const} ++	"N"
({btq add ass state})	K	{btq key ass tog crit} ++	"c"
({btq add ass state})	K	{btq key ass flg start} ++	"S"
({btq add ass state})	K	{btq key ass flg rev} ++	"R"
({btq add ass state})	K	{btq key ass flg norm} ++ 	"O"
({btq add ass state})	K	{btq key ass flg err} ++	"E"
({btq add ass state})	K	{btq key ass flg abort} ++	"A"
({btq add ass state})	K	{btq key ass flg canc} ++	"C"

[
Keys to change flags on setting vars
]

S{btq ass flag state} = 12

({btq ass flag state})	K	{btq key ass flag set} = 500	"t,T,y,Y,s,S"
({btq ass flag state})	K	{btq key ass flag unset} ++	"f,F,n,N,u,U"
({btq ass flag state})	K	{btq key ass flag toggle} ++	"~,!"

[
Edit command interpreter list
]

H,S{btq edit command ints} = 13
"Command interpreters

?       Help (this screen)
^L      Redraw
q       Quit
j k     Down, up (or arrows)
A       Add new
D       Delete
L       Set load level
N       Set name
P       Set path
n       Set nice
a       Set predefined arguments
0       (Zero) toggle ""arg 0"" flag
        Causes job title to be assigned to
        Arg 0 if set.
e	toggle ""expand args"" flag
	Do not let command interpreter expand
	arguments."

({btq edit command ints})	K	{btq key add ci} = 500	"A"
({btq edit command ints})	K	{btq key delete ci} ++	"D"
({btq edit command ints})	K	{btq key ci ll} ++	"L"
({btq edit command ints})	K	{btq key ci name} ++	"N"
({btq edit command ints})	K	{btq key ci path} ++	"P"
({btq edit command ints})	K	{btq key ci nice} ++	"n"
({btq edit command ints})	K	{btq key ci args} ++	"a"
({btq edit command ints})	K	{btq key ci arg0} ++	"0"
({btq edit command ints})	K	{btq key ci expand} ++	"E"

[
Delete of command interps
]

P,H,E,S{btq confirm del ci} = 14
"Confirm delete %s? ",
"Please confirm (Y or N) that you want to
delete this interpreter.",
"Unknown command - please reply Y or N
to confirm or cancel delete of command interp"

({btq confirm del ci})		K	{Confirm delete OK}	"y,Y,t,T"
({btq confirm del ci})		K	{Confirm delete cancel}	"n,N,f,F"

[
Set mail/write flags
]

H,E,S{btq set message flags} = 15
"T Y S    Set flag
F N U    Unset flag
! ~      Toggle",
"Unknown command. Please enter flag settings or press RETURN."

({btq set message flags})	K	{btq key set msg flag} = 500	"t,T,y,Y,s,S"
({btq set message flags})	K	{btq key unset msg flag} ++	"f,F,n,N,u,U"
({btq set message flags})	K	{btq key toggle msg flag} ++	"~,!"

[
Arguments
]

H,S{btq cmd args state} = 16
"i a         Insert before/after
d           Delete
m c         Mark for move/copy
E           Edit current
q           Quit back to job"

({btq cmd args state})		K	{btq key cedit insert} = 500	"i"
({btq cmd args state})		K	{btq key cedit add} ++		"a"
({btq cmd args state})		K	{btq key cedit delete} ++	"d"
({btq cmd args state})		K	{btq key cedit move} ++		"m"
({btq cmd args state})		K	{btq key cedit copy} ++		"c"
({btq cmd args state})		K	{btq key cedit edit} ++		"E"

[
Set environment vars
]

H,S{btq job env state} = 17
"i a         Insert before/after
d           Delete
m           Mark for move
N V         Edit current
q           Quit back to job"

({btq job env state})		K	{btq key cedit insert}		"i"
({btq job env state})		K	{btq key cedit add}		"a"
({btq job env state})		K	{btq key cedit delete}		"d"
({btq job env state})		K	{btq key cedit move}		"m"
({btq job env state})		K	{btq key env name} ++		"N"
({btq job env state})		K	{btq key env value} ++		"V"

[
Set redirections
]

H,S{btq job redir state} = 18
"i a         Insert before/after
d           Delete
m           Mark for move
N A F       Edit file number/action/file
q           Quit back to job"

({btq job redir state})		K	{btq key cedit insert}		"i"
({btq job redir state})		K	{btq key cedit add}		"a"
({btq job redir state})		K	{btq key cedit delete}		"d"
({btq job redir state})		K	{btq key cedit move}		"m"
({btq job redir state})		K	{btq key cedit redir num} ++	"N"
({btq job redir state})		K	{btq key cedit redir act} ++	"A"
({btq job redir state})		K	{btq key cedit redir file} ++	"F"

[
Process environment
]

H,S{btq process environment} = 19
"d           Edit directory
m           Edit umask (octal)
l           Edit ulimit (hex)
N E         Edit normal/error exits
A           Advance time on error exit
X           Set job export flags
D           Set job delete time
R           Set job maximum run time
K           Set signal number to kill job with
g           Set clean-up time following kill"

({btq process environment})	K	{btq pe key directory} = 500	"d"
({btq process environment})	K	{btq pe key umask} ++		"m"
({btq process environment})	K	{btq pe key ulimit} ++		"l"
({btq process environment})	K	{btq pe key nexit} ++		"N"
({btq process environment})	K	{btq pe key eexit} ++		"E"
({btq process environment})	K	{btq pe key adverr} ++		"A"
({btq process environment})	K	{btq pe key export} ++		"X"
({btq process environment})	K	{btq pe key deltime} ++		"D"
({btq process environment})	K	{btq pe key runtime} ++		"R"
({btq process environment})	K	{btq pe key killsig} ++		"K"
({btq process environment})	K	{btq pe key runon} ++		"g"


S{btq wtime state} = 20

[
Stuff for editing time
]

({btq wtime state})	K	{btq key time incr} = 500	"+"
({btq wtime state})	K	{btq key time decr} ++		"-"
({btq wtime state})	K	{btq key time left} ++		"h,\kLEFT"
({btq wtime state})	K	{btq key time right} ++		"l,\kRIGHT"
({btq wtime state})	K	{btq key time set} ++		"t,T,s,S,y,Y"
({btq wtime state})	K	{btq key time unset} ++		"f,F,u,U,n,N"
({btq wtime state})	K	{btq key time toggle} ++	"!,~"

[
Holidays
]

H,S{btq holidays state} = 25
"?           This screen
P N         Previous, next year
j k         Next, previous week
h l         Next, previous day
t s y       Set as holiday
f u n       Unset as holiday
! ~         Toggle holiday
TAB         Next month"

({btq holidays state})	K	{btq key hols prev} = 500	"P"
({btq holidays state})	K	{btq key hols next} ++		"N"
({btq holidays state})	K	{btq key hols left} ++		"h,\kLEFT"
({btq holidays state})	K	{btq key hols right} ++		"l,\kRIGHT"
({btq holidays state})	K	{btq key hols set} ++		"t,T,s,S,y"
({btq holidays state})	K	{btq key hols unset} ++		"f,F,u,U,n"
({btq holidays state})	K	{btq key hols toggle} ++	"!,~"

[
Job and Variable List Field setting
]

H,S{btq select format state} = 30
"q           Quit this screen
j k         Move down, up one row
i a         New field before/after
' ""         New separator before/after
w           Set width
c           Set code
<           Toggle left flag
D           Delete
S           Set separator string"

({btq select format state})	K	{btq key fmt new sep before} = 500	"'"
({btq select format state})	K	{btq key fmt new sep after} ++		""""
({btq select format state})	K	{btq key fmt delete} ++			"D"
({btq select format state})	K	{btq key fmt new field before} ++	"i"
({btq select format state})	K	{btq key fmt new field after} ++	"a"
({btq select format state})	K	{btq key fmt width} ++			"w"
({btq select format state})	K	{btq key fmt code} ++			"c"
({btq select format state})	K	{btq key fmt toggle left tab} ++	"<"
({btq select format state})	K	{btq key fmt separator edit} ++		"S"

[
Prompts etc for main display
]

P{btq more above} = 200			"-- %d more jobs above --"
P{btq more below} ++			"-- %d more jobs below --"
P{btq cond abbrev} ++			"**Cond**"

[
Signals to kill with.
You can add to this list if you want to
]

(2)	Q,H	{btq kill type}++	"Int",
"Please give the signal name you want to send to the job"
(3)	Q	{btq kill type} COPY 	"Quit"
(15)	R	{btq kill type} COPY	"Term"
(9)	Q	{btq kill type} COPY	"Kill"

[
Helps for various strings
]

H{btq wgets title} = ROUND(@, 20)
"Please give a new job name.
This can be any string."

H{btq wgets prio} ++
"Please give the new priority for this job,
between %d0 and %d1"

H{btq wgets ll} ++
"Please give the new load level for this job,
between %d0 and %d1"

H{btq wgets ci} ++
"Please give a new command interpreter name"

[
Comparison operations.
Insert `D' after the `A' to denote desired default.
]

(1)	Q	{Comparison names} ++	"="
(2)	R	{Comparison names}	"!="
(3)	Q	{Comparison names}	"<"
(4)	Q	{Comparison names}	"<="
(5)	Q	{Comparison names}	">"
(6)	Q	{Comparison names}	">="

[
Helps for conditions
]

H{Specify cond new cond} ++
"Please specify a condition to compare the variable with
the constant.
Press TAB until the one you want comes round and then
press RETURN."

H{Specify cond upd cond} ++
"Please specify a new condition.
Press TAB until the one you want comes round and then
press RETURN."

H{Comparison var list} ++
"Please specify the variable name you want to test against
Possibilities are:"

[
Marker showing condition critical
]

P{Critical condition marker} ++		"*"
P{cond no equiv local var} ++		"?"
P{unreadable local var} ++		"N"

[
Set job owner and group
]

P,H{New job owner} ++
"New owner for job `%s' currently %s: ",
"Please specify a new user name for the job.
Alternatively give the numeric user id.
You can `guess' at the user name by giving the first
letter(s) and pressing TAB.
Possibilities are:"

P,H{New job group} ++
"New group for job `%s' currently %s: ",
"Please specify a new group name for the job.
Alternatively give the numeric group id.
You can `guess' at the group name by giving the first
letter(s) and pressing TAB.
Possibilities are:"

[
Assignment helps
]

H{Assignment var list} ++
"Please specify the variable you want to set
Possibilities are:"

H,N{Assignment start flag help} ++
"Indicate whether you want the variable %s
set on entry
T Y S              Set
F N U              Reset
~ !                Toggle"

H,N{Assignment reverse flag help} ++
"This flag causes operations for the end of
a job to be reversed if possible
Note that the operations appear in inverse
video if this applies
T Y S              Set
F N U              Reset
~ !                Toggle"

H,N{Assignment exit flag help} ++
"Indicate whether you want the variable %s
set or reset on normal exit
T Y S              Set
F N U              Reset
~ !                Toggle"

H,N{Assignment error flag help} ++
"Indicate whether you want the variable %s
set or reset on error exit codes.
T Y S              Set
F N U              Reset
~ !                Toggle"

H,N{Assignment abort flag help} ++
"Indicate whether you want the variable %s
set or reset on abort (signal)
T Y S              Set
F N U              Reset
~ !                Toggle"

H,N{Assignment cancel flag help} ++
"Indicate whether you want the variable %s
set or reset on cancellation
(I.e. stop before it can start).
T Y S              Set
F N U              Reset
~ !                Toggle"

[
This tells us whether flags for job variables are on or not by default
1 indicates they are, -1 that they are not.
]

({Assignment start flag help})		N	1
({Assignment reverse flag help})	N	1
({Assignment exit flag help})		N	1
({Assignment error flag help})		N	1
({Assignment abort flag help})		N	1
({Assignment cancel flag help})		N	-1

[
String or numeric value setting
]

H{btq get numeric} = ROUND({Assignment cancel flag help}, 10)
"Please specify a numeric (possibly negative) or
string (starting with "") value for %s"

H{btq get upd numeric} ++
"Please specify a replacement numeric (possibly
negative) or string (starting with "") value for %s"

[
Assignment names
]

(1)	R	{Assignment names} = ROUND(@, 10)	"Set"
(2)	Q	{Assignment names}			"Add"
(3)	Q	{Assignment names}			"Subtract"
(4)	Q	{Assignment names}			"Multiply"
(5)	Q	{Assignment names}			"Divide"
(6)	Q	{Assignment names}			"Modulus"
(7)	Q	{Assignment names}			"Exit"
(8)	Q	{Assignment names}			"Signal"

[
Strings for displaying assignments
]

P{Assignment flag set} ++		"Set"
P{Assignment flag unset} ++		"--"
P{Assignment critical} ++		"*"
P{ass no equiv local var} ++		"?"
P{unwriteable local var} ++		"N"

[
Help for editing assignments
]

H{New assignment op help} ++		"Please specify what you want to happen to the variable"
H{Upd assignment op help} ++		"Please specify a new operation for the variable"

[
Markers for end of file/page
]

P{btq view end of file} = ROUND(@, 10)	"--- End of file ---"
P{btq view end of page} ++		"End of page"

[
Prompts for mail/write
]

P{btq mail msg prompt} = ROUND(@, 10)	"Mail user at end of job"
P{btq wrt msg prompt} ++		"Write message to user's terminal"
P{btq mw msg yes} ++			"Yes"
P{btq mw msg no} ++			"No"

[
Unqueue jobs from within btq
]

P,H{btq unqueue dir} = ROUND(@, 10)
"Directory to write in",
"This is the directory into which the job will be
copied. It should be writable by you."

P,H{btq unqueue command file} ++
"Command file",
"This file will be made contain a `btr' command
to re-invoke the job at a later time."

P,H{btq unqueue job file} ++
"Job file",
"This file will contain the job standard input"

P{btq unqueue hdr prompt} ++	"Unqueuing job `%s' (%d)"

(0)		R	{btq unqueue options} ++	"Unqueue"
(1)		Q	{btq unqueue options}		"Copy"
(2)		Q	{btq unqueue options}		"Save-home"
(3)		Q	{btq unqueue options}		"Options-current"

H{btq unqueue options}
"You have 4 alternatives:
   1.  Unqueue job, deleting the job
   2.  Unqueue job, without deleting it
   3.  Copy the options out of the job
       to your home directory.
   4.  Copy the options out of the job
       to your current directory."

P{btq copy options prompt} ++		"Copying options from job %s (%d)"

(0)	R		{btq unq verbose} ++	"Non-verbose"
(1)	Q		{btq unq verbose}	"Verbose"

P,H{btq unq verbose}
"Display result",
"This option determines whether you want to have
a message displayed (verbose mode) or not."

(0)	R		{btq unq jobstate} ++	"Normal"
(1)	Q		{btq unq jobstate}	"Cancelled"

P,H{btq unq jobstate}
"Job default state",
"This option determines whether you want jobs to
start off cancelled or not."

(0)     R               {btq unq set verbose} ++        "Not set"
(1)     Q               {btq unq set verbose}           "Verbose"

P,H{btq unq set verbose}
"Set verbose",
"This option marks the saved job so that without any options a message
is displayed on successful submission"

(0)     R               {btq unq set jobstate} ++       "Current state"
(1)     Q               {btq unq set jobstate}          "Force run"
(2)     Q               {btq unq set jobstate}          "Force canc"

P,H{btq unq set jobstate}
"Force set canc/run",
"This option can be used to force the saved state of the job to
be ready to run or cancelled or as the current state of the job"

P,H{Confirm overwrite unq} ++
"OK to overwrite",
"The file %s already exists. Is it OK to overwrite it"

P{xbtq unq force verbose} ++            "Force verbose on requeue"
P{xbtq unq setcanc lab} ++              "Cancelled setting"

(0)R{xbtq unq setcanc} ++               "As job"
(1)Q{xbtq unq setcanc} COPY             "Set runnable"
(2)Q{xbtq unq setcanc} COPY             "Set cancelled"

[
Variable creation
]

P{btq create new var} = ROUND(@, 20)		"Create new variable"

P,H{btq new variable name} ++
"Name:",
"Please give a name for the variable to be created.
This should be alphanumeric or '_' and starting with
a letter."

P,H{btq new variable value} ++
"Value:",
"Please give a value for the variable %s
This is either a number or a string.
(To start a string with a digit use "")."

P,H{btq new variable comment} ++
"Comment:",
"Please give a comment for the variable to be created.
This can be any string."

P,H{btq arith constant} ++
"New constant for arithmetic ops:",
"Please specify the constant to be used in
subsequent arithmetic ops + - * / %"

P,H{btq new var user} ++
"New user for %s currently %s: ",
"Please type a user name (or uid) of
a new user for this variable."

P,H{btq new var group} ++
"New group for %s currently %s: ",
"Please type a group name (or gid) of
a new user for this variable."

P,H{btq var rename} ++
"New name for variable %s: ",
"Please give a new name for this variable."

[
Edit command interpreter list
]

H,E{btq ci unique name} = ROUND(@, 20)
"Please enter a unique name for the command interpreter.
This will be used to specify the required one.
Note that the actual program is given as the `path'.",
"Duplicate name %s
Command interpreter names must be distinct"

H,E{btq ci path name} ++
"Please specify a path name for the command interpreter
called %s. This should be an absolute path, starting with a
`/'.",
"Bad path name %s"

H{btq ci load lev} ++
"This is the load level to be specified for all jobs submitted
using the command interpreter %s."

H{btq ci nice} ++
"This is the nice value to be given for all jobs submitted using the
command interpreter %s."

H{btq ci args} ++
"This is the argument string which is always supplied
to the command interpreter.
For shells ""-s"" is recommended which tells it to use standard input
and not to try to decode user-supplied arguments as file names."

[
Arguments, environment, redirections
]

H{btq job arg value} = ROUND(@, 50)
"Please specify an argument value string"

H{btq job env name} ++
"Please specifiy the environment variable name"

H{btq job env value} ++
"Please specifiy the environment variable value"

H{btq job redir fname} ++
"Please give the file (or command for pipes) to be used"

H{btq job redir fd1} ++
"Please give the file descriptor to be connected"

H{btq job redir fd2} ++
"Please give the file descriptor to be duplicated from"

[
Redirection type names "D" represents default
]

(1)	Q	{Redirection type names} ++	"Read"
(2)	R	{Redirection type names}	"Write"
(3)	Q	{Redirection type names}	"Append"
(4)	Q	{Redirection type names}	"RD/WR"
(5)	Q	{Redirection type names}	"RD/Append"
(6)	Q	{Redirection type names}	"Pipe TO"
(7)	Q	{Redirection type names}	"Pipe FROM"
(8)	Q	{Redirection type names}	"Close"
(9)	Q	{Redirection type names}	"Dup"

H{Redirection type names}
"Please choose the kind of redirection you want."

[
Names for stdin/out/err
]

(0)	Q	{stdio file names} ++	"(Stdin)"
(1)	Q	{stdio file names}	"(Stdout)"
(2)	Q	{stdio file names}	"(Stderr)"

[
Screen markers to warn that we are moving things about
]

P{btq arg moving} ++	"(Moving)"
P{btq arg copying} ++	"(Copying)"

[
Prompts/help for process environment
]

P,H{btq procp dir} = ROUND(@, 10)
"Directory: ",
"This is the directory which the job will change to when you start
You can embed environment variable names (e.g. $HOME) in this name,
also ~user to give that user's home diretory"

P,H{btq procp umask} ++
"Umask: ",
"This is the ""Umask"" value in OCTAL built up as follows
Bits set turn OFF the access bits in created files thus
           4       Forbid Read
           2       Forbid Write
           1       Forbid Execute/Search
First digit gives user, 2nd group, 3rd others
e.g. 026 forbids write for group read/write others"

P,H{btq procp ulimit} ++
"Ulimit: ",
"This gives the maximum size to which any file can be extended
in blocks. This is in hexadecimal."

P,H{btq procp nexit} ++
"Normal exit: ",
"This gives the range of process exit codes which will be
regarded as ""normal"".
Anything not ""covered"" by this and ""error"" is ""abort""."

P,H{btq procp eexit} ++
"Error exit: ",
"This gives the range of process exit codes which will be
regarded as ""error"".
Anything not ""covered"" by this and ""normal"" is ""abort""."

[
Whether or not we advance the time on error
]

(0)	R	{btq advance time opts} ++	"Advance time"
(1)	Q	{btq advance time opts}		"Do not advance time"

H{btq advance time opts}
"If the job exits with error or abort, you can choose whether you
you want the time to be advanced or not, or left as it was."

[
Export type options
]

(0)	Q	{btq export opts} ++		"Local only"
(1)	R	{btq export opts}		"Export"
(3)	Q	{btq export opts}		"Remote runnable"

P,H{btq export opts}
"Export: ",
"Please indicate whether the job is to be accessed or not from
remote machines. A job can be local only, visible but not
runnable from remote machines, or fully runnable on remote
machines."

[
Delete/run/grace times
]

P,H{btq delete time} ++
"Delete time",
"This gives the time in hours after the last run of the job
after which it will be automatically deleted.
Set it to zero to turn off the automatic deletion."

P,H{btq run time} ++
"Maximum run time",
"This gives the maximum time for which a job may run.
After this an attempt is made to stop it."

P,H{btq which signal} ++
"Signal number",
"This is the signal number to be sent to a job to stop
it. Zero indicates that a SIGKILL (9) will be sent.
You can set some other value, such as SIGTERM (15)
to give your job the opportunity to ""clean up"" if it
provides for this."

P,H{btq runon time} ++
"Run on time",
"This gives the ""grace period"" following an alternative
signal to let the job clean up before a SIGKILL is finally
to force it to stop."

[
Searches
]

P,H{btq search forward} = ROUND(@, 10)
"Search forward:",
"Please specify the string to search for . is wild card
case is insensitive."

P{btq search backward} ++
"Search backward:"

[
Prompts when off-screen
]

P{btq prompt4 ci} = ROUND(@, 10)	"Command Interpreter: "
P{btq prompt4 pri} ++			"Priority: "
P{btq prompt4 ll} ++			"Load Level: "
P{btq prompt4 title} ++			"Title: "
P{btq prompt4 comm} ++			"Comment: "
P{btq prompt4 ass} ++			"New value: "

[
Times and dates
]

H{wtime digits hours} = ROUND(@, 20)
"Set hours field for %s
Increment or decrement with + -
or type digit explicitly"

H{wtime digits mins} ++
"Set minutes field for %s
Increment or decrement with + -
or type digit explicitly
TAB - select next field"

H{wtime day of week} ++
"Set day of week for %s
Move forwards with + back with -
TAB - select next field"

H{wtime day of mon} ++
"Set day of month for %s
Increment or decrement with + -
or type digit explicitly
TAB - select next field"

H{wtime set month} ++
"Set month for %s
Move forwards with + back with -
TAB - select next field"

H{wtime set year} ++
"Set year for %s
Move forwards with + back with -
TAB - select next field"

[
Repeat options
]

P,H{btq repeat options} ++
"Repeat",
"Please indicate the repetition (if any) which
you require. Move to desired field and press RETURN"

(0)	Q	{btq repeat options}	"Once (& delete)"
(1)	R	{btq repeat options}	"Once (& retain)"
(2)	Q	{btq repeat options}	"Repeat every"

[
Month day
]

P,H{Repeat month day} ++
"day",
"Please enter the day of the month to base
the %s repetitions on."

P,H{Wtime avoiding help} ++
"Avoiding",
"A day displayed here indicates days which
should NOT be chosen."

[
Skip/delay options
]

P,H{Wtime if not poss} ++
"If not possible",
"If the job cannot be run, do you want to skip it
(or skip this run), or run it when possible.
If you delay this job do you want to delay future attempts
by the same amount? If not, runs may get lost if the current
run `runs into' the next one.
Select the required option with the cursor keys
and press RETURN to accept."

(0)	Q	{Wtime if not poss}	"Skip"
(1)	Q	{Wtime if not poss}	"Delay current"
(2)	Q	{Wtime if not poss}	"Delay all"
(3)	Q	{Wtime if not poss}	"Catch up"

[
No avoid day set
]

P{Wtime not avoid} ++	"---"

[
Do we want to set time at all
]

P,H{Do you want to set time} ++
"Yes",
"Do you want a time constraint set for job `%s' (%d0)?
Please indicate by typing Y or N"

P{Dont set time} ++	"No"

[
Program option codes for btq
]

P{btq opt queue} = 700		"Job queues (pattern)        :"
P{btq opt incnull} ++		"Include null queue names    :"
P{btq opt onlyuser} ++		"Display only user :"
P{btq opt onlygroup} ++		"Display only group:"
P{btq opt confdel} ++		"Confirm abort/delete jobs   :"
P{btq opt curs} ++		"If job moves      :"
P{btq opt loco} ++		"Local jobs/vars   :"
P{btq opt helpclr} ++		"Clear help message:"
P{btq opt helpbox} ++		"Help messages     :"
P{btq opt errbox} ++		"Error messages    :"
P{btq opt initscr} ++		"Screen on entry   :"
P{btq opt xml fmt} ++           "XML unqueue format:"

N{btq opt init cursor} = {btq opt queue} - 2
N{btq opt start row} = {btq opt queue} - 1

[
Define initial cursor position
Define starting row for display
]

({btq opt init cursor})	N	{btq opt queue}
({btq opt start row})	N	{btq opt queue}

[
Define order in which prompts appear
]

({btq opt queue})	N	{btq opt incnull}
({btq opt incnull})	N	{btq opt onlyuser}
({btq opt onlyuser})	N	{btq opt onlygroup}
({btq opt onlygroup})	N	{btq opt confdel}
({btq opt confdel})	N	{btq opt curs}
({btq opt curs})	N	{btq opt loco}
({btq opt loco})	N	{btq opt helpclr}
({btq opt helpclr})	N	{btq opt helpbox}
({btq opt helpbox})	N	{btq opt errbox}
({btq opt errbox})	N	{btq opt initscr}
({btq opt initscr})	N	{btq opt xml fmt}
({btq opt xml fmt})     N       -1

[
Help and prompts are generated by multiplying by 10
]

H{btq opt queue} * 10
"If you are only interested in jobs which have a given
prefix (""queue name""), put the prefix here. Alternatively
leave empty to display all jobs. The prefix may be specified
as a pattern with wild-cards * ? [a-z] [!0-9] or as a
series of patterns separated by commas, thus B*,*[a-m]."

H,P{btq opt incnull} * 10
"Please indicate whether you want to include jobs with no
queue prefix in the display or not.",
"No"

P{btq opt incnull} * 10 + 1
"Yes"

H{btq opt onlyuser} * 10
"If you are only interested in jobs for a given user, put
the user name (or pattern) in here. Alternatively leave empty
to display all jobs."

H{btq opt onlygroup} * 10
"If you are only interested in jobs for a given group, put
the group name (or pattern) in here. Alternatively leave empty
to display all jobs."

H,P{btq opt confdel} * 10
"Please indicate whether you want to confirm deletion of
jobs or not.",
"Never"

P{btq opt confdel} * 10 + 1
"Always"

H,P{btq opt curs} * 10
"Please indicate whether you want to follow the job if it changes
position on the screen, or try to retain the display.",
"Follow job"

P{btq opt curs} * 10 + 1
"Retain display"

H,P{btq opt loco} * 10
"Please indicate whether you want to restrict the job and variable
displays to the current machine or network-wide.",
"All jobs/vars"

P{btq opt loco} * 10 + 1
"Local only"

H,P{btq opt helpclr} * 10
"Please indicate whether you want the next keystroke afer
displaying a help message to be kept or lost",
"Use next command"

P{btq opt helpclr} * 10 + 1
"Clear help"

H,P{btq opt helpbox} * 10
"Please indicate whether you want to display help messages
in inverse video (standout mode) or boxes.",
"Inverse video"

P{btq opt helpbox} * 10 + 1
"Boxes"

H,P{btq opt errbox} * 10
"Please indicate whether you want to display error messages
in inverse video (standout mode) or boxes.",
"Inverse video"

P{btq opt errbox} * 10 + 1
"Boxes"

H,P{btq opt initscr} * 10
"Please indicate where the cursor should be on entry.
This can be the job screen, the variables screen or
`don't care'.",
"Job screen"

P{btq opt initscr} * 10 + 1
"Variables screen"

P{btq opt initscr} * 10 + 2
"Don't care"

H,P{btq opt xml fmt} * 10
"Please indicate whether you want job files saved in single-file
XML files or a separate job file and command file",
"Separate files"

P{btq opt xml fmt} * 10 + 1
"Single XML file"

[
Default format strings
]

P{Default job list format} = 1990	"%3n %<7N %7U %13H %14I %3p%5L %5t %9c %<4P"
P{Default var list fmt 1} ++		" %22N %41V %13E"
P{Default var list fmt 2} ++		"     %44C %7U %7G %7K"
P{Left tab field} ++			"<"

[
btq job list screen errors
]

E{btq jlist unknown command} = 500	"Unknown command - expecting job op"
E{btq jlist off beginning} ++		"Off beginning of job list"
E{btq jlist off end} ++			"Off end of job list"
E{btq no jobs to edit} ++		"No jobs to edit"
E{btq cannot read job} ++		"Cannot read the job"
E{btq cannot write job} ++		"Cannot write or change the job"
E{btq no special create permission} ++
"You need `special create' permission to do this to
the job."
E{btq no delete permission} ++		"Cannot delete the job"
E{btq no change mode} ++		"Cannot change mode of job."
E{btq invalid user name} ++		"Invalid user name for job"
E{btq invalid group name} ++		"Invalid group name for job"
E{btq jlist bad command interp} ++	"Unknown command interpreter %s"
E{btq no time to advance} ++
"The selected job %d0 `%s' does not
have a time to advance. Please set one first."
E{btq jlist cannot kill} ++		"Sorry but you cannot kill this job."
E{btq jlist no search string} ++		"No search string specified (yet). Please type one."
E{btq jlist search string not found} ++		"Search string not found."
E{btq jlist no space unq win} ++		"No space for unqueue command window - sorry"
E{btq jlist cannot create unq win} ++		"Cannot create unqueue window - sorry"

[
btq job view errors
]

E{btq cannot open file} = ROUND(@, 10)
"Cannot open job file for `%s' job number %d0 for display.
Maybe someone just deleted it?"
E{btq viewj unknown command} ++		"Unknown command. Type space to step through."
E{btq viewj off beginning} ++		"Off beginning of file"
E{btq viewj off end} ++		"Off end of file"
E{btq viewj off lhs} ++		"Off lhs of file"
E{btq viewj off rhs} ++		"Off rhs of file"
E{btq viewj cannot open net file} ++
"Sorry cannot access remote job
%t:%d0 `%s' for display."

[
btq job condition errors
]

E{btq jcond no space for win} = ROUND(@, 10)	"No space in window for job condition subwindow - sorry"
E{btq jcond cannot create win} ++		"Cannot create window to edit job condition variables - sorry"
E{btq no jconds yet} ++			"No condition variables yet to edit"
E{btq jcond off beginning} ++		"Off beginning of condition variables"
E{btq jcond off end} ++			"Off end of condition variables"
E{btq jcond too many conds} ++		"Sorry - too many condition variables"
E{btq jcond invalid variable} ++	"Invalid or unreadable condition variable"
E{btq jcond unknown command} ++		"Unknown command trying to set condition variables"

[
btq job assignment errors
]

E{btq jass scrambled flags} = ROUND(@, 10)	"Flag structures scrambled - sorry (this is probably a bug!)."
E{btq jass flag unknown command} ++		"Unknown command - setting flags"
E{btq jass unknown command} ++			"Unknown command - setting job var set operations"
E{btq no jass yet} ++				"No variable settings to edit yet"
E{btq jass off beginning} ++			"Off beginning of variable settings"
E{btq jass off end} ++				"Off end of variable settings"
E{btq jass too many} ++				"Too many variable settings"
E{btq jass bad variable} ++			"Invalid/undefined set variable %s"
E{btq not valid for exit codes} ++
"You can't do this command for exit codes and signals - the value is whatever
the exit code or signal was and is only set on exit."

[
btq command interpreter list errors
]

E{btq command interp in use} = ROUND(@, 10)
"Sorry but you can't delete command interpreter %s
as there are jobs using it."
E{btq no command interps} ++		"No command interpreters in file?"
E{btq ci unknown command} ++		"Unknown command - expecting cmd interp op"
E{btq ci off beginning} ++		"Off beginning of command interp file"
E{btq ci off end} ++			"Off end of command interp file"
E{btq ci deleting shell} ++
"Attempt to delete shell %s
(Path %t)."
E{btq ci no permission} ++
"Sorry but you (%R) need `special create' permission to do this
however you can display the file as you are doing."

[
btq mail write flags errors
]

E{btq mwflags no space win} = ROUND(@, 10)	"No space for mail/write flags window - sorry"
E{btq mwflags no create win} ++		"Cannot create mail/write flags window - sorry"

[
btq unqueue errors
]

E{btq unq not absolute path} = ROUND(@, 10)
"%s is not an absolute path name
Please give a name starting with a /"
E{btq unq not a directory} ++		"%s is not a directory"
E{btq unq not a file name} ++		"%s is not a valid file name (contains '/')"
E{btq unq not a regular file} ++	"%s is not a regular file"
E{btq unq cannot fork} ++		"Cannot fork to run unqueue process"
E{btq unq program fault} ++		"Program fault in unqueue process"
E{btq unq misc error} ++		"Misc error %d1 in unqueue process for job `%s' (%d0)"
E{btq unq file not found} ++		"Unqueue spool file not found for job `%s' (%d0)"
E{btq unq dir not found} ++		"Unqueue directory not found for job `%s' (%d0)"
E{btq unq cannot create directory} ++	"Cannot create file for job `%s' (%d0)"
E{btq unq unknown job} ++		"Unqueue process reports unknown job `%s' (%d0)??"
E{btq unq no permission} ++
"No permission to unqueue this job `%s' (%d0).
You (%R) need read, write and delete permissions."
E{btq unq runnable job} ++
"This job `%s' (%d0) is potentially runnable. Please
cancel it before proceeding."
E{btq unq cannot delete} ++		"Dumped out job, but did not delete it."
E{btq unq cannot save options file} ++	"Sorry cannot write configuration file"

[
btq var list errors
]

E{btq vlist unknown command} = ROUND(@, 50)	"Unknown command - expecting var op"
E{btq vlist off beginning} ++			"Off beginning of variable list"
E{btq vlist off end} ++				"Off end of variable list"
E{btq vlist no vars} ++				"No variables available to edit"
E{btq vlist cannot create} ++			"Cannot create variable - sorry"
E{btq vlist no space create win} ++
"Not enough space in window to permit subwindow for
variable creation - sorry. Please reduce heading size"
E{btq vlist cannot create win} ++		"Cannot create window for variable creation - sorry"
E{btq vlist cannot delete} ++			"Cannot delete variable"
E{btq vlist cannot assign} ++			"Cannot assign to variable %s"
E{btq vlist cannot change mode} ++		"Cannot change modes of variable %s"
E{btq vlist invalid user name} ++		"Invalid user name"
E{btq vlist invalid group name} ++		"Invalid group name"
E{btq vlist Cannot rename} ++			"Cannot rename variable %s"
E{btq vlist not arithmetic} ++
"Current value variable %s not arithmetic -
cannot perform this operation."
E{btq vlist no search string} ++		"No search string specified (yet). Please type one."
E{btq vlist Search string not found} ++		"Search string not found."

[
btq modes edit errors
]

E{btq modes no window space} = ROUND(@, 10)	"No window space for setting modes - sorry"
E{btq modes cannot create mode window} ++	"Cannot create window for setting modes - sorry"
E{btq modes unknown command} ++			"Unknown command trying to set mode"
E{btq modes off beginning} ++			"Off beginning of modes"
E{btq modes off end} ++				"Off end of modes"
E{btq modes off lhs} ++				"Off lhs of modes"
E{btq modes off rhs} ++				"Off rhs of modes"
E{btq modes cannot write} ++			"Sorry - cannot write modes, only reading permitted"
E{btq alts no window space} ++			"No window space for alternatives"
E{btq alts unknown alternative} ++		"Unknown alternative"
E{btq invalid string char in number} ++		"Invalid character (this looked like a number)."
E{btq invalid char in string} ++		"Invalid character in string value"
E{btq invalid command expecting string} ++	"Invalid command expecting string"

[
btq time set errors
]

E{btq hours unknown command} = ROUND(@, 10)
"Unknown command - attempting to set hours
for %s (%d0)"
E{btq bad hours digit} ++		"Invalid digit for hours field"
E{btq bad mins digit} ++		"Invalid digit for minutes field"
E{btq time not in future} ++		"Time for `%s' (%d0) not in future"
E{btq weekday unknown command} ++	"Unknown command (setting weekday)"
E{btq monthday unknown command} ++	"Unknown command (setting day of month)"
E{btq monthday bad digit} ++		"Invalid digit for day of month field"
E{btq month unknown command} ++		"Unknown command (setting month)"
E{btq year unknown command} ++		"Unknown command (setting year)"
E{btq stoneage} ++
"Sorry this program doesn't understand years before 1990
which was when it was written"
E{btq repeat unknown command} ++	"Unknown command (setting repeat for `%s' %d0)"
E{btq skip unknown command} ++		"Unknown command (defining skip/delay option)"
E{btq time window too small} ++		"Window too small for time/date input - sorry"
E{btq avoid days unknown command} ++	"Unknown command setting days to avoid"
E{btq avoid days cannot have all set} ++
"You cannot set all days to avoid!!
Please unset some!"
E{btq timecon unknown command} ++
"Unknown command.
Do you want a time constraint (Y) or not (N)?"

[
btq holidays
]

E{btq hols unknown command} = ROUND(@, 10)	"Unknown command setting holidays"
E{btq cannot read holidays file} ++		"Cannot read holidays file - sorry"
E{btq cannot create holidays file} ++		"Cannot create holidays file - sorry"
E{btq cannot update holidays file} ++
"Sorry but you need ""edit admin file"" privilege to edit
the holidays file. Please ask someone with this privilege to do it."

[
btq args, env, redir errors
]

E{btq args unknown command} = ROUND(@, 10)	"Unknown command editing arguments"
E{btq args off beginning} ++		"Off beginning of argument list"
E{btq args off end} ++			"Off end of argument list"
E{btq args cannot write job} ++		"Cannot write job"
E{btq args no args yet} ++		"No arguments to edit (yet)"
E{btq args finish first} ++		"Please finish your move/copy first"
E{btq env unknown command} ++		"Unknown command editing environment"
E{btq env off beginning} ++		"Off beginning of environment list"
E{btq env off end} ++			"Off end of environment list"
E{btq env cannot write job} ++		"Cannot write job"
E{btq env no env yet} ++		"No environment to edit (yet)"
E{btq env finish first} ++		"Please finish your move first"
E{btq redir unknown command} ++		"Unknown command editing redirections"
E{btq redir off beginning} ++		"Off beginning of redirections list"
E{btq redir off end of list} ++		"Off end of redirections list"
E{btq redir cannot write job} ++	"Cannot write job"
E{btq redir no redir yet} ++		"No redirections to edit (yet)"
E{btq redir finish first} ++		"Please finish your move first"

[
Process environment
]

E{btq proce unknown command} = ROUND(@, 10)	"Unknown command editing process environment"
E{btq proce cannot write job} ++	"Cannot write job"
E{btq proce directory not absolute} ++
"Directory %s is not absolute.
(No leading /)"
E{btq proce expanded directory not absolute} ++
"Directory %s, which expanded to
%t,
is not absolute. (No leading /)"
E{btq proce invalid directory name} ++	"Directory %s - bad syntax please retry"

[
Format editing
]

E{btq format unknown command} = ROUND(@, 10)	"Unknown command editing format list"
E{btq format off beginning} ++		"Off beginning of list of formats"
E{btq format off end} ++		"Off end of list of formats"
E{btq format no formats} ++		"No formats to edit yet - please create some"
E{btq format is separator} ++		"Format is separator - invalid command"
E{btq format is not separator} ++	"Format is not separator - invalid command"
E{btq format invalid code} ++		"Undefined format code"

######################################################################
##  Mostly stuff for btuser but some of the common stuff above
##  applies

[
Btuser interactive edit (changeable)
]
H,S{btuser interactive state} = 2
"Commands to view/edit user list

?      Help (this screen)
^L     Redraw screen
Q q    Quit
^F ^D  Screen, half screen down
^B ^U  Screen, half screen up
j k    Cursor down, up
B e    Beginning/end of screen or list
       if already there.
l u d  Set minimum, maximum, default user priorities
L U D  Set minimum, maximum, default default priorities
m t s  Set maximum, total, special user load levels
M T S  Set maximum, total, special default load levels
p P    Edit user/default permissions
c C    Edit user/default access
a A    Copy defaults to user, all users"

({btuser interactive state})K{btuser key user def pri} = 500	"d"
({btuser interactive state})K{btuser key sys def pri} ++	"D"
({btuser interactive state})K{btuser key user min pri} ++	"l"
({btuser interactive state})K{btuser key def min pri} ++	"L"
({btuser interactive state})K{btuser key user max pri} ++	"u"
({btuser interactive state})K{btuser key def max pri} ++	"U"
({btuser interactive state})K{btuser key user max ll} ++	"m"
({btuser interactive state})K{btuser key def max ll} ++		"M"
({btuser interactive state})K{btuser key user tot ll} ++	"t"
({btuser interactive state})K{btuser key def tot ll} ++		"T"
({btuser interactive state})K{btuser key user spec ll} ++	"s"
({btuser interactive state})K{btuser key def spec ll} ++	"S"
({btuser interactive state})K{btuser key user modes} ++		"c"
({btuser interactive state})K{btuser key def modes} ++		"C"
({btuser interactive state})K{btuser key user priv} ++		"p"
({btuser interactive state})K{btuser key def priv} ++		"P"
({btuser interactive state})K{btuser key copy user} ++		"a"
({btuser interactive state})K{btuser key copy all} ++		"A"
({btuser interactive state})K{btuser key charge} ++		"b"

({btuser interactive state})K{btuser key exec} = 600		"@"

[
Btuser interactive view (no changes)
]
H,S{btuser interactive view} = 1
"Commands to view user list

?      Help (this screen)
^L     Redraw screen
Q q    Quit
^F ^D  Screen, half screen down
^B ^U  Screen, half screen up
j k    Cursor down, up
B e    Beginning/end of screen or list
       if already there.
p P    View user/default permissions
c C    View user/default access"

({btuser interactive view})K{btuser key user modes}		"c"
({btuser interactive view})K{btuser key def modes}		"C"
({btuser interactive view})K{btuser key user priv}		"p"
({btuser interactive view})K{btuser key def priv}		"P"
({btuser interactive view})K{btuser key exec}			"@"

[
Btuser privs setting view and view/edit
]

H,S{btuser view privs} = 3
"?      Help (this screen)
^L     Redraw screen
Q q .  Quit
j k    Cursor down, up
RETURN Cursor down or quit"

P,H,S{btuser privs state} = 4
"User %s may ",
"?      Help (this screen)
^L     Redraw screen
Q q .  Quit
j k    Cursor down, up
B e    Beginning, end of list
RETURN Cursor down or quit

y t    Permit
n f    Deny
~ !    Toggle
D      Copy default"

({btuser privs state})	K	{Btuser key set priv} = 550	"y,Y,t,T"
({btuser privs state})	K	{Btuser key unset priv} ++	"n,N,f,F"
({btuser privs state})	K	{Btuser key toggle priv} ++	"~,!"
({btuser privs state})	K	{Btuser key setdef priv} ++	"D"

H,S{btuser view defprivs} = 5
"?      Help (this screen)
^L     Redraw screen
Q q .  Quit
j k    Cursor down, up
RETURN Cursor down or quit"

P,H,S{btuser defprivs state} = 6
"Default is to ",
"?      Help (this screen)
^L     Redraw screen
Q q .  Quit
j k    Cursor down, up
B e    Beginning, end of list
RETURN Cursor down or quit

y t    Permit
n f    Deny
~ !    Toggle"

({btuser defprivs state}) K	{Btuser key set priv}	"y,Y,t,T"
({btuser defprivs state}) K	{Btuser key unset priv} "n,N,f,F"
({btuser defprivs state}) K	{Btuser key toggle priv}"~,!"

[
Btuser View Modes
]

H,S{btuser view modes} = 7
"?        Help (this screen)
^L       Redraw screen
Q q      Quit
B e      Beginning or end
j k h l  Cursor down, up, left, right
RETURN   Cursor next or quit"

({btuser view modes})	K	{btuser key mode left} = 450	"h,\kLEFT"
({btuser view modes})	K	{btuser key mode right} ++	"l,\kRIGHT"
({btuser view modes})	K	{btuser key mode jobs} ++	"J"
({btuser view modes})	K	{btuser key mode vars} ++	"V"

[
Edit modes
]

H,S{btuser edit modes} = 8
"?        Help (this screen)
^L       Redraw screen
Q q      Quit
j k      Cursor down, up, left, right
RETURN   Cursor next or quit
J V	 job, var column

y t    Permit
n f    Deny
~ !    Toggle
D      Copy default"

({btuser edit modes})	K	{btuser key mode left}		"h,\kLEFT"
({btuser edit modes})	K	{btuser key mode right}		"l,\kRIGHT"
({btuser edit modes})	K	{btuser key mode jobs}		"J"
({btuser edit modes})	K	{btuser key mode vars}		"V"
({btuser edit modes})	K	{btuser key set mode} ++	"t,T,y,Y"
({btuser edit modes})	K	{btuser key unset mode} ++	"f,F,n,N"
({btuser edit modes})	K	{btuser key toggle mode} ++	"~,!"
({btuser edit modes})	K	{btuser key setdef mode} ++	"D"

[
View default modes
]

H,S{btuser view defmodes} = 9
"?        Help (this screen)
^L       Redraw screen
Q q      Quit
B e      Beginning or end
j k h l  Cursor down, up, left, right
RETURN   Cursor next or quit"

({btuser view defmodes})	K	{btuser key mode left}	"h,\kLEFT"
({btuser view defmodes})	K	{btuser key mode right}	"l,\kRIGHT"
({btuser view defmodes})	K	{btuser key mode jobs}	"J"
({btuser view defmodes})	K	{btuser key mode vars}	"V"

[
Edit default modes
]

H,S{btuser edit defmodes} = 10
"?        Help (this screen)
^L       Redraw screen
Q q      Quit
j k      Cursor down, up, left, right
RETURN   Cursor next or quit
J V	 job, var column

y t    Permit
n f    Deny
~ !    Toggle"

({btuser edit defmodes})	K	{btuser key mode left}		"h,\kLEFT"
({btuser edit defmodes})	K	{btuser key mode right}		"l,\kRIGHT"
({btuser edit defmodes})	K	{btuser key mode jobs}		"J"
({btuser edit defmodes})	K	{btuser key mode vars}		"V"
({btuser edit defmodes})	K	{btuser key set mode} 		"t,T,y,Y"
({btuser edit defmodes})	K	{btuser key unset mode} 	"f,F,n,N"
({btuser edit defmodes})	K	{btuser key toggle mode}	"~,!"

[
Ask whether to save options
]

E,S{Saveopts yorn state} = 21
"Unknown response, please answer yes or no"

({Saveopts yorn state})		K	{key yorn yes} = 500		"y,Y"
({Saveopts yorn state})		K	{key yorn no} ++		"n,N"

[
Prompts errors and help around basic commands
]

P{btuser more above} = 100	"-- %d more above --"
P{btuser more below} ++		"-- %d more below --"

E{btuser list unknown command} ++	"Unknown command. Type ? for help"
E{btuser list off beg} ++		"Off beginning of user list"
E{btuser list off end} ++		"Off end of user list"
E{btuser no users} ++			"No users yet (funny!)"
E{btuser read only} ++			"Sorry, but you are in read-only mode"
E{btuser priv unknown command} = ROUND(@, 10)	"Unknown command. Type ? for help"
E{btuser priv off beg} ++		"Off beginning of privileges list"
E{btuser priv off end} ++		"Off end of privileges list"
E{btuser priv read only} ++
"Sorry, but you are in read-only mode and you can only look
at this list."
E{btuser mode unknown command} = ROUND(@, 10)	"Unknown command. Type ? for help"
E{btuser mode off beg} ++		"Off beginning of modes list"
E{btuser mode off end} ++		"Off end of modes list"
E{btuser mode read only} ++
"Sorry, but you are in read-only mode and you can only look
at this list."
E{btuser mode off left} ++		"Off lhs of modes list"
E{btuser mode off right} ++		"Off rhs of modes list"

H{btuser user def prio help} = ROUND(@, 10)
"Please enter the required default priority (between %d0 and %d1)
for user `%s' to replace the existing value of %d2."

H{btuser sys def prio help} ++
"Please enter the required system default priority, between %d0 and %d1
to replace the existing value of %d2. This will be inserted as the
default priority of any new user created from now on."

H{btuser user min prio help} ++
"Please enter the required minimum priority (between %d0 and %d1) for
user `%s' to replace the existing value of %d2."

H{btuser sys min prio help} ++
"Please enter the required system minimum priority, between %d0 and %d1
to replace the existing value of %d2. This will be inserted as the
minimum priority of any new user created from now on."

H{btuser user max prio help} ++
"Please enter the required maximum priority (between %d0 and %d1) for
user `%s' to replace the existing value of %d2."

H{btuser sys max prio help} ++
"Please enter the required system maximum priority, between %d0 and %d1
to replace the existing value of %d2. This will be inserted as the
maximum priority of any new user created from now on."

H{btuser user max ll help} ++
"Please enter the required maximum load level (between %d0 and %d1)
for user `%s' to replace the existing value of %d2."

H{btuser sys max ll help} ++
"Please enter the required system maximum load level, between %d0
and %d1 to replace the existing value of %d2. This will be inserted as the
maximum load level for any new user created from now on."

H{btuser user tot ll help} ++
"Please enter the required total load level (between %d0 and %d1)
for user `%s' to replace the existing value of %d2."

H{btuser sys tot ll help} ++
"Please enter the required system total load level, between %d0
and %d1 to replace the existing value of %d2. This will be inserted as the
total load level for any new user created from now on."

H{btuser user spec ll help} ++
"Please enter the required special job load level (between %d0 and %d1)
for user `%s' to replace the existing value of %d2."

H{btuser sys spec ll help} ++
"Please enter the required system special job load level, between %d0
and %d1 to replace the existing value of %d2. This will be inserted as the
special job load level for any new user created from now on."

P,H{btuser search forward} ++
"Search forward:",
"Please specify the string to search for . is wild card
case is insensitive."

P,E{btuser search backward} ++
"Search backward:",
"No search string specified (yet). Please type one."

E{btuser search not found} ++
"Search string was not found.
(Couldn't find user name)."

[
Macro errors
]

E,H{Macro error} = ROUND(@, 10)
"Macro command %d0 is not defined.",
"There is no macro command or prompt defined with code %d0
to denote the action to be taken."

E,H{Macro fork failed} ++
"Macro command failed to fork - %E",
"Failed to do a ""fork"" to run a macro.
Your machine is running out of processes."

E,H{Macro command gave signal} ++
"Macro command failed - signal %d0",
"Your macro command failed with some signal."

E,H{Macro command error} ++
"Macro command failed - exit code %d0",
"Your macro failed, giving an exit code
of %d0. I don't know what this means!"

[
Stuff around privileges
]

P{Btuser priv colon} =	600	": "
P{Btuser priv no} ++		"No"
P{Btuser priv yes} ++		"Yes"

P,H{Btuser q privs copy over} ++
"Copy to everyone else (but you)?",
"If you want to copy the default privileges you have just
set to all other users, apart from yourself (%R), type `Y'.
Otherwise type `N'."

[
Stuff around modes
]

P{Btuser mode no} = ROUND(@,10)	"No"
P{Btuser mode yes} ++		"Yes"
P{Btuser job mode hdr} ++	"Jobs"
P{Btuser var mode hdr} ++	"Vars"
P{Btuser mode user hdr} ++	"U"
P{Btuser mode group hdr} ++	"G"
P{Btuser mode other hdr} ++	"O"

P,H{Btuser q modes copy over} ++
"Copy to everyone else (but you)?",
"If you want to copy the default modes s you have just
set to all other users, apart from yourself (%R), type `Y'.
Otherwise type `N'."

P{Btuser charge is} = ROUND(@, 50)	"Current charge for user %s is %g"

[
Basic `display privs and modes' stuff
Some of the following are not really errors, it's the first part of the
`display' option, but this is a handy way of dealing with it
]

E{btuser simple display} = 900
"The GNUbatch account for user %u0 group %g1.
Minimum priority %d2 maximum %d3 default %d4
Maximum load level %d5 total load %d6
Special jobs are allocated a load of %d7."

P{btuser privilege header} ++		"Privileges are:"
P{btuser privilege next} ++		", "
P{btuser disp mode jobs} ++		"Job - "
P{btuser disp mode vars} ++		"Var - "
P{btuser disp mode user} ++		"User"
P{btuser disp mode group} ++		"Group"
P{btuser disp mode others} ++		"Others"
P{btuser disp mode sep1} ++		" by: "
P{btuser disp mode sep2} ++		", "

[
Options setting for btuser - prompts
]
P{btuser opt mode} = 700		"Mode of program   :"
P{btuser opt sort} ++			"Sort users        :"
P{btuser opt helpclr} ++		"Clear help message:"
P{btuser opt helpbox} ++		"Help messages     :"
P{btuser opt errbox} ++			"Error messages    :"

# Initial cursor and starting row

N{btuser opt init cursor} = {btuser opt mode} - 2
N{btuser opt start row} = {btuser opt mode} - 1

[
Initial cursor and start display row for btuser opts
]
({btuser opt init cursor})	N	{btuser opt mode}
({btuser opt start row})	N	{btuser opt mode}

[
Order in which prompts happen
]
({btuser opt mode})		N	{btuser opt sort}
({btuser opt sort})		N	{btuser opt helpclr}
({btuser opt helpclr})		N	{btuser opt helpbox}
({btuser opt helpbox})		N	{btuser opt errbox}
({btuser opt errbox})		N	-1

[
Help and prompts are generated by multiplying by 10
]

P,H{btuser opt mode} * 10
"Display current user",
"Please specify what mode you want the program to
enter in by default"

P{btuser opt mode} * 10 + 1		"Update default modes"
P{btuser opt mode} * 10 + 2		"Read all users"
P{btuser opt mode} * 10 + 3		"Update all users"

P,H{btuser opt sort} * 10
"Numeric user id sort",
"Please indicate how you want user names sorted on entry"

P{btuser opt sort} * 10 + 1		"Alphabetic user name"
P{btuser opt sort} * 10 + 2		"Alphabetic group name"

P,H{btuser opt helpclr} * 10
"Use next command",
"Please indicate whether you want the next keystroke after
displaying a help message to be kept or lost"

P{btuser opt helpclr} * 10 + 1		"Clear help"

P,H{btuser opt helpbox} * 10
"Inverse video",
"Please indicate whether you want to display help messages
in inverse video (standout mode) or boxes."

P{btuser opt helpbox} * 10 + 1		"Boxes"

P,H{btuser opt errbox} * 10
"Inverse video",
"Please indicate whether you want to display error messages
in inverse video (standout mode) or boxes."

P{btuser opt errbox} * 10 + 1		"Boxes"

[
Ask about saving btq/btuser options
]

P,H{Save parameters} = 750
"Save parameters? ",
"Do you want to save the parameters just set to recover them
on entry?"

P,H{Save in current directory} ++
"Save in current directory? ",
"Do you want to save them in the current directory that they
will always take effect in this directory unless overridden
on the command line?
Your current directory is %s"

P,H{Save in home directory} ++
"Save in home directory? ",
"Do you want to save them in your home directory that they
will always take effect in this directory unless overridden
on the command line, current directory or environment?
Your home directory is %s"

P,H{Save format codes} = 754
"Save format codes?",
"Do you want to save the format field codes just set to
recover them on entry?"

P,H{btq fmt field width} = 756
"Width of field",
"This is the maximum width of the field"

P,H{btq fmt fmt code} ++
"Format code",
"This is the format code to assemble the display line from.
Possible codes follow:"

P,H{btq fmt sep val} ++
"Separator value",
"This is the field to generate the separator with. Enter nothing
to just give a single space."

E{Screen opts unknown command} = 760
"Unknown command - setting screen options"

[
xmbtq help stuff
]

H{xmbtq main help} = 1
"This is the main view menu for %P
Select:
     Options   -  Display and program options
     Action    -  ""One off"" actions
     Jobs      -  Operations on jobs
     Create    -  Create jobs and variables
     Delete    -  Delete jobs and variables
     Conditions-  Set interlocks on jobs
     Search    -  Search for jobs and variables
     Help      -  This screen"

H{xmbtq view job help} ++
"You are looking at the text of a job.
You can seqrch for specific strings."

H{xmbtq view syserr help} ++
"You are looking at the system error log file."

H{xmbtq jlist help} = ROUND(@, 5)
"This is a list of jobs visible to you
Please ""click"" on a job you want to work on."

H{xmbtq vlist help} ++
"This is a list of variables visible to you
Please ""click"" on a variable you want to work on."

H{xmbtq job macro help} ++
"Job macro menu - Choose from:
    Run command as macro - prompt for command to run
    Various commands with job number as parameter
    set up elsewhere in the help file %F"

H{xmbtq var macro help} ++
"Variable macro menu - Choose from:
    Run command as macro - prompt for command to run
    Various commands with variable name as parameter
    set up elsewhere in the help file %F"

H{xmbtq options menu help} ++
"This screen lets you choose from:
    View options   -  enlarge/restrict display
    Save options   -
    View system error log file
    Quit out of %P"

H{xmbtq action menu help} ++
"This menu provides various ""one offs"" on jobs
    Set runnable   -  enable job to run
    Set cancelled  -  hold job
    Force          -  bypass time constraints, run now
    Force/adv      -  bypass time constraints, run now,
                      advance time
    Interrupt      -  send interrupt signal to job
    Quit           -  send quit signal to job
    Other signal   -  send some other signal to job"

H{xmbtq jobs menu help} ++
"This menu lets you edit various job parameters
    View job       -  display job
    Set time       -  set time and repeat parameters
    Advance time   -  advance time to next repeat
    Title etc      -  set job title, priority,
                      command interpreter, load level
    Process        -  set process parameters
    Mail           -  set mail/write options
    Permissions    -  set job permissions
    Arguments      -  set arguments for job
    Environment    -  set environment variables
    Redirections   -  set I/O redirections"

H{xmbtq create menu help} ++
"This sets up various creation parameters
    Create job     -  create job from job file
    Time defaults  -  set default parameters for repeat times
    Condition defaults  -  set default parameters for conditions
    Assignment defaults -  set default parameters for assignments
    Create a variable
    Rename an existing variable
    View / edit the command interpreter list
    View / edit the holidays file"

H{xmbtq delete menu help} ++
"This does various deletes:
    Delete a job (optional confirmation)
    Delete a variable (ditto)
    Unqueue a job, deleting with save to file."

H{xmbtq conds menu help} ++
"This sets up job conditions:
    Conditions  -  Sets up constraints on jobs starting
    Assignments -  Job start/stop releases other jobs"

H{xmbtq vars menu help} ++
"Operations on variables
    Assign new value
    Change comment
    Set as exported (viewable elsewhere)
    Set as local only
    Change permissions
    Add, subtract, multiply, divide, take modulus
    with a constant value.
    Set the constant for above operations."

H{xmbtq search menu help} ++
"Search for jobs and variables
    Search for given string
    Repeat last search forwards/backwards."

H{xmbtq help menu help} ++
"This displays help."

H{xmbtq view job menu help} = ROUND(@, 20)
"Select:
    Exit - to quit display"

H{xmbtq view search menu help} ++
"Select:
    Search - search for string
    Search forwards - for previous string
    Search backwards - for previous string"

H{xmbtq view help menu help} ++
"This displays help"

H{xmbtq syse file menu help} ++
"Select:
    Clear log - to clear the log file and exit
    Exit - to quit display without clearing"

H{xmbtq syse help menu help} ++
"This displays help"

H{xmbtq view button help} = ROUND(@, 20)
"This button displays the contents of the
currently-selected job"

H{xmbtq delj button help} ++		"This button deletes the currently selected job."
H{xmbtq setrun button help} ++		"This button sets the currently-selected job as able to run."
H{xmbtq setcanc button help} ++		"This button puts the currently-selected job on hold."
H{xmbtq force button help} ++		"This button runs the job, ignoring the time."
H{xmbtq forceadv button help} ++
"This button runs the job, ignoring the time,
and also advances to the next time."
H{xmbtq intr button help} ++
"This button sends an interrupt signal to the
currently-selected job."
H{xmbtq quit button help} ++
"This button sends an quit signal to the
currently-selected job."
H{xmbtq settime button help} ++
"This button sets the time and repeat parameters for the
currently-selected job"
H{xmbtq jcond button help} ++
"This button sets the conditions for the
currently-selected job."
H{xmbtq jass button help} ++
"This button sets the assignments for the
currently-selected job."
H{xmbtq ass button help} ++
"This button assigns a new value to the currently-
selected variable."
H{xmbtq vadd button help} ++
"This button adds a constant value to the currently-
selected variable."
H{xmbtq vsub button help} ++
"This button subtracts a constant value from
the currently-selected variable."

H{xmbtq signal select dialog} = 500
"This lets you select which signal to
send to the job, or cancel to abort."

H{xmbtq display opts dialog} ++
"Use this dialog to select what you want to display,
and various other program options."

H{xmbtq search dialog} ++
"Use this dialog to select what you want to search for."

H{xmbtq edit ci dialog} ++
"Use this dialog to set up or edit command interpreter
details."

H{xmbtq ci display dialog} ++
"Use this dialog to display or edit the command interpreter
list."

H{xmbtq mw dialog} ++
"Use this dialog to display or edit the mail/write flags
for the currently-selected job."

H{xmbtq set time dialog} ++
"Use this dialog to display or edit the time and repeat
options for the currently-selected job."

H{xmbtq default time dialog} ++
"Use this dialog to display or edit default time options.
The new options will apply to new jobs you create, or new
time constraints which you add."

H{xmbtq titprill dialog} ++
"Use this dialog to edit the title, priority, command interpreter
and load level for the currently-selected job."

H{xmbtq procpar dialog} ++
"Use this dialog to edit the ""process parameters"",
directory, umask, ulimit, exit codes and export flags,
for the currently-selected job."

H{xmbtq condedit dialog} ++
"Use this dialog to edit the currently-selected condition."

H{xmbtq assedit dialog} ++
"Use this dialog to edit the currently-selected assignment."

H{xmbtq condlist dialog} ++
"Use this dialog to view/edit the condition list
for the currently-selected job."

H{xmbtq asslist dialog} ++
"Use this dialog to view/edit the assignment list
for the currently-selected job."

H{xmbtq argedit dialog} ++
"Use this dialog to view/edit the
currently-selected job argument."

H{xmbtq arglist dialog} ++
"Use this dialog to view/edit the argument list
for the currently-selected job."

H{xmbtq envedit dialog} ++
"Use this dialog to view/edit the
currently-selected job environment variable."

H{xmbtq envlist dialog} ++
"Use this dialog to view/edit the environment variable list
for the currently-selected job."

H{xmbtq rediredit dialog} ++
"Use this dialog to view/edit the
currently-selected job I/O redirection"

H{xmbtq redirlist dialog} ++
"Use this dialog to view/edit the redirection list
for the currently-selected job."

H{xmbtq unqueue dialog} ++
"Use this dialog to unqueue a job, saving to file.
Two files are created in a directory which you can choose:
    1.   A ""shell script"" file which will resubmit the
         job using the job file as data.
    2.   A ""job file"" which contains the data for
         your job."

H{xmbtq holidays dialog} ++
"Use this dialog to set holidays in various years"

H{xmbtq cval dialog} ++
"Use this dialog to reset the current ""constant""
used in quick variable arithmetic operations."

H{xmbtq vass dialog} ++
"Use this dialog to assign a new value to a variable."

H{xmbtq vcomment dialog} ++
"Use this dialog to reset the comment on a variable."

H{xmbtq vcreate dialog} ++
"Use this dialog to create a variable."

H{xmbtq renamevar dialog} ++
"Use this dialog to give a variable a new name."

H{xmbtq vperm dialog} ++
"Use this dialog to set permissions on a variable."

H{xmbtq jperm dialog} ++
"Use this dialog to set permissions on a job."

H{xmbtq jsearch dialog} ++
"Use this dialog to search for a string in a job."

H{xmbtq jlist fmt dialog} ++
"Use this dialog to set up the job display format.
You can add or remove fields from the display or vary the
size of existing fields."

H{xmbtq new job field} ++
"Use this dialog to create a field in the job display
As well as choosing the field you can set the width.
You can also set the ""backtab"" marker so if a field exceeds
the width it can backspace over the previous field."

H{xmbtq new job separator}++
"Use this dialog to create a separator string in the job display
If you just accept nothing a single space will be used."

H{xmbtq vlist fmt dialog} ++
"Use this dialog to set up the variable display"

H{xmbtq new var field} ++
"Use this dialog to create a field in the variable display
As well as choosing the field you can set the width.
You can also set the ""backtab"" marker so if a field exceeds
the width it can backspace over the previous field."

H{xmbtq new var separator} ++
"Use this dialog to create a separator string in the variable display
If you just accept nothing a single space will be used."

H{xmbtq copy to curr dir dialog} ++
"Use this dialog to copy out the options from the currently-selected
job to the current directory. Any new job submitted from this
directory (without other options) will include the options from
the job.
You can choose whether or not you want the jobs to start off in
""cancelled"" state or not, and whether you want ""verbose"" - i.e.
a message when the job is created."

H{xmbtq copy to curr dir dialog} + 1
"Use this dialog to copy out the options from the currently-selected
job to your home directory. Any new job submitted by you
will include the options from the job unless overridden in the
environment or the current directory.
You can choose whether or not you want the jobs to start off in
""cancelled"" state or not, and whether you want ""verbose"" - i.e.
a message when the job is created."

H{xmbtq set run time dialog} ++
"Use this dialog to set the maximum run time for a job."

H{xmbtq dir sel dialog} ++
"Please use this dialog to select the directory you want
to save in."

H{xmbtq sel file dialog} ++
"Please use this dialog to specify the file you want to submit."

H{xmbtr options dialog help} = 542
"Use this dialog to select your chosen editor, and whether
you need or want to invoke it in an ""xterm""."

H{xmbtr open dialog help} ++
"Use this dialog to ""open"" a job file."

H{xmbtr job file dialog help} ++
"Use this dialog to set up the file name to be used for
the job file - this is what the command interpreter
interprets."

H{xmbtr cmd file dialog help} ++
"Use this dialog to set up the file name to be used for
the command file - this contains the parameters for the
job."

H{xmbtr select dir dialog help} ++
"Use this to select the new current directory"

H{xmbtr select queue dialog help} ++
"Use this to select the queue, user, group names and verbose names."

P,H{xmbtq suspicious path name} = 550
"Suspicious path name - are you sure?",
"The path name `%t' you have given for the command
interpreter '%s' does not look like an executable
shell-like program to me."

P,H{xmbtq confirm delete ci} ++
"Are you sure that you want to delete this?",
"Please confirm that you really want to delete this
command interpreter."

P,H{xmbtq confirm delete job} ++
"Are you sure that you want to delete this job?",
"Please confirm that you really want to delete this
job. (You can avoid being asked this question by
setting ""view options"")."

P,H{xmbtq confirm delete var} ++
"Are you sure that you want to delete this variable?",
"Please confirm that you really want to delete this
variable. (You can avoid being asked this question by
setting ""view options"")."

P,H{xmbtq confirm delete log file} ++
"Are you sure that you want to clear the log file.",
"(I just thought I'd ask as it will go for good)."

P,H{Confirm write options} = 556
"Write options to file",
"This will write your currently-selected options
to a resources file ~/XI for selection next time."

P,H{xmbtr confirm close} ++
"Are you sure that you want to close this?",
"Please confirm that you really want to close this
job which has unsaved changes."

P,H{xmbtr confirm delete} ++
"Are you sure that you want to delete this job?",
"Please confirm that you really want to delete this
job which has unsaved changes."

P,H{xmbtr file exists confirm} ++
"File exists - are you sure?",
"Your proposed new file name clashes with an existing
file. This operation may overwrite it."

P,H{xmbtr file save confirm} ++
"No changes - are you sure?",
"The job has not changed since you last saved it.
This question is in case you selected the wrong job."

P,H{xmbtr job unchanged confirm} ++
"Job unchanged since last submit - are you sure?",
"You didn't change the job since the last job you submitted.
I thought I'd ask before you get identical jobs."

P,H{xbtr delete legacy job files} ++
"Delete legacy job files",
"This job is being saved in XML format, do you want to
delete the old legacy format files"

P,H{xbtr delete XML job file} ++
"Delete XML job file",
"This job is being saved in legacy format, do you want to
delete the XML job file"

[
Error messages for xmbtq/xmbtr etc
]

E,H{xmbtq null queue name} = 200
"Invalid (null) queue name.",
"You have not given the queue prefix you want.
Please enter it in the box given and try again."

E,H{xmbtq null user name} ++
"Invalid (null) user name.",
"You have not given the user name you want.
Please enter it in the box given and try again.
Various alternatives may be viewed by pressing
the select button."

E,H{xmbtq null group name} ++
"Invalid (null) group name.",
"You have not given the group name you want.
Please enter it in the box given and try again.
Various alternatives may be viewed by pressing
the select button."

E,H{xmbtq null ci name} ++
"Invalid (null) command interpreter name",
"You have not given the command interpreter you want.
Please enter it in the box given and try again.
Various alternatives may be viewed by pressing
the select button."

E,H{xmbtq var not found forw} ++
"Variable not found (searching forwards)",
"Could not find the variable you asked for, looking
forwards from the current position."

E,H{xmbtq var not found backw} ++
"Variable not found (searching backwards)",
"Could not find the variable you asked for, looking
backwards from the current position."

E,H{xmbtq job not found forw} ++
"Job not found (searching forwards)",
"Could not find the job you asked for, looking
forwards from the current position."

E,H{xmbtq job not found backw} ++
"Job not found (searching backwards)",
"Could not find the job you asked for, looking
backwards from the current position."

E,H{xmbtq no var search atts} ++
"Nothing about variables to search for.",
"You haven't set the flags to indicate what parts
of variables to search for."

E,H{xmbtq no job search atts} ++
"Nothing about jobs to search for.",
"You haven't set the flags to indicate what parts
of jobs to search for."

E,H{xmbtq null search string} ++
"Null search string",
"You didn't give a string to search for.
Please try again."

E,H{xmbtq no search string yet} ++
"No search string",
"You need to search for a string before you
can use the ""repeat last seach"" commands."

E,H{xmbtq inval ci name} ++
"Invalid command interpreter name",
"You gave a null command interpreter name, or
one not starting with a letter.
Please try again."

E,H{xmbtq ci name clash} ++
"Command interpreter name clashes",
"Your command interpreter name clashes with an existing
one. Please try again."

E,H{xmbtq ci path not abs} ++
"Command interpreter path not absolute.",
"Your command interpreter file name needs to be
""absolute"", i.e. starting with a ""/"" so that it
has the same effect regardless of the directory
the job runs in."

E,H{xmbtq selection not found} ++
"Did not find selection",
"Did not find command interp being updated.
(This may be a bug!)."

E,H{xmbtq no ci selected} ++
"No command interpreter selected",
"Please indicate the entry you want to edit by
clicking on it."

E,H{xmbtq deleting std ci} ++
"Attempting to delete standard shell %s",
"You are attempting to delete the standard shell
'%s' (%t) which is always required to be present."

E,H{xmbtq ci in use} ++
"Interpreter %s in use.",
"One or more jobs are currently using the
command interpreter %s, so you cannot delete it
yet."

E,H{xmbtq no time to adv} ++
"No time to advance.",
"There is no repeat specification on this job
so I cannot move on the time as I have no idea
what to advance it to."

E,H{xmbtq job running} ++
"Job is running",
"Your command is invalid at the moment because
the job is running (or at least starting up)."

E,H{xmbtq job not running} ++
"Job not running",
"You cannot kill job %d0 '%s' as it isn't running!"

E,H{xmbtq all days set} ++
"All days of week set",
"You cannot set all the days of the week
to be avoided."

E,H{xmbtq time passed} ++
"Time has passed",
"Your proposed time is now in the past - maybe
you took too long!"

E,H{xmbtq unknown ci} ++
"Unknown command interpreter",
"Your command interpreter is not recognised.
Use the select button to choose one."

E,H{xmbtq job op not permitted} ++
"Operation not permitted.",
"You cannot perform this operation on the
job '%s' (%d0) as the job permissions do not
allow it. Note that the job is currently owned
by %t."

E,H{xmbtq no job selected} ++
"No job selected.",
"You didn't select a job.
Click on the one you want to work on."

E,H{xmbtq no jobs to select} ++
"No jobs to select",
"There are no jobs currently being displayed
so you cannot do any job operations."

E,H{xmbtq null var name} ++
"Null variable name",
"You must give a variable name you want
to use in the job."

E,H{xmbtq invalid variable} ++
"Invalid variable",
"Your variable is not available for inclusion
in the job conditions, possibly because it
is not readable."

E,H{xmbtq var not writable} ++
"Invalid variable",
"Your variable is not available for inclusion
in the job assignments, possibly because it
is not writable."

E,H{xmbtq too many conds} ++
"Too many conditions.",
"You have too many conditions in your job - sorry"

E,H{xmbtq too many asses} ++
"Too many assignments",
"You have too many assignments in your job - sorry"

E,H{xmbtq no cond selected} ++
"No condition selected",
"Please click on the condition you want to work on."

E,H{xmbtq no ass selected} ++
"No assignment selected",
"Please click on the assignment you want to work on."

E,H{xmbtq no arg selected} ++
"No argument selected",
"Please click on the argument you want to work on."

E,H{xmbtq invalid environ} ++
"Invalid environment variable",
"You must specify environment variables in the
form ""Name=Value""."

E,H{xmbtq no environ selected} ++
"No environment variable selected.",
"Please click on the environment variable you want to work on."

E,H{xmbtq invalid file descr} ++
"Invalid file descriptor",
"The file descriptor must be numeric."

E,H{xmbtq fd too large} ++
"Invalid file descriptor",
"The file descriptor value is too large."

E,H{xmbtq invalid second fd} ++
"Invalid second file descriptor",
"The file descriptor must be numeric."

E,H{xmbtq second fd too large} ++
"Invalid second file descriptor",
"The file descriptor value is too large."

E,H{xmbtq null fill name} ++
"Null file name",
"You must give a file name to work on in
a redirection."

E,H{xmbtq no redir selected} ++
"No redirection selected.",
"Please click on the redirection you want to work on."

E,H{xmbtq unqueue no dir} ++
"No directory",
"You must give a directory to store the
unqueued job in."

E,H{xmbtq unqueue dir not abs} ++
"Directory not absolute",
"The directory name you give must start with
a ""/""."

E,H{xmbtq unq not dir} ++
"Not a directory",
"The file %s is not a directory.
Please try again."

E,H{xmbtq no cmd file name} ++
"Null command file name",
"You must give a file name to store the shell
command to recreate the job."

E,H{xmbtq no job file name} ++
"Null job file name",
"You must give a file name to store the job."

E,H{xmbtq unqueue fork failed} ++
"Fork failed",
"Could not fork off subprocess to unqueue job
Please try later (with less processes)."

E,H{xmbtq unqueue crashed} ++
"Unqueue process crashed with core dump",
"The unqueue process crashed with signal %d0
and produced a core dump - sorry about that."

E,H{xmbtq unqueue terminated} ++
"Unqueue process crashed.",
"The unqueue process crashed with signal %d0"

E,H{xmbtq unqueue misc error} ++
"Funny error in unqueue process",
"The unqueue process reported an error code
%d1 whilst performing unqueue of job %d0 ""%s""."

E,H{xmbtq no unq process} ++
"No unqueue process",
"The unqueue program does not seem to exist.
Please check that GNUbatch is installed correctly."

E,H{xmbtq unq dir not found} ++
"Directory not found",
"Could not access directory for unqueueing job."

E,H{xmbtq cannot chdir} ++
"Could not change directory.",
"Could not change directory for unqueueing job."

E,H{xmbtq cannot create cmdfile} ++
"Could not create command file",
"Unable to create command file for unqueueing job."

E,H{xmbtq unq job not found} ++
"Job not found",
"Could not find job %d0 to unqueue - sorry"

E,H{xmbtq unq cannot del} ++
"Could not delete job",
"The job was unqueued, but it couldn't be deleted
afterwards - sorry about that."

E,H{xmbtq unq cannot create config file} ++
"Failed to create configuration file - sorry.",
"I tried to create a "".gnubatch"" file to save
the selected options, but I could not."

E,H{xmbtq file not found} ++
"File not found",
"The file %s was not found"

E,H{xmbtq file not regular} ++
"File not a regular file",
"The file %s is not a flat file.
Please choose another file."

E,H{xmbtq could not submit job} ++
"Error submitting job",
"Error %d0 reported submitting job from file %s"

E,H{xmbtq cannot open file} ++
"Cannot open %s",
"Could not open %s to submit job"

E,H{xmbtq bad file type} ++
"Invalid file type %s",
"The file %s does not look like the job file of
an unqueued job. Please try again."

E,H{xmbtq submit job error} ++
"Error submitting job",
"Error %d0 reported submitting job from command file %s"

E,H{xmbtq no create permission} ++
"No create permission",
"You do not have permission to create new jobs."

E,H{xmbtq string not found forw} ++
"String not found (searching forwards)",
"Did not find your string in the file"

E,H{xmbtq string not found backw} ++
"String not found (searching backwards) ",
"Did not find your string in the file"

E,H{xmbtq view in progress} ++
"View in progress",
"Only one job file can be viewed at a time
to save system resources."

E,H{xmbtq cannot open net file} ++
"Cannot open network file",
"Cannot fetch your file over the network - sorry"

E,H{xmbtq cannot open job file} ++
"Cannot open file",
"Cannot open your job file - sorry"

E,H{xmbtq empty job file} ++
"Empty job file",
"The is no data in your job - this is not
necessarily an error though."

E,H{xmbtq cannot open syslog} ++
"Cannot open system log file",
"There is no system error log (yet!)."

E,H{xmbtq syse view in progress} ++
"View in progress",
"Only one file can be viewed at a time
to save system resources."

E,H{xmbtq cannot open hols} ++
"Cannot read holidays file",
"There is no holidays file and you
do not have permission to create one."

E,H{xmbtq cannot create hols} ++
"Error creating holidays file",
"Cannot create holidays file - sorry."

E,H{xmbtq no var access perm} ++
"No permission on variable %s",
"You do not have the permission to access the variable
which you have chosen for the given operation.
The variable is owned by %t."

E,H{xmbtq no var selected} ++
"No variable selected.",
"Please click on the variable you want to access."

E,H{xmbtq no vars to select} ++
"No variables to select.",
"You are not displaying any variables to select.
Try selecting ""view options"" to display some?"

E,H{xmbtq invalid const var} ++
"Invalid constant value",
"The arithmetic constant must be (signed) numeric."

E,H{xmbtq value not arith} ++
"Value not arithmetic",
"You can only do arithmetic on numeric-valued variables."

E,H{xmbtq divide by zero} ++
"Divide by zero attempted",
"Your constant value is zero so you cannot divide by it."

E,H{xmbtq invalid var name} ++
"Invalid variable name",
"Variable names must be alphanumeric or _
starting with a letter"

E,H{xmbtq varname too long} ++
"Variable name too long",
"Variable names are limited to %d0 characters."

E,H{xmbtq no var create perm} ++
"No create permission",
"You do not have the privilege to create variables."

E,H{xmbtq renaming remote var} ++
"Renaming remote variable",
"Variables may only be renamed on the machine they belong to - sorry."

E,H{xmbtq invalid user} ++
"Invalid user name",
"You have specified an invalid user name"

E,H{xmbtq invalid group} ++
"Invalid group name",
"You have specified an invalid group name"

E,H{xmbtq macro prompt missing} ++
"No prompt %d0 in this file????",
"There is no ""prompt"" for macro command %d0 in this file"

E,H{xmbtq macro cannot fork} ++
"Job Macro - cannot fork - %E",
"Failed to do a ""fork"" to run a job macro.
Your machine is running out of processes."

E,H{xmbtq job macro signal} ++
"Job Macro failed with signal %d0",
"Your job macro command failed with some signal."

E,H{xmbtq job macro exit code} ++
"Job Macro gave exit code %d0",
"Your job macro failed, giving an exit code
of %d0. I don't know what this means!"

E,H{xmbtq var macro cannot fork} ++
"Variable Macro - cannot fork - %E",
"Failed to do a ""fork"" to run a variable macro.
Your machine is running out of processes."

E,H{xmbtq var macro signal} ++
"Variable Macro failed with signal %d0",
"Your variable macro command failed with some signal."

E,H{xmbtq var macro exit code} ++
"Variable Macro gave exit code %d0",
"Your variable macro failed, giving an exit code
of %d0. I don't know what this means!"

E,H{xmbtr cannot fork for editor} = 302
"Cannot fork",
"Sorry, but I cannot fork to create your editor process."

E,H{xmbtr cannot execute editor} ++
"Cannot execute editor.",
"Maybe your editor name ""%s"" is incorrect."

E,H{xmbtr cannot open cmd file} ++
"Cannot open command file.",
"Sorry, cannot open the command file for the selected job."

E,H{xmbtr bad fmt cmd file} ++
"Bad format command file",
"Sorry, but the command file is of incorrect format."

E,H{xmbtr cannot write cmd file} ++
"Cannot write command file",
"Sorry, cannot write the command file for the selected job."

E,H{xmbtr file already loaded} ++
"File already loaded.",
"I think that you are already looking at this file."

E{xmbtr job created ok title} ++	"Job `%s' created successfully - job number %d0"
E{xmbtr job created ok no title} ++	"Job number %d0 created successfully"

E{xmbtr default not future} ++
"The time in the loaded defaults was not in the future.
You might want to adjust this and resave!"

E{xmbtr loaded job not future} ++
"The specified time in the loaded job was not in the future.
You might want to adjust this!"

E,H{xmbtr cannot submit not future} ++
"Cannot submit job. Time is not in the future.",
"You cannot submit this job in this form as the time is in
the past. Please reset the start time and try again."

E,H{xmbtr no job file name} ++
"No job file name.",
"Before you can do this operation you need to give
a file name to the job. Select from the file menu"

E,H{xmbtr no cmd file name} ++
"No command file name.",
"Before you can do this operation you need to give
a file name to the command options. Select from the file menu."

E,H{xmbtr no such file} ++
"File does not exist.",
"Your file name has not been created yet."

E,H{xmbtr invalid directory} ++
"Invalid job directory",
"The proposed directory name - %s
does not start with /"

E,H{xmbtr cannot open job file} ++
"Cannot open job file",
"Cannot open your proposed job file.
Maybe the permissions are not right."

E,H{xmbtr cannot delete job file} ++
"Error - %E",
"I could not delete the specified job file for the specified
job/command pair"

E,H{xmbtr cannot delete cmd file} ++
"Error - %E",
"I could not delete the specified job file for the specified
job/command pair"

E{xmbtr remote job created ok title} ++		"Job `%s' created successfully on %N1 - job number %d0"
E{xmbtr remote job created ok no title} ++	"Job number %d0 created successfully on %N1"

E,H{xbtq invalid number} ++
"Invalid number",
"The value field is expecting a numeric value.
Maybe set text value check on?"

E,H{xmbtq exporting remote var} ++
"Exporting remote variable",
"You can only set export/local on a local variable"

E,H{xbtq macname null} ++
"No macro name",
"You must give a name to the macro"

E,H{xbtq empty macro command} ++
"Empty macro command",
"You didn't give a command to run"

E,H{xmbtq path name too long} ++
"Path name too long",
"The path name %t for cmd int %s
is too long - please use a shorter one"

E,H{xmbtq args too long} ++
"Args too long",
"The argument list is too long - please use a shorter one"

E,H{xbtq no ifnposs selected} ++
"No if not possible selected",
"You didn't specify an action if the job cannot be
executed - please specify one."

E,H{xbtq no open job file} = 340
"Could not open job file",
"Could not open the job file selected"

E,H{xbq invalid format job file} ++
"Invalid format job file",
"Job file has invalid format.
Maybe it is not an XML one?"

E,H{xbq invalid conditions} ++
"Invalid conditions in job file",
"The job file has invalid saved conditions"

E,H{xbq invalid assignments} ++
"Invalid assignments in job file",
"The job file has invalid assignments"

E,H{xbq too many strings} ++
"Too many strings in job file",
"The build job structure size is too small
to accommodate the strings set in the job file"

E,H{xbq cannot write job file} ++
"Cannot write job file",
"Unable to save job file for user %R"

E,H{xbtr no script} ++
"No script on job",
"Cannot save job until script has been supplied"

[
xmbtr help
]

H{xmbtr main help} = 1
"This is the main view menu for %P
Select:
     Options   -  Set options and directories
     Defaults  -  Set default parameters for new jobs
     File      -  Open, edit and save job files
     Jobs      -  Set parameters for jobs
     Help      -  This screen"

H{xmbtr jlist help} ++
"This is a list of jobs which you have ""open"".
Please ""click"" on a job you want to work on, and
and choose a menu item."

H{xmbtr options menu help} ++
"This screen lets you choose from:
    View options   -  Display and edit program options
    Save options   -  Save to file
    Select a new working directory
    Load defaults from current directory
    Save defaults to current directory
    Load defaults from home directory
    Save defaults to home directory
    Quit out of %P"

H{xmbtr defaults menu help} ++
"This menu sets up default options
You can save the defaults to the current or home directories. 
    Queue          -  set queue name
    Set runnable   -  jobs should be ready to run when submitted
    Set cancelled  -  jobs should be in cancelled state
    Time           -  Set time parameters
    Title          -  Set title, priority and load level
    Process        -  Set process parameters
    Mail           -  Set mail and write flags
    Permissions    -  Set permissions
    Args           -  Set job arguments
    Environment    -  Set environment variables
    Redirs         -  Set redirections
    Conds          -  Set conditions
    Assignments    -  Set assignments"

H{xmbtr files menu help} ++
"This menu lets you manipulate job files
    New job        -  Create a new job
    Open           -  Open existing job file
    Close          -  Close job file
    Job file       -  Set name of job file
    Cmd file       -  Set name of command file
    Save           -  Save job file
    Edit           -  Edit job file
    Delete         -  Delete job file
    Submit         -  Submit job"

H{xmbtr jobs menu help} ++
"This menu sets up options for a selected job
First click on the job you want to edit.
    Queue          -  set queue name
    Set runnable   -  jobs should be ready to run when submitted
    Set cancelled  -  jobs should be in cancelled state
    Time           -  Set time parameters
    Title          -  Set title, priority and load level
    Process        -  Set process parameters
    Mail           -  Set mail and write flags
    Permissions    -  Set permissions
    Args           -  Set job arguments
    Environment    -  Set environment variables
    Redirs         -  Set redirections
    Conds          -  Set conditions
    Assignments    -  Set assignments"

H{xmbtr help menu help} ++
"This provides on line help"

H{xmbtr open job button help} = ROUND(@, 10)
"This button opens a job file."

H{xmbtr edit job button help} ++
"This button edits the currently selected job."

H{xmbtr set runnable button help} ++
"This button sets the currently-selected job as able to run."

H{xmbtr set canc button help} ++
"This button puts the currently-selected job on hold."

H{xmbtr set time button help} ++
"This button sets the time and repeat parameters for the
currently-selected job"

H{xmbtr set conds button help} ++
"This button sets the conditions for the
currently-selected job."

H{xmbtr set asses button help} ++
"This button sets the assignments for the
currently-selected job."

H{xmbtr submit button help} ++
"This button submits the currently-selected job."

H{xmbtr remote submit button help} ++
"This button submits the currently-selected job to another machine."

[
Menu help for xmbtuser
]

H{xmbtuser main menu} = 1
"You are looking at the main screen of the user privilege viewer
Choose:
     Options - to select different order
     Defaults- to select default parameters
     Users   - to select user parameters
     Charges - to select user changes
     Help    - to view this screen"

H{xmbtuser deflist help} ++
"These are the default options which apply to new users"

H{xmbtuser ulist help} ++
"These are the parameters for the listed users"

H{xmbtuser options menu} ++
"Options menu - Choose from:
    Display options - order display
                      as user-id, user or group name
                      Set increments for load levels
    Exit from %P"

H{xmbtuser defaults menu} ++
"Defaults menu - sets defaults for new users
Choose from:
    Priorities - set default limits of priority
    Load level - set default load levels
    Modes      - set default job and variable permissions
    Privileges - set default privileges

    Copy       - copy defaults to existing users"

H{xmbtuser user menu} ++
"Users menu - Select one or more users and choose from:
    Priorities - set limits of priority
    Load level - set load levels
    Modes      - set default job and variable permissions
    Privileges - set privileges

    Copy       - copy defaults to selected users"

H{xmbtuser search menu} = 8
"Search menu - Choose from:
    Search - for particular string
    Search forwards - using same
      parameters as last time
    Search backwards - using same
      parameters as last time"

H{xmbtuser help menu} ++
"Help menu - Choose from:
    Help - general information
    Help on - specific menu item
    About - display version information"

H{xmbtuser user macro menu} ++
"User macro menu - Choose from:
    Run command as macro - prompt for command to run
    Various commands with user names as parameters
    set up elsewhere in file %F with codes 27000+"

H{xmbtuser options dialog} = ROUND(@, 10)
"This menu lets you select the order in which
user names are displayed, and the increments for
load levels.
You can choose from:
    Sort by order of numeric user id
    Alphabetic order of user name
    Alphabetic order of group name (and then user)
You can set the value to which load levels are
rounded. (This itself will be rounded)."

H{xmbtuser default prio dialog} ++
"This menu selects the priorities which will be
automatically assigned to new users.
Use the slides to select:
    Minimum priority
    Default priority
    Maximum priority"

H{xmbtuser user prio dialog} ++
"This menu selects the priorities for the selected users

Use the slides to select:
    Minimum priority
    Default priority
    Maximum priority"

H{xmbtuser default llev dialog} ++
"This menu selects the load levels which will be
automatically assigned to new users."

H{xmbtuser user llev dialog} ++
"This menu selects the load levels for the specified
users."

H{xmbtuser default perm dialog} ++
"This menu selects the default job and variable
permissions which will be automatically assigned
to new users."

H{xmbtuser user perm dialog} ++
"This menu selects the default job and variable
permissions for the specified users.
Use the push button to set the default."

H{xmbtuser default priv dialog} ++
"This menu selects the privileges which will be
automatically assigned to new users."

H{xmbtuser user priv dialog} ++
"This menu selects the privileges for the selected users
Use the ""Set Default"" button to set to the defaults."

H{xmbtuser search menu help} = 30
"Use this menu to set up which kind of search you want to
do for particular users.
You can also set up whether you want the search to wrap around
and whether you want exact upper/lower-case matches of strings.
Use ""."" to signify a wild card."

[
Confirmation prompts and help messages
]

P,H{xmbtuser copy privs to everyone} = ROUND(@, 50)
"Copy to everyone else (but you)?",
"After editing the default privileges, you can
now choose to assign these defaults to all other users
(apart from yourself)."

[
Error messages and corresponding help
]

E,H{xmbtuser no users selected} = ROUND(@, 100)
"No users selected - please choose some",
"Please select one or more users by clicking the left mouse
button on the users, using Ctrl+left button to select more
users, then try this again."

E,H{xmbtuser minp gt maxp} ++
"Minimum priority > Maximum priority",
"You are trying to set a minimum priority greater
than the maximum. I think that this is a mistake."

E,H{xmbtuser defp lt minp} ++
"Default priority < Minimum priority",
"You are trying to set a default priority less
than the minimum. I think that this is a mistake."

E,H{xmbtuser defp gt maxp} ++
"Default priority > Maximum priority",
"You are trying to set a default priority greater
than the maximum. I think that this is a mistake."

E,H{xmbtuser maxll gt totll} ++
"Maximum load level > Total load level",
"You have set the maximum load level (for one job)
to a higher value that the total load level (for all jobs).
I think that this is a mistake."

E,H{xmbtuser specll gt maxll} ++
"Special load level > Maximum load level",
"You have set the special load level
to a higher value that the maximum load level
I think that this is a mistake."

E,H{xmbtuser specll gt totll} ++
"Special load level > Total load level",
"You have set the special load level
to a higher value that the total load level
I think that this is a mistake."

E,H{xmbtuser macro not defined} = 108
"Macro command %d0 not defined.",
"There is no macro command set up for %d0 in %F
You need to set one up so that it can be run
to implement the macro command."

E,H{xmbtuser macro fork failed} ++
"Macro command failed to fork - %E",
"Fork failed trying to run the macro command
The machine is probably full up with processes"

E,H{xmbtuser macro gave signal} ++
"Macro command failed - signal %d0",
"The macro command failed with a signal number %d0"

E,H{xmbtuser macro error exit} ++
"Macro command failed - exit code %d0",
"The macro command failed with an exit code of %d0
They are expected to exit with a zero exit code."

E,H{Fsearch user not found} ++
"User not found",
"No matching user was found searching forwards."

E,H{Rsearch user not found} ++
"User not found",
"No matching user was found searching backwards."

E,H{xmbtuser no search string} ++
"No search string given",
"You didn't give a string to search for."

E,H{xmbtuser no previous search} ++
"No existing search string.",
"You cannot do this operation until you have
given %P a search string on this instance."

[
Btq explain
]

E{btq explain} = 80
"%P - options
       -a   require confirmation of abort (default)
       -A   suppress confirmation requests
       -s   retain job/vars at current screen position
       -N   normal handling of job/var position (default)
       -j   start off in jobs screen
       -v   start off in vars screen
       -h -H lose/do not lose next character on help
       -b -B put boxes/do not put boxes round help
       -m -M put boxes/do not put boxes round errors
       -l   display local jobs only
       -r   display network wide jobs
       -q p only display jobs with the prefix pattern p
       -z -Z include/suppress jobs with null queue name
       -u user -g group
       -X xml format -D separate file format"

[
Btuser explain
]

E{btuser explain} = 90
"%P           display privileges and modes
%P -m        reset your modes
%P -v        display admin file
%P -i        update admin file
%P -u        sort users by user name
%P -g        sort users by group name
%P -n        sort users into numeric order
%P -h -H     lose/do not lose next character on help
%P -b -B     put boxes/do not put boxes round help
%P -e -E     put boxes/do not put boxes round errors
with -v/-i options append -u -g -n (default) to sort into user
group or numeric uid order."

[
Macro prompts - add to this??
]

P,H{Job or User macro} = 27000
"Run what:",
"Please give the command you want to run.
Details from the current line will be given as a parameter."

P,H{Var macro} = 27100
"Run what:",
"Please give the command you want to run.
The variable name will be given as a parameter."

[
Strings for wnum etc
]

E{wgets no defaults} = 4000		"No defaults found with prefix %s"
E{wgets string too long} ++		"String too long"
E{wgets unknown command} ++		"Unknown command (expecting string)"
E{wgets invalid char} ++		"Invalid character (expecting string)"

S{Wnum no field name} = 5
S{Wnum octal offset} = 10

[
Wnum errors add 5 if no arg.
Add 10 if octal
]

E{wnum value too large} = ROUND({wgets invalid char}, 10)"Value for %s is too large. (Maximum is %d1) Please try again."
E{wnum value too small} ++		"Value for %s is too small. (Minimum is %d0) Please try again."
E{wnum unknown cmd} ++			"Unknown command (expecting numeric value for %s)"
E{wnum invalid char} ++			"Invalid character (expecting numeric value for %s)"

E{wnum value too large} + {Wnum no field name}	"Value is too large. (Maximum is %d1) Please try again."
E{wnum value too small} + {Wnum no field name}	"Value is too small. (Minimum is %d0) Please try again."
E{wnum unknown cmd} + {Wnum no field name}	"Unknown command (expecting numeric value)"
E{wnum invalid char} + {Wnum no field name}	"Invalid character (expecting numeric value)"

[
whexnum errors (currently only ulimit)
]

E{whexnum unknown command} = ROUND(@, 10)	"Unknown command (expecting hex value for %s)"
E{whexnum invalid char} ++			"Invalid character (expecting hex value for %s)"
E{whexnum num too large} ++			"Value is too large (expecting hex value for %s)"
E{whexnum num too small} ++			"Value is too small (expecting hex value for %s)"

E{whexnum unknown command} + {Wnum no field name}"Unknown command (expecting hex value)"
E{whexnum invalid char} + {Wnum no field name}	"Invalid character (expecting hex value)"
E{whexnum num too large} + {Wnum no field name}	"Value is too large (expecting hex value)"
E{whexnum num too small} + {Wnum no field name}	"Value is too small (expecting hex value)"

E{whexnum unknown command} + {Wnum octal offset}"Unknown command (expecting octal value for %s)"
E{whexnum invalid char} + {Wnum octal offset}	"Invalid character (expecting octal value for %s)"
E{whexnum num too large} + {Wnum octal offset}	"Value is too large (expecting octal value for %s)"
E{whexnum num too small} + {Wnum octal offset}	"Value is too small (expecting octal value for %s)"

E{whexnum unknown command} + {Wnum no field name} + {Wnum octal offset}"Unknown command (expecting octal value)"
E{whexnum invalid char} + {Wnum no field name} + {Wnum octal offset}	"Invalid character (expecting octal value)"
E{whexnum num too large} + {Wnum no field name} + {Wnum octal offset}	"Value is too large (expecting octal value)"
E{whexnum num too small} + {Wnum no field name} + {Wnum octal offset}	"Value is too small (expecting octal value)"

# Prompts for xbtuser
[
Xbtuser prompts etc
]

P{xbtuser user hdr} = 1400	"User"
P{xbtuser grp hdr} ++ COPY	"Group"
P{xbtuser def hdr} ++ COPY	"Def"
P{xbtuser min hdr} ++ COPY	"Min"
P{xbtuser max hdr} ++ COPY	"Max"
P{xbtuser maxll hdr} ++ COPY	"Maxll"
P{xbtuser totll hdr} ++ COPY	"Totll"
P{xbtuser specll hdr} ++ COPY	"Specll"
P{xbtuser priv hdr} ++ COPY	"Priv"

P{xbtuser default string} = ROUND(@, 10) 	"DEFAULT"

P{xbtuser app title} = ROUND(@, 5) "Set batch user permissions"

P{xbtuser udlgtit} ++		"User settings"
P{xbtuser ddlgtit} ++		"Default settings"
P{xbtuser deflab} ++		"Defaults"

P{xbtuser framelab pris} = ROUND(@, 5) "User priority ranges"
P{xbtuser pdlg minp} ++		"Minimum priority"
P{xbtuser pdlg defp} ++		"Default priority"
P{xbtuser pdlg maxp} ++		"Default priority"
P{xbtuser framelab lls} = ROUND(@, 5) "User loadlevel limits"
P{xbtuser lldlg maxll} ++	"Maximum load level (one job)"
P{xbtuser lldlg totll} ++	"Total load level for user"
P{xbtuser lldlg specll} ++	"Special create load level"

P{xbtuser framelab jmode} = ROUND(@, 5) "Job default permissions"
P{xbtuser framelab vmode} = ROUND(@, 5) "Variable default permissions"
P{xbtuser framelab privs} = ROUND(@, 5) "Privileges"
P{xbtuser priv setdef} ++		"Set default"

P{xbtuser frametab pri}	= 1455	"Priorities"
P{xbtuser frametab ll} ++		"Load levels"
P{xbtuser frametab jmode} ++		"Job perms"
P{xbtuser frametab vmode} ++		"Var perms"
P{xbtuser frametab privs} ++		"Privileges"

[
Xbtq prompts etc
]
P{xbtq seq hdr} = 1500			"Seq"
P{xbtq jnum hdr} ++			"Jnum"
P{xbtq queue hdr} ++			"Queue"
P{xbtq qtit hdr} ++			"Title"
P{xbtq title hdr} ++			"Q:title"
P{xbtq progress hdr} ++			"Progress"
P{xbtq pid hdr} ++			"Pid"
P{xbtq juname hdr} ++			"User"
P{xbtq jgname hdr} ++			"Group"
P{xbtq jmode hdr} ++			"Perm"
P{xbtq orighost hdr} ++			"Orig host"
P{xbtq jexport hdr} ++			"Expt"
P{xbtq jremrun hdr} ++			"Remrn"
P{xbtq interp hdr} ++			"Cmdint"
P{xbtq prio hdr} ++			"Prio"
P{xbtq loadlev hdr} ++			"LL"
P{xbtq time hdr} ++			"Time"
P{xbtq fulltime hdr} ++			"Time"
P{xbtq avoid hdr} ++			"Avoid"
P{xbtq hols hdr} ++			"Hols"
P{xbtq repeat hdr} ++			"Repeat"
P{xbtq ifnp hdr} ++			"Ifnp"
P{xbtq scond hdr} ++			"Cond"
P{xbtq fcond hdr} ++			"Condition"
P{xbtq sass hdr} ++			"Ass"
P{xbtq fass hdr} ++			"Assignment"
P{xbtq umask hdr} ++			"Umask"
P{xbtq dir hdr} ++			"Dir"
P{xbtq args hdr} ++			"Args"
P{xbtq env hdr} ++			"Env"
P{xbtq redirs hdr} ++			"Redirs"
P{xbtq exits hdr} ++			"Exits"
P{xbtq exit hdr} ++			"Exit"
P{xbtq signal hdr} ++			"Sig"
P{xbtq otime hdr} ++			"Otime"
P{xbtq itime hdr} ++			"Itime"
P{xbtq sttime hdr} ++			"Stime"
P{xbtq endtime hdr} ++			"Endtime"
P{xbtq deltime hdr} ++			"Deltime"
P{xbtq runtime hdr} ++			"Runtime"
P{xbtq gracetime hdr} ++		"Grace"
P{xbtq autoksig hdr} ++			"Autosig"
P{xbtq vseq hdr} ++			"Seq"
P{xbtq vname hdr} ++			"Name"
P{xbtq vval hdr} ++			"Value"
P{xbtq vexport hdr} ++			"Export"
P{xbtq vclust hdr} ++			"Cluster"
P{xbtq vcomment hdr} ++			"Comment"
P{xbtq vuname hdr} ++			"User"
P{xbtq vgname hdr} ++			"Group"
P{xbtq vmode hdr} ++			"Perm"

P{xbtq seq full descr} = ROUND(@, 10)	"Number in queue"
P{xbtq jnum full descr} ++ COPY		"Job number"
P{xbtq queue full descr} ++ COPY	"Queue name"
P{xbtq qtit full descr} ++ COPY		"Job Title"
P{xbtq title full descr} ++ COPY	"Queue:title"
P{xbtq progress full descr} ++ COPY	"Job Progress"
P{xbtq pid full descr} ++ COPY		"Process id"
P{xbtq juname full descr} ++ COPY	"Job user"
P{xbtq jgname full descr} ++ COPY	"Job Group"
P{xbtq jmode full descr} ++ COPY	"Permissions"
P{xbtq orighost full descr} ++ COPY	"Orig host"
P{xbtq jexport full descr} ++ COPY	"Export"
P{xbtq jremrun full descr} ++ COPY	"Remote run"
P{xbtq interp full descr} ++ COPY	"Command interpreter"
P{xbtq prio full descr} ++ COPY		"Priority"
P{xbtq loadlev full descr} ++ COPY	"Load Level"
P{xbtq time full descr} ++ COPY		"Time next run"
P{xbtq fulltime full descr} ++ COPY	"Time next run in full"
P{xbtq avoid full descr} ++ COPY	"Avoiding days"
P{xbtq hols full descr} ++ COPY		"Holidays"
P{xbtq repeat full descr} ++ COPY	"Repeat type"
P{xbtq ifnp full descr} ++ COPY		"If not possible"
P{xbtq scond full descr} ++ COPY	"Conditions"
P{xbtq fcond full descr} ++ COPY	"Conditions in full"
P{xbtq sass full descr} ++ COPY		"Assignments"
P{xbtq fass full descr} ++ COPY		"Assignments in full"
P{xbtq umask full descr} ++ COPY	"Umask"
P{xbtq dir full descr} ++ COPY		"Directory"
P{xbtq args full descr} ++ COPY		"Arguments"
P{xbtq env full descr} ++ COPY		"Environment"
P{xbtq redirs full descr} ++ COPY	"Redirectionss"
P{xbtq exits full descr} ++ COPY	"Exit code actions"
P{xbtq exit full descr} ++ COPY		"Last exit code"
P{xbtq signal full descr} ++ COPY	"Last signal number"
P{xbtq otime full descr} ++ COPY	"Original submit time"
P{xbtq itime full descr} ++ COPY	"Last action time"
P{xbtq sttime full descr} ++ COPY	"Start time"
P{xbtq endtime full descr} ++ COPY	"End time"
P{xbtq deltime full descr} ++ COPY	"Delete time"
P{xbtq runtime full descr} ++ COPY	"Run time"
P{xbtq gracetime full descr} ++ COPY	"Grace period time"
P{xbtq autoksig full descr} ++ COPY	"Auto signal number"

P{xbtq vseq full descr} = ROUND(@, 10)	"Number in list"
P{xbtq vname full descr} ++ COPY	"Name of variable"
P{xbtq vval full descr} ++ COPY		"Current Value"
P{xbtq vexport full descr} ++ COPY	"Variable Exported"
P{xbtq vclust full descr} ++ COPY	"Variable Clustered"
P{xbtq vcomment full descr} ++ COPY	"Variable Comment"
P{xbtq vuname full descr} ++ COPY	"Owner User"
P{xbtq vgname full descr} ++ COPY	"Owner Group"
P{xbtq vmode full descr} ++ COPY	"Permissions"

P{xbtq app title} = ROUND(@, 5)		"GNUbatch Queue Viewer"
P,H{xbtq changes not saved ok} ++
"Changes not saved - OK?",
"You didn't save your changes to program options.
Please confirm if you want to exit anyway."

P{xbtq search jvlist} ++		"Search for job or variable"
P{xbtq search str} ++			"Search string . wild"
P{xbtq search jorv} ++			"Variable not job"
P{xbtq search title} ++			"Search in title"
P{xbtq search user} ++			"Search user name"
P{xbtq search group} ++			"Search group name"
P{xbtq search vname} ++			"Search variable name"
P{xbtq search vcomment} ++		"Search variable comment"
P{xbtq search vval} ++			"Search variable value"
P{xbtq search backw} ++			"Search backwards"
P{xbtq search wrap} ++			"Wrap search"
P{xbtq search match} ++			"Match case"

P{xbtq viewopt dlgtit} = ROUND(@, 5)	"Set display options"
P{xbtq viewopt queue lab} ++		"Queue(s) to display"
P{xbtq viewopt users lab} ++		"User(s) to display"
P{xbtq viewopt groups lab} ++		"Group(s) to display"
P{xbtq viewopt include null} ++		"Include null queue"
P{xbtq viewopt remote hosts} ++		"Display remote jobs/vars"
P{xbtq viewopt confirm delete} ++	"Confirm delete jobs/vars"
P{xbtq viewopt save XML} ++             "Unqueue jobs as XML single files"

P{xbtq macname dlg} = ROUND(@, 5)	"Set macro name"
P{xbtq macname lab} ++			"Macro name"
P,H{xbtq addmac to list} ++
"Add macro to list",
"Click OK if you want to add this macro to the list"
P{xbtq mac dlg} ++			"Edit macro"
P{xbtq macdlg cmd} ++			"Command to run"
P{xbtq macdlg descr} ++			"Description"
P{xbtq macedit dlg} ++			"Edit macros"
P{xbtq new macro} ++			"Create new"
P{xbtq del macro} ++			"Delete macro"
P{xbtq edit macro} ++			"Edit macro"

P{xbtq kill type dlg} = ROUND(@, 5)	"Select kill signal for job"
P{xbtq kill type lab} ++		"Kill job number"
P{xbtq kill descr Stopsig} ++		"Stop signal"
P{xbtq kill descr Contsig} ++		"Continue signal"
P{xbtq kill descr Termsig} ++		"Terminate signal"
P{xbtq kill descr Killsig} ++		"Kill signal"
P{xbtq kill descr Hupsig} ++		"Hangup signal"
P{xbtq kill descr Intsig} ++		"Interrupt signal"
P{xbtq kill descr Quitsig} ++		"Quit signal"
P{xbtq kill descr Alarmsig} ++		"Alarm signal"
P{xbtq kill descr Bussig} ++		"Bus error signal"
P{xbtq kill descr Segvsig} ++		"Segment violoation signal"

P{xbtq set time dlg} = ROUND(@, 5)	"Set time options"
P{xbtq set time lab} ++			"Setting time for"
P{xbtq job has time set} ++		"Job has time set"
P{xbtq repeat opt lab} ++		"Repeat option"
P{xbtq avoiding days frame lab} ++	"Days to avoid"
P{xbtq if not poss lab} ++		"If not possible"
P,H{xbtq time in past cont} ++
"Time has passed - continue",
"You have specified a time which has passed.
Please confirm this is not a mistake."

P{xbtq job props dlgtit} = ROUND(@, 5)	"Job properties"
P{xbtq job props lab} ++		"Setting properties for"
P{xbtq framelab titprill} ++		"Title, priority, load level"
P{xbtq titprill title} ++		"Job title"
P{xbtq titprill pri} ++			"Priority"
P{xbtq titprill ll} ++			"Load level"
P{xbtq titprill cmdint} ++		"Command interpreter"
P{xbtq framelab procpar} ++		"Process parameters"
P{xbtq procpar direct} ++		"Directory"
P{xbtq framelab umask} ++		"Umask - turn OFF perms for"
P{xbtq umask lab UR} ++			"User Read"
P{xbtq umask lab UW} ++ COPY		"User Write"
P{xbtq umask lab UX} ++ COPY		"User Execute"
P{xbtq umask lab GR} ++	COPY		"Group Read"
P{xbtq umask lab GW} ++ COPY		"Group Write"
P{xbtq umask lab GX} ++ COPY		"Group Execute"
P{xbtq umask lab OR} ++	COPY		"Others Read"
P{xbtq umask lab OW} ++ COPY		"Others Write"
P{xbtq umask lab OX} ++ COPY		"Others Execute"
P{xbtq procpar ulimit} ++		"Ulimit"
P{xbtq procpar bytemult} ++		"Bytes"
P{xbtq procpar kmult} ++ COPY		"KB"
P{xbtq procpar mmult} ++ COPY		"MB"
P{xbtq procpar normal exit} ++		"Exit code range - normal"
P{xbtq procpar error exit} ++		"Exit code range - error"
P{xbtq procpar noadve} ++		"Do not advance time on error"
P{xbtq procpar exporttype} ++		"Local only"
P{xbtq procpar exporttype exp} ++ COPY	"Exported"
P{xbtq procpar exporttype rr} ++ COPY	"Remote runnable"
P{xbtq select directory} ++		"Select directory for job"
P{xbtq framelab timelim} ++		"Time limits"
P{xbtq timelim deltime} ++		"Delete time"
P{xbtq procpar clear delt} ++		"Clear"
P{xbtq timelim runtime} ++		"Run time"
P{xbtq timelim clear runt} ++		"Clear"
P{xbtq timelim runtime sig} ++		"Signal to use"
P{Autoksig termsig} ++			"Terminate"
P{Autoksig killsig} ++			"Kill"
P{Autoksig hupsig} ++			"Hangup"
P{Autoksig intsig} ++			"Interrupt"
P{Autoksig quitsig} ++			"Quit"
P{Autoksig alarmsig} ++			"Alarm"
P{Autoksig bussig} ++			"Bus erro"
P{Autoksig segvsig} ++			"Segmentation error"
P{xbtq timelim gracetime} ++		"Additional time"
P{xbtq procpar clear gracetime} ++	"Clear"
P{xbtq framelab mailwrt} ++		"Mail/write message options"
P{xbtq mailwrt mail} ++			"Mail completion message"
P{xbtq mailwrt write} ++		"Display completion message"
P{xbtq frametab titp} ++		"Title/pri/ll/ci"
P{xbtq frametab procp} ++		"Process params"
P{xbtq frametab timelim} ++		"Time limits"
P{xbtq frametab mailwrt} ++		"Mail/write"

P{xbtq job mode dlgtit} = ROUND(@, 5)	"Job permissions"
P{xbtq job mode lab} ++			"Job permissions for"
P{xbtq job mode frame lab} ++		"Permissions"

P{xbtq job owner dlgtit} = ROUND(@, 5)	"Job owner/group"
P{xbtq job owner lab} ++		"Owner/group for"
P{xbtq job owner user} ++		"User"
P{xbtq job owner group} ++		"Group"

P{xbtq time dflt dlgtit} = ROUND(@, 5)	"Set time defaults"
P{xbtq deflt style lab} ++		"Repeat style"
P{xbtq deflt units lab} ++		"No of units"

P{xbtq jmac dlg} = ROUND(@, 5)		"Run command on job"
P{xbtq jmac named} ++			"Apply command to job"
P{xbtq jmac noname} ++			"Run command"
P{xbtq jmac cmd} ++			"Command to run"

P{xbtq job conds dlgtit} = ROUND(@, 5)	"Job conditions"
P{xbtq job conds lab} ++		"Job conditions for"

P{xbtq cond deflt dlgtit} = ROUND(@, 5)	"Job conditions default"

P{xbtq job asses dlgtit} = ROUND(@, 5)	"Job assignments"
P{xbtq job asses lab} ++		"Job assignments for"

P{xbtq ass deflt dlgtit} = ROUND(@, 5)	"Job assignments default"

P{xbtq job args dlgtit} = ROUND(@, 5)	"Job arguments"
P{xbtq job args lab} ++			"Job arguments for"
P{xbtq arg add dlgtit} ++		"Add job argument"
P{xbtq add arg} ++			"New argument"
P{xbtq del arg} ++			"Delete argument"
P{xbtq job env dlgtit} = ROUND(@, 5)	"Job environment"
P{xbtq job env lab} ++			"Job environment for"
P{xbtq env add dlgtit} ++		"Add environment variable"
P{xbtq add env} ++			"Add env var"
P{xbtq del env} ++			"Delete env var"
P{xbtq job redirs dlgtit} = ROUND(@, 5)	"Job redirections"
P{xbtq job redirs lab} ++		"Redirections for"
P{xbtq redir add dlgtit} ++		"Add redirection"
P{xbtq add redir} ++			"Add redir"
P{xbtq del redir} ++			"Delete redir"

P{xbtq unqueue dialogtit} = ROUND(@, 5)	"Unqueue job"
P{xbtq unqueue cmd dlg title} ++	"Unqueue shell command"
P{xbtq unqueue job dlg title} ++	"Unqueue job file"
P{xbtq unqueuing job} ++		"Unqueueing job number"
P{xbtq copy no delete} ++		"Copy only, no delete"
P{xbtq unqueue directory} ++		"Directory to copy to"
P{xbtq shell script} ++			"Shell script file"
P{xbtq job file name} ++		"File for job text"

P{xbtq job freezeh dlgtit} = ROUND(@, 5)	"Freeze options in home directory"
P{xbtq job freezec dlgtit} ++		"Freeze options in current directory"
P{xbtq job freeze lab} ++		"Freezing options from job"
P{xbtq freeze verbose setting} ++	"Turn on verbose option in freezed options"
P{xbtq freeze canc setting} ++		"Turn on cancelled state in freezed options"

P{xbtq cmd interps dlgtit} = ROUND(@, 5)	"Edit command interpreter list"
P{xbtq add interp} ++			"Add new interpreter"
P{xbtq del interp} ++			"Delete interpreter"
P{xbtq upd interp} ++			"Update interpreter"
P{xbtq add ci dlgtit} = ROUND(@, 5)	"Add new command interpreter"
P{xbtq add ci name} ++			"Name of new interpreter"
P{xbtq add ci path} ++			"Path name of interpreter executable"
P{xbtq add ci args} ++			"Predefined args (most shells need -s)"
P{xbtq add ci nice} ++			"Ci nice value"
P{xbtq add ci ll} ++			"Ci load level"
P{xbtq add ci set arg0} ++		"Set arg0 to job title"
P{xbtq add ci interparg} ++		"Interpret command args"
P{xbtq edit ci dlgtit} = ROUND(@, 5)	"Edit command interpreter"
P{xbtq edit ci name} ++			"Name of interpreter"
P{xbtq edit ci path} ++			"Path name of interpreter executable"
P{xbtq edit ci args} ++			"Predefined args (most shells need -s)"
P{xbtq edit ci nice} ++			"Ci nice valuse"
P{xbtq edit ci ll} ++			"Ci load level"
P{xbtq edit ci set arg0} ++		"Set arg0 to job title"
P{xbtq edit ci interparg} ++		"Interpret command args"

P{xbtq create var dlgtit} = ROUND(@, 5)	"Create new variable"
P{xbtq createv name lab} ++		"Variable name"
P{xbtq createv comment lab} ++		"Comment on variable"
P{xbtq createv value lab} ++		"Value"
P{xbtq createv istext} ++		"Value is text"
P{xbtq createv export lab} ++		"Export option"
P{xbtq rename var dlgtit} = ROUND(@, 5)	"Rename variable"
P{xbtq rename var title} ++		"Renaming variable"
P{xbtq rename var newname lab} ++	"New name"
P{xbtq export var dlgtit} ++		"Resetting export option for var"
P{xbtq export var title} ++		"Resetting export for"
P{xbtq export var opt lab} ++		"Export option"
P{xbtq assign var dlgtit} = ROUND(@, 5)	"Assign value to variable"
P{xbtq assign var title} ++		"Assigning to"
P{xbtq assign var value lab} ++		"Value"
P{xbtq assign istext} ++		"Value is text"
P{xbtq comment var dlgtit} = ROUND(@, 5)	"Assign variable comment"
P{xbtq comment var title} ++		"Setting comment for"
P{xbtq set comment lab} ++		"Comment"
P{xbtq set const val dlgtit} = ROUND(@, 5)	"Setting constant parameter for arithmetic"
P{xbtq set const lab} ++		"Constant value"

P{xbtq var mode dlgtit} = ROUND(@, 5)	"Variable permissions"
P{xbtq var mode lab} ++			"Variable permissions for"
P{xbtq var mode frame lab} ++		"Permissions"

P{xbtq var owner dlgtit} = ROUND(@, 5)	"Variable owner/group"
P{xbtq var owner lab} ++		"Owner/group for"
P{xbtq var owner user} ++		"User"
P{xbtq var owner group} ++		"Group"

P{xbtq vmac dlg} = ROUND(@, 5)		"Run command on variable"
P{xbtq vmac named} ++			"Apply command to variable"
P{xbtq vmac noname} ++			"Run command"
P{xbtq vmac cmd} ++			"Command to run"

P{xbtq view dlg} = ROUND(@, 5)		"View job script"
P{xbtq view job} ++			"Job viewed"
P{xbtq view no title} ++		"(No title)"
P{xbtq view user} ++			"User"
P{xbtq view group} ++			"Group"

P{xbtq vsearch dlg} = ROUND(@, 5)	"Search for string"
P{xbtq vsearch str} ++			"Search string"
P{xbtq vsearch wrap} ++			"Wrap search"
P{xbtq vsearch backw} ++		"Search backwards"

P{xbtq syserr dlg} = ROUND(@, 5)	"View system error log"

P{xbtq hols dlgtit} = ROUND(@, 5)	"Holidays"

(0)Q{xbtq var export types} = ROUND(@, 20)	"Local only"
(1)Q{xbtq var export types} COPY		"Exported"
(2)Q{xbtq var export types} COPY		"Clustered"

[
XBTR prompts etc
]

P{xbtr app title} = 1950        			"GNUbatch Job Submission"
P{xbtr job queue dlgtit} = 1955		               	"Set job queue prefix"
P{xbtq job queue lab} ++				"Job queue"
P{xbtr jqueue queue lab} ++				"Job queue"
P{xbtr jqueue user grp lab} ++				"User/group"
P{xbtr jqueue verbose} ++				"Verbose messages"
P{xbtr remote host dlgtit} = 1961			"Remote host"
P{xbtr open legacy job dlgtit} = 1964                   "Open legacy job file"
P{xbtr open job dlgtit} = 1965  			"Open job file"
P{xbtr set cmd file dlgtit} ++				"Set command file name"
P{xbtr set job file dlgtit} ++				"Set job file name"
P{xbtr select working dir} ++				"Select working directory"
P{xbtr view opt dlgtit} = 1970  			"Option setting"
P{xbtr job script editor lab} ++			"Job script editor" 
P{xbtr editor in xterm} ++				"Run editor in xterm"
P{xbtr use internal editor} ++                          "Use internal editor"
P{xbtr prefer XML format} ++                            "Prefer XML (single file) format"
P{xbtr job opt dlgtit} ++                               "Job options"
P{xbtr verbose job} ++                                  "Verbose - display job number"
P{xbtr no title} = 1980 				"(no title)"
P{xbtr no file name} ++					"(no name)"
P{xbtr poss job cmd files} ++				"Possible command files"
P{xbtr poss xml job files} ++                           "Possible XML batch job files"
P{xbtr job script edit} ++                              "Edit job script"
P,H{xbtr job list changes} ++
"Unsaved job list changes - OK?",
"There are unsaved changes in the job list.
Are you sure you want to quit before
saving them?"

E,H{xbtr editor name null} = 1990
"No editor name",
"You must give a program to run as editor"

E,H{xbtr job cmd file names same} = 1992
"File names clash",
"The job file has to have a different name from the command file"

E,H{xbtr could not delete} ++
"Could not delete",
"Unable to delete the file %s"

[
Xfilemon messages
]

P{xfilemon app title} = 2000    			"Select files to monitor"
P{xfilemon directory} ++				"Directory"
P{xfilemon choose} ++					"Choose dir"

(0)R,P{xfilemon file action} = 2005     		"File created",
"Select file action"
(1)Q{xfilemon file action} COPY				"File finishes growing"
(2)Q{xfilemon file action} COPY				"File modified"
(3)Q{xfilemon file action} COPY				"File changed"
(4)Q{xfilemon file action} COPY				"File accessed"
(5)Q{xfilemon file action} COPY				"File deleted"

P{xfilemon file timeout} ++	"Timeout to follow changes"

P{xfilemon include} ++		"Include files existing at startup"
P{xfilemon recursive} ++	"Follow subdirectories"
P{xfilemon links} ++		"Follow symbolic links"

(0)R,P{xfilemon file names} ++				"Any file name",
"Pattern type"
(1)Q{xfilemon file names} COPY				"Matching shell-style pattern"
(2)Q{xfilemon file names} COPY				"Matching name exactly"

P{xfilemon continue running} ++				"Continue running after finding files"
P{xfilemon daemon} ++					"Detach daemon process (recommended)"
P{xfilemon poll time} ++				"Poll time (secs)"
P{xfilemon command} ++					"Command/script"
P{xfilemon cscmd} ++					"Command with arguments"
P{xfilemon select dir dlgtit} = ROUND(@, 10)		"Select directory to monitor"

E,H{xfilemon no directory} ++
"No directory",
"You must give a directory to scan."

E,H{xfilemon no script} ++
"No script",
"You must give a script/command to run."

E,H{xfilemon no pattern} ++
"No pattern",
"You must give a file name or pattern to look for"

# System-ish errors

[
System errors - make sure 8000 follows!!!!!
]

E{Check file setup} = 8000
"Cannot open files - check ownership of files in spool directory, and
this program, %P. All files should be owned by `batch' or `root'
and %P should be suid to `batch' or `root'.
In fact the effective uid is %U and the real uid is %R.
Please also check that the software is not being run from a file system
which is mounted with set-uid programs disabled."

E{Cannot change directory} ++
"Cannot change directory to spool directory - %E"

E{Cannot open user control file} ++
"Error accessing user control file - %E
This may be because your copy of GNUbatch is incorrectly
installed. Please check the ownership, especially of
program `%P' and try again.
PLEASE ensure that the ""install"" script is used to
install GNUbatch."

E{Scheduler not running} ++		"%P - GNUbatch scheduler is not running."

E,H{IPC msg q error} ++
"%P: Error ""%E"" transmitting to message queue.
Please investigate your system (run ""ipcs"").",
"There seems to be some problem sending data to the message
queue for GNUbatch. This may be because someone has deleted
the message queue and/or killed the scheduler process or it
may be because your system has a problem."

E,H{IPC msg q full} ++
"%P - message queue is full
This could be because:
  (a) Your scheduler has crashed - please check for ""btsched""
      in a ""ps"" listing
  (b) Some other process unconnected with GNUbatch is sending messages
      exhausting the maximum limit of messages. Please check
      the output of ""ipcs -o"".",
"%P - message queue is full
This could be because:
  (a) Your scheduler has crashed - please check for ""btsched""
      in a ""ps"" listing
  (b) Some other process unconnected with GNUbatch is sending messages
      exhausting the maximum system-wide limit of messages. Please check
      the output of ""ipcs -o""."

E{Btquit giving up} ++				"Too many tries to send message, giving up"
E{Waiting as shutting down} ++			"Waiting as you are trying to shut down...."

E{Cannot create user control file} ++
"Trouble creating user file - please check GNUbatch is installed
correctly."

E{Cannot lock user ctrl file} ++
"Error - %E
Attempting to lock user control file."

E{Cannot unlock user ctrl file} ++
"Error - %E
Attempting to unlock user control file."

E{Wrong version of product} ++
"User control file GNUbatch version number is %d0.
GNUbatch Version is %d1.
Your user control file is either corrupted, or you are attempting
to a mixture of binaries from different versions of GNUbatch.
I am continuing, but please shut down GNUbatch as soon as possible
and investigate. Thanks."

E{Funny times passwd file} ++
"The current time on your machine comes before the time on your
/etc/passwd file. Corrective action has been taken, but I suggest that
you investigate the situation rapidly!"

E{Cannot open jshm} ++
"Panic! Cannot read job queue.
This may be due to incorrect installation.
Please check that the owner of the shared memory segments
are the same as the set-user id of %P which is '%U'."

E{Cannot open vshm} ++
"Panic! Cannot read variable list.
This may be due to incorrect installation.
Please check that the owner of the shared memory segments
are the same as the set-user id of %P which is '%U'."

E{Cannot open semaphore} ++			"Help! Cannot access semaphores - %E"
E{Cannot open ci file} ++			"Cannot open command interpreter file - %E"
E{Cannot read ci file} ++			"Trouble reading command interpreter file"
E,H{Unexpected sched message} ++
"Unexpected scheduler message code %x0",
"I got an unexpected error from the scheduler.
Something is horribly wrong."

E{Netconn failure abort} = 8020			"Aborted %P due to failure to establish network connections"
E{Netconn no service name} = 8023
"Panic! No %t service entry %s in /etc/services
Please note possible source of error - different spellings
Program name is:   %P
Service name is:   %s"

E{Netconn no open TCP} ++			"%P Unable to open TCP port number %d0 for queue port"
E{Netconn no open UDP} ++			"%P Unable to open UDP port number %d0 for user access port"

E{Netconn no API req port} ++
"%P Unable to obtain %t service %s for API request port
(not fatal - but API won't run)"

E{Netconn no API prompt port} ++
"%P Unable to obtain %t service %s for API prompt port
(not fatal - but API won't run)"

E{Semaphore error probably undo} ++
"%P - Problems with locking semaphore. This is almost certainly
due to being unable to allocate sufficient ""undo"" structures
(structures which hold adjustments to semaphore values to apply
on exit) within the kernel. If you have the program ""/etc/sysdef""
available, please run it now and look at the parameter SEMMNU.
This should be at least as great as the parameter SEMMNS and
your kernel parameters should be updated to avoid this error.
The error reported was ""%E"".
However this can vary between platforms."

E{Lock error xfer} ++	"%P - error %E locking xfer buff"
E{Unlock error} ++	"%P - error %E unlocking"
E{Lock error jobs} ++	"%P - error %E locking job seg"
E{Lock error vars} ++	"%P - error %E locking var seg"

[
Mostly panic messages
]

E{Panic couldnt create msg id} = ROUND(@, 100)	"Could not create message id - %E"
E{Panic couldnt reset msg id} ++		"Could not reset message id - %E"
E{Panic couldnt create job file} ++		"Could not create job file - %E"
E{Panic cannot create var file} ++		"Could not create var file - %E"
E{Invalid message type} ++			"Invalid message type code %x9 (%d9)"
E,H{Error on IPC} ++				"Error reading IPC message - %E", "Trouble reading results from msg q"
E{Invalid cvariable in saved job} ++		"Invalid variable %s in condition block on saved job"
E{Invalid variable in saved job} ++		"Invalid variable %s in assign block on saved job"
E{Cannot fork} ++				"Error trying to fork - %E"
E{Cannot pipe} ++				"%P: Cannot create pipe"
E{Panic invalid op message length} ++		"Invalid op message length %d9"
E{Panic invalid job message length} ++		"Invalid job message length %d9"
E{Panic invalid var message length} ++		"Invalid var message length %d9"
E{Panic couldn't create sem id} ++		"Could not create semaphore id - %E"
E{Panic couldn't reset sem id} ++		"Could not reset semaphore id - %E"
E{Panic cannot create CI file} ++		"Cannot create command interpreter file %s - %E"
E{Panic cannot open CI file} ++			"Cannot open command interpreter file %s - %E"
E{Panic cannot read CI file} ++			"Trouble reading command interpreter file %s"
E{Panic trouble creating shared mem} = 8119
"Trouble creating shared memory id %x9 - %E
To fix this try running ""ripc -d"" or removing the segment
with key %x9 with ipcrm.
More recent versions of GNUBatch use memory-mapped files and
if the error is ""File Exists"" you should try removing files
with names starting ""btmm_"" from the batch spool directory."
E{Panic trouble resetting shared mem} ++	"Trouble resetting shared memory - %E"
E{Panic trouble with job shm} ++		"Trouble attaching shared memory for jobs - %E"
E{Panic trouble with var shm} ++		"Trouble attaching shared memory for vars - %E"
E{Panic clash with system variables} ++		"Clash of system variables involving %s"

E{Panic trouble hashing system var} ++
"Trouble entering system variable %s
into hash table."

E{Panic job progress code} ++			"Job progress code %d0 undefined"
E{Panic trouble with log in config file} ++	"Trouble with log codes in this file."

E{Panic kill trouble} ++
"Trouble with kill permissions - %E
Is GNUbatch installed correctly?
Or are you debugging and not running as root??"

E{Panic queue clogged up} ++			"Message queue appears to be full up - giving up."
E{Panic cannot create network semaphore} ++	"Could not create network semaphore id - %E"
E{Panic cannot reset network semaphore} ++	"Could not reset network semaphore id - %E"
E{Panic hash deletion bug} ++			"Error in hash deletion (probably bug!)"

E{Panic lost count of vars} ++
"Probable bug in var hash table.
Lost count of variables %d9 in hash table %d8 in header."

E{Panic cannot create listen socket} ++		"Trouble creating listening socket port number %d0 - %E"
E{Panic trouble creating feeder socket} ++	"Trouble creating feeder socket port number %d0 - %E"
E{Panic poll/select error in netmonitor process} ++		"Poll/select error in netmonitor process - %E"
E{Panic trouble creating probe uda port} ++	"Trouble creating probe uda port number %d0 - %E"
E{Panic trouble creating probe socket} ++	"Trouble creating probe socket port number %d0 - %E"
E{Panic trouble binding probe socket} ++	"Trouble binding probe socket port number %d0 - %E"
E{Panic trouble sending on probe socket} ++	"Trouble sending on probe socket port number %d0 - %E"

E,H{Warn errors in host file} ++
"%P: Warning: There are errors in the system hosts file",
"There are errors in the host description file given in
the system hosts file. This is not fatal but it may give problems."

E{Reconnection whilst still connected} ++
"Warning: host %s - reconnection of unclosed connection"

E{Hash function error free_probe} = 8144
"Panic: hash failure error free_probe hid %s"

E{Invalid progress code try} ++
"Help job progress code still running try again (%d1)"

E{Invalid progress code give up} ++
"Panic job progress code still running at %d1 tries"

E{Panic couldnt lock job file} ++
"Panic could not lock saved job file
Error was %E"

E{Panic couldnt lock var file} ++
"Panic could not lock saved var file
Error was %E"

[
Errors whilst doing exec
]

E{Exec cannot open redirection file} = ROUND(@, 100)	"Cannot open file %s - %E"
E{Exec cannot change directory} ++		"Cannot change directory to %s"
E{Exec cannot dup} ++				"Trouble with dup of %d0 - %E"
E{Exec cannot pipe} ++				"Trouble with pipe for %s - %E"
E{Exec cannot fork} ++				"Trouble with fork for %s - %E"

[
Probe errors
]

E{Probe incorrect IP 1} ++		"Probe invoked with incorrect IP %s"
E{Probe incorrect IP 2} ++		"Called from IP %s"

[
btwrite messages
]

E{btwrite message from user} = ROUND(@, 100)	"Message from %U....."
E{btwrite message diverted} ++			"Subject: Message diverted to mail"

[
Error messages whilst saving .gnubatch files
]

E,H{saveopts crashed} = ROUND(@, 100)
"Sorry saveopts crashed with signal 0x%x0",
"Save options subprogram crashed with the signal 0x%x0"

{saveopts file error} = @ - 9
S{saveopts nodel} = 10
S{saveopts no init and del} = 11
S{saveopts cannot create} = 12
S{saveopts nomem} = 15

E,H{saveopts file error} + {saveopts nodel}
"Sorry cannot delete old init file from %s",
"I am trying to create a new .gnubatch file to hold
a reference to %s and I cannot delete the old one, sorry."

E,H{saveopts file error} + {saveopts no init and del}
"Sorry cannot create init file in %s
(and I've deleted your old one - sorry!!)",
"I am trying to create a new .gnubatch file to hold
a reference to %s and I cannot do so, sorry, because
I think I deleted your old one."

E,H{saveopts file error} + {saveopts cannot create}
"Sorry cannot create init file in %s",
"I cannot write to the directory to create the file "

E,H{saveopts cannot fork} = {saveopts crashed} + 4
"Sorry cannot fork to create init file",
"I need to fork to create the file."

E,H{saveopts file error} + {saveopts nomem}
"No memory available creating init file.",
"Ran out of memory creating .gnubatch file"

[
Version of messages for use within X programs
]
E,H{saveopts no write} = 8440
"No write permission saving options",
"Unable to save options to file, no write permission in directory
or to existing file"

E,H{saveopts bad setup} ++
"Setup error for saveopts",
"There is an error in the system setup relating to saveopts.
Please check your installation"

E,H{saveopts usage} ++
"Usage error for saveopts",
"The number arguments to the saveopts program are not correct"

E,H{saveopts no memory} ++
"No memory running saveopts",
"Unable to allocate memory during save options."

E,H{saveopts no exec} ++
"Saveopts bad exec",
"Unable to execute save options program"

E,H{saveopts unknown exit} ++
"Saveopts unknown exit %d0",
"The saveopts subprogram gave exit code %d0
which is not known"

[
No permissions
]

E{No read admin file priv} = ROUND(@, 100)
"You (%R) do not have permission to read the administration file
so you cannot be permitted to continue with this command. Sorry."

E,H{No write admin file priv} ++
"You (%R) do not have permission to write the administration file
only to read it.",
"You must arrange for someone to provide you with WRITE admin file
permission to do this operation"

E{No set default modes} ++
"You (%R) do not have permission to reset your default modes
only to read them."

[
Key setup errors
]

E{Global key error} = 9000
"Global key setup error - clash on character %c0 with previously-given
value %d2 and new value %d1.
This means that you have defined one key to do two or more different things,
possibly because you have specified one as a terminal key (\KKILL etc).
Please look in the file %F for lines
of the form ""K%d2:...."" and ""K%d1:.....""
You may be using a different terminal or account than before?"

E{State key error} ++
"State %d3 key setup error - clash on character %c0 with previously-given
value %d2 and new value %d1.
This means that you have defined one key to do two or more different things,
possibly because you have specified one as a terminal key (\KKILL etc).
Please look in the file %F for lines
of the form ""%d3K%d2:...."" or ""K%d2:..."" and ""%d3K%d1:.....""
You may be using a different terminal or account than before?"

E{Key sequence error} ++
"Key setup error on character number %d3 - %c0 with previously-given
value %d2 and new value %d1."

E{Key string too long} ++
"Key string too long... Aborting."

E{Key string bad syntax} ++
"Bad syntax global key definitions... Aborting."

E{Unknown key name} ++
"Unknown key name `%s' - ignored.
This is harmless, but is because you have called for something not defined in
your ""termcap"" or ""terminfo"" file - probably a ""HOME"" key.
Please look in the file %F for lines that might refer to %s
You may be using a different terminal or account than before?"

E{State key string bad syntax} ++
"Bad syntax state key definitions... Aborting."

E{Undef key seq} ++
"Undefined key sequence - ignored"

E{Key setup errors aborting} ++
"Aborted due to %d0 key setup errors."

E{getkey term input error} ++
"Error on terminal input - %E"

E,H{program arg error} = ROUND(@, 500)
"Bad argument %s to program %P",
"Saved job included bad argument %s"

E,H{program arg bad string} ++
"Bad string argument %s to program %P",
"Saved job included bad string argument %s"

E,H{program opt expects arg} ++
"Option %s to program %P expects an argument",
"Saved job included an option %s
which normally expects an argument"

[
Missing bits
]

E{Missing help code} = 10000		"Missing help message code %d9"
E{Missing prompt code} ++		"Missing prompt message code %d9"
E,H{Missing error code} ++
"Missing error message code %d9",
"The help message file %F is missing an error
code %d9. Please record a bug!" 
E,H{Missing alternative code} ++	"Missing alternative code for %d9", "Missing alternative code for %d9"
E{Missing state code} ++		"Missing state code %d9"
E{Scrambled state code} ++		"Jumbled state code for %d9"
E{Wrong number alternatives} ++		"Wrong alternatives state %d9, expected %d8 read %d7"

E{argument option error} ++
"Clash on character %c2 with state %d0 and state %d1
(i.e. you have 2 meanings for -%c2)"
E{argument keyword error} ++
"Clash on option %s with state %d0 and state %d1
(i.e. you have 2 meanings for +%s)"
E{arg conflict abort} ++
"Aborted due to %d0 conflict(s) in argument descriptions"
E{NO MEMORY} ++				"%P:Ran out of memory in %s line %d8"

[
Bounds exceeded - think about parameters
]

E,H{Too large environment} = 10100
"Too many environment variables - truncated.
%d0 found maximum allowed for is %d1.
If this is a problem PLEASE let us know as it is easy to change.",
"This requires a minor programming change to take into account the
NUMBER of environment variables rather than the space which they
take."

E,H{Too many job strings} ++
"Too many strings in job. This is almost certainly because you
have a very large environment table (please run the command
""env"" or ""printenv"" from the command line to see) and there
is too much data there to store in a single job structure.

There are two possible workarounds or fixes for this:

(a)  Please delete some of the environment variables you
     are using before trying again. Variables such as
     TERMCAP which only relate to your terminal session
     are good candidates.

(b)  Set up the ""static environment file""
     to contain common environment
     variables for all jobs. Each job will then
     contain only those variables which differ
     from the static environment file.

We could expand the size of a job structure, but this would
reduce the number of jobs for a given amount of shared memory.",
"There is a limited amount of space for strings (environment,
title, directory, arguments redirections) in a job which
has been exceeded for the current job."

E,H{Too many arguments} ++
"Too many arguments.
If this is a problem PLEASE let us know as it is easy to change.",
"This requires a minor programming change to take into account the
NUMBER of arguments rather than the space which they take."

E,H{Too many redirections} ++
"Too many redirections.
If this is a problem PLEASE let us know as it is easy to change.",
"This requires a minor programming change to take into account the
NUMBER of redirections rather than the space which they take."

E,H{Too many job strings defaults} ++
"The default job loaded has too many strings in.",
"You have just attempted to load a set of defaults with an
extra-large number of strings in it."

[
Messages regarding scheduler crashing
]

E{Scheduler started} = 12000			"%P started."
E,S{Scheduler normal exit} ++			"%P stopped normally."
E{Scheduler core dump} ++			"%P stopped due to program fault (coredump)"
E{Scheduler net core dump} ++			"%P stopped due to program fault (coredump) in net monitor"
E{Scheduler exec core dump} ++			"%P stopped due to program fault (coredump) in exec process"
E{Scheduler killed} ++				"%P stopped - killed"

######################################################################
##  Internal-ish messages

S{No title offset} = 10
S{Host name offset} = 20
{Job completed msg} = 50
{Job error halted msg} = 51
{Job aborted msg} = 52
{Job cancelled msg} = 53
{Job autodel msg} = 54
{Job exceeded limit} = 55
{Job final limit} = 56
{Job manually aborted} = 57

[
Job completion messages with title, current host
]

P,E{Job completed msg}
"completed",
"Subject: GNUbatch job '%t', (Id: %d0) completed

Your batch job, '%t', (Job Number %d0),
was completed satisfactorily."

P,E{Job error halted msg}
"error halted",
"Subject: GNUbatch job '%t', (Id: %d0) error halted

Your batch job, job '%t', (Job Number %d0),
completed with an error exit code of %d2."

P,E{Job aborted msg}
"aborted",
"Subject: GNUbatch job '%t', (Id: %d0) aborted

Your batch job, job '%t', (Job Number %d0),
was aborted due to a signal. Signal number was %d1."

P,E{Job cancelled msg}
"cancelled",
"Subject: GNUbatch job '%t', (Id: %d0) cancelled

Your batch job, job '%t', (Job Number %d0), was cancelled,
i.e. it was sent a signal before it could be run."

P,E{Job autodel msg}
"automatically deleted",
"Subject: GNUbatch job '%t', (Id: %d0) automatically deleted

Your batch job, job '%t', (Job Number %d0), was automatically deleted.
This is due to the retention time (set by -t option) being
exceeded."

P,E{Job exceeded limit}
"exceeded runtime limit",
"Subject: GNUbatch job '%t', (Id: %d0) exceeded runtime limit

Your batch job, job '%t', (Job Number %d0), exceeded the set runtime
limit. An initial signal was sent."

P,E{Job final limit}
"exceeded final runtime limit",
"Subject: GNUbatch job '%t', (Id: %d0) exceeded final runtime limit

Your batch job, job '%t', (Job Number %d0), exceeded the set final runtime
limit. A final ""kill""  signal was sent."

P,E{Job manually aborted}
"Manually aborted",
"Subject: GNUbatch job '%t', (Id: %d0) aborted.

Your batch job, job '%t', (Job number %d0), was manually aborted."

[
Job completion messages without title, current host
]

E{Job completed msg} - {No title offset}
"Subject: GNUbatch job number %d0 completed

Your batch job, job number %d0, completed satisfactorily"

E{Job error halted msg} - {No title offset}
"Subject: GNUbatch job number %d0 error halted

Your batch job, job number %d0, completed with an error exit
code of %d2."

E{Job aborted msg} - {No title offset}
"Subject: GNUbatch job number %d0 aborted

Your batch job, job number %d0, was aborted due to signal.
Signal number was %d1."

E{Job cancelled msg} - {No title offset}
"Subject: GNUbatch job number %d0 cancelled

Your batch job, job number %d0, was cancelled,
i.e. it was sent a signal before it could be run."

E{Job autodel msg} - {No title offset}
"Subject: GNUbatch job number %d0 automatically deleted

Your batch job, job number %d0, was automatically deleted.
This is due to the retention time (set by -t option) being
exceeded."

E{Job exceeded limit} - {No title offset}
"Subject: GNUbatch job number %d0 exceeded runtime limit

Your batch job, job number %d0, exceeded the set runtime
limit. An initial signal was sent."

E{Job final limit} - {No title offset}
"Subject: GNUbatch job number %d0 exceeded final runtime limit

Your batch job, job number %d0, exceeded the set final runtime
limit. A final ""kill""  signal was sent."

E{Job manually aborted} - {No title offset}
"Subject: GNUbatch job number %d0 aborted.

Your batch job, job number %d0, was manually aborted."

[
Job completion messages with title, other host
]

E{Job completed msg} + {Host name offset}
"Subject: GNUbatch job '%t', (Id: %d0) completed

Your batch job, '%t', (Job Number %d0), executed on host %s
was completed satisfactorily."

E{Job error halted msg} + {Host name offset}
"Subject: GNUbatch job '%t', (Id: %d0) error halted

Your batch job, job '%t', (Job Number %d0), executed on host %s
completed with an error exit code of %d2."

E{Job aborted msg} + {Host name offset}
"Subject: GNUbatch job '%t', (Id: %d0) aborted

Your batch job, job '%t', (Job Number %d0), executed on host %s
was aborted due to a signal. Signal number was %d1."

E{Job exceeded limit} + {Host name offset}
"Subject: GNUbatch job '%t', (Id: %d0) exceeded runtime limit

Your batch job, job '%t', (Job Number %d0), executed on host %s
exceeded the set runtime limit. An initial signal was sent."

E{Job final limit} + {Host name offset}
"Subject: GNUbatch job '%t', (Id: %d0) exceeded final runtime limit

Your batch job, job '%t', (Job Number %d0), executed on host %s
exceeded the set final runtime limit. A final ""kill""  signal was sent."

E{Job manually aborted} + {Host name offset}
"Subject: GNUbatch job '%t', (Id: %d0) aborted.

Your batch job, job '%t', (Job number %d0), executing on host %s
was manually aborted."

[
Job completion messages without title, other host
]

E{Job completed msg} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 completed

Your batch job, job number %d0, executed on host %s,
completed satisfactorily."

E{Job error halted msg} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 error halted

Your batch job, job number %d0, executed on host %s,
completed with an error exit code of %d2."

E{Job aborted msg} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 aborted

Your batch job, job number %d0, executed on host %s,
was aborted due to signal. Signal number was %d1."

E{Job cancelled msg} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 cancelled

Your batch job, job number %d0, was cancelled,
i.e. it was sent a signal before it could be run."

E{Job exceeded limit} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 exceeded runtime limit

Your batch job, job number %d0, executed on host %s,
exceeded the set runtime limit. An initial signal was sent."

E{Job final limit} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 exceeded final runtime limit

Your batch job, job number %d0, executed on host %s,
exceeded the set final runtime limit.
A final ""kill""  signal was sent."

E{Job manually aborted} - {No title offset} + {Host name offset}
"Subject: GNUbatch job number %d0 aborted.

Your batch job, job number %d0, executing on host %s,
was manually aborted."

[
Standard output/error messages
]

E{Job stdout was} = 80
"Standard output from your job was:
---------------------------------"

E{Job stderr was} ++
"Standard error from your job was:
--------------------------------"

S{log code create} =	0
S{log code delete} =	1
S{log code assign} =	2
S{log code chown} =	3
S{log code chgrp} =	4
S{log code chmod} =	5
S{log code rename} =	6
S{log code chcomment} =	7
S{log code chdetails} =	8
S{log code started} =	9
S{log code completed} =	10
S{log code error} =	11
S{log code abort} =	12
S{log code cancel} =	13
S{log code force} =	14
S{log code forcena} =	15
S{log code netabort} =	16
S{log code chflags} =	17
S{log code dstadj} =	18
S{log code autodel} =	19
S{log code runkill} =	20
S{log code runonkill} =	21
S{log code kill} =	22

[
Types of events for jobs and variables
]

({log code create})	Q	{Log event type} = 300		"create"
({log code delete})	Q	{Log event type}		"delete"
({log code assign})	Q	{Log event type}		"assign"
({log code chown})	Q	{Log event type}		"chown"
({log code chgrp})	Q	{Log event type}		"chgrp"
({log code chmod})	Q	{Log event type}		"chmod"
({log code rename})	Q	{Log event type}		"rename"
({log code chcomment})	Q	{Log event type}		"chcomment"
({log code chdetails})	Q	{Log event type}		"jdetails"
({log code started})	Q	{Log event type}		"started"
({log code completed})	Q	{Log event type}		"completed"
({log code error})	Q	{Log event type}		"error"
({log code abort})	Q	{Log event type}		"abort"
({log code cancel})	Q	{Log event type}		"cancel"
({log code force})	Q	{Log event type}		"force-start"
({log code forcena})	Q	{Log event type}		"force-run"
({log code netabort})	Q	{Log event type}		"network-aborted"
({log code chflags})	Q	{Log event type}		"change export flag"
({log code dstadj})	Q	{Log event type}		"dst adjust"
({log code autodel})	Q	{Log event type}		"auto delete"
({log code runkill})	Q	{Log event type}		"exceeded runtime"
({log code runonkill})	Q	{Log event type}		"exceeded grace period"
({log code kill})	Q	{Log event type}		"manual kill"

S{log source manual}	=	0
S{log source jstart}	=	1
S{log source jcompleted}=	2
S{log source jerror}	=	3
S{log source jabort}	=	4
S{log source jcancel}	=	5

({log source manual})	Q	{Log source type} = {Log event type} + 1	"manual"
({log source jstart})	Q	{Log source type} 		"Job start"
({log source jcompleted})Q	{Log source type} 		"Job completed"
({log source jerror})	Q	{Log source type} 		"Job error"
({log source jabort})	Q	{Log source type}		"Job abort"
({log source jcancel})	Q	{Log source type}		"Job cancel"

P{Unnamed job source} ++	"<Unnamed job>"
P{Cluster var source} ++	"Cluster"
P{Exported job source} ++	"Exported"
P{Local only source} ++		"Local only"
