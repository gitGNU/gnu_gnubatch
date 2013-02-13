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

import time
import copy

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import btclasses
import btmode
import dispopts
import gbnetid
import btqwopts
import gbident
import globlists
from operator import attrgetter

def fmt_name(var, m):
    """Display the variable name prefixed by the host"""
    return "%s:%s" % (btqwopts.Options.servers.look_hostid(var.ipaddr()), var.var_name)

def fmt_value(var, m):
    """Display the value of the variable"""
    var.var_mode.check_readable(var)
    return str(var.var_value)

def fmt_comment(var, m):
    """Return comment field"""
    var.var_mode.check_readable(var)
    return  var.var_comment

def fmt_user(var, m):
    """Report owner of variable"""
    return var.var_mode.o_user

def fmt_group(var, m):
    """Report group owner of variable"""
    return var.var_mode.o_group

def fmt_mode(var, m):
    """Report variable modes (permissions)"""
    var.var_mode.check_accessible(var, btmode.btmode.BTM_RDMODE)
    return var.var_mode.mode_disp()

def fmt_export(var, m):
    """Export markers"""
    if (var.var_flags & btclasses.btvar.VF_EXPORT) != 0: return "Export"
    else: return "Local"

def fmt_clust(var, m):
    """Cluster marker"""
    if (var.var_flags & btclasses.btvar.VF_CLUSTER) != 0: return "Clust"
    else: return ""

# Possible formats for bits of variables
# Each item gives:
# 1. Format description
# 2. Format header
# 3. Function
# 4. Alignment stuff
# 5. Initial field width (currently wants doing properly)

