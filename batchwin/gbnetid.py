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

import socket
import struct

def is_ipaddr(item):
    """Discover whether item looks like an IP address"""
    try:
        socket.inet_aton(item)
        return True
    except socket.error:
        return False

# Stuff for recording my name and IP

orig_myname = myname = socket.gethostname()
try:
    myhostid = socket.gethostbyname(myname)
except socket.gaierror:
    myhostid = localhostid
orig_myhostid = myhostid

def is_localaddr(ip):
    """Return true if address is my local address"""
    global myhostid
    return ip == myhostid or ip[0:4] == '127.'

def set_localaddr(ip, name=""):
    """Set local address after discovering it somehow"""
    global myhostid, myname
    if ip != myhostid:
        myhostid = ip
    if len(name) != 0 and name != myname:
        myname = name

class gbnetid:
    """Represent a network id - idea is to provide for IPv6"""

    def __init__(self, id=""):
        self.local = False
        self.netid = "0.0.0.0"
        try:
            xid = socket.inet_ntoa(socket.inet_aton(id))
            if is_localaddr(xid):
                self.local = True
            else:
                self.netid = xid
        except socket.error:
            self.local = True

    def set_netid(self, id):
        """After reading incoming job or whatever, reset netid"""
        self.netid = id
        self.local = is_localaddr(self.netid)
        if self.local: self.netid = "0.0.0.0"

    def get_netid(self):
        """Pack up netid for sending"""
        if self.local:
            return socket.inet_aton(myhostid)
        return socket.inet_aton(self.netid)

    def ipaddr(self):
        return self.netid

    def __hash__(self):
        return self.netid.__hash__()

    def __eq__(self, other):
        """Comparison op for netids - assumes canonical rep"""
        if isinstance(other,str):
            return self.netid == other
        return self.netid == other.netid

    def __ne__(self, other):
        """Comparison op for netids - assumes canonical rep"""
        if isinstance(other,str):
            return self.netid != other
        return self.netid != other.netid

    def samehost(self, h):
        """Is it same host as given string"""
        return self.netid == h
