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
import copy
import time
import uaclient
import gbnetid
import gbident
import btclasses
import btqwopts

# Message header including !

MSGHDR_STR = '!HH4sl12s12s'
JIDENT_STR = '4sl'
VIDENT_STR = '4sl'
MODE_STR = '4l12s12s12s12s3Hxx'

class msgexception(Exception):
    """Throw me if I get message problems"""
    pass

def parse_str(str_str):
    """Use struct logic to parse format string

Return tuple with struct for outgoing and incoming with code, incoming without code
and length for incoming where we've already got them code"""

    outgoing = struct.Struct(str_str)
    incoming_no_code = struct.Struct('!' + str_str[3:])
    return (outgoing, incoming_no_code, incoming_no_code.size)

class msgbase:
    """Base class for messages"""

    def __init__(self, serv, code, length):
        self.code = code
        self.length = length;
        self.pid = 0
        self.hostid = btqwopts.Options.servers.myhostid
        try:
            self.muser = serv.perms.username
            self.mgroup = serv.perms.groupname
        except AttributeError:
            self.muser = ""
            self.mgroup = ""
        self.jid = None

    def set_jobid(self, jobid):
        """Set jobid field as supplied or to empty"""
        if jobid is None:
            self.jid = gbident.gbident()
        else:
            self.jid = jobid


class netmsg(msgbase):
    """Class for network-oriented messages"""

    netmsg_str, rest_netmsg_str, size = parse_str(MSGHDR_STR + 'l')

    def __init__(self, c = 0, a = 0, s = None):
        msgbase.__init__(self, s, c, 40)
        self.arg = a

    def encode(self):
        """Encode message as a string"""
        try:
            return netmsg.netmsg_str.pack(self.code,
                                          self.length,
                                          socket.inet_aton(self.hostid),
                                          self.pid,
                                          self.muser,
                                          self.mgroup,
                                          self.arg)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode(self, msg):
        """Decode incoming string"""
        try:
            self.code, self.length, hs, \
            self.pid, u, g, \
            self.arg = netmsg.netmsg_str.unpack(msg)
            self.hostid = socket.inet_ntoa(hs)
            self.muser = string.split(u, '\x00')[0]
            self.mgroup = string.split(g, '\x00')[0]
        except struct.error:
            raise msgexception("Unexpected message format")

    def decode_rest(self, c, l, msg):
        """Decode incoming string where we've already met the code and length"""
        self.code = c
        self.length = l
        try:
            hs, \
            self.pid, u, g, \
            self.arg = netmsg.rest_netmsg_str.unpack(msg)
            self.hostid = socket.inet_ntoa(hs)
            self.muser = string.split(u, '\x00')[0]
            self.mgroup = string.split(g, '\x00')[0]
        except struct.error:
            raise msgexception("Unexpected message format")

class jobstatmsg(msgbase):
    """Format of message to talk about changes in job status"""

    jobstatmsg_str, rest_jobstatmsg_str, size = parse_str(MSGHDR_STR + JIDENT_STR + 'HBx2l4s')

    def __init__(self, c = 0, jobid = None, s = None):
        msgbase.__init__(self, s, c, 60)
        msgbase.set_jobid(self, jobid)
        self.lastexit = 0
        self.prog = btjob.BJP_NONE
        self.nexttime = 0
        self.lastpid = 0
        self.runhost = "0.0.0.0"

    def encode(self):
        """Encode message as a string"""
        try:
            return jobstatmsg.jobstatmsg_str.pack(self.code,
                                          self.length,
                                          socket.inet_aton(self.hostid),
                                          self.pid,
                                          self.muser,
                                          self.mgroup,
                                          socket.inet_aton(self.jid.ipaddr()),
                                          self.jid.slotno,
                                          self.lastexit,
                                          self.prog,
                                          self.nexttime,
                                          self.lastpid,
                                          socket.inet_aton(self.runhost))
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode(self, msg):
        """Decode incoming string"""
        try:
            self.code, self.length, hs, \
            self.pid, u, g, \
            hj, sj, self.lastexit, self.prog, \
            self.nexttime, self.lastpid, hr = jobstatmsg.jobstatmsg_str.unpack(msg)
            self.hostid = socket.inet_ntoa(hs)
            self.muser = string.split(u, '\x00')[0]
            self.mgroup = string.split(g, '\x00')[0]
            self.jid = gbident.gbident(socket.inet_ntoa(hj), sj)
            self.runhost = socket.inet_ntoa(hr)
        except struct.error:
            raise msgexception("Unexpected message format")

    def decode_rest(self, c, l, msg):
        """Decode incoming string where we've already met the code and length"""
        self.code = c
        self.length = l
        try:
            hs, \
            self.pid, u, g, \
            hj, sj, self.lastexit, self.prog, \
            self.nexttime, self.lastpid, hr = jobstatmsg.rest_jobstatmsg_str.unpack(msg)
            self.hostid = socket.inet_ntoa(hs)
            self.muser = string.split(u, '\x00')[0]
            self.mgroup = string.split(g, '\x00')[0]
            self.jid = gbident.gbident(socket.inet_ntoa(hj), sj)
            self.runhost = socket.inet_ntoa(hr)
        except struct.error:
            raise msgexception("Unexpected message format")

