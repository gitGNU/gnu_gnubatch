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
import btmode
import ui_createvar
import ui_assvardlg
import ui_convaldlg
import ui_varcommdlg
import ui_vpermdlg

class Ccreatevardlg(QDialog, ui_createvar.Ui_crevardlg):
    def __init__(self, parent = None):
        super(Ccreatevardlg, self).__init__(parent)
        self.setupUi(self)

    def on_istext_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval.setEnabled(True)
            self.intval.setEnabled(False)
        else:
            self.textval.setEnabled(False)
            self.intval.setEnabled(True)

    def init_servlist(self, sl):
        for s in sl:
            self.crehost.addItem(s)

    def checkdlg(self):
        """Check that the name of the variable is given correctly"""
        nam = self.varname.text()
        re = QRegExp("[a-zA-Z_]\w*")
        if re.exactMatch(nam): return False
        QMessageBox.warning(self, "Invalid Variable Name", "Invalid variable name - " + nam)
        return True

class Cvassdlg(QDialog, ui_assvardlg.Ui_assvardlg):
    def __init__(self, parent = None):
        super(Cvassdlg, self).__init__(parent)
        self.setupUi(self)

    def on_istext_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval.setEnabled(True)
            self.intval.setEnabled(False)
        else:
            self.textval.setEnabled(False)
            self.intval.setEnabled(True)

class Cconvaldlg(QDialog, ui_convaldlg.Ui_convaldlg):
   def __init__(self, parent = None):
        super(Cconvaldlg, self).__init__(parent)
        self.setupUi(self)

class Cvarcommdlg(QDialog, ui_varcommdlg.Ui_varcommdlg):
   def __init__(self, parent = None):
        super(Cvarcommdlg, self).__init__(parent)
        self.setupUi(self)

class Cvpermdlg(QDialog, ui_vpermdlg.Ui_vpermdlg):

    modenames = (("read", btmode.btmode.BTM_READ),
                 ("write", btmode.btmode.BTM_WRITE),
                 ("show", btmode.btmode.BTM_SHOW),
                 ("rdmd", btmode.btmode.BTM_RDMODE),
                 ("wrmd", btmode.btmode.BTM_WRMODE),
                 ("take", btmode.btmode.BTM_UTAKE),
                 ("gtake", btmode.btmode.BTM_GTAKE),
                 ("give", btmode.btmode.BTM_UGIVE),
                 ("ggive", btmode.btmode.BTM_GGIVE),
                 ("del", btmode.btmode.BTM_DELETE))

    def __init__(self, parent = None):
        super(Cvpermdlg, self).__init__(parent)
        self.setupUi(self)

    def init_perms(self, perms):
        """Copy permissions into dialog"""
        for wm in Cvpermdlg.modenames:
            mname, bit = wm
            for ugo in 'ugo':
                if (getattr(perms, ugo + "_flags") & bit) != 0:
                    getattr(self, ugo + mname).setChecked(True)

    def get_perms(self, perms):
        """Extract perms from dialog and plug into mode"""
        perms.u_flags = perms.g_flags = perms.o_flags = 0
        for wm in Cvpermdlg.modenames:
            mname, bit = wm
            for ugo in 'ugo':
                if getattr(self, ugo + mname).isChecked():
                    flname = ugo + "_flags"
                    setattr(perms, flname, getattr(perms, flname) | bit)

    # Turning read on should turn show on
    # Turning read off shuuld turn write off

    def on_uread_stateChanged(self, b):
        if b is None: return
        if b == Qt.Unchecked:
            self.uwrite.setChecked(False)
        else:
            self.ushow.setChecked(True)

    # Turning write on should turn read on (which will turn show on)

    def on_uwrite_stateChanged(self, b):
        if b is None or b == Qt.Unchecked: return
        self.uread.setChecked(True)

    # Turning show off should turn read off (which will turn write off)

    def on_ushow_stateChanged(self, b):
        if b is None or b != Qt.Unchecked: return
        self.uread.setChecked(False)

    # Similar for read/write permissions

    def on_urdmd_stateChanged(self, b):
        if b is None or b != Qt.Unchecked: return
        self.uwrmd.setChecked(False)

    def on_uwrmd_stateChanged(self, b):
        if b is None or b == Qt.Unchecked: return
        self.urdmd.setChecked(True)

    # Ditto for groups

    def on_gread_stateChanged(self, b):
        if b is None: return
        if b == Qt.Unchecked:
            self.gwrite.setChecked(False)
        else:
            self.gshow.setChecked(True)

    def on_gwrite_stateChanged(self, b):
        if b is None or b == Qt.Unchecked: return
        self.gread.setChecked(True)

    def on_gshow_stateChanged(self, b):
        if b is None or b != Qt.Unchecked: return
        self.gread.setChecked(False)

    def on_grdmd_stateChanged(self, b):
        if b is None or b != Qt.Unchecked: return
        self.gwrmd.setChecked(False)

    def on_gwrmd_stateChanged(self, b):
        if b is None or b == Qt.Unchecked: return
        self.grdmd.setChecked(True)

    # Ditto for others

    def on_oread_stateChanged(self, b):
        if b is None: return
        if b == Qt.Unchecked:
            self.owrite.setChecked(False)
        else:
            self.oshow.setChecked(True)

    def on_owrite_stateChanged(self, b):
        if b is None or b == Qt.Unchecked: return
        self.oread.setChecked(True)

    def on_oshow_stateChanged(self, b):
        if b is None or b != Qt.Unchecked: return
        self.oread.setChecked(False)

    def on_ordmd_stateChanged(self, b):
        if b is None or b != Qt.Unchecked: return
        self.owrmd.setChecked(False)

    def on_owrmd_stateChanged(self, b):
        if b is None or b == Qt.Unchecked: return
        self.ordmd.setChecked(True)
