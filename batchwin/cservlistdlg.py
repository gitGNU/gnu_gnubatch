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
from PyQt4.QtNetwork import *

import ui_servlist
import ui_hostdlg
import ui_upermdisp
import btuser
import btqwopts
import gbserver
import gbnetid
import uaclient

# Columns for various things

NAME_COLUMN = 0
ALIAS_COLUMN = 1
IP_COLUMN = 2
UNAME_COLUMN = 3
AUTOCONN_COLUMN = 4
CONNECTED_COLUMN = 5
SYNC_COLUMN = 6

class Chostdlg(QDialog, ui_hostdlg.Ui_hostdlg):

    def __init__(self, parent = None):
        super(Chostdlg, self).__init__(parent)
        self.setupUi(self)

class Cupermdispdlg(QDialog, ui_upermdisp.Ui_userperms):

    def __init__(self, parent = None):
        super(Cupermdispdlg, self).__init__(parent)
        self.setupUi(self)

class Cservlistdlg(QDialog, ui_servlist.Ui_serversdlg):

    def __init__(self, parent = None):
        super(Cservlistdlg, self).__init__(parent)
        self.setupUi(self)
        self.servlist.doubleClicked.connect(self.on_DoubleClicked)
        self.wusername.editingFinished.connect(self.on_Setwuser)
        self.notyet = True
        self.mainwin = None

    def setup_checkbox(self, pos, col, value):
        """Set up a checkbox in a table column"""
        item = QTableWidgetItem()
        item.setTextAlignment(Qt.AlignCenter)
        item.setFlags(Qt.ItemIsEnabled|Qt.ItemIsSelectable)
        if value:
            item.setCheckState(Qt.Checked)
        else:
            item.setCheckState(Qt.Unchecked)
        self.servlist.setItem(pos, col, item)

    def setup_stringbox(self, pos, col, value):
        """Set up a string box in a table column"""
        item = QTableWidgetItem(value)
        item.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled)
        self.servlist.setItem(pos, col, item)

    def read_checkbox(self, row, col):
        """Read checkbox value from row"""
        return self.servlist.item(row, col).checkState() == Qt.Checked

    def fillrow(self, srv, pos):
        """Insert server data into row"""
        # Set name
        if srv.namebyip:
            item = srv.servip.ipaddr()
        else:
            item = srv.servname
        self.setup_stringbox(pos, NAME_COLUMN, item)
        # Set alias, IP, IP to, user
        self.setup_stringbox(pos, ALIAS_COLUMN, srv.alias)
        self.setup_stringbox(pos, IP_COLUMN, srv.servip.ipaddr())
        if len(srv.perms.username) != 0:
            self.setup_stringbox(pos, UNAME_COLUMN, srv.perms.username)
        # Autoconnect/connected/synced
        self.setup_checkbox(pos, AUTOCONN_COLUMN, srv.autoconn)
        self.setup_checkbox(pos, CONNECTED_COLUMN, srv.connected)
        self.setup_checkbox(pos, SYNC_COLUMN, srv.synccomplete)

    def listnames(self):
        """List host names from name column as dictionary giving row number"""
        result = dict()
        for row in range(0, self.servlist.rowCount()):
            h = self.servlist.item(row, NAME_COLUMN).text()
            if '0' <= h[0] <= '9': continue
            result[str(h)] = row
        return result

    def listaliases(self):
        """As listnames but for aliases"""
        result = dict()
        for row in range(0, self.servlist.rowCount()):
            a = self.servlist.item(row, ALIAS_COLUMN).text()
            if a.length() != 0: result[str(a)] = row
        return result

    def clashcheck(self, name, skiprow):
        """Check name for clashes with other rows than the skipped row"""
        if len(name) == 0: return False
        hnames = self.listnames()
        try:
            if hnames[name] != skiprow: return True
        except KeyError:
            pass
        anames = self.listaliases()
        try:
            if anames[name] != skiprow: return True
        except KeyError:
            pass
        return False

    def approw(self, srv):
        """Append row and fill in server details"""
        nrows = self.servlist.rowCount()
        self.servlist.insertRow(nrows)
        self.fillrow(srv, nrows)

    def getselectedserv(self):
        """Return a server structure corresponding to selected row"""
        row = self.servlist.currentRow()
        if row < 0: return None
        ip = str(self.servlist.item(row, IP_COLUMN).text())
        try:
            return btqwopts.Options.servers.serversbyip[ip]
        except KeyError:
            return None

    def setlahost(self, host):
        try:
            port = int(str(self.laport.currentText()))
        except ValueError:
            QMessageBox.warning(self, "Invalid port number", "Please give a numeric port number")
            return False
        try:
            btqwopts.Options.servers.get_locaddr(host, port)
            QMessageBox.information(self, "Got address OK", "Got local address OK, result was " + btqwopts.Options.servers.myhostid)
            return True
        except gbserver.ServError as e:
            QMessageBox.Warning(self, e.args[0], "Cannot set local address from " + host + " and " + str(port))
            return False

    def on_setla_clicked(self, b = None):
        if b is None: return
        self.setlahost(str(self.lahost.text()))

    def on_getlafrom_clicked(self, b = None):
        if b is None: return
        srv = self.getselectedserv()
        if srv is None: return
        if srv.namebyip:
            name = srv.servip.ipaddr()
        else:
            name = srv.servname
        if self.setlahost(name):
            self.lahost.setText(name)

    def on_Newserv_clicked(self, b = None):
        if b is None: return
        dlg = Chostdlg()
        while dlg.exec_():
            hostn = str(dlg.hostName.text())
            if self.clashcheck(hostn, -1):
                QMessageBox.warning(self, "Clash host name", "Host name " + hostn + " clashes with existing name")
                continue
            aliasn = str(dlg.aliasName.text())
            if self.clashcheck(aliasn, -1):
                QMessageBox.warning(self, "Clash alias name", "Host name " + aliasn + " clashes with existing name")
                continue
            newserver = gbserver.gbserver()
            try:
                newserver.create(hostn, aliasn, dlg.autoconn.isChecked())
                btqwopts.Options.servers.add(newserver)
            except gbserver.ServError as e:
                QMessageBox.warning(self, "Error with server", e.args[0])
                continue
            self.servlist.setSortingEnabled(False)
            self.approw(newserver)
            self.servlist.resizeColumnsToContents()
            self.servlist.setSortingEnabled(True)
            return

    def on_Delserv_clicked(self, b = None):
        if b is None: return
        srv = self.getselectedserv()
        if srv is None: return
        if srv.connected:
            QMessageBox.warning(self, "Cannot delete server", "Server is connected please disconnect first")
            return
        btqwopts.Options.servers.delete(srv.get_servname())
        self.servlist.removeRow(self.servlist.currentRow())
        self.servlist.resizeColumnsToContents()

    def on_DoubleClicked(self):
        srv = self.getselectedserv()
        if srv is None: return
        if srv.connected:
            QMessageBox.warning(self, "Cannot change server", "Server is connected please disconnect first")
            return
        dlg = Chostdlg()
        if srv.namebyip:
            name = srv.servip.ipaddr()
        else:
            name = srv.servname
        dlg.hostName.setText(name)
        dlg.aliasName.setText(srv.alias)
        dlg.autoconn.setChecked(srv.autoconn)
        while dlg.exec_():
            row = self.servlist.currentRow()
            hostn = str(dlg.hostName.text())
            if self.clashcheck(hostn, row):
                QMessageBox.warning(self, "Clash host name", "Host name " + hostn + " clashes with existing name")
                continue
            aliasn = str(dlg.aliasName.text())
            if self.clashcheck(aliasn, row):
                QMessageBox.warning(self, "Clash alias name", "Host name " + aliasn + " clashes with existing name")
                continue
            newserver = gbserver.gbserver()
            try:
                newserver.create(hostn, aliasn, dlg.autoconn.isChecked())
            except gbserver.ServError as e:
                QMessageBox.warning(self, "Error with server", e.args[0])
                continue
            btqwopts.Options.servers.delete(srv.get_servname())
            self.servlist.setSortingEnabled(False)
            self.servlist.removeRow(self.servlist.currentRow())
            self.approw(newserver)
            self.servlist.resizeColumnsToContents()
            self.servlist.setSortingEnabled(True)
            return

    def on_Setwuser(self):
        if self.notyet: return
        wname = str(self.wusername.text())
        if len(wname) == 0 or wname == btqwopts.Options.servers.winuname: return
        btqwopts.Options.servers.winuname = wname

    def on_connectserv_clicked(self, b = None):
        if b is None: return
        srv = self.getselectedserv()
        if srv is None: return
        if srv.connected:
            QMessageBox.warning(self, "Server already connected", srv.get_servname() + " is already connected")
            return
        if btqwopts.Options.servers.perform_login(self, srv):
            if len(srv.perms.username) != 0:
                self.setup_stringbox(self.servlist.currentRow(), UNAME_COLUMN, srv.perms.username)
            self.mainwin.init_client(srv)
            self.mainwin.connect_client(srv)
            self.setup_checkbox(self.servlist.currentRow(), CONNECTED_COLUMN, True)

    def on_disconnserv_clicked(self, b = None):
        if b is None: return
        srv = self.getselectedserv()
        if srv is None: return
        if not srv.connected:
            QMessageBox.warning(self, "Server not connected", srv.get_servname() + " is not connected")
            return
        self.mainwin.disconnect_client(srv)
        try:
            srv.uasock.ualogout()
        except uaclient.uaclientException:
            pass
        crow = self.servlist.currentRow()
        self.setup_stringbox(crow, UNAME_COLUMN, "")
        self.setup_checkbox(crow, CONNECTED_COLUMN, False)
        self.setup_checkbox(crow, SYNC_COLUMN, False)

    def on_dispUperms_clicked(self, b = None):
        if b is None: return
        dlg = Cupermdispdlg()
        srv = self.getselectedserv()
        if srv is None or not srv.isok: return
        dlg.servname.setText(srv.get_servname())
        uperm = srv.perms
        dlg.username.setText(uperm.username)
        dlg.groupname.setText(uperm.groupname)
        dlg.minpri.setText(str(uperm.btu_minp))
        dlg.defpri.setText(str(uperm.btu_defp))
        dlg.maxpri.setText(str(uperm.btu_maxp))
        dlg.maxll.setText(str(uperm.btu_maxll))
        dlg.totll.setText(str(uperm.btu_totll))
        dlg.speccll.setText(str(uperm.btu_specll))
        dlg.radminperm.setChecked((uperm.btu_priv & btuser.btuser.BTM_RADMIN) != 0)
        dlg.wadminperm.setChecked((uperm.btu_priv & btuser.btuser.BTM_WADMIN) != 0)
        dlg.createperm.setChecked((uperm.btu_priv & btuser.btuser.BTM_CREATE) != 0)
        dlg.chpermperm.setChecked((uperm.btu_priv & btuser.btuser.BTM_UMASK) != 0)
        dlg.spcreateperm.setChecked((uperm.btu_priv & btuser.btuser.BTM_SPCREATE) != 0)
        dlg.or_ug_perm.setChecked((uperm.btu_priv & btuser.btuser.BTM_ORP_UG) != 0)
        dlg.or_uo_perm.setChecked((uperm.btu_priv & btuser.btuser.BTM_ORP_UO) != 0)
        dlg.or_go_perm.setChecked((uperm.btu_priv & btuser.btuser.BTM_ORP_GO) != 0)
        dlg.sstopperm.setChecked((uperm.btu_priv & btuser.btuser.BTM_SSTOP) != 0)
        for jv in 'jv':
            fln = getattr(uperm, 'btu_' + jv + 'flags')
            for ugofl in enumerate('ugo'):
                n, ugo = ugofl
                for perm in enumerate(('read', 'write', 'show', 'rperm', 'wperm', 'utake', 'gtake', 'ugive', 'ggive', 'del', 'kill')):
                    shft, nam = perm
                    isset = (fln[n] & (1 << shft)) != 0
                    try:
                        dlgck = getattr(dlg, jv + ugo + '_' + nam)
                        dlgck.setChecked(isset)
                    except AttributeError:
                        pass
        dlg.exec_()

def update_server_list(mw):
    """Run update server list"""

    dlg = Cservlistdlg()
    slist = btqwopts.Options.servers
    dlg.wusername.setText(slist.winuname)
    dlg.lahost.setText(slist.lahost)
    dlg.laport.setEditText(str(slist.laport))
    sl = dlg.servlist
    sl.setSortingEnabled(False)
    eservers = slist.list_servers()
    for srv in eservers:
        dlg.approw(srv)
    dlg.servlist.resizeColumnsToContents()
    sl.setSortingEnabled(True)
    dlg.notyet = False
    dlg.mainwin = mw
    dlg.exec_()
