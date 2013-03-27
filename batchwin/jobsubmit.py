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

# Stuff to pack up jobs for submission.
# This really needs to be replaced throughout to avoid
# having two nearly identical routines

import struct
import string
import socket
import uaclient
import gbclient
import btclasses
import gbserver
import btqwopts
import reqmess

class SubmError(Exception):
    """For errors during submission process"""
    pass

# Message to tell xbnetserv what to expect
# NB Assumes UIDSIZE=11 (plus 1 null)

ni_jobhdr = struct.Struct("!BxH12s12s")

# Condition structure with long or string

nicond_i = struct.Struct("!20s4s3Bxl48x")
nicond_s = struct.Struct("!20s4s3Bx52s")

# Assignment structure with long or string

niass_i = struct.Struct("!20s4sH3B3xl48x")
niass_s = struct.Struct("!20s4sH3B3x52s")

# Job header

nijobhmsg = struct.Struct("!7Bx5h9HllLL16s4B64x3Hxx800s672s")

def pack_job(job):
    """Pack up job for a network message"""

    # We first need to pack up the strings as we need the length of these

    stringspace = ""    # May need to check it isn't > 3000

    titoff = diroff = rediroff = envoff = argoff = -1

    spaceoffset = 0

    # Set up arguments as a vector of pointers to strings

    if len(job.bj_arg) != 0:
        argoff = spaceoffset
        spaceoffset += len(job.bj_arg) * 2
        argoffsets = []
        for a in job.bj_arg:
            argoffsets.append(spaceoffset)
            spaceoffset += len(a) + 1
        stringspace += struct.pack("!%dH" % len(argoffsets), *argoffsets)
        stringspace += string.join(job.bj_arg, '\x00') + '\x00'

    if len(job.bj_env) != 0:
        envoff = spaceoffset
        spaceoffset += len(job.bj_env) * 4
        envoffsets = []
        for e in job.bj_env:
            envoffsets.append(spaceoffset)
            spaceoffset += len(e.e_name) + 1
            envoffsets.append(spaceoffset)
            spaceoffset += len(e.e_value) + 1
        stringspace += struct.pack("!%dH" % len(envoffsets), *envoffsets)
        for e in job.bj_env:
            stringspace += e.e_name + '\x00'
            stringspace += e.e_value + '\x00'

    # And now redirections

    if len(job.bj_redirs) != 0:
        rediroff = spaceoffset
        spaceoffset += len(job.bj_redirs) * 4
        flist = []
        for r in job.bj_redirs:
            roff = r.fd2
            if r.action <= btclasses.redir.RD_ACT_PIPEI:
                roff = spaceoffset
                spaceoffset += len(r.filename) + 1
                flist.append(r.filename)
            stringspace += struct.pack("!BBH", r.fd, r.action, roff)
        if len(flist) != 0:
            stringspace += string.join(flist, '\x00') + '\x00'

    # Finally the title and directory

    if len(job.bj_direct) != 0:
        diroff = spaceoffset
        spaceoffset += len(job.bj_direct) + 1
        stringspace += job.bj_direct + '\x00'

    if len(job.bj_title) != 0:
        titoff = spaceoffset
        spaceoffset += len(job.bj_title) + 1
        stringspace += job.bj_title + '\x00'

    # Size of message is the header plus the string space actually used now given by "spaceoffset"
    # Let's get the header together.
    # First encode the conditions
    # (Conditions and assignments have variables stored as "host:varname" in btrw)

    condstr = ''

    for c in job.bj_conds:

        # Skip it if we don't have it

        if c.bjc_compar == btclasses.jcond.C_UNUSED or not c.bjc_value.isdef:
            continue

        # Look up variable name, which we are expecting in the format host:varname

        vparts = string.split(c.bjc_varname, ':')
        if len(vparts) != 2: continue

        hostname, varname = vparts
        try:
            # Say we want an exception if we don't know the hostname
            # (This "cannot happen")

            hostid = socket.inet_aton(btqwopts.Options.servers.look_host(hostname, True))

        except (gbserver.ServError, socket.error):
            continue

        # Pack up the condition

        if c.bjc_value.isint:
            condseg = nicond_i.pack(varname, hostid, c.bjc_compar, c.bjc_iscrit, btclasses.btconst.CON_LONG, c.bjc_value.value)
        else:
            condseg = nicond_s.pack(varname, hostid, c.bjc_compar, c.bjc_iscrit, btclasses.btconst.CON_STRING, c.bjc_value.value)

        condstr += condseg

    # Finished with conditions, so we pad it out to the maximum length

    condstr += '\x00' * (800 - len(condstr))

    # Pack up assignments in the same way

    assstr = ''
    for a in job.bj_asses:

        # Skip if we don't have it

        if a.bja_op == btclasses.jass.BJA_NONE or not a.bja_con.isdef:
            continue

        # Look up variable name, which we are expecting in the format host:varname

        vparts = string.split(a.bja_varname, ':')
        if len(vparts) != 2: continue

        hostname, varname = vparts
        try:
            # Say we want an exception if we don't know the hostname
            # (This "cannot happen")

            hostid = socket.inet_aton(btqwopts.Options.servers.look_host(hostname, True))

        except (gbserver.ServError, socket.error):
            continue

        if a.bja_con.isint:
            assseg = niass_i.pack(varname, hostid, a.bja_flags, a.bja_op, a.bja_iscrit, btclasses.btconst.CON_LONG, a.bja_con.value)
        else:
            assseg = niass_s.pack(varname, hostid, a.bja_flags, a.bja_op, a.bja_iscrit, btclasses.btconst.CON_STRING, a.bja_con.value)

        assstr += assseg

    # Finished with assignments, so we pad out to the maximum length

    assstr += '\x00' * (672 - len(assstr))

    # Now pack together the whole caboodle

    mesghdr = nijobhmsg.pack(job.bj_progress, job.bj_pri, job.bj_jflags, job.bj_times.tc_istime,
                             job.bj_times.tc_mday, job.bj_times.tc_repeat, job.bj_times.tc_nposs,
                             titoff, diroff, rediroff, envoff, argoff,
                             job.bj_ll, job.bj_umask, job.bj_times.tc_nvaldays,
                             len(job.bj_redirs), len(job.bj_arg), len(job.bj_env),
                             job.bj_autoksig, job.bj_runon, job.bj_deltime,
                             job.bj_ulimit, job.bj_times.tc_nexttime,
                             job.bj_times.tc_rate, job.bj_runtime,
                             job.bj_cmdinterp,
                             job.exitn.lower, job.exitn.upper, job.exite.lower, job.exite.upper,
                             job.bj_mode.u_flags, job.bj_mode.g_flags, job.bj_mode.o_flags,
                             condstr, assstr)

    return mesghdr + stringspace

