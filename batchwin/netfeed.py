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

import re
import struct
import btqwopts
import gbclient
import netmsg

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *

charmatch = re.compile("[\x00-\x08\x0b-\x1f\x7f-\xff]")

def net_feed(job):
    """Initialise socket to provide required feed and return it"""
    feedpkt = netmsg.feeder(job_number = job.bj_job)
    socket = QTcpSocket()
    socket.connectToHost(job.netid, btqwopts.Options.ports.jobview)
    if not socket.waitForConnected(1000):
        socket.close()
        return None
    gbclient.pushwrite(socket, feedpkt.encode())
    return socket

def replesc(gr):
    """Substitute sequences for control characters"""
    ch = ord(gr.group(0))
    result = ""
    if ch == 13:
        return ""
    if ch >= 128:
        result = "M-"
        ch -= 128
    if ch == 127: return result + "DEL"
    if ch < 32:
        result += '^'
        ch += 64
    return result + chr(ch)

def feed_string(socket):
    """Get a bufferworth of data"""
    socket.waitForReadyRead(500)
    buf = socket.read(1024)
    if not buf:
        return ""
    return charmatch.sub(replesc, buf)
