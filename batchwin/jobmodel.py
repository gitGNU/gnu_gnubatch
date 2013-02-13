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
import string

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

Linecount = 0

def fmt_seq(jb, m):
    """Sequence"""
    return str(Linecount)

def fmt_jobno(jb, m):
    """Display the formatted job number"""
    return "%s:%d" % (btqwopts.Options.servers.look_hostid(jb.ipaddr()), jb.bj_job)

def fmt_orighost(jb, m):
    """Originating host"""
    return btqwopts.Options.servers.look_hostid(jb.bj_orighostid)

def ca_varname(ind):
    """Return hostid:variable name string"""
    return btqwopts.Options.servers.look_hostid(ind.ipaddr()) + ':' + globlists.Var_list[ind].var_name

def fmt_condfull(jb, m):
    """Conditions in full"""
    jb.bj_mode.check_readable(jb)
    res = []
    for c in jb.bj_conds:
        try:
            vn = ca_varname(c.bjc_varind)
            op = ('=', '!=', '<', '<=', '>', '>=')[c.bjc_compar-1]
            val = str(c.bjc_value)
            r = vn + op + val
        except (KeyError, IndexError):
            r = '?'
        res.append(r)
    return string.join(res,',')

def fmt_cond(jb, m):
    """Conditions abbreviated"""
    jb.bj_mode.check_readable(jb)
    res = []
    for c in jb.bj_conds:
        try:
            vn = ca_varname(c.bjc_varind)
        except (KeyError, IndexError):
            vn = '?'
        res.append(vn)
    return string.join(res,',')

def fmt_assfull(jb, m):
    """Assignments in full"""
    jb.bj_mode.check_readable(jb)
    res = []
    for a in jb.bj_asses:
        try:
            f = a.bja_flags
            fr = ""
            if (f & btclasses.jass.BJA_START) != 0:     fr += 'S'
            if (f & btclasses.jass.BJA_REVERSE) != 0:   fr += 'R'
            if (f & btclasses.jass.BJA_OK) != 0:        fr += 'N'
            if (f & btclasses.jass.BJA_ERROR) != 0:     fr += 'E'
            if (f & btclasses.jass.BJA_ABORT) != 0:     fr += 'A'
            if (f & btclasses.jass.BJA_CANCEL) != 0:    fr += 'C'
            if len(fr) != 0: fr += ':'
            vn = ca_varname(a.bja_varind)
            op = ('=', '+=', '-=', '*=','/=','%=','=E','=S')[a.bja_op-1]
            if op < btclasses.jass.BJA_SEXIT:
                op += str(a.bja_con)
            r = fr + vn + op
        except (KeyError, IndexError):
            r = '?'
        res.append(r)
    return string.join(res,',')

def fmt_ass(jb, m):
    """Assignments abbreviated"""
    jb.bj_mode.check_readable(jb)
    res = []
    for a in jb.bj_asses:
        try:
            vn = ca_varname(a.bja_varind)
        except (KeyError, IndexError):
            vn = '?'
        res.append(vn)
    return string.join(res,',')

def fmt_interp(jb, m):
    """Command interpreter"""
    jb.bj_mode.check_readable(jb)
    return jb.bj_cmdinterp

def fmt_timefull(jb, m):
    """Time in full"""
    jb.bj_mode.check_readable(jb)
    if jb.bj_times.tc_istime:
        return time.strftime('%x %H:%M', localtime(jb.bj_times.tc_nexttime))
    return ""

def abbrevtime(t):
    """Abbreviated time as hh:mm if in next 24 hours otherwise mm/dd or dd/mm"""
    now = time.time()
    if now <= t <= now + 86400:
        return time.strftime("%H:%M", time.localtime(t))
    return time.strftime("%x", time.localtime(t))[0:5]

def fmt_time(jb, m):
    """Time as hh:mm if in next 24 hours otherwise dd/mm according to locale"""
    jb.bj_mode.check_readable(jb)
    if jb.bj_times.tc_istime:
        return abbrevtime(jb.bj_times.tc_nexttime)
    return ""

def fmt_otime(jb, m):
    """Submit time"""
    jb.bj_mode.check_readable(jb)
    return abbrevtime(jb.bj_time)

def fmt_stime(jb, m):
    """Start time"""
    jb.bj_mode.check_readable(jb)
    if jb.bj_stime == 0:
        return ""
    return abbrevtime(jb.bj_stime)