class jugmsg(msgbase):
    """Format of message to talk about changes in job owner/group also vars"""

    jugmsg_str, rest_jugmsg_str, size = parse_str(MSGHDR_STR + JIDENT_STR + '12s')

    def __init__(self, c = 0, jobid = None, s = None):
        msgbase.__init__(self, s, c, 56)
        msgbase.set_jobid(self, jobid)
        self.newug = ""

    def encode(self):
        """Encode message as a string"""
        try:
            return jugmsg.jugmsg_str.pack(self.code,
                                          self.length,
                                          socket.inet_aton(self.hostid),
                                          self.pid,
                                          self.muser,
                                          self.mgroup,
                                          socket.inet_aton(self.jid.ipaddr()),
                                          self.jid.slotno,
                                          self.newug)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode(self, msg):
        """Decode incoming string"""
        try:
            self.code, self.length, hs, \
            self.pid, u, g, \
            hj, sj, self.newug = jugmsg.jugmsg_str.unpack(msg)
            self.hostid = socket.inet_ntoa(hs)
            self.muser = string.split(u, '\x00')[0]
            self.mgroup = string.split(g, '\x00')[0]
            self.jid = gbident.gbident(socket.inet_ntoa(hj), sj)
        except struct.error:
            raise msgexception("Unexpected message format")

    def decode_rest(self, c, l, msg):
        """Decode incoming string where we've already met the code and length"""
        self.code = c
        self.length = l
        try:
            hs, \
            self.pid, u, g, \
            hj, sj, self.newug = jugmsg.jugmsg_str.unpack(msg)
            self.hostid = socket.inet_ntoa(hs)
            self.muser = string.split(u, '\x00')[0]
            self.mgroup = string.split(g, '\x00')[0]
            self.jid = gbident.gbident(socket.inet_ntoa(hj), sj)
        except struct.error:
            raise msgexception("Unexpected message format")

def getnulltstring(s, offset):
    """Get a null terminated string from string starting at offset"""
    si = s.find('\x00', offset)
    if si < 0:
        return s[offset:]
    return s[offset:si]

