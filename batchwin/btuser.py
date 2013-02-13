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

import btmode

class btuser:

    """Representation of user permission class"""

    BTM_ORP_UG = (1 << 8)       # Or user and group permissions
    BTM_ORP_UO = (1 << 7)       # Or user and other permissions
    BTM_ORP_GO = (1 << 6)       # Or group and other permissions
    BTM_SSTOP  = (1 << 5)       # Stop the thing
    BTM_UMASK  = (1 << 4)       # Change privs
    BTM_SPCREATE = (1 << 3)     # Special create
    BTM_CREATE = (1 << 2)       # Create anything
    BTM_RADMIN = (1 << 1)       # Read admin priv
    BTM_WADMIN = (1 << 0)       # Write admin priv

    def __init__(self):
        self.btu_minp = 100
        self.btu_defp = 150
        self.btu_maxp = 200

        self.btu_user = 0       # User id on machine

        self.btu_priv = btuser.BTM_UMASK | btuser.BTM_CREATE

        self.btu_maxll = 1000
        self.btu_totll = 10000
        self.btu_specll = 1000

        self.btu_jflags = (btmode.btmode.BTM_READ|btmode.btmode.BTM_WRITE|btmode.btmode.BTM_SHOW|btmode.btmode.BTM_RDMODE|btmode.btmode.BTM_WRMODE|btmode.btmode.BTM_UGIVE|btmode.btmode.BTM_GGIVE|btmode.btmode.BTM_DELETE|btmode.btmode.BTM_KILL,
                           btmode.btmode.BTM_READ|btmode.btmode.BTM_SHOW|btmode.btmode.BTM_RDMODE|btmode.btmode.BTM_GGIVE,
                           btmode.btmode.BTM_SHOW|btmode.btmode.BTM_RDMODE)
        self.btu_vflags = (btmode.btmode.BTM_READ|btmode.btmode.BTM_WRITE|btmode.btmode.BTM_SHOW|btmode.btmode.BTM_RDMODE|btmode.btmode.BTM_WRMODE|btmode.btmode.BTM_UGIVE|btmode.btmode.BTM_GGIVE|btmode.btmode.BTM_DELETE,
                           btmode.btmode.BTM_READ|btmode.btmode.BTM_SHOW|btmode.btmode.BTM_RDMODE|btmode.btmode.BTM_GGIVE,
                           btmode.btmode.BTM_SHOW|btmode.btmode.BTM_RDMODE)

class btuserenv(btuser):
    """Extend representation to include logged-in user names"""

    def __init__(self, un = "", gn = ""):
        btuser.__init__(self)
        self.username = un
        self.groupname = gn

    def set_ugname(self, un, gn):
        """Set user and group names"""
        self.username = un
        self.groupname = gn
