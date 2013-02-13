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

import struct
import string

class cmdint:
    """Representation of command interpreter"""

    cmdint_str = struct.Struct("!HBB16s76s28s")
    size = cmdint_str.size

    def __init__(self, n = ""):
        self.name = n
        self.path = ""
        self.nice = 24
        self.ll = 1000
        self.flags = 0
        self.args = []

    def __str__(self):
        return self.name

    def __hash__(self):
        return self.name.__hash__()

    def __eq__(self, other):
        """Comparison op for ids"""
        return self.name == other.name

    def __ne__(self, other):
        """Comparison op for ids"""
        return self.name != other.name

    def decode(self, msg):
        """Decode incoming structure"""
        self.ll, self.nice, self.flags, nam, pth, args = cmdint.cmdint_str.unpack(msg)
        self.name = string.split(nam, '\x00')[0]
        self.path = string.split(pth, '\x00')[0]
        args = string.split(args, '\x00')[0]
        self.args = string.split(args, ' ')
 