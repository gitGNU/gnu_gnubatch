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
import select
import re
import gbnetid
import xmlutil
import uaclient
import btclasses
import netmsg
import btuser
import btqwopts
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import ui_login

class ServError(Exception):

    def __init__(self, msg, fatal = True):
        Exception.__init__(self, msg, fatal)

class Clogindlg(QDialog, ui_login.Ui_logindlg):

    def __init__(self, parent = None):
        super(Clogindlg, self).__init__(parent)
        self.setupUi(self)

class gbserver:
    """Represent details of server we speak to"""

    def __init__(self):
        self.servip = gbnetid.gbnetid()
        self.servname = ""
        self.alias = ""
        self.namebyip = False
        self.connected = False
        self.syncreq = False
        self.synccomplete = False
        self.perms = btuser.btuserenv()
        self.uasock = uaclient.uaclient()
        self.autoconn = False
        self.isok = False

    def create(self, host, alias, ac = False):
        """Create details from host and alias names"""
        try:
            h = host[0]
        except (IndexError, TypeError):
            raise ServError("Null host name")
        if '0' <= h <= '9':
            if len(alias) == 0:
                raise ServError("No alias given with IP-based host")
            try:
                n = socket.inet_aton(host)
            except socket.error:
                raise ServError("Invalid IP address")
            self.servname = ""
            self.namebyip = True
            n = socket.inet_ntoa(n)
        else:
            try:
                n = socket.gethostbyname(host)
            except socket.gaierror as e:
                raise ServError("Host name " + host + " - " + e.argb[1])
            self.servname = host
            self.namebyip = False
        self.alias = alias
        self.servip = gbnetid.gbnetid(n)
        self.autoconn = ac

    def get_servname(self):
        """Return machine name from alias or name"""
        if len(self.alias) > 0:
            return self.alias
        if len(self.servname) > 0:
            return self.servname
        return self.servip.ipaddr()

    def get_servaddr(self):
        """Return netid from alias or name"""
        return self.servip;

    def load(self, node):
        """Load state from XML file"""
        child = node.firstChild()
        if child.toElement().tagName() == "ip":
            self.servname = ""
            self.namebyip = True
            self.servip = gbnetid.gbnetid(xmlutil.getText(child))
        else:
            self.servname = xmlutil.getText(child)
            self.namebyip = False
            try:
                self.servip = gbnetid.gbnetid(socket.gethostbyname(self.servname))
            except socket.gaierror:
                self.servip = gbnetid.gbnetid()
        el = node.toElement()
        if el.hasAttribute("alias"):
            self.alias = str(el.attribute("alias"))
        self.autoconn = False
        if el.hasAttribute("autoconn") and el.attribute("autoconn") == "y":
            self.autoconn = True

    def save(self, doc, parent, name, myip):
        """Save state to XML file"""
        saves = doc.createElement(name)
        parent.appendChild(saves)
        if self.namebyip:
            xmlutil.save_xml_string(doc, saves, "ip", self.servip.ipaddr())
        else:
            xmlutil.save_xml_string(doc, saves, "name", self.servname)
        if len(self.alias) > 0:
            saves.setAttribute("alias", self.alias)
        if self.autoconn:
            saves.setAttribute("autoconn", "y")

    def getperms_nopw(self):
        """Get my permissions and user name on server trying first without windows name/password"""
        self.uasock.setup(self.servip.ipaddr(), btqwopts.Options.ports.client_access)
        try:
            self.uasock.uaenquire()
            self.perms = self.uasock.getbtuser()
            self.isok = True
            return True
        except uaclient.uaclientException as uace:
            if not uace.retryable:
                raise
            return False

    def getperms_pw(self, w, wuser, wpasswd, machine, tried):
        """Get my permissions and user name on server using name and password"""
        try:
            self.uasock.ualogin(wuser, wpasswd, machine)
            self.perms = self.uasock.getbtuser()
            self.isok = True
            return True
        except uaclient.uaclientException as uace:
            if tried or not uace.retryable:
                QMessageBox.warning(w, "Login error on " + self.get_servname(), uace.args[0])
            self.isok = False
            if not uace.retryable:
                raise
            return False