class jobhnetmsg(msgbase):
    """Job header only where we don't have to worry about strings"""

    jhdr_str, rest_jhdr_str, size = parse_str(MSGHDR_STR +
                                              JIDENT_STR +
                                              '7Bx' +
                                              '7Hxx' +
                                              'L4l4s4s2l2L' +
                                              '16s4B' +
                                              MODE_STR +
                                              '640s' +
                                              '544s')

    jcond_str_s = struct.Struct('!' + VIDENT_STR + '3Bx52s')
    jcond_str_i = struct.Struct('!' + VIDENT_STR + '3Bxl48x')
    jass_str_s = struct.Struct('!' + VIDENT_STR + 'H3B3x52s')
    jass_str_i = struct.Struct('!' + VIDENT_STR + 'H3B3xl48x')

    jstring_str = struct.Struct('!3H5h')

    def __init__(self, c = 0, j = None, s = None):
        msgbase.__init__(self, s, c, 36 + 8 + 8 + 16 + 44 + 20 + 72 + 640 + 544)
        if j is None:
            self.job = btclasses.btjob()
        else:
            self.job = copy.deepcopy(j)

    def encode(self):
        """Encode message as a string"""
        try:
            # Pack up conditions
            condstr = ''
            for c in self.job.bj_conds:
                if c.bjc_compar == btclasses.jcond.C_UNUSED or not c.bjc_value.isdef: continue
                if c.bjc_value.isint:
                    condseg = jobhnetmsg.jcond_str_i.pack(c.bjc_varind.get_netid(),
                                                          c.bjc_varind.slotno,
                                                          c.bjc_compar,
                                                          c.bjc_iscrit,
                                                          btclasses.btconst.CON_LONG,
                                                          c.bjc_value.value)
                else:
                    condseg = jobhnetmsg.jcond_str_s.pack(c.bjc_varind.get_netid(),
                                                          c.bjc_varind.slotno,
                                                          c.bjc_compar,
                                                          c.bjc_iscrit,
                                                          btclasses.btconst.CON_STRING,
                                                          c.bjc_value.value)
                condstr += condseg
            if len(condstr) < 640:
                condstr += '\x00' * (640 - len(condstr))

            # Pack up assignments

            assstr = ''
            for a in self.job.bj_asses:
                if a.bja_op == btclasses.jass.BJA_NONE or not a.bja_con.isdef: continue
                if a.bja_con.isint:
                    assseg = jobhnetmsg.jass_str_i.pack(a.bja_varind.get_netid(),
                                                        a.bja_varind.slotno,
                                                        a.bja_flags,
                                                        a.bja_op,
                                                        a.bja_iscrit,
                                                        btclasses.btconst.CON_LONG,
                                                        a.bja_con.value)
                else:
                    assseg = jobhnetmsg.jass_str_s.pack(a.bja_varind.get_netid(),
                                                        a.bja_varind,slotno,
                                                        a.bja_flags,
                                                        a.bja_op,
                                                        a.bja_iscrit,
                                                        btclasses.btconst.CON_STRING,
                                                        a.bja_con.value)
                assstr += assseg
            if len(assstr) < 544:
                assstr += '\x00' * (544 - len(assstr))

            # Now assemble the thing

            return jobhnetmsg.\
                   jhdr_str.pack(self.code,
                                 self.length,
                                 socket.inet_aton(self.hostid),
                                 self.pid,
                                 self.muser,
                                 self.mgroup,
                                 socket.inet_aton(self.job.ipaddr()),
                                 self.job.slotno,
                                 self.job.bj_progress,
                                 self.job.bj_pri,
                                 self.job.bj_jflags,
                                 self.job.bj_times.tc_istime,
                                 self.job.bj_times.tc_mday,
                                 self.job.bj_times.tc_repeat,
                                 self.job.bj_times.tc_nposs,
                                 self.job.bj_ll,
                                 self.job.bj_umask,
                                 self.job.bj_times.tc_nvaldays,
                                 self.job.bj_autoksig,
                                 self.job.bj_runon,
                                 self.job.bj_deltime,
                                 self.job.bj_lastexit,
                                 self.job.bj_job,
                                 self.job.bj_time,
                                 self.job.bj_stime,
                                 self.job.bj_etime,
                                 self.job.bj_pid,
                                 socket.inet_aton(self.job.bj_orighostid),
                                 socket.inet_aton(self.job.bj_runhostid),
                                 self.job.bj_ulimit,
                                 self.job.bj_times.tc_nexttime,
                                 self.job.bj_times.tc_rate,
                                 self.job.bj_runtime,
                                 self.job.bj_cmdinterp,
                                 self.job.exitn.lower,
                                 self.job.exitn.upper,
                                 self.job.exite.lower,
                                 self.job.exite.upper,
                                 self.job.bj_mode.o_uid,
                                 self.job.bj_mode.o_gid,
                                 self.job.bj_mode.c_uid,
                                 self.job.bj_mode.c_gid,
                                 self.job.bj_mode.o_user,
                                 self.job.bj_mode.o_group,
                                 self.job.bj_mode.c_user,
                                 self.job.bj_mode.c_group,
                                 self.job.bj_mode.u_flags,
                                 self.job.bj_mode.g_flags,
                                 self.job.bj_mode.o_flags,
                                 condstr,
                                 assstr)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode_rest(self, c, l, msg):
        """Decode all of an incoming job-header only message"""

        self.code = c
        self.length = l

        try:
            hs, self.pid, u, g, \
            hj, sj, \
            progress, pri, jflags, istime, mday, repeat, nposs, \
            ll, umask, nvaldays, autoksig, runon, deltime, lastexit, \
            jobnum, time, stime, etime, pid, ho, hr, ulimit, nexttime, rate, runtime, \
            cmdinterp, \
            exitnl, exitnu, exitel, exiteu, \
            o_uid, o_gid, c_uid, c_gid, o_user, o_group, c_user, c_group, \
            u_flags, g_flags, o_flags, \
            condstr, assstr = jobhnetmsg.rest_jhdr_str.unpack(msg)
        except struct.error:
            raise msgexception("Unexpected message format")

        # Sort message header

        self.hostid = socket.inet_ntoa(hs)
        self.muser = string.split(u, '\x00')[0]
        self.mgroup = string.split(g, '\x00')[0]

        # Create and set up job structure

        self.job = btclasses.btjob(socket.inet_ntoa(hj), sj)
        self.job.bj_progress = progress
        self.job.bj_pri = pri
        self.job.bj_jflags = jflags
        self.job.bj_times.tc_istime = istime
        self.job.bj_times.tc_mday = mday
        self.job.bj_times.tc_repeat = repeat
        self.job.bj_times.tc_nposs = nposs
        self.job.bj_ll = ll
        self.job.bj_umask = umask
        self.job.bj_times.tc_nvaldays = nvaldays
        self.job.bj_autoksig = autoksig
        self.job.bj_runon = runon
        self.job.bj_deltime = deltime
        self.job.bj_lastexit = lastexit
        self.job.bj_job = jobnum
        self.job.bj_time = time
        self.job.bj_stime = stime
        self.job.bj_etime = etime
        self.job.bj_pid = pid
        self.job.bj_orighostid = socket.inet_ntoa(ho)
        self.job.bj_runhostid = socket.inet_ntoa(hr)
        self.job.bj_ulimit = ulimit
        self.job.bj_times.tc_nexttime = nexttime
        self.job.bj_times.tc_rate = rate
        self.job.bj_runtime = runtime
        self.job.bj_cmdinterp = string.split(cmdinterp, '\x00')[0]
        self.job.exitn = btclasses.exits(exitnl, exitnu)
        self.job.exite = btclasses.exits(exitel, exiteu)
        self.job.bj_mode.o_uid = o_uid
        self.job.bj_mode.o_gid = o_gid
        self.job.bj_mode.c_uid = c_uid
        self.job.bj_mode.c_gid = c_gid
        self.job.bj_mode.o_user = string.split(o_user, '\x00')[0]
        self.job.bj_mode.o_group = string.split(o_group, '\x00')[0]
        self.job.bj_mode.c_user = string.split(c_user, '\x00')[0]
        self.job.bj_mode.c_group = string.split(c_group, '\x00')[0]
        self.job.bj_mode.u_flags = u_flags
        self.job.bj_mode.g_flags = g_flags
        self.job.bj_mode.o_flags = o_flags

        # Now for conditions

        while len(condstr) > 0:
            condpart = condstr[0:64]
            condstr = condstr[64:]
            vh, vs, comp, crit, type, v = jobhnetmsg.jcond_str_i.unpack(condpart)
            if comp == btclasses.jcond.C_UNUSED: continue
            if type == btclasses.btconst.CON_STRING:
                vh, vs, comp, crit, type, v = jobhnetmsg.jcond_str_s.unpack(condpart)
                v = string.split(v, '\x00')[0]
            nc = btclasses.jcond()
            nc.bjc_compar = comp
            nc.bjc_iscrit = crit
            nc.bjc_varind = gbident.gbident(socket.inet_ntoa(vh), vs)
            nc.bjc_value = btclasses.btconst(v)
            self.job.bj_conds.append(nc)

        while len(assstr) > 0:
            asspart = assstr[0:68]
            assstr = assstr[68:]
            vh, vs, fl, op, crit, type, v = jobhnetmsg.jass_str_i.unpack(asspart)
            if op == btclasses.jass.BJA_NONE: continue
            if type == btclasses.btconst.CON_STRING:
                vh, vs, fl, op, crit, type, v = jobhnetmsg.jass_str_s.unpack(asspart)
                v = string.split(v, '\x00')[0]
            na = btclasses.jass()
            na.bja_op = op
            na.bja_flags = fl
            na.bja_iscrit = crit
            na.bja_varind = gbident.gbident(socket.inet_ntoa(vh), vs)
            na.bja_con = btclasses.btconst(v)
            self.job.bj_asses.append(na)

    def encode_strings(self):
        """Encode whole job including strings"""
        stringspace = ""    # May need to check it isn't > 3000

        titoff = diroff = rediroff = envoff = argoff = -1

        spaceoffset = 0

        # Set up arguments as a vector of pointers to strings

        if len(self.job.bj_arg) != 0:
            argoff = spaceoffset
            spaceoffset += len(self.job.bj_arg) * 2
            argoffsets = []
            for a in self.job.bj_arg:
                argoffsets.append(spaceoffset)
                spaceoffset += len(a) + 1
            stringspace += struct.pack("!%dH" % len(argoffsets), *argoffsets)
            stringspace += string.join(self.job.bj_arg, '\x00') + '\x00'

        if len(self.job.bj_env) != 0:
            envoff = spaceoffset
            spaceoffset += len(self.job.bj_env) * 4
            envoffsets = []
            for e in self.job.bj_env:
                envoffsets.append(spaceoffset)
                spaceoffset += len(e.e_name) + 1
                envoffsets.append(spaceoffset)
                spaceoffset += len(e.e_value) + 1
            stringspace += struct.pack("!%dH" % len(envoffsets), *envoffsets)
            for e in self.job.bj_env:
                stringspace += e.e_name + '\x00'
                stringspace += e.e_value + '\x00'

        # And now redirections

        if len(self.job.bj_redirs) != 0:
            rediroff = spaceoffset
            spaceoffset += len(self.job.bj_redirs) * 4
            flist = []
            for r in self.job.bj_redirs:
                roff = r.fd2
                if r.action <= btclasses.redir.RD_ACT_PIPEI:
                    roff = spaceoffset
                    spaceoffset += len(r.filename) + 1
                    flist.append(r.filename)
                stringspace += struct.pack("!BBH", r.fd, r.action, roff)
            if len(flist) != 0:
                stringspace += string.join(flist, '\x00') + '\x00'

        # Finally the title and directory

        if len(self.job.bj_direct) != 0:
            diroff = spaceoffset
            spaceoffset += len(self.job.bj_direct) + 1
            stringspace += self.job.bj_direct + '\x00'

        if len(self.job.bj_title) != 0:
            titoff = spaceoffset
            spaceoffset += len(self.job.bj_title) + 1
            stringspace += self.job.bj_title + '\x00'

        spacehdr = jobhnetmsg.jstring_str.pack(\
                len(self.job.bj_redirs),
                len(self.job.bj_arg),
                len(self.job.bj_env),
                titoff,
                diroff,
                rediroff,
                envoff,
                argoff)

        if len(stringspace) != spaceoffset:
            raise msgexception("Space not right")

        self.length = jobhnetmsg.jhdr_str.size + len(spacehdr) + spaceoffset
        header = self.encode()
        return header + spacehdr + stringspace

    def decode_strings(self, msgblock):
        """Decode the string part of a job message"""

        stringspace = msgblock[jobhnetmsg.jstring_str.size:]

        nredirs, narg, nenv, titoff, diroff, rediroff, envoff,argoff = \
            jobhnetmsg.jstring_str.unpack(msgblock[0:jobhnetmsg.jstring_str.size])

        # Unpick arguments

        self.job.bj_arg = []
        if narg > 0:
            argoffs = struct.unpack("!%dH" % narg, stringspace[argoff:argoff+2*narg])
            for ao in argoffs:
                self.job.bj_arg.append(getnulltstring(stringspace, ao))

        # Ditto environment

        self.job.bj_env = []
        if nenv > 0:
            envoffs = list(struct.unpack("!%dH" % (nenv*2), stringspace[envoff:envoff+4*nenv]))
            while len(envoffs) != 0:
                no = envoffs.pop(0)
                eo = envoffs.pop(0)
                self.job.bj_env.append(btclasses.envir(getnulltstring(stringspace, no), getnulltstring(stringspace, eo)))

        # Finally redirections

        self.job.bj_redirs = []
        if nredirs > 0:
            mredirs = list(struct.unpack("!" + "BBH" * nredirs, stringspace[rediroff:rediroff+4*nredirs]))
            while len(mredirs) != 0:
                fd = mredirs.pop(0)
                act = mredirs.pop(0)
                v = mredirs.pop(0)
                if act <= btclasses.redir.RD_ACT_PIPEI:
                    v = getnulltstring(stringspace, v)
                self.job.bj_redirs.append(btclasses.redir(fd, act, v))

        # Next title and directory

        self.job.bj_title = ""
        if titoff >= 0:
            self.job.bj_title = getnulltstring(stringspace, titoff)
        self.job.bj_direct = ""
        if diroff >= 0:
            self.job.bj_direct = getnulltstring(stringspace, diroff)

