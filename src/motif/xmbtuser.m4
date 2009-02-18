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
XRES_APPLICATION(gbch-xmuser, {GBCH-XMUSER - Edit GNUbatch user options}, 32)

TOPLEVEL_RESOURCE(toolTipEnable, True)
TOPLEVEL_RESOURCE(titlePresent, True)
TOPLEVEL_RESOURCE(footerPresent, True)
TOPLEVEL_RESOURCE(sortType, Username)
XRES_SPACE

XRES_WIDGETOFFSETS(10,10,5,5)
XRES_SPACE
XRES_TITLE(utitle, {User     Group   Def Min Max Maxll Totll Spcll Privs})
XRES_SPACE

XRES_FOOTER(footer)
XRES_SPACE
XRES_PANED(layout)
XRES_SPACE

TAB_SET(40)
XRES_LIST(dlist, 0)
XRES_LIST(ulist, 15)
XRES_SPACE

XRES_MENU(menubar)
XRES_SPACE
XRES_MENUHDR(Options, Options, O)
XRES_MENUHDR(Defaults, Defaults, D)
XRES_MENUHDR(Users, Users, U)
XRES_MENUHDR(Charges, Charges, C)
XRES_MENUHDR(usermacro, Usermacro, m)
XRES_MENUHDR(Help, Help, H)

XRES_COMMENT(Options pulldown)
TAB_SET(48)
XRES_MENUITEM(Disporder, Display order, o, c-o)
XRES_MENUITEM(Saveopts, Save options, S)
XRES_MENUITEM(Quit, Quit, Q, c-c)
XRES_SPACE

XRES_MENUITEM(dpri, Priorities, P, P)
XRES_MENUITEM(dloadl, Load Level, L, L)
XRES_MENUITEM(dmode, Mode, M, M)
XRES_MENUITEM(dpriv, Privileges, v, V)
XRES_MENUITEM(defcpy, Copy to All users, A, A)
XRES_SPACE

XRES_MENUITEM(upri, Priorities, P, p)
XRES_MENUITEM(uloadl, Load Level, L, l)
XRES_MENUITEM(umode, Mode, M, m)
XRES_MENUITEM(upriv, Privileges, v, v)
XRES_MENUITEM(ucpy, Copy defaults, d, c-d)
XRES_SPACE

XRES_MENUITEM(Display, Display Charges, C)
XRES_MENUITEM(Zero, Zero charges for selected users, z, Z)
XRES_MENUITEM(Zeroall, Zero charges for ALL users, Z, A-z)
XRES_MENUITEM(Impose, Impose fee, I, I)
XRES_SPACE

XRES_MENUITEM(Search, Search, S)
XRES_MENUITEM(Search, {Search for...}, S)
XRES_MENUITEM(Searchforw, Search forward, f, F3)
XRES_MENUITEM(Searchback, Search backward, b, F4)

XRES_COMMENT(Pulldowns for "help")
XRES_MENUITEM(Help, Help, H, F1)
XRES_MENUITEM(Helpon, Help on, o, S-F1)
XRES_MENUITEM(About, About, A)
XRES_SPACE

XRES_MACROMENU(usermacro, Run command macro, Macro command)

