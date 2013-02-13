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

# Load jobs from and save to XML DOM files

from PyQt4.QtCore import *
from PyQt4.QtXml import *
import os
import copy
import xmlutil
import btclasses

BatchJob = "Job"
JOBURI = "http://www.fsf.org/gnubatch/gbj1"

class JobSaveExcept(Exception):
    pass

def jobsave(fname, job, script = None):
    """Save job to given job file

Possibly add the script as a CDATA element"""

    doc = QDomDocument(BatchJob)
    root = doc.createElementNS(JOBURI, BatchJob)
    doc.appendChild(root)
    job.save(doc, root, "jobdescr")
    if script is not None:
        cd = doc.createCDATASection(script)
        cdd = doc.createElement('script')
        cdd.appendChild(cd)
        root.appendChild(cdd)
    xmlstr = doc.toString()
    fh = None
    try:
        fh = QFile(fname)
        if not fh.open(QIODevice.WriteOnly):
            raise JobSaveExcept(unicode(fh.errorString()))
        fh.write(str(xmlstr))
    except (OSError, ValueError) as s:
        raise JobSaveExcept(s.args[0])
    finally:
        fh.close()

class jobitem(btclasses.btjob):
    """Saved job item including possible script string and file status"""

    def __init__(self, clonej = None):
        btclasses.btjob.__init__(self)
        self.bj_direct = os.getcwd()
        if clonej is not None:
            self.clonefields(clonej)
        self.dirty = False
        self.submitted = False
        self.filename = None
        self.script = None
        
    def jobname(self):
        """Construct a title suitable for dialogs"""
        title = ""
        if self.filename is not None: title = "[" + os.path.basename(self.filename) + "] "
        if len(self.bj_title) != 0:
            title += self.bj_title
        return title

    def jobload(self, file):
        """Load job description and script from file"""
        doc = QDomDocument()
        try:
            fh = QFile(file)
            if not fh.open(QIODevice.ReadOnly):
                raise JobSaveExcept(unicode(fh.errorString()))
            if not doc.setContent(fh):
                raise JobSaveExcept("Could not parse XML file " + file)
        except IOError as e:
            raise JobSaveExcept("Load job file gave error", e.message)
        finally:
            fh.close()
        try:
            root = doc.documentElement()
            if root.tagName() != BatchJob:
                raise JobSaveExcept("Unexpected document tagname " + root.tagName())
            child = root.firstChild()
            while not child.isNull():
                tagn = child.toElement().tagName()
                if tagn == "jobdescr":
                    self.load(child)
                elif tagn == "script":
                    self.script = str(child.firstChild().toCharacterData().data())
                child = child.nextSibling()
        except ValueError as err:
            raise JobSaveExcept("Document load error " + err.args[0])
        self.isnew = False
        self.dirty = False
        self.filename = file

    def jobsave(self, file):
        """Save job to file"""
        jobsave(file, self, self.script)
        self.dirty = False
        self.filename = file

