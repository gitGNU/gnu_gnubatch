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

include(xres.m4)
include(stdcolours.m4)
XRES_APPLICATION(gbch-xmr, {GBCH-XMR - Submit GNUbatch Jobs}, 32)

TOPLEVEL_RESOURCE(toolTipEnable, True)
TOPLEVEL_RESOURCE(toolbarPresent, True)
TOPLEVEL_RESOURCE(jtitlePresent, True)
TOPLEVEL_RESOURCE(footerPresent, True)
XRES_SPACE

XRES_WIDGETOFFSETS(10,10,5,5)
XRES_SPACE
XRES_FOOTER(footer)
XRES_SPACE
XRES_PANED(layout)
XRES_SPACE

XRES_TITLE(jtitle, {Open jobs for submission})
XRES_LIST(jlist, 12)
XRES_SPACE

TAB_SET(40)
XRES_MENU(menubar)
XRES_SPACE

XRES_MENUHDR(Options, Options, O)
XRES_MENUHDR(Defaults, Defaults, D)
XRES_MENUHDR(File, File, F)
XRES_MENUHDR(Job, Jobs, J)
XRES_MENUHDR(Help, Help, H)
XRES_SPACE

XRES_MENUITEM(Viewopts, View options, V, equal, =)
XRES_MENUITEM(Saveopts, Save options, S)
XRES_MENUITEM(Selectdir, Select new directory, d)
XRES_MENUITEM(Loaddefsc, Load defaults current dir, L)
XRES_MENUITEM(Loaddefsh, Load defaults home dir, h)
XRES_MENUITEM(Savedefsc, Save defaults current dir, S)
XRES_MENUITEM(Savedefsh, Save defaults home dir, v)
XRES_MENUITEM(Quit, Quit, Q, C-c)
XRES_SPACE

XRES_MENUITEM(Defhost, Default remote host, h)
XRES_MENUITEM(Queued, Default queue name, q)
XRES_MENUITEM(Setrund, Default set runnable, r)
XRES_MENUITEM(Setcancd, Default set cancelled, c)
XRES_MENUITEM(Timed, Default time, t)
XRES_MENUITEM(Titled, Default title/prio/ll, l)
XRES_MENUITEM(Processd, Default Process Params, P)
XRES_MENUITEM(Runtimed, Default Time limits, T)
XRES_MENUITEM(Maild, Default Mail/write, M)
XRES_MENUITEM(Permjd, Default permissions, p)
XRES_MENUITEM(Argsd, Default Arguments, A)
XRES_MENUITEM(Envd, Default Environment, E)
XRES_MENUITEM(Redirsd, Default Redirections, R)
XRES_MENUITEM(Condsd, Default Conditions, C)
XRES_MENUITEM(Assesd, Default assignments, a)
XRES_SPACE

XRES_MENUITEM(New, New job file, N, C-n)
XRES_MENUITEM(Open, Open job file, O, C-o)
XRES_MENUITEM(Close, Close job file, C)
XRES_MENUITEM(Jobfile, Set job file name, j, j)
XRES_MENUITEM(Cmdfile, Set command file name, c, c)
XRES_MENUITEM(Save, Save job, S, C-s)
XRES_MENUITEM(Edit, Edit job file, e, e)
XRES_MENUITEM(Delete, Delete job file, D)
XRES_MENUITEM(Submit, Submit job, S, S-1, !)
XRES_MENUITEM(Rsubmit, Submit job remotely, r, at, @)
XRES_SPACE

XRES_MENUITEM(Queue, Job queue name, q)
XRES_MENUITEM(Setrun, Set job runnable, r, r)
XRES_MENUITEM(Setcanc, Set job cancelled, c, z)
XRES_MENUITEM(Time, Set job time parameters, t, t)
XRES_MENUITEM(Title, {Title, pri, Command int, loadlev}, p, p)
XRES_MENUITEM(Process, Process parameters, P, u)
XRES_MENUITEM(Runtime, Job Time Limits, L)
XRES_MENUITEM(Mail, Mail and write markers, M, F)
XRES_MENUITEM(Permj, Job permissions, p)
XRES_MENUITEM(Args, Job Arguments, A, G)
XRES_MENUITEM(Env, Job Environment, E, E)
XRES_MENUITEM(Redirs, Job I/O Redirections, R, R)
XRES_MENUITEM(Conds, Set job conditions, c)
XRES_MENUITEM(Asses, Set job assignments, a)
XRES_SPACE

XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUITEM(About, About, A)

XRES_TOOLBAR(toolbar, 50)
XRES_TOOLBARITEM(Open, Open job)
XRES_TOOLBARITEM(Edit, Edit job)
XRES_TOOLBARITEM(Srun, Set ready)
XRES_TOOLBARITEM(Scanc, Set cancel)
XRES_TOOLBARITEM(Time, Set time)
XRES_TOOLBARITEM(Cond, Condits)
XRES_TOOLBARITEM(Ass, Ass'mnts)
XRES_TOOLBARITEM(Submit, Submit job)

XRES_COMMENT(Dialog titles etc for help/error/info/confirm)
XRES_STDDIALOG(help, {On line help.....})
XRES_STDDIALOG(error, {Whoops!!!})
XRES_STDDIALOG(Confirm, {Are you sure?})

XRES_COMMENT(These are used in various places)
XRES_GENERALLABEL(jobnotitle, Editing job number:)
XRES_GENERALLABEL(stime, Job has time set)
XRES_GENERALLABEL(Del, Run once and delete)
XRES_GENERALLABEL(Ret, Run once and retain)
XRES_GENERALLABEL(Mins, Repeat in minutes)
XRES_GENERALLABEL(Hours, Repeat in hours)
XRES_GENERALLABEL(Days, Repeat in days)
XRES_GENERALLABEL(Weeks, Repeat in weeks)
XRES_GENERALLABEL(Monb, Months relative to beginning)
XRES_GENERALLABEL(Mone, Months relative to end)
XRES_GENERALLABEL(Years, Repeat in years)
XRES_GENERALLABEL(Revery, Repeat Interval)
XRES_GENERALLABEL(Mthday, Target month day)
XRES_SPACE

XRES_GENERALLABEL(Sun, Avoiding Sundays)
XRES_GENERALLABEL(Mon, Mondays)
XRES_GENERALLABEL(Tue, Tuesdays)
XRES_GENERALLABEL(Wed, Wednesdays)
XRES_GENERALLABEL(Thu, Thursdays)
XRES_GENERALLABEL(Fri, Fridays)
XRES_GENERALLABEL(Sat, Saturdays)
XRES_GENERALLABEL(Hday, Holidays)
XRES_GENERALLABEL(comesto, {Next repetition would come to.....})
XRES_GENERALLABEL(Skip, Skip repeat if not possible)
XRES_GENERALLABEL(Delay, Delay next repeat if not possible)
XRES_GENERALLABEL(Delall, Delay all if not possible)
XRES_GENERALLABEL(Catchup, Catch up delayed jobs)
XRES_SPACE

XRES_DIALOG(Viewopts, Display options)
XRES_DLGLABEL(viewtitle, Selecting view options)
XRES_DLGLABEL(editor, Editor to modify job scripts)
XRES_DLGLABEL(xtermedit, Run editor in xterm)
XRES_DLGLABEL(Ok, Apply)
XRES_SPACE

XRES_DIALOG(seldir, Select a new current directory)
XRES_SPACE

XRES_DIALOG(Queuew, Set queue name and user)
XRES_DLGLABEL(jobnotitle, Editing job:)
XRES_DLGLABEL(queuename, Queue name)
XRES_DLGLABEL(qselect, Select a queue)
XRES_DLGLABEL(username, {Set user to })
XRES_DLGLABEL(groupname,{Set group to})
XRES_DLGLABEL(uselect, {Choose a user....})
XRES_DLGLABEL(gselect, {Choose a group...})
XRES_DLGLABEL(verbose, {Verbose (display job number on submit)})
XRES_SPACE

XRES_SELDIALOG(qselect, Select a queue, {Possible queues....}, {Set to....})
XRES_SELDIALOG(uselect, Select a user, {Possible users....}, {Set to....})
XRES_SELDIALOG(gselect, Select a group, {Possible groups....}, {Set to....})
XRES_SPACE

XRES_DIALOG(Stime, Set times for job)
XRES_DIALOG(deftime, Set time defaults for any new time)
XRES_SPACE

XRES_DIALOG(Titpri, {Set title, priority, command interpreter, load level})
XRES_DLGLABEL(title, Job title)
XRES_DLGLABEL(priority, Job priority)
XRES_DLGLABEL(interp, Command interpreter)
XRES_DLGLABEL(iselect, Choose a command interpreter)
XRES_DLGLABEL(loadlevel, Load level)
XRES_SPACE

XRES_DIALOG(Procp, Set process parameters)
XRES_DLGLABEL(dselect, Select a directory)
XRES_DLGLABEL(umask, {Umask - turn OFF permissions for....})
XRES_DLGLABEL(ur, User read)
XRES_DLGLABEL(uw, User write)
XRES_DLGLABEL(ux, User execute)
XRES_DLGLABEL(gr, Group read)
XRES_DLGLABEL(gw, Group write)
XRES_DLGLABEL(gx, Group execute)
XRES_DLGLABEL(or, Others read)
XRES_DLGLABEL(ow, Others write)
XRES_DLGLABEL(ox, Others execute)
XRES_DLGLABEL(Ulimit, {Ulimit (max file size)})
XRES_DLGLABEL(Normal, {Normal exit: codes})
XRES_DLGLABEL(Error, {Error exit: codes })
XRES_DLGLABEL(Advt, {On error, advance time anyway})
XRES_DLGLABEL(Noadvt, Do not advance time on error)
XRES_DLGLABEL(local, Job is purely local to current host)
XRES_DLGLABEL(export, {Job is visible, but not runnable elsewhere})
XRES_DLGLABEL(remrun, Job is runnable remotely)

XRES_COMMENT(Time limits)
XRES_DIALOG(Runtime, Set job time limits)
XRES_DLGLABEL(deletetime, Delete job after (hours))
XRES_DLGLABEL(zerod, Cancel delete time)
XRES_DLGLABEL(runtime, Run time limit (hh:mm:ss))
XRES_DLGLABEL(zeror, Cancel run time limit)
XRES_DLGLABEL(killwith, {Use signal to kill with...})
XRES_DLGLABEL(Termsig, Terminate signal)
XRES_DLGLABEL(Killsig, KILL signal)
XRES_DLGLABEL(Hupsig, Hangup signal)
XRES_DLGLABEL(Intsig, Interrupt signal)
XRES_DLGLABEL(Quitsig, Quit signal)
XRES_DLGLABEL(Alarmsig, Alarm signal)
XRES_DLGLABEL(Bussig, Bus error signal)
XRES_DLGLABEL(Segvsig, Segment violation signal)
XRES_DLGLABEL(grace, Clean up time before KILL signal)
XRES_DLGLABEL(zerog, Cancel clean up time)

XRES_COMMENT(Mail/write)
XRES_DIALOG(Mailw, Set mail and write flags)
XRES_DLGLABEL(mail, Mail user on completion)
XRES_DLGLABEL(write, Write to user on completion)

XRES_COMMENT(Job permissions)
XRES_DIALOG(jmode, Job permissions)
XRES_DLGLABEL(username,  {Set user to })
XRES_DLGLABEL(groupname, {Set group to})
XRES_DLGLABEL(uselect, {Choose a user....})
XRES_DLGLABEL(gselect, {Choose a group...})
XRES_DLGLABEL(juread, Read user)
XRES_DLGLABEL(jgread, By group)
XRES_DLGLABEL(joread, By others)
XRES_DLGLABEL(juwrite, Write user)
XRES_DLGLABEL(jgwrite, By group)
XRES_DLGLABEL(jowrite, By others)
XRES_DLGLABEL(jushow, Reveal)
XRES_DLGLABEL(jgshow, By group)
XRES_DLGLABEL(joshow, By others)
XRES_DLGLABEL(jurmode, Read mode)
XRES_DLGLABEL(jgrmode, By group)
XRES_DLGLABEL(jormode, By others)
XRES_DLGLABEL(juwmode, Write mode)
XRES_DLGLABEL(jgwmode, By group)
XRES_DLGLABEL(jowmode, By others)
XRES_DLGLABEL(juutake, Assume owner)
XRES_DLGLABEL(jgutake, By group)
XRES_DLGLABEL(joutake, By others)
XRES_DLGLABEL(jugtake, Ass grp owner)
XRES_DLGLABEL(jggtake, By group)
XRES_DLGLABEL(jogtake, By others)
XRES_DLGLABEL(juugive, Assign user)
XRES_DLGLABEL(jgugive, By group)
XRES_DLGLABEL(jougive, By others)
XRES_DLGLABEL(juggive, Assign group)
XRES_DLGLABEL(jgggive, By group)
XRES_DLGLABEL(joggive, By others)
XRES_DLGLABEL(judel, Delete)
XRES_DLGLABEL(jgdel, By group)
XRES_DLGLABEL(jodel, By others)
XRES_DLGLABEL(jukill, Kill)
XRES_DLGLABEL(jgkill, By group)
XRES_DLGLABEL(jokill, By others)

XRES_COMMENT(Arguments)
XRES_DIALOG(Jargs, Command arguments for job)
XRES_LIST(Jarglist, 10)
XRES_DIALOG(argedit, Edit argument value)
XRES_DLGLABEL(value, Value:)

XRES_COMMENT(Environment)
XRES_DIALOG(Jenvs, Environment variables for job)
XRES_LIST(Jenvlist, 15)
XRES_DIALOG(envedit, Edit environment variable)
XRES_DLGLABEL(value, Value:)

XRES_COMMENT(Redirections)
XRES_DIALOG(Jredirs, Redirections list for job)
XRES_LIST(Jredirlist, 8)
XRES_DIALOG(rediredit, Edit redirection)
XRES_DLGLABEL(fileno, File descriptor)
XRES_DLGLABEL(file, File name)
XRES_DLGLABEL(dupfd, Duplicated file descriptor)
XRES_SPACE

XRES_DIALOG(Jcond, Condition variables for job)
XRES_SELDIALOG(vselect, Select a variable)
XRES_DIALOG(condedit, Set up conditions)
XRES_DLGLABEL(Variable, Variable to use)
XRES_DLGLABEL(vselect, {Choose a variable....})
XRES_DLGLABEL(ncrit, {If host unavailable, ignore})
XRES_DLGLABEL(crit, {If host unavailable, hold})
XRES_DLGLABEL(value, Value to compare against)
XRES_SPACE

XRES_DIALOG(Jass, Assignments for job)
TAB_SET(48)
XRES_DIALOG(assedit, Set up assignments)
XRES_DLGLABEL(Variable, Variable to use)
XRES_DLGLABEL(vselect, {Choose a variable....})
XRES_DLGLABEL(ncrit, {If host unavailable, ignore})
XRES_DLGLABEL(crit, {If host unavailable, hold})
XRES_DLGLABEL(Start, On start)
XRES_DLGLABEL(Reverse, Reverse on exit)
XRES_DLGLABEL(Normal, On normal exit)
XRES_DLGLABEL(Error, On error exit)
XRES_DLGLABEL(Abort, On abort)
XRES_DLGLABEL(Cancel, On cancel)
XRES_DLGLABEL(value, Value to assign)
XRES_SPACE

TAB_SET(32)
XRES_SELDIALOG(openj, Select job file to open)
XRES_SELDIALOG(jobfile, Select job file title)
XRES_SELDIALOG(cmdfile, Select command file title)
XRES_SELDIALOG(defhost, Select default host)
XRES_SELDIALOG(whichhost, Select host for submission, Possible hosts, Set to)
XRES_SPACE
XRES_DIALOG(about, {About xmbtr.....})
XRES_END