def fmt_etime(jb, m):
    """End time"""
    jb.bj_mode.check_readable(jb)
    if jb.bj_etime == 0:
        return ""
    return abbrevtime(jb.bj_etime)

def fmt_itime(jb, m):
    """Most relevant time"""
    if btclasses.btjob.BJP_DONE <= jb.bj_progress <= btclasses.btjob.BJP_ABORTED or jb.bj_progress == btclasses.btjob.BJP_FINISHED:
        if jb.bj_etime != 0:
            return fmt_etime(jb)
        return fmt_time(jb)
    if btclasses.btjob.BJP_STARTUP1 <= jb.bj_progress <= btclasses.btjob.BJP_RUNNING:
        return fmt_stime(jb)
    if btclasses.btjob.BJP_CANCELLED:
        return fmt_time(jb)
    if not jb.bj_times.tc_istime or jb.bj_times.tc_nexttime < time.time():
        return fmt_etime(jb)
    return fmt_time(jb)

def fmt_repeat(jb, m):
    """Display repeat option"""
    jb.bj_mode.check_readable(jb)
    if not jb.bj_times.tc_istime: return ""
    ro = jb.bj_times.tc_repeat
    try:
        rm = ('Del', 'Ret', 'Mins', 'Hours', 'Days', 'Weeks', 'Monthsb', 'Monthse', 'Years')[ro]
    except IndexError:
        return ""
    if ro >= timecon.timecon.TC_MINUTES:
        rm += ':' + str(jb.bj_times.tc_rate)
        if ro == timecon.timecon.TC_MONTHSB or ro == timecon.timecon.TC_MONTHSE:
            rm += ':' + str(jb.bj_times.tc_mday)
    return rm

def fmt_avoid(jb, m):
    """Display days to avoid"""
    jb.bj_mode.check_readable(jb)
    if not jb.bj_times.tc_istime: return ""
    avd = jb.bj_times.tc_nvaldays
    ret = []
    for d in range(0, 8):
        if (avd &  (1 << d)) != 0:
            ret.append(('Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat')[d])
    return string.join(ret, ',')

def fmt_title(jb, m):
    """Title"""
    jb.bj_mode.check_readable(jb)
    tit = jb.bj_title
    if m.dispoptions.isqlim():
        tit = tit.split(':', 1)[-1]
    return tit

def fmt_qtit(jb, m):
    """Title removing any queue prefix"""
    jb.bj_mode.check_readable(jb)
    return jb.bj_title.split(':', 1)[-1]

def fmt_queue(jb, m):
    """Report queue name"""
    jb.bj_mode.check_readable(jb)
    qt = jb.bj_title.split(':', 1)
    if len(qt) <= 1:
        return ""
    return qt[0]

def fmt_args(jb, m):
    """Report arguments"""
    jb.bj_mode.check_readable(jb)
    return string.join(jb.bj_arg, ',')

def fmt_dir(jb, m):
    """Report directory"""
    jb.bj_mode.check_readable(jb)
    return jb.bj_direct

def fmt_env(jb, m):
    """Report environment"""
    jb.bj_mode.check_readable(jb)
    return string.join(map(lambda e:e.e_name+'='+e.e_value,jb.bj_env), ',')

def fmt_redirs(jb, m):
    """Report redirections - just put filenames or fds in for now?"""
    jb.bj_mode.check_readable(jb)
    ret = []
    for r in jb.bj_redirs:
        rr = str(r.fd) + ':'
        if r.action < btclasses.redir.RD_ACT_CLOSE:
            rr += r.filename
        elif r.action == btclasses.redir.RD_ACT_CLOSE:
            rr += '&-'
        else:
            rr += '&' + str(r.fd2)
        ret.append(rr)
    return string.join(ret, ',')

def fmt_user(jb, m):
    """Report owner of job"""
    return jb.bj_mode.o_user

def fmt_group(jb, m):
    """Report group owner of job"""
    return jb.bj_mode.o_group

def fmt_mode(jb, m):
    """Report job modes (permissions)"""
    jb.bj_mode.check_accessible(jb, btmode.btmode.BTM_RDMODE)
    return jb.bj_mode.mode_disp()

def fmt_prio(jb, m):
    """Priority"""
    jb.bj_mode.check_readable(jb)
    return str(jb.bj_pri)

