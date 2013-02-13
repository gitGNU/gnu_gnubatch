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

#######################
#  Utility XML routines
#######################

from PyQt4.QtCore import *

def getText(node):
    """Extract a string from a node"""
    return str(node.firstChild().toText().data())

def save_xml_string(doc, pnode, name, val):
    """Save an item to XML file"""
    if isinstance(val,str):
        if len(val) == 0: return
    else:
        val = str(val)
    item = doc.createElement(name)
    pnode.appendChild(item)
    txt = doc.createTextNode(val)
    item.appendChild(txt)

def save_xml_bool(doc, pnode, name, val):
    """Save an XML node if item is true otherwise not"""
    if not val:
        return
    item = doc.createElement(name)
    pnode.appendChild(item)

def loadlist(pnode):
    """Load a list of fields or widths"""
    result = []
    fnode = pnode.firstChild()
    while not fnode.isNull():
        result.append(int(getText(fnode)))
        fnode = fnode.nextSibling()
    return result

def savelist(doc, pnode, name, flist, cname):
    """Save a list of fields or widths"""
    if len(flist) == 0: return
    fn = doc.createElement(name)
    pnode.appendChild(fn)
    for f in flist:
        save_xml_string(doc, fn, cname, f)

# Load and save geom
####################

def loadGeom(node):
    """Load a geometry node"""
    child = node.firstChild()
    x = y = w = h = 0
    while not child.isNull():
        tagn = child.toElement().tagName()
        val = int(getText(child))
        if tagn == "x":
            x = val
        elif tagn == "y":
            y = val
        elif tagn == "w":
            w = val
        elif tagn == "h":
            h = val
        child = child.nextSibling()
    return QRect(x, y, w, h)

def saveGeom(doc, pnode, geom, name = "geom"):
    """Save a geometry node"""
    savedg = doc.createElement(name)
    pnode.appendChild(savedg)
    save_xml_string(doc, savedg, "x", geom.x())
    save_xml_string(doc, savedg, "y", geom.y())
    save_xml_string(doc, savedg, "w", geom.width())
    save_xml_string(doc, savedg, "h", geom.height())

def findnode(pnode, name):
    """Return the child node with given tag name (top level only)"""
    child = pnode.firstChild()
    while not child.isNull():
        if child.toElement().tagName() == name: return child
        child = child.nextSibling()
    return None

