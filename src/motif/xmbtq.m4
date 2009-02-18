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
XRES_APPLICATION(gbch-xmq, {GBCH-XMQ - GNUbatch Queue viewer}, 32)

TOPLEVEL_RESOURCE(toolTipEnable, True)
TOPLEVEL_RESOURCE(toolbarPresent, True)
TOPLEVEL_RESOURCE(jtitlePresent, True)
TOPLEVEL_RESOURCE(vtitlePresent, True)
TOPLEVEL_RESOURCE(footerPresent, True)
XRES_SPACE

XRES_WIDGETOFFSETS(10,10,5,5)
XRES_SPACE
XRES_FOOTER(footer)
XRES_SPACE
XRES_PANED(layout)
XRES_SPACE

XRES_TITLE(jtitle)
XRES_LIST(jlist, 12)

XRES_POPUPMENU(do-jobpop, jobpopup)
XRES_MENUITEM(View, View job script)
XRES_MENUITEM(Time, Set job times)
XRES_MENUITEM(Conds, Conditions for job)
XRES_MENUITEM(Asses, Assignments for job)
XRES_MENUITEM(Title, Title/priority/loadlevel)
XRES_MENUITEM(Setrun, Set running)
XRES_MENUITEM(Setcanc, Set cancelled)
XRES_MENUITEM(Force, Force run)
XRES_MENUITEM(Deletej, Delete job)
XRES_MENUITEM(Intsig, Interrupt job)
XRES_MENUITEM(Quitsig, Quit signal)

XRES_TITLE(vtitle)
XRES_LIST(vlist, 10)
XRES_POPUPMENU(do-varpop, varpopup)
XRES_MENUITEM(Plus, Increment)
XRES_MENUITEM(Minus, Decrement)
XRES_MENUITEM(Assign, Assign)
XRES_MENUITEM(Export, Set for export)
XRES_MENUITEM(Clustered, Set clustered)
XRES_MENUITEM(Local, Set local)
XRES_MENUITEM(Deletev, Delete variable)
XRES_SPACE

TAB_SET(40)
XRES_MENU(menubar)
XRES_SPACE

XRES_MENUHDR(Options, Options, O)
XRES_MENUHDR(Action, Action, A)
XRES_MENUHDR(Jobs, Jobs, J)
XRES_MENUHDR(Create, Create, C)
XRES_MENUHDR(Delete, Delete, D)
XRES_MENUHDR(Condition, Condition, n)
XRES_MENUHDR(Variable, Variable, V)
XRES_MENUHDR(Search, Search, S)
XRES_MENUHDR(Help, Help, H)
XRES_MENUHDR(jobmacro, Jobmacro, b)
XRES_MENUHDR(varmacro, Varmacro, r)

XRES_COMMENT(Options pulldown)
XRES_MENUITEM(Viewopts, View options, V, equal, =)
XRES_MENUITEM(Saveopts, Save options, S)
XRES_MENUITEM(Syserror, Display Error log, E)
XRES_MENUITEM(Quit, Quit, Q, C-c)

XRES_COMMENT(Action pulldown)
XRES_MENUITEM(Setrun, Set job runnable, r, r)
XRES_MENUITEM(Setcanc, Set job cancelled, c, z)
XRES_MENUITEM(Force, Force run, F, f)
XRES_MENUITEM(Forceadv, Force run + advance, a, g)
XRES_MENUITEM(Intsig, Kill - interrupt, i, Delete, DEL)
XRES_MENUITEM(Quitsig, Kill - quit, q, C-backslash, C-\\)
XRES_MENUITEM(Stopsig, Stop signal, S)
XRES_MENUITEM(Contsig, Continue signal, C)
XRES_MENUITEM(Othersig, Kill - Other signal, O, O)

XRES_COMMENT(Jobs pulldown)
XRES_MENUITEM(View, View job, V, I)
XRES_MENUITEM(Time, Set job time parameters, t, t)
XRES_MENUITEM(Advtime, Advance to next time, A, A)
XRES_MENUITEM(Title, {Title, pri, Command int, loadlev}, p, p)
XRES_MENUITEM(Process, Process parameters, P, u)
XRES_MENUITEM(Runtime, Time limits, l, l)
XRES_MENUITEM(Mail, Mail and write markers, M, F)
XRES_MENUITEM(Permj, Job permissions, p)
XRES_MENUITEM(Args, Job Arguments, A, G)
XRES_MENUITEM(Env, Job Environment, E, E)
XRES_MENUITEM(Redirs, Job I/O Redirections, R, R)