def fmt_loadlev(jb, m):
    """Load level"""
    jb.bj_mode.check_readable(jb)
    return str(jb.bj_ll)

def fmt_exits(jb, m):
    """Exit codes"""
    jb.bj_mode.check_readable(jb)
    return "N(%d,%d),E(%d,%d)" % (jb.exitn.lower,jb.exitn.upper,jb.exite.lower,jb.exite.upper)

def fmt_export(jb, m):
    """Export markers"""
    jb.bj_mode.check_readable(jb)
    expm = "Local"
    if (jb.bj_jflags & btjob.BJ_REMRUNNABLE) != 0:
        expm = "Remrun"
    elif (jb.bj_jflags & btjob.BJ_EXPORT) != 0:
        expm = "Export"
    return expm

def fmt_deltime(jb, m):
    """Delete time"""
    jb.bj_mode.check_readable(jb)
    return str(jb.bj_deltime)

def fmt_runtime(jb, m):
    """Run time"""
    jb.bj_mode.check_readable(jb)
    mins, secs = divmod(jb.bj_runtime, 60)
    hrs, mins = divmod(mins, 60)
    if hrs > 0:
        return "%d:%.2d:%.2d" % (hrs, mins, secs)
    elif mins > 0:
        return "%.2d:%.2d" % (mins, secs)
    return ".2d" % secs

def fmt_autoksig(jb, m):
    """Auto-kill signal"""
    jb.bj_mode.check_readable(jb)
    return jb.bj_autoksig

def fmt_gracetime(jb, m):
    """Grace time"""
    jb.bj_mode.check_readable(jb)
    mins, secs = divmod(jb.bj_runon, 60)
    if mins > 0:
        return "%.2d:%.2d" % (mins, secs)
    return ".2d" % secs

def fmt_umask(jb, m):
    """Umask"""
    jb.bj_mode.check_readable(jb)
    return "%.4o" % jb.bj_umask

def fmt_ulimit(jb, m):
    """Ulimit"""
    jb.bj_mode.check_readable(jb)
    return "%.x" % jb.bj_ulimit

def fmt_progress(jb, m):
    """Progress code"""
    codenames = ('', 'Done', 'Err', 'Abrt', 'Canc', 'Init', 'Start', 'Run', 'Fin')
    try:
        return codenames[jb.bj_progress]
    except IndexError:
        return '????'

def fmt_pid(jb, m):
    """Process id"""
    jb.bj_mode.check_readable(jb)
    if jb.bj_progress == btclasses.btjob.BJP_RUNNING:
        return str(jb.bj_pid)
    return ""

def fmt_xit(jb, m):
    """Exit code last run"""
    jb.bj_mode.check_readable(jb)
    return "%d" % jb.bj_lastexit >> 8

def fmt_sig(jb, m):
    """Signal"""
    jb.bj_mode.check_readable(jb)
    return "%d" % jb.bj_lastexit & 255

# Possible formats for bits of jobs
# Each item gives:
# 1. Format description
# 2. Format header
# 3. Function
# 4. Alignment stuff
# 5. Initial field width (currently wants doing properly)

