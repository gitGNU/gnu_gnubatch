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

import os

# We believe that this tells us if we are running Windows
# Have to do this first to avoid hassles with recursive imports

WIN = os.name == "nt"

GBDOCROOT = "XiBatch"

from PyQt4.QtCore import *
from PyQt4.QtXml import *
import xmlutil
import timecon
import dispopts
import gbports
import gbserver
import savewdets
import btclasses

class OptError(Exception):
    pass

class Btrwopts:
    """Class for options specific to btrw"""

    def __init__(self):
        self.btrwgeom = QRect(10, 10, 600, 400)
        self.cwidths = None
        self.prefhost = None
        self.verbose = True
        self.useexted = False
        self.exted = ""
        self.inshell = False
        self.shell = "xterm"
        self.shellarg = "-e"
        self.qprefixes = []
        self.defjob = None          # Saved job details
        self.recentfiles = None

    def load(self, node):
        """Load options from XML file"""
        self.useexted = False
        self.inshell = False
        self.verbose = False
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "prefhost":
                self.prefhost = xmlutil.getText(child)
            elif tagn == "verbose":
                self.verbose = True
            elif tagn == "useext":
                self.useexted = True
            elif tagn == "exted":
                self.exted = xmlutil.getText(child)
            elif tagn == "inshell":
                self.inshell = True
            elif tagn == "shell":
                self.shell = xmlutil.getText(child)
            elif tagn == "shellarg":
                self.shellarg = xmlutil.getText(child)
            elif tagn == "geom":
                self.btrwgeom = xmlutil.loadGeom(child)
            elif tagn == "cwidths":
                self.cwidths = []
                gc = child.firstChild()
                while not gc.isNull():
                    self.cwidths.append(int(xmlutil.getText(gc)))
                    gc = gc.nextSibling()
            elif tagn == "qprefs":
                self.qprefixes = []
                gc = child.firstChild()
                while not gc.isNull():
                    self.qprefixes.append(xmlutil.getText(gc))
                    gc = gc.nextSibling()
            elif tagn == "defjob":
                self.defjob = btclasses.btjob()
                self.defjob.load(child)
            elif tagn == "recentfiles":
                self.recentfiles = []
                gc = child.firstChild()
                while not gc.isNull():
                    self.recentfiles.append(xmlutil.getText(gc))
                    gc = gc.nextSibling()
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Save options to XML file"""
        node = doc.createElement(name)
        if self.prefhost is not None:
            xmlutil.save_xml_string(doc, node, "prefhost", self.prefhost)
        xmlutil.save_xml_bool(doc, node, "verbose", self.verbose)
        xmlutil.save_xml_bool(doc, node, "useext", self.useexted)
        xmlutil.save_xml_string(doc, node, "exted", self.exted)
        xmlutil.save_xml_bool(doc, node, "inshell", self.inshell)
        xmlutil.save_xml_string(doc, node, "shell", self.shell)
        xmlutil.save_xml_string(doc, node, "shellarg", self.shellarg)
        xmlutil.saveGeom(doc, node, self.btrwgeom)
        if self.cwidths:
            cw = doc.createElement("cwidths")
            for c in self.cwidths:
                xmlutil.save_xml_string(doc, cw, "w", str(c))
            node.appendChild(cw)
        qp = doc.createElement("qprefs")
        for p in self.qprefixes:
            xmlutil.save_xml_string(doc, qp, "p", p)
        node.appendChild(qp)
        if self.defjob:
            self.defjob.save(doc, node, "defjob")
        if self.recentfiles and len(self.recentfiles) != 0:
            rf = doc.createElement("recentfiles")
            for f in self.recentfiles:
                xmlutil.save_xml_string(doc, rf, "f", f)
            node.appendChild(rf)
        pnode.appendChild(node)

# Main options
##############

class Btqwopts:
    """Class for recording saved options for BTQW"""

    def __init__(self, fname, wuser):
        self.confdel = True
        self.unqueueasbin = False
        self.geom = QRect(10, 10, 600, 400)
        self.udpwaittime = 750
        self.defwopts = dispopts.dispopts()
        self.winlist = []
        self.timedefs = timecon.timecon()
        self.ports = gbports.gbports()
        self.servers = gbserver.servlist(wuser)
        self.configfile = fname
        self.arithconst = 1
        self.savejobdir = os.getcwd()
        self.btrwopts = Btrwopts()
        self.modtime = 0.0

    def require_confirm(self, j):
        """Specify if job deletion requires confirmation"""
        return  self.confdel

    def load(self, node):
        """Load options from XML file"""
        self.confdel = False
        self.unqueueasbin = False
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "confdel":
                self.confdel = True
            elif tagn == "arith":
                self.arithconst = int(xmlutil.getText(child))
            elif tagn == "unqueuebin":
                self.unqueueasbin = True
            elif tagn == "geom":
                self.geom = xmlutil.loadGeom(child)
            elif tagn == "udptime":
                self.udpwaittime = int(xmlutil.getText(child))
            elif tagn == "defwopts":
                self.defwopts.load(child)
            elif tagn == "winlist":
                self.winlist = []
                wchild = child.firstChild()
                while not wchild.isNull():
                    ws = savewdets.savewdets()
                    ws.load(wchild)
                    self.winlist.append(ws)
                    wchild = wchild.nextSibling()
            elif tagn == "deftime":
                self.timedefs.load(child)
            elif tagn == "PORTS":
                self.ports.load(child)
            elif tagn == "SERVERS":
                self.servers.load(child)
            elif tagn == "savejobdir":
                self.savejobdir = xmlutil.getText(child)
            elif tagn == "btrwopts":
                self.btrwopts.load(child)
            child = child.nextSibling()

    def save(self, doc, node):
        """Save options to XML file"""
        xmlutil.save_xml_bool(doc, node, "confdel", self.confdel)
        xmlutil.save_xml_string(doc, node, "arith", self.arithconst)
        xmlutil.save_xml_bool(doc, node, "unqueuebin", self.unqueueasbin)
        xmlutil.saveGeom(doc, node, self.geom)
        xmlutil.save_xml_string(doc, node, "udptime", self.udpwaittime)
        self.defwopts.save(doc, node, "defwopts")
        self.timedefs.save(doc, node, "deftime")
        wsave = doc.createElement("winlist")
        node.appendChild(wsave)
        for w in self.winlist:
            w.save(doc, wsave)
        self.ports.save(doc, node, "PORTS")
        self.servers.save(doc, node, "SERVERS")
        xmlutil.save_xml_string(doc, node, "savejobdir", self.savejobdir)
        self.btrwopts.save(doc, node, "btrwopts")

def load_options(fname, winuser):
    """Initialise global Options and load from file"""

    global Options
    Options = Btqwopts(fname, winuser)
    doc = QDomDocument()
    try:
        fh = QFile(fname)
        if not fh.open(QIODevice.ReadOnly):
            raise IOError(unicode(fh.errorString()))
        if not doc.setContent(fh):
            raise OptError("Could not parse XML file " + fname)
    except IOError:
        return
    finally:
        fh.close()
    try:
        root = doc.documentElement()
        if root.tagName() != GBDOCROOT:
            raise OptError("Unexpected document tagname " + root.tagName())
        Options.load(root)
        Options.modtime = os.path.getmtime(fname)
    except ValueError as err:
        raise OptError("Document load error " + err.args[0])

def save_options(recentfiles = None):
    """Save options (in Options) to file (name also given in Options)

