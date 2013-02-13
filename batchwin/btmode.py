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

import btuser
import btqwopts
import xmlutil

class Nopriv(Exception):
    """Throw me if we can't display something"""
    pass

class btmode:

    BTM_READ = 1 << 0
    BTM_WRITE = 1 << 1
    BTM_SHOW = 1 << 2
    BTM_RDMODE = 1 << 3
    BTM_WRMODE = 1 << 4
    BTM_UTAKE = 1 << 5
    BTM_GTAKE = 1 << 6
    BTM_UGIVE = 1 << 7
    BTM_GGIVE = 1 << 8
    BTM_DELETE = 1 << 9
    BTM_KILL = 1 << 10

    modeletters = 'RWSMPVHUGDK'

    """Represent job or variable permission structure"""

    def __init__(self):
        self.o_uid = 0
        self.o_gid = 0
        self.c_uid = 0
        self.c_gid = 0
        self.o_user = ""
        self.o_group = ""
        self.c_user = ""
        self.c_group = ""
        self.u_flags = 0
        self.g_flags = 0
        self.o_flags = 0

    def load(self, node):
        """Load permissions from XML DDM"""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "uperm":
                self.u_flags = int(xmlutil.getText(child))
            elif tagn == "gperm":
                self.g_flags = int(xmlutil.getText(child))
            elif tagn == "operm":
                self.o_flags = int(xmlutil.getText(child))
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save permissions to XML DOM"""
        pm = doc.createElement(name)
        xmlutil.save_xml_string(doc, pm, "uperm", self.u_flags)
        xmlutil.save_xml_string(doc, pm, "gperm", self.g_flags)
        xmlutil.save_xml_string(doc, pm, "operm", self.o_flags)
        pnode.appendChild(pm)

    def modeencode(self, v):
        """Display mode letters"""
        return string.join([btmode.modeletters[i] for i in range(0,len(btmode.modeletters)) if (v & (1<<i))!=0],'')

    def mode_disp(self):
        """Display mode"""
        return 'U:' + self.modeencode(self.u_flags) + ',G:' + self.modeencode(self.g_flags) + ',O:' + self.modeencode(self.o_flags)

    def mpermitted(self, perms, *flgs):
        """See if modes permit the given operation(s)"""
        uf = self.u_flags
        gf = self.g_flags
        of = self.o_flags
        if (perms.btu_priv & btuser.btuser.BTM_ORP_UG) != 0:
            uf |= gf
            gf = uf
        if (perms.btu_priv & btuser.btuser.BTM_ORP_UO) != 0:
            uf |= of
            of = uf
        if (perms.btu_priv & btuser.btuser.BTM_ORP_GO) != 0:
            gf |= of
            of = gf
        flgss = reduce(lambda x,y: x|y, flgs)
        if perms.username == self.o_user:
            return (uf & flgss) == flgss
        if perms.groupname == self.o_group:
            return (gf & flgss) == flgss
        return (of & flgss) == flgss

    def check_accessible(self, obj, flag):
        """For actions/display of object attributes - see if we can do the thing specified"""
        slist = btqwopts.Options.servers
        try:
            serv = slist.serversbyip[obj.ipaddr()]
        except KeyError:
            raise Nopriv
        if not self.mpermitted(serv.perms, flag): raise Nopriv

    def check_readable(self, obj):
        """Specific case of is it readable"""
        self.check_accessible(obj, btmode.BTM_READ)
