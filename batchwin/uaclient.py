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
import string
import struct
import select
import btuser
import btmode
import btclasses
import btqwopts
import cmdint

# Operation error codes

errorcodes = (
    ('XBNQ_OK',                 'OK'),                          # 0
    ('XBNR_UNKNOWN_CLIENT',     'This client is unknown'),      # 1
    ('XBNR_NOT_CLIENT',         'This host is not a client'),   # 2
    ('XBNR_NOT_USERNAME',       'Not username'),                # 3
    ('XBNR_BADCI',              'Bad commmand interpreter'),    # 4
    ('XBNR_BADCVAR',            'Invalid condition variable'),  # 5
    ('XBNR_BADAVAR',            'Invalid assignment variable'), # 6
    ('XBNR_Code7',              'Unknown code 7'),              # 7
    ('XBNR_NOMEM_QF',           'No space for queue file'),     # 8
    ('XBNR_NOCRPERM',           'No create permission'),        # 9
    ('XBNR_BAD_PRIORITY',       'Invalid priority'),            # 10
    ('XBNR_BAD_LL',             'Invalid load level'),          # 11
    ('XBNR_BAD_USER',           'Invalid user'),                # 12
    ('XBNR_FILE_FULL',          'File system full'),            # 13
    ('XBNR_QFULL',              'Job/var queue full'),          # 14
    ('XBNR_BAD_JOBDATA',        'Invalid job data'),            # 15
    ('XBNR_UNKNOWN_USER',       'Unknown user'),                # 16
    ('XBNR_UNKNOWN_GROUP',      'Unknown group'),               # 17
    ('XBNR_ERR',                'Error'),                       # 18
    ('XBNR_NORADMIN',           'No read admin permission'),    # 19
    ('XBNR_NOCMODE',            'No change mode permission'),   # 20
    ('XBNR_MINPRIV',            'Too few permissions on object'),# 21
    ('XBNR_EXISTS',             'Already exists'),              # 22
    ('XBNR_NOTEXPORT',          'Inconsistent export flag'),    # 23
    ('XBNR_CLASHES',            'Name clashes with existing'),  # 24
    ('XBNR_DSYSVAR',            'Attempt to delete sys var'),   # 25
    ('XBNR_NOPERM',             'No permission for operation'),  # 26
    ('XBNR_NEXISTS',            'Variable does not exist'),     # 27
    ('XBNR_INUSE',              'Variable in use in cond/ass')) # 28

errcode_lookup = {}
n=0
for e in errorcodes:
    errcode_lookup[e[0]] = n
    n += 1

XBNQ_OK =   0           # OK

UAL_LOGIN   =   30      # Log in with user name & password
UAL_LOGOUT  =   31      # Log out
UAL_ENQUIRE =   32      # Enquire about user id
UAL_OK      =   33      # Logged in ok
UAL_NOK     =   34      # Not logged in yet
UAL_INVU    =   35      # Not logged in, invalid user
UAL_INVP    =   36      # Not logged in, invalid passwd
UAL_NEWGRP  =   37      # New group
UAL_INVG    =   38      # Invalid group

# Op codes

CL_SV_UENQUIRY = 0              # Request for permissions (single byte)
CL_SV_STARTJOB = 1              # Start job
CL_SV_CONTJOB = 2               # More or job
CL_SV_JOBDATA = 3               # Job data
CL_SV_ENDJOB = 4                # End of last job
CL_SV_HANGON = 5                # Hang on for next block of data

CL_SV_CREATEVAR = 7             # Create variable
CL_SV_DELETEVAR = 8             # Delete variable

CL_SV_ULIST = 10
CL_SV_VLIST = 11                # Send list of valid variables
CL_SV_CILIST = 12               # Send list of command interpreters
CL_SV_HLIST = 13                # Send list of holidays
CL_SV_GLIST = 14                # Send list of valid groups
CL_SV_UMLPARS = 15              # Send umask and ulimit
CL_SV_ELIST = 16                # Send environment variables