Formats = (
    ('Variable Name',                   'Name', fmt_name,       int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #0
    ('Variable Value',                  'Value',fmt_value,      int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #1
    ('Comment on variable',             'Comment',fmt_comment,  int(Qt.AlignLeft|Qt.AlignVCenter), 150),  #2
    ('Variable owner',                  'User', fmt_user,       int(Qt.AlignLeft|Qt.AlignVCenter),  80),  #3
    ('Variable group owner',            'Group',fmt_group,      int(Qt.AlignLeft|Qt.AlignVCenter),  80),  #4
    ('Permissions on variable',         'Perms',fmt_mode,       int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #5
    ('Export marker',                   'Export',fmt_export,    int(Qt.AlignLeft|Qt.AlignVCenter),  80),  #6
    ('Cluster marker',                  'Clust',fmt_clust,      int(Qt.AlignLeft|Qt.AlignVCenter),  80))  #7

Default_fieldnums = [ 0, 1, 2, 3, 4, 6  ]
Default_size = QRect(10, 10, 350, 250)

class VarModel(QAbstractTableModel):
    """Model for holding var info"""

    def __init__(self):
        super(VarModel, self).__init__()
        self.isjob = False
        self.theView = None
        self.vars = []
        self.varrows = dict()
        self.fieldlist = Default_fieldnums
        self.geom = Default_size
        self.dispoptions = copy.deepcopy(btqwopts.Options.defwopts)

    def getselectedvarrow(self):
        """Return row number of selected var"""
        try:
            inds = self.theView.selectedIndexes()
            return inds[0].row()
        except (AttributeError, IndexError):
            return -1

    def getselectedvar(self):
        """Return selected var"""
        row = self.getselectedvarrow()
        if row < 0:
            return None
        return self.vars[row]

    def renumrows(self):
        """Reset row lookup"""
        rownum = 0
        self.varrows = dict()
        for v in self.vars:
            self.varrows[v] = rownum
            rownum += 1

    def sortvars(self):
        """Sort the variables by name and host within name

Assign row numbers to variable rows with lookup by id"""

        # Remember what we had selected so we can reselect it later

        vid = self.getselectedvar()

        self.vars.sort(key=lambda v: btqwopts.Options.servers.look_hostid(v.ipaddr()))
        self.vars.sort(key=attrgetter('var_name'))

        self.renumrows()
        self.reset()

        if vid:
            try:
                self.theView.selectRow(self.varrows[vid])
            except IndexError:
                pass

    def replacerows(self):
        """Replace all the rows after we've changed the display options"""
        nrows = self.rowCount()
        if nrows > 0:
            self.beginRemoveRows(QModelIndex(), 0, nrows-1)
            self.endRemoveRows()
        self.vars = []
        for v in globlists.Var_list.values():
            if v.isvisible and self.dispoptions.okdisp_var(v):
                self.vars.append(v)
                self.insertRows(0)
        self.sortvars()

    def rowchanged(self, row):
        """Notify that a row has changed"""
        indb = self.createIndex(row, 0, QModelIndex())
        inde = self.createIndex(row, len(self.fieldlist)-1, QModelIndex())
        self.dataChanged.emit(indb, inde)

    def rowCount(self, index=QModelIndex()):
        return len(self.vars)

    def columnCount(self, index=QModelIndex()):
        return len(self.fieldlist)

    def insertRows(self, position):
        self.beginInsertRows(QModelIndex(), position, position)
        self.endInsertRows()

    def removeRows(self, position):
        self.beginRemoveRows(QModelIndex(), position, position)
        self.endRemoveRows()

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid() or not (0 <= index.row() < len(self.vars)):
            return QVariant()
        var = self.vars[index.row()]
        column = index.column()
        if role == Qt.TextAlignmentRole:
            return QVariant(Formats[self.fieldlist[column]][3])
        if role == Qt.DisplayRole:
            try:
                return QVariant(Formats[self.fieldlist[column]][2](var, self))
            except btmode.Nopriv:
                return QVariant("")
        return QVariant()

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        if role == Qt.TextAlignmentRole:
            return QVariant(Formats[self.fieldlist[section]][3])
        if role != Qt.DisplayRole:
            return QVariant()
        if orientation == Qt.Horizontal:
            try:
                return QVariant(Formats[self.fieldlist[section]][1])
            except IndexError:
                return QVariant()
        return QVariant(int(section+1000))

    def addjob(self, job):
        """Add job to model"""
        pass

    def deljob(self, job):
        """Delete job from model"""
        pass

    def addvar(self, var):
        """Add variable to model"""
        if not self.dispoptions.okdisp_var(var): return
        self.vars.append(var)
        self.insertRows(0)              # Doesn't matter where we are redrawing
        self.sortvars()

    def delvar(self, var):
        """Delete variable from model"""
        try:
            row = self.varrows[var]
            self.removeRows(row)
            del self.vars[row]
            self.renumrows()
        except KeyError:
            return

    def updjob(self, job, origpri):
        """Update existing job"""
        pass

    def boqjob(self, job):
        """Reposition job at bottom of queue"""
        pass

    def assignedvar(self, var):
        """Updates to variable"""
        # We might have previously decided not to display this variable
        # in which case it won't know about it and KeyError gets raised
        # Or it might have changed so we don't want to see it any more
        try:
            row = self.varrows[var]
            if not self.dispoptions.okdisp_var(var):
                self.delvar(var)
                return
            self.vars[row] = var
            self.rowchanged(row)
        except KeyError:
            self.addvar(var)

    def renamedvar(self, var):
        """Rename variable"""
        try:
            row = self.varrows[var]
            self.vars[row] = var
            self.sortvars()
        except KeyError:
            pass

    def findmatch(self, dlg, row):
        """Find string as per dlg in given row and select it"""
        sstring = dlg.searchstring.text()
        var = self.vars[row]
        if (dlg.sname.isChecked() and
            QString(var.var_name).indexOf(sstring, 0, Qt.CaseInsensitive) >= 0) or \
            (dlg.svalue.isChecked() and
            QString(str(var.var_value)).indexOf(sstring, 0, Qt.CaseInsensitive) >= 0) or \
            (dlg.suser.isChecked() and
            QString(var.var_mode.o_user).indexOf(sstring, 0, Qt.CaseInsensitive) >= 0) or \
            (dlg.scomment.isChecked() and \
            QString(var.var_comment).indexOf(sstring, 0, Qt.CaseInsensitive) >= 0):
            self.theView.selectRow(row)
            return True
        return False

    def exec_jvsearch(self, dlg, forcedir = False, isbackw = False):
        """Execute search function for vars matching string"""
        if forcedir:
            backsearch = isbackw
        else:
            backsearch = dlg.sbackward.isChecked()
        row = self.getselectedvarrow()
        nrows = self.rowCount()
        if backsearch:
            if row < 0: row = nrows
            while row > 0:
                row -= 1
                if self.findmatch(dlg, row):
                    return True
            if not dlg.wrapround.isChecked(): return False
            row = nrows
            while row > 0:
                row -= 1
                if self.findmatch(dlg, row):
                    return True
        else:
            if row < 0: row = 0
            while row < nrows:
                if self.findmatch(dlg, row):
                    return True
                row += 1
            if not dlg.wrapround.isChecked(): return False
            row = 0
            while row < nrows:
                if self.findmatch(dlg, row):
                    return True
                row += 1
        return False