In the case of BTRW we pass a recent file list"""

    global Options
    if recentfiles is not None:
        Options.btrwopts.recentfiles = recentfiles
    doc = QDomDocument(GBDOCROOT)
    root = doc.createElement(GBDOCROOT)
    doc.appendChild(root)
    Options.save(doc, root)
    # Check tha the btrw options haven't been saved whilst we weren't watching
    # If it has, reload it and replace the options appropriately
    fh = QFile(Options.configfile)
    try:
        # If we are actually in BTRW, we jump out as this code is
        # for when we are saving options in BTQW and we want to merge
        # in options saved in an instance of BTRW whilst this is running
        if recentfiles is not None: raise IOError('n')
        newmtime = os.path.getmtime(Options.configfile)
        if newmtime <= Options.modtime: raise IOError('n')
        if not fh.open(QIODevice.ReadOnly): raise IOError('n')
        newdoc = QDomDocument()
        if not newdoc.setContent(fh): raise IOError('n')
        newbtrnode = xmlutil.findnode(newdoc.documentElement(), "btrwopts")
        if newbtrnode is None: raise IOError('n')
        oldbtrnode = xmlutil.findnode(root, "btrwopts")
        if oldbtrnode is not None: root.removeChild(oldbtrnode)
        root.appendChild(newbtrnode)
    except (OSError, IOError):
        pass
    finally:
        fh.close()
    xmlstr = doc.toString()
    fh = None
    try:
        fh = QFile(Options.configfile)
        if not fh.open(QIODevice.WriteOnly):
            raise OptError(unicode(fh.errorString()))
        fh.write(str(xmlstr))
        Options.modtime = os.path.getmtime(Options.configfile)
    except (OSError, ValueError) as s:
        raise OptError(s.args[0])
    finally:
        fh.close()