SV_CL_ACK = 0                   # Acknowledge and continue
XBNR_UNKNOWN_CLIENT = 1         # Unknown client
XBNR_BADCI = 4                  # Invalid CI
XBNR_BADCVAR = 5                # Bad cond variable
XBNR_BADAVAR = 6                # Bad assignment variable
XBNR_BAD_JOBDATA = 15           # Bad job data
XBNR_ERR = 18                   # General error given by param
SV_CL_TOENQ = 20                # Are you still there? (single byte)
SV_CL_PEND_FULL = 21            # Queue of pending jobs full
SV_CL_UNKNOWNC = 22             # Unknown command
SV_CL_BADPROTO = 23             # Something wrong protocol
SV_CL_UNKNOWNJ = 24             # Out of sequence job

# Login ops

UAL_LOGIN = 30
UAL_LOGOUT = 31
UAL_ENQUIRE = 32
UAL_ULOGIN = 39                 # Log in as UNIX user
UAL_UENQUIRE = 40               # Enquire about user from UNIX

# Responses

UAL_OK = 33             # Logged in ok
UAL_NOK = 34            # Not logged in yet
UAL_INVU = 35           # Not logged in, invalid user
UAL_INVP = 36           # Not logged in, invalid passwd

# Sizes of things

CL_SV_BUFFSIZE = 256            # Buffer size for sending UDP stuff
UIDSIZE = 11                    # Size of buffer for user names
UA_PASSWDSZ = 31                # Password buffer size
HOSTNSIZE = 14                  # Host name size

# This is the size of a login buffer

TIMEOUTERROR = 100

if btqwopts.WIN:
    Enqcode = UAL_ENQUIRE
    Enq_username = ""
else:
    import pwd
    import os
    Enqcode = UAL_UENQUIRE
    try:
        Enq_username = pwd.getpwuid(os.getuid()).pw_name
    except KeyError:
        Enq_username = "nobody"

#######################################################################################
#
#                W A R N I N G !!!!!
#                *******************
#
# These structures are heavily dependent on UIDSIZE (length of Unix user names)
# WUIDSIZE (length of Windows user names)
# HOSTNSIZE (length of machine names)
# plus assumptions about how the things are packed into structs
#
# If you fiddle with those values you'll have to fiddle with these!!
#
# Agreed: This all wants to be replaced by lumps of XML PDQ
#
# The sizes below are based on UIDSIZE = 11 (plus 1 null)
# WUIDSIZE = 23 (plus 1 null)
# HOSTNSIZE = 30 (plus 2 nulls
#
######################################################################################

Login_struct = struct.Struct('!B3x24s32s32s')
Ulogin_struct = struct.Struct('!B3x24s32sl28x')
Ni_jobhdr = struct.Struct('!BxH12s12s')
Ua_reply = struct.Struct('!12s12s4BlL9Hxx')
Client_if = struct.Struct('!Bxxxl')
sendvar_struct = struct.Struct('!bxH12s12x')
Usendvar_struct = struct.Struct('!bxH12xl8x')
umlreply = struct.Struct('!HxxL')

def envify(es):
    """Parse environment variable into tuple of name and value"""
    parts = string.split(es, "=")
    st = parts.pop(0)
    v = string.join(parts, '=')
    return (st,v)

class createvarmsg:

    # This assumes Name size of 19, Comment size of 41 and string value size of 49
    ##############################################################################

    Createvar = struct.Struct('!BxHl3HBB49sx19sx41sx')

    def __init__(self, n, c, v, f, m):
        self.name = n
        self.comment = c
        self.flags = f
        if isinstance(v, int):
            self.longvalue = v
            self.textvalue = ""
            self.consttype = btclasses.btconst.CON_LONG
        else:
            self.longvalue = 0
            self.textvalue = v
            self.consttype = btclasses.btconst.CON_STRING
        self.modes = m

    def encode(self, code = CL_SV_CREATEVAR):
        """Encode up for sending"""
        return createvarmsg.Createvar.pack(code,
                                           createvarmsg.Createvar.size,
                                           self.longvalue,
                                           self.modes.u_flags, self.modes.g_flags, self.modes.o_flags,
                                           self.flags, self.consttype,
                                           self.textvalue,
                                           self.name, self.comment)

