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
import copy
import cmdint
import btmode
import btclasses
import btqwopts
import jobmodel
import ui_titprilldlg
import ui_procpardlg
import ui_timelimdlg
import ui_mailwrtdlg
import ui_jpermdlg
import ui_jargdlg
import ui_jargsdlg
import ui_jenvdlg
import ui_jenvsdlg
import ui_jredirdlg
import ui_jredirsdlg
import ui_jcondsdlg
import ui_jassdlg

class Ctitprilldlg(QDialog, ui_titprilldlg.Ui_titprilldlg):
    def __init__(self, parent = None):
        super(Ctitprilldlg, self).__init__(parent)
        self.setupUi(self)

    def init_cmdints(self, cis, cciname, hasspriv):
        """Initialise the cmd interpreter combo box"""
        cis |= set([cmdint.cmdint(cciname)])
        lcis = list(cis)
        lcis.sort()
        n = vcurr = 0
        for c in lcis:
            name = c.name
            if name == cciname: vcurr = n
            self.cmdint.addItem(name, QVariant(c.ll))
            n += 1
        self.cmdint.setCurrentIndex(vcurr)
        if not hasspriv:
            self.llev.setEnabled(False)
            self.rounddll.setEnabled(False)
            self.roundull.setEnabled(False)

    def init_queues(self, qset, cqueue):
        """Set up queue combo"""
        qset.add("")
        qset.add(cqueue)
        for q in qset:
            self.queue.addItem(q)
        self.queue.lineEdit().setText(cqueue)

    def init_pri(self, curr, minp, maxp):
        """Initialise priority and set limits"""
        if minp <= maxp:
            if curr < minp: curr = minp
            if curr > maxp: curr = maxp
            self.priority.setValue(curr)
            self.priority.setMaximum(maxp)
            self.priority.setMinimum(minp)
        else:
            self.priority.setValue(curr)

    def on_cmdint_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        self.llev.setValue(self.cmdint.itemData(b).toInt()[0])

    def on_llev_valueChanged(self, b = None):
        if not isinstance(b, int): return
        self.llval.setText(str(b))

    def on_rounddll_clicked(self, b = None):
        if b is None: return
        currll = self.llev.value()
        if currll < 10:
            currll -= 1
        else:
            for d in (10, 100, 1000):
                r = currll % d
                if r != 0:
                    currll -= r
                    break
            else:
                currll -= 1000
        if currll < 0: currll = 1
        self.llev.setValue(currll)

    def on_roundull_clicked(self, b = None):
        if b is None: return
        currll = self.llev.value()
        for d in (10, 100, 1000):
            r = currll % d
            if r != 0:
                currll += d - r
                break
        else:
            currll += 1000
        if  currll > 65535: currll = 65535
        self.llev.setValue(currll)

class Cprocpardlg(QDialog, ui_procpardlg.Ui_procpardlg):
    def __init__(self, parent = None):
        super(Cprocpardlg, self).__init__(parent)
        self.setupUi(self)
        self.acs = []
        for wu in 'ugo':
            for wa in 'rwx':
                self.acs.append('um_' + wu + wa)

    def copyin_jobpar(self, job):
        """Copy fields from job into dialog"""
        self.directory.setText(job.bj_direct)
        for n,p in enumerate(self.acs):
            if (job.bj_umask & (1 << (8-n))) != 0:
                getattr(self, p).setChecked(True)
        ul = job.bj_ulimit
        for n,v in enumerate((0x3ff,0xfffff,0x3fffffff)):
            if ul <= v:
                self.ulimitv.setValue(ul)
                self.ulimitmult.setCurrentIndex(n)
                break
            ul >>= 10
        else:
            self.ulimitv.setValue(0)
            self.ulimitmult.setCurrentIndex(0)
        self.norml.setValue(job.exitn.lower)
        self.normu.setValue(job.exitn.upper)
        self.errl.setValue(job.exite.lower)
        self.erru.setValue(job.exite.upper)
        self.noadverr.setChecked((job.bj_jflags & btclasses.btjob.BJ_NOADVIFERR) != 0)
        ei = 0
        if (job.bj_jflags & btclasses.btjob.BJ_REMRUNNABLE) != 0: ei = 2
        elif (job.bj_jflags & btclasses.btjob.BJ_EXPORT) != 0: ei = 1
        self.exporttype.setCurrentIndex(ei)

    def copyout_jobpar(self, job):
        """Copy results to job"""
        job.bj_direct = str(self.directory.text())
        job.bj_umask = 0
        for n,p in enumerate(self.acs):
            if getattr(self, p).isChecked():
                job.bj_umask |= 1 << (8-n)
        job.bj_ulimit = self.ulimitv.value() << self.ulimitmult.currentIndex() * 10
        l = self.norml.value()
        u = self.normu.value()
        if l > u:
            t = l
            l = u
            u = t
        job.exitn.lower = l
        job.exitn.upper = u
        l = self.errl.value()
        u = self.erru.value()
        if l > u:
            t = l
            l = u
            u = t
        job.exite.lower = l
        job.exite.upper = u
        if self.noadverr.isChecked():
            job.bj_jflags |= btclasses.btjob.BJ_NOADVIFERR
        else:
            job.bj_jflags &= ~btclasses.btjob.BJ_NOADVIFERR
        job.bj_jflags &= ~(btclasses.btjob.BJ_EXPORT|btclasses.btjob.BJ_REMRUNNABLE)
        ei = self.exporttype.currentIndex()
        if ei > 0:
             job.bj_jflags |= btclasses.btjob.BJ_EXPORT
             if ei > 1:
                 job.bj_jflags |= btclasses.btjob.BJ_REMRUNNABLE

