#   Copyright 2013 Free Software Foundation, Inc.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

import string

def qmatch(pattern, strng):
    """Perform glob-style queue matching

patterns may be separated by ,s
* matches any number of anything
? matches one character
[p-xac] matches p to x or a or c
[!p-xac] matches anything but
[^p-xac] similar
. stands for itself"""

    for p in string.split(pattern, ','):
        if qmtch(list(p), list(strng)):
            return True
    return False

def qmtch(plist, slist):
    """Match an individual pattern"""
    scnt = 0
    slen = len(slist)
    inrange = False

    for pc in plist:
        if inrange:
            if pc == ']':
                if not checkrange(rchars, slist[scnt]):
                    return False
                scnt += 1
                inrange = False
            else:
                rchars += pc
        elif pc == '*':
            for cnt in range(slen,-1,-1):
                if qmtch(plist[1:], slist[cnt:]):
                    return True
            return False
        elif pc == '?':
            if scnt >= slen:
                return False
            scnt += 1
        elif pc == '[':
            # Don't bother to check if we've exhausted the string
            if scnt >= slen:
                return False
            inrange = True
            rchars = ''
        else:
            if scnt >= slen or slist[scnt] != pc:
                return False
            scnt += 1

    return not inrange and scnt >= slen

def checkrange(rstring, ch):
    """Check if character is in range"""
    try:
        posrange = True
        if rstring[0] == '!':
            rstring = rstring[1:]
            posrange = False
        if rstring[0] == '-':
            if ch == '-': return posrange
            rstring = rstring[1:]
    except IndexError:
        return False

    scnt = 0
    slen = len(rstring)
    och = ord(ch)
    while scnt < slen:
        ch1 = ord(rstring[scnt])
        ch2 = ch1
        scnt += 1
        if scnt+1 < slen and rstring[scnt] == '-':
            ch2 = ord(rstring[scnt+1])
            scnt += 2
            if och >= ch1 and och <= ch2:
                return posrange
    return not posrange

