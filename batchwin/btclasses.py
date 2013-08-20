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
import socket
import time
import copy
import gbident
import btqwopts
import timecon
import btmode
import xmlutil
import globlists

class btconst:
    """Represent constant"""

    CON_LONG = 1
    CON_STRING = 2

    def __init__(self, val = None):
        if val is None:
            self.isdef = False
            self.value = 0
        else:
            self.isdef = True
            self.value = val                    # NB string limited to 49 chars
            self.isint = isinstance(val, int)

    def __str__(self):
        """Get ourselves a string version of the constant"""
        if not self.isint:
            if len(self.value) > 0 and '0' <= self.value[0] <= '9':
                return '"' + self.value + '"'
            return self.value
        return str(self.value)

    def intvalue(self):
        """Get current value as an int"""
        if self.isint: return self.value
        return 0

    def load(self, node):
        """Load constant value from XML DOM"""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "intval":
                self.isdef = True
                self.isint = True
                self.value = int(xmlutil.getText(child))
            elif tagn == "textval":
                self.isdef = True
                self.isint = False
                self.value = xmlutil.getText(child)
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save constant value to XML DOM"""
        if not self.isdef: return
        cn = doc.createElement(name)
        if self.isint:
            xmlutil.save_xml_string(doc, cn, "intval", str(self.value))
        else:
            xmlutil.save_xml_string(doc, cn, "textval", self.value)
        pnode.appendChild(cn)

def gethostvarname(node):
    """Extract variable name as host:variable name.

We may vary the format to have host and name separately in some cases"""
    child = node.firstChild()
    if  child.isText():
        return  str(child.toText().data())
    hostn = vname = ""
    while not child.isNull():
        tagn = child.toElement().tagName()
        if tagn == "host":
            hostn = xmlutil.getText(child)
        elif tagn == "name":
            vname = xmlutil.getText(child)
        child = child.nextSibling()
    if len(hostn) == 0: return vname
    return hostn + ':' + vname

def savehostvarname(doc, node, name, vname):
    """Save variable name as children with host and variable name"""
    vn = doc.createElement(name)
    parts = vname.split(':', 1)
    if len(parts) > 1:
        xmlutil.save_xml_string(doc, vn, "host", parts.pop(0))
    xmlutil.save_xml_string(doc, vn, "name", parts.pop())
    node.appendChild(vn)

class jcond:
    """Represent condition"""
    C_UNUSED = 0
    C_EQ = 1
    C_NE = 2
    C_LT = 3
    C_LE = 4
    C_GT = 5
    C_GE = 6

    CCRIT_NORUN = 0x01          # Do not run if remote unavailable
    CCRIT_NONAVAIL = 0x40       # Local var not avail
    CCRIT_NOPERM = 0x80         # Local var not avail

    def __init__(self):
        self.bjc_compar = jcond.C_UNUSED
        self.bjc_iscrit = 0
        self.bjc_varind = gbident.gbident()
        self.bjc_varname = None                 # Use this saving/loading to file
        self.bjc_value = btconst()

    def isvalid(self):
        """Indicate whether condition is valid"""
        if self.bjc_compar == jcond.C_UNUSED: return False
        if self.bjc_varind.isvalid(): return True
        if self.bjc_varname is None or len(self.bjc_varname) == 0: return False
        return True

    def toname(self):
        """Generate name-style reference if id"""
        if self.bjc_varname is None:
            # Need to fetch the whole thing in case just an ident without the name and such
            var = globlists.Var_list[self.bjc_varind]
            return "%s:%s" % (btqwopts.Options.servers.look_hostid(var.ipaddr()), var.var_name)
        return self.bjc_varname

    #def toident(self):
    #    """Generate ident-style reference if name"""
    #    if not self.bjc_varind.isvalid():
    #       try:
    #            host, name = string.split(self.bjc_varname, ':')
    #           hostid = btqwopts.servers.look_host(host)
    #           vid = globlists.lookup_var(hostid, name)
    #          if vid is None: return None
    #         self.bjc_varind = vid
    #         return vid
    #    except (ValueError, KeyError):
    #        return None
    # return self.bjc_varind

    def load(self, node):
        """Load condition from XML DOM"""
        self.bjc_compar = int(str(node.toElement().attribute("type", "0")))
        self.bjc_iscrit = False
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "vname":
                self.bjc_varname = gethostvarname(child)
            elif tagn == "value":
                self.bjc_value.load(child)
            elif tagn == "iscrit":
                self.bjc_iscrit = True
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save condition to XML DOM"""
        if not self.isvalid(): return
        cn = doc.createElement(name)
        cn.setAttribute("type", str(self.bjc_compar))
        xmlutil.save_xml_bool(doc, cn, "iscrit", self.bjc_iscrit)
        savehostvarname(doc, cn, "vname", self.toname())
        self.bjc_value.save(doc, cn, "value")
        pnode.appendChild(cn)