class Ctimelimdlg(QDialog, ui_timelimdlg.Ui_timelimdlg):
    def __init__(self, parent = None):
        super(Ctimelimdlg, self).__init__(parent)
        self.setupUi(self)
        for sigs in (("Terminate", 15), ("Kill", 9), ("Hangup", 1), ("Interrupt", 2), ("Quit", 3), ("Alarm", 14), ("Bus", 10), ("Segv", 11)):
            self.termsig.addItem(sigs[0], QVariant(sigs[1]))
        self.termsig.setCurrentIndex(0)

    def set_runt(self, rt):
        self.runts.setValue(rt % 60)
        rt /= 60
        self.runtm.setValue(rt % 60)
        rt /= 60
        if rt > 999: rt = 999
        self.runth.setValue(rt)

    def get_runt(self):
        return (self.runth.value() * 60 + self.runtm.value()) * 60 + self.runts.value()

    def set_gt(self, gt):
        self.gtsec.setValue(gt % 60)
        gt /= 60
        if gt > 500: gt = 500
        self.gtmin.setValue(gt)

    def get_gt(self):
        return self.gtmin.value() * 60 + self.gtsec.value()

    def set_ak(self, ak):
        ind = self.termsig.findData(QVariant(ak))
        if ind < 0: ind = 0
        self.termsig.setCurrentIndex(ind)

    def get_ak(self):
        return self.termsig.itemData(self.termsig.currentIndex()).toInt()[0]

    def on_resetdel_clicked(self, b = None):
        if b is None: return
        self.deltime.setValue(0)

    def on_resetrunt_clicked(self, b = None):
        if b is None: return
        self.runth.setValue(0)
        self.runtm.setValue(0)
        self.runts.setValue(0)

    def on_resetaddt_clicked(self, b = None):
        if b is None: return
        self.gtmin.setValue(0)
        self.gtsec.setValue(0)

class Cmailwrtdlg(QDialog, ui_mailwrtdlg.Ui_mailwrtdlg):
    def __init__(self, parent = None):
        super(Cmailwrtdlg, self).__init__(parent)
        self.setupUi(self)

class Cjpermdlg(QDialog, ui_jpermdlg.Ui_jpermdlg):

    modenames = (("read", btmode.btmode.BTM_READ),
                 ("write", btmode.btmode.BTM_WRITE),
                 ("show", btmode.btmode.BTM_SHOW),
                 ("rdmd", btmode.btmode.BTM_RDMODE),
                 ("wrmd", btmode.btmode.BTM_WRMODE),
                 ("take", btmode.btmode.BTM_UTAKE),
                 ("gtake", btmode.btmode.BTM_GTAKE),
                 ("give", btmode.btmode.BTM_UGIVE),
                 ("ggive", btmode.btmode.BTM_GGIVE),
                 ("del", btmode.btmode.BTM_DELETE),
                 ("kill", btmode.btmode.BTM_KILL))

    def __init__(self, parent = None):
        super(Cjpermdlg, self).__init__(parent)
        self.setupUi(self)

    def init_perms(self, perms):
        """Copy permissions into dialog"""
        for wm in Cjpermdlg.modenames:
            mname, bit = wm
            for ugo in 'ugo':
                if (getattr(perms, ugo + "_flags") & bit) != 0:
                    getattr(self, ugo + mname).setChecked(True)

    def get_perms(self, perms):
        """Extract perms from dialog and plug into mode"""
        perms.u_flags = perms.g_flags = perms.o_flags = 0
        for wm in Cjpermdlg.modenames:
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

class Cjargdlg(QDialog, ui_jargdlg.Ui_jargdlg):
    def __init__(self, parent = None):
        super(Cjargdlg, self).__init__(parent)
        self.setupUi(self)