def parse_retcode(retcode, param):
    """Raise an exception corresponding to the return and and parameter given"""
    if retcode == uaclient.SV_CL_UNKNOWNC or retcode == uaclient.XBNR_UNKNOWN_CLIENT or retcode == uaclient.XBNR_BAD_JOBDATA:
        raise SubmError("Communcation failure with client - unknown command?")
    # Sometime we might want to fish out the parameter from bad cond or assignment to say which one
    if retcode == uaclient.XBNR_ERR:
        retcode = param
    if retcode < 0 or retcode >= len(uaclient.errorcodes):
        raise SubmError(reqmess.lookupcode(retcode))
    raise SubmError("Job rejected - error was " + uaclient.errorcodes[retcode][1])

def send_jobhdr(serv, msg):
    """This routine sends the job header, suitably packed, to the server"""

    msgleft = msgsize = len(msg)
    hdrsize = ni_jobhdr.size
    code = uaclient.CL_SV_STARTJOB
    user = serv.perms.username
    group = serv.perms.groupname
    uac = serv.uasock

    # This is the biggest bit we can send

    biggest_chunk = uaclient.CL_SV_BUFFSIZE - hdrsize

    while msgleft > 0:

        # This or the data-sending bit is a mistake, but we put the whole
        # message size in the header each time

        hdrmsg = ni_jobhdr.pack(code, msgsize, user, group)
        code = uaclient.CL_SV_CONTJOB # For next time

        # Only send up to CL_SV_BUFFSIZE at a time including header

        segsize = msgleft
        if segsize > biggest_chunk: segsize = biggest_chunk

        # Send the chunk across and chop that off the message

        uac.sockfd.sendto(hdrmsg + msg[:segsize], uac.hostadd)
        msg = msg[segsize:]
        msgleft -= segsize

        try:
            uac.waitreply()
        except uaclient.uaclientException as err:
            raise SubmError("Timeout problem (jobhdr) from server on submit")

        indata = uac.sockfd.recv(uaclient.CL_SV_BUFFSIZE)

        if len(indata) != uaclient.Client_if.size:
            raise SubmError("Unexpected reply size from server on submit")

        retcode, param = uaclient.Client_if.unpack(indata)

        if retcode != uaclient.SV_CL_ACK:
            parse_retcode(retcode, param)