class jass:
    """Represent assignment"""

    BJA_START = 0x1
    BJA_OK = 0x2
    BJA_ERROR = 0x4
    BJA_ABORT = 0x8
    BJA_CANCEL = 0x10
    BJA_REVERSE = 0x1000

    EXIT_FLAGS = BJA_OK|BJA_ERROR|BJA_ABORT

    BJA_NONE = 0
    BJA_ASSIGN = 1
    BJA_INCR = 2
    BJA_DECR = 3
    BJA_MULT = 4
    BJA_DIV = 5
    BJA_MOD = 6
    BJA_SEXIT = 7
    BJA_SSIG = 8

    ACRIT_NORUN = 0x01
    ACRIT_NONAVAIL = 0x40
    ACRIT_NOPERM = 0x80

    def __init__(self):
        self.bja_flags = 0
        self.bja_op = jass.BJA_NONE
        self.bja_iscrit = 0
        self.bja_varind = gbident.gbident()
        self.bja_varname = None
        self.bja_con = btconst()

    def isvalid(self):
        """Indicate whether condition is valid"""
        if self.bja_op == jass.BJA_NONE: return False
        if self.bja_varind.isvalid(): return True
        if self.bja_varname is None or len(self.bja_varname) == 0: return False
        return True

    def toname(self):
        """Generate name-style reference if id"""
        if self.bja_varname is None:
            # Need to fetch the whole thing in case just an ident without the name and such
            var = globlists.Var_list[self.bja_varind]
            return "%s:%s" % (btqwopts.Options.servers.look_hostid(var.ipaddr()), var.var_name)
        return self.bja_varname

    def load(self, node):
        """Load assignment from XML DOM"""
        self.bja_op = int(str(node.toElement().attribute("type", "0")))
        self.bja_iscrit = False
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "vname":
                self.bja_varname = gethostvarname(child)
            elif tagn == "const":
                self.bja_con.load(child)
            elif tagn == "iscrit":
                self.bja_iscrit = True
            elif tagn == "flags":
                self.bja_flags = int(xmlutil.getText(child))
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save assignment to XML DOM"""
        if not self.isvalid(): return
        an = doc.createElement(name)
        an.setAttribute("type", str(self.bja_op))
        xmlutil.save_xml_bool(doc, an, "iscrit", self.bja_iscrit)
        savehostvarname(doc, an, "vname", self.toname())
        if self.bja_op < jass.BJA_SEXIT:
            self.bja_con.save(doc, an, "const")
            xmlutil.save_xml_string(doc, an, "flags", self.bja_flags)
        pnode.appendChild(an)

class envir:
    """Environment variable"""

    def __init__(self, n = "", v = ""):
        self.e_name = n
        self.e_value = v

    def load(self, node):
        """Load env var from XML DOM"""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "name":
                self.e_name = xmlutil.getText(child)
            elif tagn == "value":
                self.e_value = xmlutil.getText(child)
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save env var to XML DOM"""
        ev = doc.createElement(name)
        xmlutil.save_xml_string(doc, ev, "name", self.e_name)
        xmlutil.save_xml_string(doc, ev, "value", self.e_value)
        pnode.appendChild(ev)

