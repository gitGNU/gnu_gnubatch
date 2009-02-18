#! /bin/sh
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
#  Produce versions of screen helpfiles for various common terminals
#

cwd=`pwd`
if expr $cwd : '.*/src/helpmsg' >/dev/null
then echo Please do not run the src directory version of these files
     echo Run it from the build directory instead
     exit 10
fi

for screenhelp in btq btuser
do
	if [ -f $screenhelp.original ]
	then
		mv -f $screenhelp.original saved
		rm -f $screenhelp.*
		mv saved $screenhelp.help
	fi
	sed -f - $screenhelp.help >$screenhelp.dumb <<\%
s/,\\kUP//
s/,\\kDOWN//
s/,\\kLEFT//
s/,\\kRIGHT//
s/,\\kHOME//
s/\\kKILL/\\kINTR/
%
	sed -f - $screenhelp.help >$screenhelp.vt100 <<\%
s/\\kKILL/\\kINTR/
s/,\\kHOME//
/==============/{
s/=/-/g
s/:/:\\/
}
%
	for t in xterm nxterm vt220 vt320
	do
		ln -f $screenhelp.vt100 $screenhelp.$t
	done
	sed -f - $screenhelp.help >$screenhelp.wy60 <<\%
s/,\\kHOME//
s/\\kKILL/\\kQUIT/
s/\\kERASE/\\x7f/
s/:\^[lL]/:^R/
%
	mv $screenhelp.help $screenhelp.original
	ln -f $screenhelp.dumb $screenhelp.help
done