XRES_COMMENT(These are used in various places)
TAB_SET(32)
XRES_GENERALLABEL(defedit, Editing default options to apply to new users)
XRES_GENERALLABEL(useredit, {Editing options for users...})
XRES_GENERALLABEL(jobsm, Jobs)
XRES_GENERALLABEL(varsm, Variables)
XRES_GENERALLABEL(juread, Read user)
XRES_GENERALLABEL(jgread, By group)
XRES_GENERALLABEL(joread, By others)
XRES_GENERALLABEL(juwrite, Write user)
XRES_GENERALLABEL(jgwrite, By group)
XRES_GENERALLABEL(jowrite, By others)
XRES_GENERALLABEL(jushow, Reveal)
XRES_GENERALLABEL(jgshow, By group)
XRES_GENERALLABEL(joshow, By others)
XRES_GENERALLABEL(jurmode, Read mode)
XRES_GENERALLABEL(jgrmode, By group)
XRES_GENERALLABEL(jormode, By others)
XRES_GENERALLABEL(juwmode, Write mode)
XRES_GENERALLABEL(jgwmode, By group)
XRES_GENERALLABEL(jowmode, By others)
XRES_GENERALLABEL(juutake, Assume owner)
XRES_GENERALLABEL(jgutake, By group)
XRES_GENERALLABEL(joutake, By others)
XRES_GENERALLABEL(jugtake, Ass grp owner)
XRES_GENERALLABEL(jggtake, By group)
XRES_GENERALLABEL(jogtake, By others)
XRES_GENERALLABEL(juugive, Assign user)
XRES_GENERALLABEL(jgugive, By group)
XRES_GENERALLABEL(jougive, By others)
XRES_GENERALLABEL(juggive, Assign group)
XRES_GENERALLABEL(jgggive, By group)
XRES_GENERALLABEL(joggive, By others)
XRES_GENERALLABEL(judel, Delete)
XRES_GENERALLABEL(jgdel, By group)
XRES_GENERALLABEL(jodel, By others)
XRES_GENERALLABEL(jukill, Kill)
XRES_GENERALLABEL(jgkill, By group)
XRES_GENERALLABEL(jokill, By others)
XRES_GENERALLABEL(vuread, Read user)
XRES_GENERALLABEL(vgread, By group)
XRES_GENERALLABEL(voread, By others)
XRES_GENERALLABEL(vuwrite, Write user)
XRES_GENERALLABEL(vgwrite, By group)
XRES_GENERALLABEL(vowrite, By others)
XRES_GENERALLABEL(vushow, Reveal)
XRES_GENERALLABEL(vgshow, By group)
XRES_GENERALLABEL(voshow, By others)
XRES_GENERALLABEL(vurmode, Read mode)
XRES_GENERALLABEL(vgrmode, By group)
XRES_GENERALLABEL(vormode, By others)
XRES_GENERALLABEL(vuwmode, Write mode)
XRES_GENERALLABEL(vgwmode, By group)
XRES_GENERALLABEL(vowmode, By others)
XRES_GENERALLABEL(vuutake, Assume owner)
XRES_GENERALLABEL(vgutake, By group)
XRES_GENERALLABEL(voutake, By others)
XRES_GENERALLABEL(vugtake, Ass grp owner)
XRES_GENERALLABEL(vggtake, By group)
XRES_GENERALLABEL(vogtake, By others)
XRES_GENERALLABEL(vuugive, Assign user)
XRES_GENERALLABEL(vgugive, By group)
XRES_GENERALLABEL(vougive, By others)
XRES_GENERALLABEL(vuggive, Assign group)
XRES_GENERALLABEL(vgggive, By group)
XRES_GENERALLABEL(voggive, By others)
XRES_GENERALLABEL(vudel, Delete)
XRES_GENERALLABEL(vgdel, By group)
XRES_GENERALLABEL(vodel, By others)
XRES_GENERALLABEL(radmin, Read system administration file)
XRES_GENERALLABEL(wadmin, Write system administration file)
XRES_GENERALLABEL(create, Create entries (new jobs and variables))
XRES_GENERALLABEL(spcreate, Special create - adjust load levels and command interpreters)
XRES_GENERALLABEL(sstop, Stop the scheduler (and adjust network connections))
XRES_GENERALLABEL(cdeflt, Change default modes)
XRES_GENERALLABEL(orug, OR together USER and GROUP permissions)
XRES_GENERALLABEL(oruo, OR together USER and OTHER permissions)
XRES_GENERALLABEL(orgo, OR together GROUP and OTHER permissions)

XRES_COMMENT(Dialog titles etc for help/error/info/confirm)
XRES_STDDIALOG(help, {On line help.....})
XRES_STDDIALOG(error, {Whoops!!!})
XRES_STDDIALOG(Confirm, {Are you sure?})

XRES_COMMENT(Display options)
TAB_SET(48)
XRES_DIALOG(Disporder, Set order and options)
XRES_DLGLABEL(sortuid, Sort users into order of numeric userid)
XRES_DLGLABEL(sortuser, Sort users into alphabetic order of user name)
XRES_DLGLABEL(sortgroup, Sort users into alphabetic order of group name then user)
XRES_DLGLABEL(llstep, Increment for load levels)
XRES_DLGLABEL(Ok, Apply)

XRES_COMMENT(Priorities)
XRES_DIALOG(defpri, Set default priorities)
XRES_DLGLABEL(min, Minimum priority)
XRES_DLGLABEL(def, Default priority)
XRES_DLGLABEL(max, Maximum priority)

XRES_COMMENT(Load levels)
XRES_DIALOG(defloadlev, Default load levels)
XRES_DLGLABEL(max, Maximum load level for any one job)
XRES_DLGLABEL(tot, Total load level for all jobs on machine)
XRES_DLGLABEL(spec, {"Special create" default load level})

XRES_COMMENT(Modes)
XRES_DIALOG(defmode, Default modes)
XRES_DIALOG(umode, User modes)
XRES_DLGLABEL(Setdef, Set to default values)

XRES_COMMENT(Privileges)
XRES_DIALOG(defprivs, Default privileges)
XRES_DIALOG(uprivs, User privileges)
XRES_DLGLABEL(cdef, Set to default values)
XRES_SPACE

XRES_DIALOG(chlist, {List of charges....}, 200)
XRES_SPACE

XRES_DIALOG(uimpose, Impose charges)
XRES_DLGLABEL(amount, Charge amount:)
XRES_SPACE

XRES_DIALOG(Search, {Search list for user...})
XRES_DLGLABEL(lookfor, Search for user:)
XRES_DLGLABEL(forward, Search forward)
XRES_DLGLABEL(backward, Search backward)
XRES_DLGLABEL(match, Match case)
XRES_DLGLABEL(wrap, Wrap around)
XRES_SPACE

XRES_DIALOG(about, {About xmbtuser.....})
XRES_END