class rassmsg(msgbase):
    """Remote assignments (do we need this?)"""

    rass_str, rest_rass_str, size = parse_str(MSGHDR_STR + JIDENT_STR + 'BBH')

    def __init__(self, c = 0, jobid = None, s = None, fl = 0, src = 0, st = 0):
        msgbase.__init__(self, s, c, 48)
        msgbase.set_jobid(self, jobid)
        self.flags = fl
        self.source = src
        self.status = st

    def encode(self):
        """Encode message as a string"""
        try:
            return rassmsg.rass_str.pack(self.code,
                                         self.length,
                                         socket.inet_aton(self.hostid),
                                         self.pid,
                                         self.muser,
                                         self.mgroup,
                                         socket.inet_aton(self.jid.ipaddr()),
                                         self.jid.slotno,
                                         self.flags,
                                         self.source,
                                         self.status)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode_rest(self, c, l, msg):
        """Decode all of an incoming job-header only message"""

        self.code = c
        self.length = l

        try:
            hs, self.pid, u, g, \
            hj, sj, \
            self.flags, self.source, self.status = rassmsg.rest_rass_str.unpack(msg)
        except struct.error:
            raise msgexception("Unexpected message format")

        # Sort message header

        self.hostid = socket.inet_ntoa(hs)
        self.muser = string.split(u, '\x00')[0]
        self.mgroup = string.split(g, '\x00')[0]

