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

import socket, struct

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *

import btclasses
import netmsg
import reqmess
import btqwopts
import btmode

def ignoremsg(msg, sender):
    """Do nothing with a message for messages we don't worry about"""
    pass

def op_reqsync(msg, sender):
    """Respond to a sync request from someone by just sending an endsync"""
    sender.send_netmsg(reqmess.N_ENDSYNC, 0, sender.host)

def op_endsync(msg, sender):
    """Process end sync (we've got all the stuff)"""
    sender.syncdone()

def replynok(msg, sender):
    """Reply OK to network lock request"""
    sender.send_netmsg(reqmess.N_REMLOCK_OK, 0, sender.host)

def setup_despatch(mw):
    """Set up lookup table for message codes

Jobs and variables go through main window"""

    global Decodemsg

    Decodemsg = {

        # Job requests         msg                 processing routine

        reqmess.J_CREATE:       (netmsg.jobhnetmsg, mw.addjob),
        reqmess.J_DELETE:       (netmsg.jobcmsg,    ignoremsg),
        reqmess.J_CHANGE:       (netmsg.jobhnetmsg, ignoremsg),
        reqmess.J_CHOWN:        (netmsg.jugmsg,     ignoremsg),
        reqmess.J_CHGRP:        (netmsg.jugmsg,     ignoremsg),
        reqmess.J_CHMOD:        (netmsg.jobhnetmsg, mw.jhchanged),
        reqmess.J_KILL:         (netmsg.jobcmsg,    ignoremsg),
        reqmess.J_FORCE:        (netmsg.jobcmsg,    ignoremsg),
        reqmess.J_FORCENA:      (netmsg.jobcmsg,    ignoremsg),
        reqmess.J_CHANGED:      (netmsg.jobhnetmsg, ignoremsg),
        reqmess.J_HCHANGED:     (netmsg.jobhnetmsg, mw.jhchanged),
        reqmess.J_BCHANGED:     (netmsg.jobhnetmsg, mw.jchanged),
        reqmess.J_BHCHANGED:    (netmsg.jobhnetmsg, mw.jhchanged),
        reqmess.J_BOQ:          (netmsg.jobcmsg,    mw.boqjob),
        reqmess.J_BFORCED:      (netmsg.jobcmsg,    mw.jobforced),
        reqmess.J_BFORCEDNA:    (netmsg.jobcmsg,    mw.jobforcedna),
        reqmess.J_DELETED:      (netmsg.jobcmsg,    mw.deletedjob),
        reqmess.J_CHMOGED:      (netmsg.jobhnetmsg, mw.chmogedjob),
        reqmess.J_PROPOSE:      (netmsg.jobcmsg,    ignoremsg),
        reqmess.J_PROPOK:       (netmsg.jobcmsg,    ignoremsg),
        reqmess.J_CHSTATE:      (netmsg.jobstatmsg, mw.statchjob),
        reqmess.J_RNOTIFY:      (netmsg.jobnotmsg,  ignoremsg),
        reqmess.V_CREATE:       (netmsg.varnetmsg,  mw.createdvar),
        reqmess.V_DELETE:       (netmsg.varnetmsg,  ignoremsg),
        reqmess.V_ASSIGN:       (netmsg.varnetmsg,  ignoremsg),
        reqmess.V_CHOWN:        (netmsg.varnetmsg,  ignoremsg),
        reqmess.V_CHGRP:        (netmsg.varnetmsg,  ignoremsg),
        reqmess.V_CHMOD:        (netmsg.varnetmsg,  ignoremsg),
        reqmess.V_CHFLAGS:      (netmsg.varnetmsg,  mw.chflagsvar),
        reqmess.V_DELETED:      (netmsg.varnetmsg,  mw.deletedvar),
        reqmess.V_ASSIGNED:     (netmsg.varnetmsg,  mw.assignedvar),
        reqmess.V_CHMOGED:      (netmsg.varnetmsg,  mw.chmogedvar),
        reqmess.V_RENAMED:      (netmsg.varnetmsg,  mw.renamedvar),
        reqmess.N_SHUTHOST:     (netmsg.netmsg,     mw.clearhost),
        reqmess.N_ABORTHOST:    (netmsg.netmsg,     mw.clearhost),
        reqmess.N_NEWHOST:      (netmsg.netmsg,     ignoremsg),
        reqmess.N_TICKLE:       (netmsg.netmsg,     ignoremsg),
        reqmess.N_CONNECT:      (netmsg.netmsg,     ignoremsg),
        reqmess.N_DISCONNECT:   (netmsg.netmsg,     ignoremsg),
        reqmess.N_PCONNOK:      (netmsg.netmsg,     ignoremsg),
        reqmess.N_REQSYNC:      (netmsg.netmsg,     op_reqsync),
        reqmess.N_ENDSYNC:      (netmsg.netmsg,     op_endsync),
        reqmess.N_REQREPLY:     (netmsg.netmsg,     mw.reqreply),
        reqmess.N_DOCHARGE:     (netmsg.netmsg,     ignoremsg),
        reqmess.N_WANTLOCK:     (netmsg.netmsg,     replynok),
        reqmess.N_UNLOCK:       (netmsg.netmsg,     ignoremsg),
        reqmess.N_SYNCSINGLE:   (netmsg.netmsg,     ignoremsg),
        reqmess.N_RJASSIGN:     (netmsg.rassmsg,    mw.remassjob),
        reqmess.N_CONNOK:       (netmsg.netmsg,     ignoremsg),
        reqmess.N_REMLOCK_NONE: (netmsg.netmsg,     ignoremsg),
        reqmess.N_REMLOCK_OK:   (netmsg.netmsg,     ignoremsg),
        reqmess.N_REMLOCK_PRIO: (netmsg.netmsg,     ignoremsg)
        }