class Cjargsdlg(QDialog, ui_jargsdlg.Ui_jargsdlg):
    def __init__(self, parent = None):
        super(Cjargsdlg, self).__init__(parent)
        self.setupUi(self)

    def copyin_args(self, j):
        for arg in j.bj_arg:
            self.arglist.addItem(arg)

    def copyout_args(self, j):
        j.bj_arg = []
        for n in range(0, self.arglist.count()):
            j.bj_arg.append(str(self.arglist.item(n).text()))

    def on_newarg_clicked(self, b = None):
        if b is None: return
        dlg = Cjargdlg(self)
        dlg.edjob.setText(self.edjob.text())
        if dlg.exec_():
            self.arglist.addItem(dlg.argument.text())

    def on_delarg_clicked(self, b = None):
        if b is None: return
        carg = self.arglist.currentRow()
        if carg < 0: return
        self.arglist.takeItem(carg)
        if carg >= self.arglist.count(): carg = self.arglist.count()-1
        if carg >= 0:
            self.arglist.setCurrentRow(carg)

    def on_arglist_itemDoubleClicked(self, b = None):
        if b is None: return
        carg = self.arglist.currentRow()
        if carg < 0: return
        dlg = Cjargdlg(self)
        dlg.edjob.setText(self.edjob.text())
        dlg.argument.setText(self.arglist.currentItem().text())
        if dlg.exec_():
            self.arglist.currentItem().setText(dlg.argument.text())

class Cjenvdlg(QDialog, ui_jenvdlg.Ui_jenvdlg):
    def __init__(self, parent = None):
        super(Cjenvdlg, self).__init__(parent)
        self.setupUi(self)

class Cjenvsdlg(QDialog, ui_jenvsdlg.Ui_jenvsdlg):
    def __init__(self, parent = None):
        super(Cjenvsdlg, self).__init__(parent)
        self.setupUi(self)
        self.saveenvs = []

    def checkdlg(self, dlg):
        """Check that the name of the environment variable is given correctly"""
        nam = dlg.ename.text()
        re = QRegExp("[a-zA-Z_]\w*")
        if re.exactMatch(nam): return False
        QMessageBox.warning(self, "Invalid env name", "Invalid env name - " + nam)
        return True

    def copyin_envs(self, j):
        n = 0
        for env in j.bj_env:
            self.saveenvs.append(copy.copy(env))
            wi = QListWidgetItem(env.e_name + '=' + env.e_value)
            wi.setData(Qt.UserRole, QVariant(n))
            self.envlist.addItem(wi)
            n += 1

    def copyout_envs(self, j):
        j.bj_env = []
        for n in range(0, self.envlist.count()):
            wi = self.envlist.item(n)
            n = wi.data(Qt.UserRole).toInt()[0]
            j.bj_env.append(self.saveenvs[n])

    def on_newenv_clicked(self, b = None):
        if b is None: return
        dlg = Cjenvdlg(self)
        dlg.edjob.setText(self.edjob.text())
        while dlg.exec_():
            if self.checkdlg(dlg): continue
            n = len(self.envlist)
            self.saveenvs.append(btclasses.envir(str(dlg.ename.text()), str(dlg.evalue.text())))
            wi = QListWidgetItem(dlg.ename.text() + '=' + dlg.evalue.text())
            wi.setData(Qt.UserRole, QVariant(n))
            self.envlist.addItem(wi)
            return

    def on_delenv_clicked(self, b = None):
        if b is None: return
        cenv = self.envlist.currentRow()
        if cenv < 0: return
        self.envlist.takeItem(cenv)
        if cenv >= self.envlist.count(): cenv = self.envlist.count()-1
        if cenv >= 0:
            self.envlist.setCurrentRow(cenv)

    def on_envlist_itemDoubleClicked(self, b = None):
        if b is None: return
        cenv = self.envlist.currentRow()
        if cenv < 0: return
        dlg = Cjenvdlg(self)
        dlg.edjob.setText(self.edjob.text())
        wi = self.envlist.currentItem()
        n = wi.data(Qt.UserRole).toInt()[0]
        dlg.ename.setText(self.saveenvs[n].e_name)
        dlg.evalue.setText(self.saveenvs[n].e_value)
        while dlg.exec_():
            if self.checkdlg(dlg): continue
            self.saveenvs[n] = btclasses.envir(str(dlg.ename.text()), str(dlg.evalue.text()))
            wi.setText(dlg.ename.text() + '=' + dlg.evalue.text())
            return

class Cjredirdlg(QDialog, ui_jredirdlg.Ui_jredirdlg):

    Stdnames = ('input', 'output', 'error')

    def __init__(self, parent = None):
        super(Cjredirdlg, self).__init__(parent)
        self.setupUi(self)

    def on_fd_valueChanged(self, b = None):
        if not isinstance(b, int) or b < 0: return
        nam = ""
        if b < 3:
            nam = "(Standard " + Cjredirdlg.Stdnames[b] + ")"
        self.expl.setText(nam)

    def on_action_currentIndexChanged(self, b = None):
        if not isinstance(b, int) or b < 0: return
        rdact = b + 1
        if rdact <= btclasses.redir.RD_ACT_PIPEI:
            self.fd2.setEnabled(False)
            self.filename.setEnabled(True)
        elif rdact == btclasses.redir.RD_ACT_CLOSE:
            self.fd2.setEnabled(False)
            self.filename.setEnabled(True)
        else:
            self.fd2.setEnabled(True)
            self.filename.setEnabled(False)

    def extract_redir(self):
        """Extract a new redirection from the dialog"""
        fdn = self.fd.value()
        action = self.action.currentIndex() + 1
        if action >= btclasses.redir.RD_ACT_CLOSE:
            v = self.fd2.value()
        else:
            v = str(self.filename.text())
        return  btclasses.redir(fdn, action, v)