class jobcmsg(msgbase):
    """Job control message"""

    jobcmsg_str, rest_jobcmsg_str, size = parse_str(MSGHDR_STR + JIDENT_STR + 'L')

    def __init__(self, c = 0, jobid = None, s = None, p = 0):
        msgbase.__init__(self, s, c, 48)
        msgbase.set_jobid(self, jobid)
        self.param = p

    def encode(self):
        """Pack up outgoing message"""
        try:
            return jobcmsg.jobcmsg_str.pack(self.code,
                                                 self.length,
                                                 socket.inet_aton(self.hostid),
                                                 self.pid,
                                                 self.muser,
                                                 self.mgroup,
                                                 socket.inet_aton(self.jid.ipaddr()),
                                                 self.jid.slotno,
                                                 self.param)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode_rest(self, c, l, msg):
        """Decode rest of incoming message"""

        self.code = c
        self.length = l

        try:
            hs, self.pid, u, g, hj, sj, self.param = jobcmsg.rest_jobcmsg_str.unpack(msg)
        except struct.error:
            raise msgexception("Unexpected message format")

        # Sort message header

        self.jid = gbident.gbident(socket.inet_ntoa(hj), sj)
        self.hostid = socket.inet_ntoa(hs)
        self.muser = string.split(u, '\x00')[0]
        self.mgroup = string.split(g, '\x00')[0]

