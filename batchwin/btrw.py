#! /usr/bin/python

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

import sys
import os
import os.path
import copy
import re
import socket
import string
import locale
import time
import sip
import platform
import tempfile
import subprocess

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *

import resources_rc
import version
import ui_btrw_main
import ui_btrprogopts
import ui_jobviewdlg
import ui_jobeditdlg
import ui_submhdlg
import btuser
import btmode
import cmdint
import btclasses
import dispopts
import btqwopts
import gbserver
import gbclient
import uaclient
import gbident
import jobldsv
import jobparams
import jobsubmit
import ctimedlgs

DIR_COLUMN = 0
FILE_COLUMN = 1
QUEUE_COLUMN = 2
TITLE_COLUMN = 3
CI_COLUMN = 4
HAS_COLUMN = 5
RUNC_COLUMN = 6
SUBM_COLUMN = 7
CH_COLUMN = 8

locale.setlocale(locale.LC_ALL, os.getenv('LANG', 'C'))

if btqwopts.WIN:
    import wininfo
    import win32api
    filelocloc = "Software\\FSF\\GNUBatch\\serverdets"
    def savelocloc(fname):
        """Save location of current config file"""
        try:
            wininfo.saveregstring(filelocloc, fname)
        except wininfo.winerror as e:
            QMessageBox.warning(mw, "Save registry error", "Could not save config file location in registry")
else:
    import pwd
    def savelocloc(fname):
        """Save location of current config file"""
        try:
            fll = open(filelocloc,'w')
            fll.write(fname + '\n')
            fll.close()
        except IOError:
            QMessageBox.warning(mw, "Save file error", "Could not save config file location")

def isAlive(qobj):
    """Report if window has been deleted or not"""
    try:
        sip.unwrapinstance(qobj)
    except RuntimeError:
        return False
    return True