class Cjredirsdlg(QDialog, ui_jredirsdlg.Ui_jredirsdlg):

    redirnames = ('Read', 'Write', 'Append', 'Read/write', 'Read/Append', 'Pipe To', 'Pipe from', 'Close', 'Dup')

    def fmt_redir(self, rd):
        """Format a redirection"""
        res = str(rd.fd) + ' '
        try:
            res += Cjredirsdlg.redirnames[rd.action-1]
        except IndexError:
            res += 'Unknown'
        if rd.action <= btclasses.redir.RD_ACT_PIPEI:
            res += ' ' + rd.filename
        elif rd.action == btclasses.redir.RD_ACT_DUP:
            res += ' ' + str(rd.fd2)
        return res

    def __init__(self, parent = None):
        super(Cjredirsdlg, self).__init__(parent)
        self.setupUi(self)
        self.saveredirs = []
        self.varlist = dict()

    def checkdlg(self, dlg):
        """Check that the filename or fd2 is given correctly"""
        act = dlg.action.currentIndex() + 1
        if act <= btclasses.redir.RD_ACT_PIPEI:
            fn = dlg.filename.text()
            re = QRegExp("\s*")
            if not re.exactMatch(fn): return False
            QMessageBox.warning(self, "No filename given", "No filename given")
            return True
        elif act == btclasses.redir.RD_ACT.DUP:
            if dlg.fd.value() == dlg.fd2.value():
                QMessageBox.warning(self, "Invalid FD", "FDs cannot be the same")
                return True
        return False

    def copyin_redirs(self, j):
        n = 0
        for redir in j.bj_redirs:
            self.saveredirs.append(copy.copy(redir))
            wi = QListWidgetItem(self.fmt_redir(redir))
            wi.setData(Qt.UserRole, QVariant(n))
            self.redirlist.addItem(wi)
            n += 1

    def copyout_redirs(self, j):
        j.bj_redirs = []
        for n in range(0, self.redirlist.count()):
            wi = self.redirlist.item(n)
            n = wi.data(Qt.UserRole).toInt()[0]
            j.bj_redirs.append(self.saveredirs[n])

    def on_newredir_clicked(self, b = None):
        if b is None: return
        dlg = Cjredirdlg(self)
        dlg.edjob.setText(self.edjob.text())
        dlg.fd.setValue(0)      # Make it display std input
        while dlg.exec_():
            if self.checkdlg(dlg): continue
            n = len(self.redirlist)
            rdir = dlg.extract_redir()
            self.saveredirs.append(rdir)
            wi = QListWidgetItem(self.fmt_redir(rdir))
            wi.setData(Qt.UserRole, QVariant(n))
            self.redirlist.addItem(wi)
            return

    def on_delredir_clicked(self, b = None):
        if b is None: return
        credir = self.redirlist.currentRow()
        if credir < 0: return
        self.redirlist.takeItem(credir)
        if credir >= self.redirlist.count(): credir = self.redirlist.count()-1
        if credir >= 0:
            self.redirlist.setCurrentRow(credir)

    def on_redirlist_itemDoubleClicked(self, b = None):
        if b is None: return
        credir = self.redirlist.currentRow()
        if credir < 0: return
        dlg = Cjredirdlg(self)
        dlg.edjob.setText(self.edjob.text())
        wi = self.redirlist.currentItem()
        n = wi.data(Qt.UserRole).toInt()[0]
        rd = self.saveredirs[n]
        dlg.fd.setValue(rd.fd)
        dlg.action.setCurrentIndex(rd.action-1)
        dlg.filename.setText(rd.filename)
        dlg.fd2.setValue(rd.fd2)
        while dlg.exec_():
            if self.checkdlg(dlg): continue
            rdir = dlg.extract_redir()
            self.saveredirs[n] = rdir
            wi.setText(self.fmt_redir(rdir))
            return