Formats = (
    ('Sequence number in queue',        'Seq',  fmt_seq,        int(Qt.AlignLeft|Qt.AlignVCenter),  60),  #0
    ('Job number',                      'Jnum', fmt_jobno,      int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #1
    ('Originating host',                'Ohost',fmt_orighost,   int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #2
    ('Conditions for job in full',      'Conds',fmt_condfull,   int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #3
    ('Conditions for job abbreviated',  'Conds',fmt_cond,       int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #4
    ('Assignments in full',             'Ass',  fmt_assfull,    int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #5
    ('Assignments abbreviated',         'Ass',  fmt_ass,        int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #6
    ('Command interpreter',             'CmdI', fmt_interp,     int(Qt.AlignLeft|Qt.AlignVCenter),  60),  #7
    ('Next time in full',               'Time', fmt_timefull,   int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #8
    ('Next time (abbrev)',              'Time', fmt_time,       int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #9
    ('Submit time',                     'Otime',fmt_otime,      int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #10
    ('Start time',                      'Stime',fmt_stime,      int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #11
    ('End time',                        'Etime',fmt_etime,      int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #12
    ('Most relevant time',              'Itime',fmt_itime,      int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #13
    ('Repeat option',                   'Rep',  fmt_repeat,     int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #14
    ('Days to avoid',                   'Avoid',fmt_avoid,      int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #15
    ('Job title',                       'Title',fmt_title,      int(Qt.AlignLeft|Qt.AlignVCenter), 150),  #16
    ('Job title (no queue name)',       'Title',fmt_qtit,       int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #17
    ('Queue name',                      'Queue',fmt_queue,      int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #18
    ('Arguments for job',               'Args', fmt_args,       int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #19
    ('Working directory for job',       'Dir',  fmt_dir,        int(Qt.AlignLeft|Qt.AlignVCenter), 100),  #20
    ('Environment variables for job',   'Envir',fmt_env,        int(Qt.AlignLeft|Qt.AlignVCenter), 150),  #21
    ('Redirections',                    'Redir',fmt_redirs,     int(Qt.AlignLeft|Qt.AlignVCenter), 140),  #22
    ('Job owner',                       'User', fmt_user,       int(Qt.AlignLeft|Qt.AlignVCenter),  70),  #23
    ('Group owner of job',              'Group',fmt_group,      int(Qt.AlignLeft|Qt.AlignVCenter),  70),  #24
    ('Modes (permissions)',             'Mode', fmt_mode,       int(Qt.AlignLeft|Qt.AlignVCenter), 120),  #25
    ('Priority',                        'Pri',  fmt_prio,       int(Qt.AlignRight|Qt.AlignVCenter), 60),  #26
    ('Load Level',                      'Load', fmt_loadlev,    int(Qt.AlignRight|Qt.AlignVCenter), 70),  #27
    ('Exit code ranges',                'Exits',fmt_exits,      int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #28
    ('Export',                          'Exp',  fmt_export,     int(Qt.AlignLeft|Qt.AlignVCenter),  70),  #29
    ('Delete time',                     'Delt', fmt_deltime,    int(Qt.AlignRight|Qt.AlignVCenter), 90),  #30
    ('Run time',                        'Runt', fmt_runtime,    int(Qt.AlignRight|Qt.AlignVCenter), 90),  #31
    ('Auto-kill signal',                'Ksig', fmt_autoksig,   int(Qt.AlignLeft|Qt.AlignVCenter),  60),  #32
    ('Grace time',                      'Grace',fmt_gracetime,  int(Qt.AlignRight|Qt.AlignVCenter), 70),  #33
    ('Umask',                           'Umsk', fmt_umask,      int(Qt.AlignLeft|Qt.AlignVCenter),  60),  #34
    ('Ulimit',                          'Ulim', fmt_ulimit,     int(Qt.AlignRight|Qt.AlignVCenter), 80),  #35
    ('Progress',                        'Prog', fmt_progress,   int(Qt.AlignLeft|Qt.AlignVCenter),  90),  #36
    ('Process id of running job',       'Pid',  fmt_pid,        int(Qt.AlignRight|Qt.AlignVCenter), 80),  #37
    ('Exit code last run',              'Exit', fmt_xit,        int(Qt.AlignRight|Qt.AlignVCenter), 60),  #38
    ('Signal last run',                 'Sig',  fmt_sig,        int(Qt.AlignLeft|Qt.AlignVCenter),  60))  #39

Default_fieldnums = [ 1, 23, 16, 7, 26, 27, 9, 4, 36 ]
Default_size = QRect(10, 10, 350, 250)

class JobModel(QAbstractTableModel):
    """Model for holding job info"""

    def __init__(self):
        super(JobModel, self).__init__()
        self.isjob = True
        self.theView = None
        self.jobs = []
        self.jobrows = dict()
        self.fieldlist = Default_fieldnums
        self.geom = Default_size
        self.dispoptions = copy.deepcopy(btqwopts.Options.defwopts)

    def getselectedjobrow(self):
        """Return row number of selected job"""
        try:
            inds = self.theView.selectedIndexes()
            return inds[0].row()
        except (AttributeError, IndexError):
            return -1

    def getselectedjob(self):
        """Return selected job"""
        row = self.getselectedjobrow()
        if row < 0:
            return None
        return self.jobs[row]

    def renumrows(self):
        """Reset row lookup"""
        rownum = 0
        self.jobrows = dict()
        for j in self.jobs:
            self.jobrows[j] = rownum
            rownum += 1

    def sortjobs(self):
        """Sort the jobs into descending order of priority

Assign row numbers to job rows with lookup by id"""

        # Remember what we had selected so we can reselect it later

        jid = self.getselectedjob()

        self.jobs.sort(key=attrgetter('queuetime'))
        self.jobs.sort(key=attrgetter('bj_pri'), reverse=True)

        self.renumrows()
        self.reset()

        if jid:
            try:
                self.theView.selectRow(self.jobrows[jid])
            except IndexError:
                pass

    def replacerows(self):
        """Replace all the rows after we've changed the display options"""
        nrows = self.rowCount()
        if nrows > 0:
            self.beginRemoveRows(QModelIndex(), 0, nrows-1)
            self.endRemoveRows()
        self.jobs = []
        for j in globlists.Job_list.values():
            if self.dispoptions.okdisp_job(j):
                self.jobs.append(j)
                self.insertRows(0)
        self.sortjobs()

    def rowchanged(self, row):
        """Notify that a row has changed"""
        indb = self.createIndex(row, 0, QModelIndex())
        inde = self.createIndex(row, len(self.fieldlist)-1, QModelIndex())
        self.dataChanged.emit(indb, inde)

    def rowCount(self, index=QModelIndex()):
        return len(self.jobs)

    def columnCount(self, index=QModelIndex()):
        return len(self.fieldlist)

    def insertRows(self, position):
        self.beginInsertRows(QModelIndex(), position, position)
        self.endInsertRows()

    def removeRows(self, position):
        self.beginRemoveRows(QModelIndex(), position, position)
        self.endRemoveRows()

    def data(self, index, role=Qt.DisplayRole):
        global Linecount
        if not index.isValid() or not (0 <= index.row() < len(self.jobs)):
            return QVariant()
        job = self.jobs[index.row()]
        column = index.column()
        if role == Qt.TextAlignmentRole:
            return QVariant(Formats[self.fieldlist[column]][3])
        if role == Qt.DisplayRole:
            Linecount = index.row()
            try:
                return QVariant(Formats[self.fieldlist[column]][2](job, self))
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
        if not self.dispoptions.okdisp_job(job): return
        self.jobs.append(job)
        self.insertRows(0)              # Doesn't matter where we are redrawing
        self.sortjobs()

    def deljob(self, job):
        """Delete job from model"""
        try:
            row = self.jobrows[job]
            self.removeRows(row)
            del self.jobs[row]
            self.renumrows()
        except KeyError:
            # Ignore error if we didn't find job
            return

    def addvar(self, var):
        """Add variable to model"""
        pass

    def delvar(self, var):
        """Delete variable from model"""
        pass

    def updjob(self, job, origpri):
        """Update existing job"""

        # We might have previously decided not to display this job
        # in which case it won't know about it and KeyError gets raised
        # Or it might have changed so we don't want to see it any more

        try:
            row = self.jobrows[job]
            if not self.dispoptions.okdisp_job(job):
                self.deljob(job)
                return
            self.jobs[row] = job
            # If priority hasn't changed, then just update that row,
            # otherwise we need to sort it again
            if origpri == job.bj_pri:
                self.rowchanged(row)
            else:
                self.sortjobs()
        except KeyError:
            self.addjob(job)

    def boqjob(self, job):
        """Reposition job at bottom of queue"""
        self.sortjobs()

    def assignedvar(self, var):
        """Updates to variable"""
        pass

    def renamedvar(self, var):
        """Rename variable"""
        pass

    def findmatch(self, dlg, row):
        """Find string as per dlg in given row and select it"""
        sstring = dlg.searchstring.text()
        jb = self.jobs[row]
        if (dlg.suser.isChecked() and \
            QString(jb.bj_mode.o_user).indexOf(sstring, 0, Qt.CaseInsensitive) >= 0) or \
            (dlg.stitle.isChecked() and \
            QString(jb.bj_title).indexOf(sstring, 0, Qt.CaseInsensitive) >= 0):
            self.theView.selectRow(row)
            return True
        return False

    def exec_jvsearch(self, dlg, forcedir = False, isbackw = False):
        """Execute search function for jobs matching string"""
        if forcedir:
            backsearch = isbackw
        else:
            backsearch = dlg.sbackward.isChecked()
        row = self.getselectedjobrow()
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