class uaclientException(Exception):
    """Class for UA op errors"""

    def __init__(self, msg, code = 0, retryable = True, length = 0, explength = 0):
        Exception.__init__(self, msg)
        self.code = code
        self.retryable = retryable
        self.length = length
        self.explength = explength

class uaclient:
    """Class for user client operations"""

    def __init__(self):
        self.hostadd = None
        self.sockfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def setup(self, ip, port):
        """Register IP and port"""
        self.hostadd = (ip, port)

    def finished(self):
        """Finished with socket"""
        self.sockfd.close()

    def closeReopen(self):
        """Close and reopen socket in case we get confused by incoming data later"""
        self.sockfd.close()
        self.sockfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def isopen(self):
        """Verify that the socket hasn't been closed"""
        try:
            f = self.sockfd.fileno()
            return True
        except socket.error:
            return False

    def waitreply(self, multiplier = 1.0):
        """Wait for a reply on the socket"""
        obs = select.select([self.sockfd.fileno()],[],[], (btqwopts.Options.udpwaittime/1000.0) * multiplier)
        if len(obs[0]) == 0:
            raise uaclientException("Reply timeout from server", code=TIMEOUTERROR, retryable=False)

    def getuglist(self, code):
        """Get set of users from host"""
        self.sockfd.sendto(chr(code), self.hostadd)
        result = set()
        while 1:
            self.waitreply()
            indata = self.sockfd.recv(CL_SV_BUFFSIZE)
            if len(indata) == 1 and ord(indata) == 0: return result
            ulist = string.split(indata, '\x00')
            result |= set(filter(lambda s: len(s)>0, ulist))

    def getulist(self):
        """Get set of users from host"""
        return self.getuglist(CL_SV_ULIST)

    def getglist(self):
        """Get set of groups from host"""
        return self.getuglist(CL_SV_GLIST)

    def getvlist(self, modes):
        """Get list of variables from host"""
        if btqwopts.WIN:
            self.sockfd.sendto(sendvar_struct.pack(CL_SV_VLIST, modes, ""), self.hostadd)
        else:
            self.sockfd.sendto(Usendvar_struct.pack(CL_SV_VLIST, modes, os.getuid()), self.hostadd)
        result = set()
        while 1:
            self.waitreply()
            indata = self.sockfd.recv(CL_SV_BUFFSIZE)
            if len(indata) == 1 and ord(indata) == 0: return result
            vlist = string.split(indata, '\x00')
            result |= set(filter(lambda s: len(s)>0, vlist))

    def getcilist(self):
        """Get set of command interpreters from host"""
        self.sockfd.sendto(chr(CL_SV_CILIST), self.hostadd)
        result = set()
        piece = cmdint.cmdint.size
        while 1:
            self.waitreply()
            indata = self.sockfd.recv(CL_SV_BUFFSIZE)
            if len(indata) < piece: return result
            while len(indata) >= piece:
                msg = indata[0:cmdint.cmdint.size]
                indata = indata[cmdint.cmdint.size:]
                ci = cmdint.cmdint()
                ci.decode(msg)
                result |= set([ci])

    def getuml(self):
        self.sockfd.sendto(chr(CL_SV_UMLPARS), self.hostadd)
        self.waitreply()
        indata = self.sockfd.recv(CL_SV_BUFFSIZE)
        if len(indata) != umlreply.size: return (0,0)
        um, ul = umlreply.unpack(indata)
        return (um, ul)

    def getelist(self):
        self.sockfd.sendto(chr(CL_SV_ELIST), self.hostadd)
        result = []
        while 1:
            self.waitreply()
            indata = self.sockfd.recv(CL_SV_BUFFSIZE)
            if len(indata) < 3: break
            evs = string.split(indata, '\x00')
            result.extend(filter(lambda s: '=' in s, evs))
        return map(envify, result)

    def uaenquire(self):
        """Initial enquiry about user name"""
        if btqwopts.WIN:
            reqbuf = Login_struct.pack(Enqcode, Enq_username, "", "")
        else:
            reqbuf = Ulogin_struct.pack(Enqcode, Enq_username, "", os.getuid())
        self.sockfd.sendto(reqbuf, self.hostadd)
        self.waitreply()
        indata = self.sockfd.recv(CL_SV_BUFFSIZE)

        if len(indata) != Login_struct.size:
            raise uaclientException("Unexpected size login buffer", length=len(indata), explength=Login_struct.size, retryable=False)

        code, username, pw, mach = Login_struct.unpack(indata)
        if code != UAL_OK:
            raise uaclientException("Not logged in", code=code)
        return string.split(username, '\x00')[0]

    def ualogin(self, username, password, machinename):
        """Try to log in and return tuple of unix user and spdet structure"""
        if btqwopts.WIN:
            self.sockfd.sendto(Login_struct.pack(UAL_LOGIN, username, password, machinename), self.hostadd)
        else:
            self.sockfd.sendto(Ulogin_struct.pack(UAL_ULOGIN, username, password, os.getuid()), self.hostadd)
        self.waitreply()
        indata = self.sockfd.recv(CL_SV_BUFFSIZE)
        if len(indata) != Login_struct.size:
            raise uaclientException("Unexpected size login buffer", length=len(indata), explength=Login_struct.size, retryable=False)
        ecode, username, pw, mach = Login_struct.unpack(indata)
        if ecode != UAL_OK:
            if ecode <= 21:
                msg = errorcodes[ecode][1]
                rt = False
            else:
                msg = "Invalid user or password "
                rt = True
            raise uaclientException(msg, code=ecode, retryable=rt)
        return string.split(username, '\x00')[0]

    def ualogout(self):
        """Try to log out, no reply expected"""
        self.sockfd.sendto(Login_struct.pack(UAL_LOGOUT, '', '', ''), self.hostadd)

    def getbtuser(self):
        """Get user permissions"""
        msgout = Ni_jobhdr.pack(CL_SV_UENQUIRY, Ni_jobhdr.size, Enq_username, "")
        self.sockfd.sendto(msgout, self.hostadd)
        self.waitreply()
        indata = self.sockfd.recv(CL_SV_BUFFSIZE)
        if len(indata) != Ua_reply.size:
            raise uaclientException("Unexpected size getbtuser buffer", length=len(indata), explength=Ua_reply.size, retryable=False)
        username, groupname, isvalid, minp, maxp, defp, uid, priv, maxll, totll, spec_ll, jfu, jfg, jfo, vfu, vfg, vfo = Ua_reply.unpack(indata)
        ret = btuser.btuserenv()
        ret.btu_minp = minp
        ret.btu_maxp = maxp
        ret.btu_defp = defp
        ret.btu_user = uid
        ret.btu_priv = priv
        ret.btu_maxll = maxll
        ret.btu_totll = totll
        ret.btu_specll = spec_ll
        ret.btu_jflags = (jfu, jfg, jfo)
        ret.btu_vflags = (vfu, vfg, vfo)
        ret.set_ugname(string.split(username, '\x00')[0], string.split(groupname, '\x00')[0])
        return ret

    def createvar(self, cmsg):
        """Create a veriable"""
        self.sockfd.sendto(cmsg.encode(), self.hostadd)
        self.waitreply()
        indata = self.sockfd.recv(CL_SV_BUFFSIZE)
        if len(indata) != Client_if.size:
            raise uaclientException("Unexpected size createvar result", length=len(indata), explength=Client_if.size, retryable=False)
        code, param = Client_if.unpack(indata)
        return code

    def deletevar(self, cmsg):
        """Delete a variable"""
        self.sockfd.sendto(cmsg.encode(CL_SV_DELETEVAR), self.hostadd)
        self.waitreply()
        indata = self.sockfd.recv(CL_SV_BUFFSIZE)
        if len(indata) != Client_if.size:
            raise uaclientException("Unexpected size delete var result", length=len(indata), explength=Client_if.size, retryable=False)
        code, param = Client_if.unpack(indata)
        return code