class Cjcondsdlg(QDialog, ui_jcondsdlg.Ui_jcondsdlg):

    def __init__(self, parent = None):
        super(Cjcondsdlg, self).__init__(parent)
        self.setupUi(self)
        self.varlist = dict()

    def on_condtext0_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval0.setEnabled(True)
            self.intval0.setEnabled(False)
        else:
            self.textval0.setEnabled(False)
            self.intval0.setEnabled(True)

    def on_condtext1_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval1.setEnabled(True)
            self.intval0.setEnabled(False)
        else:
            self.textval1.setEnabled(False)
            self.intval1.setEnabled(True)

    def on_condtext2_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval2.setEnabled(True)
            self.intval2.setEnabled(False)
        else:
            self.textval2.setEnabled(False)
            self.intval2.setEnabled(True)

    def on_condtext3_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval3.setEnabled(True)
            self.intval3.setEnabled(False)
        else:
            self.textval3.setEnabled(False)
            self.intval3.setEnabled(True)

    def on_condtext4_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval4.setEnabled(True)
            self.intval4.setEnabled(False)
        else:
            self.textval4.setEnabled(False)
            self.intval4.setEnabled(True)

    def on_condtext5_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval5.setEnabled(True)
            self.intval5.setEnabled(False)
        else:
            self.textval5.setEnabled(False)
            self.intval5.setEnabled(True)

    def on_condtext6_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval6.setEnabled(True)
            self.intval6.setEnabled(False)
        else:
            self.textval6.setEnabled(False)
            self.intval6.setEnabled(True)

    def on_condtext7_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval7.setEnabled(True)
            self.intval7.setEnabled(False)
        else:
            self.textval7.setEnabled(False)
            self.intval7.setEnabled(True)

    def on_condtext8_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval8.setEnabled(True)
            self.intval8.setEnabled(False)
        else:
            self.textval8.setEnabled(False)
            self.intval8.setEnabled(True)

    def on_condtext9_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval9.setEnabled(True)
            self.intval9.setEnabled(False)
        else:
            self.textval9.setEnabled(False)
            self.intval9.setEnabled(True)

    def init_varlist(self, vl):
        """Initialise the list of variables to choose from"""
        for ip, vars in vl.iteritems():
            hname = btqwopts.Options.servers.look_hostid(ip)
            for v in vars:
                vn = hname + ':' + v.var_name
                self.varlist[vn] = v
        svl = self.varlist.keys()
        svl.sort()
        for v in svl:
            for n in range(0, 10):
                getattr(self, 'condvar' + str(n)).addItem(v)

    def init_tvarlist(self, vl):
        """Initialise the list of variables to choose from (text list)"""
        for v in vl:
            for n in range(0, 10):
                getattr(self, 'condvar' + str(n)).addItem(v)

    def copyin_conds(self, j):
        """Copy existing conditions in"""
        n = 0
        for c in j.bj_conds:
            try:
                vn = jobmodel.ca_varname(c.bjc_varind)
                cv = getattr(self, 'condvar' + str(n))
                ind = cv.findText(vn, Qt.MatchFixedString|Qt.MatchCaseSensitive)
                if ind < 0:
                    ind = cv.count()
                    cv.addItem(vn)
                cv.setCurrentIndex(ind)
                getattr(self, "condop" + str(n)).setCurrentIndex(c.bjc_compar)
                if c.bjc_iscrit != 0:
                    getattr(self, "cc" + str(n)).setChecked(True)
                if c.bjc_value.isint:
                    getattr(self, "intval" + str(n)).setValue(c.bjc_value.value)
                else:
                    getattr(self, "condtext" + str(n)).setChecked(True)
                    getattr(self, "textval" + str(n)).setText(str(c.bjc_value.value))
                n += 1
            except (KeyError, IndexError):
                pass

    def copyin_tconds(self, j):
        """Copy conditions in when held as text format (as in btrw)"""
        n = 0
        for c in j.bj_conds:
            vn = c.bjc_varname
            cv = getattr(self, 'condvar' + str(n))
            ind = cv.findText(vn, Qt.MatchFixedString|Qt.MatchCaseSensitive)
            if ind < 0:
                ind = cv.count()
                cv.addItem(vn)
            cv.setCurrentIndex(ind)
            getattr(self, "condop" + str(n)).setCurrentIndex(c.bjc_compar)
            if c.bjc_iscrit != 0:
                getattr(self, "cc" + str(n)).setChecked(True)
            if c.bjc_value.isint:
                getattr(self, "intval" + str(n)).setValue(c.bjc_value.value)
            else:
                getattr(self, "condtext" + str(n)).setChecked(True)
                getattr(self, "textval" + str(n)).setText(str(c.bjc_value.value))
            n += 1

    def copyout_conds(self, j):
        """Copy conditions to job"""
        j.bj_conds = []
        for n in range(0, 10):
            co = getattr(self, "condop" + str(n))
            cv = getattr(self, "condvar" + str(n))
            if co.currentIndex() <= 0  or cv.currentIndex() <= 0: continue
            if getattr(self, "condtext" + str(n)).isChecked():
                val = str(getattr(self, "textval" + str(n)).text())
            else:
                val = getattr(self, "intval" + str(n)).value()
            c = btclasses.jcond()
            c.bjc_compar = co.currentIndex()
            try:
                c.bjc_varind = self.varlist[str(cv.currentText())]
            except KeyError:
                continue
            c.bjc_value = btclasses.btconst(val)
            if getattr(self, "cc" + str(n)).isChecked():
                c.bjc_iscrit = 1
            j.bj_conds.append(c)

    def copyout_tconds(self, j):
        """Copy conditions in text format to job"""
        j.bj_conds = []
        for n in range(0, 10):
            co = getattr(self, "condop" + str(n))
            cv = getattr(self, "condvar" + str(n))
            if co.currentIndex() <= 0  or cv.currentIndex() <= 0: continue
            if getattr(self, "condtext" + str(n)).isChecked():
                val = str(getattr(self, "textval" + str(n)).text())
            else:
                val = getattr(self, "intval" + str(n)).value()
            c = btclasses.jcond()
            c.bjc_compar = co.currentIndex()
            c.bjc_varname = str(cv.currentText())
            c.bjc_value = btclasses.btconst(val)
            if getattr(self, "cc" + str(n)).isChecked():
                c.bjc_iscrit = 1
            j.bj_conds.append(c)