class redir:
    """Redirection"""

    RD_ACT_RD       = 1               # Open for reading
    RD_ACT_WRT      = 2               # Open/create for writing
    RD_ACT_APPEND   = 3               # Open/create and append write only
    RD_ACT_RDWR     = 4               # Open/create for read/write
    RD_ACT_RDWRAPP  = 5               # Open/create/read/write/append
    RD_ACT_PIPEO    = 6               # Pipe out
    RD_ACT_PIPEI    = 7               # Pipe in
    RD_ACT_CLOSE    = 8               # Close it (no file)
    RD_ACT_DUP      = 9               # Duplicate file descriptor given

    def __init__(self, f = 0, a = 0, v = ""):
        self.fd = f
        self.action = a
        if isinstance(v, str):
            self.fd2 = 0
            self.filename = v
        else:
            self.fd2 = v
            self.filename = ""

    def load(self, node):
        """Load redirection from XML DOM"""
        self.action = int(str(node.toElement().attribute("type", "0")))
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "fd":
                self.fd = int(xmlutil.getText(child))
            elif tagn == "file":
                self.filename = xmlutil.getText(child)
            elif tagn == "fd2":
                self.fd2 = int(xmlutil.getText(child))
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save redirection to XML DOM"""
        rd = doc.createElement(name)
        rd.setAttribute("type", str(self.action))
        xmlutil.save_xml_string(doc, rd, "fd", self.fd)
        if self.action < redir.RD_ACT_CLOSE:
            xmlutil.save_xml_string(doc, rd, "file", self.filename)
        elif self.action == rdeir.RD_ACT_DUP:
            xmlutil.save_xml_string(doc, rd, "fd2", self.fd2)
        pnode.appendChild(rd)

class exits:
    """Exit code ranges"""

    def __init__(self, f = 0, l = 0):
        self.lower = f
        self.upper = l

    def load(self, node):
        """Load exit codes from XML DOM"""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "l":
                self.lower = int(xmlutil.getText(child))
            elif tagn == "u":
                self.upper = int(xmlutil.getText(child))
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save exit codes to XML DOM"""
        ex = doc.createElement(name)
        xmlutil.save_xml_string(doc, ex, "l", self.lower)
        xmlutil.save_xml_string(doc, ex, "u", self.upper)
        pnode.appendChild(ex)