class jobnotmsg(msgbase):
    """Job notify message"""

    jobnot_str, rest_jobnot_str, size = parse_str(MSGHDR_STR + JIDENT_STR + 'hxx2L')

    def __init__(self, c = 0, jobid = None, s = None, msgc = 0, so = 0, se = 0):
        msgbase.__init__(self, s, c, 56)
        msgbase.set_jobid(self, jobid)
        self.msgcode = msgc
        self.sout = so
        self.serr = so

    def encode(self):
        """Pack up for sending"""
        try:
            return jobnotsg.jobnot_str.pack(self.code,
                                            self.length,
                                            socket.inet_aton(self.hostid),
                                            self.pid,
                                            self.muser,
                                            self.mgroup,
                                            socket.inet_aton(self.jid.ipaddr()),
                                            self.jid.slotno,
                                            self.msgcode,
                                            self.sout,
                                            self.serr)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode_rest(self, c, l, msg):
        """Decode rest of incoming message"""

        self.code = c
        self.length = l

        try:
            hs, self.pid, u, g, hj, sj, self.msgcode, self.sout, self.serr = jobnotmsg.rest_jobnot_str.unpack(msg)
        except struct.error:
            raise msgexception("Unexpected message format")

        # Sort message header

        self.hostid = socket.inet_ntoa(hs)
        self.muser = string.split(u, '\x00')[0]
        self.mgroup = string.split(g, '\x00')[0]

