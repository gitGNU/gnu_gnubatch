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

import gbnetid

class gbident(gbnetid.gbnetid):
    """Representation of hostid/slot pair

We use this everywhere to uniquely identify jobs and variables
Do it as an extension to netids"""

    def __init__(self, netid="", sl=-1):
        gbnetid.gbnetid.__init__(self, netid)
        self.slotno = sl

    def __str__(self):
        return self.netid + ':' + str(self.slotno)

    def __hash__(self):
        return gbnetid.gbnetid.__hash__(self) ^ self.slotno

    def __eq__(self, other):
        """Comparison op for ids"""
        return gbnetid.gbnetid.__eq__(self,other) and self.slotno == other.slotno

    def __ne__(self, other):
        """Comparison op for ids"""
        return gbnetid.gbnetid.__ne__(self,other) or self.slotno != other.slotno

    def isvalid(self):
        """Indicate valid ident"""
        return self.slotno >= 0
