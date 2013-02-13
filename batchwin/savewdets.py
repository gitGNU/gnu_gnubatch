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

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import xmlutil
import dispopts

# Save job or variable windows
##############################

class savewdets:
    """Class to save details of job or variable window for loading/saving to file"""

    def __init__(self, jorv = None):
        self.isjob = False
        if jorv:
            self.isjob = jorv.isjob
            self.geom = jorv.geom
            self.dispoptions = jorv.dispoptions
            self.fields = jorv.fieldlist
            self.fwidths = jorv.fieldwidths
        else:
            self.geom = QRect()
            self.dispoptions = dispopts.dispopts()
            self.fields = []
            self.fwidths = []

    def load(self, node):
        """Load saved window details from XML file"""
        self.isjob = node.toElement().attribute("type", "v") == "j"
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "geom":
                self.geom = xmlutil.loadGeom(child)
            elif tagn == "wopts":
                self.dispoptions.load(child)
            elif tagn == "fields":
                self.fields = xmlutil.loadlist(child)
            elif tagn == "fwids":
                self.fwidths = xmlutil.loadlist(child)
            child = child.nextSibling()

    def save(self, doc, pnode):
        """Save window details to XML file"""
        savedg = doc.createElement("wdets")
        pnode.appendChild(savedg)
        if self.isjob:
            savedg.setAttribute("type", "j")
        else:
            savedg.setAttribute("type", "v")
        xmlutil.saveGeom(doc, savedg, self.geom)
        self.dispoptions.save(doc, savedg, "wopts")
        xmlutil.savelist(doc, savedg, "fields", self.fields, "f")
        xmlutil.savelist(doc, savedg, "fwids", self.fwidths, "w")

def init_winlist(jm, vm):
    """Return a pair of default windows if we don't start up with any

This is probably the very first time we use it"""

    jwin = savewdets()
    jwin.isjob = True
    jwin.geom = jm.Default_size
    jwin.fields = jm.Default_fieldnums
    jwin.fwidths = [jm.Formats[f][4] for f in jwin.fields]

    vwin = savewdets()
    vwin.geom = vm.Default_size
    vwin.fields = vm.Default_fieldnums
    vwin.fwidths = [vm.Formats[f][4] for f in vwin.fields]

    return [jwin, vwin]