class varnetmsg(msgbase):
    """Messages about variables"""

    varmsg_str, rest_varmsg_str, size = parse_str(MSGHDR_STR +
                                                  VIDENT_STR +
                                                  'lBB20s42s' +
                                                  MODE_STR +
                                                  'B3x52s')

    def __init__(self, c = 0, v = None, s = None):
        msgbase.__init__(self, s, c, 36 + 8 + 6 + 20 + 42 + 72 + 58)
        if v is None:
            self.var = btclasses.btvar()
        else:
            self.var = copy.deepcopy(v)

    def encode(self):
        """Pack up for sending"""
        try:
            ctype = btclasses.btconst.CON_STRING
            cvalue = self.var.var_value.value
            if not self.var.var_value.isdef:
                cvalue = ""
            elif self.var.var_value.isint:
                ctype = btclasses.btconst.CON_LONG
                cvalue = struct.pack("!l", cvalue)
            return varnetmsg.varmsg_str.pack(self.code,
                                             self.length,
                                             socket.inet_aton(self.hostid),
                                             self.pid,
                                             self.muser,
                                             self.mgroup,
                                             socket.inet_aton(self.var.ipaddr()),
                                             self.var.slotno,
                                             self.var.var_c_time,
                                             self.var.var_type,
                                             self.var.var_flags,
                                             self.var.var_name,
                                             self.var.var_comment,
                                             self.var.var_mode.o_uid,
                                             self.var.var_mode.o_gid,
                                             self.var.var_mode.c_uid,
                                             self.var.var_mode.c_gid,
                                             self.var.var_mode.o_user,
                                             self.var.var_mode.o_group,
                                             self.var.var_mode.c_user,
                                             self.var.var_mode.c_group,
                                             self.var.var_mode.u_flags,
                                             self.var.var_mode.g_flags,
                                             self.var.var_mode.o_flags,
                                             ctype,
                                             cvalue)
        except socket.error:
            raise msgexception("Invalid IP address")
        except struct.error:
            raise msgexception("Message invalid")

    def decode_rest(self, c, l, msg):
        """Decode all of an incoming variable message"""

        self.code = c
        self.length = l

        try:
            hs, self.pid, u, g, \
            hv, sv, \
            c_time, vtype, vflags, vname, vcomment, \
            o_uid, o_gid, c_uid, c_gid, o_user, o_group, c_user, c_group, \
            u_flags, g_flags, o_flags, \
            ctype, cstring = varnetmsg.rest_varmsg_str.unpack(msg)
        except struct.error:
            raise msgexception("Unexpected message format")

        # Sort message header

        self.hostid = socket.inet_ntoa(hs)
        self.muser = string.split(u, '\x00')[0]
        self.mgroup = string.split(g, '\x00')[0]
        self.var = btclasses.btvar(socket.inet_ntoa(hv) ,sv)
        self.var.var_c_time = c_time
        self.var.var_m_time = int(time.time())
        self.var.var_type = vtype
        self.var.var_flags = vflags
        self.var.var_name = string.split(vname, '\x00')[0]
        self.var.var_comment = string.split(vcomment, '\x00')[0]
        self.var.var_mode.o_uid = o_uid
        self.var.var_mode.o_gid = o_gid
        self.var.var_mode.c_uid = c_uid
        self.var.var_mode.c_gid = c_gid
        self.var.var_mode.o_user = string.split(o_user, '\x00')[0]
        self.var.var_mode.o_group = string.split(o_group, '\x00')[0]
        self.var.var_mode.c_user = string.split(c_user, '\x00')[0]
        self.var.var_mode.c_group = string.split(c_group, '\x00')[0]
        self.var.var_mode.u_flags = u_flags
        self.var.var_mode.g_flags = g_flags
        self.var.var_mode.o_flags = o_flags
        self.var.var_value.isdef = True
        if ctype != btclasses.btconst.CON_STRING:
            self.var.var_value.isint = True
            self.var.var_value.value = struct.unpack('!l48x', cstring)[0]
        else:
            self.var.var_value.isint = False
            self.var.var_value.value = string.split(cstring, '\x00')[0]

class feeder:
    """Structure to send to say we want a file"""

    FEED_JOB    =   0           # Send job script
    FEED_SO     =   1           # Send standard output
    FEED_SE     =   2           # Send standard error

    def __init__(self, job_number = 0, feedtype = FEED_JOB):
        self.fdtype = feedtype
        self.jobno = job_number

    def encode(self):
        """Package to send"""
        return struct.pack("!b3xl", self.fdtype, self.jobno)