class Cjassdlg(QDialog, ui_jassdlg.Ui_jassdlg):

    def __init__(self, parent = None):
        super(Cjassdlg, self).__init__(parent)
        self.setupUi(self)
        self.varlist = dict()

    def on_asstext0_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval0.setEnabled(True)
            self.intval0.setEnabled(False)
        else:
            self.textval0.setEnabled(False)
            self.intval0.setEnabled(True)

    def on_asstext1_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval1.setEnabled(True)
            self.intval0.setEnabled(False)
        else:
            self.textval1.setEnabled(False)
            self.intval1.setEnabled(True)

    def on_asstext2_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval2.setEnabled(True)
            self.intval2.setEnabled(False)
        else:
            self.textval2.setEnabled(False)
            self.intval2.setEnabled(True)

    def on_asstext3_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval3.setEnabled(True)
            self.intval3.setEnabled(False)
        else:
            self.textval3.setEnabled(False)
            self.intval3.setEnabled(True)

    def on_asstext4_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval4.setEnabled(True)
            self.intval4.setEnabled(False)
        else:
            self.textval4.setEnabled(False)
            self.intval4.setEnabled(True)

    def on_asstext5_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval5.setEnabled(True)
            self.intval5.setEnabled(False)
        else:
            self.textval5.setEnabled(False)
            self.intval5.setEnabled(True)

    def on_asstext6_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval6.setEnabled(True)
            self.intval6.setEnabled(False)
        else:
            self.textval6.setEnabled(False)
            self.intval6.setEnabled(True)

    def on_asstext7_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.textval7.setEnabled(True)
            self.intval7.setEnabled(False)
        else:
            self.textval7.setEnabled(False)
            self.intval7.setEnabled(True)

    def on_assop0_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext0.isEnabled():
                self.intval0.setEnabled(True)
                self.asstext0.setEnabled(True)
                self.FS0.setEnabled(True)
                self.FN0.setEnabled(True)
                self.FE0.setEnabled(True)
                self.FA0.setEnabled(True)
                self.FC0.setEnabled(True)
                self.FR0.setEnabled(True)
        else:
            self.intval0.setEnabled(False)
            self.asstext0.setChecked(False)
            self.asstext0.setEnabled(False)
            self.FS0.setEnabled(False)
            self.FN0.setEnabled(False)
            self.FE0.setEnabled(False)
            self.FA0.setEnabled(False)
            self.FC0.setEnabled(False)
            self.FR0.setEnabled(False)

    def on_assop1_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext1.isEnabled():
                self.intval1.setEnabled(True)
                self.asstext1.setEnabled(True)
                self.FS1.setEnabled(True)
                self.FN1.setEnabled(True)
                self.FE1.setEnabled(True)
                self.FA1.setEnabled(True)
                self.FC1.setEnabled(True)
                self.FR1.setEnabled(True)
        else:
            self.intval1.setEnabled(False)
            self.asstext1.setChecked(False)
            self.asstext1.setEnabled(False)
            self.FS1.setEnabled(False)
            self.FN1.setEnabled(False)
            self.FE1.setEnabled(False)
            self.FA1.setEnabled(False)
            self.FC1.setEnabled(False)
            self.FR1.setEnabled(False)

    def on_assop2_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext2.isEnabled():
                self.intval2.setEnabled(True)
                self.asstext2.setEnabled(True)
                self.FS2.setEnabled(True)
                self.FN2.setEnabled(True)
                self.FE2.setEnabled(True)
                self.FA2.setEnabled(True)
                self.FC2.setEnabled(True)
                self.FR2.setEnabled(True)
        else:
            self.intval2.setEnabled(False)
            self.asstext2.setChecked(False)
            self.asstext2.setEnabled(False)
            self.FS2.setEnabled(False)
            self.FN2.setEnabled(False)
            self.FE2.setEnabled(False)
            self.FA2.setEnabled(False)
            self.FC2.setEnabled(False)
            self.FR2.setEnabled(False)

    def on_assop3_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext3.isEnabled():
                self.intval3.setEnabled(True)
                self.asstext3.setEnabled(True)
                self.FS3.setEnabled(True)
                self.FN3.setEnabled(True)
                self.FE3.setEnabled(True)
                self.FA3.setEnabled(True)
                self.FC3.setEnabled(True)
                self.FR3.setEnabled(True)
        else:
            self.intval3.setEnabled(False)
            self.asstext3.setChecked(False)
            self.asstext3.setEnabled(False)
            self.FS3.setEnabled(False)
            self.FN3.setEnabled(False)
            self.FE3.setEnabled(False)
            self.FA3.setEnabled(False)
            self.FC3.setEnabled(False)
            self.FR3.setEnabled(False)

    def on_assop4_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext4.isEnabled():
                self.intval4.setEnabled(True)
                self.asstext4.setEnabled(True)
                self.FS4.setEnabled(True)
                self.FN4.setEnabled(True)
                self.FE4.setEnabled(True)
                self.FA4.setEnabled(True)
                self.FC4.setEnabled(True)
                self.FR4.setEnabled(True)
        else:
            self.intval4.setEnabled(False)
            self.asstext4.setChecked(False)
            self.asstext4.setEnabled(False)
            self.FS4.setEnabled(False)
            self.FN4.setEnabled(False)
            self.FE4.setEnabled(False)
            self.FA4.setEnabled(False)
            self.FC4.setEnabled(False)
            self.FR4.setEnabled(False)

    def on_assop5_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext5.isEnabled():
                self.intval5.setEnabled(True)
                self.asstext5.setEnabled(True)
                self.FS5.setEnabled(True)
                self.FN5.setEnabled(True)
                self.FE5.setEnabled(True)
                self.FA5.setEnabled(True)
                self.FC5.setEnabled(True)
                self.FR5.setEnabled(True)
        else:
            self.intval5.setEnabled(False)
            self.asstext5.setChecked(False)
            self.asstext5.setEnabled(False)
            self.FS5.setEnabled(False)
            self.FN5.setEnabled(False)
            self.FE5.setEnabled(False)
            self.FA5.setEnabled(False)
            self.FC5.setEnabled(False)
            self.FR5.setEnabled(False)

    def on_assop6_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext6.isEnabled():
                self.intval6.setEnabled(True)
                self.asstext6.setEnabled(True)
                self.FS6.setEnabled(True)
                self.FN6.setEnabled(True)
                self.FE6.setEnabled(True)
                self.FA6.setEnabled(True)
                self.FC6.setEnabled(True)
                self.FR6.setEnabled(True)
        else:
            self.intval6.setEnabled(False)
            self.asstext6.setChecked(False)
            self.asstext6.setEnabled(False)
            self.FS6.setEnabled(False)
            self.FN6.setEnabled(False)
            self.FE6.setEnabled(False)
            self.FA6.setEnabled(False)
            self.FC6.setEnabled(False)
            self.FR6.setEnabled(False)

    def on_assop7_currentIndexChanged(self, b = None):
        if not isinstance(b, int): return
        if b < btclasses.jass.BJA_SEXIT:
            if not self.asstext7.isEnabled():
                self.intval7.setEnabled(True)
                self.asstext7.setEnabled(True)
                self.FS7.setEnabled(True)
                self.FN7.setEnabled(True)
                self.FE7.setEnabled(True)
                self.FA7.setEnabled(True)
                self.FC7.setEnabled(True)
                self.FR7.setEnabled(True)
        else:
            self.intval7.setEnabled(False)
            self.asstext7.setChecked(False)
            self.asstext7.setEnabled(False)
            self.FS7.setEnabled(False)
            self.FN7.setEnabled(False)
            self.FE7.setEnabled(False)
            self.FA7.setEnabled(False)
            self.FC7.setEnabled(False)
            self.FR7.setEnabled(False)

    def init_varlist(self, vl):
        """Initialise the list of variables to choose from"""
        for ip, vars in vl.iteritems():
            hname = btqwopts.Options.servers.look_hostid(ip)
            for v in vars:
                vn = hname + ':' + v.var_name
                self.varlist[vn] = v
        svl = self.varlist.keys()
        svl.sort()
        for v in svl:
            for n in range(0, 8):
                getattr(self, 'assvar' + str(n)).addItem(v)

    def init_tvarlist(self, vl):
        """Initialise the list of variables to choose from (text list)"""
        for v in vl:
            for n in range(0, 8):
                getattr(self, 'assvar' + str(n)).addItem(v)

    def copyin_ass(self, j):
        """Copy existing assignments in"""
        n = 0
        for a in j.bj_asses:
            try:
                vn = jobmodel.ca_varname(a.bja_varind)
                cv = getattr(self, 'assvar' + str(n))
                ind = cv.findText(vn, Qt.MatchFixedString|Qt.MatchCaseSensitive)
                if ind < 0:
                    ind = cv.count()
                    cv.addItem(vn)
                cv.setCurrentIndex(ind)
                getattr(self, "assop" + str(n)).setCurrentIndex(a.bja_op)
                if a.bja_iscrit != 0:
                    getattr(self, "ac" + str(n)).setChecked(True)
                if a.bja_con.isint:
                    getattr(self, "intval" + str(n)).setValue(a.bja_con.value)
                else:
                    getattr(self, "asstext" + str(n)).setChecked(True)
                    getattr(self, "textval" + str(n)).setText(str(a.bja_con.value))
                if a.bja_op < btclasses.jass.BJA_SEXIT:
                    if (a.bja_flags & btclasses.jass.BJA_START) != 0: getattr(self, "FS" + str(n)).setChecked(True)
                    if (a.bja_flags & btclasses.jass.BJA_OK) != 0: getattr(self, "FN" + str(n)).setChecked(True)
                    if (a.bja_flags & btclasses.jass.BJA_ERROR) != 0: getattr(self, "FE" + str(n)).setChecked(True)
                    if (a.bja_flags & btclasses.jass.BJA_ABORT) != 0: getattr(self, "FA" + str(n)).setChecked(True)
                    if (a.bja_flags & btclasses.jass.BJA_CANCEL) != 0: getattr(self, "FC" + str(n)).setChecked(True)
                    if (a.bja_flags & btclasses.jass.BJA_REVERSE) != 0: getattr(self, "FR" + str(n)).setChecked(True)
                n += 1
            except (KeyError, IndexError):
                pass

    def copyin_tass(self, j):
        """Copy existing assignments in in text format"""
        for n,a in enumerate(j.bj_asses):
            vn = a.bja_varname
            cv = getattr(self, 'assvar' + str(n))
            ind = cv.findText(vn, Qt.MatchFixedString|Qt.MatchCaseSensitive)
            if ind < 0:
                ind = cv.count()
                cv.addItem(vn)
            cv.setCurrentIndex(ind)
            getattr(self, "assop" + str(n)).setCurrentIndex(a.bja_op)
            if a.bja_iscrit != 0:
                getattr(self, "ac" + str(n)).setChecked(True)
            if a.bja_con.isint:
                getattr(self, "intval" + str(n)).setValue(a.bja_con.value)
            else:
                getattr(self, "asstext" + str(n)).setChecked(True)
                getattr(self, "textval" + str(n)).setText(str(a.bja_con.value))
            if a.bja_op < btclasses.jass.BJA_SEXIT:
                if (a.bja_flags & btclasses.jass.BJA_START) != 0: getattr(self, "FS" + str(n)).setChecked(True)
                if (a.bja_flags & btclasses.jass.BJA_OK) != 0: getattr(self, "FN" + str(n)).setChecked(True)
                if (a.bja_flags & btclasses.jass.BJA_ERROR) != 0: getattr(self, "FE" + str(n)).setChecked(True)
                if (a.bja_flags & btclasses.jass.BJA_ABORT) != 0: getattr(self, "FA" + str(n)).setChecked(True)
                if (a.bja_flags & btclasses.jass.BJA_CANCEL) != 0: getattr(self, "FC" + str(n)).setChecked(True)
                if (a.bja_flags & btclasses.jass.BJA_REVERSE) != 0: getattr(self, "FR" + str(n)).setChecked(True)

    def copyout_ass(self, j):
        """Copy assignments to job"""
        j.bj_asses = []
        for n in range(0, 8):
            co = getattr(self, "assop" + str(n))
            cv = getattr(self, "assvar" + str(n))
            if co.currentIndex() <= 0  or cv.currentIndex() <= 0: continue
            if getattr(self, "asstext" + str(n)).isChecked():
                val = str(getattr(self, "textval" + str(n)).text())
            else:
                val = getattr(self, "intval" + str(n)).value()
            a = btclasses.jass()
            a.bja_op = co.currentIndex()
            try:
                a.bja_varind = self.varlist[str(cv.currentText())]
            except KeyError:
                continue
            a.bja_con = btclasses.btconst(val)
            if getattr(self, "ac" + str(n)).isChecked():
                a.bja_iscrit = 1
            if a.bja_op < btclasses.jass.BJA_SEXIT:
                if getattr(self, "FS" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_START
                if getattr(self, "FN" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_OK
                if getattr(self, "FE" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_ERROR
                if getattr(self, "FA" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_ABORT
                if getattr(self, "FC" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_CANCEL
                if getattr(self, "FR" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_REVERSE
            j.bj_asses.append(a)

    def copyout_tass(self, j):
        """Copy assignments back into job, text mode"""
        j.bj_asses = []
        for n in range(0, 8):
            co = getattr(self, "assop" + str(n))
            cv = getattr(self, "assvar" + str(n))
            if co.currentIndex() <= 0  or cv.currentIndex() <= 0: continue
            if getattr(self, "asstext" + str(n)).isChecked():
                val = str(getattr(self, "textval" + str(n)).text())
            else:
                val = getattr(self, "intval" + str(n)).value()
            a = btclasses.jass()
            a.bja_op = co.currentIndex()
            a.bja_varname = str(cv.currentText())
            a.bja_con = btclasses.btconst(val)
            if getattr(self, "ac" + str(n)).isChecked():
                a.bja_iscrit = 1
            if a.bja_op < btclasses.jass.BJA_SEXIT:
                if getattr(self, "FS" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_START
                if getattr(self, "FN" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_OK
                if getattr(self, "FE" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_ERROR
                if getattr(self, "FA" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_ABORT
                if getattr(self, "FC" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_CANCEL
                if getattr(self, "FR" + str(n)).isChecked(): a.bja_flags |= btclasses.jass.BJA_REVERSE
            j.bj_asses.append(a)