XRES_COMMENT(Create pulldown)
XRES_MENUITEM(Createj, Create a job from file, j)
XRES_MENUITEM(Timedefs, Set job time defaults, t)
XRES_MENUITEM(Conddefs, Condition defaults, n)
XRES_MENUITEM(Assdefs, Assignment defaults, s)
XRES_MENUITEM(Createv, Create variable, C, C)
XRES_MENUITEM(Rename, Rename variable, R)
XRES_MENUITEM(Interps, Edit command interpreter list, c)
XRES_MENUITEM(Holidays, View/edit Holidays, H)

XRES_COMMENT(Delete pulldown)
XRES_MENUITEM(Deletej, Delete job, D, D)
XRES_MENUITEM(Deletev, Delete variable, v)
XRES_MENUITEM(Unqueue, Unqueue job, U)
XRES_MENUITEM(Freezeh, Copy job options as default to $HOME, H)
XRES_MENUITEM(Freezec, Copy job options as default to Current, C)

XRES_COMMENT(Conditions pulldown)
XRES_MENUITEM(Conds, Set job conditions, c)
XRES_MENUITEM(Asses, Set job assignments, a)

XRES_COMMENT(Variable pulldown)
XRES_MENUITEM(Assign, Assign variable, A)
XRES_MENUITEM(Comment, Assign comment, c)
XRES_MENUITEM(Cluster, Set variable Clustered, C, K)
XRES_MENUITEM(Export, Set variable Exported, E, E)
XRES_MENUITEM(Local, Set variable Local, L, L)
XRES_MENUITEM(Permv, Set variable permissions, p)
XRES_MENUITEM(Plus, Add to variable, A, S-plus, +)
XRES_MENUITEM(Minus, Subtract from variable, S, minus, -)
XRES_MENUITEM(Times, Multiply variable, M, S-8, *)
XRES_MENUITEM(Div, Divide variable, D, slash, /)
XRES_MENUITEM(Mod, Modulo variable, o, S-5, %)
XRES_MENUITEM(Arithc, Set arithmetic value, a, S-2, ")

XRES_COMMENT(Search pulldown)
TAB_SET(48)
XRES_MENUITEM(Search, Search for..., S)
XRES_MENUITEM(Searchforw, Search forward, f, F3)
XRES_MENUITEM(Searchback, Search backward, b, F4)
XRES_SPACE

TAB_SET(40)
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUITEM(About, About, A)
XRES_SPACE
XRES_MACROMENU(jobmacro, Run job command macro, Macro command)
XRES_SPACE
XRES_MACROMENU(varmacro, Run var command macro, Macro command)
XRES_SPACE

XRES_TOOLBAR(toolbar, 60, 100)
XRES_TOOLBARICON(View, View Job, xmbtqViewJob.xpm, View script of selected job)
XRES_TOOLBARICON(Delj, Delete Job, xmbtqDeleteJob.xpm, Delete selected job)
XRES_TOOLBARICON(Srun, Set Run, xmbtqSetRunable.xpm, Set selected job as ready to run)
XRES_TOOLBARICON(Scanc, Set Canc, xmbtqSetCancelled.xpm, Set selected job as cancelled or held)
XRES_TOOLBARICON(Go, Force, xmbtqForce.xpm, {Force job to run, ignoring time but no advance})
XRES_TOOLBARICON(Goadv, Force+Adv, xmbtqForceAdvance.xpm, {Force next run of job (advancing time at end)})
XRES_TOOLBARICON(Int, Interrupt, xmbtqInterupt.xpm, Send interrupt signal to selected job)
XRES_TOOLBARICON(Quit, Quit, xmbtqQuit.xpm, Send quit signal to selected job)
XRES_TOOLBARICON(Time, Set time, xmbtqSetTime.xpm, Set time parameters for job)
XRES_TOOLBARICON(Cond, Condits, xmbtqConditions.xpm, Set conditions for job to start)
XRES_TOOLBARICON(Ass, Assmnts, xmbtqAssignments.xpm, Set assignments for job to invoke)
XRES_TOOLBARICON(Vass, V Ass, xmbtqSetVariable.xpm, Set variable assignment value)
XRES_TOOLBARICON(Vplus, V+, xmbtqIncrementVariables.xpm, Increment variable value by constant)
XRES_TOOLBARICON(Vminus, V-, xmbtqDecrementVariables.xpm, Decrement variable value by constant)

XRES_COMMENT(Dialog titles etc for help/error/info/confirm)

XRES_STDDIALOG(help, {On line help.....})
XRES_STDDIALOG(error, {Whoops!!!})
XRES_STDDIALOG(Confirm, {Are you sure?})

XRES_COMMENT(These are used in various places)
XRES_GENERALLABEL(jobnotitle, Editing job number:)
XRES_GENERALLABEL(varnotitle, Variable to set)

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

XRES_COMMENT(Display options)
XRES_DIALOG(Viewopts, Display options)
XRES_DLGLABEL(viewtitle, Selecting view options)
XRES_DLGLABEL(queuename, Queue name)
XRES_DLGLABEL(qselect, Select a queue)
XRES_DLGLABEL(incnull, Include null queue names as well)
XRES_DLGLABEL(username, Restrict jobs display to user)
XRES_DLGLABEL(uselect, {Select...})
XRES_DLGLABEL(groupname, Restrict jobs display to group)
XRES_DLGLABEL(gselect, {Select...})
XRES_DLGLABEL(allhosts, View all hosts)
XRES_DLGLABEL(localonly, View local host only)
XRES_DLGLABEL(never, Do not confirm before deleting entries)
XRES_DLGLABEL(always, Ask confirmation before deleting entries)
XRES_DLGLABEL(jdispfmt, Reset Job Display Fields)
XRES_DLGLABEL(vdispfmt1, Reset Var Display Line 1)
XRES_DLGLABEL(vdispfmt2, Reset Var Display Line 2)
XRES_DLGLABEL(savehome, Save formats in home directory)
XRES_DLGLABEL(savecurr, Save formats in current directory)
XRES_DLGLABEL(Ok, Apply)
XRES_SPACE

XRES_SELDIALOG(uselect, {Select a user}, {Possible users....}, {Set to....})
XRES_SELDIALOG(gselect, {Select a group}, {Possible groups....}, {Set to....})
XRES_SELDIALOG(qselect, {Select a queue}, {Possible queues....}, {Set to....})
XRES_SPACE

XRES_DIALOG(Jdisp, Reset Job Display Fields)
XRES_DLGLABEL(Newfld, New field)
XRES_DLGLABEL(Newsep, New separator)
XRES_DLGLABEL(Editfld, Edit field)
XRES_DLGLABEL(Editsep, Edit separator)
XRES_DLGLABEL(Delete, Delete field/sep)
XRES_DLGLIST(Jdisplist, 16)
XRES_SPACE

XRES_DIALOG(jfldedit, Job display field)
XRES_DLGLABEL(width, Width of field)
XRES_DLGLABEL(useleft, Use previous field if too large)
XRES_SPACE

XRES_DIALOG(Vdisp, {Reset Variable Display Fields})
XRES_DLGLABEL(Newfld, {New field})
XRES_DLGLABEL(Newsep, {New separator})
XRES_DLGLABEL(Editfld, {Edit field})
XRES_DLGLABEL(Editsep, {Edit separator})
XRES_DLGLABEL(Delete, {Delete field/sep})
XRES_DLGLIST(Vdisplist, 8)
XRES_SPACE

XRES_DIALOG(vfldedit, {Variable display field})
XRES_DLGLABEL(width, {Width of field})
XRES_DLGLABEL(useleft, {Use previous field if too large})
XRES_SPACE

XRES_DIALOG(msave, {Select file to save in})

XRES_COMMENT(Other signal)
XRES_DIALOG(othersig, Send a signal, firebrick, white)
XRES_DLGLABEL(Termsig, {Send Terminate signal})
XRES_DLGLABEL(Killsig, {Send KILL signal})
XRES_DLGLABEL(Hupsig, {Send Hangup signal})
XRES_DLGLABEL(Intsig, {Send Interrupt signal})
XRES_DLGLABEL(Quitsig, {Send Quit signal})
XRES_DLGLABEL(Stopsig, {Send Stop signal})
XRES_DLGLABEL(Contsig, {Send Continue signal})
XRES_DLGLABEL(Alarmsig, {Send Alarm signal})
XRES_DLGLABEL(Bussig, {Send Bus error signal})
XRES_DLGLABEL(Segvsig, {Send Seg violation signal})
XRES_DLGLABEL(Cancel, {Cancel - no signal})

XRES_COMMENT(Time parameters)
XRES_DIALOG(Stime, Set times for job)
XRES_DIALOG(deftime, {Set time defaults for any new time})

XRES_COMMENT(Set title etc dialog)
XRES_DIALOG(Titpri, {Set title, priority, command interpreter, load level})
XRES_DLGLABEL(title, Job title)
XRES_DLGLABEL(priority, Job priority)
XRES_DLGLABEL(interp, Command interpreter)
XRES_DLGLABEL(iselect, Choose a command interpreter)
XRES_DLGLABEL(loadlevel, Load level)
XRES_SPACE

XRES_SELDIALOG(iselect, Select an interpreter, {Possible interpreters....}, {Set to....})
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
XRES_DLGLABEL(Noadvt, {Do not advance time on error})
XRES_DLGLABEL(local, {Job is purely local to current host})
XRES_DLGLABEL(export, {Job is visible, but not runnable elsewhere})
XRES_DLGLABEL(remrun, {Job is runnable remotely})

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
XRES_SPACE

XRES_COMMENT(Environment)
XRES_DIALOG(Jenvs, Environment variables for job)
XRES_LIST(Jenvlist, 15)
XRES_DIALOG(envedit, Edit environment variable)
XRES_DLGLABEL(value, Value:)
XRES_SPACE

XRES_COMMENT(Redirections)
XRES_DIALOG(Jredirs, Redirections list for job)
XRES_LISTWIDGET(Jredirlist, 8)
XRES_DIALOG(rediredit, Edit redirection)
XRES_DLGLABEL(fileno, File descriptor)
XRES_DLGLABEL(file, File name)
XRES_DLGLABEL(dupfd, Duplicated file descriptor)
XRES_SPACE

XRES_COMMENT(Job create)
XRES_SELDIALOG(fselb, Select file to create from)
XRES_SPACE

XRES_DIALOG(timedef, Set time defaults)
XRES_SPACE

XRES_DIALOG(Jcond, Condition variables for job)
XRES_SPACE

XRES_DIALOG(vselect, Select a variable)

XRES_DIALOG(condedit, Set up conditions)
XRES_DLGLABEL(Variable, Variable to use)
XRES_DLGLABEL(vselect, {Choose a variable....})
XRES_DLGLABEL(ncrit, {If host unavailable, ignore})
XRES_DLGLABEL(crit, {If host unavailable, hold})
XRES_DLGLABEL(value, Value to compare against)
XRES_SPACE

TAB_SET(48)
XRES_DIALOG(Jass, Assignments for job)
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

TAB_SET(40)
XRES_DIALOG(vcreate, Create a variable)
XRES_DLGLABEL(varname, {Variable name  })
XRES_DLGLABEL(value, Initial value)
XRES_DLGLABEL(comment, Comments)
XRES_SPACE

XRES_DIALOG(vrename, Rename a variable)
XRES_DLGLABEL(newname, {New name       })
XRES_SPACE

XRES_DIALOG(Interps, Command interpreter list)
XRES_LIST(Interplist, 6)
XRES_SPACE

XRES_DIALOG(ciedit, Edit command interpreter)
XRES_DLGLABEL(name, Name of c.i.)
XRES_DLGLABEL(nice, {Nice level  })
XRES_DLGLABEL(loadlev, Load level)
XRES_DLGLABEL(path, {Path        })
XRES_DLGLABEL(args, {Predef args })
XRES_DLGLABEL(setarg0, Set arg 0 to job title)
XRES_SPACE

XRES_DIALOG(holidays, Holidays file)
XRES_DLGLABEL(selh, Select as a holiday)
XRES_DLGLABEL(uselh, Not a holiday)
XRES_TRANSPOPUP(cal, do-caltog)
XRES_SPACE

XRES_DIALOG(Junqueue, Unqueue job)
XRES_DLGLABEL(dselect, Choose a directory)
XRES_DLGLABEL(copyonly, {Copy out only, no delete})
XRES_DLGLABEL(Cmdfile, {Shell script file name          })
XRES_DLGLABEL(Jobfile, {Job file name (copy of job data)})
XRES_SPACE

XRES_DIALOG(Jfreeze, Copy default options)
XRES_DLGLABEL(noverbose, Non-verbose)
XRES_DLGLABEL(verbose, Verbose)
XRES_DLGLABEL(normal, {Normal (ready to run)})
XRES_DLGLABEL(cancelled, Cancelled state)
XRES_SPACE

XRES_DIALOG(vass, Assign value to variable)
XRES_DLGLABEL(value, Value)
XRES_SPACE

XRES_DIALOG(vcomm, Reset comment of variable)
XRES_DLGLABEL(varnotitle, Variable comment to set)
XRES_DLGLABEL(comment, Comment)
XRES_SPACE

XRES_DIALOG(vmode, Variable permissions)
XRES_DLGLABEL(username, {Set user to })
XRES_DLGLABEL(groupname, {Set group to})
XRES_DLGLABEL(uselect, {Choose a user....})
XRES_DLGLABEL(gselect, {Choose a group...})
XRES_DLGLABEL(vuread, Read user)
XRES_DLGLABEL(vgread, By group)
XRES_DLGLABEL(voread, By others)
XRES_DLGLABEL(vuwrite, Write user)
XRES_DLGLABEL(vgwrite, By group)
XRES_DLGLABEL(vowrite, By others)
XRES_DLGLABEL(vushow, Reveal)
XRES_DLGLABEL(vgshow, By group)
XRES_DLGLABEL(voshow, By others)
XRES_DLGLABEL(vurmode, Read mode)
XRES_DLGLABEL(vgrmode, By group)
XRES_DLGLABEL(vormode, By others)
XRES_DLGLABEL(vuwmode, Write mode)
XRES_DLGLABEL(vgwmode, By group)
XRES_DLGLABEL(vowmode, By others)
XRES_DLGLABEL(vuutake, Assume owner)
XRES_DLGLABEL(vgutake, By group)
XRES_DLGLABEL(voutake, By others)
XRES_DLGLABEL(vugtake, Ass grp owner)
XRES_DLGLABEL(vggtake, By group)
XRES_DLGLABEL(vogtake, By others)
XRES_DLGLABEL(vuugive, Assign user)
XRES_DLGLABEL(vgugive, By group)
XRES_DLGLABEL(vougive, By others)
XRES_DLGLABEL(vuggive, Assign group)
XRES_DLGLABEL(vgggive, By group)
XRES_DLGLABEL(voggive, By others)
XRES_DLGLABEL(vudel, Delete)
XRES_DLGLABEL(vgdel, By group)
XRES_DLGLABEL(vodel, By others)
XRES_SPACE

XRES_DIALOG(setconst, {Set constant for arithmetic operations})
XRES_DLGLABEL(constval, {Value to be set to ......})
XRES_SPACE

XRES_DIALOG(Search, {Search for...})
XRES_DLGLABEL(lookfor, {Searching for...})
XRES_DLGLABEL(jobs, Jobs)
XRES_DLGLABEL(vars, Variables)
XRES_DLGLABEL(title, Job title)
XRES_DLGLABEL(user, {User (exact match)})
XRES_DLGLABEL(group, {Group (exact match)})
XRES_DLGLABEL(name, Variable name)
XRES_DLGLABEL(comment, Variable comment)
XRES_DLGLABEL(value, Variable value)
XRES_DLGLABEL(forward, Search forwards)
XRES_DLGLABEL(backward, Search backwards)
XRES_DLGLABEL(match, Match case)
XRES_DLGLABEL(wrap, Wrap around)
XRES_SPACE

XRES_DIALOG(about, {About xmbtq.....})

XRES_COMMENT(Titles for view / view error)
XRES_SUBAPP(xmbtqview, {Display job....}, 600, 500)
XRES_PANED(pane)
XRES_MENU(viewmenu)

XRES_GENERALLABEL(jobnotitle, Viewing job number:)
XRES_GENERALLABEL(Header, Title:)
XRES_GENERALLABEL(User, Owner:)

TAB_SET(48)
XRES_MENUHDR(Job, Job, J)
XRES_MENUHDR(Srch, Search, S)
XRES_MENUITEM(Exit, Exit, E, C-c)
XRES_MENUITEM(Search, {Search for...}, S)
XRES_MENUITEM(Searchforw, Search forward, f, F3)
XRES_MENUITEM(Searchback, Search backward, b, F4)
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)

XRES_VIEWHDR(vscroll, LIST)
XRES_VIEWHDR(actform, VIEWACT)
XRES_SPACE
XRES_ENDSUBAPP

TAB_SET(32)
XRES_SUBAPP(xmbtqverr, {Display system error log...},, 700)
XRES_PANED(pane)
XRES_VIEWHDR(form, VIEWHDR)
XRES_VIEWHDR(vscroll, ERRLOG)
XRES_VIEWHDR(actform, VIEWACT)
XRES_MENU(viewmenu)
XRES_SPACE

TAB_SET(48)
XRES_MENUHDR(File, File, F)
XRES_MENUITEM(Clearlog, Clear log file, C)
XRES_MENUITEM(Exit, Exit, E, C-c)
XRES_SPACE
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_ENDSUBAPP
XRES_END