class btjob(gbident.gbident):
    """Representation of job structure"""

    BJP_NONE    =   0                   # Nothing done yet
    BJP_DONE    =   1                   # Done once ok
    BJP_ERROR   =   2                   # Done but gave error
    BJP_ABORTED =   3                   # Done but aborted by oper
    BJP_CANCELLED=  4                   # Cancelled before it could run
    BJP_STARTUP1 =  5                   # Currently starting - phase 1
    BJP_STARTUP2 =  6                   # Currently starting - phase 2
    BJP_RUNNING =   7                   # Currently running
    BJP_FINISHED=   8                   # Finished ok

    # Flags for bj_jflags

    BJ_WRT      =   (1 << 0)            # Send message to users terminal
    BJ_MAIL     =   (1 << 1)            # Mail message to user
    BJ_NOADVIFERR = (1 << 3)            # No advance time if error #
    BJ_EXPORT   =   (1 << 4)            # Job is visible from outside world
    BJ_REMRUNNABLE= (1 << 5)            # Job is runnable from outside world
    BJ_CLIENTJOB =  (1 << 6)            # From client host
    BJ_ROAMUSER =   (1 << 7)            # Roaming user

    # Flags for bj_jrunflags

    BJ_PROPOSED =   (1 << 0)            # Remote job proposed - inhibits further propose
    BJ_SKELHOLD =   (1 << 1)            # Job depends on unaccessible remote var
    BJ_AUTOKILLED=  (1 << 2)            # Initial kill applied
    BJ_AUTOMURDER=  (1 << 3)            # Final kill applied
    BJ_HOSTDIED =   (1 << 4)            # Murdered because host died
    BJ_FORCE    =   (1 << 5)            # Force job to run NB moved from bj_jflags
    BJ_FORCENA  =   (1 << 6)            # Do not advance time on Force job to run
    BJ_PENDKILL =   (1 << 7)            # Pending kill

    def __init__(self, netid = "", sl = 0):
        gbident.gbident.__init__(self, netid, sl)
        self.bj_job = 0
        self.bj_time = 0                # When originally submitted (UNIX time)
        self.bj_stime = 0               # Time started
        self.bj_etime = 0               # Last end time
        self.bj_pid = 0                 # Process id if running
        self.bj_orighostid = "0.0.0.0"  # Original hostid (for remotely submitted jobs)
        self.bj_runhostid = "0.0.0.0"   # Host id job running on (might be different)

        self.bj_progress = 0            # Job progress code as above
        self.bj_pri = 150               # Priority
        self.bj_ll = 1000               # Load level
        self.bj_umask = 0               # Copy of umask
        self.bj_ulimit = 0

        self.bj_jflags = 0              # Job Flags see above
        self.bj_jrunflags = 0           # Run flags see above

        self.bj_title = ""              # Name of job
        self.bj_direct = "/tmp"            # Directory

        self.bj_runtime = 0             # Run limit (secs)
        self.bj_autoksig = 0            # Auto kill sig before size 9s applied */
        self.bj_runon = 0               # Grace period (secs)
        self.bj_deltime = 0             # Delete job automatically

        self.bj_cmdinterp = "sh"        # Command interpreter
        self.bj_mode = btmode.btmode()  # Permissions
        self.bj_conds = []              # Conditions
        self.bj_asses = []              # Assignments
        self.bj_times = timecon.timecon()       # Time control

        self.bj_redirs = []             # Redirections
        self.bj_env = []                # Environment
        self.bj_arg = []                # Arguments

        self.bj_lastexit = 0            # Last exit status
        self.exitn = exits(0,0)         # Normal exit code
        self.exite = exits(1,255)       # Error exit code

        self.queuetime = time.time()    # To aid sorting
        self.isvisible = True

    def clonefields(self, clone):
        """Deep copy the fields only applies doing btrw"""
        self.bj_progress = clone.bj_progress
        self.bj_pri = clone.bj_pri
        self.bj_ll = clone.bj_ll
        self.bj_umask = clone.bj_umask
        self.bj_ulimit = clone.bj_ulimit
        self.bj_jflags = clone.bj_jflags
        self.bj_title = clone.bj_title
        self.bj_direct = clone.bj_direct
        self.bj_runtime = clone.bj_runtime
        self.bj_autoksig = clone.bj_autoksig
        self.bj_runon = clone.bj_runon
        self.bj_deltime = clone.bj_deltime
        self.bj_cmdinterp = clone.bj_cmdinterp
        self.bj_mode = copy.deepcopy(clone.bj_mode)
        self.bj_conds = copy.deepcopy(clone.bj_conds)
        self.bj_asses = copy.deepcopy(clone.bj_asses)
        self.bj_times = copy.deepcopy(clone.bj_times)
        self.bj_redirs = copy.deepcopy(clone.bj_redirs)
        self.bj_env = copy.deepcopy(clone.bj_env)
        self.bj_arg = copy.deepcopy(clone.bj_arg)
        self.exitn = copy.deepcopy(clone.exitn)
        self.exite = copy.deepcopy(clone.exite)

    def copy_headers(self, srcj):
        """Copy the fixed parts of a job structure"""
        self.bj_time = srcj.bj_time
        self.bj_stime = srcj.bj_stime
        self.bj_etime = srcj.bj_etime
        self.bj_times = srcj.bj_times
        self.bj_runhostid = srcj.bj_runhostid
        self.bj_progress = srcj.bj_progress
        self.bj_pri = srcj.bj_pri
        self.bj_jflags = srcj.bj_jflags
        self.bj_cmdinterp = srcj.bj_cmdinterp
        self.bj_pid = srcj.bj_pid
        self.bj_lastexit = srcj.bj_lastexit
        self.bj_ll = srcj.bj_ll
        self.bj_umask = srcj.bj_umask
        self.bj_ulimit = srcj.bj_ulimit
        self.bj_deltime = srcj.bj_deltime
        self.bj_runtime = srcj.bj_runtime
        self.bj_autoksig = srcj.bj_autoksig
        self.bj_runon = srcj.bj_runon
        self.bj_mode = srcj.bj_mode
        self.bj_conds = srcj.bj_conds
        self.bj_asses = srcj.bj_asses
        self.exitn = srcj.exitn
        self.exite = srcj.exite

    def copy_strings(self, srcj):
        """Copy the string parts of a job structure"""
        self.bj_title = srcj.bj_title
        self.bj_direct = srcj.bj_direct
        self.bj_arg = srcj.bj_arg
        self.bj_env = srcj.bj_env
        self.bj_redirs = srcj.bj_redirs

    def queue_title(self):
        """Split job title field into separate pieces and return pair"""
        bits = self.bj_title.split(':', 1)
        if len(bits) == 2: return bits
        return ('', bits[0])

    def queueof(self):
        """Get the queue name of the job"""
        return self.queue_title()[0]

    def titleof(self, dispq = ""):
        """Return the title of the job stripping off the queue name if it matches the arg"""
        if len(dispq) == 0:
            return self.bj_title
        q, tit = self.queue_title()
        if len(q) == 0 or q == dispq: return tit
        return self.bj_title

    def set_title(self, qnam, titnam):
        """Set title from queue name and title"""
        if len(qnam) == 0:
            self.bj_title = titnam
        else:
            self.bj_title = qnam + ':' + titnam

    def load(self, node):
        """Load job from XML DOM"""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "progress":
                self.bj_progress = int(xmlutil.getText(child))
            elif tagn == "pri":
                self.bj_pri = int(xmlutil.getText(child))
            elif tagn == "ll":
                self.bj_ll = int(xmlutil.getText(child))
            elif tagn == "umask":
                self.bj_umask = int(xmlutil.getText(child))
            elif tagn == "ulimit":
                self.bj_ulimit = int(xmlutil.getText(child))
            elif tagn == "jflags":
                self.bj_jflags = int(xmlutil.getText(child))
            elif tagn == "title":
                self.bj_title = xmlutil.getText(child)
            elif tagn == "direct":
                self.bj_direct = xmlutil.getText(child)
            elif tagn == "runtime":
                self.bj_runtime = int(xmlutil.getText(child))
            elif tagn == "autoksig":
                self.bj_autoksig = int(xmlutil.getText(child))
            elif tagn == "runon":
                self.bj_runon = int(xmlutil.getText(child))
            elif tagn == "deltime":
                self.bj_deltime = int(xmlutil.getText(child))
            elif tagn == "cmdinterp":
                self.bj_cmdinterp = xmlutil.getText(child)
            elif tagn == "jmode":
                self.bj_mode.load(child)
            elif tagn == "times":
                self.bj_times.load(child)
            elif tagn == "conds":
                gc = child.firstChild()
                self.bj_conds = []
                while not gc.isNull():
                    if gc.toElement().tagName() == "cond":
                        nc = jcond()
                        nc.load(gc)
                        self.bj_conds.append(nc)
                    gc = gc.nextSibling()
            elif tagn == "asses":
                gc = child.firstChild()
                self.bj_asses = []
                while not gc.isNull():
                    if gc.toElement().tagName() == "ass":
                        na = jass()
                        na.load(gc)
                        self.bj_asses.append(na)
                    gc = gc.nextSibling()
            elif tagn == "args":
                gc = child.firstChild()
                self.bj_arg = []
                while not gc.isNull():
                    if gc.toElement().tagName() == "arg":
                        self.bj_arg.append(xmlutil.getText(gc))
                    gc = gc.nextSibling()
            elif tagn == "envs":
                gc = child.firstChild()
                self.bj_env = []
                while not gc.isNull():
                    if gc.toElement().tagName() == "env":
                        ne = envir()
                        ne.load(gc)
                        self.bj_env.append(ne)
                    gc = gc.nextSibling()
            elif tagn == "redirs":
                gc = child.firstChild()
                self.bj_redirs = []
                while not gc.isNull():
                    if gc.toElement().tagName() == "redir":
                        nr = redir()
                        nr.load(gc)
                        self.bj_redirs.append(nr)
                    gc = gc.nextSibling()
            elif tagn == "nexit":
                self.exitn.load(child)
            elif tagn == "eexit":
                self.exite.load(child)
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save job to XML DOM"""
        jobnode = doc.createElement(name)
        xmlutil.save_xml_string(doc, jobnode, "progress", self.bj_progress)
        xmlutil.save_xml_string(doc, jobnode, "pri", self.bj_pri)
        xmlutil.save_xml_string(doc, jobnode, "ll", self.bj_ll)
        xmlutil.save_xml_string(doc, jobnode, "umask", self.bj_umask)
        if self.bj_ulimit != 0:
            xmlutil.save_xml_string(doc, jobnode, "ulimit", self.bj_ulimit)
        xmlutil.save_xml_string(doc, jobnode, "jflags", self.bj_jflags)
        if len(self.bj_title) != 0:
            xmlutil.save_xml_string(doc, jobnode, "title", self.bj_title)
        xmlutil.save_xml_string(doc, jobnode, "direct", self.bj_direct)
        if self.bj_runtime != 0:
            xmlutil.save_xml_string(doc, jobnode, "runtime", self.bj_runtime)
            if self.bj_autoksig !=0 and self.bj_runon != 0:
                xmlutil.save_xml_string(doc, jobnode, "autoksig", self.bj_autoksig)
                xmlutil.save_xml_string(doc, jobnode, "runon", self.bj_runon)
        if self.bj_deltime != 0:
            xmlutil.save_xml_string(doc, jobnode, "deltime", self.bj_deltime)
        xmlutil.save_xml_string(doc, jobnode, "cmdinterp", self.bj_cmdinterp)
        self.bj_mode.save(doc, jobnode, "jmode")
        self.bj_times.save(doc, jobnode, "times")
        if len(self.bj_conds) != 0:
            cl = doc.createElement("conds")
            jobnode.appendChild(cl)
            for c in self.bj_conds:
                c.save(doc, cl, "cond")
        if len(self.bj_asses) != 0:
            al = doc.createElement("asses")
            jobnode.appendChild(al)
            for a in self.bj_asses:
                a.save(doc, al, "ass")
        if len(self.bj_arg) != 0:
            al = doc.createElement("args")
            jobnode.appendChild(al)
            for a in self.bj_arg:
                xmlutil.save_xml_string(doc, al, "arg", a)
        if len(self.bj_env) != 0:
            el = doc.createElement("envs")
            jobnode.appendChild(el)
            for e in self.bj_env:
                e.save(doc, el, "env")
        if len(self.bj_redirs) != 0:
            rl = doc.createElement("redirs")
            jobnode.appendChild(rl)
            for r in self.bj_redirs:
                r.save(doc, rl, "redir")
        self.exitn.save(doc, jobnode, "nexit")
        self.exite.save(doc, jobnode, "eexit")
        pnode.appendChild(jobnode)

class btvar(gbident.gbident):
    """Representation of variable"""

    VT_LOADLEVEL    =   1               # Maximum Load Level
    VT_CURRLOAD     =   2               # Current load level
    VT_LOGJOBS      =   3               # Log jobs
    VT_LOGVARS      =   4               # Log vars
    VT_MACHNAME     =   5               # Machine name
    VT_STARTLIM     =   6               # Max number of jobs to start at once
    VT_STARTWAIT    =   7               # Wait time

    VF_READONLY     =   0x01
    VF_STRINGONLY   =   0x02
    VF_LONGONLY     =   0x04
    VF_EXPORT       =   0x08            # Visible to outside world
    VF_SKELETON     =   0x10            # Skeleton variable for offline host
    VF_CLUSTER      =   0x20            # Local to machine in conditions/assignments

    MACH_SLOT       =   -10             # Marker for "machine name" slot

    def __init__(self, netid = "", sl = 0):
        gbident.gbident.__init__(self, netid, sl)
        self.var_sequence = 0
        self.var_c_time = 0
        self.var_m_time = 0
        self.var_type = 0
        self.var_flags = 0
        self.var_name = ""              # Max length 19 + null
        self.var_comment = ""           # Max length 41 + null
        self.var_mode = btmode.btmode()
        self.var_value = btconst()
        self.isvisible = True

    def load(self, node):
        """Load variable from XML DOM"""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "name":
                self.var_name = xmlutil.getText(child)
            elif tagn == "comment":
                self.var_comment = xmlutil.getText(child)
            elif tagn == "value":
                self.var_value.load(child)
            elif tagn == "vmode":
                self.var_mode.load(child)
            elif tagn == "type":
                self.var_type = int(xmlutil.getText(child))
            elif tagn == "flags":
                self.var_flags = int(xmutil.getText(child))
            child = child.nextSibling();

    def save(self, doc, pnode, name):
        """Save variable to XML DOM"""
        varnode = doc.createElement(name)
        xmlutil.save_xml_string(doc, varnode, "name", self.var_name)
        xmlutil.save_xml_string(doc, varnode, "comment", self.var_comment)
        if self.var_type != 0:
            xmlutil.save_xml_string(doc, varnode, "type", self.var_type)
        if self.var_flags != 0:
            xmlutil.save_xml_string(doc, varnode, "flags", self.var_flags)
        self.var_value.save(doc, varnode, "value")
        self.var_mode.save(doc, varnode, "vmode")
        pnode.appendChild(varnode)