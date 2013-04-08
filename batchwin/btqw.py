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
import locale
import time
import sip
import platform

import copy, re, socket, string
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *

import resources_rc
import version
import ui_btqw_main
import ui_portdlg
import ui_servlist
import ui_progopts
import ui_winoptsdlg
import ui_fldviewdlg
import ui_jobviewdlg
import ui_jvsearchdlg
import ui_jssearchdlg
import ui_othersigdlg
import cservlistdlg
import ctimedlgs
import jobparams
import dispopts
import btqwopts
import gbserver
import gbclient
import uaclient
import gbports
import gbident
import savewdets
import btuser
import timecon
import jobmodel
import varmodel
import netfeed
import globlists
import qmatch
import btmode
import reqmess
import btclasses
import varops
import jobldsv

locale.setlocale(locale.LC_ALL, os.getenv('LANG', 'C'))

def isAlive(qobj):
    """Report if window has been deleted or not"""
    try:
        sip.unwrapinstance(qobj)
    except RuntimeError:
        return False
    return True

def get_serv_uglist(func, box, existing, lookup = True):
    """Get a sorted list of users or groups and add to Combo box also set up edit field"""
    ind = -1
    if lookup:
        ulist = func()
        for nu in enumerate(ulist):
            n,u = nu
            box.addItem(u)
            if u == existing:
                ind = n
    if ind >= 0:
        box.setCurrentIndex(ind)
    else:
        box.lineEdit().setText(existing)

class Cprogopts(QDialog, ui_progopts.Ui_Progopts):
    def __init__(self, parent = None):
        super(Cprogopts, self).__init__(parent)
        self.setupUi(self)

class Cportdlg(QDialog, ui_portdlg.Ui_portdlg):
    def __init__(self, parent = None):
        super(Cportdlg, self).__init__(parent)
        self.setupUi(self)

    def on_setDefs_clicked(self, b = None):
        if b is None: return
        newports = gbports.gbports()
        bpqwopts.Options.ports = newports
        self.clientAccessPort.setValue(newports.client_access)
        self.connectUDP.setValue(newports.connect_udp)
        self.connectTCP.setValue(newports.connect_tcp)
        self.viewTCP.setValue(newports.jobview)
        self.APITCP.setValue(newports.api_tcp)
        self.APIUDP.setValue(newports.api_udp)

    def on_plus_clicked(self, b = None):
        if b is None: return
        offset = self.adjAllBy.value()
        newports = copy.copy(btqwopts.Options.ports)
        newports.client_access = self.clientAccessPort.value() + offset
        newports.connect_udp = self.connectUDP.value() + offset
        newports.connect_tcp = self.connectTCP.value() + offset
        newports.jobview = self.viewTCP.value() + offset
        newports.api_tcp = self.APITCP.value() + offset
        newports.api_udp = self.APIUDP.value() + offset
        if newports.client_access > 65535 or newports.connect_udp > 65535 or newports.connect_tcp > 65535 or newports.jobview > 65535 or newports.api_tcp > 65535 or newports.api_udp > 65535: return
        self.clientAccessPort.setValue(newports.client_access)
        self.connectUDP.setValue(newports.connect_udp)
        self.connectTCP.setValue(newports.connect_tcp)
        self.viewTCP.setValue(newports.jobview)
        self.APITCP.setValue(newports.api_tcp)
        self.APIUDP.setValue(newports.api_udp)

    def on_minus_clicked(self, b = None):
        if b is None: return
        offset = self.adjAllBy.value()
        newports = copy.copy(btqwopts.Options.ports)
        newports.client_access = self.clientAccessPort.value() - offset
        newports.connect_udp = self.connectUDP.value() - offset
        newports.connect_tcp = self.connectTCP.value() - offset
        newports.jobview = self.viewTCP.value() - offset
        newports.api_tcp = self.APITCP.value() - offset
        newports.api_udp = self.APIUDP.value() - offset
        if newports.client_access <= 0 or newports.connect_udp <= 0 or newports.connect_tcp <= 0 or newports.jobview <= 0 or newports.api_tcp <= 0 or newports.api_udp <= 0: return
        self.clientAccessPort.setValue(newports.client_access)
        self.connectUDP.setValue(newports.connect_udp)
        self.connectTCP.setValue(newports.connect_tcp)
        self.viewTCP.setValue(newports.jobview)
        self.APITCP.setValue(newports.api_tcp)
        self.APIUDP.setValue(newports.api_udp)

class Cwinopts(QDialog, ui_winoptsdlg.Ui_winoptdlg):
    def __init__(self, parent = None):
        super(Cwinopts, self).__init__(parent)
        self.setupUi(self)

    def on_removeall_clicked(self):
        self.queuename.lineEdit().setText("")
        self.username.lineEdit().setText("")
        self.groupname.lineEdit().setText("")
        self.inclnull.setChecked(True)

class Cfldview(QDialog, ui_fldviewdlg.Ui_fldviewdlg):

    def __init__(self, parent = None):
       super(Cfldview, self).__init__(parent)
       self.setupUi(self)
       self.father = parent

    def on_addattr_clicked(self, checked = None):
       if checked is None: return
       item = self.possattr.currentText()
       if item.size() == 0: return
       self.currattr.insertItem(self.currattr.currentRow()+1, item)

    def on_delattr_clicked(self, checked = None):
       if checked is None: return
       self.currattr.takeItem(self.currattr.currentRow())

