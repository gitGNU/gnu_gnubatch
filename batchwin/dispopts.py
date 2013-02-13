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

# Display options
#################

import qmatch
import xmlutil

class dispopts:
    """Class for remembering what restrictions a user wants on what is displayed"""

    def __init__(self):
        self.inclnull = True
        self.limuser = ""
        self.limgroup = ""
        self.limqueue = ""

    def okdisp_job(self, j):
        """Decide whether to display job based on restrictions"""
        if len(self.limuser) > 0  and not qmatch.qmatch(self.limuser, j.bj_mode.o_user):
            return False
        if len(self.limgroup) > 0  and not qmatch.qmatch(self.limgroup, j.bj_mode.o_group):
            return False
        if len(self.limqueue) == 0: return True
        titl = j.bj_title
        qbits = titl.split(':', 1)
        # If no queue name but we are limiting by queue name, depends in value of inclnull
        if len(qbits) <= 1:
            return self.inclnull
        return qmatch.qmatch(self.limqueue, qbits[0])

    def okdisp_var(self, v):
        """Decide whether to display variable based on restrictions"""
        if len(self.limuser) > 0 and not qmatch.qmatch(self.limuser, v.var_mode.o_user):
            return False
        if len(self.limgroup) > 0 and not qmatch.qmatch(self.limgroup, v.var_mode.o_group):
            return False
        return True

    def isqlim(self):
        """Report whether we are limiting display to a queue name"""
        return len(self.limqueue) != 0

    def load(self, node):
        """Slurp from XML node"""
        self.inclnull = False
        self.limuser = ""
        self.limgroup = ""
        self.limqueue = ""
        child = node.firstChild()
        while not child.isNull():
            tagn = child.toElement().tagName()
            if tagn == "inclnull":
                self.inclnull = True
            elif tagn == "limuser":
                self.limuser = xmlutil.getText(child)
            elif tagn == "limgroup":
                self.limgroup = xmlutil.getText(child)
            elif tagn == "limqueue":
                self.limqueue = xmlutil.getText(child)
            child = child.nextSibling()

    def save(self, doc, pnode, name):
        """Output options to XML file"""
        saved = doc.createElement(name)
        pnode.appendChild(saved)
        xmlutil.save_xml_bool(doc, saved, "inclnull", self.inclnull)
        if len(self.limuser) != 0:
            xmlutil.save_xml_string(doc, saved, "limuser", self.limuser)
        if len(self.limgroup) != 0:
            xmlutil.save_xml_string(doc, saved, "limgroup", self.limgroup)
        if len(self.limqueue) != 0:
            xmlutil.save_xml_string(doc, saved, "limqueue", self.limqueue)