def send_jobdata(serv, data):
    """Wend script of job to the server"""

    user = serv.perms.username
    group = serv.perms.groupname
    uac = serv.uasock

    # This is the biggest bit we can send

    biggest_chunk = uaclient.CL_SV_BUFFSIZE - ni_jobhdr.size

    while len(data) > 0:

        # This is different from sending the job header in that we put the length left (or the max
        # message size if shorter in the header each time

        msgsize = len(data)
        if  msgsize > biggest_chunk: msgsize = biggest_chunk

        hdrmsg = ni_jobhdr.pack(uaclient.CL_SV_JOBDATA, msgsize, user, group)
        uac.sockfd.sendto(hdrmsg + data[:msgsize], uac.hostadd)
        data = data[msgsize:]

        try:
            uac.waitreply()
        except uaclient.uaclientException as err:
            raise SubmError("Timeout problem (jobdata) from server on submit")

        indata = uac.sockfd.recv(uaclient.CL_SV_BUFFSIZE)

        if len(indata) != uaclient.Client_if.size:
            raise SubmError("Unexpected reply size from server on submit")

        retcode, param = uaclient.Client_if.unpack(indata)

        if retcode != uaclient.SV_CL_ACK:
           parse_retcode(retcode, param)

def send_endjob(serv):
    """Send job complete code and if all goes OK return job number"""

    uac = serv.uasock
    uac.sockfd.sendto(ni_jobhdr.pack(uaclient.CL_SV_ENDJOB, ni_jobhdr.size, serv.perms.username, serv.perms.groupname), uac.hostadd)
    try:
        uac.waitreply(200.0)
    except uaclient.uaclientException as err:
        raise SubmError("Timeout problem (endjob) from server on submit")

    indata = uac.sockfd.recv(uaclient.CL_SV_BUFFSIZE)

    if len(indata) != uaclient.Client_if.size:
        raise SubmError("Unexpected reply size from server on submit")

    retcode, param = uaclient.Client_if.unpack(indata)

    if retcode != uaclient.SV_CL_ACK:
           parse_retcode(retcode, param)

    return param       # Job number

def sock_write(sock, data):
    """Push data out to TCP socket"""
    while len(data) > 0:
        nbytes = sock.send(data)
        if nbytes <= 0:
            raise SubmError("Error sending to TCP socket, cannot write")
        data = data[nbytes:]

def sock_read(sock, length):
    """Read data from socket up to given length"""
    res = ""
    while length > 0:
        buf = sock.recv(length)
        nb = len(buf)
        if nb <= 0:
            raise SubmError("Error reading on TCP socket")
        length -= nb
        res += buf
    return res

def job_submit(serv, job):
    """Submit job given to server, which is all logged in"""

    if btqwopts.WIN:
        send_jobhdr(serv, pack_job(job))
        send_jobdata(serv, job.script)
        return send_endjob(serv)

    # Use TCP when this program isn't running on Windows

    tsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        tsocket.connect(serv.uasock.hostadd)
        jobmsg = pack_job(job)
        jobhdr = ni_jobhdr.pack(uaclient.CL_SV_STARTJOB, len(jobmsg), serv.perms.username, serv.perms.groupname)
        sock_write(tsocket, jobhdr + jobmsg)
        jobhdr = ni_jobhdr.pack(uaclient.CL_SV_JOBDATA, len(job.script), serv.perms.username, serv.perms.groupname)
        sock_write(tsocket, jobhdr + job.script)
        jobhdr = ni_jobhdr.pack(uaclient.CL_SV_ENDJOB, len(job.script), serv.perms.username, serv.perms.groupname)
        sock_write(tsocket, jobhdr)
        retmsg = sock_read(tsocket, uaclient.Client_if.size)
        retcode, param = uaclient.Client_if.unpack(retmsg)
        if retcode != uaclient.SV_CL_ACK:
           parse_retcode(retcode, param)
        return param
    except socket.error as err:
        tsocket.close()
        raise SubmError("Cannot connect server - " + err.args[0])
    finally:
        tsocket.close()



