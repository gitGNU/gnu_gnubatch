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
XRES_APPLICATION(gbch-xmfilemon, Select file(s) for monitoring, 32)

XRES_DIALOG(dselb, Select directory to monitor)

XRES_DIALOG(fsdlg, Select file(s) for monitoring)
XRES_SPACE

TAB_SET(40)
XRES_DLGLABEL(dirname, Directory to watch)
XRES_DLGLABEL(selectd, {Select dir...})
XRES_SPACE

XRES_DLGLABEL(ifapp, Proceed if file appears)
XRES_DLGLABEL(nogrow, Proceed if file does not grow)
XRES_DLGLABEL(mtime, Proceed if file unwritten)
XRES_DLGLABEL(ctime, Proceed if file unchanged)
XRES_DLGLABEL(atime, Proceed if file unread)
XRES_DLGLABEL(ifrem, Proceed if file deleted)
XRES_SPACE

XRES_DLGLABEL(include, Include pre-existing files)
XRES_DLGLABEL(recursive, Recursively scan subdirectories)
XRES_DLGLABEL(followlinks, Follow symbolic links)
XRES_SPACE

XRES_DLGLABEL(nogrowtime, Time to wait for no changes)
XRES_SPACE

XRES_DLGLABEL(anyf, Any file name)
XRES_DLGLABEL(pattf, File(s) matching pattern)
XRES_DLGLABEL(specf, Exact file name)
XRES_SPACE

XRES_DLGLABEL(filename, File name or pattern)
XRES_DLGLABEL(cont, Continue looking for other files)
XRES_SPACE

XRES_DLGLABEL(daem, Detatch as separate daemon process)
XRES_SPACE

XRES_DLGLABEL(cscmd, Run command not script (%f = file name, %d = directory))
XRES_SPACE

XRES_DLGLABEL(polltime, Poll time)
XRES_DLGLABEL(command, Command to execute)
XRES_END