def pushwrite(sock, data):
    """Push out data to socket"""
    l = len(data)
    while l > 0:
        nb = sock.write(data)
        if nb <= 0:
            raise netmsg.msgexception("Error writing on socket to " + str(sock.peerAddress().toString()))
        l -= nb
        data = data[nb:]

class gbclient(QObject):
    """Responsible for managing connection with a server"""

    STATE_NULL = 0
    STATE_CONNRQ = 1
    STATE_SYNCRQ = 2
    STATE_COMPLETE = 3

    def __init__(self, host, callw):
        self.state = gbclient.STATE_NULL
        self.host = host
        self.caller = callw
        self.socket = QTcpSocket()
        self.socket.connected.connect(callw.conndone)
        self.socket.readyRead.connect(callw.readready)
        self.socket.disconnected.connect(callw.servdisc)
        self.socket.error.connect(callw.serverror)
        self.msglength = 4
        self.pendinglength = 4      # Length of code and length
        self.pendingcode = 0
        self.pendingmsg = ""

    def __hash__(self):
        return gbnetid.gbnetid.__hash__(self.host)

    def __eq__(self, other):
        return gbnetid.gbnetid.__eq__(self.host, other.host)

    def ipaddr(self):
        return self.host.servip.ipaddr()

    def startconnect(self):
        """Start up the TCP connection"""
        if self.state != gbclient.STATE_NULL: return
        self.state = gbclient.STATE_CONNRQ
        self.socket.connectToHost(self.ipaddr(), btqwopts.Options.ports.connect_tcp)

    def conndone(self):
        """Note connection done"""
        if self.state != gbclient.STATE_CONNRQ: return
        if self.socket.state() != QAbstractSocket.ConnectedState: return
        # Request sync with host
        self.send_netmsg(reqmess.N_REQSYNC)
        self.state = gbclient.STATE_SYNCRQ
        self.host.connected = True
        self.host.syncreq = True

    def syncdone(self):
        """Note sync completed"""
        self.state = gbclient.STATE_COMPLETE
        self.host.synccomplete = True

    def is_sync(self):
        """Return whether sync done"""
        return  self.state == gbclient.STATE_COMPLETE

    def readready(self):
        """Process incoming message"""

        # We may get readready called with only half a message available to read
        # but even if the rest of the message arrives whilst we're looking at it,
        # we have to read what we can and exit waiting for another call

        while 1:

            # How much we are expecting - might be nothing if nothing there

            tocome = self.socket.bytesAvailable()
            if tocome == 0:  return
            if tocome > self.pendinglength:
                tocome = self.pendinglength

            # Read what we can

            buf = self.socket.read(tocome)
            nb = len(buf)
            if nb <= 0:
                raise netmsg.msgexception("Error  on socket from " + str(sock.peerAddress().toString()))

            # Add what we've read to the bit we have to read

            self.pendinglength -= nb
            self.pendingmsg += buf
            if self.pendinglength > 0: continue     # Expecting more

            # OK got the buffer we are looking for, decide what to do according to pendingcode

            if self.pendingcode == 0:

                # Just getting code and length

                (code, length) = struct.unpack("!2H", self.pendingmsg)
                if  length <= 4:
                    raise netmsg.msgexception("Unexpected message length %d" % length)

                # Now we look up the code
                # If we don't understand it give up

                if code not in Decodemsg:
                    raise netmsg.msgexception("Unexpected code %d in message" % code)

                self.pendingcode = code
                self.msglength = length
                self.pendinglength = length - 4         # Supplied length includes code and length
                self.pendingmsg = ""

            else:
                # We've got the message, so decode it

                # Get the class we're reading in from the code and the function to apply
                # Certain message types (currently only jobs with strings)
                # might require stuff on the end - get from the length being greater

                msgtype, proc = Decodemsg[self.pendingcode]
                msg = msgtype()
                msg.decode_rest(self.pendingcode, self.msglength, self.pendingmsg[0:msgtype.size])
                if self.msglength != msgtype.size+4:
                    # If it's not a job header we don't like it
                    if not isinstance(msg, netmsg.jobhnetmsg):
                        raise netmsg.msgexception("unexpected message length %d expecting %d" % (self.msglength, msgtype.size))
                    msg.decode_strings(self.pendingmsg[msgtype.size:])

                # Actually do the business

                proc(msg, self)

                # Reset pointers to expect code/length next time

                self.pendingcode = 0
                self.pendingmsg = ""
                self.pendinglength = self.msglength = 4

    def ispermitted(self, modestr, perm):
        """See if specified operation is permitted by the given server"""
        return modestr.mpermitted(self.host.perms, perm)

    def visible(self, modestr):
        """Report whether given object (mode supplied) is visible"""
        return modestr.mpermitted(self.host.perms, btmode.btmode.BTM_SHOW)

    def servdisc(self):
        """Note server has disconnected"""
        if self.state == gbclient.STATE_NULL: return
        if self.socket.state() == QAbstractSocket.ConnectedState: return
        self.caller.serverdisconn(self)

    def serverror(self, error):
        """Note error from server"""
        if self.state == gbclient.STATE_NULL: return
        if self.socket.error() == -1: return
        self.caller.serverdisconn(self)

    def send_netmsg(self, code, arg=0):
        """Send a random sort of net message to a host."""
        outmsg = netmsg.netmsg(code, arg, self.host)
        pushwrite(self.socket, outmsg.encode())

    def closesock(self):
        """Close socket when finished"""
        self.socket.close()

    def shuthost(self):
        """Shut down connection tidily"""
        if self.socket and self.socket.isValid():
            try:
                self.send_netmsg(reqmess.N_SHUTHOST)
            except netmsg.msgexception:
                pass
            self.closesock()

    def chgjobhdr(self, j):
        """Update job header (everything but strings)"""
        msg = netmsg.jobhnetmsg(reqmess.J_HCHANGED, j, self.host)
        pushwrite(self.socket, msg.encode())

    def chgjob(self, j):
        """Update job including strings"""
        msg = netmsg.jobhnetmsg(reqmess.J_CHANGED, j, self.host)
        pushwrite(self.socket, msg.encode_strings())

    def jchmod(self, j):
        msg = netmsg.jobhnetmsg(reqmess.J_CHMOD, j, self.host)
        pushwrite(self.socket, msg.encode())

    def jobop(self, j, code, param = 0):
        msg = netmsg.jobcmsg(code, j, self.host, param)
        pushwrite(self.socket, msg.encode())

    def vassign(self, v):
        msg = netmsg.varnetmsg(reqmess.V_ASSIGN, v, self.host)
        pushwrite(self.socket, msg.encode())

    def vcomment(self, v):
        msg = netmsg.varnetmsg(reqmess.V_CHCOMM, v, self.host)
        pushwrite(self.socket, msg.encode())

    def vchmod(self, v):
        msg = netmsg.varnetmsg(reqmess.V_CHMOD, v, self.host)
        pushwrite(self.socket, msg.encode())