class Cjobview(QDialog, ui_jobviewdlg.Ui_jobviewdlg):
    def __init__(self, parent = None):
        super(Cjobview, self).__init__(parent)
        self.setupUi(self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        if parent:
            self.destroyed.connect(parent.viewkilled)

class Cjvsearchdlg(QDialog, ui_jvsearchdlg.Ui_jvsearchdlg):
    def __init__(self, parent = None):
        super(Cjvsearchdlg, self).__init__(parent)
        self.setupUi(self)

class Cjssearchdlg(QDialog, ui_jssearchdlg.Ui_jssearchdlg):
    def __init__(self, parent = None):
        super(Cjssearchdlg, self).__init__(parent)
        self.setupUi(self)

class Cothersigdlg(QDialog, ui_othersigdlg.Ui_othersigdlg):
    def __init__(self, parent = None):
        super(Cothersigdlg, self).__init__(parent)
        self.setupUi(self)

class  jorvview(QTableView):
    """Job or variable view - initialise as we want and provide context menu"""

    def __init__(self, parent):
        QTableView.__init__(self, parent)
        self.xp = 0
        self.yp = 0
        self.dad = parent
        self.verticalHeader().setVisible(False)
        self.setShowGrid(False)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        if parent: self.pressed.connect(parent.updateUI)

    # These are versions for the right-click menu as the main menu item wants an argument

    def viewjob(self): self.dad.on_action_View_job_triggered(self)
    def titprill(self): self.dad.on_action_titprill_triggered(self)
    def procpar(self): self.dad.on_action_procpar_triggered(self)
    def deljob(self): self.dad.on_action_delete_job_triggered(self)
    def unqueue(self): self.dad.on_action_Unqueue_Job_triggered(self)
    def interrupt(self): self.dad.on_action_interrupt_job_triggered(self)
    def quitjob(self): self.dad.on_action_quit_job_triggered(self)
    def setrun(self): self.dad.on_action_set_runnable_triggered(self)
    def setcanc(self): self.dad.on_action_setcanc_triggered(self)
    def assvar(self): self.dad.on_action_Assign_triggered(self)
    def incvar(self): self.dad.on_action_Increment_triggered(self)
    def decvar(self): self.dad.on_action_Decrement_triggered(self)
    def commvar(self): self.dad.on_action_Set_comment_triggered(self)
    def delvar(self): self.dad.on_action_Delete_Variable_triggered(self)

    def contextMenuEvent(self, event):
        mod = self.model()
        if not mod: return
        if len(self.selectedIndexes()) == 0: return
        menu = QMenu(self)
        if isinstance(mod, jobmodel.JobModel):
             menu.addAction("&View Job Script").triggered.connect(self.viewjob)
             menu.addSeparator()
             menu.addAction("&Title, priority, cmd int").triggered.connect(self.titprill)
             menu.addAction("&Process parameters").triggered.connect(self.procpar)
             menu.addSeparator()
             menu.addAction("&Delete job").triggered.connect(self.deljob)
             menu.addAction("&Unqueue job").triggered.connect(self.unqueue)
             menu.addSeparator()
             menu.addAction("&Interrupt job").triggered.connect(self.interrupt)
             menu.addAction("&Quit job").triggered.connect(self.quitjob)
             menu.addSeparator()
             menu.addAction("Set &runnable").triggered.connect(self.setrun)
             menu.addAction("Set &cancelled").triggered.connect(self.setcanc)
        else:
             menu.addAction("&Assign variable").triggered.connect(self.assvar)
             menu.addAction("&Increment variable").triggered.connect(self.incvar)
             menu.addAction("&Decrement variable").triggered.connect(self.decvar)
             menu.addAction("&Set comment").triggered.connect(self.commvar)
             menu.addSeparator()
             menu.addAction("&Remove variable").triggered.connect(self.delvar)
        menu.exec_(event.globalPos())

class BtqwMainwin(QMainWindow, ui_btqw_main.Ui_MainWindow):

    def __init__(self):
        super(BtqwMainwin, self).__init__(None)
        # Get UI stuff - this will be changed to get user's config
        self.setupUi(self)
        self.mdiArea = QMdiArea()
        self.setCentralWidget(self.mdiArea)
        self.mdiArea.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.mdiArea.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        # Init table of hosts indexed by IP address
        self.client_conns = dict()
        self.viewdlglist = set()
        self.jvsearchdlg = None
        self.viewsearchdlg = None
        # Load these up on demand
        self.userlist = None
        self.grouplist = None
        self.cilist = None

    def updateUI(self):
        jobson = self.getselectedjob() is not None
        varson = self.getselectedvar() is not None
        self.action_View_job.setEnabled(jobson)
        self.action_time.setEnabled(jobson)
        self.action_titprill.setEnabled(jobson)
        self.action_procpar.setEnabled(jobson)
        self.action_time_limits.setEnabled(jobson)
        self.action_mailwrt.setEnabled(jobson)
        self.action_JPermissions.setEnabled(jobson)
        self.action_Arguments.setEnabled(jobson)
        self.action_Environment.setEnabled(jobson)
        self.action_Redirections.setEnabled(jobson)
        self.action_Job_Conditions.setEnabled(jobson)
        self.action_Job_Assignments.setEnabled(jobson)
        self.action_delete_job.setEnabled(jobson)
        self.action_interrupt_job.setEnabled(jobson)
        self.action_quit_job.setEnabled(jobson)
        self.action_othersig.setEnabled(jobson)
        self.action_force_run.setEnabled(jobson)
        self.action_go_job.setEnabled(jobson)
        self.action_advance_time.setEnabled(jobson)
        self.action_set_runnable.setEnabled(jobson)
        self.action_setcanc.setEnabled(jobson)
        self.action_Unqueue_Job.setEnabled(jobson)
        self.action_Copy_Job.setEnabled(jobson)
        self.action_Delete_Variable.setEnabled(varson)
        self.action_Assign.setEnabled(varson)
        self.action_Set_comment.setEnabled(varson)
        self.action_Increment.setEnabled(varson)
        self.action_Decrement.setEnabled(varson)
        self.action_Multiply.setEnabled(varson)
        self.action_Divide.setEnabled(varson)
        self.action_Remainder.setEnabled(varson)
        self.action_VPermissions.setEnabled(varson)

    def conndone(self):
        """Note that something has finished connecting"""
        for cl in self.client_conns.values():
            cl.conndone()
        QTimer.singleShot(1000, self.readready)     # Prod it first time

    def readready(self):
        """Note that there is something to read"""
        for cl in self.client_conns.values():
            cl.readready()

    def servdisc(self):
        """Note that something has disconnected"""
        for cl in self.client_conns.values():
            cl.servdisc()

    def serverror(self, err):
        """Note that a server has given an error"""
        for cl in self.client_conns.values():
            cl.serverror(err)

    def viewkilled(self, qobj):
        """Note deleted view"""
        self.viewdlglist = set([w for w in self.viewdlglist if isAlive(w)])

    def init_subwins(self):
        """Initialise sub windows for jobs and variables"""
        for wi in btqwopts.Options.winlist:
            view = jorvview(self)
            if wi.isjob:
                model = jobmodel.JobModel()
            else:
                model = varmodel.VarModel()
            model.fieldlist = wi.fields
            view.setModel(model)
            model.theView = view
            model.dispoptions = wi.dispoptions
            self.mdiArea.addSubWindow(view)
            if wi.geom.isValid():
                view.parentWidget().setGeometry(wi.geom)
                view.xp = wi.geom.x()
                view.yp = wi.geom.y()
            for nfw in enumerate(wi.fwidths):
                view.setColumnWidth(*nfw)

    def getselectedjob(self):
        """Discover which job is selected and return it or none if none or not job window"""
        cwin = self.mdiArea.currentSubWindow()
        if not cwin: return None
        cwidg = cwin.widget()
        mod = cwidg.model()
        if not isinstance(mod, jobmodel.JobModel): return None
        return mod.getselectedjob()

    def getselectedvar(self):
        """Discover which var is selected and return it or none if none or not var window"""
        cwin = self.mdiArea.currentSubWindow()
        if not cwin: return None
        cwidg = cwin.widget()
        mod = cwidg.model()
        if not isinstance(mod, varmodel.VarModel): return None
        return mod.getselectedvar()

    def permcheck_j(self, j, perm, moan = True):
        """Check job against permissions for the server in question"""
        try:
            srv = self.client_conns[j.ipaddr()]
        except KeyError:
            return False
        if  srv.ispermitted(j.bj_mode, perm):  return True
        if moan:
            QMessageBox.warning(self, "Not allowed", "You are not allowed to perform this job operation")
        return False

    def permcheck_v(self, v, perm, moan = True):
        """Check variable against permissions for the server in question"""
        try:
            srv = self.client_conns[v.ipaddr()]
        except KeyError:
            return False
        if  srv.ispermitted(v.var_mode, perm):  return True
        if moan:
            QMessageBox.warning(self, "Not allowed", "You are not allowed to perform this variable operation")
        return False

    def listvars_perm(self, perm):
        """List variables known as a dictionary of lists by IP satisfying the permission given"""
        result = dict()
        for ip, cl in self.client_conns.iteritems():
            result[ip] = [v for v in globlists.varsfor(ip) if self.permcheck_v(v, perm, False)]
        return result

    def init_client(self, h):
        """Initialise client connection"""
        if h.servip not in self.client_conns:
            self.client_conns[h.servip] = gbclient.gbclient(h, self)

    def connect_client(self, h):
        """Connect a single client"""
        self.client_conns[h.servip].startconnect()

    def disconnect_client(self, h):
        """Disconnect a single client"""
        try:
            self.client_conns[h.servip].shuthost()
            self.serverdisconn(self.client_conns[h.servip])
            h.connected = False
            h.syncreq = False
            h.synccomplete = False
        except KeyError:
            pass

    def connect_clients(self):
        """Connect clients not already connected"""

        for cl in self.client_conns.values():
            cl.startconnect()

    def serverdisconn(self, h):
        """Note server has disconnected

Remove all jobs and vars for that server and close the socket"""

        self.clearhost(h, h)

    def iterate_windows(self):
        """Iterator for active windows"""
        for w in self.mdiArea.subWindowList():
            m = w.widget().model()
            if m:
                yield m

    def addjob_int(self, job):
        """Add new job to each window"""
        for m in self.iterate_windows():
            m.addjob(job)

    def deljob_int(self, jid):
        """Ditto to delete jobs"""
        for m in self.iterate_windows():
            m.deljob(jid)

    def addvar_int(self, var):
        """Add new var to each window"""
        for m in self.iterate_windows():
            m.addvar(var)

    def delvar_int(self, vid):
        """Delete var from each window"""
        for m in self.iterate_windows():
            m.delvar(vid)

    def addjob(self, jobmsg, sender):
        """Add job to job list and each window"""
        newjob = jobmsg.job
        newjob.isvisible = sender.visible(newjob.bj_mode)
        globlists.addjob(newjob)
        if newjob.isvisible:
            self.addjob_int(newjob)

    def jhchanged(self, jobmsg, sender):
        """Job headers changed.

We may have to reposition the job according to the priority"""
        jhdrs = jobmsg.job
        if jhdrs not in globlists.Job_list: return
        origjob = globlists.Job_list[jhdrs]
        oldpri = origjob.bj_pri
        origjob.copy_headers(jhdrs)
        # Theory: job isn't suddenly going to be visible where it wasn't before
        # or vice versa because we don't use this message type to talk about
        # permissions and ownership
        if origjob.isvisible:
            for m in self.iterate_windows():
                m.updjob(origjob, oldpri)

    def jchanged(self, jobmsg, sender):
        """Job has changed including strings"""
        newjob = jobmsg.job
        if newjob not in globlists.Job_list: return
        origjob = globlists.Job_list[newjob]
        oldpri = origjob.bj_pri
        origjob.copy_headers(newjob)
        origjob.copy_strings(newjob)
        if origjob.isvisible:
            for m in self.iterate_windows():
                m.updjob(origjob, oldpri)

    def boqjob(self, jobmsg, sender):
        """Note job gone to bottom of queue"""
        if jobmsg.jid not in globlists.Job_list: return
        origjob = globlists.Job_list[jobmsg.jid]
        # Reset queue time so sort routine will pick it up
        origjob.queuetime = time.time()
        if origjob.isvisible:
            for m in self.iterate_windows():
                m.boqjob()

    def jobforced(self, jobmsg, sender):
        """Note job forced - probably don't need to worry"""
        if jobmsg.jid not in globlists.Job_list: return
        origjob = globlists.Job_list[jobmsg.jid]
        origjob.bj_jflags |= btclasses.btjob.BJ_FORCE
        # I don't think we need to tell anyone

    def jobforcedna(self, jobmsg, sender):
        """Ditto with no advance"""
        if jobmsg.jid not in globlists.Job_list: return
        origjob = globlists.Job_list[jobmsg.jid]
        origjob.bj_jflags |= btclasses.btjob.BJ_FORCE | btclasses.btjob.BJ_FORCENA
        # Likewise I don't think we need to tell anyone

    def deletedjob(self, jobmsg, sender):
        """Note job deleted"""
        globlists.deljob(jobmsg.jid)
        self.deljob_int(jobmsg.jid)

    def chmogedjob(self, jobmsg, sender):
        """Note changes to permissions/ownership

This might make a previously invisible job visible or vice versa"""
        jhdrs = jobmsg.job
        if jhdrs not in globlists.Job_list: return
        origjob = globlists.Job_list[jhdrs]
        newvis = sender.visible(jhdrs.bj_mode)
        origjob.bj_mode = jhdrs.bj_mode
        # See if that changed the visibility
        # and add or delete from windows as required
        if newvis != origjob.isvisible:
            origjob.isvisible = newvis
            if newvis: self.addjob_int(origjob)
            else: self.deljob_int(origjob)
        elif newvis:
            for m in self.iterate_windows():
                m.updjob(origjob, origjob.bj_pri)

    def statchjob(self, jobmsg, sender):
        """Note changes in job status"""
        if jobmsg.jid not in globlists.Job_list: return
        origjob = globlists.Job_list[jobmsg.jid]
        origjob.bj_pid = jobmsg.pid
        origjob.bj_lastexit = jobmsg.lastexit
        origjob.bj_progress = jobmsg.prog
        origjob.bj_times.tc_nexttime = jobmsg.nexttime
        origjob.bj_flags &= ~(btclasses.btjob.BJ_FORCE|btclasses.btjob.BJ_FORCENA)
        origjob.bj_runhostid = jobmsg.runhostid
        if origjob.isvisible:
            for m in self.iterate_windows():
                m.updjob(origjob, origjob.bj_pri)

    def createdvar(self, varmsg, sender):
        """Note variable created"""
        newvar = varmsg.var
        newvar.isvisible = sender.visible(newvar.var_mode)
        globlists.addvar(newvar)
        if newvar.isvisible: self.addvar_int(newvar)

    def deletedvar(self, varmsg, sender):
        """Note variable deleted"""
        globlists.delvar(varmsg.var)
        self.delvar_int(varmsg.var)

    def assignedvar(self, varmsg, sender):
        """Note variable assigned"""
        newvar = varmsg.var
        if newvar not in globlists.Var_list: return
        origvar = globlists.Var_list[newvar]
        origvar.var_value = newvar.var_value
        origvar.var_comment = newvar.var_comment
        if origvar.isvisible:
            for m in self.iterate_windows():
                m.assignedvar(origvar)

    def chflagsvar(self, varmsg, sender):
        """Note variable flags changed"""
        newvar = varmsg.var
        if newvar not in globlists.Var_list: return
        origvar = globlists.Var_list[newvar]
        origvar.var_flags = newvar.var_flags
        if origvar.isvisible:
            for m in self.iterate_windows():
                m.assignedvar(origvar)

    def chmogedvar(self, varmsg, sender):
        """Note variable permissions changed"""
        newvar = varmsg.var
        if newvar not in globlists.Var_list: return
        origvar = globlists.Var_list[newvar]
        newvis = sender.visible(newvar.var_mode)
        origvar.var_mode = newvar.var_mode
        # See if that changed the visibility
        # and add or delete from windows as required
        if newvis != origvar.isvisible:
            origvar.isvisible = newvis
            if newvis: self.addvar_int(origvar)
            else: self.delvar_int(origvar)
        elif newvis:
            for m in self.iterate_windows():
                m.assignedvar(origvar)

    def renamedvar(self, varmsg, sender):
        """Note variable renamed"""
        newvar = varmsg.var
        if newvar not in globlists.Var_list: return
        origvar = globlists.Var_list[newvar]
        origvar.var_name = newvar.var_name
        if origvar.isvisible:
            for m in self.iterate_windows():
                m.renamedvar(origvar)

    def remassjob(self, assmsg, sender):
        """Note remote job assignment and simulate variable action"""
        if assmsg.jid not in globlists.Job_list: return
        job = globlists.Job_list[assmsg.jid]
        flags = assmsg.flags
        status = assmsg.status
        # Run over assignments and do the business
        assvars = []
        for ass in job.bj_asses:
            op = ass.bja_op
            flwhen = ass.bja_flags
            if op >= btclasses.jass.BJA_SEXIT: flwhen = btclasses.jass.EXIT_FLAGS
            # Forget assignment if it doesn't apply now
            if (flags & flwhen) == 0: continue
            # Find the variable we are talking about
            if ass.bja_varind not in globlists.Var_list: continue
            affvar = globlists.Var_list[ass.bja_varind]
            newvalue = btclasses.btconst()
            origvalue = affvar.var_value
            donev = False
            # Invert operator if we are reversing
            # Special case of reversing assignment set 0 or null string
            if (flwhen & btclasses.jass.BJA_REVERSE) != 0 and (flags & btclasses.jass.BJA_START) == 0:
                if op == btclasses.jass.BJA_ASSIGN:
                    if not origvalue.isint: newvalue = btclasses.btconst("")
                    else: newvalue = btclasses.btconst(0)
                    donev = True
                elif op == btclasses.jass.BJA_INCR:
                    op = btclasses.jass.BJA_DECR
                elif op == btclasses.jass.BJA_DECR:
                    op = btclasses.jass.BJA_INCR
                elif op == btjclasses.jass.BJA_MULT:
                    op = btclasses.jass.BJA_DIV
                elif op == btclasses.jass.BJA_DIV:
                    op = btclasses.jass.BJA_MULT
            if not donev:
                if op == btclasses.jass.BJA_ASSIGN:
                    newvalue = ass.bja_con
                elif op == btclasses.jass.BJA_INCR:
                    newvalue = btclasses.btconst(origvalue.intvalue() + ass.bja_con.intvalue())
                elif op == btclasses.jass.BJA_DECR:
                    newvalue = btclasses.btconst(origvalue.intvalue() - ass.bja_con.intvalue())
                elif op == btclasses.jass.BJA_MULT:
                    newvalue = btclasses.btconst(origvalue.intvalue() * ass.bja_con.intvalue())
                elif op == btclasses.jass.BJA_DIV:
                    denom = ass.bja_con.intvalue()
                    if denom == 0: newvalue = origvalue
                    else: newvalue = btclasses.btconst(origvalue.intvalue() / denom)
                elif op == btclasses.jass.BJA_MOD:
                    denom = ass.bja_conn.intvalue()
                    if denom == 0: newvalue = origvalue
                    else: newvalue = btclasses.btconst(origvalue.intvalue() % denom)
                elif op == btclasses.jass.BJA_SEXIT:
                    newvalue = btclasses.btconst((status >> 8) & 255)
                elif op == btclasses.jass.BJA_SSIG:
                    newvalue = btclasses.btconst(status & 255)
            affvar.var_value = newvalue
            if affvar.isvisible: assvars.append(affvar)
        for v in assvars:
            for m in self.iterate_windows():
                m.assignedvar(v)

    def clearhost(self, netm, sender):
        """Note host shut down"""
        netip = sender.host.servip

        for j in globlists.jobsfor(netip):
            self.deljob_int(j)
        for v in globlists.varsfor(netip):
            self.delvar_int(v)
        try:
            c = self.client_conns[netip]
            del self.client_conns[netip]
            c.closesock()
        except KeyError:
            pass

    def reqreply(self, netm, sender):
        """Process request reply - only do things if errors"""
        messagetext = reqmess.lookupcode(netm.arg)
        if len(messagetext) != 0: QMessageBox.warning(self, "Request error", messagetext)

    def on_action_Program_options_triggered(self, checked = None):
        """Run program options"""
        if checked is None: return
        dlg = Cprogopts()
        if btqwopts.Options.confdel:
            dlg.confAlways.setChecked(True)
        dlg.unqueuebin.setChecked(btqwopts.Options.unqueueasbin)
        dlg.udpwaittime.setValue(btqwopts.Options.udpwaittime)
        if dlg.exec_():
            btqwopts.Options.confdel = dlg.confAlways.isChecked()
            btqwopts.Options.unqueueasbin = dlg.unqueuebin.isChecked()
            btqwopts.Options.udpwaittime = dlg.udpwaittime.value()

    def on_action_Time_defaults_triggered(self, checked = None):
        """Set default times"""
        if checked is None: return
        dlg = ctimedlgs.Cdeftimeparam(self)
        btqwopts.Options.timedefs.init_dlg(dlg)
        dlg.set_enabled()
        if dlg.exec_():
            btqwopts.Options.timedefs.copyfrom_dlg(dlg)

    def on_action_Server_list_triggered(self, checked = None):
        if checked is None: return
        cservlistdlg.update_server_list(self)

    def on_action_Port_settings_triggered(self, checked = None):
        if checked is None: return
        dlg = Cportdlg()
        dlg.clientAccessPort.setValue(btqwopts.Options.ports.client_access)
        dlg.connectUDP.setValue(btqwopts.Options.ports.connect_udp)
        dlg.connectTCP.setValue(btqwopts.Options.ports.connect_tcp)
        dlg.viewTCP.setValue(btqwopts.Options.ports.jobview)
        dlg.APITCP.setValue(btqwopts.Options.ports.api_tcp)
        dlg.APIUDP.setValue(btqwopts.Options.ports.api_udp)
        if dlg.exec_():
            btqwopts.Options.ports.client_access = dlg.clientAccessPort.value()
            btqwopts.Options.ports.connect_udp = dlg.connectUDP.value()
            btqwopts.Options.ports.connect_tcp = dlg.connectTCP.value()
            btqwopts.Options.ports.jobview = dlg.viewTCP.value()
            btqwopts.Options.ports.api_tcp = dlg.APITCP.value()
            btqwopts.Options.ports.api_udp = dlg.APIUDP.value()

    def on_action_Quit_triggered(self, checked = None):
        if checked is None: return
        # Disconnect clients
        for h in self.client_conns.values():
            h.shuthost()
        # Kick off by saving window geoms - don't use iterate_windows as we want the views
        btqwopts.Options.winlist = []
        btqwopts.Options.geom = self.geometry()
        for w in self.mdiArea.subWindowList():
            widg = w.widget()
            m = widg.model()
            if m:
                m.geom = w.geometry()
                m.fieldwidths = []
                for col in range(0, len(m.fieldlist)):
                    m.fieldwidths.append(widg.columnWidth(col))
            btqwopts.Options.winlist.append(savewdets.savewdets(m))
        btqwopts.save_options()
        QApplication.exit(0)

    def closeEvent(self, event):
        self.on_action_Quit_triggered(True)

    def on_action_Cascade_triggered(self, checked = None):
        if checked is None: return
        self.mdiArea.cascadeSubWindows()

    def on_action_Window_options_triggered(self, checked = None):
        if checked is None: return
        subw = self.mdiArea.activeSubWindow()
        if subw is None: return
        view = subw.widget()
        m = view.model()
        if not m: return
        opts = m.dispoptions
        dlg = Cwinopts()
        get_serv_uglist(globlists.get_queue_names, dlg.queuename, opts.limqueue)
        get_serv_uglist(btqwopts.Options.servers.get_ulist, dlg.username, opts.limuser)
        get_serv_uglist(btqwopts.Options.servers.get_glist, dlg.groupname, opts.limgroup)
        dlg.inclnull.setChecked(opts.inclnull)
        if dlg.exec_():
            opts.limqueue = str(dlg.queuename.lineEdit().text())
            opts.limuser = str(dlg.username.lineEdit().text())
            opts.limgroup = str(dlg.groupname.lineEdit().text())
            opts.inclnull = dlg.inclnull.isChecked()
            m.replacerows()

    def on_action_Tile_vertically_triggered(self, checked = None):
        if checked is None: return
        wl = self.mdiArea.subWindowList()
        nw = len(wl)
        if  nw < 2:
            self.mdiArea.tileSubWindows()
            return
        mdiw = self.mdiArea.width()
        mdih = self.mdiArea.height()
        cht = mdih / nw
        y = 0
        for w in wl:
            w.resize(mdiw, cht)
            w.move(0, y)
            w.widget().resizeColumnsToContents()
            y += cht

    def on_action_Tile_horizontally_triggered(self, checked = None):
        if checked is None: return
        wl = self.mdiArea.subWindowList()
        nw = len(wl)
        if  nw < 2:
            self.mdiArea.tileSubWindows()
            return
        mdiw = self.mdiArea.width()
        mdih = self.mdiArea.height()
        cwid = mdiw / nw
        x = 0
        for w in wl:
            w.resize(cwid, mdih)
            w.move(x, 0)
            w.widget().resizeColumnsToContents()
            x += cwid

    def on_action_list_format_triggered(self, checked = None):
        if checked is None: return
        cwin = self.mdiArea.currentSubWindow()
        if not cwin: return
        cwidg = cwin.widget()
        mod = cwidg.model()
        dlg = Cfldview()
        if isinstance(mod, jobmodel.JobModel):
            possfields = jobmodel.Formats
            dlg.jorv.setText('Jobs')
        else:
            possfields = varmodel.Formats
            dlg.jorv.setText('Variables')
        n = 0
        fldlu = dict()
        for poss in possfields:
            fld = poss[0]
            dlg.possattr.addItem(fld)
            fldlu[fld] = n
            n += 1
        for fld in mod.fieldlist:
            dlg.currattr.addItem(possfields[fld][0])
        if dlg.exec_():
            mod.fieldlist = []
            for row in range(0, dlg.currattr.count()):
                mod.fieldlist.append(fldlu[str(dlg.currattr.item(row).data(Qt.DisplayRole).toString())])
            mod.reset()

    def on_action_New_job_window_triggered(self, checked = None):
        if checked is None: return
        jobw = jorvview(self)
        jobm = jobmodel.JobModel()
        jobw.setModel(jobm)
        jobm.theView = jobw
        self.mdiArea.addSubWindow(jobw)
        jobw.parentWidget().show()
        jobm.replacerows()

    def on_action_New_variable_window_triggered(self, checked = None):
        if checked is None: return
        varw = jorvview(self)
        varm = varmodel.VarModel()
        varw.setModel(varm)
        varm.theView = varw
        self.mdiArea.addSubWindow(varw)
        varw.parentWidget().show()
        varm.replacerows()

    def on_action_View_job_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_READ): return
        sock = netfeed.net_feed(cjob)
        if not sock:
            QMessageBox.warning(self, "Display problem", "Cannot display job")
            return
        dlg = Cjobview(self)
        title = "%s:%d" % (btqwopts.Options.servers.look_hostid(cjob.ipaddr()), cjob.bj_job)
        if len(cjob.bj_title) != 0:
                title += ' ' + cjob.bj_title
        dlg.jobdescr.setText(title)
        while 1:
            buf = netfeed.feed_string(sock)
            if len(buf) == 0: break
            dlg.jobtext.appendPlainText(buf)
        sock.close()
        dlg.jobtext.moveCursor(QTextCursor.Start) # So find finds from beginning
        dlg.show()
        self.viewdlglist |= set([dlg])

    def on_action_time_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        dlg = ctimedlgs.Ctimeparam(self)
        cjob.bj_times.init_dlg(dlg)
        dlg.set_enabled()
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            njob.bj_times.copyfrom_dlg(dlg)
            self.client_conns[cjob.ipaddr()].chgjobhdr(njob)

    def on_action_titprill_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        if self.cilist is None:
            self.cilist =  btqwopts.Options.servers.get_cilist()
        cclient = self.client_conns[cjob.ipaddr()]
        uperm = cclient.host.perms
        dlg = jobparams.Ctitprilldlg(self)
        dlg.init_cmdints(self.cilist, cjob.bj_cmdinterp, (uperm.btu_priv & btuser.btuser.BTM_SPCREATE) != 0)
        dlg.init_pri(cjob.bj_pri, uperm.btu_minp, uperm.btu_maxp)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        qnam, tit = cjob.queue_title()
        dlg.init_queues(set(globlists.get_queue_names()), qnam)
        dlg.title.setText(tit)
        pri = cjob.bj_pri
        dlg.llev.setValue(cjob.bj_ll)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            njob.set_title(str(dlg.queue.lineEdit().text()), str(dlg.title.text()))
            njob.bj_pri = dlg.priority.value()
            njob.bj_ll = dlg.llev.value()
            njob.bj_cmdinterp = str(dlg.cmdint.currentText())
            if njob.bj_title == cjob.bj_title:
                cclient.chgjobhdr(njob)
            else:
                cclient.chgjob(njob)

    def on_action_procpar_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cprocpardlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.copyin_jobpar(cjob)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.copyout_jobpar(njob)
            if njob.bj_direct == cjob.bj_direct:
                cclient.chgjobhdr(njob)
            else:
                cclient.chgjob(njob)

    def on_action_time_limits_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Ctimelimdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.deltime.setValue(cjob.bj_deltime)
        dlg.set_runt(cjob.bj_runtime)
        dlg.set_gt(cjob.bj_runon)
        dlg.set_ak(cjob.bj_autoksig)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            njob.bj_deltime = dlg.deltime.value()
            njob.bj_runtime = dlg.get_runt()
            njob.bj_runon = dlg.get_gt()
            njob.bj_autoksig = dlg.get_ak()
            cclient.chgjobhdr(njob)

    def on_action_mailwrt_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cmailwrtdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        if (cjob.bj_jflags & btclasses.btjob.BJ_MAIL) != 0:
            dlg.mailresult.setChecked(True)
        if (cjob.bj_jflags & btclasses.btjob.BJ_WRT) != 0:
            dlg.writeresult.setChecked(True)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            njob.bj_jflags &= ~(btclasses.btjob.BJ_MAIL|btclasses.btjob.BJ_WRT)
            if dlg.mailresult.isChecked():
                njob.bj_jflags |= btclasses.btjob.BJ_MAIL
            if dlg.writeresult.isChecked():
                njob.bj_jflags |= btclasses.btjob.BJ_WRT
            if njob.bj_jflags != cjob.bj_jflags:
                cclient.chgjobhdr(njob)

    def on_action_JPermissions_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRMODE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cjpermdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.init_perms(cjob.bj_mode)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.get_perms(njob.bj_mode)
            cclient.jchmod(njob)

    def on_action_Arguments_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cjargsdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.copyin_args(cjob)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.copyout_args(njob)
            cclient.chgjob(njob)

    def on_action_Environment_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cjenvsdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.copyin_envs(cjob)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.copyout_envs(njob)
            cclient.chgjob(njob)

    def on_action_Redirections_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cjredirsdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.copyin_redirs(cjob)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.copyout_redirs(njob)
            cclient.chgjob(njob)

    def on_action_Job_Conditions_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cjcondsdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.init_varlist(self.listvars_perm(btmode.btmode.BTM_READ))
        dlg.copyin_conds(cjob)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.copyout_conds(njob)
            cclient.chgjobhdr(njob)

    def on_action_Job_Assignments_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        dlg = jobparams.Cjassdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        dlg.init_varlist(self.listvars_perm(btmode.btmode.BTM_WRITE))
        dlg.copyin_ass(cjob)
        if dlg.exec_():
            njob = copy.deepcopy(cjob)
            dlg.copyout_ass(njob)
            cclient.chgjobhdr(njob)

    def on_action_Create_Jobs_triggered(self, checked = None):
        if checked is None: return
        indir = os.path.dirname(sys.argv[0])
        if btqwopts.WIN:
            execfile = "btrw.exe"
        else:
            execfile = "btrw.py"
        if len(indir) != 0:
            execfile = os.path.join(indir, execfile)
        if not os.path.exists(execfile):
            QMessageBox.warning(self, "Program not found", "Cannot find BTRW executable " + execfile)
            return
        if btqwopts.WIN:
            os.spawnl(os.P_NOWAIT, execfile, "BTRW")
        else:
            cpid = os.fork()
            if cpid == 0:
                if os.fork() == 0: os.execl(sys.executable, "BTRW", execfile)
                sys.exit(0)
            if cpid < 0: return
            os.waitpid(cpid, 0)

    def on_action_delete_job_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_DELETE): return
        if btqwopts.Options.confdel and \
             QMessageBox.question(self, "Please confirm delete", "Please confirm you want to delete this job",
                QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return
        cclient = self.client_conns[cjob.ipaddr()]
        cclient.jobop(cjob, reqmess.J_DELETE)

    def on_action_interrupt_job_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_KILL): return
        self.client_conns[cjob.ipaddr()].jobop(cjob, reqmess.J_KILL, 2)

    def on_action_quit_job_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_KILL): return
        self.client_conns[cjob.ipaddr()].jobop(cjob, reqmess.J_KILL, 3)

    def on_action_othersig_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_KILL): return
        dlg = Cothersigdlg(self)
        dlg.edjob.setText(jobmodel.fmt_jobno(cjob, self))
        if dlg.exec_():
            for sigp in (('stop', 19), ('cont', 18), ('term', 15), ('kill', 9), ('hup', 1), ('int', 2), ('quit', 3), ('alarm', 14), ('bus', 10), ('segv', 11)):
                if getattr(dlg, sigp[0] + "sig").isChecked():
                    signum = sigp[1]
                    break
            else:
                signum = 15
            self.client_conns[cjob.ipaddr()].jobop(cjob, reqmess.J_KILL, signum)

    def on_action_force_run_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_KILL): return
        self.client_conns[cjob.ipaddr()].jobop(cjob, reqmess.J_FORCENA)

    def on_action_go_job_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_KILL): return
        self.client_conns[cjob.ipaddr()].jobop(cjob, reqmess.J_FORCE)

    def on_action_advance_time_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None: return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cjob.ipaddr()]
        newtime = cjob.bj_times.advtime()
        if newtime == 0:
            QMessageBox.warning(self, "No repeat set", "No appropriate repeat set, cannot advance time")
            return
        njob = copy.copy(cjob)
        njob.bj_times.tc_nexttime = newtime
        self.client_conns[cjob.ipaddr()].chgjobhdr(njob)

    def on_action_set_runnable_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None or cjob.bj_progress == btclasses.btjob.BJP_NONE: return
        if cjob.bj_progress >= btclasses.btjob.BJP_STARTUP1:
            QMessageBox.warning(self, "Job running", "Cannot alter job state - it is still running")
            return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        njob = copy.copy(cjob)
        njob.bj_progress = btclasses.btjob.BJP_NONE
        self.client_conns[cjob.ipaddr()].chgjobhdr(njob)

    def on_action_setcanc_triggered(self, checked = None):
        if checked is None: return
        cjob = self.getselectedjob()
        if cjob is None or cjob.bj_progress == btclasses.btjob.BJP_CANCELLED: return
        if cjob.bj_progress >= btclasses.btjob.BJP_STARTUP1:
            QMessageBox.warning(self, "Job running", "Cannot alter job state - it is still running")
            return
        if not self.permcheck_j(cjob, btmode.btmode.BTM_WRITE): return
        njob = copy.copy(cjob)
        njob.bj_progress = btclasses.btjob.BJP_CANCELLED
        self.client_conns[cjob.ipaddr()].chgjobhdr(njob)

    def unqueuemain(self, deleteit):
        """Main body of unqueue, boolean argument says whether we delete"""
        cjob = self.getselectedjob()
        if cjob is None: return
        if cjob.bj_progress >= btclasses.btjob.BJP_STARTUP1:
            QMessageBox.warning(self, "Job running", "Cannot unqueue a running job")
        pflgs = btmode.btmode.BTM_READ
        if deleteit: pflgs |= btmode.btmode.BTM_DELETE
        if not self.permcheck_j(cjob, pflgs): return
        if deleteit:
            mesg = "File to save job to before deletion"
        else:
            mesg = "File to copy job to"
        newf = QFileDialog.getSaveFileName(self,
                                           self.tr(mesg),
                                           btqwopts.Options.savejobdir + "/untitled.gbj1",
                                           self.tr("Saved GNUBatch jobs (*.gbj1)"))
        if  newf.length() == 0:
            return
        newfs = str(newf)
        btqwopts.Options.savejobdir = os.path.dirname(newfs)
        sock = netfeed.net_feed(cjob)
        if not sock:
            QMessageBox.warning(self, "Script problem", "Cannot obtain job script")
            return
        scriptstr = ""
        while 1:
            buf = netfeed.feed_string(sock)
            if len(buf) == 0: break
            scriptstr += buf
        sock.close()
        jobldsv.jobsave(newfs, cjob, scriptstr)
        if deleteit:
            self.client_conns[cjob.ipaddr()].jobop(cjob, reqmess.J_DELETE)

    def on_action_Unqueue_Job_triggered(self, checked = None):
        if checked is None: return
        self.unqueuemain(True)

    def on_action_Copy_Job_triggered(self, checked = None):
        if checked is None: return
        self.unqueuemain(False)

    def on_action_Create_Variable_triggered(self, checked = None):
        if checked is None: return
        dlg = varops.Ccreatevardlg(self)
        dlg.init_servlist([cl.host.get_servname() for cl in self.client_conns.values() if cl.is_sync()])
        while dlg.exec_():
            if dlg.checkdlg(): continue
            hostname = str(dlg.crehost.currentText())
            cl = self.client_conns[btqwopts.Options.servers.look_host(hostname)]
            vname = str(dlg.varname.text())
            vcomment = str(dlg.comment.text())
            if dlg.istext.isChecked():
                vval = str(dlg.textval.text())
            else:
                vval = dlg.intval.value()
            vmode = btmode.btmode()
            vmode.u_flags = cl.host.perms.btu_vflags[0]
            vmode.g_flags = cl.host.perms.btu_vflags[1]
            vmode.o_flags = cl.host.perms.btu_vflags[2]
            flags = 0
            ci = dlg.exporttype.currentIndex()
            if ci == 1:
                flags = btclasses.btvar.VF_EXPORT
            elif ci == 2:
                flags = btclasses.btvar.VF_EXPORT|btclasses.btvar.VF_CLUSTER
            cmsg = uaclient.createvarmsg(vname, vcomment, vval, flags, vmode)
            ret = cl.host.uasock.createvar(cmsg)
            if ret != uaclient.XBNQ_OK:
                QMessageBox.warning(self, "Variable create error", uaclient.errorcodes[ret][1])
            return

    def on_action_Delete_Variable_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_DELETE): return
        if btqwopts.Options.confdel and \
             QMessageBox.question(self, "Please confirm delete", "Please confirm you want to delete variable " + cvar.var_name,
                QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) != QMessageBox.Yes: return
        cclient = self.client_conns[cvar.ipaddr()]
        cmsg = uaclient.createvarmsg(cvar.var_name, cvar.var_comment, 0, 0, cvar.var_mode)
        ret = cclient.host.uasock.deletevar(cmsg)
        if ret != uaclient.XBNQ_OK:
            QMessageBox.warning(self, "Variable create error", uaclient.errorcodes[ret][1])

    def on_action_Assign_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cvar.ipaddr()]
        dlg = varops.Cvassdlg(self)
        dlg.edvar.setText(varmodel.fmt_name(cvar, self))
        if cvar.var_value.isint:
            dlg.intval.setValue(cvar.var_value.value)
        else:
            dlg.istext.setChecked(True)
            dlg.textval.setText(cvar.var_value.value)
        if dlg.exec_():
            nvar = copy.deepcopy(cvar)
            if dlg.istext.isChecked():
                vval = str(dlg.textval.text())
            else:
                vval = dlg.intval.value()
            nvar.var_value = btclasses.btconst(vval)
            cclient.vassign(nvar)

    def on_action_Set_comment_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        cclient = self.client_conns[cvar.ipaddr()]
        dlg = varops.Cvarcommdlg(self)
        dlg.edvar.setText(varmodel.fmt_name(cvar, self))
        dlg.comment.setText(cvar.var_comment)
        if dlg.exec_():
            nvar = copy.deepcopy(cvar)
            nvar.var_comment = str(dlg.comment.text())
            cclient.vcomment(nvar)

    def on_action_Set_constant_triggered(self, checked = None):
        if checked is None: return
        dlg = varops.Cconvaldlg(self)
        dlg.conval.setValue(btqwopts.Options.arithconst)
        if dlg.exec_():
            btqwopts.Options.arithconst = dlg.conval.value()

    def on_action_Increment_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        if not cvar.var_value.isint:
            QMessageBox.warning(self, "Invalid value", "Variable " + cvar.var_name + " does not have arithmetic value")
            return
        cclient = self.client_conns[cvar.ipaddr()]
        nvar = copy.deepcopy(cvar)
        nvar.var_value = btclasses.btconst(cvar.var_value.intvalue() + btqwopts.Options.arithconst)
        cclient.vassign(nvar)

    def on_action_Decrement_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        if not cvar.var_value.isint:
            QMessageBox.warning(self, "Invalid value", "Variable " + cvar.var_name + " does not have arithmetic value")
            return
        cclient = self.client_conns[cvar.ipaddr()]
        nvar = copy.deepcopy(cvar)
        nvar.var_value = btclasses.btconst(cvar.var_value.intvalue() - btqwopts.Options.arithconst)
        cclient.vassign(nvar)

    def on_action_Multiply_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        if not cvar.var_value.isint:
            QMessageBox.warning(self, "Invalid value", "Variable " + cvar.var_name + " does not have arithmetic value")
            return
        cclient = self.client_conns[cvar.ipaddr()]
        nvar = copy.deepcopy(cvar)
        nvar.var_value = btclasses.btconst(cvar.var_value.intvalue() * btqwopts.Options.arithconst)
        cclient.vassign(nvar)

    def on_action_Divide_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        if not cvar.var_value.isint:
            QMessageBox.warning(self, "Invalid value", "Variable " + cvar.var_name + " does not have arithmetic value")
            return
        if btqwopts.Options.arithconst == 0:
            QMessageBox.warning(self, "Zero Divide", "Saved aritmetic constant is zero")
            return
        cclient = self.client_conns[cvar.ipaddr()]
        nvar = copy.deepcopy(cvar)
        nvar.var_value = btclasses.btconst(cvar.var_value.intvalue() / btqwopts.Options.arithconst)
        cclient.vassign(nvar)

    def on_action_Remainder_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRITE): return
        if not cvar.var_value.isint:
            QMessageBox.warning(self, "Invalid value", "Variable " + cvar.var_name + " does not have arithmetic value")
            return
        if btqwopts.Options.arithconst == 0:
            QMessageBox.warning(self, "Zero Divide", "Saved aritmetic constant is zero")
            return
        cclient = self.client_conns[cvar.ipaddr()]
        nvar = copy.deepcopy(cvar)
        nvar.var_value = btclasses.btconst(cvar.var_value.intvalue() % btqwopts.Options.arithconst)
        cclient.vassign(nvar)

    def on_action_VPermissions_triggered(self, checked = None):
        if checked is None: return
        cvar = self.getselectedvar()
        if cvar is None: return
        if not self.permcheck_v(cvar, btmode.btmode.BTM_WRMODE): return
        cclient = self.client_conns[cvar.ipaddr()]
        dlg = varops.Cvpermdlg(self)
        dlg.edvar.setText(varmodel.fmt_name(cvar, self))
        dlg.init_perms(cvar.var_mode)
        if dlg.exec_():
            nvar = copy.deepcopy(cvar)
            dlg.get_perms(nvar.var_mode)
            cclient.vchmod(nvar)

    def find_active_window(self):
        """Find the type and get a pointer to the current active window so we can get the search type"""
        global app
        activewidg = app.activeWindow()
        if activewidg is None: return
        for w in self.viewdlglist:
            if isAlive(w):
                if w.parentWidget() == activewidg:
                    return ('s', w)
        jorvwin = self.mdiArea.currentSubWindow()
        if not jorvwin: return None
        mod = jorvwin.widget().model()
        if isinstance(mod, jobmodel.JobModel):
            return ('j', mod)
        else:
            return ('v', mod)

    def exec_viewsearch(self, vieww, forcedir=False, isbackw=False):
        """Search for string in view dialog"""
        dlg = self.viewsearchdlg
        txtbox = vieww.jobtext
        backsearch = False
        if forcedir:
            backsearch = isbackw
        else:
            backsearch = dlg.sbackward.isChecked()
        searchfl = 0
        if backsearch: searchfl = QTextDocument.FindBackward
        if not dlg.igncase.isChecked(): searchfl |= QTextDocument.FindCaseSensitively
        searchfl = QTextDocument.FindFlags(searchfl)
        if txtbox.find(dlg.searchstring.text(), searchfl):
            txtbox.ensureCursorVisible()
            return
        if dlg.wrapround.isChecked():
            movefl = QTextCursor.Start
            if backsearch: movefl = QTextCursor.End
            txtbox.moveCursor(movefl)
            if txtbox.find(dlg.searchstring.text(), searchfl):
                txtbox.ensureCursorVisible()
                return
        QMessageBox.warning(self, "Search string not found", "Looking for " + dlg.searchstring.text())

    def on_action_Search_for_triggered(self, checked = None):
        if checked is None: return
        act = self.find_active_window()
        if act is None: return
        which, mod = act
        if which == 's':
            if self.viewsearchdlg is None:
                self.viewsearchdlg = Cjssearchdlg()
            if not self.viewsearchdlg.exec_(): return
            self.exec_viewsearch(mod)
        else:
            dlg = self.jvsearchdlg
            if dlg is None:                dlg = self.jvsearchdlg = Cjvsearchdlg()
            if which == 'j':
                dlg.suser.setEnabled(True)
                dlg.stitle.setEnabled(True)
            else:
                dlg.suser.setEnabled(False)
                dlg.stitle.setEnabled(False)
            if not dlg.exec_(): return
            if not mod.exec_jvsearch(dlg):
                QMessageBox.warning(self, "Search string not found", "Looking for " + dlg.searchstring.text())

    def on_action_Search_forwards_triggered(self, checked = None):
        if checked is None: return
        act = self.find_active_window()
        if act is None: return
        which, mod = act
        if which == 's':
            if self.viewsearchdlg is None:
                QMessageBox.warning(self, "No previous search", "No previous search job view request")
                return
            self.exec_viewsearch(mod, True, False)
        else:
            if self.jvsearchdlg is None:
                QMessageBox.warning(self, "No previous search", "No previous search job or var request")
                return
            if not mod.exec_jvsearch(self.jvsearchdlg, True, False):
                QMessageBox.warning(self, "Search string not found", "Looking for " + self.jvsearchdlg.searchstring.text())

    def on_action_Search_Backwards_triggered(self, checked = None):
        if checked is None: return
        act = self.find_active_window()
        if act is None: return
        which, mod = act
        if which == 's':
            if self.viewsearchdlg is None:
                QMessageBox.warning(self, "No previous search", "No previous search job view request")
                return
            self.exec_viewsearch(mod, True, True)
        else:
            if self.jvsearchdlg is None:
                QMessageBox.warning(self, "No previous search", "No previous search job or var request")
                return
            if not mod.exec_jpsearch(self.jvsearchdlg, True, True):
                QMessageBox.warning(self, "Search string not found", "Looking for " + self.jvsearchdlg.searchstring.text())

    def on_action_About_BTQW_triggered(self, checked = None):
        if checked is None: return
        QMessageBox.about(self, "About BTQW",
                                """<b>Btqw</b> v %s
                                <p>Copyright &copy; %d Free Software Foundation
                                <p>Python %s - Qt %s""" % (version.VERSION, time.localtime().tm_year, platform.python_version(), QT_VERSION_STR))

# We now save the files in the User's directory for Windows
# or $HOME/.gbatch/ for UNIX
# We also grab the Windows user or the logged-in user if on UNIX/Linux

if btqwopts.WIN:
    import win32api
    floc = os.path.expanduser('~\\btqw.xbs')
    winuser = win32api.GetUserName()
else:
    import pwd
    # I think we want the home directory of the su-ed to user so we don't
    # end up saving a config file unwriteable by the user
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
mw = BtqwMainwin()

# Load options

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

if len(btqwopts.Options.winlist) == 0:
    btqwopts.Options.winlist = savewdets.init_winlist(jobmodel, varmodel)

mw.init_subwins()
if btqwopts.Options.geom.isValid():
    mw.setGeometry(btqwopts.Options.geom)
mw.show()

gbclient.setup_despatch(mw)
btqwopts.Options.servers.login_all(mw)
for h in btqwopts.Options.servers.list_servers():
    if h.isok:
        mw.init_client(h)
mw.connect_clients()
os._exit(app.exec_())