class servlist():
    """Represent list of servers and options"""

    def __init__(self, wuser):
        self.serversanyname = dict()
        self.serversbyname = dict()
        self.serversbyip = dict()
        self.namecache = dict()
        self.ipcache = dict()
        self.myname = socket.gethostname()
        self.myhostid = socket.gethostbyname(self.myname)
        self.winuname = wuser
        self.password = ""
        self.lahost = "www.google.com"
        self.laport = 80

    def get_locaddr(self, h = None, p = None):
        """Get local address from specified host and port"""
        host = self.lahost
        port = self.laport
        if h is not None: host = h
        if p is not None: port = p
        sk = socket.socket()
        try:
            sk.connect((host, port))
            gbnetid.myhostid = self.myhostid = sk.getsockname()[0]
            self.lahost = host
            self.laport = port
        except (socket.gaierror, socket.error):
            raise  ServError("Cannot get local address")
        finally:
            sk.close()

    def look_host(self, name, excpt = False):
        """Look up host name"""
        try:
            return self.serversanyname[name].get_servaddr().ipaddr()
        except KeyError:
            pass
        try:
            return self.namecache[name]
        except KeyError:
            pass
        try:
            ip = socket.gethostbyname(name)
            self.namecache[name] = ip
            return ip
        except socket.gaierror:
            if excpt:
                raise ServError("Unknown host name - " + name)
            return '0.0.0.0'

    def look_hostid(self, ipaddr):
        """Look up IP address"""
        try:
            return self.serversbyip[ipaddr].get_servname()
        except KeyError:
            pass
        try:
            return self.ipcache[ipaddr]
        except KeyError:
            pass
        try:
            namet = socket.gethostbyaddr(ipaddr)
            al = namet[1]
            al.append(namet[0])
            al.sort(lambda x,y: len(x)-len(y))
            self.ipcache[ipaddr] = al[0]
            return al[0]
        except (socket.gaierror, TypeError, KeyError, IndexError):
            return "Unknown"

    def add(self, srv):
        """Add server to list"""
        self.serversbyip[srv.get_servaddr().ipaddr()] = srv
        if len(srv.servname) != 0 and not srv.namebyip:
            self.serversbyname[srv.servname] = srv
            self.serversanyname[srv.servname] = srv
        if len(srv.alias) != 0:
            self.serversanyname[srv.alias] = srv

    def load(self, node):
        """Load list from XML file"""
        # Do it in two passes so we've got getloc and myaddr set up first
        child = node.firstChild()
        self.getlocaddr = False
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "lahost":
                self.lahost = xmlutil.getText(child)
            elif tagn == "laport":
                self.laport = int(xmlutil.getText(child))
            elif tagn == "servers":
                self.serversanyname = dict()
                self.serversbyname = dict()
                self.serversbyip = dict()
                self.namecache = dict()
                self.ipcache = dict()
                srv = child.firstChild()
                while not srv.isNull():
                    s = gbserver()
                    s.load(srv)
                    self.add(s)
                    srv = srv.nextSibling()
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save list to XML file"""
        sl = doc.createElement(name)
        pnode.appendChild(sl)
        xmlutil.save_xml_string(doc, sl, "lahost", self.lahost)
        xmlutil.save_xml_string(doc, sl, "laport", str(self.laport))
        if len(self.serversbyip) != 0:
            dl = doc.createElement("servers")
            sl.appendChild(dl)
            for srv in self.list_servers():
                srv.save(doc, dl, "server", self.myhostid)

    def list_servers(self):
        """Get list of servers"""
        return self.serversbyip.values()

    def list_servnames(self):
        """Get list of server names"""
        return  [s.get_servname() for s in self.list_servers()]

    def delete(self, sname):
        """Delete specified server from list"""
        if sname not in self.serversanyname: return
        srv = self.serversanyname[sname]
        del self.serversbyip[srv.servip.ipaddr()]
        if len(srv.servname) != 0 and not srv.namebyip:
            del self.serversanyname[srv.servname]
            del self.serversbyname[srv.servname]
        if len(srv.alias) != 0:
            del self.serversanyname[srv.alias]

    def perform_login(self, w, server):
        """Run login procedure on specified server"""

        # First try logging in using "enquire" in case we were already logged in
        # as far as the server is concerned

        try:
            if server.getperms_nopw():
                return True
        except uaclient.uaclientException:
            return False

        # Didn't manage to log in first retry with user name and password given
        # (possibly none)

        tried = False

        while 1:
            try:
                if server.getperms_pw(w, self.winuname, self.password, self.myname, tried):
                    return True
            except uaclient.uaclientException:
                return False

            tried = True

            dlg = Clogindlg()
            dlg.servName.setText(server.get_servname())
            dlg.clientName.setText(self.myname)
            dlg.userName.setText(self.winuname)

            if not dlg.exec_():
                return False
            self.winuname = str(dlg.userName.text())
            self.password = str(dlg.password.text())

    def login_all(self, w):
        """Try to log in everything"""
        for srv in self.list_servers():
            if srv.autoconn:
                self.perform_login(w, srv)

    def get_ulist(self):
        """Get list of possible users"""
        result = set()
        for srv in self.list_servers():
            if srv.isok: result |= srv.uasock.getulist()
        result = list(result)
        result.sort()
        return result

    def get_glist(self):
        """Get list of possible groups"""
        result = set()
        for srv in self.list_servers():
            if srv.isok: result |= srv.uasock.getglist()
        result = list(result)
        result.sort()
        return result

    def get_cilist(self):
        """Get set of possible cis"""
        result = set()
        for srv in self.list_servers():
            if srv.isok: result |= srv.uasock.getcilist()
        return  result