class Cjobview(QDialog, ui_jobviewdlg.Ui_jobviewdlg):
    def __init__(self, parent = None):
        super(Cjobview, self).__init__(parent)
        self.setupUi(self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        if parent:
            self.destroyed.connect(parent.viewkilled)

class Cjobedit(QDialog, ui_jobeditdlg.Ui_jobeditdlg):
    def __init__(self, parent = None):
        super(Cjobedit, self).__init__(parent)
        self.setupUi(self)

class Cprogopts(QDialog, ui_btrprogopts.Ui_btrwprogopts):
    def __init__(self, parent = None):
        super(Cprogopts, self).__init__(parent)
        self.setupUi(self)

    def on_useexted_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.edname.setEnabled(True)
            self.interm.setEnabled(True)
            if self.interm.isChecked():
                self.shell.setEnabled(True)
                self.shellarg.setEnabled(True)
            else:
                self.shell.setEnabled(False)
                self.shellarg.setEnabled(False)
        else:
            self.edname.setEnabled(False)
            self.interm.setEnabled(False)
            self.shell.setEnabled(False)
            self.shellarg.setEnabled(False)

    def on_interm_stateChanged(self, b = None):
        if b is None: return
        if b != Qt.Unchecked:
            self.shell.setEnabled(True)
            self.shellarg.setEnabled(True)
        else:
            self.shell.setEnabled(False)
            self.shellarg.setEnabled(False)

    def on_shell_editTextChanged(self, b = None):
         if not isinstance(b, QString): return
         if b == "gnome-terminal":
             self.shellarg.setText("--disable-factory -x")
         else:
             self.shellarg.setText("-e")

class Csubmhdlg(QDialog, ui_submhdlg.Ui_submhdlg):
    def __init__(self, parent = None):
        super(Csubmhdlg, self).__init__(parent)
        self.setupUi(self)

class BtrwMainwin(QMainWindow, ui_btrw_main.Ui_MainWindow):

    def __init__(self):
        super(BtrwMainwin, self).__init__(None)
        # Get UI stuff - this will be changed to get user's config
        self.setupUi(self)
        self.setCentralWidget(self.joblist)
        self.jobs = []
        self.viewdlglist = set()
        self.cilist = None
        self.servperms = None
        self.initing = True
        self.connect(self.joblist, SIGNAL('customContextMenuRequested(const QPoint&)'), self.on_context_menu)

    def viewjob(self):
        self.on_action_view_script_triggered(True)

    def editjob(self):
        self.on_action_edit_script_triggered(True)

    def subjob(self):
        self.on_action_submit_locally_triggered(True)

    def titprill(self):
        self.on_action_job_titprill_triggered(True)

    def procpar(self):
        self.on_action_def_procpar_triggered(True)

    def savejob(self):
        self.on_action_save_triggered(True)

    def setrun(self):
        self.on_action_job_setrun_triggered(True)

    def setcanc(self):
        self.on_action_job_setcanc_triggered(True)

    def on_context_menu(self, point):
        item = self.joblist.itemAt(point)
        if item is None: return
        menu = QMenu(self)
        menu.addAction("&View Job Script").triggered.connect(self.viewjob)
        menu.addAction("&Edit Job script").triggered.connect(self.editjob)
        menu.addSeparator()
        menu.addAction("Submit &job").triggered.connect(self.subjob)
        menu.addSeparator()
        menu.addAction("&Title, priority, cmd int").triggered.connect(self.titprill)
        menu.addAction("&Process parameters").triggered.connect(self.procpar)
        menu.addSeparator()
        menu.addAction("&Save job").triggered.connect(self.savejob)
        menu.addSeparator()
        menu.addAction("Set &runnable").triggered.connect(self.setrun)
        menu.addAction("Set &cancelled").triggered.connect(self.setcanc)
        menu.exec_(self.joblist.mapToGlobal(point))

    def on_joblist_currentItemChanged(self, curr, prev):
        self.updateUI()

    def enableall(self, setting):
        """Enable/disable all the menu items relevant to jobs"""
        self.action_save.setEnabled(setting)
        self.action_save_as.setEnabled(setting)
        self.action_close.setEnabled(setting)
        self.action_closedel.setEnabled(setting)
        self.action_view_script.setEnabled(setting)
        self.action_edit_script.setEnabled(setting)
        self.action_job_set_time.setEnabled(setting)
        self.action_job_titprill.setEnabled(setting)
        self.action_job_procpar.setEnabled(setting)
        self.action_job_timelim.setEnabled(setting)
        self.action_job_mailwrite.setEnabled(setting)
        self.action_job_perms.setEnabled(setting)
        self.action_job_args.setEnabled(setting)
        self.action_job_env.setEnabled(setting)
        self.action_job_redirs.setEnabled(setting)
        self.action_job_conds.setEnabled(setting)
        self.action_job_asses.setEnabled(setting)
        self.action_job_setcanc.setEnabled(setting)
        self.action_job_setrun.setEnabled(setting)
        self.action_submit_locally.setEnabled(setting)
        self.action_submit_remote.setEnabled(setting)

    def updateUI(self):
        """Disable intapplicable menu entries"""
        crow = self.joblist.currentRow()
        if self.joblist.rowCount() == 0 or crow < 0:
            self.enableall(False)
            return
        self.enableall(True)
        cjob = self.jobs[crow]
        if not cjob.dirty:
            self.action_save.setEnabled(False)
            self.action_save_as.setEnabled(False)
        if cjob.submitted or cjob.script is None:
            self.action_submit_locally.setEnabled(False)
            self.action_submit_remote.setEnabled(False)
        if cjob.script is None:
            self.action_view_script.setEnabled(False)

    def viewkilled(self, qobj):
        """Note deleted view"""
        self.viewdlglist = set([w for w in self.viewdlglist if isAlive(w)])

    def get_cwidths(self):
        """Get vector of column widths"""
        result = []
        for c in range(0, self.joblist.columnCount()):
            result.append(self.joblist.columnWidth(c))
        return result

    def set_cwidths(self, cws):
        """Reset column widths"""
        for nc in enumerate(cws):
            n,w = nc
            self.joblist.setColumnWidth(n, w)

    def unsaved_jobs(self):
        """Indicate if any jobs were unsaved"""
        for j in self.jobs:
            if j.dirty: return True
        return False

    def get_recentfiles(self):
        """Get list of recent files to reopen next time"""
        result = []
        for j in self.jobs:
            if j.filename is not None and len(j.filename) != 0:
                result.append(j.filename)
        return result

    def load_recentfiles(self, flist):
        """Load up recent files into list at start

Generally ignore errors"""
        for f in flist:
            job = jobldsv.jobitem()
            try:
                job.jobload(f)
            except jobldsv.JobSaveExcept:
                continue
            self.jobs.append(job)
            self.append_job(job)

    def oktoclose(self):
        """Check user really meant to close"""
        if self.unsaved_jobs() and QMessageBox.question(self, "Please confirm quit", "There are unsaved changes, are you sure you want to quit",
                QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return False
        return True

    def getprefserv(self):
        """Get server structure for preferred host"""
        hostn = btqwopts.Options.btrwopts.prefhost
        if hostn is None or len(hostn) == 0:
            QMessageBox.warning(self, "No default host", "You need to have a default host set up to get permissions from")
            return None
        try:
            serv = btqwopts.Options.servers.serversanyname[hostn]
        except KeyError:
            QMessageBox.warning(self, "Unknown server", "The default server name of " + hostn + " is not known")
            return None
        if serv.isok or btqwopts.Options.servers.perform_login(self, serv):
            return serv
        QMessageBox.warning(self, "Cannot get permissions", "Cannot get permissions from default server " + hostn)
        return None

    def getservperms(self):
        """Get perms structure for server"""
        if self.servperms is not None: return self.servperms
        serv = self.getprefserv()
        if serv is not None:
            self.servperms = serv.perms
            return serv.perms
        return None

    def listvars_perm(self, perm):
        """List variables known as a dictionary of lists by IP satisfying the permission given"""
        serv = self.getprefserv()
        if serv is None: return []
        vars = serv.uasock.getvlist(perm)
        # Put pref server anem in front of ones without a :
        cvars = [v for v in vars if ':' in v]
        ncvars = [v for v in vars if ':' not in v]
        hostn = btqwopts.Options.btrwopts.prefhost
        for v in map(lambda x: hostn + ':' + x, ncvars):
            cvars.append(v)
        cvars.sort()
        return cvars

    def getdefaultjob(self):
        """Get default job from options, allocating if needed"""
        j = btqwopts.Options.btrwopts.defjob
        if j is None:
            serv = self.getprefserv()
            if serv is None: return None
            j = btqwopts.Options.btrwopts.defjob = btclasses.btjob()
            perms = serv.perms
            j.bj_mode.u_flags, j.bj_mode.g_flags, j.bj_mode.o_flags = perms.btu_jflags
            j.bj_umask, j.bj_ulimit = serv.uasock.getuml()
            for ep in serv.uasock.getelist():
                n,v = ep
                j.bj_env.append(btclasses.envir(n,v))
            if self.cilist is None:
                self.cilist =  btqwopts.Options.servers.get_cilist()
            try:
                d = dict()
                for si in self.cilist:
                    d[si] = si;
                ci = d[cmdint.cmdint('sh')]
            except KeyError:
                ci = cmdint.cmdint('sh')
            j.bj_cmdinterp = ci.name
            j.bj_ll = ci.ll
        return j

    def getselectedjob(self):
        """Return a job structure corresponding to selected row"""
        row = self.joblist.currentRow()
        if row < 0: return None
        return self.jobs[row]

    def setup_checkbox(self, pos, col, value, icontrue, iconfalse):
        """Set up a checkbox in a table column"""
        if value: icon = icontrue
        else: icon = iconfalse
        item = QTableWidgetItem()
        item.setIcon(QIcon(":/%s" % icon))
        self.joblist.setItem(pos, col, item)

    def setup_stringbox(self, pos, col, value):
        """Set up a string box in a table column"""
        item = QTableWidgetItem(value)
        item.setFlags(Qt.ItemIsSelectable|Qt.ItemIsEnabled)
        self.joblist.setItem(pos, col, item)

    def fillrow(self, job, rownum = None):
        """Fill in fields in the row from the job"""
        if rownum is None:
            rownum = self.joblist.currentRow()
            if rownum < 0: return
        if job.filename is not None:
            dir, file = os.path.split(job.filename)
            self.setup_stringbox(rownum, DIR_COLUMN, dir)
            self.setup_stringbox(rownum, FILE_COLUMN, file)
        else:
            self.setup_stringbox(rownum, DIR_COLUMN, "")
            self.setup_stringbox(rownum, FILE_COLUMN, "")
        qname, tit = job.queue_title()
        self.setup_stringbox(rownum, QUEUE_COLUMN, qname)
        self.setup_stringbox(rownum, TITLE_COLUMN, tit)
        self.setup_stringbox(rownum, CI_COLUMN, job.bj_cmdinterp)
        st = "Run"
        if job.bj_progress == btclasses.btjob.BJP_CANCELLED: st = "Canc"
        self.setup_stringbox(rownum, RUNC_COLUMN, st)
        self.setup_checkbox(rownum, HAS_COLUMN, job.script is not None, "Has script", "No script")
        self.setup_checkbox(rownum, SUBM_COLUMN, job.submitted, "Submitted", "Unsubmitted")
        self.setup_checkbox(rownum, CH_COLUMN, job.dirty, "Changed", "Saved")

    def append_job(self, job):
        """Append a job (given as a jobitem) to the job list"""
        nrows = self.joblist.rowCount()
        self.joblist.insertRow(nrows)
        self.fillrow(job, nrows)

    def on_action_new_job_triggered(self, checked = None):
        if checked is None: return
        job = jobldsv.jobitem(self.getdefaultjob())
        self.jobs.append(job)
        self.append_job(job)

    def on_action_open_job_triggered(self, checked = None):
        if checked is None: return
        fpath = str(QFileDialog.getOpenFileName(self, self.tr("Select job file"), btqwopts.Options.savejobdir, self.tr("Saved GNUBatch jobs (*.gbj1)")))
        if len(fpath) == 0: return
        btqwopts.Options.savejobdir = os.path.dirname(fpath)
        job = jobldsv.jobitem()
        try:
            job.jobload(fpath)
        except jobldsv.JobSaveExcept as err:
            QMessageBox.warning(self, "Load job error", err.args[0])
            return
        self.jobs.append(job)
        self.append_job(job)

    def on_action_save_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not cjob.dirty: return
        if cjob.filename is None or len(cjob.filename) == 0:
            self.on_action_save_as_triggered(checked)
            return
        try:
            cjob.jobsave(cjob.filename)
        except jobldsv.JobSaveExcept as err:
            QMessageBox.warning(self, "Job save error", err.args[0])
            return
        self.fillrow(cjob)
        self.updateUI()

    def on_action_save_as_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not cjob.dirty: return
        oldf = cjob.filename
        if oldf is None or len(oldf) == 0:
            oldf =  btqwopts.Options.savejobdir + "/untitled.gbj1"
        newf = QFileDialog.getSaveFileName(self, self.tr("Saved job to file"), oldf, self.tr("Saved GNUBatch jobs (*.gbj1)"))
        if  newf.length() == 0:
            return
        newf = str(newf)
        btqwopts.Options.savejobdir = os.path.dirname(newf)
        try:
            cjob.jobsave(newf)
        except jobldsv.JobSaveExcept as err:
            QMessageBox.warning(self, "Job save error", err.args[0])
            return
        self.fillrow(cjob)
        self.updateUI()

    def on_action_saveall_triggered(self, checked = None):
        if checked is None: return
        for ncjob in enumerate(self.jobs):
            n, cjob = ncjob
            if not cjob.dirty or cjob.filename is None or len(cjob.filename) == 0: continue
            try:
                cjob.jobsave(cjob.filename)
                self.fillrow(cjob, n)
            except jobldsv.JobSaveExcept as err:
                QMessageBox.warning(self, "Job save error", "Saving to file " + cjob.filename + " gave error " + err.args[0])
        self.updateUI()

    def on_action_close_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if cjob.dirty and QMessageBox.question(self, "Unsaved changes", "There are unsaved changes to the job are you sure you want to close",
                QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return
        row = self.joblist.currentRow()
        self.joblist.removeRow(row)
        self.jobs.pop(row)
        self.updateUI()

    def on_action_closedel_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if cjob.dirty and QMessageBox.question(self, "Unsaved changesss", "There are unsaved changes to the job are you sure you want to delete it",
                QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return
        if cjob.filename is not None and len(cjob.filename) != 0:
            try:
                os.unlink(cjob.filename)
            except OSError as err:
                if QMessageBox.question(self, "Unable to delete file", "Deleting " + err.filename + " gave error " + err.args[1] + " - continue",
                   QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return
        row = self.joblist.currentRow()
        self.joblist.removeRow(row)
        self.jobs.pop(row)
        self.updateUI()

    def on_action_progopts_triggered(self, checked = None):
        """Run program options"""
        if checked is None: return
        trim = re.compile(r'^\s*(.*?)\s*$')
        dlg = Cprogopts()
        bwo = btqwopts.Options.btrwopts;
        if bwo.verbose:
            dlg.verbose.setChecked(True)
        if bwo.useexted:
            dlg.useexted.setChecked(True)
        dlg.edname.setText(bwo.exted)
        if bwo.inshell:
            dlg.interm.setChecked(True)
        dlg.shell.lineEdit().setText(bwo.shell)
        slist = btqwopts.Options.servers.list_servnames()
        for s in slist:
            dlg.defserver.addItem(s)
        if bwo.prefhost is not None:
            dlg.defserver.lineEdit().setText(bwo.prefhost)
        while dlg.exec_():
            ue = dlg.useexted.isChecked()
            ish = dlg.interm.isChecked()
            editor = trim.sub(r'\1',str(dlg.edname.text()))
            sh = trim.sub(r'\1',str(dlg.shell.lineEdit().text()))
            sa = trim.sub(r'\1',str(dlg.shellarg.text()))
            if ue:
                if len(editor) == 0:
                    QMessageBox.warning(self, "No editor", "You did not specify an editor")
                    continue
                if ish and len(sh) == 0:
                     QMessageBox.warning(self, "No editor", "You did not specify an editor")
                     continue
            bwo.verbose = dlg.verbose.isChecked()
            bwo.useexted = ue
            bwo.inshell = ish
            newh = str(dlg.defserver.currentText())
            if bwo.prefhost is None:
                bwo.prefhost = newh
            elif bwo.prefhost != newh:
                bwo.prefhost = newh
                self.cilist = None
                self.servperms = None
            bwo.exted = editor
            bwo.shell = sh
            bwo.shellarg = sa
            return

    def on_action_view_script_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if cjob.script is None:
            QMessageBox.warning(self, "No script", "Job does not currently have a script to view")
            return
        dlg = Cjobview(self)
        dlg.jobdescr.setText("Job " + cjob.bj_title + " c/i " + cjob.bj_cmdinterp)
        dlg.jobtext.appendPlainText(cjob.script)
        dlg.jobtext.moveCursor(QTextCursor.Start) # So find finds from beginning
        dlg.show()
        self.viewdlglist |= set([dlg])

    def on_action_edit_script_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        bop = btqwopts.Options.btrwopts
        if bop.useexted:
            tfd, tfname = tempfile.mkstemp(suffix='.script', prefix='btrw.')
            tfd = os.fdopen(tfd, "r+")
            if cjob.script is not None:
                tfd.write(cjob.script)
                tfd.seek(0)
            if bop.inshell:
                # In case shell is given as a full space-separated command, also use re in case of multiple spaces
                shargs = re.split(r'\s+', bop.shell)
                shargs.extend(re.split(r'\s+', bop.shellarg))
                shargs.append(bop.exted)
                shargs.append(tfname)
                subprocess.call(shargs)
            else:
                subprocess.call([bop.exted, tfname])
            scr = tfd.read()
            tfd.close()
            os.unlink(tfname)
        else:
            # Use Qt dialog to edit it
            dlg = Cjobedit(self)
            dlg.jobdescr.setText("Job " + cjob.bj_title + " c/i " + cjob.bj_cmdinterp)
            if cjob.script is not None:
                dlg.jobtext.appendPlainText(cjob.script)
            dlg.jobtext.moveCursor(QTextCursor.Start)
            if not dlg.exec_(): return
            scr = str(dlg.jobtext.toPlainText())
        if len(scr) != 0 and scr[-1] != '\n': scr += '\n'
        cjob.script = scr
        cjob.dirty = True
        cjob.submitted = False
        self.fillrow(cjob)
        self.updateUI()

    def on_action_def_set_time_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = ctimedlgs.Cdeftimeparam(self)
        cjob.bj_times.init_dlg(dlg)
        dlg.set_enabled()
        if dlg.exec_():
            cjob.bj_times.copyfrom_dlg(dlg)

    def on_action_job_set_time_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = ctimedlgs.Ctimeparam(self)
        dlg.inbtr = True
        cjob.bj_times.init_dlg(dlg)
        dlg.set_enabled()
        if dlg.exec_():
            cjob.bj_times.copyfrom_dlg(dlg)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def getqueues(self):
        """Get a list of the current queues and ones we've ever encountered"""
        qset = set(btqwopts.Options.btrwopts.qprefixes)
        qset |= set([j.queueof() for j in self.jobs])
        return qset

    def on_action_def_titprill_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        if self.cilist is None:
            self.cilist = btqwopts.Options.servers.get_cilist()
        dlg = jobparams.Ctitprilldlg(self)
        perms = self.getservperms()
        if perms is None: return
        dlg.init_cmdints(self.cilist, cjob.bj_cmdinterp, (perms.btu_priv & btuser.btuser.BTM_SPCREATE) != 0)
        dlg.init_pri(cjob.bj_pri, perms.btu_minp, perms.btu_maxp)
        dlg.edjob.setText("Defaults")
        qnam, tit = cjob.queue_title()
        dlg.init_queues(self.getqueues(), qnam)
        dlg.title.setText(tit)
        dlg.llev.setValue(cjob.bj_ll)
        if dlg.exec_():
            cjob.set_title(str(dlg.queue.lineEdit().text()), str(dlg.title.text()))
            cjob.bj_pri = dlg.priority.value()
            cjob.bj_ll = dlg.llev.value()
            cjob.bj_cmdinterp = str(dlg.cmdint.currentText())

    def on_action_job_titprill_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if self.cilist is None:
            self.cilist =  btqwopts.Options.servers.get_cilist()
        dlg = jobparams.Ctitprilldlg(self)
        perms = self.getservperms()
        if perms is None: return
        dlg.init_cmdints(self.cilist, cjob.bj_cmdinterp, (perms.btu_priv & btuser.btuser.BTM_SPCREATE) != 0)
        dlg.init_pri(cjob.bj_pri, perms.btu_minp, perms.btu_maxp)
        dlg.edjob.setText(cjob.jobname())
        qnam, tit = cjob.queue_title()
        dlg.init_queues(self.getqueues(), qnam)
        dlg.title.setText(tit)
        pri = cjob.bj_pri
        dlg.llev.setValue(cjob.bj_ll)
        if dlg.exec_():
            cjob.set_title(str(dlg.queue.lineEdit().text()), str(dlg.title.text()))
            cjob.bj_pri = dlg.priority.value()
            cjob.bj_ll = dlg.llev.value()
            cjob.bj_cmdinterp = str(dlg.cmdint.currentText())
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_procpar_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cprocpardlg(self)
        dlg.edjob.setText("Defaults")
        dlg.copyin_jobpar(cjob)
        if dlg.exec_():
            dlg.copyout_jobpar(cjob)

    def on_action_job_procpar_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cprocpardlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.copyin_jobpar(cjob)
        if dlg.exec_():
            dlg.copyout_jobpar(cjob)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_timelim_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Ctimelimdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.deltime.setValue(cjob.bj_deltime)
        dlg.set_runt(cjob.bj_runtime)
        dlg.set_gt(cjob.bj_runon)
        dlg.set_ak(cjob.bj_autoksig)
        if dlg.exec_():
            cjob.bj_deltime = dlg.deltime.value()
            cjob.bj_runtime = dlg.get_runt()
            cjob.bj_runon = dlg.get_gt()
            cjob.bj_autoksig = dlg.get_ak()

    def on_action_job_timelim_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Ctimelimdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.deltime.setValue(cjob.bj_deltime)
        dlg.set_runt(cjob.bj_runtime)
        dlg.set_gt(cjob.bj_runon)
        dlg.set_ak(cjob.bj_autoksig)
        if dlg.exec_():
            cjob.bj_deltime = dlg.deltime.value()
            cjob.bj_runtime = dlg.get_runt()
            cjob.bj_runon = dlg.get_gt()
            cjob.bj_autoksig = dlg.get_ak()
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_mailwrite_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cmailwrtdlg(self)
        dlg.edjob.setText("Defaults")
        if (cjob.bj_jflags & btclasses.btjob.BJ_MAIL) != 0:
            dlg.mailresult.setChecked(True)
        if (cjob.bj_jflags & btclasses.btjob.BJ_WRT) != 0:
            dlg.writeresult.setChecked(True)
        if dlg.exec_():
            cjob.bj_jflags &= ~(btclasses.btjob.BJ_MAIL|btclasses.btjob.BJ_WRT)
            if dlg.mailresult.isChecked():
                cjob.bj_jflags |= btclasses.btjob.BJ_MAIL
            if dlg.writeresult.isChecked():
                cjob.bj_jflags |= btclasses.btjob.BJ_WRT

    def on_action_job_mailwrite_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cmailwrtdlg(self)
        dlg.edjob.setText(cjob.jobname())
        if (cjob.bj_jflags & btclasses.btjob.BJ_MAIL) != 0:
            dlg.mailresult.setChecked(True)
        if (cjob.bj_jflags & btclasses.btjob.BJ_WRT) != 0:
            dlg.writeresult.setChecked(True)
        if dlg.exec_():
            cjob.bj_jflags &= ~(btclasses.btjob.BJ_MAIL|btclasses.btjob.BJ_WRT)
            if dlg.mailresult.isChecked():
                cjob.bj_jflags |= btclasses.btjob.BJ_MAIL
            if dlg.writeresult.isChecked():
                cjob.bj_jflags |= btclasses.btjob.BJ_WRT
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_perms_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cjpermdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.init_perms(cjob.bj_mode)
        if dlg.exec_():
            dlg.get_perms(cjob.bj_mode)

    def on_action_job_perms_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cjpermdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.init_perms(cjob.bj_mode)
        if dlg.exec_():
            dlg.get_perms(cjob.bj_mode)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_args_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cjargsdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.copyin_args(cjob)
        if dlg.exec_():
            dlg.copyout_args(cjob)

    def on_action_job_args_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cjargsdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.copyin_args(cjob)
        if dlg.exec_():
            dlg.copyout_args(cjob)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_redirs_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cjredirsdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.copyin_redirs(cjob)
        if dlg.exec_():
            dlg.copyout_redirs(cjob)

    def on_action_job_redirs_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cjredirsdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.copyin_redirs(cjob)
        if dlg.exec_():
            dlg.copyout_redirs(cjob)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_env_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cjenvsdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.copyin_envs(cjob)
        if dlg.exec_():
            dlg.copyout_envs(cjob)

    def on_action_job_env_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cjenvsdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.copyin_envs(cjob)
        if dlg.exec_():
            dlg.copyout_envs(cjob)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_conds_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cjcondsdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.init_tvarlist(self.listvars_perm(btmode.btmode.BTM_READ))
        dlg.copyin_tconds(cjob)
        if dlg.exec_():
            dlg.copyout_tconds(cjob)

    def on_action_job_conds_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cjcondsdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.init_tvarlist(self.listvars_perm(btmode.btmode.BTM_READ))
        dlg.copyin_tconds(cjob)
        if dlg.exec_():
            dlg.copyout_tconds(cjob)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_asses_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        dlg = jobparams.Cjassdlg(self)
        dlg.edjob.setText("Defaults")
        dlg.init_tvarlist(self.listvars_perm(btmode.btmode.BTM_WRITE))
        dlg.copyin_tass(cjob)
        if dlg.exec_():
            dlg.copyout_tass(cjob)

    def on_action_job_asses_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        dlg = jobparams.Cjassdlg(self)
        dlg.edjob.setText(cjob.jobname())
        dlg.init_tvarlist(self.listvars_perm(btmode.btmode.BTM_WRITE))
        dlg.copyin_tass(cjob)
        if dlg.exec_():
            dlg.copyout_tass(cjob)
            cjob.dirty = True
            cjob.submitted = False
            self.fillrow(cjob)
            self.updateUI()

    def on_action_def_setcanc_toggled(self, checked = None):
        if checked is None: return
        if self.initing: return
        cjob = self.getdefaultjob()
        if cjob is None: return
        if checked:
            cjob.bj_progress = btclasses.btjob.BJP_CANCELLED
        else:
            cjob.bj_progress = btclasses.btjob.BJP_NONE

    def on_action_job_setrun_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if cjob.bj_progress == btclasses.btjob.BJP_NONE: return
        cjob.bj_progress = btclasses.btjob.BJP_NONE
        cjob.dirty = True
        cjob.submitted = False
        self.fillrow(cjob)
        self.updateUI()

    def on_action_job_setcanc_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if cjob.bj_progress == btclasses.btjob.BJP_CANCELLED: return
        cjob.bj_progress = btclasses.btjob.BJP_CANCELLED
        cjob.dirty = True
        cjob.submitted = False
        self.fillrow(cjob)
        self.updateUI()

    def subm_init(self):
        """Startup for submit routines"""
        cjob = self.getselectedjob()
        if cjob is None: return None
        if cjob.script is None:
            QMessageBox.warning(self, "No script", "Job does not currently have a script - cannot submit")
            return None
        if cjob.submitted and QMessageBox.question(self, "Resubmit?", "This job has already been submitted - resubmit",
                QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return None
        return cjob

    def subm_core(self, serv, cjob):
        """Called as core of submit routine in either case after server has been selected"""
        try:
            jobnum = jobsubmit.job_submit(serv, cjob)
        except jobsubmit.SubmError as err:
            QMessageBox.warning(self, "Submit problem", err.args[0])
            serv.uasock.closeReopen()
            serv.isok = False
            return
        if btqwopts.Options.btrwopts.verbose:
            QMessageBox.information(self, "Submission OK", "Job submitted OK, job number is " + str(jobnum))
        cjob.submitted = True
        self.fillrow(cjob)
        self.updateUI()

    def on_action_submit_locally_triggered(self, checked = None):
        if checked is None: return
        cjob = self.subm_init()
        serv = self.getprefserv()
        if serv is None: return
        self.subm_core(serv, cjob)

    def on_action_submit_remote_triggered(self, checked = None):
        if checked is None: return
        cjob = self.subm_init()
        slist = btqwopts.Options.servers.list_servnames()
        slist.sort()
        dlg = Csubmhdlg(self)
        for s in slist:
            dlg.hostname.addItem(s)
        if not dlg.exec_(): return
        sname = str(dlg.hostname.currentText())
        if len(sname) == 0: return
        serv = btqwopts.Options.servers.serversanyname[sname]
        if not serv.isok and not btqwopts.Options.servlist.perform_login(self, serv): return
        self.subm_core(serv, cjob)

    def on_action_quit_triggered(self, checked = None):
        if checked is None: return
        if not self.oktoclose(): return
        btqwopts.Options.btrwopts.btrwgeom = self.geometry()
        btqwopts.Options.btrwopts.cwidths = self.get_cwidths()
        btqwopts.Options.btrwopts.qprefixes = list(self.getqueues())
        btqwopts.save_options(self.get_recentfiles())
        QApplication.exit(0)

    def closeEvent(self, event):
        if self.oktoclose():
            btqwopts.Options.btrwopts.btrwgeom = self.geometry()
            btqwopts.Options.btrwopts.cwidths = self.get_cwidths()
            btqwopts.save_options(self.get_recentfiles())
            QApplication.exit(0)
        event.ignore()

    def on_action_About_BTRW_triggered(self, checked = None):
        if checked is None: return
        QMessageBox.about(self, "About BTRW",
                                """<b>Btrw</b> v %s
                                <p>Copyright &copy; %d Free Software Foundation
                                <p>Python %s - Qt %s""" % (version.VERSION, time.localtime().tm_year, platform.python_version(), QT_VERSION_STR))

# We now save the files in the User's directory for Windows
# or $HOME/.gbatch/ for UNIX
# We also grab the Windows user or the logged-in user if on UNIX/Linux

if btqwopts.WIN:
    floc = os.path.expanduser('~\\btqw.xbs')
    winuser = win32api.GetUserName()
else:
    # If we don't know who it is, then use HOME or if all else fails
    # the current directory
    try:
        udets = pwd.getpwuid(os.getuid())
        homed = udets.pw_dir
        winuser = udets.pw_name
    except KeyError:
        homed = os.getenv('HOME')
        if homed is None: homed = os.getcwd()
        winuser = os.getenv('LOGNAME')
        if winuser is None: winuser = 'nobody'
    # Create the ".gbatch" directory if it isn't there already
    dloc = os.path.join(homed, '.gbatch')
    if not os.path.isdir(dloc):
        try:
            os.mkdir(dloc)
        except OSError as err:
            sys.stdout = sys.stderr
            os._exit(100)
    floc = dloc + "/btqw.xbs"

# Set up Qt stuff and create main window

app = QApplication(sys.argv)
mw = BtrwMainwin()

# Load options (NB shared with btqw)

try:
    btqwopts.load_options(floc, winuser)
except btqwopts.OptError as err:
    QMessageBox.warning(mw, "Config file error", err.args[0])
    os._exit(10)

# Set up local address, we always do that by connecting to something

try:
    btqwopts.Options.servers.get_locaddr()
except gbserver.ServError as e:
    QMessageBox.warning(mw, e.args[0], "Local address fetch did not work, please reset parameters")

if btqwopts.Options.btrwopts.recentfiles is not None:
    mw.load_recentfiles(btqwopts.Options.btrwopts.recentfiles)
if btqwopts.Options.btrwopts.btrwgeom.isValid():
    mw.setGeometry(btqwopts.Options.btrwopts.btrwgeom)
if btqwopts.Options.btrwopts.cwidths is not None:
    mw.set_cwidths(btqwopts.Options.btrwopts.cwidths)
if btqwopts.Options.btrwopts.defjob and btqwopts.Options.btrwopts.defjob.bj_progress != btclasses.btjob.BJP_NONE:
    mw.action_def_setcanc.setChecked(True)
mw.initing = False
mw.updateUI()
mw.show()
btqwopts.Options.servers.login_all(mw)
os._exit(app.exec_())
